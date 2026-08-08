// Copyright 2026, Peter Goodman. All rights reserved.
//
// Stage B (RegionalDataFlowCore.artifacts/stage-b-diff.md H2/H9): the
// degenerate planner + freeze. `FrozenRegionalProgram::Build` derives the
// one-root/one-region skeleton — program-root ABIs, the R0 ports, the
// permanent roots, and the R-STORE row contracts — from the FINAL Query
// graph, then freezes it.
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
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "Query.h"  // lib/DataFlow private (see CMakeLists.txt): the ONE
                    // friend leak — `query.impl->row_contracts`, read inside
                    // `FrozenRegionalProgram::Build` only.

namespace hyde {
namespace {

// Type spelling, mirroring the `.df` emitter's TypeLoc rendering
// (lib/DataFlow/Format.cpp `typed_tok` -> Parse/Format.cpp TypeLoc
// operator<<): the source spelling for built-ins ("u64"/"i32"), the foreign
// type's declared name otherwise — via the module-resolving
// `TypeLoc::Spelling(module)`, which needs no DisplayManager at freeze time.
static std::string TypeText(const ParsedModule &module, TypeLoc type) {
  return std::string(type.Spelling(module));
}

// `<name>/<arity>(<pname>:<type>, ...)` for a message declaration.
static std::string MessageDeclText(const ParsedModule &module,
                                   ParsedMessage m) {
  std::string out(m.NameAsString());
  out += "/";
  out += std::to_string(m.Arity());
  out += "(";
  const char *sep = "";
  for (auto i = 0u; i < m.Arity(); ++i) {
    const ParsedParameter p = m.NthParameter(i);
    out += sep;
    out += p.NameAsString();
    out += ":";
    out += TypeText(module, p.Type());
    sep = ", ";
  }
  out += ")";
  return out;
}

// `bound` / `free` for a query parameter.
static const char *BindingText(ParsedParameter p) {
  switch (p.Binding()) {
    case ParameterBinding::kBound: return "bound";
    default: return "free";
  }
}

// `<name>(<pname>:<binding> <type>, ...)` for a query redeclaration (NO
// /arity for queries).
static std::string QueryDeclText(const ParsedModule &module,
                                 ParsedDeclaration decl) {
  std::string out(decl.NameAsString());
  out += "(";
  const char *sep = "";
  for (auto i = 0u; i < decl.Arity(); ++i) {
    const ParsedParameter p = decl.NthParameter(i);
    out += sep;
    out += p.NameAsString();
    out += ":";
    out += BindingText(p);
    out += " ";
    out += TypeText(module, p.Type());
    sep = ", ";
  }
  out += ")";
  return out;
}

// `(<name>, ...)` over ALL parameters (names only).
static std::string AllParamNames(ParsedDeclaration decl) {
  std::string out = "(";
  const char *sep = "";
  for (auto i = 0u; i < decl.Arity(); ++i) {
    out += sep;
    out += decl.NthParameter(i).NameAsString();
    sep = ", ";
  }
  out += ")";
  return out;
}

// `(<name>, ...)` over all message parameters (names only).
static std::string MessageFieldNames(ParsedMessage m) {
  std::string out = "(";
  const char *sep = "";
  for (auto i = 0u; i < m.Arity(); ++i) {
    out += sep;
    out += m.NthParameter(i).NameAsString();
    sep = ", ";
  }
  out += ")";
  return out;
}

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
// sets — are the Tier-2 contracts, emitted ASCENDING `decl.Id()` (a pure
// mode-stable total order; the per-view sets are already Id-sorted, so the
// cross-view merge is a k-way dedup). Consulted IDENTICALLY by
// `DeriveRegionalCensus` and the contract build below, so V-REGION-CENSUS stays
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
    : query(query_) {}

const ::hyde::Query &FrozenRegionalProgram::Query(void) const {
  return query;
}

const RegionalCensus &FrozenRegionalProgram::Census(void) const {
  return census;
}

const std::vector<RegionalAbi> &FrozenRegionalProgram::Abis(void) const {
  return abis;
}

const std::vector<RegionalPort> &FrozenRegionalProgram::Ports(void) const {
  return ports;
}

const std::vector<RegionalInternal> &FrozenRegionalProgram::Internals(
    void) const {
  return internals;
}

const std::vector<RegionalPermanentRoot> &
FrozenRegionalProgram::PermanentRoots(void) const {
  return permanent_roots;
}

const std::vector<RegionalContract> &FrozenRegionalProgram::Contracts(
    void) const {
  return contracts;
}

std::optional<FrozenRegionalProgram> FrozenRegionalProgram::Build(
    const ::hyde::Query &query, const ErrorLog &log) {
  (void) log;  // No Stage-B reject exists; the parameter reserves the seam.

  FrozenRegionalProgram out(query);
  const ParsedModule module = query.ParsedModule();

  // ---- INPUT / RESULT ports + input/output ABIs (declaration order). Post-
  // cut there are no request-ports (demand is deleted), so port ids start at 0.
  std::vector<ParsedMessage> received, published;
  CollectMessages(query, received, published);

  unsigned next_port = 0u;

  std::vector<RegionalAbi> input_abis, query_abis, output_abis;

  for (ParsedMessage m : received) {
    RegionalPort port;
    port.kind = RegionalPort::kInput;
    port.port_index = next_port++;
    port.head_text =
        "message=" + std::string(m.NameAsString()) + "/" +
        std::to_string(m.Arity());
    port.fields_text = MessageFieldNames(m);

    input_abis.push_back(RegionalAbi{
        RegionalAbi::kInput, MessageDeclText(module, m),
        "-> R0 via P" + std::to_string(port.port_index)});
    out.ports.push_back(std::move(port));
  }

  for (ParsedMessage m : published) {
    RegionalPort port;
    port.kind = RegionalPort::kResult;
    port.port_index = next_port++;
    port.head_text =
        "message=" + std::string(m.NameAsString()) + "/" +
        std::to_string(m.Arity());
    port.fields_text = MessageFieldNames(m);

    output_abis.push_back(RegionalAbi{
        RegionalAbi::kOutput, MessageDeclText(module, m),
        "-> R0 via P" + std::to_string(port.port_index)});
    out.ports.push_back(std::move(port));
  }

  if (output_abis.empty()) {
    output_abis.push_back(RegionalAbi{RegionalAbi::kOutput, "<none>", ""});
  }

  // ---- QUERY ABIs + permanent roots: declaration order; per name, the
  // UniqueRedeclarations walk with the BindingPattern dedup (the
  // BuildQueryEntryPoint idiom, lib/ControlFlow/Build/Build.cpp). Post-cut
  // every redeclaration is a permanent-root observation — a bound `#query`
  // reads the canonical fully-materialized relation via the plain cursor.
  std::unordered_set<uint64_t> seen_queries;
  for (ParsedModule sub_module : ParsedModuleIterator(module)) {
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

        RegionalAbi abi;
        abi.kind = RegionalAbi::kQuery;
        abi.decl_text = QueryDeclText(module, redecl);
        abi.route_text = "-> permanent-root";
        out.permanent_roots.push_back(RegionalPermanentRoot{
            std::string(redecl.NameAsString()) + AllParamNames(redecl)});
        query_abis.push_back(std::move(abi));
      }
    }
  }

  // ABI block order: inputs, queries, outputs (the RegionalAbi::Kind order).
  out.abis = std::move(input_abis);
  out.abis.insert(out.abis.end(), query_abis.begin(), query_abis.end());
  out.abis.insert(out.abis.end(), output_abis.begin(), output_abis.end());

  // ---- ROW-CONTRACTS (Rule R-STORE, Stage-B narrowed form). The member key
  // comes from `query.impl->row_contracts` (the friend leak; the Stage-A
  // side-table, keyed by QueryViewImpl*), rendered POSITIONALLY: for each
  // visible field position `i` (visible_fields order) whose FieldId is in
  // the member key, the RELATION's i-th declared parameter name.
  const RowContractMap &row_contracts = query.impl->row_contracts;
  unsigned edge = 0u;
  for (const auto &[decl, ins] : CollectContractInserts(query)) {
    const QueryView view(ins);
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

    std::string key = "(";
    const char *sep = "";
    for (auto i = 0u; i < rc.visible_fields.size(); ++i) {
      // A desugared zero-arity condition predicate materializes as a UNIT
      // relation whose insert carries the 1 bool `(true)` token column with
      // NO declared parameter behind it — positions beyond the declared
      // arity have no name and are skipped (the contract itself stays; a
      // unit relation renders `member-key=()`).
      if (decl.Arity() <= i) {
        break;
      }
      const FieldId field = rc.visible_fields[i];
      if (std::find(rc.member_key.begin(), rc.member_key.end(), field) ==
          rc.member_key.end()) {
        continue;
      }
      key += sep;
      key += decl.NthParameter(i).NameAsString();
      sep = ", ";
    }
    key += ")";

    RegionalContract contract;
    contract.edge_index = edge++;
    contract.rel_name = std::string(decl.NameAsString());
    contract.member_key_text = std::move(key);
    contract.support_text =
        view.CanReceiveDeletions() ? "differential" : "monotone";
    contract.declared_key = decl.HasInstanceKey();  // K6-7a (DOT-only badge).
    out.contracts.push_back(std::move(contract));
  }

  // ---- Tier-2 ORIGIN-INTERIOR contracts (K5): undemanded #local/#export
  // interiors nameable ONLY via origin decl-sets, APPENDED after the insert-
  // derived R-STORE contracts on the same dense `edge` counter, in ascending
  // decl Id. member-key = AllFields positional (the ORC-3 passthrough
  // contract); support = OR over origin-carrying live views' CanReceiveDeletions
  // (ResolveOriginSupport, a RES-2 loud-abort belt). Arm-A render: plain
  // `rel=NAME`, indistinguishable from R-STORE/Tier-1 (the tier distinction is
  // provenance-only, served by the advisory -origin-out dump). LINE-ADDITIVE but
  // NOT always byte-additive — the per-dump member-key column MAX (Format.cpp)
  // re-pads existing lines when a Tier-2 member-key is the longest (E-K5-PAD).
  for (ParsedDeclaration decl : CollectOriginInteriorDecls(query)) {
    RegionalContract contract;
    contract.edge_index = edge++;
    contract.rel_name = std::string(decl.NameAsString());
    contract.member_key_text = std::string(AllParamNames(decl));
    contract.support_text =
        ResolveOriginSupport(query, decl) ? "differential" : "monotone";
    contract.declared_key = decl.HasInstanceKey();  // K6-7a (DOT-only badge).
    out.contracts.push_back(std::move(contract));
  }

  // ---- CENSUS: DeriveRegionalCensus is the single authority ...
  out.census = DeriveRegionalCensus(query);

  // ... AND the built vectors are independently recounted against it — a
  // divergence is an internal freeze invariant break (fprintf+abort).
  unsigned built_request = 0u, built_input = 0u, built_result = 0u;
  for (const RegionalPort &port : out.ports) {
    switch (port.kind) {
      case RegionalPort::kRequest: ++built_request; break;
      case RegionalPort::kInput: ++built_input; break;
      case RegionalPort::kResult: ++built_result; break;
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
  check_count("row-contracts", static_cast<unsigned>(out.contracts.size()),
              out.census.row_contracts);

  // ---- FREEZE VALIDATORS (always-on, fprintf+abort, surviving NDEBUG — the
  // V-SCC-SEAM house style). Both are the H9 scaffold Stage C extends.

  // V-FROZEN-NO-OPEN-PORT: the Stage-B closed-ABI check — every built port
  // has a set kind and a nonempty head. Stage C extends this to the
  // request-edge lifecycle ports.
  for (const RegionalPort &port : out.ports) {
    if (port.kind != RegionalPort::kRequest &&
        port.kind != RegionalPort::kInput &&
        port.kind != RegionalPort::kResult) {
      fprintf(stderr, "V-FROZEN-NO-OPEN-PORT: port P%u has no kind\n",
              port.port_index);
      abort();
    }
    if (port.head_text.empty()) {
      fprintf(stderr,
              "V-FROZEN-NO-OPEN-PORT: port P%u has an empty head\n",
              port.port_index);
      abort();
    }
  }

  // V-OWNERSHIP-ACYCLIC: the ownership forest is exactly one region with
  // zero child calls at Stage B. Stage C extends this to a real forest
  // acyclicity walk once child calls exist.
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
