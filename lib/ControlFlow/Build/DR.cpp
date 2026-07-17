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
#include <tuple>
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
    // R3 chain-breaker (critique HIGH #4a): an aggregate/KV-index successor is
    // owned by its GROUP_UPDATE op — never a section target reached through the
    // table-less walk (mirror of the `SuffixesOf` skip above).
    if (succ.IsAggregate() || succ.IsKVIndex()) {
      continue;
    }
    if (FollowsDeltaEdgeDR(context, succ)) {
      CollectSectionTargetsDR(impl, context, succ, targets);
    }
  }
}

// Replicate the cut-successor test (Build.cpp:857-858) EXACTLY: a deletion-
// capable / aggregate / KV-index successor is fed by phases or its
// GROUP_UPDATE, never by the eager walk.
static bool AnyCutSuccessorDR(QueryView view) {
  for (QueryView succ : view.Successors()) {
    if (succ.CanReceiveDeletions() || succ.IsAggregate() || succ.IsKVIndex()) {
      return true;
    }
  }
  return false;
}

// P2/R1e: the monotone-boundary net-additions rule at TABLE level (Build.cpp
// :886-893). The walk provisions a monotone table's kNetAdditions frontier at
// WHICHEVER same-table fold site meets the cut during descent (Build.cpp
// :868-872 — not necessarily the receive), so the rule quantifies over every
// member view: each member of a monotone table is itself monotone plumbing (a
// deletion-receiving member would make the table differential) and hence
// eager-walked. Shared by the kIngestFold construction and its census recount;
// §7d cross-checks it against the walk-produced map on every compile.
static VecRole MonotoneIngestRoleDR(Context &context, TABLE *table) {
  if (TableIsDifferential(table)) {
    return VecRole::kEmpty;
  }
  if (context.monotone_negated_tables.count(table) != 0u) {
    return VecRole::kNetAddition;
  }
  for (const QueryView &member : table->views) {
    if (AnyCutSuccessorDR(member)) {
      return VecRole::kNetAddition;
    }
  }
  return VecRole::kEmpty;
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

    // R3 chain-breaker (critique HIGH #4a / spec §2.2): an AGGREGATE or
    // KV-INDEX successor is NOT a branch terminal and NOT table-less plumbing to
    // walk through. Its entire delta maintenance — folding the summarized
    // input's net frontiers into a StateCell, then emitting the one-net-pair
    // into its OWN differential table — is owned by the GROUP_UPDATE op
    // (`BuildGroupUpdateOps`, above), which reads the input table's frontiers
    // directly. If we let the walk make the agg view a table-boundary terminal
    // (its model->table is the agg's own table, != source), the seed loop would
    // mint a bogus SEED_FOLD folding raw input deltas into the agg table
    // (double-deriving it alongside GROUP_UPDATE, breaking V-AGG-SOLE) — and it
    // cannot even bind the summary columns (they come from the fold, not the
    // input scan), aborting `VariableFor`. STOP: drop this successor edge.
    if (succ.IsAggregate() || succ.IsKVIndex()) {
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
// sign-derived (F-5 / V-NEG-CTX). Mirrors EmitChainStep (seed→kInI,
// fixpoint→kInNew, both signs) and Negate.cpp's eager gate (eager normal→kInI,
// eager @never→kPresent). DEFINED in the `hyde` namespace (declared in DR.h) so
// V-PRED-XCHECK can reach it from Stratum.cpp; callers here resolve it by
// enclosing-namespace lookup.

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

// The authoritative (context, hint) → negate-gate predicate (declared in DR.h;
// see the note above its former in-namespace site). The DR model's sole source
// for a negate gate's predicate, and V-PRED-XCHECK Site 1's reference value.
Pred NegateGatePred(Ctx ctx, NegateHint hint) {
  if (ctx == Ctx::kEager) {
    return (hint == NegateHint::kNever) ? Pred::kPresent : Pred::kInI;
  }
  if (ctx == Ctx::kSeed) {
    return Pred::kInI;  // E-13/F18: seed reads kInI BOTH signs
  }
  return Pred::kInNew;  // fixpoint refire, both signs
}

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

std::vector<const DROp *> DRFlowGraph::GroupUpdates(void) const {
  std::vector<const DROp *> out;
  for (const DROp &op : ops) {
    if (op.kind == DROpKind::kGroupUpdate) {
      out.push_back(&op);
    }
  }
  return out;
}

std::vector<const DROp *> DRFlowGraph::StateSeals(void) const {
  std::vector<const DROp *> out;
  for (const DROp &op : ops) {
    if (op.kind == DROpKind::kStateSeal) {
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

// R3 (spec §2.5 V-ALGEBRA / §1.3 fork): pick the lowering algebra for a
// reduction functor. Over-provenance defaults to @recompute when undeclared;
// kv-provenance-undeclared is already REJECTED in the Build.cpp pre-pass
// (num_errors gate), so any kv reaching here is declared. R3 ships (I) and
// (III) only — a declared @invertible selects kInvertible, everything else
// (declared @recompute, or the over() undeclared default) is kRecompute.
static Algebra SelectAlgebra(const ParsedFunctor &f) {
  return f.IsInvertible() ? Algebra::kInvertible : Algebra::kRecompute;
}

// R3 (spec §2.2/§2.3): build a GROUP_UPDATE op for one aggregate/KV view and
// its trailing STATE_SEAL, appending both to `flow.ops`. Mirrors the
// PRODUCT_ARM construction idiom (single frontier-vec consumer, no join
// partner). `group`/`summary` are the cell key / value column projections;
// `input` is the single summarized relation; `algebra_functor` backs the
// reduction. Registers the emit_touched counter± def-edges into the agg
// table's add/delete queues (A-4 multi-def) so the linearizer's E3
// seed-before-drain edge is derived.
static void BuildGroupUpdateOps(
    DRFlowGraph &flow, ProgramImpl *impl, Context &context,
    const std::unordered_map<TABLE *, unsigned> &scc_map, QueryView agg_view,
    AggProvenance prov, std::vector<QueryColumn> group,
    std::vector<QueryColumn> summary, QueryView input,
    const ParsedFunctor &algebra_functor) {
  const Algebra alg = SelectAlgebra(algebra_functor);
  TABLE *const agg_table =
      impl->view_to_model[agg_view]->FindAs<DataModel>()->table;
  // The summarized input may be reached through table-less plumbing (a
  // pass-through TUPLE/SELECT chain — average_weight's summary aggregates read
  // edge_weight through such a chain). Walk the single-predecessor plumbing
  // until the first view that OWNS a differential table: that table's net
  // frontiers are what the StateCell folds over (spec §2.3 E1). The walked-to
  // view becomes the effective `input`.
  TABLE *input_table =
      impl->view_to_model[input]->FindAs<DataModel>()->table;
  while (input_table == nullptr) {
    auto preds = input.Predecessors();
    if (preds.begin() == preds.end()) {
      break;  // dead end — the assert below reports the unsupported shape.
    }
    input = *preds.begin();
    input_table = impl->view_to_model[input]->FindAs<DataModel>()->table;
  }
  assert(agg_table != nullptr);
  assert(input_table != nullptr);
  assert(agg_table != input_table);  // V-AGG-SOLE non-aliasing (spec §1.1)

  const unsigned sc = static_cast<unsigned>(flow.statecells.size());
  DRStateCell cell(agg_view);
  cell.provenance = prov;
  cell.algebra = alg;
  cell.key_cols = group;
  cell.summary_cols = summary;
  cell.algebra_functor = &algebra_functor;
  flow.statecells.push_back(std::move(cell));

  DROp op(DROpKind::kGroupUpdate);
  op.ctx = Ctx::kSeed;
  op.agg_view = agg_view;
  op.provenance = prov;
  op.algebra = alg;
  op.agg_table = agg_table;
  op.input_view = input;
  op.statecell_id = sc;
  op.group_cols = std::move(group);
  op.summary_cols = std::move(summary);

  // ---- BAND (a) frontier_in : two per-sign arms over the input net frontiers.
  // Structurally a THIRD frontier-vec consumer, sibling of CROSSOVER/
  // PRODUCT_ARM, but with NO position-keyed partner read (single input). Per
  // sign: vector:drain(input frontier) + statecell:fold (NOT a counter — C-0c).
  for (int sign : {-1, +1}) {
    DREffect drain;
    drain.kind = EffKind::kVecDrain;
    drain.value_table = input_table;
    drain.vec_role = (sign < 0) ? VecRole::kNetRemoval : VecRole::kNetAddition;
    op.effects.push_back(drain);

    DREffect fold;
    fold.kind = EffKind::kStateFold;
    fold.value_table = agg_table;  // the cell is a peer of this table
    fold.sign = sign;
    op.effects.push_back(fold);
  }

  // ---- BAND (b) emit_touched : the ONE-NET-PAIR band.
  // Reads statecell:emit (working) + statecell:old (sealed, frozen); then two
  // ordinary counter± into the agg's OWN DiffTable (reserved-already vocab).
  DREffect emit;
  emit.kind = EffKind::kStateEmit;
  emit.read_table = agg_table;
  op.effects.push_back(emit);
  DREffect old;
  old.kind = EffKind::kStateOld;
  old.read_table = agg_table;
  op.effects.push_back(old);

  const DerivClass klass = RuleClass(scc_map, agg_table, {input_table});
  assert(klass == DerivClass::kNonRecursive);  // acyclic fence (Stratify)

  for (int sign : {-1, +1}) {
    DREffect counter;
    counter.kind = EffKind::kCounter;
    counter.counter_table = agg_table;
    counter.sign = sign;
    counter.klass = klass;
    op.effects.push_back(counter);
    DREffect crossing;
    crossing.kind = EffKind::kInIReadFrozen;
    crossing.read_table = agg_table;
    crossing.pred = Pred::kInI;
    crossing.ctx = Ctx::kSeed;
    op.effects.push_back(crossing);
    DREffect append;
    append.kind = EffKind::kVecAppend;
    append.value_table = agg_table;
    append.vec_role = (sign < 0) ? VecRole::kDeleteQueue : VecRole::kAddQueue;
    op.effects.push_back(append);
  }

  const unsigned op_idx = static_cast<unsigned>(flow.ops.size());
  flow.ops.push_back(std::move(op));
  // The emit_touched counter± def-edges (E3 seed-before-drain, A-4 multi-def).
  flow.vecs[flow.TableVec(agg_table, VecRole::kDeleteQueue)].defs.push_back(
      op_idx);
  flow.vecs[flow.TableVec(agg_table, VecRole::kAddQueue)].defs.push_back(op_idx);

  // ---- STATE_SEAL : commit-band tail (mirror kCommitSweep; spec §2.3).
  DROp seal(DROpKind::kStateSeal);
  seal.ctx = Ctx::kSeed;
  seal.agg_view = agg_view;
  seal.statecell_id = sc;
  seal.agg_table = agg_table;
  DREffect seal_fx;
  seal_fx.kind = EffKind::kStateFold;  // global:rmw sealed := working (§B-8)
  seal_fx.value_table = agg_table;
  seal_fx.sign = 0;
  seal.effects.push_back(seal_fx);
  flow.ops.push_back(std::move(seal));
}

// P2 CUTOVER: the single authority for a deletion-capable receive's two
// stage-1 ingest-fold ops (+1 before -1). Both the flow enrollment
// (BuildDRInventory's INGEST_FOLD block) and the walk-position lowering
// (ExtendEagerProcedure → LowerIngestFold) construct their ops here, so the
// censused payload and the lowered payload cannot diverge. Pure function of
// (message, receive, table) — no ids, no context.
std::vector<DROp> MakeStageOneIngestFolds(ParsedMessage message,
                                          QueryView receive, TABLE *table) {
  assert(receive.CanReceiveDeletions());
  assert(table != nullptr);
  std::vector<DROp> ops;
  ops.reserve(2u);
  for (int sign : {+1, -1}) {
    DROp op(DROpKind::kIngestFold);
    op.ctx = Ctx::kEager;
    op.ingest_message = message;
    op.ingest_receive = receive;
    op.ingest_table = table;
    op.ingest_sign = sign;
    op.ingest_is_explicit = true;  // the message-support-bit toggle
    op.ingest_role = (sign > 0) ? VecRole::kAddQueue : VecRole::kDeleteQueue;
    op.ingest_stage1 = true;
    DREffect cnt;
    cnt.kind = EffKind::kCounter;
    cnt.counter_table = table;
    cnt.sign = sign;
    cnt.klass = DerivClass::kNonRecursive;
    op.effects.push_back(cnt);
    DREffect app;
    app.kind = EffKind::kVecAppend;
    app.value_table = table;
    app.vec_role = op.ingest_role;
    op.effects.push_back(app);
    ops.push_back(std::move(op));
  }
  return ops;
}

// §6 (subgraphs/demand P1): the single authority for a MONOTONE table-bearing
// receive's ONE stage-1=false ingest-fold op. Sibling to
// MakeStageOneIngestFolds — the monotone op differs on four fields
// (is_explicit=false, stage1=false, single-signed +1, role from the boundary
// predicate not from polarity) and its counter klass is EmissionDerivClass, not
// a hard kNonRecursive; two constructors, one census. Both the flow enrollment
// (BuildDRInventory's INGEST_FOLD block) and the walk-position lowering
// (ExtendEagerProcedure → LowerIngestFold) construct their op here, so the
// censused payload and the lowered payload cannot diverge (§12.6 single-
// authority discipline). Pure function of (impl, context, message, receive,
// table) — no ids. `impl`/`context` are threaded because MonotoneIngestRoleDR
// and EmissionDerivClass both need them.
DROp MakeMonotoneIngestFold(ProgramImpl *impl, Context &context,
                            ParsedMessage message, QueryView receive,
                            TABLE *table) {
  assert(!receive.CanReceiveDeletions());
  assert(table != nullptr);
  DROp op(DROpKind::kIngestFold);
  op.ctx = Ctx::kEager;
  op.ingest_message = message;
  op.ingest_receive = receive;
  op.ingest_table = table;
  op.ingest_sign = 1;
  op.ingest_is_explicit = false;
  op.ingest_stage1 = false;
  op.ingest_role = MonotoneIngestRoleDR(context, table);
  DREffect cnt;
  cnt.kind = EffKind::kCounter;
  cnt.counter_table = table;
  cnt.sign = 1;
  cnt.klass = EmissionDerivClass(impl, context, receive);
  op.effects.push_back(cnt);
  if (op.ingest_role != VecRole::kEmpty) {
    DREffect app;
    app.kind = EffKind::kVecAppend;
    app.value_table = table;
    app.vec_role = op.ingest_role;
    op.effects.push_back(app);
  }
  return op;
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

  // ------------------------------------------------------------ group updates
  // R3 (spec §2.2/§3, C-0d): one GROUP_UPDATE per QueryAggregate AND per
  // QueryKVIndex view (the KV index desugars to the degenerate aggregate — no
  // separate KVINDEX lowering). Built AFTER products, BEFORE branches: the
  // aggregate is a chain-BREAKER, so no branch chain may traverse it (a branch
  // reaching the agg view terminates at the agg table boundary in `SuffixesOf`
  // — the agg view owns its own differential table). Provenance is carried for
  // V-ALGEBRA + the oracle; both provenances lower identically.
  for (QueryAggregate agg : query.Aggregates()) {
    // The group/summary projection columns are taken in the INPUT VIEW's space
    // (spec §5, stage-A handoff item 5): the StateCell folds over the input's
    // net-frontier rows, so the key/value are positions in the input row, not
    // the aggregate's output row. `InputGroupColumns` / `InputAggregatedColumns`
    // are uses of the summarized predecessor's columns.
    std::vector<QueryColumn> group;
    for (auto col : agg.InputGroupColumns()) {
      group.push_back(col);
    }
    for (auto col : agg.InputConfigurationColumns()) {
      group.push_back(col);
    }
    std::vector<QueryColumn> summary;
    for (auto col : agg.InputAggregatedColumns()) {
      summary.push_back(col);
    }
    // The summarized input is the aggregate's predecessor carrying the
    // aggregated columns (Link.cpp binds it as a predecessor). A plain over()
    // aggregate has a single incoming summarized relation.
    QueryView input = QueryView(agg).Predecessors()[0];
    BuildGroupUpdateOps(flow, impl, context, scc_map, QueryView(agg),
                        AggProvenance::kOver, std::move(group),
                        std::move(summary), input, agg.Functor());
  }
  for (QueryKVIndex kv : query.KVIndices()) {
    std::vector<QueryColumn> group;  // config = () for a KV index
    for (auto col : kv.InputKeyColumns()) {
      group.push_back(col);
    }
    std::vector<QueryColumn> summary;
    for (auto col : kv.InputValueColumns()) {
      summary.push_back(col);
    }
    QueryView input = QueryView(kv).Predecessors()[0];
    BuildGroupUpdateOps(flow, impl, context, scc_map, QueryView(kv),
                        AggProvenance::kKv, std::move(group), std::move(summary),
                        input, kv.NthValueMergeFunctor(0));
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
#ifndef NDEBUG
        // R3 (spec §2.2 chain-breaker): an aggregate/KV-index view may be a
        // branch ROOT (path[0] — its OWN output table's rows propagate
        // downstream normally), but the `SuffixesOf` skip guarantees no branch
        // ever TRAVERSES one as an interior/terminal edge (which would mint a
        // bogus SEED_FOLD into the agg table, double-deriving it).
        for (auto pi = 1u; pi < branch.path.size(); ++pi) {
          assert(!branch.path[pi].IsAggregate() && !branch.path[pi].IsKVIndex());
        }
#endif
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
    // F17 gate DATA (finding 2): a dedicated ClaimGate per sign — del gates on
    // C_nr<=0 (TryClaimDel), add on total>0 (TryClaimAdd). NOT a Pred: those are
    // flag-read predicates and would be semantically WRONG if consumed as a gate.
    op.claim_gate = is_del ? ClaimGate::kDelGateCnrNonPositive
                           : ClaimGate::kAddGateTotalPositive;
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
  // (Build.cpp:1048-1051 for every negate view). This is one cleanly-derivable
  // half of the eager web; the other, the eager INGEST_FOLD family, is
  // populated in P2/R1e below (deletion-capable receives stage-1; the
  // monotone/descent surface lowered via LowerIngestFold since §6 —
  // subgraphs/demand P1).
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

  // --------------------------------------------------------------- INGEST_FOLD
  // P2/R1e (p2-ingest-inventory-target.md §2/§3a): one op per (io × receive ×
  // polarity), derived from `query.IOs()` — NEVER by copying the eager walk.
  // A DELETION-CAPABLE receive yields TWO stage-1 ops (+1 then -1, is_explicit,
  // roles kAddQueue/kDeleteQueue from polarity alone — the two
  // LowerIngestFold folds (Stratum.cpp, ex-build_explicit_loop); fold body = counter± +
  // queue append, nothing nested below). A MONOTONE receive with a table
  // yields ONE stage1=false walk-metadata op: its role re-runs the boundary
  // predicate (Build.cpp:857-858 cut successors + :886-889 monotone-negated ∨
  // cut ⇒ kNetAddition, else kEmpty) over EVERY member view of the receive
  // table — NOT the receive's own successors alone, a deliberate deviation
  // from the artifact's §2 rule: the walk's append sits inside the table's
  // fold crossing wherever the descent meets the cut (Build.cpp:868-872, the
  // §5 migration), witnessed in-corpus by deep_chain_retract, whose receive
  // reaches its cut JOIN through a same-table TUPLE. Every member view of a
  // monotone table is itself monotone plumbing (a deletion-receiving member
  // would make the table differential) and hence eager-walked, so member-set
  // existence == the walk's provisioning (§7d cross-checks that on every
  // compile). Since §6 (subgraphs/demand P1) the fold LOWERS via
  // LowerIngestFold from MakeMonotoneIngestFold — the same op the census
  // enrolls — and the descent's INSERT-stream publish / net-additions append
  // is a WALK effect emitted INTO the returned UPDATECOUNT's hole, never an
  // ingest-seed effect. A table-less monotone receive mints no counter fold
  // (ExtendEagerProcedure's monotone else-branch) and is not an ingest fold.
  // Construction is ID-NEUTRAL (no region/vector ids); the queue-vec ids are
  // minted at LOWER time in the same (message, receive, polarity) order (§4).
  for (QueryIO io : query.IOs()) {
    const auto receives = io.Receives();
    if (receives.empty()) {
      continue;
    }
    const ParsedMessage message = ParsedMessage::From(io.Declaration());
    for (QueryView receive : receives) {
      TABLE *const table =
          impl->view_to_model[receive]->FindAs<DataModel>()->table;

      // Deletion-capable (STAGE-1 IN): the +1 op BEFORE the -1 op (the
      // add-before-remove emitted order). Since the cutover, the SAME helper
      // constructs the ops `ExtendEagerProcedure` lowers — the flow's copies
      // and the lowered copies cannot diverge (single authority), and the
      // census recount stays the independent check.
      if (receive.CanReceiveDeletions()) {
        for (DROp &op : MakeStageOneIngestFolds(message, receive, table)) {
          const unsigned op_idx = static_cast<unsigned>(flow.ops.size());
          const VecRole role = op.ingest_role;
          flow.ops.push_back(std::move(op));
          // The queue vec exists (a deletion-capable receive's table is
          // differential, so MintTableVec minted it above); A-4 multi-def.
          flow.vecs[flow.TableVec(table, role)].defs.push_back(op_idx);
        }
        continue;  // no cut/descent for a deletion-capable receive.
      }

      // Monotone with a table (STAGE-1 OUT): inventoried + censused, and since
      // §6 (subgraphs/demand P1) LOWERED via LowerIngestFold from this SAME
      // constructor (the hand-coded monotone arm is gone). The single-authority
      // MakeMonotoneIngestFold is shared with the walk-position lowering.
      if (table != nullptr) {
        flow.ops.push_back(
            MakeMonotoneIngestFold(impl, context, message, receive, table));
        // NO def-edge: the monotone kNetAddition vec is not minted by the
        // differential-only MintTableVec loop; the append parks via
        // TableDeltaVector at LOWER time (§3a).
      }
    }
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

void ValidateDRInventory(const DRFlowGraph &flow) {

  // R2 FAMILY #3: the V-OLD-EQUIV legs (SCC-map / crossover-SET / product-SET /
  // branch-MULTISET / join-SET / per-unit-strata comparisons against the old
  // discovery) are RETIRED with their comparands — the discovery is deleted.
  // What remains here are the INTRINSIC B-3 family validators, evaluated over
  // `flow` alone: V-XOVER-ONE, V-PROD-MONO, V-PROD-CLASS, V-JOIN-ONE.

  // ------------------------------------------------------------ V-XOVER-ONE
  // The DR-IR emits a PAIR of DROps per crossover (the `-` arm always, `+` iff
  // negated_differential). Fold the DR ops back to per-negate crossover records
  // keyed by negate identity. V-XOVER-ONE (B-3.1): exactly one `-` arm per
  // non-@never negate, no negate_table folded by two distinct negates, no
  // @never negate present.
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

  // -------------------------------------------------- V-PROD-MONO / V-PROD-CLASS
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

  // ---------------------------------------------------------------- V-JOIN-ONE
  // Each join view has exactly ONE DR join, with exactly ONE minted shared pivot
  // vec (the join-pivots role, sort-unique-at-drain).
  std::unordered_map<QueryView, const DRJoin *> dr_join;
  for (const DRJoin &j : flow.joins) {
    if (!dr_join.emplace(j.join_view, &j).second) {
      ValidatorFail("V-JOIN-ONE: two DR joins share one join view");
    }
    if (j.pivot_vec >= flow.vecs.size() ||
        flow.vecs[j.pivot_vec].role != VecRole::kJoinPivots ||
        flow.vecs[j.pivot_vec].uniq != UniqueContract::kSortUniqueAtDrain) {
      ValidatorFail("V-JOIN-ONE: join pivot vec missing or mis-typed");
    }
  }
}

// R2 FAMILY #3 (B-13 RETIREMENT): DERIVE the pinned strata from the DR
// inventory independently — the port of the old scheduling fixpoint
// (Stratum.cpp:1985-2139) to run over DR branches/joins/crossover ops/product
// ops + the SCC map, instead of copying the old lift's converged integers. This
// is the SAME monotone integer lift, now the DR side's own authority; the old
// discovery + its fixpoint are deleted with this cut.
//
// Fills: `branch_stratum` (parallel to `flow.branches`), `join_stratum`,
// `crossover_stratum`, `product_stratum`, `drain_stratum`. The lift's inputs and
// rules are replicated EXACTLY (each rule cites its Stratum.cpp anchor), so the
// derived integers equal what the old lift produced — the V-OLD-EQUIV(strata)
// legs that used to check this against the old copy retire with the comparand.
void DeriveDRStrata(DRFlowGraph &flow, ProgramImpl *impl, Context &context,
                    Query query,
                    const std::unordered_map<TABLE *, unsigned> &scc_map) {
  (void) query;

  // Replicate `TableOwnerStratum` (Stratum.cpp:310): the max spec stratum over
  // a table's member views. The owner stratum of the head table (head chain) or
  // the join view's own stratum (join chain) is a unit's INITIAL stratum.
  const auto owner_stratum = [](TABLE *table) -> unsigned {
    unsigned stratum = 0u;
    for (const QueryView &view : table->views) {
      assert(view.Stratum().has_value());
      stratum = std::max(stratum, *(view.Stratum()));
    }
    return stratum;
  };

  const auto same_scc = [&](TABLE *a, TABLE *b) -> bool {
    const auto sa = SccOf(scc_map, a);
    return sa.has_value() && sa == SccOf(scc_map, b);
  };

  // The phase-owned differential tables get a drain stratum, initialized to
  // their owner stratum (Stratum.cpp:1969-1979). Non-phase tables have none.
  flow.drain_stratum.clear();
  for (TABLE *table : impl->tables) {
    if (TableIsDifferential(table) && !TableIsInductionOwnedDR(context, table)) {
      flow.drain_stratum.emplace(table, owner_stratum(table));
    }
  }

  // Per-unit INITIAL strata (the spec stratum before the lift):
  //   head branch  -> owner_stratum(target)   (Stratum.cpp:403 via DiscoverBranches)
  //   join branch  -> join view stratum       (Stratum.cpp:387)
  //   join         -> join view stratum        (the emission's `stratum`)
  //   crossover    -> negate view stratum      (Stratum.cpp:1932)
  //   product      -> product view stratum     (Stratum.cpp:1957)
  flow.branch_stratum.assign(flow.branches.size(), 0u);
  for (unsigned bi = 0u; bi < flow.branches.size(); ++bi) {
    const DRBranch &branch = flow.branches[bi];
    if (branch.ends_at_join) {
      flow.branch_stratum[bi] = branch.path.back().Stratum().value_or(0u);
    } else {
      flow.branch_stratum[bi] = owner_stratum(branch.target);
    }
  }
  flow.join_stratum.clear();
  for (const DRJoin &j : flow.joins) {
    flow.join_stratum[j.join_view] = j.join_view.Stratum().value_or(0u);
  }
  flow.crossover_stratum.clear();
  flow.product_stratum.clear();
  flow.group_update_stratum.clear();
  for (const DROp &op : flow.ops) {
    if (op.kind == DROpKind::kCrossover) {
      const QueryView nv(*op.negate);
      flow.crossover_stratum.emplace(nv, nv.Stratum().value_or(0u));
    } else if (op.kind == DROpKind::kProductArm) {
      flow.product_stratum.emplace(*op.product_view,
                                   op.product_view->Stratum().value_or(0u));
    } else if (op.kind == DROpKind::kGroupUpdate) {
      // R3: the GROUP_UPDATE sits at its agg VIEW's stratum. Stratify placed the
      // aggregate STRICTLY ABOVE its input (spec §5, mirror of the negate
      // reject), so view.Stratum() already exceeds the input's.
      flow.group_update_stratum.emplace(*op.agg_view,
                                        op.agg_view->Stratum().value_or(0u));
    }
  }

  // `ready_after(T)` = drain_stratum[T] + 1 (0 if T is not phase owned) — the
  // stratum a reader must sit at to see T's consolidated frontier as final
  // (Stratum.cpp:1999-2004). `ready_across` exempts same-SCC reads
  // (Stratum.cpp:2017-2019: an SCC's mutual reads must not ratchet each other).
  const auto ready_after = [&](TABLE *table) -> unsigned {
    if (auto it = flow.drain_stratum.find(table); it != flow.drain_stratum.end()) {
      return it->second + 1u;
    }
    return 0u;
  };
  const auto ready_across = [&](TABLE *head, TABLE *read) -> unsigned {
    return same_scc(head, read) ? 0u : ready_after(read);
  };

  // The negated tables read on a branch chain must be phase-final when the
  // chain runs (Stratum.cpp:2021-2034).
  const auto negated_tables_ready = [&](const DRBranch &branch,
                                        TABLE *head) -> unsigned {
    unsigned stratum = 0u;
    for (const QueryView &view : branch.path) {
      if (view.IsNegate()) {
        const QueryView negated_view = QueryNegate::From(view).NegatedView();
        TABLE *const nt =
            impl->view_to_model[negated_view]->FindAs<DataModel>()->table;
        stratum = std::max(stratum, ready_across(head, nt));
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

  // The join view -> its parallel `flow.branches` join-terminal index (the old
  // lift stored the join's stratum on the JoinEmission, keyed by view; here the
  // authority is `join_stratum`, and join-terminal branches read it back).
  for (auto changed = true; changed; ) {
    changed = false;

    // Branch lift (Stratum.cpp:2046-2066).
    for (unsigned bi = 0u; bi < flow.branches.size(); ++bi) {
      const DRBranch &branch = flow.branches[bi];
      TABLE *const head =
          branch.ends_at_join
              ? impl->view_to_model[branch.path.back()]->FindAs<DataModel>()
                    ->table
              : branch.target;
      unsigned stratum =
          std::max(flow.branch_stratum[bi], ready_across(head, branch.source));
      stratum = std::max(stratum, negated_tables_ready(branch, head));

      if (branch.ends_at_join) {
        lift(flow.join_stratum[branch.path.back()], stratum, changed);
      } else {
        lift(flow.branch_stratum[bi], stratum, changed);
        lift(flow.drain_stratum[branch.target], stratum, changed);
      }
    }

    // Join lift (Stratum.cpp:2068-2082).
    for (const DRJoin &j : flow.joins) {
      TABLE *const head =
          impl->view_to_model[j.join_view]->FindAs<DataModel>()->table;
      unsigned stratum = flow.join_stratum[j.join_view];
      for (QueryView side : QueryJoin::From(j.join_view).JoinedViews()) {
        TABLE *const st = impl->view_to_model[side]->FindAs<DataModel>()->table;
        stratum = std::max(stratum, ready_across(head, st));
      }
      lift(flow.join_stratum[j.join_view], stratum, changed);
      for (TABLE *target : j.targets) {
        lift(flow.drain_stratum[target], flow.join_stratum[j.join_view],
             changed);
      }
    }

    // Crossover lift (Stratum.cpp:2091-2097): above both read tables'
    // readiness, then lift the negate table's drain to it.
    for (const DROp &op : flow.ops) {
      if (op.kind != DROpKind::kCrossover) {
        continue;
      }
      const QueryView nv(*op.negate);
      unsigned stratum = flow.crossover_stratum[nv];
      stratum =
          std::max(stratum, ready_across(op.negate_table, op.negated_table));
      stratum = std::max(stratum, ready_across(op.negate_table, op.pred_table));
      lift(flow.crossover_stratum[nv], stratum, changed);
      lift(flow.drain_stratum[op.negate_table], flow.crossover_stratum[nv],
           changed);
    }

    // Product lift (Stratum.cpp:2109-2116): above EVERY side's readiness
    // (strict `ready_after`, no SCC exemption — the acyclic fence rules out
    // same-SCC sides), then lift the product table's drain to it.
    for (const DROp &op : flow.ops) {
      if (op.kind != DROpKind::kProductArm) {
        continue;
      }
      const QueryView pv(*op.product_view);
      unsigned stratum = flow.product_stratum[pv];
      for (TABLE *side_table : op.side_tables) {
        stratum = std::max(stratum, ready_after(side_table));
      }
      lift(flow.product_stratum[pv], stratum, changed);
      lift(flow.drain_stratum[op.product_table], flow.product_stratum[pv],
           changed);
    }

    // R3 GROUP_UPDATE lift (spec §2.4 E1): the op's frontier_in DRAINS the
    // input's net-removal/net-addition frontiers, written by the input
    // stratum's FRONTIER_FILTERs — so the op cannot run until the input
    // stratum's filters are final. STRICT `ready_after` (like PRODUCT, not
    // crossover's SCC exemption): an aggregate over its own SCC is a Stratify
    // reject, so no self-read exemption is ever needed. Then the agg table's
    // OWN drain lifts to the update's stratum (E4: the agg-table acyclic band
    // is downstream of emit_touched). The double-nesting (KV stratum-0 ->
    // sum/count stratum-1 -> join stratum-2 in average_weight) falls out of the
    // SAME monotone fixpoint via `ready_after(agg_table)` on the join lift.
    for (const DROp &op : flow.ops) {
      if (op.kind != DROpKind::kGroupUpdate) {
        continue;
      }
      const QueryView av(*op.agg_view);
      TABLE *const input_table =
          impl->view_to_model[*op.input_view]->FindAs<DataModel>()->table;
      unsigned stratum = flow.group_update_stratum[av];
      stratum = std::max(stratum, ready_after(input_table));
      lift(flow.group_update_stratum[av], stratum, changed);
      lift(flow.drain_stratum[op.agg_table], flow.group_update_stratum[av],
           changed);
    }

    // SCC pinning (Stratum.cpp:2124-2138): every table of a recursive SCC
    // drains at one shared stratum (the max over its members).
    if (!scc_map.empty()) {
      std::unordered_map<unsigned, unsigned> scc_stratum;
      for (const auto &[table, group] : scc_map) {
        if (auto it = flow.drain_stratum.find(table);
            it != flow.drain_stratum.end()) {
          unsigned &slot = scc_stratum[group];
          slot = std::max(slot, it->second);
        }
      }
      for (const auto &[table, group] : scc_map) {
        if (auto it = flow.drain_stratum.find(table);
            it != flow.drain_stratum.end()) {
          lift(it->second, scc_stratum[group], changed);
        }
      }
    }
  }

  // A join-terminal branch's stratum is its join's (Stratum.cpp:2142-2146).
  for (unsigned bi = 0u; bi < flow.branches.size(); ++bi) {
    const DRBranch &branch = flow.branches[bi];
    if (branch.ends_at_join) {
      flow.branch_stratum[bi] = flow.join_stratum[branch.path.back()];
    }
  }

  // Stamp the SCC drain stratum onto each FIXPOINT_ROUND shell (family #3
  // DRAIN-STRATUM NATIVIZATION): the round's group drains at the shared stratum
  // of its SCC tables. `LowerDRRounds` reads this field instead of the old
  // discovery's `drain_stratum` map.
  for (DRRound &round : flow.rounds) {
    unsigned stratum = 0u;
    for (const auto &[table, group] : scc_map) {
      if (group != round.scc_group) {
        continue;
      }
      if (auto it = flow.drain_stratum.find(table);
          it != flow.drain_stratum.end()) {
        stratum = std::max(stratum, it->second);
      }
    }
    round.drain_stratum = stratum;
  }

  // Stamp `scc_group` onto each FIXPOINT_FIRE op (family #3: retire the
  // family-#2 RecursiveSCC re-derivation in `LowerRoundBody`).
  for (DROp &op : flow.ops) {
    if (op.kind == DROpKind::kFixpointFire && op.fire_table) {
      if (auto g = SccOf(scc_map, op.fire_table); g.has_value()) {
        op.scc_group = *g;
      }
    }
  }
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
        // V-CLAIM-GATE (F17): every claim drain carries the SEMANTICALLY-CORRECT
        // gate for its sign AS DATA (finding 2): a `-` drain must gate on
        // C_nr<=0, a `+` drain on total>0. Checked against the dedicated
        // ClaimGate value (not a Pred stand-in).
        const bool is_del = (op.table_op_sign < 0);
        const ClaimGate want = is_del ? ClaimGate::kDelGateCnrNonPositive
                                      : ClaimGate::kAddGateTotalPositive;
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
      case DROpKind::kIngestFold: {
        // V-INGEST (P2 artifact §7a): EFFECT-SET TOTALITY — exactly one
        // kCounter (sign == the op's, klass kNonRecursive) plus at most one
        // kVecAppend (present iff the role is non-empty), nothing else.
        unsigned counters = 0u, appends = 0u, others = 0u;
        for (const DREffect &e : op.effects) {
          if (e.kind == EffKind::kCounter) {
            ++counters;
            if (e.sign != op.ingest_sign ||
                e.klass != DerivClass::kNonRecursive ||
                e.counter_table != op.ingest_table) {
              ValidatorFail("V-INGEST: ingest counter effect disagrees with "
                            "the op (sign/klass/table)");
            }
          } else if (e.kind == EffKind::kVecAppend) {
            ++appends;
            if (e.vec_role != op.ingest_role ||
                e.value_table != op.ingest_table) {
              ValidatorFail("V-INGEST: ingest append effect disagrees with "
                            "the op's queue role");
            }
          } else {
            ++others;
          }
        }
        if (counters != 1u || others != 0u ||
            appends != ((op.ingest_role != VecRole::kEmpty) ? 1u : 0u)) {
          ValidatorFail("V-INGEST: ingest fold effect set is not "
                        "{kCounter} or {kCounter, kVecAppend}");
        }
        if (op.ingest_sign != 1 && op.ingest_sign != -1) {
          ValidatorFail("V-INGEST: ingest fold sign not in {-1, +1}");
        }
        if (op.ingest_table == nullptr || !op.ingest_message.has_value() ||
            !op.ingest_receive.has_value()) {
          ValidatorFail("V-INGEST: ingest fold missing table/message/receive");
        }
        // Stage-1 membership IS deletion-capability (== is_explicit) in R1e.
        if (op.ingest_stage1 != op.ingest_is_explicit) {
          ValidatorFail("V-INGEST: stage1 bit disagrees with is_explicit");
        }
        // §7c QUEUE-ROLE AGREEMENT (pure DR VecRole terms, never VectorKind):
        // sign<0 ⇒ kDeleteQueue; sign>0 ⇒ {kAddQueue, kNetAddition, kEmpty};
        // an explicit (deletion-capable) op parks into a Queue role and its
        // receive table is differential; a monotone op parks into
        // kNetAddition|kEmpty, and kNetAddition only over a monotone table
        // (mirrors TableIsDifferential — the Build.cpp:888 guard).
        if (op.ingest_sign < 0 && op.ingest_role != VecRole::kDeleteQueue) {
          ValidatorFail("V-INGEST: a `-` ingest fold does not park into the "
                        "delete queue");
        }
        if (op.ingest_sign > 0 && op.ingest_role != VecRole::kAddQueue &&
            op.ingest_role != VecRole::kNetAddition &&
            op.ingest_role != VecRole::kEmpty) {
          ValidatorFail("V-INGEST: a `+` ingest fold has an invalid role");
        }
        if (op.ingest_is_explicit) {
          if (op.ingest_role != VecRole::kAddQueue &&
              op.ingest_role != VecRole::kDeleteQueue) {
            ValidatorFail("V-INGEST: a deletion-capable ingest fold does not "
                          "park into a queue role");
          }
          if (!TableIsDifferential(op.ingest_table)) {
            ValidatorFail("V-INGEST: a deletion-capable receive's table is "
                          "not differential");
          }
        } else {
          if (op.ingest_role != VecRole::kNetAddition &&
              op.ingest_role != VecRole::kEmpty) {
            ValidatorFail("V-INGEST: a monotone ingest fold parks into a "
                          "queue role");
          }
          if (op.ingest_role == VecRole::kNetAddition &&
              TableIsDifferential(op.ingest_table)) {
            ValidatorFail("V-INGEST: a differential table's monotone ingest "
                          "fold claims the net-additions frontier");
          }
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

  // Expected INGEST_FOLD count + per-op key multiset (P2/R1e, artifact §7):
  // an INDEPENDENT recount over query.IOs()×Receives()×polarity (the same
  // discovery-input re-derivation discipline as exp_branches above — never a
  // region-tree walk). A deletion-capable receive counts 2 (stage-1 add +
  // delete); a monotone receive with a table counts 1 (stage-1 OUT, still
  // censused); a table-less monotone receive counts 0. The key is
  // (table, sign, is_explicit, role, message), compared order-free.
  unsigned exp_ingest = 0u;
  using IngestKey = std::tuple<uintptr_t, int, bool, uint8_t, uint64_t>;
  std::vector<IngestKey> exp_ingest_keys;
  for (QueryIO io : query.IOs()) {
    const auto io_receives = io.Receives();
    if (io_receives.empty()) {
      continue;
    }
    const uint64_t mid = ParsedMessage::From(io.Declaration()).Id();
    for (QueryView receive : io_receives) {
      TABLE *const table =
          impl->view_to_model[receive]->FindAs<DataModel>()->table;
      const auto tid = reinterpret_cast<uintptr_t>(table);
      if (receive.CanReceiveDeletions()) {
        exp_ingest += 2u;
        exp_ingest_keys.emplace_back(
            tid, 1, true, static_cast<uint8_t>(VecRole::kAddQueue), mid);
        exp_ingest_keys.emplace_back(
            tid, -1, true, static_cast<uint8_t>(VecRole::kDeleteQueue), mid);
      } else if (table != nullptr) {
        exp_ingest += 1u;
        const VecRole role = MonotoneIngestRoleDR(context, table);
        exp_ingest_keys.emplace_back(tid, 1, false,
                                     static_cast<uint8_t>(role), mid);
      }
    }
  }

  // Expected GROUP_UPDATE / STATE_SEAL count + per-op key multiset (P0, the
  // E-25 census gap closed): an INDEPENDENT recount over query.Aggregates()
  // and query.KVIndices() — the mint loop's own discovery inputs, never
  // flow-derived (E-27: `flow.statecells.size()` is pushed in lockstep with
  // the ops by BuildGroupUpdateOps and would compare flow against itself).
  // The recount is tight: the mint loops iterate the same accessors with no
  // skip, and the Build.cpp pre-pass rejects (induction-owned input, config
  // columns, undeclared KV algebra) bail via the num_errors→nullopt gate
  // BEFORE any DR construction, so no unsupported agg/kv view coexists with
  // a running census. The key is (agg table, provenance, algebra, view id),
  // compared order-free — statecell_id is a mint-order artifact and is
  // deliberately NOT keyed (E-28); its bijection with the seals is the
  // V-AGG-PAIR structural check below.
  unsigned exp_group_update = 0u;
  using GroupUpdateKey = std::tuple<uintptr_t, uint8_t, uint8_t, uint64_t>;
  std::vector<GroupUpdateKey> exp_gu_keys;
  const auto add_gu_key = [&](QueryView view, AggProvenance prov,
                              const ParsedFunctor &functor) {
    ++exp_group_update;
    TABLE *const agg_table =
        impl->view_to_model[view]->FindAs<DataModel>()->table;
    exp_gu_keys.emplace_back(reinterpret_cast<uintptr_t>(agg_table),
                             static_cast<uint8_t>(prov),
                             static_cast<uint8_t>(SelectAlgebra(functor)),
                             view.UniqueId());
  };
  for (QueryAggregate agg : query.Aggregates()) {
    add_gu_key(QueryView(agg), AggProvenance::kOver, agg.Functor());
  }
  for (QueryKVIndex kv : query.KVIndices()) {
    add_gu_key(QueryView(kv), AggProvenance::kKv, kv.NthValueMergeFunctor(0));
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
  expect(DROpKind::kIngestFold, exp_ingest, "ingest folds");
  expect(DROpKind::kGroupUpdate, exp_group_update, "group updates");
  expect(DROpKind::kStateSeal, exp_group_update, "state seals");

  // The INGEST_FOLD per-op key multiset (order-free; artifact §7): the
  // recomputed (table, sign, is_explicit, role, message) keys must equal the
  // multiset read off `flow`'s ops. This is the E-22 completeness half of the
  // census — a dropped or double-inventoried receive/polarity mismatches here
  // even when the count happens to agree.
  {
    std::vector<IngestKey> got_ingest_keys;
    for (const DROp &op : flow.ops) {
      if (op.kind != DROpKind::kIngestFold) {
        continue;
      }
      got_ingest_keys.emplace_back(
          reinterpret_cast<uintptr_t>(op.ingest_table), op.ingest_sign,
          op.ingest_is_explicit, static_cast<uint8_t>(op.ingest_role),
          op.ingest_message->Id());
    }
    std::sort(exp_ingest_keys.begin(), exp_ingest_keys.end());
    std::sort(got_ingest_keys.begin(), got_ingest_keys.end());
    if (exp_ingest_keys != got_ingest_keys) {
      std::fprintf(stderr,
                   "error: DR-IR op census mismatch (ingest fold keys): the "
                   "derived (table, sign, is_explicit, role, message) multiset "
                   "disagrees with the independent recount\n");
      std::abort();
    }
  }

  // The GROUP_UPDATE per-op key multiset (order-free; P0): the recomputed
  // (agg table, provenance, algebra, view id) keys must equal the multiset
  // read off `flow`'s ops — a wrong-view enrollment, a wrong provenance, or a
  // wrong algebra selection mismatches here even when the count agrees.
  {
    std::vector<GroupUpdateKey> got_gu_keys;
    for (const DROp &op : flow.ops) {
      if (op.kind != DROpKind::kGroupUpdate) {
        continue;
      }
      got_gu_keys.emplace_back(reinterpret_cast<uintptr_t>(op.agg_table),
                               static_cast<uint8_t>(op.provenance),
                               static_cast<uint8_t>(op.algebra),
                               op.agg_view->UniqueId());
    }
    std::sort(exp_gu_keys.begin(), exp_gu_keys.end());
    std::sort(got_gu_keys.begin(), got_gu_keys.end());
    if (exp_gu_keys != got_gu_keys) {
      std::fprintf(stderr,
                   "error: DR-IR op census mismatch (group update keys): the "
                   "derived (table, provenance, algebra, view) multiset "
                   "disagrees with the independent recount\n");
      std::abort();
    }
  }

  // V-AGG-EFFECT (P0 effect-set totality; the promoted DR.cpp mint-time
  // asserts, E-29): every kGroupUpdate carries exactly the spec §2.3 effect
  // set — 2 frontier drains, 2 state folds (both signs), 1 emit + 1 old, and
  // the one-net-pair tail (2 NonRecursive counters, 2 kInI crossings, 2 queue
  // appends). Every kStateSeal carries exactly its sign-0 global:rmw fold.
  // V-AGG-SOLE: the cell is a PEER of the agg's own differential table — the
  // summarized input's table is non-null and never aliases the agg table.
  // V-AGG-PAIR: statecell ids are a bijection — each kGroupUpdate owns a
  // distinct id in [0, |statecells|) and exactly one kStateSeal carries each
  // of the same ids.
  {
    std::vector<unsigned> gu_cells, seal_cells;
    for (const DROp &op : flow.ops) {
      if (op.kind == DROpKind::kGroupUpdate) {
        unsigned drains = 0u, folds = 0u, emits = 0u, olds = 0u,
                 counters = 0u, crossings = 0u, appends = 0u;
        int fold_signs = 0, counter_signs = 0;
        for (const DREffect &fx : op.effects) {
          switch (fx.kind) {
            case EffKind::kVecDrain: ++drains; break;
            case EffKind::kStateFold: ++folds; fold_signs += fx.sign; break;
            case EffKind::kStateEmit: ++emits; break;
            case EffKind::kStateOld: ++olds; break;
            case EffKind::kCounter:
              ++counters;
              counter_signs += fx.sign;
              if (fx.klass != DerivClass::kNonRecursive) {
                ValidatorFail("V-AGG-EFFECT: a GROUP_UPDATE counter is not "
                              "NonRecursive (the acyclic fence)");
              }
              break;
            case EffKind::kInIReadFrozen: ++crossings; break;
            case EffKind::kVecAppend: ++appends; break;
            default:
              ValidatorFail("V-AGG-EFFECT: a GROUP_UPDATE carries an effect "
                            "kind outside the spec §2.3 set");
          }
        }
        if (drains != 2u || folds != 2u || fold_signs != 0 || emits != 1u ||
            olds != 1u || counters != 2u || counter_signs != 0 ||
            crossings != 2u || appends != 2u) {
          ValidatorFail("V-AGG-EFFECT: a GROUP_UPDATE effect set is not the "
                        "spec §2.3 totality (2 drains, ± folds, emit+old, "
                        "one-net-pair tail)");
        }
        if (op.agg_table == nullptr || !op.input_view || !op.agg_view) {
          ValidatorFail("V-AGG-SOLE: a GROUP_UPDATE lacks its table or views");
        }
        TABLE *const input_table =
            impl->view_to_model[*op.input_view]->FindAs<DataModel>()->table;
        if (input_table == nullptr || input_table == op.agg_table) {
          ValidatorFail("V-AGG-SOLE: a GROUP_UPDATE's summarized input table "
                        "is missing or aliases the aggregate's own table");
        }
        gu_cells.push_back(op.statecell_id);
      } else if (op.kind == DROpKind::kStateSeal) {
        if (op.effects.size() != 1u ||
            op.effects[0].kind != EffKind::kStateFold ||
            op.effects[0].sign != 0) {
          ValidatorFail("V-AGG-EFFECT: a STATE_SEAL effect set is not the "
                        "single sign-0 sealed:=working fold");
        }
        seal_cells.push_back(op.statecell_id);
      }
    }
    std::sort(gu_cells.begin(), gu_cells.end());
    std::sort(seal_cells.begin(), seal_cells.end());
    if (gu_cells != seal_cells ||
        std::adjacent_find(gu_cells.begin(), gu_cells.end()) !=
            gu_cells.end() ||
        gu_cells.size() != flow.statecells.size() ||
        (!gu_cells.empty() &&
         gu_cells.back() != static_cast<unsigned>(gu_cells.size() - 1u))) {
      ValidatorFail("V-AGG-PAIR: statecell ids are not a GROUP_UPDATE ↔ "
                    "STATE_SEAL bijection onto [0, |statecells|)");
    }
  }

  // §7b ONE-OP-PER-(message, receive, polarity): no receive owns two ingest
  // folds of one sign. (The message is determined by the receive — one io per
  // receive — so (receive, sign) is the full key; the message identity is
  // already pinned by the key-multiset census above.)
  {
    std::unordered_map<QueryView, unsigned> receive_signs;  // → sign bitmask
    for (const DROp &op : flow.ops) {
      if (op.kind != DROpKind::kIngestFold) {
        continue;
      }
      const unsigned bit = (op.ingest_sign < 0) ? 1u : 2u;
      unsigned &mask = receive_signs[*op.ingest_receive];
      if (mask & bit) {
        ValidatorFail("V-INGEST: two ingest folds share one "
                      "(message, receive, polarity) key");
      }
      mask |= bit;
    }
  }

  // §7d MONOTONE QUEUE-ROLE CROSS-CHECK (the §5 R1e-phase check): for every
  // monotone (stage-1 OUT) op, the boundary-predicate-derived role must agree
  // with the walk-produced `context.table_delta_vecs` — the map holds a
  // kNetAdditions vec for the receive table iff the predicate said
  // kNetAddition. Both read at phase time (the map is full: BuildEntryProcedure
  // runs the eager walk before BuildStratumPhases). A disagreement
  // means the table-level member-view predicate (MonotoneIngestRoleDR) and
  // the walk's actual append-site provisioning diverged — abort.
  for (const DROp &op : flow.ops) {
    if (op.kind != DROpKind::kIngestFold || op.ingest_is_explicit) {
      continue;
    }
    bool map_has_net_additions = false;
    if (auto it = context.table_delta_vecs.find(op.ingest_table);
        it != context.table_delta_vecs.end()) {
      map_has_net_additions =
          it->second.count(
              static_cast<unsigned>(VectorKind::kNetAdditions)) != 0u;
    }
    const bool pred_says = (op.ingest_role == VecRole::kNetAddition);
    if (map_has_net_additions != pred_says) {
      ValidatorFail("V-INGEST: a monotone receive's derived queue role "
                    "disagrees with the walk-produced net-additions frontier "
                    "(table_delta_vecs membership)");
    }
  }

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
        case EffKind::kStateFold:
          // R3 (spec §2.3): a value-fold WRITES the state cell's working word
          // (a peer of the agg table). Model it as a write hazard keyed on the
          // agg table so E2 fold-before-emit is a derived RAW. A STATE_SEAL's
          // sealed:=working fold (sign 0) is also a write; it trails in the
          // commit band, so the derived edge is emission-order-safe.
          flag_accesses.push_back(FlagAccess{oi, e.value_table, true});
          break;
        case EffKind::kStateEmit:
          // A VALUED read of the working word (E2 RAW after every fold).
          flag_accesses.push_back(FlagAccess{oi, e.read_table, false});
          break;
        case EffKind::kStateOld:
          break;  // frozen sealed read: no within-band hazard (C-0b).
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
      case DROpKind::kGroupUpdate:
        // R3: the GROUP_UPDATE keys on its agg view's lifted stratum (E1: above
        // the input's frontier filters). The critique's false-negative-space
        // fix — a silent default-0 would let a mis-stratified frontier drain
        // pass V-READY (the equal-strata WRONG-A shape).
        if (op.agg_view.has_value()) {
          if (auto it = flow.group_update_stratum.find(*op.agg_view);
              it != flow.group_update_stratum.end()) {
            return it->second;
          }
        }
        return 0u;
      case DROpKind::kStateSeal:
        return 0u;  // trailing commit band (V-READY skip, below)
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
      case DROpKind::kGroupUpdate:
        // R3: GROUP_UPDATE emits in band 0 (with seeds/products) at its lifted
        // stratum — BEFORE the agg table's own single-pass claim drain (band 1)
        // and immediate frontier filter (band 2) at the SAME stratum. So the
        // emit_touched counter± seeds the agg queues before the agg drain reads
        // them (E3 seed-before-drain), riding the existing V-SEED-DRAIN edge.
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
               : op.agg_table     ? op.agg_table
               : op.negate_table  ? op.negate_table
               : op.ingest_table  ? op.ingest_table
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
    // P2/R1e: ingest folds are lead-0 off-lattice (VALIDATOR-ORDERING ONLY —
    // emission never reads this key; the lowering default is the old walk's
    // query.IOs()×Receives()×(+before−) construction order, artifact §4).
    // Lead 0 puts every ingest→drain RAW forward in the key (ingest seeds the
    // queue the lead-1 phase drain reads — seed_before_drain).
    if (op.kind == DROpKind::kIngestFold) {
      return Key{0u, 0u, 0u, op_table_id(op), op.ingest_sign, oi};
    }
    if (op.kind == DROpKind::kCommitSweep) {
      return Key{2u, max_stratum + 1u, 9u, op_table_id(op), 0, oi};
    }
    if (op.kind == DROpKind::kStateSeal) {
      // R3: STATE_SEAL trails all strata alongside commit sweeps (spec §2.4
      // E5 / V-COMMIT-TRAILS) — after every emit_touched read of working, band
      // 9+1 so it sorts strictly after commit sweeps for determinism.
      return Key{2u, max_stratum + 1u, 10u, op_table_id(op), 0, oi};
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
    // The net_* frontiers (assemble-fills, filter-drains across the epoch) AND
    // the delete/add INPUT QUEUES are epoch-carried accumulators: a queue is
    // filled by seeds/folds AND by the OVERDELETE round's REDERIVE output, then
    // DRAINED-and-cleared by the (later-phase) claim drain. The REDERIVE fill
    // sorts AFTER the drain in the FLAT band key (rederive is OVERDELETE-round
    // OUTPUT, band-numbered high) even though the OVERDELETE phase executes
    // BEFORE the INSERT phase's drain — the flat key cannot see the round-phase
    // nesting, so the write legitimately runs against the band key. That is a
    // loop-carried refill, NOT a band hazard (finding 3(a)): classifying the
    // queue roles here makes it a loop-carried RAW + WAR witness (the same topo
    // outcome the pre-review silent flip produced), so V-BAND-HAZARD stays
    // reserved for a GENUINE non-carried same-scope inversion.
    return r == VecRole::kNetRemoval || r == VecRole::kNetAddition ||
           r == VecRole::kDeleteQueue || r == VecRole::kAddQueue;
  };

  // Emit a WAW hazard for a WRITE/WRITE pair (SYMMETRIC — no natural
  // producer→consumer direction). Two writers of one vec/flag class have no
  // read to order against; the band-template key IS the ground-truth order, so
  // the edge always follows the key (lower-key → higher-key). This is the ONLY
  // caller of the flip that finding 3(a) leaves legitimate: a WAW has no
  // "natural direction" to run against the band key, so it never band-hazards.
  const auto emit_waw = [&](unsigned a, unsigned b) {
    if (a == b) {
      return;
    }
    DRDep dep;
    dep.scope = scope_pair(a, b);
    dep.kind = DepKind::kWAW;
    dep.loop_carried = false;
    if (key_less(keys[a], keys[b])) {
      dep.from = a;
      dep.to = b;
    } else {
      dep.from = b;
      dep.to = a;
    }
    flow.dep_edges.push_back(dep);
  };

  // Emit the hazard(s) for a WRITE/READ pair, classified by ACCESSOR KINDS +
  // KEY ORDER (finding 3(c)): whether the pair is a RAW or a WAR is a property
  // of which access is the write and which the read, composed with their
  // EXECUTION (band-key) order — NEVER the op-construction order the outer loop
  // happens to visit them in. (The earlier code keyed RAW/WAR on the (i,j)
  // iteration order of the access list; a write appended to the list AFTER the
  // read it feeds — a legal construction order — was then mislabelled WAR, and
  // a true RAW recorded as WAR silently skips V-READY's reads-lower-or-same
  // check. Order of DISCOVERY is not order of EXECUTION.)
  //
  // Execution order is the band key: the writer W and reader R execute in key
  // order. If W precedes R ⇒ RAW (from=W, to=R): the reader consumes what the
  // writer produced this scope. If R precedes W ⇒ WAR (from=R, to=W): the read
  // drains before the writer refills — the drain-before-refill witness.
  //
  // For a LOOP-CARRIED role (round Δ frontier / epoch net_* frontier / the
  // delete/add input queues) the R-before-W case ALSO carries a loop-carried
  // RAW (this iteration's read sees the PREVIOUS-phase fill; the refill sorts
  // later in the flat key because the round-phase nesting is invisible to it):
  // we emit the intra-scope WAR *and* the loop-carried RAW W→R (excluded from
  // the topo sort, its witness certified by V-LOOP, finding 3(b)). For a
  // NON-carried role, R-before-W is a plain WAR — the read consumed the old
  // value and the write clobbers after (e.g. a REDERIVE reads a table's flags
  // that the end-of-batch COMMIT_SWEEP later clears): legitimate, no carry.
  //
  // Every edge is DIRECTED from the lower-key op to the higher-key op (execution
  // order): a RAW is writer→reader (writer earlier), a WAR is reader→writer
  // (reader earlier). The band hazard finding 3(a) guards against — a non-loop-
  // carried edge that RUNS AGAINST the band key — therefore cannot arise from
  // THIS function by construction; it is asserted globally as V-BAND-HAZARD
  // below, replacing the pre-review SILENT FLIP that hid exactly such an edge.
  const auto emit_rw_hazard = [&](unsigned writer, unsigned reader,
                                  VecRole carried_role) {
    if (writer == reader) {
      return;
    }
    const bool carried = is_round_carried_role(carried_role) ||
                         is_epoch_carried_role(carried_role);
    if (key_less(keys[writer], keys[reader])) {
      // Writer executes first ⇒ RAW, intra-scope (from=writer so V-READY reads
      // the true producer stratum — finding 3(c)).
      DRDep dep;
      dep.scope = scope_pair(writer, reader);
      dep.kind = DepKind::kRAW;
      dep.from = writer;
      dep.to = reader;
      dep.loop_carried = false;
      flow.dep_edges.push_back(dep);
    } else {
      // Reader executes first (in band-key order) ⇒ an intra-scope WAR
      // (from=reader, to=writer): the read drained before the write refilled.
      DRDep war;
      war.scope = scope_pair(writer, reader);
      war.kind = DepKind::kWAR;
      war.from = reader;
      war.to = writer;
      war.loop_carried = false;
      flow.dep_edges.push_back(war);
      // For a carried role this WAR is the drain-before-refill witness for a
      // loop-carried RAW (this iteration reads the previous phase's fill); emit
      // that RAW too (loop-carried, topo-excluded, checked by V-LOOP).
      if (carried) {
        DRDep raw;
        raw.scope = scope_pair(writer, reader);
        raw.kind = DepKind::kRAW;
        raw.from = writer;
        raw.to = reader;
        raw.loop_carried = true;
        flow.dep_edges.push_back(raw);
      }
    }
  };

  // Vec hazards: group accesses by vec; for every access pair classify by
  // ACCESSOR KINDS (finding 3(c)) — a write/write pair is a WAW (symmetric,
  // band-ordered), a write/read pair (either construction order) is a RAW or
  // WAR determined by the writer/reader EXECUTION order in `emit_rw_hazard`,
  // NOT by which of (i,j) the outer loop visits first.
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
          emit_waw(x->op_idx, y->op_idx);
        } else if (x->is_write != y->is_write) {
          // Identify writer/reader SEMANTICALLY (by is_write), not by (i,j).
          const VecAccess *w = x->is_write ? x : y;
          const VecAccess *r = x->is_write ? y : x;
          emit_rw_hazard(w->op_idx, r->op_idx, role);
        }
        // read/read: no hazard.
      }
    }
  }

  // Table-flag hazards (§4.1-4.4): counter±/gate writes and membership reads
  // over one table's flag class, classified by accessor kinds (as above).
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
          emit_waw(x->op_idx, y->op_idx);
        } else if (x->is_write != y->is_write) {
          const FlagAccess *w = x->is_write ? x : y;
          const FlagAccess *r = x->is_write ? y : x;
          // Flag classes carry no loop-carried role (kInI reads are frozen and
          // never recorded; the recorded reads/writes are within-scope).
          emit_rw_hazard(w->op_idx, r->op_idx, VecRole::kEmpty);
        }
      }
    }
  }

  // V-BAND-HAZARD (finding 3(a)): every INTRA-SCOPE (non-loop-carried) edge must
  // run FORWARD in the band key — from the lower-key op to the higher-key op.
  // The band key IS the emission walk order, so an intra-scope edge that ran
  // against it would force the topo sort to invert the walk (a producer emitted
  // after its consumer, with no loop-carried role to make the read a legal
  // cross-iteration read). emit_rw_hazard/emit_waw direct every edge from the
  // lower-key op by construction, so this can only trip if a FUTURE edge deriver
  // introduced a key-inverting forced edge — we ABORT rather than let the old
  // code's silent flip hide it. (Loop-carried edges legitimately run backward in
  // the flat key — the round-phase nesting is invisible to it — so they are
  // excluded here and validated by V-LOOP instead.)
  for (const DRDep &d : flow.dep_edges) {
    if (d.loop_carried) {
      continue;
    }
    if (key_less(keys[d.to], keys[d.from])) {
      ValidatorFail("V-BAND-HAZARD: a non-loop-carried dep edge runs against the "
                    "band key (its producer sorts after its consumer with no "
                    "loop-carried role — a genuine ordering fault)");
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
  // filter) WITHIN the epoch.
  //
  // THE DOCUMENTED CHECK (finding 3(b)): a loop-carried RAW (writer W → reader
  // R, W after R in the band key) is only SOUND if the read genuinely drains
  // THIS scope BEFORE the writer refills — i.e. there is a matching intra-scope
  // WAR witness R→W over the SAME resource, and V-LINEAR has certified it
  // (R precedes W in the pinned order). Without that witness a loop-carried RAW
  // would let the refill clobber unread contents. We ASSERT the witness exists,
  // is not itself loop-carried, is in the same scope, and (belt-and-suspenders)
  // R actually precedes W in the pinned order. Abort otherwise — no silent pass.
  for (const DRDep &d : flow.dep_edges) {
    if (!d.loop_carried) {
      continue;
    }
    if (d.kind != DepKind::kRAW && d.kind != DepKind::kWAR) {
      ValidatorFail("V-LOOP: a loop-carried edge is neither RAW nor WAR");
    }
    if (d.from >= n || d.to >= n) {
      ValidatorFail("V-LOOP: loop-carried edge endpoint out of range");
    }
    // A loop-carried WAR is the witness FOR some RAW, not a carried dep itself
    // (emit_rw_hazard only ever marks the RAW loop-carried); require RAW here.
    if (d.kind != DepKind::kRAW) {
      ValidatorFail("V-LOOP: a WAR edge is marked loop-carried (only RAWs carry)");
    }
    // The drain-before-refill witness: an intra-scope WAR from the reader (d.to)
    // back to the writer (d.from) over the same resource+scope.
    bool has_witness = false;
    for (const DRDep &w : flow.dep_edges) {
      if (!w.loop_carried && w.kind == DepKind::kWAR && w.from == d.to &&
          w.to == d.from && w.scope == d.scope) {
        has_witness = true;
        break;
      }
    }
    if (!has_witness) {
      ValidatorFail("V-LOOP: a loop-carried RAW has no matching intra-scope WAR "
                    "drain-before-refill witness on the same resource");
    }
    // And that witness must be respected by the linearization (reader precedes
    // writer), the actual drain-before-refill fact.
    if (pos[d.to] >= pos[d.from]) {
      ValidatorFail("V-LOOP: the drain-before-refill witness is violated "
                    "(the reader does not precede its refilling writer)");
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
        wo.kind == DROpKind::kCommitSweep || wo.kind == DROpKind::kStateSeal ||
        wo.kind == DROpKind::kIngestFold;
    const bool ro_offlattice =
        (ro.kind == DROpKind::kNegateGate && ro.ctx == Ctx::kEager) ||
        ro.kind == DROpKind::kCommitSweep || ro.kind == DROpKind::kStateSeal ||
        ro.kind == DROpKind::kIngestFold;
    if (wo_offlattice || ro_offlattice) {
      continue;
    }
    if (ws > rs) {
      ValidatorFail("V-READY: a RAW edge reads from a strictly-higher stratum "
                    "producer (reads-lower-or-same violated)");
    }
  }

  // ===========================================================================
  // V-ORDER-CONSISTENT: the derived pinned order matches the emission driver's
  // band-key walk. (R2 family #3 retired the old part-(a) V-OLD-EQUIV(strata)
  // check — a per-op `op_stratum(op) == seeded-map[op]` comparison. With B-13's
  // separate seeded copy gone and `DeriveDRStrata` the sole writer of the
  // `*_stratum` maps that `op_stratum` reads, that comparison compared a value
  // against itself — a tautology. The strata authority is now `DeriveDRStrata`
  // itself, cross-checked against the emitter only through the byte-identical
  // golden gate; the load-bearing DR-graph checks that CAN fail on a
  // perturbation are V-READY, V-RETIRE-AFTER, V-LOOP, and the order check below.)
  // ===========================================================================
  // Emission-order consistency: the derived pinned order must be NON-
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
