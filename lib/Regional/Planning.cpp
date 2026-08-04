// Copyright 2026, Peter Goodman. All rights reserved.
//
// Stage B (RegionalDataFlowCore.artifacts/stage-b-diff.md H2/H9): the
// degenerate planner + freeze. `FrozenRegionalProgram::Build` derives the
// one-root/one-region skeleton — program-root ABIs, the R0 ports, the
// region-internal fabricated-message lines, the permanent roots, and the
// R-STORE row contracts — from the FINAL Query graph, then freezes it.
//
// Every derivation is DETERMINISTIC: declaration order via
// `ParsedModuleIterator` sub-module walks, forcing order via the
// `DemandForcings()` vector (== ascending forcing index), and contract order
// via the `query.Inserts()` range walk — never a pointer order, never a
// `UniqueId` (the HP-9 rule).

#include <drlojekyll/Regional/Regional.h>

#include <drlojekyll/Parse/ErrorLog.h>
#include <drlojekyll/Parse/ModuleIterator.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
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

// `(<name>, ...)` over the BOUND parameters, in declaration order.
static std::string BoundParamNames(ParsedDeclaration decl) {
  std::string out = "(";
  const char *sep = "";
  for (auto i = 0u; i < decl.Arity(); ++i) {
    const ParsedParameter p = decl.NthParameter(i);
    if (p.Binding() != ParameterBinding::kBound) {
      continue;
    }
    out += sep;
    out += p.NameAsString();
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

// Number of demand forcings carried by `q`'s NAME (`ParsedQuery::operator==`
// compares the declaration context, i.e. (name, arity) across adornments).
static unsigned NumForcingsOfName(
    const std::vector<QueryDemandForcing> &forcings, ParsedQuery q) {
  unsigned n = 0u;
  for (const QueryDemandForcing &entry : forcings) {
    if (entry.query == q) {
      ++n;
    }
  }
  return n;
}

// The RECEIVED REAL messages (received, not demand-fabricated) and the
// PUBLISHED messages, each in declaration order across the sub-module walk,
// deduplicated by declaration id.
static void CollectMessages(const ::hyde::Query &query,
                            std::vector<ParsedMessage> &received,
                            std::vector<ParsedMessage> &published) {
  std::unordered_set<uint64_t> seen;
  for (ParsedModule sub_module : ParsedModuleIterator(query.ParsedModule())) {
    for (ParsedMessage m : sub_module.Messages()) {
      if (!seen.insert(m.Id()).second) {
        continue;
      }
      if (m.IsReceived() && !query.IsDemandMessage(m)) {
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
    if (std::string_view(decl.NameAsString()).starts_with("demand__")) {
      continue;  // The fabricated demand machinery is not a user relation.
    }
    if (!seen_decls.insert(decl.Id()).second) {
      continue;  // Keep the FIRST insert view per distinct declaration.
    }
    out.emplace_back(decl, ins);
  }
  return out;
}

// Tier-1 naming lift: the demand-INTERIOR contracts. The demanded relation's
// model is merge-materialized (no relation-INSERT), so R-STORE cannot name
// it; the mint-time `RecognizedSubgraph::demanded_decl` snapshot can.
// EXISTENCE and COUNT are DECL-DRIVEN and resolve-free (RES-2): the distinct
// `demanded_decl` Ids over `RecognizedSubgraphs()` (append order == forcing
// order — the pass's own deterministic stamp order, HP-9-clean), skipping
// decls already surfaced by an insert-derived R-STORE contract. A pure
// function of the frozen Query, mode-stable by construction, consulted
// IDENTICALLY by `DeriveRegionalCensus` and the contract build below.
static std::vector<ParsedDeclaration> CollectDemandInteriorDecls(
    const ::hyde::Query &query) {
  std::unordered_set<uint64_t> insert_decl_ids;
  for (const auto &[decl, ins] : CollectContractInserts(query)) {
    insert_decl_ids.insert(decl.Id());
  }
  std::vector<ParsedDeclaration> out;
  std::unordered_set<uint64_t> seen;
  for (const RecognizedSubgraph &rs : query.RecognizedSubgraphs()) {
    const ParsedDeclaration decl = rs.demanded_decl;
    if (insert_decl_ids.count(decl.Id())) {
      continue;  // Demanded AND insert-materialized: R-STORE already names it.
    }
    if (!seen.insert(decl.Id()).second) {
      continue;  // First forcing wins (multi-adornment: one shared interior).
    }
    out.push_back(decl);
  }
  return out;
}

// The ONE field the decl cannot supply — `support=` — resolves off the LIVE
// post-Optimize graph (provenance symmetry with the R-STORE render, NEC-1):
// support = the OR over ALL live annotated guard JOINs of the decl's
// forcings of `v.CanReceiveDeletions()` — the deletability of p's demanded
// content (each guard JOIN's output IS a demanded slice of p; the OR covers
// multi-body content and includes demand-side retraction, which is correct
// under `-demand-retract`: retracting demand retracts the guarded rows).
// ROLE-BLIND by design: a kQueryProjection-only resolve is CSE-FRAGILE —
// `PromoteSurvivorToBody` (View.cpp, the g1 survivor policy) promotes a
// projection guard folded into a body guard to kBody, so under `df` opt no
// live view may carry the projection role at all (found live on
// demand_neighborhood_mono_witness at implementation). The OR is order-free
// (no DefList-position dependence); the stored `RecognizedSubgraph` view
// handles dangle post-Optimize and are NEVER read — the walk buckets live
// views by the CSE-migrating `GuardAnnotationIndex` stamp only. A counted
// decl with ZERO live annotated guard JOINs ABORTS the freeze — existence
// is decl-counted above, so a resolve failure can never silently drop a
// contract line (the RES-2 loud-failure construction).
static bool ResolveInteriorSupport(const ::hyde::Query &query,
                                   ParsedDeclaration decl) {
  const std::vector<GuardAnnotation> &annots = query.GuardAnnotations();
  const std::vector<RecognizedSubgraph> &subgraphs =
      query.RecognizedSubgraphs();

  std::unordered_set<unsigned> decl_forcings;
  for (const RecognizedSubgraph &rs : subgraphs) {
    if (rs.demanded_decl.Id() == decl.Id()) {
      decl_forcings.insert(rs.forcing_index);
    }
  }

  bool resolved = false;
  bool support = false;
  query.ForEachView([&](QueryView v) {
    const unsigned ai = v.GuardAnnotationIndex();
    if (ai == QueryView::kNoGuardAnnotation || ai >= annots.size()) {
      return;
    }
    if (!decl_forcings.count(annots[ai].forcing_index) || !v.IsJoin()) {
      return;  // Proxy-TUPLE annotation carriers gate nothing; JOINs do.
    }
    resolved = true;
    support = support || v.CanReceiveDeletions();
  });

  if (!resolved) {
    fprintf(stderr,
            "TIER1-SUPPORT-RESOLVE: no live annotated guard JOIN for "
            "demanded interior relation (rel=%.*s)\n",
            static_cast<int>(decl.NameAsString().size()),
            decl.NameAsString().data());
    abort();
  }
  return support;
}

}  // namespace

RegionalCensus DeriveRegionalCensus(const ::hyde::Query &query) {
  RegionalCensus census;

  census.request_ports =
      static_cast<unsigned>(query.DemandForcings().size());

  std::vector<ParsedMessage> received, published;
  CollectMessages(query, received, published);
  census.input_ports = static_cast<unsigned>(received.size());
  census.result_ports = static_cast<unsigned>(published.size());

  census.row_contracts =
      static_cast<unsigned>(CollectContractInserts(query).size() +
                            CollectDemandInteriorDecls(query).size());

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
  const std::vector<QueryDemandForcing> &forcings = query.DemandForcings();

  // ---- REQUEST-PORTS + region-internal lines: one of each per demand
  // forcing, in DemandForcings() vector order (== ascending forcing index).
  // Port ids are dense across request-ports FIRST, so a request port's index
  // equals its forcing index.
  for (auto fi = 0u; fi < forcings.size(); ++fi) {
    const QueryDemandForcing &entry = forcings[fi];
    const ParsedDeclaration fdecl(entry.query);

    RegionalPort port;
    port.kind = RegionalPort::kRequest;
    port.port_index = fi;
    port.head_text = "query=" + std::string(fdecl.NameAsString());
    if (2u <= NumForcingsOfName(forcings, entry.query)) {
      port.head_text += "  adorn=" + std::string(fdecl.BindingPattern());
    }
    port.fields_text = BoundParamNames(fdecl);
    out.ports.push_back(std::move(port));

    out.internals.push_back(RegionalInternal{
        MessageDeclText(module, entry.message) +
        "  [fabricated, driver-suppressed]"});
  }

  // ---- INPUT / RESULT ports + input/output ABIs (declaration order).
  std::vector<ParsedMessage> received, published;
  CollectMessages(query, received, published);

  unsigned next_port = static_cast<unsigned>(forcings.size());

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
  // BuildQueryEntryPoint idiom, lib/ControlFlow/Build/Build.cpp). A
  // redeclaration matching a demand forcing (same declaration context + same
  // BindingPattern — the injector's matching idiom) is a REQUEST query
  // routed to its forcing's request port; every other one is a
  // permanent-root observation.
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
        const ParsedQuery redecl_query = ParsedQuery::From(redecl);

        int matched_forcing = -1;
        for (auto fi = 0u; fi < forcings.size(); ++fi) {
          if (forcings[fi].query == redecl_query &&
              ParsedDeclaration(forcings[fi].query).BindingPattern() ==
                  redecl.BindingPattern()) {
            matched_forcing = static_cast<int>(fi);
            break;
          }
        }

        std::string decl_text = QueryDeclText(module, redecl);
        if (2u <= NumForcingsOfName(forcings, redecl_query)) {
          decl_text += "  adorn=" + binding;
        }

        RegionalAbi abi;
        abi.kind = RegionalAbi::kQuery;
        abi.decl_text = std::move(decl_text);
        if (0 <= matched_forcing) {
          abi.route_text = "-> R0 via P" + std::to_string(matched_forcing);
        } else {
          abi.route_text = "-> permanent-root";
          out.permanent_roots.push_back(RegionalPermanentRoot{
              std::string(redecl.NameAsString()) + AllParamNames(redecl)});
        }
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
    out.contracts.push_back(std::move(contract));
  }

  // ---- Tier-1 demand-INTERIOR contracts, APPENDED after the insert-derived
  // R-STORE contracts (their E-indices stay stable), in ascending
  // first-forcing order, on the same dense `edge` counter. member-key is the
  // SNAPSHOTTED decl's AllFields rendered positionally (a demanded relation
  // keeps the AllFields/passthrough contract — verified empirically across
  // all 4 modes at the desired-states phase, ORC-3); `support=` resolves off
  // the live projection-guard read (abort on failure — never a silent drop).
  for (ParsedDeclaration decl : CollectDemandInteriorDecls(query)) {
    RegionalContract contract;
    contract.edge_index = edge++;
    contract.rel_name = std::string(decl.NameAsString());
    contract.member_key_text = std::string(AllParamNames(decl));
    contract.support_text =
        ResolveInteriorSupport(query, decl) ? "differential" : "monotone";
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
