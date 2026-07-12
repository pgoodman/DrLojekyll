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

// One negation-crossover emission (the §5.4 crossover, D1'): the dual of the
// forward pass. The forward pass folds a pred-row delta into the negate's
// table gated on the negated key's absence; the crossover folds the SAME
// negate row when the NEGATED key crosses, sign-DUALIZED — a negated-view
// GAIN retracts the negate output (the `-` arm, over the negated table's
// net-additions frontier — always present), a negated-view LOSS re-derives it
// (the `+` arm, over net-removals — only when the negated table is
// differential). Each arm loops the negated frontier, scans the pred table by
// the shared key, reads the pred at `kInNew` (R4), and folds one signed count
// into the negate's OWN table (R6), whose queue crossing rides the existing
// claim drain / frontier filters already emitted for that table.
//
// Discovered one per non-@never `QueryNegate` (a @never negate never
// retracts, so it has no crossover — R1). Registered with the scheduling
// fixpoint like a branch (R3): its emission stratum is lifted above both the
// negated table's and the pred table's readiness, and the negate table's
// drain stratum is lifted to it, so the crossover is emitted in the seed
// block BEFORE the negate table's claim drains.
struct CrossoverEmission {
  QueryNegate negate;
  TABLE *negate_table;    // The negate view's own (differential) table.
  TABLE *negated_table;   // The negated view's model table (the crossover src).
  TABLE *pred_table;      // The data predecessor's table (scanned).
  QueryView pred_view;    // `QueryView(negate).Predecessors()[0]`.
  bool negated_differential;  // Whether the `+` arm exists.
  unsigned multiplicity;  // Forward fold count to match (>= 1).

  // Starts at the negate view's stratum and is lifted by the scheduling
  // fixpoint above both read tables' readiness.
  unsigned stratum;

  CrossoverEmission(QueryNegate negate_, TABLE *negate_table_,
                    TABLE *negated_table_, TABLE *pred_table_,
                    QueryView pred_view_, bool negated_differential_,
                    unsigned multiplicity_, unsigned stratum_)
      : negate(negate_),
        negate_table(negate_table_),
        negated_table(negated_table_),
        pred_table(pred_table_),
        pred_view(pred_view_),
        negated_differential(negated_differential_),
        multiplicity(multiplicity_),
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
// SCC) is `kNonRecursive`. Both LINEAR and NONLINEAR recursive SCCs classify
// identically: the claim-round loop (with the k-position JOIN fire, see
// `EmitJoinFire`) balances a `kRecursive` fold in either shape.
static DerivClass RuleClass(const RecursiveSccMap &sccs, TABLE *target,
                            const std::vector<TABLE *> &read_tables) {
  const auto target_scc = RecursiveSCC(sccs, target);
  if (!target_scc.has_value()) {
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
// a MAP, and the position-keyed forward gate for a NEGATE. Returns the
// region under which the rest of the chain nests.
//
// `in_fixpoint` selects the negate gate's read predicate by EMISSION CONTEXT,
// not by sign (the F-A consistency theorem, analysis §2, R2): a chain emitted
// as a SEED (`in_fixpoint == false`) reads the negated table batch-frozen
// (`kInI`) for BOTH signs — the same fixed read the crossover's exactly-once
// accounting is proved against (§5.1 seed table 1, oracle SeedReads:1312-1323,
// position-keyed and sign-independent); a chain RE-FIRED inside a recursive
// SCC's claim-round loop (`in_fixpoint == true`) reads `kInNew` for BOTH signs
// (§5.1 fixpoint table 2, oracle FixReads:1334-1335). The old sign-keyed read
// (`is_add ? kInNew : kInI`) is incompatible with the crossover: it lost/
// double-counted the mixed same-batch pred×negated cases (analysis §2).
static OP *EmitChainStep(ProgramImpl *impl, Context &context,
                         QueryView pred_view, QueryView view, bool is_add,
                         bool in_fixpoint, OP *parent) {

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

  // The forward pass of negation maintenance, position-keyed per emission
  // context (R2): a SEED-context walk (`!in_fixpoint`) reads the negated
  // table batch-frozen (`kInI`) for BOTH signs; a fixpoint-refire walk
  // (`in_fixpoint`, the recursive-SCC claim-round re-fire) reads the
  // batch-final-so-far state (`kInNew`) for BOTH signs. The negated table is
  // phase-final here: its stratum is strictly lower than the chain's emission
  // stratum (seed context), or it is a same-SCC lower-position read the
  // claim-round loop keeps final (fixpoint context).
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
        in_fixpoint ? MembershipPredicate::kInNew : MembershipPredicate::kInI,
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
    OP *const step =
        EmitChainStep(impl, context, view, succ, is_add, false, let);
    EmitSectionWalk(impl, context, succ, is_add, deriv_class, step);
  }
}

// Emit one seed: a loop over the source table's frontier vector `vec`
// binding the branch's member view's columns, followed by the chain's
// plumbing, ending at a head fold or at a pivot append into the join's
// delta pivot vector (`join_pivot_vec`, null for head chains).
// `in_fixpoint` marks a seed emitted INSIDE a recursive-SCC claim-round loop
// (the same-SCC internal re-fire): it selects the fixpoint-context negate
// gate (`kInNew`) for any NEGATE on the chain (R2). A base/lower-stratum seed
// passes `false`, reading batch-frozen `kInI`.
static void EmitSeedLoop(ProgramImpl *impl, Context &context,
                         const BranchChain &branch, bool is_add,
                         DerivClass deriv_class, VECTOR *vec,
                         VECTOR *join_pivot_vec, bool in_fixpoint, SERIES *seq) {
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

    parent =
        EmitChainStep(impl, context, pred_view, view, is_add, in_fixpoint,
                      parent);
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

// Emit one arm of a negation crossover (D1', §5.4). `is_add` picks the sign:
// the `+` arm folds a `+recursive`/`+nonrecursive` count when the negated key
// is LOST (over the negated table's net-removals frontier), the `-` arm folds
// the dual when the negated key is GAINED (over net-additions). Shape:
//
//   vector-unique  <negated frontier>
//   vector-loop {negated cols} over <negated frontier>
//     scan pred where pred.key = negated key  ->  {rest of pred row}
//       check-member in-new {pred row} in pred_table
//         if-member
//           update-count ±class {negate outputs} in negate_table
//             if-crossed -> append into negate_table add/delete queue
//
// The negated frontier row binds the negated view's columns; those bind BOTH
// the negate's key output columns (`NegatedColumns()`, index-aligned to the
// negated view) AND the pred's key input columns (`InputColumns()`, the pred
// contribution to the same key), which are the scan's available columns. Zero
// key columns (a unit-relation negation) degenerates to a full pred scan.
static void EmitCrossover(ProgramImpl *impl, Context &context,
                          const RecursiveSccMap &sccs,
                          const CrossoverEmission &x, bool is_add,
                          SERIES *seq) {
  const QueryNegate negate = x.negate;
  const QueryView negated_view = negate.NegatedView();
  const unsigned num_key = negate.NumInputColumns();

  // Sort-unique the consumed frontier (mirrors `seed_vector`): a monotone
  // boundary's net-additions can be appended at several same-model fold sites
  // (R7), and each frontier row must drive the crossover once.
  VECTOR *const frontier = TableDeltaVector(
      impl, context, x.negated_table,
      is_add ? VectorKind::kNetRemovals : VectorKind::kNetAdditions);
  VECTORUNIQUE *const unique =
      impl->operation_regions.CreateDerived<VECTORUNIQUE>(
          seq, ProgramOperation::kSortAndUniqueInductionVector);
  unique->vector.Emplace(unique, frontier);
  seq->AddRegion(unique);

  // Loop over the negated frontier, binding the negated view's columns.
  VECTORLOOP *const loop = impl->operation_regions.CreateDerived<VECTORLOOP>(
      impl->next_id++, seq, ProgramOperation::kLoopOverInductionVector);
  seq->AddRegion(loop);
  loop->vector.Emplace(loop, frontier);
  for (auto col : negated_view.Columns()) {
    VAR *const var = loop->defined_vars.Create(impl->next_id++,
                                               VariableRole::kVectorVariable);
    var->query_column = col;
    loop->col_id_to_var.emplace(col.Id(), var);
  }

  // Bind the negate's key output columns and the pred's key input columns from
  // the negated view's columns (the shared key). `NegatedColumns()[i]` is the
  // negate output at the negated view's column `i`; `NthInputColumn(i)` is the
  // pred column contributing that same key position. This is the REVERSE of
  // `BuildEagerNegateRegion`'s / `EmitChainStep`'s key mapping: there the
  // negate output defines the negated view column; here the negated view
  // column (from the frontier) defines both the negate output and the pred
  // input.
  std::vector<QueryColumn> avail;
  {
    auto i = 0u;
    for (QueryColumn out_col : negate.NegatedColumns()) {
      const auto j = *(out_col.Index());
      const auto neg_col = negated_view.NthColumn(j);
      VAR *const key_var = loop->VariableFor(impl, neg_col);
      loop->col_id_to_var[out_col.Id()] = key_var;

      // The pred column contributing this key position. A constant key column
      // (a condition / all-constant-key negate, negate_3) does not constrain
      // the pred scan — it is already fixed — so it is bound but NOT an
      // available scan column; a zero-real-key negate degenerates to a full
      // pred scan (the unit-relation case, cond_*).
      const QueryColumn pred_key_col = negate.NthInputColumn(i++);
      if (!pred_key_col.IsConstant()) {
        loop->col_id_to_var[pred_key_col.Id()] = key_var;
        avail.push_back(pred_key_col);
      }
    }
    assert(i == num_key);
    (void) num_key;
  }

  // The fold's rule class: recursive iff the negate's table shares a recursive
  // SCC with a read table. The negated table contributes nothing today
  // (Stratify forbids a same-SCC negated view — a negated predicate cannot be
  // recursively derived from the negation's own result), passed belt-and-
  // braces per R5; a same-SCC PRED (a recursive negate, the D5 shape) makes
  // the crossover fold `kRecursive`.
  const DerivClass deriv_class =
      RuleClass(sccs, x.negate_table, {x.pred_table, x.negated_table});

  SERIES *const body_seq = impl->series_regions.Create(loop);
  loop->body.Emplace(loop, body_seq);

  std::vector<QueryColumn> negate_cols;
  for (auto col : negate.Columns()) {
    negate_cols.push_back(col);
  }

  // The fold + if-crossed queue append into the negate's own table (R6),
  // nested under `parent` (the pred-membership gate).
  const auto emit_fold = [&](REGION *parent) -> REGION * {
    UPDATECOUNT *const fold = BuildUpdateCount(
        impl, x.negate_table, parent, negate_cols, is_add, deriv_class);

    VECTOR *const queue = TableDeltaVector(
        impl, context, x.negate_table,
        is_add ? VectorKind::kAddQueue : VectorKind::kDeleteQueue);
    VECTORAPPEND *const append =
        impl->operation_regions.CreateDerived<VECTORAPPEND>(
            fold, ProgramOperation::kAppendToInductionVector);
    append->vector.Emplace(append, queue);
    for (auto col : negate_cols) {
      append->tuple_vars.AddUse(fold->VariableFor(impl, col));
    }
    fold->body.Emplace(fold, append);
    return fold;
  };

  // Scan the pred table by the bound key (the §3.4 index request; zero bound
  // columns — a unit relation — degenerates to a full scan), then read the
  // scanned pred row at `kInNew` (R4: the readiness lift makes InNew
  // batch-final for a lower pred; for a same-SCC pred the seed-time flags are
  // untouched so InNew ≡ InI extensionally). The pred row's non-key columns
  // bind the negate's copied output columns.
  BuildMaybeScanPartial(
      impl, x.pred_view, avail, x.pred_table, body_seq,
      [&](REGION *in_scan, bool) -> REGION * {
        std::vector<QueryColumn> pred_cols;
        for (auto col : x.pred_view.Columns()) {
          pred_cols.push_back(col);
        }
        return BuildCheckMember(
            impl, in_scan, x.pred_table, pred_cols, MembershipPredicate::kInNew,
            [&](ProgramImpl *impl_, REGION *in_check) -> REGION * {

              // Bind the negate's copied (attached) output columns from the
              // scanned pred row: `CopiedColumns()` are the negate outputs for
              // the attached pred columns (`InputCopiedColumns()`), in order.
              // The key outputs are already bound on the loop.
              auto attached_it = negate.InputCopiedColumns().begin();
              for (QueryColumn out_col : negate.CopiedColumns()) {
                const QueryColumn attached = *attached_it++;
                in_check->col_id_to_var[out_col.Id()] =
                    in_check->VariableFor(impl_, attached);
              }
              return emit_fold(in_check);
            },
            [](ProgramImpl *, REGION *) -> REGION * { return nullptr; });
      });
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

    // Inside a claim-round loop, DRAIN the queue destructively: clear it once
    // the round's rows have been claimed into the accumulated set and the
    // per-round frontier `Δ`. The fixpoint fire that runs later in the same
    // round re-appends only the rows whose folds cross zero this round, so the
    // NEXT round's drain sees just that new work — semi-naive draining. Without
    // the clear the queue accumulates every crossed row for the whole batch and
    // each round re-sort-uniques and re-loops the full accumulation, an O(N^2)
    // blow-up on a length-N recursion (e.g. `deep_chain_retract` at N=100000).
    // Claim dedup (table state) already guarantees each row is claimed once, so
    // dropping already-drained rows from the queue changes nothing claimed:
    // a diamond re-enqueue of an already-claimed row is re-drained in a later
    // round but claim skips it, exactly as when the whole queue was re-scanned.
    VECTORCLEAR *const drain_clear =
        impl->operation_regions.CreateDerived<VECTORCLEAR>(
            seq, ProgramOperation::kClearInductionVector);
    drain_clear->vector.Emplace(drain_clear, queue);
    seq->AddRegion(drain_clear);
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

// Emit the claim-round fixpoint fire of one recursive JOIN over the per-round
// claimed frontier `Δ` of a same-SCC side (§5.2 OVERDELETE for `is_del`, §5.3
// INSERT mirror). A JOIN with `k` same-SCC sides (`k >= 1`) emits `k` separate
// loop-scan-fold units, one per same-SCC position `p` (in raw `JoinedViews()`
// order — the deterministic total order the strict/permissive asymmetry is
// keyed on; NOT `SortedPredecessors`, whose equivalence-set tiebreak is not
// mode-stable). Position `p` drives the delta; every OTHER same-SCC side `j` is
// read with the position-relative FLAG-F/H predicate keyed on the STATIC join
// position of `j` relative to `p`, and every lower (non-SCC) side at `kInNew`.
//
// LINEAR `t(F,T) : t(F,Y), edge(Y,T)` (k=1): one emission, no other same-SCC
// side; the lower side `edge` reads `kInNew` (MD §5.1 lower `j < i` reads
// `InNew`):
//
//   vector-loop {F,Y} over Δ(t)            « the newly claimed t rows »
//     scan edge where edge.Y = Y  →  {edge.T}
//       check-member InNew {Y,T} in edge
//         if-member: update-count ∓recursive t(F,T)   « the join's persisted »
//           if-crossed: append into the join table's sign queue
//
// NONLINEAR `p(F,T) : p(F,Y), p(Y,T)` (k=2, both sides table `p`): TWO
// emissions, `p={0,1}`. For `p=0` the delta binds side 0's columns and side 1
// (`j=1 > p`) reads the STRICT later predicate (`AliveAtClaim`/
// `InNewSansFrontier`); for `p=1` side 0 (`j=0 < p`) reads the PERMISSIVE
// earlier predicate (`SurvivesSoFar`/`InNewWithFrontier`). The asymmetry keyed
// on `kDelNow`/`kAddNow` delivers same-round exactly-once STRUCTURALLY: with
// both sides in `Δ` this round, the earlier-as-delta fires (its permissive read
// of the later position passes) and the later-as-delta does not (its strict
// read of the earlier position fails on the frontier bit). One emission with
// per-side dynamic dispatch would double-fire that same-round double-claim.
//
// The fold lands on the JOIN's own persisted table (the recursion anchor),
// whose net-removal frontier the next round's projection seeds carry onward.
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

  // The joined sides in raw (deterministic, mode-stable) `JoinedViews()` order,
  // partitioned into same-SCC positions and lower (InNew-read) sides. A
  // self-join yields two DISTINCT views over one table at two positions — the
  // partition is POSITIONAL, never table-keyed.
  std::vector<QueryView> all_sides;
  std::vector<size_t> same_pos;  // Indices into `all_sides`.
  std::vector<QueryView> lower_sides;
  for (QueryView side : join_view.JoinedViews()) {
    TABLE *const side_table =
        impl->view_to_model[side]->FindAs<DataModel>()->table;
    if (RecursiveSCC(sccs, side_table) == join_scc) {
      same_pos.push_back(all_sides.size());
    } else {
      lower_sides.push_back(side);
    }
    all_sides.push_back(side);
  }
  assert(!same_pos.empty());

  // The columns folded into the join's persisted table (its output columns).
  std::vector<QueryColumn> join_cols;
  for (auto col : emission.join_view.Columns()) {
    join_cols.push_back(col);
  }

  // Emit one full loop-scan-fold unit for same-SCC position `all_sides[p_pos]`
  // driving the delta. Other same-SCC sides read the position-relative matrix
  // predicate; lower sides read `kInNew`. `k=1` (single same-SCC position, no
  // other same-SCC side) reproduces the linear emission exactly.
  const auto emit_for_position = [&](size_t p_pos) {
    const QueryView delta_side = all_sides[p_pos];
    TABLE *const delta_table =
        impl->view_to_model[delta_side]->FindAs<DataModel>()->table;
    VECTOR *const round_frontier = TableDeltaVector(
        impl, context, delta_table,
        is_del ? VectorKind::kClaimedDeleteFrontier
               : VectorKind::kClaimedAddFrontier);

    // Loop over the claimed rows of the delta side, binding that side's columns.
    VECTORLOOP *const loop = impl->operation_regions.CreateDerived<VECTORLOOP>(
        impl->next_id++, seq, ProgramOperation::kLoopOverInductionVector);
    seq->AddRegion(loop);
    loop->vector.Emplace(loop, round_frontier);

    for (auto col : delta_side.Columns()) {
      VAR *const var = loop->defined_vars.Create(
          impl->next_id++, VariableRole::kVectorVariable);
      var->query_column = col;
      loop->col_id_to_var.emplace(col.Id(), var);
    }

    // Bind the join's pivot output variables from the delta side's columns: the
    // pivot value equals the delta side's pivot input column. This makes the
    // pivot available to the nested scans below.
    for (auto j = 0u, num_pivots = join_view.NumPivotColumns(); j < num_pivots;
         ++j) {
      const QueryColumn out_pivot = join_view.NthOutputPivotColumn(j);
      for (QueryColumn in_pivot : join_view.NthInputPivotSet(j)) {
        if (QueryView::Containing(in_pivot) == delta_side) {
          loop->col_id_to_var.emplace(out_pivot.Id(),
                                      loop->VariableFor(impl, in_pivot));
          break;
        }
      }
    }

    // Bind the join's non-pivot outputs contributed by the delta side directly
    // (the other sides' non-pivot outputs are bound by their scans).
    join_view.ForEachUse([&](QueryColumn in_col, InputColumnRole role,
                             std::optional<QueryColumn> out_col) {
      if (!out_col || role != InputColumnRole::kJoinNonPivot) {
        return;
      }
      if (QueryView::Containing(in_col) == delta_side) {
        loop->col_id_to_var.emplace(out_col->Id(),
                                    loop->VariableFor(impl, in_col));
      }
    });

    // The ordered scan list for this emission: every OTHER same-SCC side (in
    // position order) with its position-relative matrix predicate, then every
    // lower side at `kInNew`. For k=1 the same-SCC part is empty, so this is
    // exactly `lower_sides` — the linear emission, byte-for-byte.
    std::vector<std::pair<QueryView, MembershipPredicate>> scan_list;
    for (size_t j_pos : same_pos) {
      if (j_pos == p_pos) {
        continue;  // The delta side itself is the loop, not a scan.
      }
      MembershipPredicate pred;
      if (j_pos < p_pos) {
        pred = is_del ? MembershipPredicate::kSurvivesSoFar
                      : MembershipPredicate::kInNewWithFrontier;  // permissive
      } else {
        pred = is_del ? MembershipPredicate::kAliveAtClaim
                      : MembershipPredicate::kInNewSansFrontier;  // strict
      }
      scan_list.emplace_back(all_sides[j_pos], pred);
    }
    for (QueryView side : lower_sides) {
      scan_list.emplace_back(side, MembershipPredicate::kInNew);
    }

    // Fold ∓recursive into the join's persisted table at the deepest scan
    // level.
    const auto emit_fold = [&](SERIES *inner_seq) {
      // Bind the join's non-pivot output columns contributed by the scanned
      // sides to the sides' scanned column variables (the scans bind the SIDE's
      // columns, whose ids differ from the join outputs'; the delta side's
      // outputs and all pivots are already bound on the outer loop). Resolve
      // through `inner_seq`, which nests under the sides' scans.
      join_view.ForEachUse([&](QueryColumn in_col, InputColumnRole role,
                               std::optional<QueryColumn> out_col) {
        if (!out_col || role != InputColumnRole::kJoinNonPivot) {
          return;
        }
        if (QueryView::Containing(in_col) != delta_side) {
          inner_seq->col_id_to_var.emplace(
              out_col->Id(), inner_seq->VariableFor(impl, in_col));
        }
      });

      UPDATECOUNT *const fold = BuildUpdateCount(
          impl, join_table, inner_seq, join_cols, !is_del /* is_add */,
          DerivClass::kRecursive);
      inner_seq->AddRegion(fold);

      VECTOR *const queue = TableDeltaVector(
          impl, context, join_table,
          is_del ? VectorKind::kDeleteQueue : VectorKind::kAddQueue);
      VECTORAPPEND *const append =
          impl->operation_regions.CreateDerived<VECTORAPPEND>(
              fold, ProgramOperation::kAppendToInductionVector);
      append->vector.Emplace(append, queue);
      for (auto col : join_cols) {
        append->tuple_vars.AddUse(fold->VariableFor(impl, col));
      }
      fold->body.Emplace(fold, append);
    };

    // Recursively scan each side in `scan_list` under `parent_seq`, reading it
    // at its assigned predicate, then fold at the deepest level. Each level
    // attaches its scan and gate under the SERIES it is handed; the gate's
    // member body is a fresh SERIES the next level attaches to.
    std::function<void(size_t, SERIES *)> scan_next =
        [&](size_t idx, SERIES *parent_seq) {
          if (idx == scan_list.size()) {
            emit_fold(parent_seq);
            return;
          }
          const QueryView side = scan_list[idx].first;
          const MembershipPredicate pred = scan_list[idx].second;
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
                    parent_seq->VariableFor(
                        impl, join_view.NthOutputPivotColumn(j)));
                avail.push_back(in_pivot);
              }
            }
          }

          BuildMaybeScanPartial(
              impl, side, avail, side_table, parent_seq,
              [&](REGION *in_scan, bool) -> REGION * {
                std::vector<QueryColumn> side_cols;
                for (auto col : side.Columns()) {
                  side_cols.push_back(col);
                }
                return BuildCheckMember(
                    impl, in_scan, side_table, side_cols, pred,
                    [&](ProgramImpl *impl_, REGION *in_check) -> REGION * {
                      SERIES *const next_seq =
                          impl_->series_regions.Create(in_check);
                      scan_next(idx + 1u, next_seq);
                      return next_seq;
                    },
                    [](ProgramImpl *, REGION *) -> REGION * { return nullptr; });
              });
        };

    SERIES *const body_seq = impl->series_regions.Create(loop);
    loop->body.Emplace(loop, body_seq);
    scan_next(0u, body_seq);
  };

  for (size_t p_pos : same_pos) {
    emit_for_position(p_pos);
  }
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

  // The recursive-SCC membership of every table (empty for programs with no
  // stratum-phase-owned recursion). Drives the rule-class of folds into
  // recursive tables and the same-SCC read exemption in the readiness assert.
  RecursiveSccMap recursive_sccs = ComputeRecursiveSCCs(impl, context);

  // Whether a table is in a stratum-phase-owned recursive SCC — the shape whose
  // folds class `kRecursive` and whose claim-round OVERDELETE/REDERIVE/INSERT
  // loop is emitted. Both LINEAR (exactly one same-SCC join side) and NONLINEAR
  // (`tc(F,T):tc(F,X),tc(X,T)`, ≥2 same-SCC join sides) recursions answer true:
  // the claim-round fire's k-position JOIN emission (`EmitJoinFire`) handles
  // both, folding the round-relative predicate matrix
  // (`SurvivesSoFar`/`AliveAtClaim`/`InNewWithFrontier`/`InNewSansFrontier`)
  // over each other same-SCC side keyed on static join position. Nonlinear SCCs
  // no longer fall back to the pre-A2 single-pass path.
  const auto is_recursive = [&](TABLE *table) -> bool {
    return RecursiveSCC(recursive_sccs, table).has_value();
  };

  // Whether EVERY joined side of `join_view` is in the JOIN's own recursive
  // SCC (e.g. `p(F,T):p(F,Y),p(Y,T)` — both sides read `p`, the join's own
  // relation). Such a join has NO lower body atom, hence NO seed (MD §5.1: a
  // recursive rule with no lower atom has no init-position firing). Its seed
  // machinery is dead-by-construction: the pivot-append seed loops range over
  // the join's OWN SCC frontier vectors, which are structurally EMPTY at seed
  // time (they are filled only in `add_output`, after the claim-round loops);
  // and its dual-section join-tables region folds over those same empty
  // frontiers. The `join-tables` region and pivot appends are therefore
  // suppressed — the round-0 firing is instead carried by the claim-round
  // loop's own `EmitJoinFire` (fed by the SCC tables' seed queues). The
  // JoinEmission record is KEPT in `joins` (scc_joins, the fixpoint fire, is
  // derived from it). A MIXED join (>=1 lower + >=1 same-SCC side) keeps its
  // dual sections: the InNew-at-seed-time == InI equivalence makes them
  // correct for its same-SCC sides.
  const auto all_sides_same_scc = [&](QueryView join_view) -> bool {
    DataModel *const jm =
        impl->view_to_model[join_view]->FindAs<DataModel>();
    const auto join_scc = RecursiveSCC(recursive_sccs, jm->table);
    if (!join_scc.has_value()) {
      return false;
    }
    for (QueryView side : QueryJoin::From(join_view).JoinedViews()) {
      TABLE *const st = impl->view_to_model[side]->FindAs<DataModel>()->table;
      if (RecursiveSCC(recursive_sccs, st) != join_scc) {
        return false;
      }
    }
    return true;
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

  // The negation crossovers (D1'/§5.4), one per non-@never negate. Discovered
  // here from `query.Negations()` (the audit confirmed `Predecessors()[0]` is
  // the DATA pred, and `FillDataModel` materializes the pred and negated
  // tables for every negate). A @never negate never retracts, so it has no
  // crossover (R1). Each crossover's emission stratum is registered with the
  // scheduling fixpoint below.
  //
  // Discovered per non-@never `QueryNegate` (from `query.Negations()`) — but
  // with the crossover's fold MULTIPLICITY matched to the forward pass's, so a
  // row derived several ways retracts exactly. `Predecessors()[0]` is the data
  // pred (audit-confirmed); `FillDataModel` materializes the pred and negated
  // tables. A @never negate never retracts, so it gets no crossover (R1).
  //
  // Forward multiplicity: a negate over a MONOTONE negated view is reached by
  // the EAGER walk (`BuildEagerNegateRegion`, once per eager predecessor
  // arrival), so its forward fold count is the negate's predecessor count. A
  // negate over a DIFFERENTIAL negated view is cut from the eager walk
  // (Build.cpp) and reached by STRATUM branch chains (`EmitSeedLoop` folds
  // once per chain into the negate's table); a union-shared negate (merge_5)
  // is reached by several chains, so its forward fold count is the number of
  // such branches. Either way the crossover emits that many arm-pairs.
  std::vector<CrossoverEmission> crossovers;
  for (QueryNegate negate : query.Negations()) {
    if (negate.HasNeverHint()) {
      continue;
    }
    const QueryView negate_view(negate);
    const QueryView negated_view = negate.NegatedView();
    const QueryView pred_view = negate_view.Predecessors()[0];

    TABLE *const negate_table =
        impl->view_to_model[negate_view]->FindAs<DataModel>()->table;
    TABLE *const negated_table =
        impl->view_to_model[negated_view]->FindAs<DataModel>()->table;
    TABLE *const pred_table =
        impl->view_to_model[pred_view]->FindAs<DataModel>()->table;
    assert(negate_table != nullptr);
    assert(negated_table != nullptr);
    assert(pred_table != nullptr);

    // Count the stratum branch chains that fold into this negate's table
    // (target == negate_table, terminal == this negate). Zero for an
    // eager-reached (monotone-negated) negate — then the forward count is the
    // number of eager predecessor arrivals (the negate's predecessors other
    // than the negated view).
    unsigned multiplicity = 0u;
    for (const BranchChain &branch : branches) {
      if (!branch.ends_at_join && branch.target == negate_table &&
          !branch.path.empty() && branch.path.back() == negate_view) {
        ++multiplicity;
      }
    }
    if (multiplicity == 0u) {
      for (QueryView p : negate_view.Predecessors()) {
        if (p != negated_view) {
          ++multiplicity;
        }
      }
    }
    assert(multiplicity != 0u);

    crossovers.emplace_back(negate, negate_table, negated_table, pred_table,
                            pred_view, TableIsDifferential(negated_table),
                            multiplicity, negate_view.Stratum().value_or(0u));
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
  if (branches.empty() && joins.empty() && phase_table_order.empty() &&
      crossovers.empty()) {
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

    // The negation crossovers (R3): lift each above both read tables'
    // readiness (the negated frontier it loops and the pred table it scans at
    // `kInNew`), then lift the negate table's drain stratum to it, so the
    // crossover's fold into the negate table lands in the seed block strictly
    // before that table's claim drains. Same-SCC reads are exempt
    // (`ready_across`): a recursive negate's same-SCC pred read is a lower
    // `kInNew` the claim-round loop keeps final, closing no scheduling cycle.
    for (CrossoverEmission &x : crossovers) {
      unsigned stratum = x.stratum;
      stratum = std::max(stratum, ready_across(x.negate_table, x.negated_table));
      stratum = std::max(stratum, ready_across(x.negate_table, x.pred_table));
      lift(x.stratum, stratum, changed);
      lift(drain_stratum[x.negate_table], x.stratum, changed);
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
  for (const CrossoverEmission &x : crossovers) {
    assert(ready_across(x.negate_table, x.negated_table) <= x.stratum);
    assert(ready_across(x.negate_table, x.pred_table) <= x.stratum);
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
  for (const CrossoverEmission &x : crossovers) {
    strata.insert(x.stratum);
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
      // union edges of a linear or nonlinear recursion) is NOT seeded here: it fires
      // exclusively inside the SCC's claim-round loop, delta over the
      // predecessor's per-round `Δ_D`. Seeding it here too (over the source's
      // accumulated net-removal frontier) would double-decrement each internal
      // derivation once per round. The recursive-rule SEED that round 0 owns is
      // the JOIN over the SCC's LOWER predecessor frontier (handled by the join
      // emission below); the base rule and any non-SCC source stay here.
      if (!branch.ends_at_join && is_recursive(branch.target) &&
          same_scc(branch.target, branch.source)) {
        continue;
      }

      // An all-same-SCC join has no seed (see `all_sides_same_scc`): its
      // pivot-append seed loops range over the join's own SCC frontier vectors,
      // empty at seed time. Suppress them; the claim-round loop's `EmitJoinFire`
      // carries the round-0 firing.
      if (branch.ends_at_join && all_sides_same_scc(branch.path.back())) {
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
        branch_class = RuleClass(recursive_sccs, branch.target, {source});
      }

      if (TableIsDifferential(source) &&
          !TableIsInductionOwned(context, source)) {
        EmitSeedLoop(impl, context, branch, false /* is_add */, branch_class,
                     seed_vector(source, VectorKind::kNetRemovals),
                     join_pivot_vec, false /* in_fixpoint */, stratum_seq);
      }
      EmitSeedLoop(impl, context, branch, true /* is_add */, branch_class,
                   seed_vector(source, VectorKind::kNetAdditions),
                   join_pivot_vec, false /* in_fixpoint */, stratum_seq);
    }

    // Dual-section joins: the sort-uniqued pivot vector makes the join
    // enumerate each combination once per batch, so each section fires
    // exactly once per started/stopped rule instance.
    for (const JoinEmission &emission : joins) {
      if (emission.stratum != stratum) {
        continue;
      }

      // An all-same-SCC join has no seed: its dual-section walk folds over the
      // join's own SCC frontier vectors, empty at seed time (see
      // `all_sides_same_scc`). Suppress the seed join-tables region entirely;
      // the JoinEmission is still recorded, so its fixpoint fire runs each
      // round inside the claim-round loop (`EmitJoinFire`).
      if (all_sides_same_scc(emission.join_view)) {
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
          RuleClass(recursive_sccs, join_model->table, side_tables);

      LET *const added = impl->operation_regions.CreateDerived<LET>(join);
      join->added_body.Emplace(join, added);
      EmitSectionWalk(impl, context, emission.join_view, true /* is_add */,
                      join_class, added);

      LET *const removed = impl->operation_regions.CreateDerived<LET>(join);
      join->removed_body.Emplace(join, removed);
      EmitSectionWalk(impl, context, emission.join_view, false /* is_add */,
                      join_class, removed);
    }

    // Negation crossovers (D1'/§5.4): emitted in the SEED BLOCK, BEFORE the
    // claim drains below (R3 — a crossover fold after claim-add would let a
    // phantom `+` be claimed and, on a non-terminal negate, leak a NetAdded
    // frontier entry downstream). The `-` arm (negated key gained) fires
    // always over the negated table's net-additions frontier; the `+` arm
    // (negated key lost) only when the negated table is differential.
    for (const CrossoverEmission &x : crossovers) {
      if (x.stratum != stratum) {
        continue;
      }

      // The fold target (the negate's own table) drains at or after this
      // stratum — the scheduling fixpoint lifted it to `x.stratum` (mirrors
      // the seed source-drain assert).
      assert(drain_stratum.count(x.negate_table) &&
             drain_stratum.find(x.negate_table)->second >= stratum);

      // Emit `multiplicity` arm-pairs, matching the forward pass's fold count
      // into the negate table so a multiply-derived row retracts exactly.
      for (unsigned m = 0u; m < x.multiplicity; ++m) {
        EmitCrossover(impl, context, recursive_sccs, x, false /* is_add */,
                      stratum_seq);
        if (x.negated_differential) {
          EmitCrossover(impl, context, recursive_sccs, x, true /* is_add */,
                        stratum_seq);
        }
      }
    }

    // Partition this stratum's phase tables into single-pass (genuinely
    // acyclic) tables and recursive-SCC tables (whose OVERDELETE/INSERT is a
    // claim-round fixpoint per §5.2/§5.3 — see below). A single-pass table's
    // every fold is `kNonRecursive`, so its recursive counter is identically
    // zero and one drain settles it; the scheduling fixpoint lifted its
    // emissions above every table it reads. Both linear and nonlinear recursive
    // SCCs are `scc_tables` — the k-position JOIN fire handles either shape.
    std::vector<TABLE *> acyclic_tables;
    std::vector<TABLE *> scc_tables;
    for (TABLE *table : phase_table_order) {
      if (drain_stratum[table] != stratum) {
        continue;
      }
      if (is_recursive(table)) {
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
                       nullptr, true /* in_fixpoint */, round_seq);
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
    // output region, after the fixpoint quiesces — run REDERIVE (every
    // overdeleted row still recursively supported, `C_r > 0`, re-enters via the
    // add queue; a counter read over `D_s`). REDERIVE's output feeds the INSERT
    // add queue below, so it must run between OVERDELETE and INSERT.
    //
    // Note that the del-side net-removal frontier (`outDel = D_s ∧ !kAdd`) is
    // deliberately NOT built here (contrast the add side below). Spec §5.0 puts
    // BUILDFRONTIERS after INSERT: a row overdeleted this batch and then
    // re-added by INSERT (its `kAdd` set) must NOT appear in `net_removals`.
    // Building the del frontier in this output region — before INSERT can set
    // `kAdd` — would leak such a re-added row into `net_removals`. Both signed
    // frontiers are therefore consolidated in the INSERT loop's output region
    // (`add_output`), after INSERT has quiesced.
    INDUCTION *const del_loop = build_claim_round_loop(true, stratum_seq);
    SERIES *const del_output = impl->series_regions.Create(del_loop);
    del_loop->output_region.Emplace(del_loop, del_output);
    for (TABLE *table : scc_tables) {
      EmitRederive(impl, context, table, del_output);
    }

    // INSERT (§5.3): the add-side claim-round fixpoint (mirror of OVERDELETE),
    // draining the add queue — the batch's `+` seeds AND REDERIVE's output —
    // through the same claim/fire/retire discipline so that recursive
    // derivations are ADDED (their `C_r` incremented) symmetrically with the
    // del side.
    INDUCTION *const add_loop = build_claim_round_loop(false, stratum_seq);
    SERIES *const add_output = impl->series_regions.Create(add_loop);
    add_loop->output_region.Emplace(add_loop, add_output);

    // BUILDFRONTIERS (§5.0): both consolidated signed net frontiers are built
    // HERE, after INSERT has quiesced, so that `kAdd` (set by INSERT on a
    // re-added row) and `kDel` (set by OVERDELETE) are both final. `outDel =
    // D_s ∧ !kAdd` (net removals) and `outAdd = A_s ∧ !kDel` (net additions);
    // a row both overdeleted and re-added this batch passes neither filter.
    // These are the consolidated frontiers higher strata's seed joins range
    // over.
    for (TABLE *table : scc_tables) {
      EmitFrontierFilter(impl, context, table, true /* is_del */, add_output);
    }
    for (TABLE *table : scc_tables) {
      EmitFrontierFilter(impl, context, table, false /* is_del */, add_output);
    }
  }
}

}  // namespace hyde
