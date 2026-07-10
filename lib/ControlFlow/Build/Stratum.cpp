// Copyright 2020, Trail of Bits. All rights reserved.

#include <functional>
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

// A table's differential phases are owned by an induction when a
// `ProgramInductionRegion` was actually built for one of its member views
// (i.e. the view is keyed in `context.view_to_induction`, populated by
// `GetOrInitInduction` before this pass runs): the fixpoint machinery feeds
// and drains such tables, and the stratum phases leave them alone (except as
// chain sources, through the net-additions frontier the induction's output
// loop fills).
//
// This is strictly narrower than "has a view with an `InductionGroupId`": a
// recursive SCC whose views carry a group id but for which no induction
// region was built (a JOIN-terminal linear recursion whose union is not the
// owner of an emitted induction — e.g. transitive closure) is *not* induction
// owned. Its OVERDELETE/REDERIVE/INSERT is emitted here, at its own recursive
// stratum, per the derivation-counter design; routing it by group id would
// orphan it (no induction feeds it, and the phases would skip it too).
static bool TableIsInductionOwned(Context &context, TABLE *table) {
  for (const QueryView &view : table->views) {
    if (context.view_to_induction.count(view)) {
      return true;
    }
  }
  return false;
}

// Whether a single view is fed/drained by an induction region (see
// `TableIsInductionOwned`).
static bool ViewIsInductionOwned(Context &context, QueryView view) {
  return context.view_to_induction.count(view) != 0u;
}

// The mapping from a table to the identity of the stratum-phase-owned
// recursive SCC it belongs to (its group id). A table is in such an SCC iff
// it lies on a delta-edge cycle through a group-tagged (`InductionGroupId`)
// view for which no induction region was built. The `InductionGroupId`
// analysis tags only the SCC's anchor views (the recursive UNION and, for a
// linear recursion, its JOIN); the intermediate projection/plumbing tables on
// the cycle (a linear recursion's head projection, e.g.) carry no group id but
// ARE part of the recursion, and folds into them or the anchor via the
// recursive rule must be classed `kRecursive`. This map closes that gap by
// forward/backward reachability over the delta graph from the anchor views.
using RecursiveSccMap = std::unordered_map<TABLE *, unsigned>;

// The successor edges the recursion traverses (mirrors `FollowsDeltaEdge`
// minus the induction-ownership gate: the SCC computation reasons about the
// whole recursive shape, including any induction-owned neighbor that would
// close a cycle — there is none for a stratum-phase-owned SCC, but the gate is
// dropped so the reachability is topology-faithful).
static bool RecursionFollowsEdge(QueryView succ) {
  if (!succ.CanReceiveDeletions()) {
    return false;
  }
  if (succ.IsInsert() && QueryInsert::From(succ).IsStream()) {
    return false;
  }
  return true;
}

static void CollectReachable(QueryView view,
                             std::unordered_set<QueryView> &seen) {
  if (!seen.insert(view).second) {
    return;
  }
  for (QueryView succ : view.Successors()) {
    if (RecursionFollowsEdge(succ)) {
      CollectReachable(succ, seen);
    }
  }
}

// Compute the recursive-SCC membership of every table. For each group-tagged
// anchor view (not induction-owned), the set of views on a cycle through it —
// forward-reachable from it AND able to reach it — all belong to the anchor's
// SCC; their tables are mapped to the group id. Induction-owned recursion is
// left out entirely (handled by the fixpoint machinery).
static RecursiveSccMap ComputeRecursiveSCCs(ProgramImpl *impl,
                                            Context &context) {
  RecursiveSccMap scc_of_table;

  // Anchor views per group, and the reverse adjacency for backward reach.
  std::unordered_map<unsigned, std::vector<QueryView>> anchors;
  std::unordered_map<QueryView, std::vector<QueryView>> preds;
  for (TABLE *table : impl->tables) {
    for (const QueryView &view : table->views) {
      for (QueryView succ : view.Successors()) {
        if (RecursionFollowsEdge(succ)) {
          preds[succ].push_back(view);
        }
      }
      if (auto g = view.InductionGroupId();
          g.has_value() && !ViewIsInductionOwned(context, view)) {
        anchors[*g].push_back(view);
      }
    }
  }

  for (auto &[group, anchor_views] : anchors) {

    // Forward reach from every anchor.
    std::unordered_set<QueryView> forward;
    for (QueryView a : anchor_views) {
      CollectReachable(a, forward);
    }

    // Backward reach to any anchor (BFS over the reverse adjacency).
    std::unordered_set<QueryView> backward;
    std::vector<QueryView> stack(anchor_views.begin(), anchor_views.end());
    while (!stack.empty()) {
      QueryView v = stack.back();
      stack.pop_back();
      if (!backward.insert(v).second) {
        continue;
      }
      if (auto it = preds.find(v); it != preds.end()) {
        for (QueryView p : it->second) {
          stack.push_back(p);
        }
      }
    }

    // A view on a cycle through an anchor is in both sets; map its table.
    for (QueryView v : forward) {
      if (!backward.count(v)) {
        continue;
      }
      DataModel *const model = impl->view_to_model[v]->FindAs<DataModel>();
      if (model->table) {
        scc_of_table[model->table] = group;
      }
    }
  }

  return scc_of_table;
}

// The recursive-SCC identity of a table, or `nullopt` if the table is not
// part of a stratum-phase-owned recursive SCC.
static std::optional<unsigned> RecursiveSCC(const RecursiveSccMap &sccs,
                                            TABLE *table) {
  if (auto it = sccs.find(table); it != sccs.end()) {
    return it->second;
  }
  return std::nullopt;
}

// The counter class of a rule that folds into `target`, read from a source
// (chain source table, or the joined sides of a JOIN emission). A rule is
// `kRecursive` iff its head table and at least one read table are in the same
// recursive SCC — i.e. a body atom recursively derives the head. This is the
// rule's *fixed* class: it does not depend on the emission site's stratum
// position (a recursive rule seeded on its lower atom sits at an init
// position yet is still `kRecursive`, per the derivation-counter errata). A
// non-recursive rule (base rule, or any rule whose head is not in a recursive
// SCC) is `kNonRecursive`.
//
// `nonlinear` names the SCC groups whose linear claim-round fire cannot lower
// them (≥2 same-SCC join sides); a fold into such a group is classed
// `kNonRecursive` (the pre-A2 fallback, sound counters, no crash), because no
// claim-round loop is emitted to balance a `kRecursive` fold there.
static DerivClass RuleClass(const RecursiveSccMap &sccs,
                            const std::unordered_set<unsigned> &nonlinear,
                            TABLE *target,
                            const std::vector<TABLE *> &read_tables) {
  const auto target_scc = RecursiveSCC(sccs, target);
  if (!target_scc.has_value() || nonlinear.count(*target_scc)) {
    return DerivClass::kNonRecursive;
  }
  for (TABLE *read : read_tables) {
    if (RecursiveSCC(sccs, read) == target_scc) {
      return DerivClass::kRecursive;
    }
  }
  return DerivClass::kNonRecursive;
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
// owned by other machinery are skipped: induction-owned arrivals belong to
// the induction's fixpoint vectors, monotone consumers to the eager insertion
// walk, and stream INSERTs to the publication paths (a deletion-capable
// transmit publishes through its backing table's commit sweep).
//
// An arrival into a recursive SCC for which no induction region was built
// (`ViewIsInductionOwned` is false even though the view carries an
// `InductionGroupId`) IS followed: that SCC's differential phases are the
// stratum path's responsibility. The recursion is broken by the terminal-at-
// table-boundary rule in `DiscoverBranches` (a same-table successor is not
// re-entered), so following these edges does not diverge.
static bool FollowsDeltaEdge(Context &context, QueryView succ) {
  if (ViewIsInductionOwned(context, succ)) {
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
static void DiscoverBranches(ProgramImpl *impl, Context &context, TABLE *source,
                             std::vector<QueryView> &path, QueryView view,
                             std::vector<BranchChain> &branches) {
  for (QueryView succ : view.Successors()) {
    if (!FollowsDeltaEdge(context, succ)) {
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
      // machinery, not by stratum phases; the chain into it is dropped. A
      // recursive table for which no induction region was built is owned by
      // the stratum phases and IS recorded.
      if (!TableIsInductionOwned(context, table)) {
        BranchChain branch;
        branch.source = source;
        branch.path = path;
        branch.ends_at_join = false;
        branch.target = table;
        branch.stratum = TableOwnerStratum(table);
        branches.push_back(std::move(branch));
      }

    } else {
      DiscoverBranches(impl, context, source, path, succ, branches);
    }

    path.pop_back();
  }
}

// Collects the fold targets that a join's section walk reaches (a
// differential join is persisted, so this is normally just the join's own
// table), mirroring `EmitSectionWalk`'s recursion.
static void CollectSectionTargets(ProgramImpl *impl, Context &context,
                                  QueryView view,
                                  std::vector<TABLE *> &targets) {
  DataModel *const model = impl->view_to_model[view]->FindAs<DataModel>();
  TABLE *const table = model->table;
  if (table != nullptr) {
    if (!TableIsInductionOwned(context, table)) {
      targets.push_back(table);
    }
    return;
  }
  for (QueryView succ : view.Successors()) {
    if (FollowsDeltaEdge(context, succ)) {
      CollectSectionTargets(impl, context, succ, targets);
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

// Emit the terminal fold of a delta chain: one signed counter fold of
// `view`'s tuple into `table` in the counter class `deriv_class`, whose zero
// crossing parks the row in the table's add or delete queue for the owner
// stratum's claim drain. Claimed rows do not walk successors: all downstream
// propagation is higher-stratum seeds ranging over the table's net frontiers.
//
// The counter class is the *rule's fixed class*, not the emission-site's
// stratum position: a rule is `kRecursive` iff a body atom's derivation of
// the head is recursive (same recursive SCC), computed by the caller. For a
// non-recursive fold target — a table outside any recursive SCC, whose
// emission the scheduling fixpoint has lifted above every table it reads — the
// class is `kNonRecursive` and the assert-checked "all reads are strictly
// lower strata" property holds. For a recursive rule seeded on its lower atom
// the emission may sit at an init position yet still be `kRecursive`, because
// the class tracks the rule, not the site (see the seed-schema routing in
// `BuildStratumPhases`).
static void EmitHeadFold(ProgramImpl *impl, Context &context, QueryView view,
                         TABLE *table, bool is_add, DerivClass deriv_class,
                         OP *parent) {
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
                                             is_add, deriv_class);
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
                            QueryView view, bool is_add, DerivClass deriv_class,
                            OP *parent) {
  DataModel *const model = impl->view_to_model[view]->FindAs<DataModel>();
  TABLE *const table = model->table;

  if (table != nullptr) {
    if (!TableIsInductionOwned(context, table)) {
      EmitHeadFold(impl, context, view, table, is_add, deriv_class, parent);
    }
    return;
  }

  PARALLEL *const par = impl->parallel_regions.Create(parent);
  parent->body.Emplace(parent, par);
  for (QueryView succ : view.Successors()) {
    if (!FollowsDeltaEdge(context, succ)) {
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
    EmitSectionWalk(impl, context, succ, is_add, deriv_class, step);
  }
}

// Emit one seed: a loop over the source table's frontier vector `vec`
// binding the branch's member view's columns, followed by the chain's
// plumbing, ending at a head fold or at a pivot append into the join's
// delta pivot vector (`join_pivot_vec`, null for head chains).
static void EmitSeedLoop(ProgramImpl *impl, Context &context,
                         const BranchChain &branch, bool is_add,
                         DerivClass deriv_class, VECTOR *vec,
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
  EmitHeadFold(impl, context, head, model->table, is_add, deriv_class, parent);
}

// Emit one claim drain: sort-unique the queue of rows whose folds crossed
// zero this batch, then claim each row into the batch's overdeletion or
// addition set; the claim dedups (its body runs only on the row's first
// claim), so the set holds each claimed row once.
//
// `round_frontier` is the per-round claimed frontier (`$t_ΔD`/`$t_ΔA`, the
// `kClaimedDeleteFrontier`/`kClaimedAddFrontier` vector): non-null inside a
// recursive-SCC claim-round loop (§5.2/§5.3), where the claim appends the
// claimed row into BOTH the accumulated set (`D_s`/`A_s`, read by REDERIVE and
// the frontier build) AND the per-round frontier (the loop's break condition
// and the fixpoint fire's delta source). Null for an acyclic table's
// single-pass drain, whose claimed set is not iterated again.
static void EmitClaimDrain(ProgramImpl *impl, Context &context, TABLE *table,
                           bool is_del, SERIES *seq,
                           VECTOR *round_frontier = nullptr) {
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

  SERIES *const claimed_seq = impl->series_regions.Create(claim);
  claim->body.Emplace(claim, claimed_seq);

  VECTORAPPEND *const append =
      impl->operation_regions.CreateDerived<VECTORAPPEND>(
          claimed_seq, ProgramOperation::kAppendToInductionVector);
  append->vector.Emplace(append, claimed_set);
  for (VAR *var : row_vars) {
    append->tuple_vars.AddUse(var);
  }
  claimed_seq->AddRegion(append);

  if (round_frontier) {
    VECTORAPPEND *const round_append =
        impl->operation_regions.CreateDerived<VECTORAPPEND>(
            claimed_seq, ProgramOperation::kAppendToInductionVector);
    round_append->vector.Emplace(round_append, round_frontier);
    for (VAR *var : row_vars) {
      round_append->tuple_vars.AddUse(var);
    }
    claimed_seq->AddRegion(round_append);
  }
}

// Emit a retirement pass over the per-round claimed frontier `round_frontier`
// (`$t_ΔD`): a loop that clears each claimed row's current-round bit
// (`kDelNow`) so the next round's fixpoint reads a clean same-round frontier
// (§5.2 `RetireDelFrontier(Δ_D)`). Ranges over the CLAIMED set, not the
// drained queue.
static void EmitRetireFrontier(ProgramImpl *impl, TABLE *table, bool is_del,
                               VECTOR *round_frontier, SERIES *seq) {
  VECTORLOOP *const loop = impl->operation_regions.CreateDerived<VECTORLOOP>(
      impl->next_id++, seq, ProgramOperation::kLoopOverInductionVector);
  seq->AddRegion(loop);
  loop->vector.Emplace(loop, round_frontier);

  RETIRE *const retire =
      impl->operation_regions.CreateDerived<RETIRE>(loop, is_del);
  retire->table.Emplace(retire, table);
  for (auto i = 0u, max_i = table->columns.Size(); i < max_i; ++i) {
    VAR *const var = loop->defined_vars.Create(impl->next_id++,
                                               VariableRole::kVectorVariable);
    retire->col_values.AddUse(var);
  }
  loop->body.Emplace(loop, retire);
}

// Emit the REDERIVE pass for one recursive-SCC table (§5.2): a loop over the
// accumulated overdeletion set `D_s` that appends each still-recursively-
// supported row (`C_r > 0` after OVERDELETE quiesced — the
// `kRecursivelySupported` read) into the table's add queue, whence INSERT
// re-enters it. A counter read, not a search; ranges over `D_s`, not the
// per-round `Δ_D`.
static void EmitRederive(ProgramImpl *impl, Context &context, TABLE *table,
                         SERIES *seq) {
  VECTOR *const overdelete_set =
      TableDeltaVector(impl, context, table, VectorKind::kOverdeleteSet);
  VECTOR *const add_queue =
      TableDeltaVector(impl, context, table, VectorKind::kAddQueue);

  VECTORLOOP *const loop = impl->operation_regions.CreateDerived<VECTORLOOP>(
      impl->next_id++, seq, ProgramOperation::kLoopOverInductionVector);
  seq->AddRegion(loop);
  loop->vector.Emplace(loop, overdelete_set);

  std::vector<VAR *> row_vars;
  for (auto i = 0u, max_i = table->columns.Size(); i < max_i; ++i) {
    row_vars.push_back(loop->defined_vars.Create(
        impl->next_id++, VariableRole::kVectorVariable));
  }

  CHECKMEMBER *const check =
      impl->operation_regions.CreateDerived<CHECKMEMBER>(
          loop, MembershipPredicate::kRecursivelySupported);
  check->table.Emplace(check, table);
  for (VAR *var : row_vars) {
    check->col_values.AddUse(var);
  }
  loop->body.Emplace(loop, check);

  VECTORAPPEND *const append =
      impl->operation_regions.CreateDerived<VECTORAPPEND>(
          check, ProgramOperation::kAppendToInductionVector);
  append->vector.Emplace(append, add_queue);
  for (VAR *var : row_vars) {
    append->tuple_vars.AddUse(var);
  }
  check->OP::body.Emplace(check, append);
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

// Emit the OVERDELETE fixpoint fire of one recursive JOIN, delta over the
// per-round claimed-delete frontier of its same-SCC side (§5.2 "for each
// fixpoint-schema firing (OVERDELETE column), delta over Δ_D"). For the linear
// recursion `t(F,T) : t(F,Y), edge(Y,T)` the same-SCC side is `t` and the
// lower side `edge` is read at its batch-final state (`kInNew`, per the
// fixpoint-schema table MD §5.1: lower `j < i` reads `InNew`):
//
//   vector-loop {F,Y} over Δ_D(t)          « the newly claimed-deleted t rows »
//     scan edge where edge.Y = Y  →  {edge.T}    « lower atom, index scan »
//       check-member InNew {Y,T} in edge
//         if-member: update-count -recursive t(F,T)   « the join's persisted »
//           if-crossed: append into the join table's delete queue
//
// The fold lands on the JOIN's own persisted table (the recursion anchor),
// whose net-removal frontier the next round's projection seeds carry onward.
// Restricted to the linear shape: exactly one same-SCC joined side, every
// other side a strictly-lower table read via `kInNew`. A nonlinear recursion
// (≥2 same-SCC sides) needs the round-relative predicate matrix
// (`SurvivesSoFar`/`AliveAtClaim`), deferred to the matrix-fixture slice; it
// is asserted out here rather than mis-lowered.
static void EmitJoinFire(ProgramImpl *impl, Context &context,
                         const RecursiveSccMap &sccs,
                         const JoinEmission &emission, bool is_del,
                         SERIES *seq) {
  const QueryJoin join_view = QueryJoin::From(emission.join_view);
  DataModel *const join_model =
      impl->view_to_model[emission.join_view]->FindAs<DataModel>();
  TABLE *const join_table = join_model->table;
  const auto join_scc = RecursiveSCC(sccs, join_table);
  assert(join_scc.has_value());

  // Partition the joined sides into the same-SCC delta side and the lower
  // (InNew-read) sides. Linear recursion: exactly one same-SCC side.
  std::optional<QueryView> delta_side;
  std::vector<QueryView> lower_sides;
  for (QueryView side : join_view.JoinedViews()) {
    TABLE *const side_table =
        impl->view_to_model[side]->FindAs<DataModel>()->table;
    if (RecursiveSCC(sccs, side_table) == join_scc) {
      assert(!delta_side.has_value());  // Linear recursion only (doc comment).
      delta_side = side;
    } else {
      lower_sides.push_back(side);
    }
  }
  assert(delta_side.has_value());

  TABLE *const delta_table =
      impl->view_to_model[*delta_side]->FindAs<DataModel>()->table;
  VECTOR *const round_frontier = TableDeltaVector(
      impl, context, delta_table,
      is_del ? VectorKind::kClaimedDeleteFrontier
             : VectorKind::kClaimedAddFrontier);

  // Loop over the claimed-deleted rows of the same-SCC side, binding that
  // side's columns.
  VECTORLOOP *const loop = impl->operation_regions.CreateDerived<VECTORLOOP>(
      impl->next_id++, seq, ProgramOperation::kLoopOverInductionVector);
  seq->AddRegion(loop);
  loop->vector.Emplace(loop, round_frontier);

  for (auto col : delta_side->Columns()) {
    VAR *const var = loop->defined_vars.Create(impl->next_id++,
                                               VariableRole::kVectorVariable);
    var->query_column = col;
    loop->col_id_to_var.emplace(col.Id(), var);
  }

  // Bind the join's pivot output variables from the delta side's columns: the
  // pivot value equals the delta side's pivot input column. This makes the
  // pivot available to the lower-side scans below.
  for (auto j = 0u, num_pivots = join_view.NumPivotColumns(); j < num_pivots;
       ++j) {
    const QueryColumn out_pivot = join_view.NthOutputPivotColumn(j);
    for (QueryColumn in_pivot : join_view.NthInputPivotSet(j)) {
      if (QueryView::Containing(in_pivot) == *delta_side) {
        loop->col_id_to_var.emplace(out_pivot.Id(),
                                    loop->VariableFor(impl, in_pivot));
        break;
      }
    }
  }

  // Bind the join's non-pivot outputs contributed by the delta side directly
  // (the lower sides' non-pivot outputs are bound by their scans).
  join_view.ForEachUse([&](QueryColumn in_col, InputColumnRole role,
                           std::optional<QueryColumn> out_col) {
    if (!out_col || role != InputColumnRole::kJoinNonPivot) {
      return;
    }
    if (QueryView::Containing(in_col) == *delta_side) {
      loop->col_id_to_var.emplace(out_col->Id(),
                                  loop->VariableFor(impl, in_col));
    }
  });

  // Nest the lower-side scans. Each lower side is scanned by its pivot
  // connection (the pivot value is bound above); the scan completes the side's
  // columns, then a `kInNew` gate reads the side at its batch-final state.
  // Finally fold `-recursive` into the join's persisted table.
  const auto emit_fold = [&](SERIES *inner_seq) {

    // Bind the join's non-pivot output columns contributed by the lower sides
    // to the sides' scanned column variables (the scans bind the SIDE's
    // columns, whose ids differ from the join outputs'; the delta side's
    // outputs and all pivots are already bound on the outer loop). Resolve
    // through `inner_seq`, which nests under the sides' scans.
    join_view.ForEachUse([&](QueryColumn in_col, InputColumnRole role,
                             std::optional<QueryColumn> out_col) {
      if (!out_col || role != InputColumnRole::kJoinNonPivot) {
        return;
      }
      if (QueryView::Containing(in_col) != *delta_side) {
        inner_seq->col_id_to_var.emplace(out_col->Id(),
                                         inner_seq->VariableFor(impl, in_col));
      }
    });

    std::vector<QueryColumn> cols;
    for (auto col : emission.join_view.Columns()) {
      cols.push_back(col);
    }
    UPDATECOUNT *const fold = BuildUpdateCount(
        impl, join_table, inner_seq, cols, !is_del /* is_add */,
        DerivClass::kRecursive);
    inner_seq->AddRegion(fold);

    VECTOR *const queue = TableDeltaVector(
        impl, context, join_table,
        is_del ? VectorKind::kDeleteQueue : VectorKind::kAddQueue);
    VECTORAPPEND *const append =
        impl->operation_regions.CreateDerived<VECTORAPPEND>(
            fold, ProgramOperation::kAppendToInductionVector);
    append->vector.Emplace(append, queue);
    for (auto col : cols) {
      append->tuple_vars.AddUse(fold->VariableFor(impl, col));
    }
    fold->body.Emplace(fold, append);
  };

  // Recursively scan each lower side under `parent_seq`, reading it at
  // `kInNew`, then fold at the deepest level. Each level attaches its scan and
  // gate under the SERIES it is handed; the gate's member body is a fresh
  // SERIES the next level attaches to.
  std::function<void(size_t, SERIES *)> scan_lower =
      [&](size_t idx, SERIES *parent_seq) {
        if (idx == lower_sides.size()) {
          emit_fold(parent_seq);
          return;
        }
        const QueryView side = lower_sides[idx];
        TABLE *const side_table =
            impl->view_to_model[side]->FindAs<DataModel>()->table;

        // The side's pivot columns are bound by the pivot connection (the
        // pivot output variable is in scope); the scan completes the rest.
        std::vector<QueryColumn> avail;
        for (auto j = 0u, num_pivots = join_view.NumPivotColumns();
             j < num_pivots; ++j) {
          for (QueryColumn in_pivot : join_view.NthInputPivotSet(j)) {
            if (QueryView::Containing(in_pivot) == side) {
              parent_seq->col_id_to_var.emplace(
                  in_pivot.Id(),
                  parent_seq->VariableFor(impl,
                                          join_view.NthOutputPivotColumn(j)));
              avail.push_back(in_pivot);
            }
          }
        }

        BuildMaybeScanPartial(
            impl, side, avail, side_table, parent_seq,
            [&](REGION *in_scan, bool) -> REGION * {
              // Read the lower side at its batch-final state (`kInNew`), then
              // descend to the next lower side (or the fold).
              std::vector<QueryColumn> side_cols;
              for (auto col : side.Columns()) {
                side_cols.push_back(col);
              }
              return BuildCheckMember(
                  impl, in_scan, side_table, side_cols,
                  MembershipPredicate::kInNew,
                  [&](ProgramImpl *impl_, REGION *in_check) -> REGION * {
                    SERIES *const next_seq =
                        impl_->series_regions.Create(in_check);
                    scan_lower(idx + 1u, next_seq);
                    return next_seq;
                  },
                  [](ProgramImpl *, REGION *) -> REGION * { return nullptr; });
            });
      };

  SERIES *const body_seq = impl->series_regions.Create(loop);
  loop->body.Emplace(loop, body_seq);
  scan_lower(0u, body_seq);
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

  // The recursive-SCC membership of every table (empty for programs with no
  // stratum-phase-owned recursion). Drives the rule-class of folds into
  // recursive tables and the same-SCC read exemption in the readiness assert.
  RecursiveSccMap recursive_sccs = ComputeRecursiveSCCs(impl, context);

  // Identify NONLINEAR recursion: an SCC group whose fold shape the linear
  // claim-round fire cannot handle — some JOIN whose persisted table is in the
  // group has ≥2 same-SCC joined sides (e.g. `tc(F,T):tc(F,X),tc(X,T)`). Such a
  // group needs the round-relative predicate matrix
  // (`SurvivesSoFar`/`AliveAtClaim`) — the matrix-fixture slice. Its tables
  // STAY in `recursive_sccs` for SCHEDULING purposes (the scheduling fixpoint's
  // same-SCC read exemption and shared-stratum pinning are termination
  // requirements, independent of whether a claim-round loop is emitted; without
  // them the `lift` fixpoint diverges on the SCC's mutual reads). But
  // `RecursiveRuleClass`/`IsLinearRecursiveGroup` below treat them as
  // non-recursive for FOLD-CLASS and LOOP-EMISSION, so they fall back to the
  // pre-A2 single-pass path (wrong differential output, but sound counters and
  // no crash) rather than being mis-lowered.
  std::unordered_set<unsigned> nonlinear_groups;
  for (TABLE *table : impl->tables) {
    const auto group = RecursiveSCC(recursive_sccs, table);
    if (!group.has_value()) {
      continue;
    }
    for (const QueryView &view : table->views) {
      if (!view.IsJoin()) {
        continue;
      }
      unsigned same_scc_sides = 0u;
      for (QueryView side : QueryJoin::From(view).JoinedViews()) {
        TABLE *const st =
            impl->view_to_model[side]->FindAs<DataModel>()->table;
        if (RecursiveSCC(recursive_sccs, st) == group) {
          ++same_scc_sides;
        }
      }
      if (same_scc_sides >= 2u) {
        nonlinear_groups.insert(*group);
      }
    }
  }

  // Whether a table is in a LINEAR recursive SCC — the shape whose folds class
  // `kRecursive` and whose claim-round OVERDELETE/REDERIVE/INSERT loop is
  // emitted. A table in a nonlinear SCC answers false here (scheduled as
  // recursive, but folded/lowered as non-recursive; see above).
  const auto is_linear_recursive = [&](TABLE *table) -> bool {
    const auto g = RecursiveSCC(recursive_sccs, table);
    return g.has_value() && !nonlinear_groups.count(*g);
  };

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
      DiscoverBranches(impl, context, table, path, member, branches);
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
      CollectSectionTargets(impl, context, join_view, joins.back().targets);
    }
  }

  // The tables whose claim drains and frontier filters the phases own. A
  // table's drain stratum starts at its owner stratum and is lifted below
  // to the stratum of the latest fold into it.
  std::vector<TABLE *> phase_table_order;
  std::unordered_map<TABLE *, unsigned> drain_stratum;
  for (TABLE *table : impl->tables) {
    if (TableIsDifferential(table) && !TableIsInductionOwned(context, table)) {
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

  // Two tables in the same recursive SCC drain together at one stratum, so a
  // read of a same-SCC table is not a strictly-lower readiness dependency and
  // must not lift the reader (that lift would never converge — the SCC's
  // mutual reads would ratchet each other's stratum up without bound). A
  // same-SCC read is `InI`-batch-frozen or a lower-table `InNew`; it closes
  // no scheduling cycle. `ready_across` returns the source's readiness unless
  // it shares `head`'s recursive SCC, in which case it contributes nothing.
  const auto same_scc = [&](TABLE *a, TABLE *b) {
    const auto sa = RecursiveSCC(recursive_sccs, a);
    return sa.has_value() && sa == RecursiveSCC(recursive_sccs, b);
  };
  const auto ready_across = [&](TABLE *head, TABLE *read) -> unsigned {
    return same_scc(head, read) ? 0u : ready_after(read);
  };

  const auto negated_tables_ready = [&](const BranchChain &branch,
                                        TABLE *head) {
    unsigned stratum = 0u;
    for (const QueryView &view : branch.path) {
      if (view.IsNegate()) {
        const QueryView negated_view =
            QueryNegate::From(view).NegatedView();
        DataModel *const negated_model =
            impl->view_to_model[negated_view]->FindAs<DataModel>();
        stratum = std::max(stratum, ready_across(head, negated_model->table));
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

      // The head table this branch folds into: its own target for a head
      // chain, the join's persisted table for a join chain. Same-SCC reads
      // (including the source frontier of a recursive rule) are exempt.
      TABLE *const head =
          branch.ends_at_join
              ? impl->view_to_model[branch.path.back()]->FindAs<DataModel>()
                    ->table
              : branch.target;
      unsigned stratum =
          std::max(branch.stratum, ready_across(head, branch.source));
      stratum = std::max(stratum, negated_tables_ready(branch, head));

      if (branch.ends_at_join) {
        lift(joins[join_index[branch.path.back()]].stratum, stratum, changed);
      } else {
        lift(branch.stratum, stratum, changed);
        lift(drain_stratum[branch.target], stratum, changed);
      }
    }

    for (JoinEmission &emission : joins) {
      TABLE *const head =
          impl->view_to_model[emission.join_view]->FindAs<DataModel>()->table;
      unsigned stratum = emission.stratum;
      for (QueryView side :
           QueryJoin::From(emission.join_view).JoinedViews()) {
        DataModel *const side_model =
            impl->view_to_model[side]->FindAs<DataModel>();
        stratum = std::max(stratum, ready_across(head, side_model->table));
      }
      lift(emission.stratum, stratum, changed);
      for (TABLE *target : emission.targets) {
        lift(drain_stratum[target], emission.stratum, changed);
      }
    }

    // Pin every table of a recursive SCC to a single shared drain stratum:
    // the max over its members. The SCC drains as one fixpoint, so its seed,
    // join, claim-round loop, REDERIVE and frontier build all run in the one
    // stratum series. (Without this, the members' owner strata could differ —
    // t's union, its recursive join, and the head projection each carry their
    // own — and the claim-round loop would be split across strata.)
    if (!recursive_sccs.empty()) {
      std::unordered_map<unsigned, unsigned> scc_stratum;
      for (auto &[table, group] : recursive_sccs) {
        auto it = drain_stratum.find(table);
        if (it != drain_stratum.end()) {
          unsigned &slot = scc_stratum[group];
          slot = std::max(slot, it->second);
        }
      }
      for (auto &[table, group] : recursive_sccs) {
        if (auto it = drain_stratum.find(table); it != drain_stratum.end()) {
          lift(it->second, scc_stratum[group], changed);
        }
      }
    }
  }

  // A join chain is emitted with its join's series.
  for (BranchChain &branch : branches) {
    if (branch.ends_at_join) {
      branch.stratum = joins[join_index[branch.path.back()]].stratum;
    }
  }

  // Cash the readiness precondition, SPLIT by emission-site kind (A4).
  //
  // For a NON-RECURSIVE emission — a base-rule seed, or any fold whose head
  // table is not in a recursive SCC — every table it reads must be phase-final
  // strictly before it runs: its drain stratum is strictly lower than the
  // emission's. This is the original soundness precondition the REDERIVE
  // omission and the `kNonRecursive` head-fold class rest on (see Phase 8c /
  // `EmitHeadFold`): the emission reads no table at its own stratum, so no fold
  // it performs closes a same-stratum recursion cycle, and its non-recursive
  // counter class is sound. It survives verbatim for base-rule seeds.
  //
  // For a RECURSIVE emission — a fold whose head shares a recursive SCC with a
  // read — the same-SCC reads are EXEMPT: they are `InI` batch-frozen (the
  // seed-schema same-stratum read) or a lower-table `InNew`, never a
  // drain-order dependency, so they close no scheduling cycle. Only its
  // strictly-lower (non-SCC) reads must be phase-final before it runs. And a
  // recursive emission must have at least one same-SCC read — that same-SCC
  // read IS the recursion; without it the fold would not be recursive and the
  // `kRecursive` class would be wrong.
  //
  // `ready_after(T)` is `drain_stratum[T] + 1` (0 if T is not phase owned), so
  // `ready_across(head, read) <= emission stratum` is "read is strictly lower
  // OR same-SCC (exempt)."
#ifndef NDEBUG
  for (const BranchChain &branch : branches) {
    TABLE *const head =
        branch.ends_at_join
            ? impl->view_to_model[branch.path.back()]->FindAs<DataModel>()
                  ->table
            : branch.target;
    assert(ready_across(head, branch.source) <= branch.stratum);
    for (const QueryView &view : branch.path) {
      if (view.IsNegate()) {
        const QueryView negated_view = QueryNegate::From(view).NegatedView();
        DataModel *const negated_model =
            impl->view_to_model[negated_view]->FindAs<DataModel>();
        assert(ready_across(head, negated_model->table) <= branch.stratum);
      }
    }
  }
  for (const JoinEmission &emission : joins) {
    TABLE *const head =
        impl->view_to_model[emission.join_view]->FindAs<DataModel>()->table;
    bool has_same_scc_read = false;
    for (QueryView side :
         QueryJoin::From(emission.join_view).JoinedViews()) {
      DataModel *const side_model =
          impl->view_to_model[side]->FindAs<DataModel>();
      assert(ready_across(head, side_model->table) <= emission.stratum);
      has_same_scc_read |= same_scc(head, side_model->table);
    }

    // A join emitting into a recursive SCC is a recursive rule (the JOIN's
    // persisted table is on the cycle); it must read a same-SCC side. A join
    // outside any recursive SCC has no same-SCC read (trivially).
    assert(!RecursiveSCC(recursive_sccs, head).has_value() ||
           has_same_scc_read);
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

      // A same-SCC internal projection edge (a head chain whose source and
      // target share a recursive SCC — e.g. the join-table → head-projection →
      // union edges of a linear recursion) is NOT seeded here: it fires
      // exclusively inside the SCC's claim-round loop, delta over the
      // predecessor's per-round `Δ_D`. Seeding it here too (over the source's
      // accumulated net-removal frontier) would double-decrement each internal
      // derivation once per round. The recursive-rule SEED that round 0 owns is
      // the JOIN over the SCC's LOWER predecessor frontier (handled by the join
      // emission below); the base rule and any non-SCC source stay here.
      if (!branch.ends_at_join && is_linear_recursive(branch.target) &&
          same_scc(branch.target, branch.source)) {
        continue;
      }

      // A phase-table source's frontier is complete strictly before this
      // series runs (the scheduling fixpoint's guarantee), UNLESS the source
      // is same-SCC as the fold's head — a recursive rule seeded on its own
      // SCC's frontier reads it batch-frozen (`InI`) at the same stratum, so
      // it is exempt (A4).
      TABLE *const source = branch.source;
      TABLE *const branch_head =
          branch.ends_at_join
              ? impl->view_to_model[branch.path.back()]->FindAs<DataModel>()
                    ->table
              : branch.target;
      assert(!drain_stratum.count(source) ||
             drain_stratum.find(source)->second < stratum ||
             same_scc(branch_head, source));

      VECTOR *join_pivot_vec = nullptr;
      DerivClass branch_class = DerivClass::kNonRecursive;
      if (branch.ends_at_join) {
        join_pivot_vec = joins[join_index[branch.path.back()]].pivot_vec;

        // A join-terminal seed only appends pivots; the fold's class is
        // decided by the join emission itself (below). Leave `branch_class`
        // at its unused default.
      } else {

        // A head-chain seed folds into `branch.target`. Its rule reads only
        // the source frontier (base rule ⇒ NR when the source is a lower,
        // non-SCC table; recursive when the source shares the target's SCC,
        // e.g. the join-table → union edge of a linear recursion).
        branch_class = RuleClass(recursive_sccs, nonlinear_groups, branch.target, {source});
      }

      if (TableIsDifferential(source) &&
          !TableIsInductionOwned(context, source)) {
        EmitSeedLoop(impl, context, branch, false /* is_add */, branch_class,
                     seed_vector(source, VectorKind::kNetRemovals),
                     join_pivot_vec, stratum_seq);
      }
      EmitSeedLoop(impl, context, branch, true /* is_add */, branch_class,
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

      // The join emission's fold class is the recursive rule's fixed class:
      // recursive iff the join's own (persisted) table shares a recursive SCC
      // with any joined side (e.g. the linear-recursion rule `t:t,edge` reads
      // `t` at the same recursive stratum it folds into). The section walk
      // folds into the join's table, so classify against it.
      DataModel *const join_model =
          impl->view_to_model[emission.join_view]->FindAs<DataModel>();
      std::vector<TABLE *> side_tables;
      for (QueryView side :
           QueryJoin::From(emission.join_view).JoinedViews()) {
        side_tables.push_back(
            impl->view_to_model[side]->FindAs<DataModel>()->table);
      }
      const DerivClass join_class =
          RuleClass(recursive_sccs, nonlinear_groups, join_model->table, side_tables);

      LET *const added = impl->operation_regions.CreateDerived<LET>(join);
      join->added_body.Emplace(join, added);
      EmitSectionWalk(impl, context, emission.join_view, true /* is_add */,
                      join_class, added);

      LET *const removed = impl->operation_regions.CreateDerived<LET>(join);
      join->removed_body.Emplace(join, removed);
      EmitSectionWalk(impl, context, emission.join_view, false /* is_add */,
                      join_class, removed);
    }

    // Partition this stratum's phase tables into single-pass tables and
    // linear-recursive-SCC tables (whose OVERDELETE/INSERT is a claim-round
    // fixpoint per §5.2/§5.3 — see below). A single-pass table's every fold is
    // `kNonRecursive`, so its recursive counter is identically zero and one
    // drain settles it. This holds for a genuinely acyclic table (the
    // scheduling fixpoint lifted its emissions above every table it reads) and
    // for a nonlinear-SCC table (scheduled as recursive — a shared stratum with
    // the same-SCC read exemption — but FOLDED non-recursively and lowered
    // single-pass, the pre-A2 fallback, since the linear claim-round fire
    // cannot handle its ≥2 same-SCC join sides).
    std::vector<TABLE *> acyclic_tables;
    std::vector<TABLE *> scc_tables;
    for (TABLE *table : phase_table_order) {
      if (drain_stratum[table] != stratum) {
        continue;
      }
      if (is_linear_recursive(table)) {
        scc_tables.push_back(table);
      } else {
        acyclic_tables.push_back(table);
      }
    }

    // Acyclic claim drains, overdeletions first, then their net-frontier
    // filters. The per-table interleaving (each table's del/add drain then
    // del/add filter, rather than a global del-then-add pass over all tables)
    // is unobservable: each such table's reads are strictly lower (the
    // reads-are-lower assert above rules this in), so there is no same-stratum
    // cross-table dependency to mis-order, and the spec's within-table
    // del-before-add order is honored.
    for (TABLE *table : acyclic_tables) {
      EmitClaimDrain(impl, context, table, true /* is_del */, stratum_seq);
      EmitClaimDrain(impl, context, table, false /* is_del */, stratum_seq);
      EmitFrontierFilter(impl, context, table, true /* is_del */,
                         stratum_seq);
      EmitFrontierFilter(impl, context, table, false /* is_del */,
                         stratum_seq);
    }

    if (scc_tables.empty()) {
      continue;
    }

    // The set of SCC group ids drained at this stratum (the SCC tables share
    // one drain stratum, so this is normally a single group).
    std::unordered_set<unsigned> scc_groups_here;
    for (TABLE *table : scc_tables) {
      scc_groups_here.insert(*RecursiveSCC(recursive_sccs, table));
    }

    // The join emissions whose persisted table is in an SCC drained at this
    // stratum — their fixpoint fire runs each round HERE, regardless of where
    // the seed join itself is scheduled (the seed join runs once at its own,
    // possibly lower, stratum; the fire is part of the SCC's claim-round loop).
    std::vector<const JoinEmission *> scc_joins;
    for (const JoinEmission &emission : joins) {
      DataModel *const jm =
          impl->view_to_model[emission.join_view]->FindAs<DataModel>();
      if (auto g = RecursiveSCC(recursive_sccs, jm->table);
          g.has_value() && scc_groups_here.count(*g)) {
        scc_joins.push_back(&emission);
      }
    }

    // Build one signed claim-round fixpoint for the SCC (§5.2 OVERDELETE for
    // `is_del`, §5.3 INSERT for the add side). The base- and recursive-rule
    // seed folds have already parked their zero-crossings in the SCC tables'
    // queues (the seed/join loops above). Each round drains the queues, claims
    // the crossing rows into the per-round claimed frontier `Δ` (the
    // kClaimedDeleteFrontier/kClaimedAddFrontier vector) AND the accumulated
    // set (`D_s`/`A_s`, the kOverdeleteSet/kAdditionSet vector), fires the
    // recursive rules DELTA over `Δ` (JOINs through their directional scan
    // reading each lower atom at its batch-final `kInNew` state; projection
    // edges by re-projecting the predecessor's `Δ`), and retires `Δ`. The loop
    // breaks when a round claims nothing (a `Δ`-emptiness break — NOT queue-
    // emptiness, which a diamond re-enqueue could live-lock; MD §5.2/§5.3, B3).
    //
    // The loop is an INDUCTION vehicle whose only maintained vectors are the
    // per-round `Δ` frontiers, so codegen's `for (changed; changed;
    // changed = !all_Δ_empty()) { round }` is exactly the break-on-claim-
    // progress discipline.
    const auto build_claim_round_loop = [&](bool is_del, SERIES *out_seq)
        -> INDUCTION * {
      const VectorKind frontier_kind = is_del
                                           ? VectorKind::kClaimedDeleteFrontier
                                           : VectorKind::kClaimedAddFrontier;

      INDUCTION *const loop = impl->induction_regions.Create(impl, out_seq);
      loop->parent = out_seq;
      out_seq->AddRegion(loop);

      PARALLEL *const round_par = impl->parallel_regions.Create(loop);
      loop->cyclic_region.Emplace(loop, round_par);
      SERIES *const round_seq = impl->series_regions.Create(round_par);
      round_par->AddRegion(round_seq);

      // Per round: clear each SCC table's `Δ`, drain + claim (into `Δ` and the
      // accumulated set), fire the recursive rules delta over `Δ`, retire `Δ`.
      for (TABLE *table : scc_tables) {
        VECTOR *const round_frontier =
            TableDeltaVector(impl, context, table, frontier_kind);
        loop->vectors.AddUse(round_frontier);

        VECTORCLEAR *const clear =
            impl->operation_regions.CreateDerived<VECTORCLEAR>(
                round_seq, ProgramOperation::kClearInductionVector);
        clear->vector.Emplace(clear, round_frontier);
        round_seq->AddRegion(clear);
      }
      for (TABLE *table : scc_tables) {
        VECTOR *const round_frontier =
            TableDeltaVector(impl, context, table, frontier_kind);
        EmitClaimDrain(impl, context, table, is_del, round_seq,
                       round_frontier);
      }

      // Re-fire the recursive rules DELTA over the per-round claimed frontier
      // `Δ` — NOT the accumulated net frontier (which never clears within the
      // loop, so re-firing over it would double-count a row claimed in an
      // earlier round). Each claimed row is in `Δ` for exactly the one round it
      // is claimed, so each recursive derivation is folded once. The seed/join
      // loops above already fired round 0 from the batch's lower-stratum
      // frontiers; this re-fire propagates the SCC-internal cascade.
      for (const JoinEmission *emission : scc_joins) {
        EmitJoinFire(impl, context, recursive_sccs, *emission, is_del,
                     round_seq);
      }
      for (const BranchChain &branch : branches) {
        if (branch.ends_at_join) {
          continue;
        }
        auto tgt_scc = RecursiveSCC(recursive_sccs, branch.target);
        if (tgt_scc.has_value() && scc_groups_here.count(*tgt_scc) &&
            same_scc(branch.target, branch.source) &&
            TableIsDifferential(branch.source) &&
            !TableIsInductionOwned(context, branch.source)) {
          EmitSeedLoop(impl, context, branch, !is_del /* is_add */,
                       DerivClass::kRecursive,
                       TableDeltaVector(impl, context, branch.source,
                                        frontier_kind),
                       nullptr, round_seq);
        }
      }
      for (TABLE *table : scc_tables) {
        VECTOR *const round_frontier =
            TableDeltaVector(impl, context, table, frontier_kind);
        EmitRetireFrontier(impl, table, is_del, round_frontier, round_seq);
      }
      return loop;
    };

    // OVERDELETE (§5.2): the del-side claim-round fixpoint, then — in its
    // output region, after the fixpoint quiesces — build each table's
    // accumulated net-removal frontier (the consolidated delta higher strata's
    // seeds range over) and run REDERIVE (every overdeleted row still
    // recursively supported, `C_r > 0`, re-enters via the add queue; a counter
    // read over `D_s`). REDERIVE's output feeds the INSERT add queue below.
    INDUCTION *const del_loop = build_claim_round_loop(true, stratum_seq);
    SERIES *const del_output = impl->series_regions.Create(del_loop);
    del_loop->output_region.Emplace(del_loop, del_output);
    for (TABLE *table : scc_tables) {
      EmitFrontierFilter(impl, context, table, true /* is_del */, del_output);
    }
    for (TABLE *table : scc_tables) {
      EmitRederive(impl, context, table, del_output);
    }

    // INSERT (§5.3): the add-side claim-round fixpoint (mirror of OVERDELETE),
    // draining the add queue — the batch's `+` seeds AND REDERIVE's output —
    // through the same claim/fire/retire discipline so that recursive
    // derivations are ADDED (their `C_r` incremented) symmetrically with the
    // del side. Its output builds each table's accumulated net-addition
    // frontier for higher strata.
    INDUCTION *const add_loop = build_claim_round_loop(false, stratum_seq);
    SERIES *const add_output = impl->series_regions.Create(add_loop);
    add_loop->output_region.Emplace(add_loop, add_output);
    for (TABLE *table : scc_tables) {
      EmitFrontierFilter(impl, context, table, false /* is_del */, add_output);
    }
  }
}

}  // namespace hyde
