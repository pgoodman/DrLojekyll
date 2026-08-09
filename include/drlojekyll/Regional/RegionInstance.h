// Copyright 2026, Peter Goodman. All rights reserved.

#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <variant>
#include <vector>

// Stage C / Phase 3 of the Regional Dataflow proposal
// (RegionalDataFlowCore.artifacts/p3-grounding.md): the regional typed-id
// domains + the P3 request/derivation MODEL.
//
// This header hosts every regional typed-id domain in one place (the
// lib/DataFlow/Identity.h idiom: each id reserves a DOMAIN with intra-domain
// comparison only — no cross-domain operator, no implicit conversion). The
// Stage-B region-skeleton ids (`RegionId`/`PortId`/`EdgeId`/`RelationId`/
// `SymbolicFieldId`) live here so `Regional.h` (the typed-template + frozen-
// program header) can include this leaf and store a `RegionInstanceRelations`
// BY VALUE without a header cycle.
//
// P3 (the RequestEdge / RuleActivationEdge / FactDerivation / RoutedResult
// acyclic slice) adds the residual/ownership/support id domains + the
// `RegionInstanceRelations` model. At P3 this model is a COMPILE-TIME typed
// structure + self-validator, NOT a runtime engine: codegen is UNCHANGED and
// the compiled program's behavior is delivered entirely by the retained
// full-materialization backend (M3). `BuildRequestPorts` (lib/Regional/
// Planning.cpp) populates the request-ownership half at freeze; the
// derivation/routing half is DEFINED here and exercised by the Regional unit
// test (tests/RegionInstance) with synthetic rows — no real compile invokes
// `AddDerivation` (the rule authority `RegionTemplate::rules` is RESERVED-EMPTY
// until P6). Every id is DENSE, minted by deterministic declaration/range walks
// (the HP-9 rule) — never a pointer order, never a `UniqueId`.

namespace hyde {

// ==================== Stage-B region-skeleton id domains ====================
// (Moved here from Regional.h at P3 so the frozen program can hold the P3
// model by value. Semantics unchanged.)

struct RegionId {
  uint32_t v;

  constexpr bool operator==(const RegionId &) const noexcept = default;
  constexpr auto operator<=>(const RegionId &) const noexcept = default;
};

struct PortId {
  uint32_t v;

  constexpr bool operator==(const PortId &) const noexcept = default;
  constexpr auto operator<=>(const PortId &) const noexcept = default;
};

struct EdgeId {
  uint32_t v;

  constexpr bool operator==(const EdgeId &) const noexcept = default;
  constexpr auto operator<=>(const EdgeId &) const noexcept = default;
};

// A relation's stable logical identity (== `decl.Id()`), the key of a
// `RelationSchema` in the typed logical-fact authority and of a
// `RegionalFactId`'s relation field.
struct RelationId {
  uint64_t v;

  constexpr bool operator==(const RelationId &) const noexcept = default;
  constexpr auto operator<=>(const RelationId &) const noexcept = default;
};

// A decl-ordinal field identity (one per parameter position). RESERVED at
// Stage B (`RegionTemplate::inherited_symbolic_fields` is empty; P6.2 is the
// sole consumer).
struct SymbolicFieldId {
  uint32_t v;

  constexpr bool operator==(const SymbolicFieldId &) const noexcept = default;
  constexpr auto operator<=>(const SymbolicFieldId &) const noexcept = default;
};

// ==================== P3 residual / ownership / support ids ====================

// One per bound `#query` redeclaration adornment — an external request owner.
// P3 LIVE.
struct RootLeaseId {
  uint32_t v;

  constexpr bool operator==(const RootLeaseId &) const noexcept = default;
  constexpr auto operator<=>(const RootLeaseId &) const noexcept = default;
};

// One per all-free `#query` redeclaration — a permanent observation owner
// (`RootAlive ≡ true`, always). P3 LIVE.
struct PermanentRootId {
  uint32_t v;

  constexpr bool operator==(const PermanentRootId &) const noexcept = default;
  constexpr auto operator<=>(const PermanentRootId &) const noexcept = default;
};

// A lexical region instance. P3 LIVE-SINGLETON: only `RegionInstanceId{0}`
// exists at Stage B (one ProgramRoot, one region). P6.1 lifts the singleton.
struct RegionInstanceId {
  uint32_t v;

  constexpr bool operator==(const RegionInstanceId &) const noexcept = default;
  constexpr auto operator<=>(const RegionInstanceId &) const noexcept = default;
};

// The canonical bound-field SET of a binding state (order-free identity). P3
// LIVE-but-EMPTY-ONLY: only the empty schema `{0}` is interned.
struct BindingStateSchemaId {
  uint32_t v;

  constexpr bool operator==(const BindingStateSchemaId &) const noexcept =
      default;
  constexpr auto operator<=>(const BindingStateSchemaId &) const noexcept =
      default;
};

// Interns a `#query` redeclaration call site (1:1 with a dedup'd redecl). P3
// LIVE.
struct CallSiteId {
  uint32_t v;

  constexpr bool operator==(const CallSiteId &) const noexcept = default;
  constexpr auto operator<=>(const CallSiteId &) const noexcept = default;
};

// Dense per-`FactDerivation` id. P3 LIVE.
struct DerivationId {
  uint32_t v;

  constexpr bool operator==(const DerivationId &) const noexcept = default;
  constexpr auto operator<=>(const DerivationId &) const noexcept = default;
};

// A regional rule id. RESERVED: `RegionTemplate::rules` is empty at P3 (the M3
// backend performs derivation; P6.2 is the sole populator).
struct RuleId {
  uint32_t v;

  constexpr bool operator==(const RuleId &) const noexcept = default;
  constexpr auto operator<=>(const RuleId &) const noexcept = default;
};

// ==================== value / member substrate (H3) ====================
// The Regional layer names members and bound values by PROJECTED VALUES, never
// by the lib/DataFlow-private `FieldId` / `SemanticMemberKey`. The
// FieldId→positional conversion happens ONCE at P2
// (`BuildRelationSchemaFromInsert`, Planning.cpp) into
// `RelationSchema::member_key_positions`; P3 projects a row through that mask.

// A single projected member / bound-field cell value (the codegen cell repr;
// `uint64_t` at P3 — a P4 concern to confirm against the row-cell type once a
// concrete row reaches `AddDerivation`).
using RegionalCellValue = uint64_t;

// The residual bound-value tuple of a binding state. P3: ALWAYS EMPTY (the
// empty state). Non-empty tuples are P4+. NOTE: indexed by the schema's
// canonical (field-id-sorted) field order — NOT a value-sort — so within one
// schema `A=1,B=2` and `A=2,B=1` are distinct (P5 correctness, §8-F7).
using SortedBoundFieldValues = std::vector<RegionalCellValue>;

// A relation member named by the VALUES projected out of a row through the
// `RelationSchema::member_key_positions` mask — one entry per SET bit, in
// position order. The ONE positional projection F29 (fact interning) and F13
// (declared key) share; no second value-id bridge.
struct SemanticMemberIdentity {
  std::vector<RegionalCellValue> projected_values;

  bool operator==(const SemanticMemberIdentity &) const noexcept = default;
  auto operator<=>(const SemanticMemberIdentity &) const noexcept = default;
};

// ==================== composite (keyed) ids ====================

// A residual specialization. P3 interns to EXACTLY the empty state.
struct BindingStateId {
  RegionInstanceId ri;
  BindingStateSchemaId schema;
  SortedBoundFieldValues vals;  // P3: {}.

  bool operator==(const BindingStateId &) const noexcept = default;
  auto operator<=>(const BindingStateId &) const noexcept = default;
};

// The SOLE `BindingStateId` constructor exercised at P3.
inline BindingStateId EmptyBindingState(RegionInstanceId ri) {
  return BindingStateId{ri, BindingStateSchemaId{0u}, {}};
}

// Logical-fact identity — `RegionalFactRelation` is the SOLE fact owner.
struct RegionalFactId {
  RegionInstanceId ri;
  RelationId relation;         // == RelationSchema.id == decl.Id().
  SemanticMemberIdentity member;

  bool operator==(const RegionalFactId &) const noexcept = default;
  auto operator<=>(const RegionalFactId &) const noexcept = default;
};

// ==================== physical-access authority (P4) ====================
// The FOURTH authority (physical structure) — distinct from the logical fact id
// (RegionalFactId), the residual binding state (BindingStateId), and the logical
// access path (DeclaredAccessPath, P5). It NEVER aliases any of them. Three arms
// are LIVE at P4; each names an emission code generation actually produces. P7
// adds the trie/prefix arms and dual-homes a Rel plan_kind for interior/join
// scans (p4-grounding.md §3.1/§8-S4/S5).
enum class AccessPlan : uint8_t {
  kFullScanFilter = 0,     // withhold the index; full scan + bound-col filter
                           //   (a bound+free query read — an acyclic scan of the
                           //    fully-materialized relation, §8-S2)
  kFullKeyHashLookup = 1,  // the all-bound `.Find` — an honest full-key hash probe
  kRetainedIndexScan = 2,  // RESERVED (P7): keep the retained index seek as a
                           //   cost-based decision. Also the default/sentinel for a
                           //   record that carries no selected plan.
};

// P4 always reads a COMPLETE relation (an ordinary unbound-style read); kActiveSubset
// is a P5 residual-specialization concern.
enum class AccessCompleteness : uint8_t { kCompleteRelation, kActiveSubset /*P5*/ };

// What a request edge needs to read. `available_bindings` is decl-ordinal (P5 upgrades
// to typed BoundFieldValue + required_fields — §8-S11).
struct AccessRequirement {
  RelationId relation;
  bool has_free{false};
  std::vector<uint32_t> available_bindings;
  AccessCompleteness completeness{AccessCompleteness::kCompleteRelation};
};

// The branching selector (§8-S2/S4): a bound+free query read specializes to a full
// scan + bound-col filter (answer-correct even over a recursive relation — the read is
// an acyclic scan of the settled table, and the recursion's own indexes are untouched);
// an all-bound query keeps `.Find` (a full-key hash probe codegen already emits). P7
// adds the cost-based kRetainedIndexScan / trie arms.
inline AccessPlan SelectAccessPlan(const AccessRequirement &req) {
  if (!req.has_free) {
    return AccessPlan::kFullKeyHashLookup;
  }
  return AccessPlan::kFullScanFilter;
}

// ==================== ownership edges ====================
// RequestEdge (exact ownership, ACYCLIC forest) is NEVER unified with
// RuleActivationEdge (derivation dep, MAY cycle — RESERVED at P3).

// The subgraph-member ownership arm (RESERVED P6): a newtype wrapping a
// `RegionalFactId` so the OWNERSHIP authority never names a bare logical-fact
// id (the fact ⟂ owner separation; §8-F8).
struct RegionalMember {
  RegionalFactId fact;

  bool operator==(const RegionalMember &) const noexcept = default;
  auto operator<=>(const RegionalMember &) const noexcept = default;
};

// P3-live arms: RootLease + PermanentRoot. RegionalMember is RESERVED (P6).
using RequestOwnerId =
    std::variant<RootLeaseId, PermanentRootId, RegionalMember>;

// P3 LIVE. A 2nd owner over the same (call_site, dest) is a DISTINCT edge.
struct RequestEdgeId {
  RequestOwnerId owner;
  CallSiteId call_site;
  BindingStateId dest;  // P3: always EmptyBindingState(ri).

  bool operator==(const RequestEdgeId &) const noexcept = default;
  auto operator<=>(const RequestEdgeId &) const noexcept = default;
};

// RESERVED P4/P5: a cross-state derivation dependency (MAY cycle in a region;
// liveness = rooted reachability). `DeriveActivationEdge` mints NONE at P3 (no
// intra-state self-loop; the acyclic RULE DAG carries order — F18).
struct RuleActivationEdgeId {
  BindingStateId src_state;
  RegionalFactId src_fact;
  RuleId rule;
  BindingStateId dst;

  bool operator==(const RuleActivationEdgeId &) const noexcept = default;
  auto operator<=>(const RuleActivationEdgeId &) const noexcept = default;
};

// ==================== support authority (B4) ====================
// FactDerivation is keyed BY `RegionalFactId` — a PER-FACT support counter, the
// single `RegionalFactRelation`. It is NOT a per-BindingState C_nr/C_r mirror
// (that would make a binding state a second fact owner). `support` is a plain
// count at P3 (the acyclic slice has no recursive/non-recursive split; P6
// upgrades it to the split `DerivationSupportCount`). `contributing_states` is
// the SET of binding states that derive this fact — routing keys on this set,
// never on a single `src_state` (§8-F3).
struct FactDerivation {
  DerivationId id{0u};
  RegionalFactId fact;
  int64_t support{0};
  std::set<BindingStateId> contributing_states;
};

// A canonical fact routed to a request edge's requester (caller-qualified).
struct RoutedResultId {
  RequestEdgeId request_edge;
  RegionalFactId fact;

  bool operator==(const RoutedResultId &) const noexcept = default;
  auto operator<=>(const RoutedResultId &) const noexcept = default;
};

// ==================== the P3 model container ====================
// A freeze-side peer of `RegionTemplate`, stored by value on
// `FrozenRegionalProgram`. The request-ownership half is populated by
// `BuildRequestPorts` for real programs; the derivation/routing half stays
// empty for real compiles (no rule sweep at P3) and is exercised by the
// Regional unit test.
struct RegionInstanceRelations {
  std::set<RequestEdgeId> request_edges;
  std::set<RuleActivationEdgeId> activation_edges;  // RESERVED-EMPTY at P3.
  std::map<RegionalFactId, FactDerivation> derivations;  // B4: per-fact.
  std::set<RoutedResultId> routed_results;

  // L5: the relation a request owner asked for (to filter routing). Keyed on
  // the owner (1:1 with a `#query` redecl at P3).
  std::map<RequestOwnerId, RelationId> requested_relation_of_owner;

  uint32_t next_derivation{0u};

  // At P3 every request owner is RootAlive (the M3 backend materializes
  // unconditionally — §8-F2): RootLease is alive though no lease is runtime-
  // held, PermanentRoot ≡ true. RegionalMember (P6) is not yet minted.
  static bool RootAlive(const RequestOwnerId &owner) {
    return !std::holds_alternative<RegionalMember>(owner);
  }

  // Intern a request edge. A 2nd owner over the same (call_site, dest) is a
  // DISTINCT edge and re-derives NO facts (ownership ⟂ support).
  RequestEdgeId AddRequestEdge(RequestOwnerId owner, CallSiteId call_site,
                               BindingStateId dest, RelationId requested) {
    RequestEdgeId e{std::move(owner), call_site, std::move(dest)};
    request_edges.insert(e);
    requested_relation_of_owner[e.owner] = requested;
    return e;
  }

  // Caller-qualified removal: erase the edge + only ITS routed copies. The
  // fact stays present iff another edge still routes it; derivations untouched.
  void RemoveRequestEdge(const RequestEdgeId &e) {
    request_edges.erase(e);
    for (auto it = routed_results.begin(); it != routed_results.end();) {
      if (it->request_edge == e) {
        it = routed_results.erase(it);
      } else {
        ++it;
      }
    }
  }

  // Project a row through the positional member-key mask (H3): one value per
  // SET bit, in position order. `mask` is size == decl.Arity(), decl-ordinal;
  // `row` is the head relation's column tuple in the same decl-ordinal space.
  static SemanticMemberIdentity ProjectRow(
      const std::vector<bool> &mask, const std::vector<RegionalCellValue> &row) {
    SemanticMemberIdentity member;
    for (size_t i = 0u; i < mask.size(); ++i) {
      if (mask[i] && i < row.size()) {
        member.projected_values.push_back(row[i]);
      }
    }
    return member;
  }

  // Add/remove one derivation of a fact from a binding state. `delta` is +1
  // (add) or -1 (remove). Fact identity projects the row through the positional
  // mask (H3/F29: two rows agreeing on member-key columns intern to ONE fact).
  // Support is per-fact (B4). Publish transitions are the 0<->>0 crossings.
  RegionalFactId AddDerivation(const BindingStateId &src_state,
                               RelationId relation, const std::vector<bool> &mask,
                               const std::vector<RegionalCellValue> &row,
                               int64_t delta) {
    RegionalFactId fact{src_state.ri, relation, ProjectRow(mask, row)};
    auto &d = derivations[fact];
    if (d.support == 0 && d.contributing_states.empty()) {
      d.id = DerivationId{next_derivation++};
      d.fact = fact;
    }
    d.support += delta;
    if (delta > 0) {
      d.contributing_states.insert(src_state);
    }
    return fact;
  }

  // Rooted reachability over the ACYCLIC request forest (∪ the empty
  // activation set at P3). The region is rooted by the ALWAYS-PRESENT
  // ProgramRoot (§8-F1): `live` always contains the ProgramRoot's empty state,
  // so a no-`#query` publisher is live. Live request edges ADD observation
  // roots. One topo sweep, no fixpoint (activation_edges is empty at P3).
  std::set<BindingStateId> RootedReachability(RegionInstanceId root_ri) const {
    std::set<BindingStateId> live;
    live.insert(EmptyBindingState(root_ri));  // ProgramRoot seed.
    for (const RequestEdgeId &e : request_edges) {
      if (RootAlive(e.owner)) {
        live.insert(e.dest);
      }
    }
    // activation_edges is empty at P3 → no propagation. (P4/P5 add the sweep.)
    return live;
  }

  // Route each fact derived in `st` (support > 0) to every request edge whose
  // dest is `st`, FILTERED by the request edge's requested relation (L5) and by
  // whether `st` actually contributes the fact (F3 — never a single src_state).
  void RouteResults(const BindingStateId &st) {
    for (const RequestEdgeId &e : request_edges) {
      if (e.dest != st) {
        continue;
      }
      auto rel_it = requested_relation_of_owner.find(e.owner);
      for (const auto &[fact, d] : derivations) {
        if (d.support <= 0 || !d.contributing_states.count(st)) {
          continue;
        }
        if (rel_it != requested_relation_of_owner.end() &&
            !(fact.relation == rel_it->second)) {
          continue;  // L5: a #query on p never receives edge/q facts.
        }
        routed_results.insert(RoutedResultId{e, fact});
      }
    }
  }
};

}  // namespace hyde
