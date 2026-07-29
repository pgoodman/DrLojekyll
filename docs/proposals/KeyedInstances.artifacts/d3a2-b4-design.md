# D3.a.2 STAGE-(b) DESIGN — LANE b4: witness family + demand_diff_input_1 disposition + the gate plan

> **House banner.** Tip **b4d08307** (branch keyed-instances; code bytes ==
> the D3.a.1 landing 33cabcf1 — every commit atop is docs-only, so every code
> anchor below is live at the landed binaries and at the frozen-A debug
> compiler `scratchpad/frozenA/drlojekyll-debug`). BINDING context honored, not
> re-litigated: d3a2-substrate.md §7 (the R-A2-TRIGGER ruling: TWO DRAINS, NO
> RECYCLE, gate-set identity, band order a0→a1→a2→a2', the FORBIDDEN
> ungated-late-Recycle), §5 (the gap ledger — this lane owns rows **C15, H-16,
> H-17, H-18, H-19**), §4 (the epoch catalogue E-A..E-F3 + OB1-OB8), §1 (the
> flat oracle O-1..O-9); d3a-ruling-brief.md (OQ-INPUT / OQ-MODEL /
> OQ-DEATH-VS-REBUILD); d3a1-design.md + d3a1-b4-design.md (the MOLD:
> edit-spec granularity, the (d0) baseline protocol, the d7 L-table idiom, the
> gate roll-call); KeyedInstances.md §20(AK)-(AM). Prediction tags: **[BYTE]**
> byte-identical, **[STRUCT]** structured change with exact predicted shape,
> **[COMPUTED]** exact bytes produced at design time by running the frozen-A
> tooling. Every empirical claim is reproduced against
> `scratchpad/frozenA/drlojekyll-debug` / `-oracle-debug`; artifacts in
> `scratchpad/d3a2b/`.

---
## §0 HEADLINE DECISIONS + THE WITNESS MATRIX

**The slice serves BOTH regimes (d3a2-substrate §4/O-9, R-A2-TRIGGER §2):
diff-input × MONO-demand (the e5 carrier — first P-STORE∧¬P-DEATH program) AND
diff-input × diff-demand.** These are two DISTINCT generated-code paths at the
band-(a2) removal arm — the `diff` selector is PUB-keyed and TRUE in both, so
both emit the `Find`+`Present` gate, but the demand table MEMBER is a monotone
`Table` (mono-demand: `Table::Present` → XC-9 IRREVOCABILITY soundness) vs a
`DiffTable` (diff-demand: `DiffTable::Present` → the D3.a.1 QUIESCENCE
soundness / the R-3 dead-key gate-close). A single case has ONE `.drflags`, so
the two regimes need TWO witnesses. The `demand-death interleaved with
edge-retract` cells (E-F1/E-F3) are ONLY reachable when demand can die =
diff-demand — so the composition witness is load-bearing, not a redundant
cross-product.

**D-b4-1 (the witness disposition — RECOMMENDED, fully spec'd below).** A NEW
FLAGSHIP case + REPURPOSE the existing fence case, completing a clean 2×2
`{mono,diff}demand × {mono,diff}input` eqgate matrix:

| case | demand | input | .drflags | role | new? |
|---|---|---|---|---|---|
| `demand_neighborhood_mono_witness` | mono | mono | `-demand` | R-MONO baseline | landed |
| `demand_neighborhood_witness` | **diff** | mono | `-demand -demand-retract` | D3.a.1 diff-demand | landed |
| **`demand_diff_neighborhood_witness`** | mono | **diff** | `-demand` | **e5 carrier (H-16)** — the flagship | **NEW** |
| **`demand_diff_input_1`** (repurposed) | **diff** | **diff** | `-demand -demand-retract` | full composition (E-F1/E-F2/E-F3) | REPURPOSED |

Grounds for NOT extending `demand_neighborhood_witness` (the prompt's
weigh-in): it is the D3.a.1 PINNED artifact whose goldens are the diff-DEMAND
regression anchor; extending it (flip `add_edge` → `@differential`, append
edge-retract batches) would CHURN its three blessed goldens AND collapse the
demand axis onto the input axis (its input would go differential, destroying
the "mono input" reference point that pins the input drain=1 half of
V-INST-EFFECT flag-off). Keeping it frozen preserves the reference and keeps
the diff `.rel`/`.h` config-invariance carrier stable. The NEW flagship is the
e5 divergence carrier that this slice FIRST makes real; the repurpose
discharges the H-17(iii) demand_diff_input_1 disposition in the SAME move that
covers the diff-demand composition. **Suite 177 → 178** (one net-new case;
demand_diff_input_1 flips diagnostic→golden, no count change). Eqgate carriers
2 → **4**.

**D-b4-2 (the observable — diverges from D3.a.1's demand-death).** Death here is
INPUT-driven, not demand-driven. In the FLAGSHIP (mono demand), demand NEVER
retracts (`neighborhood_bf_retract` does not exist — VERIFIED absent in the
flat `-demand` header, `scratchpad/d3a2b/out_flat/datalog.h`); a neighborhood
SHRINKS by retracting its EDGES (a two-Vec `add_edge_2(db, log, functors, adds,
removes)` — VERIFIED the differential message takes two Vecs, and the demand
relation `table_4` is a MONOTONE `Table<Row4>` beside `DiffTable` edge/nbhd —
the e5 divergence, EMPIRICAL). The demand-blind observable is the published
`@differential` `nbhd_out` tap (the D3.a.1 idiom; probes re-inject demand so a
probe is never a death observable). **Unlike D3.a.1, the ORACLE SEES the
retractions** (input retracts are `@differential` MESSAGES, legal `-` ops —
VERIFIED the frozen-A oracle nets `- add_edge 1 2` to a shrunken
`neighborhood`, `scratchpad/d3a2b/probe.batches` → stdout `neighborhood 1 3 /
3 5`, INVARIANT to stderr). So the oracle/monotone goldens reflect NET input
state — a stronger cross-check than the demand-retract witness's demand-blind
oracle.

**D-b4-3 (the FORBIDDEN-fence catcher — the ungated-late-Recycle, §6(ii)/R-A2
§3).** The one observably-divergent shape (an UNCONDITIONAL `RecycleCurrent` in
the removal arm, ordered after a same-epoch add-rescan) is caught ONLY by the
eqgate + a directed driver assert. The flagship's **`send_pm` same-batch ±
step (E-E)** is exactly that discriminator: batch `{+(K,a), -(K,b)}` for a
live-demanded K with standing frozen rows — the add arm rescans K (net
content), and an ungated-late Recycle wipes `cur` before band-(b) → K's entire
frozen set drops, births nothing; V-INST-PARTITION does NOT abort (counts
balance) and V-INST-FRESH never fires. The eqgate catches the silent
full-retract. §5 L-table row L-b4-E encodes the perturbation.

**D-b4-4 (E-71 — b4 owes ZERO new grammar note).** The input removal leg
renders as `kVecDrain(<input>, kNetRemoval)` on the instantiate's `effects:`
line — token `kNetRemoval` ALREADY renders in every pinned/nested dump
(kInstanceDeath effects, every `kVecAppend`; VERIFIED in
`scratchpad/frozenA/nested-ref/w.rel:54,:59`). It is a first-live-PRODUCTION of
that (op, table, role) triple on a `kSubgraphInstantiate`, appearing only in
the two NEW witness `.rel` dumps (unpinned), which is the D3.a.1 kInstanceDeath
precedent (production, not spelling). **b4 files touch no dump grammar.**
Obligation recorded for b2/b3: IF the C15 input-diff dump marker (a NEW token,
not the existing `input=` at Format.cpp:668/:870) is added, THAT needs an E-71
grammar note in t2b-grammar.md — a review gate, not a b4 edit.

**D-b4-5 (H-18 discharges recorded).** G-INPUT-NEG is discharged by H-11's
removal band (the a2' trigger rebuilds on input net-removals). G-STALE is
discharged by SUBSUMPTION: every input content change surfaces in the input's
± frontier CARRYING ITS KEY, so the keyed rescan revisits exactly the affected
instances — no broader revisit protocol (OQ-INPUT "G-STALE subsumed",
d3a-ruling-brief:64). The flagship's O-5 step (retract of a non-demanded key ⇒
silence) is the directed witness that no over-revisit occurs.

---
## §1 THE e6 FLAGSHIP — `demand_diff_neighborhood_witness` (NEW; the e5 carrier)

Diff-input × MONO-demand. Mirrors the `demand_neighborhood_witness` graph so
the answer semantics are already refereed by the landed neighborhood oracle,
but the `add_edge` message is `@differential` and death is EDGE-driven. VERIFIED
end-to-end at frozen-A: flat `-demand` compiles (rc=0), nested
`-demand-instance` REJECTS at fence (iii) with the exact string
(`scratchpad/d3a2b/{flat,nested}.log`).

### E4a — `cases/demand_diff_neighborhood_witness.dr` [new file]

```
; Copyright 2026, Peter Goodman. All rights reserved.
;
; demand_diff_neighborhood_witness -- the D3.a.2 DIFFERENTIAL-INPUT eqgate
; carrier and THE e5 DIVERGENCE WITNESS (d3a2-substrate H-16): the first program
; where P-STORE (TableIsDifferential(pub), Rel.cpp:1055) is TRUE while P-DEATH
; (TableIsDifferential(demand), Rel.cpp:1140) is FALSE. The `add_edge` input is
; @differential (deletable) while the demand relation stays MONOTONE (bare
; `-demand`, no `-demand-retract`), so the band-(a2) demand-liveness gate probes
; a MONOTONE demand member -- soundness by IRREVOCABILITY (monotone demand never
; retracts, d3a2-substrate XC-9), not the D3.a.1 quiescence argument.
;
; DEATH IS EDGE-DRIVEN, never demand-driven: demand is monotone, so a probe
; re-injects demand idempotently and `neighborhood_bf_retract` does NOT exist. A
; neighborhood SHRINKS by retracting its EDGES via the two-Vec differential
; message `add_edge_2(db, log, functors, adds, removes)`. Death is observed
; through the published @differential `nbhd_out` tap (probes cannot see death).
; The band-(a2) input net-REMOVALS trigger (a2', R-A2-TRIGGER: TWO DRAINS) +
; the Present-filtered rescan (ADV-3) are exactly what a live-demanded edge
; retract exercises.
;
; HP-5: the graph carries edges OUTSIDE each probed key's neighborhood; each
; probe ASSERTS its drained answer is EXACTLY neighborhood(Start), so an
; over-materialized nested arm both aborts the driver AND diverges from the
; golden. Under flat `-demand` this compiles and answers; the nested
; `-demand-instance` lowering must be ANSWER-IDENTICAL on the same batches --
; the two-lowerings equivalence gate (run_eqgate), refereed live: flat ==
; nested == golden, PLUS sorted published-delta identity through nbhd_out.

#message add_edge(u64 From, u64 To) @differential.

#local edge(u64 From, u64 To).
edge(From, To) : add_edge(From, To).

#query neighborhood(bound u64 Start, free u64 Node) : edge(Start, Node).

; G-15: any published output over the demanded closure MUST be @differential
; (Differential.cpp faithfulness check) once the input closure is differential.
#message nbhd_out(u64 Start, u64 Node) @differential
    : neighborhood(Start, Node).
```

### E4b — `cases/demand_diff_neighborhood_witness.drflags` [new file]

```
-demand
```

One line, bare `-demand`. This IS the e5 carrier: flat `-demand` = mono demand
+ diff input; the eqgate appends `-demand-instance` (runall.sh:331) → the
nested arm is mono-demand × diff-input. `-demand-retract` is DELIBERATELY absent
(that regime is the composition witness §2).

### E4c — `cases/demand_diff_neighborhood_witness.batches` [new file]

The oracle mirror of the driver's `add_edge` stream (adds AND removes; the
oracle is demand-blind + sees `-` ops on the `@differential` message). One batch
per driver entry-point call that touches the input, in order:

```
# demand_diff_neighborhood_witness -- oracle input batches. The @differential
# add_edge stream the driver sends (adds and net-removals). The oracle evaluates
# the plain undemanded program and NETS the retractions (unlike the D3.a.1
# demand-retract witness, whose death was demand-surface -- here death is an
# input MESSAGE, so the oracle sees the shrink). neighborhood rows = the NET
# single-hop relation, the answer-identity referee for the demand-ON per-key
# answers.
#
# BIRTH: 1's out-edges, the 9->9 self-loop, the detached 7->8, then 3's edges.
batch
+ add_edge 1 2
+ add_edge 1 3
+ add_edge 9 9
+ add_edge 7 8
end
batch
+ add_edge 1 4
+ add_edge 3 5
+ add_edge 3 6
end
# E-D edge-retract of a LIVE-demanded neighborhood: drop 1->2 (a2' trigger).
batch
- add_edge 1 2
end
# E-E same-batch +/- for one demanded key (different rows): 1 gains 11, loses 3.
batch
+ add_edge 1 11
- add_edge 1 3
end
# O-6 same-batch +/- of the EXACT row (nets at NetBatch -> no effect).
batch
+ add_edge 3 5
- add_edge 3 5
end
# O-5 edge-retract of an UNDEMANDED key (never probed): 7->8 goes, no leak.
batch
- add_edge 7 8
end
# a1-Present-conjunct (retract-before-demand): 5 is UNDEMANDED here; add then
# remove 5->12 BEFORE 5 is ever probed, so the later probe(5) a1 birth-rescan
# must SKIP the physically-present-but-dead (5,12) row (ADV-3 on the a1 source).
batch
+ add_edge 5 12
- add_edge 5 12
end
```

**Oracle/monotone applicability — YES (decided, VERIFIED).** The `.batches`
runs the derivation-counter oracle + monotone projection (runall.sh:210-257);
the oracle handles `@differential` + `-` ops (frozen-A: `scratchpad/d3a2b`).
This is a STRONGER cross-check than the D3.a.1 witness (whose oracle was
demand-blind to death) — here the oracle nets input retractions, so
`neighborhood`'s NET closure is refereed. The `INVARIANT:` line goes to
stderr (VERIFIED) → absent from the stdout golden. Both goldens [COMPUTED] at
stage (d) from the frozen-A `-oracle-debug` on E4a + E4c.

### E4d — `cases/demand_diff_neighborhood_witness.eqgate` [new file]

The landed `demand_neighborhood_witness.eqgate` marker text verbatim (retitled).
Presence turns on run_eqgate: nested (`-demand -demand-instance`) built + run
with the SAME driver in all four modes, each stdout byte-compared to
`goldens/demand_diff_neighborhood_witness.stdout`. No nested golden blessed
(OD-10/OWN-5). This is where the e5-carrier a2' removal trigger + Present-
filtered rescan get refereed against the flat oracle LIVE.

### E4e — `cases/demand_diff_neighborhood_witness.main.cpp` [new file]

The `demand_neighborhood_witness.main.cpp` driver is the MOLD (PrintLog with a
sorted per-epoch flush, `probe` with the HP-5 exact-answer assert + cursor
drain-and-sort, keyed-drain sort discipline). AMENDMENTS for the diff-input
regime:

1. **`send`** stays adds-only but constructs the differential two-Vec call:
   `Vec<Tup_u64_u64> add(allocator); Vec<Tup_u64_u64> rem(allocator);
   add.Add({f,t})...; add_edge_2(db, log, functors, std::move(add),
   std::move(rem));` (the `deep_chain_retract.main.cpp` two-Vec idiom; element
   type `Tup_u64_u64`, NOT `add_edge_input` — the differential message emits no
   `_input` alias, VERIFIED). Empty `rem`.
2. **`retract_edges`** [NEW lambda]: same call with `add` empty and `rem`
   carrying the removed rows.
3. **`send_pm`** [NEW lambda]: both `add` and `rem` non-empty (E-E and O-6).
4. **NO `retract` lambda** (mono demand — no `neighborhood_bf_retract`).
5. **`send_expect_silent`** twin (A4.3 normative): before flushing, hard-ABORT
   if `!log.rows.empty()` — the O-5 dead/undemanded-key discriminator teeth
   (survives NDEBUG).

Phase structure (the assertion skeleton; exact stdout hand-derived at stage
(d) per §4.2 (d0) protocol — do NOT bless bytes not derived from the flat run):

```
init(db, log, functors);
// ---- BIRTH ----
send({{1,2},{1,3},{9,9},{7,8}}, "e1");
send({{1,4},{3,5},{3,6}}, "e2");
probe(1, {2,3,4}, "p1");        // stands demand for 1 (mono, idempotent).
probe(3, {5,6}, "p3");
probe(9, {9}, "p9");
// ---- E-D: edge-retract shrinking a LIVE-demanded neighborhood (a2') ----
retract_edges({{1,2}}, "d1");   // -(1,2) published; 1 still demanded.
probe(1, {3,4}, "p1b");         // shrunk; HP-5 asserts EXACTLY {3,4}.
// ---- E-E: same-batch +/- for one demanded key (the FORBIDDEN-fence catcher) ----
send_pm({{1,11}}, {{1,3}}, "m1"); // +11 -3 net; one rescan reads NET input.
probe(1, {4,11}, "p1c");        // an ungated-late Recycle drops ALL of 1 here.
// ---- O-6: same-batch +/- of the EXACT row -> no-op (NetBatch annihilates) ----
send_pm({{3,5}}, {{3,5}}, "m2");
probe(3, {5,6}, "p3b");         // unchanged.
// ---- O-5: edge-retract of an UNDEMANDED key -> SILENT (send_expect_silent) --
retract_edges_silent({{7,8}}, "d2");  // 7 never demanded; nothing published.
// ---- a1-Present-conjunct: retract-BEFORE-demand (a1 birth must skip dead row)
send_pm({{5,12}}, {{5,12}}, "m3"); // add+remove same row while 5 UNdemanded.
probe(5, {}, "p5");             // a1 birth-rescan over input where (5,12) is
                                //   Present==false; MUST be empty, not {12}.
return 0;
```

Discriminator roll-call (which step forces which gap):
- **d1 (E-D)** forces H-11 (the a2' input net-removals trigger, OB1) — without
  it the pub sticks present (§3 silent-miscompile (i)); triply loud
  (eqgate/golden + the shrunk-answer HP-5 assert).
- **p1b/p1c** force ADV-3 (the a2' rescan's `input.Present(s)` conjunct, OB2) —
  an unfiltered `NumRows()` rescan re-materializes the retracted (1,2)/(1,3).
- **m1/p1c (E-E)** force the FORBIDDEN ungated-late-Recycle fence (§6(ii)/R-A2
  §3) + shared-TouchedFlag one-rescan (OB5/OB6).
- **m2/p3b (O-6)** witness the NetBatch same-row annihilation (O-6/OB7).
- **d2 (O-5)** witnesses death-only-inside-the-demanded-neighborhood (O-5,
  G-STALE subsumption/H-18) — hard-abort teeth.
- **m3/p5 (a1-Present)** force the ADV-3 conjunct on the a1 BIRTH source (the
  E-F2-analog reachable in mono-demand: input row dead before first demand).

### E4f — the predicted goldens (files, shapes; bytes at stage (d))

- `goldens/demand_diff_neighborhood_witness.stdout` [STRUCT] — full driver
  stdout: per-epoch `label: <sorted signed deltas>` + `nbhd <k>: <sorted
  nodes>` per probe. Delta placement law (EMPIRICAL on flat at frozen-A, the
  D3.a.1 precedent): deltas appear in the epoch that DERIVES them; a demanded
  key's initial rows publish at its first-probe epoch; edge epochs for a
  standing demanded key carry that epoch's ± rows; undemanded-key epochs
  publish nothing. Confidence: birth/shrink placement EMPIRICALLY grounded;
  the m1/m3 net lines derive from the ruled Present-filtered net-rescan
  semantics — a deviation at bless REJECTS.
- `goldens/demand_diff_neighborhood_witness.oracle.stdout` [COMPUTED at (d)] —
  `ORACLE: OK (7 batches, N assertions)` + the NET `neighborhood` rows (after
  all retractions): keys present = `1→{4,11}`, `3→{5,6}`, `9→{9}`; `7→{}`
  (7,8 retracted), `5→{}` (5,12 netted). VERIFIED shape via
  `scratchpad/d3a2b/probe.batches` (retraction nets to shrunken output).
- `goldens/demand_diff_neighborhood_witness.monotone.stdout` [COMPUTED at (d)]
  — `MONOTONE-PROJECTION: M surviving facts` + the same NET rows (the
  differential-final == monotone-projection INVARIANT holds; VERIFIED the
  projection nets retractions).

---
## §2 THE COMPOSITION WITNESS — `demand_diff_input_1` REPURPOSED (diff × diff)

Disposition **(ii)** of H-17(iii): repoint the lifted-fence case as the
diff-input × diff-demand runtime + eqgate witness. It reuses the §1 flat-oracle
shape (pt/ans/getpt) and is the ONLY witness of the demand-death × edge-retract
interleavings (E-F1/E-F2/E-F3), which need a differential DEMAND to have demand
death. VERIFIED at frozen-A: flat `-demand -demand-retract` compiles (rc=0),
nested still rejects at fence (iii) pre-slice (`scratchpad/d3a2b/retract.log`,
`nested.log`).

### E4g — `cases/demand_diff_input_1.dr` [rewrite header + add tap + fix ADV-8]

Replace the fence-witness header with the runtime-witness header and append the
tap. The relations are unchanged (pt/ans/getpt); ADV-8 fixes the stale
`Build.cpp:1344` citation.

```
; Copyright 2026, Peter Goodman. All rights reserved.
;
; demand_diff_input_1 -- the D3.a.2 LIFTED-FENCE witness (was fence-(iii)): a
; bound #query whose demanded body summarizes a @differential (deletable) input,
; NOW SUPPORTED under -demand-instance (the fence at Build.cpp diff_input arm was
; lifted at D3.a.2 / lane b1). It is the diff-input x DIFF-demand composition
; carrier (P-STORE true, P-DEATH true): both axes on = plain symmetric
; differential (d3a2-substrate O-9). The eqgate re-drives it nested and referees
; flat == nested == golden + sorted nbhd tap deltas.
;
; This witness exercises the demand-death x edge-retract interleavings that need
; a differential DEMAND (E-F1/E-F2/E-F3, d3a2-substrate §4): retract demand then
; retract the edge (the R-3 gate CLOSES on the dead key's absent demand.Present);
; edge-retract for an already-dead key (gate closes, nothing to drop); and
; edge-retract then re-demand (a1 birth rebuilds from the SHRUNKEN input, the
; ADV-3 conjunct on the birth source).

#message pt(u64 Key, u64 Val) @differential.

#local ans(u64 Key, u64 Val).
ans(K, V) : pt(K, V).

#query getpt(bound u64 Key, free u64 Val) : ans(Key, Val).

; G-15 tap (@differential over the demanded closure; the nbhd_out idiom).
#message getpt_out(u64 Key, u64 Val) @differential
    : getpt(Key, Val).
```

### E4h — `cases/demand_diff_input_1.drflags` [rewrite]

```
-demand -demand-retract
```

Was `-demand -demand-instance` (the old diagnostic drove `-demand-instance`
directly). Now bare flat is `-demand -demand-retract` (diff demand + diff
input); the eqgate appends `-demand-instance` → nested = diff × diff.

### E4i — `cases/demand_diff_input_1.main.cpp` [rewrite: inert stub → runtime driver]

Driver mold = the flagship (§1) but with the demand-retract surface present
(`getpt_bf_retract` exists in `-demand-retract`) and the diff-demand E-F cells.
Query is `getpt(bound Key, free Val)` (cursor). Phase skeleton:

```
init(...);
send({{1,10},{1,20},{3,30}}, "e1");   // pt adds (two-Vec differential).
probe(1, {10,20}, "p1");              // stand demand for key 1.
probe(3, {30},    "p3");
// ---- E-F1: demand-death THEN edge-retract same key (gate CLOSES) ----
retract_demand(1, "rd1");             // getpt_bf_retract(db,log,functors,1).
retract_edges_silent({{1,10}}, "d1"); // 1 is dead: demand.Present(1)==false ->
                                      //   a2' gate closes -> SILENT (teeth).
// ---- E-F2: edge-retract then RE-DEMAND (a1 birth from shrunken input) ----
probe(1, {20}, "p1b");                // re-demand 1: a1 rebuilds; (1,10) was
                                      //   retracted at d1 -> must be ABSENT.
// ---- E-F3: edge-retract for a key whose demand is dead (still bound iid) ----
retract_demand(1, "rd1b");
retract_edges_silent({{1,20}}, "d2"); // dead key: gate closes, nothing drops.
probe(1, {}, "p1c");                  // rebirth from fully-shrunken input: {}.
return 0;
```

Discriminators: **rd1/d1 (E-F1)** = the R-3 dead-key gate-close in the a2'
REMOVAL arm (demand `DiffTable::Present`==false) — the diff-demand twin of the
flagship's mono-demand O-5; **p1b (E-F2)** = the ADV-3 conjunct on the a1
birth source under diff-demand rebirth; **d2/p1c (E-F3)** = edge-retract for a
dead-but-iid-bound key (no tombstone) closes the gate. `retract_edges_silent`
carries the hard-abort teeth.

### E4j — `cases/demand_diff_input_1.batches` + `.eqgate` [new files]

`.batches` = the demand-blind pt stream (adds + `- pt` retractions mirroring
d1/d2); the oracle nets to the surviving pt rows. `.eqgate` = the marker text
(retitled). Goldens [COMPUTED at (d)]: `.stdout` (driver), `.oracle.stdout`,
`.monotone.stdout`.

---
## §3 demand_diff_input_1 DISPOSITION — the mechanics (runall.sh / CLAUDE.md / in-file)

### E4k — `tests/OptDiff/runall.sh` [STRUCT]

1. **:361 diagnostic `case` pattern** — REMOVE `|demand_diff_input_1` (it is no
   longer a diagnostic; it falls through to the `*)` diffrun arm). Exact edit:
   `...|demand_cyclic_1|demand_recursive_content_1|demand_diff_input_1)` →
   `...|demand_cyclic_1|demand_recursive_content_1)`. `demand_cyclic_1` and
   `demand_recursive_content_1` STAY (surviving fences).
2. **Header inventory (runall.sh:20-28)** — rewrite the
   `demand_cyclic_1/demand_diff_input_1` clause: `demand_diff_input_1` is no
   longer a `-demand-instance` reject; `demand_cyclic_1` remains the sole
   recursive-demand nested fence. Add `negate_never_diff_1` stays; add nothing
   for the two NEW golden cases (they are ordinary golden cases, not listed).
3. **Header suite-count-adjacent text** — none in runall.sh (the count lives in
   CLAUDE.md); the new cases are auto-discovered by `ls cases/*.dr` (:406).

### E4l — `CLAUDE.md` [landing commit; three edits]

- **:53** `177 corner-case programs as of the D3.a.1 differential-demand
  landing` → `178 corner-case programs as of the D3.a.2 differential-input
  landing` (+ optionally naming the new e5 witness).
- **:97-100** the diagnostic-list sentence
  `` `demand_cyclic_1`/`demand_diff_input_1` (two `-demand-instance`
  nested-lowering feature-gap fences — recursive demand and a @differential
  summarized input; both COMPILE under plain `-demand` and reject only under
  `-demand-instance`) `` → drop `demand_diff_input_1`; keep `demand_cyclic_1`
  as the single recursive-demand fence. Add a sentence: the diff-input fence is
  LIFTED at D3.a.2; `demand_diff_input_1` is now the diff×diff composition
  eqgate witness; `demand_diff_neighborhood_witness` is the e5 (diff-input ×
  mono-demand) carrier.
- **:484-486** the keyed-instance section fence sentence `` recursive demand
  (`demand_cyclic_1`) and a @differential summarized input
  (`demand_diff_input_1`) reject at the Program::Build nested pre-pass
  (Build.cpp:1336-1346) only under `-demand-instance` `` → `` recursive demand
  (`demand_cyclic_1`) rejects at the Program::Build nested pre-pass only under
  `-demand-instance` `` (drop the diff-input clause; the fence is gone). Add
  the D3.a.2 keyed-instance paragraph: differential summarized input is now
  admitted; the input net-removals a2' trigger + the Present-filtered rescan;
  the e5 P-STORE∧¬P-DEATH divergence.

### E4m — ADV-8 in-file fix

`demand_diff_input_1.dr`'s stale `Build.cpp:1344` citation is DELETED by the
E4g header rewrite (the new header cites no line). Recorded as discharged.

---
## §4 THE GATE PLAN (the d3a1-design §6 mold, adapted)

### 4.1 Per-surface predictions

| surface | prediction |
|---|---|
| 176 non-witness case stdouts × 4 modes | **[BYTE]** — no other case carries a diff-input demanded query; the fence lift (b1) and the input-drain split (b2/b3) fire ONLY under `-demand-instance` on the two witnesses; every flag-off program is untouched |
| 20 pinned dump surfaces (demand_tc_witness ×4, 11 `.rel` pins, 14 `.irgold`, `.df`/`.h` regen) | **[BYTE]** — flag-off; census `kInstanceDeath`/`kSubgraphInstantiate` unchanged on every pin; the input-drain split renders only in the new witnesses' `.rel` (unpinned) |
| `demand_neighborhood_witness` (D3.a.1 diff-demand) ×4 + oracle + monotone + eqgate ×4 | **[BYTE]** — FROZEN; not touched. Its input stays MONOTONE (`add_edge` unchanged), so V-INST-EFFECT `input_drains==1` there; the regime split (b1's ADV-6) leaves the mono-input arm byte-identical (d5-selector discipline) |
| `demand_neighborhood_mono_witness` (mono×mono) ×4 + eqgate | **[BYTE]** — FROZEN; bare `-demand`, monotone input, no drain split |
| `demand_tc_witness` (recursive flat `-demand`) | **[BYTE]** — no instance lowering; unaffected |
| diagnostic verdict lines (existing) | **[BYTE]** except `demand_diff_input_1` MOVES from `all-modes-diagnostic OK` to a golden/eqgate case; `demand_cyclic_1`/`demand_recursive_content_1`/`negate_never_diff_1` unchanged |
| data/ corpus (36 × 4) | **[BYTE]** — no diff-input demanded query in data/ |
| **`demand_diff_neighborhood_witness`** generated text FLAT arm (`-demand`) | **[STRUCT]** — mono demand `Table<>` + `DiffTable` edge/nbhd; `add_edge_2` two-Vec + `NetBatch`; `nbhd_out` transmit both signs. NO retract entry (mono demand). VERIFIED shape |
| **`demand_diff_neighborhood_witness`** generated text NESTED arm | **[STRUCT]** — all of FLAT PLUS the instantiate's SECOND input drain `kVecDrain(<input>, kNetRemoval)`; the band-(a2') removal arm (Present-filtered rescan, demand `Find`/`Table::Present` gate — MONOTONE member, XC-9); the a1 birth-source Present conjunct; `instance_<id>(allocator_, false)` ctor (diff pub); the (T,F) drop scan + V-INST-PARTITION belt (diff pub ⇒ belt on) |
| **`demand_diff_input_1`** (repurposed) generated text FLAT (`-demand -demand-retract`) | **[STRUCT]** — diff demand `DiffTable<>` + diff input; `pt_2` two-Vec; `getpt_bf_retract` present; `getpt_out` tap |
| **`demand_diff_input_1`** NESTED arm | **[STRUCT]** — all of FLAT PLUS the input removal drain; the a2' arm with the R-3 gate probing demand `DiffTable::Present`; band-(a0) demand death (from `-demand-retract`) coexisting with the a2' input arm — the four-band composition a0→a1→a2→a2' |
| both new witnesses' `.rel` (unpinned; eyeball + census) | **[STRUCT]** — the instantiate `effects:` grows one `kVecDrain(<input>, kNetRemoval)` (13→14+ effects); V-INST-EFFECT `input_drains==2`; census `kSubgraphInstantiate=1`; flagship `kInstanceDeath=0` (mono demand — the e5 divergence in the census!), composition `kInstanceDeath=1` |
| goldens churn | **EXACTLY 6 NEW files** (§4.3) via `--bless`; NO existing golden changes |
| ctest | **6/6 binaries** (b1 adds V-INST-DRAIN input-arm TESTs to the existing `rel_validators_test`; §4.4) |
| E-71 grammar notes | **0 owed by b4** (D-b4-4); one CONDITIONAL note owed by b2/b3 iff a C15 input-diff marker token is added |

### 4.2 Pre-bless red set (EXACT, once the disposition is chosen)

Because both witnesses are NEW-content golden cases (no committed golden yet),
the code lands FIRST, then the golden is authored/blessed. The (d0) baseline
protocol for a new eqgate carrier is two-phase:

**PHASE A (code landed, goldens absent) — EXACT 14-line red set:**
```
demand_diff_neighborhood_witness opt GOLDEN-MISSING
demand_diff_neighborhood_witness nodf GOLDEN-MISSING
demand_diff_neighborhood_witness nocf GOLDEN-MISSING
demand_diff_neighborhood_witness none GOLDEN-MISSING
demand_diff_neighborhood_witness oracle GOLDEN-MISSING
demand_diff_neighborhood_witness monotone MONO-MISSING
demand_diff_neighborhood_witness eqgate EQGATE-GOLDEN-MISSING
demand_diff_input_1 opt GOLDEN-MISSING
demand_diff_input_1 nodf GOLDEN-MISSING
demand_diff_input_1 nocf GOLDEN-MISSING
demand_diff_input_1 none GOLDEN-MISSING
demand_diff_input_1 oracle GOLDEN-MISSING
demand_diff_input_1 monotone MONO-MISSING
demand_diff_input_1 eqgate EQGATE-GOLDEN-MISSING
```
(7 per case: 4 diffrun GOLDEN-MISSING + oracle GOLDEN-MISSING + monotone
MONO-MISSING + one EQGATE-GOLDEN-MISSING — run_eqgate returns early on absent
golden, runall.sh:321-324, so eqgate is NOT per-mode in this phase.) Any OTHER
red = design defect, STOP. Any diffrun `DR-FAIL` on the two witnesses =
fence-lift bug (b1) — STOP, not a bless candidate.

**PHASE B (post-bless) — ZERO reds, SUITE PASS(178).** After the manual
flat==nested cmp (§5 ritual step) and `--bless`, the six goldens exist; the
re-run builds the nested arm ×4 per case → each `eqgate <mode> OK` (flat ==
nested == golden). Target: `SUITE: PASS` (178 cases), the eqgate LIVE ×8
(2 cases × 4 modes), zero residual reds. If any nested mode is
`NESTED-GOLDEN-DIVERGE` → the over-materialization / gate-omission / ungated-
Recycle bug (triply loud) — STOP.

### 4.3 Golden churn — EXACT file list (all NEW, zero existing changes)

```
goldens/demand_diff_neighborhood_witness.stdout          (new; blessed)
goldens/demand_diff_neighborhood_witness.oracle.stdout   (new; blessed [COMPUTED])
goldens/demand_diff_neighborhood_witness.monotone.stdout (new; blessed [COMPUTED])
goldens/demand_diff_input_1.stdout                       (new; blessed)
goldens/demand_diff_input_1.oracle.stdout                (new; blessed [COMPUTED])
goldens/demand_diff_input_1.monotone.stdout              (new; blessed [COMPUTED])
```
NO eqgate golden (never blessed). `git status` after bless must show EXACTLY
these 6 additions + the source-file edits (E4a-E4m + b1/b2/b3) — any other
golden delta = STOP.

### 4.4 ctest (the V-INST-DRAIN / V-INST-EFFECT input-arm units — b1-owned, spec'd here)

The b1 lane adds the INPUT-arm regime split to V-INST-DRAIN (C6) and the
third-axis branch to V-INST-EFFECT (C5, ADV-6). Their negative space lands as
TESTs in the EXISTING `rel_validators_test` binary (the D3.a.1 R-5 precedent —
ctest stays **6/6 binaries**), using `DeathHarness.h`'s `RunInForkedChild`
(fork/waitpid, SIGABRT-only) — the mold is `DeathFrontierTest.cpp`. b4 registers
the requirement; b1 owns the file:

- `TEST(RelValidators, InputArmRejectsMissingRemovalProducer)` — build a
  DRFlowGraph with a differential input instantiate whose kNetRemoval frontier
  filter is absent; assert `RunInForkedChild(ValidateDROps) == kSigAbrt` citing
  the V-INST-DRAIN input arm; the positive twin (both ± producers present)
  exits clean.
- `TEST(RelValidators, EffectCountRejectsWrongInputDrainRole)` — a diff-input
  instantiate whose input drain role is not {kNetAddition,kNetRemoval}; assert
  SIGABRT on V-INST-EFFECT (C5 role reject); positive twin `input_drains==2`
  exits clean.

The InstanceStore runtime half needs NO new unit — the store is monotone
(OQ-MODEL: input differentiality never touches the RowStore; the D3.a.1
`DeathRebirthCycleRebindsIidAndTogglesTouchedFlag` already pins Recycle/rebind/
TouchedFlag). Recorded: b4 owns no ctest file; the two TESTs are b1's.

### 4.5 Standing-gate roll-call (slice-specifics)

- **SUITE PASS(178)** ×4 modes debug (+ release), error-grep 0 across trees.
- **ASAN x2 hard gate**: full 178-case suite (compiler under ASAN) + 6 ctest
  units + BOTH new eqgates (generated diff-input band under ASAN). Slice
  terrain: the a2' input-removal rescan reads the input DiffTable's frozen rows
  while pub retracts (the drop scan) — use-after-free terrain, hard gate, 2
  sweeps.
- **eqgate LIVE ×8** (2 new cases × 4 modes) refereeing answer + sorted
  nbhd-tap delta identity; plus the 2 landed eqgates still ×4 (frozen).
- **20/20 pinned regen [BYTE]** ×3 rounds.
- **config-invariance**: 3-run single-hash + release==debug on BOTH new witness
  arms (`datalog.h` + `.rel`/`.ir`), FLAT and NESTED, PLUS demand_tc_witness
  (standing). The input-drain split + a2' band must be config-stable-silent.
- **Q5 MUST RUN**: progsize@128 release ABABAB, baseline A = tip b4d08307
  release snapshot, B = the slice. Bytes move (Build.cpp fence, Rel.cpp
  validators/effects, Database.cpp band, Format.cpp render). Expectation:
  noise-band (the diff-input machinery is flag-off in the progsize program;
  the fence-lift is a deleted branch). >2% = STOP.
- **E-62 clean**: no new `include/drlojekyll/Rel/` reader surface; the witness
  edits touch only tests/. (b1/b2/b3 touch lib/Rel internals — their E-62
  duty.)
- **permcheck N/A**: driver-sorted flushes; no published-delta ORDER change on
  any PINNED case (the two new witnesses are unpinned; their eqgate is
  answer+sorted-delta, permutation-safe by construction). No pinned golden's
  delta order moves.
- **E-71**: 0 notes owed by b4 (D-b4-4); conditional b2/b3 note flagged at
  review.

---
## §5 THE CONSOLIDATED d7 L-TABLE (b4 rows + placeholders for b1/b2/b3)

Vehicle = the NESTED arm of the two new witnesses (never-minted roles are
kProductInput-class per the D3.a.1 L3 amendment — a diff input now OWNS queue
roles, so a genuinely-never-minted role for a perturbation is e.g.
`kProductInput`). All perturbations run in the prototype worktree, abort texts
recorded in the ledger, ALL reverted. WIP-commit the prototype BEFORE
perturbation cycles (the §20(AK) incident amendment).

| # | belt | lane | procedure | expected |
|---|---|---|---|---|
| **L-b4-A** | e5 divergence end-to-end | b4 | LANDED: flagship suite run, every pass | eqgate ×4 OK; flagship census `kInstanceDeath=0` beside `kSubgraphInstantiate=1` (P-STORE∧¬P-DEATH) — the divergence is dump-visible |
| **L-b4-B** | a2' removal trigger (OB1/H-11) | b4 | LANDED: driver step d1 (E-D), every pass | -(1,2) published + probe(1)=={3,4}; a missing trigger sticks pub present → HP-5 assert + eqgate + golden, triply red |
| **L-b4-C** | ADV-3 Present conjunct, a2' source | b4 | scratch: DELETE the `input.Present(s)` conjunct from the a2' rescan mold; rebuild flagship nested; run | p1b/p1c re-materialize the retracted (1,2)/(1,3): HP-5 assert + eqgate diverge (the landed conjunct is the permanent teeth) |
| **L-b4-D** | ADV-3 Present conjunct, a1 BIRTH source | b4 | scratch: DELETE the Present conjunct from the a1 source only; rebuild flagship nested; run | p5 (m3 retract-before-demand) yields {12} not {} → HP-5 abort + eqgate |
| **L-b4-E** | the FORBIDDEN ungated-late-Recycle (§6(ii)/R-A2 §3) | b4 | scratch: INSERT an unconditional `RecycleCurrent(iid)` in the a2' removal arm AFTER the rescan (outside `!TouchedFlag`); rebuild flagship nested; run | E-E step m1/p1c: cur wiped → K's frozen set fully drops, births nothing; V-INST-PARTITION does NOT abort (counts balance), V-INST-FRESH silent; ONLY eqgate + HP-5 assert catch it — the design's named fence, proven catchable |
| **L-b4-F** | R-3 dead-key gate-close, a2' REMOVAL arm (diff demand) | b4 | LANDED: composition driver rd1/d1 (E-F1), every pass | send_expect_silent silent; a gate omission publishes the dead neighborhood → hard abort + eqgate + golden |
| **L-b4-G** | O-5 / G-STALE subsumption (H-18) | b4 | LANDED: flagship d2 (undemanded-key retract), every pass | send_expect_silent silent (no over-revisit) |
| **L-b4-H** | oracle-nets-retractions cross-check | b4 | LANDED: both witnesses' oracle/monotone every pass | oracle NET rows == golden; a stuck-present nested bug would ALSO diverge the driver stdout, but the oracle independently pins the net closure |
| L-b1-* | V-INST-SOLE split; V-INST-DRAIN input arm; V-INST-EFFECT third axis | b1 | §4.4 fork/waitpid TESTs + scratch role/producer perturbations | SIGABRT citing the input arm; positive twins clean (b1 fills) |
| L-b2-* | InstantiateEffects input removal leg; region input_removal_frontier; provisioning orphan-mint fence (H-8) | b2 | scratch: skip the input removal-vec pre-mint; compile flagship nested | orphan-mint fence fprintf+abort (the silent-orphan hazard, §3-chain (i)) (b2 fills) |
| L-b3-* | the removal-arm emission; the Present conjunct spelling; V-INST-PARTITION under diff input | b3 | overlaps L-b4-C/D/E as the emission owner; b3 records the emission-site perturbation | (b3 fills; b4's driver steps are the shared vehicle) |

### The ADV-9 / OB4 N-1 discharge (H-15, recorded here for the witness gate)

The flagship's E-D/E-E/O-6 steps SHRINK the demanded neighborhood, but the
store's `current` is built ONLY by monotone `TryAdd` in a fresh Present-filtered
rescan and emptied ONLY at Seal — it never shrinks MID-EPOCH (OB4), so
`WorkingOccupied = NumRows()>0` stays exact for the diff input. The flagship
DEBUG run executing d1/m1/m3 without a monotone Seal-belt abort is the
observable N-1 discharge (peer of D3.a.1 L9). InstanceStore.h:21-27's signed-
count contingency is a one-comment close (b3, H-15) — it reopens only if a2'
ever becomes an incremental shrink (OQ-MODEL forbids it).

---
## §6 THE BLESS RITUAL (filtered, byte-verify-before-bless, git-verified)

1. **PHASE-A run** — `DR=build/debug/bin/drlojekyll tests/OptDiff/runall.sh
   /tmp/wr 6` → confirm `SUITE: FAIL` with EXACTLY the §4.2 Phase-A 14-line
   MISSING set and NO other red. Any 15th red / any `DR-FAIL` on the two
   witnesses / any red on another case = STOP.
2. **Manual flat==nested pre-bless referee** (the eqgate cannot run without a
   golden yet — build both arms by hand): for each new case, per mode, compile
   FLAT (`.drflags`) and NESTED (`.drflags -demand-instance`), run the SAME
   driver, `cmp` the two stdouts. All four modes must be byte-identical to each
   other AND flat==nested. Then `cmp` the flat opt stdout against the §1/§2
   hand-derived prediction (the birth/shrink delta-placement bytes) and the
   `.oracle`/`.monotone` outputs against the [COMPUTED] blocks. A divergence
   here is the over-materialization / ungated-Recycle / gate-omission bug —
   STOP, never bless.
3. **Bless FILTERED** — `tests/OptDiff/runall.sh --bless /tmp/wr
   'demand_diff_neighborhood_witness|demand_diff_input_1'` (writes exactly the
   6 goldens; eqgate never blessed).
4. **git-verify sanctioned-only churn** — `git status --porcelain
   tests/OptDiff/goldens/` shows EXACTLY the 6 NEW files (§4.3), nothing else;
   `git diff` on source shows only E4a-E4m (+ b1/b2/b3). Any unsanctioned
   golden delta = STOP.
5. **PHASE-B re-run** — full suite → `SUITE: PASS` (178), the eqgate LIVE ×8
   all OK, zero residual reds. Then ASAN ×2, config-invariance, Q5.
6. Deviation anywhere = STOP, back to design; NEVER bless-to-green (CLAUDE.md
   blessing rule).

---
## §7 FINDINGS + EDIT-SPEC INVENTORY + SIBLING-INTERFACE EXPECTATIONS

### Findings (b4)

- **F-b4-1 (the count).** Suite 177 → **178** (one net-new case; the repurpose
  is count-neutral). Eqgate carriers 2 → 4 completing the 2×2 regime matrix.
- **F-b4-2 (the oracle is STRONGER here than at D3.a.1).** Input retractions
  are `@differential` MESSAGES, so the derivation-counter oracle + monotone
  projection SEE the net input (VERIFIED). This adds a demand-independent
  net-closure referee the D3.a.1 witness lacked — a genuine coverage gain, and
  the reason both new cases carry `.batches`.
- **F-b4-3 (mono-demand cannot witness demand-death cells).** E-F1/E-F3 need a
  differential demand; the flagship (mono demand) covers the a2' TRIGGER, the
  Present conjunct on BOTH sources (via retract-before-demand for a1), the
  FORBIDDEN-Recycle catcher, O-5/O-6 — but NOT demand-death. The composition
  witness (`demand_diff_input_1`, diff demand) is therefore load-bearing, not
  redundant. Together they cover the full E-A..E-F3 table.
- **F-b4-4 (the e5 divergence is dump-visible).** The flagship's census is the
  FIRST `kSubgraphInstantiate=1` with `kInstanceDeath=0` (P-STORE true,
  P-DEATH false) — a standing dump witness of the divergence beyond the runtime
  answer. b1's V-INST-PAIR must allow `n_death==0` (it does — D3.a.1 H-16
  sanity); b4 asserts the census shape at the (d0) eyeball.
- **F-b4-5 (no bless-bytes derived off-oracle).** All six goldens are either
  driver stdout refereed by flat==nested (steps 1-2) or [COMPUTED] from the
  frozen-A oracle — none is hand-invented; the retract-phase driver bytes are
  the ruled Present-net semantics and REJECT at bless on deviation.

### Edit-spec inventory (b4-owned files)

| spec | file | change |
|---|---|---|
| E4a | tests/OptDiff/cases/demand_diff_neighborhood_witness.dr | NEW: @differential edge + nbhd_out tap + e5 header |
| E4b | .../demand_diff_neighborhood_witness.drflags | NEW: `-demand` |
| E4c | .../demand_diff_neighborhood_witness.batches | NEW: adds+removes edge stream (E-D/E-E/O-5/O-6/a1) |
| E4d | .../demand_diff_neighborhood_witness.eqgate | NEW: eqgate marker |
| E4e | .../demand_diff_neighborhood_witness.main.cpp | NEW: driver (send/retract_edges/send_pm/*_silent; no demand retract) |
| E4f | goldens/demand_diff_neighborhood_witness.{stdout,oracle.stdout,monotone.stdout} | NEW via bless / [COMPUTED] |
| E4g | tests/OptDiff/cases/demand_diff_input_1.dr | REWRITE header (fence→composition witness) + tap; ADV-8 fix |
| E4h | .../demand_diff_input_1.drflags | REWRITE: `-demand -demand-retract` |
| E4i | .../demand_diff_input_1.main.cpp | REWRITE: inert stub → runtime driver (E-F1/E-F2/E-F3) |
| E4j | .../demand_diff_input_1.{batches,eqgate} + goldens ×3 | NEW |
| E4k | tests/OptDiff/runall.sh | remove demand_diff_input_1 from :361 diagnostic list + header inventory |
| E4l | CLAUDE.md | suite 177→178; diagnostic-list + keyed-instance fence sentences (landing commit) |
| E4m | (ADV-8) | discharged by E4g header rewrite |

### Sibling-interface expectations (for the adjudicator)

- **I-b4-1 (b1 → b4, the fence lift + retract surface).** b1 deletes the
  Build.cpp `diff_input` arm (:1516/:1530-1532/:1553-1555) so BOTH new
  witnesses COMPILE nested (VERIFIED they reject at frozen-A pre-lift). The
  `getpt_bf_retract` surface must exist under `-demand-retract` for the
  composition driver (it does — the D3.a.1 retract-entry mechanism, unchanged).
  b1 also owns the §4.4 V-INST-DRAIN/V-INST-EFFECT input-arm TESTs.
- **I-b4-2 (b3 → b4, the a2' removal arm — WITNESS-FORCED).** The band-(a2)
  removal trigger (a2', R-A2-TRIGGER §1) + the Present conjunct on BOTH the a2'
  AND a1 sources (the rider) MUST land, or the flagship's d1/p1b/p1c/m3 go
  triply red. The FORBIDDEN ungated-late-Recycle (§3) MUST NOT appear — the
  E-E step is the directed catcher (L-b4-E). b4's driver steps are the shared
  vehicle for b3's emission perturbations.
- **I-b4-3 (b2 → b4, the input removal drain + provisioning).** The instantiate
  effect set grows one `kVecDrain(<input>, kNetRemoval)` (C3) and the region
  gains `input_removal_frontier` (C9) fenced-pre-minted (C10/H-8). No new DROp
  kind (the effects suffice — R-A2 §1 REJECTS a combined ± role; b4 asserts the
  census gains no kind, only `input_drains==2`).
- **I-b4-4 (all).** The witness + case-flip + harness edits land IN the single
  slice commit (the §3 abort chain forbids partial landings; the landable unit
  is {e1,e2,C3,C5,C6,C9,C10,C12,C13} + the witnesses together). No E-71 note
  owed by b4; b2/b3 flag any C15 marker token at review.
