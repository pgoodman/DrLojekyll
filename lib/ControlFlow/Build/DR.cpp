// Copyright 2020, Trail of Bits. All rights reserved.
//
// The delta-relational IR (DR-IR) construction + validation — R1a inventory.
//
// `BuildDRInventory` derives the crossover + product-arm op families
// INDEPENDENTLY from the `Query` (per spec §6/§7.3: never by copying the old
// discovery structs), reusing the discovery's already-computed recursive-SCC
// map only for the RuleClass derivation (so the class matches bit-for-bit).
// `ValidateDRInventory` is the always-on V-OLD-EQUIV isomorphism against the
// old discovery's shared state plus the B-3 family validators; a mismatch is a
// silent-breakage bug, so it `fprintf`s + `abort()`s (surviving NDEBUG per
// ledger §6.1 B-3).

#include "DR.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>
#include <unordered_set>

#include "Build.h"

namespace hyde {
namespace {

// The always-on validator abort. Mirrors the C++ backend's `Unsupported`
// (Database.cpp:40) — the sole always-on abort convention in the repo.
[[noreturn]] void ValidatorFail(const char *what) {
  std::fprintf(stderr, "error: DR-IR inventory validator failed: %s\n", what);
  std::abort();
}

// The recursive-SCC identity of a table over the passed-in map, or nullopt.
// Replicated from `RecursiveSCC` (Stratum.cpp:272) — the map is passed in from
// the discovery's `ComputeRecursiveSCCs`, so this is a pure lookup.
static std::optional<unsigned> SccOf(
    const std::unordered_map<TABLE *, unsigned> &sccs, TABLE *table) {
  if (auto it = sccs.find(table); it != sccs.end()) {
    return it->second;
  }
  return std::nullopt;
}

// Replicate `RuleClass` (Stratum.cpp:291-303) EXACTLY: a rule folding into
// `target` from `read_tables` is `kRecursive` iff `target` is in a recursive
// SCC and at least one read table shares that SCC; else `kNonRecursive`.
static DerivClass RuleClass(const std::unordered_map<TABLE *, unsigned> &sccs,
                            TABLE *target,
                            const std::vector<TABLE *> &read_tables) {
  const auto target_scc = SccOf(sccs, target);
  if (!target_scc.has_value()) {
    return DerivClass::kNonRecursive;
  }
  for (TABLE *read : read_tables) {
    if (SccOf(sccs, read) == target_scc) {
      return DerivClass::kRecursive;
    }
  }
  return DerivClass::kNonRecursive;
}

// ---------------------------------------------------------------------------
// §1.4 BRANCH/JOIN DISCOVERY — replicated EXACTLY from Stratum.cpp but as a
// MEMOIZED WORKLIST (the §1.4 hazard fix). These helpers mirror the `static`
// discovery helpers there (which are anonymous-namespace-local, so replicated,
// per the R1a `RuleClass`/`SccOf` pattern).
// ---------------------------------------------------------------------------

// Replicate `ViewIsInductionOwned` (Stratum.cpp:163).
static bool ViewIsInductionOwnedDR(Context &context, QueryView view) {
  return context.view_to_induction.count(view) != 0u;
}

// Replicate `TableIsInductionOwned` (Stratum.cpp:152).
static bool TableIsInductionOwnedDR(Context &context, TABLE *table) {
  for (const QueryView &view : table->views) {
    if (context.view_to_induction.count(view)) {
      return true;
    }
  }
  return false;
}

// Replicate `FollowsDeltaEdge` (Stratum.cpp:331) EXACTLY.
static bool FollowsDeltaEdgeDR(Context &context, QueryView succ) {
  if (ViewIsInductionOwnedDR(context, succ)) {
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

// Replicate `CollectSectionTargets` (Stratum.cpp:418) EXACTLY.
static void CollectSectionTargetsDR(ProgramImpl *impl, Context &context,
                                    QueryView view,
                                    std::vector<TABLE *> &targets) {
  DataModel *const model = impl->view_to_model[view]->FindAs<DataModel>();
  TABLE *const table = model->table;
  if (table != nullptr) {
    if (!TableIsInductionOwnedDR(context, table)) {
      targets.push_back(table);
    }
    return;
  }
  for (QueryView succ : view.Successors()) {
    if (FollowsDeltaEdgeDR(context, succ)) {
      CollectSectionTargetsDR(impl, context, succ, targets);
    }
  }
}

// One terminal outcome of the walk suffix rooted at a view: the path SUFFIX
// (the sequence of consumer edges from — but NOT including — the memoized view
// onward, ending at the terminal), whether it ends at a pivot-join, and the
// head-chain fold target (null for a join terminal).
struct BranchSuffix {
  std::vector<QueryView> suffix;  // consumer edges after the memoized view
  bool ends_at_join{false};
  TABLE *target{nullptr};
};

// The MEMOIZED branch walk. `Suffixes(view)` is the set of path-suffixes from
// `view` to each reachable terminal, computed ONCE per view (memoized in
// `memo`) so reconvergent table-less plumbing costs O(V+E) rather than the old
// path-copying DFS's exponential re-walk. `source` flavors the terminal rules
// (same-table stop) exactly as the old `DiscoverBranches` (Stratum.cpp:353).
//
// CORRECTNESS (§1.4 / v3-spec OQ): the old DFS records one `BranchChain` per
// DISTINCT PATH (no visited set), so a diamond of plumbing emits path-
// multiplicity. This memoization preserves the per-path MULTISET faithfully:
// the caller CROSS-PRODUCTS its own prefix path with the memoized suffix set
// (one emitted branch per (prefix, suffix) pair). Memoizing SUFFIXES (not
// terminal outcomes) is what keeps each distinct prefix→terminal path a
// distinct emitted branch — a reconvergent view's suffixes are shared storage,
// but every path THROUGH it still materializes as its own branch.
//
// The same-table stop rule (`table == source && !IsInsert`) makes suffixes
// source-dependent, so the memo is PER-SOURCE (the driver mints a fresh
// `BranchMemo` per source table). Within one source the walk is O(V+E).
struct BranchMemo {
  std::unordered_map<QueryView, std::vector<BranchSuffix>> by_view;
};

static const std::vector<BranchSuffix> &SuffixesOf(
    ProgramImpl *impl, Context &context, TABLE *source, QueryView view,
    BranchMemo &memo) {
  if (auto it = memo.by_view.find(view); it != memo.by_view.end()) {
    return it->second;
  }

  // Accumulate the suffixes locally, then memoize at the tail. Non-inductive
  // plumbing is a DAG (the same-table stop + induction gate cut every back
  // edge), so the recursion never re-enters `view` before this returns — no
  // in-progress slot is needed, and the memo hit above short-circuits every
  // reconvergent re-arrival (the O(V+E) property).
  std::vector<BranchSuffix> result;

  for (QueryView succ : view.Successors()) {
    if (!FollowsDeltaEdgeDR(context, succ)) {
      continue;
    }

    DataModel *const model = impl->view_to_model[succ]->FindAs<DataModel>();
    TABLE *const table = model->table;

    if (table == source && !succ.IsInsert()) {
      continue;  // a same-model root's own walk covers its edges (Stratum:364)
    }

    if (succ.IsJoin()) {
      // A pivot-join terminal (0-pivot @product joins are NOT terminals — their
      // propagation is owned by the product arms; Stratum.cpp:380).
      if (0u < QueryJoin::From(succ).NumPivotColumns()) {
        BranchSuffix bs;
        bs.suffix.push_back(succ);
        bs.ends_at_join = true;
        result.push_back(std::move(bs));
      }

    } else if (table != nullptr && table != source) {
      // A table-boundary terminal (dropped if induction-owned; Stratum:397).
      if (!TableIsInductionOwnedDR(context, table)) {
        BranchSuffix bs;
        bs.suffix.push_back(succ);
        bs.ends_at_join = false;
        bs.target = table;
        result.push_back(std::move(bs));
      }

    } else {
      // Table-less plumbing: prepend `succ` onto every suffix of `succ`.
      for (const BranchSuffix &deeper :
           SuffixesOf(impl, context, source, succ, memo)) {
        BranchSuffix bs;
        bs.suffix.reserve(deeper.suffix.size() + 1u);
        bs.suffix.push_back(succ);
        bs.suffix.insert(bs.suffix.end(), deeper.suffix.begin(),
                         deeper.suffix.end());
        bs.ends_at_join = deeper.ends_at_join;
        bs.target = deeper.target;
        result.push_back(std::move(bs));
      }
    }
  }

  auto [it, ok] = memo.by_view.emplace(view, std::move(result));
  (void) ok;
  return it->second;
}

// Build the effect set of a crossover arm (spec §2.1 CROSSOVER). `sign` is -1
// (the `-` arm, over the negated table's NetAdditions) or +1 (the `+` arm,
// over NetRemovals). `klass` is the shared RuleClass over the negate table &
// {pred, negated}. Effects: vector:drain(negated frontier), kInNew pred read
// (seed ctx), counter±(negate table, klass) + kInI crossing read, vector:
// append(negate delQ/addQ). The crossover does NOT read the negated table (its
// delta is already the frontier vec it drains — d5rn).
static std::vector<DREffect> CrossoverArmEffects(DROp &op, int sign,
                                                 DerivClass klass) {
  std::vector<DREffect> fx;

  // vector:drain of the negated side's signed frontier (`-` arm → NetAdditions
  // A\D, `+` arm → NetRemovals D\A — the sign DUAL of the negate's own queue).
  DREffect drain;
  drain.kind = EffKind::kVecDrain;
  drain.value_table = op.negated_table;
  drain.vec_role = (sign < 0) ? VecRole::kNetAddition : VecRole::kNetRemoval;
  fx.push_back(drain);

  // The positive-predecessor scan read at kInNew (seed context).
  DREffect pred_read;
  pred_read.kind = EffKind::kFlagRead;
  pred_read.read_table = op.pred_table;
  pred_read.pred = Pred::kInNew;
  pred_read.ctx = Ctx::kSeed;
  fx.push_back(pred_read);

  // counter±(negate table, klass).
  DREffect counter;
  counter.kind = EffKind::kCounter;
  counter.counter_table = op.negate_table;
  counter.sign = sign;
  counter.klass = klass;
  fx.push_back(counter);

  // The kInI crossing read on the negate's own table.
  DREffect crossing;
  crossing.kind = EffKind::kInIReadFrozen;
  crossing.read_table = op.negate_table;
  crossing.pred = Pred::kInI;
  crossing.ctx = Ctx::kSeed;
  fx.push_back(crossing);

  // vector:append into the negate table's delete queue (`-`) / add queue (`+`).
  DREffect append;
  append.kind = EffKind::kVecAppend;
  append.value_table = op.negate_table;
  append.vec_role = (sign < 0) ? VecRole::kDeleteQueue : VecRole::kAddQueue;
  fx.push_back(append);

  return fx;
}

// Build the effect set of a product arm (spec §2.1 PRODUCT_ARM). `sign` is -1
// (minus arm, side is differential) or +1 (plus arm). Effects: vector:drain
// (this side's signed frontier), the position-keyed other-side reads (already
// on `op.arm_reads`), counter±(product table, kNonRecursive), kInI crossing
// read, vector:append(product delQ/addQ).
static std::vector<DREffect> ProductArmEffects(DROp &op, int sign) {
  std::vector<DREffect> fx;

  DREffect drain;
  drain.kind = EffKind::kVecDrain;
  drain.value_table = op.side_tables[op.side_index];
  drain.vec_role = (sign < 0) ? VecRole::kNetRemoval : VecRole::kNetAddition;
  fx.push_back(drain);

  // Position-keyed sign-INDEPENDENT other-side reads (j<i kInNew, j>i kInI).
  for (const auto &[read_table, pred] : op.arm_reads) {
    DREffect read;
    read.kind = EffKind::kFlagRead;
    read.read_table = read_table;
    read.pred = pred;
    read.ctx = Ctx::kSeed;
    fx.push_back(read);
  }

  DREffect counter;
  counter.kind = EffKind::kCounter;
  counter.counter_table = op.product_table;
  counter.sign = sign;
  counter.klass = DerivClass::kNonRecursive;  // B-3.2 / V-PROD-CLASS
  fx.push_back(counter);

  DREffect crossing;
  crossing.kind = EffKind::kInIReadFrozen;
  crossing.read_table = op.product_table;
  crossing.pred = Pred::kInI;
  crossing.ctx = Ctx::kSeed;
  fx.push_back(crossing);

  DREffect append;
  append.kind = EffKind::kVecAppend;
  append.value_table = op.product_table;
  append.vec_role = (sign < 0) ? VecRole::kDeleteQueue : VecRole::kAddQueue;
  fx.push_back(append);

  return fx;
}

// ===========================================================================
// R1c op-family derivation helpers. Each mirrors the emitter's semantics EXACTLY
// (the cited Emit* function is the reference); the effect sets + plan trees are
// derived, never read back from IR.
// ===========================================================================

// The canonical bound-col SET (B-1) for a view's key columns feeding a scan of
// `scan_view`: the scanned view's column ids that a partial scan binds. R1c
// records the SET the access binds (sorted, deduped); the concrete index
// identity is R2 lowering. For a full scan (zero bound columns) the set is
// empty. `bound` are the QueryColumns the caller has bound going into the scan.
static std::vector<unsigned> CanonBoundCols(
    const std::vector<QueryColumn> &bound) {
  std::vector<unsigned> ids;
  ids.reserve(bound.size());
  for (QueryColumn c : bound) {
    ids.push_back(c.Id());
  }
  std::sort(ids.begin(), ids.end());
  ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
  return ids;
}

// The negate forward-gate predicate DERIVED from (context, hint) — NEVER
// sign-derived (F-5 / V-NEG-CTX). Mirrors EmitChainStep:520-522 (seed→kInI,
// fixpoint→kInNew, both signs) and Negate.cpp's eager gate (eager normal→kInI,
// eager @never→kPresent).
static Pred NegateGatePred(Ctx ctx, NegateHint hint) {
  if (ctx == Ctx::kEager) {
    return (hint == NegateHint::kNever) ? Pred::kPresent : Pred::kInI;
  }
  if (ctx == Ctx::kSeed) {
    return Pred::kInI;  // E-13/F18: seed reads kInI BOTH signs
  }
  return Pred::kInNew;  // fixpoint refire, both signs
}

// NOTE on NEGATE_GATE representation: R1c carries negate forward gates as
// ACCESS-attribute sub-objects — `PlanKind::kGate` nodes on the owning seed/
// chain fold's plan spine (plus the matching flag-read effect on the arm) —
// rather than standalone `kNegateGate` ops. The `kNegateGate` kind exists in
// the vocabulary (a standalone form is expected for the EAGER context when the
// eager walk is inventoried, R1d+); V-NEG-CTX validates BOTH forms.

// The Fold-leaf PlanNode for a head fold into `table` with `sign`/`klass`.
static std::unique_ptr<PlanNode> MakeFoldLeaf(TABLE *table, int sign,
                                              DerivClass klass) {
  auto leaf = std::make_unique<PlanNode>();
  leaf->kind = PlanKind::kFold;
  leaf->fold_table = table;
  leaf->fold_sign = sign;
  leaf->fold_class = klass;
  return leaf;
}

// Build the SEED_FOLD arm (effects + minimal PlanTree) for a head-chain seed:
// vector:drain(source frontier), the chain's negate-gate reads (context=seed),
// counter±(target, klass) + kInI crossing, vector:append(target delQ/addQ).
// The PlanTree is a spine of GATE nodes (per NEGATE on the chain, seed ctx)
// ending at the Fold leaf. `neg_gates` are the negate views on the chain.
static void FillSeedFoldArm(DROp &op, ProgramImpl *impl,
                            const std::vector<QueryView> &neg_gates) {
  DRArm arm;
  arm.delta_pos = 0u;  // the source member is the delta position
  arm.sign = op.seed_sign;

  // vector:drain of the source's signed frontier.
  DREffect drain;
  drain.kind = EffKind::kVecDrain;
  drain.value_table = op.seed_source;
  drain.vec_role =
      (op.seed_sign < 0) ? VecRole::kNetRemoval : VecRole::kNetAddition;
  arm.effects.push_back(drain);

  // Build the plan spine root→leaf. We construct leaf-first then wrap.
  std::unique_ptr<PlanNode> spine;
  if (op.join_pivot) {
    // A join-terminal seed appends pivots — no fold leaf, no head table. The
    // plan spine is just the negate gates (usually none corpus-wide). Model the
    // "leaf" as an empty gate-less node with no fold (child=null); V-ONE-FOLD
    // requires exactly one Fold, so a join-pivot seed carries NO PlanTree.
    // (The pivot append is a vector effect, below.)
  } else {
    spine = MakeFoldLeaf(op.seed_target, op.seed_sign, op.seed_class);

    // counter±(target) + kInI crossing read for the head fold.
    DREffect counter;
    counter.kind = EffKind::kCounter;
    counter.counter_table = op.seed_target;
    counter.sign = op.seed_sign;
    counter.klass = op.seed_class;
    arm.effects.push_back(counter);

    DREffect crossing;
    crossing.kind = EffKind::kInIReadFrozen;
    crossing.read_table = op.seed_target;
    crossing.pred = Pred::kInI;
    crossing.ctx = Ctx::kSeed;
    arm.effects.push_back(crossing);
  }

  // Wrap the spine with a GATE per NEGATE on the chain (seed context, kInI both
  // signs), and record each gate's read effect (E-13).
  for (QueryView neg : neg_gates) {
    DataModel *const nm =
        impl->view_to_model[QueryNegate::From(neg).NegatedView()]
            ->FindAs<DataModel>();
    TABLE *const negated_table = nm->table;

    const Pred gate_pred = NegateGatePred(Ctx::kSeed, NegateHint::kNormal);
    auto gate = std::make_unique<PlanNode>();
    gate->kind = PlanKind::kGate;
    gate->table = negated_table;
    gate->pred = gate_pred;
    gate->absent = true;  // negate forward gate is polarity=ABSENT
    gate->ctx = Ctx::kSeed;
    gate->child = std::move(spine);
    spine = std::move(gate);

    DREffect read;
    read.kind = EffKind::kFlagRead;
    read.read_table = negated_table;
    read.pred = gate_pred;
    read.ctx = Ctx::kSeed;
    arm.effects.push_back(read);
  }

  // vector:append into the target's sign queue (head chain) OR the join pivots
  // (join-terminal seed).
  if (op.join_pivot) {
    // The pivot append targets the join-pivots vec (no per-table role); model
    // it as a param/output append with a null table (a message-less vec).
    DREffect append;
    append.kind = EffKind::kVecAppend;
    append.value_table = nullptr;
    append.vec_role = VecRole::kJoinPivots;
    arm.effects.push_back(append);
  } else {
    DREffect append;
    append.kind = EffKind::kVecAppend;
    append.value_table = op.seed_target;
    append.vec_role =
        (op.seed_sign < 0) ? VecRole::kDeleteQueue : VecRole::kAddQueue;
    arm.effects.push_back(append);
  }

  arm.body = std::move(spine);
  op.effects = arm.effects;  // op-level union == the single arm's set (A-3)
  op.arms.push_back(std::move(arm));
}

// The four-cell claim-relative matrix predicate for a FIXPOINT_FIRE arm reading
// same-SCC side `j` while `p` is the delta (mirrors EmitJoinFire:1184-1190).
static Pred FixpointSamePred(size_t j_pos, size_t p_pos, bool is_del) {
  if (j_pos < p_pos) {
    return is_del ? Pred::kSurvivesSoFar : Pred::kInNewWithFrontier;  // permissive
  }
  return is_del ? Pred::kAliveAtClaim : Pred::kInNewSansFrontier;  // strict
}

}  // namespace

std::vector<const DROp *> DRFlowGraph::Crossovers(void) const {
  std::vector<const DROp *> out;
  for (const DROp &op : ops) {
    if (op.kind == DROpKind::kCrossover) {
      out.push_back(&op);
    }
  }
  return out;
}

std::vector<const DROp *> DRFlowGraph::ProductArms(void) const {
  std::vector<const DROp *> out;
  for (const DROp &op : ops) {
    if (op.kind == DROpKind::kProductArm) {
      out.push_back(&op);
    }
  }
  return out;
}

std::vector<const DROp *> DRFlowGraph::OpsOfKind(DROpKind kind) const {
  std::vector<const DROp *> out;
  for (const DROp &op : ops) {
    if (op.kind == kind) {
      out.push_back(&op);
    }
  }
  return out;
}

unsigned DRFlowGraph::TableVec(TABLE *table, VecRole role) const {
  auto it = table_vecs.find(table);
  assert(it != table_vecs.end());
  auto jt = it->second.find(role);
  assert(jt != it->second.end());
  return jt->second;
}

// Mint one materialized `DRVec` of `role` for `table` and record it in
// `flow.table_vecs`. `shape`/`kind`/`uniq` are the R1b constructor knowledge
// (A-5 element_shape; the demoted debug (table,kind); G6 unique contract).
static void MintTableVec(DRFlowGraph &flow, TABLE *table, VecRole role,
                         ElementShape shape, VectorKind kind,
                         UniqueContract uniq) {
  const unsigned idx = static_cast<unsigned>(flow.vecs.size());
  DRVec v;
  v.shape = shape;
  v.role = role;
  v.uniq = uniq;
  v.debug_table = table;
  v.debug_kind = kind;
  flow.vecs.push_back(std::move(v));
  flow.table_vecs[table][role] = idx;
}

DRFlowGraph BuildDRInventory(
    ProgramImpl *impl, Context &context, Query query,
    const std::unordered_map<TABLE *, unsigned> &scc_map) {

  DRFlowGraph flow;
  flow.scc_map = scc_map;

  // ------------------------------------------------------------------ tables
  // A debug-labelled model entry per differential/monotone table, with its
  // identity-distinct member-view list (V-MEMBER-ID: never structurally
  // dedup). The member list mirrors `table->views` (already identity-deduped
  // by Data.cpp GetOrCreate).
  for (TABLE *table : impl->tables) {
    DRTable t;
    t.model = table;
    t.differential = TableIsDifferential(table);
    for (const QueryView &view : table->views) {
      t.member_views.push_back(view);
    }
    flow.tables.push_back(std::move(t));
  }

  // --------------------------------------------------------------- per-table vecs
  // Materialize the six vecs each differential non-induction-owned table's
  // discovery band owns (spec §1): delete/add queues, overdelete/addition sets,
  // net-removals/net-additions frontiers. element_shape is kIds (queues/sets/
  // frontiers carry ROW IDS into the one table — A-5 constructor knowledge, not
  // read back from IR). uniq: the two claim-drain INPUT queues carry
  // sort-unique-at-drain (mirrors the tc 10-site census — a claim drain
  // VectorUniques its queue immediately before the loop, G6); the sets and
  // consolidated frontiers are multiset (their producers append deduped rows,
  // and the frontier drains sort-unique at the HIGHER seed, an edge attribute
  // R1c records). Induction-owned tables are fed by the fixpoint machinery, not
  // the phases, so they own no phase vec here (mirrors `phase_table_order`,
  // Stratum.cpp:1664).
  for (TABLE *table : impl->tables) {
    if (!TableIsDifferential(table) || TableIsInductionOwnedDR(context, table)) {
      continue;
    }
    MintTableVec(flow, table, VecRole::kDeleteQueue, ElementShape::kIds,
                 VectorKind::kDeleteQueue, UniqueContract::kSortUniqueAtDrain);
    MintTableVec(flow, table, VecRole::kAddQueue, ElementShape::kIds,
                 VectorKind::kAddQueue, UniqueContract::kSortUniqueAtDrain);
    MintTableVec(flow, table, VecRole::kOverdeleteSet, ElementShape::kIds,
                 VectorKind::kOverdeleteSet, UniqueContract::kMultiset);
    MintTableVec(flow, table, VecRole::kAdditionSet, ElementShape::kIds,
                 VectorKind::kAdditionSet, UniqueContract::kMultiset);
    MintTableVec(flow, table, VecRole::kNetRemoval, ElementShape::kIds,
                 VectorKind::kNetRemovals, UniqueContract::kMultiset);
    MintTableVec(flow, table, VecRole::kNetAddition, ElementShape::kIds,
                 VectorKind::kNetAdditions, UniqueContract::kMultiset);
  }

  // -------------------------------------------------------------- crossovers
  // One arm-PAIR per non-@never negate (spec §2.1 / B-3.1). Derived from
  // `query.Negations()` INDEPENDENTLY: the negate's own table, its negated
  // view's model table, its data predecessor's table (`Predecessors()[0]`),
  // and whether the `+` arm exists (the negated table is differential — the
  // exact `TableIsDifferential` rule). Emitted as TWO `DROp`s (the `-` arm
  // always, the `+` arm iff `negated_differential`) sharing the crossover data.
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

    const bool negated_differential = TableIsDifferential(negated_table);

    // The shared RuleClass over the negate table & its two read tables (the
    // pred it scans and the negated table it retracts against).
    const DerivClass klass =
        RuleClass(scc_map, negate_table, {pred_table, negated_table});

    // Record the arm's queue-append DEF edge (A-4: multi-def). The `-` arm
    // appends into the negate table's delete queue, the `+` arm into its add
    // queue. The op index is the post-push slot.
    const auto record_append_def = [&](unsigned op_idx, VecRole queue) {
      const unsigned vec_idx = flow.TableVec(negate_table, queue);
      flow.vecs[vec_idx].defs.push_back(op_idx);
    };

    // The `-` arm always exists (a negated-view GAIN retracts the output).
    {
      DROp minus(DROpKind::kCrossover);
      minus.ctx = Ctx::kSeed;
      minus.negate = negate;
      minus.negate_table = negate_table;
      minus.negated_table = negated_table;
      minus.pred_table = pred_table;
      minus.pred_view = pred_view;
      minus.negated_differential = negated_differential;
      minus.crossover_sign = -1;
      minus.effects = CrossoverArmEffects(minus, -1, klass);
      const unsigned op_idx = static_cast<unsigned>(flow.ops.size());
      flow.ops.push_back(std::move(minus));
      record_append_def(op_idx, VecRole::kDeleteQueue);
    }

    // The `+` arm exists only when the negated table is differential (a
    // negated-view LOSS re-derives the output over its NetRemovals frontier).
    if (negated_differential) {
      DROp plus(DROpKind::kCrossover);
      plus.ctx = Ctx::kSeed;
      plus.negate = negate;
      plus.negate_table = negate_table;
      plus.negated_table = negated_table;
      plus.pred_table = pred_table;
      plus.pred_view = pred_view;
      plus.negated_differential = negated_differential;
      plus.crossover_sign = +1;
      plus.effects = CrossoverArmEffects(plus, +1, klass);
      const unsigned op_idx = static_cast<unsigned>(flow.ops.size());
      flow.ops.push_back(std::move(plus));
      record_append_def(op_idx, VecRole::kAddQueue);
    }
  }

  // ----------------------------------------------------------------- products
  // One arm per side×sign of each ACYCLIC differential 0-pivot join (spec §2.1
  // / B-3.2). Derived INDEPENDENTLY from `query.Joins()`: a 0-pivot join whose
  // view can receive deletions. The minus arm exists iff the side is
  // differential; every fold is `kNonRecursive` (the acyclic fence guarantees
  // no same-SCC side). The other-side reads are position-keyed sign-
  // INDEPENDENT: j < side_index at kInNew, j > side_index at kInI.
  for (QueryJoin join : query.Joins()) {
    const QueryView join_view(join);
    if (join.NumPivotColumns() || !join_view.CanReceiveDeletions()) {
      continue;
    }

    DataModel *const join_model =
        impl->view_to_model[join_view]->FindAs<DataModel>();
    TABLE *const product_table = join_model->table;

    std::vector<TABLE *> side_tables;
    std::vector<bool> side_diff;
    for (QueryView side : join.JoinedViews()) {
      TABLE *const side_table =
          impl->view_to_model[side]->FindAs<DataModel>()->table;
      side_tables.push_back(side_table);
      side_diff.push_back(TableIsDifferential(side_table));
    }

    const unsigned num_sides = static_cast<unsigned>(side_tables.size());
    for (unsigned i = 0u; i < num_sides; ++i) {

      // The signs this side contributes: `+` always, `-` iff differential.
      const bool has_minus = side_diff[i];
      const int signs[2] = {+1, -1};
      for (int k = 0; k < (has_minus ? 2 : 1); ++k) {
        const int sign = signs[k];

        DROp arm(DROpKind::kProductArm);
        arm.ctx = Ctx::kSeed;
        arm.product_view = join_view;
        arm.product_table = product_table;
        arm.side_index = i;
        arm.product_sign = sign;
        arm.side_tables = side_tables;
        arm.side_differential = side_diff;

        // Position-keyed reads over the OTHER sides.
        for (unsigned j = 0u; j < num_sides; ++j) {
          if (j == i) {
            continue;  // the delta side is looped, not read
          }
          const Pred pred = (j < i) ? Pred::kInNew : Pred::kInI;
          arm.arm_reads.emplace_back(side_tables[j], pred);
        }

        arm.effects = ProductArmEffects(arm, sign);
        const unsigned op_idx = static_cast<unsigned>(flow.ops.size());
        flow.ops.push_back(std::move(arm));

        // The arm's queue-append DEF edge (A-4: multi-def): the `-` arm into
        // the product table's delete queue, the `+` arm into its add queue.
        const VecRole queue =
            (sign < 0) ? VecRole::kDeleteQueue : VecRole::kAddQueue;
        flow.vecs[flow.TableVec(product_table, queue)].defs.push_back(op_idx);
      }
    }
  }

  // ------------------------------------------------------------- branches/joins
  // The §1.4 branch/join inventory, derived INDEPENDENTLY via the memoized
  // worklist (`SuffixesOf`). Rooted at every member view of every source table
  // (source = differential OR carries a net-additions frontier — replicated
  // EXACTLY from Stratum.cpp:1517-1524), it reproduces the old path-copying
  // DFS's per-PATH branch multiset (a diamond of plumbing fans out to one
  // branch per distinct path — see `SuffixesOf`). Joins are deduped by view;
  // the DR side MINTS the ONE shared join-pivots vec each owns.
  for (TABLE *table : impl->tables) {
    bool is_source = TableIsDifferential(table);
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
    BranchMemo memo;  // per-source memo (source-dependent same-table stop)
    for (const QueryView &member : table->views) {
      if (member.IsInsert() || member.Columns().size() != num_cols) {
        continue;
      }
      for (const BranchSuffix &bs :
           SuffixesOf(impl, context, table, member, memo)) {
        DRBranch branch;
        branch.source = table;
        branch.path.reserve(bs.suffix.size() + 1u);
        branch.path.push_back(member);  // path[0] = source member (Stratum:1535)
        branch.path.insert(branch.path.end(), bs.suffix.begin(),
                           bs.suffix.end());
        branch.ends_at_join = bs.ends_at_join;
        branch.target = bs.target;
        flow.branches.push_back(std::move(branch));
      }
    }
  }

  // Dedup the join emissions by join view, minting ONE shared join-pivots vec
  // per (A-5: element_shape id+cols over the union pivot column set — the two
  // section walks project different subsets, carried in the future plan tree,
  // not the vec; the pivot vec is sort-unique-at-drain, G2/PIVOT_ASSEMBLE).
  std::unordered_map<QueryView, size_t> join_index;
  for (const DRBranch &branch : flow.branches) {
    if (!branch.ends_at_join) {
      continue;
    }
    const QueryView join_view = branch.path.back();
    if (join_index.emplace(join_view, flow.joins.size()).second) {
      DRJoin j(join_view);

      // Mint the shared pivot vec (id+cols, sort-unique-at-drain).
      const unsigned vec_idx = static_cast<unsigned>(flow.vecs.size());
      DRVec v;
      v.shape = ElementShape::kIdCols;  // A-5: union pivot column set
      v.role = VecRole::kJoinPivots;
      v.uniq = UniqueContract::kSortUniqueAtDrain;
      v.debug_table = nullptr;  // a message-less join-owned vec
      v.debug_kind = VectorKind::kJoinPivots;
      flow.vecs.push_back(std::move(v));
      j.pivot_vec = vec_idx;

      CollectSectionTargetsDR(impl, context, join_view, j.targets);
      flow.joins.push_back(std::move(j));
    }
  }

  // ==========================================================================
  // R1c op families. Each is DERIVED independently and mirrors the emission
  // driver's counting rules EXACTLY (Stratum.cpp cites inline). Strata are NOT
  // consulted (B-13-seeded later): the op SET is structural, only its emission
  // ORDER is per-stratum, so grouping by stratum is unnecessary here.
  // ==========================================================================

  const auto is_recursive = [&](TABLE *table) -> bool {
    return SccOf(scc_map, table).has_value();
  };
  const auto same_scc = [&](TABLE *a, TABLE *b) -> bool {
    const auto sa = SccOf(scc_map, a);
    return sa.has_value() && sa == SccOf(scc_map, b);
  };
  // Replicate `all_sides_same_scc` (Stratum.cpp:1492-1506): every joined side
  // shares the join's own SCC.
  const auto all_sides_same_scc = [&](QueryView join_view) -> bool {
    DataModel *const jm = impl->view_to_model[join_view]->FindAs<DataModel>();
    const auto join_scc = SccOf(scc_map, jm->table);
    if (!join_scc.has_value()) {
      return false;
    }
    for (QueryView side : QueryJoin::From(join_view).JoinedViews()) {
      TABLE *const st = impl->view_to_model[side]->FindAs<DataModel>()->table;
      if (SccOf(scc_map, st) != join_scc) {
        return false;
      }
    }
    return true;
  };
  // The negate views on a branch chain, in path order (for the seed plan-tree
  // gates + their E-13 reads).
  const auto chain_negates = [&](const DRBranch &branch) {
    std::vector<QueryView> gates;
    for (const QueryView &view : branch.path) {
      if (view.IsNegate()) {
        gates.push_back(view);
      }
    }
    return gates;
  };

  // ------------------------------------------------------- SEED_FOLD / CHAIN_FOLD
  // Per branch × available sign (mirror Stratum.cpp:2012-2080). Two suppressions
  // (both DERIVED, V-SEED-SUP): (1) a same-SCC internal head projection edge is
  // NOT a top-level seed — it fires inside the claim round as a CHAIN_FOLD
  // (Stratum.cpp:2026-2029 / :2311-2326); (2) an all-same-SCC join has NO seed
  // (Stratum.cpp:2035-2037). For a non-suppressed branch: `-` arm iff the source
  // is differential & not induction-owned (Stratum.cpp:2071-2076), `+` arm
  // always (Stratum.cpp:2077-2079). Join-terminal seeds append pivots (no fold).
  for (unsigned bi = 0u; bi < flow.branches.size(); ++bi) {
    const DRBranch &branch = flow.branches[bi];
    TABLE *const source = branch.source;
    const bool src_seedable =
        TableIsDifferential(source) && !TableIsInductionOwnedDR(context, source);
    const bool has_minus = src_seedable;  // `-` arm existence (Stratum:2071)

    // (1) same-SCC internal head projection → CHAIN_FOLD (in-round, both signs).
    if (!branch.ends_at_join && is_recursive(branch.target) &&
        same_scc(branch.target, source)) {
      // The in-fixpoint re-fire only runs for a differential non-induction
      // source (Stratum.cpp:2317-2319); both signs each round.
      if (src_seedable) {
        for (int sign : {-1, +1}) {
          DROp op(DROpKind::kChainFold);
          op.ctx = Ctx::kFixpoint;
          op.chain_branch = bi;
          op.chain_sign = sign;
          op.chain_source = source;
          op.chain_target = branch.target;
          op.scc_group = *SccOf(scc_map, branch.target);
          op.seed_class = DerivClass::kRecursive;  // Stratum.cpp:2321

          DRArm arm;
          arm.sign = sign;
          arm.delta_pos = 0u;
          DREffect drain;
          drain.kind = EffKind::kVecDrain;
          drain.value_table = source;
          drain.vec_role =
              (sign < 0) ? VecRole::kClaimedDel : VecRole::kClaimedAdd;
          arm.effects.push_back(drain);
          // The chain's negate gates (fixpoint context — kInNew both signs).
          for (QueryView neg : chain_negates(branch)) {
            DataModel *const nm =
                impl->view_to_model[QueryNegate::From(neg).NegatedView()]
                    ->FindAs<DataModel>();
            DREffect read;
            read.kind = EffKind::kFlagRead;
            read.read_table = nm->table;
            read.pred = NegateGatePred(Ctx::kFixpoint, NegateHint::kNormal);
            read.ctx = Ctx::kFixpoint;
            arm.effects.push_back(read);
          }
          DREffect counter;
          counter.kind = EffKind::kCounter;
          counter.counter_table = branch.target;
          counter.sign = sign;
          counter.klass = DerivClass::kRecursive;
          arm.effects.push_back(counter);
          DREffect crossing;
          crossing.kind = EffKind::kInIReadFrozen;
          crossing.read_table = branch.target;
          crossing.pred = Pred::kInI;
          crossing.ctx = Ctx::kFixpoint;
          arm.effects.push_back(crossing);
          DREffect append;
          append.kind = EffKind::kVecAppend;
          append.value_table = branch.target;
          append.vec_role =
              (sign < 0) ? VecRole::kDeleteQueue : VecRole::kAddQueue;
          arm.effects.push_back(append);

          // Plan spine: negate gates (fixpoint) → fold leaf.
          std::unique_ptr<PlanNode> spine =
              MakeFoldLeaf(branch.target, sign, DerivClass::kRecursive);
          for (QueryView neg : chain_negates(branch)) {
            DataModel *const nm =
                impl->view_to_model[QueryNegate::From(neg).NegatedView()]
                    ->FindAs<DataModel>();
            auto gate = std::make_unique<PlanNode>();
            gate->kind = PlanKind::kGate;
            gate->table = nm->table;
            gate->pred = NegateGatePred(Ctx::kFixpoint, NegateHint::kNormal);
            gate->absent = true;
            gate->ctx = Ctx::kFixpoint;
            gate->child = std::move(spine);
            spine = std::move(gate);
          }
          arm.body = std::move(spine);
          op.effects = arm.effects;
          op.arms.push_back(std::move(arm));

          const unsigned op_idx = static_cast<unsigned>(flow.ops.size());
          flow.ops.push_back(std::move(op));
          const VecRole q =
              (sign < 0) ? VecRole::kDeleteQueue : VecRole::kAddQueue;
          flow.vecs[flow.TableVec(branch.target, q)].defs.push_back(op_idx);
        }
      }
      continue;  // NEVER also a top-level seed (V-SEED-SUP negative space).
    }

    // (2) all-same-SCC join → no seed at all (V-SEED-SUP).
    if (branch.ends_at_join && all_sides_same_scc(branch.path.back())) {
      continue;
    }

    // The head fold class (Stratum.cpp:2068) — recursive iff target shares the
    // source's SCC (a linear-recursion join-table → union edge). Null target
    // for a join-terminal seed.
    TABLE *target = nullptr;
    DerivClass klass = DerivClass::kNonRecursive;
    if (!branch.ends_at_join) {
      target = branch.target;
      klass = RuleClass(scc_map, target, {source});
    }

    const std::vector<QueryView> neg_gates = chain_negates(branch);
    const int signs[2] = {-1, +1};
    for (int k = (has_minus ? 0 : 1); k < 2; ++k) {
      const int sign = signs[k];
      DROp op(DROpKind::kSeedFold);
      op.ctx = Ctx::kSeed;
      op.seed_branch = bi;
      op.seed_sign = sign;
      op.seed_source = source;
      op.seed_target = target;
      op.seed_class = klass;
      op.join_pivot = branch.ends_at_join;
      FillSeedFoldArm(op, impl, neg_gates);

      const unsigned op_idx = static_cast<unsigned>(flow.ops.size());
      flow.ops.push_back(std::move(op));
      if (!branch.ends_at_join) {
        const VecRole q = (sign < 0) ? VecRole::kDeleteQueue : VecRole::kAddQueue;
        flow.vecs[flow.TableVec(target, q)].defs.push_back(op_idx);
      }
    }
  }

  // ------------------------------------------------------------ FIXPOINT_FIRE
  // ONE op per (SCC join, sign) (G2 / Stratum.cpp:2238-2246 × del+add loops). A
  // join is a fixpoint fire iff its persisted table is in a recursive SCC. Each
  // op groups one arm per same-SCC delta position (JoinedViews() order); other
  // same-SCC positions read the claim-relative matrix, lower positions kInNew.
  for (const DRJoin &j : flow.joins) {
    const QueryView join_view = j.join_view;
    DataModel *const jm = impl->view_to_model[join_view]->FindAs<DataModel>();
    TABLE *const join_table = jm->table;
    const auto join_scc = SccOf(scc_map, join_table);
    if (!join_scc.has_value()) {
      continue;  // only SCC joins fire in a claim round.
    }

    // Partition sides into same-SCC positions and lower sides (EmitJoinFire:
    // 1099-1112).
    std::vector<QueryView> all_sides;
    std::vector<size_t> same_pos;
    std::vector<size_t> lower_pos;
    for (QueryView side : QueryJoin::From(join_view).JoinedViews()) {
      TABLE *const st = impl->view_to_model[side]->FindAs<DataModel>()->table;
      if (SccOf(scc_map, st) == join_scc) {
        same_pos.push_back(all_sides.size());
      } else {
        lower_pos.push_back(all_sides.size());
      }
      all_sides.push_back(side);
    }
    assert(!same_pos.empty());

    for (int sign : {-1, +1}) {
      const bool is_del = (sign < 0);
      DROp op(DROpKind::kFixpointFire);
      op.ctx = Ctx::kFixpoint;
      op.fire_join = join_view;
      op.fire_table = join_table;
      op.fire_sign = sign;

      std::vector<DREffect> op_fx;
      for (size_t p_pos : same_pos) {
        DRArm arm;
        arm.delta_pos = static_cast<unsigned>(p_pos);
        arm.sign = sign;

        TABLE *const delta_table =
            impl->view_to_model[all_sides[p_pos]]->FindAs<DataModel>()->table;
        DREffect drain;
        drain.kind = EffKind::kVecDrain;
        drain.value_table = delta_table;
        drain.vec_role =
            is_del ? VecRole::kClaimedDel : VecRole::kClaimedAdd;
        arm.effects.push_back(drain);

        // The ordered scan list (EmitJoinFire:1178-1195): other same-SCC sides
        // with the claim-relative matrix pred, then lower sides at kInNew.
        std::unique_ptr<PlanNode> spine =
            MakeFoldLeaf(join_table, sign, DerivClass::kRecursive);
        // Build leaf-first: iterate scan list in REVERSE so the outermost scan
        // wraps last (matching scan_next's ascending nesting).
        std::vector<std::pair<size_t, Pred>> scan_list;
        for (size_t j_pos : same_pos) {
          if (j_pos == p_pos) {
            continue;
          }
          scan_list.emplace_back(j_pos, FixpointSamePred(j_pos, p_pos, is_del));
        }
        for (size_t j_pos : lower_pos) {
          scan_list.emplace_back(j_pos, Pred::kInNew);
        }
        for (auto it = scan_list.rbegin(); it != scan_list.rend(); ++it) {
          const size_t j_pos = it->first;
          const Pred pred = it->second;
          const QueryView side = all_sides[j_pos];
          TABLE *const st =
              impl->view_to_model[side]->FindAs<DataModel>()->table;

          // The side's bound columns: its pivot input columns (mirrors
          // EmitJoinFire's `avail`, :1250-1263), canonicalized per B-1. The
          // re-test column (the D1 cursor pivot compare) is the first pivot
          // input in join-pivot order.
          std::vector<QueryColumn> pivot_inputs;
          const QueryJoin jv = QueryJoin::From(join_view);
          for (auto pj = 0u, num_pivots = jv.NumPivotColumns(); pj < num_pivots;
               ++pj) {
            for (QueryColumn in_pivot : jv.NthInputPivotSet(pj)) {
              if (QueryView::Containing(in_pivot) == side) {
                pivot_inputs.push_back(in_pivot);
              }
            }
          }

          auto acc = std::make_unique<PlanNode>();
          acc->kind = PlanKind::kAccess;
          acc->table = st;
          acc->bound_cols = CanonBoundCols(pivot_inputs);  // B-1 canonical set
          acc->pivot_col =
              pivot_inputs.empty() ? PlanNode::kNoPivot : pivot_inputs[0].Id();
          acc->pred = pred;
          acc->ctx = Ctx::kFixpoint;
          acc->lowering = pivot_inputs.empty() ? Lowering::kFullScan
                                               : Lowering::kSectionWalk;
          acc->child = std::move(spine);
          spine = std::move(acc);

          DREffect read;
          read.kind = EffKind::kFlagRead;
          read.read_table = st;
          read.pred = pred;
          read.ctx = Ctx::kFixpoint;
          arm.effects.push_back(read);
        }

        DREffect counter;
        counter.kind = EffKind::kCounter;
        counter.counter_table = join_table;
        counter.sign = sign;
        counter.klass = DerivClass::kRecursive;
        arm.effects.push_back(counter);
        DREffect crossing;
        crossing.kind = EffKind::kInIReadFrozen;
        crossing.read_table = join_table;
        crossing.pred = Pred::kInI;
        crossing.ctx = Ctx::kFixpoint;
        arm.effects.push_back(crossing);
        DREffect append;
        append.kind = EffKind::kVecAppend;
        append.value_table = join_table;
        append.vec_role =
            is_del ? VecRole::kDeleteQueue : VecRole::kAddQueue;
        arm.effects.push_back(append);

        arm.body = std::move(spine);
        for (const DREffect &e : arm.effects) {
          op_fx.push_back(e);
        }
        op.arms.push_back(std::move(arm));
      }
      op.effects = std::move(op_fx);  // op-level union (A-3)

      const unsigned op_idx = static_cast<unsigned>(flow.ops.size());
      flow.ops.push_back(std::move(op));
      const VecRole q = is_del ? VecRole::kDeleteQueue : VecRole::kAddQueue;
      flow.vecs[flow.TableVec(join_table, q)].defs.push_back(op_idx);
    }
  }

  // -------------------- CLAIM_DRAIN / RETIRE / REDERIVE / FRONTIER_FILTER
  // Per phase table (differential, not induction-owned — Stratum.cpp:1664-1668).
  // Acyclic tables: single-pass del+add drains, immediate del+add filters
  // (Stratum.cpp:2214-2221). SCC tables: in-round del+add drains (dual-append +
  // input-queue clear), retire del+add, rederive (del only), deferred del+add
  // filters (Stratum.cpp:2293-2377).
  const auto mint_claim = [&](TABLE *table, int sign, ClaimForm form) {
    const bool is_del = (sign < 0);
    DROp op(DROpKind::kClaimDrain);
    op.ctx = (form == ClaimForm::kInRound) ? Ctx::kFixpoint : Ctx::kSeed;
    op.table_op_table = table;
    op.table_op_sign = sign;
    op.claim_form = form;
    op.claim_gate = is_del ? Pred::kNetDeleted : Pred::kNetAdded;  // F17 gate DATA
    if (is_recursive(table)) {
      op.scc_group = *SccOf(scc_map, table);
    }

    // vector:drain(queue) — the F17 gate re-tests at dequeue (V-CLAIM-GATE).
    DREffect drain;
    drain.kind = EffKind::kVecDrain;
    drain.value_table = table;
    drain.vec_role = is_del ? VecRole::kDeleteQueue : VecRole::kAddQueue;
    op.effects.push_back(drain);
    // flags:write (kDel|kDelNow / kAdd|kAddNow).
    DREffect fw;
    fw.kind = EffKind::kFlagWrite;
    fw.write_table = table;
    fw.sign = sign;
    op.effects.push_back(fw);
    // vector:append(overdelete-set / addition-set) — the persistent claim set.
    DREffect set_append;
    set_append.kind = EffKind::kVecAppend;
    set_append.value_table = table;
    set_append.vec_role =
        is_del ? VecRole::kOverdeleteSet : VecRole::kAdditionSet;
    op.effects.push_back(set_append);
    if (form == ClaimForm::kInRound) {
      // The DUAL-APPEND (G10 / B-11): also the per-round claimed frontier.
      DREffect front_append;
      front_append.kind = EffKind::kVecAppend;
      front_append.value_table = table;
      front_append.vec_role = is_del ? VecRole::kClaimedDel : VecRole::kClaimedAdd;
      op.effects.push_back(front_append);
      // The B-7 input-queue CLEAR (in-round only) — V-QCLEAR.
      DREffect clear;
      clear.kind = EffKind::kVecClear;
      clear.value_table = table;
      clear.vec_role = is_del ? VecRole::kDeleteQueue : VecRole::kAddQueue;
      op.effects.push_back(clear);
    }
    flow.ops.push_back(std::move(op));
  };

  const auto mint_filter = [&](TABLE *table, int sign, Deferral deferral) {
    const bool is_del = (sign < 0);
    DROp op(DROpKind::kFrontierFilter);
    op.ctx = (deferral == Deferral::kAddLoopOutput) ? Ctx::kFixpoint : Ctx::kSeed;
    op.table_op_table = table;
    op.table_op_sign = sign;
    op.deferral = deferral;
    if (is_recursive(table)) {
      op.scc_group = *SccOf(scc_map, table);
    }
    DREffect drain;
    drain.kind = EffKind::kVecDrain;
    drain.value_table = table;
    drain.vec_role = is_del ? VecRole::kOverdeleteSet : VecRole::kAdditionSet;
    op.effects.push_back(drain);
    DREffect read;
    read.kind = EffKind::kFlagRead;
    read.read_table = table;
    read.pred = is_del ? Pred::kNetDeleted : Pred::kNetAdded;
    read.ctx = op.ctx;
    op.effects.push_back(read);
    DREffect append;
    append.kind = EffKind::kVecAppend;
    append.value_table = table;
    append.vec_role = is_del ? VecRole::kNetRemoval : VecRole::kNetAddition;
    op.effects.push_back(append);
    flow.ops.push_back(std::move(op));
  };

  for (TABLE *table : impl->tables) {
    if (!TableIsDifferential(table) || TableIsInductionOwnedDR(context, table)) {
      continue;
    }
    if (is_recursive(table)) {
      const unsigned g = *SccOf(scc_map, table);
      // In-round drains (del in OVERDELETE loop, add in INSERT loop).
      mint_claim(table, -1, ClaimForm::kInRound);
      mint_claim(table, +1, ClaimForm::kInRound);
      // Retire del + add (per round tail).
      for (int sign : {-1, +1}) {
        DROp op(DROpKind::kRetire);
        op.ctx = Ctx::kFixpoint;
        op.table_op_table = table;
        op.table_op_sign = sign;
        op.scc_group = g;
        DREffect drain;
        drain.kind = EffKind::kVecDrain;
        drain.value_table = table;
        drain.vec_role = (sign < 0) ? VecRole::kClaimedDel : VecRole::kClaimedAdd;
        op.effects.push_back(drain);
        DREffect fw;
        fw.kind = EffKind::kFlagWrite;
        fw.write_table = table;
        fw.sign = sign;  // clear kDelNow (-) / kAddNow (+)
        op.effects.push_back(fw);
        flow.ops.push_back(std::move(op));
      }
      // Rederive (del side only): drain(overdelete-set), read(RecursivelySupported),
      // append(addQ). Gate kRecursivelySupported.
      {
        DROp op(DROpKind::kRederive);
        op.ctx = Ctx::kFixpoint;
        op.table_op_table = table;
        op.scc_group = g;
        DREffect drain;
        drain.kind = EffKind::kVecDrain;
        drain.value_table = table;
        drain.vec_role = VecRole::kOverdeleteSet;
        op.effects.push_back(drain);
        DREffect read;
        read.kind = EffKind::kFlagRead;
        read.read_table = table;
        read.pred = Pred::kRecursivelySupported;
        read.ctx = Ctx::kFixpoint;
        op.effects.push_back(read);
        DREffect append;
        append.kind = EffKind::kVecAppend;
        append.value_table = table;
        append.vec_role = VecRole::kAddQueue;
        op.effects.push_back(append);
        flow.ops.push_back(std::move(op));
      }
      // Deferred filters (BOTH signs, add-loop-output — E-17).
      mint_filter(table, -1, Deferral::kAddLoopOutput);
      mint_filter(table, +1, Deferral::kAddLoopOutput);
    } else {
      // Acyclic: single-pass drains + immediate filters.
      mint_claim(table, -1, ClaimForm::kSinglePass);
      mint_claim(table, +1, ClaimForm::kSinglePass);
      mint_filter(table, -1, Deferral::kImmediate);
      mint_filter(table, +1, Deferral::kImmediate);
    }
  }

  // ----------------------------------------------------------- COMMIT_SWEEP
  // One per differential table (flavor=differential, publish-target iff the
  // table backs a @differential transmit); one per MONOTONE boundary table with
  // a delta-vec entry (flavor=monotone / Seal). Mirror Procedure.cpp:307-345.
  std::unordered_set<TABLE *> published_tables;
  for (const auto &[message, transmit] : context.commit_published_view) {
    const auto pred = transmit.Predecessors()[0];
    DataModel *const pm = impl->view_to_model[pred]->FindAs<DataModel>();
    if (pm->table) {
      published_tables.insert(pm->table);
    }
  }
  for (TABLE *table : impl->tables) {
    if (!TableIsDifferential(table)) {
      // A monotone boundary table with a delta-vec entry gets a Seal sweep.
      if (context.table_delta_vecs.find(table) ==
          context.table_delta_vecs.end()) {
        continue;
      }
      DROp op(DROpKind::kCommitSweep);
      op.ctx = Ctx::kSeed;
      op.table_op_table = table;
      op.sweep_flavor = SweepFlavor::kMonotone;
      DREffect fw;
      fw.kind = EffKind::kFlagWrite;
      fw.write_table = table;
      op.effects.push_back(fw);
      flow.ops.push_back(std::move(op));
      continue;
    }
    DROp op(DROpKind::kCommitSweep);
    op.ctx = Ctx::kSeed;
    op.table_op_table = table;
    op.sweep_flavor = SweepFlavor::kDifferential;
    op.join_pivot = published_tables.count(table) != 0u;  // reuse flag: published?
    DREffect fr;
    fr.kind = EffKind::kInIReadFrozen;
    fr.read_table = table;
    fr.pred = Pred::kInI;
    fr.ctx = Ctx::kSeed;
    op.effects.push_back(fr);
    DREffect fw;
    fw.kind = EffKind::kFlagWrite;
    fw.write_table = table;
    op.effects.push_back(fw);
    flow.ops.push_back(std::move(op));
  }

  return flow;
}

void ValidateDRInventory(
    const DRFlowGraph &flow,
    const std::vector<OldCrossoverRef> &old_crossovers,
    const std::vector<OldProductRef> &old_products,
    const std::vector<OldBranchRef> &old_branches,
    const std::vector<OldJoinRef> &old_joins,
    const std::unordered_map<TABLE *, unsigned> &old_scc_map,
    const std::unordered_map<TABLE *, unsigned> &old_drain_stratum) {

  // ----------------------------------------------------------- V-OLD-EQUIV(SCC)
  // Same table-SCC map (both directions: every entry present, same group).
  if (flow.scc_map.size() != old_scc_map.size()) {
    ValidatorFail("SCC map size differs from old discovery");
  }
  for (const auto &[table, group] : old_scc_map) {
    auto it = flow.scc_map.find(table);
    if (it == flow.scc_map.end() || it->second != group) {
      ValidatorFail("SCC map entry differs from old discovery");
    }
  }

  // ---------------------------------------------- V-OLD-EQUIV(crossovers) + B-3
  // The DR-IR emits a PAIR of DROps per crossover (the `-` arm always, `+` iff
  // negated_differential); the old discovery has ONE record per crossover. Fold
  // the DR ops back to per-negate crossover records keyed by negate identity,
  // then compare set-for-set. Along the way run V-XOVER-ONE (B-3.1): exactly
  // one `-` arm per non-@never negate, no negate_table folded by two distinct
  // negates, no @never negate present.
  struct XoverAgg {
    std::optional<QueryNegate> negate;
    TABLE *negate_table{nullptr};
    TABLE *negated_table{nullptr};
    TABLE *pred_table{nullptr};
    std::optional<QueryView> pred_view;
    bool negated_differential{false};
    unsigned minus_arms{0u};
    unsigned plus_arms{0u};
    bool seen{false};
  };
  std::unordered_map<QueryView, XoverAgg> agg;  // keyed by negate view identity

  for (const DROp &op : flow.ops) {
    if (op.kind != DROpKind::kCrossover) {
      continue;
    }
    if (op.negate->HasNeverHint()) {
      ValidatorFail("V-XOVER-ONE: a @never negate has a crossover arm");
    }
    XoverAgg &a = agg[QueryView(*op.negate)];
    if (!a.seen) {
      a.seen = true;
      a.negate = op.negate;
      a.negate_table = op.negate_table;
      a.negated_table = op.negated_table;
      a.pred_table = op.pred_table;
      a.pred_view = op.pred_view;
      a.negated_differential = op.negated_differential;
    } else {
      // Both arms of a pair must agree on the shared data.
      if (a.negate_table != op.negate_table ||
          a.negated_table != op.negated_table ||
          a.pred_table != op.pred_table ||
          a.negated_differential != op.negated_differential) {
        ValidatorFail("crossover arm-pair carries inconsistent tables");
      }
    }
    if (op.crossover_sign < 0) {
      ++a.minus_arms;
    } else if (op.crossover_sign > 0) {
      ++a.plus_arms;
    } else {
      ValidatorFail("crossover arm has no sign");
    }
  }

  // B-3.1 positive/negative space + arm-count consistency, and the
  // negate_table uniqueness (no two distinct negates fold the same table via a
  // crossover) — the promotion of the NDEBUG assert at Stratum.cpp:1595-1617.
  std::unordered_map<TABLE *, QueryView> negate_table_owner;
  for (auto &[view, a] : agg) {
    if (a.minus_arms != 1u) {
      ValidatorFail("V-XOVER-ONE: negate does not have exactly one `-` arm");
    }
    const unsigned expect_plus = a.negated_differential ? 1u : 0u;
    if (a.plus_arms != expect_plus) {
      ValidatorFail("V-XOVER-ONE: `+` arm presence disagrees with "
                    "negated_differential");
    }
    auto owner = negate_table_owner.emplace(a.negate_table, view);
    if (!owner.second && !(owner.first->second == view)) {
      ValidatorFail("V-XOVER-ONE: two distinct negates fold one negate table");
    }
  }

  // V-OLD-EQUIV: same crossover SET as the old discovery (same negate views,
  // same 3 tables + pred view + negated_differential).
  if (agg.size() != old_crossovers.size()) {
    ValidatorFail("crossover count differs from old discovery");
  }
  for (const OldCrossoverRef &old : old_crossovers) {
    auto it = agg.find(QueryView(old.negate));
    if (it == agg.end()) {
      ValidatorFail("old crossover has no DR-IR counterpart");
    }
    const XoverAgg &a = it->second;
    if (a.negate_table != old.negate_table ||
        a.negated_table != old.negated_table ||
        a.pred_table != old.pred_table ||
        !(a.pred_view.has_value() && *a.pred_view == old.pred_view) ||
        a.negated_differential != old.negated_differential) {
      ValidatorFail("crossover data differs from old discovery");
    }
  }

  // ------------------------------------------------ V-OLD-EQUIV(products) + B-3
  // Fold the product arms back to per-product records keyed by product view
  // identity. V-PROD-MONO (B-3.2): no `-` arm for a monotone side; a
  // differential side has both signs. V-PROD-CLASS (B-3.2): every product fold
  // is kNonRecursive.
  struct ProdAgg {
    std::optional<QueryView> product_view;
    TABLE *product_table{nullptr};
    std::vector<TABLE *> side_tables;
    std::vector<bool> side_differential;
    std::vector<unsigned> minus_arms;  // per side
    std::vector<unsigned> plus_arms;   // per side
    bool seen{false};
  };
  std::unordered_map<QueryView, ProdAgg> prod;

  for (const DROp &op : flow.ops) {
    if (op.kind != DROpKind::kProductArm) {
      continue;
    }
    ProdAgg &p = prod[*op.product_view];
    if (!p.seen) {
      p.seen = true;
      p.product_view = op.product_view;
      p.product_table = op.product_table;
      p.side_tables = op.side_tables;
      p.side_differential = op.side_differential;
      p.minus_arms.assign(op.side_tables.size(), 0u);
      p.plus_arms.assign(op.side_tables.size(), 0u);
    }
    if (op.side_index >= p.side_tables.size()) {
      ValidatorFail("product arm side_index out of range");
    }
    if (op.product_sign < 0) {
      ++p.minus_arms[op.side_index];
    } else if (op.product_sign > 0) {
      ++p.plus_arms[op.side_index];
    } else {
      ValidatorFail("product arm has no sign");
    }

    // V-PROD-CLASS: every counter fold in this arm is kNonRecursive.
    for (const DREffect &fx : op.effects) {
      if (fx.kind == EffKind::kCounter &&
          fx.klass != DerivClass::kNonRecursive) {
        ValidatorFail("V-PROD-CLASS: product fold is not kNonRecursive");
      }
    }
  }

  // V-PROD-MONO per side.
  for (auto &[view, p] : prod) {
    for (unsigned i = 0u; i < p.side_tables.size(); ++i) {
      if (p.side_differential[i]) {
        if (p.plus_arms[i] != 1u || p.minus_arms[i] != 1u) {
          ValidatorFail("V-PROD-MONO: differential side lacks both signs");
        }
      } else {
        if (p.minus_arms[i] != 0u) {
          ValidatorFail("V-PROD-MONO: monotone side has a `-` arm");
        }
        if (p.plus_arms[i] != 1u) {
          ValidatorFail("V-PROD-MONO: monotone side lacks its `+` arm");
        }
      }
    }
  }

  // V-OLD-EQUIV: same product SET (same product views/tables + side vectors +
  // side_differential).
  if (prod.size() != old_products.size()) {
    ValidatorFail("product count differs from old discovery");
  }
  for (const OldProductRef &old : old_products) {
    auto it = prod.find(old.product_view);
    if (it == prod.end()) {
      ValidatorFail("old product has no DR-IR counterpart");
    }
    const ProdAgg &p = it->second;
    if (p.product_table != old.product_table ||
        p.side_tables != old.side_tables ||
        p.side_differential != old.side_differential) {
      ValidatorFail("product data differs from old discovery");
    }
  }

  // -------------------------------------------- V-OLD-EQUIV(branches) — MULTISET
  // The §1.4 memoized worklist must reproduce the old path-copying DFS's branch
  // set as a MULTISET of (source, path, ends_at_join, target): the old code
  // records one chain PER DISTINCT PATH (no visited set), so reconvergent
  // plumbing produces path-multiplicity that must survive memoization. Match by
  // equality with a consumed-flag over the DR side (an O(n²) mark, branch
  // counts are suite-sized) — count equality plus a 1:1 pairing proves multiset
  // identity.
  const auto branch_eq = [](const DRBranch &d, const OldBranchRef &o) -> bool {
    return d.source == o.source && d.ends_at_join == o.ends_at_join &&
           d.target == o.target && d.path == o.path;
  };
  if (flow.branches.size() != old_branches.size()) {
    ValidatorFail("branch count differs from old discovery");
  }
  std::vector<bool> dr_branch_used(flow.branches.size(), false);
  for (const OldBranchRef &old : old_branches) {
    bool matched = false;
    for (size_t i = 0u; i < flow.branches.size(); ++i) {
      if (!dr_branch_used[i] && branch_eq(flow.branches[i], old)) {
        dr_branch_used[i] = true;
        matched = true;
        break;
      }
    }
    if (!matched) {
      ValidatorFail("old branch has no DR-IR counterpart (multiset mismatch)");
    }
  }

  // ------------------------------------------------------ V-OLD-EQUIV(joins) + B
  // Same join SET (join view + section targets), each with exactly ONE minted
  // shared pivot vec (the join-pivots role, sort-unique-at-drain). Keyed by join
  // view identity.
  std::unordered_map<QueryView, const DRJoin *> dr_join;
  for (const DRJoin &j : flow.joins) {
    if (!dr_join.emplace(j.join_view, &j).second) {
      ValidatorFail("V-JOIN-ONE: two DR joins share one join view");
    }
    // The pivot vec exists and carries the join-pivots role + contract.
    if (j.pivot_vec >= flow.vecs.size() ||
        flow.vecs[j.pivot_vec].role != VecRole::kJoinPivots ||
        flow.vecs[j.pivot_vec].uniq != UniqueContract::kSortUniqueAtDrain) {
      ValidatorFail("V-JOIN-ONE: join pivot vec missing or mis-typed");
    }
  }
  if (flow.joins.size() != old_joins.size()) {
    ValidatorFail("join count differs from old discovery");
  }
  for (const OldJoinRef &old : old_joins) {
    auto it = dr_join.find(old.join_view);
    if (it == dr_join.end()) {
      ValidatorFail("old join has no DR-IR counterpart");
    }
    if (it->second->targets != old.targets) {
      ValidatorFail("join section targets differ from old discovery");
    }
  }

  // ---------------------------------------- V-OLD-EQUIV(strata) — B-13 seeding
  // Per-unit stratum equality: every old unit the scheduler ordered has a DR
  // stratum (seeded by `SeedDRStrata`) equal to the old lift's final integer.
  // R1b is bookkeeping equality (the DR side STORES the old integers; the
  // independent deriver is R1c+) — it proves the DR graph CARRIES every unit.
  if (flow.branch_stratum.size() != flow.branches.size()) {
    ValidatorFail("branch_stratum not seeded parallel to branches");
  }
  for (const OldBranchRef &old : old_branches) {
    bool matched = false;
    for (size_t i = 0u; i < flow.branches.size(); ++i) {
      if (branch_eq(flow.branches[i], old) &&
          flow.branch_stratum[i] == old.stratum) {
        matched = true;
        break;
      }
    }
    if (!matched) {
      ValidatorFail("branch stratum differs from old discovery (B-13)");
    }
  }
  for (const OldJoinRef &old : old_joins) {
    auto it = flow.join_stratum.find(old.join_view);
    if (it == flow.join_stratum.end() || it->second != old.stratum) {
      ValidatorFail("join stratum differs from old discovery (B-13)");
    }
  }
  for (const OldCrossoverRef &old : old_crossovers) {
    auto it = flow.crossover_stratum.find(QueryView(old.negate));
    if (it == flow.crossover_stratum.end() || it->second != old.stratum) {
      ValidatorFail("crossover stratum differs from old discovery (B-13)");
    }
  }
  for (const OldProductRef &old : old_products) {
    auto it = flow.product_stratum.find(old.product_view);
    if (it == flow.product_stratum.end() || it->second != old.stratum) {
      ValidatorFail("product stratum differs from old discovery (B-13)");
    }
  }
  if (flow.drain_stratum.size() != old_drain_stratum.size()) {
    ValidatorFail("drain_stratum size differs from old discovery (B-13)");
  }
  for (const auto &[table, stratum] : old_drain_stratum) {
    auto it = flow.drain_stratum.find(table);
    if (it == flow.drain_stratum.end() || it->second != stratum) {
      ValidatorFail("drain stratum differs from old discovery (B-13)");
    }
  }
}

// B-13: STORE the old lift's converged integers into `flow`. Bookkeeping — the
// independent deriver is R1c+. Branch strata are matched to DR branches by
// identity (source, path, ends_at_join, target); note the old DFS may hold
// MULTIPLE branches with the SAME identity (reconvergent plumbing) that carry
// the SAME stratum (the lift keys the drain stratum on the terminal/target, not
// the path), so a per-identity stratum is well-defined and we assign it to
// EVERY matching DR branch slot.
void SeedDRStrata(
    DRFlowGraph &flow,
    const std::vector<OldBranchRef> &old_branches,
    const std::vector<OldJoinRef> &old_joins,
    const std::vector<OldCrossoverRef> &old_crossovers,
    const std::vector<OldProductRef> &old_products,
    const std::unordered_map<TABLE *, unsigned> &old_drain_stratum) {

  const auto branch_eq = [](const DRBranch &d, const OldBranchRef &o) -> bool {
    return d.source == o.source && d.ends_at_join == o.ends_at_join &&
           d.target == o.target && d.path == o.path;
  };

  flow.branch_stratum.assign(flow.branches.size(), 0u);
  std::vector<bool> seeded(flow.branches.size(), false);
  for (const OldBranchRef &old : old_branches) {
    // Assign this old branch's stratum to ONE not-yet-seeded matching DR slot
    // (1:1 pairing over the identity multiset, mirroring the V-OLD-EQUIV mark).
    for (size_t i = 0u; i < flow.branches.size(); ++i) {
      if (!seeded[i] && branch_eq(flow.branches[i], old)) {
        flow.branch_stratum[i] = old.stratum;
        seeded[i] = true;
        break;
      }
    }
  }

  for (const OldJoinRef &old : old_joins) {
    flow.join_stratum[old.join_view] = old.stratum;
  }
  for (const OldCrossoverRef &old : old_crossovers) {
    flow.crossover_stratum[QueryView(old.negate)] = old.stratum;
  }
  for (const OldProductRef &old : old_products) {
    flow.product_stratum[old.product_view] = old.stratum;
  }
  flow.drain_stratum = old_drain_stratum;
}

// ---------------------------------------------------------------------------
// R1c op-family validators (spec §5) + the V-OLD-EQUIV op-inventory census.
// ---------------------------------------------------------------------------
void ValidateDROps(
    const DRFlowGraph &flow, ProgramImpl *impl, Context &context, Query query,
    const std::unordered_map<TABLE *, unsigned> &scc_map) {
  (void) query;

  const auto is_recursive = [&](TABLE *table) -> bool {
    return SccOf(scc_map, table).has_value();
  };
  const auto same_scc = [&](TABLE *a, TABLE *b) -> bool {
    const auto sa = SccOf(scc_map, a);
    return sa.has_value() && sa == SccOf(scc_map, b);
  };
  const auto all_sides_same_scc = [&](QueryView join_view) -> bool {
    DataModel *const jm = impl->view_to_model[join_view]->FindAs<DataModel>();
    const auto join_scc = SccOf(scc_map, jm->table);
    if (!join_scc.has_value()) {
      return false;
    }
    for (QueryView side : QueryJoin::From(join_view).JoinedViews()) {
      TABLE *const st = impl->view_to_model[side]->FindAs<DataModel>()->table;
      if (SccOf(scc_map, st) != join_scc) {
        return false;
      }
    }
    return true;
  };

  // Count the Fold leaves reachable down a plan spine (V-ONE-FOLD helper).
  const std::function<unsigned(const PlanNode *)> count_folds =
      [&](const PlanNode *n) -> unsigned {
    if (!n) {
      return 0u;
    }
    return (n->kind == PlanKind::kFold ? 1u : 0u) + count_folds(n->child.get());
  };

  // V-NEG-CTX at SUB-OBJECT granularity: negate gates ride the fold plan spines
  // as kGate nodes (see the representation note in the derivation helpers).
  // Every gate's pred must be CONTEXT-derived (seed→kInI, fixpoint→kInNew),
  // polarity ABSENT — never sign-derived. Because the pred is a function of the
  // node's ctx alone, a sign-keyed read (the F18 bug shape) cannot validate.
  const std::function<void(const PlanNode *)> check_gates =
      [&](const PlanNode *n) {
        if (!n) {
          return;
        }
        if (n->kind == PlanKind::kGate) {
          if (!n->absent) {
            ValidatorFail("V-NEG-CTX: plan-spine negate gate not polarity=ABSENT");
          }
          const Pred want =
              (n->ctx == Ctx::kSeed) ? Pred::kInI
              : (n->ctx == Ctx::kFixpoint) ? Pred::kInNew
                                           : Pred::kInI;  // eager-normal
          if (n->pred != want) {
            ValidatorFail("V-NEG-CTX: plan-spine negate gate pred not "
                          "context-derived");
          }
        }
        check_gates(n->child.get());
      };
  // Count the distinct table boundaries a plan spine's ACCESS nodes span
  // (V-ONE-FOLD "no plan tree crosses two table boundaries" — here: the fold
  // target plus the access tables it reads through form one walk; the check is
  // that the spine has exactly one Fold, no second Fold under an access).
  const std::function<void(const PlanNode *)> check_one_fold =
      [&](const PlanNode *n) {
        if (!n) {
          return;
        }
        // No Fold may have a child (it is the leaf).
        if (n->kind == PlanKind::kFold && n->child) {
          ValidatorFail("V-ONE-FOLD: a Fold leaf has a child");
        }
        check_one_fold(n->child.get());
      };

  // ------------------------------------------------------- internal validators
  for (const DROp &op : flow.ops) {
    switch (op.kind) {
      case DROpKind::kClaimDrain: {
        // V-CLAIM-GATE (F17): every claim drain carries its gate as DATA.
        const bool is_del = (op.table_op_sign < 0);
        const Pred want = is_del ? Pred::kNetDeleted : Pred::kNetAdded;
        if (op.claim_gate != want) {
          ValidatorFail("V-CLAIM-GATE: claim drain gate not sign-appropriate");
        }
        // V-QCLEAR: an in-round drain clears its accumulating input queue.
        if (op.claim_form == ClaimForm::kInRound) {
          bool has_clear = false;
          for (const DREffect &e : op.effects) {
            if (e.kind == EffKind::kVecClear &&
                e.vec_role ==
                    (is_del ? VecRole::kDeleteQueue : VecRole::kAddQueue)) {
              has_clear = true;
            }
          }
          if (!has_clear) {
            ValidatorFail("V-QCLEAR: in-round claim drain lacks a queue clear");
          }
          // Dual-append: an in-round drain appends to BOTH the persistent set
          // and the per-round frontier (G10).
          bool set_app = false, front_app = false;
          for (const DREffect &e : op.effects) {
            if (e.kind != EffKind::kVecAppend) {
              continue;
            }
            if (e.vec_role ==
                (is_del ? VecRole::kOverdeleteSet : VecRole::kAdditionSet)) {
              set_app = true;
            }
            if (e.vec_role ==
                (is_del ? VecRole::kClaimedDel : VecRole::kClaimedAdd)) {
              front_app = true;
            }
          }
          if (!set_app || !front_app) {
            ValidatorFail("V-QCLEAR: in-round drain missing a dual-append");
          }
        } else {
          // Single-pass drains must NOT clear (negative space).
          for (const DREffect &e : op.effects) {
            if (e.kind == EffKind::kVecClear) {
              ValidatorFail("V-QCLEAR: single-pass claim drain has a queue clear");
            }
          }
        }
        break;
      }
      case DROpKind::kNegateGate: {
        // V-NEG-CTX (F18/F-5): pred DERIVED from (context,hint), never sign.
        const Pred want = (op.ctx == Ctx::kEager)
                              ? (op.gate_hint == NegateHint::kNever ? Pred::kPresent
                                                                    : Pred::kInI)
                              : (op.ctx == Ctx::kSeed ? Pred::kInI : Pred::kInNew);
        if (op.gate_pred != want) {
          ValidatorFail("V-NEG-CTX: negate gate pred not context-derived");
        }
        if (op.ctx == Ctx::kSeed && op.gate_pred == Pred::kInNew) {
          ValidatorFail("V-NEG-CTX: seed-context negate reads InNew (must be InI)");
        }
        if (op.gate_hint == NegateHint::kNever && op.ctx != Ctx::kEager) {
          ValidatorFail("V-NEG-CTX: @never negate outside eager context");
        }
        break;
      }
      case DROpKind::kRederive: {
        // The REDERIVE gate is kRecursivelySupported (mandatory data).
        bool has_gate = false;
        for (const DREffect &e : op.effects) {
          if (e.kind == EffKind::kFlagRead &&
              e.pred == Pred::kRecursivelySupported) {
            has_gate = true;
          }
        }
        if (!has_gate) {
          ValidatorFail("V-CLAIM-GATE: rederive lacks kRecursivelySupported gate");
        }
        break;
      }
      case DROpKind::kFrontierFilter: {
        // V-DEFER: an SCC-table filter is add-loop-output (BOTH signs); a
        // non-recursive table's filter is immediate.
        const bool scc = is_recursive(op.table_op_table);
        if (scc && op.deferral != Deferral::kAddLoopOutput) {
          ValidatorFail("V-DEFER: SCC-table filter is not deferred");
        }
        if (!scc && op.deferral != Deferral::kImmediate) {
          ValidatorFail("V-DEFER: non-recursive filter is deferred");
        }
        break;
      }
      case DROpKind::kSeedFold:
      case DROpKind::kChainFold: {
        // V-ONE-FOLD: exactly one Fold leaf per arm's plan tree (a join-pivot
        // seed carries no fold — it appends pivots).
        for (const DRArm &arm : op.arms) {
          const unsigned folds = count_folds(arm.body.get());
          const unsigned want = (op.kind == DROpKind::kSeedFold && op.join_pivot)
                                    ? 0u
                                    : 1u;
          if (folds != want) {
            ValidatorFail("V-ONE-FOLD: seed/chain fold arm has wrong fold count");
          }
          check_one_fold(arm.body.get());
          check_gates(arm.body.get());
        }
        break;
      }
      case DROpKind::kFixpointFire: {
        // V-ONE-FOLD per arm; V-RETIRE-AFTER structural companion is checked
        // below (retire ops exist per SCC per sign).
        for (const DRArm &arm : op.arms) {
          if (count_folds(arm.body.get()) != 1u) {
            ValidatorFail("V-ONE-FOLD: fixpoint fire arm has wrong fold count");
          }
          check_one_fold(arm.body.get());
          check_gates(arm.body.get());
        }
        break;
      }
      default:
        break;
    }
  }

  // V-RETIRE-AFTER (structural, ARM granularity): for every SCC (group) that has
  // any FIXPOINT_FIRE, there exist RETIRE ops for that group per sign. The full
  // ordering (retire strictly after same-round fires) lands with R1d's dep
  // edges; here we assert the retire ops EXIST per claimed frontier per round.
  {
    std::unordered_set<unsigned> fire_groups;  // SCC groups with a fixpoint fire
    for (const DROp &op : flow.ops) {
      if (op.kind == DROpKind::kFixpointFire) {
        TABLE *const t = op.fire_table;
        if (auto g = SccOf(scc_map, t); g.has_value()) {
          fire_groups.insert(*g);
        }
      }
      // Chain folds also fire in a round; their group must retire too.
      if (op.kind == DROpKind::kChainFold && op.scc_group != ~0u) {
        fire_groups.insert(op.scc_group);
      }
    }
    // Every SCC table's group has both-sign retires (minted per SCC table).
    std::unordered_map<unsigned, unsigned> retire_signs;  // group → bitmask
    for (const DROp &op : flow.ops) {
      if (op.kind == DROpKind::kRetire && op.scc_group != ~0u) {
        retire_signs[op.scc_group] |= (op.table_op_sign < 0) ? 1u : 2u;
      }
    }
    for (unsigned g : fire_groups) {
      auto it = retire_signs.find(g);
      if (it == retire_signs.end() || it->second != 3u) {
        ValidatorFail("V-RETIRE-AFTER: SCC with fires lacks both-sign retires");
      }
    }
  }

  // ============================ V-OLD-EQUIV op-inventory census ============
  // Recompute the EXPECTED op counts by replicating the emission driver's rules
  // from the same discovery inputs, then compare per-kind counts + keys.
  //
  // We re-derive the branch/join inventory the same way BuildDRInventory did
  // (the memoized worklist), so the census is a pure function of impl/context/
  // query/scc_map — an INDEPENDENT recount, not a copy of `flow`'s ops.

  // Recompute the branch inventory (mirrors BuildDRInventory's branch block).
  std::vector<DRBranch> exp_branches;
  for (TABLE *table : impl->tables) {
    bool is_source = TableIsDifferential(table);
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
    BranchMemo memo;
    for (const QueryView &member : table->views) {
      if (member.IsInsert() || member.Columns().size() != num_cols) {
        continue;
      }
      for (const BranchSuffix &bs :
           SuffixesOf(impl, context, table, member, memo)) {
        DRBranch branch;
        branch.source = table;
        branch.path.push_back(member);
        branch.path.insert(branch.path.end(), bs.suffix.begin(),
                           bs.suffix.end());
        branch.ends_at_join = bs.ends_at_join;
        branch.target = bs.target;
        exp_branches.push_back(std::move(branch));
      }
    }
  }

  // Expected SEED_FOLD + CHAIN_FOLD counts (mirror Stratum.cpp:2012-2080).
  unsigned exp_seed = 0u, exp_chain = 0u;
  for (const DRBranch &branch : exp_branches) {
    TABLE *const source = branch.source;
    const bool src_seedable =
        TableIsDifferential(source) && !TableIsInductionOwnedDR(context, source);
    if (!branch.ends_at_join && is_recursive(branch.target) &&
        same_scc(branch.target, source)) {
      if (src_seedable) {
        exp_chain += 2u;  // both signs, in-round
      }
      continue;
    }
    if (branch.ends_at_join && all_sides_same_scc(branch.path.back())) {
      continue;  // suppressed (V-SEED-SUP)
    }
    exp_seed += src_seedable ? 2u : 1u;
  }

  // Expected FIXPOINT_FIRE count: SCC joins × 2 signs (deduped by join view).
  unsigned exp_fire = 0u;
  {
    std::unordered_set<QueryView> seen_join;
    for (const DRBranch &branch : exp_branches) {
      if (!branch.ends_at_join) {
        continue;
      }
      const QueryView jv = branch.path.back();
      if (!seen_join.insert(jv).second) {
        continue;
      }
      DataModel *const jm = impl->view_to_model[jv]->FindAs<DataModel>();
      if (is_recursive(jm->table)) {
        exp_fire += 2u;
      }
    }
  }

  // Expected per-table drains/retires/rederives/filters (mirror the phase band).
  unsigned exp_claim = 0u, exp_retire = 0u, exp_rederive = 0u, exp_filter = 0u;
  for (TABLE *table : impl->tables) {
    if (!TableIsDifferential(table) || TableIsInductionOwnedDR(context, table)) {
      continue;
    }
    if (is_recursive(table)) {
      exp_claim += 2u;     // in-round del + add
      exp_retire += 2u;    // del + add
      exp_rederive += 1u;  // del side
      exp_filter += 2u;    // deferred del + add
    } else {
      exp_claim += 2u;   // single-pass del + add
      exp_filter += 2u;  // immediate del + add
    }
  }

  // Expected COMMIT_SWEEP count: differential tables + monotone boundary tables.
  unsigned exp_sweep = 0u;
  for (TABLE *table : impl->tables) {
    if (TableIsDifferential(table)) {
      ++exp_sweep;
    } else if (context.table_delta_vecs.find(table) !=
               context.table_delta_vecs.end()) {
      ++exp_sweep;
    }
  }

  // Compare against the DERIVED op inventory.
  const auto count_kind = [&](DROpKind k) -> unsigned {
    unsigned n = 0u;
    for (const DROp &op : flow.ops) {
      if (op.kind == k) {
        ++n;
      }
    }
    return n;
  };
  const auto expect = [&](DROpKind k, unsigned want, const char *what) {
    if (count_kind(k) != want) {
      std::fprintf(stderr,
                   "error: DR-IR op census mismatch (%s): derived %u, "
                   "emitter-expected %u\n",
                   what, count_kind(k), want);
      std::abort();
    }
  };
  expect(DROpKind::kSeedFold, exp_seed, "seed folds");
  expect(DROpKind::kChainFold, exp_chain, "chain folds");
  expect(DROpKind::kFixpointFire, exp_fire, "fixpoint fires");
  expect(DROpKind::kClaimDrain, exp_claim, "claim drains");
  expect(DROpKind::kRetire, exp_retire, "retires");
  expect(DROpKind::kRederive, exp_rederive, "rederives");
  expect(DROpKind::kFrontierFilter, exp_filter, "frontier filters");
  expect(DROpKind::kCommitSweep, exp_sweep, "commit sweeps");

  // V-SEED-SUP negative space: no SEED_FOLD folds into an all-same-SCC join's
  // table (the tc G1 bug). POSITIVE: an all-same-SCC join has ≥1 fixpoint fire.
  for (const DROp &op : flow.ops) {
    if (op.kind != DROpKind::kSeedFold || op.join_pivot) {
      continue;
    }
    // A seed fold whose target is an SCC table reached from a same-SCC source
    // would be the tc G1 bug (a wrongly-emitted same-SCC seed).
    if (op.seed_target && is_recursive(op.seed_target) && op.seed_source &&
        same_scc(op.seed_target, op.seed_source)) {
      ValidatorFail("V-SEED-SUP: a same-SCC seed fold exists (should be chain)");
    }
  }
}

}  // namespace hyde
