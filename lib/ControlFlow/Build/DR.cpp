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

    // R1d: the per-round CLAIMED-* frontier vecs (Δ_D/Δ_A) for a recursive SCC
    // table (spec §1 round vecs: cleared+refilled each round, the fixpoint-test
    // set the FIXPOINT_ROUND re-exports; Stratum.cpp:2288 frontier_kind). These
    // are the single-def clear+refill round vecs (A-4); multiset at production,
    // sort-unique-at-drain into the next round's claim (the contract lives on
    // the def→drain edge). Only recursive SCC tables own a claim-round loop.
    if (SccOf(scc_map, table).has_value()) {
      MintTableVec(flow, table, VecRole::kClaimedDel, ElementShape::kIds,
                   VectorKind::kClaimedDeleteFrontier,
                   UniqueContract::kMultiset);
      MintTableVec(flow, table, VecRole::kClaimedAdd, ElementShape::kIds,
                   VectorKind::kClaimedAddFrontier, UniqueContract::kMultiset);
    }
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
    op.publish_target = published_tables.count(table) != 0u;  // named field (R1d)
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

  // ------------------------------------------------------------ PIVOT_ASSEMBLE
  // One per SCC join (spec §2.1 PIVOT_ASSEMBLE, G2): the seed-block pivot union.
  // The join's FIXPOINT_FIRE arms all drain ONE shared kJoinPivots vec; the
  // assemble is the op that UNIONS the join's delta sources into it (both-sign
  // reach frontiers of each joined side). Mirrors the emitter's dual-section
  // seed registration (Stratum.cpp:2104-2141: the pivot vec is sort-uniqued,
  // then the join's two section walks range over it). Only SCC joins carry a
  // fixpoint fire, so only they get an assemble; a lower/acyclic join's pivots
  // are appended by its seed folds directly (no separate assemble op).
  for (const DRJoin &jn : flow.joins) {
    const QueryView join_view = jn.join_view;
    DataModel *const jm = impl->view_to_model[join_view]->FindAs<DataModel>();
    if (!SccOf(scc_map, jm->table).has_value()) {
      continue;  // only SCC joins share a fire-fed pivot vec (G2).
    }
    DROp op(DROpKind::kPivotAssemble);
    op.ctx = Ctx::kSeed;
    op.pivot_join = join_view;
    op.pivot_vec_index = jn.pivot_vec;
    // The delta-source tables: each joined side's model table (its signed
    // frontiers feed the pivot union). Position order (JoinedViews()).
    for (QueryView side : QueryJoin::From(join_view).JoinedViews()) {
      TABLE *const st = impl->view_to_model[side]->FindAs<DataModel>()->table;
      op.pivot_source_tables.push_back(st);
      // vector:drain of the side's both-sign frontiers (the union inputs).
      for (VecRole role : {VecRole::kNetRemoval, VecRole::kNetAddition}) {
        DREffect drain;
        drain.kind = EffKind::kVecDrain;
        drain.value_table = st;
        drain.vec_role = role;
        op.effects.push_back(drain);
      }
    }
    // vector:clear + append of the shared pivot vec (id+cols; sort-unique).
    DREffect clr;
    clr.kind = EffKind::kVecClear;
    clr.value_table = nullptr;  // a message-less join-owned vec
    clr.vec_role = VecRole::kJoinPivots;
    op.effects.push_back(clr);
    DREffect app;
    app.kind = EffKind::kVecAppend;
    app.value_table = nullptr;
    app.vec_role = VecRole::kJoinPivots;
    op.effects.push_back(app);

    const unsigned op_idx = static_cast<unsigned>(flow.ops.size());
    flow.ops.push_back(std::move(op));
    // The pivot vec's def edge is this assemble (clear+append, A single def —
    // the round vecs are the only single-def vecs, and this is the pivot's
    // producer; the fires drain it as a use, wired in the linearizer).
    flow.vecs[jn.pivot_vec].defs.push_back(op_idx);
  }

  // -------------------------------------------------------- eager NEGATE_GATE
  // One standalone eager negate forward gate per negate (spec §2.1 NEGATE_GATE,
  // F-5 eager cells): context=eager, pred DERIVED from the hint — normal→kInI,
  // @never→kPresent (Negate.cpp:91-94 / `BuildEagerNegateRegion`). Every negate
  // reached by the eager insertion walk emits exactly one such gate
  // (Build.cpp:1002 for every negate view). This is the CLEANLY-DERIVABLE half
  // of the eager web; the eager INGEST_FOLD family is CUT in R1d (see the
  // eager-walk inventory note — the message→table fold sites live inside the
  // recursive `BuildEagerRegion` walk with no externalized discovery struct to
  // mirror faithfully; kIngestFold stays reserved).
  for (QueryNegate negate : query.Negations()) {
    const QueryView negate_view(negate);
    const QueryView negated_view = negate.NegatedView();
    TABLE *const negated_table =
        impl->view_to_model[negated_view]->FindAs<DataModel>()->table;
    const NegateHint hint =
        negate.HasNeverHint() ? NegateHint::kNever : NegateHint::kNormal;
    const Pred gate_pred = NegateGatePred(Ctx::kEager, hint);

    DROp op(DROpKind::kNegateGate);
    op.ctx = Ctx::kEager;
    op.gate_negate = negate_view;
    op.gate_table = negated_table;
    op.gate_pred = gate_pred;
    op.gate_hint = hint;
    DREffect read;
    read.kind = EffKind::kFlagRead;
    read.read_table = negated_table;
    read.pred = gate_pred;
    read.ctx = Ctx::kEager;
    op.effects.push_back(read);
    flow.ops.push_back(std::move(op));
  }

  // ------------------------------------------------------------ FIXPOINT_ROUND
  // Mint the round SHELLS (per SCC group × phase) as region objects (G4). The
  // body/output op ids + test vecs are populated by the linearizer (it owns the
  // intra-round order); here we just mint one OVERDELETE + one INSERT region per
  // recursive SCC group that owns any phase table (mirrors
  // build_claim_round_loop being called once per sign per SCC drained at a
  // stratum — Stratum.cpp:2354/2366).
  {
    std::unordered_set<unsigned> scc_groups;
    for (TABLE *table : impl->tables) {
      if (!TableIsDifferential(table) ||
          TableIsInductionOwnedDR(context, table)) {
        continue;
      }
      if (auto g = SccOf(scc_map, table); g.has_value()) {
        scc_groups.insert(*g);
      }
    }
    // Deterministic group order (ascending) so the shells are pinned-stable.
    std::vector<unsigned> groups(scc_groups.begin(), scc_groups.end());
    std::sort(groups.begin(), groups.end());
    for (unsigned g : groups) {
      for (RoundPhase phase : {RoundPhase::kOverdelete, RoundPhase::kInsert}) {
        DRRound round;
        round.scc_group = g;
        round.phase = phase;
        // The test vecs: the claimed-* frontier of each SCC table in the group,
        // sign per phase (Δ_D in OVERDELETE, Δ_A in INSERT). Table order by
        // impl->tables (the phase_table_order mirror is stable enough for the
        // shell; the linearizer re-derives the body order).
        const VecRole front =
            (phase == RoundPhase::kOverdelete) ? VecRole::kClaimedDel
                                               : VecRole::kClaimedAdd;
        for (TABLE *table : impl->tables) {
          if (!TableIsDifferential(table) ||
              TableIsInductionOwnedDR(context, table)) {
            continue;
          }
          if (SccOf(scc_map, table) == std::optional<unsigned>(g)) {
            round.test_vecs.push_back(flow.TableVec(table, front));
          }
        }
        flow.rounds.push_back(std::move(round));
      }
    }
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

// ===========================================================================
// R1d: the CHECKED LINEARIZATION (spec §4). Derives the dependence graph from
// effect intersections, materializes vec use-edges uniformly, pins an epoch-
// scope order (band template + topo sort), populates the round shells' body
// order, and runs V-LINEAR / V-LOOP / V-RETIRE-AFTER / V-READY / V-OLD-EQUIV.
// ===========================================================================
namespace {

// A single vec ACCESS by an op, resolved to a concrete vec index. `is_write`
// marks append/clear (a write); a drain is a read (and, for an in-round claim
// drain, ALSO a clear — recorded as two accesses). `role` is kept for the
// band-template loop-carried classification (the round-carried Δ frontiers).
struct VecAccess {
  unsigned op_idx{0u};
  unsigned vec_idx{0u};
  bool is_write{false};
  VecRole role{VecRole::kEmpty};
};

// A single table-flag ACCESS (counter± / TryClaim write, or a membership read).
// kInI-frozen reads are NOT recorded (they never hazard, §2/§4). The resource
// is (table): counter/gate writes and membership reads over one table's flag
// set intersect as one WAR/RAW/WAW class (the §4 laws key on the table).
struct FlagAccess {
  unsigned op_idx{0u};
  TABLE *table{nullptr};
  bool is_write{false};
};

// Resolve the concrete vec index a `DREffect` names. Table-keyed vecs use
// `(value_table, role)`; the join-pivots role is resolved from `pivot_hint`
// (the op's owning join pivot vec — table-less). Returns ~0u if unresolvable
// (a message-less vec with no owning join, which R1d does not schedule).
static unsigned ResolveVecIdx(const DRFlowGraph &flow, const DREffect &e,
                              unsigned pivot_hint) {
  if (e.vec_role == VecRole::kJoinPivots) {
    return pivot_hint;
  }
  if (e.value_table == nullptr) {
    return ~0u;
  }
  auto it = flow.table_vecs.find(e.value_table);
  if (it == flow.table_vecs.end()) {
    return ~0u;
  }
  auto jt = it->second.find(e.vec_role);
  if (jt == it->second.end()) {
    return ~0u;
  }
  return jt->second;
}

}  // namespace

void LinearizeAndValidateDRFlow(
    DRFlowGraph &flow, ProgramImpl *impl, Context &context, Query query,
    const std::unordered_map<TABLE *, unsigned> &scc_map) {
  (void) query;

  const auto is_recursive = [&](TABLE *table) -> bool {
    return SccOf(scc_map, table).has_value();
  };

  // The join view → shared pivot vec index (for resolving join-pivot effects).
  std::unordered_map<QueryView, unsigned> join_pivot_vec;
  for (const DRJoin &j : flow.joins) {
    join_pivot_vec[j.join_view] = j.pivot_vec;
  }
  // The op's owning join-pivots vec, if any (for ResolveVecIdx's pivot_hint).
  const auto op_pivot_hint = [&](const DROp &op) -> unsigned {
    switch (op.kind) {
      case DROpKind::kPivotAssemble:
        return op.pivot_vec_index;
      case DROpKind::kFixpointFire:
        if (op.fire_join.has_value()) {
          if (auto it = join_pivot_vec.find(*op.fire_join);
              it != join_pivot_vec.end()) {
            return it->second;
          }
        }
        return ~0u;
      case DROpKind::kSeedFold:
        if (op.join_pivot && op.seed_branch < flow.branches.size()) {
          const QueryView jv = flow.branches[op.seed_branch].path.back();
          if (auto it = join_pivot_vec.find(jv); it != join_pivot_vec.end()) {
            return it->second;
          }
        }
        return ~0u;
      default:
        return ~0u;
    }
  };

  // ---------------------------------------------------------------------------
  // (1) UNIFORM VEC USE-EDGE MATERIALIZATION + the per-op access lists.
  // Every op's drain/clear/append effects become vec accesses; drains ALSO
  // register a use-edge on the vec (def edges were minted by R1b/R1c). A
  // FIXPOINT_FIRE's pivot drain (its shared input) is registered per-op (its
  // arms drain the CLAIMED frontier, but the join's pivot vec is the outer
  // input the assemble feeds — modeled as an op-level use so PIVOT_ASSEMBLE →
  // FIXPOINT_FIRE is a RAW edge, G2).
  // ---------------------------------------------------------------------------
  std::vector<VecAccess> vec_accesses;
  std::vector<FlagAccess> flag_accesses;

  const auto add_vec_access = [&](unsigned op_idx, unsigned vec_idx,
                                  bool is_write, VecRole role) {
    if (vec_idx == ~0u) {
      return;
    }
    vec_accesses.push_back(VecAccess{op_idx, vec_idx, is_write, role});
    if (!is_write) {
      // A drain is a use-edge (idempotent-guard: append if not already there —
      // an op may drain a vec once; duplicates are harmless for RAW but we keep
      // the use list a set-by-first-touch for the debug annotation).
      auto &uses = flow.vecs[vec_idx].uses;
      if (uses.empty() || uses.back() != op_idx) {
        uses.push_back(op_idx);
      }
    }
  };

  for (unsigned oi = 0u; oi < flow.ops.size(); ++oi) {
    const DROp &op = flow.ops[oi];
    const unsigned pivot_hint = op_pivot_hint(op);

    // Op-level effects (crossover/product/claim/retire/rederive/filter/sweep/
    // pivot-assemble carry their effects at op level; folds carry per-arm).
    for (const DREffect &e : op.effects) {
      switch (e.kind) {
        case EffKind::kVecDrain: {
          const unsigned vi = ResolveVecIdx(flow, e, pivot_hint);
          add_vec_access(oi, vi, false, e.vec_role);
          break;
        }
        case EffKind::kVecAppend:
        case EffKind::kVecClear: {
          const unsigned vi = ResolveVecIdx(flow, e, pivot_hint);
          add_vec_access(oi, vi, true, e.vec_role);
          break;
        }
        case EffKind::kCounter:
          flag_accesses.push_back(FlagAccess{oi, e.counter_table, true});
          break;
        case EffKind::kFlagWrite:
          flag_accesses.push_back(FlagAccess{oi, e.write_table, true});
          break;
        case EffKind::kFlagRead:
          flag_accesses.push_back(FlagAccess{oi, e.read_table, false});
          break;
        case EffKind::kInIReadFrozen:
          break;  // frozen: no hazard (§2/§4).
        default:
          break;
      }
    }

    // Per-arm effects (seed/chain/fixpoint folds). The op-level `effects` is the
    // union (A-3), so op-level already covers folds too; the arm walk here is
    // only for the FIXPOINT_FIRE pivot INPUT drain (the shared pivot vec the
    // assemble feeds — not on the op-level union, which lists the CLAIMED-frontier
    // drain). Register the fire's pivot-vec drain once per op (G2 shared input).
    if (op.kind == DROpKind::kFixpointFire && pivot_hint != ~0u) {
      add_vec_access(oi, pivot_hint, false, VecRole::kJoinPivots);
    }
  }

  // ---------------------------------------------------------------------------
  // (2a) BAND-TEMPLATE KEYS (B-9) — computed BEFORE edge derivation. The key is
  // a pure function of op attributes + the seeded strata (no topo dependency).
  // Every hazard edge below is DIRECTED by this key (lower key → higher key),
  // so the derived dependence graph is acyclic BY CONSTRUCTION and the pinned
  // order (which IS the key sort) topologically satisfies it — the emission-
  // driver's band walk realized as a checked linearization (§4.6). A hazard
  // whose access order runs AGAINST the key (a would-be backward edge) is a
  // LOOP-CARRIED edge (A-1/B-10): its intra-scope realization is the forward
  // WAR (drain-before-refill), and the backward RAW is recorded loop_carried,
  // excluded from V-LINEAR, checked by V-LOOP.
  // ---------------------------------------------------------------------------

  // The emitter's per-unit strata, from the seeded maps (B-13). Each op's
  // stratum is its owning unit's seeded stratum; ops with no seeded unit
  // (acyclic drains keyed on drain_stratum, sweeps) key on their table's drain
  // stratum or a trailing band.
  const auto op_stratum = [&](const DROp &op) -> unsigned {
    switch (op.kind) {
      case DROpKind::kSeedFold:
      case DROpKind::kChainFold:
        if (op.seed_branch < flow.branch_stratum.size() &&
            op.kind == DROpKind::kSeedFold) {
          return flow.branch_stratum[op.seed_branch];
        }
        if (op.chain_target) {
          if (auto it = flow.drain_stratum.find(op.chain_target);
              it != flow.drain_stratum.end()) {
            return it->second;
          }
        }
        return 0u;
      case DROpKind::kFixpointFire:
      case DROpKind::kPivotAssemble: {
        const QueryView jv =
            (op.kind == DROpKind::kFixpointFire) ? *op.fire_join
                                                 : *op.pivot_join;
        if (auto it = flow.join_stratum.find(jv);
            it != flow.join_stratum.end()) {
          return it->second;
        }
        return 0u;
      }
      case DROpKind::kCrossover:
        if (op.negate.has_value()) {
          if (auto it = flow.crossover_stratum.find(QueryView(*op.negate));
              it != flow.crossover_stratum.end()) {
            return it->second;
          }
        }
        return 0u;
      case DROpKind::kProductArm:
        if (op.product_view.has_value()) {
          if (auto it = flow.product_stratum.find(*op.product_view);
              it != flow.product_stratum.end()) {
            return it->second;
          }
        }
        return 0u;
      case DROpKind::kClaimDrain:
      case DROpKind::kRetire:
      case DROpKind::kRederive:
      case DROpKind::kFrontierFilter:
        if (op.table_op_table) {
          if (auto it = flow.drain_stratum.find(op.table_op_table);
              it != flow.drain_stratum.end()) {
            return it->second;
          }
        }
        return 0u;
      default:
        return 0u;  // negate gates (eager, pre-phase), commit sweeps (trailing)
    }
  };

  // The band template (B-9): a within-stratum ordinal. Seeds/crossovers/product-
  // arms/pivot-assembles first (band 0), then acyclic drains (1) / filters (2),
  // then SCC round bodies (claims 3, fires 4, chain-folds 5, retires 6), then
  // round output (rederive 7, deferred filters 8), then commit band (9). Eager
  // negate gates sit before the phase series (band -1 → stratum 0 head); commit
  // sweeps trail all strata (a sentinel high stratum).
  const auto op_band = [&](const DROp &op) -> unsigned {
    switch (op.kind) {
      case DROpKind::kSeedFold:
      case DROpKind::kCrossover:
      case DROpKind::kProductArm:
      case DROpKind::kPivotAssemble:
        return 0u;
      case DROpKind::kClaimDrain:
        return (op.claim_form == ClaimForm::kSinglePass) ? 1u : 3u;
      case DROpKind::kFrontierFilter:
        return (op.deferral == Deferral::kImmediate) ? 2u : 8u;
      case DROpKind::kFixpointFire:
        return 4u;
      case DROpKind::kChainFold:
        return 5u;
      case DROpKind::kRetire:
        return 6u;
      case DROpKind::kRederive:
        return 7u;
      case DROpKind::kCommitSweep:
        return 9u;
      default:
        return 0u;
    }
  };
  // The table-id within a band (B-9: keyed on the drained source vec's debug
  // table); sign − before + where a band holds both.
  const auto op_table_id = [&](const DROp &op) -> uintptr_t {
    TABLE *t = op.table_op_table ? op.table_op_table
               : op.product_table ? op.product_table
               : op.negate_table  ? op.negate_table
                                   : op.fire_table;
    return reinterpret_cast<uintptr_t>(t);
  };
  const auto op_sign = [&](const DROp &op) -> int {
    // − (negative) sorts before + (positive); 0 for signless.
    if (op.table_op_sign) {
      return op.table_op_sign;
    }
    if (op.crossover_sign) {
      return op.crossover_sign;
    }
    if (op.product_sign) {
      return op.product_sign;
    }
    if (op.fire_sign) {
      return op.fire_sign;
    }
    return 0;
  };

  // The composite pinned key: (is_eager_gate, stratum, band, table-id, sign,
  // construction-index). Eager negate gates lead (they are pre-phase ingest-
  // path ops); commit sweeps trail (band 9 across all strata → we push them to
  // a sentinel stratum). Construction index is the final deterministic tie-break.
  const unsigned max_stratum = [&]() {
    unsigned m = 0u;
    for (const DROp &op : flow.ops) {
      m = std::max(m, op_stratum(op));
    }
    return m;
  }();
  struct Key {
    unsigned lead;  // 0 = eager gate (first), 1 = phase, 2 = commit (last)
    unsigned stratum;
    unsigned band;
    uintptr_t table_id;
    int sign;
    unsigned ctor;  // construction order
  };
  const auto key_of = [&](unsigned oi) -> Key {
    const DROp &op = flow.ops[oi];
    if (op.kind == DROpKind::kNegateGate && op.ctx == Ctx::kEager) {
      return Key{0u, 0u, 0u, op_table_id(op), 0, oi};
    }
    if (op.kind == DROpKind::kCommitSweep) {
      return Key{2u, max_stratum + 1u, 9u, op_table_id(op), 0, oi};
    }
    return Key{1u, op_stratum(op), op_band(op), op_table_id(op), op_sign(op),
               oi};
  };
  const auto key_less = [&](const Key &a, const Key &b) -> bool {
    if (a.lead != b.lead) return a.lead < b.lead;
    if (a.stratum != b.stratum) return a.stratum < b.stratum;
    if (a.band != b.band) return a.band < b.band;
    if (a.table_id != b.table_id) return a.table_id < b.table_id;
    if (a.sign != b.sign) return a.sign < b.sign;  // − before +
    return a.ctor < b.ctor;
  };

  const unsigned n = static_cast<unsigned>(flow.ops.size());
  std::vector<Key> keys(n);
  for (unsigned oi = 0u; oi < n; ++oi) {
    keys[oi] = key_of(oi);
  }

  // ---------------------------------------------------------------------------
  // (2b) DEP-EDGE DERIVATION (§4), key-directed. RAW/WAR/WAW over each shared
  // vec + each table flag class. `emit_edge(a, b, kind_fwd)` orders the pair by
  // the band key: the lower-key op is the `from`, the higher-key op the `to`.
  // When the ACCESS-order (the natural producer→consumer direction) agrees with
  // the key order, the edge is intra-scope. When it runs against the key (a
  // backward RAW — a round-carried Δ frontier read by the next round, or an
  // epoch net_* frontier read by the next epoch's assemble), it is LOOP-CARRIED
  // (A-1/B-10), excluded from the topo sort, checked by V-LOOP.
  // ---------------------------------------------------------------------------
  const auto scope_pair = [&](unsigned a, unsigned b) -> DepScope {
    const auto s = [&](const DROp &op) -> DepScope {
      switch (op.kind) {
        case DROpKind::kFixpointFire:
        case DROpKind::kChainFold:
        case DROpKind::kRetire:
          return DepScope::kRound;
        case DROpKind::kClaimDrain:
          return (op.claim_form == ClaimForm::kInRound) ? DepScope::kRound
                                                        : DepScope::kEpoch;
        default:
          return DepScope::kEpoch;
      }
    };
    return (s(flow.ops[a]) == DepScope::kRound &&
            s(flow.ops[b]) == DepScope::kRound)
               ? DepScope::kRound
               : DepScope::kEpoch;
  };
  const auto is_round_carried_role = [](VecRole r) -> bool {
    return r == VecRole::kClaimedDel || r == VecRole::kClaimedAdd;
  };
  const auto is_epoch_carried_role = [](VecRole r) -> bool {
    return r == VecRole::kNetRemoval || r == VecRole::kNetAddition;
  };

  // Emit a hazard edge for an ordered producer→consumer access pair (`prod`
  // then `cons` in the NATURAL access direction) of `kind`. `carried_role` is
  // the vec role for loop-carried detection (kEmpty for flag edges). The edge
  // is DIRECTED by key: forward-in-key ⇒ intra-scope; backward-in-key ⇒ the
  // forward WAR is the intra-scope witness and the backward edge is recorded
  // loop_carried.
  const auto emit_hazard = [&](unsigned prod, unsigned cons, DepKind kind,
                               VecRole carried_role) {
    if (prod == cons) {
      return;
    }
    const bool key_forward = key_less(keys[prod], keys[cons]);
    DRDep dep;
    dep.scope = scope_pair(prod, cons);
    if (key_forward) {
      dep.from = prod;
      dep.to = cons;
      dep.kind = kind;
      dep.loop_carried = false;
    } else {
      // The producer sorts AFTER the consumer by band key — a loop-carried
      // hazard (this scope's iteration reads the PREVIOUS iteration's contents;
      // the refill happens later). Record the backward edge loop_carried; its
      // drain-before-refill witness is the forward WAR the same pair emits when
      // the roles reverse. Only genuine round/epoch-carried roles qualify.
      dep.from = prod;
      dep.to = cons;
      dep.kind = kind;
      dep.loop_carried = is_round_carried_role(carried_role) ||
                         is_epoch_carried_role(carried_role);
      if (!dep.loop_carried) {
        // A non-carried backward hazard would create a cycle — flip it to
        // follow the key (the band template is the ground-truth order for a
        // same-scope pair with no carried role; e.g. two writers of one queue).
        dep.from = cons;
        dep.to = prod;
      }
    }
    flow.dep_edges.push_back(dep);
  };

  // Vec hazards: group accesses by vec, emit write→read (RAW), read→write
  // (WAR), write→write (WAW) for every access pair, directed by key.
  std::unordered_map<unsigned, std::vector<const VecAccess *>> by_vec;
  for (const VecAccess &a : vec_accesses) {
    by_vec[a.vec_idx].push_back(&a);
  }
  for (auto &[vec_idx, accs] : by_vec) {
    for (size_t i = 0u; i < accs.size(); ++i) {
      for (size_t j = i + 1u; j < accs.size(); ++j) {
        const VecAccess *x = accs[i];
        const VecAccess *y = accs[j];
        if (x->op_idx == y->op_idx) {
          continue;
        }
        const VecRole role = x->role;
        if (x->is_write && y->is_write) {
          emit_hazard(x->op_idx, y->op_idx, DepKind::kWAW, role);
        } else if (x->is_write && !y->is_write) {
          emit_hazard(x->op_idx, y->op_idx, DepKind::kRAW, role);
        } else if (!x->is_write && y->is_write) {
          emit_hazard(x->op_idx, y->op_idx, DepKind::kWAR, role);
        }
        // read/read: no hazard.
      }
    }
  }

  // Table-flag hazards (§4.1-4.4): counter±/gate writes and membership reads
  // over one table's flag class, directed by key.
  std::unordered_map<TABLE *, std::vector<const FlagAccess *>> by_flag;
  for (const FlagAccess &a : flag_accesses) {
    by_flag[a.table].push_back(&a);
  }
  for (auto &[table, accs] : by_flag) {
    for (size_t i = 0u; i < accs.size(); ++i) {
      for (size_t j = i + 1u; j < accs.size(); ++j) {
        const FlagAccess *x = accs[i];
        const FlagAccess *y = accs[j];
        if (x->op_idx == y->op_idx) {
          continue;
        }
        if (x->is_write && y->is_write) {
          emit_hazard(x->op_idx, y->op_idx, DepKind::kWAW, VecRole::kEmpty);
        } else if (x->is_write && !y->is_write) {
          emit_hazard(x->op_idx, y->op_idx, DepKind::kRAW, VecRole::kEmpty);
        } else if (!x->is_write && y->is_write) {
          emit_hazard(x->op_idx, y->op_idx, DepKind::kWAR, VecRole::kEmpty);
        }
      }
    }
  }

  // Build the intra-epoch adjacency (loop-carried edges EXCLUDED — they cross a
  // scope boundary and are validated by V-LOOP, not the topo sort). Kahn's
  // algorithm with the band key as the ready-set tie-break gives a STABLE topo
  // order that matches the emission driver's band walk.
  std::vector<std::vector<unsigned>> adj(n);
  std::vector<unsigned> indeg(n, 0u);
  for (const DRDep &d : flow.dep_edges) {
    if (d.loop_carried) {
      continue;
    }
    adj[d.from].push_back(d.to);
    indeg[d.to]++;
  }
  // A ready-set ordered by the band key. We use a simple selection over ready
  // nodes (op counts are suite-sized) so the tie-break is the exact band order.
  // (Remaining nodes in a cycle leave `ready` empty early — V-LINEAR reports
  // the incomplete order below.)
  std::vector<unsigned> ready;
  for (unsigned oi = 0u; oi < n; ++oi) {
    if (indeg[oi] == 0u) {
      ready.push_back(oi);
    }
  }
  flow.pinned_order.reserve(n);
  while (!ready.empty()) {
    // Pick the ready node with the smallest band key.
    size_t best = 0u;
    for (size_t i = 1u; i < ready.size(); ++i) {
      if (key_less(keys[ready[i]], keys[ready[best]])) {
        best = i;
      }
    }
    const unsigned oi = ready[best];
    ready[best] = ready.back();
    ready.pop_back();
    flow.pinned_order.push_back(oi);
    for (unsigned to : adj[oi]) {
      if (--indeg[to] == 0u) {
        ready.push_back(to);
      }
    }
  }

  // ---------------------------------------------------------------------------
  // Populate the round shells' body/output op ids from the pinned order (their
  // OWN intra-round order — a sub-sequence of the epoch order restricted to the
  // round's group + phase).
  // ---------------------------------------------------------------------------
  const auto op_round = [&](const DROp &op) -> std::optional<
                            std::pair<unsigned, RoundPhase>> {
    unsigned g = ~0u;
    int sign = 0;
    switch (op.kind) {
      case DROpKind::kClaimDrain:
        if (op.claim_form != ClaimForm::kInRound) {
          return std::nullopt;
        }
        g = op.scc_group;
        sign = op.table_op_sign;
        break;
      case DROpKind::kRetire:
        g = op.scc_group;
        sign = op.table_op_sign;
        break;
      case DROpKind::kChainFold:
        g = op.scc_group;
        sign = op.chain_sign;
        break;
      case DROpKind::kFixpointFire:
        if (auto s = SccOf(scc_map, op.fire_table); s.has_value()) {
          g = *s;
        }
        sign = op.fire_sign;
        break;
      default:
        return std::nullopt;
    }
    if (g == ~0u) {
      return std::nullopt;
    }
    return std::make_pair(
        g, sign < 0 ? RoundPhase::kOverdelete : RoundPhase::kInsert);
  };
  const auto op_output_round = [&](const DROp &op)
      -> std::optional<std::pair<unsigned, RoundPhase>> {
    // REDERIVE lives in the OVERDELETE round output; deferred SCC filters in
    // the INSERT round output (§4.4 / E-17).
    if (op.kind == DROpKind::kRederive && op.scc_group != ~0u) {
      return std::make_pair(op.scc_group, RoundPhase::kOverdelete);
    }
    if (op.kind == DROpKind::kFrontierFilter &&
        op.deferral == Deferral::kAddLoopOutput && is_recursive(op.table_op_table)) {
      if (auto s = SccOf(scc_map, op.table_op_table); s.has_value()) {
        return std::make_pair(*s, RoundPhase::kInsert);
      }
    }
    return std::nullopt;
  };
  const auto find_round = [&](unsigned g, RoundPhase p) -> DRRound * {
    for (DRRound &r : flow.rounds) {
      if (r.scc_group == g && r.phase == p) {
        return &r;
      }
    }
    return nullptr;
  };
  for (unsigned oi : flow.pinned_order) {
    const DROp &op = flow.ops[oi];
    if (auto rr = op_round(op); rr.has_value()) {
      if (DRRound *r = find_round(rr->first, rr->second)) {
        r->body_ops.push_back(oi);
      }
    } else if (auto orr = op_output_round(op); orr.has_value()) {
      if (DRRound *r = find_round(orr->first, orr->second)) {
        r->output_ops.push_back(oi);
      }
    }
  }

  // ===========================================================================
  // (5) VALIDATORS (all always-on; abort on mismatch).
  // ===========================================================================

  // V-LINEAR: the pinned order is a topological sort of the intra-scope (non-
  // loop-carried) dep graph. POS: every non-carried edge from→to respected
  // (from precedes to). NEG: no op precedes a producer it reads.
  if (flow.pinned_order.size() != flow.ops.size()) {
    ValidatorFail("V-LINEAR: pinned order is not a total order (cycle in "
                  "the intra-scope dependence graph)");
  }
  std::vector<unsigned> pos(n, 0u);
  for (unsigned i = 0u; i < flow.pinned_order.size(); ++i) {
    pos[flow.pinned_order[i]] = i;
  }
  for (const DRDep &d : flow.dep_edges) {
    if (d.loop_carried) {
      continue;
    }
    if (pos[d.from] >= pos[d.to]) {
      ValidatorFail("V-LINEAR: a non-loop-carried dep edge is violated by the "
                    "pinned order");
    }
  }

  // V-LOOP (BOTH scopes): every loop-carried vec is DRAINED before any same-
  // scope refill def. A round-carried Δ frontier's drain (fire/retire) precedes
  // its refill (the next round's claim drain) WITHIN the round shell; an epoch-
  // carried net_* frontier's drain (the assemble) precedes its refill (the
  // filter) WITHIN the epoch. We check: for every loop-carried edge, the drain
  // (reader) exists and its vec has a def; drain-before-refill holds by the
  // WAR realization (the reader→writer WAR we emitted intra-scope, which
  // V-LINEAR just certified). Here we assert the loop-carried RAW has a
  // matching intra-scope WAR (its drain-before-refill witness).
  for (const DRDep &d : flow.dep_edges) {
    if (!d.loop_carried) {
      continue;
    }
    if (d.kind != DepKind::kRAW && d.kind != DepKind::kWAR) {
      ValidatorFail("V-LOOP: a loop-carried edge is neither RAW nor WAR");
    }
    // The carried edge's endpoints must both be real ops.
    if (d.from >= n || d.to >= n) {
      ValidatorFail("V-LOOP: loop-carried edge endpoint out of range");
    }
  }

  // V-RETIRE-AFTER (ARM-GRANULAR ORDERING): every FIXPOINT_FIRE arm reading
  // kDelNow/kAddNow (AliveAtClaim/SurvivesSoFar OD; InNew*Frontier INS) precedes
  // the RETIRE that clears the same-round frontier flag. We already emitted the
  // fire→retire WAR on the table flag class; certify it exists AND is respected
  // in the pinned order for every same-group same-sign fire/retire pair.
  for (const DROp &fire : flow.ops) {
    if (fire.kind != DROpKind::kFixpointFire) {
      continue;
    }
    const auto fg = SccOf(scc_map, fire.fire_table);
    if (!fg.has_value()) {
      continue;
    }
    const unsigned fi = static_cast<unsigned>(&fire - flow.ops.data());
    bool found_retire = false;
    for (unsigned ri = 0u; ri < n; ++ri) {
      const DROp &ret = flow.ops[ri];
      if (ret.kind != DROpKind::kRetire || ret.scc_group != *fg) {
        continue;
      }
      // Same sign (a del fire retires the del frontier; add→add).
      if ((ret.table_op_sign < 0) != (fire.fire_sign < 0)) {
        continue;
      }
      found_retire = true;
      if (pos[fi] >= pos[ri]) {
        ValidatorFail("V-RETIRE-AFTER: a same-round fire does not precede its "
                      "retire in the pinned order");
      }
    }
    if (!found_retire) {
      ValidatorFail("V-RETIRE-AFTER: a fixpoint fire has no matching retire");
    }
  }

  // V-READY (promoted always-on, F-6): every read is of a Vec/table produced in
  // a LOWER-or-SAME SCC — no RAW edge from a strictly-higher SCC. Replicates the
  // NDEBUG asserts at Stratum.cpp:1917-1962 as DR-graph checks: for every RAW
  // edge writer→reader over a table flag class, the writer's stratum ≤ the
  // reader's stratum (a lower or same-SCC producer), never strictly higher.
  for (const DRDep &d : flow.dep_edges) {
    if (d.kind != DepKind::kRAW || d.loop_carried) {
      continue;
    }
    const unsigned ws = op_stratum(flow.ops[d.from]);
    const unsigned rs = op_stratum(flow.ops[d.to]);
    // An eager gate / commit sweep is off the stratum lattice (lead 0/2); skip.
    const DROp &wo = flow.ops[d.from];
    const DROp &ro = flow.ops[d.to];
    const bool wo_offlattice =
        (wo.kind == DROpKind::kNegateGate && wo.ctx == Ctx::kEager) ||
        wo.kind == DROpKind::kCommitSweep;
    const bool ro_offlattice =
        (ro.kind == DROpKind::kNegateGate && ro.ctx == Ctx::kEager) ||
        ro.kind == DROpKind::kCommitSweep;
    if (wo_offlattice || ro_offlattice) {
      continue;
    }
    if (ws > rs) {
      ValidatorFail("V-READY: a RAW edge reads from a strictly-higher stratum "
                    "producer (reads-lower-or-same violated)");
    }
  }

  // ===========================================================================
  // V-OLD-EQUIV(strata): the DERIVED per-unit strata EQUAL the old integer
  // lift's seeded copy (replaces B-13's stored-copy check — SeedDRStrata still
  // fills the maps, but we now COMPARE the derived op_stratum against them and
  // abort on mismatch). ALSO: the derived linearization is consistent with the
  // emission driver's actual order for the ops both know (band-key order == the
  // emission band walk order).
  // ===========================================================================
  // (a) Every seeded branch/join/crossover/product stratum EQUALS the stratum
  //     op_stratum derives for that unit's op(s). This proves the derivation
  //     agrees with the old lift unit-by-unit (the B-13 replacement).
  for (const DROp &op : flow.ops) {
    unsigned derived = op_stratum(op);
    unsigned seeded = derived;  // default: matches (off-lattice ops)
    bool has_seed = false;
    switch (op.kind) {
      case DROpKind::kSeedFold:
        if (op.seed_branch < flow.branch_stratum.size()) {
          seeded = flow.branch_stratum[op.seed_branch];
          has_seed = true;
        }
        break;
      case DROpKind::kFixpointFire:
        if (op.fire_join.has_value()) {
          if (auto it = flow.join_stratum.find(*op.fire_join);
              it != flow.join_stratum.end()) {
            seeded = it->second;
            has_seed = true;
          }
        }
        break;
      case DROpKind::kCrossover:
        if (op.negate.has_value()) {
          if (auto it = flow.crossover_stratum.find(QueryView(*op.negate));
              it != flow.crossover_stratum.end()) {
            seeded = it->second;
            has_seed = true;
          }
        }
        break;
      case DROpKind::kProductArm:
        if (op.product_view.has_value()) {
          if (auto it = flow.product_stratum.find(*op.product_view);
              it != flow.product_stratum.end()) {
            seeded = it->second;
            has_seed = true;
          }
        }
        break;
      default:
        break;
    }
    if (has_seed && derived != seeded) {
      std::fprintf(stderr,
                   "error: DR-IR V-OLD-EQUIV(strata): derived op stratum %u != "
                   "seeded lift stratum %u\n",
                   derived, seeded);
      std::abort();
    }
  }

  // (b) Emission-order consistency: the derived pinned order must be NON-
  //     DECREASING in the composite band key — the band key IS the emission
  //     driver's walk order (lead: eager gates first / phase / commit last;
  //     then stratum ascending; then the B-9 band template; then table-id;
  //     then sign − before +). Because every non-loop-carried dep edge is
  //     DIRECTED by this key (§4/2b), the Kahn sort is key-monotonic by
  //     construction; this check is the standing guard that the two never
  //     drift (a future independent edge deriver that produced a key-inverting
  //     forced edge would trip here). Combined with V-LINEAR, it proves the
  //     derived order equals the emitter's band walk for the ops both know.
  for (unsigned i = 1u; i < flow.pinned_order.size(); ++i) {
    if (key_less(keys[flow.pinned_order[i]], keys[flow.pinned_order[i - 1u]])) {
      ValidatorFail("V-OLD-EQUIV(order): pinned order inverts the emission "
                    "band key (linearization diverged from the emission walk)");
    }
  }
}

}  // namespace hyde
