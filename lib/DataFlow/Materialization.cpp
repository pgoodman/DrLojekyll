// Copyright 2026, Peter Goodman. All rights reserved.

#include "Materialization.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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

}  // namespace hyde
