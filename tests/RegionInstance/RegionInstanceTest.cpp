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
