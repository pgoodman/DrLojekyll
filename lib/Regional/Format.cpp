// Copyright 2026, Peter Goodman. All rights reserved.
//
// Stage B (RegionalDataFlowCore.artifacts/stage-b-diff.md H6/H7 +
// p2-typed-owner-grounding.md): the G1 `-region-out` dump — the byte-golden-
// able skeleton text (program-root ABIs, the R0 ports/permanent-roots/row-
// contracts, the trailing census line) — and its `-region-dot-out` GraphViz
// DOT twin (advisory, never goldened). Both DERIVE their text from the typed
// `RegionTemplate` records at dump time (the P2 typed-owner cut); the module
// is obtained from the retained DataFlow graph.

#include <drlojekyll/Display/Format.h>
#include <drlojekyll/Regional/Regional.h>

#include <algorithm>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace hyde {
namespace {

// Left-justify `s` to width `w` (no-op when already wider).
static std::string Pad(std::string s, size_t w) {
  if (s.size() < w) {
    s.append(w - s.size(), ' ');
  }
  return s;
}

// ---- Text derivations from the typed records (mirroring the pre-P2 planner's
// string builders — the render authority now lives here).

// Type spelling, mirroring the `.df` emitter's TypeLoc rendering
// (lib/DataFlow/Format.cpp `typed_tok` -> Parse/Format.cpp TypeLoc
// operator<<): the source spelling for built-ins ("u64"/"i32"), the foreign
// type's declared name otherwise — via the module-resolving
// `TypeLoc::Spelling(module)`, which needs no DisplayManager at dump time.
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

static const char *AbiKindTok(AbiKind kind) {
  switch (kind) {
    case AbiKind::kInput: return "input-abi";
    case AbiKind::kQuery: return "query-abi";
    case AbiKind::kOutput: return "output-abi";
  }
  return "output-abi";
}

static const char *PortKindTok(PortKind kind) {
  switch (kind) {
    case PortKind::kRequest: return "request-port";
    case PortKind::kInput: return "input-port";
    case PortKind::kResult: return "result-port";
  }
  return "input-port";
}

// P4/P7: the physical AccessPlan render token (p4-grounding.md §3.1/§9;
// p7-execution-grounding.md §3).
static const char *AccessPlanText(AccessPlan plan) {
  switch (plan) {
    case AccessPlan::kUnplanned: return "unplanned";
    case AccessPlan::kFullScanFilter: return "full-scan-filter";
    case AccessPlan::kFullKeyHashLookup: return "full-key-hash-lookup";
    case AccessPlan::kPartialKeyHashSeek: return "partial-key-hash-seek";
  }
  return "unplanned";
}

// The program-root ABI's declaration text: a message decl for input/output, a
// query decl for query ABIs, and the literal `<none>` for the synthetic
// decl-less output ABI (monostate).
static std::string AbiDeclText(const ParsedModule &module,
                               const AbiRecord &abi) {
  if (std::holds_alternative<ParsedMessage>(abi.decl)) {
    return MessageDeclText(module, std::get<ParsedMessage>(abi.decl));
  }
  if (std::holds_alternative<ParsedDeclaration>(abi.decl)) {
    return QueryDeclText(module, std::get<ParsedDeclaration>(abi.decl));
  }
  return "<none>";  // std::monostate — the `output-abi <none>` line.
}

// The routing text: `-> R0 via P<k>` / `-> permanent-root` /
// `-> request-port P<k>` / "" (no route).
static std::string AbiRouteText(const AbiRecord &abi) {
  switch (abi.route) {
    case RouteKind::kToPortP:
      return "-> R0 via P" + std::to_string(abi.route_port);
    case RouteKind::kPermanentRoot: return "-> permanent-root";
    case RouteKind::kRequestPort:
      return "-> request-port P" + std::to_string(abi.route_port);
    case RouteKind::kNone: return "";
  }
  return "";
}

// `(<bound param names>)` over a query decl's BOUND parameters (the request
// key). All-free decls never reach here (they render as permanent roots).
static std::string BoundFieldNames(ParsedDeclaration decl) {
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

// `message=<name>/<arity>` for a port.
static std::string PortHeadText(const PortRecord &port) {
  return "message=" + std::string(port.message.NameAsString()) + "/" +
         std::to_string(port.message.Arity());
}

// `<name>(<param names>)` for a permanent root.
static std::string PermanentRootText(const PermanentRootRecord &root) {
  return std::string(root.decl.NameAsString()) + AllParamNames(root.decl);
}

// The positional member-key render: the relation's i-th declared parameter
// name for each set position (no RowContract re-lookup, no value-id bridge —
// reads the precomputed positional mask). A unit relation (all-false mask)
// renders `()`.
static std::string RenderMemberKeyText(const RelationSchema &schema) {
  std::string out = "(";
  const char *sep = "";
  for (auto i = 0u; i < schema.decl.Arity(); ++i) {
    if (i >= schema.member_key_positions.size() ||
        !schema.member_key_positions[i]) {
      continue;
    }
    out += sep;
    out += schema.decl.NthParameter(i).NameAsString();
    sep = ", ";
  }
  out += ")";
  return out;
}

// P5: the ORDERED declared-key path render — the relation's declared parameter
// name per ORDINAL in the path's WRITTEN order (order IS identity; no sort). A
// distinct authority from the order-free member key above.
static std::string RenderDeclaredPathText(const RelationSchema &schema,
                                          const DeclaredAccessPath &path) {
  std::string out = "(";
  const char *sep = "";
  for (uint32_t ordinal : path.ordered_fields) {
    out += sep;
    out += schema.decl.NthParameter(ordinal).NameAsString();
    sep = ", ";
  }
  out += ")";
  return out;
}

// P6.2: the relation name for a RelationId (linear scan over relation_schemas,
// like the P6.1 recursive-component member-name lookup).
static std::string RelNameOf(const RegionTemplate &R, RelationId id) {
  for (const RelationSchema &s : R.relation_schemas) {
    if (s.id.v == id.v) {
      return std::string(s.decl.NameAsString());
    }
  }
  return {};
}

// P6.2: "relName.paramName" for a SymbolicFieldId, via the interner's inverse.
// The interner is small; a linear scan keeps the render side-effect-free.
static std::string SymFieldText(const RegionTemplate &R,
                                const RegionInstanceRelations &inst,
                                SymbolicFieldId f) {
  for (const auto &[key, id] : inst.symbolic_field_table) {
    if (id.v != f.v) {
      continue;
    }
    const RelationId rel{key.first};
    const uint32_t ord = key.second;
    for (const RelationSchema &s : R.relation_schemas) {
      if (s.id.v == rel.v) {
        return std::string(s.decl.NameAsString()) + "." +
               std::string(s.decl.NthParameter(ord).NameAsString());
      }
    }
  }
  return {};
}

}  // namespace

OutputStream &operator<<(OutputStream &os, FrozenRegionalDump d) {
  const FrozenRegionalProgram &p = d.program;
  const RegionTemplate &R = p.Region();
  const ParsedModule module = p.DataFlowGraph().ParsedModule();

  os << "region-program\n";

  // ---- program-root block. Kind token left-justified to (max kind length
  // + 2); decl_text left-justified to (max decl_text length + 2); then the
  // route. The `output-abi  <none>` line has no route (and no trailing pad).
  os << "program-root {\n";
  {
    size_t kind_w = 0u, decl_w = 0u;
    for (const AbiRecord &abi : R.abis) {
      kind_w = std::max(kind_w, std::string(AbiKindTok(abi.kind)).size());
      decl_w = std::max(decl_w, AbiDeclText(module, abi).size());
    }
    kind_w += 2u;
    decl_w += 2u;
    for (const AbiRecord &abi : R.abis) {
      const std::string decl_str = AbiDeclText(module, abi);
      const std::string route_str = AbiRouteText(abi);
      os << "  " << Pad(AbiKindTok(abi.kind), kind_w);
      if (route_str.empty()) {
        os << decl_str << "\n";
      } else {
        os << Pad(decl_str, decl_w) << route_str << "\n";
      }
    }
  }
  os << "}\n";

  // ---- region R0 block.
  os << "region R0  owner=program-root  parents=()  children=() {\n";
  {
    // Kind-token width over the EMITTED lines only.
    size_t kind_w = 0u;
    for (const PortRecord &port : R.ports) {
      kind_w = std::max(kind_w, std::string(PortKindTok(port.kind)).size());
    }
    if (!R.request_ports.empty()) {
      kind_w = std::max(kind_w, std::string("request-port").size());
    }
    if (!R.permanent_roots.empty()) {
      kind_w = std::max(kind_w, std::string("permanent-root").size());
    }
    if (!R.relation_schemas.empty()) {
      kind_w = std::max(kind_w, std::string("row-contract").size());
    }
    // P5: the declared-key block (gated on any keyed schema). "declared-key" is
    // 12 chars == "row-contract", so kind_w never widens — no existing region
    // golden moves (E-K5-PAD preserved).
    for (const RelationSchema &schema : R.relation_schemas) {
      if (schema.decl.HasInstanceKey()) {
        kind_w = std::max(kind_w, std::string("declared-key").size());
        break;
      }
    }
    kind_w += 2u;

    // Port lines: `P<k>` then head then `fields=...`, each field padded to
    // its per-dump max + 2. Request-port indices share the `P<k>` column width.
    size_t ptok_w = 0u, head_w = 0u;
    for (const PortRecord &port : R.ports) {
      ptok_w =
          std::max(ptok_w, 1u + std::to_string(port.port_index).size());
      head_w = std::max(head_w, PortHeadText(port).size());
    }
    for (const RequestPortRecord &rp : R.request_ports) {
      ptok_w = std::max(ptok_w, 1u + std::to_string(rp.port_index).size());
    }
    ptok_w += 2u;
    head_w += 2u;
    for (const PortRecord &port : R.ports) {
      os << "  " << Pad(PortKindTok(port.kind), kind_w)
         << Pad("P" + std::to_string(port.port_index), ptok_w)
         << Pad(PortHeadText(port), head_w) << "fields="
         << MessageFieldNames(port.message) << "\n";
    }

    // Request-port lines: `P<k>  query=<name>/<arity>  bound=(<keys>)
    // plan=<access-plan>` — the P4 AccessPlan the freeze selected + codegen reads
    // (p4-grounding.md §3.1/§9). A RootLease-owned bound-query observation entry.
    for (const RequestPortRecord &rp : R.request_ports) {
      os << "  " << Pad("request-port", kind_w)
         << Pad("P" + std::to_string(rp.port_index), ptok_w)
         << "query=" << std::string(rp.query_decl.NameAsString()) << "/"
         << rp.query_decl.Arity() << "  bound="
         << BoundFieldNames(rp.query_decl) << "  plan=" << AccessPlanText(rp.plan)
         << "\n";
    }

    for (const PermanentRootRecord &root : R.permanent_roots) {
      os << "  " << Pad("permanent-root", kind_w) << PermanentRootText(root)
         << "\n";
    }

    // Row-contract lines: `E<k>` padded like ports, then the whole
    // `rel=<name>` field, then the whole `member-key=<key>` field, then
    // `support=<s>`. The edge index is the position in `relation_schemas`.
    size_t etok_w = 0u, rel_w = 0u, key_w = 0u;
    for (auto e = 0u; e < R.relation_schemas.size(); ++e) {
      const RelationSchema &schema = R.relation_schemas[e];
      etok_w = std::max(etok_w, 1u + std::to_string(e).size());
      rel_w = std::max(rel_w, 4u + schema.decl.NameAsString().size());
      key_w = std::max(key_w, 11u + RenderMemberKeyText(schema).size());
    }
    etok_w += 2u;
    rel_w += 2u;
    key_w += 2u;
    for (auto e = 0u; e < R.relation_schemas.size(); ++e) {
      const RelationSchema &schema = R.relation_schemas[e];
      os << "  " << Pad("row-contract", kind_w)
         << Pad("E" + std::to_string(e), etok_w)
         << Pad("rel=" + std::string(schema.decl.NameAsString()), rel_w)
         << Pad("member-key=" + RenderMemberKeyText(schema), key_w)
         << "support=" << (schema.support ? "differential" : "monotone")
         << "\n";
    }

    // P5 declared-key block: one line per declared access path, appended AFTER
    // the row-contract block, in E-order then KeyPathId order (the deterministic
    // InternDeclaredPaths order — a pragma reorder renders byte-identically,
    // F21). Reuses the row-contract `E<k>`/`rel=` columns so they align;
    // `path=(…)` is the trailing unpadded token (like `support=`). The ORDERED
    // logical access path — distinct from the order-free `member-key`. Emitted
    // in its OWN width block (never folded into the row-contract widths — A7),
    // so an unkeyed corpus program emits nothing here and no golden moves.
    for (auto e = 0u; e < R.relation_schemas.size(); ++e) {
      const RelationSchema &schema = R.relation_schemas[e];
      for (const DeclaredAccessPath &path : schema.declared_access_paths.paths) {
        os << "  " << Pad("declared-key", kind_w)
           << Pad("E" + std::to_string(e), etok_w)
           << Pad("rel=" + std::string(schema.decl.NameAsString()), rel_w)
           << "path=" << RenderDeclaredPathText(schema, path) << "\n";
      }
    }

    // P6.1 recursive-component block: one line per recursive component, appended
    // AFTER the declared-key block. Gated on non-empty `recursive_components` (a
    // non-recursive program emits nothing → no golden moves), so — like the P5
    // declared-key block — this carries NO census count. Rendered with its OWN
    // literal spacing: "recursive-component" (19 chars) is NOT folded into the
    // shared `kind_w` column (E-K5-PAD — it would re-pad every row-contract/port
    // line in a recursive dump). Members render by NAME in ascending-RelationId
    // order (the golden-order authority). MODE-FAITHFUL: an un-optimized graph's
    // surviving vacuous self-cycle shows here where the optimized graph strips it.
    for (auto c = 0u; c < R.recursive_components.size(); ++c) {
      const RecursiveComponent &comp = R.recursive_components[c];
      os << "  recursive-component  C" << c << "  members=(";
      for (auto m = 0u; m < comp.members.size(); ++m) {
        if (m) {
          os << ", ";
        }
        std::string name;
        for (const RelationSchema &schema : R.relation_schemas) {
          if (schema.id.v == comp.members[m].v) {
            name = std::string(schema.decl.NameAsString());
            break;
          }
        }
        os << name;
      }
      os << ")\n";
    }

    // P6.2 rule block: one line per PRODUCER CLAUSE that carries at least one
    // frozen route (empty-body_to_head base-case rules are stored for promotion
    // but render nothing). OWN literal width (NOT kind_w — E-K5-PAD). Rendered in
    // stored order (the dense per-clause ordinal, RuleId), so rendered ids may be
    // non-contiguous (base-case producers occupy ids but emit no line). A route
    // renders as srcRel.srcName->headRel.headName.
    bool any_rule = false;
    for (const RuleRoutingProjection &r : R.rules) {
      if (!r.body_to_head.empty()) {
        any_rule = true;
        break;
      }
    }
    if (any_rule) {
      for (const RuleRoutingProjection &r : R.rules) {
        if (r.body_to_head.empty()) {
          continue;
        }
        os << "  rule  R" << r.id.v << "  produces=" << RelNameOf(R, r.head)
           << "  routes=(";
        for (size_t k = 0u; k < r.body_to_head.size(); ++k) {
          if (k) {
            os << ", ";
          }
          os << SymFieldText(R, p.Instances(), r.body_to_head[k].first) << "->"
             << SymFieldText(R, p.Instances(), r.body_to_head[k].second);
        }
        os << ")\n";
      }
    }

    // P6.2 shared-field block: one line per PROMOTED class (size >= 2 members),
    // by representative F<rep> (the class-minimum SymbolicFieldId, union-by-min).
    // Singleton classes are NOT rendered (noise). Members ascend by
    // SymbolicFieldId (== the seed order == relation_schemas E-order x ordinal).
    // OWN literal width. Gated on any non-singleton class.
    {
      std::map<uint32_t, std::vector<uint32_t>> classes;  // rep -> members.
      for (uint32_t f = 0u; f < R.inherited_symbolic_fields.size(); ++f) {
        classes[R.inherited_symbolic_fields[f].v].push_back(f);
      }
      bool any_shared = false;
      for (const auto &[rep, mem] : classes) {
        if (mem.size() >= 2u) {
          any_shared = true;
          break;
        }
      }
      if (any_shared) {
        for (const auto &[rep, mem] : classes) {
          if (mem.size() < 2u) {
            continue;
          }
          os << "  shared-field  F" << rep << "  members=(";
          for (size_t k = 0u; k < mem.size(); ++k) {
            if (k) {
              os << ", ";
            }
            os << SymFieldText(R, p.Instances(), SymbolicFieldId{mem[k]});
          }
          os << ")\n";
        }
      }
    }
  }
  os << "}\n";

  // ---- census (reads the frozen census; the Stage-B skeleton counts).
  const RegionalCensus &census = p.Census();
  os << "census: regions=" << census.regions
     << " child-calls=" << census.child_calls
     << " program-roots=" << census.program_roots
     << " request-ports=" << census.request_ports
     << " input-ports=" << census.input_ports
     << " result-ports=" << census.result_ports
     << " row-contracts=" << census.row_contracts << "\n";

  return os;
}

OutputStream &operator<<(OutputStream &os, FrozenRegionalDOT d) {
  const FrozenRegionalProgram &p = d.program;
  const RegionTemplate &R = p.Region();
  const ParsedModule module = p.DataFlowGraph().ParsedModule();

  os << "digraph {\n"
     << "node [shape=box];\n";

  // The one Stage-B region as a cluster (the house subgraph-cluster DOT
  // directive; region clusters generalize at Stage D).
  os << "subgraph cluster_region_0 {\n"
     << "label=\"region R0\";\n";
  for (const PortRecord &port : R.ports) {
    os << "port_p" << port.port_index << " [label=\"P" << port.port_index
       << " " << PortHeadText(port) << " fields="
       << MessageFieldNames(port.message) << "\"];\n";
  }
  for (const RequestPortRecord &rp : R.request_ports) {
    os << "port_p" << rp.port_index << " [label=\"P" << rp.port_index
       << " request-port query=" << std::string(rp.query_decl.NameAsString())
       << "/" << rp.query_decl.Arity() << " bound="
       << BoundFieldNames(rp.query_decl) << " plan=" << AccessPlanText(rp.plan)
       << "\"];\n";  // P4 plan badge (advisory, never goldened — §8-S12).
  }
  for (auto i = 0u; i < R.permanent_roots.size(); ++i) {
    os << "proot_" << i << " [label=\""
       << PermanentRootText(R.permanent_roots[i]) << "\"];\n";
  }
  for (auto e = 0u; e < R.relation_schemas.size(); ++e) {
    const RelationSchema &schema = R.relation_schemas[e];
    os << "contract_e" << e << " [label=\"E" << e
       << " rel=" << std::string(schema.decl.NameAsString())
       << " member-key=" << RenderMemberKeyText(schema)
       << " support=" << (schema.support ? "differential" : "monotone")
       << (schema.decl.HasInstanceKey() ? " declared-key" : "")  // K6-7a badge.
       << "\"];\n";
  }
  os << "}\n";

  // Program-root ABI nodes OUTSIDE the cluster, with routing edges: a
  // `kToPortP` route points at its port node, a `kPermanentRoot` route at its
  // permanent-root node (permanent roots are minted in the same walk order as
  // their routed ABIs, so the j-th permanent-root route pairs with proot_j).
  unsigned next_proot = 0u;
  for (auto i = 0u; i < R.abis.size(); ++i) {
    const AbiRecord &abi = R.abis[i];
    os << "abi_" << i << " [label=\"" << AbiKindTok(abi.kind) << " "
       << AbiDeclText(module, abi) << "\"];\n";
    if (abi.route == RouteKind::kPermanentRoot) {
      os << "abi_" << i << " -> proot_" << next_proot++ << ";\n";
    } else if (abi.route == RouteKind::kToPortP ||
               abi.route == RouteKind::kRequestPort) {
      os << "abi_" << i << " -> port_p" << abi.route_port << ";\n";
    }
  }

  os << "}\n";
  return os;
}

}  // namespace hyde
