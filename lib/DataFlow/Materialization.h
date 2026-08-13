// Copyright 2026, Peter Goodman. All rights reserved.

#pragma once

#include <cstdint>
#include <optional>
#include <set>
#include <vector>

#include <drlojekyll/DataFlow/Query.h>

#include "Identity.h"
#include "InstanceFlow.h"

// MaterializationPlan (docs/proposals/InstanceFlow.md §10) — the RESOURCE
// authority, resources-first slice (session-34). A NEW typed IR authority that
// DERIVES today's stateful-storage decisions from the InstanceFlow grove and
// CROSS-CHECKS them byte-for-byte against the real `view_to_model` allocation
// ControlFlow performs later. It is a codegen-byte-identical OBSERVER (like the
// flat grove): nothing consumes it yet, so the first codegen move is Phase D by
// design. It is the CRITICAL-PATH input to Phase C (§11.3
// `BuildRel(query, instance_flow, materialization, ...)`): the Phase-C
// `TABLE*` retype happens "behind the materialization map", so a resource must
// be a WELL-DEFINED inverse of a physical table.
//
// GRAIN (session-34 grounding, refuter panel claim-d fold): ONE authoritative
// `StateResource` per stateful PHYSICAL storage class (`EquivalenceSetId`), NOT
// per logical collection. `StateResourceId <-> physical class` is a BIJECTION
// (V-MAT-BIJECTION) so `ResourceForView(v) = authority(EquivalenceSetId(v))` is
// total and single-valued — the property the `TABLE*`->`StateResourceId` retype
// depends on. Co-recursive collections that SHARE one store (reachable_from +
// reaching_to alias tc's table; ping + pong share one class) resolve through a
// single authoritative resource: the canonical (min-`LogicalCollectionId`)
// writer is the authority; the other collections are `ForwardingAlias` entries
// (§10 `aliases`). This satisfies §17 V-MAT-AUTHORITY (every stateful collection
// resolves to exactly one authoritative resource — its own or its alias's).
//
// SCOPE (session-34-seed.md §2.5.1, owner-recommended resources-only-first):
// derive + cross-check RESOURCES + aliases; DEFER arrangements (the §2.5.3 gap —
// no standing `collect_arrangement_requirements` pass; index requirements only
// materialize inline at the `GetOrCreateIndex` call sites mid-region-build).
//
// Like `RowContract` / `InstanceFlowProgram`, this is a PURE, RECOMPUTABLE
// function of the FINAL (post-Optimize, post-Stratify) graph, materialized ONCE
// in the `Query::Build` tail; never present during Optimize. Stored by value on
// `QueryImpl` beside `instance_flow`.

namespace hyde {

class QueryImpl;
class ErrorLog;

// ---------------------------------------------------------------------------
// Typed identity domains (InstanceFlow.md §4 / §10). Same `Identity.h` idiom as
// the grove ids: each reserves a DOMAIN; intra-domain equality/order only, no
// cross-domain operator, no implicit convert.
// ---------------------------------------------------------------------------

// One authoritative stateful-storage resource == one stateful physical class.
// Dense [0,R) in canonical plan order: all collection-authority resources first
// (by canonical `LogicalCollectionId`), then internal-residue resources (by
// ascending backing `EquivalenceSetId`). Deterministic — never a `TABLE*`
// pointer / iteration id.
struct StateResourceId {
  uint32_t v;
  constexpr bool operator==(const StateResourceId &) const noexcept = default;
  constexpr auto operator<=>(const StateResourceId &) const noexcept = default;
};

// A stateful backing store with NO logical INSERT-target collection (§10
// `InternalResidualCollectionId`): a JOIN input, an induction merge, a
// negated-view materialization, a differential-join output — the residue
// outside insert-named `LogicalCollectionId`s, exactly the Tier-2 provenance
// shape (`CollectOriginInteriorDecls` precedent). Dense [0,K) in ascending
// backing `EquivalenceSetId` order.
struct InternalResidualCollectionId {
  uint32_t v;
  constexpr bool operator==(const InternalResidualCollectionId &) const noexcept =
      default;
  constexpr auto operator<=>(const InternalResidualCollectionId &) const noexcept =
      default;
};

// The support regime of a resource's backing store — the `support=` token,
// mirroring the Regional row-contract convention (`differential` iff OR over
// the class's live members of `CanReceiveDeletions()`).
enum class SupportPolicy : uint8_t { kMonotone, kDifferential };

// ---------------------------------------------------------------------------
// The resources-first plan object model (InstanceFlow.md §10, resources subset).
// ---------------------------------------------------------------------------

// One authoritative stateful-storage resource (one physical class). Its
// `authority_for` is a `LogicalCollectionId` (the canonical collection writer)
// OR an `InternalResidualCollectionId` (a residue store) — exactly one is valid,
// selected by `internal` (the `OriginUse` two-field idiom).
struct StateResource {
  StateResourceId id;
  bool internal;                          // false: collection; true: residue
  LogicalCollectionId collection;         // valid iff !internal (canonical)
  InternalResidualCollectionId residual;  // valid iff internal
  SemanticMemberKey schema;   // canonical writer0 residual (collection) /
                              // representative visible columns (residue)
  SupportPolicy support;
  QueryOriginId representative;  // canonical writer0 (collection) / min-det_seq
                                 // class member (residue) — schema/name source
  unsigned eqset;                // backing EquivalenceSetId — the bijection key
};

// A non-canonical stateful collection that SHARES a physical class with the
// resource it points at (§10 `ForwardingAlias`). E.g. `reaching_to` aliasing
// `reachable_from`'s resource (both back tc's store). One per non-canonical
// stateful collection.
struct ForwardingAlias {
  LogicalCollectionId collection;  // the aliased (non-authoritative) collection
  StateResourceId to;              // the authoritative resource it resolves to
};

// The whole resources-first plan. Empty unless built at the Query::Build tail.
struct MaterializationResources {
  std::vector<StateResource> resources;  // one per stateful physical class
  std::vector<ForwardingAlias> aliases;  // shared-store non-canonical writers

  bool empty() const noexcept { return resources.empty(); }
};

// ---------------------------------------------------------------------------
// Derivation + validation (all PURE QueryView-API; NO lib/ControlFlow dep).
// ---------------------------------------------------------------------------

// Re-derive the set of stateful `EquivalenceSetId` storage classes from the
// FINAL graph by replaying the `FillDataModel` TABLE-need rules (Build.cpp:37)
// over the public `QueryView`/`QueryInsert` API. The result equals the classes
// ControlFlow's `FillDataModel` will make table-backed — the falsifiable claim
// the cross-check belt verifies. The two load-bearing premises (panel claim-a,
// verified in code): the DataModel classes ARE the `EquivalenceSetId` partition
// (`BuildDataModel` unions ONLY by `EquivalenceSetId`, and `GetOrCreate` sets a
// table without re-unioning), and every `TABLE` is created ONLY inside
// `FillDataModel`. R9 (the monotone-stream-tap rule) is applied in a second pass
// over the R1-R8 accumulated class set (its real order/mutation dependence — it
// runs LAST, reading whether the class is already table-backed).
std::set<unsigned> DeriveStatefulClasses(Query query);

// Build the resources-first plan for the FINAL graph. Must be called AFTER
// `BuildFlatInstanceFlow` (it reads the grove's collections). The grove is
// passed in (the caller holds the raw `QueryImpl`), so this is pure public-API
// and needs no friendship on the `Query` wrapper. The caller stores the result
// on `QueryImpl`.
MaterializationResources PlanResources(Query query,
                                       const InstanceFlowProgram &flow);

// V-MAT-AUTHORITY (§17) + V-MAT-BIJECTION (panel claim-d): resources <-> stateful
// classes is a bijection; every stateful collection resolves to exactly one
// authoritative resource (its own or its alias's); every ephemeral collection
// has none. Internal-invariant belt (fprintf+abort, surviving NDEBUG); the bool
// return is vestigial (kept for the `ValidateInstanceFlow` call-site idiom).
bool ValidateMaterialization(Query query, const InstanceFlowProgram &flow,
                             const MaterializationResources &plan,
                             const ErrorLog &log);

// Session-34 Phase-C: the stored resources plan, for lib/Rel to stamp each
// `DRTable` with its `StateResourceId` (the first `TABLE*`->resource retype,
// behind the materialization map). Reads `query.impl->materialization` (a `Query`
// friend). The plan is empty on a graph built before the tail; callers on the
// FINAL frozen graph always see the populated plan.
const MaterializationResources &MaterializationPlanOf(Query query);

}  // namespace hyde
