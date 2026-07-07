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
// `IsTrivialCycle`). Finally, any condition variable left with no setters
// is resolved to a fixpoint: negative tests of it are vacuously true and
// are dropped, while positive testers are unsatisfiable and are deleted
// (which may in turn strip the setters of further conditions). This is
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
//    repeat until no change:               // dead conditions
//      for cond with no setters:
//        drop cond from negative testers   // !cond is vacuously true
//        PrepareToDelete each positive tester  // cond is unsatisfiable
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
bool QueryImpl::EliminateDeadFlows(void) {

  std::unordered_set<void *> derived_from_input;
  std::vector<VIEW *> views;
  ForEachView([&views](VIEW *view) { views.push_back(view); });

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
        for (auto joined_view : view->joined_views) {
          if (joined_view->is_dead || !derived_from_input.count(joined_view)) {
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
    TUPLE *tuple = this->tuples.Create();
    auto col_index = 0u;
    for (auto col : negate->columns) {
      tuple->columns.Create(col->var, col->type, tuple, col->id, col_index);

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

  // Deleting an untainted view (or, through the dead-condition cascade in
  // `PrepareToDelete`, a tainted one) may leave dead views inside surviving
  // MERGEs, so the merged-view lists are pruned after the whole sweep.
  for (auto view : views) {
    if (auto merge = view->AsMerge(); merge && !merge->is_dead) {
      merge->merged_views.RemoveIf([&](VIEW *merged_view) {
        return merged_view->is_dead || !derived_from_input.count(merged_view);
      });
    }
  }

  for (auto changed = true; changed;) {
    changed = false;
    for (auto cond : conditions) {
      if (!cond->setters.Empty()) {
        continue;
      }

      // Negated uses of this (now dead) condition are fine, and so we can
      // remove the condition entirely.
      for (auto user_view : cond->negative_users) {
        if (user_view) {
          user_view->negative_conditions.RemoveIf(
              [=](COND *c) { return c == cond; });
        }
      }

      // Positive uses of the condition are unsatisfiable, and so we should
      // kill all positive users.
      for (auto user_view : cond->positive_users) {
        if (user_view && !user_view->is_dead) {
          user_view->PrepareToDelete();
          changed = true;
        }
      }

      cond->negative_users.Clear();
      cond->positive_users.Clear();
    }
  }

  return RemoveUnusedViews();
}

// Eliminate trivial cycles on unions. A TUPLE is a trivial cycle when its
// only user is the very view that feeds it, and it forwards that view's
// columns in identical positional order. Such a TUPLE only routes a
// (possibly condition-restricted) subset of a MERGE's data straight back
// into that same MERGE, which contributes no new records; deleting it breaks
// the cyclic dependency without changing the fixpoint. Condition variables
// complicate this: if the TUPLE sets a condition, that side effect is real
// even though the data flow is a no-op. When the TUPLE sets a condition and
// also tests conditions (i.e. it introduces a control dependency), it is
// kept alive but unlinked from the MERGE's merged views, so the cycle is
// broken while its condition-setting behavior is preserved. When it sets a
// condition unconditionally, that condition-setting is transferred onto the
// incoming view and the TUPLE is deleted. When it merely tests conditions,
// the tests are irrelevant to a self-subset and the TUPLE is deleted.
//
//    if tuple's only user == tuple's incoming view V,
//       and columns map 1:1 by index:
//      if tuple sets a condition and tests conditions:
//        remove tuple from V.merged_views; keep tuple   -> false
//      else if tuple sets a condition:
//        transfer condition-setting to V; delete tuple  -> true
//      else if V is a MERGE:
//        delete tuple                                   -> true
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

    // This TUPLE operates on a restriction of the set of nodes in the MERGE.
    // If the conditions are satisfied, then we set a separate condition, and
    // contribute back the record to the MERGE. Contributing back the data to
    // the MERGE is a no-op; however, setting the condition is not. Thus, we
    // can break the cyclic dependency between the TUPLE and the MERGE whilst
    // maintaining the TUPLE and its condition setting behavior.
    if (tuple->sets_condition && tuple->IntroducesControlDependency()) {
      if (auto merge = incoming_view->AsMerge(); merge) {
        merge->merged_views.RemoveIf([=](VIEW *v) { return v == tuple; });
      }

      return false;

    } else if (tuple->sets_condition) {
      tuple->TransferSetConditionTo(incoming_view);
      return true;

    // This TUPLE may or may not test any conditions. Any conditions tested are
    // irrelevant because they just send a subset of the MERGE's own data data
    // back into itself, which is a no-op.
    } else if (incoming_view->AsMerge()) {
      return true;
    }
  }

  return false;
}

}  // namespace hyde
