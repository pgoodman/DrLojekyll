// Copyright 2026, Peter Goodman. All rights reserved.

#include "RowContract.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <unordered_set>
#include <vector>

#include "Query.h"

// H-A3/H-A4/H-A5/H-A7. Doc, algorithm, and before/after diagram.
//
// ALGORITHM (InferConservativeRowContracts — a two-phase PURE GRAPH FUNCTION,
// NOT a fixpoint):
//
//   Phase 0  : tally |views| per stratum (from view->stratum, populated by
//              Stratify). A stratum with >1 member view IS a recursive SCC
//              (Stratify assigns equal stratum ids iff same SCC; a size-1 SCC
//              is never a self-cycle — RelabelGroupIDs forbids a view being
//              its own direct user).
//   Phase 1  : every view on a MULTI-VIEW stratum gets the conservative key
//              member_key = AllFields(columns), read DIRECTLY from SCC
//              structure. This dissolves the contract-dependency fixpoint that
//              a recursive program would otherwise have (a back-edge input's
//              contract is never read empty).
//   Phase 2  : the remaining single-view strata form an ACYCLIC condensation;
//              cross-SCC data edges go low->high depth, so DEPTH ORDER over
//              them is topological. One pass, per-operator transfer, each
//              acyclic view's inputs already resolved (acyclic-earlier or
//              cyclic-Phase-1). No fixpoint, bounded by |columns|.
//
//   PSEUDOCODE:
//     stratum_size = histogram(v.stratum for live v)
//     for v in depth_order: if stratum_size[v.stratum] > 1: out[v] = AllFields
//     for v in depth_order: if v not in out:                out[v] = Transfer(v)
//
// TRANSFER (Phase 2, acyclic; flat-key, no support term). A FieldId's value is
// a column VALUE id (QueryColumnImpl::id), so a producer column and the output
// column that forwards its value share a FieldId. "mapped input key" is then a
// value-id intersection: an output column is in the key iff its value id is in
// the producer's member key.
//
//     SELECT/MERGE/MAP/KVINDEX : key = AllFields(columns)   (sound conservative)
//     TUPLE kDistinct          : key = AllFields(columns)   (visible IS the key)
//     TUPLE kMember / CMP /
//        NEGATE / INSERT       : key = { out col : out col.id in producer key }
//     JOIN                     : key = { out col : some join-input col of it is
//                                        in that input's producer key }
//     AGGREGATE                : key = { out col : out col.id in group_by/config
//                                        input ids }   (output = group key)
//
// BEFORE (member identity implicit, only empirically documented):
//
//     select edge/3 ---> tuple[distinct] ---> aggregate ---> insert
//        (?key)             (?key)              (?key)        (?key)
//
// AFTER (member identity typed + proven, recomputable side-table):
//
//     select edge/3 ---> tuple[distinct] ---> aggregate ---> insert
//     key=(B,X,W)       key=(X,W2)          key=(X)         key=(X,N)
//                       [visible IS key]    in=(X,W2)       [passthrough]

namespace hyde {
namespace {

// The view's VISIBLE fields, as column pointers in output order. An INSERT is
// terminal (its `columns` DefList is empty — Insert.cpp asserts it); its
// visible fields are what it INSERTS, i.e. its `input_columns`. Every other
// kind uses its output `columns`.
static std::vector<QueryColumnImpl *> VisibleCols(QueryViewImpl *v) {
  std::vector<QueryColumnImpl *> cols;
  if (v->AsInsert()) {
    for (auto col : v->input_columns) {
      cols.push_back(col);
    }
  } else {
    for (auto col : v->columns) {
      cols.push_back(col);
    }
  }
  return cols;
}

// AllFields as a raw FieldId vector, in visible-field order.
static SemanticMemberKey AllFieldIds(QueryViewImpl *v) {
  SemanticMemberKey key;
  for (auto col : VisibleCols(v)) {
    key.push_back(FieldId{col->id});
  }
  return key;
}

// The value-id set of all of a view's visible fields.
static std::unordered_set<unsigned> AllFieldIdSet(QueryViewImpl *v) {
  std::unordered_set<unsigned> s;
  for (auto col : VisibleCols(v)) {
    s.insert(col->id);
  }
  return s;
}

// The producer's member-key value-id set: from `out` when present, else its
// conservative AllFields (an out-of-order read stays sound, never empty).
static std::unordered_set<unsigned> ProducerKeyIds(QueryViewImpl *p,
                                                   const RowContractMap &out) {
  std::unordered_set<unsigned> s;
  const auto it = out.find(p);
  if (it != out.end()) {
    for (auto f : it->second.member_key) {
      s.insert(f.v);
    }
  } else {
    for (auto col : p->columns) {
      s.insert(col->id);
    }
  }
  return s;
}

// Materialize a member key (deduped, in output-column order) from a value-id
// set. Guarantees every FieldId resolves to a live output column of `v`.
static SemanticMemberKey MemberKeyFromIds(
    QueryViewImpl *v, const std::unordered_set<unsigned> &ids) {
  SemanticMemberKey key;
  std::unordered_set<unsigned> seen;
  for (auto col : VisibleCols(v)) {
    if (ids.count(col->id) && seen.insert(col->id).second) {
      key.push_back(FieldId{col->id});
    }
  }
  return key;
}

// The single summarized-input view of an aggregate: the producer of its
// aggregated / group / config columns.
static QueryViewImpl *AggInputView(QueryAggregateImpl *agg) {
  for (auto c : agg->aggregated_columns) {
    if (!c->IsConstant()) {
      return c->view;
    }
  }
  for (auto c : agg->group_by_columns) {
    if (!c->IsConstant()) {
      return c->view;
    }
  }
  for (auto c : agg->config_columns) {
    if (!c->IsConstant()) {
      return c->view;
    }
  }
  return nullptr;
}

// Phase-2 per-operator transfer for one ACYCLIC (single-view-stratum) view.
static RowContract TransferContract(QueryViewImpl *v,
                                    const RowContractMap &out) {
  RowContract rc;
  rc.visible_fields = AllFieldIds(v);

  std::unordered_set<unsigned> key_ids;

  // output key = producer key mapped by value id.
  const auto passthrough = [&](QueryViewImpl *p) {
    if (!p) {
      return AllFieldIdSet(v);
    }
    const auto pk = ProducerKeyIds(p, out);
    std::unordered_set<unsigned> s;
    for (auto col : VisibleCols(v)) {
      if (pk.count(col->id)) {
        s.insert(col->id);
      }
    }
    return s;
  };

  if (auto tuple = v->AsTuple()) {
    if (tuple->projection_role == QueryTupleImpl::ProjectionRole::kDistinct) {
      key_ids = AllFieldIdSet(v);  // §4.4 distinct: visible tuple IS the key.
    } else {  // kMember: preserve the mapped producer member key.
      key_ids = passthrough(QueryViewImpl::GetIncomingView(v->input_columns,
                                                           v->attached_columns));
    }

  } else if (v->AsCompare()) {  // §4.4 filter: preserve input key.
    key_ids = passthrough(QueryViewImpl::GetIncomingView(v->input_columns,
                                                         v->attached_columns));

  } else if (v->AsNegate()) {  // §4.4 negation: positive member key preserved.
    key_ids = passthrough(QueryViewImpl::GetIncomingView(v->input_columns,
                                                         v->attached_columns));

  } else if (v->AsInsert()) {  // §4.4 insert: passthrough of the input head.
    key_ids = passthrough(QueryViewImpl::GetIncomingView(v->input_columns));

  } else if (auto join = v->AsJoin()) {  // §4.4 join: union of mapped keys.
    for (auto col : v->columns) {
      const auto it = join->out_to_in.find(col);
      if (it == join->out_to_in.end()) {
        continue;
      }
      for (auto in_col : it->second) {
        if (in_col->IsConstant()) {
          continue;
        }
        if (ProducerKeyIds(in_col->view, out).count(in_col->id)) {
          key_ids.insert(col->id);
          break;
        }
      }
    }

  } else if (auto agg = v->AsAggregate()) {  // §4.4 aggregate: group key.
    // Output columns are laid out [group_by..., config..., summary...] (see
    // QueryAggregate::SummaryColumns). The OUTPUT member key is the group +
    // config prefix (the summary columns are functionally determined by the
    // group and are NOT key members). Positional, not id-matched: an output
    // group column need not share its input group column's value id.
    const auto n_key =
        agg->group_by_columns.Size() + agg->config_columns.Size();
    unsigned i = 0u;
    for (auto col : v->columns) {
      if (i++ < n_key) {
        key_ids.insert(col->id);
      }
    }

  } else {
    // SELECT (DeclaredKeyOf deferred -> AllFields), MERGE, MAP, KVINDEX, and
    // any future kind: the conservative AllFields key, always a sound member
    // key. (MAP/KVINDEX refined rules defer with Stage B; AllFields is a sound
    // over-approximation and no witness pins them.)
    key_ids = AllFieldIdSet(v);
  }

  // A mapped key that collapses to nothing (e.g. a facade forwarding only
  // constants) falls back to the conservative AllFields — never an empty key.
  if (key_ids.empty()) {
    key_ids = AllFieldIdSet(v);
  }

  rc.member_key = MemberKeyFromIds(v, key_ids);
  return rc;
}

// ---- H-A7 validator belts (always-on fprintf+abort, survive NDEBUG) -------

// V-CONTRACT-CENSUS: |row_contracts| == |live views|, exactly one per live
// view, none for a dead view.
static void CheckContractCensus(QueryImpl *impl) {
  unsigned live = 0u;
  impl->ForEachView([&](QueryViewImpl *v) {
    ++live;
    if (!impl->row_contracts.count(v)) {
      fprintf(stderr,
              "V-CONTRACT-CENSUS: live view %s has no row contract\n",
              v->KindName());
      abort();
    }
  });
  if (impl->row_contracts.size() != live) {
    fprintf(stderr,
            "V-CONTRACT-CENSUS: %zu contracts over %u live views "
            "(a dead view carries a contract)\n",
            impl->row_contracts.size(), live);
    abort();
  }
}

// V-MEMBERKEY-REALIZED: every live view's member key is non-empty (when the
// view has any column) and every FieldId in it resolves to a live column.
static void CheckMemberKeyRealized(QueryImpl *impl) {
  impl->ForEachView([&](QueryViewImpl *v) {
    const auto &rc = impl->row_contracts.at(v);
    const auto live_ids = AllFieldIdSet(v);
    if (!live_ids.empty() && rc.member_key.empty()) {
      fprintf(stderr,
              "V-MEMBERKEY-REALIZED: %s view has an empty member key\n",
              v->KindName());
      abort();
    }
    for (auto f : rc.member_key) {
      if (!live_ids.count(f.v)) {
        fprintf(stderr,
                "V-MEMBERKEY-REALIZED: %s view member key field %u resolves to "
                "no live column\n",
                v->KindName(), f.v);
        abort();
      }
    }
  });
}

// V-NO-COLLAPSE (BELT-ONLY, per E-A2): the amended text's user-facing firing
// condition — "a kMember TUPLE dropping a column not DeterminedBy its retained
// key" — is UNSOUND AS A HARD ERROR at Stage A, and is therefore NOT enforced
// as one. The reason is fundamental to the flat-key model: Stage-A producer
// keys are the CONSERVATIVE `AllFields` (never a proven-minimal key), so EVERY
// dropped column reads as a dropped "key" column even for an ordinary,
// perfectly valid projection (verified: `MiniDisassembler`/`PointsTo` have
// acyclic kMember facades that legitimately drop a data column; the F2 lesson's
// real collapse is indistinguishable from a benign projection without a proven
// key, which is Stage-B `Minimize` territory). A conservative producer key
// makes any drop SOUND — the kMember tuple simply gets the smaller MAPPED key,
// which is itself a sound member key — so there is nothing to reject.
//
// The invariant this belt CAN soundly assert (and does): a kMember TUPLE's
// mapped member key must be non-empty and a subset of its visible fields —
// which V-MEMBERKEY-REALIZED already guarantees for every view. So Stage A's
// V-NO-COLLAPSE is inert on a correct pipeline (the `collapse_error=0`
// census witness), exactly the E-A2 "belt-only internal invariant" outcome.
// A real user-facing "unproven collapse" reject reappears at Stage B, when a
// proven-minimal key exists to make the drop distinguishable.
static void CheckNoCollapse(QueryImpl *impl) {
  impl->ForEachView([&](QueryViewImpl *v) {
    auto tuple = v->AsTuple();
    if (!tuple ||
        tuple->projection_role != QueryTupleImpl::ProjectionRole::kMember) {
      return;
    }
    // The sound belt: a kMember projection never yields an empty mapped key
    // while carrying visible fields (the transfer's AllFields fallback ensures
    // this; a regression that broke it would abort here).
    const auto &rc = impl->row_contracts.at(v);
    if (!VisibleCols(v).empty() && rc.member_key.empty()) {
      fprintf(stderr,
              "V-NO-COLLAPSE: kMember TUPLE has visible fields but an empty "
              "member key (the projection collapsed to nothing)\n");
      abort();
    }
  });
}

// V-AGG-INPUT-KEY: every aggregate's summarized-input view has a realized
// member key (H-A5).
static void CheckAggInputKey(QueryImpl *impl) {
  impl->ForEachView([&](QueryViewImpl *v) {
    auto agg = v->AsAggregate();
    if (!agg) {
      return;
    }
    auto in = AggInputView(agg);
    if (!in) {
      fprintf(stderr,
              "V-AGG-INPUT-KEY: aggregate has no summarized-input view\n");
      abort();
    }
    const auto it = impl->row_contracts.find(in);
    if (it == impl->row_contracts.end() || it->second.member_key.empty()) {
      fprintf(stderr,
              "V-AGG-INPUT-KEY: aggregate's summarized input (%s) has no "
              "realized member key\n",
              in->KindName());
      abort();
    }
  });
}

}  // namespace

RowContractMap InferConservativeRowContracts(QueryImpl *impl) {
  RowContractMap out;

  // Live views in ascending FINALIZED-depth order. CRITICAL: this is a
  // READ-ONLY sort over the `view->depth` values already set by
  // `FinalizeDepths` — it must NOT call `QueryImpl::ForEachViewInDepthOrder`,
  // which RESETS `view->depth = 0` and recomputes it as a side effect.
  // Downstream Program::Build / Rel lowering read `view->depth` for their
  // work-item ordering, so mutating it here silently re-orders `.rel`/`.ir`/
  // codegen output (the F-side-effect this replaces). A stable sort by the
  // frozen depth is topological over the acyclic condensation (cross-SCC edges
  // go low->high depth) and mutates nothing.
  std::vector<QueryViewImpl *> views_by_depth;
  impl->ForEachView([&](QueryViewImpl *v) { views_by_depth.push_back(v); });
  std::stable_sort(views_by_depth.begin(), views_by_depth.end(),
                   [](QueryViewImpl *a, QueryViewImpl *b) {
                     return a->depth < b->depth;
                   });

  // Phase 0 — per-stratum view histogram (deterministic).
  std::unordered_map<unsigned, unsigned> stratum_size;
  for (QueryViewImpl *v : views_by_depth) {
    if (v->stratum.has_value()) {
      ++stratum_size[*v->stratum];
    }
  }

  // Phase 1 — cyclic rule: every view on a multi-view stratum -> AllFields.
  for (QueryViewImpl *v : views_by_depth) {
    if (v->stratum.has_value() && stratum_size[*v->stratum] > 1u) {
      RowContract rc;
      rc.visible_fields = AllFieldIds(v);
      rc.member_key = MemberKeyFromIds(v, AllFieldIdSet(v));
      out.emplace(v, std::move(rc));
    }
  }

  // Phase 2 — acyclic per-operator transfer, topological by frozen depth.
  for (QueryViewImpl *v : views_by_depth) {
    if (out.count(v)) {
      continue;  // already assigned by Phase 1.
    }
    out.emplace(v, TransferContract(v, out));
  }

  return out;
}

bool ValidateRowContracts(QueryImpl *impl, const ErrorLog &) {
  CheckContractCensus(impl);
  CheckMemberKeyRealized(impl);
  CheckNoCollapse(impl);
  CheckAggInputKey(impl);
  return true;
}

}  // namespace hyde
