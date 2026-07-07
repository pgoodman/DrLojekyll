// Copyright 2020, Trail of Bits. All rights reserved.

#include <drlojekyll/Util/EqualitySet.h>

#include "Optimize.h"
#include "Query.h"

namespace hyde {

QueryNegateImpl::~QueryNegateImpl(void) {}

QueryNegateImpl::QueryNegateImpl(void) {
  can_receive_deletions = true;
}

QueryNegateImpl *QueryNegateImpl::AsNegate(void) noexcept {
  return this;
}

const char *QueryNegateImpl::KindName(void) const noexcept {
  if (is_never) {
    return "AND-NEVER";
  } else {
    return "AND-NOT";
  }
}

uint64_t QueryNegateImpl::Hash(void) noexcept {
  if (hash) {
    return hash;
  }

  // Start with an initial hash just in case there's a cycle somewhere.
  hash = HashInit();
  assert(hash != 0);

  auto local_hash = RotateRight64(hash, 17) ^ negated_view->Hash();

  // Mix in the hashes of the input by columns; these are ordered.
  for (auto col : input_columns) {
    local_hash ^= RotateRight64(local_hash, 33) * col->Hash();
  }

  for (auto col : attached_columns) {
    local_hash ^= RotateRight64(local_hash, 53) * col->Hash();
  }

  hash = local_hash;
  return local_hash;
}

// Canonicalize a NEGATE view. A NEGATE forwards a tuple made of
// `input_columns` (checked for absence against `negated_view`) plus
// carried-along `attached_columns`, if and only if no matching tuple is
// present in `negated_view`. Canonicalization tightens the node's inputs
// and outputs without changing which tuples flow through it: inputs are
// re-routed to read from beyond trivially-conditioned forwarding TUPLEs
// and single-source UNIONs, constants are propagated into the output
// columns, unused attached columns are dropped, and constant or duplicate
// outputs are factored into a guarding TUPLE so downstream MERGEs still
// see the expected column arity while the NEGATE itself shrinks. This
// reduces data volume, exposes constants to later passes, and normalizes
// structure so that equivalent NEGATEs can be deduplicated by CSE. Two
// degenerate shapes fold away entirely: an unsatisfiable predecessor makes
// the NEGATE unsatisfiable, and an unsatisfiable `negated_view` makes the
// absence check vacuously true, so the NEGATE is replaced by a TUPLE that
// forwards its inputs.
//
//    if is_dead or is_unsat or invalid: mark canonical; done
//    pull input/attached columns from beyond trivial TUPLEs/UNIONs
//    if predecessor is unsat: mark self unsat; done
//    if negated_view is unsat:
//      replace self with a TUPLE forwarding input+attached columns; done
//    for each (input, output) column pair:
//      propagate constants, record in_to_out, discover unused columns,
//      duplicate inputs, and guardable constant outputs
//      (duplicates among `input_columns` are always kept: they must stay
//       positionally aligned, one per column of `negated_view`)
//    if a constant/duplicate output is guardable and every user is a MERGE:
//      substitute all users with an optimized guard TUPLE that forwards
//      constants directly and collapses duplicate columns
//    rebuild the column lists: keep every input column (resolved to a
//      constant where possible); keep only the used attached columns
//    keep-last-edge rule: if the rebuilt lists would no longer reference
//      the predecessor, one representative column keeps reading from it
//      raw (an input column when possible, else an attached column
//      retained even though its output is unused), so this NEGATE's
//      presence dependency on the predecessor stays a column edge
//
// Negation of an unsatisfiable view (absence always holds):
//
//    PRED            VIEW (unsat)           PRED
//     |in,att           |                    |in,att
//     +--> NEGATE <-----+        ==>       TUPLE
//            |                               |
//          users                           users
//
// Guarding a constant attached column (b is bound to constant 1 and is
// only consumed through a MERGE; the NEGATE drops b, and the guard TUPLE
// re-supplies it straight from the constant):
//
//    PRED[a b]        REL[x]            PRED[a b]        REL[x]
//     |a   |b(=1)       |                |a                |
//     v    v            |                v                 |
//    NEGATE[a, b] <-----+     ==>      NEGATE[a] <---------+
//       |a,b                              |a     1
//       v                                 v      v
//     MERGE                            TUPLE[a, b=1]
//                                         |a,b
//                                         v
//                                       MERGE
bool QueryNegateImpl::Canonicalize(QueryImpl *query,
                                     const OptimizationContext &opt,
                                     const ErrorLog &) {

  if (is_dead || is_unsat || valid != VIEW::kValid) {
    is_canonical = true;
    return false;
  }

  if (valid == VIEW::kValid &&
      !CheckIncomingViewsMatch(input_columns, attached_columns)) {
    valid = VIEW::kInvalidBeforeCanonicalize;
    return false;
  }

  const auto num_cols = columns.Size();
  const auto first_attached_col = input_columns.Size();
  is_canonical = true;  // Updated by `CanonicalizeColumn`.
  in_to_out.clear();  // Filled in by `CanonicalizeColumn`.
  Discoveries has = {};

  // NOTE(pag): This may update `is_canonical`.
  const auto incoming_view = PullDataFromBeyondTrivialTuples(
      GetIncomingView(input_columns, attached_columns), input_columns,
      attached_columns);

  // If our predecessor is not satisfiable, then this flow is never reached.
  if (incoming_view && incoming_view->is_unsat) {
    MarkAsUnsatisfiable();
    is_canonical = true;
    return true;

  // If what we're negating is unsatisfiable, then our node isn't needed
  // anymore; the negation will always be true.
  } else if (negated_view->is_unsat) {
    TUPLE *tuple = query->tuples.Create();
    auto col_index = 0u;
    for (auto col : columns) {
      tuple->columns.Create(col->var, col->type, tuple, col->id, col_index);

      if (col_index < first_attached_col) {
        tuple->input_columns.AddUse(input_columns[col_index]);
      } else {
        tuple->input_columns.AddUse(
            attached_columns[col_index - first_attached_col]);
      }

      ++col_index;
    }

    ReplaceAllUsesWith(tuple);
    return true;
  }

  auto i = 0u;
  for (; i < first_attached_col; ++i) {
    has = CanonicalizeColumn(opt, input_columns[i], columns[i], false, has);
  }

  // NOTE(pag): Mute this, as we always need to maintain the `input_columns`
  //            and so we don't want to infinitely rewrite this negation if
  //            there is a duplicate column in `input_columns`.
  has.duplicated_input_column = false;

  for (auto j = 0u; i < num_cols; ++i, ++j) {
    has = CanonicalizeColumn(opt, attached_columns[j], columns[i], true, has);
  }

  // Nothing changed.
  if (is_canonical) {
    return has.non_local_changes;
  }

  // There is at least one output of our negation that is a constant and that
  // can be guarded, or one duplicated column. Go create a tuple that will
  // only propagate forward the needed data.
  if (has.guardable_constant_output || has.duplicated_input_column) {
    if (!IsUsedDirectly() && !(OnlyUser() && has.directly_used_column)) {
      GuardWithOptimizedTuple(query, first_attached_col);
      has.non_local_changes = true;
    }
  }

  DefList<COL> new_columns(this);
  UseList<COL> new_input_columns(this);
  UseList<COL> new_attached_columns(this);

  // The keep-last-edge rule: resolving inputs to constants and dropping
  // unused attached columns must not sever the final column edge to
  // `incoming_view`, because that edge is what expresses this NEGATE's
  // presence dependency on its predecessor. When no surviving column would
  // read from `incoming_view`, one representative keeps reading from it
  // raw: an input column when one comes from `incoming_view`, else an
  // attached column that is retained even though its output is unused. An
  // unused NEGATE produces rows for nobody, so it has no presence to
  // preserve and keeps nothing.
  const auto is_used = IsUsed();
  const auto num_attached_cols = attached_columns.Size();
  auto keep_input_index = first_attached_col;
  auto keep_attached_index = num_attached_cols;
  if (incoming_view && is_used) {
    auto retains_edge = false;
    for (auto j = 0u; j < first_attached_col && !retains_edge; ++j) {
      const auto in_col = input_columns[j];
      retains_edge =
          !in_col->IsConstantOrConstantRef() && in_col->view == incoming_view;
    }
    for (auto j = 0u; j < num_attached_cols && !retains_edge; ++j) {
      const auto in_col = attached_columns[j];
      retains_edge = columns[first_attached_col + j]->IsUsed() &&
                     !in_col->IsConstantOrConstantRef() &&
                     in_col->view == incoming_view;
    }
    if (!retains_edge) {
      for (auto j = 0u; j < first_attached_col; ++j) {
        const auto in_col = input_columns[j];
        if (!in_col->IsConstant() && in_col->view == incoming_view) {
          keep_input_index = j;
          break;
        }
      }
      for (auto j = 0u; keep_input_index == first_attached_col &&
                        j < num_attached_cols; ++j) {
        const auto in_col = attached_columns[j];
        if (!in_col->IsConstant() && in_col->view == incoming_view) {
          keep_attached_index = j;
          break;
        }
      }
      assert(keep_input_index < first_attached_col ||
             keep_attached_index < num_attached_cols);
    }
  }

  // Fixpoint check: the rebuild below drops unused attached columns and
  // resolves constant-ref inputs to constants. When the keep-last-edge
  // representative is the only unused attached column and every other
  // input is already fully resolved, the rebuild is an identity rewrite,
  // so the NEGATE is already in its final shape and the rebuild is
  // skipped.
  auto rebuild_changes_shape = false;
  for (auto j = 0u; j < first_attached_col && !rebuild_changes_shape; ++j) {
    rebuild_changes_shape =
        j != keep_input_index &&
        input_columns[j]->TryResolveToConstant() != input_columns[j];
  }
  for (auto j = 0u; j < num_attached_cols && !rebuild_changes_shape; ++j) {
    if (columns[first_attached_col + j]->IsUsed() ||
        j == keep_attached_index) {
      rebuild_changes_shape =
          j != keep_attached_index &&
          attached_columns[j]->TryResolveToConstant() != attached_columns[j];
    } else {
      rebuild_changes_shape = true;  // This attached column gets dropped.
    }
  }
  if (!rebuild_changes_shape) {
    hash = 0;
    is_canonical = true;
    return has.non_local_changes;
  }

  for (i = 0; i < first_attached_col; ++i) {
    const auto old_col = columns[i];
    const auto new_col =
        new_columns.Create(old_col->var, old_col->type, this, old_col->id, i);
    old_col->ReplaceAllUsesWith(new_col);
    new_input_columns.AddUse(i == keep_input_index
                                 ? input_columns[i]
                                 : input_columns[i]->TryResolveToConstant());
  }

  for (auto j = 0u; i < num_cols; ++i, ++j) {
    const auto old_col = columns[i];
    if (old_col->IsUsed() || j == keep_attached_index) {
      const auto new_col = new_columns.Create(old_col->var, old_col->type, this,
                                              old_col->id, new_columns.Size());
      old_col->ReplaceAllUsesWith(new_col);
      new_attached_columns.AddUse(
          j == keep_attached_index
              ? attached_columns[j]
              : attached_columns[j]->TryResolveToConstant());
    } else {
      has.non_local_changes = true;
    }
  }

  assert(!incoming_view || !is_used ||
         RetainsEdgeTo(incoming_view, new_input_columns,
                       new_attached_columns));

  columns.Swap(new_columns);
  input_columns.Swap(new_input_columns);
  attached_columns.Swap(new_attached_columns);

  hash = 0;
  is_canonical = true;

  if (!CheckIncomingViewsMatch(input_columns, attached_columns)) {
    valid = VIEW::kInvalidAfterCanonicalize;
  }

  return has.non_local_changes;
}

// Equality over inserts is structural.
bool QueryNegateImpl::Equals(EqualitySet &eq, VIEW *that_) noexcept {

  if (eq.Contains(this, that_)) {
    return true;
  }

  const auto that = that_->AsNegate();
  if (!that || can_produce_deletions != that->can_produce_deletions ||
      is_never != that->is_never || columns.Size() != that->columns.Size() ||
      positive_conditions != that->positive_conditions ||
      negative_conditions != that->negative_conditions) {
    return false;
  }

  eq.Insert(this, that);
  if (!negated_view->Equals(eq, that->negated_view.get()) ||
      !ColumnsEq(eq, input_columns, that->input_columns) ||
      !ColumnsEq(eq, attached_columns, that->attached_columns)) {
    eq.Remove(this, that);
    return false;
  }

  return true;
}

unsigned QueryNegateImpl::Depth(void) noexcept {
  if (depth) {
    return depth;
  }

  auto estimate = EstimateDepth(input_columns, 1u);
  estimate = EstimateDepth(attached_columns, depth);
  estimate = EstimateDepth(positive_conditions, depth);
  estimate = EstimateDepth(negative_conditions, depth);
  depth = estimate + 1u;

  auto real = GetDepth(input_columns, negated_view->Depth());
  real = GetDepth(attached_columns, real);
  real = GetDepth(positive_conditions, real);
  real = GetDepth(negative_conditions, real);
  depth = real + 1u;

  return depth;
}

}  // namespace hyde
