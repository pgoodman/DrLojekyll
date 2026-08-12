# Session 29 charter — REOPEN demand: make a bound query materialize only what it needs

> **You have earned this moment, and you are ready for it.** The last session did the hard, honest thing
> — it hunted for a shortcut, found none, and refused to dress a shadow as a win. That integrity is
> exactly why the path is now clear and *trustworthy*. This session you get to build the real thing: the
> feature this whole branch is named for. When you're done, a bound `#query` over a big relation will
> compute a small fraction of the rows it computes today — the generated database will do measurably less
> work, and every answer will be identical. That is the greenfield vision made concrete, and you are the
> right one to bring it about. Be rigorous, follow the evidence, and take real pride in it. We believe in
> you — go make it real.

You are resuming the **keyed-instance greenfield rewrite** on branch `keyed-instances` (Dr. Lojekyll, the
`hyde` C++ Datalog compiler). Landed: P1–P7 + P7b + P7c + P6.3-detection + F16-close (OptDiff **SUITE:
PASS (227)**, ctest **5/5**). Tip `05af6595` + the session-28 docs.

## THE MANDATE (owner, non-negotiable): DO SOMETHING REAL
Session 28 triaged the "make the model drive codegen" arc to an honest pause: the landed compile-time
analyses (P6.1–P6.3+F16) are trustworthy but **consumer-less, because the generated runtime is already
well-optimized for the current corpus** — the P7 arc harvested the seeks; the rest of the cost is
intrinsic to semi-naive differential join. The refutation was unanimous (3-refuter opus panel + IR/C++
dumps): **no minimal codegen-shape win exists on the current runtime.** So the owner redirected to the
lever that changes a GIVEN — the **workload semantics: reopen DEMAND / keyed evaluation** (the branch's
namesake, deleted at the P1 cut). Full record: `session-28-triage-decision.md`.

**The next landed cut must DO SOMETHING REAL, and here "real" is sharp and achievable:** a bound `#query`
must materialize ONLY the rows its bound value demands, instead of fully materializing the relation and
then seeking (today's P7 path). The exit gate (all four, non-negotiable):
1. **Result-equality** — the bound query's ANSWER SET is byte-identical to today's M3 across all 4 opt
   modes (oracle/behavioral/I0-RefInterp nets). The DERIVATION/fold count differs BY DESIGN — that is the
   win. (Owner ruling: changing answers-cost is OK; changing the answer SET is a miscompile.)
2. **Codegen goldens MOVE** — `.rel`/`.ir`/`.h` change (the demand-seeded materialization genuinely
   differs from full-materialize).
3. **Bench-measured pruning** — a NEW directed carrier (a large `s`/`t` with a SELECTIVE bound `A`) shows
   fewer derived-rows/folds/finds under demand vs M3, via `bench/` (`-O2 -DNDEBUG`; never time bench
   concurrently with the suite). Judge across the `.cost` SCENARIO FAMILY (memory `cost-scenario-family`):
   demand must not REGRESS the non-selective case, or be cost-gated off it.
4. **ctest 5/5, OptDiff SUITE PASS** — the non-demand corpus stays byte-identical (demand is mode-gated
   OFF / cost-gated, orthogonal to the 4 golden modes, exactly as the old `-demand` was).
**A cut that lands demand infrastructure WITHOUT a bench-measured pruning on a carrier is a shadow and
FAILS the mandate.** The bench carrier + its measured win ship IN the same cut.

**FIRST-RISK (settle before building — the whole-program seed §0/§5-O1):** the recovered cost model
(`recursive-demand-seed.md` §7) warns that demand pays only when `pruned_derivation > machinery_cost`
(≥3 extra tables + guard joins), that a *message-rooted trivially-derived* relation NEVER pays, and that
*recursive* demand is where pruning is asymptotic. The confirmed non-recursive slice (`r:-s,t`) is a JOIN,
so it CAN prune (bound A restricts s) — but only in a **selective-query regime** (large relation, few
distinct queried A). So the spike's FIRST job is to ENGINEER the carrier in the paying regime and MEASURE
that the non-recursive pruning actually beats the machinery. If it can't, say so plainly and put "pull the
recursive case (S4, asymptotic pruning) forward vs re-scope the carrier" to the owner — do not force a
non-paying slice to satisfy the gate.

## Owner decisions RATIFIED at the session-28 close (execute on these)
- **Architecture fork → LET THE SESSION-29 SPIKE DECIDE.** Ground BOTH and choose with evidence:
  - **Path R (resurrect + adapt):** bring back the deleted `Demand.cpp` magic-sets transform + the keyed
    `InstanceStore` lowering from git `48cd0a4f`, adapt to the current Rel IR + typed foundation. Fast
    (proven code) but re-adds the mode-gated `-demand` DataFlow rewrite the greenfield cut removed.
  - **Path N (native rebuild):** demand becomes a property of the request edge (P3/P7 already carry the
    bound query); drive a keyed, demand-seeded materialization natively on the request-port/AccessPlan
    foundation — no fabricated `demand__` messages, no mode-gated DataFlow rewrite. Aligned with the
    greenfield goal; larger first slice.
  - The spike costs BOTH to first-win distance, then commits — not a priori.
- **First slice → CONFIRMED (non-recursive, single-adornment):**
  ```
  #query lookup(bound u64 A, free u64 C) : r(A, C).
  r(A, C) : s(A, B), t(B, C).      ; bound A prunes s to s(A,·) → far fewer join outputs
  ```
  Recursive demand stays FENCED in slice 1 (it was deferred in the old impl — `OQ-INDUCTION-UNION`).
- **Constraint → CHANGING ANSWERS-COST OK** (result-equal, not byte-equal derivation).

## Read first (resume authority, in order)
1. **`session-29-whole-program-seed.md`** — START HERE. The code-grounded whole-program pseudocode: §A the
   current bound-`#query` full-materialize+seek path (Regional request-port → ControlFlow Build →
   codegen seek); §B where the relation fully materializes (the eager descent + fixpoint that run
   unconditionally); §C the deleted demand machinery recovered (the magic-sets transform + the keyed
   InstanceStore lowering); §D the path forward as DIFFS on that pseudocode, both Path R and Path N; §E
   the exit gate + IR desired-state sketch. Re-verify every anchor at tip — the pipeline drifts each session.
2. **`session-29-demand-reopen-seed.md`** — the scoping seed: the fork, the first slice, the multi-session
   roadmap (S1 this cut → S2 multi-adornment → S3 differential input → S4 recursive demand → S5 @key
   activation).
3. **`session-28-triage-decision.md`** — why the model-drives-codegen minimal cut is refuted (so you don't
   re-hunt it) + the deeper "consumer-less because the runtime is already optimized" truth.
4. The recovered design corpus: `recursive-demand-seed.md` (§1 two-lowering frame, §2 InstanceStore
   pseudocode, §7 cost model); `DemandSeeds.md`, `SubgraphsDemand.md`, `KeyedInstances.md` (the pre-P1
   demand design of record). memory `demand-cost-model` (when demand pays), `demand-subgraph-unification`
   (flat-vs-nested = two lowerings), `regional-dataflow-core-epoch` (s28 banner at head),
   `cost-scenario-family`, `perf-guiding-oracles`.
5. **`bench/BASELINE.md` + `bench/README.md`** — the COST instrument that enforces gate (3). The demand
   carrier is NEW; you author it (a large selective `s`/`t`). Never time bench concurrently with the suite.

## Method — the grounding loop, run via WORKFLOWS with opus + sonnet (the loop that landed P2–F16)
Thin orchestrator; sequential single-phase workflows; watch `(await parallel(...)).filter(...)` precedence.
DOCS-ONLY until the owner green-lights execution. Model tiering (memory `subagent-model-tiering`):
**sonnet** = anchor re-verification, baseline IR/C++ dumps, mechanical code-reads, bench baselines;
**opus** = the R-vs-N spike judgment, design diffs, the refuter panel, IR desired-states. Produce, in order:

1. **Build out the AS-IS pseudocode** — extend §A/§B/§C of the whole-program seed to full fidelity;
   re-verify EVERY anchor at tip (sonnet reads; the seed is a starting point, not gospel).
2. **The R-vs-N spike (settle the fork FIRST)** — a throwaway-worktree spike that stands up the SIMPLEST
   demand seed BOTH ways on the confirmed first slice, measures the first-win distance (effort + does the
   materialization actually prune + does the answer stay result-equal), and commits to one architecture
   with evidence. This is itself the first grounding deliverable — do not build blind.
3. **Design-goal DIFFS at hunk grain** on the pseudocode (opus) — the chosen architecture's real cut: the
   demand seed + the guard/keyed-materialization + the bench carrier. Each diff carries a DISCRIMINATING
   exit gate that INCLUDES the bench pruning delta + result-equality + codegen-golden move.
4. **Critique adversarially** (opus 3-refuter panel) — and **VERIFY EMPIRICALLY** (throwaway-worktree
   spike: apply, build, run the FULL suite for result-invariance, run `bench/` for the pruning delta). The
   refuters must confirm the cut DOES SOMETHING REAL (codegen moves + bench pruning), is RESULT-EQUAL (the
   answer set is provably unchanged — this is the FIRST answer-set gate; treat a divergence as a
   miscompile, adjudicate per the suite's REFINTERP-DISAGREE discipline), and is MINIMAL (non-recursive,
   single-adornment — no drag-in of multi-adornment/differential/recursive demand).
5. **IR desired output states** (predict-then-verify) — build out the exact `.rel`/`.ir`/`.h` BEFORE/AFTER
   on the carrier, formulate them as diffs, critique them, decide precisely which goldens MOVE and the
   bench-carrier's measured pruning. This is the predict half of predict-then-verify (memory
   `predict-then-verify-ir`).

Keep the orchestrator thin. Present the design + exit gate and **STOP for the execution go/no-go.**

On green-light: execute as one coherent commit (SUITE PASS, ctest 5/5, result goldens byte-equal ×4 modes,
codegen goldens re-blessed after review, bench pruning recorded per `bench/BASELINE.md`), then update
CLAUDE.md + memory + write the session-30 seed.

## Gotchas (carried)
- clangd diagnostics are NOISE (no include paths) — trust the real build only.
- macOS bash 3.2 / zsh word-splitting: use `${=var}` when a var holds multiple CLI args. `.dr` ASCII-only.
- Run builds/suite SILENT on success (memory `silent-tests-save-tokens`); the full OptDiff suite takes
  ~3–4 min — run BACKGROUNDED, await. Never rebuild the compiler mid-bench-run.
- `runall.sh --bless <workroot> [filter]` takes NO jobs arg (PROMOTES from an existing workroot; run the
  suite into it FIRST, with any new `.irgold` step added). Bless ONLY after reviewing the delta; never
  through a symlink golden; never to make a red case green. A new carrier needs `.dr`+`.main.cpp`(+
  `.irgold` for codegen goldens); a `.batches` sidecar adds the oracle/monotone/behavioral answer nets.
- The result-equality gate is the FIRST answer-set gate in this arc — a demand lowering that changes an
  ANSWER (not just its derivation cost) is a MISCOMPILE. Gate hard: oracle + behavioral + I0 RefInterp
  across all 4 modes, on BOTH the demand carrier and the whole (byte-identical) non-demand corpus.
- If the R-vs-N spike shows the first real win is further than one coherent commit, SAY SO and put the
  slice-scope decision to the owner — do not land infrastructure without a measured pruning.

## A closing word
The discipline that got this project here — predict-then-verify, adversarial critique, honest
value-accounting, never blessing a shadow — is exactly what will land this. You already proved that
discipline last session by refusing to fake a win; now you get to spend it on a win that is genuinely
there. When the bench harness shows a selective query touching a fraction of the rows and every answer
golden stays byte-identical, that is the namesake of this whole branch, finally rebuilt on the typed
foundation — the model earning its keep, the generated database measurably better, correctness never in
doubt. You have the map, the tools, and the judgment. Make it real — and enjoy it. We're rooting for you.
