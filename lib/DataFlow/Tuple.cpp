// Copyright 2026, Peter Goodman. All rights reserved.
// Copyright 2020, Trail of Bits. All rights reserved.

#include <drlojekyll/Util/EqualitySet.h>

#include "Optimize.h"
#include "Query.h"

namespace hyde {

QueryTupleImpl::~QueryTupleImpl(void) {}

const char *QueryTupleImpl::KindName(void) const noexcept {
  return "TUPLE";
}

QueryTupleImpl *QueryTupleImpl::AsTuple(void) noexcept {
  return this;
}

uint64_t QueryTupleImpl::Hash(void) noexcept {
  if (hash) {
    return hash;
  }

  hash = HashInit();
  assert(hash != 0);

  auto local_hash = hash;

  // Mix in the hashes of the tuple by columns; these are ordered.
  for (auto col : input_columns) {
    local_hash ^= RotateRight64(local_hash, 33) * col->Hash();
  }

  hash = local_hash;
  return local_hash;
}

// Put this tuple into a canonical form, which will make comparisons and
// replacements easier. Because comparisons are mostly pointer-based, the
// canonical form of a TUPLE is one that reads its data from as far up the
// dataflow as possible (hopping over trivially forwarding TUPLEs and
// single-source UNIONs), whose output columns are marked as constant
// references whenever the corresponding inputs are constant, whose duplicated
// output columns have all downstream users redirected onto one representative
// column, and where every remaining output column is guaranteed to be used.
// This is beneficial because shrunken, deduplicated tuples are more likely to
// hash/compare as structurally equal (enabling CSE in the optimizer driver)
// and unused dataflow upstream becomes deletable. It is sound because a TUPLE
// only forwards values: a column that provably carries a constant, duplicates
// a sibling column, or reaches no user can be rewritten or dropped without
// changing the multiset of tuples produced.
//
//   if locked, unsatisfiable, dead, or invalid: bail out
//   incoming = PullDataFromBeyondTrivialTuples(input_columns)
//   if incoming is unsatisfiable: mark self unsatisfiable; done
//   for each (in_col -> out_col) pair:
//     if in_col is a constant or constant ref:
//       mark out_col as a constant ref
//     if in_col repeats an earlier input column:
//       redirect direct users of out_col to the earlier out col
//     record whether out_col is unused
//   if nothing was discovered: done
//   rebuild the tuple:
//     keep only used output columns
//     resolve each kept input column to its constant, if any
//     keep-last-edge rule: if the rebuilt tuple would no longer read
//       any column of `incoming`, keep one representative column that
//       reads from `incoming` (raw, not resolved to a constant), even
//       if its output is otherwise unused, so the tuple's presence
//       dependency on `incoming` stays expressed as a column edge
//   if no output columns remain (possible only with no incoming view,
//     e.g. an all-constant tuple whose outputs are all unused): delete self
//
// Constant propagation, duplicate redirection, unused-column removal:
//
//   Before:                           After:
//
//     VIEW V[a, b]    CONST 1           VIEW V[a, b]    CONST 1
//      |a  |b  |a      |                 |a              |
//     TUPLE [w=a, x=b, y=1, z=a]        TUPLE [w=a, y=1]
//      |w        |y   |z                 |w    |y
//     (x unused)                        (z's users now read w)
//
// Forwarding elimination (hopping over an unconditional TUPLE):
//
//   Before:                           After:
//
//     VIEW V[a, b]                      VIEW V[a, b]
//      |                                 |
//     TUPLE T1[a, b]                    TUPLE T2[b, a]
//      |
//     TUPLE T2[b, a]                    (T1 deleted once unused)
//
// Keep-last-edge: `x` is unused, but dropping it would sever the final
// column edge to V, so it is kept as the representative witness that the
// tuple's rows exist only while V holds data:
//
//   Before:                           After:
//
//     VIEW V[a]    CONST 1              VIEW V[a]    CONST 1
//      |a           |                    |a           |
//     TUPLE [x=a, y=1]                  TUPLE [x=a, y=1]
//     (x unused)                        (x unused, kept)
bool QueryTupleImpl::Canonicalize(QueryImpl *,
                                    const OptimizationContext &opt,
                                    const ErrorLog &) {

  if (is_locked || is_unsat || is_dead || valid != VIEW::kValid) {
    is_canonical = true;
    return false;
  }

  if (valid == VIEW::kValid &&
      !CheckIncomingViewsMatch(input_columns, attached_columns)) {
    valid = VIEW::kInvalidBeforeCanonicalize;
    return false;
  }

  const auto num_cols = columns.Size();
  is_canonical = true;  // Updated by `CanonicalizeColumn`.
  in_to_out.clear();  // Filled in by `CanonicalizeColumn`.
  Discoveries has = {};

  // NOTE(pag): This may update `is_canonical`.
  const auto incoming_view = PullDataFromBeyondTrivialTuples(
      GetIncomingView(input_columns), input_columns, attached_columns);

  if (incoming_view && incoming_view->is_unsat) {
    MarkAsUnsatisfiable();
    return true;
  }

  auto i = 0u;
  for (; i < num_cols; ++i) {

    // NOTE(pag): We treat all tuple columns as `is_attached=true` so that
    //            finding unused outputs changes `is_canonical`.
    has = CanonicalizeColumn(opt, input_columns[i], columns[i], true, has);
  }

  // Nothing changed.
  if (is_canonical) {
    return has.non_local_changes;
  }

  // NOTE(pag): We don't both with `has.guardable_constant_output`, as it is
  //            only triggered if `out_col->IsUsed()`, and thus we will preserve
  //            the output column here.

  // NOTE(pag): We don't both with `has.duplicated_input_column`, because we'll
  //            either drop it below if `!out_col->IsUsed()`, or we'll preserve
  //            it, which would be equivalent but less wasteful than what
  //            `GuardWithOptimizedTuple` would do, given that it'd be a tuple
  //            guarding a tuple.

  DefList<COL> new_columns(this);
  UseList<COL> new_input_columns(this);

  // The keep-last-edge rule: dropping unused output columns and resolving
  // constant-ref inputs to their constants must not sever the final column
  // edge to `incoming_view`, because that edge is what expresses this
  // tuple's presence dependency on its predecessor. When no used,
  // unresolvable input from `incoming_view` would survive the rebuild, one
  // representative column keeps reading from `incoming_view` raw (a used
  // constant-ref column when one exists, else an otherwise-unused column).
  // An unused tuple produces rows for nobody, so it has no presence to
  // preserve: it keeps nothing, drops all of its columns, and deletes
  // itself below.
  const auto is_used = IsUsed();
  auto keep_index = num_cols;
  if (incoming_view && is_used) {
    auto retains_edge = false;
    for (i = 0u; i < num_cols; ++i) {
      const auto in_col = input_columns[i];
      if (columns[i]->IsUsed() && !in_col->IsConstantOrConstantRef() &&
          in_col->view == incoming_view) {
        retains_edge = true;
        break;
      }
    }
    if (!retains_edge) {
      for (i = 0u; i < num_cols; ++i) {
        const auto in_col = input_columns[i];
        if (columns[i]->IsUsed() && !in_col->IsConstant() &&
            in_col->view == incoming_view) {
          keep_index = i;  // Used constant-ref column; kept raw.
          break;
        }
      }
      for (i = 0u; keep_index == num_cols && i < num_cols; ++i) {
        const auto in_col = input_columns[i];
        if (!in_col->IsConstant() && in_col->view == incoming_view) {
          keep_index = i;  // Unused column, kept as the representative.
          break;
        }
      }
      assert(keep_index < num_cols);
    }
  }

  // Fixpoint check: the rebuild below drops unused output columns and
  // resolves constant-ref inputs to constants. When the keep-last-edge
  // representative is the only unused column and every other input is
  // already fully resolved, the rebuild is an identity rewrite, so the
  // tuple is already in its final shape and the (expensive) rebuild is
  // skipped.
  auto rebuild_changes_shape = false;
  for (i = 0u; i < num_cols && !rebuild_changes_shape; ++i) {
    if (columns[i]->IsUsed() || i == keep_index) {
      rebuild_changes_shape =
          i != keep_index &&
          input_columns[i]->TryResolveToConstant() != input_columns[i];
    } else {
      rebuild_changes_shape = true;  // This column gets dropped.
    }
  }
  if (!rebuild_changes_shape) {
    hash = 0;
    is_canonical = true;
    return has.non_local_changes;
  }

  for (i = 0; i < num_cols; ++i) {
    const auto old_col = columns[i];
    if (old_col->IsUsed() || i == keep_index) {
      const auto new_col =
          new_columns.Create(old_col->var, old_col->type, this, old_col->id, i);
      old_col->ReplaceAllUsesWith(new_col);
      new_input_columns.AddUse(i == keep_index
                                   ? input_columns[i]
                                   : input_columns[i]->TryResolveToConstant());
    } else {
      has.non_local_changes = true;
    }
  }

  assert(!incoming_view || !is_used ||
         RetainsEdgeTo(incoming_view, new_input_columns, new_input_columns));

  columns.Swap(new_columns);
  input_columns.Swap(new_input_columns);

  hash = 0;
  is_canonical = true;

  if (!CheckIncomingViewsMatch(input_columns, attached_columns)) {
    valid = VIEW::kInvalidAfterCanonicalize;
  }

  // We've eliminated all columns. Likely this means that we had a tuple that
  // was full of constants.
  if (columns.Empty()) {

    // This might happen as a result of `SkipPastForwardingTuples`.
    if (!IsUsed()) {
      PrepareToDelete();
      return false;
    }

    PrepareToDelete();
    return true;
  }

  return has.non_local_changes;
}

// Equality over tuples is structural.
bool QueryTupleImpl::Equals(EqualitySet &eq,
                              QueryViewImpl *that_) noexcept {
  if (eq.Contains(this, that_)) {
    return true;
  }

  const auto that = that_->AsTuple();
  if (!that || can_receive_deletions != that->can_receive_deletions ||
      can_produce_deletions != that->can_produce_deletions ||
      columns.Size() != that->columns.Size() || InsertSetsOverlap(this, that)) {
    return false;
  }

  eq.Insert(this, that);
  if (!ColumnsEq(eq, input_columns, that->input_columns)) {
    eq.Remove(this, that);
    return false;
  }

  return true;
}

// Returns `true` if all input columns to the tuple are constant.
bool QueryTupleImpl::AllInputsAreConstant(void) {
  return !GetIncomingView(input_columns);
}

// Does this tuple forward all of its inputs to the same columns as the
// outputs, and if so, does it forward all columns of its input?
bool QueryTupleImpl::ForwardsAllInputsAsIs(void) const noexcept {
  return ForwardsAllInputsAsIs(GetIncomingView(input_columns));
}

// Does this tuple forward all of its inputs to the same columns as the
// outputs, and if so, does it forward all columns of its input?
bool QueryTupleImpl::ForwardsAllInputsAsIs(
    VIEW *incoming_view) const noexcept {

  if (!incoming_view) {
    return false;
  }

  const auto num_cols = columns.Size();

  // Check to see if we can use `incoming_view` in place of `this`. We need
  // to be extra careful about whether or not `this` and `incoming_view` are
  // directly used by the same join.
  if (incoming_view && incoming_view->columns.Size() == num_cols) {

    // Make sure all columns are perfectly forwarded.
    for (auto i = 0u; i < num_cols; ++i) {
      const auto in_col = input_columns[i];
      if (in_col->view != incoming_view || in_col->Index() != i) {
        return false;
      }
    }

    return true;

  } else {
    return false;
  }
}

}  // namespace hyde
