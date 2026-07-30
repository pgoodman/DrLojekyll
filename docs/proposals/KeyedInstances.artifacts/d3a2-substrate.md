# D3.a.2 — STAGE (a) WHOLE-PROGRAM SUBSTRATE (COMMITTED; ledger §20(AM);
# all ten load-bearing anchors orchestrator-re-verified at code pre-commit;
# §7 at the tail is the BINDING ritual-head ruling)

> **House banner.** Tip **bf0315a0** (branch keyed-instances; docs-only atop the
> D3.a.1 landing 33cabcf1, so every code anchor below is live at the landed
> binaries). Consolidated from four code-derived lane maps (laneA flat-oracle,
> laneB input_table census, laneC epoch catalogue, fleet1 lane2 input-substrate
> — same code bytes, re-used per house precedent) + the §20(AL) consolidation's
> 9 ADV items, under the BINDING context: d3a1-substrate.md §7 (the d2
> CO-ACTIVATION ruling) + §8 (the post-slice-1 state, e1-e8),
> d3a-ruling-brief.md (OQ-INPUT / OQ-MODEL / OQ-DEATH-VS-REBUILD — ruled, not
> re-litigated here), KeyedInstances.md §20(AK)-(AL). Every disputed anchor was
> re-read at code by this consolidator; inter-lane contradictions ruled in the
> XC section (numbering continues d3a1-substrate at XC-5). §6 is FACTS ONLY —
> the a2-trigger-shape ruling is the ritual head's.

THE SLICE IN ONE SENTENCE: lift FENCE (iii) + V-INST-SOLE's input arm so a
`@differential` (deletion-capable) SUMMARIZED INPUT is admitted under
`-demand-instance`, adding the input net-removals rebuild trigger + the
Present-filtered rescan, with flat `-demand` (which already compiles the shape)
as the standing answer+delta oracle — the first programs where P-STORE
(`TableIsDifferential(pub)`, Rel.cpp:1055) and P-DEATH
(`TableIsDifferential(demand)`, Rel.cpp:1140) DIVERGE by design (§7 d2 ruling).

===============================================================================
## §1 THE FLAT ORACLE (laneA folded — the semantics the nested arm must match)
===============================================================================

Case: `tests/OptDiff/cases/demand_diff_input_1.dr` (`#message pt(u64,u64)
@differential`; single-hop `ans(K,V) : pt(K,V)`; `#query getpt(bound Key, free
Val)`). Frozen-A compiler: plain `-demand` COMPILES exit 0 (the oracle);
`-demand -demand-retract` also compiles (both axes compose as symmetric
ordinary differential); `-demand-instance` rejects at fence (iii). All O-facts
below verified in the laneA generated artifacts.

- **O-1 (regime map).** Under plain `-demand`: the demand relation
  (`demand__getpt_bf`) is a MONOTONE `Table`; the `@differential` input `pt`,
  the demanded `ans`, and the answer `getpt` are ALL `DiffTable`s. So a
  differential INPUT forces a DIFFERENTIAL PUB through the
  lib/DataFlow/Differential.cpp closure while the demand stays monotone —
  **exactly the e5 divergence** (P-STORE true, P-DEATH false). Confirms laneB
  B31's coupling conjecture empirically.
- **O-2 (guard web).** Two demand-guarded joins, demand as pivot source of
  both: `d ⋈ pt → ans`, `d ⋈ ans → getpt`. A pt row materializes downstream
  ONLY if its Key matches a standing demand row.
- **O-3 (two-polarity ingest).** `pt_2` takes TWO Vec params (adds, removes);
  `::hyde::rt::NetBatch(adds, removes)` fires AT THE MESSAGE BOUNDARY inside
  `pt_2_detail` (Vec.h:176-218: per-side dedup + adds∩removes annihilation)
  BEFORE any table fold. Two DR ingest folds (`kIngestFold` sign=+ →
  kAddQueue, sign=− → kDeleteQueue); lowered as `AddExplicit`/`SubExplicit`
  with crossing-frontier appends. **This discharges laneC's OB7:** the edge
  channel HAS the same SET netting as the demand channel; `{+x,−x}` in one
  batch is a no-op before the counters ever see it.
- **O-4 (retraction path).** A retracted edge rides OVERDELETE → REDERIVE →
  INSERT: SubExplicit down-crossing → TryClaimDel (C_nr ≤ 0 re-test, F17) →
  NetDeleted frontier → the guard join's DEL arm (`demand.InI && pt.InI &&
  NetDeleted` → `SubDerivation` on ans) → ans's own claim/frontier → same on
  getpt. Each hop is a down-crossing on the next DiffTable; exactly the
  last-support rows die (split C_nr/C_r counters; SubDerivation crossing =
  `kInI && after_C_nr <= 0`, Table.h:348-351).
- **O-5 (demand gating of death).** A retract of a NON-demanded key is
  absorbed at the input table (SubExplicit still fires — the input tracks
  every edge) and NEVER propagates: no matching demand pivot / `demand.InI`
  false ⇒ no SubDerivation downstream. **Over-materialization AND
  over-retraction are both impossible; the nested arm must preserve
  death-only-inside-the-demanded-neighborhood.**
- **O-6 (same-batch ±).** Same-batch add+remove of one row nets at NetBatch
  (no table effect). Cross-batch add-then-remove goes through the ordinary
  up-then-down crossings. Final presence is order-independent across epochs
  (net-state/least-model); only intermediate published deltas vary per
  interleaving.
- **O-7 (the tap surface).** `getpt` is a CURSOR query: all four commit
  sweeps are `publish_target=false`, no log hook fires. The observable is
  the Present-set the cursor drains at query time (`Present(id)` per row).
  A signed published-delta surface exists only when a demanded output is a
  published `@differential` message (the `nbhd_out` tap idiom). **Oracle
  contract for the nested arm: answer-level identity = the Present set for
  query-shaped outputs, PLUS sorted published-delta identity when the
  witness carries a @differential output tap (the D3.a.1 eqgate upgrade) —
  the e6 witness should carry such a tap so both surfaces are refereed.**
- **O-8 (census, plain `-demand`).** `kIngestFold=3` (pt ± + demand +),
  `kSeedFold=6` (per join × side × sign: demand+ / input− / input+ twice),
  `kClaimDrain=6`, `kFrontierFilter=6` (a ± pair per DiffTable: pt, ans,
  getpt), `kCommitSweep=4`, `kJoinEmit=2 form=delta`, `kEagerForward=1`,
  everything else 0 — **no exotic op**: the flat oracle is made entirely of
  landed machinery. The `-` seed arms on the input side are the semantics
  D3.a.2's nested arm must reproduce at the answer level; kInstance* = 0
  flat (those are precisely what the nested lowering adds).
- **O-9 (`-demand-retract` compose).** Adding `-demand-retract` flips the
  demand DiffTable too: census doubles the demand's arms (kSeedFold 6→8,
  kClaimDrain/kFrontierFilter 6→8, kIngestFold 3→4, kEagerForward 1→0), no
  special machinery. **D3.a.2's axis (input) is ORTHOGONAL to D3.a.1's axis
  (demand); both-on = plain symmetric differential.** The nested slice must
  accept BOTH regimes: diff-input × mono-demand (the e5 carrier) and
  diff-input × diff-demand (the full four-band composition of §4).

===============================================================================
## §2 THE INPUT PATH AS LANDED (laneB + lane2 folded; monotone regime)
===============================================================================

End-to-end, each site anchored at tip:

1. **PROVISIONING (eager boundary append).** `Build.cpp:1110-1114`: a
   monotone table with a cut successor gets
   `AppendViewTupleToVector(..., TableDeltaVector(..., kNetAdditions))`,
   gated `table != nullptr && !TableIsDifferential(table) && (any_cut_succ
   || is_monotone_negated)`. The recognized-subgraph guard-JOIN successor is
   a cut successor (`IsCutSuccessorDR`, Build.cpp:1080 — differentiality-
   agnostic), so the walk stops AND provisions the input's kNetAdditions CF
   vector. [NOTE: the in-source cross-references "Build.cpp:999" /
   ":999-1004" (Rel.cpp V-INST-DRAIN comment ~:4507; Procedure.cpp ~:325)
   are STALE-BY-DRIFT — :995-1105 is a commented-out block at tip; see
   XC-6. The SEPARATE append at :980-985 is the differential ADD-QUEUE
   append, a different mechanism.]
2. **THE MODEL (mint + effects).** `ResolvedInstance.input_table` = the BODY
   guard JOIN's `joined[1]` model (Rel.cpp:905ff; jl[0]=demand, jl[1]=input —
   same convention as fence (iii)). `InstantiateEffects(diff, pub, demand,
   input)` (Rel.cpp:781-859, called :1098 with `diff` = PUB differentiality
   only): input legs are ONE `kVecDrain` `{value_table=input,
   vec_role=kNetAddition}` (the `edge_drain`, :796-800, pushed second) + the
   `kFlagRead/kPresent/kSeed` rederive leaf on input (:806-813). The
   `if (diff)` fork touches only pub-side counters/crossings/appends.
3. **STRATUM LIFT.** The instantiate's derived stratum =
   `1 + max(ready_after(demand), ready_after(input))` (Rel.cpp:1173; second
   lift in DeriveDRStrata ~:3266) — already reflects whatever stratum
   produces the input's frontiers.
4. **VALIDATORS.** V-INST-EFFECT (Rel.cpp:4272-4341): counts
   `drains/demand_drains/input_drains`; REJECTS any drain whose role is not
   kNetAddition (:4288-4293); totality hard-codes `drains==2u &&
   demand_drains==1u && input_drains==1u` (:4325); the regime ternary keys
   on `TableIsDifferential(op.table_op_table)` — THE PUB — nowhere on the
   input. V-INST-SOLE (:4336-4341) = fence F-B, §3. V-INST-DRAIN
   (:4514-4548): demand arm regime-split at D3.a.1 (`dr_ok` differential /
   `cf_ok` monotone); INPUT arm still `cf_ok(input, kNetAdditions)` only,
   with the in-source promise ":4542-4544 … D3.a.2 owns the split here."
   CheckInstanceDeathFrontier (:4789-4813) touches ONLY the demand table.
5. **THE REGION.** `lib/ControlFlow/Program.h:1175` `input_frontier` (a2
   drain), `:1176` `removal_frontier` (a0 DEMAND death drain — not input),
   `:1189` `input_table` ("the summarized monotone input"), `:1191`
   `demand_table`. Public accessors include/.../Program.h:861 InputFrontier,
   :866 RemovalFrontier, :869 InputTable, :875 DemandTable, :895/:896
   InputKeyCols/InputRowCols. ClassifyVector's kSubgraphInstance arm
   (Procedure.cpp:196-211): demand_frontier/input_frontier/removal_frontier
   → read set; del/add queues → written set.
6. **THE LOWERING.** `BuildSubgraphInstanceRegions` (Procedure.cpp:~286-457):
   orphan-mint fence for the DIFFERENTIAL demand frontier (:318 abort if not
   pre-minted by the stratum lowering); `input_front = TableDeltaVector(...,
   op->input_table, kNetAdditions)` (:327-331) — a MINT-ON-MISS memoized
   fetch with NO fence (safe today: the eager append pre-minted it);
   V-INST-DIFF-COHERENCE (:297-304, PUB bit only); V-INST-DEATH-COHERENCE
   (:379ff); V-INST-EMITTED enrollment (:449-456).
7. **THE BAND (codegen).** `EmitSubgraphInstance` (Database.cpp:2341-2650):
   `emit_instance_rescan` (:2366-2405) — the ONE mold, two drain sources
   ([D-COLLAPSE]): V-INST-FRESH `WorkingOccupied` abort, `TouchCurrent`,
   then `for s < input.NumRows() { RowAt(s); key-filter; cur.TryAdd }` —
   **NO presence predicate anywhere in the mold** (verified verbatim at
   :2377-2400). Band-(a0) death :2406-2439 (drains the DEMAND net-removals;
   RecycleCurrent). Band-(a1) :~2450-2460 (demand births, `emit_instance_
   rescan(kbinds)` :2457). Band-(a2) :~2480-2523: drains `input_front`
   (net-ADDS only); the diff-arm DEMAND-LIVENESS gate `if (iid !=
   kNoInstance) { dq = demand.Find(key); if (dq != kNoRow && Present(dq) &&
   !TouchedFlag(iid)) rescan }` (:2496-2514; the R-3 / review-[I] nest),
   with the in-source **"D3.a.2 RIDER: a differential input interleaving
   re-derives this quiescence argument" at :2494-2495**. Band-(b) :2525ff
   (drop scan :2571ff, born scan :2590ff, V-INST-PARTITION belt). The gate
   selector `diff` is `region.IsDifferential()` == TableIsDifferential(PUB).
8. **THE RUNTIME.** InstanceStore.h: FindInstance :99 (non-adding),
   FindOrAddInstance :105, TouchCurrent :130, TouchedFlag :149 (epoch-scoped,
   reset at Seal), WorkingOccupied :161-163 (`current->NumRows() > 0`; the
   N-1 caveat is the header's own comment :21-27), RecycleCurrent :216
   (Touch + Reset; UNCONDITIONAL + IDEMPOTENT per :210-215), Seal :174ff
   (swap + Reset retired). Inner per-instance tables are always monotone
   RowStores — input differentiality never touches the store representation
   (OQ-MODEL: predicate-free set island).

===============================================================================
## §3 THE FENCES + EVERY input_table SITE + THE ABORT CHAIN
===============================================================================

### F-A — the nested pre-pass reject (Build.cpp)

Gate `if (demand_instance)` at **Build.cpp:1504** (the whole block ~:1504-1558
runs only under `-demand-instance`). Per forcing group, for each
`GuardAnnotation::kBody` guard JOIN: `in = jl[1]` (:1529); the predicate is
**`in.CanReceiveDeletions()` (:1530)** setting `diff_input`; recursive-content
(:1533-1540, incl. predecessors' induction); `cyclic_demand =
ViewSelfReachable(jl[0])` (:1542-1543). Diagnostic PRIORITY (else-if,
:1546-1556): cyclic > recursive_content > **diff_input (:1553-1555:
"Demanded subgraphs over deletable (differential) inputs are not yet
supported under -demand-instance")**. D3.a.2 deletes ONLY the diff_input arm
+ flag; cyclic and recursive-content arms SURVIVE — the admitted shape is a
differential, acyclic, non-induction-fed input. A both-cyclic-and-diff-input
program keeps rejecting (cyclic wins) — that priority is load-bearing for the
fence-lift diff (no reject-message churn on the surviving fences).

### F-B — V-INST-SOLE's input arm (Rel.cpp:4336-4341)

One conjunction, ONE ValidatorFail string covering TWO forbiddances:
```
if (op.input_table != nullptr &&
    (TableIsDifferential(op.input_table) ||      // half 1: DIFFERENTIAL input  <- lifts
     op.input_table == op.table_op_table)) {     // half 2: input aliases pub   <- SURVIVES
  ValidatorFail("V-INST-SOLE: an instantiate's summarized input is "
                "differential or aliases the published table");
}
```
Lifting half 1 requires SPLITTING/rewording the message so the surviving
pub-alias reject stays truthful (ADV-1). The differential check does not
vanish — it MIGRATES to the V-INST-DRAIN input-arm regime split + the
provisioning fences + the rescan Present conjunct (the demand-side D3.a.1
precedent). Untouched neighbors: `inst_per_pub` (:4392-4396), V-INST-PAIR.

### The input_table site census (what lifts / splits / grows per site)

| # | site | anchor | disposition under diff input |
|---|---|---|---|
| C1 | fence F-A diff_input arm | Build.cpp:1530, :1553-1555 | **LIFT** (arm + flag deleted; siblings survive) |
| C2 | V-INST-SOLE half 1 | Rel.cpp:4336-4341 | **LIFT + message split** (half 2 survives) — ADV-1 |
| C3 | InstantiateEffects input drain | Rel.cpp:796-800 (edge_drain), :1098 call | **GROWS**: second `kVecDrain{input, kNetRemoval}` leg under a NEW `input_diff` selector (the call passes no input differentiality today) |
| C4 | InstantiateEffects rederive leaf | Rel.cpp:806-813 (kPresent/kSeed on input) | UNCHANGED (Present is meaningful on a DiffTable) — but codegen does not honor it (C12) |
| C5 | V-INST-EFFECT counting | Rel.cpp:4272-4341 (:4288 role reject, :4298 ++input_drains, :4325 totality) | **SPLITS on the THIRD AXIS** `TableIsDifferential(op.input_table)`: accept kNetRemoval when value_table==input ∧ input_diff; `input_drains == input_diff?2:1`; `drains` bump — ADV-6 |
| C6 | V-INST-DRAIN input arm | Rel.cpp:4542-4548 | **SPLITS** mirroring the demand arm: `input_diff ? dr_ok(input,kNetAddition,+1) ∧ dr_ok(input,kNetRemoval,−1) : cf_ok(input,kNetAdditions)` (dr_ok :4517-4528 already checks flow.table_vecs + the signed kFrontierFilter producer) |
| C7 | frontier producer | Rel.cpp:2541-2604 (the generic both-sign mint loop; acyclic arm :2597-2603) | **FREE** — a differential non-induction input already gets both-sign mint_claim/mint_filter + commit sweep; zero new producer code (ADV-5) |
| C8 | stratum lift | Rel.cpp:1173; ~:3266 | UNCHANGED mechanism; confirm ready_after reflects the input's frontier-filter band (it does for the demand twin) |
| C9 | region members | Program.h:1175/1189; include Program.h:861/869 | **GROWS**: new `input_removal_frontier` UseRef + accessor, peer of `input_frontier`; ClassifyVector read-set gains it (Procedure.cpp:201-206) |
| C10 | lowering provisioning | Procedure.cpp:327-331 | **SPLITS + FENCES**: differential input's fetches must be fence-guarded pre-minted (the :318 idiom) — the current mint-on-miss fetch would be a SILENT ORPHAN for a diff input (producer-less empty vec ⇒ under-rebuild); optional input-diff coherence stamp (peer of :297-304) so codegen has an authority for its selector |
| C11 | V-INST-DIFF/DEATH-COHERENCE, V-INST-EMITTED, V-INST-PAIR, CheckInstanceDeathFrontier | Procedure.cpp:297-304/:379ff/:449-456; Rel.cpp:4398ff/:4789-4813 | **INVARIANT** — no new store op, no input death (ADV-2); count-per-store multisets unchanged |
| C12 | the rescan mold | Database.cpp:2366-2405 | **GROWS the Present(s) conjunct** guarding RowAt/TryAdd, gated on input_diff (DiffTable::Present at Table.h:421-424; monotone Table::Present :261) — ADV-3 LOUD |
| C13 | band-(a2) drains | Database.cpp:~2480-2523 | **GROWS the removal trigger** (shape = ritual-head ruling, §6); the R-3 gate conjuncts carry unchanged; the :2494-2495 RIDER discharges via §4's e4 lemma |
| C14 | band-(b) + belt | Database.cpp:2525-2650 | UNCHANGED (cur/frz are monotone; counts hold); belt already active for every D3.a.2 program because diff-input ⇒ diff-pub (O-1) |
| C15 | dump render | Format.cpp:668/:739-748/:870-878 (`input=` token) | OPTIONAL: an input-diff marker so a mis-classified input is dump-visible (stage-(b) choice) |
| C16 | GROUP_UPDATE's input_table | Rel.h:711; V-AGG-SOLE :4228-4232 | OUT OF SCOPE (aggregate family; shares only the name + the SOLE mold) |

### THE XC-3-CLASS ABORT CHAIN (fence lifts with NO other change — in order)

1. **ABORT 1 — V-INST-SOLE (Rel.cpp:4336-4341), validator-time.** The mint
   runs, V-INST-EFFECT passes (effect set unchanged), then SOLE's half-1
   fires on `TableIsDifferential(op.input_table)`. So e1 (F-A) is NOT
   independently landable past e2.
2. **ABORT 2 — V-INST-DRAIN input arm (Rel.cpp:4545), validator-time.** With
   F-B's half 1 also lifted: the eager boundary append was SKIPPED
   (`!TableIsDifferential(table)` at Build.cpp:1110) so `cf_ok(input,
   kNetAdditions)` is false — the exact structural twin of D3.a.1's XC-3
   (the differential frontier is a commit-band product minted in the stratum
   lowering AFTER validation; the fix is the C6 regime split, NEVER
   re-enabling the eager append — double-provision hazard, laneB B14).
3. **NO THIRD ABORT — SILENT MISCOMPILE.** With C6 also split (both dr_ok
   pass — the C7 producer is free) and the lowering's memoized fetch
   resolving (the stratum lowering pre-mints the CF vecs, as the demand
   fence at :318 demonstrates), compilation SUCCEEDS and the generated band
   is WRONG twice over: (i) band-(a2) drains only net-ADDS, so a pure
   edge-retract epoch never rebuilds — the flat web decrements its arm, the
   band never decrements its twin, the doubled counter parks at 1 ⇒ STUCK
   PRESENT (laneC OB1); (ii) any rescan that does fire re-materializes dead
   rows via the unfiltered `NumRows()` scan (ADV-3) ⇒ over-materialization.
   Neither aborts a validator or the belt (counts balance); ONLY the
   eqgate/flat-oracle catches it. **This is why the slice's landable unit is
   {e1,e2,C3,C5,C6,C9,C10,C12,C13} together (or a temporary fence for
   intermediate commits) — the d2 §7 XC-3 rider's lesson applies verbatim.**

===============================================================================
## §4 THE EPOCH CATALOGUE (laneC folded — the e4 raw material)
===============================================================================

### Timeline facts (nested witness, `-demand -demand-retract -demand-instance`)

- **T-1.** One entry call = one ingest proc + one flow proc = one epoch (the
  epoch counter increments once at the flow head).
- **T-2/T-3 (DISJOINT CHANNELS — the quiescence backbone).** Four entries;
  each writes exactly ONE channel: `add_edge` writes only the edge table;
  the query/retract injectors write only the demand table. **No entry writes
  both.** An edge epoch reads a FROZEN demand table; a demand epoch reads a
  frozen edge table.
- **T-4.** ALL ingest folds run in the ingest proc BEFORE the flow proc
  (Explicit folds bump counters there).
- **T-5 (THE load-bearing timing fact).** `Present` is COUNTER-based
  (Table.h:421-424) and flips AT THE INGEST FOLD, not at the commit sweep;
  the claim drains in the flow touch FLAGS only (kDel/kAdd), never counts.
  So a table written only in the ingest proc has counters FROZEN through the
  whole flow, and `Present` at band time == `kInI` post-commit (Commit sets
  `kInI := counts>0`, Table.h:571-575). In a demand-retract epoch
  `Present(dq)` is FALSE throughout the flow — this is what kills the
  zombie rebuild at the R-3 gate.
- **T-7 (FORWARD, DiffTable input; grounded on the flat diff-input build).**
  In an edge-retract epoch, at band time: the retracted row's counter is
  already at its net epoch value (SubExplicit ran in the ingest proc), the
  row is still physically at `RowAt(s)` (CompactDead runs only at the
  commit-sweep tail past the 4096 floor — Table.h NeedsCompaction ~:598-608,
  CompactDead :614 — never in suite-sized runs), `InI(s)` is STILL TRUE
  (batch-start frozen bit), `InNew(s)` reflects claim flags. **The one
  predicate equal to post-epoch committed content mid-band is `Present(s)`.**
  Hence a same-epoch rebuild is sound with a Present-filtered rescan — no
  pre-epoch snapshot needed.
- **T-8.** TouchedFlag/Touched() dedup rescans ACROSS all band drain sources
  (append-once Touch; Seal resets) — a0/a1/a2 already share one flag set.
- **T-11/T-12.** Commit sweeps run AFTER the bands; the nested flow is
  byte-identical to flat except the inserted band block, so the flat web ⊕
  band(b) DOUBLE-derive the pub symmetrically (every row's counter 2× the
  flat value; presence and `was!=now` publication identical — the
  observable-neutral doubling the eqgate referees).

### The interleaving table (✅ = D3.a.1 argument carries verbatim; ⚠ LOUD =
### does NOT carry verbatim, re-derivation obligation)

| shape | frontiers | bands | Present==committed? | net |
|---|---|---|---|---|
| E-A edge-adds (LANDED) | input net-adds | a2 | ✅ demand untouched this epoch | +2 symmetric birth |
| E-B demand-add (LANDED) | demand births | a1 | ✅ (a2 idle; a1 needs no gate) | +2 |
| E-C demand-retract/DEATH (LANDED) | demand deaths | a0 → b | ✅ (gate sees FALSE if probed — correct) | −2 symmetric death |
| E-D edge-retract | **input net-REMOVALS (NEW)** | a2 MUST fire | ⚠ **LOUD**: demand side carries (edge epoch keeps demand frozen, T-3) — but the INPUT side needs the NEW twin lemma below; today a2 never fires (net-adds only) ⇒ stuck-present (OB1) | needs −2 |
| E-E same-batch edge add+remove (different rows, one key) | both input frontiers | a2 once (TouchedFlag) | ⚠ LOUD: one Present-filtered rescan reads the NET input — correct ONLY with the presence conjunct + shared TouchedFlag (OB2/OB6) | net |
| E-F1 death (ep1) then edge-retract same key (ep2) | ep2 input removals | ep2 a2 gate CLOSES (Present(dq) FALSE / committed-absent) | ✅ verbatim (the R-3 gate's charter) | 0 |
| E-F2 edge-retract (ep1) then re-demand (ep2) | ep2 demand births | ep2 a1 rebuild from the SHRUNKEN input | ⚠ LOUD: a1's rescan ALSO needs the Present conjunct or rebirth resurrects the retracted edge | rebirth reflects net input |
| E-F3 edge-retract for a DEAD key (iid bound, demand absent) | input removals | a2 gate closes | ✅ verbatim | 0 |
| e5-carrier: any epoch under diff-input × MONO-demand | — | a2 gate emitted (diff=PUB is true) against a MONOTONE demand member | ⚠ LOUD (benign): the gate's `Present(dq)` degenerates to monotone-true; soundness argument shifts from quiescence to IRREVOCABILITY (monotone demand never retracts) — a NEW argument to state, not the D3.a.1 one (XC-9) | correct |
| hypothetical combined demand+input entry | both channels | a0+a2 same epoch | ⚠ LOUD: `Present(dq)` would reflect UNCOMMITTED same-epoch demand writes — the D3.a.1 argument FAILS; no such entry exists today (T-3); e4 must pin a fence or a proof (the :2494-2495 RIDER's exact content) | n/a |

### The quiescence obligations (e4 raw material)

- **OB1 (e3 CORE).** Band-(a2) must be TRIGGERED by the input net-removals
  frontier, not only net-adds — else pure edge-retract never rebuilds and
  the pub sticks present (the §3 chain's silent miscompile (i)).
- **OB2 (ADV-3, LOUD).** BOTH rescan sources (a1 AND a2/a2') must gain the
  `input.Present(s)` conjunct under diff input; the spelling is **Present**
  — see the ADJUDICATION below.
- **OB3.** Preserve the flat-web ⊕ band ±2 symmetry: E-D must decrement BOTH
  arms (flat via the input NetDeleted del arm — free, O-4; band via the
  removal-triggered Present-filtered rescan + drop scan).
- **OB4 (ADV-7).** N-1 DISCHARGE, favorable: under the ruled full-rescan
  model `current` never shrinks mid-epoch (built only by monotone TryAdd;
  emptied only at Seal/Recycle), so `WorkingOccupied = NumRows()>0` STAYS
  EXACT for a DiffTable input. The InstanceStore.h:21-27 signed-count
  contingency can be CLOSED at stage (b) with one comment edit — it reopens
  only if a2 ever becomes an incremental shrink (it must not; OQ-MODEL).
- **OB5/OB6.** The new removal trigger carries the SAME gate set (iid +
  demand Find/Present + !TouchedFlag + V-INST-FRESH) and SHARES the
  TouchedFlag/Touched() set with the add trigger — one rescan per key per
  epoch, reading net state.
- **OB7 — DISCHARGED by O-3:** the edge message already NetBatches; the
  same-epoch death-vs-rebuild impossibility extends to input epochs via
  channel disjointness (a0 vs a2) + per-row netting (within a2).
- **OB8 (NEW — the input-side quiescence lemma, the e4 deliverable).** Pin
  first-class: *the a1/a2 rescan's `input.Present(s)` reads the epoch-net
  committed-equivalent input state because (i) every input-counter write
  precedes the bands (explicit folds in the ingest proc; for a DERIVED
  acyclic input, seed-fold Add/SubDerivations run in flow steps that the
  instantiate's ready_after-lifted stratum orders before the band) and
  (ii) nothing after the bands writes the input before its Commit.* The
  recursive-content fence is a PRECONDITION of (i) — a fixpoint-refired
  input would break "counters final at band time"; the fence surviving F-A's
  lift is therefore load-bearing for correctness, not just scope.

===============================================================================
## §5 THE GAP LEDGER (gap → anchor → discharging design sub-lane b1..b4)
===============================================================================

Proposed stage-(b) lane partition (orchestrator may re-cut): **b1**
fences+validators (DR layer), **b2** model/effects+region+provisioning,
**b3** codegen band + the quiescence/soundness arguments, **b4** witnesses+
tests+census.

| # | gap | anchor | lane |
|---|---|---|---|
| H-1 | F-A diff_input arm lift (e1); demand_diff_input_1 flips diagnostic→compiling (runall.sh list + CLAUDE.md ride) | Build.cpp:1530, :1553-1555 | b1 (+b4 for the case flip) |
| H-2 | F-B half-1 lift + **ADV-1** message split (pub-alias half keeps a truthful string) | Rel.cpp:4336-4341 | b1 |
| H-3 | **ADV-6** V-INST-EFFECT third-axis branch: accept `kNetRemoval{input}` when input_diff; `input_drains==input_diff?2:1`; totality bump; pose the L3-style which-predicate question BEFORE code — the answer is `TableIsDifferential(op.input_table)`, a third axis that must NOT fold into P-STORE/P-DEATH (§7 d2 anti-fold rider extends) | Rel.cpp:4288-4293, :4298, :4325 | b1 |
| H-4 | V-INST-DRAIN input-arm regime split (both-sign dr_ok) | Rel.cpp:4542-4548 | b1 |
| H-5 | **ADV-2** one explicit sentence: the demand-removal death pieces (CheckInstanceDeathFrontier :4789-4813, {sid,kInstanceDeath} enrollment, band-(a0)) have NO input twins — input removals NEVER mint kInstanceDeath (S2/S3; OQ-DEATH-VS-REBUILD) | — | b1 (statement) |
| H-6 | InstantiateEffects removal leg under a new `input_diff` selector (the :1098 call passes none today) | Rel.cpp:796-800, :1098 | b2 |
| H-7 | region `input_removal_frontier` UseRef + accessor + ClassifyVector read-set | Program.h:1175ff; include Program.h:861ff; Procedure.cpp:201-206 | b2 |
| H-8 | provisioning: fence-guarded pre-minted fetches for the diff input's BOTH vecs (the :318 orphan-mint idiom; the current :327-331 mint-on-miss is a silent-orphan hazard under diff input) + optional input-diff coherence stamp (peer of :297-304) as codegen's selector authority | Procedure.cpp:318, :327-331 | b2 |
| H-9 | **ADV-5** (scoping, no code): the producer is FREE — the generic both-sign mint loop covers a differential non-induction input | Rel.cpp:2541-2604 | b2 (cite) |
| H-10 | **ADV-3 (LOUD, MUST-HAVE)** the rescan-mold `Present(s)` conjunct, BOTH sources, gated input_diff; spelling ruled Present (adjudication below) | Database.cpp:2366-2405; Table.h:421-424 | b3 |
| H-11 | the removal trigger (e3/OB1): drain shape per the ritual-head ruling (§6) with the full gate set | Database.cpp:~2480-2523 | b3 |
| H-12 | **ADV-9** Recycle placement: if kept it MUST sit behind !TouchedFlag (unconditional-late-Recycle is the ONE observably-divergent option — §6(ii)) | InstanceStore.h:216; Database.cpp band order | b3 (per ruling) |
| H-13 | e4: the quiescence re-derivation — OB8 lemma + the no-combined-entry fence-or-proof + the e5-carrier irrevocability note; discharges the in-source RIDER | Database.cpp:2494-2495 | b3 (design §) |
| H-14 | **ADV-4** the symmetric-firing argument written first-class: pub now has THREE signed pathways on retract (flat del arm, band drop scan — and the doubling must stay exactly 2×, the eqgate's premise); state why each fires iff the other does | laneC T-12; Database.cpp:2571ff | b3 (argument) + b4 (eqgate referee) |
| H-15 | **ADV-7** N-1 close (OB4): comment-edit the InstanceStore.h contingency as discharged-under-full-rescan | InstanceStore.h:21-27, :160-163 | b3 |
| H-16 | e5 divergence carrier: first P-STORE∧¬P-DEATH program; V-INST-PAIR already allows n_death==0; verify the monotone-demand gate emission (XC-9) end-to-end | Rel.cpp:1055 vs :1140; :4398ff | b4 (witness) + b1 (sanity) |
| H-17 | e6 witnesses: (i) a differential-INPUT eqgate witness with a @differential output tap (answer + sorted-delta identity, per O-7) exercising E-D/E-E/E-F1/E-F2/E-F3; (ii) BOTH regimes covered (diff-input×mono-demand AND ×diff-demand, per O-9); (iii) demand_diff_input_1 disposition (promote vs lifted-fence compile witness) + **ADV-8** its stale `Build.cpp:1344` comment (line 6) fixed when touched | tests/OptDiff | b4 |
| H-18 | e7: G-INPUT-NEG discharged by H-11 (the removals rebuild band, d3a-substrate:721-727); G-STALE discharged by SUBSUMPTION (every content change surfaces in the input's ± frontier carrying its key — no broader revisit protocol; d3a-substrate:728-732, OQ-INPUT "G-STALE subsumed") — record both discharges | d3a-substrate.md §5 | b4 (record) |
| H-19 | e8: perturbation rows for the new arm (L-table idiom; NESTED-arm vehicles; genuinely never-minted roles per the L3 amendment — kProductInput-class, since a diff input now OWNS queue roles) | §8 ritual amendments | b4 |
| H-20 | stale in-source "Build.cpp:999" cross-refs (XC-6) — cosmetic ride-along when e1/H-8 touch those files | Rel.cpp ~:4507; Procedure.cpp ~:325 | b1/b2 rider |

### THE ADV-3 ADJUDICATION (cross-lane, maximum care — the slice's most
### load-bearing design fact): **CONFIRMED, spelling = `Present`.**

All four lanes converge INDEPENDENTLY: laneB B28 (the mold has no filter;
DiffTable::Present exists at Table.h:421), lane2 S4 (same, seed-unread),
laneC T7/OB2 (the mid-epoch analysis: at band time `Present(s)` == post-commit
`kInI` because counters are final at the fold and claim drains touch flags
only; `InI(s)` is DOUBLY wrong — stale-TRUE for this-epoch retractions AND
stale-FALSE for this-epoch adds; `InNew` is claim-flag-derived, extensionally
close for explicit-only tables but dependent on claim-drain ordering), and
laneA O-4/O-7 (the flat oracle's observables are counter-based). This
consolidator re-verified at code: Table.h:421-424 ("Post-commit presence:
counts > 0"), :571-575 (Commit's kInI := counts>0), the flag-only claim
mutations, and the mold's filterless scan at Database.cpp:2377-2400. NOT
unresolved. Two riders stage (b) must carry: (i) the OB8 lemma is the
conjunct's soundness precondition (recursive-content fence load-bearing);
(ii) emitting `Present(s)` on a MONOTONE input also compiles
(Table::Present :261) but the clean shape gates on input_diff (C12 selector
from H-8's stamp).

===============================================================================
## §6 THE RITUAL-HEAD FACT BASE (facts ONLY — the a2-trigger-shape ruling is
##    the orchestrator's; no recommendation is expressed here)
===============================================================================

### (i) Second-drain vs combined ± drain

- **What exists.** DR-side: the mint loop gives the diff input BOTH
  `VecRole::kNetRemoval` and `kNetAddition` table_vecs + both-sign
  kFrontierFilter producers + a commit sweep (free, C7). CF-side: two
  VectorKinds (kNetAdditions / kNetRemovals). **The VecRole enum
  (Rel.h:54-66) has NO combined ± role** — a combined drain would need new
  model surface (a new role or a codegen-level concatenation), while the
  two-drain shape reuses landed roles verbatim (the demand a0 drain is the
  exact one-sign precedent, and DeathEffects' `kNetRemoval` drain Rel.cpp:868
  is the effect-declaration mold for the removal leg).
- **What the netting guarantees.** NetBatch at the message boundary (O-3):
  one ROW never appears in both frontiers in one epoch; the SAME KEY can
  appear in both via different rows (E-E). Same-batch ± of one row produces
  NO frontier row at all (annihilated pre-fold) — matching flat's no-op.
- **What TouchedFlag guarantees.** One flag set, append-once, Seal-reset,
  already shared by a0/a1/a2 (T-8). Whichever arm first touches key K does
  the ONE full Present-filtered rescan (which reads the NET epoch state,
  T-5/T-7); every later arm skips. Therefore **drain order between the add
  arm and the removal arm is behavior-neutral**, and second-drain vs
  combined-drain compute IDENTICAL `cur` — they differ ONLY in generated-code
  shape (two gated loops vs one merged loop) and in model surface (none vs a
  new role). PROVIDED both arms carry the identical gate set (iid + demand
  Find/Present + !TouchedFlag + V-INST-FRESH inside the mold).
- **Effect/validator shape per option.** Two-drain: InstantiateEffects grows
  one `kVecDrain{input, kNetRemoval}`; V-INST-EFFECT totality
  `input_drains==2`. Combined: a single drain of a new role; V-INST-EFFECT
  would need the new role admitted instead — MORE new surface, not less.
  (Fact: the demand side at D3.a.1 chose the per-role shape.)

### (ii) Recycle-vs-rescan ordering vs the death band

- **Current is provably EMPTY at first touch** in every band arm:
  V-INST-FRESH (`WorkingOccupied` abort, Database.cpp:2368-2374) polices it,
  and Seal/RecycleCurrent are the only emptiers (InstanceStore.h:174ff/:216).
  So a gated `RecycleCurrent` at band position is a NO-OP (Reset-on-empty;
  Touch is append-once) — e3's "RecycleCurrent + full rescan" spelling and
  "pure Touch+rescan" are byte-different, behavior-identical AT FIRST TOUCH.
- **THE ONE OBSERVABLY-DIVERGENT OPTION (LOUD).** An UNCONDITIONAL Recycle
  (outside the !TouchedFlag gate) in the removal arm, ordered after any
  same-epoch rescan of the same key: interleaving **E-E** distinguishes it —
  batch {add (K,x), remove (K,y)}, K live-demanded with standing frozen
  rows; the add arm rescans K (TouchedFlag set, cur = net content); the
  unconditional Recycle then WIPES cur; the dedup skips the re-rescan;
  band-(b) sees cur=∅ ⇒ drops K's ENTIRE frozen set and births nothing.
  **V-INST-PARTITION does NOT abort** (born 0 + carried 0 == cur 0; dropped
  == frz — the counts balance) and V-INST-FRESH never fires (Recycle doesn't
  rescan): a SILENT wrong answer diverging from the flat oracle's net delta.
  Only the eqgate catches it. Every gated option (Recycle-behind-
  !TouchedFlag, or no Recycle) is observation-equivalent; only this
  ungated-late shape is not.
- **V-INST-FRESH exposure per option.** No-Recycle: the belt keeps policing
  empty-at-entry for the new arm exactly as for a1/a2 — unchanged teeth.
  Gated-Recycle-then-mold: the mold's WorkingOccupied check still runs after
  a no-op Reset — teeth unchanged, but the belt now sits behind dead code
  (it can no longer catch a hypothetical non-empty current in THIS arm,
  because Recycle just emptied it — a strict weakening of the belt's reach
  in the removal arm). Fact for the ruling: keeping Recycle trades
  death-arm-mirroring symmetry for one belt's blind spot; dropping it keeps
  the belt's reach and lets V-INST-FRESH own the invariant.
- **Ordering vs the death band (a0).** Channel disjointness (T-3) means a0
  (demand deaths) and the input arms can NEVER co-fire for one key in one
  epoch under the current entry-point structure — the a0-first band order is
  behavior-neutral today. a0's RecycleCurrent also sets TouchedFlag, so even
  under a hypothetical combined entry the OD-15 suppression (death wins,
  rescans skip) carries — but that entry is exactly the ⚠ LOUD cell whose
  Present argument fails (§4); the fact base ends there.
- **The dead-key removal (E-F3) needs NO Recycle under any option:** the
  gate closes before any touch; the instance's frozen content was already
  retracted at its death epoch (E-C), so there is nothing to drop — both
  options identically no-op.

===============================================================================
## XC ADJUDICATIONS (ruled at code by this consolidator; numbering continues
## the d3a1 substrate)
===============================================================================

- **XC-5 — the a2-gate/RIDER anchor: laneB RIGHT, fleet1-lane2 WRONG.** The
  D3.a.1 demand-liveness gate comment block opens at Database.cpp:~2487; the
  sentence "D3.a.2 RIDER: a differential input interleaving re-derives this
  quiescence argument" is at **Database.cpp:2494-2495** (re-read verbatim);
  the Present/TouchedFlag emit at :2505, the rescan call at :2514. Lane2's
  ":2472-2478" cites the wrong lines for the same (correctly quoted) text —
  immaterial to substance, corrected here for the H-13 anchor.
- **XC-6 — the eager boundary append anchor: laneB RIGHT; the IN-SOURCE
  cross-references are stale.** The live monotone append is
  **Build.cpp:1110-1114** (gate `!TableIsDifferential(table)` at :1110);
  Build.cpp:995-1105 is a commented-out block at tip, so the in-source
  citations "Build.cpp:999" (Rel.cpp V-INST-DRAIN comment ~:4507) and
  "Build.cpp:999-1004" (Procedure.cpp ~:325) are STALE-BY-DRIFT (comment
  text only; behavior claims unaffected). H-20 rides the fix. Do not confuse
  the SEPARATE differential add-queue append at :980-985 (gated
  `TableIsDifferential && !InductionGroupId`).
- **XC-7 — CompactDead anchors: both lanes approximate, substance one.**
  NeedsCompaction with the 4096 floor at Table.h:~598-608; CompactDead at
  :614. LaneB's ":594/619" and laneC's ":596-603" both point inside the
  block; the shared claim (dead rows persist at RowAt until the
  epoch-boundary sweep, suite never fires the floor) is EXACT.
- **XC-8 — F-A block extent (carried from the (AL) consolidation):** gate
  `if (demand_instance)` at Build.cpp:1504 (lane2's ":1502"/":1513" slips),
  predicate :1530, diff_input arm :1553-1555 — re-confirmed at bf0315a0.
- **XC-9 — the two lanes' ORACLE REGIMES are complementary, not
  contradictory (substantive).** LaneA grounds D3.a.2 on plain `-demand`
  (diff input × MONOTONE demand — the e5 divergence carrier); laneC's
  forward analysis is grounded on `-demand -demand-retract` + diff edge
  (both axes). RULED: the slice must serve BOTH; O-9 shows they compose flat
  as plain symmetric differential. NEW FACT surfaced by the reconciliation:
  the a2 gate's emit selector is PUB-keyed (`diff` =
  `region.IsDifferential()`), so the e5-carrier program emits the R-3 gate
  probing a MONOTONE demand member — `Find` + monotone `Table::Present`
  (:261) — whose soundness argument is IRREVOCABILITY (monotone demand
  never retracts), not the D3.a.1 quiescence argument. Correct, but a NEW
  sentence e4 must state (§4 LOUD cell; H-13/H-16).
- **XC-10 — lane2's "re-rescan into a recycled current (shrink)" vs ADV-9's
  "Recycle is dead at band position": NOT a contradiction.** Lane2's
  phrasing predates the consolidation; at first touch current is empty
  (V-INST-FRESH), so "recycled current" == "fresh empty current". The §6(ii)
  fact base carries both spellings' behavior-equivalence under gating and
  the ONE divergent ungated shape. Ruling remains the ritual head's.

===============================================================================
## THE TEN MOST LOAD-BEARING ANCHORS (orchestrator: re-verify personally)
===============================================================================

1. **Build.cpp:1504 / :1529-1530 / :1553-1555** — the demand_instance gate;
   `in = jl[1]`; `in.CanReceiveDeletions()`; the diff_input diagnostic arm
   (else-if priority cyclic > recursive_content > diff_input). [e1/H-1]
2. **Rel.cpp:4336-4341** — V-INST-SOLE's one-string two-forbiddance
   conjunction (differential half lifts; pub-alias half survives). [e2/H-2]
3. **Rel.cpp:4288-4293 / :4298 / :4325** — V-INST-EFFECT's kNetAddition-only
   drain-role reject, ++input_drains, and the hard-coded
   `drains==2 && input_drains==1` totality; the regime ternary pub-keyed.
   [H-3, ADV-6]
4. **Rel.cpp:4542-4548** — the V-INST-DRAIN input arm + the "D3.a.2 owns the
   split here" comment; `dr_ok` at :4517-4528. [H-4]
5. **Rel.cpp:2541-2604** — the generic both-sign mint loop (acyclic arm
   :2597-2603): the input's ± producers are FREE. [H-9, ADV-5]
6. **Rel.cpp:796-800 (+ :1098, :868)** — InstantiateEffects' single
   kNetAddition edge_drain; the call passing no input differentiality; the
   DeathEffects kNetRemoval drain as the removal-leg mold. [H-6]
7. **Database.cpp:2366-2405** — emit_instance_rescan: NO presence predicate
   (the ADV-3 hole), + **Table.h:421-424** DiffTable::Present ("post-commit
   presence: counts > 0") vs :571-575 Commit's kInI — the Present-spelling
   adjudication's two poles. [H-10]
8. **Database.cpp:2487-2514 (RIDER :2494-2495)** — the R-3 demand-liveness
   gate + the in-source D3.a.2 quiescence rider; the `diff` selector is
   PUB-keyed. [H-11/H-13, XC-5/XC-9]
9. **Build.cpp:1110-1114** — the monotone-only eager boundary append (the
   XC-3-twin skip that makes ABORT 2 fire); Procedure.cpp:318 vs :327-331 —
   the fenced demand fetch vs the unfenced mint-on-miss input fetch (the
   silent-orphan hazard). [§3 chain, H-8]
10. **Rel.cpp:1055 vs :1140** — P-STORE (`diff` local, pub-keyed) vs P-DEATH
    (demand-keyed death-mint gate): the §7-ruled pair that D3.a.2 makes
    diverge; plus InstanceStore.h:21-27/:161-163/:216 (N-1 note,
    WorkingOccupied, RecycleCurrent) for OB4/§6(ii). [e5/H-15/H-16]

UNRESOLVED FLAGS: **NONE at stage (a).** ADV-3 is CONFIRMED (spelling =
Present, all four lanes + this consolidator's code read converge — §5
adjudication); ADV-9's option space is fully fact-based in §6(ii) with the
one observably-divergent shape identified; the only open DECISIONS are the
ritual-head a2-trigger-shape ruling (§6 — RULED, §7 below) and stage-(b)
choices explicitly marked (C15 dump marker, H-17 witness disposition).

===============================================================================
## §7 THE RITUAL-HEAD RULING (orchestrator, AT CODE — the first ritual-head
##    question of D3.a.2, ruled BEFORE the stage-(b) design lanes; the d2/
##    §20(AI) precedent: no owner brief because the §6 fact base proves no
##    admissible option changes observable behavior — flat-oracle answers and
##    published deltas are identical across them; only generated-code shape
##    and model surface differ)
===============================================================================

**RULING R-A2-TRIGGER: TWO DRAINS, NO RECYCLE.** Binding for every stage-(b)
design lane:

1. **Second-drain shape (combined ± drain REJECTED).** The input table's
   kNetRemovals frontier becomes a SECOND band-(a2) drain arm — the THIRD
   rescan source overall (a1 demand-additions, a2 edge-additions, a2'
   edge-removals) feeding the ONE shared rescan mold. Grounds: the VecRole
   enum has NO combined ± role (Rel.h:54-66) — a combined drain mints new
   model surface for zero behavioral gain, while the two-drain shape reuses
   landed roles verbatim (the demand a0 drain is the one-sign drain
   precedent; DeathEffects' kNetRemoval drain Rel.cpp:868 is the
   effect-declaration mold). V-INST-EFFECT's totality grows
   `input_drains==2` under the input-differential regime (the ADV-6 third
   predicate axis — the regime split is INPUT-keyed, never folded into
   P-STORE/P-DEATH per the d2 discipline).
2. **Identical gate set.** The removal arm carries EXACTLY the landed a2
   diff-arm gates — `iid != kNoInstance` outer, the demand Find+Present
   conjuncts per the demand regime (diff demand: the R-3 probe; monotone
   demand: the XC-9 irrevocability degeneration), `!TouchedFlag` — and the
   mold's V-INST-FRESH belt. The §6(i) behavior-neutrality of drain order
   and count holds ONLY under gate-set identity; a gate divergence between
   the two a2 arms is a design ERROR (stage-(b) belt-check material).
3. **RecycleCurrent stays DEATH-ONLY (band-(a0)).** No input arm calls it.
   Grounds: current is provably EMPTY at first touch (V-INST-FRESH +
   Seal/Recycle sole emptiers — §6(ii)), so a gated Recycle is a no-op that
   additionally parks the V-INST-FRESH belt behind dead code in that arm (a
   strict reach-weakening); the UNGATED-late Recycle is the ONE
   observably-divergent shape (the §6(ii) E-E silent full-retract wrong
   answer that no landed belt catches) and is FORBIDDEN — stage (b) records
   it as a named design fence. OQ-INPUT's ruled SEMANTICS ("any input
   change, either sign, for a live-demanded key fires a full rescan;
   band-(b) diff-at-publish emits the net retractions") is preserved
   VERBATIM; its "RecycleCurrent + full rescan" MECHANISM wording is
   superseded at band position (the R-2/§20(AJ) precedent for
   mechanism-level supersession of a ruling's incidental wording — the
   ruling's semantics and every observable are unchanged).
4. **Band order.** a0 (death) → a1 (birth) → a2 (edge-additions) → a2'
   (edge-removals, APPENDED AFTER the landed a2 arm). Inter-arm order is
   behavior-neutral (§6(i): TouchedFlag + gate-set identity make whichever
   arm first touches a key do the one net-state rescan), so the tie-break
   is diff minimality: the landed a0/a1/a2 emission bytes stay untouched
   ahead of the new arm (id-stream stability, the (d0) baseline
   discipline). a0-before-input-arms preserves the OD-15 death-wins
   suppression under any FUTURE combined entry, which today cannot occur
   (T-3 channel disjointness); the hypothetical combined entry remains the
   §4 LOUD cell that e4's fence-or-proof must close (OB8) — this ruling
   does NOT discharge it.

Rider (binding, from §5/OB2): the Present(s) rescan conjunct (ADV-3,
spelling RULED Present per the four-lane convergence) applies to the ONE
shared mold — therefore to ALL THREE sources (a1 birth included: the E-F2
rebirth-after-edge-retract cell) — gated on the input-differential regime;
the monotone-input emission stays byte-identical.

===============================================================================
## §8 THE POST-D3.a.2 STATE (2026-07-29, tip bfc068d1; orchestrator-read
##    anchors — the epoch's whole-program view AFTER slice 2, §20(AO).
##    SINGLE-PASS [DISCHARGED 2026-07-30, ledger §20(AP): the fleet
##    re-verified THIS section + §20(AL)-(AO); VERDICT SOUND, zero errata,
##    zero false anchors; g5 amended below per ADV-1]. §1-§6 above are the
##    PRE-slice-2 map
##    (stamped at b4d08307; the slice's ~1078 net inserted lines drifted
##    their anchors); §7 is the R-A2-TRIGGER ruling, still binding.
===============================================================================

    THE PIPELINE AS IT STANDS (whole-program; only slice-2 deltas spelled
    out — the demand/retract/death machinery is per d3a1-substrate.md §8
    modulo drift, the pre-slice input path per §1-§6 above):

    admission (lib/ControlFlow/Build/Build.cpp):
      the nested pre-pass at :1504ff keeps ONLY the cyclic-demand and
      recursive-content fences (trigger :1530-1531, reject :1543-1549);
      the diff_input arm + flag are DELETED — a @differential summarized
      input is ADMITTED under -demand-instance. No new flag: input
      differentiality flows from the user's @differential on the input
      message through the ordinary Differential.cpp closure.

    the mint (lib/Rel/Rel.cpp):
      BuildSubgraphInstanceOps :1035 (called from the :2062-2065 mint loop,
      per recognized+live instance); P-STORE = the `diff` local
      (TableIsDifferential(pub)) :1071; P-DEATH gate
      (demand_table && TableIsDifferential(demand_table)) :1162; P-INPUT =
      `input_diff` (TableIsDifferential(input_table)) computed at the mint
      and threaded into InstantiateEffects, which emits the second
      kVecDrain{input, kNetRemoval} leg under it; DeathEffects unchanged.

    validators (lib/Rel/Rel.cpp):
      V-INST-SOLE = pub-alias half + the NEW induction-owned-input belt
      :4397-4402 (recursive content stays fenced at the DR layer too);
      V-INST-EFFECT totality drains==(input_diff?3:2),
      input_drains==(input_diff?2:1) + the O-1 CLOSURE BELT
      (input_diff && !diff aborts — the checked one-directional theorem);
      V-INST-DRAIN input arm regime-split (monotone cf_ok kNetAdditions;
      differential dr_ok BOTH signs + both filter producers); the pure
      belts CheckInstanceDeathFrontier :4626 and CheckInstanceInputArm
      :4627 (body :4906ff — detects input-diff STRUCTURALLY via the
      kNetRemoval role, co-true with TableIsDifferential on real flows;
      the INLINE checks' fork-tested teeth are a D3.a.3 obligation,
      review design-1). V-INST-INPUT-COHERENCE + the fenced pre-minted
      ± fetch live in Procedure.cpp (the coherence check is a
      construction tautology within its scope — the LIVE fence is the
      orphan-mint check; comment says so honestly).

    the band (lib/CodeGen/CPlusPlus/Database.cpp, EmitSubgraphInstance):
      the codegen selector = MEMBER PRESENCE: input_removal =
      region.InputRemovalFrontier(); input_diff =
      input_removal.has_value() :2406-2407 (belt-checked against
      TableIsDifferential by V-INST-INPUT-COHERENCE); the ONE rescan mold
      :2423ff carries the input.Present(s) conjunct under input_diff —
      ALL THREE sources (a1 :2536, a2 :2620, a2' :2700); band-(a2')
      :2631ff = the edge net-REMOVALS drain APPENDED after a2, a gate-set
      CLONE of a2 (iid -> nested demand Find+Present under diff demand /
      plain gate under mono demand -> !TouchedFlag -> the mold), NO
      RecycleCurrent anywhere in it (the §7 fence, named in-code); the
      five-way coupling block heads the function; band-(b)/Seal
      UNCHANGED (the (T,F) drop scan + born arm + V-INST-PARTITION
      already handle shrink). RecycleCurrent remains DEATH-ONLY (a0).

    dumps:
      the .ir subgraph-instance line gains the only-when-present
      ` input-removals <vec>` production (death-branch mold; the slice's
      ONE E-71 note, lib/ControlFlow/Format.cpp); the .rel instantiate
      op's effects: line shows the kVecDrain(<input_tid>, kNetRemoval)
      leg from EXISTING tokens; census KINDS unchanged (29).

    witnesses (tests/OptDiff; suite 178, eqgate carriers 4 = 16 live
    verdicts):
      demand_diff_neighborhood_witness — the e5 carrier (diff-input x
      MONO-demand; the FIRST P-STORE && !P-DEATH program; census
      kInstanceDeath=0 kSubgraphInstantiate=1 kSeedFold=6); its E-F2b-
      analogue live-key retract phases + the A4.1 cross-batch a1-Present
      teeth. demand_diff_input_1 — REPURPOSED diagnostic->golden, the
      diff x diff composition (E-F1 dead-key silence / E-F2 rebirth /
      E-F2b live-key net retraction, review WIT-4 / E-F3 second-death;
      census kInstanceDeath=1 kSeedFold=8). The two D3.a.1 witnesses are
      FROZEN regression anchors (byte-identical nested dumps).

    THE THREE PREDICATES (the §7-d2 discipline, extended): P-STORE =
    TableIsDifferential(pub) (:1071); P-DEATH = TableIsDifferential(demand)
    (:1162); P-INPUT = TableIsDifferential(input) (the mint's input_diff).
    Separately spelled, NEVER folded. Live divergences: the flagship is
    P-STORE && !P-DEATH && P-INPUT; the O-1 closure P-INPUT => P-STORE is
    a belt-checked THEOREM (not an invariant to lean on silently).

    THE ADORNMENT SUBSTRATE (what D3.a.3 touches, orchestrator-read):
      lib/DataFlow/Demand.cpp — the multi-adornment rejects: the
      parse-redecl belt :444-460 (a demanded query NAME with >1
      BindingPattern rejects; patterns collected :455) and the body-walk
      second-adornment/left-linear rejects :667-670 + :725; the adornment
      string (b/f per column) built :797-810 and ALREADY suffixes every
      fabricated name (demand__<name>_<adorn>); the pass head :979 notes
      the single-shot slice.
      lib/ControlFlow/Build/Build.cpp — the registries are ALREADY
      (query, BindingPattern)-keyed at both walks (forcer match :468-473,
      retract match :570-573); the builder TWINS are the f2 dedup target:
      BuildQueryForceProcedureFromRegistry :385 (assert-only handler
      guard) vs BuildQueryRetractProcedureFromRegistry :494 (always-on
      fence) — ~90 shared lines, vector kinds + fence strength the only
      deltas.
      lib/DataFlow/View.cpp — GuardAnnotationsCompatible :584-588 keys on
      (forcing_index, instance_key); the LABELED RESIDUAL block :571-583
      is the f1 precondition VERBATIM (survivorship role policy;
      proxy-TUPLE Equals-invariance; corpus-DORMANT fold arm).

    THE PATH FORWARD AS DIFFS ON THIS STATE (ruled order, OD-15):

    D3.a.3 — MULTI-ADORNMENT (NEXT; OQ-ADORN-KEY ruled: N disjoint
      stores, one per (query, BindingPattern) forcing; the pass loops
      STEP 1b->10 per adornment; opens at stage (a) — re-derive the
      adornment-side substrate from code, THEN diffs):
      g1 PRECONDITION f1 (BINDING, FIRST): re-derive
         GuardAnnotationsCompatible (View.cpp:584-588) against REAL fold
         shapes with directed witnesses BOTH directions — (a) the
         survivorship policy (the surviving record's role is load-bearing
         in ResolveLiveRecognition), (b) the proxy-TUPLE invariance
         question (a same-forcing different-key fold may be LEGAL on
         propagate-arm annotations — false-abort hazard). The fold arm
         goes LIVE with multi-guard folds; the D3.a.0 debug-assert
         evidence expires.
      g2 PRECONDITION f2 (BINDING): the retract/forcer builder dedup
         (ONE parameterized builder + one dispatcher; harden the forcer's
         :393-394-class assert to the always-on fence while there).
         CARRIES: review design-1 (>=1 RelValidators death test forking a
         REAL ValidateDROps over a minimally-mutated flow — teeth on the
         INLINE V-INST-EFFECT totality + V-INST-DRAIN input arm, not only
         the pure belts) and the E2c a2/a2' ~24-line gate-clone dedup
         (same sweep, same commit).
      g3 THE PASS LOOP: lift the Demand.cpp:444-460 redecl reject; run
         the SIP walk + fabrication + guard minting once per adornment
         (names already adorned — the fabricated-message namespace is
         collision-free by construction); the :667-670/:725 body-walk
         rejects narrow to genuinely-unsupported shapes (left-linear
         stays out).
      g4 THE KEYING SWEEP: every registry/lookup keys
         (query, BindingPattern) — the two Build.cpp walks already do;
         sweep every name-only keying (fabrication memos, injector
         suppression, forcing_index assignment) with a directed
         two-adornment probe per site.
      g5 N DISJOINT STORES: one SUBGRAPHINSTANCE region per adornment's
         forcing; the Rel.cpp mint loop is already per recognized
         instance — the work is per-adornment RECOGNITION (annotation
         stamps per forcing_index) + census expectations (N instantiates,
         N seals; kInstanceDeath per adornment's demand regime).
         OBSTRUCTION (§20(AP) ADV-1, orchestrator-verified at code — the
         CENTRAL D3.a.3 design question): the STORE side is already
         N-safe (inst_per_store/seal_per_store/death_per_store keyed on
         op.instance_store_id, Rel.cpp:4404/:4438/:4448/:4460), but the
         PUB side is NOT — V-INST-SOLE's inst_per_pub keyed on
         op.table_op_table (:4406) LOUD-ABORTS at :4454-4458 when the
         count != 1, and N adornments of ONE (query,arity) resolve pub by
         q_decl.Id() (name+arity, ResolveLiveRecognition :992) to the
         SAME model table -> N mints set table_op_table = the shared
         pub_table -> count N -> abort. g5 MUST rule the per-pub check
         redesign FIRST: relax to per-(pub, forcing/key-shape) OR merge
         the N publishes.
      g6 WITNESS FAMILY: a two-adornment query witness (eqgate; decide
         demand_multi_adorn_1's disposition — it pins the REJECT today);
         regime matrix cells worth carrying: two adornments with
         DIFFERENT demand regimes (one -demand-retract-forced name?—
         ruled at stage (b)).
      g7 FENCES THAT SURVIVE: recursive demand (FENCE (i), all-epoch;
         the §20(AB) NeedsInductionCycleVector precondition binds); the
         R-5 widening obligation (any body-walk/recognition widening
         re-derives OB8(i)'s derived-input branch with a directed
         witness FIRST).
      g8 LIVENESS: L-rows for the fold predicate both directions (g1's
         witnesses double), the keying sweep probes, the dedup'd
         builder's fence; vehicles NESTED-arm; never-minted roles per
         the L3 amendment; design a REAL red-team catcher when a fence's
         claimed observable is load-bearing (the L15 lesson: split
         counters >= 2 masked the E-E wipe — pick support-1 rows).
    DEFERRED all-epoch: recursive demand.

    RITUAL AMENDMENTS BANKED THIS SLICE (bind future stage-(d) runs):
      P-lane CHAINS WIP-COMMIT AT EVERY LANE BOUNDARY (P2 ended without
      committing; P3 had to supply the anchor — make the commit each
      lane's LAST protocol step, verified by the next lane);
      the placeholder-gate rule EXERCISED (P2 capped mid-suite; the
      orchestrator personally re-executed Phase A/cmp/bless/Phase B —
      never respawn, never trust a lane's cited-but-unrun gate);
      masked-negative honesty: when a perturbation row PREDICTS a
      catcher, verify the catcher's observable is not masked (L15's
      split-counter masking; L17's single-proc byte-identity) and record
      the miss loudly rather than re-running to green.
