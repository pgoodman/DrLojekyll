// Copyright 2020, Trail of Bits. All rights reserved.

#include "Build.h"

namespace hyde {

// Build a top-down checker on a select.
REGION *BuildTopDownSelectChecker(ProgramImpl *impl, Context &context,
                                  REGION *proc, QuerySelect select,
                                  std::vector<QueryColumn> &view_cols,
                                  TABLE *already_checked) {

  const QueryView view(select);

  // Organize the checking so that we check the non-differential predecessors
  // first, then the differential predecessors.
  const auto seq = impl->series_regions.Create(proc);
  const auto par_normal = impl->parallel_regions.Create(seq);
  const auto par_diff = impl->parallel_regions.Create(seq);
  seq->AddRegion(par_normal);
  seq->AddRegion(par_diff);

  // The base case is that we get to a SELECT from a stream. In this case,
  // there are no predecessors, and so we'll fall through to this
  // `return-false`. We treat data received as ephemeral, and so there is no
  // way to actually check if the tuple exists, and so we treat it as not
  // existing. If this is a differential stream, then we will have removed
  // the data from the corresponding table, and so we must be in a situation
  // where we've already checked in the table and the data is gone, hence we
  // can only return false.
  seq->AddRegion(BuildStateCheckCaseReturnFalse(impl, seq));

  auto do_rec_check = [&](QueryView pred_view, PARALLEL *parent) -> REGION * {
    return CallTopDownChecker(
        impl, context, parent, view, view_cols, pred_view, already_checked,
        [=](REGION *parent_if_true) -> REGION * {
          return BuildStateCheckCaseReturnTrue(impl, parent_if_true);
        },
        [](REGION *) -> REGION * { return nullptr; });
  };

  // The predecessors of a `SELECT` are inserts. `SELECT`s don't have input
  // nodes, and `INSERT`s don't have output nodes. The data flow guarantees
  // that every `INSERT` is preceded by a `TUPLE` with matching columns. So
  // to check a `SELECT`, we go and find these preceding `TUPLE`s and check
  // them.
  for (auto pred_view : view.Predecessors()) {
    assert(pred_view.IsInsert());
    const auto insert = QueryInsert::From(pred_view);
    const auto insert_pred = pred_view.Predecessors()[0];
    assert(insert_pred.IsTuple());

    // A witness-bearing INSERT reads more columns than it stores: its
    // attached columns keep a read edge to `insert_pred` without being
    // persisted in the relation, so the stored column values alone don't
    // identify a row of `insert_pred`. The requested tuple is present if
    // any surviving row of `insert_pred` matches it on the stored columns:
    // scan `insert_pred`'s table with an empty bound prefix (a full scan),
    // compare each row's stored columns against the requested values, and
    // recursively re-prove matching rows.
    if (insert.NumAttachedColumns()) {
      DataModel *const pred_model =
          impl->view_to_model[insert_pred]->FindAs<DataModel>();
      TABLE *const pred_table = pred_model->table;
      assert(pred_table != nullptr);

      PARALLEL *const par =
          insert_pred.CanReceiveDeletions() ? par_diff : par_normal;
      SERIES *const scan_seq = impl->series_regions.Create(par);
      par->AddRegion(scan_seq);

      COMMENT(scan_seq->comment = __FILE__
              ": BuildTopDownSelectChecker witness-insert scan";)

      std::vector<QueryColumn> scan_cols;  // Empty bound prefix.
      BuildMaybeScanPartial(
          impl, insert_pred, scan_cols, pred_table, scan_seq,
          [&](REGION *in_scan, bool in_loop) -> REGION * {
            assert(in_loop);
            (void) in_loop;

            // The scanned row proves the requested tuple only if its
            // stored columns match the requested values.
            TUPLECMP *const stored_cmp =
                impl->operation_regions.CreateDerived<TUPLECMP>(
                    in_scan, ComparisonOperator::kEqual);

            auto col_index = 0u;
            for (auto col : select.Columns()) {
              const QueryColumn in_col = insert.NthInputColumn(col_index++);
              stored_cmp->lhs_vars.AddUse(in_scan->VariableFor(impl, in_col));
              stored_cmp->rhs_vars.AddUse(in_scan->VariableFor(impl, col));
            }

            // `scan_cols` now holds all of `insert_pred`'s columns; any
            // surviving row proves presence.
            const auto [rec_check, rec_check_call] =
                CallTopDownChecker(impl, context, stored_cmp, insert_pred,
                                   scan_cols, insert_pred, nullptr);
            stored_cmp->body.Emplace(stored_cmp, rec_check);
            rec_check_call->body.Emplace(
                rec_check_call,
                BuildStateCheckCaseReturnTrue(impl, rec_check_call));

            return stored_cmp;
          });
      continue;
    }

    std::vector<QueryColumn> insert_cols;

    for (auto col : select.Columns()) {
      const QueryColumn in_col = insert.InputColumns()[*(col.Index())];
      assert(QueryView::Containing(in_col) == insert_pred);
      assert(*(in_col.Index()) == *(col.Index()));
      insert_cols.push_back(in_col);
      proc->col_id_to_var[in_col.Id()] = proc->VariableFor(impl, col);
    }

    // If the predecessor can produce deletions, then check it in `par_diff`.
    if (insert_pred.CanReceiveDeletions()) {
      const auto rec_check = do_rec_check(pred_view, par_diff);
      par_diff->AddRegion(rec_check);

      COMMENT(rec_check->comment = __FILE__
              ": BuildTopDownSelectChecker call differential pred";)

    // If the predecessor can't produce deletions, then check it in
    // `par_normal`.
    } else {
      const auto rec_check = do_rec_check(pred_view, par_normal);
      par_normal->AddRegion(rec_check);

      COMMENT(rec_check->comment =
                  __FILE__ ": BuildTopDownSelectChecker call normal pred";)
    }
  }

  return seq;
}

}  // namespace hyde
