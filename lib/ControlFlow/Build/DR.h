// Copyright 2020, Trail of Bits. All rights reserved.
//
// The delta-relational IR (DR-IR) object model — R1a inventory layer.
//
// This header is the vocabulary-v3 core (spec §7.1), trimmed to what R1a
// derives: the ENUM vocabulary, an `DREffect` value, and three node families
// (`DRVec`, `DRTable`, `DROp`) owned by a `DRFlowGraph`. It is INTERNAL to the
// build directory: the public header (include/drlojekyll/ControlFlow/DR.h)
// arrives when the API stabilizes (R2+). R1a builds ONLY two op families —
// the negation CROSSOVER arm-pair and the differential @product ARM — derived
// INDEPENDENTLY from the `Query` (never by copying the old discovery structs)
// and cross-checked against that discovery by an always-on isomorphism
// validator (V-OLD-EQUIV). No plan trees, no Σ-term derivation of seed/fire
// folds, no branches/joins, no emission, no Runtime change ride on this yet.
//
// The design contract (spec §7.3): R1 CONSTRUCTS ALONGSIDE the old discovery
// and VALIDATES against it; emission still runs off the old web. So this
// object model is a checked SHADOW of the E-16 shared state, faithful enough
// that R2 lowering can later cut over family-by-family.

#pragma once

#include <drlojekyll/DataFlow/Query.h>
#include <drlojekyll/Runtime/Table.h>  // RowFlags, DerivClass (runtime copy)

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "../Program.h"  // TABLE, VECTOR, ProgramImpl, MembershipPredicate

namespace hyde {

class Context;

// ---------------------------------------------------------------------------
// §7.1 ENUM VOCABULARY (copied verbatim from the spec; the R3 tails exist so
// the scheduler/validators are total from day one, even though R1a emits no op
// that uses them).
// ---------------------------------------------------------------------------

// What a vec's elements ARE (F-9): raw column tuples, row ids into one table,
// or a row id plus a projected column subset. Demotes the (table, VectorKind)
// pair that VectorKind smuggled to a debug annotation.
enum class ElementShape : uint8_t { kValues, kIds, kIdCols };

// The producing-op-DERIVED role of a vec (spec §1). Not free choice; a
// validator (post-R1a) checks it against the op that defines the vec. R1a only
// names the queue/frontier roles it references.
enum class VecRole : uint8_t {
  kParam,
  kNetRemoval,   // D\A
  kNetAddition,  // A\D
  kDeleteQueue,
  kAddQueue,
  kOverdeleteSet,   // D
  kAdditionSet,     // A
  kClaimedDel,      // Δ_D
  kClaimedAdd,      // Δ_A
  kJoinPivots,
  kProductInput,
  kTableScan,
  kMessageOutput,
  kEmpty,
};

// G6: sort-unique is a per-vec attribute checked at the DRAIN site.
enum class UniqueContract : uint8_t { kMultiset, kSortUniqueAtDrain };

// The five effect sub-domains (F-1), plus the R3 statecell tail (reserved).
enum class EffKind : uint8_t {
  kVecAppend,
  kVecDrain,
  kVecClear,
  kCounter,        // an UPDATECOUNT-family RMW (± + DerivClass)
  kFlagRead,       // flags:read(T, F) — a membership-predicate bundle read
  kFlagWrite,      // flags:write(T, F)
  kInIReadFrozen,  // the distinguished always-legal frozen kInI read
  kStateFold,      // R3 (aggregates) — reserved
  kStateEmit,      // R3 — reserved
  kStateOld,       // R3 — reserved
};

// The membership/scan context that flavors a context-sensitive predicate.
enum class Ctx : uint8_t { kEager, kSeed, kFixpoint };

// The TEN membership predicates (E-14). Kept as its own enum matching the spec
// §7.1 sketch; semantics are Table.h-exact. NOTE: the build layer already has
// `hyde::MembershipPredicate` (Program.h) with the same ten members in a
// different order; `Pred` is the DR-IR's own vocabulary value. A helper below
// maps between them where a real membership read is referenced.
enum class Pred : uint8_t {
  kPresent,
  kInI,
  kInNew,
  kSurvivesSoFar,
  kAliveAtClaim,
  kInNewWithFrontier,
  kInNewSansFrontier,
  kRecursivelySupported,
  kNetDeleted,
  kNetAdded,
};

// The op families the DR-IR inventories. R1a built the two frontier-arm
// families (crossover, product arm); R1c adds the seed/fixpoint/claim/retire/
// rederive/filter/sweep families + the context-keyed negate gate (spec §2.1,
// vocabulary v3). STILL construct-alongside: no op is emitted; each family is
// DERIVED independently and cross-checked (V-OLD-EQUIV) against the old
// emission driver's discovery state.
enum class DROpKind : uint8_t {
  kCrossover,      // a non-@never negate's arm-pair (spec §2.1 CROSSOVER, B-3.1)
  kProductArm,     // one side×sign arm of an acyclic differential @product
  kSeedFold,       // §6.2 SEED-schema arm: branch chain × sign (head fold)
  kFixpointFire,   // §6.3 FIXPOINT-schema fire: join × sign (claim-relative)
  kChainFold,      // §2.1 CHAIN_FOLD: claimed-frontier → projection → head fold
  kClaimDrain,     // §2.1 CLAIM_DRAIN: table × sign (single-pass | in-round)
  kRetire,         // §2.1 RETIRE: per SCC table × sign (clear kDelNow/kAddNow)
  kRederive,       // §2.1 REDERIVE: per SCC table (C_r>0 gate → addQ)
  kFrontierFilter, // §2.1 FRONTIER_FILTER: table × sign (immediate|deferred)
  kCommitSweep,    // §2.1 COMMIT_SWEEP: per differential table (flavor)
  kNegateGate,     // §2.1 NEGATE_GATE: context∈{eager,seed,fixpoint} × hint
  kPivotAssemble,  // §2.1 PIVOT_ASSEMBLE: per SCC join (union → shared pivot vec)
  kIngestFold,     // §2.1 INGEST_FOLD: entry-proc message→table seed (R1d cut —
                   //   see the eager-walk inventory note; NOT populated in R1d)
};

// The negate-gate hint (F-5): a normal negate vs an @never negate. Distinct
// from context — @never negates read Present (eager only), normal negates read
// InI (eager/seed) or InNew (fixpoint refire). NEVER sign-derived (V-NEG-CTX).
enum class NegateHint : uint8_t { kNormal, kNever };

// The AUTHORITATIVE (context, hint) → negate-gate predicate derivation (F-5 /
// F18): eager reads Present(@never)/InI(normal), seed reads InI BOTH signs,
// fixpoint refire reads InNew BOTH signs — NEVER sign-derived. This is the DR
// model's sole source for a negate gate's predicate (every DR gate node + the
// eager NEGATE_GATE op is populated from it). Exposed so V-PRED-XCHECK (Site 1)
// can cross-check the emitter's EmitChainStep negate-gate choice against it.
Pred NegateGatePred(Ctx ctx, NegateHint hint);

// The claim-drain form (spec §2.1 CLAIM_DRAIN): single-pass for a lower
// differential table (dead/edge), in-round for an SCC table (dual-append +
// input-queue clear, B-7). DERIVED from whether the table is in a recursive SCC.
enum class ClaimForm : uint8_t { kSinglePass, kInRound };

// The frontier-filter deferral (E-17 / spec §4.3): immediate for a lower
// non-recursive table, add-loop-output for an SCC table (BOTH signs deferred
// into the INSERT round's output). DERIVED, never a placement flag.
enum class Deferral : uint8_t { kImmediate, kAddLoopOutput };

// The commit-sweep flavor (spec §2.1 COMMIT_SWEEP): a differential table gets
// Commit+validate+compact+reindex; a monotone table gets Seal. DERIVED from the
// table's model flavor.
enum class SweepFlavor : uint8_t { kDifferential, kMonotone };

// The F17 claim-drain DEQUEUE re-test (spec §2.1 CLAIM_DRAIN; F17). A DEDICATED
// vocabulary for the two gates the emitter's TryClaimDel/TryClaimAdd apply —
// NOT a `Pred` membership predicate (finding 2: the earlier encoding abused
// Pred::kNetDeleted/kNetAdded as a stand-in, which is semantically WRONG if ever
// consumed — those are flag-read predicates, not the C_nr/total counter gates).
// DERIVED purely from the drain sign: a `-` drain gates on C_nr<=0 (the row's
// net-delete counter is non-positive — nothing left to overdelete), a `+` drain
// gates on total>0 (the row's total derivation count is positive — it is live).
enum class ClaimGate : uint8_t {
  kDelGateCnrNonPositive,  // TryClaimDel: C_nr <= 0
  kAddGateTotalPositive,   // TryClaimAdd: total > 0
};

// ---------------------------------------------------------------------------
// §2 EFFECT SET. One entry of an op's effect vector (A-3: an op's set is the
// union of its per-arm sets; R1a carries the op-level union directly). A
// `DREffect` is a tagged union over the five sub-domains — only the fields the
// `kind` selects are meaningful.
// ---------------------------------------------------------------------------
struct DREffect {
  EffKind kind{EffKind::kFlagRead};

  // For vector effects: the target table + role naming the vec this effect
  // appends/drains/clears. R1a represents queues/frontiers as TABLE-KEYED
  // PENDING-VEC REFERENCES (the E-16 `Context::table_delta_vecs` key); R1b
  // makes these real `DRVec` def/use edges. `value_table` is null for a
  // message-only vec (none in R1a's two families).
  TABLE *value_table{nullptr};
  VecRole vec_role{VecRole::kEmpty};

  // For a counter effect (kCounter): the target table, the sign (+1 add / -1
  // retract), and the RuleClass-derived class.
  TABLE *counter_table{nullptr};
  int sign{0};
  DerivClass klass{DerivClass::kNonRecursive};

  // For a flags:read / kInIReadFrozen effect: the read table and its predicate
  // bundle, flavored by `ctx`. `pred` is the DR-IR vocabulary value.
  TABLE *read_table{nullptr};
  Pred pred{Pred::kInI};
  Ctx ctx{Ctx::kSeed};

  // For a flags:write effect (kFlagWrite): the written table. The specific bits
  // are implied by the owning op's kind/sign (claim → kDel|kDelNow / kAdd|
  // kAddNow; retire → clear kDelNow/kAddNow; commit → clear all + set kInI) —
  // R1c does not enumerate individual RowFlags on the effect (a later stage
  // may). `sign` (above) carries the direction where meaningful.
  TABLE *write_table{nullptr};
};

// ---------------------------------------------------------------------------
// §1 VEC — a typed IR value (F-1/F-9). R1b MATERIALIZES the queues/frontiers
// the discovery bands own as real `DRVec` values (R1a left them as TABLE-keyed
// references inside effects). Per differential non-induction-owned table, six
// per-table vecs are minted (delete/add queues, overdelete/addition sets, net-
// removals/net-additions frontiers — VecRole per DR.h); per JoinEmission, ONE
// shared join-pivots vec. Attributes per spec §1: `shape` is CONSTRUCTOR
// knowledge (A-5, the producing op's element type — not read back from emitted
// IR); `uniq` is the sort-unique-at-drain contract keyed on the DRAIN site
// (mirrors the tc 10-site census, G6); `debug_table`/`debug_kind` are the
// demoted (table, VectorKind) v2 identity, DEBUG-only. Def/use edges (A-4:
// append-accumulating queues/sets may MULTI-def) are recorded in R1b ONLY for
// the two R1a op families that touch them: a crossover appends to the negate
// table's delete/add queue; a product arm appends to the product table's
// delete/add queue. Full def/use wiring (seed/fire folds, claim drains,
// filters) arrives with R1c's op families.
// ---------------------------------------------------------------------------
class DRVec {
 public:
  ElementShape shape{ElementShape::kIds};
  VecRole role{VecRole::kEmpty};
  UniqueContract uniq{UniqueContract::kMultiset};

  // The demoted (table, kind) v2 identity — DEBUG annotation only (§1).
  TABLE *debug_table{nullptr};
  VectorKind debug_kind{VectorKind::kEmpty};

  // Def/use edges by op index into `DRFlowGraph::ops`. R1b records the two
  // R1a families' append def-edges (multi-def per A-4); use-edges and the
  // remaining families land with R1c.
  std::vector<unsigned> defs;  // A-4: append-accumulating vecs may multi-def.
  std::vector<unsigned> uses;
};

// ---------------------------------------------------------------------------
// §1 TABLE (debug-labelled model). Wraps a `TABLE *` model with the
// differential flavor and the V-MEMBER-ID member-view list (each feeder view
// at most once, BY IDENTITY — never structurally deduped).
// ---------------------------------------------------------------------------
class DRTable {
 public:
  TABLE *model{nullptr};
  bool differential{false};
  std::vector<QueryView> member_views;  // identity-distinct feeders
};

// ---------------------------------------------------------------------------
// §1.4 BRANCH INVENTORY — one rule chain from a member view of a source table
// to its terminal (the first pivot-JOIN, or the first table-boundary view that
// is not induction-owned), mirroring the old discovery's `BranchChain`
// (Stratum.cpp:17-33). R1b DERIVES the branch set INDEPENDENTLY of the old
// path-copying DFS, via a MEMOIZED WORKLIST (the §1.4 hazard fix: reconvergent
// table-less plumbing costs O(V+E), not exponential). The derived set is
// compared to the old DFS's as a MULTISET of (source, path, ends_at_join,
// target) — the old code records one chain PER DISTINCT PATH (its DFS carries
// no visited set), so a diamond of plumbing produces path-multiplicity; the
// memoized form reproduces that per-path multiset faithfully by memoizing
// path-SUFFIXES per view and prepending each caller's prefix.
// ---------------------------------------------------------------------------
class DRBranch {
 public:
  TABLE *source{nullptr};
  std::vector<QueryView> path;   // path[0] = source member; back() = terminal
  bool ends_at_join{false};
  TABLE *target{nullptr};        // head-chain fold target (null for a join)
};

// ---------------------------------------------------------------------------
// §1.4 JOIN INVENTORY — one dual-section pivot-JOIN emission, deduped by join
// view, owning ONE shared join-pivots `DRVec` (index into `DRFlowGraph::vecs`)
// that every feeding branch/sign drains. Mirrors the old `JoinEmission`
// (Stratum.cpp:38-53); the DR side MINTS the pivot vec (A-5: element_shape
// id+cols over the union pivot column set).
// ---------------------------------------------------------------------------
class DRJoin {
 public:
  QueryView join_view;
  unsigned pivot_vec{0u};             // index into DRFlowGraph::vecs
  std::vector<TABLE *> targets;       // the join section walks' fold targets

  explicit DRJoin(QueryView join_view_) : join_view(join_view_) {}
};

// ---------------------------------------------------------------------------
// §3 ACCESS-PLAN TREE (F-7). A SEED_FOLD/FIXPOINT_FIRE/CHAIN_FOLD body is a
// nested chain of `PlanNode`s mirroring `EmitJoinFire`/`EmitSeedLoop`'s
// scan_next nesting. R1c builds the MINIMAL tree the §3.1 grammar needs to
// certify V-ONE-FOLD (exactly one Fold leaf) and the per-node access reads —
// NOT the full column-projection detail an R2 lowering will carry. Each node
// is one of: an ACCESS (a scanned/point-tested side, carrying its bound-col
// set per B-1 + membership pred + polarity + identity lowering), a GATE (a
// bare membership check with no scan — the negate forward gate), or the FOLD
// leaf (counter± into the head, appended to the sign queue).
//
// The chain is a SPINE (each node has ≤1 child); a join fire's k-way scan is a
// left-deep spine, exactly as `scan_next` nests. `is_fold` marks the sole leaf.
// ---------------------------------------------------------------------------
enum class PlanKind : uint8_t { kAccess, kGate, kFold };

// The identity lowering choice for an ACCESS (spec §3.2): a point-test
// (Find+pred), a keyed section walk (idx.First/Next + pivot re-test), or a full
// scan (zero bound columns). `kSeek` is reserved for the D5 substrate.
enum class Lowering : uint8_t { kPointTest, kSectionWalk, kFullScan, kSeek };

class PlanNode {
 public:
  PlanKind kind{PlanKind::kFold};

  // ---- ACCESS / GATE (kind == kAccess | kGate) ---------------------------
  TABLE *table{nullptr};  // the scanned/gated table (the read's target)
  // The canonical column-id SET this access binds (B-1: GetOrCreateIndex
  // SortAndUnique order == the index identity, NEVER prefix contiguity). Empty
  // for a full scan or a bare gate. Column ids, sorted ascending (canonical).
  std::vector<unsigned> bound_cols;
  Pred pred{Pred::kInNew};
  bool absent{false};       // polarity: membership gate ABSENT (negate) vs member
  Lowering lowering{Lowering::kFullScan};
  Ctx ctx{Ctx::kSeed};      // the context flavoring a context-sensitive pred

  // For a section-walk ACCESS, the pivot re-test column id (G3 disambiguation:
  // which body atom is the delta on the scan cursor). kNoPivot when n/a.
  static constexpr unsigned kNoPivot = ~0u;
  unsigned pivot_col{kNoPivot};

  // ---- FOLD leaf (kind == kFold) -----------------------------------------
  TABLE *fold_table{nullptr};   // the head table folded into
  int fold_sign{0};             // -1 retract / +1 add
  DerivClass fold_class{DerivClass::kNonRecursive};

  // The single child (null at the leaf, or at a childless partial node).
  std::unique_ptr<PlanNode> child;
};

// ---------------------------------------------------------------------------
// §A-3 PER-ARM structure. A FIXPOINT_FIRE groups per-delta-position arms of ONE
// join under ONE op (G2); each arm carries its OWN effect list + PlanTree. A
// SEED_FOLD carries a single arm (one delta position). Validators keyed on
// frontier-flag reads (V-RETIRE-AFTER) evaluate at ARM granularity (A-3).
// ---------------------------------------------------------------------------
class DRArm {
 public:
  unsigned delta_pos{0u};                 // the delta body position (static)
  int sign{0};                            // -1 (OVERDELETE) / +1 (INSERT)
  std::vector<DREffect> effects;          // this arm's own effect set (A-3)
  std::unique_ptr<PlanNode> body;         // §3 access-plan tree (spine)
};

// ---------------------------------------------------------------------------
// §2.1 OP — one DR-IR operator node with a per-op effect vector. R1a populates
// only the two families named by `DROpKind`; the crossover carries its arm-pair
// data (the three tables + pred view + negated_differential + the derived
// class), the product arm its (side index, sign, side tables + differential).
// R1c adds the seed/fixpoint/claim/retire/rederive/filter/sweep/negate-gate
// families (each with its own payload block below).
// ---------------------------------------------------------------------------
class DROp {
 public:
  DROpKind kind;

  // The op's effect vector (A-3 union). Derived by `BuildDRInventory`.
  std::vector<DREffect> effects;

  // The membership/scan context this op's reads resolve in.
  Ctx ctx{Ctx::kSeed};

  // seed-before-drain (B-3.4): the counter± into the target table precedes
  // that table's own claim drain. An ordering ATTRIBUTE R1b turns into a RAW
  // dep edge; true for both R1a families.
  bool seed_before_drain{true};

  // ---- CROSSOVER data (kind == kCrossover) ---------------------------------
  // Query-typed members are `optional` because `QueryNegate`/`QueryView` have
  // no default constructor (Node<> wrappers); they are always set when
  // `kind == kCrossover`.
  std::optional<QueryNegate> negate;
  TABLE *negate_table{nullptr};    // the negate view's own differential table
  TABLE *negated_table{nullptr};   // the negated view's model table (the src)
  TABLE *pred_table{nullptr};      // the data predecessor's table (scanned)
  std::optional<QueryView> pred_view;  // QueryView(negate).Predecessors()[0]
  bool negated_differential{false};  // whether the `+` arm exists
  // Which arm this DROp is: -1 for the `-` arm, +1 for the `+` arm. A crossover
  // is emitted as a PAIR of DROps sharing the above data.
  int crossover_sign{0};

  // ---- PRODUCT ARM data (kind == kProductArm) ------------------------------
  std::optional<QueryView> product_view;
  TABLE *product_table{nullptr};
  unsigned side_index{0u};              // position in `JoinedViews()` order
  int product_sign{0};                  // -1 (minus arm) / +1 (plus arm)
  std::vector<TABLE *> side_tables;     // all sides, position order
  std::vector<bool> side_differential;  // per-side `-` arm existence
  // The position-keyed non-delta reads of THIS arm (§4): the other sides,
  // j < side_index read at kInNew, j > side_index at kInI (sign-independent).
  // Stored as (table, pred) pairs in position order (delta side omitted).
  std::vector<std::pair<TABLE *, Pred>> arm_reads;

  // ---- SEED_FOLD data (kind == kSeedFold) ----------------------------------
  // One §6.2 SEED-schema arm: a branch chain × available sign. `seed_branch`
  // indexes `DRFlowGraph::branches`; `seed_sign` is -1 (over the source's
  // net-removals) / +1 (over net-additions). `seed_target` is the head table
  // the chain folds into (== branch.target for a head chain; null for a
  // join-terminal chain — a join-terminal SEED_FOLD only APPENDS pivots, no
  // fold, so it carries `join_pivot=true` and no Fold leaf). `seed_class` is
  // the RuleClass of the head fold. The arm (effects + PlanTree) is `arm`.
  unsigned seed_branch{0u};
  int seed_sign{0};
  TABLE *seed_source{nullptr};
  TABLE *seed_target{nullptr};
  DerivClass seed_class{DerivClass::kNonRecursive};
  bool join_pivot{false};       // a join-terminal seed (pivot append, no fold)
  bool in_fixpoint_seed{false}; // a same-SCC internal projection seed (§2320)

  // ---- FIXPOINT_FIRE data (kind == kFixpointFire) --------------------------
  // ONE op per (join, sign) (G2). `fire_join` is the join view; `fire_sign`
  // -1 (OVERDELETE) / +1 (INSERT). The per-position arms (one per same-SCC
  // delta position, in JoinedViews() order) are `arms`.
  std::optional<QueryView> fire_join;
  TABLE *fire_table{nullptr};   // the join's persisted (recursion-anchor) table
  int fire_sign{0};

  // ---- CHAIN_FOLD data (kind == kChainFold) --------------------------------
  // A same-SCC internal projection/merge-arm fold (the tc G7 shape): its delta
  // is a CLAIMED-* round frontier of `chain_source`, folded (Recursive) into
  // `chain_target`. `chain_branch` indexes the branch; `chain_sign` -1/+1.
  unsigned chain_branch{0u};
  int chain_sign{0};
  TABLE *chain_source{nullptr};
  TABLE *chain_target{nullptr};

  // ---- CLAIM_DRAIN / RETIRE / REDERIVE / FRONTIER_FILTER / COMMIT_SWEEP -----
  // The per-table family target + attributes. `table_op_table` is the table;
  // `table_op_sign` -1 (del) / +1 (add) where signed (drain/retire/filter);
  // 0 for REDERIVE/COMMIT_SWEEP. `claim_form`/`deferral`/`sweep_flavor` are the
  // DERIVED attributes (see the enums). `claim_gate` is the F17 gate predicate
  // carried as DATA (kNetDeleted-flavored C_nr<=0 for del, Total>0 for add) —
  // mandatory on every CLAIM_DRAIN (V-CLAIM-GATE).
  TABLE *table_op_table{nullptr};
  int table_op_sign{0};
  ClaimForm claim_form{ClaimForm::kSinglePass};
  Deferral deferral{Deferral::kImmediate};
  SweepFlavor sweep_flavor{SweepFlavor::kDifferential};
  // The F17 dequeue re-test carried AS DATA (finding 2): a dedicated ClaimGate,
  // NOT a Pred. Set per sign at the CLAIM_DRAIN mint; checked by V-CLAIM-GATE to
  // be semantically correct for the drain's sign (del ⇒ C_nr<=0, add ⇒ total>0).
  ClaimGate claim_gate{ClaimGate::kDelGateCnrNonPositive};
  unsigned scc_group{~0u};             // the owning SCC id for in-round/scc ops

  // R1d: the COMMIT_SWEEP publish-target, its OWN named field (drops the
  // reused `join_pivot` bool the R1c sweep block borrowed — flagged in the
  // R1c ledger). True iff the swept table backs a @differential TRANSMIT (G9).
  bool publish_target{false};

  // ---- PIVOT_ASSEMBLE data (kind == kPivotAssemble) ------------------------
  // The seed-block pivot union per SCC join (G2): the dual-section walk unions
  // each feeding branch/sign's pivots into the join's ONE shared kJoinPivots
  // vec (the FIXPOINT_FIRE arms' shared input). `pivot_join` is the join view;
  // `pivot_vec_index` indexes `DRFlowGraph::vecs` (== the DRJoin's pivot_vec);
  // `pivot_source_tables` are the delta-source tables whose signed frontiers
  // the assemble drains (dual-section registration mirror). The effects carry
  // one vector:drain per source×sign + the vector:clear+append of the pivot vec.
  std::optional<QueryView> pivot_join;
  unsigned pivot_vec_index{0u};
  std::vector<TABLE *> pivot_source_tables;

  // ---- NEGATE_GATE data (kind == kNegateGate) ------------------------------
  // An ACCESS whose pred is the negate forward gate, polarity=ABSENT, derived
  // from (context, hint) — NEVER sign-derived (F-5 / V-NEG-CTX). `gate_negate`
  // is the negate view; `gate_table` its negated table; `gate_pred` the
  // (context,hint)-derived predicate; `gate_hint` the normal/@never hint.
  std::optional<QueryView> gate_negate;
  TABLE *gate_table{nullptr};
  Pred gate_pred{Pred::kInI};
  NegateHint gate_hint{NegateHint::kNormal};

  // The per-arm structure (A-3). A FIXPOINT_FIRE has one arm per same-SCC delta
  // position; a SEED_FOLD/CHAIN_FOLD has exactly one. Empty for the per-table
  // families (claim/retire/rederive/filter/sweep) and the negate gate.
  std::vector<DRArm> arms;

  DROp(DROpKind kind_) : kind(kind_) {}
};

// ---------------------------------------------------------------------------
// §2.1 FIXPOINT_ROUND — a STRUCTURED REGION (container) per SCC × phase (G4).
// R1d models the round SHELL: it OWNS its body op ids + test vecs (the per-SCC-
// table claimed-* frontiers re-exported as the loop break condition) and the
// per-round bookkeeping FACTS: the round-start frontier CLEAR (one per SCC
// table, Stratum.cpp:2287-2297) and the Δ-emptiness BREAK (the loop terminates
// when a round claims nothing — NOT queue-emptiness; Stratum.cpp:2262-2264).
// The round is a scheduling BLACK BOX to the epoch linearizer (its internal
// order is its own checked linearization, §4.6); round-carried vecs are
// loop-carried at ROUND scope (A-1/B-10, V-LOOP).
// ---------------------------------------------------------------------------
enum class RoundPhase : uint8_t { kOverdelete, kInsert };

class DRRound {
 public:
  unsigned scc_group{~0u};
  RoundPhase phase{RoundPhase::kOverdelete};

  // R2 FAMILY #3 (DRAIN-STRATUM NATIVIZATION): the stratum whose phase series
  // emits this round (the shared drain stratum of the group's SCC tables).
  // Stamped by `DeriveDRStrata`; read by `LowerDRRounds` instead of the old
  // discovery's `drain_stratum` map.
  unsigned drain_stratum{0u};

  // The claimed-* frontier vec of each SCC table this round clears/re-exports
  // (the fixpoint-test vec set — Δ-emptiness break reads these). Index into
  // `DRFlowGraph::vecs`. Cleared at round start, refilled by the in-round
  // claim drains, drained by the fires/chain-folds, retired at round tail.
  std::vector<unsigned> test_vecs;

  // R2+ SUBSTRATE (dead-but-alive): populated by LinearizeAndValidateDRFlow from
  // the pinned order, never read — the round's OWN intra-round op sequence an
  // R2+ lowering will emit from instead of re-deriving the sub-order.
  // The body op ids (claim drains, fires, chain-folds, retires), in the round's
  // OWN pinned order (band template: clears → claims → fires → chain-folds →
  // retires; §4.6 / B-9). Index into `DRFlowGraph::ops`.
  std::vector<unsigned> body_ops;

  // R2+ SUBSTRATE (dead-but-alive): populated, never read — see `body_ops`.
  // The output-region op ids: REDERIVE (OVERDELETE round) or the deferred
  // FRONTIER_FILTERs (INSERT round) — E-17 / §4.4.
  std::vector<unsigned> output_ops;
};

// ---------------------------------------------------------------------------
// §4 DEPENDENCE EDGE. A derived RAW/WAR/WAW edge X→Y over a shared Value W
// (a Vec, or a (table,flag-class) counter/flag-set). `loop_carried` marks a
// cross-scope RAW whose intra-scope realization is a WAR (drain-before-refill):
// A-1 at epoch scope, B-10 at round scope. Loop-carried edges are EXCLUDED
// from intra-scope topo checking (V-LINEAR) and validated by V-LOOP instead.
// ---------------------------------------------------------------------------
enum class DepKind : uint8_t { kRAW, kWAR, kWAW };
enum class DepScope : uint8_t { kEpoch, kRound };

struct DRDep {
  unsigned from{0u};    // op index (producer/earlier)
  unsigned to{0u};      // op index (consumer/later)
  DepKind kind{DepKind::kRAW};
  DepScope scope{DepScope::kEpoch};
  bool loop_carried{false};
};

// ---------------------------------------------------------------------------
// §0 FLOW GRAPH — owns the value/op vectors and the recursive-SCC map. R1b
// populates the crossover + product-arm ops, the table inventory, the
// materialized per-table/per-join DRVecs, the independently-derived branch/join
// inventory, and the per-unit seeded pinned strata (B-13: R1b STORES the old
// lift's integers; the independent deriver is R1c+).
// ---------------------------------------------------------------------------
class DRFlowGraph {
 public:
  std::vector<DRVec> vecs;      // R1b: per-table queues/sets/frontiers + pivots
  // R2+ SUBSTRATE (dead-but-alive): populated (BuildDRInventory), never read —
  // the debug-labelled table models an R2+ lowering will resolve ops through.
  std::vector<DRTable> tables;
  std::vector<DROp> ops;        // R1a: crossover pairs + product arms

  // R1d: the FIXPOINT_ROUND region shells (per SCC × phase), the derived §4
  // dependence graph, and the pinned linearization (op ids in emission order —
  // the epoch-scope order; each round's internal order is on `DRRound`).
  std::vector<DRRound> rounds;
  std::vector<DRDep> dep_edges;
  std::vector<unsigned> pinned_order;  // §4.6 checked linearization (epoch scope)

  // The independently-derived branch/join inventory (§1.4, memoized worklist).
  std::vector<DRBranch> branches;
  std::vector<DRJoin> joins;

  // The six materialized per-table vecs, by (table, VecRole) → index into
  // `vecs`. R1b mints these for every differential non-induction-owned table
  // (the discovery bands' owned queues/sets/frontiers). The join-pivots vecs
  // live on `DRJoin::pivot_vec`.
  std::unordered_map<TABLE *, std::unordered_map<VecRole, unsigned>>
      table_vecs;

  // table -> recursive-SCC group id (only tables in a stratum-phase-owned SCC
  // appear). Copied from the discovery's `ComputeRecursiveSCCs` result.
  std::unordered_map<TABLE *, unsigned> scc_map;

  // B-13 per-unit SEEDED pinned strata: the old scheduling fixpoint's converged
  // integers, STORED (not independently derived — R1c+). Keyed by unit identity
  // so V-OLD-EQUIV can prove the DR graph carries every unit the scheduler
  // ordered. `branch_stratum` is indexed parallel to `branches`; the others are
  // keyed by view/table identity.
  std::vector<unsigned> branch_stratum;                  // parallel to branches
  std::unordered_map<QueryView, unsigned> join_stratum;  // by join view
  std::unordered_map<QueryView, unsigned> crossover_stratum;  // by negate view
  std::unordered_map<QueryView, unsigned> product_stratum;    // by product view
  std::unordered_map<TABLE *, unsigned> drain_stratum;        // by table

  // Convenience accessors over `ops`.
  std::vector<const DROp *> Crossovers(void) const;
  std::vector<const DROp *> ProductArms(void) const;
  // R1c: every op of one kind, in construction order.
  std::vector<const DROp *> OpsOfKind(DROpKind kind) const;

  // Look up a materialized per-table vec index; asserts it exists.
  unsigned TableVec(TABLE *table, VecRole role) const;
};

// ---------------------------------------------------------------------------
// The R1a construction + validation entry points (spec §7.1 / §7.3).
// ---------------------------------------------------------------------------

// Derive the inventory INDEPENDENTLY from the `Query` (§6-style derivation:
// crossover + product-arm families, the R1b materialized vecs, and the §1.4
// memoized branch/join inventory). `scc_map` is the discovery's already-
// computed `ComputeRecursiveSCCs` output, passed in so the RuleClass derivation
// matches bit-for-bit. Strata are DERIVED by `DeriveDRStrata` (R2 family #3;
// B-13's old-lift seeding is retired).
DRFlowGraph BuildDRInventory(
    ProgramImpl *impl, Context &context, Query query,
    const std::unordered_map<TABLE *, unsigned> &scc_map);

// The INTRINSIC B-3 family validators (V-XOVER-ONE, V-PROD-MONO, V-PROD-CLASS,
// V-JOIN-ONE), ALWAYS-ON (survive NDEBUG). The V-OLD-EQUIV legs that used to
// compare against the old discovery are RETIRED (R2 family #3: the discovery is
// deleted). On ANY mismatch: `fprintf(stderr, ...)` + `abort()`.
void ValidateDRInventory(const DRFlowGraph &flow);

// R1c: validate the DERIVED op families (seed folds, fixpoint fires, chain
// folds, claim drains, retires, rederives, frontier filters, commit sweeps,
// negate gates). ALWAYS-ON. Two halves:
//
//  (1) INTERNAL graph validators (spec §5, evaluated over `flow` alone):
//      V-QCLEAR, V-CLAIM-GATE, V-NEG-CTX, V-RETIRE-AFTER (structural),
//      V-DEFER, V-ONE-FOLD, V-SEED-SUP.
//  (2) OP-INVENTORY CENSUS: the DR op inventory (per-kind counts + keys) must
//      match an EXPECTED census recomputed HERE. There is no live "old emission
//      driver" left to compare against (R2 family #3 deleted the old discovery);
//      the census is an INDEPENDENT RECOUNT that replicates the emitter's
//      counting RULES (branches×signs minus suppressed, joins with same-SCC side
//      counts, per-table drains/filters/sweeps) as a pure function of the same
//      discovery inputs (impl/context/query/scc_map) — re-deriving the branch/
//      join inventory the way BuildDRInventory does — then compares count-for-
//      count and key-for-key against `flow`'s ops.
//
// This validator needs the live discovery inputs to recompute the census
// independently (the flat op lists alone cannot express the per-join same-SCC
// side test), so it takes them directly. On ANY mismatch: fprintf + abort
// (survives NDEBUG, matching `ValidateDRInventory`).
void ValidateDROps(
    const DRFlowGraph &flow, ProgramImpl *impl, Context &context, Query query,
    const std::unordered_map<TABLE *, unsigned> &scc_map);

// R1d: the LAST construction stage. In one pass over `flow` (already carrying
// R1a-R1c's ops + R1d's PIVOT_ASSEMBLE / eager NEGATE_GATE / round shells,
// minted by `BuildDRInventory`), this:
//   (1) uniformly MATERIALIZES every op's vec use-edges from its drain/gate
//       effects (def-edges stay as R1b/R1c minted them, multi-def per A-4);
//   (2) DERIVES the §4 RAW/WAR/WAW dependence graph from effect intersections
//       over vecs + flag-classes + counters (kInI reads frozen — no hazard),
//       with loop-carried classification at BOTH epoch and round scope
//       (A-1/B-10);
//   (3) LINEARIZES independently: `pinned_order` = a topological sort of the
//       intra-epoch dep graph + the B-9 band-template tie-breaks; each round's
//       `body_ops`/`output_ops` are its own intra-round order.
// Then it VALIDATES (all always-on, abort on mismatch):
//   V-LINEAR (pinned order is a topo sort of intra-scope edges),
//   V-LOOP (every loop-carried RAW has a matching intra-scope WAR drain-before-
//     refill witness on the same resource; abort otherwise — finding 3(b)),
//   V-BAND-HAZARD (a non-carried RAW never runs against the band key —
//     finding 3(a)),
//   V-RETIRE-AFTER (arm-granular ordering: fires reading kDelNow/kAddNow
//     precede the retire clearing them),
//   V-READY (every read strictly-lower-or-same-SCC),
//   V-OLD-EQUIV(order): the derived pinned order is non-decreasing in the
//     emission band key (the standing guard that the independent linearization
//     never drifts from the emitter's band walk).
// NOTE: the former V-OLD-EQUIV(strata) leg — a per-op `derived == seeded-copy`
// comparison — is RETIRED. R2 family #3 made `DeriveDRStrata` the SOLE writer of
// the `*_stratum` maps, so that check compared a value against itself (a
// tautology, and B-13's separate seeded copy is gone). Strata authority is now
// `DeriveDRStrata`, cross-checked against the emitter only through V-OLD-EQUIV
// (order) above and the byte-identical golden gate.
//
// Takes the live discovery inputs (like `ValidateDROps`) to recompute the
// emitter's per-unit strata + census-order independently.
void LinearizeAndValidateDRFlow(
    DRFlowGraph &flow, ProgramImpl *impl, Context &context, Query query,
    const std::unordered_map<TABLE *, unsigned> &scc_map);

// R2 FAMILY #3: DERIVE the pinned strata from the DR inventory independently
// (the port of the old scheduling fixpoint; replaces `SeedDRStrata`). Fills
// `branch_stratum`/`join_stratum`/`crossover_stratum`/`product_stratum`/
// `drain_stratum`, stamps each round's `drain_stratum` and each FIXPOINT_FIRE's
// `scc_group`. The DR side is now the strata AUTHORITY (B-13 retired).
void DeriveDRStrata(DRFlowGraph &flow, ProgramImpl *impl, Context &context,
                    Query query,
                    const std::unordered_map<TABLE *, unsigned> &scc_map);

// R2 FAMILY #3: LOWER the end-of-batch commit-sweep band from the DR-IR's
// kCommitSweep ops into `seq` (replaces the hand-coded Procedure.cpp band).
// Called from `PublishDifferentialMessageVectors` with the flow graph stashed
// on `context.dr_flow`.
void LowerCommitSweeps(ProgramImpl *impl, Context &context,
                       const DRFlowGraph &dr_flow, SERIES *seq);

}  // namespace hyde
