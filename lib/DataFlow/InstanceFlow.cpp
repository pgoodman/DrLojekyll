// Copyright 2026, Peter Goodman. All rights reserved.

#include "InstanceFlow.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <unordered_map>
#include <vector>

#include "Query.h"

// InstanceFlow.md §7.3 / §8 flat empty-context grove builder + the always-on
// grove validators. See InstanceFlow.h for the scope and the grounding-doc
// references (docs/proposals/InstanceFlow.artifacts/session-32-phaseA-grounding.md).
//
// STAGING (predict-then-verify): this file is being built in two passes. Pass 1
// (LANDED here) = the typed-id catalogs, the SCC-condensation families + nodes,
// and the emission authorities — the structural skeleton (grounding B4). Pass 2
// = the total-ordered OriginUse walk + the coverage bijection (grounding
// B1/B2/S1-S4). Sites tagged `TODO(CP1)` mark the pass-2 seam.

namespace hyde {

void ForEachViewKindTagged(
    Query query,
    const std::function<void(QueryView, unsigned kind, const char *tag)> &cb) {
  const auto wrap = [&cb](QueryView v, unsigned kind, const char *tag) {
    if (!v.impl->is_dead) {
      cb(v, kind, tag);
    }
  };
  for (auto v : query.Selects())    wrap(QueryView::From(v), kIFSelect, "select");
  for (auto v : query.Tuples())     wrap(QueryView::From(v), kIFTuple, "tuple");
  for (auto v : query.KVIndices())  wrap(QueryView::From(v), kIFKVIndex, "kv_index");
  for (auto v : query.Joins())      wrap(QueryView::From(v), kIFJoin, "join");
  for (auto v : query.Maps())       wrap(QueryView::From(v), kIFMap, "map");
  for (auto v : query.Aggregates()) wrap(QueryView::From(v), kIFAggregate, "aggregate");
  for (auto v : query.Merges())     wrap(QueryView::From(v), kIFMerge, "merge");
  for (auto v : query.Negations())  wrap(QueryView::From(v), kIFNegate, "negate");
  for (auto v : query.Compares())   wrap(QueryView::From(v), kIFCompare, "compare");
  for (auto v : query.Inserts())    wrap(QueryView::From(v), kIFInsert, "insert");
}

std::vector<QueryColumn> VisibleColumnsOf(QueryView v, unsigned kind) {
  std::vector<QueryColumn> cols;
  if (kind == kIFInsert) {
    const auto ins = QueryInsert::From(v);
    for (auto i = 0u; i < ins.NumInputColumns(); ++i) {
      cols.push_back(ins.NthInputColumn(i));
    }
  } else {
    for (auto c : v.Columns()) {
      cols.push_back(c);
    }
  }
  return cols;
}

namespace {

// The full visible schema as a flat vector of column VALUE ids (`FieldId`) — the
// empty-context residual (context is empty, so residual == full schema).
static SemanticMemberKey ResidualOf(QueryView v, unsigned kind) {
  SemanticMemberKey key;
  for (auto c : VisibleColumnsOf(v, kind)) {
    key.push_back(FieldId{c.Id()});
  }
  return key;
}

}  // namespace

InstanceFlowProgram BuildFlatInstanceFlow(Query query) {
  InstanceFlowProgram flow;

  // ---- 1. origins (det_seq catalog) -------------------------------------
  // The canonical live-view walk stamps a QueryOriginId == det_seq. det_seq is
  // a dense [0,N) bijection over live views (the .df DF-BIJECTION invariant), so
  // we index a per-origin info table by it.
  struct ViewInfo { QueryView v; unsigned kind; const char *tag; };
  std::unordered_map<unsigned, ViewInfo> by_det;
  unsigned num_views = 0u;
  ForEachViewKindTagged(query, [&](QueryView v, unsigned kind, const char *tag) {
    by_det.emplace(v.DeterministicOrder(), ViewInfo{v, kind, tag});
    ++num_views;
  });
  flow.origins.resize(num_views);
  for (unsigned d = 0u; d < num_views; ++d) {
    flow.origins[d] = QueryOriginId{d};
  }

  // ---- 2. collections + sites (INSERT-target only, S1) ------------------
  // LogicalCollectionId interns `Declaration().Id()` over live INSERT views in
  // first-seen order; one DerivationSite per live INSERT in stored order.
  std::unordered_map<uint64_t, LogicalCollectionId> coll_by_decl;
  std::unordered_map<unsigned, DerivationSiteId> site_of_origin;  // det_seq -> site
  for (auto iv : query.Inserts()) {
    const QueryView v = QueryView::From(iv);
    if (v.impl->is_dead) {
      continue;
    }
    const auto ins = QueryInsert::From(v);
    const uint64_t decl_id = ins.Declaration().Id();
    const QueryOriginId origin{v.DeterministicOrder()};

    auto it = coll_by_decl.find(decl_id);
    LogicalCollectionId cid;
    if (it == coll_by_decl.end()) {
      cid = LogicalCollectionId{static_cast<uint32_t>(flow.collections.size())};
      coll_by_decl.emplace(decl_id, cid);
      flow.collections.push_back(LogicalCollection{cid, decl_id, origin});
    } else {
      cid = it->second;
    }
    const DerivationSiteId sid{static_cast<uint32_t>(flow.sites.size())};
    site_of_origin.emplace(origin.v, sid);
    flow.sites.push_back(DerivationSite{sid, origin, cid});
  }

  // ---- 3. SCC-condensation families + nodes (B4) ------------------------
  // A stratum with >1 live member view IS a recursive SCC (Stratify assigns
  // equal stratum ids iff same SCC — the RowContract Phase-0 / P6.1 notion).
  // NB: `InductionGroupId` tags only the union node, NOT the whole cycle, so it
  // would leave the cycle's tuples/joins in the acyclic family — a §7.6 "half an
  // SCC" violation. Use the multi-view stratum instead (verified empirically on
  // transitive_closure: the 5-view tc cycle shares one stratum).
  std::unordered_map<unsigned, unsigned> stratum_size;
  for (unsigned d = 0u; d < num_views; ++d) {
    if (const auto s = by_det.at(d).v.Stratum()) {
      ++stratum_size[*s];
    }
  }

  // Iterate origins in det_seq order so family/scc ids are first-seen-stable.
  std::optional<FamilyId> acyclic_fam;
  std::unordered_map<unsigned, FamilyId> scc_fam;   // stratum id -> family
  std::unordered_map<unsigned, QuerySccId> scc_id;  // stratum id -> QuerySccId
  std::unordered_map<unsigned, FamilyNodeId> node_of_origin;  // det_seq -> node

  for (unsigned d = 0u; d < num_views; ++d) {
    const ViewInfo &info = by_det.at(d);
    const std::optional<unsigned> stratum = info.v.Stratum();
    const bool recursive =
        stratum.has_value() && stratum_size[*stratum] > 1u;

    FamilyId fam;
    if (!recursive) {
      if (!acyclic_fam.has_value()) {
        acyclic_fam = FamilyId{static_cast<uint32_t>(flow.families.size())};
        flow.families.push_back(Family{*acyclic_fam, SccOwnership::kAcyclic,
                                       std::nullopt, std::nullopt, {}, {}});
      }
      fam = *acyclic_fam;
    } else {
      auto it = scc_fam.find(*stratum);
      if (it == scc_fam.end()) {
        fam = FamilyId{static_cast<uint32_t>(flow.families.size())};
        const QuerySccId s{static_cast<uint32_t>(scc_id.size())};
        scc_id.emplace(*stratum, s);
        scc_fam.emplace(*stratum, fam);
        flow.families.push_back(Family{fam, SccOwnership::kWholeQueryScc, s,
                                       std::nullopt, {}, {}});
      } else {
        fam = it->second;
      }
    }

    Family &family = flow.families[fam.v];
    FamilyNode node;
    node.id = FamilyNodeId{fam.v, static_cast<uint32_t>(family.nodes.size())};
    node.origin = QueryOriginId{d};
    node.kind = info.kind;
    node.tag = info.tag;
    node.role = OccurrenceRole::kInterior;  // TODO(CP1): root selection.
    node.residual = ResidualOf(info.v, info.kind);
    if (info.kind == kIFInsert) {
      node.output_collection =
          coll_by_decl.at(QueryInsert::From(info.v).Declaration().Id());
    }
    node_of_origin.emplace(d, node.id);
    family.nodes.push_back(std::move(node));
  }

  // ---- 4. emission authorities (one per site, domain=All) ---------------
  for (const DerivationSite &site : flow.sites) {
    flow.authorities.push_back(EmissionAuthority{
        EmissionAuthorityId{static_cast<uint32_t>(flow.authorities.size())},
        site.id, node_of_origin.at(site.writer.v)});
  }

  // ---- 5. uses + coverage (B1 total order, B2 decoupled) ----------------
  // Collect every ForEachUse column-role edge over all live views. Producer =
  // Containing(in) resolved to its origin; a CONSTANT operand records the
  // reserved sentinel `kConstProducer` with `producer_col = kNoCol` (S4). A use
  // whose consumer is an INSERT with no out-column (kMaterialized boundary) is a
  // kTerminalInsert obligation carrying its site; every other edge is kInterior.
  constexpr uint32_t kNoCol = UINT32_MAX;
  const QueryOriginId kConstProducer{UINT32_MAX};

  std::vector<OriginUse> raw;
  for (unsigned d = 0u; d < num_views; ++d) {
    const ViewInfo &info = by_det.at(d);
    const unsigned consumer_kind = info.kind;
    info.v.ForEachUse([&](QueryColumn in, InputColumnRole role,
                          std::optional<QueryColumn> out) {
      OriginUse u;
      u.consumer = QueryOriginId{d};
      if (in.IsConstantOrConstantRef()) {
        u.producer = kConstProducer;
        u.producer_col = kNoCol;
      } else {
        const QueryView pview = QueryView::Containing(in);
        u.producer = QueryOriginId{pview.DeterministicOrder()};
        u.producer_col = in.Index().value_or(kNoCol);
      }
      u.consumer_out_col =
          out.has_value() ? out->Index().value_or(kNoCol) : kNoCol;
      u.role = static_cast<unsigned>(role);
      if (consumer_kind == kIFInsert && !out.has_value()) {
        u.cls = UseClass::kTerminalInsert;
        u.terminal_site = site_of_origin.at(d);
      } else {
        u.cls = UseClass::kInterior;
      }
      raw.push_back(u);
    });
  }

  // B1: a strict total order — producer det_seq breaks the leading-shared-column
  // and multi-INSERT ties that (consumer, producer_col, role) alone leave.
  std::sort(raw.begin(), raw.end(), [](const OriginUse &a, const OriginUse &b) {
    if (a.consumer.v != b.consumer.v) return a.consumer.v < b.consumer.v;
    if (a.producer.v != b.producer.v) return a.producer.v < b.producer.v;
    if (a.producer_col != b.producer_col) return a.producer_col < b.producer_col;
    if (a.role != b.role) return a.role < b.role;
    return a.consumer_out_col < b.consumer_out_col;
  });

  flow.uses = std::move(raw);
  for (uint32_t i = 0u; i < flow.uses.size(); ++i) {
    OriginUse &u = flow.uses[i];
    u.id = OriginUseId{i};
    // B2: coverage is a per-consumer-obligation bijection; the occurrence is the
    // consumer's node (for a terminal insert the consumer IS the writer node).
    const FamilyNodeId occ = node_of_origin.at(u.consumer.v);
    flow.coverage.push_back(UseCoverage{u.id, occ});
    flow.families[occ.family].covers.push_back(u.id);
  }

  // ---- 6. seeds (computed, NOT goldened — S5) ---------------------------
  // TODO(CP1+): candidate JoinPivot / AggregateGroup / BoundaryBinding seeds.
  // Deferred; the §5.1 literal-operand vocabulary is unresolved.

  return flow;
}

// ---------------------------------------------------------------------------
// The always-on grove validators. Internal-invariant belts (fprintf+abort,
// surviving NDEBUG); on a correct pipeline they fire nothing. Abort-only — a
// fire is a compiler bug, never a user diagnostic (the return is vestigial,
// kept for the ValidateRowContracts call-site idiom).
// ---------------------------------------------------------------------------

bool ValidateInstanceFlow(Query query, const InstanceFlowProgram &flow,
                          const ErrorLog &) {
  const unsigned n = static_cast<unsigned>(flow.origins.size());

  // Re-derive the det_seq -> (view, kind) map from the live graph (the
  // validators read the SOURCE, never trust the built grove blindly).
  std::unordered_map<unsigned, std::pair<QueryView, unsigned>> by_det;
  ForEachViewKindTagged(query, [&](QueryView v, unsigned kind, const char *) {
    by_det.emplace(v.DeterministicOrder(), std::make_pair(v, kind));
  });

  // V-IF-ORIGIN: the union of all families' nodes is a det_seq bijection onto
  // live views; each node's residual size == its origin view's visible-column
  // count.
  std::vector<bool> seen(n, false);
  for (const Family &fam : flow.families) {
    for (const FamilyNode &node : fam.nodes) {
      const unsigned d = node.origin.v;
      if (d >= n || seen[d]) {
        fprintf(stderr,
                "V-IF-ORIGIN: family node origin det_seq %u out of range or "
                "duplicated over %u live views\n",
                d, n);
        abort();
      }
      seen[d] = true;
      auto it = by_det.find(d);
      if (it == by_det.end()) {
        fprintf(stderr, "V-IF-ORIGIN: node origin det_seq %u is not a live "
                        "view\n", d);
        abort();
      }
      const size_t want =
          VisibleColumnsOf(it->second.first, it->second.second).size();
      if (node.residual.size() != want) {
        fprintf(stderr,
                "V-IF-ORIGIN: node det_seq %u residual width %zu != visible "
                "column count %zu\n",
                d, node.residual.size(), want);
        abort();
      }
    }
  }
  for (unsigned d = 0u; d < n; ++d) {
    if (!seen[d]) {
      fprintf(stderr,
              "V-IF-ORIGIN: live view det_seq %u has no family node\n", d);
      abort();
    }
  }

  // V-IF-SCC: every kWholeQueryScc family's nodes share exactly one multi-view
  // stratum (a recursive SCC); the kAcyclic family holds only views in
  // single-member (or absent) strata. This discharges §7.6 (a family never
  // contains half a recursive SCC).
  std::unordered_map<unsigned, unsigned> stratum_size;
  for (const auto &kv : by_det) {
    if (const auto s = kv.second.first.Stratum()) {
      ++stratum_size[*s];
    }
  }
  for (const Family &fam : flow.families) {
    std::optional<unsigned> shared_stratum;
    for (const FamilyNode &node : fam.nodes) {
      const std::optional<unsigned> s = by_det.at(node.origin.v).first.Stratum();
      const bool recursive = s.has_value() && stratum_size[*s] > 1u;
      if (fam.ownership == SccOwnership::kAcyclic) {
        if (recursive) {
          fprintf(stderr,
                  "V-IF-SCC: acyclic family if#%u holds recursive view det_seq "
                  "%u (multi-view stratum %u)\n",
                  fam.id.v, node.origin.v, *s);
          abort();
        }
      } else {
        if (!recursive) {
          fprintf(stderr,
                  "V-IF-SCC: whole-query-scc family if#%u holds non-recursive "
                  "view det_seq %u\n",
                  fam.id.v, node.origin.v);
          abort();
        }
        if (!shared_stratum.has_value()) {
          shared_stratum = s;
        } else if (*shared_stratum != *s) {
          fprintf(stderr,
                  "V-IF-SCC: whole-query-scc family if#%u mixes strata %u and "
                  "%u\n",
                  fam.id.v, *shared_stratum, *s);
          abort();
        }
      }
    }
  }

  // V-IF-EMISSION: authorities <-> sites is a bijection; every writer is an
  // INSERT-origin node.
  if (flow.authorities.size() != flow.sites.size()) {
    fprintf(stderr,
            "V-IF-EMISSION: %zu authorities over %zu derivation sites\n",
            flow.authorities.size(), flow.sites.size());
    abort();
  }
  for (const EmissionAuthority &ea : flow.authorities) {
    const FamilyNode &writer =
        flow.families[ea.writer.family].nodes[ea.writer.local];
    if (writer.kind != kIFInsert) {
      fprintf(stderr,
              "V-IF-EMISSION: authority ea#%u writer is a %s node, not an "
              "insert\n",
              ea.id.v, writer.tag);
      abort();
    }
  }

  // V-IF-COVERAGE (B2): the coverage is a bijection onto uses — every
  // OriginUseId in [0,M) appears exactly once; each cover's use/occurrence
  // resolves; the occurrence's family lists the use in its `covers`.
  const unsigned m = static_cast<unsigned>(flow.uses.size());
  if (flow.coverage.size() != m) {
    fprintf(stderr, "V-IF-COVERAGE: %zu coverage records over %u uses\n",
            flow.coverage.size(), m);
    abort();
  }
  std::vector<bool> covered(m, false);
  for (const UseCoverage &uc : flow.coverage) {
    if (uc.use.v >= m || covered[uc.use.v]) {
      fprintf(stderr,
              "V-IF-COVERAGE: use u#%u out of range or covered more than once\n",
              uc.use.v);
      abort();
    }
    covered[uc.use.v] = true;
    if (uc.occurrence.family >= flow.families.size() ||
        uc.occurrence.local >=
            flow.families[uc.occurrence.family].nodes.size()) {
      fprintf(stderr, "V-IF-COVERAGE: use u#%u occurrence if#%u.%u does not "
                      "resolve\n",
              uc.use.v, uc.occurrence.family, uc.occurrence.local);
      abort();
    }
  }
  for (unsigned u = 0u; u < m; ++u) {
    if (!covered[u]) {
      fprintf(stderr, "V-IF-COVERAGE: use u#%u is not covered\n", u);
      abort();
    }
  }

  return true;
}

}  // namespace hyde
