<!-- Copyright 2026, Peter Goodman. All rights reserved. -->
# Session-36 grounding — the ARRANGEMENT derivation (Stage A census + Stage B cross-check)

Status: DRAFT for the refuter panel. Tip `46fa4b32` (branch `keyed-instances`).

## §1 What is true at tip (orchestrator-verified in code + live compiles)

### §1.1 The index universe

- `TABLEINDEX` (`DataIndexImpl`, `lib/ControlFlow/Program.h:91-108`) lives on
  `DataTableImpl::indices` (`:137`), keyed by `column_spec` (a string encoding of
  the sorted-unique column-ordinal set, `Data.cpp:357`). `GetOrCreateIndex`
  (`Data.cpp:348`) SortAndUniques the requested ordinals and dedups by spec —
  the per-table arrangement set is canonical-by-construction.
- SIX live mint sites (the `#if 0` block at `Build.cpp:1502-1515` is dead):
  | Site | Column set | When |
  |---|---|---|
  | `Data.cpp:205` | ALL columns (the default full index every table gets) | table creation (FillDataModel) |
  | `Build.h:443` (`BuildMaybeScanPartial`) | the strict-subset bound set (all-bound early-returns at `:414`) | interior partial scans, region build |
  | `Join.cpp:272` | a pivot-JOIN side's pivot column ordinals | join lowering |
  | `Join.cpp:408` | same, second arm | join lowering |
  | `Build.cpp:432` | a bound `#query`'s bound-parameter ordinals (unless plan==kFullScanFilter withholds) | query entry points, post-BuildEntryProcedure |
  | `Build.cpp:490/526` | bound subset / ALL columns on the EMPTY-QUERY table | `BuildEmptyQueryEntryPoint`, post-BuildEntryProcedure |
- NOTHING mints or deletes a `TABLEINDEX` outside these sites: zero hits in
  `lib/CodeGen`, `lib/Rel`, `lib/Regional`; `ProgramImpl::Optimize`
  (`lib/ControlFlow/Optimize.cpp`) never mentions `indices`. The index universe
  is FINAL at the `Program::Build` tail.

### §1.2 The census inputs survive to the tail

- `context.dr_flow` (`Build.h:205`, a `shared_ptr<DRFlowGraph>`) is ALWAYS
  stashed at `Stratum.cpp:2290` ("before any early return"), and `context`
  (minted `Build.cpp:1354`) is in scope through `Program::Build`'s return at
  `:1516`. So the s35 `table_to_resource` map is READABLE at the tail — the
  census can literally consume it (the map's first real consumer).
- The `-materialization-out` dump drains AFTER `Program::Build`
  (`Main.cpp:184` vs `:112`), so arrangements recorded onto
  `query.impl->materialization` at the `Program::Build` tail ARE rendered.
- Attribution totality has ONE hole, verified in code: the empty-query table
  (`Build.cpp:519`, minted AFTER `BuildDRInventory`, NO member views) is outside
  `table_to_resource` and outside the resources plan entirely (no INSERT ⇒ no
  collection ⇒ no class). It carries a full-column index (`:526`) and possibly
  bound-subset indexes (`:490`). Every OTHER table is total in the map
  (V-REL-RESOURCE proved it live at s35).

### §1.3 The five witnesses, hand-derived (predict-then-verify targets)

Live opt-mode compiles at tip; table→sr# correlated through the `.rel` dump's
`resource=` tokens (ONE id space with `-materialization-out`). Canonical order:
ascending `StateResourceId`, then ascending lexicographic column-ordinal vector.

```
join_1               (resources=6): arrangements=8
  ar#0 sr#0 (0) | ar#1 sr#1 (0) | ar#2 sr#2 (0) | ar#3 sr#2 (0,1)
  ar#4 sr#3 (0) | ar#5 sr#4 (0) | ar#6 sr#4 (0,1) | ar#7 sr#5 (0)
transitive_closure   (resources=2): arrangements=4
  ar#0 sr#0 (0) | ar#1 sr#0 (0,1) | ar#2 sr#0 (1) | ar#3 sr#1 (0)
corecursion_1        (resources=2): arrangements=4
  ar#0 sr#0 (0,1) | ar#1 sr#0 (1) | ar#2 sr#1 (0) | ar#3 sr#1 (0,1)
negate_1             (resources=3): arrangements=4
  ar#0 sr#0 (0,1) | ar#1 sr#1 (0) | ar#2 sr#1 (0,1) | ar#3 sr#2 (0)
tc_nonlinear_diff    (resources=4): arrangements=6
  ar#0 sr#0 (0) | ar#1 sr#0 (0,1) | ar#2 sr#0 (1)
  ar#3 sr#1 (0,1) | ar#4 sr#2 (0,1) | ar#5 sr#3 (0,1,2)
```

All five witnesses' tables are fully resource-attributed (no empty-query table
in any of them), and every witness carries at least one PARTIAL (access-path)
arrangement beside the full defaults — the census is non-vacuous corpus-wide.

## §2 The design (A+B BUNDLED as derive-early / cross-check-late — the s34 shape)

The recon (§3) showed every live site is pure-derivable, which unlocks the
ARCHITECTURALLY RIGHT shape: the arrangement plan is DERIVED at the
`Query::Build` tail (the stored authority — exactly what Stage C's allocation
inversion must consume BEFORE region build), and the `Program::Build`-tail
census of the real `indices` is the CROSS-CHECK. This mirrors s34 resources
verbatim (PlanResources early, CrossCheckMaterialization late) and collapses
"Stage A vs Stage B" into one coherent slice with real teeth.

### §2.1 Types (Materialization.h — the DataFlow-side plan owns arrangements)

```
struct ArrangementId { uint32_t v; ==, <=> };       // Identity.h idiom
struct Arrangement {
  ArrangementId id;
  StateResourceId resource;       // the owning table's resource
  std::vector<unsigned> columns;  // canonical sorted column ordinals
};
struct MaterializationResources { ...; std::vector<Arrangement> arrangements; };
```

Empty-query interface tables are NOT Arrangements (no resource; interface
artifacts of dead queries, invisible to the plan/Rel/`-rel-out` — verified
§3-E4). The census tallies them separately (assert: member-view-free), so no
index is silently dropped — the no-silent-caps rule.

### §2.2 The pure derivation (`PlanArrangements`, Query::Build tail)

Pure `QueryView`-API (the `DeriveStatefulClasses` precedent, NO lib/ControlFlow
dep), run after `PlanResources`, storing dense-id arrangements sorted by
`(StateResourceId, columns)` lexicographic. The site-replay rules (from §3):

- **R-FULL** (Data.cpp:205): every stateful class ⇒ `{0..arity-1}` (the default
  full index every table gets at creation).
- **R-JOIN-UNIFORM** (Join.cpp:408 BuildJoin eager+delta, memoized, AND
  Stratum.cpp:1033 EmitJoinFire → Build.h:443 — panel-(c) fold, §4): for EVERY
  pivot-JOIN view, per joined side, the side's pivot input-column ordinals on
  the side's class. The BuildJoin/EmitJoinFire routing split is extensionally
  invisible in the arrangement SET (fully-interior ⇒ k≥2 ⇒ fire scans all
  sides; partially-interior ⇒ an eager/delta BuildJoin always accompanies and
  mints all sides) — verified on 16 corpus + 4 constructed programs.
  (Join.cpp:272 is DEAD — `BuildNestedLoopJoin`, unreachable
  `else if (true || ...)`.) Assert one-input-pivot-per-(side,pivot).
- **R-NEG** (Stratum.cpp:1215 LowerCrossoverArm → Build.h:443): per non-@never
  NEGATE's crossover: the negate's non-constant key input-column ordinals on the
  predecessor view's class — dropped when the set covers ALL pred columns (the
  Build.h:414 all-bound early return) or is empty.
- **R-QUERY** (Build.cpp:432): per bound `#query` unique binding pattern over a
  surviving INSERT: the bound-parameter ordinals on the insert's class (empty
  bound set = kFullScanFilter withholds ⇒ no requirement).
- **R-PRODUCT** (Stratum.cpp:1333): @product non-driving sides scan FULL —
  no requirement (constant empty set).

DEDUP-INVISIBILITY (load-bearing simplification, verified in recon): unit-side
skips (eager BuildJoin skips unit sides; delta includes them) and all-bound
query patterns are INVISIBLE in the arrangement SET — a unit relation has 1
column so its pivot set == full set; all-bound == full set; `GetOrCreateIndex`
dedups by `column_spec`. The derivation needs NO differentialness predicate at
all — sidestepping the s35 §4 view-set-equality blocker entirely.

### §2.3 The census cross-check (`Program::Build` tail — the teeth)

At the tail (post-both-Optimize; the index universe is FINAL there — nothing
mints/deletes TABLEINDEX after region build, §3-E4c): walk
`impl->tables[*]->indices`, resolve each table through
`context.dr_flow->table_to_resource` (the s35 map's first real consumer;
verified alive at the tail — always stashed at Stratum.cpp:2290 before any
early return), decode each index's ordinals; a table absent from the map must
be member-view-free (assert; empty-query interface residue, tallied). Pass the
`(resource, columns)` multiset to a new `Query.h` friend
`CrossCheckArrangements(query, real_pairs)` (the `CrossCheckMaterialization`
twin): STORED-derived == real, fprintf+abort naming every divergent arrangement
on either side. Falsifiable claim: the emission walk's index requests are a
pure function of the final graph. The suite exercises it 227 cases × 4 modes
(mode-faithful: nodf/none derive over their own graphs — index universe is
dataflow-opt-dependent, controlflow-opt-independent, §3-E4b).

### §2.4 The render (Format.cpp — reads the stored DERIVED plan)

```
materialization  resources=N aliases=M arrangements=K

arrangements
  ar#0 resource=sr#0 columns=(0)
  ...
```

Columns render as plain ordinals — positionally precise against the resource's
`schema=` tuple; a names variant would have to trust cross-view positional
naming (the table's column order comes from its FIRST registered member view,
the resource's schema from its REPRESENTATIVE — not proven the same view;
ordinals never lie). The `ar#`/`sr#` id spaces cross-reference `-rel-out`'s
`resource=sr#K` tokens (ONE id space).

### §2.5 Goldens + gate

Only the 5 `.materialization.opt` goldens move; the moved bytes are EXACTLY
§1.3 (predict-then-verify). Codegen + `.rel`/`.contract`/`.region`/`.df`
byte-identical (observer invariant — nothing consumes arrangements for
emission). OptDiff SUITE PASS + ctest 5/5. Belt live-verify: corrupt the
derivation (drop a site rule) → the cross-check fires naming the missing
arrangement; revert → quiescent. The E4 empty-query probe live-verifies the
interface-residue arm (no corpus case reaches it — 0/181 in the sweep).

## §3 Recon findings (workflow fan-out, 4 sonnet extractors — full text in the
run journal `wf_df667996-b66`)

- **E1 (BuildMaybeScanPartial):** policy-free about the bound set; all-bound
  early-returns minting nothing; empty set ⇒ full scan, no index. THREE live
  callers, all Stratum.cpp: `:1033` EmitJoinFire (side's pivot columns via
  `NthInputPivotSet`+`Containing` — pure), `:1215` LowerCrossoverArm (negate's
  non-constant key input columns on the pred side — pure), `:1333`
  LowerProductArm (always empty — trivially pure). ALL pure-derivable.
- **E2 (join pivots):** `Join.cpp:272` is in statically-DEAD
  `BuildNestedLoopJoin` (sole caller is the `else` of `else if (true || …)`).
  `:408` is in `BuildJoin`, shared by the eager walk (`for_delta=false`, unit
  sides skipped) and the delta walk (`LowerJoinEmit`, `for_delta=true`, unit
  sides included); memoized by `column_spec` so dual-mints dedup. Column values
  are 100%-pure graph queries (`NthInputPivotSet` per side). NAMED GAP: the
  view-ordinal→table-ordinal identity across a SHARED model is an upstream CSE
  congruence assumption asserted nowhere at the sites (the derivation replays
  the same assumption; the cross-check pins byte-equality, not independent
  soundness). SCC-interior joins never reach BuildJoin (EmitJoinFire instead —
  no GetOrCreateIndex anywhere in Stratum.cpp except via BuildMaybeScanPartial).
- **E3 (empirical universes):** §1.3 CONFIRMED independently, byte-level, all
  five witnesses; zero anomalies; every table resource-paired; sr# order does
  NOT track %table:N order (derive per case, never assume). Partial-index `.ir`
  syntax: positional `_` placeholders at excluded slots.
- **E4 (edges):** empty-query table triggered by ZERO of 181 corpus cases
  (scripted orphan-table sweep); minimal probe built + verified (`probe.dr`, a
  source-less recursive cycle feeding a query — table+index with no writer,
  invisible to .rel/.mat). Mode dependence: opt≡nocf and nodf≡none
  byte-identical index censuses; dataflow-opt changes the universe,
  controlflow-opt NEVER. Mutation scan: single mint site (`indices.Create`
  `Data.cpp:364`), no deletion outside teardown, codegen read-only.

## §4 Refuter panel verdicts (4 opus refuters, run `wf_06050c1b-c51`, all
high-confidence, all empirical with live compiles + constructed probes)

- **(a) totality/soundness: NOT refuted.** All five attack vectors run to
  ground: the empty-query table is the SOLE attribution hole and is provably
  member-view-free (`views.push_back` exists only inside `TABLE::GetOrCreate`);
  aggregates/KV/config-agg mint NO extra indexes (GROUP_UPDATE/StateCellStore
  never touch TABLEINDEX); a live `-demand` compile's universe is fully
  accounted (guard joins = R-JOIN, forced query = R-QUERY); `context.dr_flow`
  can never be null at the tail (unconditional stash; diagnostic exits precede
  impl creation). R-NEG verified EXACT against Stratum.cpp:1168-1185.
- **(b) goldens/observer: NOT refuted.** All five §1.3 blocks independently
  re-derived byte-consistent (incl. the (0)<(0,1)<(1) lexicographic
  discriminator); grep proves exactly 5 goldens carry `arrangements=`; exactly
  5 `.irgold` sidecars pin materialization, all opt-only; run_irgold compares
  only pinned surfaces; `MaterializationResources` is unhashed (sole consumers:
  the dump + Rel.cpp:1518 iterating `.resources` only); the dump drains
  post-Program::Build with no partial-dump failure path. MUST-FOLD: write the
  LITERAL §2.4-syntax golden text per witness before blessing.
- **(c) replay rules: REFUTED AS STATED → mandatory fold adopted.** The
  R-JOIN/R-FIRE routing language was wrong (the real router is `AllSidesSameScc`
  over the DIFFERENTIAL-only `RecursiveSccMap` — monotone inductions excluded,
  so transitive_closure's fully-interior self-join is a BuildJoin case; mixed
  differential joins lower via BOTH authorities; the "only if scanned" clause is
  vacuous in its correct scope since k≥2). THE FIX (verified extensionally
  identical on 16 corpus + 4 constructed adversarial programs): collapse to ONE
  unconditional rule — **per joined side of every pivot-JOIN view: the side's
  pivot input-column ordinals on the side's class** (R-JOIN-UNIFORM). Advisories
  adopted: R-QUERY notes it mirrors `SelectAccessPlan` (kFullScanFilter ⟺ empty
  bound set at tip — cite the mirrored source; derivation cannot call it,
  Regional→DataFlow layering); assert the one-input-pivot-per-(side,pivot)
  singleton (BuildJoin `break`s, fire collects all — divergent only on shapes
  clause-build desugars away; the assert makes both conventions coincide);
  quantification is (unique pattern)×(shared class), mirroring the per-INSERT
  loop.
- **(d) architecture/realness: NOT refuted.** Per-resource keying is LOSSLESS
  (table↔resource injective by construction; multiset==set); the cross-check is
  genuinely falsifiable (pure derivation vs stateful emission walk are disjoint
  code paths; catches emission drift, new sites, rule misunderstandings); the
  ONE named blind spot stays named: the E2 view-ordinal→table-ordinal CSE
  congruence is REPLAYED by both sides, so the belt certifies byte-equality,
  not independent soundness (the congruence-unchecked union path is the
  INSERT↔guard-TUPLE union, DataFlow Build.cpp:2412-2428). MUST-FOLD (adopted,
  §5): name the Stage-C residual lacks.

## §4.5 Execution record (post-panel, gate GREEN)

Landed as CP1 (`53cab01e`) with every panel must-fold applied:
- **R-JOIN-UNIFORM** implemented as the ONE unconditional per-side rule (the
  refuted routing split never entered code), with the
  one-input-pivot-per-(side,pivot) fprintf+abort tripwire and the
  R-QUERY-mirrors-`SelectAccessPlan` citation in the header comment.
- **Typed domains** (owner mid-session direction): `ArrangementId`,
  `ColumnOrdinal` (its own domain: a position in the store's column order —
  not a `FieldId`, not a count), `ArrangementKey` (the id-free canonical
  content whose defaulted ordering IS the canonical arrangement order; also
  the census seam type). No bare `uint32_t`/`unsigned` crosses the seam.
- **Literal golden bytes predicted first** (panel-b must-fold): the five full
  `.materialization.opt` texts written to scratch BEFORE the build; the built
  compiler's dumps byte-matched all five on the FIRST run; blessed via
  `runall.sh --bless` after review (`BLESS: 5 golden(s) updated`, purely
  additive).
- **Gate:** OptDiff **SUITE: PASS (227)** — the cross-check quiescent
  corpus-wide across all 4 modes (belt runs every compile; zero fires);
  ctest **5/5**; codegen + `.rel`/`.contract`/`.region`/`.df` byte-identical.
- **Belt live-verify:** dropping the R-NEG requirement → negate_1 aborts with
  `CROSS-CHECK (arrangements): ControlFlow minted an index the plan MISSED:
  sr#1 columns=(0)`; revert → quiescent (rc=0).
- **Interface arm live-verify:** the E4 probe (dead-query program) compiles
  clean with `interface-tables=1` derived AND censused in agreement.

## §5 Stage-C residual lacks (named per panel (d) — what the inversion STILL
needs after this slice lands)

1. **An interface-table authority**: `BuildEmptyQueryEntryPoint` mints outside
   the plan; pure-derivable from decls (this slice derives + cross-checks the
   COUNT; the per-table index sets are the residual).
2. **An id-assignment contract**: TABLEINDEX ids come from the global
   `impl->next_id` at mint time and codegen embeds them (`idx_<Id()>`), so
   up-front minting renumbers ids program-wide → wholesale (non-answer) golden
   churn at Stage C — must be an explicit decision (CARVE-3 says index ids are
   non-contractual; answer goldens are the net).
3. **A column-order/schema authority**: table column order comes from the
   first-registered member view; the resource schema from the canonical
   representative — not proven the same view; Stage C minting tables from
   resources must pin this and discharge the E2 congruence.
4. **Step 2b** (support vs TableIsDifferential view-set equality) before the
   runtime store shape is decided from the plan at mint.
5. **Honesty**: the inversion re-points the six sites' key computation to
   lookups; it deletes their ALLOCATION authority, not their column logic.
