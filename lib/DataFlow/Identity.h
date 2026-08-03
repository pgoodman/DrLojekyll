// Copyright 2026, Peter Goodman. All rights reserved.

#pragma once

#include <cstdint>
#include <vector>

// Stage A subset of the Regional Dataflow proposal's §4.1 identity domains
// (RegionalDataFlowCore.artifacts/stage-a-diff.md, hunk H-A1). Every type
// here reserves a DOMAIN. No two domains share an operator: there is no
// cross-domain `operator+`/`operator<`/`operator==` and no implicit
// conversion between any two of them, so a value drawn from one domain can
// never be silently used as a value of another (the F4 deliverable — a count
// must never stand in for demand ownership). Only INTRA-domain equality (and,
// where an order is genuinely needed, an explicit comparison) is provided.
//
// The cross-domain no-implicit-convert disjointness is proven by the
// `static_assert` battery in `tests/DataFlowIdentity/IdentityTypesTest.cpp`
// (H-A1 exit gate / X3: a static_assert unit, NOT a `V-*` validator).
//
// SCOPE (candidate 1 / A-nec-1 + candidate 3 amendments): Stage A produces NO
// support VALUE and NO proven-equal FieldExpression classes. The four support
// domains below (`DeltaSign`, `DerivationSupportCount`, `DemandSupportCount`,
// `SupportAlgebra`) earn their place SOLELY via the disjointness battery — no
// `RowContract` field carries them in Stage A. `FieldExpression` and the
// antichain-as-identity machinery DEFER to Stage B; here a `SemanticMemberKey`
// is a FLAT vector of `FieldId`.
//
// Dense runtime indices (the ControlFlow-lowering `*Id` families) are
// deliberately NOT here — those are minted downstream (proposal §4.1).

namespace hyde {

// ---- arrive NOW (Stage A produces AND consumes them) ----

// A semantic field: a view-relative field identity that is STABLE under column
// renumbering. Its own domain — never an array index, never a support count.
struct FieldId {
  uint32_t v;

  // Intra-domain only: defaulted comparisons take a same-type parameter, so
  // they enable no cross-domain compare. An order is provided because
  // `SemanticMemberKey` (below) is rendered/compared in field order.
  constexpr bool operator==(const FieldId &) const noexcept = default;
  constexpr auto operator<=>(const FieldId &) const noexcept = default;
};

// The RowContract member key: the flat, ordered set of fields whose values
// name a member of a relation's set. Stage A is FLAT-KEY (candidate 3): a plain
// vector of `FieldId`, with no proven-equal `FieldExpression` classes (those
// defer to Stage B). Consumed by the H-A3 RowContract (the next slice).
using SemanticMemberKey = std::vector<FieldId>;

// Typed add/remove sign for the support-transfer rules (proposal §4b Step 8).
// Its own domain, disjoint from every count: a sign is not a magnitude.
enum class DeltaSign : uint8_t {
  kAdd,
  kRemove,
};

// ---- arrive NOW as a TYPE, but Rel is the OWNER (proposal §1.3) ----

// The packed C_nr/C_r derivation-support counter domain. Stage A only NAMES
// this domain (via the disjointness battery); it populates no value. Rel keeps
// producing the real per-row split counters. Reserving the domain here makes
// proposal §5's "a count stands in for an owner set" confusion a TYPE ERROR
// rather than a silent arithmetic against a demand-ownership value.
struct DerivationSupportCount {
  int64_t nr;
  int64_t r;

  constexpr bool operator==(const DerivationSupportCount &) const noexcept =
      default;
};

// The support-algebra selector domain: which reduction algebra supplies a
// view's derivation support (leaf input, forwarded, merge-arm fold, join
// product, aggregate, or KV update). Stage A names it in the disjointness
// battery only; the real support algebra reappears at Stage C, where
// `RequestEdgeRelation` is its first genuine consumer.
enum class SupportAlgebra : uint8_t {
  kBaseInput,    // leaf relation / stream read
  kPassthrough,  // forwarded from the sole input
  kMergeFold,    // merge-arm add/remove collision fold
  kJoinProduct,  // product of contributor supports
  kAggregate,    // derivation over distinct input-member-key tuples
  kKvUpdate,     // @-algebra (invertible / recompute) update
};

// ---- declared NOW, INERT until later (reserve the domain early) ----

// The demand-support counter domain ("derived from exact edges", proposal
// §4.1). There is NO producer until Stage C's `RequestEdgeRelation`. It is
// declared now ONLY so arithmetic against `DerivationSupportCount` is a
// pre-emptive type error — the two counts must never be confused.
struct DemandSupportCount {
  uint32_t v;

  constexpr bool operator==(const DemandSupportCount &) const noexcept =
      default;
};

}  // namespace hyde
