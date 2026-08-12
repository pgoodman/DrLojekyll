// Copyright 2026, Peter Goodman. All rights reserved.

#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <unordered_map>
#include <vector>

#include <drlojekyll/DataFlow/Query.h>

#include "Identity.h"

// InstanceFlow (docs/proposals/InstanceFlow.md): a NEW compiler IR between the
// optimized logical Query and Rel that represents a deterministic GROVE of
// context-parameterized computation families built backward from consumer uses.
//
// SCOPE (session-32, owner-ratified NARROW first slice — see
// docs/proposals/InstanceFlow.artifacts/session-32-phaseA-grounding.md): Phase A
// typed-identity catalogs + the FLAT empty-context grove + the always-on
// validators. The grove is an OBSERVER at the Query->Rel seam — nothing consumes
// it yet, so codegen is byte-identical. This is the maximally-shared FlatFamily
// baseline (InstanceFlow.md §7.3): every family binds NO context, every node's
// residual schema IS its full logical schema. The grove is partitioned by the
// Query SCC condensation (grounding finding B4 / §6 typed `scc_ownership`): one
// `kAcyclic` family for all trivial components, one `kWholeQueryScc` family per
// non-trivial SCC — so a family never straddles a recursive SCC (§7.6 is
// discharged structurally).
//
// Like `RowContract`, the grove is a PURE, RECOMPUTABLE function of the FINAL
// (post-Optimize, post-Stratify) graph, materialized ONCE in the `Query::Build`
// tail; never present during Optimize (the F1 lesson). It is stored by value on
// `QueryImpl` beside `row_contracts`.

namespace hyde {

class QueryImpl;
class ErrorLog;

// ---------------------------------------------------------------------------
// Typed identity domains (InstanceFlow.md §4). Each reserves a DOMAIN; only
// intra-domain equality/order (defaulted, same-type parameter) — no cross-domain
// operator, no implicit convert, exactly the `Identity.h` idiom. A grove id must
// never silently stand in for another domain's id.
// ---------------------------------------------------------------------------

// A live post-optimization Query view occurrence. Dense [0, N). SOURCE: the
// view's `DeterministicOrder()` (`det_seq`, stamped once at `IdentifyInductions`
// — run-stable, pointer-INDEPENDENT). A det_seq bijection onto live views.
struct QueryOriginId {
  uint32_t v;
  constexpr bool operator==(const QueryOriginId &) const noexcept = default;
  constexpr auto operator<=>(const QueryOriginId &) const noexcept = default;
};

// One producer->consumer column-role edge, OR one root boundary obligation
// (terminal INSERT / bound-#query read). Dense [0, M) in the canonical total
// order (grounding finding B1). No tip source — minted by the canonical walk.
struct OriginUseId {
  uint32_t v;
  constexpr bool operator==(const OriginUseId &) const noexcept = default;
  constexpr auto operator<=>(const OriginUseId &) const noexcept = default;
};

// A logical set-valued truth collection (§8.1). Dense [0, C). SOURCE: interned
// `ParsedDeclaration::Id()` over LIVE INSERT views only (grounding S1) — message
// inputs and read-only bound-query relations are ports, NOT collections. MUST
// NOT be conflated with `EquivalenceSetId` (co-recursive relations may share one
// storage set yet be two collections).
struct LogicalCollectionId {
  uint32_t v;
  constexpr bool operator==(const LogicalCollectionId &) const noexcept = default;
  constexpr auto operator<=>(const LogicalCollectionId &) const noexcept = default;
};

// One live INSERT view into a collection (grounding candidate (a): per live
// INSERT, NOT per parse rule — CSE folds rule provenance unpredictably). Dense
// [0, D). One writer per site by construction.
struct DerivationSiteId {
  uint32_t v;
  constexpr bool operator==(const DerivationSiteId &) const noexcept = default;
  constexpr auto operator<=>(const DerivationSiteId &) const noexcept = default;
};

// The sole writer of one derivation coverage cell (§8.4). 1:1 with
// `DerivationSiteId` in the flat slice (domain == All).
struct EmissionAuthorityId {
  uint32_t v;
  constexpr bool operator==(const EmissionAuthorityId &) const noexcept = default;
  constexpr auto operator<=>(const EmissionAuthorityId &) const noexcept = default;
};

// One parameterized computation family = one SCC-condensation component in the
// flat slice. Dense [0, F).
struct FamilyId {
  uint32_t v;
  constexpr bool operator==(const FamilyId &) const noexcept = default;
  constexpr auto operator<=>(const FamilyId &) const noexcept = default;
};

// One Query-origin occurrence inside a family. `(family, family-local index)`.
struct FamilyNodeId {
  uint32_t family;
  uint32_t local;
  constexpr bool operator==(const FamilyNodeId &) const noexcept = default;
  constexpr auto operator<=>(const FamilyNodeId &) const noexcept = default;
};

// One atomic recursive component (§4 `ActivationSccId` peer, Query-side). A
// dense remap of `QueryView::InductionGroupId()`.
struct QuerySccId {
  uint32_t v;
  constexpr bool operator==(const QuerySccId &) const noexcept = default;
  constexpr auto operator<=>(const QuerySccId &) const noexcept = default;
};

// ---------------------------------------------------------------------------
// The shared canonical live-view walk (grounding B3/R1-F6): ONE ordering
// contract for the grove builder, the dump, and `QueryContracts`. The SAME
// kind-tagged det_seq-order per-kind DefList traversal the `.df`/`-contract-out`
// dumps use (NEVER `Query::ForEachView`, which walks JOINs first). Ordinals are
// the canonical dump order; kept stable — `QueryContracts` branches on `kInsert`.
// ---------------------------------------------------------------------------

enum ViewKindTag : unsigned {
  kIFSelect = 0u,
  kIFTuple = 1u,
  kIFKVIndex = 2u,
  kIFJoin = 3u,
  kIFMap = 4u,
  kIFAggregate = 5u,
  kIFMerge = 6u,
  kIFNegate = 7u,
  kIFCompare = 8u,
  kIFInsert = 9u,
};

// Visit every LIVE view once, in canonical `.df` order, tagged with its
// `ViewKindTag` ordinal and stable literal name. Defined in InstanceFlow.cpp.
void ForEachViewKindTagged(
    Query query,
    const std::function<void(QueryView, unsigned kind, const char *tag)> &cb);

// The VISIBLE columns of a view: an INSERT's INPUT columns (it is terminal),
// every other kind's OUTPUT columns. Shared by the builder, the validators, and
// the `-instanceflow-out` dump (one visible-schema contract). `kind` is a
// `ViewKindTag` ordinal.
std::vector<QueryColumn> VisibleColumnsOf(QueryView v, unsigned kind);

// ---------------------------------------------------------------------------
// The flat grove object model (InstanceFlow.md §6, empty-context subset).
// ---------------------------------------------------------------------------

// A node's position in its family topology.
enum class OccurrenceRole : uint8_t { kRoot, kInterior };

// Whether a family owns a whole recursive SCC or a set of acyclic components.
enum class SccOwnership : uint8_t { kAcyclic, kWholeQueryScc };

// The obligation class of an OriginUse.
enum class UseClass : uint8_t {
  kInterior,       // a producer->consumer column-role edge inside the grove
  kTerminalInsert, // an INSERT boundary obligation (carries an authority)
  kBoundQueryRead, // a bound-#query read of a collection (no authority)
};

// One Query-origin occurrence inside a family. Empty context in this slice:
// `residual` IS the full logical schema; no transfers.
struct FamilyNode {
  FamilyNodeId id;
  QueryOriginId origin;
  unsigned kind;              // ViewKindTag ordinal
  const char *tag;            // stable literal ("select", "join", ...)
  OccurrenceRole role;
  SemanticMemberKey residual; // full visible schema (column value ids)
  std::optional<LogicalCollectionId> output_collection;  // INSERT nodes only
};

// One OriginUse record. Authority is NOT stored here (grounding B2): a terminal
// use maps to its site; emission lives in `authorities`.
struct OriginUse {
  OriginUseId id;
  QueryOriginId consumer;   // the view whose ForEachUse produced this edge
  QueryOriginId producer;   // Containing(in) (a literal SELECT for constants)
  uint32_t producer_col;    // in.Index(), or UINT32_MAX for a constant (col=*)
  uint32_t consumer_out_col;// consumer slot ordinal (total-order tie-break, B1)
  unsigned role;            // InputColumnRole ordinal
  UseClass cls;
  DerivationSiteId terminal_site;    // valid iff cls == kTerminalInsert
  LogicalCollectionId read_collection;  // valid iff cls == kBoundQueryRead
  std::vector<uint32_t> read_bound_cols;  // bound param indices; kBoundQueryRead
};

// A family = one SCC-condensation component (grounding B4).
struct Family {
  FamilyId id;
  SccOwnership ownership;
  std::optional<QuerySccId> scc;  // set iff ownership == kWholeQueryScc
  std::optional<OriginUseId> root_use;
  std::vector<FamilyNode> nodes;  // family-local det_seq order
  std::vector<OriginUseId> covers;
};

// The coverage assignment: every use covered by exactly one occurrence node
// (grounding B2 — a per-consumer-obligation bijection, no authority field).
struct UseCoverage {
  OriginUseId use;
  FamilyNodeId occurrence;
};

// The sole writer of one (site, domain=All) cell.
struct EmissionAuthority {
  EmissionAuthorityId id;
  DerivationSiteId site;
  FamilyNodeId writer;  // the INSERT-origin node
};

// A logical collection catalog entry.
struct LogicalCollection {
  LogicalCollectionId id;
  uint64_t decl_id;      // ParsedDeclaration::Id()
  QueryOriginId writer0; // first-seen INSERT view (name/arity source)
};

// A derivation site catalog entry (one live INSERT).
struct DerivationSite {
  DerivationSiteId id;
  QueryOriginId writer;             // the INSERT view
  LogicalCollectionId collection;
};

// A candidate context seed (§7.2) — COMPUTED for review, NEVER goldened
// (grounding S5): the §5.1 literal-operand vocabulary is unresolved.
struct CandidateSeed {
  enum class Kind : uint8_t { kJoinPivot, kAggregateGroup, kBoundaryBinding };
  Kind kind;
  QueryOriginId origin;
  std::vector<uint32_t> key_cols;  // pivot / group / bound positions
};

// The whole flat grove. Empty unless built at the Query::Build tail.
struct InstanceFlowProgram {
  std::vector<QueryOriginId> origins;       // dense, det_seq order
  std::vector<OriginUse> uses;              // total-ordered (B1)
  std::vector<LogicalCollection> collections;  // INSERT-target only (S1)
  std::vector<DerivationSite> sites;
  std::vector<Family> families;             // SCC-partitioned (B4)
  std::vector<UseCoverage> coverage;        // bijection onto uses (B2)
  std::vector<EmissionAuthority> authorities;  // bijection onto sites (B2)
  std::vector<CandidateSeed> seeds;         // computed, un-goldened (S5)

  bool empty() const noexcept { return origins.empty(); }
};

// Build the flat empty-context grove for the FINAL graph. Must be called AFTER
// `Stratify` and `FinalizeColumnIDs` (it reads `det_seq`, `InductionGroupId`,
// and finalized column ids). Takes a shared-ownership `Query` wrapper (the
// public range/lineage API); the caller stores the result on `QueryImpl`.
InstanceFlowProgram BuildFlatInstanceFlow(Query query);

// Run the always-on grove validators (V-IF-ORIGIN / V-IF-CONTEXT /
// V-IF-COVERAGE / V-IF-EMISSION / V-IF-SCC) over `flow`. Returns `true` on
// success. Violations are internal-invariant belts (fprintf+abort, surviving
// NDEBUG); on a correct pipeline it fires nothing (abort-only — the bool return
// is vestigial, kept for the `ValidateRowContracts` call-site idiom).
bool ValidateInstanceFlow(Query query, const InstanceFlowProgram &flow,
                          const ErrorLog &log);

}  // namespace hyde
