<!-- Copyright 2026, Peter Goodman. All rights reserved. -->
# Session-35 forward scope — the ARRANGEMENT derivation (§2.5.3): the true Step-3 unblocker

Owner-ratifiable plan produced at the close of s35 (Phase-C step 2 landed). Step 3
(the allocation inversion) mints ALL runtime resources — tables AND indexes —
from a Rel authority BEFORE region construction, then region builders CONSUME ids
instead of allocating. The RESOURCE (table) half is derived + cross-checked
(s34 MaterializationPlan) and now op-addressable (s35 step 2, `table_to_resource`).
The ARRANGEMENT (index) half does NOT exist: there is no standing
`collect_arrangement_requirements` pass. This doc scopes building it.

## §1 The gap, precisely (verified at tip)

Index requirements materialize ONLY as a SIDE EFFECT of emission, at six inline
`GetOrCreateIndex` call sites (each mints/dedups a `DataIndex` on a `TABLE`,
keyed by a sorted-unique `column_spec`; `lib/ControlFlow/Data.cpp:348`):

| Site | What it keys on |
|---|---|
| `Data.cpp:205` | a keyed DataModel's key columns (FillDataModel-time) |
| `Build.h:443` | a partial-key scan/seek bound-column subset (`BuildMaybeScanPartial`) |
| `Join.cpp:272` | a pivot-JOIN side's pivot columns |
| `Join.cpp:408` | a pivot-JOIN side's pivot columns (second arm) |
| `Build.cpp:432` | a bound `#query` entry-point's bound-column subset |
| `Build.cpp:490/526` | query entry-point / empty-query indexes |

The real allocation is `DataTableImpl::indices` (a `DefList<DataIndex>`, deduped
by `column_spec`). There is NO derivation of this set from the graph — it is
whatever the emission walk happens to request. Step 3 cannot mint indexes up
front until the requirement set is a DERIVED, cross-checked authority.

## §2 The plan — mirror s34 MaterializationPlan (resources-first), staged

The s34 discipline (derive a plan as a codegen-byte-identical OBSERVER, cross-
check it byte-for-byte against the real allocation, THEN invert) applies verbatim.
Three stages, each its own gate; A is a shadow like s34, C is the inversion.

### Stage A — the arrangement CENSUS (observer; codegen byte-identical)
A POST-`Program::Build` read of the real `table->indices`, tagged by the owning
table's `StateResourceId` (**reuse `table_to_resource` from s35 step 2** — this
is the direct Step-2 payoff: arrangements become `(StateResourceId, column-set)`-
addressed, the `ArrangementSpec.source` shape). Emit an `ArrangementId` per
distinct `(resource, sorted column-set)`; render in `-materialization-out`
(the `arrangements=N` header is already printed, always 0 today) + goldens.
Deterministic ids (by `StateResourceId` then ascending column-set). NOTHING
consumes them ⇒ byte-identical codegen. This is the arrangement analog of the
s34 resources slice — low-risk, high-visibility, and it makes the currently-
invisible index set a pinned artifact.

### Stage B — the pure `collect_arrangement_requirements` derivation (+ cross-check)
A pure-`QueryView`-API pass (no `lib/ControlFlow` dep, the `DeriveStatefulClasses`
precedent) that DERIVES the required `(resource, column-set)` arrangements from
the FINAL graph by replaying the six sites' column logic:
- keyed-model key columns (Data.cpp:205 rule);
- per bound `#query`, the SIP bound-column subset (Build.cpp:432 rule — the P7
  `SelectAccessPlan` already names this);
- per pivot JOIN side, the pivot columns (Join.cpp rule);
- per interior partial scan, the bound subset (Build.h:443 rule — the P7b
  `AccessPlan` already classifies these).
Cross-check byte-for-byte against Stage A's census (the falsifiable claim, the
`CrossCheckMaterialization` analog: `derived == real`, abort on divergence).
This is where the P7/P7b/P9 access-path analyses (already landed/deferred)
finally earn a consumer — the derivation IS the access-path authority the
firewall deferred.

### Stage C — the inversion (the megaproject payoff)
`AllocateRuntimeResources(materialization)` mints one `TABLE` per `StateResource`
AND one index per `Arrangement` up front; the six `GetOrCreateIndex` sites become
LOOKUPS (`resource,column-set -> ArrangementId -> index handle`); the Query-shape
join/scan rediscovery is deleted. Region builders consume ids (the Phase-D
codegen move rides on this). Needs Stages A+B first; Step 2's `table_to_resource`
+ this doc's arrangement plan are its two inputs.

## §3 Options considered (seed §5) + recommendation

- (i) pre-pass replicating the inline index logic — this is Stage B; the real
  work, but de-risked by doing Stage A (the census/cross-check target) FIRST.
- (ii) post-Program read (cross-check only) — this is Stage A; necessary but not
  sufficient (derives nothing; can't drive Stage C alone).
- (iii) defer — rejected: it is THE Step-3 blocker; deferring blocks Phase D.

**RECOMMENDATION: do Stage A next session** (the arrangement census + render +
goldens, a codegen-byte-identical observer that reuses `table_to_resource`), then
Stage B (the derivation + cross-check), then Stage C (the inversion). This is the
exact s34→s35 cadence (observer → cross-check → retype → invert) that got the
resource half here safely, and each stage is independently gate-able. Stage A is
a clean one-session slice with a real, visible, goldened artifact.

## §4 Why this is the honest continuation of s35

s35 step 2 made the op model resource-addressable but is admittedly a regression
fence + observability (grounding §4.6). Its ONE forward-load-bearing output —
`table_to_resource` — is exactly the map Stage A consumes to id-address
arrangements, and the map Stage C inverts. So the arrangement arc is where Step
2's map stops being a shadow and becomes a consumed authority. That is the
concrete, named line from the thin-but-real s35 brick to the codegen payoff.
