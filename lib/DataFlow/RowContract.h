// Copyright 2026, Peter Goodman. All rights reserved.

#pragma once

#include <unordered_map>

#include "Identity.h"

// Stage A hunk H-A3/H-A4/H-A5/H-A7 (RegionalDataFlowCore.artifacts/
// stage-a-diff.md): the RowContract side-table, its conservative inference,
// and the always-on validators.
//
// A RowContract makes a view's SEMANTIC MEMBER IDENTITY typed and provable
// over the existing QueryView graph. It is a PURE, RECOMPUTABLE FUNCTION of
// the FINAL (post-Optimize, post-Stratify) graph — materialized ONCE in the
// Query::Build tail (H-A4), never present during Optimize, so CSE and
// canonicalization have nothing to preserve (the F1 lesson). Nothing here is a
// satellite annotation optimization must be taught to migrate.
//
// SCOPE (flat-key, candidate 1/A-nec-1 + candidate 3): the Stage-A contract is
// FLAT-KEY `{visible_fields, member_key}` only. No derivation-support value
// (the support domains live solely in the H-A1 disjointness battery), no
// candidate-key antichain and no `Minimize` (both defer to Stage B).

namespace hyde {

class QueryImpl;
class QueryViewImpl;
class ErrorLog;

// The conservative row contract for one live view (proposal §4.2, Stage-A
// subset). Both fields are flat vectors of `FieldId`, where a `FieldId`'s value
// is the view-relative column VALUE id (`QueryColumnImpl::id`, finalized by
// `FinalizeColumnIDs`) — value identity, stable under column renumbering,
// which is exactly what a member key needs (two columns with the same value id
// name the same value). Rendered by mapping each `FieldId` back to the view's
// own columns (H-A8).
struct RowContract {
  // Every visible output field of the view, in output-column order.
  SemanticMemberKey visible_fields;

  // The PROVEN semantic member key: the subset of visible fields whose values
  // name a member of the view's relation. A conservative over-approximation is
  // always sound (a larger key never conflates two distinct members); the
  // cyclic rule (H-A3 Phase 1) uses the most conservative key `AllFields`.
  SemanticMemberKey member_key;
};

// The QueryImpl-owned side-table, rebuilt wholesale by the H-A4 call. Keyed by
// raw view pointer (OWNER-GATE O-A2: `LogicalNodeId` deferred to Stage B).
using RowContractMap = std::unordered_map<QueryViewImpl *, RowContract>;

// H-A3/H-A4: infer a conservative RowContract for every live view. A TWO-PHASE
// PURE GRAPH FUNCTION (NOT a fixpoint): Phase 1 assigns every view on a
// multi-view stratum (a recursive SCC) the conservative `AllFields` key
// directly from SCC structure; Phase 2 does a single acyclic per-operator
// transfer over the remaining single-view strata. Must be called AFTER
// `impl->Stratify(log)` (Phase 1 reads `view->stratum`) and after
// `FinalizeColumnIDs`.
RowContractMap InferConservativeRowContracts(QueryImpl *impl);

// H-A7: run the always-on contract validators over `impl->row_contracts`.
// Returns `true` on success. Violations are internal-invariant belts
// (fprintf+abort, surviving NDEBUG, Rel V-* house style): V-CONTRACT-CENSUS
// (one contract per live view), V-MEMBERKEY-REALIZED (every key non-empty and
// field-resolvable), V-NO-COLLAPSE (no kMember TUPLE drops an un-determined
// producer-key column), V-AGG-INPUT-KEY (every aggregate's summarized input
// has a realized member key). On a correct pipeline it fires nothing.
bool ValidateRowContracts(QueryImpl *impl, const ErrorLog &log);

}  // namespace hyde
