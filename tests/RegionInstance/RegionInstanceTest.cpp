// Copyright 2026, Peter Goodman. All rights reserved.
//
// P3 exit gate (the F4a deliverable, RegionalDataFlowCore.artifacts/
// p3-grounding.md §7/§8-F4): the DISCRIMINATING structural battery over the
// P3 request/derivation model (include/drlojekyll/Regional/RegionInstance.h).
// A P3 STUB that builds request edges but never populates derivations/
// routed_results FAILS these gates — unlike the -region-out goldens, which a
// no-op could pass. Two layers:
//   (1) a compile-time static_assert disjointness battery over the P3 id
//       domains (a regression that lets an owner id stand in for a fact id, or
//       a support count for an owner, fails the BUILD);
//   (2) runtime TESTs driving AddRequestEdge / RemoveRequestEdge /
//       AddDerivation / RouteResults / RootedReachability with SYNTHETIC rows,
//       one per numbered gate.

#include <DrTest.h>

#include <type_traits>

#include <drlojekyll/Regional/RegionInstance.h>

namespace {

using hyde::BindingStateId;
using hyde::CallSiteId;
using hyde::EmptyBindingState;
using hyde::PermanentRootId;
using hyde::RegionalFactId;
using hyde::RegionalMember;
using hyde::RegionInstanceId;
using hyde::RegionInstanceRelations;
using hyde::RelationId;
using hyde::RequestEdgeId;
using hyde::RequestOwnerId;
using hyde::RootLeaseId;

// ---- (1) compile-time disjointness: no cross-domain implicit conversion.
// The four authorities + two edges must never collapse to one integer domain.

template <typename A, typename B>
constexpr bool Disjoint() {
  static_assert(!std::is_convertible_v<A, B>, "A must not convert to B");
  static_assert(!std::is_convertible_v<B, A>, "B must not convert to A");
  return true;
}

// Ownership ids are pairwise disjoint (an owner can never silently be another).
static_assert(Disjoint<RootLeaseId, PermanentRootId>());
static_assert(Disjoint<RootLeaseId, CallSiteId>());
static_assert(Disjoint<PermanentRootId, CallSiteId>());
// A logical-fact id is not a binding-state id (fact ⟂ residual — the SOLE fact
// owner is RegionalFactRelation, a binding state is not a second fact owner).
static_assert(Disjoint<RegionalFactId, BindingStateId>());
// A support count (int64_t) can never stand in for an owner id (count ⟂ owner).
static_assert(Disjoint<int64_t, RootLeaseId>());
static_assert(Disjoint<int64_t, RegionalFactId>());
// The reserved P6 subgraph-member owner wraps a fact id in a newtype, so the
// ownership authority never NAMES a bare logical-fact id.
static_assert(!std::is_convertible_v<RegionalFactId, RegionalMember>);

// ---- helpers ----

static const RegionInstanceId kRi{0u};
static const RelationId kRelP{100u};  // "relation p".
static const RelationId kRelQ{200u};  // "relation q".

// The member-key mask projects column 0 only (member key = first column).
static const std::vector<bool> kKeyCol0{true, false};

}  // namespace

// ---- (2) runtime gates ----

// Gate 5 (F29 member-key intern): two rows AGREEING on the member-key columns
// (differing elsewhere) intern to ONE RegionalFactId; a row DIFFERING there
// interns to another. This is the sole P3 exercise of AddDerivation's identity.
TEST(RegionInstanceP3, Gate5_MemberKeyIntern) {
  RegionInstanceRelations rr;
  const BindingStateId st = EmptyBindingState(kRi);

  // rows (7, 100) and (7, 999) agree on col 0 -> same fact, support 2.
  const RegionalFactId f1 =
      rr.AddDerivation(st, kRelP, kKeyCol0, {7u, 100u}, +1);
  const RegionalFactId f2 =
      rr.AddDerivation(st, kRelP, kKeyCol0, {7u, 999u}, +1);
  ASSERT_TRUE(f1 == f2);

  // row (8, 100) differs on col 0 -> distinct fact.
  const RegionalFactId f3 =
      rr.AddDerivation(st, kRelP, kKeyCol0, {8u, 100u}, +1);
  ASSERT_FALSE(f1 == f3);

  ASSERT_EQ(rr.derivations.size(), 2u);            // exactly two facts.
  ASSERT_EQ(rr.derivations[f1].support, int64_t{2});  // (7,*) folded.
  ASSERT_EQ(rr.derivations[f3].support, int64_t{1});
}

// Gate 1 (owner-count): distinct owners requesting the same dest are DISTINCT
// request edges; no fact is minted by AddRequestEdge (ownership ⟂ support).
TEST(RegionInstanceP3, Gate1_OwnerCount) {
  RegionInstanceRelations rr;
  const BindingStateId st = EmptyBindingState(kRi);
  rr.AddRequestEdge(RequestOwnerId{RootLeaseId{0u}}, CallSiteId{0u}, st, kRelP);
  rr.AddRequestEdge(RequestOwnerId{PermanentRootId{0u}}, CallSiteId{1u}, st,
                    kRelP);
  ASSERT_EQ(rr.request_edges.size(), 2u);
  ASSERT_EQ(rr.derivations.size(), 0u);  // AddRequestEdge derives nothing.
}

// Gate 2 (2nd requester adds RoutedResults, ZERO new FactDerivations).
TEST(RegionInstanceP3, Gate2_SecondRequesterRoutesNoNewDerivations) {
  RegionInstanceRelations rr;
  const BindingStateId st = EmptyBindingState(kRi);
  rr.AddDerivation(st, kRelP, kKeyCol0, {7u, 100u}, +1);  // one fact.

  rr.AddRequestEdge(RequestOwnerId{RootLeaseId{0u}}, CallSiteId{0u}, st, kRelP);
  rr.RouteResults(st);
  ASSERT_EQ(rr.routed_results.size(), 1u);
  ASSERT_EQ(rr.derivations.size(), 1u);

  // A SECOND requester over the same relation: a new routed copy, NO new fact.
  rr.AddRequestEdge(RequestOwnerId{PermanentRootId{0u}}, CallSiteId{1u}, st,
                    kRelP);
  rr.RouteResults(st);
  ASSERT_EQ(rr.routed_results.size(), 2u);  // two routed copies...
  ASSERT_EQ(rr.derivations.size(), 1u);     // ...one shared fact.
}

// Gate 3 (caller-qualified removal): RemoveRequestEdge on one of two retracts
// only ITS routed copies; the fact stays present (derivations untouched).
TEST(RegionInstanceP3, Gate3_RemoveRetractsOnlyRouted) {
  RegionInstanceRelations rr;
  const BindingStateId st = EmptyBindingState(kRi);
  rr.AddDerivation(st, kRelP, kKeyCol0, {7u, 100u}, +1);

  const RequestEdgeId e1 = rr.AddRequestEdge(
      RequestOwnerId{RootLeaseId{0u}}, CallSiteId{0u}, st, kRelP);
  rr.AddRequestEdge(RequestOwnerId{PermanentRootId{0u}}, CallSiteId{1u}, st,
                    kRelP);
  rr.RouteResults(st);
  ASSERT_EQ(rr.routed_results.size(), 2u);

  rr.RemoveRequestEdge(e1);
  ASSERT_EQ(rr.request_edges.size(), 1u);   // one owner remains.
  ASSERT_EQ(rr.routed_results.size(), 1u);  // only e1's routed copy dropped.
  ASSERT_EQ(rr.derivations.size(), 1u);     // the fact stays present.
}

// Gate 4 (support ⟂ ownership): the per-fact support count is independent of
// how many request edges observe the fact.
TEST(RegionInstanceP3, Gate4_SupportIndependentOfOwnership) {
  RegionInstanceRelations rr;
  const BindingStateId st = EmptyBindingState(kRi);
  const RegionalFactId f =
      rr.AddDerivation(st, kRelP, kKeyCol0, {7u, 100u}, +1);

  for (uint32_t i = 0u; i < 3u; ++i) {
    rr.AddRequestEdge(RequestOwnerId{RootLeaseId{i}}, CallSiteId{i}, st, kRelP);
  }
  ASSERT_EQ(rr.derivations[f].support, int64_t{1});  // ownership did not touch it.

  rr.AddDerivation(st, kRelP, kKeyCol0, {7u, 42u}, +1);  // 2nd support, same fact.
  ASSERT_EQ(rr.derivations[f].support, int64_t{2});
  ASSERT_EQ(rr.request_edges.size(), 3u);  // ...support did not touch ownership.
}

// Gate 6 (ProgramRoot rooting, §8-F1): the region's empty state is live even
// with ZERO request edges (a no-#query publisher must still be live). Request
// edges only ADD observation roots.
TEST(RegionInstanceP3, Gate6_ProgramRootRoots) {
  RegionInstanceRelations rr;
  const auto live_bare = rr.RootedReachability(kRi);
  ASSERT_EQ(live_bare.size(), 1u);  // the ProgramRoot empty state alone.
  ASSERT_TRUE(live_bare.count(EmptyBindingState(kRi)) == 1u);

  rr.AddRequestEdge(RequestOwnerId{RootLeaseId{0u}}, CallSiteId{0u},
                    EmptyBindingState(kRi), kRelP);
  const auto live_with_edge = rr.RootedReachability(kRi);
  ASSERT_EQ(live_with_edge.size(), 1u);  // dest == the same empty state.
  ASSERT_TRUE(live_with_edge.count(EmptyBindingState(kRi)) == 1u);
}

// L5 (route by requested relation): a request edge asking for q never receives
// a fact of p in the single-empty-state world.
TEST(RegionInstanceP3, L5_RouteFiltersByRequestedRelation) {
  RegionInstanceRelations rr;
  const BindingStateId st = EmptyBindingState(kRi);
  rr.AddDerivation(st, kRelP, kKeyCol0, {7u, 100u}, +1);  // a fact of p.

  // A requester of q: routes nothing (relation mismatch).
  rr.AddRequestEdge(RequestOwnerId{RootLeaseId{0u}}, CallSiteId{0u}, st, kRelQ);
  rr.RouteResults(st);
  ASSERT_EQ(rr.routed_results.size(), 0u);

  // A requester of p: routes the fact.
  rr.AddRequestEdge(RequestOwnerId{PermanentRootId{0u}}, CallSiteId{1u}, st,
                    kRelP);
  rr.RouteResults(st);
  ASSERT_EQ(rr.routed_results.size(), 1u);
}

// Two-edge separation / acyclic forest: at P3 no activation edge exists and
// RemoveRequestEdge never touches derivations, so the request forest is a
// depth-1 acyclic set (dest is always the empty state, which owns no outgoing
// request edge).
TEST(RegionInstanceP3, AcyclicRequestForest) {
  RegionInstanceRelations rr;
  const BindingStateId st = EmptyBindingState(kRi);
  const RegionalFactId f =
      rr.AddDerivation(st, kRelP, kKeyCol0, {7u, 100u}, +1);
  const RequestEdgeId e = rr.AddRequestEdge(
      RequestOwnerId{RootLeaseId{0u}}, CallSiteId{0u}, st, kRelP);

  ASSERT_EQ(rr.activation_edges.size(), 0u);  // no cross-state edges at P3.
  rr.RemoveRequestEdge(e);
  ASSERT_EQ(rr.derivations.size(), 1u);       // ownership removal ⟂ support.
  ASSERT_EQ(rr.derivations[f].support, int64_t{1});
}

// ---- (3) P4 gates: the AccessPlan authority + the model driving the acyclic
// complete-path slice over a NON-EMPTY binding state (p4-grounding.md §4 B3-3).
// These fail for a P3-era model that never leaves the empty state, and for a P4
// that wires the plan selector but not the FullScanFilter -> AddDerivation ->
// RouteResults path.

using hyde::AccessCompleteness;
using hyde::AccessPlan;
using hyde::AccessRequirement;
using hyde::BindingStateSchemaId;
using hyde::SelectAccessPlan;

// A non-empty binding state: schema `sch`, bound value tuple `vals`.
static BindingStateId KeyedState(uint32_t sch,
                                 std::vector<hyde::RegionalCellValue> vals) {
  return BindingStateId{kRi, BindingStateSchemaId{sch}, std::move(vals)};
}

// The abstract FullScanFilter iterator (p4-grounding.md §6): scan synthetic rows,
// keep those whose bound columns equal the state's bound values, feed each to
// AddDerivation. Returns the number of rows that passed the filter.
static unsigned DriveFullScanFilter(
    RegionInstanceRelations &rr, const BindingStateId &st, RelationId rel,
    const std::vector<bool> &member_mask, uint32_t bound_col, uint64_t bound_val,
    const std::vector<std::vector<hyde::RegionalCellValue>> &rows) {
  unsigned kept = 0u;
  for (const auto &row : rows) {
    if (bound_col < row.size() && row[bound_col] == bound_val) {
      rr.AddDerivation(st, rel, member_mask, row, +1);
      ++kept;
    }
  }
  return kept;
}

// P4 Gate A (selector): the AccessPlan authority classifies by binding arity.
// bound+free -> full scan; all-bound -> full-key hash probe (p4-grounding.md §3.1).
TEST(RegionInstanceP4, GateA_SelectAccessPlan) {
  const AccessRequirement bound_free{kRelP, /*has_free=*/true, {0u},
                                     AccessCompleteness::kCompleteRelation};
  const AccessRequirement all_bound{kRelP, /*has_free=*/false, {0u, 1u},
                                    AccessCompleteness::kCompleteRelation};
  ASSERT_TRUE(SelectAccessPlan(bound_free) == AccessPlan::kFullScanFilter);
  ASSERT_TRUE(SelectAccessPlan(all_bound) == AccessPlan::kFullKeyHashLookup);
}

// P4 Gate B (the model DRIVES the acyclic slice): a FullScanFilter over a
// NON-EMPTY keyed state feeds AddDerivation, and RouteResults routes the derived
// facts to that state's requester. A model stuck at the empty state, or one that
// wires SelectAccessPlan but not the scan->derive->route path, produces zero
// routed results here.
TEST(RegionInstanceP4, GateB_FullScanFilterDrivesRouting) {
  RegionInstanceRelations rr;
  const BindingStateId st = KeyedState(1u, {7u});  // schema {col0}, value 7.

  // Rows of p: (7,100),(7,999) pass the col-0==7 filter; (8,100) does not.
  const unsigned kept = DriveFullScanFilter(
      rr, st, kRelP, kKeyCol0, /*bound_col=*/0u, /*bound_val=*/7u,
      {{7u, 100u}, {7u, 999u}, {8u, 100u}});
  ASSERT_EQ(kept, 2u);
  // (7,100) and (7,999) agree on the member key col0 -> ONE fact.
  ASSERT_EQ(rr.derivations.size(), 1u);

  rr.AddRequestEdge(RequestOwnerId{RootLeaseId{0u}}, CallSiteId{0u}, st, kRelP);
  rr.RouteResults(st);
  ASSERT_EQ(rr.routed_results.size(), 1u);  // the derived fact reaches the requester.
}

// P4 Gate C (S6/S9 - the NON-EMPTY state is load-bearing): two DISTINCT keyed
// states route DISJOINTLY; a fact derived in one is not routed to the other's
// requester. An empty-state-only model cannot exhibit this (there is only one
// state to route to), so this gate discriminates a real P4 from a P3 stub.
TEST(RegionInstanceP4, GateC_DistinctStatesRouteDisjointly) {
  RegionInstanceRelations rr;
  const BindingStateId s7 = KeyedState(1u, {7u});
  const BindingStateId s8 = KeyedState(1u, {8u});
  ASSERT_FALSE(s7 == s8);  // distinct binding identities within one schema.

  rr.AddDerivation(s7, kRelP, kKeyCol0, {7u, 100u}, +1);
  rr.AddDerivation(s8, kRelP, kKeyCol0, {8u, 200u}, +1);

  const RequestEdgeId e7 = rr.AddRequestEdge(
      RequestOwnerId{RootLeaseId{0u}}, CallSiteId{0u}, s7, kRelP);
  const RequestEdgeId e8 = rr.AddRequestEdge(
      RequestOwnerId{RootLeaseId{1u}}, CallSiteId{1u}, s8, kRelP);
  rr.RouteResults(s7);
  rr.RouteResults(s8);

  // Each requester receives exactly its own state's fact — no cross-routing.
  unsigned for_e7 = 0u, for_e8 = 0u;
  for (const auto &rrid : rr.routed_results) {
    if (rrid.request_edge == e7) {
      ++for_e7;
    }
    if (rrid.request_edge == e8) {
      ++for_e8;
    }
  }
  ASSERT_EQ(for_e7, 1u);
  ASSERT_EQ(for_e8, 1u);
  ASSERT_EQ(rr.routed_results.size(), 2u);
}

// ---- (4) P5 gates: the partial-binding DAG (p5-grounding.md §4/§9.3). The
// ORDERED DeclaredAccessPath authority + the ORDER-FREE binding-schema DAG,
// kept distinct. The `-region-out` render carries NO numeric schema/edge id, so
// these ctest gates are the MANDATORY structural anchor (A2): a stub that
// interns nothing / sorts within a path / materializes the power set / conflates
// ordered path with order-free schema FAILS here even though a render-only stub
// could pass a golden.

using hyde::BindingEdge;
using hyde::DeclaredAccessPath;
using hyde::DeclaredAccessPathSet;
using hyde::InternDeclaredPaths;
using hyde::KeyPathId;
using hyde::RelSchemaLocalId;

// Count interned schema nodes for one relation.
static unsigned SchemaNodeCount(const RegionInstanceRelations &rr,
                                RelationId rel) {
  unsigned n = 0u;
  for (const auto &[key, id] : rr.schema_table) {
    if (key.first == rel) {
      ++n;
    }
  }
  return n;
}

// Count ordered edges whose CHILD is `child` (edges converging on a node).
static unsigned EdgesInto(const RegionInstanceRelations &rr,
                          RelSchemaLocalId child) {
  unsigned n = 0u;
  for (const BindingEdge &e : rr.binding_edges) {
    if (e.child == child) {
      ++n;
    }
  }
  return n;
}

// P5 Gate D (DeclaredAccessPath identity): ORDER-SIGNIFICANT within a path,
// ORDER-FREE across paths, deterministic KeyPathId (F21), exact-dup dedup.
TEST(RegionInstanceP5, GateD_DeclaredPathOrderSemantics) {
  // [A,B] and [B,A] are DISTINCT ordered paths (order IS identity).
  const DeclaredAccessPathSet s = InternDeclaredPaths(kRelP, {{0u, 1u}, {1u, 0u}});
  ASSERT_EQ(s.paths.size(), 2u);
  // KeyPathId assigned deterministically by ascending ordered_fields:
  // {0,1} before {1,0}.
  ASSERT_EQ(s.paths[0].id.v, 0u);
  ASSERT_TRUE((s.paths[0].ordered_fields == std::vector<uint32_t>{0u, 1u}));
  ASSERT_EQ(s.paths[1].id.v, 1u);
  ASSERT_TRUE((s.paths[1].ordered_fields == std::vector<uint32_t>{1u, 0u}));

  // Pragma order is IRRELEVANT to identity (order-free across paths -> F21).
  const DeclaredAccessPathSet swapped =
      InternDeclaredPaths(kRelP, {{1u, 0u}, {0u, 1u}});
  ASSERT_TRUE(s == swapped);

  // An exact-duplicate ordered path is deduped to one (parser-rejected upstream;
  // this is the belt).
  const DeclaredAccessPathSet dup = InternDeclaredPaths(kRelP, {{0u, 1u}, {0u, 1u}});
  ASSERT_EQ(dup.paths.size(), 1u);
}

// P5 Gate E (F8 prefix chain present + non-prefix subsets ABSENT + no power set).
TEST(RegionInstanceP5, GateE_F8PrefixChainAndNonPrefixAbsent) {
  RegionInstanceRelations rr;
  rr.MaterializePrefixChain(
      DeclaredAccessPath{KeyPathId{0u}, kRelP, {0u, 1u, 2u}});  // @key(A,B,C).

  // EXACTLY the 4 visited prefixes {}, {A}, {A,B}, {A,B,C} — no power set.
  ASSERT_EQ(SchemaNodeCount(rr, kRelP), 4u);
  ASSERT_TRUE(rr.HasBindingSchema(kRelP, {}));
  ASSERT_TRUE(rr.HasBindingSchema(kRelP, {0u}));
  ASSERT_TRUE(rr.HasBindingSchema(kRelP, {0u, 1u}));
  ASSERT_TRUE(rr.HasBindingSchema(kRelP, {0u, 1u, 2u}));
  // Genuine NON-prefix subsets are ABSENT.
  ASSERT_FALSE(rr.HasBindingSchema(kRelP, {0u, 2u}));
  ASSERT_FALSE(rr.HasBindingSchema(kRelP, {1u}));
  ASSERT_FALSE(rr.HasBindingSchema(kRelP, {2u}));
  ASSERT_FALSE(rr.HasBindingSchema(kRelP, {1u, 2u}));
  // A linear chain: exactly 3 edges.
  ASSERT_EQ(rr.binding_edges.size(), 3u);
}

// P5 Gate F (prefix sharing): @key(A) reuses the {A} node/edge of @key(A,B).
TEST(RegionInstanceP5, GateF_PrefixShare) {
  RegionInstanceRelations rr;
  rr.MaterializePrefixChain(DeclaredAccessPath{KeyPathId{0u}, kRelP, {0u}});
  rr.MaterializePrefixChain(DeclaredAccessPath{KeyPathId{1u}, kRelP, {0u, 1u}});
  // Nodes {}, {A}, {A,B} — the {A} node is SHARED, not duplicated.
  ASSERT_EQ(SchemaNodeCount(rr, kRelP), 3u);
  const RelSchemaLocalId a_from_short = rr.InternBindingSchema(kRelP, {0u});
  const RelSchemaLocalId a_from_long = rr.InternBindingSchema(kRelP, {0u});
  ASSERT_TRUE(a_from_short == a_from_long);
  // Edges {}--A-->{A}, {A}--B-->{A,B}: the {}--A-->{A} edge is deduped -> 2.
  ASSERT_EQ(rr.binding_edges.size(), 2u);
}

// P5 Gate G (convergence): @key(A,B) and @key(B,A) converge on ONE {A,B} schema
// via TWO ordered edges (the headline order-free-node / order-sig-edge split).
TEST(RegionInstanceP5, GateG_ConvergeTwoEdgesOneSchema) {
  RegionInstanceRelations rr;
  rr.MaterializePrefixChain(DeclaredAccessPath{KeyPathId{0u}, kRelP, {0u, 1u}});
  rr.MaterializePrefixChain(DeclaredAccessPath{KeyPathId{1u}, kRelP, {1u, 0u}});
  // Nodes {}, {A}, {B}, {A,B} — {A,B} is ONE converged node.
  ASSERT_EQ(SchemaNodeCount(rr, kRelP), 4u);
  ASSERT_TRUE(rr.HasBindingSchema(kRelP, {0u, 1u}));
  const RelSchemaLocalId ab = rr.InternBindingSchema(kRelP, {0u, 1u});
  // TWO ordered edges converge on {A,B}: {A}--B-->{A,B} and {B}--A-->{A,B}.
  ASSERT_EQ(EdgesInto(rr, ab), 2u);
  ASSERT_EQ(rr.binding_edges.size(), 4u);
}

// P5 Gate H (per-relation identity): {A} of relP and {A} of relQ are DISTINCT
// nodes (no cross-relation conflation; Free-Join sharing is P8).
TEST(RegionInstanceP5, GateH_PerRelationDistinct) {
  RegionInstanceRelations rr;
  const RelSchemaLocalId p_a = rr.InternBindingSchema(kRelP, {0u});
  const RelSchemaLocalId q_a = rr.InternBindingSchema(kRelQ, {0u});
  ASSERT_FALSE(p_a == q_a);
  ASSERT_TRUE(rr.HasBindingSchema(kRelP, {0u}));
  ASSERT_TRUE(rr.HasBindingSchema(kRelQ, {0u}));
}

// P5 Gate I (A5 sibling path): @key(A,B) @key(B) legally interns the {B} sibling
// (NOT a prefix of [A,B]) — F8 non-prefix-absence is a UNION over declared
// paths, never per-path, so this must NOT abort/false-fail.
TEST(RegionInstanceP5, GateI_SiblingPathNotFalseAborted) {
  RegionInstanceRelations rr;
  rr.MaterializePrefixChain(DeclaredAccessPath{KeyPathId{0u}, kRelP, {0u, 1u}});
  rr.MaterializePrefixChain(DeclaredAccessPath{KeyPathId{1u}, kRelP, {1u}});
  // {B} is a declared sibling, legitimately present alongside {}, {A}, {A,B}.
  ASSERT_EQ(SchemaNodeCount(rr, kRelP), 4u);
  ASSERT_TRUE(rr.HasBindingSchema(kRelP, {1u}));
  ASSERT_TRUE(rr.HasBindingSchema(kRelP, {0u, 1u}));
}
