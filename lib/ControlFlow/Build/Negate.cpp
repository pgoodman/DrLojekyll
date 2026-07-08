// Copyright 2020, Trail of Bits. All rights reserved.

#include "Induction.h"

namespace hyde {

// Build an eager region for testing the absence of some data in another view.
void BuildEagerNegateRegion(ProgramImpl *impl, QueryView pred_view,
                            QueryNegate negate, Context &context, OP *parent_,
                            TABLE *last_table_) {

  // NOTE(pag): NEGATEs are like simple JOINs, but instead of matching in
  //            another table, we don't want to match in another table. Thus,
  //            data must be present in both sides of the negation, similar to
  //            what is needed for it being required in both sides of a JOIN.
  auto [parent, pred_table, _] =
      InTryInsert(impl, context, pred_view, parent_, last_table_);

  const QueryView negate_view(negate);
  const QueryView negated_view = negate.NegatedView();
  std::vector<QueryColumn> negated_view_cols;
  for (QueryColumn out_col : negate.NegatedColumns()) {
    const auto i = *(out_col.Index());
    const auto neg_col = negated_view.NthColumn(i);
    VAR *out_col_var = parent->VariableFor(impl, out_col);
    assert(out_col_var != nullptr);
    parent->col_id_to_var[neg_col.Id()] = out_col_var;
    negated_view_cols.push_back(neg_col);
  }

  // The absence of the negated tuple is established: pass the data through
  // the negation and onward to its successors.
  //
  // NOTE(pag): A negation can never share the same data model as its
  //           predecessor, as it might not pass through all of its
  //           predecessor's data.
  auto continue_negation = [&](OP *let) {
    auto [succ_parent, table, last_table] =
        InTryInsert(impl, context, negate_view, let, nullptr);
    (void) table;

    // If this is an inductive negation, then we might defer processing its
    // outputs until we get into a successor.
    if (negate_view.InductionGroupId().has_value()) {
      INDUCTION *const induction =
          GetOrInitInduction(impl, negate_view, context, succ_parent);
      if (NeedsInductionCycleVector(negate_view)) {
        AppendToInductionInputVectors(impl, negate_view, negate_view, context,
                                      succ_parent, induction);
        return;
      }
    }

    BuildEagerInsertionRegions(impl, negate_view, context, succ_parent,
                               negate_view.Successors(), last_table);
  };

  // The forward pass of negation maintenance: one membership read of the
  // negated view's table (unit condition relations included — the token row
  // `(true)` is a row like any other). The tuple flows through the negation
  // only when the negated key is absent.
  DataModel *const negated_model =
      impl->view_to_model[negated_view]->FindAs<DataModel>();
  TABLE *const negated_table = negated_model->table;
  assert(negated_table != nullptr);

  CHECKMEMBER *const gate = BuildCheckMember(
      impl, parent, negated_table, negated_view_cols,
      MembershipPredicate::kPresent,
      [](ProgramImpl *, REGION *) -> REGION * { return nullptr; },
      [&](ProgramImpl *impl_, REGION *in_check) -> REGION * {
        OP *const let = impl_->operation_regions.CreateDerived<LET>(in_check);
        continue_negation(let);
        return let;
      });
  parent->body.Emplace(parent, gate);
}

}  // namespace hyde
