// Copyright 2026, Peter Goodman. All rights reserved.
// Copyright 2019, Trail of Bits. All rights reserved.

#include "Optimize.h"

#include <drlojekyll/Util/EqualitySet.h>

#include "Query.h"

#include <sstream>
#include <string_view>
#include <unordered_set>

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

// CSE candidate bucketing by color refinement (bisimulation-style partition
// refinement). `HashInit` alone buckets by kind/flags/arity, so on programs
// with many same-shaped views (e.g. a chain of N join rules) every bucket
// holds O(N) views and the pairwise `Equals` below degenerates to O(N^2)
// deep graph walks that discover nothing — the measured superlinear compile
// cost (DeltaRelationalIR.md §1). Refinement starts from `HashInit` and
// iteratively mixes each view's color with its children's colors until the
// color partition stops refining; `Equals` then runs only within same-color
// buckets.
//
//    color[v] := HashInit(v)  (+ SELECT: the stream/relation identity)
//    repeat (≤ kMaxColorRounds, or until #distinct colors stops growing):
//      color'[v] := mix(color[v], position-salted colors of v's children,
//                       per-kind scalar fields that Equals requires equal)
//
// CONSERVATIVENESS CONTRACT (audited per node kind, 2026-07-15; the merge
// set must equal the pairwise-Equals merge set): a color folds a value ONLY
// if that kind's `Equals` requires it to be equal — precondition scalars
// (op, functor id, binding pattern, num_pivots, is_never, ...), and child
// colors folded POSITIONALLY because every Equals here compares child
// views/columns positionally (ColumnsEq is positional; Join/Merge compare
// joined/merged view lists by index — no set semantics anywhere). Two
// Equals-equal views therefore keep equal colors through every round.
// group_ids are NEVER folded (the InsertSetsOverlap guard stays inside
// Equals); SELECT folds its stream/relation pointer because Select::Equals
// requires pointer identity there. Cycles are safe: refinement only ever
// splits color classes, so cyclic (bisimilar) equal views stay together —
// this is why colors replace `Hash()`, whose memo-guard makes it
// cycle-order-unstable (the old NOTE about grouping by `HashInit`).
namespace cse_color {

using ColorMap = std::unordered_map<VIEW *, uint64_t>;

static uint64_t ColorOf(const ColorMap &colors, VIEW *v) {
  if (auto it = colors.find(v); it != colors.end()) {
    return it->second;
  }
  return v->HashInit();  // A child outside the candidate set.
}

// Fold one input/attached column: its producing view's color plus the two
// per-column fields ColumnsEq checks (type kind, index within the view).
static uint64_t FoldCol(const ColorMap &colors, uint64_t acc, unsigned pos,
                        COL *col) {
  uint64_t c = ColorOf(colors, col->view);
  c ^= RotateRight64(c, (col->Index() + 13u) % 64u) *
       (static_cast<uint64_t>(col->type.Kind()) + 17u);
  return acc ^ RotateRight64(acc, (pos + 7u) % 64u) * c;
}

static uint64_t InitColor(VIEW *v) {
  uint64_t c = v->HashInit();
  if (auto sel = v->AsSelect(); sel) {
    // Select::Equals is pointer-identity on the stream or relation; folding
    // that identity is what lets refinement separate selects of distinct
    // relations (and everything downstream of them) without deep Equals.
    const void *ident = sel->stream ? static_cast<const void *>(sel->stream.get())
                                    : static_cast<const void *>(sel->relation.get());
    c ^= RotateRight64(c, 21u) *
         (static_cast<uint64_t>(reinterpret_cast<uintptr_t>(ident)) | 1u);
  } else if (auto insert = v->AsInsert(); insert) {
    // Insert::Equals requires `declaration` equality; declaration Ids can
    // collide (auto-declared zero-arity exports), which only makes colors
    // COARSER — conservative either way, and it seeds round-0 variety at
    // every relation boundary of the graph.
    c ^= RotateRight64(c, 27u) * (insert->declaration.Id() + 71u);
  }
  return c;
}

static uint64_t StepColor(const ColorMap &colors, VIEW *v) {
  uint64_t acc = colors.at(v);

  // The generic input/attached-column folds below fold EVERY view's column
  // colors positionally. This is sound only because, for a kind whose `Equals`
  // does NOT compare these lists, they are EMPTY — otherwise a fold would refine
  // color CLASSES that `Equals` would still merge, splitting equal views and
  // breaking the "color partition ⊇ Equals partition" invariant CSE relies on
  // (finding 6). JOIN's `Equals` keys on `out_to_in`/`joined_views` (folded in
  // the AsJoin() block below), SELECT's on stream/relation identity, MERGE's on
  // `merged_views` — none read input/attached columns, so all three MUST carry
  // them empty. Assert it (cheap, positive-space); if a future rewrite ever
  // parks columns on one of these, the fold would silently over-split.
  assert(!(v->AsJoin() || v->AsSelect() || v->AsMerge()) ||
         (v->input_columns.Empty() && v->attached_columns.Empty()));

  unsigned pos = 0u;
  for (auto col : v->input_columns) {
    acc = FoldCol(colors, acc, pos++, col);
  }
  for (auto col : v->attached_columns) {
    acc = FoldCol(colors, acc, pos++, col);
  }

  if (auto join = v->AsJoin(); join) {
    acc ^= RotateRight64(acc, 11u) * (join->num_pivots + 3u);
    acc ^= RotateRight64(acc, 19u) * (join->out_to_in.size() + 5u);
    unsigned j = 0u;
    for (auto joined_view : join->joined_views) {
      acc ^= RotateRight64(acc, (j++ + 23u) % 64u) * ColorOf(colors, joined_view);
    }
    // Per-output-column pivot/input sets, walked in output-column order —
    // mirrors Join::Equals's positional walk of `columns` through
    // `out_to_in`.
    unsigned out_pos = 0u;
    for (auto out_col : v->columns) {
      if (auto it = join->out_to_in.find(out_col); it != join->out_to_in.end()) {
        unsigned k = 0u;
        for (auto in_col : it->second) {
          acc = FoldCol(colors, acc, out_pos * 31u + k++, in_col);
        }
      }
      ++out_pos;
    }

  } else if (auto merge = v->AsMerge(); merge) {
    unsigned j = 0u;
    for (auto merged_view : merge->merged_views) {
      acc ^= RotateRight64(acc, (j++ + 29u) % 64u) * ColorOf(colors, merged_view);
    }

  } else if (auto negate = v->AsNegate(); negate) {
    acc ^= RotateRight64(acc, 31u) *
           (ColorOf(colors, negate->negated_view.get()) +
            (negate->is_never ? 41u : 43u));

  } else if (auto map = v->AsMap(); map) {
    acc ^= RotateRight64(acc, 37u) *
           (map->functor.Id() + map->num_free_params * 2u +
            (map->is_positive ? 1u : 0u) + 47u);
    acc ^= std::hash<std::string_view>{}(
        ParsedDeclaration(map->functor).BindingPattern());

  } else if (auto cmp = v->AsCompare(); cmp) {
    acc ^= RotateRight64(acc, 41u) * (static_cast<uint64_t>(cmp->op) + 53u);

  } else if (auto agg = v->AsAggregate(); agg) {
    acc ^= RotateRight64(acc, 43u) * (agg->functor.Id() + 59u);
    unsigned j = 0u;
    for (auto col : agg->group_by_columns) {
      acc = FoldCol(colors, acc, 1009u + j++, col);
    }
    for (auto col : agg->config_columns) {
      acc = FoldCol(colors, acc, 2003u + j++, col);
    }
    for (auto col : agg->aggregated_columns) {
      acc = FoldCol(colors, acc, 3001u + j++, col);
    }

  } else if (auto kv = v->AsKVIndex(); kv) {
    unsigned j = 0u;
    for (const auto &functor : kv->merge_functors) {
      acc ^= RotateRight64(acc, (j++ + 47u) % 64u) * (functor.Id() + 61u);
    }
  }
  // SELECT/TUPLE/CMP/INSERT need nothing beyond the generic folds and their
  // InitColor/HashInit terms.

  return acc;
}

// Refine to a stable partition. Distinct-color count is monotone
// non-decreasing under refinement, but distinguishing information travels
// ONE dataflow hop per round, so a chain of N rules legitimately gains as
// little as one color per round for ~N rounds (measured on the progsize
// curve) — rounds are O(V) hash folds, so running to stability is cheap
// and capping is not. Stop when the count has not grown for TWO
// consecutive rounds (the grace round tolerates a one-round plateau from a
// hash collision on an otherwise still-refining partition); bound by
// #views + 2 as the structural maximum.
static void Refine(const CandidateList &all_views, ColorMap &colors) {
  colors.reserve(all_views.size());
  for (auto view : all_views) {
    colors.emplace(view, InitColor(view));
  }
  size_t prev_distinct = 0u;
  unsigned stalled_rounds = 0u;
  const auto max_rounds = static_cast<unsigned>(all_views.size()) + 2u;
  for (auto round = 0u; round < max_rounds; ++round) {
    ColorMap next;
    next.reserve(colors.size());
    std::unordered_set<uint64_t> distinct;
    for (auto view : all_views) {
      const auto c = StepColor(colors, view);
      next.emplace(view, c);
      distinct.insert(c);
    }
    colors = std::move(next);
    if (distinct.size() <= prev_distinct) {
      if (++stalled_rounds >= 2u) {
        break;
      }
    } else {
      stalled_rounds = 0u;
      prev_distinct = distinct.size();
    }
    if (distinct.size() >= all_views.size()) {
      break;  // Every view already has a unique color.
    }
  }
}

}  // namespace cse_color

static bool CSE(QueryImpl *impl, CandidateList &all_views) {
  EqualitySet eq;
  CandidateLists candidate_groups;

  // Bucket by refined color (see above). Buckets are iterated in first-seen
  // order over `all_views` so that CSE's merge order stays independent of
  // the pointer-derived color values.
  cse_color::ColorMap colors;
  cse_color::Refine(all_views, colors);
  std::vector<uint64_t> group_order;
  for (auto view : all_views) {
    auto &group = candidate_groups[colors[view]];
    if (group.empty()) {
      group_order.push_back(colors[view]);
    }
    group.push_back(view);
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

  for (auto group_color : group_order) {
    auto &candidates = candidate_groups[group_color];

    std::sort(candidates.begin(), candidates.end());
    for (auto i = 0u; i < candidates.size(); ++i) {
      auto v1 = candidates[i];
      for (auto j = i + 1u; j < candidates.size(); ++j) {
        auto v2 = candidates[j];
        assert(v1 != v2);

        // Same color implies same HashInit ingredients: a color-fn bug that
        // co-buckets different shapes (e.g. a unit SELECT with a non-unit
        // view) is caught here rather than surfacing as a bogus merge.
        assert(v1->columns.Size() == v2->columns.Size());

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
