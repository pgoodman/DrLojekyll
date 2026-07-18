// Copyright 2026, Peter Goodman. All rights reserved.
// Copyright 2020, Trail of Bits. All rights reserved.

#include "Build.h"

namespace hyde {

// Build the generator call for a MAP view's functor application, binding
// its bound parameters (from the data flow when `bottom_up`, from the
// output columns otherwise) and defining its free parameters.
GENERATOR *CreateGeneratorCall(ProgramImpl *impl, QueryMap view,
                               ParsedFunctor functor, Context &context,
                               REGION *parent, bool bottom_up) {
  std::vector<QueryColumn> input_cols;
  std::vector<QueryColumn> output_cols;

  auto gen = impl->operation_regions.CreateDerived<GENERATOR>(parent, functor,
                                                              impl->next_id++);
  auto i = 0u;

  // Deal with the functor inputs and outputs.
  for (auto j = 0u, num_cols = functor.Arity(); i < num_cols; ++i) {
    const auto out_col = view.NthColumn(i);

    // Outputs correspond to `free`-attributed parameters.
    if (functor.NthParameter(i).Binding() == ParameterBinding::kFree) {
      const auto out_var = gen->defined_vars.Create(
          impl->next_id++, VariableRole::kFunctorOutput);
      out_var->query_column = out_col;
      gen->col_id_to_var[out_col.Id()] = out_var;

    // Inputs correspond to `bound`-attributed parameters.
    } else {
      assert(functor.NthParameter(i).Binding() == ParameterBinding::kBound);

      const auto in_col = view.NthInputColumn(j++);

      VAR *in_var = nullptr;
      if (bottom_up) {
        in_var = parent->VariableFor(impl, in_col);
        gen->col_id_to_var[out_col.Id()] = in_var;
      } else {
        in_var = parent->VariableFor(impl, out_col);
        gen->col_id_to_var[in_col.Id()] = in_var;
      }

      gen->used_vars.AddUse(in_var);
      if (!in_var->query_column) {
        in_var->query_column = in_col;
      }
      if (bottom_up && !in_var->query_const &&
          in_col.IsConstantOrConstantRef()) {
        in_var->query_const = QueryConstant::From(in_col);
      }
    }
  }

  // Deal with the copied/attached columns, which emulate lexical scope. Here
  // we turn them back into actual lexical scope :-D
  for (auto j = 0u, num_cols = view.Columns().size(); i < num_cols; ++i, ++j) {
    auto out_col = view.NthCopiedColumn(j);
    auto in_col = view.NthInputCopiedColumn(j);
    VAR *in_var = nullptr;

    if (bottom_up) {
      in_var = parent->VariableFor(impl, in_col);
      gen->col_id_to_var[out_col.Id()] = in_var;
    } else {
      in_var = parent->VariableFor(impl, out_col);
      gen->col_id_to_var[in_col.Id()] = in_var;
    }

    if (!in_var->query_column) {
      in_var->query_column = in_col;
    }
    if (in_col.IsConstantOrConstantRef() && !in_var->query_const) {
      in_var->query_const = QueryConstant::From(in_col);
    }
  }

  return gen;
}

// Build an eager region for a `QueryMap`.
void BuildEagerGenerateRegion(ProgramImpl *impl, QueryView pred_view,
                              QueryMap map, Context &context, OP *parent_,
                              TABLE *last_table_) {
  const QueryView view(map);

  const auto functor = map.Functor();
  assert(functor.IsPure());

  auto [parent, pred_table, _] =
      InTryInsert(impl, context, pred_view, parent_, last_table_);

  // TODO(pag): Think about requiring persistence of the predecessor, so that
  //            we always have the inputs persisted.

  const auto gen =
      CreateGeneratorCall(impl, map, functor, context, parent, true);
  parent->body.Emplace(parent, gen);

  // If we're dealing with a negated generator, then make sure that children
  // end up in the `empty_body`.
  if (!map.IsPositive()) {
    parent = impl->operation_regions.CreateDerived<LET>(gen);
    gen->empty_body.Emplace(gen, parent);

  // In the positive case, child nodes will put themselves into `parent->body`.
  } else {
    parent = gen;
  }

  // NOTE(pag): A generator will never share the data model of its predecessor,
  //            otherwise it would be too accepting.
  BuildEagerInsertionRegions(impl, view, context, parent, view.Successors(),
                             nullptr);
}

}  // namespace hyde
