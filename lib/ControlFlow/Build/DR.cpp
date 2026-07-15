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

#include <cassert>
#include <cstdio>
#include <cstdlib>
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

}  // namespace hyde
