// Copyright 2026, Peter Goodman. All rights reserved.
//
// Stage B (RegionalDataFlowCore.artifacts/stage-b-diff.md H2/H9 +
// p2-typed-owner-grounding.md): the degenerate planner + freeze.
// `FrozenRegionalProgram::Build` derives the one-root/one-region skeleton as
// TYPED records — the program-root ABIs, the R0 ports, the permanent roots,
// and the R-STORE / Tier-2 relation schemas — from the FINAL Query graph,
// then freezes it. Render text is NOT computed here (it derives from the typed
// records at dump time, lib/Regional/Format.cpp); the P2 typed-owner cut.
//
// Every derivation is DETERMINISTIC: declaration order via
// `ParsedModuleIterator` sub-module walks and contract order via the
// `query.Inserts()` range walk — never a pointer order, never a
// `UniqueId` (the HP-9 rule).

#include <drlojekyll/Regional/Regional.h>

#include <drlojekyll/Parse/ErrorLog.h>
#include <drlojekyll/Parse/ModuleIterator.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <optional>
#include <set>
#include <unordered_set>
#include <variant>
#include <vector>

#include "Query.h"  // lib/DataFlow private (see CMakeLists.txt): the ONE
                    // friend leak — `query.impl->row_contracts`, read inside
                    // `BuildRelationSchemaFromInsert` only.

namespace hyde {
namespace {

// The RECEIVED messages and the PUBLISHED messages, each in declaration order
// across the sub-module walk, deduplicated by declaration id.
static void CollectMessages(const ::hyde::Query &query,
                            std::vector<ParsedMessage> &received,
                            std::vector<ParsedMessage> &published) {
  std::unordered_set<uint64_t> seen;
  for (ParsedModule sub_module : ParsedModuleIterator(query.ParsedModule())) {
    for (ParsedMessage m : sub_module.Messages()) {
      if (!seen.insert(m.Id()).second) {
        continue;
      }
      if (m.IsReceived()) {
        received.push_back(m);
      }
      if (m.IsPublished()) {
        published.push_back(m);
      }
    }
  }
}

// Rule R-STORE (Stage-B NARROWED form: insert-materialized relations only):
// the distinct non-demand relation-insert declarations, each paired with its
// FIRST insert view, in first-encounter order of the declaration over the
// `query.Inserts()` range walk. That walk order is deterministic (a DefList
// range), and is the contract order the dump renders — a source-position
// sort is deliberately NOT used (the first-encounter order is already a pure
// function of the final graph).
static std::vector<std::pair<ParsedDeclaration, QueryInsert>>
CollectContractInserts(const ::hyde::Query &query) {
  std::vector<std::pair<ParsedDeclaration, QueryInsert>> out;
  std::unordered_set<uint64_t> seen_decls;
  for (QueryInsert ins : query.Inserts()) {
    if (!ins.IsRelation()) {
      continue;  // Stream inserts materialize no stored relation.
    }
    const ParsedDeclaration decl = ins.Relation().Declaration();
    if (!seen_decls.insert(decl.Id()).second) {
      continue;  // Keep the FIRST insert view per distinct declaration.
    }
    out.emplace_back(decl, ins);
  }
  return out;
}

// Tier-2 naming lift (K5): the UNDEMANDED #local/#export interiors nameable
// ONLY via origin decl-sets (an insert-CLEARED, undemanded, merge-materialized
// relation has no R-STORE contract). Walk live views, accumulate the Id-keyed
// union of `v.OriginDecls()`, DEDUP against the insert-named decls
// (CollectContractInserts). The residue — decls reachable ONLY through origin
// sets — are the Tier-2 schemas, emitted ASCENDING `decl.Id()` (a pure
// mode-stable total order; the per-view sets are already Id-sorted, so the
// cross-view merge is a k-way dedup). Consulted IDENTICALLY by
// `DeriveRegionalCensus` and the schema build below, so V-REGION-CENSUS stays
// green by construction.
static std::vector<ParsedDeclaration> CollectOriginInteriorDecls(
    const ::hyde::Query &query) {
  std::unordered_set<uint64_t> named;  // insert-named.
  for (const auto &[decl, ins] : CollectContractInserts(query)) {
    named.insert(decl.Id());
  }
  std::map<uint64_t, ParsedDeclaration> out;  // Id-ordered, deduped.
  query.ForEachView([&](QueryView v) {
    for (ParsedDeclaration decl : v.OriginDecls()) {
      if (named.count(decl.Id())) {
        continue;
      }
      out.emplace(decl.Id(), decl);
    }
  });

  std::vector<ParsedDeclaration> result;
  result.reserve(out.size());
  for (const auto &[id, decl] : out) {
    // K5-D6b NEGATIVE space: a Tier-2 decl is, by construction, exactly the
    // residue outside all prior naming tiers — never spliced (@inline), never a
    // query (R-STORE's job), never insert-named, never a Tier-1 demand-interior.
    // A dedup/seed regression (a decl double-counted across tiers) is then a
    // loud abort, not a silent census inflation.
    assert(!decl.IsInline());
    assert(!decl.IsQuery());
    assert(!named.count(decl.Id()));
    result.push_back(decl);
  }
  return result;
}

// Tier-2 origin-interior support: an undemanded origin-interior has NO guard
// JOIN, so support resolves off the LIVE post-Optimize views that
// carry `decl` in their origin set — support = OR over those carriers of
// `v.CanReceiveDeletions()`. SOUND because CDaGI confines a decl to carriers
// that AGREE on differentialness (the differentialness-migration invariant,
// K5-D3: CDaGI OR-propagates can_receive_deletions loser->survivor in lockstep
// with the origin union, and CSE co-location is HashInit-gated), so the OR is
// exact; the DEBUG support-agreement assert (K5-D6b) turns a future
// cross-differentialness fold into a tripwire rather than a wrong/mode-split
// support byte. A counted Tier-2 decl with ZERO live carrier ABORTS the freeze
// (the RES-2 loud-failure construction):
// existence is decl-counted from live origin sets, so >=1 carrier always exists
// in correct code.
static bool ResolveOriginSupport(const ::hyde::Query &query,
                                 ParsedDeclaration decl) {
  bool resolved = false;
  bool support = false;
#ifndef NDEBUG
  bool first_crd = false;
#endif
  query.ForEachView([&](QueryView v) {
    for (ParsedDeclaration d : v.OriginDecls()) {
      if (d.Id() != decl.Id()) {
        continue;
      }
      const bool crd = v.CanReceiveDeletions();
#ifndef NDEBUG
      // K5-D6b support-agreement (positive): every live carrier of `decl`
      // agrees on differentialness (the OR is not a mix).
      assert(!resolved || first_crd == crd);
      first_crd = crd;
#endif
      resolved = true;
      support = support || crd;
      break;  // A view carries `decl` at most once (sorted-unique).
    }
  });

  if (!resolved) {
    fprintf(stderr,
            "ORIGIN-SUPPORT-RESOLVE: no live view carries origin-interior "
            "relation (rel=%.*s)\n",
            static_cast<int>(decl.NameAsString().size()),
            decl.NameAsString().data());
    abort();
  }
  return support;
}

// R-STORE arm: a RelationSchema for an insert-materialized relation. The
// POSITIONAL member-key mask comes from the Stage-A `row_contracts` side-table
// (the friend leak; keyed by QueryViewImpl*, read once in `Build` and threaded
// in here): position `i` (over [0, decl.Arity())) is set iff the i-th visible
// field's value id is in the member key. This is the exact bool the pre-P2
// inline render loop computed.
static RelationSchema BuildRelationSchemaFromInsert(
    const RowContractMap &row_contracts, ParsedDeclaration decl,
    QueryView view) {
  const auto contract_it = row_contracts.find(view.impl);
  if (contract_it == row_contracts.end()) {
    fprintf(stderr,
            "FROZEN-REGIONAL: relation insert view has no row contract "
            "(rel=%.*s)\n",
            static_cast<int>(decl.NameAsString().size()),
            decl.NameAsString().data());
    abort();
  }
  const RowContract &rc = contract_it->second;

  std::vector<bool> positions(decl.Arity(), false);
  for (auto i = 0u; i < decl.Arity(); ++i) {
    // A desugared zero-arity condition predicate materializes as a UNIT
    // relation whose insert carries the 1 bool `(true)` token column with NO
    // declared parameter behind it — positions beyond the visible fields have
    // no member-key value and stay false (the contract itself stays; a unit
    // relation renders `member-key=()`).
    if (i >= rc.visible_fields.size()) {
      break;
    }
    const FieldId field = rc.visible_fields[i];
    positions[i] = std::find(rc.member_key.begin(), rc.member_key.end(),
                             field) != rc.member_key.end();
  }
  return RelationSchema{RelationId{decl.Id()}, decl, std::move(positions),
                        view.CanReceiveDeletions(),
                        InternDeclaredPaths(RelationId{decl.Id()},
                                            decl.InstanceKeys())};  // P5.
}

// Tier-2 origin arm: a RelationSchema for an undemanded origin-interior with
// NO RowContract. Member key = AllFields (all positions set, so render
// reproduces the ORC-3 all-parameter passthrough); support = OR over
// origin-carrying live views (ResolveOriginSupport, a RES-2 loud-abort belt).
static RelationSchema BuildRelationSchemaFromOrigin(const ::hyde::Query &query,
                                                    ParsedDeclaration decl) {
  return RelationSchema{RelationId{decl.Id()}, decl,
                        std::vector<bool>(decl.Arity(), true),
                        ResolveOriginSupport(query, decl),
                        InternDeclaredPaths(RelationId{decl.Id()},
                                            decl.InstanceKeys())};  // P5 (A4).
}

// P3: does a `#query` redeclaration carry any bound parameter? (The bound test
// idiom is BuildQueryEntryPointImpl's `param.Binding()==kBound` loop,
// lib/ControlFlow/Build/Build.cpp:422-426.) A bound query becomes a RootLease
// request-port owner; an all-free query a PermanentRoot.
static bool HasBoundParam(ParsedDeclaration redecl) {
  for (ParsedParameter p : redecl.Parameters()) {
    if (p.Binding() == ParameterBinding::kBound) {
      return true;
    }
  }
  return false;
}

// P4 (§3.1/§3.2): the AccessPlan the freeze selects for a bound `#query` redecl.
// `decl` is the query declaration (its Id is the RelationId); `redecl` carries the
// binding pattern. A bound+free query specializes to a full scan + bound-col filter
// (answer-correct even over a recursive relation — the read is an acyclic scan of the
// settled table, and the recursion's own indexes are untouched — §8-S2); an all-bound
// query keeps `.Find`.
static AccessPlan ComputeQueryAccessPlan(ParsedDeclaration decl,
                                         ParsedDeclaration redecl) {
  bool has_free = false;
  std::vector<uint32_t> available_bindings;
  for (ParsedParameter p : redecl.Parameters()) {
    if (p.Binding() == ParameterBinding::kBound) {
      available_bindings.push_back(p.Index());
    } else {
      has_free = true;
    }
  }

  AccessRequirement req{RelationId{decl.Id()}, has_free,
                        std::move(available_bindings),
                        AccessCompleteness::kCompleteRelation};
  return SelectAccessPlan(req);
}

// P3: the number of request ports == the number of dedup'd bound-query
// redeclarations, over the SAME Id-then-BindingPattern dedup the freeze uses.
// The single census authority (DeriveRegionalCensus) and the built
// `R.request_ports` must agree — the check_count belt aborts otherwise.
static unsigned CountBoundQueryRedecls(const ::hyde::Query &query) {
  unsigned count = 0u;
  std::unordered_set<uint64_t> seen_queries;
  for (ParsedModule sub_module : ParsedModuleIterator(query.ParsedModule())) {
    for (ParsedQuery parsed_query : sub_module.Queries()) {
      if (!seen_queries.insert(parsed_query.Id()).second) {
        continue;
      }
      const ParsedDeclaration decl(parsed_query);
      std::unordered_set<std::string> seen_variants;
      for (ParsedDeclaration redecl : decl.UniqueRedeclarations()) {
        std::string binding(redecl.BindingPattern());
        if (!seen_variants.insert(binding).second) {
          continue;
        }
        if (HasBoundParam(redecl)) {
          ++count;
        }
      }
    }
  }
  return count;
}

// P6.1: the query-independent recursive components — a projection of the
// DataFlow view-graph SCC condensation (`QueryView::Stratum()`, a Tarjan
// condensation closing message publish->receive seams, Stratify.cpp) onto the
// frozen relations. A recursive component IS one MULTI-VIEW stratum (an SCC
// cycle; Stratify: "a recursive fixpoint is exactly a multi-view stratum"); its
// members are the frozen relations whose rows are ORIGIN-materialized within it.
//
// Sound by (session-21 panel, empirically validated on tc / ping-pong):
//  (a) a multi-view stratum is a cycle by construction (the "no view is its own
//      user" invariant forces >=2 views), so SELF-recursion is a size-1 members
//      over such a stratum — no self-edge check is needed;
//  (b) `OriginDecls` is seeded at the insert-proxy and migrates ONLY via CSE
//      folds — strictly NARROWER than "rows flow through" — so a base relation
//      (edge) whose rows merely pass through a recursive JOIN never lands on the
//      recursive UNION's origin (edge is correctly excluded from tc's component);
//  (c) all decls sharing one multi-view stratum are ONE SCC ⟹ one component.
// It reads NO `rules` (P6.1 ⊥ P6.2). It is a per-compile OBSERVER of the actual
// (per-mode) graph — MODE-FAITHFUL, not mode-invariant: a canonicalization-
// stripped vacuous `p:-p` self-loop is a recursive stratum only in the
// un-optimized modes whose graph still holds it. Query-INDEPENDENT: `Stratum()`
// is assigned pre-query and a `#query` introduces no dataflow cycle.
static std::vector<RecursiveComponent> ComputeRecursiveComponents(
    const ::hyde::Query &query,
    const std::vector<RelationSchema> &relation_schemas) {
  std::unordered_set<uint64_t> rel_ids;
  for (const RelationSchema &schema : relation_schemas) {
    rel_ids.insert(schema.id.v);
  }

  // (1) The recursive strata are the MULTI-VIEW ones.
  std::map<unsigned, unsigned> views_per_stratum;
  query.ForEachView([&](QueryView v) {
    if (std::optional<unsigned> s = v.Stratum()) {
      ++views_per_stratum[*s];
    }
  });

  // (2) Project each view's ORIGIN decls onto its stratum, keeping only frozen
  // relations sitting in a recursive (multi-view) stratum.
  std::map<unsigned, std::set<uint64_t>> members;  // stratum(ascending) -> rels.
  query.ForEachView([&](QueryView v) {
    const std::optional<unsigned> s = v.Stratum();
    if (!s) {
      return;
    }
    const auto it = views_per_stratum.find(*s);
    if (it == views_per_stratum.end() || it->second <= 1u) {
      return;  // A singleton stratum is not an SCC cycle.
    }
    for (ParsedDeclaration decl : v.OriginDecls()) {
      if (rel_ids.count(decl.Id())) {
        members[*s].insert(decl.Id());
      }
    }
  });

  // (3) One component per non-empty recursive-stratum bucket, in ascending
  // stratum order; members ascending by RelationId (== decl.Id(), the committed
  // golden-order authority, matching CollectOriginInteriorDecls).
  std::vector<RecursiveComponent> out;
  for (const auto &[stratum, mem] : members) {
    if (mem.empty()) {
      continue;
    }
    RecursiveComponent component;
    for (uint64_t id : mem) {  // std::set iterates ascending == RelationId order.
      component.members.push_back(RelationId{id});
    }
    // Tigerstyle internal-consistency belt (NOT anti-stub — the region golden on
    // the recursive carriers is the discriminating gate; this catches a corrupt
    // build: an empty or unsorted component).
    assert(!component.members.empty());
    assert(std::is_sorted(
        component.members.begin(), component.members.end(),
        [](RelationId a, RelationId b) { return a.v < b.v; }));
    out.push_back(std::move(component));
  }
  return out;
}

// ============================ P6.2: routing rules =============================
// The SECOND (and last) compile-time slice of P6. Populate `RegionTemplate::rules`
// (per-clause field-routing projections, CLAUSE-SOURCE) + mint the region-global
// `SymbolicFieldId` frame, then promote fields that provably carry the same value
// in every derivation into shared classes (`inherited_symbolic_fields`). All
// compile-time model + `-region-out` render — codegen byte-unchanged (nobody in
// codegen reads these; `Program::Build` consumes `DataFlowGraph()`, never
// `Region()`). Clause-source, NOT DataFlow-source: the post-Optimize graph
// CSE-merges co-recursive relations onto one model table and loses per-relation
// field identity, whereas the parsed clauses retain it (p6.2-grounding.md §1.1 —
// the same identity lesson that flipped P6.1's insert-arm to the origin
// projection). P6.2 mints NO RuleActivationEdge and touches none of the dormant
// runtime half (activation_edges / AddDerivation / RouteResults).

// P6.2: seed ONE SymbolicFieldId per (schema.id, ordinal) for every frozen
// relation, in relation_schemas E-order x ascending ordinal. Fixes the dense,
// deterministic id space (HP-9) BEFORE any rule reads it.
static void AssignSymbolicFields(const RegionTemplate &R,
                                 RegionInstanceRelations &inst) {
  for (const RelationSchema &schema : R.relation_schemas) {
    for (uint32_t i = 0u; i < schema.decl.Arity(); ++i) {
      (void) inst.InternSymbolicField(schema.id, i);
    }
  }
}

// Append `(b -> h)` to `proj` unless the EXACT pair is already present (a
// self-join can match one (i, j) once per body pred; two body preds can bind the
// same head field — both legitimately distinct pairs; we drop only exact repeats
// from ONE pred so a doubly-used variable is counted once).
static void AddRoutePair(RuleRoutingProjection &proj, SymbolicFieldId b,
                         SymbolicFieldId h) {
  std::pair<SymbolicFieldId, SymbolicFieldId> pair{b, h};
  if (std::find(proj.body_to_head.begin(), proj.body_to_head.end(), pair) ==
      proj.body_to_head.end()) {
    proj.body_to_head.push_back(pair);
  }
}

// P6.2: the CLAUSE-SOURCE routing walk. For every frozen relation, for every
// clause defining it, for every POSITIVE body predicate whose declaration is ALSO
// a frozen relation, emit (SymFld(body, i) -> SymFld(head, j)) for each body arg
// pos i and head param pos j sharing a clause variable (ParsedVariable::Id()
// equality — clause-scoped, same name ⇒ same id within a clause). A clause with no
// such pair is STILL emitted (empty body_to_head) so `PromoteSharedSymbolicField`
// sees its base-case producer. RuleId is a dense per-clause ordinal
// (relation_schemas E-order, then decl.Clauses() parse order) — HP-9, never a
// UniqueId/pointer. Negated/aggregate body atoms do NOT participate (a sound
// under-approximation, F18): only positive predicates whose Of(pred).Id() is a
// frozen relation — messages/functors/foreign drop out here; @product mates share
// no variable and emit no pair naturally.
static std::vector<RuleRoutingProjection> BuildRuleRoutingProjections(
    const RegionTemplate &R, RegionInstanceRelations &inst) {
  std::unordered_set<uint64_t> rel_ids;
  for (const RelationSchema &schema : R.relation_schemas) {
    rel_ids.insert(schema.id.v);
  }

  std::vector<RuleRoutingProjection> out;
  uint32_t next_rule = 0u;
  for (const RelationSchema &schema : R.relation_schemas) {
    const ParsedDeclaration head_decl = schema.decl;
    const RelationId head = schema.id;

    // `Clauses()` spans the whole redeclaration group (ping's 2 clauses, tc's 2,
    // etc. all appear under one decl — verified). The C5 belt: resolve through the
    // declaration context so a `@key` on a non-first redeclaration is not missed
    // (the F31 / F-K6-SHADOW precedent); `Clauses()` already does this.
    for (ParsedClause clause : head_decl.Clauses()) {
      RuleRoutingProjection proj;
      proj.id = RuleId{next_rule++};
      proj.head = head;

      for (unsigned g = 0u; g < clause.NumGroups(); ++g) {
        for (ParsedPredicate pred : clause.PositivePredicates(g)) {
          const ParsedDeclaration body_decl = ParsedDeclaration::Of(pred);
          if (!rel_ids.count(body_decl.Id())) {
            continue;  // not frozen (message/functor/foreign) -> no route.
          }
          const RelationId body_rel{body_decl.Id()};
          for (unsigned i = 0u; i < pred.Arity(); ++i) {
            const uint64_t body_var = pred.NthArgument(i).Id();
            for (unsigned j = 0u; j < clause.Arity(); ++j) {
              if (clause.NthParameter(j).Id() == body_var) {
                AddRoutePair(proj, inst.SymbolicFieldOf(body_rel, i),
                             inst.SymbolicFieldOf(head, j));
              }
            }
          }
        }
      }
      out.push_back(std::move(proj));  // stored even if empty (base-case producer).
    }
  }
  return out;
}

// P6.2: promote symbolic fields that provably carry the same value in EVERY
// derivation into one union-find class. The primitive is directional, per (head
// relation H, head ordinal j): union SymFld(H, j) with a source class `s` iff
// EVERY producer clause of H sources H.j from exactly ONE frozen body field and
// all those per-producer sources are the SAME class (under the current `find`).
//   - a producer with NO source for H.j (empty) is a base case (message/constant/
//     negation-fed) ⇒ H.j is NOT uniformly frozen-sourced ⇒ NO promotion;
//   - a producer with >= 2 sources for H.j (a JOIN) ⇒ ambiguous ⇒ NO promotion
//     (never union join-mates — the F16 co-occurrence trap).
// A union performed on one head field can make a different head field's sources
// collapse to one class, so iterate to a FIXPOINT (F28). union-by-min gives a
// deterministic class-minimum representative (HP-9).
static void PromoteSharedSymbolicField(RegionTemplate &R,
                                       RegionInstanceRelations &inst) {
  const uint32_t n = inst.next_symbolic_field;
  std::vector<uint32_t> parent(n);
  for (uint32_t i = 0u; i < n; ++i) {
    parent[i] = i;
  }
  auto find = [&parent](uint32_t x) {  // path-halving; no self-recursion.
    while (parent[x] != x) {
      parent[x] = parent[parent[x]];
      x = parent[x];
    }
    return x;
  };
  auto merge = [&](uint32_t a, uint32_t b) {  // union-by-min.
    a = find(a);
    b = find(b);
    if (a == b) {
      return false;
    }
    if (a < b) {
      parent[b] = a;
    } else {
      parent[a] = b;
    }
    return true;
  };

  // Group producer clauses by head relation once.
  std::map<uint64_t, std::vector<const RuleRoutingProjection *>> by_head;
  for (const RuleRoutingProjection &r : R.rules) {
    by_head[r.head.v].push_back(&r);
  }

  bool changed = true;
  while (changed) {
    changed = false;
    for (const RelationSchema &schema : R.relation_schemas) {
      const auto it = by_head.find(schema.id.v);
      if (it == by_head.end() || it->second.empty()) {
        continue;
      }
      for (uint32_t j = 0u; j < schema.decl.Arity(); ++j) {
        const uint32_t head_field = inst.SymbolicFieldOf(schema.id, j).v;
        bool all_single = true;
        std::optional<uint32_t> agreed;  // the one source class, if any.
        for (const RuleRoutingProjection *r : it->second) {
          std::set<uint32_t> src;
          for (const std::pair<SymbolicFieldId, SymbolicFieldId> &route :
               r->body_to_head) {
            if (route.second.v == head_field) {
              src.insert(find(route.first.v));
            }
          }
          if (src.size() != 1u) {  // base case (0) or join (>=2).
            all_single = false;
            break;
          }
          const uint32_t s = *src.begin();
          if (!agreed) {
            agreed = s;
          } else if (find(*agreed) != s) {
            all_single = false;
            break;
          }
        }
        if (all_single && agreed && find(head_field) != find(*agreed)) {
          if (merge(head_field, *agreed)) {
            changed = true;
          }
        }
      }
    }
  }

  R.inherited_symbolic_fields.assign(n, SymbolicFieldId{0u});
  for (uint32_t i = 0u; i < n; ++i) {
    const uint32_t rep = find(i);
    assert(rep <= i);  // union-by-min ⇒ representative is the class minimum.
    R.inherited_symbolic_fields[i] = SymbolicFieldId{rep};
  }
}

}  // namespace

RegionalCensus DeriveRegionalCensus(const ::hyde::Query &query) {
  RegionalCensus census;

  // P3: one request port per dedup'd bound-query redeclaration. (This must
  // stay in lockstep with BuildRequestPorts's built `R.request_ports`.)
  census.request_ports = CountBoundQueryRedecls(query);

  std::vector<ParsedMessage> received, published;
  CollectMessages(query, received, published);
  census.input_ports = static_cast<unsigned>(received.size());
  census.result_ports = static_cast<unsigned>(published.size());

  census.row_contracts =
      static_cast<unsigned>(CollectContractInserts(query).size() +
                            CollectOriginInteriorDecls(query).size());

  // regions=1, child_calls=0, program_roots=1 are the Stage-B constants
  // (the struct defaults): one ProgramRoot, one observation-root region.
  return census;
}

FrozenRegionalProgram::FrozenRegionalProgram(const ::hyde::Query &query_)
    : dataflow_graph(query_) {}

const ::hyde::Query &FrozenRegionalProgram::DataFlowGraph(void) const {
  return dataflow_graph;
}

const RegionalCensus &FrozenRegionalProgram::Census(void) const {
  return census;
}

const RegionTemplate &FrozenRegionalProgram::Region(void) const {
  return region;
}

const RegionInstanceRelations &FrozenRegionalProgram::Instances(void) const {
  return instances;
}

std::optional<AccessPlan> FrozenRegionalProgram::PlanFor(
    ParsedDeclaration redecl) const {
  // Match by decl Id (shared across a query name's redecls) + binding pattern
  // (distinguishes the adornments) — the same key BuildRequestPorts dedups on.
  const std::string binding(redecl.BindingPattern());
  for (const RequestPortRecord &rp : region.request_ports) {
    if (rp.query_decl.Id() == redecl.Id() &&
        std::string(rp.query_decl.BindingPattern()) == binding) {
      return rp.plan;
    }
  }
  return std::nullopt;
}

std::optional<FrozenRegionalProgram> FrozenRegionalProgram::Build(
    const ::hyde::Query &query, const ErrorLog &log) {
  (void) log;  // No Stage-B reject exists; the parameter reserves the seam.

  FrozenRegionalProgram out(query);
  RegionTemplate &R = out.region;
  R.id = RegionId{0};

  // ---- INPUT / RESULT ports + input/output ABIs (declaration order). Post-
  // cut there are no request-ports (demand is deleted), so port ids start at 0.
  std::vector<ParsedMessage> received, published;
  CollectMessages(query, received, published);

  unsigned next_port = 0u;

  std::vector<AbiRecord> input_abis, query_abis, output_abis;

  for (ParsedMessage m : received) {
    const unsigned port_index = next_port++;
    R.ports.push_back(PortRecord{PortKind::kInput, port_index, m});
    input_abis.push_back(
        AbiRecord{AbiKind::kInput, m, RouteKind::kToPortP, port_index});
  }

  for (ParsedMessage m : published) {
    const unsigned port_index = next_port++;
    R.ports.push_back(PortRecord{PortKind::kResult, port_index, m});
    output_abis.push_back(
        AbiRecord{AbiKind::kOutput, m, RouteKind::kToPortP, port_index});
  }

  if (output_abis.empty()) {
    // The synthetic `output-abi <none>` line: no decl (monostate), no route.
    output_abis.push_back(AbiRecord{AbiKind::kOutput, std::monostate{},
                                    RouteKind::kNone, 0u});
  }

  // ---- QUERY ABIs + request ports + permanent roots (BuildRequestPorts, P3):
  // declaration order; per name, the UniqueRedeclarations walk with the
  // BindingPattern dedup (the BuildQueryEntryPoint idiom, lib/ControlFlow/Build/
  // Build.cpp:504-517). A bound `#query` redecl becomes a RootLease-owned
  // request port (`kRequestPort`, numbered AFTER input/result ports); an
  // all-free redecl a PermanentRoot (`kPermanentRoot`). BOTH also mint a
  // RequestEdge into the P3 model, so a request owner is exact (B2). The region
  // itself is rooted by the always-present ProgramRoot (RootedReachability's
  // implicit empty-state seed, §8-F1) — query request edges only ADD
  // observation roots, they are not the sole liveness source. The
  // derivation/routing half of the model stays empty (no rule sweep at P3, M3).
  const RegionInstanceId ri{0u};
  unsigned next_lease = 0u, next_perm = 0u, next_call_site = 0u;
  std::unordered_set<uint64_t> seen_queries;
  for (ParsedModule sub_module :
       ParsedModuleIterator(query.ParsedModule())) {
    for (ParsedQuery parsed_query : sub_module.Queries()) {
      if (!seen_queries.insert(parsed_query.Id()).second) {
        continue;
      }
      const ParsedDeclaration decl(parsed_query);
      const RelationId requested{decl.Id()};  // L5: the query decl IS its relation.
      std::unordered_set<std::string> seen_variants;
      for (ParsedDeclaration redecl : decl.UniqueRedeclarations()) {
        std::string binding(redecl.BindingPattern());
        if (!seen_variants.insert(binding).second) {
          continue;
        }
        const CallSiteId call_site{next_call_site++};
        if (HasBoundParam(redecl)) {
          const RootLeaseId lease{next_lease++};
          const unsigned port_index = next_port++;
          const AccessPlan plan = ComputeQueryAccessPlan(decl, redecl);
          R.request_ports.push_back(
              RequestPortRecord{port_index, redecl, lease, call_site, plan});
          query_abis.push_back(AbiRecord{AbiKind::kQuery, redecl,
                                         RouteKind::kRequestPort, port_index});
          out.instances.AddRequestEdge(RequestOwnerId{lease}, call_site,
                                       EmptyBindingState(ri), requested);
        } else {
          const PermanentRootId perm{next_perm++};
          R.permanent_roots.push_back(PermanentRootRecord{redecl});
          query_abis.push_back(AbiRecord{AbiKind::kQuery, redecl,
                                         RouteKind::kPermanentRoot, 0u});
          out.instances.AddRequestEdge(RequestOwnerId{perm}, call_site,
                                       EmptyBindingState(ri), requested);
        }
      }
    }
  }

  // ABI block order: inputs, queries, outputs (the AbiKind order).
  R.abis = std::move(input_abis);
  R.abis.insert(R.abis.end(), query_abis.begin(), query_abis.end());
  R.abis.insert(R.abis.end(), output_abis.begin(), output_abis.end());

  // ---- RELATION SCHEMAS (Rule R-STORE, Stage-B narrowed form). Two arms on
  // one dense positional edge counter: the insert arm (rc-copy) then the
  // Tier-2 origin arm (AllFields + OR-over-carriers support), appended in that
  // order — the byte-order the dump renders as E0..En.
  // The ONE friend leak (`query.impl` is private; FrozenRegionalProgram is a
  // declared friend of Query — its members, incl. this static Build, have the
  // access). Read once, threaded into the insert arm (M1: a public accessor
  // would WIDEN the leak — RowContractMap/RowContract/QueryViewImpl are private
  // lib types).
  const RowContractMap &row_contracts = query.impl->row_contracts;
  for (const auto &[decl, ins] : CollectContractInserts(query)) {
    R.relation_schemas.push_back(
        BuildRelationSchemaFromInsert(row_contracts, decl, QueryView(ins)));
  }
  for (ParsedDeclaration decl : CollectOriginInteriorDecls(query)) {
    R.relation_schemas.push_back(BuildRelationSchemaFromOrigin(query, decl));
  }

  // ---- P5: the binding-schema DAG (order-free nodes, order-sig edges). LAZY
  // prefix chains from the declared `@key` paths of every relation schema —
  // NEVER the power set. This is compile-time structure the AccessPlan does not
  // yet consume (P5 moves no codegen); it is made LOAD-BEARING by V-PREFIX-CHAIN.
  for (const RelationSchema &schema : R.relation_schemas) {
    for (const DeclaredAccessPath &p : schema.declared_access_paths.paths) {
      out.instances.MaterializePrefixChain(p);
    }
  }

  // ---- P6.1: recursive components — the query-independent SCC projection onto
  // the frozen relations (`ComputeRecursiveComponents` is the SOLE populator;
  // compile-time model + render only, codegen byte-unchanged). `rules` stays
  // RESERVED EMPTY (P6.2 is its sole populator).
  R.recursive_components =
      ComputeRecursiveComponents(query, R.relation_schemas);

  // ---- P6.2: routing rules + the promoted symbolic-field frame (COMPILE-TIME
  // model + `-region-out` render only; codegen byte-unchanged — nobody in codegen
  // reads these). Seed the region-global field interner, build the clause-source
  // routing rules, then promote shared fields to a fixpoint. Order matters:
  // `AssignSymbolicFields` must precede `BuildRuleRoutingProjections`
  // (`SymbolicFieldOf` aborts on an un-seeded field), which precedes
  // `PromoteSharedSymbolicField` (reads `R.rules`). No census re-derivation
  // (routing rules carry no census count — the P6.1 precedent).
  AssignSymbolicFields(R, out.instances);
  R.rules = BuildRuleRoutingProjections(R, out.instances);
  PromoteSharedSymbolicField(R, out.instances);

  // ---- CENSUS: DeriveRegionalCensus is the single authority ...
  out.census = DeriveRegionalCensus(query);

  // ... AND the built TYPED records are independently recounted against it (H2:
  // the real anti-stub belt — a hollow/stub `R` diverges from the query re-walk
  // and aborts). Census is a function of `query`, never of `RegionTemplate`.
  unsigned built_request =
      static_cast<unsigned>(R.request_ports.size());  // P3: own vector.
  unsigned built_input = 0u, built_result = 0u;
  for (const PortRecord &port : R.ports) {
    switch (port.kind) {
      case PortKind::kRequest: ++built_request; break;  // (none in R.ports at P3)
      case PortKind::kInput: ++built_input; break;
      case PortKind::kResult: ++built_result; break;
    }
  }
  const auto check_count = [](const char *what, unsigned built,
                              unsigned derived) {
    if (built != derived) {
      fprintf(stderr,
              "FROZEN-REGIONAL: census mismatch (%s): built %u != derived "
              "%u\n",
              what, built, derived);
      abort();
    }
  };
  check_count("request-ports", built_request, out.census.request_ports);
  check_count("input-ports", built_input, out.census.input_ports);
  check_count("result-ports", built_result, out.census.result_ports);
  check_count("row-contracts",
              static_cast<unsigned>(R.relation_schemas.size()),
              out.census.row_contracts);

  // ---- FREEZE VALIDATORS (always-on, fprintf+abort, surviving NDEBUG — the
  // V-SCC-SEAM house style). Both are the H9 scaffold Stage C extends.

  // V-FROZEN-NO-OPEN-PORT: the Stage-B closed-ABI check — every built port has
  // a set kind. Stage C extends this to the request-edge lifecycle ports.
  for (const PortRecord &port : R.ports) {
    if (port.kind != PortKind::kRequest && port.kind != PortKind::kInput &&
        port.kind != PortKind::kResult) {
      fprintf(stderr, "V-FROZEN-NO-OPEN-PORT: port P%u has no kind\n",
              port.port_index);
      abort();
    }
  }

  // V-OWNERSHIP-ACYCLIC: the ownership forest is exactly one region with zero
  // child calls at Stage B. Stage C extends this to a real forest acyclicity
  // walk once child calls exist.
  if (out.census.regions != 1u || out.census.child_calls != 0u) {
    fprintf(stderr,
            "V-OWNERSHIP-ACYCLIC: Stage-B ownership forest is not exactly "
            "one region / zero child calls (regions=%u child-calls=%u)\n",
            out.census.regions, out.census.child_calls);
    abort();
  }

  // V-PREFIX-CHAIN (P5): the binding-schema DAG is COMPLETE and non-dead. Tied
  // to the INDEPENDENT parse authority `HasInstanceKey()` (NOT a self-derivation
  // from the same paths the producer read — A2), so a stub that skips
  // InternDeclaredPaths / MaterializePrefixChain aborts instead of passing
  // vacuously. For every relation the parser marked keyed: (i) its declared
  // paths are non-empty, AND (ii) the relation's interned schema nodes are
  // EXACTLY the UNION over its declared paths of all their prefixes (A5 —
  // union, never per-path, so `@key(A,B) @key(B)` legitimately interns `{B}`;
  // this proves terminal-present + every-intermediate-interned + non-prefix-
  // absent in one equality).
  for (const RelationSchema &schema : R.relation_schemas) {
    if (!schema.decl.HasInstanceKey()) {
      continue;
    }
    if (schema.declared_access_paths.paths.empty()) {
      fprintf(stderr,
              "V-PREFIX-CHAIN: keyed relation '%.*s' has no interned declared "
              "access paths (a stub skipped InternDeclaredPaths)\n",
              static_cast<int>(schema.decl.NameAsString().size()),
              schema.decl.NameAsString().data());
      abort();
    }
    std::set<std::vector<uint32_t>> expected;
    for (const DeclaredAccessPath &p : schema.declared_access_paths.paths) {
      std::vector<uint32_t> running;
      expected.insert(running);  // the empty prefix (per-relation root).
      for (uint32_t f : p.ordered_fields) {
        running.push_back(f);
        std::sort(running.begin(), running.end());
        expected.insert(running);
      }
    }
    std::set<std::vector<uint32_t>> actual;
    for (const auto &[key, id] : out.instances.schema_table) {
      if (key.first == schema.id) {
        actual.insert(key.second);
      }
    }
    if (actual != expected) {
      fprintf(stderr,
              "V-PREFIX-CHAIN: binding-schema DAG for keyed relation '%.*s' is "
              "not exactly its declared-prefix union (interned %zu, expected "
              "%zu) — a missed prefix, a stray non-prefix subset, or a "
              "power-set materialization\n",
              static_cast<int>(schema.decl.NameAsString().size()),
              schema.decl.NameAsString().data(), actual.size(),
              expected.size());
      abort();
    }
  }

  return out;
}

}  // namespace hyde
