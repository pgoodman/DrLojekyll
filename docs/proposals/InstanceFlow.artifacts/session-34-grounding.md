<!-- Copyright 2026, Peter Goodman. All rights reserved. -->
# Session-34 grounding — resources-first MaterializationPlan (adjudicated)

Branch `keyed-instances`, tip after CP2 (`c489715c`). Method: read the real
materialization-side code (`FillDataModel`/`BuildDataModel`/`GetOrCreateIndex`)
+ empirical witness dumps, then a 3-refuter opus panel (claims a/b/d refuted;
claim c errored on a StructuredOutput retry cap but its concern is SUBSUMED by
the claim-d fold). Predict-then-verify byte-matched every witness dump. Raw
panel transcripts: `subagents/workflows/wf_8fb20106-005/journal.jsonl`.

## The mission (unchanged, executed)

Build the `MaterializationPlan` RESOURCE authority — resources-only-first slice
(§2.5.1). A codegen-byte-identical OBSERVER that DERIVES ControlFlow's
stateful-storage decisions from the grove and CROSS-CHECKS them against the real
`view_to_model` allocation. The §2.5.3 fork was settled (iii) resources-only,
arrangements deferred — grounding confirmed there is no standing
`collect_arrangement_requirements` pass (index requirements only materialize
inline at `GetOrCreateIndex` call sites) and no Rel struct carries an
arrangement identity today, so deferral is sound (panel claim-d evidence).

## The panel verdicts (folded)

### CLAIM (a) — derivable without `program->tables` — REFUTED (survives), high
Every `FillDataModel` R1-R9 predicate maps to a public `QueryView`/`QueryInsert`
accessor, all populated BEFORE the grove is built (`IdentifyInductions` :2633,
`TrackDifferentialUpdates` :2640, `BuildEquivalenceSets` :2645, grove :2664).
`NeedsInductionCycleVector/OutputVector` reduce, for a MERGE, to
`InductionGroupId().has_value()` / `!NonInductiveSuccessors().empty()`. R9's
"class already table-backed" is exactly `EquivalenceSetId(v) ∈ classes_1to8`,
order/mutation-free at class granularity. **Load-bearing sub-claim confirmed:**
the DataModel classes ARE the `EquivalenceSetId` partition — `BuildDataModel`
(Build.cpp:233-248) unions models STRICTLY by `EquivalenceSetId`; `GetOrCreate`
(Data.cpp:147-206) sets `model->table` WITHOUT unioning; all 13-15
`TABLE::GetOrCreate` sites are inside `FillDataModel`. So `program->tables` is
redundant with the pure derivation.
- **FOLD (adopted):** the belt validates the STORED plan (not a fresh
  `DeriveStatefulClasses`) against real tables, so a FUTURE graph mutation
  between the grove point and `FillDataModel` is caught rather than silently
  staling the shipped plan. `CrossCheckMaterialization` reads
  `query.impl->materialization`.

### CLAIM (b) — cross-check byte-identical AND non-vacuous — REFUTED, med-high
The belt is a genuine independent-code-path check, not covered-by-construction.
Non-vacuity is real and PLURAL: measured distinct `%table` counts 6 (join_1),
2 (transitive_closure), 2 (corecursion_1) — the join-input residues (R4) make
join_1 bite hard (unlike CP2's vacuous kBoundQueryRead arm). A transcription bug
FIRES the belt (verified live: corrupting R4 → the belt aborts, naming the 4
missed residue classes eqset 7-10; reverting → quiescent).
- **NOTE (accepted):** R4's second clause (differential JOIN whose successor
  drops a pivot) is the most intricate reconstruction — the corpus's differential
  join cases exercise it and the belt referees them corpus-wide.

### CLAIM (d) — real, non-shadow line to the Phase-C `TABLE*` retype — REFUTED, high — THE KEY FOLD
`StateResourceId` IS consumable by `BuildRel` (no Rel field carries arrangement
identity; an arrangement is a pure function of `(resource-table, bound_cols)`
recovered at lowering via `GetOrCreateIndex`), so deferring arrangements is
sound. **BUT the surviving concern:** "one resource per stateful COLLECTION" is
finer than physical tables — transitive_closure's `reachable_from` + `reaching_to`
both `authority_for` ONE table, making `StateResourceId → TABLE*` MANY-TO-ONE and
its inverse (which the retype needs at each Rel site) AMBIGUOUS.
- **FOLD (adopted — the load-bearing design change):** ONE authoritative
  `StateResource` per stateful PHYSICAL class (`EquivalenceSetId`), authority =
  the canonical (min-`LogicalCollectionId`) writer; other collections sharing the
  store are `ForwardingAlias` entries (§10 `aliases`). `StateResourceId ↔
  physical class` is a **BIJECTION** (V-MAT-BIJECTION) so
  `ResourceForView(v) = authority(EquivalenceSetId(v))` is total and
  single-valued — the property the `TABLE*`→`StateResourceId` retype depends on.
  §17 V-MAT-AUTHORITY stays satisfied: every stateful collection resolves to
  exactly one authoritative resource (its own or its alias's).

### CLAIM (c) — co-recursion soundness — errored, SUBSUMED
The refuter hit a StructuredOutput retry cap. Its three probes (double-counting a
shared class; one-representative-schema for a shared residue; multi-writer
collections splitting classes) are all resolved by the claim-d bijection fold:
one resource per class (no double-count), one representative schema per shared
store (advisory label, physical schema shared), and the belt catches any
writer/class divergence. transitive_closure (aliases) + corecursion_1 (shared
residue) are the goldened witnesses.

## What landed (CP1, committed)

- `lib/DataFlow/Materialization.{h,cpp}`: `StateResourceId` /
  `InternalResidualCollectionId` typed ids; `SupportPolicy`; `StateResource` +
  `ForwardingAlias` + `MaterializationResources`; `DeriveStatefulClasses` (pure
  R1-R9 replay, no ControlFlow dep); `PlanResources` (per-class bijection +
  aliases); `ValidateMaterialization` (V-MAT-BIJECTION + V-MAT-AUTHORITY);
  `CrossCheckMaterialization` (reads the STORED plan).
- Stored by-value on `QueryImpl::materialization`, built at the `Query::Build`
  tail after `BuildFlatInstanceFlow`.
- Cross-check belt at the `Program::Build` tail after `FillDataModel`
  (Build.cpp) — the real table-backed `EquivalenceSetId` set vs the stored plan.
- `-materialization-out` dump (`QueryMaterialization` tag), Main.cpp wiring.
- Belt-verified LIVE (corrupt R4 → fires; revert → quiescent).

## Predict-then-verify (byte-matched)

| case | resources | shape |
|---|---|---|
| join_1 | 6 (2 collection + 4 internal residue) | join-input residues, all monotone |
| transitive_closure | 2 + 1 alias | co-recursion shared store (`reaching_to → sr#0`) |
| corecursion_1 | 2 (1 collection + 1 residue) | shared ping/pong residue |
| negate_1 | 3 | mixed differential/monotone |
| tc_nonlinear_diff | 4 (all differential) | the §2.5.2 shape `internal#2 (X,From,To) differential` |

## Line of sight to Phase C (explicit, grounded)

The bijection makes `ResourceForView(v) = authority(EquivalenceSetId(v))` the
well-defined map the Phase-C retype consumes. The first Phase-C step (seed §2)
retypes ONE Rel struct family off `TABLE*` onto `StateResourceId` behind this
map, cross-checked. Rel has ~44 `TABLE*` fields (all base-table/resource
identities; grounding claim-d evidence) and NO arrangement identity — so
resources-first is exactly the base-table half the retype needs; arrangements
stay deferred to when a Rel site needs a seek (Phase D). The TRUE blocker named
in s33 (allocation interleaved into `lib/ControlFlow/Build/*`, must move AFTER a
Rel authority decides) is unchanged and is the megaproject after the first
retype step.

## Open follow-ons (not blockers)
- Arrangement derivation (§2.5.3) — the `collect_arrangement_requirements`
  question, deferred until a consumer exists.
- A discriminating multi-non-mergeable-INSERT witness (still not in corpus) would
  exercise a collection with writers potentially spanning classes; the belt
  guards it regardless.
