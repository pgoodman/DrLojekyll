// Copyright 2026, Peter Goodman. All rights reserved.
// Copyright 2019, Trail of Bits. All rights reserved.

#include <drlojekyll/Parse/ErrorLog.h>

#include "Query.h"

namespace hyde {
namespace {

VIEW *CreateProxyOfInserts(QueryImpl *impl, UseList<QueryViewImpl> &inserts) {

  UseList<QueryViewImpl> old_inserts(inserts.Owner());
  old_inserts.Swap(inserts);

  MERGE *merge = nullptr;

  // Create a MERGE that takes in TUPLEs that replace each INSERT, except
  // for the INSERTs representing DELETEs. EVERY relation gets a MERGE, even
  // single-clause ones, and that shape is LOAD-BEARING: the demand pass
  // (Demand.cpp) unconditionally resolves a relation's post-Connect proxy
  // via AsMerge() in all 4 modes. (F32-adjacent cleanup, 2026-08-05: a dead
  // `has_one_insert` bare-TUPLE early return — it read the swapped-empty
  // list, so it never fired — was deleted rather than "fixed"; activating
  // it would break the demand pass and drift the nodf/none IR goldens
  // corpus-wide. K5-D8 RIDER-1 records the history.)
  for (VIEW *insert : old_inserts) {
    assert(insert->AsInsert());

    // A witness-bearing INSERT reads more columns than it stores; inlining
    // it into its readers through a stored-columns-only proxy would sever
    // its read edge to the incoming view, so such INSERTs are never inlined
    // here.
    assert(insert->attached_columns.Empty());

    // Only proxy an INSERT if it actually inserts data; otherwise it's a
    // DELETE and we want to maintain that.
    TUPLE *const proxy = Mint(impl->tuples, "connect/insert-proxy");

#ifndef NDEBUG
    proxy->producer = "INSERT";
#endif

    insert->CopyDifferentialAndGroupIdsTo(proxy);

    auto col_index = 0u;
    for (auto in_col : insert->input_columns) {
      COL *const proxy_col = Mint(proxy->columns, "connect/insert-proxy",
          in_col->var, in_col->type, proxy, in_col->id, col_index++);
      proxy->input_columns.AddUse(in_col);
      proxy_col->CopyConstantFrom(in_col);
    }

    insert->PrepareToDelete();

    if (!merge) {
      merge = Mint(impl->merges, "connect/insert-union");
#ifndef NDEBUG
      merge->producer = "MERGE-INSERT";
#endif
      col_index = 0u;
      for (auto col : proxy->columns) {
        (void) Mint(merge->columns, "connect/insert-union", col->var, col->type, merge, col->id,
                                     col_index++);
      }
    }

    merge->merged_views.AddUse(proxy);
  }

  assert(merge != nullptr);
  return merge;
}

static VIEW *CreateProxyForMutableParams(QueryImpl *impl, VIEW *view,
                                         ParsedDeclaration decl) {

  assert(!view->columns.Empty());

  // If the decl has at least one `mutable`-attributed parameter then we
  // need a KVINDEX.
  if (!decl.HasMutableParameter()) {
    return view;
  }

  KVINDEX *const index = Mint(impl->kv_indices, "connect/kv-index");
  std::unordered_map<COL *, COL *> col_map;

  // Create the key columns.
  auto i = 0u;
  auto col_index = 0u;
  for (ParsedParameter param : decl.Parameters()) {
    const auto view_col = view->columns[i++];
    if (param.Binding() != ParameterBinding::kMutable) {
      const auto key_col = Mint(index->columns, "connect/kv-index",
          view_col->var, view_col->type, index, view_col->id, col_index++);
      col_map.emplace(view_col, key_col);

      index->input_columns.AddUse(view_col);
    }
  }

  // Create the value columns.
  i = 0u;
  for (ParsedParameter param : decl.Parameters()) {
    const auto view_col = view->columns[i++];
    if (param.Binding() == ParameterBinding::kMutable) {
      const auto val_col = Mint(index->columns, "connect/kv-index",
          view_col->var, view_col->type, index, view_col->id, col_index++);
      col_map.emplace(view_col, val_col);

      index->merge_functors.push_back(ParsedFunctor::MergeOperatorOf(param));
      index->attached_columns.AddUse(view_col);
    }
  }

  // We need to return the columns in the expected order.
  TUPLE *const proxy = Mint(impl->tuples, "connect/kv-reorder");
  col_index = 0u;
  for (auto col : view->columns) {
    (void) Mint(proxy->columns, "connect/kv-reorder", col->var, col->type, proxy, col->id,
                                 col_index++);
    proxy->input_columns.AddUse(col_map[col]);
  }

  return proxy;
}

static void ProxySelects(QueryImpl *impl, UseList<QueryViewImpl> &selects,
                         VIEW *insert_proxy) {
  UseList<QueryViewImpl> old_selects(selects.Owner());
  old_selects.Swap(selects);

  for (VIEW *select : old_selects) {
    assert(select->AsSelect());

    // Only proxy an INSERT if it actually inserts data; otherwise it's a
    // DELETE and we want to maintain that.
    TUPLE *const proxy = Mint(impl->tuples, "connect/select-proxy");

#ifndef NDEBUG
    proxy->producer = "SELECT";
#endif

    select->CopyDifferentialAndGroupIdsTo(proxy);

    auto col_index = 0u;
    for (auto in_col : insert_proxy->columns) {
      COL *const sel_col = select->columns[col_index];
      COL *const proxy_col = Mint(proxy->columns, "connect/select-proxy",
          sel_col->var, sel_col->type, proxy, sel_col->id, col_index++);
      proxy->input_columns.AddUse(in_col);
      proxy_col->CopyConstantFrom(in_col);
    }

    select->ReplaceAllUsesWith(proxy);
  }
}

}  // namespace

// Connect INSERT nodes to SELECT nodes when the "full state" of the relation
// does not need to be visible for point queries.
bool QueryImpl::ConnectInsertsToSelects(
    const ErrorLog &log,
    std::unordered_map<VIEW *, ParsedDeclaration> &proxy_view_to_decl) {

  // First, deal with all messages.
  for (IO *io : ios) {

    io->transmits.Unique();
    io->receives.Unique();

    // Messages should only ever be sent or received, but not both.
    if (!io->transmits.Empty() && !io->receives.Empty()) {
      log.Append(io->declaration.SpellingRange())
          << "Internal error: cannot have both sends and receives on the "
          << "message '" << io->declaration.Name() << '/'
          << io->declaration.Arity() << "'";
      return false;
    }

    assert(!io->declaration.HasMutableParameter());

    if (auto num_transmits = io->transmits.Size(); num_transmits) {

      if (1u == num_transmits) {
        continue;
      }

      // If a message has more than one transmit, then we want to merge all
      // of those transmits via a single UNION.
      VIEW *const proxy = CreateProxyOfInserts(this, io->transmits);

      INSERT *insert = Mint(inserts, "connect/transmit-insert", io, io->declaration);
      for (auto col : proxy->columns) {
        insert->input_columns.AddUse(col);
      }

      io->transmits.AddUse(insert);

    } else if (auto num_receives = io->receives.Size(); num_receives) {

      if (1u == num_receives) {
        continue;
      }

      SELECT *const prev_sel = io->receives[0]->AsSelect();

      SELECT *select = nullptr;
      if (prev_sel->pred) {
        select = Mint(selects, "connect/receive-select", io, *(prev_sel->pred));
      } else {
        select = Mint(selects, "connect/receive-select", io, DisplayRange(prev_sel->position, {}));
      }

      auto col_index = 0u;
      for (auto col : io->receives[0]->columns) {
        (void) Mint(select->columns, "connect/receive-select", col->var, col->type, select, col->id,
                                      col_index++);
      }

      ProxySelects(this, io->receives, select);
      assert(io->receives.Empty());
      io->receives.AddUse(select);

    } else {
      assert(false);
    }
  }

  // Then, deal with all relations (queries, locals, exports).
  for (REL *rel : relations) {

    rel->inserts.Unique();
    rel->selects.Unique();

    const ParsedDeclaration decl(rel->declaration);

    // A unit (condition) relation keeps its materialized INSERT -> RELATION
    // -> SELECT structure: its setter INSERTs store only the `true` token
    // while carrying witness reads of differing arities, so they can never
    // be inlined into their readers. Wire each SELECT to the relation's
    // INSERTs so that dead-flow tainting and view linking see the edges.
    if (!decl.Arity()) {
      assert(rel->is_condition);
      for (VIEW *sel_view : rel->selects) {
        QuerySelectImpl *const sel = sel_view->AsSelect();
        assert(sel != nullptr);
        for (VIEW *insert_view : rel->inserts) {
          sel->inserts.AddUse(insert_view);
        }
      }
      continue;
    }

    if (rel->inserts.Empty()) {
      log.Append(rel->declaration.SpellingRange())
          << "Declaration of " << rel->declaration.KindName()
          << " is missing a definition";
      continue;
    }

    VIEW *const insert_proxy = CreateProxyForMutableParams(
        this, CreateProxyOfInserts(this, rel->inserts), rel->declaration);

    // Tier-1 naming-lift snapshot: `rel->declaration` is discarded right
    // below (`rel->inserts.Clear()` severs the only REL->proxy edge), so the
    // proxy->decl correlation is recorded HERE or nowhere. Single writer;
    // the single reader is `ApplyDemandTransform` (pre-`Optimize`).
    proxy_view_to_decl.emplace(insert_proxy, rel->declaration);

    // K5 seed (Tier-2 origin provenance): the decl SOURCE for the per-view
    // origin set. `insert_proxy` is the top of the relation's definition (a
    // MERGE, or a TUPLE-over-KVINDEX for the mutable arm); every downstream
    // CDaGI fold carries the seed forward (View.cpp), so this is the ONLY seed
    // site. Node-kind-agnostic (K5-Q6 Arm B: no reliance on the always-MERGE
    // `has_one_insert` quirk). Skip @inline (non-materialized -> never a
    // row-contract) AND queries (already R-STORE-named via CollectContractInserts
    // at freeze); `!IsQuery()` is redundant with `!IsInline()` at tip
    // (IsInline() subsumes IsQuery(), Parse.cpp) but written explicitly as
    // forward-looking hygiene (K5P-corr-2). Condition/unit relations
    // (Arity()==0) already `continue` above before reaching here.
    if (!decl.IsInline() && !decl.IsQuery()) {
      assert(insert_proxy->origin_decls.empty());  // TIGERSTYLE: seed-once.
      insert_proxy->origin_decls.assign(1u, rel->declaration);
    }

    rel->inserts.Clear();

    // If there are no SELECTs on this declaration, then any INSERTs are
    // ineffectual. It's possible that those INSERTs are conditional, though,
    // and `proxy` will deal with those conditions being linked. Thus, in this
    // case, we'll just leave `proxy` dangling, to be cleaned up by
    // canonicalization
    if (rel->selects.Empty() && !decl.IsQuery()) {
      continue;
    }

    ProxySelects(this, rel->selects, insert_proxy);
    assert(rel->selects.Empty());

    if (decl.IsQuery()) {
      INSERT *insert = Mint(inserts, "connect/query-insert", rel, rel->declaration);
      for (auto col : insert_proxy->columns) {
        insert->input_columns.AddUse(col);
      }

      rel->inserts.AddUse(insert);
    }
  }

  RemoveUnusedViews();
  TrackDifferentialUpdates(log, true);

  return true;
}

}  // namespace hyde
