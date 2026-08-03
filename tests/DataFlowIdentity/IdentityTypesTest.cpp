// Copyright 2026, Peter Goodman. All rights reserved.
//
// H-A1 exit gate (the F4 deliverable): a compile-time static_assert battery
// proving the Stage-A identity domains (lib/DataFlow/Identity.h) are pairwise
// DISJOINT -- no cross-domain implicit conversion and no cross-domain operator
// (`+`, `<`, `==`) exists between any two of them. This is a static_assert
// UNIT, not a `V-*` validator (X3 / Errata-5): the checks below fire at compile
// time, so a regression that makes a count silently usable as demand ownership
// (or a field id usable as a count) fails the BUILD of this test, not a runtime
// abort. The runtime `TEST` body is a formality so ctest has something to run.

#include <DrTest.h>

#include <type_traits>

#include "Identity.h"

namespace {

using hyde::DeltaSign;
using hyde::DemandSupportCount;
using hyde::DerivationSupportCount;
using hyde::FieldId;
using hyde::SupportAlgebra;

// ---- concepts detecting a cross-domain operator (true iff it COMPILES) ----

template <typename A, typename B>
concept HasAdd = requires(A a, B b) { a + b; };

template <typename A, typename B>
concept HasLess = requires(A a, B b) { a < b; };

template <typename A, typename B>
concept HasEq = requires(A a, B b) { a == b; };

// The full disjointness assertion for an unordered domain pair {A, B}: neither
// implicitly converts to the other, and no cross-domain `+`/`<`/`==` compiles
// in either direction.
template <typename A, typename B>
constexpr bool Disjoint() {
  static_assert(!std::is_convertible_v<A, B>, "A must not convert to B");
  static_assert(!std::is_convertible_v<B, A>, "B must not convert to A");
  static_assert(!HasAdd<A, B> && !HasAdd<B, A>, "no cross-domain operator+");
  static_assert(!HasLess<A, B> && !HasLess<B, A>, "no cross-domain operator<");
  static_assert(!HasEq<A, B> && !HasEq<B, A>, "no cross-domain operator==");
  return true;
}

// ---- the battery: every unordered pair of the five value/enum domains ----
// (`SemanticMemberKey` is a std::vector alias, not a scalar domain, so it is
//  not part of the arithmetic-disjointness battery.)

// The load-bearing pair (proposal §1.3 / §5): a DERIVATION count must never be
// silently used as a DEMAND-support (ownership) count.
static_assert(Disjoint<DerivationSupportCount, DemandSupportCount>());

// FieldId is an identity, never a magnitude or a sign.
static_assert(Disjoint<FieldId, DerivationSupportCount>());
static_assert(Disjoint<FieldId, DemandSupportCount>());
static_assert(Disjoint<FieldId, DeltaSign>());
static_assert(Disjoint<FieldId, SupportAlgebra>());

// A sign is not a magnitude, an algebra selector is not a count, etc.
static_assert(Disjoint<DeltaSign, DerivationSupportCount>());
static_assert(Disjoint<DeltaSign, DemandSupportCount>());
static_assert(Disjoint<DeltaSign, SupportAlgebra>());
static_assert(Disjoint<SupportAlgebra, DerivationSupportCount>());
static_assert(Disjoint<SupportAlgebra, DemandSupportCount>());

// ---- INTRA-domain equality IS available (sanity: the domains are usable) ----
static_assert(FieldId{1u} == FieldId{1u});
static_assert(FieldId{1u} != FieldId{2u});
static_assert(FieldId{1u} < FieldId{2u});
static_assert(DerivationSupportCount{0, 0} == DerivationSupportCount{0, 0});
static_assert(DemandSupportCount{0u} == DemandSupportCount{0u});

}  // namespace

// A no-op runtime witness: all the real content is the compile-time battery
// above (if it failed, this TU would not have compiled).
TEST(DataFlowIdentity, DomainsAreDisjoint) {
  constexpr bool ok = Disjoint<FieldId, DemandSupportCount>();
  ASSERT_TRUE(ok);
}
