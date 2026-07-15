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

#include <cstdio>
#include <cstdlib>

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

DRFlowGraph BuildDRInventory(
    ProgramImpl *impl, Context &context, Query query,
    const std::unordered_map<TABLE *, unsigned> &scc_map) {
  (void) context;

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
      flow.ops.push_back(std::move(minus));
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
      flow.ops.push_back(std::move(plus));
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
        flow.ops.push_back(std::move(arm));
      }
    }
  }

  return flow;
}

void ValidateDRInventory(
    const DRFlowGraph &flow,
    const std::vector<OldCrossoverRef> &old_crossovers,
    const std::vector<OldProductRef> &old_products,
    const std::unordered_map<TABLE *, unsigned> &old_scc_map) {

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
}

}  // namespace hyde
