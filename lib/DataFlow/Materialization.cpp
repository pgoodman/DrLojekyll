// Copyright 2026, Peter Goodman. All rights reserved.

#include "Materialization.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <drlojekyll/Parse/ModuleIterator.h>
#include <drlojekyll/Parse/Parse.h>

#include "Query.h"

// MaterializationPlan resources-first observer (docs/proposals/InstanceFlow.md
// §10; session-34-seed.md §2.5.1). See Materialization.h for scope. This file
// re-derives ControlFlow's stateful-storage decisions from the FINAL graph and
// builds one authoritative resource per stateful collection + one per stateful
// internal residue. It is a PURE QueryView-API function — it MUST NOT depend on
// lib/ControlFlow (the acyclic layering: ControlFlow -> DataFlow). The
// falsifiable claim — that this derivation equals what `FillDataModel` actually
// allocates — is checked by the `CrossCheckMaterialization` belt in
// lib/ControlFlow/Build/Build.cpp (which reads `program->view_to_model`).

namespace hyde {
namespace {

// A faithful copy of `NeedsInductionCycleVector` / `NeedsInductionOutputVector`
// (lib/ControlFlow/Build/Induction.cpp:10-25) expressed over public QueryView
// API — the R3 merge-table predicate. Kept a byte-for-byte transcription so the
// cross-check belt stays quiescent; a drift in the original is caught by the
// belt, not silently mirrored.
static bool NeedsInductionCycleVectorLocal(QueryView view) {
  if (view.InductionGroupId().has_value()) {
    if (view.IsMerge()) {
      return true;
    } else {
      return !view.NonInductivePredecessors().empty() ||
             view.IsOwnIndirectInductiveSuccessor();
    }
  } else {
    return false;
  }
}

static bool NeedsInductionOutputVectorLocal(QueryView view) {
  return !view.NonInductiveSuccessors().empty();
}

}  // namespace

// Replay `FillDataModel`'s TABLE-need rule set (lib/ControlFlow/Build/Build.cpp
// :37-185) over the FINAL graph, returning the set of stateful storage classes
// as `EquivalenceSetId` values (the DataModel classes are EXACTLY the
// EquivalenceSetId partition — BuildDataModel:233 union-finds one DataModel per
// view by `view.EquivalenceSetId()`). A view named by a rule gives its CLASS a
// table; so the stateful class set is `{ EquivalenceSetId(v) : v named }`.
std::set<unsigned> DeriveStatefulClasses(Query query) {
  std::unordered_set<QueryView> marked;
  const auto mark = [&marked](QueryView v) { marked.insert(v); };

  // R1: every deletion-receiver persists each predecessor.
  query.ForEachView([&](QueryView view) {
    if (view.CanReceiveDeletions()) {
      for (auto pred : view.Predecessors()) {
        mark(pred);
      }
    }
  });

  // R2: relation INSERTs (+ the condition-witness setter's predecessor).
  for (auto insert : query.Inserts()) {
    const QueryView view = QueryView::From(insert);
    if (insert.IsRelation()) {
      mark(view);
      if (insert.NumAttachedColumns()) {
        mark(view.Predecessors()[0]);
      }
    }
  }

  // R3: induction cycle/output MERGE vectors.
  for (auto merge : query.Merges()) {
    const QueryView view = QueryView::From(merge);
    if (NeedsInductionCycleVectorLocal(view) ||
        NeedsInductionOutputVectorLocal(view)) {
      mark(view);
    }
  }

  // R4: JOIN inputs (+ the differential-JOIN output when a successor drops a
  // pivot). The pivot-usage successor walk is copied verbatim.
  for (auto join : query.Joins()) {
    const QueryView view = QueryView::From(join);
    for (auto pred : join.JoinedViews()) {
      mark(pred);
    }
    if (view.CanReceiveDeletions()) {
      const auto num_pivots = join.NumPivotColumns();
      for (auto succ_view : view.Successors()) {
        std::vector<bool> used_pivots(num_pivots);
        auto num_used_pivots = 0u;
        succ_view.ForEachUse([&](QueryColumn in_col, InputColumnRole,
                                 std::optional<QueryColumn>) {
          if (!in_col.IsConstant() && QueryView::Containing(in_col) == view) {
            if (auto index = *(in_col.Index());
                index < num_pivots && !used_pivots[index]) {
              used_pivots[index] = true;
              ++num_used_pivots;
            }
          }
        });
        if (num_used_pivots < num_pivots) {
          mark(view);
          break;
        }
      }
    }
  }

  // R5: NEGATE — the negated view and the pre-negate predecessor.
  for (auto negate : query.Negations()) {
    const QueryView view = QueryView::From(negate);
    mark(negate.NegatedView());
    mark(view.Predecessors()[0]);
  }

  // R6/R7: differential MAP / COMPARE outputs.
  for (auto map : query.Maps()) {
    const QueryView view = QueryView::From(map);
    if (view.CanProduceDeletions()) {
      mark(view);
    }
  }
  for (auto cmp : query.Compares()) {
    const QueryView view = QueryView::From(cmp);
    if (view.CanProduceDeletions()) {
      mark(view);
    }
  }

  // R8: AGGREGATE / KVINDEX view + every predecessor.
  const auto force_agg = [&](QueryView view) {
    mark(view);
    for (auto pred : view.Predecessors()) {
      mark(pred);
    }
  };
  for (auto agg : query.Aggregates()) {
    force_agg(QueryView::From(agg));
  }
  for (auto kv : query.KVIndices()) {
    force_agg(QueryView::From(kv));
  }

  // R9 (runs LAST in FillDataModel): a monotone message-tap INSERT whose CLASS
  // is not already table-backed by R1-R8 gets its own dedup table. The real
  // pass reads the accumulated table set; we freeze the R1-R8 class set and test
  // membership — the resulting CLASS set is identical either way (two stream
  // taps sharing one class both pass, adding the same class once).
  std::set<unsigned> classes_1to8;
  for (QueryView v : marked) {
    classes_1to8.insert(v.EquivalenceSetId());
  }
  for (auto insert : query.Inserts()) {
    const QueryView view = QueryView::From(insert);
    if (insert.IsStream() && !view.CanReceiveDeletions() &&
        !classes_1to8.count(view.EquivalenceSetId())) {
      mark(view);
    }
  }

  std::set<unsigned> classes;
  for (QueryView v : marked) {
    classes.insert(v.EquivalenceSetId());
  }
  return classes;
}

MaterializationResources PlanResources(Query query,
                                       const InstanceFlowProgram &flow) {
  MaterializationResources plan;
  const std::set<unsigned> stateful = DeriveStatefulClasses(query);

  // det_seq -> (view, kind) over live views (the grove/dump walk order); plus a
  // per-class support flag (differential iff any member CanReceiveDeletions) and
  // the min-det_seq representative per class.
  std::unordered_map<unsigned, std::pair<QueryView, unsigned>> by_det;
  std::unordered_map<unsigned, bool> class_support;
  std::unordered_map<unsigned, unsigned> class_rep_det;  // eqset -> min det_seq
  ForEachViewKindTagged(query, [&](QueryView v, unsigned kind, const char *) {
    const unsigned d = v.DeterministicOrder();
    by_det.emplace(d, std::make_pair(v, kind));
    const unsigned eq = v.EquivalenceSetId();
    class_support[eq] = class_support[eq] || v.CanReceiveDeletions();
    auto it = class_rep_det.find(eq);
    if (it == class_rep_det.end() || d < it->second) {
      class_rep_det[eq] = d;
    }
  });

  const auto support_of = [&](unsigned eq) {
    return class_support[eq] ? SupportPolicy::kDifferential
                             : SupportPolicy::kMonotone;
  };
  const auto schema_of = [&](QueryView v, unsigned kind) {
    SemanticMemberKey key;
    for (auto c : VisibleColumnsOf(v, kind)) {
      key.push_back(FieldId{c.Id()});
    }
    return key;
  };

  // Group the STATEFUL collections by their backing physical class. A class may
  // hold several collections (co-recursive relations sharing one store —
  // reachable_from + reaching_to alias tc's table). `flow.collections` is in
  // lc-id order, so each vector is ascending; its FRONT is the canonical
  // (min-lc) authority. Ephemeral collections (class not stateful) are dropped
  // here (V-MAT-AUTHORITY: they get no persistent resource).
  std::unordered_map<unsigned, std::vector<uint32_t>> class_collections;
  for (const LogicalCollection &lc : flow.collections) {
    const unsigned eq = by_det.at(lc.writer0.v).first.EquivalenceSetId();
    if (stateful.count(eq)) {
      class_collections[eq].push_back(lc.id.v);
    }
  }

  // Collection-authority resources: ONE per stateful class that has >=1
  // collection writer, emitted in canonical-lc-id order (iterate collections in
  // lc order; the canonical writer is the first-seen of its class). Record the
  // class -> its StateResourceId for the alias pass. The BIJECTION
  // (resource <-> physical class) is the property the Phase-C TABLE* retype
  // needs (`ResourceForView(v) = authority(EquivalenceSetId(v))`).
  std::unordered_map<unsigned, StateResourceId> class_to_resource;
  for (const LogicalCollection &lc : flow.collections) {
    const auto &info = by_det.at(lc.writer0.v);
    const unsigned eq = info.first.EquivalenceSetId();
    if (!stateful.count(eq) || class_collections.at(eq).front() != lc.id.v) {
      continue;  // ephemeral, or a non-canonical shared-store writer (-> alias).
    }
    const StateResourceId sid{static_cast<uint32_t>(plan.resources.size())};
    class_to_resource.emplace(eq, sid);
    StateResource r;
    r.id = sid;
    r.internal = false;
    r.collection = lc.id;
    r.schema = schema_of(info.first, info.second);
    r.support = support_of(eq);
    r.representative = lc.writer0;
    r.eqset = eq;
    plan.resources.push_back(std::move(r));
  }

  // Internal-residue resources: one per stateful class with NO collection
  // writer, in ascending EquivalenceSetId order (the deterministic
  // InternalResidualCollectionId key). Schema/name from the class's min-det_seq
  // representative view (a shared co-recursive store — ping/pong — names via one
  // representative; the physical schema is shared, so the label is advisory).
  uint32_t residue_k = 0u;
  for (unsigned eq : stateful) {  // std::set: ascending
    if (class_collections.count(eq)) {
      continue;  // has a collection authority already.
    }
    const unsigned rep_det = class_rep_det.at(eq);
    const auto &info = by_det.at(rep_det);
    const StateResourceId sid{static_cast<uint32_t>(plan.resources.size())};
    class_to_resource.emplace(eq, sid);
    StateResource r;
    r.id = sid;
    r.internal = true;
    r.residual = InternalResidualCollectionId{residue_k++};
    r.schema = schema_of(info.first, info.second);
    r.support = support_of(eq);
    r.representative = QueryOriginId{rep_det};
    r.eqset = eq;
    plan.resources.push_back(std::move(r));
  }

  // Forwarding aliases: each non-canonical stateful collection sharing a store
  // resolves to that store's authoritative resource (§10 `ForwardingAlias`), in
  // lc-id order. This is how §17 V-MAT-AUTHORITY stays satisfied under sharing:
  // every stateful collection resolves to exactly one authoritative resource —
  // its own (canonical) or its alias target.
  for (const LogicalCollection &lc : flow.collections) {
    const unsigned eq = by_det.at(lc.writer0.v).first.EquivalenceSetId();
    if (!stateful.count(eq) || class_collections.at(eq).front() == lc.id.v) {
      continue;
    }
    plan.aliases.push_back(ForwardingAlias{lc.id, class_to_resource.at(eq)});
  }

  return plan;
}

// Session-36: derive the ARRANGEMENT (index) requirements from the FINAL graph
// (see Materialization.h for the rule set R-FULL/R-JOIN-UNIFORM/R-NEG/R-QUERY
// and session-36-grounding.md for the panel record). PURE QueryView-API — the
// falsifiable claim (the emission walk's `GetOrCreateIndex` requests are a pure
// function of the final graph) is checked by `CrossCheckArrangements` at the
// `Program::Build` tail. NOTE the one named blind spot (grounding §4-d): the
// view-ordinal -> table-ordinal congruence across a shared model is REPLAYED
// from emission's own assumption (both sides use `QueryColumn::Index()`), so
// the cross-check certifies byte-equality with emission, not independent
// soundness of that congruence.
void DeriveArrangements(Query query, MaterializationResources &plan,
                        bool demand_instance) {
  // Re-derivation into a plan COPY is legal (the S2a demand_instance arm):
  // clear the two outputs this function owns.
  plan.arrangements.clear();
  plan.interface_tables = 0u;

  // The bijection map (V-MAT-BIJECTION): class -> its authoritative resource.
  std::unordered_map<unsigned, StateResourceId> class_to_resource;
  for (const StateResource &r : plan.resources) {
    class_to_resource.emplace(r.eqset, r.id);
  }

  // Per-class arity under the SAME rule that fixes the real table's column
  // count (Data.cpp:150-205: InputColumns for an INSERT view, Columns
  // otherwise — `VisibleColumnsOf` is that rule). Belt: every member of a
  // class must agree (the class IS one physical schema; a divergence means the
  // positional ordinal space is broken — the E2 congruence tripwire).
  std::unordered_map<unsigned, unsigned> class_arity;
  ForEachViewKindTagged(query, [&](QueryView v, unsigned kind, const char *) {
    const unsigned eq = v.EquivalenceSetId();
    const unsigned arity =
        static_cast<unsigned>(VisibleColumnsOf(v, kind).size());
    const auto it = class_arity.find(eq);
    if (it == class_arity.end()) {
      class_arity.emplace(eq, arity);
    } else if (it->second != arity) {
      fprintf(stderr,
              "V-MAT-ARRANGE: storage class %u members disagree on arity "
              "(%u vs %u) — the positional ordinal space is broken\n",
              eq, it->second, arity);
      abort();
    }
  });

  // The requirement set. `ArrangementKey`'s defaulted ordering IS the
  // canonical order (ascending resource, then lexicographic ordinal vector),
  // and the set dedups exactly like `GetOrCreateIndex`'s per-table
  // `column_spec` dedup (per-resource == per-table: table <-> class <->
  // resource are bijective).
  std::set<ArrangementKey> required;
  const auto require = [&](QueryView v, std::vector<ColumnOrdinal> ords,
                           const char *rule) {
    std::sort(ords.begin(), ords.end());
    ords.erase(std::unique(ords.begin(), ords.end()), ords.end());
    const auto it = class_to_resource.find(v.EquivalenceSetId());
    if (it == class_to_resource.end()) {
      fprintf(stderr,
              "V-MAT-ARRANGE: rule %s requires an index on non-stateful "
              "class %u — the emitter would have no table there\n",
              rule, v.EquivalenceSetId());
      abort();
    }
    required.insert(ArrangementKey{it->second, std::move(ords)});
  };

  // R-FULL: the all-columns default index every table gets at creation.
  for (const StateResource &r : plan.resources) {
    const auto it = class_arity.find(r.eqset);
    if (it == class_arity.end()) {
      fprintf(stderr,
              "V-MAT-ARRANGE: resource sr#%u backs class %u with no live "
              "member view\n",
              r.id.v, r.eqset);
      abort();
    }
    std::vector<ColumnOrdinal> all;
    all.reserve(it->second);
    for (auto i = 0u; i < it->second; ++i) {
      all.push_back(ColumnOrdinal{i});
    }
    required.insert(ArrangementKey{r.id, std::move(all)});
  }

  // R-JOIN-UNIFORM: per joined side of every pivot-JOIN, the side's pivot
  // input-column ordinals (BuildJoin takes the FIRST input pivot of a side per
  // pivot set — Join.cpp:393-401 `break` — while EmitJoinFire collects ALL;
  // the singleton belt below makes the two conventions coincide, aborting on
  // any future shape where they would not).
  for (auto join : query.Joins()) {
    const auto num_pivots = join.NumPivotColumns();
    if (!num_pivots) {
      continue;  // zero-pivot == @product: non-driving sides scan FULL.
    }
    // S2a demand_instance arm (the §2 belts-integration rule; s37 panel-fixed
    // predicate): under the nested lowering a recognized-subgraph guard JOIN
    // is excised from the eager walk and its band rescan is a full scan, so
    // no pivot index is minted — UNLESS the join is deletion-capable, in
    // which case the delta/stratum path still emits it (and mints the
    // indexes) regardless of the eager excision. Set semantics keep entries
    // that coincide with R-FULL or another rule's request. Soundness
    // precondition: joins strictly inside a demanded body are rejected
    // upstream by the plain `-demand` body walk (see Materialization.h).
    if (demand_instance &&
        QueryView(join).GuardAnnotationIndex() !=
            QueryView::kNoGuardAnnotation &&
        !QueryView(join).CanReceiveDeletions()) {
      continue;
    }
    for (QueryView side : join.JoinedViews()) {
      std::vector<ColumnOrdinal> ords;
      for (auto j = 0u; j < num_pivots; ++j) {
        auto found = 0u;
        for (auto pivot_col : join.NthInputPivotSet(j)) {
          if (QueryView::Containing(pivot_col) == side) {
            if (!found++) {
              ords.push_back(ColumnOrdinal{*(pivot_col.Index())});
            }
          }
        }
        if (found > 1u) {
          fprintf(stderr,
                  "V-MAT-ARRANGE: join pivot %u has %u input columns on one "
                  "side — BuildJoin/EmitJoinFire request different sets\n",
                  j, found);
          abort();
        }
      }
      if (ords.empty()) {
        fprintf(stderr,
                "V-MAT-ARRANGE: a pivot-JOIN side participates in no pivot "
                "set — the emitter would mint an empty-spec index\n");
        abort();
      }
      require(side, std::move(ords), "R-JOIN");
    }
  }

  // R-NEG: the negation crossover's predecessor-side partial scan
  // (Stratum.cpp:1168-1185 replayed verbatim; both crossover arms request the
  // same set, and monotone crossovers request it too, so no differentialness
  // predicate is needed). The two drop clauses mirror BuildMaybeScanPartial:
  // empty -> full scan (no index), all-columns -> early return (no scan).
  for (auto negate : query.Negations()) {
    if (negate.HasNeverHint()) {
      continue;  // @never gates on Present — no crossover arm-pair.
    }
    const QueryView view = QueryView::From(negate);
    const QueryView pred_view = view.Predecessors()[0];
    std::vector<ColumnOrdinal> ords;
    auto i = 0u;
    for (QueryColumn out_col : negate.NegatedColumns()) {
      (void) out_col;
      const QueryColumn pred_key_col = negate.NthInputColumn(i++);
      if (!pred_key_col.IsConstant()) {
        if (QueryView::Containing(pred_key_col) != pred_view) {
          fprintf(stderr,
                  "V-MAT-ARRANGE: a negate key input is not a predecessor "
                  "column — the crossover scan would mis-bind\n");
          abort();
        }
        ords.push_back(ColumnOrdinal{*(pred_key_col.Index())});
      }
    }
    std::sort(ords.begin(), ords.end());
    ords.erase(std::unique(ords.begin(), ords.end()), ords.end());
    if (ords.empty() || ords.size() == pred_view.Columns().size()) {
      continue;
    }
    require(pred_view, std::move(ords), "R-NEG");
  }

  // R-QUERY: bound `#query` entry-point seeks (Build.cpp:432), one per unique
  // binding pattern per surviving relation-INSERT of a query decl (the
  // (unique pattern) x (shared class) quantification — every INSERT of one
  // relation shares a class, so the per-insert replay dedups). Mirrors
  // `SelectAccessPlan` (RegionInstance.h — Regional layering bars calling it):
  // kFullScanFilter <=> empty bound set at tip, which withholds the index; an
  // all-bound pattern requests all columns and dedups into R-FULL.
  for (auto insert : query.Inserts()) {
    if (!insert.IsRelation()) {
      continue;
    }
    const ParsedDeclaration decl = insert.Relation().Declaration();
    if (!decl.IsQuery()) {
      continue;
    }
    const QueryView view = QueryView::From(insert);
    std::unordered_set<std::string> seen_variants;
    for (ParsedDeclaration redecl : decl.UniqueRedeclarations()) {
      std::string binding(redecl.BindingPattern());
      if (!seen_variants.insert(std::move(binding)).second) {
        continue;
      }
      std::vector<ColumnOrdinal> ords;
      for (auto param : redecl.Parameters()) {
        if (param.Binding() == ParameterBinding::kBound) {
          ords.push_back(ColumnOrdinal{param.Index()});
        }
      }
      if (ords.empty()) {
        continue;  // kFullScanFilter withholds — no requirement.
      }
      require(view, std::move(ords), "R-QUERY");
    }
  }

  // R-INTERFACE (count only): one always-empty interface table per `#query`
  // declaration with NO surviving INSERT view (Build.cpp:1445-1458 /
  // BuildEmptyQueryEntryPoint). Its per-table index sets are a NAMED Stage-C
  // residual (grounding §5.1); the COUNT is derived + cross-checked so the
  // census never silently drops an unattributed table.
  std::unordered_set<uint64_t> covered;
  for (auto insert : query.Inserts()) {
    if (insert.IsRelation()) {
      const ParsedDeclaration decl = insert.Relation().Declaration();
      if (decl.IsQuery()) {
        covered.insert(decl.Id());
      }
    }
  }
  std::unordered_set<uint64_t> counted;
  for (ParsedModule sub_module : ParsedModuleIterator(query.ParsedModule())) {
    for (ParsedQuery parsed_query : sub_module.Queries()) {
      const uint64_t id = ParsedDeclaration(parsed_query).Id();
      if (!covered.count(id) && counted.insert(id).second) {
        ++plan.interface_tables;
      }
    }
  }

  // Finalize: dense ids in the set's canonical order.
  plan.arrangements.reserve(required.size());
  for (const ArrangementKey &key : required) {
    Arrangement a;
    a.id = ArrangementId{static_cast<uint32_t>(plan.arrangements.size())};
    a.key = key;
    plan.arrangements.push_back(std::move(a));
  }
}

// V-MAT-BIJECTION (panel claim-d) + V-MAT-AUTHORITY (§17). Internal-invariant
// belt (fprintf+abort, surviving NDEBUG).
bool ValidateMaterialization(Query query, const InstanceFlowProgram &flow,
                             const MaterializationResources &plan,
                             const ErrorLog &) {
  const std::set<unsigned> stateful = DeriveStatefulClasses(query);

  // det_seq -> view for writer0 -> EquivalenceSetId resolution.
  std::unordered_map<unsigned, QueryView> by_det;
  ForEachViewKindTagged(query, [&](QueryView v, unsigned, const char *) {
    by_det.emplace(v.DeterministicOrder(), v);
  });

  // V-MAT-BIJECTION: resources <-> stateful classes is a bijection (each stateful
  // class appears as EXACTLY ONE resource's eqset; every resource's class is
  // stateful). This is the property the Phase-C TABLE* retype depends on
  // (ResourceForView(v) = authority(EquivalenceSetId(v)) is total, single-valued).
  std::set<unsigned> resource_classes;
  uint32_t expect_residue = 0u;
  for (const StateResource &r : plan.resources) {
    if (!stateful.count(r.eqset)) {
      fprintf(stderr,
              "V-MAT-BIJECTION: resource sr#%u backs non-stateful class "
              "(eqset %u)\n",
              r.id.v, r.eqset);
      abort();
    }
    if (!resource_classes.insert(r.eqset).second) {
      fprintf(stderr,
              "V-MAT-BIJECTION: two resources back one physical class "
              "(eqset %u) — not a bijection\n",
              r.eqset);
      abort();
    }
    if (r.internal && r.residual.v != expect_residue++) {
      fprintf(stderr,
              "V-MAT-AUTHORITY: internal residue sr#%u has non-dense id "
              "internal#%u\n",
              r.id.v, r.residual.v);
      abort();
    }
  }
  if (resource_classes != stateful) {
    fprintf(stderr,
            "V-MAT-BIJECTION: %zu resources cover %zu stateful classes\n",
            resource_classes.size(), stateful.size());
    abort();
  }

  // Alias resolution: every alias points at a real resource whose class equals
  // the aliased collection's class; and every stateful collection is EITHER a
  // resource's canonical writer OR aliased — exactly one (§17 V-MAT-AUTHORITY).
  std::unordered_map<uint32_t, unsigned> resolved;  // lc.v -> #authoritative
  for (const StateResource &r : plan.resources) {
    if (!r.internal) {
      ++resolved[r.collection.v];
    }
  }
  for (const ForwardingAlias &a : plan.aliases) {
    if (a.to.v >= plan.resources.size()) {
      fprintf(stderr, "V-MAT-AUTHORITY: alias lc#%u points at missing sr#%u\n",
              a.collection.v, a.to.v);
      abort();
    }
    const unsigned lc_eq = by_det.at(flow.collections[a.collection.v].writer0.v)
                               .EquivalenceSetId();
    if (plan.resources[a.to.v].eqset != lc_eq) {
      fprintf(stderr,
              "V-MAT-AUTHORITY: alias lc#%u (eqset %u) resolves to sr#%u backing "
              "a different class (eqset %u)\n",
              a.collection.v, lc_eq, a.to.v, plan.resources[a.to.v].eqset);
      abort();
    }
    ++resolved[a.collection.v];
  }
  for (const LogicalCollection &lc : flow.collections) {
    const unsigned eq = by_det.at(lc.writer0.v).EquivalenceSetId();
    const unsigned have = resolved.count(lc.id.v) ? resolved[lc.id.v] : 0u;
    if (stateful.count(eq)) {
      if (have != 1u) {
        fprintf(stderr,
                "V-MAT-AUTHORITY: stateful collection lc#%u resolves to %u "
                "authoritative resources (want exactly 1)\n",
                lc.id.v, have);
        abort();
      }
    } else if (have != 0u) {
      fprintf(stderr,
              "V-MAT-AUTHORITY: ephemeral collection lc#%u has %u persistent "
              "support resources (want 0)\n",
              lc.id.v, have);
      abort();
    }
  }

  return true;
}

// The shadow-contract cross-check (session-34-seed.md §2.5.1). ControlFlow
// passes the REAL table-backed storage-class set (EquivalenceSetId of every view
// whose `view_to_model` class got a TABLE); we compare it against the STORED
// plan's resource classes. Validating the STORED plan (not a fresh
// DeriveStatefulClasses) is deliberate — it catches a future graph mutation
// between the Query::Build grove point and Program::Build's FillDataModel that
// would silently stale the shipped plan (panel claim-a fold ii). Abort on
// divergence; on a correct pipeline it fires nothing.
const MaterializationResources &MaterializationPlanOf(Query query) {
  return query.impl->materialization;
}

void CrossCheckMaterialization(Query query,
                               const std::set<unsigned> &real) {
  const MaterializationResources &plan = query.impl->materialization;
  std::set<unsigned> planned;
  for (const StateResource &r : plan.resources) {
    planned.insert(r.eqset);
  }
  if (planned == real) {
    return;
  }
  fprintf(stderr,
          "CROSS-CHECK (materialization): the resources-first plan's %zu "
          "storage classes diverge from the %zu ControlFlow actually "
          "table-backed:\n",
          planned.size(), real.size());
  for (unsigned eq : planned) {
    if (!real.count(eq)) {
      fprintf(stderr, "  plan resources a class ControlFlow did NOT back "
                      "(eqset %u)\n",
              eq);
    }
  }
  for (unsigned eq : real) {
    if (!planned.count(eq)) {
      fprintf(stderr, "  ControlFlow backed a class the plan MISSED (eqset "
                      "%u)\n",
              eq);
    }
  }
  abort();
}

// Session-36: the arrangement shadow contract. ControlFlow passes the REAL
// index universe censused at the `Program::Build` tail (each `TABLEINDEX` as
// its owning table's `StateResourceId` via the s35 `table_to_resource` map +
// its sorted column ordinals; the universe is FINAL there — no pass mints or
// deletes a TABLEINDEX after region build) plus the count of resource-less
// interface tables (empty-query tables, provably member-view-free). We compare
// against the STORED derived plan and abort naming every divergent arrangement
// on either side. Always-on (fprintf+abort, survives NDEBUG); on a correct
// pipeline it fires nothing — exercised suite-wide across all 4 modes.
void CrossCheckArrangements(Query query,
                            const std::vector<ArrangementKey> &real,
                            unsigned num_interface_tables) {
  // The public form compares against the STORED (flat-derived) plan; the
  // S2a `-demand-instance` caller passes a flag-aware re-derived copy to the
  // 4-arg core instead (Materialization.h).
  CrossCheckArrangements(query, real, num_interface_tables,
                         query.impl->materialization);
}

void CrossCheckArrangements(Query query,
                            const std::vector<ArrangementKey> &real,
                            unsigned num_interface_tables,
                            const MaterializationResources &plan) {
  (void) query;

  std::set<ArrangementKey> real_set;
  for (const ArrangementKey &key : real) {
    if (!real_set.insert(key).second) {
      fprintf(stderr,
              "CROSS-CHECK (arrangements): ControlFlow holds DUPLICATE "
              "index (sr#%u, %zu cols) — table<->resource not injective?\n",
              key.resource.v, key.columns.size());
      abort();
    }
  }
  std::set<ArrangementKey> derived_set;
  for (const Arrangement &a : plan.arrangements) {
    derived_set.insert(a.key);
  }

  const auto render = [](const ArrangementKey &key) {
    std::string s = "sr#" + std::to_string(key.resource.v) + " columns=(";
    const char *sep = "";
    for (ColumnOrdinal c : key.columns) {
      s += sep + std::to_string(c.v);
      sep = ",";
    }
    return s + ")";
  };

  bool diverged = false;
  for (const ArrangementKey &key : derived_set) {
    if (!real_set.count(key)) {
      if (!diverged) {
        fprintf(stderr, "CROSS-CHECK (arrangements): derived plan diverges "
                        "from the real index universe:\n");
        diverged = true;
      }
      fprintf(stderr, "  plan derives an index ControlFlow did NOT mint: %s\n",
              render(key).c_str());
    }
  }
  for (const ArrangementKey &key : real_set) {
    if (!derived_set.count(key)) {
      if (!diverged) {
        fprintf(stderr, "CROSS-CHECK (arrangements): derived plan diverges "
                        "from the real index universe:\n");
        diverged = true;
      }
      fprintf(stderr, "  ControlFlow minted an index the plan MISSED: %s\n",
              render(key).c_str());
    }
  }
  if (num_interface_tables != plan.interface_tables) {
    fprintf(stderr,
            "CROSS-CHECK (arrangements): %u resource-less interface tables "
            "censused, plan derived %u\n",
            num_interface_tables, plan.interface_tables);
    diverged = true;
  }
  if (diverged) {
    abort();
  }
}

}  // namespace hyde
