// Copyright 2020, Trail of Bits. All rights reserved.

#include "Induction.h"

namespace hyde {
namespace {

static unsigned ContinueJoinOrder(QueryView view) {
  unsigned depth = view.Depth();
  unsigned order = 0;

  // We're doing a kind of priority inversion here. We are saying that there
  // is a JOIN, and this JOIN leads into an induction, and that induction
  // cycles back to the JOIN. But, we may not yet be inside of that induction,
  // or we're blocked on it, so what we're going to do is invert the ordering
  // of the JOIN and the INDUCTION work items, so that the continuation of
  // the JOIN is ordered to happen /after/ the continuation of the INDUCTION.
  //
  //                  .------.
  //                UNION    |
  //               /   |     B
  //            ...   JOIN   |
  //                  /  '---'
  //                 A
  //
  // Basically, we might come in via `A`, and we know that the JOIN will lead to
  // the UNION, and when we continue the UNION, we will eventually find our
  // way back to the JOIN via `B`, so we will treat the initial appends to the
  // JOIN's pivot vector from A as an inductive input vector to the UNION.
  if (auto ind_depth = view.InductionDepth(); ind_depth.has_value()) {
    order = WorkItem::kContinueInductionOrder |
            (ind_depth.value() << WorkItem::kInductionDepthShift);
    assert(0u < depth);  // Achieves priority inversion w.r.t. induction.
    depth += 1u;

  } else {
    order = WorkItem::kContinueJoinOrder;
  }

  return depth | order;
}

}  // namespace

ContinueJoinWorkItem::ContinueJoinWorkItem(Context &context, QueryView view_,
                                           VECTOR *input_pivot_vec_,
                                           VECTOR *swap_pivot_vec_,
                                           INDUCTION *induction_)
    : WorkItem(context, ContinueJoinOrder(view_)),
      view(view_),
      input_pivot_vec(input_pivot_vec_),
      swap_pivot_vec(swap_pivot_vec_),
      induction(induction_) {}

// Find the common ancestor of all insert regions.
REGION *ContinueJoinWorkItem::FindCommonAncestorOfInsertRegions(void) const {

  // This is quite subtle and there is a ton of collusion with induction
  // creation going on here. Basically, if we have a JOIN that "straddles"
  // an inductive back-edge, i.e. some of its predecessors are on that back-
  // edge, but others are more like inputs to the induction, then the induction
  // is in charge of the appends, pivot vectors, etc. To some extent, this is
  // a "cost-saving" measure: we avoid having the same logical JOIN execute
  // both outside and inside of the INDUCTION, and it also means we get to have
  // "inductive joins" have a more uniform concurrency story, by only sharding
  // induction vectors across workers. The big trick, though, is that the
  // induction code doesn't know what the variables being output by the join
  // will be until the JOIN itself is created. And so, it fakes this by going
  // and making a `LET` with some defined variables, but deferring their
  // assignment to the JOIN.
  if (NeedsInductionCycleVector(view)) {
    assert(inserts.empty());
    assert(induction != nullptr);
    PARALLEL *const par = induction->fixpoint_cycles[view];
    LET *const let = par->parent->AsOperation()->AsLetBinding();
    assert(let != nullptr);

    // This is the trick!
    assert(!let->defined_vars.Empty());
    assert(let->used_vars.Empty());
    return let;

  } else {
    assert(!inserts.empty());
    PROC *const proc = inserts[0]->containing_procedure;
    REGION *common_ancestor = nullptr;

    for (const auto insert : inserts) {
      if (!common_ancestor) {
        common_ancestor = insert;
      } else {
        common_ancestor = common_ancestor->FindCommonAncestor(insert);
      }
    }

    assert(common_ancestor != nullptr);
    if (proc == common_ancestor || !common_ancestor) {
      common_ancestor = proc->body.get();
    }

    // NOTE(pag): We *CAN'T* go any higher than `common_ancestor`, because then
    //            we might accidentally "capture" the vector appends for an
    //            unrelated induction, thereby introducing super weird ordering
    //            problems where an induction A is contained in the init region
    //            of an induction B, and B's fixpoint cycle region appends to
    //            A's induction vector.
    //
    // TODO(pag): Test this more thoroughly. In the case where we only have one
    //            thing in `inserts`, we end up doing a bad job with just
    //            `common_ancestor`, e.g. in a loop, we might observe an append,
    //            a join, then a clear of the pivot vector.
    return common_ancestor->NearestRegionEnclosedByInduction();
  }
}

namespace {

// A joined view backed by a unit (condition) table is not a scan source of
// the join: its only possible row is `(true)`, so it never multiplies
// cardinality — it only gates. `BuildJoin` excludes such views from the
// emitted TABLEJOIN's scan arms, and `ContinueJoinWorkItem::Run` gates the
// join body on a CHECKMEMBER membership test of the unit table instead.
static bool JoinedViewIsUnit(ProgramImpl *impl, QueryView pred_view) {
  DataModel *const model = impl->view_to_model[pred_view]->FindAs<DataModel>();
  return model->table != nullptr && model->table->is_condition;
}

// Heuristic sorting of predecessors.
std::vector<QueryView> SortedPredecessors(QueryJoin join) {

  std::unordered_map<QueryView, unsigned> num_non_pivots;
  std::vector<QueryView> pred_views;

  for (QueryView pred_view : join.JoinedViews()) {
    join.ForEachUse([&](QueryColumn in_col, InputColumnRole role,
                        std::optional<QueryColumn>) {
      if (InputColumnRole::kJoinNonPivot == role && !in_col.IsConstant() &&
          QueryView::Containing(in_col) == pred_view) {
        num_non_pivots[pred_view] += 1u;
      }
    });
    pred_views.push_back(pred_view);
  }

  std::sort(pred_views.begin(), pred_views.end(),
          [&] (QueryView a, QueryView b) {

    // Order nodes that have all of their pivot columns first.
    auto a_non_pivots = num_non_pivots[a];
    auto b_non_pivots = num_non_pivots[b];
    if (a_non_pivots != b_non_pivots) {
      return a_non_pivots < b_non_pivots;
    }

    // Assume that a deeper node is more constrained, i.e. has fewer records.
    auto a_depth = a.Depth();
    auto b_depth = b.Depth();
    if (a_depth != b_depth) {
      return a_depth > b_depth;
    }

    // Assume that relations with more columns are the result of many joins,
    // and so may themselves be more constrained.
    auto a_num_cols = a.Columns().size();
    auto b_num_cols = b.Columns().size();
    if (a_num_cols != b_num_cols) {
      return a_num_cols > b_num_cols;
    }

    // Now we'll just order with whatever.
    return a.EquivalenceSetId() < b.EquivalenceSetId();
  });

  return pred_views;
}

// Build a nested loop join.
static std::pair<OP *, OP *>
BuildNestedLoopJoin(ProgramImpl *impl, QueryJoin join, QueryView pred_view,
                    REGION *container) {
  LET *const let = impl->operation_regions.CreateDerived<LET>(container);

  std::vector<VAR *> out_vars(join.Columns().size());

  // Make sure to assign the input/output columns vars for the predecessor
  // view.
  join.ForEachUse([&](QueryColumn in_col, InputColumnRole role,
                      std::optional<QueryColumn> out_col) {
    if (!in_col.IsConstant() && QueryView::Containing(in_col) == pred_view) {
      VAR *const in_var = container->VariableFor(impl, in_col);
      let->col_id_to_var[in_col.Id()] = in_var;
      if (out_col) {
        let->col_id_to_var[out_col->Id()] = in_var;
        out_vars[*(out_col->Index())] = in_var;
      }
    }
  });

  OP *parent = let;

  // Order the predecessors so that we do scans that cover the maximum number
  // of columns first.
  auto found_pred_view = false;
  auto pred_views = SortedPredecessors(join);

  // Build up a bunch of index scans that will perform the join.
  for (QueryView other_pred_view : pred_views) {
    if (other_pred_view == pred_view) {
      found_pred_view = true;
    }

    const auto num_cols = other_pred_view.Columns().size();
    std::vector<unsigned> pivot_cols;
    std::vector<VAR *> pivot_vars(num_cols);
    std::vector<std::optional<QueryColumn>> out_cols(num_cols, std::nullopt);

    join.ForEachUse([&](QueryColumn in_col, InputColumnRole role,
                        std::optional<QueryColumn> out_col) {
      if (!in_col.IsConstant() &&
          QueryView::Containing(in_col) == other_pred_view) {

        const auto index = *(in_col.Index());

        if (InputColumnRole::kJoinPivot == role) {
          pivot_cols.push_back(index);
          assert(out_col.has_value());
          pivot_vars[index] = out_vars[*(out_col->Index())];
        }

        out_cols[index].swap(out_col);
      }
    });

    assert(!pivot_cols.empty());

    DataModel *const pred_model =
        impl->view_to_model[other_pred_view]->FindAs<DataModel>();
    TABLE *const pred_table = pred_model->table;
    assert(pred_table != nullptr);

    TABLESCAN *const scan = impl->operation_regions.CreateDerived<TABLESCAN>(
        impl->next_id++, parent);
    parent->body.Emplace(parent, scan);
    scan->table.Emplace(scan, pred_table);

    TUPLECMP *const cmp = impl->operation_regions.CreateDerived<TUPLECMP>(
        scan, ComparisonOperator::kEqual);
    scan->body.Emplace(scan, cmp);

    TABLEINDEX *const pred_index =
        pred_table->GetOrCreateIndex(impl, std::move(pivot_cols));

    scan->index.Emplace(scan, pred_index);

    for (TABLECOLUMN *table_col : pred_table->columns) {
      VAR *out_var = scan->out_vars.Create(
          impl->next_id++, VariableRole::kScanOutput);

      const QueryColumn other_pred_view_col =
          other_pred_view.NthColumn(table_col->index);
      out_var->query_column = other_pred_view_col;

      if (VAR *pivot_var = pivot_vars[table_col->index]) {
        scan->in_cols.AddUse(table_col);
        scan->in_vars.AddUse(pivot_var);
        cmp->lhs_vars.AddUse(pivot_var);
        cmp->rhs_vars.AddUse(out_var);
        cmp->col_id_to_var[other_pred_view_col.Id()] = pivot_var;

      } else {
        scan->out_cols.AddUse(table_col);
        cmp->col_id_to_var[other_pred_view_col.Id()] = out_var;

        if (auto join_out_col = out_cols[table_col->index];
            join_out_col.has_value()) {
          cmp->col_id_to_var[join_out_col->Id()] = out_var;
          out_vars[*(join_out_col->Index())] = out_var;
        } else {
          assert(false);
        }

        cmp->col_id_to_var[other_pred_view_col.Id()] = out_var;
      }
    }

    parent = cmp;
  }

  assert(found_pred_view);
  (void) found_pred_view;

  return {let, parent};
}

// Build a join region given a JOIN view and a pivot vector.
static std::pair<TABLEJOIN *, TUPLECMP *>
BuildJoin(ProgramImpl *impl, QueryJoin join_view, VECTOR *pivot_vec,
          SERIES *seq) {

  // We're now either looping over pivots in a pivot vector, or there was only
  // one entrypoint to the `QueryJoin` that was followed pre-work item, and
  // so we're in the body of an `insert`.
  TABLEJOIN * const join = impl->join_regions.Create(
      seq, join_view, impl->next_id++);
  seq->AddRegion(join);

  TUPLECMP * const cmp = impl->operation_regions.CreateDerived<TUPLECMP>(
      join, ComparisonOperator::kEqual);

  join->body.Emplace(join, cmp);

  // The JOIN internalizes the loop over its pivot vector. This is so that
  // it can have visibility into the sortedness, and choose what to do based
  // of of runs of sorted elements.
  join->pivot_vec.Emplace(join, pivot_vec);

  // After running the join, clear out the pivot vector.
  const auto clear = impl->operation_regions.CreateDerived<VECTORCLEAR>(
      seq, ProgramOperation::kClearJoinPivotVector);
  clear->vector.Emplace(clear, pivot_vec);
  seq->AddRegion(clear);

  // Fill in the pivot variables/columns.
  for (auto pivot_col : join_view.PivotColumns()) {
    auto var =
        join->pivot_vars.Create(impl->next_id++, VariableRole::kJoinPivot);
    var->query_column = pivot_col;
    if (pivot_col.IsConstantRef()) {
      var->query_const = QueryConstant::From(pivot_col);
    }

    join->col_id_to_var[pivot_col.Id()] = var;
  }

  std::vector<unsigned> pivot_col_indices;
  std::vector<QueryColumn> pivot_cols;
  std::unordered_map<QueryView, unsigned> view_to_index;
  const auto pred_views = join_view.JoinedViews();
  const auto num_pivots = join_view.NumPivotColumns();
  const auto max_i = pred_views.size();
  const auto sorted_pred_views = SortedPredecessors(join_view);

  // Add in the pivot columns, the tables from which we're selecting, and
  // the indexes that we're scanning.
  auto pred_view_index = 0u;
  for (QueryView pred_view : sorted_pred_views) {

    // Unit sides are gated with a CHECKMEMBER by our callers; they contribute
    // no scan arm. Their sole `bool` column is the pivot, whose variable
    // comes from the pivot vector.
    if (JoinedViewIsUnit(impl, pred_view)) {
      assert(pred_view.Columns().size() == 1u);
      continue;
    }

    auto i = 0u;
    for (; i < max_i; ++i) {
      if (pred_views[i] == pred_view) {
        break;
      }
    }

    pivot_cols.clear();
    for (auto j = 0u; j < num_pivots; ++j) {
      for (auto pivot_col : join_view.NthInputPivotSet(j)) {
        assert(!pivot_col.IsConstant());
        if (QueryView::Containing(pivot_col) == pred_view) {
          pivot_cols.push_back(pivot_col);
          pivot_col_indices.push_back(*(pivot_col.Index()));
          break;
        }
      }
    }

    DataModel *const pred_model =
        impl->view_to_model[pred_view]->FindAs<DataModel>();
    TABLE *const pred_table = pred_model->table;
    TABLEINDEX *const pred_index =
        pred_table->GetOrCreateIndex(impl, std::move(pivot_col_indices));

    join->tables.AddUse(pred_table);
    if (pred_index) {
      join->indices.AddUse(pred_index);
      join->index_of_index.push_back(join->indices.Size());
    } else {
      join->index_of_index.push_back(0u);
    }
    join->pivot_cols.emplace_back(join);
    join->output_cols.emplace_back(join);
    join->output_vars.emplace_back(join);

    assert(!view_to_index.count(pred_view));
    view_to_index.emplace(pred_view, pred_view_index++);

    auto &pivot_table_cols = join->pivot_cols.back();
    for (auto pivot_col : pivot_cols) {
      if (pred_index) {
        for (TABLECOLUMN *indexed_col : pred_index->columns) {
          if (pivot_col.Index() && indexed_col->index == *(pivot_col.Index())) {
            pivot_table_cols.AddUse(indexed_col);
            goto matched_pivot_col;
          }
        }
      } else {
        for (TABLECOLUMN *indexed_col : pred_table->columns) {
          if (pivot_col.Index() && indexed_col->index == *(pivot_col.Index())) {
            pivot_table_cols.AddUse(indexed_col);
            goto matched_pivot_col;
          }
        }
      }
      assert(false);
    matched_pivot_col:
      continue;
    }
  }

  // Every join keeps at least one scanned side: the desugaring always joins
  // unit relations against a non-unit view carrying the user's data flow.
  assert(0u < join->tables.Size());

  // Figure out which input relation is "most represented" in the outputs,
  // in terms of non-pivot columns. The way JOINs get emitted is as a loop
  // over a pivot vector, followed by some index scans, and then comparing
  // the scanned results against the records from the pivot vector. We want
  // to make sure that code contained inside of this comparison guard ends
  // up using variables derived from one of the index scans, rather than from
  // the pivot vector iteration. This is so that we can maintain better
  // provenance between downstream writes to tables that can reference upstream
  // records.

  std::vector<unsigned> num_non_pivot_outputs;
  num_non_pivot_outputs.resize(view_to_index.size());

  // Add in the non-pivot columns.
  join_view.ForEachUse([&](QueryColumn in_col, InputColumnRole role,
                           std::optional<QueryColumn> out_col) {
    if (!out_col || InputColumnRole::kJoinNonPivot != role) {
      return;
    }

    assert(!out_col->IsConstant());
    assert(!in_col.IsConstant());

    const QueryView pred_view = QueryView::Containing(in_col);
    const auto it = view_to_index.find(pred_view);
    assert(it != view_to_index.end());  // Unit sides contribute only pivots.
    num_non_pivot_outputs[it->second] += 1u;
  });

  unsigned most_represented_pred_view_idx = 0u;
  for (auto i = 1u; i < num_non_pivot_outputs.size(); ++i) {
    if (num_non_pivot_outputs[i] >
        num_non_pivot_outputs[most_represented_pred_view_idx]) {
      most_represented_pred_view_idx = i;
    }
  }

  // Add in the non-pivot columns.
  join_view.ForEachUse([&](QueryColumn in_col, InputColumnRole role,
                           std::optional<QueryColumn> out_col) {
    if (!out_col) {
      assert(false);
      return;
    }

    assert(!out_col->IsConstant());
    assert(!in_col.IsConstant());

    const QueryView pred_view = QueryView::Containing(in_col);
    const auto it = view_to_index.find(pred_view);
    if (it == view_to_index.end()) {

      // A unit side: there is no scanned row to bind an output variable to.
      // Its sole column is a pivot, and the join's output pivot column is
      // already bound to the pivot-vector variable via `join->col_id_to_var`.
      assert(InputColumnRole::kJoinPivot == role);
      return;
    }
    const unsigned pred_view_idx = it->second;
    TABLE * const pred_table = join->tables[pred_view_idx];
    auto &out_cols = join->output_cols.at(pred_view_idx);
    auto &out_vars = join->output_vars.at(pred_view_idx);

    assert(impl->view_to_model[pred_view]->FindAs<DataModel>()->table ==
           pred_table);

    out_cols.AddUse(pred_table->columns[*(in_col.Index())]);
    VAR * var = nullptr;

    if (InputColumnRole::kJoinPivot == role) {
      var = out_vars.Create(impl->next_id++, VariableRole::kJoinPivot);

      // If we're using an index for this JOIN, then we want to double check
      // that what we've selected is indeed what we asked for (from the
      // pivot vector). This may seem redundant but it permits index scans to
      // be approximate.
      if (join->index_of_index[pred_view_idx]) {
        cmp->lhs_vars.AddUse(join->pivot_vars[*(out_col->Index())]);
        cmp->rhs_vars.AddUse(var);

      // NOTE(pag): We're currently operating with always having indices. If we
      //            don't have indices, then it requires that the codegen do
      //            point lookups instead of scans. If we switch back to a
      //            possibly indexless approach, then we should revisit the
      //            above code.
      } else {
        assert(false);
      }

      // For child nodes, this "hides" the variable from the pivot vector
      // iteration.
      if (pred_view_idx == most_represented_pred_view_idx) {
        cmp->col_id_to_var[out_col->Id()] = var;
      }

    } else {
      var = out_vars.Create(impl->next_id++, VariableRole::kJoinNonPivot);
      join->col_id_to_var[in_col.Id()] = var;
      join->col_id_to_var[out_col->Id()] = var;
    }

    // Failure suggests that a JOIN takes the same view or same column twice.
    assert(out_vars.Size() <= pred_table->columns.Size());

    var->query_column = in_col;
  });

  // Put the defined variables in the order of their views.
  for (auto &out_vars : join->output_vars) {
    out_vars.Sort([] (VAR *a, VAR *b) {
      return *(a->query_column->Index()) < *(b->query_column->Index());
    });
  }

  return {join, cmp};
}

}  // namespace

void ContinueJoinWorkItem::Run(ProgramImpl *impl, Context &context) {
  const auto join_view = QueryJoin::From(view);
  const auto needs_inductive_cycle_vec = NeedsInductionCycleVector(view);
  const auto needs_inductive_output_vec = NeedsInductionOutputVector(view);

  context.view_to_join_action.erase(view);

  for (OP *insert : inserts) {
    assert(!needs_inductive_cycle_vec);

    const auto append = impl->operation_regions.CreateDerived<VECTORAPPEND>(
        insert, ProgramOperation::kAppendJoinPivotsToVector);
    insert->body.Emplace(insert, append);

    for (auto col : join_view.PivotColumns()) {
      const auto var = insert->VariableFor(impl, col);
      append->tuple_vars.AddUse(var);
    }

    append->vector.Emplace(append, input_pivot_vec);
  }

  // Find the common ancestor of all of the `kInsertIntoView` associated with
  // the reached `QueryJoin`s that happened before this work item. Everything
  // under this common ancestor must execute before the loop over the join_view
  // pivots.
  const auto ancestor = FindCommonAncestorOfInsertRegions();
  const auto seq = impl->series_regions.Create(ancestor->parent);
  ancestor->ReplaceAllUsesWith(seq);

  // Sort and unique the pivot vector before looping.
  if (!needs_inductive_cycle_vec) {
    assert(!inserts.empty());

    ancestor->parent = seq;
    seq->AddRegion(ancestor);

    const auto unique = impl->operation_regions.CreateDerived<VECTORUNIQUE>(
        seq, ProgramOperation::kSortAndUniquePivotVector);
    unique->vector.Emplace(unique, swap_pivot_vec);
    seq->AddRegion(unique);
  }

  auto [join, cmp] = BuildJoin(impl, join_view, swap_pivot_vec, seq);
  OP *parent = cmp;

  // Figure out if any of the joined views is backed by a unit (condition)
  // table. Such views are not scan sources of the join (`BuildJoin` excludes
  // them), so each one is tested here with a CHECKMEMBER membership test on
  // its unit table.
  auto has_unit_pred = false;
  for (auto pred_view : view.Predecessors()) {
    if (JoinedViewIsUnit(impl, pred_view)) {
      has_unit_pred = true;
      break;
    }
  }

  // Gate the join body on row membership. Unit sides always need their
  // CHECKMEMBER gate. If this join can receive deletions, then we also need
  // to possibly double check its non-unit sources, because indices don't
  // maintain liveness.
  if (view.CanReceiveDeletions() || has_unit_pred) {

    // Map the JOIN's output variables to its inputs so that we can do the
    // membership checks below.
    view.ForEachUse([&](QueryColumn in_col, InputColumnRole role,
                        std::optional<QueryColumn> out_col) {
      if (out_col) {
        parent->col_id_to_var[in_col.Id()] =
            parent->VariableFor(impl, *out_col);
      }
    });

    // Gate on each predecessor whose liveness the scans cannot witness.
    for (auto pred_view : view.Predecessors()) {

      // NOTE(pag): All views leading into a JOIN are always backed by a table.
      DataModel *const pred_model =
          impl->view_to_model[pred_view]->FindAs<DataModel>();
      TABLE *const pred_table = pred_model->table;
      assert(pred_table != nullptr);

      if (!pred_table->is_condition &&
          !(view.CanReceiveDeletions() && pred_view.CanProduceDeletions())) {
        continue;
      }

      OP *parent_out = nullptr;
      CHECKMEMBER *const check = BuildCheckMember(
          impl, parent, pred_table, pred_view.Columns(),
          MembershipPredicate::kPresent,
          [&parent_out](ProgramImpl *impl_, REGION *in_check) -> REGION * {
            parent_out = impl_->operation_regions.CreateDerived<LET>(in_check);
            return parent_out;
          },
          [](ProgramImpl *, REGION *) -> REGION * { return nullptr; });

      parent->body.Emplace(parent, check);
      parent = parent_out;
    }
  }

  // Add a tuple to the output vector. We don't need to compute a worker ID
  // because we know we're dealing with only worker-specific data in this
  // cycle.
  if (needs_inductive_output_vec) {
    PARALLEL *par = impl->parallel_regions.Create(parent);
    parent->body.Emplace(parent, par);
    par->AddRegion(
        AppendToInductionOutputVectors(impl, view, context, induction, par));

    parent = impl->operation_regions.CreateDerived<LET>(par);
    par->AddRegion(parent);
  }

  auto [insert_parent, table, last_table] =
      InTryInsert(impl, context, view, parent, nullptr);
  parent = insert_parent;

  // Collusion with inductions!!!! The `BuildFixpointLoop` function in
  // `Induction.cpp` sets up our ancestor to be this `LET`, and the induction
  // will manually handle calling `BuildEagerInsertionRegions` from inside
  // this `LET`. It does this *before* this function runs, though, so it has
  // to stub out the output variables of the JOIN, so that we can fill them
  // in here.
  if (needs_inductive_cycle_vec) {
    assert(induction != nullptr);
    LET *let_in_fixpoint_region = ancestor->AsOperation()->AsLetBinding();
    let_in_fixpoint_region->parent = parent;
    parent->body.Emplace(parent, let_in_fixpoint_region);

    // Fill in the assignments!
    assert(let_in_fixpoint_region->defined_vars.Size() ==
           view.Columns().size());
    assert(let_in_fixpoint_region->used_vars.Empty());
    for (auto col : view.Columns()) {
      let_in_fixpoint_region->used_vars.AddUse(parent->VariableFor(impl, col));
    }
    assert(!let_in_fixpoint_region->used_vars.Empty());

  } else {
    BuildEagerInsertionRegions(impl, view, context, parent, view.Successors(),
                               last_table);
  }
}

// Build an eager region for a join.
void BuildEagerJoinRegion(ProgramImpl *impl, QueryView pred_view,
                          QueryJoin join, Context &context, OP *parent_,
                          TABLE *last_table_) {
  const QueryView view(join);

  // NOTE(pag): What's interesting about JOINs is that we force the data of
  //            our *predecessors* into tables, so that we can always complete
  //            the JOINs later and see "the other sides."
  auto [insert, pred_table, last_table] =
      InTryInsert(impl, context, pred_view, parent_, last_table_);

  OP *parent = insert;

  INDUCTION *induction = nullptr;
  if (view.InductionGroupId().has_value()) {
    induction = GetOrInitInduction(impl, view, context, parent);
  }

  // If this join is on the edge of an induction, i.e. one or more of the
  // JOIN's input views is a back-edge from and induction, and one or more of
  // the input views is an input source to the induction., then we need to
  // collude with an INDUCTION to make this work. In practice, this turns out
  // to get really crazy.
  if (NeedsInductionCycleVector(view)) {
    VECTOR *const pivot_vec = induction->view_to_add_vec[view];
    VECTOR *const swap_vec = induction->view_to_swap_vec[view];
    assert(pivot_vec && swap_vec);
    (void) pivot_vec;
    (void) swap_vec;
    AppendToInductionInputVectors(impl, view, view, context, parent,
                                  induction);

  // This is a join that is whose predecessors live inside of an induction,
  // but has at least one successor that lives outside of the induction.
  //
  // NOTE(pag): Simple JOINs contained inside of inductions may require output
  //            vectors, so that is why we calculate `induction` above.
  } else if (true ||
             NeedsInductionOutputVector(view) ||
             view.CanReceiveDeletions()) {
    auto &join_action = context.view_to_join_action[view];

    // Suggests some kind of infinite loop in how inductive joins are being
    // (mis-)handled, usually a cycle of join...join, or join..negate..join,
    // or going through a union that doesn't have an induction vector.
    if (induction && !join_action) {
      assert(false);
      return;
    }

    if (!join_action) {
      PROC *const proc = parent->containing_procedure;
      VECTOR *const pivot_vec =
          proc->VectorFor(impl, VectorKind::kJoinPivots, join.PivotColumns());

      join_action = new ContinueJoinWorkItem(context, view, pivot_vec,
                                             pivot_vec, induction);
      context.work_list.emplace_back(join_action);
    }

    join_action->inserts.push_back(parent);

  // Yay, it's just a "simple" join, i.e. it's entirely contained outside
  // of an inductive region, or it's entirely contained inside of an inductive
  // region. We have all the data we need in `pred_view`, so now we want to
  // generate a bunch of unrolled table/index scans on the other views of the
  // join.
  } else {
    assert(false && "Disabled\n");

    auto [begin, inner] = BuildNestedLoopJoin(impl, join, pred_view, parent);
    parent->body.Emplace(parent, begin);

    BuildEagerInsertionRegions(
        impl, view, context, inner, view.Successors(), last_table);
  }
}

}  // namespace hyde
