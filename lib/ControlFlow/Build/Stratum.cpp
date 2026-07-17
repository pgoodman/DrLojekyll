// Copyright 2020, Trail of Bits. All rights reserved.

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <set>
#include <tuple>
#include <vector>

#include "Build.h"
#include "DR.h"

namespace hyde {
namespace {

// ---------------------------------------------------------------------------
// V-PRED-XCHECK (big-review finding 1): an ALWAYS-ON cross-check tying the
// DR-IR model to the surviving Emit* templates. At the emission sites where an
// Emit* chooses a membership predicate, we compare the emitter's CHOICE against
// the predicate the corresponding DR op/arm STORED. A reintroduced F18-style
// sign-keyed read then aborts at COMPILE TIME on every case, instead of relying
// on the byte-identical goldens to catch it downstream. Observation-only: it
// reads both values and either agrees (no effect) or aborts — never changes an
// emitted region, so the suite stays byte-identical.
//
// `Pred` (DR-IR vocabulary, DR.h) and `MembershipPredicate` (Program.h) are
// intentionally-distinct enums with no positional correspondence; this maps
// them by NAME and reports any DR `Pred` that has no MembershipPredicate twin as
// a mismatch (the cross-check must never silently accept an unmapped value).
static bool PredMatches(Pred dr, MembershipPredicate emitted) {
  switch (dr) {
    case Pred::kPresent: return emitted == MembershipPredicate::kPresent;
    case Pred::kInI: return emitted == MembershipPredicate::kInI;
    case Pred::kInNew: return emitted == MembershipPredicate::kInNew;
    case Pred::kSurvivesSoFar:
      return emitted == MembershipPredicate::kSurvivesSoFar;
    case Pred::kAliveAtClaim:
      return emitted == MembershipPredicate::kAliveAtClaim;
    case Pred::kInNewWithFrontier:
      return emitted == MembershipPredicate::kInNewWithFrontier;
    case Pred::kInNewSansFrontier:
      return emitted == MembershipPredicate::kInNewSansFrontier;
    case Pred::kRecursivelySupported:
      return emitted == MembershipPredicate::kRecursivelySupported;
    case Pred::kNetDeleted: return emitted == MembershipPredicate::kNetDeleted;
    case Pred::kNetAdded: return emitted == MembershipPredicate::kNetAdded;
  }
  return false;
}

[[noreturn]] static void PredXCheckFail(const char *site) {
  std::fprintf(stderr,
               "V-PRED-XCHECK: emitter-chosen membership predicate at %s "
               "differs from the DR op's stored predicate (a model/emitter "
               "divergence — e.g. a reintroduced sign-keyed read)\n",
               site);
  std::abort();
}

// Assert the emitter's `emitted` predicate agrees with the DR-stored `dr`.
static void PredXCheck(Pred dr, MembershipPredicate emitted, const char *site) {
  if (!PredMatches(dr, emitted)) {
    PredXCheckFail(site);
  }
}

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

  // R2+ SUBSTRATE (dead-but-alive): set by BranchChainOf, never read. The
  // stratum whose phase series emits this chain — retained as a landing pad for
  // an R2+ stage that reads the chain's own stratum instead of the caller's.
  // Formerly: the spec stratum lifted by the scheduling fixpoint until every
  // read the chain performs is phase-final; every fold targets a queue drained
  // at or after this stratum.
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

    const MembershipPredicate negate_pred =
        in_fixpoint ? MembershipPredicate::kInNew : MembershipPredicate::kInI;

    // V-PRED-XCHECK (finding 1) — Site 1: cross-check the emitter's negate-gate
    // choice against the DR model's AUTHORITATIVE predicate derivation
    // (`NegateGatePred`), the sole function every DR negate gate node is
    // populated from. A reintroduced F18 sign-keyed read — in EITHER the emitter
    // here or the DR model — diverges and aborts at compile time. Observation-
    // only. NOTE (residual, R3c-ii): this cross-checks against the model
    // FUNCTION, not a threaded per-arm STORED gate node — the seed/fixpoint
    // negate gate is a PlanKind::kGate sub-object on the owning seed/chain-fold
    // arm's plan spine, and EmitChainStep is invoked deep inside EmitSeedLoop's
    // branch walk (no DR arm correlated at this call depth). The eager
    // NEGATE_GATE is a standalone DROp and is separately covered by V-NEG-CTX.
    PredXCheck(NegateGatePred(in_fixpoint ? Ctx::kFixpoint : Ctx::kSeed,
                              NegateHint::kNormal),
               negate_pred, "EmitChainStep negate gate");

    OP *continuation = nullptr;
    CHECKMEMBER *const gate = BuildCheckMember(
        impl, parent, negated_table, negated_view_cols, negate_pred,
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
                           VECTOR *round_frontier = nullptr,
                           const DROp *drain_op = nullptr) {
  // V-PRED-XCHECK (finding 1) — Site 3: EmitClaimDrain's F17 dequeue gate is
  // IMPLICIT in `is_del` (CLAIM lowers to TryClaimDel C_nr<=0 / TryClaimAdd
  // total>0). Cross-check that `is_del` agrees with the DR op's STORED
  // ClaimGate — a reintroduced sign-flipped gate then aborts at compile time.
  // (This is a ClaimGate check, not a Pred one: the claim gate is a counter
  // test, never a membership predicate — see finding 2.) Observation-only.
  if (drain_op != nullptr) {
    const ClaimGate want = is_del ? ClaimGate::kDelGateCnrNonPositive
                                  : ClaimGate::kAddGateTotalPositive;
    if (drain_op->claim_gate != want) {
      PredXCheckFail("EmitClaimDrain (gate sign disagrees with DR ClaimGate)");
    }
  }
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
// Find the DR kFrontierFilter op for (table, sign, deferral). Threaded into
// EmitFrontierFilter for the Site-4 cross-check; a missing op is itself a
// model/emitter divergence (the census guarantees per-kind counts, not
// per-(table, sign, deferral) coverage — this lookup closes that gap).
static const DROp *FindFrontierFilterOp(const DRFlowGraph &dr_flow,
                                        TABLE *table, bool is_del,
                                        Deferral deferral) {
  for (const DROp &o : dr_flow.ops) {
    if (o.kind == DROpKind::kFrontierFilter && o.table_op_table == table &&
        o.table_op_sign == (is_del ? -1 : +1) && o.deferral == deferral) {
      return &o;
    }
  }
  PredXCheckFail("EmitFrontierFilter (no matching DR kFrontierFilter op)");
}

static void EmitFrontierFilter(ProgramImpl *impl, Context &context,
                               TABLE *table, bool is_del, SERIES *seq,
                               const DROp *filter_op = nullptr) {
  const MembershipPredicate emitted_pred = is_del
                                               ? MembershipPredicate::kNetDeleted
                                               : MembershipPredicate::kNetAdded;

  // V-PRED-XCHECK (finding 1) — Site 4 (P2 stage iv; closes the §12.2(B)
  // "EmitFrontierFilter reads un-cross-checked" residual): the filter's
  // membership read is IMPLICIT in `is_del`. Cross-check it against the DR
  // kFrontierFilter op's stored kFlagRead predicate — a reintroduced
  // sign-flipped or wrong-frontier read aborts at compile time.
  // Observation-only.
  if (filter_op != nullptr) {
    for (const DREffect &e : filter_op->effects) {
      if (e.kind == EffKind::kFlagRead) {
        PredXCheck(e.pred, emitted_pred, "EmitFrontierFilter");
      }
    }
  }

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
      impl->operation_regions.CreateDerived<CHECKMEMBER>(loop, emitted_pred);
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
                         SERIES *seq, const DROp *fire_op = nullptr) {
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

    // V-PRED-XCHECK (finding 1) — Site 2: cross-check every scanned side's
    // emitter-chosen matrix predicate against the predicate the matching DR
    // FIXPOINT_FIRE arm STORED (its access-plan spine's ACCESS nodes carry the
    // per-side `Pred`, populated by FixpointSamePred/kInNew in BuildDRInventory).
    // A reintroduced F18-style position/sign-keyed read here (or in the DR
    // model) aborts at compile time. Observation-only.
    if (fire_op != nullptr) {
      // The arm driving delta position p_pos, this sign.
      const DRArm *arm = nullptr;
      for (const DRArm &a : fire_op->arms) {
        if (a.delta_pos == static_cast<unsigned>(p_pos) &&
            (a.sign < 0) == is_del) {
          arm = &a;
          break;
        }
      }
      if (arm == nullptr) {
        PredXCheckFail("EmitJoinFire (no DR arm for delta position)");
      }
      // Collect the STORED per-table predicate from the arm's plan spine.
      std::unordered_map<TABLE *, Pred> stored;
      for (const PlanNode *pn = arm->body.get(); pn != nullptr;
           pn = pn->child.get()) {
        if (pn->kind == PlanKind::kAccess && pn->table != nullptr) {
          stored.emplace(pn->table, pn->pred);
        }
      }
      for (const auto &[side, emitted_pred] : scan_list) {
        TABLE *const st =
            impl->view_to_model[side]->FindAs<DataModel>()->table;
        auto it = stored.find(st);
        if (it == stored.end()) {
          PredXCheckFail("EmitJoinFire (scanned side absent from DR arm spine)");
        }
        PredXCheck(it->second, emitted_pred, "EmitJoinFire matrix");
      }
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

// ===========================================================================
// R2 FAMILY #1 — ACYCLIC-BAND LOWERING (dr → cf).
//
// `LowerDRFlow` (below) replaces the hand-coded acyclic-band emission — the old
// driver's per-stratum seeds / join section walks / crossovers / product arms /
// single-pass claim drains + immediate frontier filters. It walks the DR-IR
// flow graph (branch/join inventory, the CROSSOVER/PRODUCT_ARM DROps, and the
// single-pass CLAIM_DRAIN / immediate FRONTIER_FILTER DROps), restricted to one
// stratum's band 0 + acyclic drains/filters, and emits the SAME region trees.
// Emission ORDER is the old driver's band walk realized as a lowering default
// (B-9/B-14: tree-shape identity modulo an id-bijection — NOT the raw
// pinned_order sign-major sort, which reorders product arms). The SCC round
// shells + fixpoint fires stay on the old web (F-2: recursive family is R2 #2).
// This cut DELETED the acyclic-exclusive EmitSectionWalk / EmitCrossover /
// EmitProductArms, whose bodies are absorbed below; EmitSeedLoop / EmitHeadFold
// / EmitChainStep / EmitClaimDrain / EmitFrontierFilter SURVIVE (still called by
// the SCC path) and are reused verbatim.

// Adapt a `DRBranch` (the DR-IR's independently-derived branch inventory) to
// the `BranchChain` `EmitSeedLoop` consumes. Structurally identical (source /
// path / ends_at_join / target); `stratum` is the DR-seeded value.
static BranchChain BranchChainOf(const DRBranch &b, unsigned stratum) {
  BranchChain chain;
  chain.source = b.source;
  chain.path = b.path;
  chain.ends_at_join = b.ends_at_join;
  chain.target = b.target;
  chain.stratum = stratum;
  return chain;
}

// Emit one join section's downstream walk (absorbed from the deleted
// `EmitSectionWalk`): starting at the join view, fold at the first table (the
// join's persisted table) and otherwise continue through table-less plumbing
// into each eligible consumer edge. Each walk folds at most once (chain
// discovery terminates every chain at its first table boundary). Reuses the
// surviving `EmitHeadFold` / `EmitChainStep`.
static void LowerSectionWalk(ProgramImpl *impl, Context &context, QueryView view,
                             bool is_add, DerivClass deriv_class, OP *parent) {
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
    assert(!succ.IsNegate());
    LET *const let = impl->operation_regions.CreateDerived<LET>(par);
    par->AddRegion(let);
    OP *const step =
        EmitChainStep(impl, context, view, succ, is_add, false, let);
    LowerSectionWalk(impl, context, succ, is_add, deriv_class, step);
  }
}

// Emit one negation-crossover arm (absorbed from the deleted `EmitCrossover`,
// now driven by a CROSSOVER `DROp`). `is_add` picks the sign: the `+` arm folds
// over the negated table's net-removals frontier, the `-` arm over net-
// additions. Loops the negated frontier, scans the pred table by the shared
// key, reads the pred at `kInNew`, folds one signed count into the negate's own
// table. The DROp carries the identity (negate / negate_table / negated_table /
// pred_table / pred_view); the column machinery is re-derived from the Query.
static void LowerCrossoverArm(ProgramImpl *impl, Context &context,
                              const RecursiveSccMap &sccs, const DROp &op,
                              bool is_add, SERIES *seq) {
  assert(op.kind == DROpKind::kCrossover);
  const QueryNegate negate = *op.negate;
  const QueryView negated_view = negate.NegatedView();
  const unsigned num_key = negate.NumInputColumns();
  TABLE *const negate_table = op.negate_table;
  TABLE *const negated_table = op.negated_table;
  TABLE *const pred_table = op.pred_table;
  const QueryView pred_view = *op.pred_view;

  // Sort-unique the consumed frontier (mirrors `seed_vector`): a monotone
  // boundary's net-additions can be appended at several same-model fold sites,
  // and each frontier row must drive the crossover once.
  VECTOR *const frontier = TableDeltaVector(
      impl, context, negated_table,
      is_add ? VectorKind::kNetRemovals : VectorKind::kNetAdditions);
  VECTORUNIQUE *const unique =
      impl->operation_regions.CreateDerived<VECTORUNIQUE>(
          seq, ProgramOperation::kSortAndUniqueInductionVector);
  unique->vector.Emplace(unique, frontier);
  seq->AddRegion(unique);

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

  std::vector<QueryColumn> avail;
  {
    auto i = 0u;
    for (QueryColumn out_col : negate.NegatedColumns()) {
      const auto j = *(out_col.Index());
      const auto neg_col = negated_view.NthColumn(j);
      VAR *const key_var = loop->VariableFor(impl, neg_col);
      loop->col_id_to_var[out_col.Id()] = key_var;

      const QueryColumn pred_key_col = negate.NthInputColumn(i++);
      if (!pred_key_col.IsConstant()) {
        loop->col_id_to_var[pred_key_col.Id()] = key_var;
        avail.push_back(pred_key_col);
      }
    }
    assert(i == num_key);
    (void) num_key;
  }

  const DerivClass deriv_class =
      RuleClass(sccs, negate_table, {pred_table, negated_table});

  SERIES *const body_seq = impl->series_regions.Create(loop);
  loop->body.Emplace(loop, body_seq);

  std::vector<QueryColumn> negate_cols;
  for (auto col : negate.Columns()) {
    negate_cols.push_back(col);
  }

  const auto emit_fold = [&](REGION *parent) -> REGION * {
    UPDATECOUNT *const fold = BuildUpdateCount(
        impl, negate_table, parent, negate_cols, is_add, deriv_class);
    VECTOR *const queue = TableDeltaVector(
        impl, context, negate_table,
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

  BuildMaybeScanPartial(
      impl, pred_view, avail, pred_table, body_seq,
      [&](REGION *in_scan, bool) -> REGION * {
        std::vector<QueryColumn> pred_cols;
        for (auto col : pred_view.Columns()) {
          pred_cols.push_back(col);
        }
        return BuildCheckMember(
            impl, in_scan, pred_table, pred_cols, MembershipPredicate::kInNew,
            [&](ProgramImpl *impl_, REGION *in_check) -> REGION * {
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

// Emit one differential @product arm (absorbed from the deleted
// `EmitProductArms`, now driven by a PRODUCT_ARM `DROp`). One arm = one
// side×sign: loop the delta side's signed frontier, nested full scans of every
// other side at its position-relative membership predicate (j<i → kInNew, j>i →
// kInI, sign-independent), fold ±nonrecursive into the product's own table.
static void LowerProductArm(
    ProgramImpl *impl, Context &context, const RecursiveSccMap &sccs,
    const DROp &op,
    const std::function<VECTOR *(TABLE *, VectorKind)> &seed_vector,
    SERIES *seq) {
  assert(op.kind == DROpKind::kProductArm);
  const QueryJoin join = QueryJoin::From(*op.product_view);
  TABLE *const product_table = op.product_table;
  const size_t num_sides = op.side_tables.size();
  const size_t i = op.side_index;
  const bool is_add = (op.product_sign > 0);

  std::vector<QueryView> sides;
  for (QueryView side : join.JoinedViews()) {
    sides.push_back(side);
  }
  assert(sides.size() == num_sides);

  const DerivClass deriv_class =
      RuleClass(sccs, product_table, op.side_tables);
  assert(deriv_class == DerivClass::kNonRecursive);

  std::vector<QueryColumn> product_cols;
  for (auto col : op.product_view->Columns()) {
    product_cols.push_back(col);
  }

  VECTOR *const frontier = seed_vector(
      op.side_tables[i],
      is_add ? VectorKind::kNetAdditions : VectorKind::kNetRemovals);

  VECTORLOOP *const loop = impl->operation_regions.CreateDerived<VECTORLOOP>(
      impl->next_id++, seq, ProgramOperation::kLoopOverInductionVector);
  seq->AddRegion(loop);
  loop->vector.Emplace(loop, frontier);
  for (auto col : sides[i].Columns()) {
    VAR *const var = loop->defined_vars.Create(impl->next_id++,
                                               VariableRole::kVectorVariable);
    var->query_column = col;
    loop->col_id_to_var.emplace(col.Id(), var);
  }

  const auto emit_fold = [&](SERIES *inner_seq) {
    join.ForEachUse([&](QueryColumn in_col, InputColumnRole role,
                        std::optional<QueryColumn> out_col) {
      if (out_col) {
        assert(role == InputColumnRole::kJoinNonPivot);
        (void) role;
        inner_seq->col_id_to_var.emplace(
            out_col->Id(), inner_seq->VariableFor(impl, in_col));
      }
    });

    UPDATECOUNT *const fold = BuildUpdateCount(
        impl, product_table, inner_seq, product_cols, is_add, deriv_class);
    inner_seq->AddRegion(fold);

    VECTOR *const queue = TableDeltaVector(
        impl, context, product_table,
        is_add ? VectorKind::kAddQueue : VectorKind::kDeleteQueue);
    VECTORAPPEND *const append =
        impl->operation_regions.CreateDerived<VECTORAPPEND>(
            fold, ProgramOperation::kAppendToInductionVector);
    append->vector.Emplace(append, queue);
    for (auto col : product_cols) {
      append->tuple_vars.AddUse(fold->VariableFor(impl, col));
    }
    fold->body.Emplace(fold, append);
  };

  std::vector<size_t> scan_positions;
  for (size_t j = 0u; j < num_sides; ++j) {
    if (j != i) {
      scan_positions.push_back(j);
    }
  }

  std::function<void(size_t, SERIES *)> scan_next =
      [&](size_t idx, SERIES *parent_seq) {
        if (idx == scan_positions.size()) {
          emit_fold(parent_seq);
          return;
        }
        const size_t j = scan_positions[idx];
        const QueryView side = sides[j];
        TABLE *const side_table = op.side_tables[j];
        const MembershipPredicate pred = (j < i)
                                             ? MembershipPredicate::kInNew
                                             : MembershipPredicate::kInI;

        std::vector<QueryColumn> avail;  // Zero bound columns: full scan.
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
}

// Lower ONE GROUP_UPDATE (spec §2.2): fold the summarized input's net removal
// / net addition frontiers into the view's StateCell store, then emit the
// occupancy-generalized one-net-pair into the aggregate's own DiffTable. The
// whole two-band body is written by codegen from the `GROUPUPDATE` region's
// carried data; here we just mint the region, wire its frontier / queue vecs
// and the group/summary projection positions. Mirrors LowerProductArm's single
// frontier-vec-consumer idiom, but with NO position-keyed partner read (one
// input source), and the emit_touched pair rides the ordinary counter± + queue
// machinery through the agg DiffTable's downstream acyclic tail.
static void LowerGroupUpdate(
    ProgramImpl *impl, Context &context, const DRFlowGraph &dr_flow,
    const DROp &op,
    const std::function<VECTOR *(TABLE *, VectorKind)> &seed_vector,
    SERIES *seq) {
  assert(op.kind == DROpKind::kGroupUpdate);
  TABLE *const agg_table = op.agg_table;
  const QueryView input = *op.input_view;

  DataModel *const input_model =
      impl->view_to_model[input]->FindAs<DataModel>();
  TABLE *const input_table = input_model->table;
  assert(input_table != nullptr);

  // The input frontier vecs (the input stratum's FRONTIER_FILTER outputs, E1
  // RAW) — shared/sort-uniqued via `seed_vector`, exactly like a product arm.
  VECTOR *const neg_front =
      seed_vector(input_table, VectorKind::kNetRemovals);
  VECTOR *const pos_front =
      seed_vector(input_table, VectorKind::kNetAdditions);

  // The agg table's delete / add queues (emit_touched appends, E3).
  VECTOR *const del_queue =
      TableDeltaVector(impl, context, agg_table, VectorKind::kDeleteQueue);
  VECTOR *const add_queue =
      TableDeltaVector(impl, context, agg_table, VectorKind::kAddQueue);

  // Map each group/summary column (input-view space, spec §5) to its position
  // in the input frontier row (== `input.Columns()` order).
  std::vector<QueryColumn> input_cols;
  for (auto col : input.Columns()) {
    input_cols.push_back(col);
  }
  const auto pos_of = [&](QueryColumn c) -> unsigned {
    for (unsigned i = 0u; i < input_cols.size(); ++i) {
      if (input_cols[i].Id() == c.Id()) {
        return i;
      }
    }
    assert(false && "group/summary column not found in input frontier row");
    return 0u;
  };

  GROUPUPDATE *const gu = impl->operation_regions.CreateDerived<GROUPUPDATE>(
      seq, op.statecell_id, op.algebra == Algebra::kInvertible);
  seq->AddRegion(gu);
  gu->neg_frontier.Emplace(gu, neg_front);
  gu->pos_frontier.Emplace(gu, pos_front);
  gu->del_queue.Emplace(gu, del_queue);
  gu->add_queue.Emplace(gu, add_queue);
  gu->agg_table.Emplace(gu, agg_table);
  for (auto col : op.group_cols) {
    gu->group_positions.push_back(pos_of(col));
  }
  for (auto col : op.summary_cols) {
    gu->summary_positions.push_back(pos_of(col));
  }
}

// Lower ONE stratum's ACYCLIC band from the DR-IR flow graph into `stratum_seq`:
// seeds (SEED_FOLD branches via `EmitSeedLoop`), join section walks, crossovers
// (CROSSOVER DROps), product arms (PRODUCT_ARM DROps), then acyclic single-pass
// claim drains + immediate frontier filters. The band order reproduces the old
// driver's exact sibling order (identity gate B-14). `seed_vector` is the per-
// stratum sort-unique-once frontier provisioning the caller shares with the SCC
// band. This is the R2 family-#1 cutover of the acyclic band; the SCC round
// shells stay on the old web (caller continues below).
//
// `join_pivots` maps each join view to the ONE shared pivot VECTOR* the old
// discovery already minted (Stratum.cpp `joins` loop) — reused verbatim so no
// SECOND pivot vec is allocated here (VectorFor is NOT memoized: a fresh call
// would mint a duplicate vec at the wrong id, shifting every downstream id and
// breaking identity — d5_recursive_negate's join is the witness).
static void LowerDRFlow(ProgramImpl *impl, Context &context,
                        const RecursiveSccMap &recursive_sccs,
                        const DRFlowGraph &dr_flow, unsigned stratum,
                        const std::function<VECTOR *(TABLE *, VectorKind)>
                            &seed_vector,
                        const std::unordered_map<QueryView, VECTOR *>
                            &join_pivots,
                        SERIES *stratum_seq) {

  const auto all_sides_same_scc = [&](QueryView join_view) -> bool {
    DataModel *const jm = impl->view_to_model[join_view]->FindAs<DataModel>();
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
  const auto same_scc = [&](TABLE *a, TABLE *b) -> bool {
    const auto ga = RecursiveSCC(recursive_sccs, a);
    return ga.has_value() && ga == RecursiveSCC(recursive_sccs, b);
  };
  const auto is_recursive = [&](TABLE *table) -> bool {
    return RecursiveSCC(recursive_sccs, table).has_value();
  };

  // (1) SEEDS: every branch at this stratum, per available sign — reproducing
  // the old driver's :2025-2093 loop (branch order, `-` then `+`), with the
  // three suppressions (same-SCC internal projection, all-same-SCC join,
  // pivot-terminal). Uses the surviving `EmitSeedLoop`.
  for (unsigned bi = 0u; bi < dr_flow.branches.size(); ++bi) {
    const DRBranch &branch = dr_flow.branches[bi];
    if (dr_flow.branch_stratum[bi] != stratum) {
      continue;
    }
    if (!branch.ends_at_join && is_recursive(branch.target) &&
        same_scc(branch.target, branch.source)) {
      continue;  // fires only inside the SCC claim round (CHAIN_FOLD)
    }
    if (branch.ends_at_join && all_sides_same_scc(branch.path.back())) {
      continue;  // all-same-SCC join has no seed
    }

    const BranchChain chain = BranchChainOf(branch, stratum);
    TABLE *const source = branch.source;

    VECTOR *join_pivot_vec = nullptr;
    DerivClass branch_class = DerivClass::kNonRecursive;
    if (branch.ends_at_join) {
      join_pivot_vec = join_pivots.at(branch.path.back());
    } else {
      branch_class = RuleClass(recursive_sccs, branch.target, {source});
    }

    if (TableIsDifferential(source) &&
        !TableIsInductionOwned(context, source)) {
      EmitSeedLoop(impl, context, chain, false /* is_add */, branch_class,
                   seed_vector(source, VectorKind::kNetRemovals),
                   join_pivot_vec, false /* in_fixpoint */, stratum_seq);
    }
    EmitSeedLoop(impl, context, chain, true /* is_add */, branch_class,
                 seed_vector(source, VectorKind::kNetAdditions),
                 join_pivot_vec, false /* in_fixpoint */, stratum_seq);
  }

  // (2) JOIN section walks: every join at this stratum (old driver :2098-2149).
  // An all-same-SCC join's seed is suppressed (its fire runs in the SCC loop).
  for (const DRJoin &dr_join : dr_flow.joins) {
    const QueryView join_view = dr_join.join_view;
    auto js = dr_flow.join_stratum.find(join_view);
    if (js == dr_flow.join_stratum.end() || js->second != stratum) {
      continue;
    }
    if (all_sides_same_scc(join_view)) {
      continue;
    }

    VECTOR *const pivot_vec = join_pivots.at(join_view);

    VECTORUNIQUE *const unique =
        impl->operation_regions.CreateDerived<VECTORUNIQUE>(
            stratum_seq, ProgramOperation::kSortAndUniquePivotVector);
    unique->vector.Emplace(unique, pivot_vec);
    stratum_seq->AddRegion(unique);

    auto [join, cmp] = BuildJoin(impl, QueryJoin::From(join_view), pivot_vec,
                                 stratum_seq, true /* for_delta */);
    assert(cmp == nullptr);
    (void) cmp;

    DataModel *const join_model =
        impl->view_to_model[join_view]->FindAs<DataModel>();
    std::vector<TABLE *> side_tables;
    for (QueryView side : QueryJoin::From(join_view).JoinedViews()) {
      side_tables.push_back(
          impl->view_to_model[side]->FindAs<DataModel>()->table);
    }
    const DerivClass join_class =
        RuleClass(recursive_sccs, join_model->table, side_tables);

    LET *const added = impl->operation_regions.CreateDerived<LET>(join);
    join->added_body.Emplace(join, added);
    LowerSectionWalk(impl, context, join_view, true /* is_add */, join_class,
                     added);

    LET *const removed = impl->operation_regions.CreateDerived<LET>(join);
    join->removed_body.Emplace(join, removed);
    LowerSectionWalk(impl, context, join_view, false /* is_add */, join_class,
                     removed);
  }

  // (3) CROSSOVERS: the CROSSOVER DROp pairs at this stratum (old driver
  // :2157-2178). Construction order (== old `crossovers` order); `-` arm then
  // `+` arm (the `+` arm is a distinct DROp, minted iff negated differential).
  for (const DROp *op : dr_flow.Crossovers()) {
    if (!op->negate.has_value()) {
      continue;
    }
    auto xs = dr_flow.crossover_stratum.find(QueryView(*op->negate));
    if (xs == dr_flow.crossover_stratum.end() || xs->second != stratum) {
      continue;
    }
    LowerCrossoverArm(impl, context, recursive_sccs, *op,
                      op->crossover_sign > 0 /* is_add */, stratum_seq);
  }

  // (4) PRODUCT ARMS: the PRODUCT_ARM DROps at this stratum (old driver
  // :2186-2198). Construction order (side ascending, `+` then `-` per side).
  for (const DROp *op : dr_flow.ProductArms()) {
    if (!op->product_view.has_value()) {
      continue;
    }
    auto ps = dr_flow.product_stratum.find(*op->product_view);
    if (ps == dr_flow.product_stratum.end() || ps->second != stratum) {
      continue;
    }
    LowerProductArm(impl, context, recursive_sccs, *op, seed_vector,
                    stratum_seq);
  }

  // (4b) GROUP_UPDATES at this stratum (spec §2.2). The op sits at its agg
  // view's lifted stratum (E1: strictly above the input's frontier filters).
  // Emitted after products, before the acyclic drains — the emit_touched pair
  // it appends to the agg table's queues is drained by that table's own
  // CLAIM_DRAIN below (E3 seed-before-drain), which for the corpus double-nest
  // lives at a HIGHER stratum, so the ordering is a cross-stratum RAW handled
  // by the strata lift.
  for (const DROp *op : dr_flow.GroupUpdates()) {
    if (!op->agg_view.has_value()) {
      continue;
    }
    auto gs = dr_flow.group_update_stratum.find(*op->agg_view);
    if (gs == dr_flow.group_update_stratum.end() || gs->second != stratum) {
      continue;
    }
    LowerGroupUpdate(impl, context, dr_flow, *op, seed_vector, stratum_seq);
  }

  // (5) ACYCLIC claim drains + immediate frontier filters, per single-pass
  // CLAIM_DRAIN table (old driver :2227-2234). The DR ops are iterated to pick
  // the acyclic tables at this stratum, in construction order (== table order);
  // each table gets del-drain, add-drain, del-filter, add-filter. Uses the
  // surviving `EmitClaimDrain` / `EmitFrontierFilter`.
  for (const DROp &op : dr_flow.ops) {
    if (op.kind != DROpKind::kClaimDrain ||
        op.claim_form != ClaimForm::kSinglePass || op.table_op_sign != -1) {
      continue;  // once per table, keyed on the del single-pass drain
    }
    TABLE *const table = op.table_op_table;
    if (is_recursive(table)) {
      continue;
    }
    auto ds = dr_flow.drain_stratum.find(table);
    if (ds == dr_flow.drain_stratum.end() || ds->second != stratum) {
      continue;
    }
    // V-PRED-XCHECK (finding 1, Site 3): thread the DR drain ops. This loop is
    // keyed on the DEL single-pass drain (`op`); find the matching ADD drain for
    // the add call so BOTH signs cross-check against their own stored ClaimGate.
    const DROp *add_op = nullptr;
    for (const DROp &o : dr_flow.ops) {
      if (o.kind == DROpKind::kClaimDrain &&
          o.claim_form == ClaimForm::kSinglePass && o.table_op_sign == +1 &&
          o.table_op_table == table) {
        add_op = &o;
        break;
      }
    }
    EmitClaimDrain(impl, context, table, true /* is_del */, stratum_seq,
                   nullptr, &op);
    EmitClaimDrain(impl, context, table, false /* is_del */, stratum_seq,
                   nullptr, add_op);
    EmitFrontierFilter(
        impl, context, table, true /* is_del */, stratum_seq,
        FindFrontierFilterOp(dr_flow, table, true, Deferral::kImmediate));
    EmitFrontierFilter(
        impl, context, table, false /* is_del */, stratum_seq,
        FindFrontierFilterOp(dr_flow, table, false, Deferral::kImmediate));
  }
}

// ===========================================================================
// R2 FAMILY #2 — SCC/RECURSIVE-BAND LOWERING (dr → cf).
//
// `LowerDRRounds` (below) replaces the hand-coded SCC driver band — the old
// driver's `build_claim_round_loop` closure + the two-call del/add loop
// structure + REDERIVE + the two deferred frontier filters (Stratum.cpp
// :2140-2321). It lowers the per-SCC×phase FIXPOINT_ROUND shells the DR-IR
// constructs (`DRFlowGraph::rounds`, DR.cpp:1423-1468), driving the surviving
// Emit* primitives (EmitClaimDrain in-round form, EmitJoinFire, EmitSeedLoop
// in-fixpoint form for CHAIN_FOLD, EmitRetireFrontier, EmitRederive,
// EmitFrontierFilter) 1:1 from the round's DR ops.
//
// Structure reproduced verbatim (the lowering DEFAULT, not the sign-major
// pinned order — family #1's lesson, DeltaRelationalIR.md §8): per SCC group
// drained at this stratum, an OVERDELETE INDUCTION (round body: clear each
// Δ_D, in-round del claim drains, del fires, del chain-folds, del retires;
// output: REDERIVE per table) then an INSERT INDUCTION (mirror on the add
// side; output: BOTH deferred FRONTIER_FILTERs, E-17). The DR round shells
// decide WHICH groups/tables/joins/folds; the Emit* primitives do the codegen.
//
// VECTOR reuse discipline (the (table,kind)→VECTOR* bridge): every round
// frontier / queue / set is fetched via `TableDeltaVector` (memoized in
// `context.table_delta_vecs`), so the vecs the old driver minted are REUSED,
// never re-minted (VectorFor is NOT memoized — a fresh call would shift every
// downstream id and break identity; §8 family-#1 hazard).

// Recover the SCC table set of a round from its test vecs (Δ frontiers), in
// vec order == `impl->tables` order (the DR shell minted them so — DR.cpp:1456)
// == the old driver's `scc_tables` order (phase_table_order filtered).
static std::vector<TABLE *> RoundTables(const DRFlowGraph &dr_flow,
                                        const DRRound &round) {
  std::vector<TABLE *> tables;
  for (unsigned vi : round.test_vecs) {
    tables.push_back(dr_flow.vecs[vi].debug_table);
  }
  return tables;
}

// Lower ONE FixpointRound region body into an INDUCTION whose cyclic region is
// the round body (absorbed from the deleted `build_claim_round_loop`): per
// round — clear each SCC table's Δ; in-round claim-drain each SCC table (dual-
// append + B-7 queue clear); fire the recursive joins DELTA over Δ; re-fire the
// same-SCC internal projections (CHAIN_FOLD ≡ in-fixpoint EmitSeedLoop); retire
// each Δ. Δ-emptiness break is the INDUCTION's own maintained-vector discipline
// (loop->vectors == the round frontiers). Driven by the round's DR ops.
static INDUCTION *LowerRoundBody(
    ProgramImpl *impl, Context &context, const RecursiveSccMap &recursive_sccs,
    const DRFlowGraph &dr_flow, const DRRound &round,
    const std::vector<TABLE *> &scc_tables, SERIES *out_seq) {
  const bool is_del = (round.phase == RoundPhase::kOverdelete);
  const VectorKind frontier_kind = is_del ? VectorKind::kClaimedDeleteFrontier
                                          : VectorKind::kClaimedAddFrontier;

  INDUCTION *const loop = impl->induction_regions.Create(impl, out_seq);
  loop->parent = out_seq;
  out_seq->AddRegion(loop);

  PARALLEL *const round_par = impl->parallel_regions.Create(loop);
  loop->cyclic_region.Emplace(loop, round_par);
  SERIES *const round_seq = impl->series_regions.Create(round_par);
  round_par->AddRegion(round_seq);

  // Round-start CLEAR of each Δ frontier + register it as a maintained vector
  // (the fixpoint-test vec set — the Δ-emptiness break condition). These are the
  // DRRound `test_vecs`, recovered as SCC tables (RoundTables), each fetched via
  // the memoized `TableDeltaVector`.
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

  // In-round CLAIM_DRAIN per SCC table (the round's kClaimDrain in-round ops —
  // one del, one add per SCC table; we emit the phase sign here). Dual-append +
  // B-7 clear live inside EmitClaimDrain's round_frontier path.
  for (TABLE *table : scc_tables) {
    VECTOR *const round_frontier =
        TableDeltaVector(impl, context, table, frontier_kind);
    // V-PRED-XCHECK (finding 1, Site 3): the loop iterates tables, not DR ops,
    // so look up this table's in-round drain op of the round's sign+group and
    // thread it — both signs then cross-check against their stored ClaimGate.
    const int want_sign = is_del ? -1 : +1;
    const DROp *drain_op = nullptr;
    for (const DROp &o : dr_flow.ops) {
      if (o.kind == DROpKind::kClaimDrain &&
          o.claim_form == ClaimForm::kInRound && o.table_op_table == table &&
          o.table_op_sign == want_sign && o.scc_group == round.scc_group) {
        drain_op = &o;
        break;
      }
    }
    EmitClaimDrain(impl, context, table, is_del, round_seq, round_frontier,
                   drain_op);
  }

  // FIXPOINT_FIRE: re-fire every recursive JOIN in this SCC group DELTA over Δ.
  // Driven by the group's kFixpointFire ops of this sign (in flow.joins order ==
  // discovery `joins` order, V-OLD-EQUIV-proven). Each op carries `fire_join`;
  // EmitJoinFire reads only `emission.join_view`, so a lightweight JoinEmission
  // suffices.
  for (const DROp *op : dr_flow.OpsOfKind(DROpKind::kFixpointFire)) {
    // Family #3 stamps `scc_group` onto each FIXPOINT_FIRE (DeriveDRStrata), so
    // the round selects its own group's fires directly (the family-#2
    // RecursiveSCC re-derivation is retired).
    if (op->fire_sign != (is_del ? -1 : +1) ||
        op->scc_group != round.scc_group) {
      continue;
    }
    const JoinEmission emission(*op->fire_join, nullptr /* pivot */, 0u);
    // Thread the DR op for V-PRED-XCHECK (finding 1, Site 2): EmitJoinFire's
    // matrix choice is cross-checked against this op's stored arm predicates.
    EmitJoinFire(impl, context, recursive_sccs, emission, is_del, round_seq, op);
  }

  // CHAIN_FOLD: re-fire the same-SCC internal projection seeds DELTA over the
  // source table's Δ (≡ the old in-fixpoint EmitSeedLoop). Driven by the group's
  // kChainFold ops of this sign (in flow.branches order == discovery `branches`
  // order). `chain_branch` indexes `dr_flow.branches`; convert to a BranchChain.
  for (const DROp *op : dr_flow.OpsOfKind(DROpKind::kChainFold)) {
    if (op->chain_sign != (is_del ? -1 : +1) ||
        op->scc_group != round.scc_group) {
      continue;
    }
    const BranchChain chain =
        BranchChainOf(dr_flow.branches[op->chain_branch], 0u);
    EmitSeedLoop(impl, context, chain, !is_del /* is_add */,
                 DerivClass::kRecursive,
                 TableDeltaVector(impl, context, op->chain_source, frontier_kind),
                 nullptr, true /* in_fixpoint */, round_seq);
  }

  // RETIRE: clear each Δ's same-round bit at the round-body tail (kRetire ops,
  // one per SCC table this sign). Table order == scc_tables (== retire-op
  // construction order).
  for (TABLE *table : scc_tables) {
    VECTOR *const round_frontier =
        TableDeltaVector(impl, context, table, frontier_kind);
    EmitRetireFrontier(impl, table, is_del, round_frontier, round_seq);
  }
  return loop;
}

// Lower every FixpointRound region shell whose SCC group is drained at this
// `stratum` (absorbed from the deleted SCC driver band). Reproduces the old
// two-phase structure: OVERDELETE round → REDERIVE (its output) → INSERT round
// → BOTH deferred frontier filters (its output), E-17. The DRRound shells
// enumerate the groups/tables; `LowerRoundBody` drives the surviving Emit*.
static void LowerDRRounds(ProgramImpl *impl, Context &context,
                          const RecursiveSccMap &recursive_sccs,
                          const DRFlowGraph &dr_flow, unsigned stratum,
                          SERIES *stratum_seq) {
  // The round shells come in (group ascending, OVERDELETE then INSERT) pairs
  // (DR.cpp:1442-1467). Walk the OVERDELETE rounds; for each whose group drains
  // at this stratum, emit the OD loop + REDERIVE, then find its INSERT sibling
  // and emit the add loop + deferred filters. (A group has exactly one OD + one
  // INSERT round; both share the SCC table set, so both drain at one stratum.)
  const auto find_round = [&](unsigned g, RoundPhase p) -> const DRRound * {
    for (const DRRound &r : dr_flow.rounds) {
      if (r.scc_group == g && r.phase == p) {
        return &r;
      }
    }
    return nullptr;
  };

  for (const DRRound &del_round : dr_flow.rounds) {
    if (del_round.phase != RoundPhase::kOverdelete) {
      continue;
    }

    // Skip a round whose SCC group is NOT drained at this stratum. Family #3
    // reads the drain stratum stamped on the round shell (`DeriveDRStrata`),
    // not the discovery `drain_stratum` map.
    if (del_round.drain_stratum != stratum) {
      continue;
    }
    const std::vector<TABLE *> scc_tables = RoundTables(dr_flow, del_round);

    // OVERDELETE (§5.2): the del-side claim-round fixpoint, then — in its output
    // region, after quiescence — REDERIVE (every overdeleted row still
    // recursively supported re-enters via the add queue). The del net-removal
    // frontier is deliberately NOT built here (spec §5.0: both signed frontiers
    // are consolidated in the INSERT output, after `kAdd` is final, so a row
    // overdeleted then re-added leaks into neither).
    INDUCTION *const del_loop = LowerRoundBody(
        impl, context, recursive_sccs, dr_flow, del_round, scc_tables,
        stratum_seq);
    SERIES *const del_output = impl->series_regions.Create(del_loop);
    del_loop->output_region.Emplace(del_loop, del_output);
    for (TABLE *table : scc_tables) {  // REDERIVE band (kRederive output ops).
      EmitRederive(impl, context, table, del_output);
    }

    // INSERT (§5.3): the add-side claim-round fixpoint (mirror), draining the
    // add queue — the batch's `+` seeds AND REDERIVE's output.
    const DRRound *const add_round =
        find_round(del_round.scc_group, RoundPhase::kInsert);
    assert(add_round != nullptr);
    INDUCTION *const add_loop = LowerRoundBody(
        impl, context, recursive_sccs, dr_flow, *add_round, scc_tables,
        stratum_seq);
    SERIES *const add_output = impl->series_regions.Create(add_loop);
    add_loop->output_region.Emplace(add_loop, add_output);

    // BUILDFRONTIERS (§5.0 / E-17): BOTH consolidated signed net frontiers are
    // built HERE, after INSERT has quiesced, so `kAdd` (INSERT) and `kDel`
    // (OVERDELETE) are both final. These are the deferred FRONTIER_FILTERs
    // hosted in the INSERT round's output (V-DEFER); del filter band then add
    // filter band (the deferred kFrontierFilter output ops).
    for (TABLE *table : scc_tables) {
      EmitFrontierFilter(
          impl, context, table, true /* is_del */, add_output,
          FindFrontierFilterOp(dr_flow, table, true, Deferral::kAddLoopOutput));
    }
    for (TABLE *table : scc_tables) {
      EmitFrontierFilter(impl, context, table, false /* is_del */, add_output,
                         FindFrontierFilterOp(dr_flow, table, false,
                                              Deferral::kAddLoopOutput));
    }
  }
}

}  // namespace

// INGEST-FOLD LOWERING (dr → cf). P2 CUTOVER (family #4, stage 1) for the
// DELETION-CAPABLE folds; §6 (subgraphs/demand P1) EXTENDS it to the MONOTONE
// table-bearing fold — the two are now one lowering, driven entirely by the
// DROp payload (sign, is_explicit, role, table — never re-derived from the
// Query here, the F1 discipline).
//
// TWO LEGAL SHAPES (op.ingest_is_explicit == op.ingest_stage1, R1e invariant):
//   * DELETION-CAPABLE (stage-1): explicit ±, role kAddQueue/kDeleteQueue.
//     Emits VECTORLOOP → EXPLICIT UPDATECOUNT± (the message-support-bit toggle;
//     its zero crossing parks the row) → VECTORAPPEND into the receive table's
//     add/delete queue (the memoized TableDeltaVector — no new vector id;
//     VecRole→VectorKind mapped only here). The fold body is the bare queue
//     append; the caller discards the return.
//   * MONOTONE (§6): non-explicit +1, role kNetAddition/kEmpty. Emits
//     VECTORLOOP → NON-EXPLICIT UPDATECOUNT+ (`update-count +nonrecursive`)
//     with an EMPTY body — the HOLE. NO queue/net-additions append is emitted
//     here: the net-additions append is the DESCENT's job (Build.cpp:886-893),
//     placed at its actual fold-nesting site. The caller threads the returned
//     UPDATECOUNT as `next_parent` so the descent Emplaces the publish /
//     net-additions subtree INTO fold->body.
//
// Called from `ExtendEagerProcedure` at the ORIGINAL walk position so the
// VECTORLOOP/VAR ids occupy their pre-cutover / pre-§6 slots in the shared
// `impl->next_id` stream (byte-identity; see MakeStageOneIngestFolds /
// MakeMonotoneIngestFold docs). RETURNS the UPDATECOUNT fold body cursor (the
// exact analog of the hand-coded arm's `next_parent = insert`, E-34 (iii)).
//
// Records the emitted fold's (table, sign, is_explicit, role, message) 5-tuple
// into `context.emitted_ingest_folds` for the V-INGEST-XCHECK Site 5
// coverage/payload check (a closing pass in BuildStratumPhases — the flow does
// not exist at walk time, the §12.6 authority shape).
OP *LowerIngestFold(ProgramImpl *impl, Context &context, const DROp &op,
                    PARALLEL *parent, VECTOR *loop_vec) {
  assert(op.kind == DROpKind::kIngestFold);
  // The two disjoint legal shapes (§6): deletion-capable (stage-1) folds are
  // explicit with a queue role; monotone folds are non-explicit with a
  // net-additions/empty role. The R1e invariant ties stage1 to is_explicit.
  assert(op.ingest_is_explicit == op.ingest_stage1);  // R1e (DR.cpp)
  assert(op.ingest_is_explicit
             ? (op.ingest_role == VecRole::kAddQueue ||
                op.ingest_role == VecRole::kDeleteQueue)
             : (op.ingest_role == VecRole::kNetAddition ||
                op.ingest_role == VecRole::kEmpty));
  assert(op.ingest_sign == 1 || op.ingest_sign == -1);
  // The sign is tied to the shape: an explicit fold's sign matches its queue
  // role; a monotone fold is always the +1 arrival fold.
  assert(op.ingest_is_explicit
             ? ((op.ingest_sign > 0) == (op.ingest_role == VecRole::kAddQueue))
             : op.ingest_sign == 1);

  const QueryView receive = *op.ingest_receive;
  TABLE *const table = op.ingest_table;
  assert(table != nullptr);
  const bool is_add = 0 < op.ingest_sign;

  // F1 discipline: the fold's derivation class is CONSUMED from the op payload
  // (both authorities push the kCounter effect first), never re-derived here.
  // Every reachable receive is kNonRecursive today (nothing feeds a receive);
  // the klass rides the Site-5 key below, so payload drift aborts per-compile.
  assert(!op.effects.empty() && op.effects.front().kind == EffKind::kCounter);
  const DerivClass klass = op.effects.front().klass;

  const auto loop = impl->operation_regions.CreateDerived<VECTORLOOP>(
      impl->next_id++, parent, ProgramOperation::kLoopOverInputVector);
  parent->AddRegion(loop);
  loop->vector.Emplace(loop, loop_vec);

  UPDATECOUNT *const fold = impl->operation_regions.CreateDerived<UPDATECOUNT>(
      loop, is_add, klass, op.ingest_is_explicit);
  fold->table.Emplace(fold, table);
  loop->body.Emplace(loop, fold);

  for (auto col : receive.Columns()) {
    VAR *const var = loop->defined_vars.Create(impl->next_id++,
                                               VariableRole::kVectorVariable);
    var->query_column = col;
    loop->col_id_to_var.emplace(col.Id(), var);
    fold->col_values.AddUse(var);
  }

  // The queue append is the DELETION-CAPABLE fold's own body. A MONOTONE fold
  // emits NOTHING here — the descent (BuildEagerInsertionRegions) owns the
  // net-additions append at its actual fold-nesting site (§2.4), filling the
  // returned UPDATECOUNT's hole.
  if (op.ingest_is_explicit) {
    VECTOR *const queue = TableDeltaVector(
        impl, context, table,
        op.ingest_role == VecRole::kAddQueue ? VectorKind::kAddQueue
                                             : VectorKind::kDeleteQueue);
    VECTORAPPEND *const append =
        impl->operation_regions.CreateDerived<VECTORAPPEND>(
            fold, ProgramOperation::kAppendToInductionVector);
    append->vector.Emplace(append, queue);
    for (auto col : receive.Columns()) {
      append->tuple_vars.AddUse(fold->VariableFor(impl, col));
    }
    fold->body.Emplace(fold, append);
  }

  // V-INGEST-XCHECK Site 5: record the emitted fold. `table` and `klass` are
  // read back off the constructed node (what the emission actually holds);
  // sign/is_explicit/role/message identify the op it lowered from.
  context.emitted_ingest_folds.push_back(
      {static_cast<const void *>(fold->table.get()), op.ingest_sign,
       op.ingest_is_explicit, static_cast<uint8_t>(op.ingest_role),
       static_cast<uint8_t>(fold->deriv_class), op.ingest_message->Id()});

  return fold;
}

// R2 FAMILY #3 — COMMIT-SWEEP BAND LOWERING (dr → cf).
//
// Replaces the hand-coded end-of-batch sweep band (the deleted
// Procedure.cpp:307-345 loop): one COMMITSWEEP per differential table (with its
// @differential-transmit message attached iff publish_target), one Seal
// COMMITSWEEP per monotone boundary table with a delta-vec entry. The DR-IR
// `kCommitSweep` ops (DR.cpp:1288-1334) are minted in `impl->tables` order — the
// exact order the old loop walked — so the sibling order is the lowering default
// (B-9/B-14 tree-shape identity). Emitted into the SAME `seq` the old band used
// (PublishDifferentialMessageVectors' outer series, AFTER the monotone
// iter_par), so the region-tree nesting is unchanged.
void LowerCommitSweeps(ProgramImpl *impl, Context &context,
                       const DRFlowGraph &dr_flow, SERIES *seq) {
  // Recover each swept table's @differential transmit message (the sweep op
  // carries only `publish_target`; the message identity lives on
  // `commit_published_view`). Mirror Procedure.cpp:311-318.
  std::unordered_map<TABLE *, ParsedMessage> table_to_message;
  for (const auto &[message, transmit] : context.commit_published_view) {
    const auto pred = transmit.Predecessors()[0];
    DataModel *const pm = impl->view_to_model[pred]->FindAs<DataModel>();
    assert(pm->table != nullptr);
    table_to_message.emplace(pm->table, message);
  }

  // R3: which agg table each STATE_SEAL seals (V-AGG-SOLE: one per table).
  std::unordered_map<TABLE *, unsigned> table_to_statecell;
  for (const DROp *seal : dr_flow.StateSeals()) {
    table_to_statecell.emplace(seal->agg_table, seal->statecell_id);
  }

  for (const DROp &op : dr_flow.ops) {
    if (op.kind != DROpKind::kCommitSweep) {
      continue;
    }
    TABLE *const table = op.table_op_table;
    COMMITSWEEP *const sweep =
        impl->operation_regions.CreateDerived<COMMITSWEEP>(seq);
    sweep->table.Emplace(sweep, table);
    if (op.sweep_flavor == SweepFlavor::kDifferential && op.publish_target) {
      if (auto it = table_to_message.find(table);
          it != table_to_message.end()) {
        sweep->message.emplace(it->second);
      }
    }
    // STATE_SEAL rides the commit-sweep tail (spec §2.3 E5): sealed := working
    // for the agg table's StateCell, AFTER emit_touched read working as `new`.
    if (auto it = table_to_statecell.find(table);
        it != table_to_statecell.end()) {
      sweep->seal_statecell_id = it->second;
    }
    seq->AddRegion(sweep);
  }
}

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

  // (Both LINEAR (exactly one same-SCC join side) and NONLINEAR
  // (`tc(F,T):tc(F,X),tc(X,T)`, ≥2 same-SCC join sides) recursions are handled
  // by the claim-round fire's k-position JOIN emission (`EmitJoinFire`), which
  // folds the round-relative predicate matrix
  // (`SurvivesSoFar`/`AliveAtClaim`/`InNewWithFrontier`/`InNewSansFrontier`)
  // over each other same-SCC side keyed on static join position; nonlinear SCCs
  // no longer fall back to the pre-A2 single-pass path.
  //
  // The `is_recursive` / `all_sides_same_scc` / SCC-partition tests that used to
  // live here now live inside the lowering: `LowerDRFlow` (family #1, acyclic
  // band) and `LowerDRRounds` (family #2, recursive band) drive the emission
  // from the DR-IR flow graph + FixpointRound shells. `BuildStratumPhases`
  // retains only the discovery + scheduling fixpoint that SEED the DR strata and
  // back V-OLD-EQUIV — family #3 deletes those too.)

  // R2 FAMILY #3 (THE DELETION): the hand-coded discovery (DiscoverBranches
  // path-DFS, the JoinEmission/CrossoverEmission/ProductEmission vectors) and
  // the scheduling fixpoint that used to live here are GONE. The DR-IR flow
  // graph is now the SOLE authority: `BuildDRInventory` derives the branch/join/
  // crossover/product inventory (the memoized worklist, independent of the old
  // DFS), and `DeriveDRStrata` (the ported integer lift) computes the pinned
  // strata over the DR objects. V-OLD-EQUIV is retired with its comparands; the
  // intrinsic B-3 validators (V-XOVER-ONE, V-PROD-*), the census (V-ONE-FOLD,
  // V-SEED-SUP, ...), and the linearization checks (V-LINEAR/V-LOOP/
  // V-RETIRE-AFTER/V-READY) stand.
  DRFlowGraph dr_flow = BuildDRInventory(impl, context, query, recursive_sccs);

  // The pinned strata: DERIVE them from the DR inventory (B-13 retired). Fills
  // `dr_flow.{branch,join,crossover,product,drain}_stratum`, stamps each round's
  // drain stratum + each fire's scc_group. This is the ported scheduling
  // fixpoint, now the DR side's own authority. (No id-affecting work — DR
  // construction and derivation allocate no region/vector ids.)
  DeriveDRStrata(dr_flow, impl, context, query, recursive_sccs);

  // Always-on op-family + linearization validators (V-OLD-EQUIV legs retired):
  // the census (V-ONE-FOLD/V-SEED-SUP/V-NEG-CTX/V-CLAIM-GATE/... per-kind counts
  // recomputed independently) and the dependence-graph checks (V-LINEAR/V-LOOP/
  // V-RETIRE-AFTER/V-READY). Both recompute their expectations from the query,
  // not the deleted discovery.
  ValidateDRInventory(dr_flow);
  ValidateDROps(dr_flow, impl, context, query, recursive_sccs);
  LinearizeAndValidateDRFlow(dr_flow, impl, context, query, recursive_sccs);

  // V-PRED-XCHECK Site 5 — INGEST FOLD-OP COVERAGE + PAYLOAD (subgraphs/demand
  // P1, §5). Ties the folds LowerIngestFold actually EMITTED (recorded at
  // emission time into `context.emitted_ingest_folds`, since the flow does not
  // exist at walk time — the §12.6 authority shape) back to the flow's
  // kIngestFold ops, in the Site-4 mold. A per-op (table, sign, is_explicit,
  // role, message) 5-tuple multiset (the R1e census key, DR.cpp): every emitted
  // fold must have an enrolled op (a fold the flow never censused = a
  // MakeStageOneIngestFolds/MakeMonotoneIngestFold-vs-walk divergence), and
  // every EMITTABLE flow op (stage1==true, or stage1==false with a table) must
  // have been emitted (a fold the census counted but the walk dropped).
  // BLIND to tree SHAPE — the descent's net-additions append PLACEMENT and the
  // hole-fill site are the -ir-out structural gate's job, not Site 5's. Closes
  // the P2 cutover's un-cross-checked-ingest deviation for BOTH the
  // deletion-capable (already DR-lowered) and monotone (§6 DR-lowered) folds.
  {
    using Key =
        std::tuple<const void *, int, bool, uint8_t, uint8_t, uint64_t>;
    std::vector<Key> emitted;
    emitted.reserve(context.emitted_ingest_folds.size());
    for (const auto &e : context.emitted_ingest_folds) {
      emitted.emplace_back(e.table, e.sign, e.is_explicit, e.role, e.klass,
                           e.message);
    }
    std::vector<Key> enrolled;
    for (const DROp &op : dr_flow.ops) {
      if (op.kind != DROpKind::kIngestFold) {
        continue;
      }
      // Only the EMITTABLE flow ops: a deletion-capable pair (stage1) and a
      // monotone table-bearing op (stage1==false, table!=null). A table-less
      // monotone receive enrolls no ingest op (DR.cpp), so none appears here.
      if (!op.ingest_stage1 && op.ingest_table == nullptr) {
        continue;
      }
      // Both authorities push the kCounter effect first; its klass is the
      // payload field LowerIngestFold consumed (R-KLASS closed).
      assert(!op.effects.empty() &&
             op.effects.front().kind == EffKind::kCounter);
      enrolled.emplace_back(static_cast<const void *>(op.ingest_table),
                            op.ingest_sign, op.ingest_is_explicit,
                            static_cast<uint8_t>(op.ingest_role),
                            static_cast<uint8_t>(op.effects.front().klass),
                            op.ingest_message->Id());
    }
    std::sort(emitted.begin(), emitted.end());
    std::sort(enrolled.begin(), enrolled.end());
    if (emitted != enrolled) {
      // Its own abort text (not the PredXCheckFail predicate wording): this is
      // a coverage/payload multiset failure, the Site-5 flavor of the family.
      std::fprintf(
          stderr,
          "error: V-INGEST-XCHECK (Site 5) failed: the emitted ingest folds' "
          "(table, sign, is_explicit, role, klass, message) multiset "
          "disagrees with the flow's emittable kIngestFold enrollment — a "
          "MakeStageOneIngestFolds/MakeMonotoneIngestFold-vs-walk coverage "
          "or payload divergence (%zu emitted vs %zu enrolled)\n",
          emitted.size(), enrolled.size());
      std::abort();
    }
  }

  // ALWAYS stash the flow graph (before any early return) so
  // `PublishDifferentialMessageVectors` can lower the commit-sweep band from its
  // kCommitSweep ops even when there is no per-stratum phase work — the old code
  // emitted commit sweeps for every differential table (incl. induction-owned)
  // regardless of whether `BuildStratumPhases` short-circuited. The
  // `drain_stratum` map + rounds referenced by `LowerDRRounds` read through it.
  context.dr_flow = std::make_shared<DRFlowGraph>(std::move(dr_flow));
  const DRFlowGraph &flow = *context.dr_flow;

  // R3: publish one StateCell store descriptor per GROUP_UPDATE for codegen
  // (the `statecell_<id>` member + Key/Reduce types + commit-tail Seal). The
  // key/summary column types come from the DR statecell's projection columns
  // (input-view space, which carry their declared TypeLoc).
  impl->state_cells.clear();
  for (unsigned i = 0u; i < flow.statecells.size(); ++i) {
    const DRStateCell &cell = flow.statecells[i];
    ProgramStateCell desc(i, cell.algebra == Algebra::kInvertible,
                          *cell.algebra_functor);
    for (auto col : cell.key_cols) {
      desc.key_types.push_back(col.Type());
    }
    for (auto col : cell.summary_cols) {
      desc.summary_types.push_back(col.Type());
    }
    impl->state_cells.push_back(std::move(desc));
  }

  // No per-stratum phase work? (No branch/join/crossover/product op, no phase-
  // owned differential table.) Then the entry-body nesting + phase-series
  // emission below are skipped — but the stash above still feeds the sweeps.
  bool any_phase_table = false;
  for (TABLE *table : impl->tables) {
    if (TableIsDifferential(table) && !TableIsInductionOwned(context, table)) {
      any_phase_table = true;
      break;
    }
  }
  if (flow.branches.empty() && flow.joins.empty() && !any_phase_table &&
      flow.Crossovers().empty() && flow.ProductArms().empty()) {
    return;
  }

  // The ONE shared pivot VECTOR* per join view. Minted HERE (iterating
  // `flow.joins`, whose order == the old discovery `joins` order — both derive
  // branches identically and dedup by view), at the exact program point the old
  // discovery `joins` loop minted them, so no `next_id` shifts (DR construction
  // allocates no ids; the pivot VectorFor is the only id-affecting call between
  // inventory and emission — the family-#1 VectorFor-non-memoization hazard:
  // `LowerDRFlow` must REUSE these, never re-mint).
  std::unordered_map<QueryView, VECTOR *> join_pivots;
  for (const DRJoin &dr_join : flow.joins) {
    const QueryView join_view = dr_join.join_view;
    join_pivots.emplace(
        join_view,
        context.entry_proc->VectorFor(
            impl, VectorKind::kJoinPivots,
            QueryJoin::From(join_view).PivotColumns()));
  }

  // The strata that need a phase series, ascending. Sources mirror the old
  // driver's set EXACTLY: branch / join / crossover / product strata, plus the
  // drain strata of the PHASE-OWNED tables ONLY (differential, not induction-
  // owned == the old `phase_table_order`). `flow.drain_stratum` may carry extra
  // default-inserted keys for monotone/induction fold targets the lift touched
  // via `operator[]`; those are NOT phase tables and must not add a stratum
  // series (they converge to a branch/join stratum already inserted, but we key
  // on phase ownership directly to keep the set identical to the old code).
  std::set<unsigned> strata;
  for (unsigned s : flow.branch_stratum) {
    strata.insert(s);
  }
  for (const auto &[view, s] : flow.join_stratum) {
    strata.insert(s);
  }
  for (const auto &[view, s] : flow.crossover_stratum) {
    strata.insert(s);
  }
  for (const auto &[view, s] : flow.product_stratum) {
    strata.insert(s);
  }
  for (const auto &[table, s] : flow.drain_stratum) {
    if (TableIsDifferential(table) && !TableIsInductionOwned(context, table)) {
      strata.insert(s);
    }
  }
  // R3: a stratum owning ONLY a GROUP_UPDATE (no branch/join/product there)
  // must still be lowered — e.g. average_weight's sum/count aggregates.
  for (const auto &[view, s] : flow.group_update_stratum) {
    strata.insert(s);
  }

  // Nest the entry procedure's body (the ingest walk, which parks every
  // fold crossing in the queues the phases drain) ahead of the phase series.
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

    // R2 FAMILY #1 (F-2): the ACYCLIC BAND — seeds, join section walks,
    // crossovers, product arms, and single-pass claim drains + immediate
    // frontier filters — is now LOWERED from the DR-IR flow graph, replacing
    // the hand-coded EmitSeedLoop/EmitSectionWalk/EmitCrossover/EmitProductArms
    // + acyclic EmitClaimDrain/EmitFrontierFilter emission that used to live
    // here. `LowerDRFlow` reproduces the old band's exact sibling order (B-14
    // tree-shape identity). The SCC round shells continue below on the old web.
    LowerDRFlow(impl, context, recursive_sccs, flow, stratum, seed_vector,
                join_pivots, stratum_seq);

    // R2 FAMILY #2 (F-2): the SCC/RECURSIVE BAND — the per-SCC×phase claim-round
    // fixpoints (OVERDELETE + REDERIVE, INSERT + deferred frontier filters) — is
    // now LOWERED from the DR-IR FixpointRound shells (`dr_flow.rounds`),
    // replacing the hand-coded `build_claim_round_loop` closure + the two-call
    // del/add driver band that used to live here. `LowerDRRounds` reproduces the
    // old band structure verbatim (§5.2/§5.3), driving the surviving Emit*
    // primitives 1:1 from the round's DR ops. Family #3 nativized the drain
    // stratum onto the DRRound shell (`round.drain_stratum`), so `LowerDRRounds`
    // reads the round's own field, not the discovery `drain_stratum` map.
    LowerDRRounds(impl, context, recursive_sccs, flow, stratum, stratum_seq);
  }
}

}  // namespace hyde
