# Stage-B seed — current architecture + the Stage-B path as diffs (session-3 close, 2026-08-03)

Purpose: ground the NEXT session's Stage-B slice in a whole-program view at
the tip (423b8139: Stage A + I0 + the F29 fix all COMMITTED). This is a SEED
(orchestrator, session-3 close, single pass) — the next session MUST
fleet-re-verify every anchor before building; see `next-session-prompt.md`.

ONE-AUTHORITY RULE: the deep pseudocode lives in `regional-arch-pseudocode.md`
(§1–§7 + the fleet-verified Part R); the formalized epoch diffs live in
`region-model-diffs.md`; the predicted IR bytes live in
`region-model-desired-states.md`. This seed does NOT duplicate them — Part 1
below is the COMPACT Stage-B-grain view with pointers, Part 2 is the
Stage-B path expressed as DIFFS on it, centered on the SESSION-3 DELTA that
`stage-b-diff.md` (authored at f0c913e0, pre-Stage-A) does not know about.

---

## Part 1 — the pipeline at the Stage-B grain, as pseudocode (verified at 423b8139)

### 1.1 The whole pipeline (bin/drlojekyll/Main.cpp)

```
CompileModule(module):
  query := Query::Build(module, log, policy, demand, demand_retract)   # Main.cpp:71
    # ... parse-side demand fabrication + dataflow build + Optimize ...
    # THE TAIL (lib/DataFlow/Build.cpp:2629-2650, verified this session):
    FinalizeDepths(); FinalizeColumnIDs()
    TrackDifferentialUpdates(log, true); TrackConstAfterInit()
    BuildEquivalenceSets(impl)            # identity-erasing model sharing
    impl->Stratify(log)                   # Tarjan; view->stratum; V-SCC-SEAM
    impl->row_contracts :=                # STAGE A (landed 1d28a74c): pure,
        InferConservativeRowContracts(impl)   # post-Stratify, flat-key
    ValidateRowContracts(impl, log)       # V-CONTRACT-CENSUS / -MEMBERKEY- /
                                          # -AGG-INPUT-KEY always-on
    return Query(impl)
  emit -dot-out / -df-out / -contract-out                     # Main.cpp:120-141
  SetRelDumpStream(-rel-out)              # BEFORE Program::Build (Main.cpp:76)
  program := Program::Build(query, log, first_id, policy)     # Main.cpp:83
    # nested pre-pass fences -> BuildDataModel/FillDataModel -> DR-IR
    # (BuildDRInventory -> DeriveDRStrata -> Linearize+validators -> Lower*)
    # -> eager web -> ProgramImpl::Optimize
  emit -ir-out / C++ codegen
```

Part R pointers: R.1.1 (demand transform), R.1.2 (keyed-instance lowering),
R.1.3 (fixpoint lowering), R.1.4 (Stage-A contracts), R.1.5 (forcer/injector),
R.1.6 (commit/Seal epoch), R.1.7 (eager-web reachability authority).

### 1.2 What exists TODAY that Stage B's planning tier consumes

```
QueryImpl (post-Build, frozen-in-practice):
  views + column edges                    # the one dependency currency
  equivalence sets (models)               # physical-table sharing, identity-erasing
  view->stratum                           # SCC condensation
  row_contracts : view -> {visible_fields, member_key}   # Stage A; AllFields on cycles
  guard_annotations                       # demand-guard satellite (OWN-3 fold rules)
  RecognizedSubgraphs()                   # the nested-lowering satellite records
  DemandForcings()                        # BindingPattern-keyed forcing registry
  projection_role                         # in TUPLE structural identity (Equals-only)
```

There is NO planning object: Rel/ControlFlow read QueryImpl directly, the
"regional program" exists only as these scattered satellites + the walk
structure. THAT is the Stage-B thesis: one `PlanningRegionalProgram` built at
the Query::Build tail, FROZEN, with everything downstream consuming the
frozen object — a THIN wrapper at Stage B (one region, zero child calls),
byte-preserving by construction.

### 1.3 The referee stack now standing (session-3 addition; Stage B's exit gate)

```
per corpus case:  4 opt modes x byte golden        (190 cases)
  + .batches:     oracle + monotone goldens        (63 cases)
  + .batches:     run_refinterp                    (63 cases; I0 LANDED)
      I0 CBF == goldens/<c>.behavioral.stdout == behavioral binary (all 4 modes)
      I0 = definitional eval over RAW PARSED CLAUSES (no Query::Build!) ->
      the Stage-B builder-tail refactor is refereed INDEPENDENTLY (E2 is live)
  + .eqgate:      flat==nested live gate           (5 cases)
  + .irgold:      .df/.rel/.contract pinned        (structural surfaces)
```

---

## Part 2 — the Stage-B path as DIFFS (the session-3 delta on stage-b-diff.md)

`stage-b-diff.md` (H1–H9) remains the hunk-grain plan. The following DELTAS
supersede or amend it; the next session's step (b) re-expresses the hunks
against Part 1 above with these folded in:

### DELTA-1 (H1, the freeze point — ANCHOR MOVED, semantics enriched)
H1 said the tail is `BuildEquivalenceSets -> Stratify -> return`. It is now
`... -> Stratify -> row_contracts -> ValidateRowContracts -> return`.
```
  impl->row_contracts := InferConservativeRowContracts(impl)
  ValidateRowContracts(impl, log)
+ planning := BuildPlanningRegionalProgram(impl)   # consumes contracts,
+     # models, strata, guard annotations, RecognizedSubgraphs, forcings —
+     # ALL the 1.2 inputs; ONE ProgramRoot + ONE observation-root template
+ frozen := Freeze(planning)                       # Stage B: no extraction
+ return Query(impl, frozen)                       # or H1-ALT (owner-gated):
+     # a main-level step between Query::Build and Program::Build
```
The freeze MUST come after the contract pass (contracts are planning input —
the hoist-vs-nest DeterminedBy reading, the DIFF-R5 logical keys). H1-ALT's
ripple analysis needs re-doing: Query's ctor/callers changed at Stage A.

### DELTA-2 (the exit gate — WIDER and STRONGER)
"180 goldens byte-identical" -> **190 bespoke + 63 oracle + 63 monotone +
59 behavioral + 5 eqgate + the .irgold pins, ALL byte-identical**, suite run
WITH run_refinterp live. The I0 referee makes the pure-refactor claim
independently checkable for the first time (I0 shares nothing with
Query::Build). Any behavioral-golden divergence during Stage B is a hard
stop (the goldens are FROZEN through the Stage-C cutover — stage-i0 §8).

### DELTA-3 (H6/H7, the -region-out dump — GRAMMAR NOW RATIFIED + PREDICTED)
H7's three grammar candidates were resolved: G1 ratified (D2.2), and
`region-model-desired-states.md` §2 carries PREDICTED G1 BYTES for
demand_tc_witness + tc_nonlinear_diff (with the determinism ledger: emission
sort key = ascending forcing_index; request-edges block byte-sorted until
permcheck learns it). §3 carries the DOT twin (cluster_region_<id>, owner
DOT directive; advisory, never goldened). Stage B's dump work = realize G1
for the ONE-region program + pin the witness set; the desired-states H1/H2
owner items gate only the REQUEST-EDGE rendering (Stage C), not the Stage-B
single-region block — but read §5/§6 before predicting bytes.

### DELTA-4 (H9, validators — errata-amended + panel-amended)
Errata-5 already re-pointed H9 at stage-c H-I (V-PORT-AGREE/V-PURE-REGION by
name). NEW since: `region-model-diffs.md` panel records carry normative
amendments naming Stage-B homes (esp. DIFF-R1's V-* family birth order and
DIFF-R5's key-canonicalization checks — the logical-key set canonicalization
for CSE/equivalence lands Stage B; physical order stays D5-inert). Apply the
amendment rule: no Stage-B implementation before its amendments are applied
to stage-b-diff.md as dated diffs.

### DELTA-5 (NEW inputs recorded since H-authoring, owner-adjudication-record.md)
- LOGICAL-ORIGIN PROVENANCE on model tables (candidate home: Stage-B planning
  table nodes) — direction, not yet ratified as a stage item.
- The demand-areas-vs-SCCs framing (regions admissibility-drawn, one box per
  area annotated key-invariant, stratum clusters INNER) — binds the dump/DOT.
- Role-inheritance-through-proxies — OPEN owner item (D2.9 default rule).
- The adornment-fuzzing direction + bracket parser obligations — DIFF-R3
  territory, but Stage B should not FORECLOSE the placement-enumeration
  harness (keep the dump grammar generator-friendly).
- DIFF-R5's Stage-B hook: `Minimize`/FieldExpression classes (real minimal
  keys) are DEFERRED-TO-B by D3.4 — decide in-session whether they enter
  this slice or stay deferred (owner call; the flat-key AllFields floor is
  the current contract).

### DELTA-6 (panel-yield lesson, method)
Session-3's critique panels survived ~95% of findings vs Stage A's ~22% —
the refuter charge was too permissive. Next session: refuters must produce
CONCRETE code evidence per confirmation, re-triage severities, and a
finding without a failure scenario is not a finding.

---

## Part 3 — what the next session verifies before building

1. Fleet re-verify: every stage-b-diff.md anchor (authored at f0c913e0) +
   this seed's Part 1 against the then-tip; Part R governs on conflict.
2. The H1-ALT placement decision needs the owner (return-type ripple vs
   main-level step) — a STOP if it becomes load-bearing.
3. The desired-states H1 (key-in-output convention) + H2 (kRequestEdgeAdd
   vs H-G.3 mono-arm-collapse) owner adjudications — they gate Stage-C
   surfaces but the G1 census field-order choice should not prejudge them.
4. Confirm the Stage-B slice boundary: pure refactor + G1 dump + scaffold
   validators (H9 as amended); NO extraction, NO request-edge mint (that is
   Stage C, paused on D2.6 + the §6-vs-§11 routing rule).
