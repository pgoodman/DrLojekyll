// Copyright 2019, Trail of Bits. All rights reserved.

#include "Optimize.h"

#include <drlojekyll/Util/EqualitySet.h>

#include "Query.h"

#include <sstream>

namespace hyde {
namespace {

using CandidateList = std::vector<VIEW *>;
using CandidateLists = std::unordered_map<uint64_t, CandidateList>;

// Perform common subexpression elimination (CSE) over the views in
// `all_views`. Two structurally equivalent views compute the same relation,
// so all users of one can be redirected to the other; this shrinks the graph
// and lets downstream consumers share a single materialization and a single
// stream of updates. Candidates are bucketed by `HashInit`, a shallow hash of
// a view's kind and shape that ignores the view's transitive inputs, so views
// participating in cycles (e.g. through inductive UNIONs) still land in the
// same bucket. Deep equivalence is then established pairwise by `Equals`,
// which records in-progress assumptions in an `EqualitySet` so that checks
// over cyclic graphs converge. Verified pairs are applied in a priority
// order: pairs whose `UpHash`es (rough hashes of how each view is *used*,
// looking a bounded depth up the graph) agree come first, and shallower views
// come before deeper ones, so merges tend to unify similarly used subgraphs
// and cascade upward. Soundness around cross-products is maintained via
// `group_ids`, refreshed by `RelabelGroupIDs` before the pass and after each
// merge: `Equals` refuses to unify two views whose group ID sets overlap,
// e.g. two SELECTs of the same relation feeding one JOIN as distinct pivots.
//
//    bucket views by HashInit()
//    for each bucket, for each pair (v1, v2) in the bucket:
//      if v1.Equals(v2): record (v1.UpHash, v1, v2.UpHash, v2)
//    sort records: matching up-hashes first, then shallower first
//    for each record (v1, v2), chasing v2 through prior merges:
//      if v1 != v2, both still used, and still Equals:
//        v1.ReplaceAllUsesWith(v2); RelabelGroupIDs()
//
//  Before:                             After:
//
//    SELECT[edge]  SELECT[edge]          SELECT[edge]
//         |             |                     |
//       MAP[f]        MAP[f]                MAP[f]
//         |             |                   /    \
//     INSERT[a]     INSERT[b]        INSERT[a]  INSERT[b]
// Returns `true` if `user` reads any of `def`'s output columns. Two views
// related this way can only prove `Equals` around a source-less forwarding
// cycle; they do not represent a common subexpression, and redirecting the
// user's reads onto its own outputs would make it its own user.
static bool DirectlyUsesColumnsOf(VIEW *user, VIEW *def) {
  for (auto col : def->columns) {
    auto used = false;
    col->ForEachUser([&](VIEW *user_view) {
      if (user_view == user) {
        used = true;
      }
    });
    if (used) {
      return true;
    }
  }
  return false;
}

static bool CSE(QueryImpl *impl, CandidateList &all_views) {
  EqualitySet eq;
  CandidateLists candidate_groups;

  // NOTE(pag): We group by `HashInit` rather than `Hash` as `Hash` will force
  //            us to miss opportunities due to cycles in the dataflow graph.
  //            `HashInit` ends up being a good enough filter to restrict us to
  //            plausibly similar things.
  for (auto view : all_views) {
    candidate_groups[view->HashInit()].push_back(view);
  }

  auto changed = false;

  using CandidatePair = std::tuple<uint64_t, VIEW *, uint64_t, VIEW *>;
  std::vector<CandidatePair> to_replace;
  std::unordered_map<VIEW *, VIEW *> top_map;
  auto resolve = [&](VIEW *a) -> VIEW * {
    while (top_map.count(a)) {
      a = top_map[a];
    }
    return a;
  };

  impl->RelabelGroupIDs();

  for (auto &[hash, candidates] : candidate_groups) {
    (void) hash;

    std::sort(candidates.begin(), candidates.end());
    for (auto i = 0u; i < candidates.size(); ++i) {
      auto v1 = candidates[i];
      for (auto j = i + 1u; j < candidates.size(); ++j) {
        auto v2 = candidates[j];
        assert(v1 != v2);

        eq.Clear();
        if (v1->Equals(eq, v2)) {
          to_replace.emplace_back(v1->UpHash(1), v1, v2->UpHash(1), v2);
          top_map.emplace(v1, v2);
        }
      }
    }

    std::sort(to_replace.begin(), to_replace.end(),
              [](CandidatePair a, CandidatePair b) {
                const auto a_v1_uphash = std::get<0>(a);
                const auto a_v2_uphash = std::get<2>(a);

                const auto b_v1_uphash = std::get<0>(b);
                const auto b_v2_uphash = std::get<2>(b);

                int a_bad = a_v1_uphash != a_v2_uphash;
                int b_bad = b_v1_uphash != b_v2_uphash;

                if (a_bad != b_bad) {
                  return a_bad < b_bad;
                }

                const auto a_v1 = std::get<1>(a);
                const auto a_v2 = std::get<3>(a);

                const auto b_v1 = std::get<1>(b);
                const auto b_v2 = std::get<3>(b);

                return std::min(a_v1->Depth(), a_v2->Depth()) <
                       std::min(b_v1->Depth(), b_v2->Depth());
              });

    while (!to_replace.empty()) {
      auto [v1_uphash, v1, v2_uphash, v2] = to_replace.back();
      to_replace.pop_back();
      v2 = resolve(v2);

      (void) v1_uphash;
      (void) v2_uphash;

      eq.Clear();
      if (v1 != v2 && v1->IsUsed() && v2->IsUsed() && v1->Equals(eq, v2) &&
          !DirectlyUsesColumnsOf(v1, v2) && !DirectlyUsesColumnsOf(v2, v1)) {
#ifndef NDEBUG
        std::stringstream ss;
        ss << "CSE(" << v2->producer << ", " << v1->producer << ")";
        ss.str().swap(v2->producer);
#endif
        v1->ReplaceAllUsesWith(v2);
        impl->RelabelGroupIDs();
        changed = true;
      }
    }
  }

  impl->ClearGroupIDs();

  return changed;
}

template <typename T>
static void FillViews(T &def_list, CandidateList &views_out) {
  for (auto view : def_list) {
    if (view->IsUsed()) {
      views_out.push_back(view);
    }
  }
  std::sort(views_out.begin(), views_out.end(),
            [](VIEW *a, VIEW *b) { return a->Depth() < b->Depth(); });
}

}  // namespace

// Clear all group IDs. Sometimes we want to do optimizations that excplicitly
// don't need to deal with the issues of accidentally over-merging nodes.
void QueryImpl::ClearGroupIDs(void) {
  const_cast<const QueryImpl *>(this)->ForEachView([&](VIEW *view) {
    view->group_ids.clear();
  });
}

// Relabel group IDs. This enables us to better optimize SELECTs. Our initial
// assignment of `group_id`s works well enough to start with, but isn't good
// enough to help us merge some SELECTs. The key idea is that if a given
// INSERT reaches two SELECTs, then those SELECTs cannot be merged.
void QueryImpl::RelabelGroupIDs(void) {

  // Clear out all `group_id` sets, and reset the depth counters.
  std::vector<COL *> sorted_cols;

  unsigned i = 1u;
  const_cast<const QueryImpl *>(this)->ForEachView([&](VIEW *view) {
    if (view->is_dead) {
      return;
    }

    view->depth = 0;
    view->hash = 0;
    view->group_ids.clear();

    if (view->AsJoin() || view->AsAggregate() || view->AsKVIndex()) {
      view->group_id = i++;
      view->group_ids.push_back(view->group_id);

    } else {
      view->group_id = 0;
    }

    for (auto col : view->columns) {
      sorted_cols.push_back(col);
    }
  });

  const_cast<const QueryImpl *>(this)->ForEachView([&](VIEW *view) {
    if (view->is_dead) {
      return;
    }

    (void) view->Depth();  // Calculate view depth.
  });

  // Sort it so that we process deeper views (closer to INSERTs) first.
  std::sort(sorted_cols.begin(), sorted_cols.end(),
            [](COL *a, COL *b) { return a->view->Depth() > b->view->Depth(); });

  // Propagate the group IDs down through the graph.
  for (auto changed = true; changed;) {
    changed = false;
    for (auto col : sorted_cols) {
      const auto view = col->view;

      const auto old_size = view->group_ids.size();

      // Look at the users of this column, e.g. joins, aggregates, tuples,
      // and copy their view's group ids back to this view.
      col->ForEachUser([=](VIEW *user) {
        assert(view != user);

        // If the user if a JOIN, AGGREGATE, or KVINDEX, then take its group
        // ID.
        if (user->group_id) {
          view->group_ids.push_back(user->group_id);

        // Otherwise, take its set of group IDs.
        } else {
          view->group_ids.insert(view->group_ids.end(), user->group_ids.begin(),
                                 user->group_ids.end());
        }
      });

      std::sort(view->group_ids.begin(), view->group_ids.end());
      auto it = std::unique(view->group_ids.begin(), view->group_ids.end());
      view->group_ids.erase(it, view->group_ids.end());

      if (view->group_ids.size() > old_size) {
        changed = true;
      }
    }
  }
}

// Remove unused views.
bool QueryImpl::RemoveUnusedViews(void) {
  size_t ret = 0;
  size_t all_ret = 0;

  std::vector<VIEW *> views;

  ForEachViewInReverseDepthOrder([&](VIEW *view) { views.push_back(view); });

  for (auto changed = true; changed;) {
    changed = false;
    for (auto view : views) {
      if (!view->IsUsed()) {
        if (view->PrepareToDelete()) {
          changed = true;
        }
      }
    }
  }

  do {
    ret = 0u;
    ret |= selects.RemoveUnused() | tuples.RemoveUnused() |
           kv_indices.RemoveUnused() | joins.RemoveUnused() |
           maps.RemoveUnused() | aggregates.RemoveUnused() |
           merges.RemoveUnused() | compares.RemoveUnused() |
           inserts.RemoveUnused() | negations.RemoveUnused();
    all_ret |= ret;
  } while (ret);

  all_ret |= relations.RemoveIf(
      [](REL *rel) { return rel->inserts.Empty() && rel->selects.Empty(); });

  all_ret |= ios.RemoveIf(
      [](IO *io) { return io->receives.Empty() && io->transmits.Empty(); });

  return 0 != all_ret;
}

// Performs a limited amount of optimization before INSERTs are linked to
// SELECTs. CSE runs over the SELECTs alone so that duplicate SELECTs of the
// same relation or stream collapse first, which makes the initial TUPLEs
// built on top of them look identical and thus easier to canonicalize later.
// Then every used JOIN is canonicalized with default (conservative) options,
// which drops useless pivot columns and rewrites degenerate JOINs -- e.g. a
// JOIN whose columns all derive from a single incoming view -- into
// forwarding TUPLEs; a second sweep over the JOINs canonicalizes any forms
// exposed by the first sweep. Finally, unused views are removed. Running
// this before INSERT-to-SELECT linking is beneficial because it shrinks the
// graph while uses are still simple, without risking the over-merging
// hazards of full canonicalization.
//
//    CSE(used SELECTs)
//    for each used JOIN: JOIN.Canonicalize(default opts)
//    for each used JOIN: JOIN.Canonicalize(default opts)  // cascaded forms
//    RemoveUnusedViews()
//
//  Before:                            After:
//
//    SELECT[t]                          SELECT[t]
//      |    |                               |
//    JOIN(A=A)   (both pivot             TUPLE
//        |        inputs are the           |
//     INSERT      same column)          INSERT
//
// TODO(pag): The join canonicalization introduces a bug in Solypsis if the
//            dataflow builder builds functors before joins. I'm not sure why
//            and this is probably a serious bug.
void QueryImpl::Simplify(const ErrorLog &log) {
  CandidateList views;

  // Start by applying CSE to the SELECTs only. This will improve
  // canonicalization of the initial TUPLEs and other things.
  FillViews(selects, views);
  CSE(this, views);

  OptimizationContext opt;

  // Now canonicalize JOINs, which will eliminate columns of useless joins.
  views.clear();
  FillViews(joins, views);
  for (auto view : views) {
    view->Canonicalize(this, opt, log);
  }

  // Some of those useless JOINs are converted into TUPLEs, so canonicalize
  // those.
  views.clear();
  FillViews(joins, views);
  for (auto view : views) {
    view->Canonicalize(this, opt, log);
  }

  RemoveUnusedViews();
}

// Canonicalize the dataflow: drive every VIEW's local `Canonicalize` rewrite
// (constant propagation, duplicate/unused column elimination, guard-TUPLE
// introduction, deduplication of UNION inputs, collapsing of trivial nodes
// into forwarding TUPLEs, etc.) to a fixpoint, putting each node into its
// "most optimal" form. Each per-node rewrite is locally sound and preserves
// the view's produced relation, and iterating them in depth order lets one
// node's simplification expose simplifications in its neighbors, so the
// whole graph settles into a smaller, more uniform shape that later CSE can
// exploit. Every view is first marked non-canonical; passes then visit live
// views in depth order (bottom-up, from SELECTs toward INSERTs) or reverse
// depth order when `opt.bottom_up` is false. Termination is doubly guarded:
// a pass-count cap of roughly max(2N, N^2) for N views, and an eight-entry
// history of per-pass change hashes that detects when the rewrites cycle
// through a repeating pattern without net progress and breaks out of the
// loop. Unused views are removed afterward.
//
//    mark all views non-canonical
//    repeat up to max(2N, N^2) passes:
//      hash = 0
//      for each live view in (reverse) depth order:
//        if view.Canonicalize(opt) reports non-local changes:
//          hash = rotate(hash) ^ view.Hash()
//      stop if no view reported non-local changes, or if the
//      last 8 pass hashes are all identical (cyclic non-progress)
//    RemoveUnusedViews()
//
//  Before (one rewrite among many):     After:
//
//        MAP[f]                             MAP[f]
//         |  |     (duplicate                 |
//    UNION(MERGE)   inputs)                 TUPLE
//         |                                   |
//       INSERT                             INSERT
void QueryImpl::Canonicalize(const OptimizationContext &opt,
                             const ErrorLog &log) {
  uint64_t num_views = 0u;
  const_cast<const QueryImpl *>(this)->ForEachView([&num_views](VIEW *view) {
    view->is_canonical = false;
    ++num_views;
  });

  auto max_iters = std::max<uint64_t>(num_views, num_views * 2u);
  max_iters = std::max<uint64_t>(max_iters, num_views * num_views);

  // Canonicalize all views.
  uint64_t iter = 0u;

  constexpr auto kNumHistories = 8u;
  uint64_t hash_history[kNumHistories] = {};
  auto curr_hash_index = 0u;

#ifndef NDEBUG
  auto check_consistency = [=] (VIEW *v) {
    for (auto c : v->columns) {
      assert(c->view == v);
    }
  };
#else
#  define check_consistency(v)
#endif


  // Running hash of which views produced non-local changes.
  auto non_local_changes = true;
  uint64_t hash = 0u;

  // Applied to canonicalize each view.
  auto on_each_view = [&](VIEW *view) {
    if (!view->is_dead) {
      check_consistency(view);
      const auto ret = view->Canonicalize(this, opt, log);
      check_consistency(view);
      if (ret) {
        non_local_changes = true;

        // A view may unlink itself from the graph during canonicalization,
        // e.g. by replacing all of its uses with another view. A dead view
        // has no structure left to fingerprint for convergence detection.
        if (!view->is_dead) {
          hash = RotateRight64(hash, 13) ^ view->Hash();
        }
      }
    }
  };

  for (; non_local_changes && iter < max_iters; ++iter) {

    non_local_changes = false;
    hash = 0u;

    if (opt.bottom_up) {
      ForEachViewInDepthOrder(on_each_view);
    } else {
      ForEachViewInReverseDepthOrder(on_each_view);
    }

    // Store our running hash into our history of hashes.
    const auto prev_hash = hash_history[curr_hash_index];
    hash_history[curr_hash_index] = hash;
    curr_hash_index = (curr_hash_index + 1u) % kNumHistories;

    // Now check if all hashes in our history of hashes match. This is a pretty
    // easy way to detect if we've converged to some kind of cyclic pattern
    // that keeps popping up and this lets us break out of a loop.
    //
    // TODO(pag): Really, there are deeper problems of monotonicity that need
    //            to be solved, and this is a convenient band-aid.
    if (prev_hash == hash) {
      auto all_eq = true;
      for (auto existing_hash : hash_history) {
        if (existing_hash != hash) {
          all_eq = false;
          break;
        }
      }

      // Looks like we've converged.
      if (all_eq) {
        break;
      }
    }
  }

  RemoveUnusedViews();
}

// Top-level dataflow optimization driver. It interleaves three global
// passes -- CSE, canonicalization, and dead-flow cleanup --
// because each exposes work for the others: CSE merges duplicate subgraphs,
// which gives UNIONs duplicate inputs for canonicalization to fold;
// canonicalization normalizes node shapes, which makes more views
// structurally equal for the next CSE round. `do_cse` re-runs CSE until no
// merges happen (bounded by the number of views), removing dead views and
// re-deriving differential-update tracking after each round, since merging
// changes which flows can carry deletions. `do_sink` is the UNION-sinking
// hook, a re-canonicalization point that pushes MERGEs below the TUPLEs,
// MAPs, and NEGATEs above them; its body is currently disabled. After a
// first conservative bottom-up canonicalization, up to max-INSERT-depth
// rounds of aggressive top-down canonicalization run, with unused-column
// removal and constant-input replacement enabled, each followed by
// taint-based dead-flow elimination; the loop
// continues while dead flows keep disappearing. A final CSE round and
// unused-view removal finish the pipeline.
//
//    do_cse():  while CSE(all views) merges something:
//                 RemoveUnusedViews(); TrackDifferentialUpdates()
//    do_sink(): UNION-sinking re-canonicalization (disabled)
//
//    do_sink(); do_cse()
//    Canonicalize(default: bottom-up, conservative)
//    do_sink(); do_cse(); do_sink()
//    for up to max INSERT depth, while EliminateDeadFlows() changes:
//      Canonicalize(top-down, +remove unused columns,
//                   +replace constant inputs)
//      do_sink()
//      RemoveUnusedViews()
//    do_cse(); RemoveUnusedViews()
//
//  Before:                            After:
//
//    SELECT[r]    SELECT[r]              SELECT[r]
//       |            |                       |
//     MAP[f]       MAP[f]                  MAP[f]
//        \          /                        |
//       UNION(MERGE)                       TUPLE
//            |                               |
//         INSERT                          INSERT
void QueryImpl::Optimize(const ErrorLog &log) {
  CandidateList views;

  auto do_cse = [&](void) {
    views.clear();
    const_cast<const QueryImpl *>(this)->ForEachView(
        [&views](VIEW *view) { views.push_back(view); });

    for (auto max_cse = views.size(); max_cse-- && CSE(this, views);) {
      RemoveUnusedViews();
      TrackDifferentialUpdates(log, true);
      views.clear();
      const_cast<const QueryImpl *>(this)->ForEachView(
          [&views](VIEW *view) { views.push_back(view); });
    }
  };

  auto do_sink = [&](void) {
//    OptimizationContext opt;
//    for (auto i = 0u; i < merges.Size(); ++i) {
//      MERGE *const merge = merges[i];
//      if (!merge->is_dead) {
//        merge->is_canonical = false;
//        opt.can_sink_unions = false;
//        opt.can_remove_unused_columns = false;
//        merge->Canonicalize(this, opt, log);
//        if (!merge->is_dead) {
//          merge->is_canonical = false;
//          opt.can_sink_unions = true;
//          opt.can_remove_unused_columns = false;
//          merge->Canonicalize(this, opt, log);
//        }
//      }
//    }
  };

  do_sink();
  do_cse();  // Apply CSE to all views before most canonicalization.

  OptimizationContext opt;
  Canonicalize(opt, log);

  do_sink();
  do_cse();  // Apply CSE to all canonical views.
  do_sink();

  auto max_depth = 1u;
  for (auto view : this->inserts) {
    max_depth = std::max(view->Depth(), max_depth);
  }

  for (auto changed = true; changed && max_depth--;) {

    // Now do a stronger form of canonicalization.
    opt.can_remove_unused_columns = true;
    opt.can_replace_inputs_with_constants = true;
    opt.can_sink_unions = false;
    opt.bottom_up = false;
    Canonicalize(opt, log);
    do_sink();

    RemoveUnusedViews();
    changed = EliminateDeadFlows();
  }

  do_cse();  // Apply CSE to all canonical views.

  RemoveUnusedViews();
}

}  // namespace hyde
