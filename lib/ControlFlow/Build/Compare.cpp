// Copyright 2020, Trail of Bits. All rights reserved.

#include "Build.h"

namespace hyde {
namespace {

// Build an eager region for performing a comparison.
std::pair<TUPLECMP *, OP *>
CreateCompareRegion(ProgramImpl *impl, QueryCompare view, Context &context,
                    REGION *parent) {
  auto lhs_var = parent->VariableFor(impl, view.InputLHS());
  auto rhs_var = parent->VariableFor(impl, view.InputRHS());

  if (view.Operator() == ComparisonOperator::kEqual) {
    auto cmp = impl->operation_regions.CreateDerived<TUPLECMP>(
        parent, ComparisonOperator::kEqual);

    cmp->lhs_vars.AddUse(lhs_var);
    cmp->rhs_vars.AddUse(rhs_var);

    auto do_lhs = [&](void) {
      cmp->col_id_to_var[view.InputLHS().Id()] = lhs_var;
      cmp->col_id_to_var[view.InputRHS().Id()] = lhs_var;
      cmp->col_id_to_var[view.LHS().Id()] = lhs_var;
      cmp->col_id_to_var[view.RHS().Id()] = lhs_var;
    };

    auto do_rhs = [&](void) {
      cmp->col_id_to_var[view.InputLHS().Id()] = rhs_var;
      cmp->col_id_to_var[view.InputRHS().Id()] = rhs_var;
      cmp->col_id_to_var[view.LHS().Id()] = rhs_var;
      cmp->col_id_to_var[view.RHS().Id()] = rhs_var;
    };

    if (DataVariable(lhs_var).IsConstant()) {
      do_lhs();

    } else if (DataVariable(rhs_var).IsConstant()) {
      do_rhs();

    } else if (lhs_var->id < rhs_var->id) {
      do_lhs();

    } else {
      do_rhs();
    }

    return {cmp, cmp};

  // Make not-equals look like equals so that we can better merge them.
  } else if (view.Operator() == ComparisonOperator::kNotEqual) {
    auto cmp = impl->operation_regions.CreateDerived<TUPLECMP>(
        parent, ComparisonOperator::kEqual);

    cmp->lhs_vars.AddUse(lhs_var);
    cmp->rhs_vars.AddUse(rhs_var);

    cmp->col_id_to_var[view.LHS().Id()] = lhs_var;
    cmp->col_id_to_var[view.InputLHS().Id()] = lhs_var;

    cmp->col_id_to_var[view.RHS().Id()] = rhs_var;
    cmp->col_id_to_var[view.InputRHS().Id()] = rhs_var;

    auto let = impl->operation_regions.CreateDerived<LET>(cmp);
    cmp->false_body.Emplace(cmp, let);

    return {cmp, let};

  } else {
    auto cmp = impl->operation_regions.CreateDerived<TUPLECMP>(parent,
                                                               view.Operator());
    cmp->lhs_vars.AddUse(lhs_var);
    cmp->rhs_vars.AddUse(rhs_var);

    cmp->col_id_to_var[view.LHS().Id()] = lhs_var;
    cmp->col_id_to_var[view.InputLHS().Id()] = lhs_var;

    cmp->col_id_to_var[view.RHS().Id()] = rhs_var;
    cmp->col_id_to_var[view.InputRHS().Id()] = rhs_var;

    return {cmp, cmp};
  }
}

}  // namespace

// Build an eager region for performing a comparison.
void BuildEagerCompareRegions(ProgramImpl *impl, QueryCompare cmp,
                              Context &context, OP *parent) {
  const QueryView view(cmp);
  const auto [check, body] = CreateCompareRegion(impl, cmp, context, parent);
  parent->body.Emplace(parent, check);

  // NOTE(pag): A compare will never share the data model of its predecessor,
  //            otherwise it would be too accepting.
  BuildEagerInsertionRegions(impl, view, context, body, view.Successors(),
                             nullptr);
}

}  // namespace hyde
