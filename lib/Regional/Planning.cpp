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
                        view.CanReceiveDeletions()};
}

// Tier-2 origin arm: a RelationSchema for an undemanded origin-interior with
// NO RowContract. Member key = AllFields (all positions set, so render
// reproduces the ORC-3 all-parameter passthrough); support = OR over
// origin-carrying live views (ResolveOriginSupport, a RES-2 loud-abort belt).
static RelationSchema BuildRelationSchemaFromOrigin(const ::hyde::Query &query,
                                                    ParsedDeclaration decl) {
  return RelationSchema{RelationId{decl.Id()}, decl,
                        std::vector<bool>(decl.Arity(), true),
                        ResolveOriginSupport(query, decl)};
}

}  // namespace

RegionalCensus DeriveRegionalCensus(const ::hyde::Query &query) {
  RegionalCensus census;

  census.request_ports = 0u;  // Post-cut: demand is deleted, no request ports.

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

  // ---- QUERY ABIs + permanent roots: declaration order; per name, the
  // UniqueRedeclarations walk with the BindingPattern dedup (the
  // BuildQueryEntryPoint idiom, lib/ControlFlow/Build/Build.cpp). Post-cut
  // every redeclaration is a permanent-root observation — a bound `#query`
  // reads the canonical fully-materialized relation via the plain cursor.
  std::unordered_set<uint64_t> seen_queries;
  for (ParsedModule sub_module :
       ParsedModuleIterator(query.ParsedModule())) {
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
        query_abis.push_back(AbiRecord{AbiKind::kQuery, redecl,
                                       RouteKind::kPermanentRoot, 0u});
        R.permanent_roots.push_back(PermanentRootRecord{redecl});
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

  // `rules` / `recursive_components` stay RESERVED EMPTY (P6.1/P6.2 populate).

  // ---- CENSUS: DeriveRegionalCensus is the single authority ...
  out.census = DeriveRegionalCensus(query);

  // ... AND the built TYPED records are independently recounted against it (H2:
  // the real anti-stub belt — a hollow/stub `R` diverges from the query re-walk
  // and aborts). Census is a function of `query`, never of `RegionTemplate`.
  unsigned built_request = 0u, built_input = 0u, built_result = 0u;
  for (const PortRecord &port : R.ports) {
    switch (port.kind) {
      case PortKind::kRequest: ++built_request; break;
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

  return out;
}

}  // namespace hyde
