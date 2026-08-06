// Copyright 2026, Peter Goodman. All rights reserved.
// Copyright 2020, Trail of Bits. All rights reserved.

#include <unordered_set>

#include "Query.h"

namespace hyde {

static bool IsTrivialCycle(TUPLE *tuple);

// Eliminate dead flows. This is a mark-and-sweep pass over the dataflow
// graph. It uses a taint-based approach and identifies a VIEW as dead if it
// is not derived directly or indirectly from input messages: the taint seeds
// are the RECEIVEs of messages, SELECTs over streams, and constants (an
// all-constant input set is represented by a `nullptr` incoming view, which
// is pre-inserted into the tainted set). Taint then propagates forward to a
// fixpoint, with each node kind imposing its own liveness rule: pass-through
// nodes (TUPLE, INSERT, CMP, MAP, KVINDEX, NEGATE) are tainted when their
// incoming view is tainted; a MERGE is tainted when ANY merged view is live
// and tainted; a JOIN only when ALL joined views are live and tainted; an
// AGG when its aggregated-column source (or failing that, its
// group-by/config source) is tainted, or when all of its inputs are
// constant; and a SELECT over a relation is tainted when any live INSERT
// into that relation is tainted. After the fixpoint, empty-relation folding
// applies: a NEGATE whose negated view is untainted (it can never hold
// data, and the sweep is about to delete it) has a vacuously true absence
// check, so it is replaced by a TUPLE forwarding its input and attached
// columns; the TUPLE inherits the predecessor's taint. The sweep unlinks
// untainted views, prunes untainted predecessors out of surviving MERGEs,
// and deletes TUPLEs that form trivial self-cycles through a MERGE (see
// `IsTrivialCycle`). This is
// beneficial because underivable recursive cycles (e.g. a relation defined
// only in terms of itself) keep themselves alive under pure use-count
// reclamation; taint from the input boundary is what proves such cycles can
// never produce data, so removing them is sound.
//
//    tainted = {message RECEIVEs, stream SELECTs, nullptr /* constants */}
//    repeat until no change:
//      SELECT(rel):  tainted if any live INSERT into rel is tainted
//      TUPLE/INSERT/CMP/MAP/KVINDEX/NEGATE v:
//                    tainted if GetIncomingView(v) in tainted
//      AGG:          tainted if aggregated-source tainted, else if
//                    group/config source tainted, else if all-constant
//      MERGE:        tainted if any live merged view tainted
//      JOIN:         tainted if every joined view is live and tainted
//      INSERT with all-constant inputs: tainted
//    for NEGATE n with untainted negated view:   // empty-relation folding
//      replace n with a TUPLE forwarding n's input+attached columns
//      TUPLE tainted iff n's predecessor tainted
//    for v in views:                       // sweep
//      if v not tainted:        v.PrepareToDelete()
//      else if v is MERGE:      remove untainted merged views
//      else if IsTrivialCycle(v as TUPLE): v.PrepareToDelete()
//    return RemoveUnusedViews()
//
// Before:                                After:
//
//   RECV add_fact    (nothing feeds p)     RECV add_fact
//       |                  |                   |
//     TUPLE            TUPLE p                TUPLE
//        \               /                     |
//        UNION (MERGE) <-'               UNION (MERGE)
//              |                               |
//           INSERT                          INSERT
//
//   The `TUPLE p` arm is not derived from any message, so it is deleted
//   and unlinked from the UNION's list of merged views.
// The shared input-taint fixpoint of the dead-flow family: mark every live
// view (transitively) derivable from the input boundary. The seeds are the
// RECEIVEs of messages, SELECTs over streams, and constants (`nullptr`
// stands for an all-constant input set); the per-node-kind propagation
// rules are documented on `EliminateDeadFlows` above. Callers:
// `EliminateDeadFlows` (the `df.dfe`-gated dead-flow OPTIMIZATION) and
// `CollectDeadCycles` (the REQUIRED hygiene half; FINDINGS.md F26).
void QueryImpl::TaintDerivedFromInput(
    std::unordered_set<void *> &derived_from_input) {

  for (auto io : ios) {
    for (auto view : io->receives) {
      if (!view->is_unsat) {
        derived_from_input.insert(view);  // Inputs come from the outside world.
      }
    }
  }

  for (SELECT *view : selects) {
    if (view->stream && !view->is_unsat) {
      derived_from_input.insert(view);  // Inputs come from the outside world.
    }
  }

  // So that all constant inputs look like a view derived from input.
  derived_from_input.insert(nullptr);

  auto changed = true;

  // Unsatisfiable views never have data, so they are never tainted, and the
  // sweep deletes them along with the views that only they feed.
  auto should_check_view = [&](VIEW *view) {
    return !view->is_dead && !view->is_unsat && !derived_from_input.count(view);
  };

  auto check_incoming_view = [&](VIEW *view, VIEW *incoming_view) {
    if (derived_from_input.count(incoming_view)) {
      changed = true;
      derived_from_input.insert(view);
    }
  };

  while (changed) {
    changed = false;

    for (SELECT *view : selects) {
      if (view->is_unsat || derived_from_input.count(view)) {
        continue;
      }
      for (auto insert : view->inserts) {
        if (insert && !insert->is_dead && derived_from_input.count(insert)) {
          derived_from_input.insert(view);
          changed = true;
          break;
        }
      }
    }

    for (TUPLE *view : tuples) {
      if (should_check_view(view)) {
        check_incoming_view(view, VIEW::GetIncomingView(view->input_columns));
      }
    }

    for (INSERT *view : inserts) {
      if (should_check_view(view)) {
        check_incoming_view(view, VIEW::GetIncomingView(view->input_columns));
      }
    }

    for (CMP *view : compares) {
      if (should_check_view(view)) {
        check_incoming_view(
            view,
            VIEW::GetIncomingView(view->input_columns, view->attached_columns));
      }
    }

    for (MAP *view : maps) {
      if (should_check_view(view)) {
        check_incoming_view(
            view,
            VIEW::GetIncomingView(view->input_columns, view->attached_columns));
      }
    }

    for (KVINDEX *view : kv_indices) {
      if (should_check_view(view)) {
        check_incoming_view(
            view,
            VIEW::GetIncomingView(view->input_columns, view->attached_columns));
      }
    }

    for (AGG *view : aggregates) {
      if (should_check_view(view)) {
        auto iview0 = VIEW::GetIncomingView(view->aggregated_columns);
        auto iview1 =
            VIEW::GetIncomingView(view->group_by_columns, view->config_columns);

        if (iview0) {
          check_incoming_view(view, iview0);

        } else if (iview1) {
          check_incoming_view(view, iview1);

        // All constant inputs...
        } else {
          changed = true;
          derived_from_input.insert(view);
        }
      }
    }

    for (MERGE *view : merges) {
      if (should_check_view(view)) {
        for (auto merged_view : view->merged_views) {
          if (merged_view && !merged_view->is_dead &&
              derived_from_input.count(merged_view)) {
            derived_from_input.insert(view);
            changed = true;
            break;
          }
        }
      }
    }

    // A NEGATE forwards its predecessor's rows (filtered by the absence
    // check), so its liveness follows the predecessor alone: an untainted
    // negated view only makes the check vacuously true (handled by the
    // empty-relation folding after the fixpoint), it never blocks data.
    for (NEGATION *view : negations) {
      if (should_check_view(view)) {
        check_incoming_view(
            view,
            VIEW::GetIncomingView(view->input_columns, view->attached_columns));
      }
    }

    for (JOIN *view : joins) {
      if (should_check_view(view)) {
        auto all_tainted = true;
        // `joined_views` is a WeakUseList: an entry nulls out when its view
        // is reclaimed, exactly like `merged_views` below (F23 hardening —
        // a null entry means an input that can never produce data).
        for (auto joined_view : view->joined_views) {
          if (!joined_view || joined_view->is_dead ||
              !derived_from_input.count(joined_view)) {
            all_tainted = false;
            break;
          }
        }

        if (all_tainted) {
          changed = true;
          derived_from_input.insert(view);
        }
      }
    }

    for (INSERT *view : inserts) {
      if (!view->is_dead && !view->is_unsat &&
          !VIEW::GetIncomingView(view->input_columns)) {
        derived_from_input.insert(view);  // All inputs are constants.
      }
    }
  }
}

bool QueryImpl::EliminateDeadFlows(void) {

  std::unordered_set<void *> derived_from_input;
  std::vector<VIEW *> views;
  ForEachView([&views](VIEW *view) { views.push_back(view); });

  TaintDerivedFromInput(derived_from_input);

  // Empty-relation folding: an untainted negated view can never hold data
  // (the sweep below deletes it), so the absence check of any NEGATE over
  // it is vacuously true and the NEGATE forwards its predecessor's rows
  // unconditionally. Fold each such NEGATE into a TUPLE forwarding its
  // input and attached columns; the TUPLE inherits the predecessor's taint
  // and joins the sweep's worklist, and the now-unused NEGATE is reclaimed
  // by `RemoveUnusedViews`.
  //
  //    PRED           VIEW (untainted)        PRED
  //     |in,att          |                     |in,att
  //     +--> NEGATE <----+          ==>       TUPLE
  //            |                                |
  //          users                            users
  for (NEGATION *negate : negations) {
    if (negate->is_dead || negate->is_unsat ||
        derived_from_input.count(negate->negated_view.get())) {
      continue;
    }

    const auto first_attached_col = negate->input_columns.Size();
    TUPLE *tuple = Mint(this->tuples, "dfe/negate-passthrough");
    auto col_index = 0u;
    for (auto col : negate->columns) {
      Mint(tuple->columns, "dfe/negate-passthrough", col->var, col->type, tuple, col->id, col_index);

      if (col_index < first_attached_col) {
        tuple->input_columns.AddUse(negate->input_columns[col_index]);
      } else {
        tuple->input_columns.AddUse(
            negate->attached_columns[col_index - first_attached_col]);
      }

      ++col_index;
    }

    negate->ReplaceAllUsesWith(tuple);
    views.push_back(tuple);
    if (derived_from_input.count(negate)) {
      derived_from_input.insert(tuple);
    }
  }

  for (auto view : views) {
    if (!derived_from_input.count(view)) {
      view->PrepareToDelete();

    } else if (auto tuple = view->AsTuple(); tuple && IsTrivialCycle(tuple)) {
      view->PrepareToDelete();
    }
  }

  // Deleting an untainted view may leave dead views inside surviving
  // MERGEs, so the merged-view lists are pruned after the whole sweep.
  for (auto view : views) {
    if (auto merge = view->AsMerge(); merge && !merge->is_dead) {
      merge->merged_views.RemoveIf([&](VIEW *merged_view) {
        return merged_view->is_dead || !derived_from_input.count(merged_view);
      });
    }
  }

  return RemoveUnusedViews();
}

// Collect dead cycles: the REQUIRED graph-hygiene half of the dead-flow
// family (FINDINGS.md F26). A source-less forwarding cycle — views mutually
// derivable only from one another, never from a message, stream, or
// constant — denotes an unsatisfiable (empty) relation. The IR is
// well-formed and semantically meaningful (a user may legally write
// `p(A) : p(A).`), but canonicalization DEMOLISHES the structures that keep
// such cycles recognizable downstream (folding one-arm MERGEs, collapsing
// io seams, leaving pure TUPLE self-cycles), and `Stratify`'s V-SCC-SEAM
// validator requires every surviving multi-view SCC to carry an inductive
// MERGE or io seam. So whenever canonicalization runs, the cycles (and the
// views whose data could only come from them) MUST be collected — this
// collection never consults the pass policy, exactly like
// `RemoveUnusedViews`. `EliminateDeadFlows` is a superset (it also removes
// acyclic dead arms), so the `df.dfe` gate picks WHICH of the two runs,
// never whether one runs.
//
// Membership splits the shared input-taint fixpoint by WELL-FOUNDEDNESS: an
// untainted view is well-founded when every live predecessor is tainted or
// well-founded — its emptiness bottoms out at leaves (an ordinary
// empty-relation arm, kept here, removed only by the gated optimization).
// An untainted view that is NOT well-founded sits on, or strictly
// downstream of, a dead cycle and is deleted; deleting the whole
// non-well-founded set at once keeps the survivors closed (no survivor
// reads a deleted view, except MERGEs — pruned — and NEGATEs over a dying
// negated view — folded to pass-through TUPLEs exactly as in
// `EliminateDeadFlows`). Trivial cycles (`IsTrivialCycle`) are deleted
// regardless of taint, as in the full pass.
//
//    tainted = TaintDerivedFromInput()
//    WF: least fixpoint over live untainted views:
//      v in WF  if every live predecessor p of v: p tainted or p in WF
//      (MERGE: every live member; JOIN: every joined view; SELECT: every
//       live INSERT; AGG: both incoming views; a null/constant input is
//       vacuously settled)
//    dying(v) := live(v) and not tainted(v) and not WF(v)
//    fold NEGATEs with dying negated view (keep rule of the full pass)
//    delete dying views; delete trivial-cycle TUPLEs
//    prune dead members out of surviving MERGEs
//    return RemoveUnusedViews()
//
// Before (canonicalized `p(A) : p(A).`         After:
//         beside a live flow):
//
//    RECV in      TUPLE p <--.                RECV in
//       |            |       |                   |
//     TUPLE        TUPLE ----'                 TUPLE
//       |         (dead cycle)                   |
//    INSERT out                               INSERT out
bool QueryImpl::CollectDeadCycles(void) {

  std::unordered_set<void *> derived_from_input;
  TaintDerivedFromInput(derived_from_input);

  std::vector<VIEW *> views;
  ForEachView([&views](VIEW *view) { views.push_back(view); });

  // Well-foundedness fixpoint over the live untainted views.
  std::unordered_set<VIEW *> well_founded;

  auto settled = [&](VIEW *pred) {
    return !pred || pred->is_dead || derived_from_input.count(pred) ||
           well_founded.count(pred);
  };

  auto changed = true;
  auto consider = [&](VIEW *view, bool is_settled) {
    if (is_settled) {
      well_founded.insert(view);
      changed = true;
    }
  };
  auto should_check_view = [&](VIEW *view) {
    return !view->is_dead && !derived_from_input.count(view) &&
           !well_founded.count(view);
  };

  while (changed) {
    changed = false;

    for (SELECT *view : selects) {
      if (!should_check_view(view)) {
        continue;
      }
      auto all_settled = true;
      for (auto insert : view->inserts) {
        if (insert && !insert->is_dead && !settled(insert)) {
          all_settled = false;
          break;
        }
      }
      consider(view, all_settled);
    }

    for (TUPLE *view : tuples) {
      if (should_check_view(view)) {
        consider(view, settled(VIEW::GetIncomingView(view->input_columns)));
      }
    }

    for (INSERT *view : inserts) {
      if (should_check_view(view)) {
        consider(view, settled(VIEW::GetIncomingView(view->input_columns)));
      }
    }

    for (CMP *view : compares) {
      if (should_check_view(view)) {
        consider(view, settled(VIEW::GetIncomingView(view->input_columns,
                                                     view->attached_columns)));
      }
    }

    for (MAP *view : maps) {
      if (should_check_view(view)) {
        consider(view, settled(VIEW::GetIncomingView(view->input_columns,
                                                     view->attached_columns)));
      }
    }

    for (KVINDEX *view : kv_indices) {
      if (should_check_view(view)) {
        consider(view, settled(VIEW::GetIncomingView(view->input_columns,
                                                     view->attached_columns)));
      }
    }

    for (AGG *view : aggregates) {
      if (should_check_view(view)) {
        consider(view,
                 settled(VIEW::GetIncomingView(view->aggregated_columns)) &&
                     settled(VIEW::GetIncomingView(view->group_by_columns,
                                                   view->config_columns)));
      }
    }

    for (MERGE *view : merges) {
      if (!should_check_view(view)) {
        continue;
      }
      auto all_settled = true;
      for (auto merged_view : view->merged_views) {
        if (merged_view && !merged_view->is_dead && !settled(merged_view)) {
          all_settled = false;
          break;
        }
      }
      consider(view, all_settled);
    }

    // A NEGATE's data follows its predecessor alone (the absence check
    // reads the negated view but never supplies rows), matching the taint
    // rule of the full pass; a dying negated view is handled by folding.
    for (NEGATION *view : negations) {
      if (should_check_view(view)) {
        consider(view, settled(VIEW::GetIncomingView(view->input_columns,
                                                     view->attached_columns)));
      }
    }

    for (JOIN *view : joins) {
      if (!should_check_view(view)) {
        continue;
      }
      auto all_settled = true;
      for (auto joined_view : view->joined_views) {
        if (!settled(joined_view)) {
          all_settled = false;
          break;
        }
      }
      consider(view, all_settled);
    }
  }

  auto dying = [&](VIEW *view) {
    return view && !view->is_dead && !derived_from_input.count(view) &&
           !well_founded.count(view);
  };

  // Fold NEGATEs whose negated view dies here (and which survive
  // themselves), mirroring the empty-relation folding of the full pass:
  // the dying view can never hold data, so the absence check is vacuously
  // true and the NEGATE forwards its predecessor's rows unconditionally.
  for (NEGATION *negate : negations) {
    if (negate->is_dead || negate->is_unsat || dying(negate) ||
        !dying(negate->negated_view.get())) {
      continue;
    }

    const auto first_attached_col = negate->input_columns.Size();
    TUPLE *tuple = Mint(this->tuples, "dfe/negate-passthrough");
    auto col_index = 0u;
    for (auto col : negate->columns) {
      Mint(tuple->columns, "dfe/negate-passthrough", col->var, col->type, tuple, col->id, col_index);

      if (col_index < first_attached_col) {
        tuple->input_columns.AddUse(negate->input_columns[col_index]);
      } else {
        tuple->input_columns.AddUse(
            negate->attached_columns[col_index - first_attached_col]);
      }

      ++col_index;
    }

    negate->ReplaceAllUsesWith(tuple);
    views.push_back(tuple);

    // The fresh TUPLE inherits the NEGATE's classification so the sweep
    // below keeps it (the NEGATE survives by construction here).
    if (derived_from_input.count(negate)) {
      derived_from_input.insert(tuple);
    } else {
      well_founded.insert(tuple);
    }
  }

  for (auto view : views) {
    if (view->is_dead) {
      continue;
    }
    if (dying(view)) {
      view->PrepareToDelete();

    } else if (auto tuple = view->AsTuple(); tuple && IsTrivialCycle(tuple)) {
      view->PrepareToDelete();
    }
  }

  // Deleting a dying view may leave dead members inside surviving MERGEs.
  for (auto view : views) {
    if (auto merge = view->AsMerge(); merge && !merge->is_dead) {
      merge->merged_views.RemoveIf([](VIEW *merged_view) {
        return !merged_view || merged_view->is_dead;
      });
    }
  }

  return RemoveUnusedViews();
}

// Eliminate trivial cycles on unions. A TUPLE is a trivial cycle when its
// only user is the very view that feeds it, and it forwards that view's
// columns in identical positional order. Such a TUPLE only routes a subset
// of a MERGE's data straight back into that same MERGE, which contributes
// no new records; deleting it breaks the cyclic dependency without changing
// the fixpoint.
//
//    if tuple's only user == tuple's incoming view V,
//       and columns map 1:1 by index, and V is a MERGE:
//      delete tuple                                     -> true
//    otherwise                                          -> false
//
// Before:                          After:
//
//    other sources                  other sources
//         |    .----------.              |
//         v    v          |              v
//     UNION (MERGE)       |         UNION (MERGE)
//        |        \       |              |
//      users      TUPLE --'            users
//             (identity columns)
bool IsTrivialCycle(TUPLE *tuple) {
  if (!tuple) {
    return false;
  }

  auto incoming_view = VIEW::GetIncomingView(tuple->input_columns);

  // There is an incoming view and not all inputs are constant
  // There is only a single user view, which is the same as the incoming
  // view, meaning it's a cycle
  if (auto only_user = tuple->OnlyUser();
      only_user && incoming_view && only_user == incoming_view &&
      incoming_view->columns.Size() == tuple->columns.Size()) {
    for (auto i = 0u; i < tuple->columns.Size(); ++i) {
      auto *in_col = incoming_view->columns[i];
      auto *out_col = tuple->columns[i];
      if (in_col->Index() != out_col->Index()) {
        return false;
      }
    }

    // Contributing a subset of the MERGE's own data back to the MERGE is a
    // no-op, so the cycle-forming TUPLE is deleted.
    if (incoming_view->AsMerge()) {
      return true;
    }
  }

  return false;
}

}  // namespace hyde
