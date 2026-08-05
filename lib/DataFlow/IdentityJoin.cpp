// Copyright 2026, Peter Goodman. All rights reserved.
//
// The identity-join recognizer -- eliminate a JOIN that provably does no
// filtering. See Prov.h and docs/proposals/CostModel.artifacts/
// prov-recognizer-impl.md. This is the general form of the magic-sets
// double-join fix (owner: "a transform pass to recognize degenerate double joins
// that are identities"), enabled by the shared Prov analysis.
//
// A JOIN `J` over two views {R, S} is an IDENTITY on R when S is a pure GUARD
// side (contributes only pivot columns) whose pivots are EXACT relation
// projections that subsume R's pivot values (`Prov(R.pivot) ∋ ExactProjection(
// S.pivot)`). Then `J = R ⋉ S` with `π_A(R) ⊆ π_A(S)`, which under set semantics
// equals R -- so J's users forward to R and J is dead. MONOTONE-fenced: the
// differential regime (where a rowset-identity join can still be load-bearing
// for split-counter / seed-before-drain ordering) is a later slice.

#include "Prov.h"

#include <optional>
#include <vector>

#include "Query.h"

namespace hyde {
namespace {

// The joined view that a JOIN output column draws from, when it draws from
// exactly ONE view (a non-pivot / carry output). `nullptr` for a pivot (>= 2
// inputs) or an unmapped output.
static QueryViewImpl *SoleSourceView(QueryJoinImpl *join,
                                     QueryColumnImpl *out_col) {
  auto it = join->out_to_in.find(out_col);
  if (it == join->out_to_in.end() || it->second.Size() != 1u) {
    return nullptr;
  }
  return it->second[0]->view;
}

// Is `join` a monotone identity on one of its two joined views? If so, returns
// the KEEP view (R); otherwise nullptr. `guard_out` receives the guard view (S).
static QueryViewImpl *RecognizeIdentity(QueryJoinImpl *join, const ProvMap &prov,
                                        QueryViewImpl **guard_out) {
  // SLICE-1 SHAPE: exactly two joined views and EXACTLY ONE pivot column.
  // The single-pivot restriction is a SOUNDNESS requirement, not a convenience:
  // for >= 2 pivots, per-column containment `π_A(keep) ⊆ π_A(S) ∧ π_B(keep) ⊆
  // π_B(S)` does NOT imply the PAIR `(keep.A, keep.B)` is a row of S, so a
  // genuine multi-column filter would be dropped -> over-answer miscompile. Prov
  // tracks each column's value-set independently and never records the tuple
  // coupling; joint containment for the multi-pivot arm is a later slice.
  if (join->num_pivots != 1u || join->joined_views.Size() != 2u) {
    return nullptr;
  }

  // MONOTONE FENCE: a join that can carry deletions is out of scope -- a
  // rowset-identity join can still order split counters / seed-before-drain
  // under retraction. These flags are the LIVE differential guard (the caller
  // recomputes TrackDifferentialUpdates immediately before this pass so they are
  // fresh even under -opt-disable=df.cse). NOTE: there is deliberately no
  // IsInductive() clause -- induction_info is not populated until AFTER Optimize
  // (IdentifyInductions runs later in Build.cpp), so it would be dead code. The
  // monotone-CYCLE backstop is instead structural: Prov is bot on an inductive
  // back-edge (so the keep pivot never carries the guard's projection), AND
  // ExactProjection rejects a recursive guard (a JOIN/MERGE, never a bare
  // SELECT/TUPLE). Any future Prov rule that over-approximates on a cycle MUST
  // add an explicit acyclicity fence here.
  if (join->can_receive_deletions || join->can_produce_deletions) {
    return nullptr;
  }

  // KEEP = the unique view that is the sole source of a non-pivot (carry)
  // output. Every output column must be accounted for: a carry from keep, or
  // THE pivot spanning {keep, guard}. An unmapped / constant / guard-sourced
  // non-pivot output disqualifies (and would also feed AddUse(nullptr) in
  // ForwardToKeep).
  QueryViewImpl *keep = nullptr;
  for (auto out_col : join->columns) {
    QueryViewImpl *src = SoleSourceView(join, out_col);
    if (!src) {
      continue;  // a pivot (>=2 inputs) or unmapped -- checked in the pass below
    }
    if (!keep) {
      keep = src;
    } else if (keep != src) {
      return nullptr;  // two non-pivot sources -> ambiguous keep
    }
  }
  if (!keep) {
    return nullptr;  // all-pivot join: keep side undetermined
  }

  QueryViewImpl *guard = join->joined_views[0] == keep ? join->joined_views[1]
                                                       : join->joined_views[0];
  if (guard == keep) {
    return nullptr;  // self-join on one view -> not this shape
  }

  // Second pass: verify EVERY output column, and prove the single pivot's
  // containment. The pivot's guard input must be an EXACT projection K (so
  // `π_A(guard) = π_K` exactly), and the keep input must carry `Prov ∋ K` (so
  // `π_A(keep) ⊆ π_K = π_A(guard)`). Then the semijoin removes nothing.
  unsigned pivot_cols = 0u;
  for (auto out_col : join->columns) {
    auto it = join->out_to_in.find(out_col);
    if (it == join->out_to_in.end() || it->second.Empty()) {
      return nullptr;  // unmapped / constant output -> cannot forward soundly
    }
    if (it->second.Size() == 1u) {
      if (it->second[0]->view != keep) {
        return nullptr;  // a non-pivot output sourced from guard -> guard filters
      }
      continue;
    }
    // A pivot. It must span EXACTLY {keep, guard} (a 2-way pivot); a wider pivot
    // (a third input, e.g. a self-join) is not this shape.
    ++pivot_cols;
    if (it->second.Size() != 2u) {
      return nullptr;
    }
    QueryColumnImpl *g_col = nullptr, *k_col = nullptr;
    for (auto in_col : it->second) {
      if (in_col->view == guard) {
        g_col = in_col;
      } else if (in_col->view == keep) {
        k_col = in_col;
      }
    }
    if (!g_col || !k_col) {
      return nullptr;
    }
    std::optional<ProjKey> exact = ExactProjection(g_col);
    if (!exact) {
      return nullptr;
    }
    auto kp = prov.find(k_col);
    if (kp == prov.end() || !ProvContains(kp->second, *exact)) {
      return nullptr;
    }
  }
  if (pivot_cols != 1u) {
    return nullptr;  // belt: exactly one pivot column (H1 soundness)
  }

  *guard_out = guard;
  return keep;
}

// Forward `join`'s users to the keep view by interposing a pass-through TUPLE
// carrying, for each of `join`'s output columns, the keep-side input column.
static void ForwardToKeep(QueryImpl *query, QueryJoinImpl *join,
                          QueryViewImpl *keep) {
  // M1: FOLD any guard annotation rather than let it MIGRATE onto the fresh
  // tuple. CopyDifferentialAndGroupIdsTo (invoked by ReplaceAllUsesWith) would
  // otherwise move `join`'s guard_annotation_index onto the tuple, where
  // IsCutSuccessorDR (Rel.cpp) honors an annotation on ANY view kind -> a cut
  // successor with no provisioned demand frontier -> orphaned forcing under
  // -demand-instance. The guard is being ELIMINATED as redundant, so it is a
  // FOLD: increment the folded count and clear the index BEFORE the transfer,
  // keeping the OWN-3 census (n_stamped + folded == guard_annotations.size())
  // balanced (n_stamped drops by one, folded rises by one).
  if (join->guard_annotation_index != QueryView::kNoGuardAnnotation) {
    ++query->guard_annotation_folded_count;
    join->guard_annotation_index = QueryView::kNoGuardAnnotation;
  }

  QueryTupleImpl *const tuple = query->tuples.Create();
#ifndef NDEBUG
  tuple->producer = "IDENTITY-JOIN";
#endif
  unsigned i = 0u;
  for (auto out_col : join->columns) {
    auto it = join->out_to_in.find(out_col);
    QueryColumnImpl *keep_in = nullptr;
    if (it != join->out_to_in.end()) {
      for (auto in_col : it->second) {
        if (in_col->view == keep) {
          keep_in = in_col;
          break;
        }
      }
    }
    // A pivot's value equals its keep-side input; a carry's sole input is on
    // keep. RecognizeIdentity guarantees keep_in exists for every output.
    tuple->input_columns.AddUse(keep_in);
    (void) tuple->columns.Create(out_col->var, out_col->type, tuple,
                                 out_col->id, i++);
  }
  tuple->is_canonical = false;
  join->ReplaceAllUsesWith(tuple);
}

}  // namespace

bool EliminateIdentityJoins(QueryImpl *query) {
  const ProvMap prov = ComputeColumnProvenance(query);

  // Collect first (we mutate the join list by creating tuples / killing joins).
  std::vector<QueryJoinImpl *> candidates;
  QueryViewImpl *guard = nullptr;
  for (auto join : query->joins) {
    if (join->is_dead) {
      continue;
    }
    if (RecognizeIdentity(join, prov, &guard)) {
      candidates.push_back(join);
    }
  }

  bool changed = false;
  for (auto join : candidates) {
    if (join->is_dead) {
      continue;
    }
    // Re-recognize against the (unchanged) snapshot: eliminations only forward,
    // never add containment, so a candidate stays valid, but a keep/guard view
    // could have been folded by a prior elimination in this batch.
    QueryViewImpl *g = nullptr;
    QueryViewImpl *keep = RecognizeIdentity(join, prov, &g);
    if (!keep || keep->is_dead) {
      continue;
    }
    ForwardToKeep(query, join, keep);
    changed = true;
  }
  return changed;
}

}  // namespace hyde
