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

// The op families R1a inventories. The full §2.1 operator table lands in later
// R1 stages; R1a builds only these two.
enum class DROpKind : uint8_t {
  kCrossover,   // a non-@never negate's arm-pair (spec §2.1 CROSSOVER, B-3.1)
  kProductArm,  // one side×sign arm of an acyclic differential @product
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
// §2.1 OP — one DR-IR operator node with a per-op effect vector. R1a populates
// only the two families named by `DROpKind`; the crossover carries its arm-pair
// data (the three tables + pred view + negated_differential + the derived
// class), the product arm its (side index, sign, side tables + differential).
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

  DROp(DROpKind kind_) : kind(kind_) {}
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
  std::vector<DRTable> tables;  // debug-labelled models referenced by ops
  std::vector<DROp> ops;        // R1a: crossover pairs + product arms

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
// matches bit-for-bit. Strata are NOT derived here (B-13: seeded from the old
// lift by `SeedDRStrata`, called after the scheduling fixpoint converges).
DRFlowGraph BuildDRInventory(
    ProgramImpl *impl, Context &context, Query query,
    const std::unordered_map<TABLE *, unsigned> &scc_map);

// V-OLD-EQUIV (§7.3) + the B-3 family validators, ALWAYS-ON (survive NDEBUG).
// Called from `BuildStratumPhases` where the old discovery vectors are in
// scope; the caller passes the discovery's crossover/product/branch/join
// records (opaque here to avoid leaking the anonymous-namespace structs — the
// caller supplies the comparison payload). On ANY mismatch: `fprintf(stderr,
// ...)` + `abort()`.
//
// The old-discovery payloads are passed as parallel flat vectors so this
// header does not depend on the `.cpp`'s anonymous-namespace types.
struct OldCrossoverRef {
  QueryNegate negate;
  TABLE *negate_table;
  TABLE *negated_table;
  TABLE *pred_table;
  QueryView pred_view;
  bool negated_differential;
  unsigned stratum;  // B-13 seeded pinned stratum (by negate view identity)
};
struct OldProductRef {
  QueryView product_view;
  TABLE *product_table;
  std::vector<TABLE *> side_tables;
  std::vector<bool> side_differential;
  unsigned stratum;  // B-13 seeded pinned stratum (by product view identity)
};
struct OldBranchRef {
  TABLE *source;
  std::vector<QueryView> path;
  bool ends_at_join;
  TABLE *target;
  unsigned stratum;  // B-13 seeded pinned stratum (by branch identity)
};
struct OldJoinRef {
  QueryView join_view;
  std::vector<TABLE *> targets;
  unsigned stratum;  // B-13 seeded pinned stratum (by join view identity)
};

// B-13: STORE the old scheduling fixpoint's converged integers into `flow` as
// the seeded pinned strata (the independent deriver is R1c+). Matches each DR
// branch to its old counterpart by identity to fill `branch_stratum`; fills the
// keyed join/crossover/product/drain maps directly. Bookkeeping only — no
// derivation.
void SeedDRStrata(
    DRFlowGraph &flow,
    const std::vector<OldBranchRef> &old_branches,
    const std::vector<OldJoinRef> &old_joins,
    const std::vector<OldCrossoverRef> &old_crossovers,
    const std::vector<OldProductRef> &old_products,
    const std::unordered_map<TABLE *, unsigned> &old_drain_stratum);

void ValidateDRInventory(
    const DRFlowGraph &flow,
    const std::vector<OldCrossoverRef> &old_crossovers,
    const std::vector<OldProductRef> &old_products,
    const std::vector<OldBranchRef> &old_branches,
    const std::vector<OldJoinRef> &old_joins,
    const std::unordered_map<TABLE *, unsigned> &old_scc_map,
    const std::unordered_map<TABLE *, unsigned> &old_drain_stratum);

}  // namespace hyde
