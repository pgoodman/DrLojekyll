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
//   if the rebuilt tuple no longer reads from `incoming`:
//     make `incoming` set a CONDition tested by this tuple, so
//     outputs stay predicated on `incoming` producing data
//   if no output columns remain:
//     delete self when unused, unconditional, or guarded only by
//     trivial positive conditions; otherwise restore the old
//     columns and lock the tuple against future canonicalization
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
// Data dependence converted to control dependence when the last
// non-constant input is dropped:
//
//   Before:                           After:
//
//     VIEW V[a]    CONST 1              VIEW V[a] --sets COND--+
//      |a           |                                          |
//     TUPLE [x=a, y=1]                  TUPLE [y=1] --tests COND
//     (x unused)
bool QueryTupleImpl::Canonicalize(QueryImpl *query,
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

  for (i = 0; i < num_cols; ++i) {
    const auto old_col = columns[i];
    if (old_col->IsUsed()) {
      const auto new_col =
          new_columns.Create(old_col->var, old_col->type, this, old_col->id, i);
      old_col->ReplaceAllUsesWith(new_col);
      new_input_columns.AddUse(input_columns[i]->TryResolveToConstant());
    } else {
      has.non_local_changes = true;
    }
  }

  // We dropped a reference to our predecessor; maintain it via a condition.
  if (incoming_view) {
    const auto new_incoming_view = GetIncomingView(new_input_columns);
    if (incoming_view != new_incoming_view) {
      CreateDependencyOnView(query, incoming_view);
      has.non_local_changes = true;
    }
  }

  columns.Swap(new_columns);
  input_columns.Swap(new_input_columns);

  hash = 0;
  is_canonical = true;

  if (!CheckIncomingViewsMatch(input_columns, attached_columns)) {
    valid = VIEW::kInvalidAfterCanonicalize;
  }

  // We've eliminated all columns. Likely this means that we had a tuple that
  // was full of constants. Now we're in the unenviable position where we need
  // to deal with any conditions.
  if (columns.Empty()) {

    // This might happen as a result of `SkipPastForwardingTuples`.
    if (!IsUsed()) {
      PrepareToDelete();
      return false;
    }

    // This tuple doesn't test any conditions.
    if (positive_conditions.Empty() && negative_conditions.Empty()) {
      PrepareToDelete();
      return true;

    // This tuple only tests trivial positive conditions.
    } else if (negative_conditions.Empty()) {
      auto all_trivial = true;
      for (auto cond : positive_conditions) {
        if (!cond->IsTrivial()) {
          all_trivial = false;
          break;
        }
      }

      if (all_trivial) {
        PrepareToDelete();
        return true;
      }
    }

    // Restore the old columns.
    columns.Swap(new_columns);
    input_columns.Swap(new_input_columns);
    is_locked = true;
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
  if (!that || positive_conditions != that->positive_conditions ||
      negative_conditions != that->negative_conditions ||
      can_receive_deletions != that->can_receive_deletions ||
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
  if (incoming_view && !sets_condition && positive_conditions.Empty() &&
      negative_conditions.Empty() &&
      incoming_view->columns.Size() == num_cols) {

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
