// Copyright 2020, Trail of Bits. All rights reserved.

#include "Build.h"

namespace hyde {

// Build an eager region for a `QueryMerge` that is NOT part of an inductive
// loop, and thus passes on its data to the next thing down as long as that
// data is unique.
//
// NOTE(pag): These merges could actually be part of an induction set, but
//            really the induction loop belongs to another merge which dominates
//            this merge.
void BuildEagerUnionRegion(ProgramImpl *impl, QueryView pred_view,
                           QueryMerge merge, Context &context, OP *parent_,
                           TABLE *last_table_) {
  const QueryView view(merge);
  auto [parent, table, last_table] =
      InTryInsert(impl, context, view, parent_, last_table_);

#ifndef NDEBUG

  // A column of a multi-predecessor MERGE may be a constant reference only
  // when every predecessor supplies that same constant for the column. Every
  // predecessor then inserts the identical value, so downstream reads that
  // resolve the column to its constant observe the same data as reads of the
  // materialized column. A constant reference fed by predecessors that could
  // supply differing values would substitute one constant for all of them and
  // produce wrong data.
  if (1u < view.Predecessors().size()) {
    for (auto col : view.Columns()) {
      if (col.IsConstantRef()) {
        const auto const_col = col.AsConstantColumn();
        for (auto pred : view.Predecessors()) {
          const auto pred_col = pred.NthColumn(*(col.Index()));
          assert(pred_col.IsConstantOrConstantRef());
          assert(pred_col.AsConstantColumn() == const_col);
        }
      }
    }
  }
#endif

  BuildEagerInsertionRegions(impl, view, context, parent, view.Successors(),
                             last_table);
}

}  // namespace hyde
