// Copyright 2020, Trail of Bits. All rights reserved.

#include "Build.h"

#include <drlojekyll/Parse/ErrorLog.h>
#include <drlojekyll/Parse/ModuleIterator.h>

#include <algorithm>
#include <sstream>

namespace hyde {
namespace {

// Figure out what data definitely must be stored persistently. We need to do
// this ahead-of-time, as opposed to just-in-time, because otherwise we run
// into situations where a node N will have two successors S1 and S2, e.g.
// two identically-shaped TUPLEs, and those tuples will feed into JOINs.
// Those tuples will both need persistent storage (because they feed JOINs),
// but we'll only observe this when we are generating code for the JOINs, and
// thus when we generate the state changes for the join, we'll generate two
// identical state changes, where one will make the other one unsatisfiable.
// For example:
//
//      join-tables
//        vector-loop ...
//        select ...
//        select ...
//          par
//            if-transition-state {@A:29} in %table:43[...] from ...
//              ...
//            if-transition-state {@A:29} in %table:43[...] from ...
//              ...
static void FillDataModel(const Query &query, ProgramImpl *impl,
                          Context &context) {

  query.ForEachView([&](QueryView view) {
    // Every predecessor of a deletion-receiving view needs persistent
    // storage: differential maintenance enumerates the rule instances that
    // stop or start firing by joining a changed predecessor's frontier
    // against the other predecessors' tables, so each predecessor's rows
    // must be persisted.
    if (view.CanReceiveDeletions()) {
      for (auto pred : view.Predecessors()) {
        (void) TABLE::GetOrCreate(impl, context, pred);
      }
    }

  });

//  // We will always unique all input data into records, to help kick off
//  // all later record-based analysis.
//  for (auto io : query.IOs()) {
//    for (auto receive : io.Receives()) {
//      (void) TABLE::GetOrCreate(impl, context, receive);
//    }
//  }

  for (auto view : query.Inserts()) {
    auto insert = QueryInsert::From(view);
    if (insert.IsRelation()) {
      (void) TABLE::GetOrCreate(impl, context, view);

      // A witness-bearing INSERT (the setter of a unit condition relation)
      // stores only the `true` token; the token's derivation count is fed
      // by the predecessor's witness rows, so the predecessor must be
      // persisted.
      if (insert.NumAttachedColumns()) {
        (void) TABLE::GetOrCreate(impl, context,
                                  QueryView(view).Predecessors()[0]);
      }
    }
  }

  for (auto view : query.Merges()) {
    if (NeedsInductionCycleVector(view) || NeedsInductionOutputVector(view)) {
      (void) TABLE::GetOrCreate(impl, context, view);
    }
  }

  for (auto join : query.Joins()) {

    QueryView view(join);
    if (!view.CanReceiveDeletions()) {
      auto num_constant = 0u;
      auto num_variable = 0u;
      for (auto pred : join.JoinedViews()) {
        if (pred.IsConstantAfterInitialization()) {
          (void) TABLE::GetOrCreate(impl, context, pred);
          ++num_constant;
        } else {
          ++num_variable;
        }
      }
      if (num_constant && 1u == num_variable) {
        // TODO(pag): Issue #240.
      }
    }

    for (auto pred : join.JoinedViews()) {
      (void) TABLE::GetOrCreate(impl, context, pred);
    }

    // A differential JOIN whose successors do not use every pivot column
    // needs its own output table so its rows can be maintained directly.
    if (view.CanReceiveDeletions()) {

      //      // Easier to just avoid any possible performance issues; storage is
      //      // cheap... right? :-P
      //      (void) TABLE::GetOrCreate(impl, context, view);

      auto num_pivots = join.NumPivotColumns();
      for (auto succ_view : view.Successors()) {
        std::vector<bool> used_pivots(num_pivots);
        auto num_used_pivots = 0u;
        succ_view.ForEachUse([&](QueryColumn in_col, InputColumnRole role,
                                 std::optional<QueryColumn> out_col) {
          if (!in_col.IsConstant() && QueryView::Containing(in_col) == view) {
            if (auto index = *(in_col.Index());
                index < num_pivots && !used_pivots[index]) {
              used_pivots[index] = true;
              ++num_used_pivots;
            }
          }
        });
        if (num_used_pivots < num_pivots) {
          (void) TABLE::GetOrCreate(impl, context, view);
          break;
        }
      }
    }
  }

  for (auto negate : query.Negations()) {
    const QueryView view(negate);
    if (!view.CanReceiveDeletions()) {
      if (negate.NegatedView().IsConstantAfterInitialization()) {
        // TODO(pag): Issue #242.
      }
    }
    (void) TABLE::GetOrCreate(impl, context, negate.NegatedView());
    (void) TABLE::GetOrCreate(impl, context, view.Predecessors()[0]);
  }

  for (auto map : query.Maps()) {
    QueryView view(map);
    if (view.CanProduceDeletions()) {
      (void) TABLE::GetOrCreate(impl, context, view);
    }
  }

  for (auto map : query.Compares()) {
    QueryView view(map);
    if (view.CanProduceDeletions()) {
      (void) TABLE::GetOrCreate(impl, context, view);
    }
  }
}

// Whether `view` lies on a dataflow cycle: DFS over `Successors()` with a
// visited set, true iff the walk re-reaches `view`. Used as the Stage-5
// differential-@product fence: an ACYCLIC 0-pivot differential join is
// lowered by the stratum phases' product arms (`EmitProductArms`), while an
// on-cycle one is rejected — `EmitJoinFire` is not generalized to 0 pivots,
// and the scheduling fixpoint's product clause (a strict `ready_after` lift
// on every side) would ratchet forever against the SCC drain-stratum pin if
// a same-SCC side slipped through.
//
// The test must be reachability, NOT `InductionGroupId().has_value()`: a
// JOIN fully interior to a recursive cycle (no non-inductive predecessors
// and no non-inductive successors, e.g. `t(X,Y) : t(X,A), t(B,Y).`) has its
// induction info RESET by `IdentifyInductions` (lib/DataFlow/Induction.cpp)
// and carries no group id despite being on the cycle.
//
// `Successors()` covers every stored dataflow edge, including the
// INSERT→SELECT relation hop. The one edge it omits is the DataFlow-internal
// negated-view→NEGATE coupling — safe here because a cycle closed only
// through a negated edge is unstratified negation, already rejected by the
// dataflow Stratify pass before `Program::Build` runs.
static bool ViewSelfReachable(QueryView view) {
  std::unordered_set<QueryView> seen;
  std::vector<QueryView> stack;
  for (QueryView succ : view.Successors()) {
    stack.push_back(succ);
  }
  while (!stack.empty()) {
    const QueryView v = stack.back();
    stack.pop_back();
    if (v == view) {
      return true;
    }
    if (!seen.insert(v).second) {
      continue;
    }
    for (QueryView succ : v.Successors()) {
      stack.push_back(succ);
    }
  }
  return false;
}

// Building the data model means figuring out which `QueryView`s can share the
// same backing storage. This doesn't mean that all views will be backed by
// such storage, but when we need backing storage, we can maximally share it
// among other places where it might be needed.
static void BuildDataModel(const Query &query, ProgramImpl *program) {
  std::unordered_map<unsigned, DataModel *> eq_classes;

  query.ForEachView([&](QueryView view) {
    auto model = new DataModel;
    program->models.emplace_back(model);
    program->view_to_model.emplace(view, model);
    eq_classes.emplace(view.EquivalenceSetId(), model);
  });

  query.ForEachView([&](QueryView view) {
    auto curr_model = program->view_to_model[view]->FindAs<DataModel>();
    auto dest_model = eq_classes[view.EquivalenceSetId()];
    DisjointSet::Union(curr_model, dest_model);
  });
}

// Try to build a forcing procedure. We'll re-figure out the relation between
// clause head variables and the forced message variables here, rather than
// trying to wire through all the information.
static std::optional<ProgramProcedure> BuildQueryForceProcedureImpl(
    ProgramImpl *impl, Context &context, ParsedQuery query,
    ParsedClause clause, ParsedPredicate forcing_pred) {

  ParsedDeclaration query_decl(query);
  auto message_decl = ParsedDeclaration::Of(forcing_pred);
  assert(message_decl.IsMessage());
  ParsedMessage message = ParsedMessage::From(message_decl);
  assert(message.IsReceived());

  std::unordered_map<uint64_t, DisjointSet> var_to_set;

  unsigned next_id = 0u;
  auto do_var = [&] (ParsedVariable var) -> DisjointSet * {
    return &(var_to_set.emplace(var.IdInClause(), next_id++).first->second);
  };

  for (ParsedVariable var : clause.Parameters()) {
    do_var(var);
  }

  for (ParsedVariable var : clause.Variables()) {
    do_var(var);
  }

  for (auto g = 0u, max_g = clause.NumGroups(); g < max_g; ++g) {
    for (ParsedComparison cmp : clause.Comparisons(g)) {
      DisjointSet *lhs = do_var(cmp.LHS());
      DisjointSet *rhs = do_var(cmp.RHS());
      if (cmp.Operator() == ComparisonOperator::kEqual) {
        DisjointSet::Union(lhs, rhs);
      }
    }

    // NOTE(pag): We ignore aggregates.
  }

  // Collect the bound parameters of the query.
  std::vector<std::pair<ParsedVariable, ParsedParameter>> bound_query_params;
  for (ParsedParameter query_param : query_decl.Parameters()) {
    if (query_param.Binding() == ParameterBinding::kBound) {
      bound_query_params.emplace_back(
          clause.NthParameter(query_param.Index()), query_param);
    }
  }

  std::unordered_map<ParsedParameter, ParsedParameter>
      message_param_to_query_param;

  // Match up the bound parameters of the query with the arguments to the
  // forcing message.
  auto mp = 0u;
  for (ParsedVariable message_var : forcing_pred.Arguments()) {
    auto message_vs = do_var(message_var);
    ParsedParameter message_param = message.NthParameter(mp++);
    for (auto [query_var, query_param] : bound_query_params) {
      auto query_vs = do_var(query_var);
      if (query_vs->Find() == message_vs->Find()) {
        message_param_to_query_param.emplace(
            message_param, query_param);
        break;
      }
    }
  }

  assert(message_param_to_query_param.size() == message.Arity());

  // Now make the procedure. It will have one argument per bound parameter,
  // create a vector for the message, add in a single tuple to the vector,
  // then it will call the message procedure with the vector.

  auto proc = impl->procedure_regions.Create(
      impl->next_id++, ProcedureKind::kQueryMessageInjector);
  proc->has_raw_use = true;

  // Add in the parameters.
  for (auto [query_var, query_param] : bound_query_params) {
    const auto var =
        proc->input_vars.Create(impl->next_id++, VariableRole::kParameter);
    var->parsed_param = query_param;
  }

  // Define a vector that we can pass to the message.
  std::vector<TypeLoc> col_types;
  for (ParsedVariable message_var : forcing_pred.Arguments()) {
    col_types.push_back(message_var.Type());
  }

  VECTOR *add_vec = proc->vectors.Create(
      impl->next_id++, VectorKind::kParameter, col_types,
      0  /* disambiguation */);
  VECTOR *del_vec = nullptr;

  if (message.IsDifferential()) {
    del_vec = proc->vectors.Create(
        impl->next_id++, VectorKind::kEmpty, col_types, 0);
  }

  SERIES *seq = impl->series_regions.Create(proc);
  proc->body.Emplace(proc, seq);

  VECTORAPPEND *append = impl->operation_regions.CreateDerived<VECTORAPPEND>(
      seq, ProgramOperation::kAppendQueryParamsToMessageInjectVector);
  seq->regions.AddUse(append);
  append->vector.Emplace(append, add_vec);
  for (VAR *param_var : proc->input_vars) {
    append->tuple_vars.AddUse(param_var);
  }

  CALL *call = impl->operation_regions.CreateDerived<CALL>(
      impl->next_id++, seq, context.messsage_handler[message]);
  seq->regions.AddUse(call);
  call->arg_vecs.AddUse(add_vec);
  if (del_vec) {
    call->arg_vecs.AddUse(del_vec);  // Empty.
  }

  RETURN *ret = impl->operation_regions.CreateDerived<RETURN>(
      seq, ProgramOperation::kReturnTrueFromProcedure);
  seq->regions.AddUse(ret);

  return proc;
}

// Try to build a forcing procedure. We'll re-figure out the relation between
// clause head variables and the forced message variables here, rather than
// trying to wire through all the information.
static std::optional<ProgramProcedure> BuildQueryForceProcedure(
    ProgramImpl *impl, Context &context, ParsedQuery query) {
  if (auto pred = query.ForcingMessage()) {
    return BuildQueryForceProcedureImpl(
        impl, context, query, ParsedClause::Containing(*pred), *pred);
  }

  return std::nullopt;
}

// Add entry point records for each query of the program.
static void BuildQueryEntryPointImpl(ProgramImpl *impl, Context &context,
                                     ParsedDeclaration decl,
                                     QueryInsert insert) {

  const QueryView view(insert);
  const auto query = ParsedQuery::From(decl);
  const auto model = impl->view_to_model[view]->FindAs<DataModel>();
  assert(model->table != nullptr);

  std::vector<unsigned> col_indices;
  for (auto param : decl.Parameters()) {
    if (param.Binding() == ParameterBinding::kBound) {
      col_indices.push_back(param.Index());
    }
  }

  const DataTable table(model->table);
  std::optional<ProgramProcedure> forcer_proc =
      BuildQueryForceProcedure(impl, context, query);
  std::optional<DataIndex> scanned_index;

  if (!col_indices.empty()) {
    if (const auto index = model->table->GetOrCreateIndex(impl, col_indices)) {
      scanned_index.emplace(DataIndex(index));
    }
  }

  impl->queries.emplace_back(query, table, scanned_index, forcer_proc);
}


// Add an entry point record for a query none of whose derivations survived
// dataflow optimization: every flow into it was proven to never produce data,
// so there is no INSERT view and no data model for it. The query is still
// part of the program's external interface, so it scans a fresh table that
// nothing ever writes to.
static void BuildEmptyQueryEntryPointImpl(ProgramImpl *impl,
                                          ParsedDeclaration decl,
                                          TABLE *table) {
  const auto query = ParsedQuery::From(decl);

  std::vector<unsigned> col_indices;
  for (auto param : decl.Parameters()) {
    if (param.Binding() == ParameterBinding::kBound) {
      col_indices.push_back(param.Index());
    }
  }

  std::optional<DataIndex> scanned_index;
  if (!col_indices.empty()) {
    if (const auto index =
            table->GetOrCreateIndex(impl, std::move(col_indices))) {
      scanned_index.emplace(DataIndex(index));
    }
  }

  impl->queries.emplace_back(query, DataTable(table), scanned_index,
                             std::nullopt);
}

// Add entry point records, over a shared always-empty table, for each unique
// binding pattern of a query declaration with no backing INSERT view.
static void BuildEmptyQueryEntryPoint(ProgramImpl *impl,
                                      ParsedDeclaration decl) {
  TABLE *const table = impl->tables.Create(impl->next_id++);
  std::vector<unsigned> offsets;
  unsigned col_index = 0;
  for (auto param : decl.Parameters()) {
    offsets.push_back(col_index++);
    (void) table->columns.Create(impl->next_id++, param.Type(), table);
  }
  (void) table->GetOrCreateIndex(impl, std::move(offsets));

  std::unordered_set<std::string> seen_variants;
  for (auto redecl : decl.UniqueRedeclarations()) {
    std::string binding(redecl.BindingPattern());
    if (seen_variants.count(binding)) {
      continue;
    }
    seen_variants.insert(std::move(binding));
    BuildEmptyQueryEntryPointImpl(impl, redecl, table);
  }
}

// Add entry point records for each query to the program.
static void BuildQueryEntryPoint(ProgramImpl *impl, Context &context,
                                 ParsedDeclaration decl, QueryInsert insert) {
  std::unordered_set<std::string> seen_variants;

  for (auto redecl : decl.UniqueRedeclarations()) {

    // We may have duplicate redeclarations, so don't repeat any.
    std::string binding(redecl.BindingPattern());
    if (seen_variants.count(binding)) {
      continue;
    }
    seen_variants.insert(std::move(binding));
    BuildQueryEntryPointImpl(impl, context, redecl, insert);
  }
}

// Map all variables to their defining regions.
static void MapVariables(REGION *region) {
  if (!region) {
    return;

  } else if (auto op = region->AsOperation(); op) {
    if (auto let = op->AsLetBinding(); let) {
      for (auto var : let->defined_vars) {
        var->defining_region = region;
      }
    } else if (auto loop = op->AsVectorLoop(); loop) {
      for (auto var : loop->defined_vars) {
        var->defining_region = region;
      }
    } else if (auto join = op->AsTableJoin(); join) {
      for (auto var : join->pivot_vars) {
        var->defining_region = region;
      }
      for (const auto &var_list : join->output_vars) {
        for (auto var : var_list) {
          var->defining_region = region;
        }
      }

      MapVariables(join->added_body.get());
      MapVariables(join->removed_body.get());

    } else if (auto product = op->AsTableProduct(); product) {
      for (const auto &var_list : product->output_vars) {
        for (auto var : var_list) {
          var->defining_region = region;
        }
      }
    } else if (auto gen = op->AsGenerate(); gen) {
      for (auto var : gen->defined_vars) {
        var->defining_region = region;
      }

      MapVariables(gen->empty_body.get());

    } else if (auto call = op->AsCall(); call) {
      MapVariables(call->false_body.get());

    } else if (auto scan = op->AsTableScan(); scan) {
      for (auto var : scan->out_vars) {
        var->defining_region = region;
      }
    } else if (auto emplace = op->AsChangeRecord(); emplace) {
      for (auto var : emplace->record_vars) {
        var->defining_region = region;
      }

    } else if (auto check = op->AsCheckMember(); check) {
      MapVariables(check->absent_body.get());

    } else if (auto get = op->AsCheckRecord(); get) {
      for (auto var : get->record_vars) {
        var->defining_region = region;
      }
      MapVariables(get->absent_body.get());

    } else if (auto cmp = op->AsTupleCompare(); cmp) {
      MapVariables(cmp->false_body.get());
    }

    MapVariables(op->body.get());

  } else if (auto induction = region->AsInduction(); induction) {
    MapVariables(induction->init_region.get());
    MapVariables(induction->cyclic_region.get());
    MapVariables(induction->output_region.get());

  } else if (auto par = region->AsParallel(); par) {
    for (auto sub_region : par->regions) {
      MapVariables(sub_region);
    }
  } else if (auto series = region->AsSeries(); series) {
    for (auto sub_region : series->regions) {
      MapVariables(sub_region);
    }
  } else if (auto proc = region->AsProcedure(); proc) {
    for (auto var : proc->input_vars) {
      var->defining_region = proc;
    }
    MapVariables(proc->body.get());
  }
}

}  // namespace

OP *BuildStateCheckCaseReturnFalse(ProgramImpl *impl, REGION *parent) {
  return impl->operation_regions.CreateDerived<RETURN>(
      parent, ProgramOperation::kReturnFalseFromProcedure);
}

OP *BuildStateCheckCaseReturnTrue(ProgramImpl *impl, REGION *parent) {
  return impl->operation_regions.CreateDerived<RETURN>(
      parent, ProgramOperation::kReturnTrueFromProcedure);
}

// The derivation class of a counter fold emitted at the current build
// position for `view`'s table: `kRecursive` while the fold is being emitted
// inside the fixpoint cycle of `view`'s induction (a back-edge arrival),
// else `kNonRecursive` (a seed/init-position arrival).
DerivClass EmissionDerivClass(ProgramImpl *impl, Context &context,
                              QueryView view) {
  (void) impl;
  if (auto it = context.view_to_induction.find(view);
      it != context.view_to_induction.end() &&
      it->second->state == INDUCTION::kAccumulatingCycleRegions) {
    return DerivClass::kRecursive;
  }
  return DerivClass::kNonRecursive;
}

// A table is differential when any of its member views can produce
// deletions (the codegen table-flavor rule).
bool TableIsDifferential(TABLE *table) {
  for (const QueryView &view : table->views) {
    if (view.CanProduceDeletions()) {
      return true;
    }
  }
  return false;
}

// Populate `context.monotone_negated_tables`: the model table of each
// non-@never negate's negated view, when that table is MONOTONE (D2'). A
// differential negated table already carries both frontiers from its own
// stratum phases, so it is excluded; a @never negate never retracts and has no
// crossover, so its negated table stays a plain monotone gate (kPresent).
void FindMonotoneNegatedTables(ProgramImpl *impl, Context &context,
                               Query query) {
  for (QueryNegate negate : query.Negations()) {
    if (negate.HasNeverHint()) {
      continue;
    }
    TABLE *const negated_table =
        impl->view_to_model[negate.NegatedView()]->FindAs<DataModel>()->table;
    assert(negated_table != nullptr);
    if (!TableIsDifferential(negated_table)) {
      context.monotone_negated_tables.insert(negated_table);
    }
  }
}

// The lazily created per-table delta vector of `kind` (one of the six
// batch-skeleton kinds documented on `Context::table_delta_vecs`).
VECTOR *TableDeltaVector(ProgramImpl *impl, Context &context, TABLE *table,
                         VectorKind kind) {
  VECTOR *&vec = context.table_delta_vecs[table][static_cast<unsigned>(kind)];
  if (vec) {
    return vec;
  }

  // Shape the vector on a member view whose columns span the table (an
  // INSERT view exposes its stored columns as input columns).
  const auto num_cols = table->columns.Size();
  for (const QueryView &view : table->views) {
    if (!view.IsInsert() && view.Columns().size() == num_cols) {
      vec = context.entry_proc->VectorFor(impl, kind, view.Columns());
      return vec;
    }
  }
  for (const QueryView &view : table->views) {
    if (view.IsInsert()) {
      const auto insert = QueryInsert::From(view);
      if (insert.InputColumns().size() == num_cols) {
        std::vector<TypeLoc> col_types;
        for (auto col : insert.InputColumns()) {
          col_types.push_back(col.Type());
        }
        vec = context.entry_proc->vectors.Create(impl->next_id++, kind,
                                                 col_types, 0u);
        return vec;
      }
    }
  }
  assert(false);
  return nullptr;
}

// Append the tuple of `view` (its columns, or an INSERT's stored input
// columns) to `vec` inside `parent`.
VECTORAPPEND *AppendViewTupleToVector(ProgramImpl *impl,
                                      REGION *parent, QueryView view,
                                      VECTOR *vec) {
  VECTORAPPEND *const append =
      impl->operation_regions.CreateDerived<VECTORAPPEND>(
          parent, ProgramOperation::kAppendToInductionVector);
  append->vector.Emplace(append, vec);

  if (view.IsInsert()) {
    for (auto col : QueryInsert::From(view).InputColumns()) {
      append->tuple_vars.AddUse(parent->VariableFor(impl, col));
    }
  } else {
    for (auto col : view.Columns()) {
      append->tuple_vars.AddUse(parent->VariableFor(impl, col));
    }
  }
  return append;
}

// Possibly add a counter fold into `parent` persisting the tuple of `view`
// into its table. Returns the fold (or `parent` when no fold was needed),
// the table of `view`, and the updated `already_added`.
std::tuple<ProgramOperationRegionImpl *, TABLE *, TABLE *>
InTryInsert(ProgramImpl *impl, Context &context, QueryView view, OP *parent,
            TABLE *already_added) {

  const auto model = impl->view_to_model[view]->FindAs<DataModel>();
  TABLE *const table = model->table;
  if (table) {
    if (already_added != table) {

      // Figure out what columns to pass in for the fold.
      std::vector<QueryColumn> cols;
      if (view.IsInsert()) {
        auto insert = QueryInsert::From(view);
        cols.insert(cols.end(), insert.InputColumns().begin(),
                    insert.InputColumns().end());
      } else {
        cols.insert(cols.end(), view.Columns().begin(), view.Columns().end());
      }

      assert(!cols.empty());

      // Apply the fold; successors run only on the zero crossing.
      const auto fold =
          BuildUpdateCount(impl, table, parent, cols, true /* is_add */,
                           EmissionDerivClass(impl, context, view));

      parent->body.Emplace(parent, fold);
      parent = fold;
      already_added = table;
    }
  } else {
    already_added = nullptr;
  }

  return {parent, table, already_added};
}

// Add in all of the successors of a view inside of `parent`, which is
// usually some kind of loop. The successors execute in parallel.
void BuildEagerInsertionRegionsImpl(ProgramImpl *impl, QueryView view,
                                    Context &context, OP *parent_,
                                    const std::vector<QueryView> &successors,
                                    TABLE *last_table_) {

  // Make sure all output columns are available.
  for (auto col : view.Columns()) {
    (void) parent_->VariableFor(impl, col);
  }

  auto [parent, table, last_table] =
      InTryInsert(impl, context, view, parent_, last_table_);

  // All successors execute in a PARALLEL region, even if there are zero or
  // one successors. Empty and trivial PARALLEL regions are optimized out later.
  //
  // A key benefit of PARALLEL regions is that within them, CSE can be performed
  // to identify and eliminate repeated branches.
  PARALLEL *par = impl->parallel_regions.Create(parent);
  parent->body.Emplace(parent, par);

  // A deletion-capable non-inductive view's rows park in its table's add
  // queue at the fold's zero crossing; propagation through its
  // non-inductive consumers runs in the table's stratum phases, driven by
  // the frontier vectors the phases derive from the claimed queues.
  // (Inductive views' propagation is owned by their fixpoint machinery.)
  if (table != nullptr && parent != parent_ && TableIsDifferential(table) &&
      !view.InductionGroupId().has_value()) {
    par->AddRegion(AppendViewTupleToVector(
        impl, par, view,
        TableDeltaVector(impl, context, table, VectorKind::kAddQueue)));
  }

  //  std::unordered_map<DataModel *, std::vector<QueryView>> grouped_successors;
  //  for (auto succ_view : successors) {
  //    const auto succ_view_model =
  //        impl->view_to_model[succ_view]->FindAs<DataModel>();
  //    grouped_successors[succ_view_model].push_back(succ_view);
  //  }
  //
  //  for (const auto &[succ_model, succ_views] : grouped_successors) {
  //    const auto let = impl->operation_regions.CreateDerived<LET>(par);
  //    par->AddRegion(let);
  //
  //    const auto num_succ_views = succ_views.size();
  //
  //    if (1u == num_succ_views) {
  //      BuildEagerRegion(impl, view, succ_views[0], context, let, last_table);
  //
  //    // We may have multiple successors sharing a data model. If this happens,
  //    // then the code for one successor might
  //    } else {
  //
  //      OP *succ_parent = let;
  //      TABLE *succ_table = table;
  //      TABLE *succ_last_table = last_table;
  //
  //      // Only try to do an insert for the group of table-sharing successors
  //      // if their table is different than the table of `view`, into which we
  //      // have just inserted (initial `InTryInsert` in this function).
  //      //
  //      //
  //      if (table != succ_model->table &&
  //          (succ_views[0].IsTuple() || succ_views[0].IsMerge())) {
  //        auto combined_succ_insert = InTryInsert(
  //            impl, context, succ_views[0], let, last_table);
  //        succ_parent = std::get<0>(combined_succ_insert);
  //        succ_table = std::get<1>(combined_succ_insert);
  //        succ_last_table = std::get<2>(combined_succ_insert);
  //      }
  //
  //      PARALLEL * const succ_par = impl->parallel_regions.Create(succ_parent);
  //      succ_parent->body.Emplace(succ_parent, succ_par);
  //
  ////      const auto first_cols = succ_views[0].Columns();
  ////      for (auto i = 1ull; i < num_succ_views; ++i) {
  ////        auto j = 0u;
  ////        for (auto cols : succ_views[i].Columns()) {
  ////
  ////        }
  ////      }
  //
  //      for (auto succ_view : succ_views) {
  //        LET * const succ_let =
  //            impl->operation_regions.CreateDerived<LET>(succ_par);
  //        succ_par->AddRegion(succ_let);
  //        BuildEagerRegion(impl, view, succ_view, context, succ_let, succ_last_table);
  //      }
  //    }
  //  }

  auto any_cut_succ = false;
  for (QueryView succ_view : successors) {

    // A deletion-capable successor is fed by its own table's stratum phases
    // (its seeds range over this view's table's frontier vectors), never by
    // the eager walk. This holds even when the successor is inductive: a
    // recursive SCC that can produce deletions IS a differential stratum,
    // owned by the OVERDELETE/REDERIVE/INSERT phases, not by an eager
    // `ProgramInductionRegion` (StackSafeNegation.md §7). Pulling such a
    // successor into an eager induction would make its head table
    // `TableIsInductionOwned`, causing the stratum phases to skip it and its
    // counters to be never seeded — the recursive head then reads as absent
    // (e.g. a linear recursion over a monotone lower atom, whose monotone
    // predecessor's eager walk is the only path that reaches the JOIN).
    // Monotone inductions are unaffected: they cannot receive deletions.
    if (succ_view.CanReceiveDeletions()) {
      any_cut_succ = true;
      continue;
    }

    const auto let = impl->operation_regions.CreateDerived<LET>(par);
    par->AddRegion(let);
    BuildEagerRegion(impl, view, succ_view, context, let, last_table);
  }

  // A monotone boundary: this view's table feeds a differential consumer's
  // seeds, so its rows added this batch accumulate into the table's
  // net-additions frontier. The append always sits inside the table's fold
  // crossing — either the fold this call just made, or an ancestor fold of
  // the same table that this walk is nested under — so only new rows
  // accumulate; the seeds sort-unique the frontier, so one row appended at
  // several same-model sites still seeds once. A differential table needs
  // no boundary append: its frontiers are consolidated by its own stratum's
  // phases from the claimed queues.
  //
  // The second disjunct (D2', R7): a MONOTONE table that is the negated view
  // of a non-@never negate ALSO accumulates its gained keys into
  // net-additions — the `-` crossover arm's source — even when it has no cut
  // successor (a negate is NOT among the negated view's Successors(); the
  // crossover is the sole consumer). Creating the frontier auto-enrolls the
  // table in the Seal commit-sweep (Procedure.cpp), which the eager gate's new
  // InI read (Negate.cpp) also requires — one atomic provisioning (R1).
  // Duplicate appends across same-model sites are harmless: seeds sort-unique.
  const bool is_monotone_negated =
      table != nullptr && context.monotone_negated_tables.count(table) != 0u;
  if (table != nullptr && !TableIsDifferential(table) &&
      (any_cut_succ || is_monotone_negated)) {
    par->AddRegion(AppendViewTupleToVector(
        impl, par, view,
        TableDeltaVector(impl, context, table, VectorKind::kNetAdditions)));
  }
}

// Complete a procedure by exhausting the work list.
void CompleteProcedure(ProgramImpl *impl, PROC *proc, Context &context,
                       bool add_return) {
  while (!context.work_list.empty()) {

    std::stable_sort(context.work_list.begin(), context.work_list.end(),
                     [](const WorkItemPtr &a, const WorkItemPtr &b) {
                       return a->order > b->order;
                     });

    WorkItemPtr action = std::move(context.work_list.back());
    context.work_list.pop_back();
    action->Run(impl, context);
  }

  // Add a default `return false` at the end of normal procedures.
  if (add_return && !EndsWithReturn(proc)) {
    const auto ret = impl->operation_regions.CreateDerived<RETURN>(
        proc, ProgramOperation::kReturnFalseFromProcedure);
    ret->ExecuteAfter(impl, proc);
  }
}

// Map `view`'s output columns to the variables that `pred_view` has in
// scope at `parent`: the standard per-edge variable plumbing used by both
// the eager insertion walk and the delta-chain walk.
void MapVariablesInEagerRegion(ProgramImpl *impl, QueryView pred_view,
                               QueryView view, OP *parent) {
  view.ForEachUse([=](QueryColumn in_col, InputColumnRole role,
                      std::optional<QueryColumn> out_col) {
    if (!out_col) {
      return;
    }

    assert(in_col.Id() != out_col->Id());

    // An INSERT into a relation reports its readers' (SELECTs') columns as
    // outputs; every other view's outputs are its own columns.
    assert(QueryView::Containing(*out_col) == view || view.IsInsert());

    // Comparisons merge two inputs into a single output.
    if ((InputColumnRole::kCompareLHS == role ||
         InputColumnRole::kCompareRHS == role) &&
        (ComparisonOperator::kEqual == QueryCompare::From(view).Operator())) {
      return;

    // Index values are merged with prior values to form the output. We don't
    // overwrite join outputs otherwise they don't necessarily get assigned to
    // the right selection variables.
    } else if (InputColumnRole::kIndexValue == role) {
      return;

    } else if (QueryView::Containing(in_col) == pred_view) {
      const auto src_var = parent->VariableFor(impl, in_col);
      assert(src_var != nullptr);
      parent->col_id_to_var[out_col->Id()] = src_var;


    } else if (in_col.IsConstantRef()) {
      const auto src_var = parent->VariableFor(impl, in_col);
      assert(src_var != nullptr);
      parent->col_id_to_var.emplace(out_col->Id(), src_var);

    // NOTE(pag): This is subtle. We use `emplace` here instead of `[...] =`
    //            to give preference to the constant matching the incoming view.
    //            The key issue here is when we have a column of a MERGE node
    //            taking in a lot constants, we can't be certain which constant
    //            we're getting.
    } else if (in_col.IsConstant() && InputColumnRole::kMergedColumn != role) {
      const auto src_var = parent->VariableFor(impl, in_col);
      assert(src_var != nullptr);
      parent->col_id_to_var.emplace(out_col->Id(), src_var);
    }
  });
}

// Build an eager region.
void BuildEagerRegion(ProgramImpl *impl, QueryView pred_view, QueryView view,
                      Context &context, OP *parent, TABLE *last_table) {

  MapVariablesInEagerRegion(impl, pred_view, view, parent);

  if (view.IsJoin()) {
    const auto join = QueryJoin::From(view);
    if (join.NumPivotColumns()) {
      BuildEagerJoinRegion(impl, pred_view, join, context, parent, last_table);
    } else {
      BuildEagerProductRegion(impl, pred_view, join, context, parent,
                              last_table);
    }

  } else if (view.IsMerge()) {
    const auto merge = QueryMerge::From(view);
    if (view.InductionGroupId().has_value()) {
      BuildEagerInductiveRegion(impl, pred_view, merge, context, parent,
                                last_table);
    } else {
      BuildEagerUnionRegion(impl, pred_view, merge, context, parent,
                            last_table);
    }

  } else if (view.IsAggregate()) {
    assert(false && "TODO(pag): Aggregates");

  } else if (view.IsKVIndex()) {
    assert(false && "TODO(pag): KV Indices.");

  } else if (view.IsMap()) {
    auto map = QueryMap::From(view);
    if (map.Functor().IsPure()) {
      BuildEagerGenerateRegion(impl, pred_view, map, context, parent,
                               last_table);

    } else {
      assert(false && "TODO(pag): Impure functors");
    }

  } else if (view.IsCompare()) {
    BuildEagerCompareRegions(impl, QueryCompare::From(view), context, parent);

  } else if (view.IsSelect()) {

    // A SELECT is reached bottom-up only from an INSERT into its relation
    // (a unit condition relation keeps its INSERT -> RELATION -> SELECT
    // structure). Bind the SELECT's columns to the INSERT's stored columns,
    // whose variables are in scope.
    assert(pred_view.IsInsert());
    const auto insert = QueryInsert::From(pred_view);
    auto i = 0u;
    for (auto col : view.Columns()) {
      const auto in_col = insert.NthInputColumn(i++);
      parent->col_id_to_var[col.Id()] = parent->VariableFor(impl, in_col);
    }

    BuildEagerInsertionRegions(impl, view, context, parent, view.Successors(),
                               last_table);

  } else if (view.IsTuple()) {
    BuildEagerTupleRegion(impl, pred_view, QueryTuple::From(view), context,
                          parent, last_table);

  } else if (view.IsInsert()) {
    const auto insert = QueryInsert::From(view);
    BuildEagerInsertRegion(impl, pred_view, insert, context, parent,
                           last_table);

  } else if (view.IsNegate()) {
    const auto negate = QueryNegate::From(view);
    BuildEagerNegateRegion(impl, pred_view, negate, context, parent,
                           last_table);

  } else {
    assert(false);
  }
}

WorkItem::WorkItem(Context &context, unsigned order_) : order(order_) {}

WorkItem::~WorkItem(void) {}

// Build a program from a query.
std::optional<Program> Program::Build(const ::hyde::Query &query,
                                      const ErrorLog &log, unsigned first_id,
                                      bool optimize) {

  // Reject data-flow view kinds that the control-flow builder does not yet
  // support. Each region-dispatch switch below asserts on these kinds; this
  // pre-pass dominates those asserts (they remain as internal-invariant
  // backstops) and turns the feature gaps into user-facing diagnostics.
  const auto num_errors = log.Size();
  for (auto agg : query.Aggregates()) {
    log.Append(agg.Functor().SpellingRange())
        << "Aggregating functors are not yet supported";
  }
  for (auto kv : query.KVIndices()) {
    log.Append(kv.NthValueMergeFunctor(0).SpellingRange())
        << "Relations with mutable-attributed parameters are not yet "
           "supported";
  }
  for (auto map : query.Maps()) {
    if (!map.Functor().IsPure()) {
      log.Append(map.Functor().SpellingRange())
          << "Impure functors are not yet supported";
    }
  }
  for (auto join : query.Joins()) {
    if (!join.NumPivotColumns() && QueryView(join).CanReceiveDeletions() &&
        ViewSelfReachable(QueryView(join))) {

      // The ACYCLIC differential @product is lowered by the stratum phases
      // (`EmitProductArms`, Stratum.cpp); only the on-cycle shape remains a
      // gap (see `ViewSelfReachable` above).
      log.Append()
          << "Cross-products over differential (deletable) data inside "
             "recursive cycles are not yet supported";
    }
  }
  if (num_errors != log.Size()) {
    return std::nullopt;
  }

  auto impl = std::make_shared<ProgramImpl>(query, first_id);
  const auto program = impl.get();

  Context context;
  context.init_proc = impl->procedure_regions.Create(
      impl->next_id++, ProcedureKind::kInitializer);

  BuildDataModel(query, program);

  // Create constant variables. The literal-less boolean constant is the
  // `true` token of unit (condition) relations; it maps to the program's
  // built-in `true` variable.
  for (auto const_val : query.Constants()) {
    if (!const_val.Literal() && !const_val.IsTag()) {
      impl->const_to_var.emplace(const_val, impl->true_);
      continue;
    }
    const auto var =
        impl->const_vars.Create(impl->next_id++, VariableRole::kConstant);
    var->query_const = const_val;
    impl->const_to_var.emplace(const_val, var);
  }

  // Create tag variables.
  for (auto const_val : query.Tags()) {
    const auto var =
        impl->const_vars.Create(impl->next_id++, VariableRole::kConstantTag);
    var->query_const = QueryConstant(const_val);
    impl->const_to_var.emplace(const_val, var);
  }

  //  // Go figure out which merges are inductive, and then classify their
  //  // predecessors and successors in terms of which ones are inductive and
  //  // which aren't.
  //  DiscoverInductions(query, context, log);
  //  if (!log.IsEmpty()) {
  //    return std::nullopt;
  //  }

  // Now that we've identified our inductions, we can fill our data model,
  // i.e. assign persistent tables to each disjoint set of views.
  FillDataModel(query, program, context);

  // Identify the monotone negated tables that need a net-additions frontier
  // and Seal enrollment for the negation crossover (D2'), before the eager
  // insertion walk (which appends into that frontier) runs.
  FindMonotoneNegatedTables(program, context, query);

  // Build bottom-up procedures starting from message receives.
  PROC *const entry_proc = BuildEntryProcedure(program, context, query);

  for (auto io : query.IOs()) {
    BuildIOProcedure(impl.get(), query, io, context, entry_proc);
  }

  // Build the initialization procedure, needed to start data flows from
  // things like constant tuples.
  BuildInitProcedure(program, context, query);

  for (auto insert : query.Inserts()) {
    if (insert.IsRelation()) {
      auto decl = insert.Relation().Declaration();
      if (decl.IsQuery()) {
        BuildQueryEntryPoint(program, context, decl, insert);
      }
    }
  }

  // A query declaration all of whose derivations were proven to never
  // produce data has no INSERT view, and therefore got no entry point above.
  // It is still part of the program's external interface, so it gets an
  // entry point over an always-empty table.
  std::unordered_set<uint64_t> queries_with_entry_points;
  for (const ProgramQuery &spec : impl->queries) {
    queries_with_entry_points.insert(spec.query.Id());
  }
  for (class ParsedModule sub_module :
       ParsedModuleIterator(query.ParsedModule())) {
    for (ParsedQuery parsed_query : sub_module.Queries()) {
      if (!queries_with_entry_points.count(parsed_query.Id())) {
        queries_with_entry_points.insert(parsed_query.Id());
        BuildEmptyQueryEntryPoint(program, ParsedDeclaration(parsed_query));
      }
    }
  }

  for (auto proc : impl->procedure_regions) {
    if (!EndsWithReturn(proc)) {
      BuildStateCheckCaseReturnFalse(impl.get(), proc)
          ->ExecuteAfter(impl.get(), proc);
    }
  }

  FixupContainingProcedure(impl.get());
  if (optimize) {
    impl->Optimize();
  }

  ExtractPrimaryProcedure(impl.get(), entry_proc, context);

  FixupContainingProcedure(impl.get());
  if (optimize) {
    impl->Optimize();
  }

  // Assign defining regions to each variable.
  //
  // NOTE(pag): We don't really want to map variables throughout the building
  //            process because otherwise every time we replaced all uses of
  //            one region with another, we'd screw up the mapping.
  for (auto proc : impl->procedure_regions) {
    MapVariables(proc);
  }

  // Assign defining regions to each variable.
  //
  // NOTE(pag): We don't really want to map variables throughout the building
  //            process because otherwise every time we replaced all uses of
  //            one region with another, we'd screw up the mapping.
  for (auto proc : impl->procedure_regions) {
    MapVariables(proc);
  }

  // impl->Analyze();
#if 0
  // Finally, go through our tables. Any table with no indices is given a
  // full table index, on the assumption that it is used for things like state
  // checking.
  for (TABLE *table : impl->tables) {
    if (table->indices.Empty()) {
      std::vector<unsigned> offsets;
      for (auto col : table->columns) {
        offsets.push_back(col->index);
      }
      (void) table->GetOrCreateIndex(impl.get(), std::move(offsets));
    }
  }
#endif
  return Program(std::move(impl));
}

}  // namespace hyde
