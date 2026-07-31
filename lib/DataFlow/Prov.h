// Copyright 2026, Peter Goodman. All rights reserved.
//
// Prov -- keyset (value-containment) provenance over the dataflow graph.
//
// `Prov(col)` is the set of relation/stream projections `(source, index)` whose
// value-set PROVABLY contains `col`'s value-set (an under-approximation:
// bot/empty unless proven). It is the shared substrate for (a) the identity-join
// recognizer (IdentityJoin.cpp) and (b) the cost model's L1 cardinality pass
// (docs/proposals/CostModel.md). See CostModel.artifacts/prov-recognizer-impl.md.
//
// SOUNDNESS: bot-default. A node not proven to preserve containment drops every
// output column to bot. Over-approximating (claiming a containment that does not
// hold) is an over-answer miscompile in the recognizer, so every rule here is a
// conservative UNDER-approximation of the true value-containment.

#pragma once

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace hyde {

class QueryImpl;
class QueryColumnImpl;

// A projection of a base source (a relation or a message/IO stream) at a
// particular column index. `source` is the stable in-graph identity of the
// QueryRelationImpl / QueryStreamImpl the values originate from; `index` is the
// column position within that source. Two SELECTs of the same relation share a
// ProjKey, so a value proven to lie in relation R's column i is comparable
// across the whole graph.
struct ProjKey {
  const void *source{nullptr};
  unsigned index{0u};

  bool operator==(const ProjKey &that) const noexcept {
    return source == that.source && index == that.index;
  }
  bool operator<(const ProjKey &that) const noexcept {
    if (source != that.source) {
      return source < that.source;
    }
    return index < that.index;
  }
};

// The provenance of a single column: the SORTED, de-duplicated set of
// projections whose value-set contains this column's value-set. Empty == bot.
using ProvSet = std::vector<ProjKey>;

// `Prov(col)` for every output column of every view. A column absent from the
// map is bot (know nothing). Values are sorted (ProjKey::operator<) + unique.
using ProvMap = std::unordered_map<QueryColumnImpl *, ProvSet>;

// Compute value-containment provenance for every output column, in a single
// forward pass over the acyclic condensation (depth order). Columns on a
// not-yet-computed back-edge default to bot (safe: the recognizer is monotone-
// fenced, and bot never fires a wrong drop).
ProvMap ComputeColumnProvenance(QueryImpl *query);

// `true` iff `p` contains `key` (a proven `col_values ⊆ π(key)`).
bool ProvContains(const ProvSet &p, const ProjKey &key);

// The EXACT source projection of `col` -- the `(source, index)` whose value-set
// col's value-set provably EQUALS (not merely ⊆): col is a SELECT output, or a
// pass-through TUPLE chain of one, with no row-narrowing on the path. Used for
// the guard side of an identity join, where `π_A(S) = π_index(source)` must hold
// with equality for the subset argument to close. `nullopt` if not exact.
std::optional<ProjKey> ExactProjection(QueryColumnImpl *col);

// The identity-join recognizer: eliminate a JOIN whose output provably equals
// one joined view R (a semijoin `R ⋉ S` where the guard side S is an EXACT
// relation projection subsuming R's pivot values, `π_A(R) ⊆ π_A(S)`, proven by
// Prov). Forwards J's users to R. MONOTONE-FENCED: skips any join that can carry
// deletions or is inductive (the differential regime is a later slice). Returns
// `true` if it changed the graph; the caller re-runs to fixpoint and re-derives
// differential tracking. Consumes ComputeColumnProvenance internally.
bool EliminateIdentityJoins(QueryImpl *query);

// V-PROV-* : always-on structural validators (fprintf+abort, survive NDEBUG),
// asserting the lattice's soundness -- NOT any workload magnitude, so they are
// total by construction over the whole corpus and never false-abort on an
// unmodeled op. Aborts on a violated invariant.
void ValidateProvenance(QueryImpl *query, const ProvMap &prov);

}  // namespace hyde
