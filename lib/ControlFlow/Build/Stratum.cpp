// Copyright 2020, Trail of Bits. All rights reserved.

#include <set>

#include "Build.h"

namespace hyde {
namespace {

// One rule chain from a member view of a source table to its terminal: the
// first table-backed view (a head, the chain's fold target) or the first
// JOIN (whose dual-section emission owns the rest of the walk). `path[0]`
// is the source member view; the remaining entries are the consumer edges
// walked to the terminal, which is `path.back()`.
struct BranchChain {
  TABLE *source{nullptr};
  std::vector<QueryView> path;
  bool ends_at_join{false};

  // The head chain's fold target (null for a join chain, whose fold
  // targets belong to the join emission's section walk).
  TABLE *target{nullptr};

  // The stratum whose phase series emits this chain. Starts at the spec
  // stratum — the owner stratum of the head table for a head chain, the
  // join view's stratum for a join chain — and is lifted by the scheduling
  // fixpoint until every read the chain performs (the source frontier,
  // negated tables, join sides) is phase-final when it runs; every fold
  // targets a queue drained at or after this stratum.
  unsigned stratum{0u};
};

// One dual-section TABLEJOIN emission: the join view, its per-batch delta
// pivot vector (shared by both signs of every feeding chain), and the
// stratum (the join view's own) whose series emits it.
struct JoinEmission {
  QueryView join_view;
  VECTOR *pivot_vec;

  // Starts at the join view's stratum and is lifted by the scheduling
  // fixpoint alongside the feeding chains.
  unsigned stratum;

  // The fold targets of the join's section walks.
  std::vector<TABLE *> targets;

  JoinEmission(QueryView join_view_, VECTOR *pivot_vec_, unsigned stratum_)
      : join_view(join_view_),
        pivot_vec(pivot_vec_),
        stratum(stratum_) {}
};

// A table's differential phases are owned by an induction when any view of
// its data model is inductive: the fixpoint machinery feeds and drains such
// tables, and the stratum phases leave them alone (except as chain sources,
// through the net-additions frontier the induction's output loop fills).
static bool TableHasInductiveView(TABLE *table) {
  for (const QueryView &view : table->views) {
    if (view.InductionGroupId().has_value()) {
      return true;
    }
  }
  return false;
}

// The stratum that owns a table's differential phases: the maximum stratum
// over the table's member views. Folds into the table can come from any
// member's deriving rules, so only at the owner stratum have all of the
// batch's folds landed and can the claim drain run.
static unsigned TableOwnerStratum(TABLE *table) {
  unsigned stratum = 0u;
  for (const QueryView &view : table->views) {
    assert(view.Stratum().has_value());
    stratum = std::max(stratum, *(view.Stratum()));
  }
  return stratum;
}

// Whether the delta walk follows the consumer edge `view -> succ`. Edges
// owned by other machinery are skipped: inductive arrivals belong to the
// induction's fixpoint vectors, monotone consumers to the eager insertion
// walk, and stream INSERTs to the publication paths (a deletion-capable
// transmit publishes through its backing table's commit sweep).
static bool FollowsDeltaEdge(QueryView succ) {
  if (succ.InductionGroupId().has_value()) {
    return false;
  }
  if (!succ.CanReceiveDeletions()) {
    return false;
  }
  if (succ.IsInsert() && QueryInsert::From(succ).IsStream()) {
    return false;
  }
  return true;
}

// Depth-first discovery of the branch chains rooted at a source table's
// member view. `path` holds the views walked so far (starting with the
// member); a chain is recorded at each terminal (first JOIN, or first view
// whose table differs from the source's), and the walk continues through
// table-less plumbing. A non-insert view sharing the source's model is a
// chain root itself, so the walk stops at it without recording — its own
// rooted walk covers its consumer edges exactly once; a same-model INSERT
// is passed through (its readers are same-model SELECTs, themselves
// roots). Non-inductive views are acyclic, so the walk terminates.
static void DiscoverBranches(ProgramImpl *impl, TABLE *source,
                             std::vector<QueryView> &path, QueryView view,
                             std::vector<BranchChain> &branches) {
  for (QueryView succ : view.Successors()) {
    if (!FollowsDeltaEdge(succ)) {
      continue;
    }

    DataModel *const model = impl->view_to_model[succ]->FindAs<DataModel>();
    TABLE *const table = model->table;

    if (table == source && !succ.IsInsert()) {
      assert(succ.Columns().size() == source->columns.Size());
      continue;
    }

    path.push_back(succ);

    if (succ.IsJoin()) {

      // Differential cross-products are rejected with a diagnostic before
      // the control-flow build starts.
      assert(0u < QueryJoin::From(succ).NumPivotColumns());
      assert(succ.Stratum().has_value());

      BranchChain branch;
      branch.source = source;
      branch.path = path;
      branch.ends_at_join = true;
      branch.stratum = *(succ.Stratum());
      branches.push_back(std::move(branch));

    } else if (table != nullptr && table != source) {

      // A fold target inside an induction's model is fed by the fixpoint
      // machinery, not by stratum phases; the chain into it is dropped.
      if (!TableHasInductiveView(table)) {
        BranchChain branch;
        branch.source = source;
        branch.path = path;
        branch.ends_at_join = false;
        branch.target = table;
        branch.stratum = TableOwnerStratum(table);
        branches.push_back(std::move(branch));
      }

    } else {
      DiscoverBranches(impl, source, path, succ, branches);
    }

    path.pop_back();
  }
}

// Collects the fold targets that a join's section walk reaches (a
// differential join is persisted, so this is normally just the join's own
// table), mirroring `EmitSectionWalk`'s recursion.
static void CollectSectionTargets(ProgramImpl *impl, QueryView view,
                                  std::vector<TABLE *> &targets) {
  DataModel *const model = impl->view_to_model[view]->FindAs<DataModel>();
  TABLE *const table = model->table;
  if (table != nullptr) {
    if (!TableHasInductiveView(table)) {
      targets.push_back(table);
    }
    return;
  }
  for (QueryView succ : view.Successors()) {
    if (FollowsDeltaEdge(succ)) {
      CollectSectionTargets(impl, succ, targets);
    }
  }
}

// Emit the per-view plumbing of one delta-chain step arriving at `view`
// from `pred_view`, mirroring the eager walk's per-node regions: variable
// mapping for forwarding views, a TUPLECMP for a CMP, a generator call for
// a MAP, and the dualized forward gate for a NEGATE. Returns the region
// under which the rest of the chain nests.
static OP *EmitChainStep(ProgramImpl *impl, Context &context,
                         QueryView pred_view, QueryView view, bool is_add,
                         OP *parent) {

  // A SELECT is reached from an INSERT into its relation; bind the SELECT's
  // columns to the INSERT's stored columns, whose variables are in scope.
  if (view.IsSelect()) {
    assert(pred_view.IsInsert());
    const auto insert = QueryInsert::From(pred_view);
    auto i = 0u;
    for (auto col : view.Columns()) {
      const auto in_col = insert.NthInputColumn(i++);
      parent->col_id_to_var[col.Id()] = parent->VariableFor(impl, in_col);
    }
    return parent;
  }

  MapVariablesInEagerRegion(impl, pred_view, view, parent);

  if (view.IsCompare()) {
    const auto [cmp, body] =
        CreateCompareRegion(impl, QueryCompare::From(view), context, parent);
    parent->body.Emplace(parent, cmp);
    return body;
  }

  if (view.IsMap()) {
    const auto map = QueryMap::From(view);
    const auto functor = map.Functor();
    assert(functor.IsPure());
    GENERATOR *const gen =
        CreateGeneratorCall(impl, map, functor, context, parent, true);
    parent->body.Emplace(parent, gen);
    if (!map.IsPositive()) {
      OP *const let = impl->operation_regions.CreateDerived<LET>(gen);
      gen->empty_body.Emplace(gen, let);
      return let;
    }
    return gen;
  }

  // The forward pass of negation maintenance, dualized per sign: a `+` walk
  // continues only when the negated key is absent from the negated table's
  // batch-final state (`InNew`), a `-` walk only when it was absent from
  // the batch-start state (`InI`). The negated table is phase-final here:
  // its stratum is strictly lower than the chain's emission stratum.
  if (view.IsNegate()) {
    const auto negate = QueryNegate::From(view);
    const QueryView negated_view = negate.NegatedView();
    std::vector<QueryColumn> negated_view_cols;
    for (QueryColumn out_col : negate.NegatedColumns()) {
      const auto i = *(out_col.Index());
      const auto neg_col = negated_view.NthColumn(i);
      VAR *const out_col_var = parent->VariableFor(impl, out_col);
      assert(out_col_var != nullptr);
      parent->col_id_to_var[neg_col.Id()] = out_col_var;
      negated_view_cols.push_back(neg_col);
    }

    DataModel *const negated_model =
        impl->view_to_model[negated_view]->FindAs<DataModel>();
    TABLE *const negated_table = negated_model->table;
    assert(negated_table != nullptr);

    OP *continuation = nullptr;
    CHECKMEMBER *const gate = BuildCheckMember(
        impl, parent, negated_table, negated_view_cols,
        is_add ? MembershipPredicate::kInNew : MembershipPredicate::kInI,
        [](ProgramImpl *, REGION *) -> REGION * { return nullptr; },
        [&](ProgramImpl *impl_, REGION *in_check) -> REGION * {
          continuation = impl_->operation_regions.CreateDerived<LET>(in_check);
          return continuation;
        });
    parent->body.Emplace(parent, gate);
    return continuation;
  }

  // TUPLE / MERGE / INSERT forwarding: the variable mapping is the whole
  // step.
  return parent;
}

// Emit the terminal fold of a delta chain: one signed nonrecursive counter
// fold of `view`'s tuple into `table`, whose zero crossing parks the row in
// the table's add or delete queue for the owner stratum's claim drain.
// Claimed rows do not walk successors: all downstream propagation is
// higher-stratum seeds ranging over the table's net frontiers.
//
// The counter class is always `kNonRecursive`: `table` is a non-inductive
// differential table (the `TableHasInductiveView` partition drops inductive
// fold targets), and the scheduling fixpoint has lifted this fold's emission
// stratum above every table it reads, so no fold here closes a same-stratum
// recursion cycle. That "all reads are strictly lower strata" property is
// assert-checked after the fixpoint runs.
static void EmitHeadFold(ProgramImpl *impl, Context &context, QueryView view,
                         TABLE *table, bool is_add, OP *parent) {
  std::vector<QueryColumn> cols;
  if (view.IsInsert()) {
    const auto insert = QueryInsert::From(view);
    cols.insert(cols.end(), insert.InputColumns().begin(),
                insert.InputColumns().end());
  } else {
    cols.insert(cols.end(), view.Columns().begin(), view.Columns().end());
  }
  assert(!cols.empty());

  UPDATECOUNT *const fold = BuildUpdateCount(impl, table, parent, cols,
                                             is_add, DerivClass::kNonRecursive);
  parent->body.Emplace(parent, fold);

  VECTOR *const queue = TableDeltaVector(
      impl, context, table,
      is_add ? VectorKind::kAddQueue : VectorKind::kDeleteQueue);
  VECTORAPPEND *const append =
      impl->operation_regions.CreateDerived<VECTORAPPEND>(
          fold, ProgramOperation::kAppendToInductionVector);
  append->vector.Emplace(append, queue);
  for (auto col : cols) {
    append->tuple_vars.AddUse(fold->VariableFor(impl, col));
  }
  fold->body.Emplace(fold, append);
}

// Emit the downstream walk of one join section: starting at the join view
// itself, fold at the first table (a differential join is persisted, so
// this is normally the join's own table) and otherwise continue through
// table-less plumbing into each eligible consumer edge.
//
// Each walk folds at most once: it returns at the first table it reaches, and
// chain discovery (`DiscoverBranches`) already terminates every chain at its
// first table boundary, so no walk traverses two table boundaries or
// reconverges on one fold site. A single-fold guarantee is required because
// each fold appends to the table's update-count (a load-bearing multiset
// value): a double fold would over-count and break net-zero detection. No
// running already-folded set is needed — the structure enforces it.
static void EmitSectionWalk(ProgramImpl *impl, Context &context,
                            QueryView view, bool is_add, OP *parent) {
  DataModel *const model = impl->view_to_model[view]->FindAs<DataModel>();
  TABLE *const table = model->table;

  if (table != nullptr) {
    if (!TableHasInductiveView(table)) {
      EmitHeadFold(impl, context, view, table, is_add, parent);
    }
    return;
  }

  PARALLEL *const par = impl->parallel_regions.Create(parent);
  parent->body.Emplace(parent, par);
  for (QueryView succ : view.Successors()) {
    if (!FollowsDeltaEdge(succ)) {
      continue;
    }

    // The scheduling fixpoint lifts a chain's emission stratum above every
    // NEGATE'd table it reads by scanning `BranchChain::path`, but a NEGATE
    // reached only inside a join section walk (past the kJoin terminal, not
    // in any `path`) is outside that scan's coverage. Such a NEGATE would be
    // emitted here without its negated table's readiness having been lifted
    // for, so its `kInNew`/`kInI` gate could read a non-final state. This
    // shape is out of scope: no corpus join section walks table-less plumbing
    // at all, so none contains a NEGATE. Guard the boundary rather than
    // silently under-lift.
    assert(!succ.IsNegate());

    LET *const let = impl->operation_regions.CreateDerived<LET>(par);
    par->AddRegion(let);
    OP *const step = EmitChainStep(impl, context, view, succ, is_add, let);
    EmitSectionWalk(impl, context, succ, is_add, step);
  }
}

// Emit one seed: a loop over the source table's frontier vector `vec`
// binding the branch's member view's columns, followed by the chain's
// plumbing, ending at a head fold or at a pivot append into the join's
// delta pivot vector (`join_pivot_vec`, null for head chains).
static void EmitSeedLoop(ProgramImpl *impl, Context &context,
                         const BranchChain &branch, bool is_add, VECTOR *vec,
                         VECTOR *join_pivot_vec, SERIES *seq) {
  const QueryView member = branch.path[0];

  VECTORLOOP *const loop = impl->operation_regions.CreateDerived<VECTORLOOP>(
      impl->next_id++, seq, ProgramOperation::kLoopOverInductionVector);
  seq->AddRegion(loop);
  loop->vector.Emplace(loop, vec);

  for (auto col : member.Columns()) {
    VAR *const var = loop->defined_vars.Create(impl->next_id++,
                                               VariableRole::kVectorVariable);
    var->query_column = col;
    loop->col_id_to_var.emplace(col.Id(), var);
  }

  OP *parent = loop;
  QueryView pred_view = member;
  for (auto i = 1u; i < branch.path.size(); ++i) {
    const QueryView view = branch.path[i];
    const bool is_terminal = (i + 1u == branch.path.size());

    if (is_terminal && branch.ends_at_join) {

      // Bind the join's output pivot columns to this side's contribution,
      // then append the pivot values; the join region itself runs after
      // every feeding seed loop in this stratum's series.
      MapVariablesInEagerRegion(impl, pred_view, view, parent);
      VECTORAPPEND *const append =
          impl->operation_regions.CreateDerived<VECTORAPPEND>(
              parent, ProgramOperation::kAppendJoinPivotsToVector);
      append->vector.Emplace(append, join_pivot_vec);
      for (auto col : QueryJoin::From(view).PivotColumns()) {
        append->tuple_vars.AddUse(parent->VariableFor(impl, col));
      }
      parent->body.Emplace(parent, append);
      return;
    }

    parent = EmitChainStep(impl, context, pred_view, view, is_add, parent);
    pred_view = view;
  }

  // A head chain: `path.back()` is the fold target, and its plumbing (the
  // chain-step above) already ran.
  const QueryView head = branch.path.back();
  assert(!branch.ends_at_join);
  assert(pred_view == head);
  DataModel *const model = impl->view_to_model[head]->FindAs<DataModel>();
  assert(model->table != nullptr);
  EmitHeadFold(impl, context, head, model->table, is_add, parent);
}

// Emit one claim drain: sort-unique the queue of rows whose folds crossed
// zero this batch, then claim each row into the batch's overdeletion or
// addition set; the claim dedups (its body runs only on the row's first
// claim), so the set holds each claimed row once.
static void EmitClaimDrain(ProgramImpl *impl, Context &context, TABLE *table,
                           bool is_del, SERIES *seq) {
  VECTOR *const queue = TableDeltaVector(
      impl, context, table,
      is_del ? VectorKind::kDeleteQueue : VectorKind::kAddQueue);
  VECTOR *const claimed_set = TableDeltaVector(
      impl, context, table,
      is_del ? VectorKind::kOverdeleteSet : VectorKind::kAdditionSet);

  VECTORUNIQUE *const unique =
      impl->operation_regions.CreateDerived<VECTORUNIQUE>(
          seq, ProgramOperation::kSortAndUniqueInductionVector);
  unique->vector.Emplace(unique, queue);
  seq->AddRegion(unique);

  VECTORLOOP *const loop = impl->operation_regions.CreateDerived<VECTORLOOP>(
      impl->next_id++, seq, ProgramOperation::kLoopOverInductionVector);
  seq->AddRegion(loop);
  loop->vector.Emplace(loop, queue);

  std::vector<VAR *> row_vars;
  for (auto i = 0u, max_i = table->columns.Size(); i < max_i; ++i) {
    row_vars.push_back(loop->defined_vars.Create(
        impl->next_id++, VariableRole::kVectorVariable));
  }

  CLAIM *const claim = impl->operation_regions.CreateDerived<CLAIM>(
      loop, is_del);
  claim->table.Emplace(claim, table);
  for (VAR *var : row_vars) {
    claim->col_values.AddUse(var);
  }
  loop->body.Emplace(loop, claim);

  VECTORAPPEND *const append =
      impl->operation_regions.CreateDerived<VECTORAPPEND>(
          claim, ProgramOperation::kAppendToInductionVector);
  append->vector.Emplace(append, claimed_set);
  for (VAR *var : row_vars) {
    append->tuple_vars.AddUse(var);
  }
  claim->body.Emplace(claim, append);
}

// Emit one net-frontier filter: each row of the batch's overdeletion or
// addition set whose outcome is a net presence change (net-deleted rows of
// the overdeletion set, net-added rows of the addition set) lands in the
// consolidated frontier that higher strata's seeds range over. A row both
// overdeleted and re-added this batch passes neither filter.
static void EmitFrontierFilter(ProgramImpl *impl, Context &context,
                               TABLE *table, bool is_del, SERIES *seq) {
  VECTOR *const claimed_set = TableDeltaVector(
      impl, context, table,
      is_del ? VectorKind::kOverdeleteSet : VectorKind::kAdditionSet);
  VECTOR *const frontier = TableDeltaVector(
      impl, context, table,
      is_del ? VectorKind::kNetRemovals : VectorKind::kNetAdditions);

  VECTORLOOP *const loop = impl->operation_regions.CreateDerived<VECTORLOOP>(
      impl->next_id++, seq, ProgramOperation::kLoopOverInductionVector);
  seq->AddRegion(loop);
  loop->vector.Emplace(loop, claimed_set);

  std::vector<VAR *> row_vars;
  for (auto i = 0u, max_i = table->columns.Size(); i < max_i; ++i) {
    row_vars.push_back(loop->defined_vars.Create(
        impl->next_id++, VariableRole::kVectorVariable));
  }

  CHECKMEMBER *const check =
      impl->operation_regions.CreateDerived<CHECKMEMBER>(
          loop, is_del ? MembershipPredicate::kNetDeleted
                       : MembershipPredicate::kNetAdded);
  check->table.Emplace(check, table);
  for (VAR *var : row_vars) {
    check->col_values.AddUse(var);
  }
  loop->body.Emplace(loop, check);

  VECTORAPPEND *const append =
      impl->operation_regions.CreateDerived<VECTORAPPEND>(
          check, ProgramOperation::kAppendToInductionVector);
  append->vector.Emplace(append, frontier);
  for (VAR *var : row_vars) {
    append->tuple_vars.AddUse(var);
  }
  check->OP::body.Emplace(check, append);
}

}  // namespace

// Build the per-stratum differential phases into the entry procedure: for
// each stratum in ascending order, the seed enumeration over lower strata's
// consolidated frontiers (frontier-vector loops walking delta chains, and
// dual-section TABLEJOINs fed by pivot appends), the claim drains of the
// stratum's differential tables (delete/add queues into the batch
// overdeletion/addition sets), and the net-frontier construction that
// higher strata's seeds range over.
void BuildStratumPhases(ProgramImpl *impl, Context &context, Query query) {
  (void) query;

  // Discover the branch chains out of every chain source, in the tables'
  // creation order (deterministic). A source is a table with a frontier:
  //   - a differential table outside inductions has both signs, built by
  //     its own stratum's frontier filters;
  //   - a differential table inside an induction has additions only,
  //     filled by the induction's output loop;
  //   - a monotone boundary table (its eager folds accumulate a
  //     net-additions frontier for cut consumers) has additions only.
  std::vector<BranchChain> branches;
  for (TABLE *table : impl->tables) {
    auto is_source = TableIsDifferential(table);
    if (!is_source) {
      if (auto it = context.table_delta_vecs.find(table);
          it != context.table_delta_vecs.end()) {
        is_source = it->second.count(
            static_cast<unsigned>(VectorKind::kNetAdditions)) != 0u;
      }
    }
    if (!is_source) {
      continue;
    }

    const auto num_cols = table->columns.Size();
    for (const QueryView &member : table->views) {
      if (member.IsInsert() || member.Columns().size() != num_cols) {
        continue;
      }
      std::vector<QueryView> path{member};
      DiscoverBranches(impl, table, path, member, branches);
    }
  }

  // The dual-section joins, one emission per join view, each with one
  // delta pivot vector shared by every feeding chain and sign.
  std::vector<JoinEmission> joins;
  std::unordered_map<QueryView, size_t> join_index;
  for (const BranchChain &branch : branches) {
    if (!branch.ends_at_join) {
      continue;
    }
    const QueryView join_view = branch.path.back();
    if (join_index.emplace(join_view, joins.size()).second) {
      joins.emplace_back(
          join_view,
          context.entry_proc->VectorFor(
              impl, VectorKind::kJoinPivots,
              QueryJoin::From(join_view).PivotColumns()),
          branch.stratum);
      CollectSectionTargets(impl, join_view, joins.back().targets);
    }
  }

  // The tables whose claim drains and frontier filters the phases own. A
  // table's drain stratum starts at its owner stratum and is lifted below
  // to the stratum of the latest fold into it.
  std::vector<TABLE *> phase_table_order;
  std::unordered_map<TABLE *, unsigned> drain_stratum;
  for (TABLE *table : impl->tables) {
    if (TableIsDifferential(table) && !TableHasInductiveView(table)) {
      phase_table_order.push_back(table);
      drain_stratum.emplace(table, TableOwnerStratum(table));
    }
  }

  if (branches.empty() && joins.empty() && phase_table_order.empty()) {
    return;
  }

  // The scheduling fixpoint. A data model's member views can straddle
  // strata, so the spec strata alone do not guarantee that a read runs
  // after the state it reads is final. Each emission unit is lifted until:
  //
  //   - a seed runs after its source's consolidated frontier is built
  //     (the source's drain stratum; an inductive source's output loop and
  //     a monotone boundary's eager folds fill theirs before any phase);
  //   - a negation gate and a join section run after the batch-final state
  //     of every table they read is settled (the table's insert drain);
  //   - a table's claim drain runs at or after every fold into it.
  //
  // Non-inductive dataflow is acyclic, so the lift reaches a fixpoint. When
  // no model straddles strata, every constraint is already satisfied and
  // the schedule is exactly the spec strata.
  const auto ready_after = [&](TABLE *table) -> unsigned {
    if (auto it = drain_stratum.find(table); it != drain_stratum.end()) {
      return it->second + 1u;
    }
    return 0u;
  };

  const auto negated_tables_ready = [&](const BranchChain &branch) {
    unsigned stratum = 0u;
    for (const QueryView &view : branch.path) {
      if (view.IsNegate()) {
        const QueryView negated_view =
            QueryNegate::From(view).NegatedView();
        DataModel *const negated_model =
            impl->view_to_model[negated_view]->FindAs<DataModel>();
        stratum = std::max(stratum, ready_after(negated_model->table));
      }
    }
    return stratum;
  };

  const auto lift = [](unsigned &slot, unsigned value, bool &changed) {
    if (slot < value) {
      slot = value;
      changed = true;
    }
  };

  for (auto changed = true; changed; ) {
    changed = false;

    for (BranchChain &branch : branches) {
      unsigned stratum = std::max(branch.stratum, ready_after(branch.source));
      stratum = std::max(stratum, negated_tables_ready(branch));

      if (branch.ends_at_join) {
        lift(joins[join_index[branch.path.back()]].stratum, stratum, changed);
      } else {
        lift(branch.stratum, stratum, changed);
        lift(drain_stratum[branch.target], stratum, changed);
      }
    }

    for (JoinEmission &emission : joins) {
      unsigned stratum = emission.stratum;
      for (QueryView side :
           QueryJoin::From(emission.join_view).JoinedViews()) {
        DataModel *const side_model =
            impl->view_to_model[side]->FindAs<DataModel>();
        stratum = std::max(stratum, ready_after(side_model->table));
      }
      lift(emission.stratum, stratum, changed);
      for (TABLE *target : emission.targets) {
        lift(drain_stratum[target], emission.stratum, changed);
      }
    }
  }

  // A join chain is emitted with its join's series.
  for (BranchChain &branch : branches) {
    if (branch.ends_at_join) {
      branch.stratum = joins[join_index[branch.path.back()]].stratum;
    }
  }

  // Cash the soundness precondition that the REDERIVE omission and the
  // always-`kNonRecursive` head fold rest on (see Phase 8c / `EmitHeadFold`):
  // every table an emission unit reads must be phase-final strictly before the
  // unit runs, i.e. its final drain stratum is strictly lower than the unit's
  // emission stratum. If it held, the emission never reads a table at its own
  // stratum, so no fold it performs closes a same-stratum recursion cycle and
  // the non-recursive counter class is sound. The scheduling fixpoint above
  // establishes this by construction; the assert makes it checked rather than
  // narrated. `ready_after(T)` is `drain_stratum[T] + 1` (0 if T is not phase
  // owned — inductive/monotone sources are filled before any phase runs), so
  // `ready_after(read) <= emission stratum` is exactly "read is strictly
  // lower."
#ifndef NDEBUG
  for (const BranchChain &branch : branches) {
    assert(ready_after(branch.source) <= branch.stratum);
    for (const QueryView &view : branch.path) {
      if (view.IsNegate()) {
        const QueryView negated_view = QueryNegate::From(view).NegatedView();
        DataModel *const negated_model =
            impl->view_to_model[negated_view]->FindAs<DataModel>();
        assert(ready_after(negated_model->table) <= branch.stratum);
      }
    }
  }
  for (const JoinEmission &emission : joins) {
    for (QueryView side :
         QueryJoin::From(emission.join_view).JoinedViews()) {
      DataModel *const side_model =
          impl->view_to_model[side]->FindAs<DataModel>();
      assert(ready_after(side_model->table) <= emission.stratum);
    }
  }
#endif  // NDEBUG

  // The strata that need a phase series, ascending.
  std::set<unsigned> strata;
  for (const BranchChain &branch : branches) {
    strata.insert(branch.stratum);
  }
  for (const JoinEmission &emission : joins) {
    strata.insert(emission.stratum);
  }
  for (TABLE *table : phase_table_order) {
    strata.insert(drain_stratum[table]);
  }

  // Nest the entry procedure's body (the ingest walk, which parks every
  // fold crossing in the queues the phases drain) ahead of the phase
  // series.
  PROC *const proc = context.entry_proc;
  SERIES *const seq = impl->series_regions.Create(proc);
  proc->body->parent = seq;
  seq->AddRegion(proc->body.get());
  proc->body.Emplace(proc, seq);

  for (const unsigned stratum : strata) {
    SERIES *const stratum_seq = impl->series_regions.Create(seq);
    seq->AddRegion(stratum_seq);

    // Seeds: every chain emitted at this stratum, per available sign. The
    // `-` sign ranges over the source's net removals (differential
    // non-inductive sources only); the `+` sign over its net additions.
    // Each consumed frontier is sort-uniqued first: a claim-built frontier
    // is duplicate-free already, but a monotone boundary's eager appends
    // (several same-model fold sites) and an induction's per-view output
    // loops can park one row more than once, and each frontier row must
    // seed exactly once.
    std::unordered_set<VECTOR *> uniqued_frontiers;
    const auto seed_vector = [&](TABLE *source, VectorKind kind) -> VECTOR * {
      VECTOR *const vec = TableDeltaVector(impl, context, source, kind);
      if (uniqued_frontiers.insert(vec).second) {
        VECTORUNIQUE *const unique =
            impl->operation_regions.CreateDerived<VECTORUNIQUE>(
                stratum_seq, ProgramOperation::kSortAndUniqueInductionVector);
        unique->vector.Emplace(unique, vec);
        stratum_seq->AddRegion(unique);
      }
      return vec;
    };

    for (const BranchChain &branch : branches) {
      if (branch.stratum != stratum) {
        continue;
      }

      // A phase-table source's frontier is complete strictly before this
      // series runs (the scheduling fixpoint's guarantee).
      TABLE *const source = branch.source;
      assert(!drain_stratum.count(source) ||
             drain_stratum.find(source)->second < stratum);

      VECTOR *join_pivot_vec = nullptr;
      if (branch.ends_at_join) {
        join_pivot_vec = joins[join_index[branch.path.back()]].pivot_vec;
      }

      if (TableIsDifferential(source) && !TableHasInductiveView(source)) {
        EmitSeedLoop(impl, context, branch, false /* is_add */,
                     seed_vector(source, VectorKind::kNetRemovals),
                     join_pivot_vec, stratum_seq);
      }
      EmitSeedLoop(impl, context, branch, true /* is_add */,
                   seed_vector(source, VectorKind::kNetAdditions),
                   join_pivot_vec, stratum_seq);
    }

    // Dual-section joins: the sort-uniqued pivot vector makes the join
    // enumerate each combination once per batch, so each section fires
    // exactly once per started/stopped rule instance.
    for (const JoinEmission &emission : joins) {
      if (emission.stratum != stratum) {
        continue;
      }

      VECTORUNIQUE *const unique =
          impl->operation_regions.CreateDerived<VECTORUNIQUE>(
              stratum_seq, ProgramOperation::kSortAndUniquePivotVector);
      unique->vector.Emplace(unique, emission.pivot_vec);
      stratum_seq->AddRegion(unique);

      auto [join, cmp] =
          BuildJoin(impl, QueryJoin::From(emission.join_view),
                    emission.pivot_vec, stratum_seq, true /* for_delta */);
      assert(cmp == nullptr);
      (void) cmp;

      LET *const added = impl->operation_regions.CreateDerived<LET>(join);
      join->added_body.Emplace(join, added);
      EmitSectionWalk(impl, context, emission.join_view, true /* is_add */,
                      added);

      LET *const removed = impl->operation_regions.CreateDerived<LET>(join);
      join->removed_body.Emplace(join, removed);
      EmitSectionWalk(impl, context, emission.join_view, false /* is_add */,
                      removed);
    }

    // Claim drains of the tables owned by this stratum, overdeletions
    // first, then their net-frontier filters. There is no REDERIVE step: the
    // partition drains only non-inductive differential tables, and the
    // fixpoint has lifted every emission above the strata it reads (the
    // assert-checked "reads are strictly lower" property), so no table here
    // carries a same-stratum recursive derivation and its recursive counter
    // is identically zero.
    //
    // The per-table interleaving (each table's del/add drain then del/add
    // filter, rather than a global del-then-add pass over all tables) is
    // unobservable while a stratum drains a single table: there is nothing to
    // interleave against, and the spec's within-table del-before-add order is
    // honored. A cross-table dependency would only be mis-ordered if one
    // stratum drained two tables whose reads are not strictly lower — which
    // the reads-are-lower assert above rules out. (Couples with that assert.)
    for (TABLE *table : phase_table_order) {
      if (drain_stratum[table] != stratum) {
        continue;
      }
      EmitClaimDrain(impl, context, table, true /* is_del */, stratum_seq);
      EmitClaimDrain(impl, context, table, false /* is_del */, stratum_seq);
      EmitFrontierFilter(impl, context, table, true /* is_del */,
                         stratum_seq);
      EmitFrontierFilter(impl, context, table, false /* is_del */,
                         stratum_seq);
    }
  }
}

}  // namespace hyde
