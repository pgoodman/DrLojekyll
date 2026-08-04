// Copyright 2026, Peter Goodman. All rights reserved.
//
// Stage B (RegionalDataFlowCore.artifacts/stage-b-diff.md H6/H7): the G1
// `-region-out` dump — the byte-golden-able skeleton text (program-root ABIs,
// the R0 ports/internals/permanent-roots/row-contracts, the trailing census
// line) — and its `-region-dot-out` GraphViz DOT twin (advisory, never
// goldened). Both are pure renders of the frozen row structs; nothing is
// re-derived from the Query graph here.

#include <drlojekyll/Display/Format.h>
#include <drlojekyll/Regional/Regional.h>

#include <algorithm>
#include <string>
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

static const char *AbiKindTok(RegionalAbi::Kind kind) {
  switch (kind) {
    case RegionalAbi::kInput: return "input-abi";
    case RegionalAbi::kQuery: return "query-abi";
    case RegionalAbi::kOutput: return "output-abi";
  }
  return "output-abi";
}

static const char *PortKindTok(RegionalPort::Kind kind) {
  switch (kind) {
    case RegionalPort::kRequest: return "request-port";
    case RegionalPort::kInput: return "input-port";
    case RegionalPort::kResult: return "result-port";
  }
  return "input-port";
}

}  // namespace

OutputStream &operator<<(OutputStream &os, FrozenRegionalDump d) {
  const FrozenRegionalProgram &p = d.program;

  os << "region-program\n";

  // ---- program-root block. Kind token left-justified to (max kind length
  // + 2); decl_text left-justified to (max decl_text length + 2); then the
  // route. The `output-abi  <none>` line has no route (and no trailing pad).
  os << "program-root {\n";
  {
    size_t kind_w = 0u, decl_w = 0u;
    for (const RegionalAbi &abi : p.Abis()) {
      kind_w = std::max(kind_w, std::string(AbiKindTok(abi.kind)).size());
      decl_w = std::max(decl_w, abi.decl_text.size());
    }
    kind_w += 2u;
    decl_w += 2u;
    for (const RegionalAbi &abi : p.Abis()) {
      os << "  " << Pad(AbiKindTok(abi.kind), kind_w);
      if (abi.route_text.empty()) {
        os << abi.decl_text << "\n";
      } else {
        os << Pad(abi.decl_text, decl_w) << abi.route_text << "\n";
      }
    }
  }
  os << "}\n";

  // ---- region R0 block.
  os << "region R0  owner=program-root  parents=()  children=() {\n";
  {
    // Kind-token width over the EMITTED lines only.
    size_t kind_w = 0u;
    for (const RegionalPort &port : p.Ports()) {
      kind_w = std::max(kind_w, std::string(PortKindTok(port.kind)).size());
    }
    if (!p.Internals().empty()) {
      kind_w = std::max(kind_w, std::string("region-internal").size());
    }
    if (!p.PermanentRoots().empty()) {
      kind_w = std::max(kind_w, std::string("permanent-root").size());
    }
    if (!p.Contracts().empty()) {
      kind_w = std::max(kind_w, std::string("row-contract").size());
    }
    kind_w += 2u;

    // Port lines: `P<k>` then head then `fields=...`, each field padded to
    // its per-dump max + 2.
    size_t ptok_w = 0u, head_w = 0u;
    for (const RegionalPort &port : p.Ports()) {
      ptok_w = std::max(
          ptok_w, 1u + std::to_string(port.port_index).size());
      head_w = std::max(head_w, port.head_text.size());
    }
    ptok_w += 2u;
    head_w += 2u;
    for (const RegionalPort &port : p.Ports()) {
      os << "  " << Pad(PortKindTok(port.kind), kind_w)
         << Pad("P" + std::to_string(port.port_index), ptok_w)
         << Pad(port.head_text, head_w) << "fields=" << port.fields_text
         << "\n";
    }

    for (const RegionalInternal &internal : p.Internals()) {
      os << "  " << Pad("region-internal", kind_w) << internal.text << "\n";
    }

    for (const RegionalPermanentRoot &root : p.PermanentRoots()) {
      os << "  " << Pad("permanent-root", kind_w) << root.text << "\n";
    }

    // Row-contract lines: `E<k>` padded like ports, then the whole
    // `rel=<name>` field, then the whole `member-key=<key>` field, then
    // `support=<s>`.
    size_t etok_w = 0u, rel_w = 0u, key_w = 0u;
    for (const RegionalContract &contract : p.Contracts()) {
      etok_w = std::max(
          etok_w, 1u + std::to_string(contract.edge_index).size());
      rel_w = std::max(rel_w, 4u + contract.rel_name.size());
      key_w = std::max(key_w, 11u + contract.member_key_text.size());
    }
    etok_w += 2u;
    rel_w += 2u;
    key_w += 2u;
    for (const RegionalContract &contract : p.Contracts()) {
      os << "  " << Pad("row-contract", kind_w)
         << Pad("E" + std::to_string(contract.edge_index), etok_w)
         << Pad("rel=" + contract.rel_name, rel_w)
         << Pad("member-key=" + contract.member_key_text, key_w)
         << "support=" << contract.support_text << "\n";
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

  os << "digraph {\n"
     << "node [shape=box];\n";

  // The one Stage-B region as a cluster (the house subgraph-cluster DOT
  // directive; region clusters generalize at Stage D).
  os << "subgraph cluster_region_0 {\n"
     << "label=\"region R0\";\n";
  for (const RegionalPort &port : p.Ports()) {
    os << "port_p" << port.port_index << " [label=\"P" << port.port_index
       << " " << port.head_text << " fields=" << port.fields_text << "\"];\n";
  }
  for (auto i = 0u; i < p.Internals().size(); ++i) {
    os << "internal_" << i << " [label=\"" << p.Internals()[i].text
       << "\"];\n";
  }
  for (auto i = 0u; i < p.PermanentRoots().size(); ++i) {
    os << "proot_" << i << " [label=\"" << p.PermanentRoots()[i].text
       << "\"];\n";
  }
  for (const RegionalContract &contract : p.Contracts()) {
    os << "contract_e" << contract.edge_index << " [label=\"E"
       << contract.edge_index << " rel=" << contract.rel_name
       << " member-key=" << contract.member_key_text
       << " support=" << contract.support_text << "\"];\n";
  }
  os << "}\n";

  // Program-root ABI nodes OUTSIDE the cluster, with routing edges: a
  // `-> R0 via P<k>` route points at its port node, a `-> permanent-root`
  // route at its permanent-root node (permanent roots are minted in the same
  // walk order as their routed ABIs, so the j-th permanent-root route pairs
  // with proot_j).
  unsigned next_proot = 0u;
  for (auto i = 0u; i < p.Abis().size(); ++i) {
    const RegionalAbi &abi = p.Abis()[i];
    os << "abi_" << i << " [label=\"" << AbiKindTok(abi.kind) << " "
       << abi.decl_text << "\"];\n";
    if (abi.route_text == "-> permanent-root") {
      os << "abi_" << i << " -> proot_" << next_proot++ << ";\n";
    } else if (auto pos = abi.route_text.rfind(" P");
               pos != std::string::npos) {
      os << "abi_" << i << " -> port_p"
         << abi.route_text.substr(pos + 2u) << ";\n";
    }
  }

  os << "}\n";
  return os;
}

}  // namespace hyde
