// Copyright 2026, Peter Goodman. All rights reserved.
// Copyright 2020, Trail of Bits. All rights reserved.

#include <drlojekyll/Util/EqualitySet.h>

#include <sstream>
#include <unordered_set>

#include "Optimize.h"
#include "Query.h"

namespace hyde {

QueryJoinImpl::~QueryJoinImpl(void) {}

QueryJoinImpl *QueryJoinImpl::AsJoin(void) noexcept {
  return this;
}

const char *QueryJoinImpl::KindName(void) const noexcept {
  if (num_pivots) {
    return "JOIN";
  } else {
    return "PRODUCT";
  }
}

uint64_t QueryJoinImpl::Hash(void) noexcept {
  if (hash) {
    return hash;
  }

  hash = HashInit();
  assert(hash != 0);

  if (out_to_in.empty()) {
    return hash;
  }

  auto local_hash = hash;

  assert(input_columns.Size() == 0);

  //
  //  for (auto col : columns) {
  //    auto in_set = out_to_in.find(col);
  //    assert(in_set != out_to_in.end());
  //    uint64_t pivot_hash = 0xC4CEB9FE1A85EC53ull;
  //    for (auto in_col : in_set->second) {
  //      pivot_hash ^= in_col->Hash();
  //    }
  //    local_hash ^= RotateRight64(local_hash, 13) * pivot_hash;
  //  }

  for (auto joined_view : joined_views) {
    local_hash ^= joined_view->Hash();
  }

  if (num_pivots) {
    local_hash ^=
        RotateRight64(local_hash, (num_pivots + 53u) % 64u) * local_hash;
  }

  local_hash ^=
      RotateRight64(local_hash, (columns.Size() + 43u) % 64u) * local_hash;

  local_hash ^=
      RotateRight64(local_hash, (joined_views.Size() + 33u) % 64u) * local_hash;

  hash = local_hash;
  return local_hash;
}

unsigned QueryJoinImpl::Depth(void) noexcept {
  if (depth) {
    return depth;
  }

  auto estimate = 1u;
  for (const auto &[out_col, in_cols] : out_to_in) {
    (void) out_col;
    for (COL *in_col : in_cols) {
      estimate = std::max(estimate, in_col->view->depth);
    }
  }

  depth = estimate + 1u;  // Base case in case of cycles.

  auto real = 1u;
  for (const auto &[out_col, in_cols] : out_to_in) {
    (void) out_col;
    real = GetDepth(in_cols, real);
  }

  depth = real + 1u;

  return depth;
}

// Convert a trivial join (only has a single input view) into a TUPLE. With a
// single joined view there are no pivot sets spanning multiple views, so the
// JOIN performs no matching: every output column forwards exactly one input
// column, or a constant. A TUPLE expresses that forwarding directly, carries
// no pivot bookkeeping, and is a better candidate for later merging and
// elimination.
//
//    create TUPLE with one output column per JOIN output column
//    for each JOIN output column:
//      if it maps to an input column: TUPLE forwards that input column
//      else (constant output):        TUPLE forwards the constant
//    replace all uses of the JOIN with the TUPLE
//
//   Before:                          After:
//     V[a b]                           V[a b]
//       |                                |
//     JOIN[out=(a, b)]                 TUPLE[out=(a, b)]
//       |                                |
//     users                            users
void QueryJoinImpl::ConvertTrivialJoinToTuple(QueryImpl *impl) {
  TUPLE * const tuple = impl->tuples.Create();
  tuple->color = color;

  auto col_index = 0u;
  for (auto out_col : columns) {
    const auto new_out_col = tuple->columns.Create(
        out_col->var, out_col->type, tuple, out_col->id, col_index++);
    new_out_col->CopyConstantFrom(out_col);
  }

  UseList<COL> new_tuple_inputs(tuple);
  for (COL *out_col : columns) {
    auto in_cols_it = out_to_in.find(out_col);
    if (in_cols_it != out_to_in.end()) {
      const auto &in_cols = in_cols_it->second;
      assert(in_cols.Size() == 1u);
      new_tuple_inputs.AddUse(in_cols[0]);

    } else if (auto const_col = out_col->AsConstant(); const_col) {
      new_tuple_inputs.AddUse(const_col);

    } else {
      assert(false);
    }
  }

  ReplaceAllUsesWith(tuple);
#ifndef NDEBUG
  producer += "->DEAD:TO-TRIVIAL-TUPLE";
#endif
  tuple->input_columns.Swap(new_tuple_inputs);
}

// Returns `true` if any joined views were identified where one or more of
// their columns are not used by the JOIN. If so, we proxy those views with
// TUPLEs. A JOIN implicitly carries every column of every joined view, so a
// joined view column that is neither a pivot nor demanded by any user of the
// JOIN only widens the data flowing through the JOIN. Interposing a TUPLE
// that forwards just the needed columns narrows each such input, and the
// JOIN's own output shape is rebuilt without the unused non-pivot columns.
// This does not run when the JOIN is used directly as a whole view (e.g. by
// a MERGE), because those uses depend on the JOIN keeping its exact column
// shape.
//
//    if the JOIN is used directly as a view: return false
//    needed = all pivot input/output columns
//           + non-pivot input/output columns whose output is used
//    if every input view column is needed: return false
//    for each joined view with a column not in needed:
//      interpose TUPLE forwarding only its needed columns
//    rebuild columns/out_to_in, keeping pivots and used non-pivots
//
//   Before (b unused by any user):    After:
//     U[x]      V[x b]                  U[x]     V[x b]
//        \      /                         \        |
//     JOIN[P={U.x,V.x},                    \    TUPLE[x]
//          out=(P, b)]                      \    /
//          |                            JOIN[P={U.x,T.x}, out=(P)]
//     users of P only                        |
//                                          users
bool QueryJoinImpl::ProxyUnusedInputColumns(QueryImpl *impl) {
  const auto is_used_in_merge = IsUsedDirectly();
  if (is_used_in_merge) {
    return false;
  }

  auto has_unused_cols = false;
  auto num_cols = 0u;

  // Look to see if any of the non-pivot output columns of the JOIN are unused.
  for (const auto &[out_col, in_cols] : out_to_in) {
    const auto is_pivot = out_col->Index() < num_pivots;
    if (is_pivot) {
      assert(1u < in_cols.Size());
      num_cols += in_cols.Size();
      continue;
    } else if (!out_col->IsUsedIgnoreMerges()) {
      has_unused_cols = true;
      break;
    } else {
      num_cols += 1u;
    }
  }

  // Look to see if any of the columns of any of the input joined views aren't
  // represented by the JOIN.
  if (!has_unused_cols) {
    auto num_expected_cols = 0u;
    for (auto joined_view : joined_views) {
      num_expected_cols += joined_view->columns.Size();
    }

    has_unused_cols = num_cols < num_expected_cols;
  }

  if (!has_unused_cols) {
    return false;
  }

  std::unordered_map<COL *, bool> needed_cols;

  for (const auto &[out_col, in_cols] : out_to_in) {
    const auto is_pivot = out_col->Index() < num_pivots;
    if (is_pivot) {
      for (auto in_col : in_cols) {
        needed_cols.emplace(in_col, true);
      }
      needed_cols.emplace(out_col, true);

    } else if (out_col->IsUsedIgnoreMerges()) {
      assert(in_cols.Size() == 1u);
      needed_cols.emplace(in_cols[0], true);
      needed_cols.emplace(out_col, true);
    }
  }

  WeakUseList<VIEW> new_joined_views(this);
  std::unordered_map<COL *, COL *> col_map;

  for (auto joined_view : joined_views) {
    has_unused_cols = false;
    for (auto in_col : joined_view->columns) {
      if (!needed_cols[in_col]) {
        has_unused_cols = true;
      } else {
        col_map.emplace(in_col, in_col);
      }
    }

    if (!has_unused_cols) {
      new_joined_views.AddUse(joined_view);
      continue;
    }

    auto tuple = impl->tuples.Create();
    tuple->color = color;
    auto col_index = 0u;
    for (auto in_col : joined_view->columns) {
      if (needed_cols[in_col]) {
        auto new_in_col = tuple->columns.Create(in_col->var, in_col->type,
                                                tuple, in_col->id, col_index++);
        new_in_col->CopyConstantFrom(in_col);
        tuple->input_columns.AddUse(in_col);
        col_map[in_col] = new_in_col;
      }
    }

    joined_view->CopyDifferentialAndGroupIdsTo(tuple);
    new_joined_views.AddUse(tuple);
  }

  std::unordered_map<COL *, UseList<COL>> new_out_to_in;
  DefList<COL> new_columns;

  auto col_index = 0u;
  for (auto out_col : columns) {
    const auto &in_cols = out_to_in.find(out_col)->second;
    const auto is_pivot = out_col->Index() < num_pivots;
    UseList<COL> new_in_cols(this);
    if (is_pivot) {
      for (auto in_col : in_cols) {
        new_in_cols.AddUse(col_map[in_col]);
      }
      auto new_out_col = new_columns.Create(out_col->var, out_col->type, this,
                                            out_col->id, col_index++);
      new_out_to_in.emplace(new_out_col, std::move(new_in_cols));
      out_col->ReplaceAllUsesWith(new_out_col);

    } else if (out_col->IsUsedIgnoreMerges()) {
      auto new_out_col = new_columns.Create(out_col->var, out_col->type, this,
                                            out_col->id, col_index++);
      new_in_cols.AddUse(col_map[in_cols[0]]);
      new_out_to_in.emplace(new_out_col, std::move(new_in_cols));
      out_col->ReplaceAllUsesWith(new_out_col);
    }
  }

  columns.Swap(new_columns);
  joined_views.Swap(new_joined_views);
  out_to_in.swap(new_out_to_in);
  return true;
}

// Put this join into a canonical form, which will make comparisons and
// replacements easier. A JOIN equates each pivot column across all of its
// joined views and forwards every other column unchanged, so several shapes
// admit a strictly simpler equivalent: an input column feeding two output
// columns makes those outputs identical, so all but one are redundant; a
// joined view with columns the JOIN never uses can be narrowed by
// ProxyUnusedInputColumns; and a JOIN over a single view performs no
// matching at all and becomes a TUPLE via ConvertTrivialJoinToTuple. Each
// rewrite shrinks the JOIN's columns or removes the JOIN, exposing further
// optimization (e.g. CSE of identical views, dead node removal) to the
// surrounding fixpoint driver.
//
// Pivot columns are never removed, even when their value is a known
// constant: a pivot's column edges are what express that a joined row's
// presence depends on EVERY joined view holding a matching row (the
// keep-last-edge rule, applied to joins). This also keeps the constant
// `true` pivot of a desugared condition test (a JOIN against a SELECT of a
// unit `is_condition` relation) intact. Constant knowledge still propagates
// through: a pivot fed by a constant column marks its output column as a
// constant reference, because the join equates every view's pivot column
// with the constant-valued one at runtime.
//
//    if out_to_in is empty:        unlink; the JOIN is dead
//    if dead/unsat/invalid:        do nothing
//    if any joined view is unsat:  mark this JOIN unsatisfiable
//    propagate constants from input columns to output columns
//    if two output columns share an input column:
//      redirect users of the later duplicates to the first output,
//      rebuild the JOIN with unique columns (recounting pivots), put a
//      TUPLE above it restoring the original shape, re-canonicalize
//    if allowed and profitable:          ProxyUnusedInputColumns
//    else if only one joined view:       ConvertTrivialJoinToTuple
//    else: the JOIN is canonical
//
// Duplicate output elimination (two outputs read V.b):
//
//   Before:                          After:
//     U[a]      V[a b]                 U[a]      V[a b]
//        \      /                         \      /
//     JOIN[P={U.a,V.a},               JOIN[P={U.a,V.a},
//          out=(P, b, b)]                  out=(P, b)]
//          |                               |
//        users                        TUPLE[out=(P, b, b)]
//                                          |
//                                        users
//
// TODO(pag): If *all* incoming columns for a pivot column are the same, then
//            it no longer needs to be a pivot column.
//
// TODO(pag): If we make the above transform, then a JOIN could devolve into
//            a cross-product.
bool QueryJoinImpl::Canonicalize(QueryImpl *query,
                                   const OptimizationContext &opt,
                                   const ErrorLog &log) {

  if (out_to_in.empty()) {
    PrepareToDelete();
#ifndef NDEBUG
      producer += "->DEAD:EMTPY-OUT-TO-IN";
#endif
    return false;
  }

  if (is_dead || is_unsat || valid != kValid) {
    is_canonical = true;
    return false;
  }

  for (VIEW *incoming_view : joined_views) {
    if (incoming_view && incoming_view->is_unsat) {
      MarkAsUnsatisfiable();
      return true;
    }
  }

  is_canonical = false;

  in_to_out.clear();

  auto has_repeated_inputs = false;

  for (COL *out_col : columns) {
    COL *const_col = out_col->AsConstant();
    for (COL *in_col : out_to_in.at(out_col)) {
      if (auto [it, added] = in_to_out.emplace(in_col, out_col); !added) {
        out_col->ReplaceAllUsesWith(it->second);
        has_repeated_inputs = true;
        is_canonical = false;
      }
      if (!const_col) {
        if (auto in_const_col = in_col->AsConstant(); in_const_col) {
          out_col->CopyConstantFrom(in_const_col);
          const_col = in_const_col;
        }
      }
    }
  }

  // There are repeats of inputs, get rid of them.
  if (has_repeated_inputs) {

    // First, we need a tuple that will forward all columns as they previously
    // were.
    TUPLE * const tuple = query->tuples.Create();
    for (COL *out_col : columns) {
      (void) tuple->columns.Create(out_col->var, out_col->type, tuple,
                                   out_col->id, out_col->Index());
    }

    SubstituteAllUsesWith(tuple);

    DefList<COL> new_columns(this);
    std::unordered_map<COL *, UseList<COL>> new_out_to_in;
    unsigned new_num_pivots = 0u;

    // Now that all uses have been replaced, we can make our proxy tuple use
    // the new columns that we will create that won't have any repeated input
    // columns.
    for (COL *out_col : columns) {
      auto &in_cols = out_to_in.at(out_col);
      COL * const first_out_col = in_to_out[in_cols[0]];
      assert(first_out_col != nullptr);
      COL *&new_out_col = in_to_out[first_out_col];
      if (!new_out_col) {
        if (1u < in_cols.Size()) {
          ++new_num_pivots;
        }
        new_out_col = new_columns.Create(out_col->var, out_col->type, this,
                                         out_col->id, out_col->Index());
        new_out_to_in.emplace(new_out_col, std::move(in_cols));
      }

      tuple->input_columns.AddUse(new_out_col);
    }

    // Swap in the new input/output columns.
    columns.Swap(new_columns);
    out_to_in.swap(new_out_to_in);
    std::swap(num_pivots, new_num_pivots);
    Canonicalize(query, opt, log);
    return true;
  }

  if (opt.can_remove_unused_columns && ProxyUnusedInputColumns(query)) {
    return true;
  }

  // There's only one incoming view, convert this into a tuple.
  if (joined_views.Size() == 1u) {
    ConvertTrivialJoinToTuple(query);
    return true;
  }

  is_canonical = true;
  return false;
}

// Equality over joins is pointer-based.
bool QueryJoinImpl::Equals(EqualitySet &eq, QueryViewImpl *that_) noexcept {
  if (eq.Contains(this, that_)) {
    return true;
  }

  const auto that = that_->AsJoin();
  if (!that || columns.Size() != that->columns.Size() ||
      num_pivots != that->num_pivots ||
      out_to_in.size() != that->out_to_in.size() ||
      joined_views.Size() != that->joined_views.Size() ||
      InsertSetsOverlap(this, that)) {
    return false;
  }

  eq.Insert(this, that);

  // Check that we've joined together the right views.
  const auto num_joined_views = joined_views.Size();
  auto i = 0u;
  for (; i < num_joined_views; ++i) {
    if (!joined_views[i]->Equals(eq, that->joined_views[i])) {
      eq.Remove(this, that);
      return false;
    }
  }

  // Check that the columns are joined together in the same way.
  i = 0u;
  for (const auto j1_out_col : columns) {
    assert(j1_out_col->index == i);

    const auto j2_out_col = that->columns[i];
    assert(j2_out_col->index == i);
    ++i;

    const auto j1_in_cols = out_to_in.find(j1_out_col);
    const auto j2_in_cols = that->out_to_in.find(j2_out_col);

    if (j1_in_cols == out_to_in.end() ||  // Join not used.
        j2_in_cols == that->out_to_in.end() ||  // Join not used.
        !ColumnsEq(eq, j1_in_cols->second, j2_in_cols->second)) {
      eq.Remove(this, that);
      return false;
    }
  }

  return true;
}

}  // namespace hyde
