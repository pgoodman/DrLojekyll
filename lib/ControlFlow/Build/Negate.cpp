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

    // The negation's own table is deletion-capable (a NEGATE retracts when
    // its negated view gains a row), so it is a differential table whose
    // non-inductive consumers run in the table's stratum phases. Those
    // phases drain the table's add queue, which must be seeded here at the
    // fold's zero crossing — the eager-insertion seeder (Build.cpp) is
    // skipped for this table because `continue_negation` already folded it,
    // so its recursive InTryInsert is a no-op (parent == parent_).
    //
    // A recursive (InductionGroupId) negate does NOT reach here: it can
    // produce deletions ⇒ its eager successor edge is cut (Build.cpp), so the
    // eager walk stops before this negate and its differential phases (D/R/I)
    // own it. The old InductionGroupId branch (an eager-induction defer via
    // GetOrInitInduction) was therefore dead — verified 0/178 corpus files
    // reached it, IR byte-identical without it — and is removed (D5', R8-iv).
    if (table != nullptr && succ_parent != let &&
        TableIsDifferential(table)) {
      PARALLEL *const par = impl->parallel_regions.Create(succ_parent);
      succ_parent->body.Emplace(succ_parent, par);
      par->AddRegion(AppendViewTupleToVector(
          impl, par, negate_view,
          TableDeltaVector(impl, context, table, VectorKind::kAddQueue)));
      LET *const cont_let = impl->operation_regions.CreateDerived<LET>(par);
      par->AddRegion(cont_let);
      succ_parent = cont_let;
    }

    BuildEagerInsertionRegions(impl, negate_view, context, succ_parent,
                               negate_view.Successors(), last_table);
  };

  // The forward pass of negation maintenance: one membership read of the
  // negated view's table (unit condition relations included — the token row
  // `(true)` is a row like any other). The tuple flows through the negation
  // only when the negated key is absent.
  //
  // The read is POSITION-KEYED, not count-based (D1'', R1): a `!` negate reads
  // the negated table batch-frozen (`kInI`, the §5.4 seed schema — the eager
  // gate runs mid-ingest where the count-based `kPresent`/`kInNew` depends on
  // parallel arm order, but `kInI` is the sealed watermark, order-independent;
  // D2' seals the negated table per batch so this read is well-defined). A
  // @never negate keeps `kPresent`: it never retracts, gets no crossover, and
  // its negated table is not sealed — so its gate stays count-based. Because
  // the two predicates now differ, a `!` and a @never gate over the SAME
  // negated table are NOT CSE'd (R8-i).
  DataModel *const negated_model =
      impl->view_to_model[negated_view]->FindAs<DataModel>();
  TABLE *const negated_table = negated_model->table;
  assert(negated_table != nullptr);

  CHECKMEMBER *const gate = BuildCheckMember(
      impl, parent, negated_table, negated_view_cols,
      negate.HasNeverHint() ? MembershipPredicate::kPresent
                            : MembershipPredicate::kInI,
      [](ProgramImpl *, REGION *) -> REGION * { return nullptr; },
      [&](ProgramImpl *impl_, REGION *in_check) -> REGION * {
        OP *const let = impl_->operation_regions.CreateDerived<LET>(in_check);
        continue_negation(let);
        return let;
      });
  parent->body.Emplace(parent, gate);
}

}  // namespace hyde
