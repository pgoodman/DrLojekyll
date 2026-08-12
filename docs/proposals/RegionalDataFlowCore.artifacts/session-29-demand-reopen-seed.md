# Session-29 seed — REOPEN demand / keyed evaluation (the branch's namesake)

> Cold-start START-HERE for session 29. Branch `keyed-instances`, tip `05af6595` + the session-28
> triage-decision doc (`session-28-triage-decision.md`). **Session-28 outcome:** the "DO SOMETHING
> REAL" minimal-cut hunt was triaged and refuted unanimously (C1 fused-fixpoint DEAD, C2/WCOJ
> multi-session, C3 no removable redundancy — the dominant hash-probe cost is intrinsic to semi-naive
> differential join). **Owner decision (session 28):** direction = **reopen demand/keyed evaluation
> (option c)**; constraint = **changing answers (derivation cost) is ACCEPTABLE** (result-equal, not
> byte-equal derivation). This seed scopes that arc's first slice and the architectural fork the
> session-29 grounding must settle FIRST. DOCS-ONLY; nothing built.

## §0 Why this is the right "something real"
The four landed compile-time analyses (P6.1–P6.3 + F16) are trustworthy but consumer-less because the
generated runtime is already well-optimized for the corpus it targets (P7 harvested the seeks; the rest
is intrinsic). The only way to make the model do REAL product work is to change a given: the runtime
(C2/P8 — narrow, refuted as the next minimal cut) or the **workload semantics** — reopen demand so a
bound `#query` materializes ONLY the rows it needs instead of fully materializing the relation and then
seeking (today's P7 path). Demand is the branch's namesake (`keyed-instances`) and the deleted-at-P1
payoff. A selective bound query over a large derived relation touches a small fraction of the
derivations — a genuine, directed, bench-measurable win, with the query ANSWER identical (result-equal).

## §1 What was deleted at P1 (recovered from git `48cd0a4f` + the seed docs)
Demand lowered TWO ways (the load-bearing frame — recursive-demand-seed.md §1):
```
parse → DATA-FLOW ──────→ Rel (DR-IR) ──────→ control-flow → C++
          │                   │
   magic-sets TRANSFORM   instance LOWERING selector
   (Demand.cpp, -demand)  (-demand-instance)
```
- **The TRANSFORM** (`QueryImpl::ApplyDemandTransform`, `lib/DataFlow/Demand.cpp`, DELETED): a SIP walk
  propagates the query's bound adornment α backward through relation `p`'s rule bodies, FABRICATES a
  demand message `demand__<q>_<α>` + a `#local` demand relation `d_p`, mints `d_p` (root = the fabricated
  receive; one propagation member per demanding subgoal), and GUARDS each rule at its SIP site with a
  JOIN `d_p ⋈ p` (a real column-edge 1→N-pivot join). Plus a query-projection raw-seed guard. Mode-gated
  by `-demand` (orthogonal to the 4 opt modes). Single-adornment slice; everything else clean-diagnostic.
- **The INSTANCE LOWERING** (`-demand-instance`, control-flow selector, DELETED): recognizes the demanded
  subgraph and mints ONE `kSubgraphInstantiate` per live forcing, emitting a keyed `InstanceStore<Key=α,
  Row=nested-cols>` (band-a1 BIRTH on demand-arrival, band-a2 REBUILD on input-arrival, band-b PUBLISH
  the born rows) instead of the flat guard-join web. Answer-identical to flat `-demand`.
Both were mode-gated OFF and orthogonal to the golden suite; `@key` was the flagless force-activation.

## §2 The current (post-P7) bound-`#query` path — the as-is baseline
Today a bound `#query` over `p` FULLY MATERIALIZES `p` during evaluation (the fixpoint/joins run to
completion for ALL keys), then P7 answers via a `partial-key-hash-seek` (`Index::First/Next`) on the
bound columns (`RequestPortRecord{plan: AccessPlan}`, `frozen->PlanFor(decl)` at `Build.cpp:447/504`).
So the SEEK is cheap but the MATERIALIZATION is unpruned — demand's win is entirely in the materialization
it avoids. The typed foundation ALREADY knows the bound query: the **request port** (`RouteKind::
kRequestPort`, P3) carries the query + bound adornment; the `AccessPlan` selects the physical seek. What
is missing is the backward propagation of the bound value into the *evaluation* so fewer rows are derived.

## §3 THE ARCHITECTURAL FORK (session-29 grounding must settle this BEFORE any code)
- **Path R — resurrect + adapt.** Bring back the deleted `Demand.cpp` magic-sets transform + the keyed
  `InstanceStore` lowering from git (`48cd0a4f`), adapt to the current Rel IR + typed foundation (the old
  lowering read `Rel.cpp:918/1035` etc., since heavily changed). PRO: proven code, fastest to a first
  win, the whole design corpus (DemandSeeds.md, SubgraphsDemand.md, KeyedInstances.md) still applies. CON:
  re-introduces the mode-gated `-demand` DataFlow transform the greenfield cut *deliberately* removed;
  fights the P1 motivation ("rebuild on the typed foundation, not resurrect").
- **Path N — native rebuild on the Regional/AccessPlan model.** Build keyed evaluation natively: the
  request port already carries the bound query; extend it (a `RecursiveEvaluationPlan`-style authority, or
  a demand-seed on the request edge) to DRIVE a demand-seeded, keyed materialization in the Rel/ControlFlow
  emission — no fabricated `demand__` messages, no mode-gated DataFlow rewrite. PRO: aligned with the
  greenfield goal + the typed foundation; demand becomes a first-class property of the request edge, not a
  bolt-on graph rewrite. CON: more design; the InstanceStore runtime + the band-a1/a2/b lifecycle still
  need porting; larger first slice.
- **Recommended:** Path N in spirit (the typed foundation is the whole point of the rewrite), but the
  session-29 grounding should HONESTLY cost both — Path R may be the pragmatic first-win that de-risks the
  runtime (InstanceStore) before the native re-plumb. The grounding loop decides with a spike, not a priori.

## §4 The minimal FIRST real slice (proposed — grounding to confirm/re-scope)
**Shape:** ONE bound `#query`, single adornment, over a NON-RECURSIVE derived relation whose body the
bound value prunes (recursive demand was FENCED/deferred in the old impl — `OQ-INDUCTION-UNION → DEFER`;
keep it fenced in slice 1). Canonical carrier:
```
#query lookup(bound u64 A, free u64 C) : r(A, C).
r(A, C) : s(A, B), t(B, C).        ; bound A prunes s to s(A,·) → far fewer join outputs
```
Without demand: `r` fully materializes (all A). With demand + a selective A: only r-rows for the bound A
are derived — the answer to `lookup` is identical, the derivation count drops ~1/|distinct A|.
**EXIT GATE (does-something-real, all four required):**
1. **Result-equality** — the bound query's answer set is byte-identical to the M3 full-materialize+seek
   path across all 4 opt modes (oracle/behavioral/I0-RefInterp nets; the DERIVATION count differs by
   design, the ANSWER does not — this is the "changing answers OK" boundary made precise: cost changes,
   result does not).
2. **Codegen goldens MOVE** — `.rel`/`.ir`/`.h` change (the demand-seeded materialization differs from
   full-materialize).
3. **Bench delta** — a NEW directed carrier (a large `s`/`t` with a selective bound `A`) shows fewer
   folds/finds/derived-rows under demand vs M3 (`bench/`, `-O2 -DNDEBUG`; per the `cost-scenario-family`
   memory, judge across selective AND non-selective A — demand must not REGRESS the non-selective case, or
   be cost-gated off it).
4. **ctest 5/5, OptDiff SUITE PASS** — the non-demand corpus stays byte-identical (mode-gated OFF, exactly
   as the old `-demand` was orthogonal to the 4 golden modes).

## §5 The multi-session roadmap (indicative; owner re-ranks each cut)
1. **S1 (this arc's first cut):** the §4 non-recursive single-adornment slice — the transform-or-native
   seed + guard + keyed materialization + the bench carrier proving the win. (Architecture per §3.)
2. **S2:** multi-adornment (N disjoint keyed stores over one pub — the old D3.a.3 shape).
3. **S3:** differential input (deletion-capable summarized input — old D3.a.2).
4. **S4:** recursive demand (the deferred `OQ-INDUCTION-UNION` — the largest gap; the
   `recursive-demand-seed.md` precondition `NeedsInductionCycleVector`).
5. **S5:** `@key` flagless force-activation re-attach + the RP-1..10 policy surface (currently inert
   parsed metadata).

## §6 What the session-29 grounding loop must produce (charter method)
1. **As-is pseudocode** — the current bound-`#query` full-materialize+seek path at hunk grain
   (Build.cpp/Rel emission/AccessPlan), re-verified at tip.
2. **The §3 fork resolved** — Path R vs Path N, decided by a throwaway spike costing BOTH (build the
   simplest seed both ways, measure the first-win distance).
3. **Design diffs** for the §4 slice on the chosen architecture, each with the §4 exit gate.
4. **Adversarial refuter panel** — is the win REAL (bench delta + codegen move), RESULT-EQUAL (no
   miscompile — the answer set is provably unchanged), and MINIMAL (non-recursive single-adornment, no
   drag-in of multi-adornment/differential/recursive)?
5. **IR desired states** (predict-then-verify) on the carrier + the exact goldens that MOVE.
Then present + STOP for the execution go/no-go.

## §7 Owner decisions (RATIFIED at the session-28 close — session 29 executes on these)
- **§3 fork → LET THE SPIKE DECIDE.** Session 29 grounds BOTH: a throwaway spike that costs Path R
  (resurrect+adapt `Demand.cpp` + the `InstanceStore` lowering from git `48cd0a4f`) AND Path N (native
  rebuild on the request-port/AccessPlan model) to the first-win distance, then commits to one WITH
  evidence — not a priori. Build the simplest seed both ways, measure the effort/first-win, choose.
- **§4 slice → CONFIRMED.** The non-recursive single-adornment `lookup(bound A,free C):-r(A,C);
  r(A,C):-s(A,B),t(B,C)` is the first carrier. Recursive demand stays FENCED in slice 1.
- **Constraint → CHANGING ANSWERS OK** (result-equal, not byte-equal derivation) — the exit gate's
  answer-equality is RESULT-set equality (§4.1), the derivation/fold count differs by design.
- **STILL OPEN (decide during session-29 grounding, not blocking):** the activation surface —
  re-introduce a mode-gated `-demand` flag (as before, orthogonal to the 4 opt modes) vs a cost-model-
  driven AUTOMATIC decision on the request edge (the typed-foundation way). The Path R/N spike will
  inform this (Path R comes with the flag; Path N invites the automatic form).

## §8 Session-29 START-HERE checklist
1. Re-verify every anchor at tip (the pipeline drifts each session): `Build.cpp:447/504` (PlanFor),
   the Rel emission path, the request-port record, `Demand.cpp` shape in git `48cd0a4f`.
2. Recover the old design corpus: `DemandSeeds.md`, `SubgraphsDemand.md`, `KeyedInstances.md`,
   `recursive-demand-seed.md` §1–§2 + §7 (cost model), memory `demand-cost-model`.
3. Run the grounding loop (§6): as-is pseudocode → the R/N spike (§3, decide the fork) → design diffs
   on the chosen arch → adversarial refuter panel → IR desired states. Present + STOP for go/no-go.
