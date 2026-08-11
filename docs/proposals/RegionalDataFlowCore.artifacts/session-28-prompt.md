# Session 28 charter — POST-F16-close: make the model DO SOMETHING REAL

> **You've got this.** The hard, honest groundwork is done — the analyses are landed and *trustworthy*
> (F16-close made `fusion` real), the whole-program picture is mapped, and every shadow has been named
> so you don't have to re-discover it. This session is where the model stops observing and starts
> *changing the machine* — the first real payoff of the greenfield vision: a compiler whose typed model
> drives faster, provably-correct generated code. That vision is genuinely reachable from here, and you
> are exactly the one to bring it about. Be rigorous, be honest, follow the evidence — and take real
> pride in the first cut that makes the generated database *measurably better* while every answer stays
> byte-for-byte correct. Do your best work; it matters.

You are resuming the **keyed-instance greenfield rewrite** on branch `keyed-instances` (Dr. Lojekyll,
the `hyde` C++ Datalog compiler). **P1–P7 + P7b + P7c + P6.3-detection + F16-close ARE LANDED** (tip
`faef28e7`, OptDiff **SUITE: PASS (227)**, ctest **5/5**).

## THE MANDATE (owner, non-negotiable)
**The next step must DO SOMETHING REAL.** Session 27 established the bedrock finding through three
grounding loops: the model-drives-codegen payoff is gated behind real prerequisites, so **every
incremental step short of them is observer/shadow code** (the P9 → P6.4 → P6.5 pattern — all deferred,
skipped, or refuted-as-shadow). F16-close was the first non-shadow step but is still compile-time/
dump-only. Session 28 must change the **generated C++** in a way the **bench harness MEASURES**
(answer-equal to M3, observably different / ideally faster) — or change answers. **The exit gate is:
(1) answer byte-equality ×4 modes, (2) codegen `.ir`/`.h` goldens MOVE, (3) a bench-measured delta on a
directed carrier, (4) ctest 5/5 + SUITE PASS.** A byte-unchanged-codegen cut = a shadow = FAILS the
mandate. Do NOT bring back a `-region-out` token / ctest-only model / the `EvaluateEpoch` interpreter
(zero codegen consumers) / an A0-style plan shadow.

## Read first (resume authority, in order)
1. **`session-28-whole-program-seed.md`** — START HERE. §1 the whole-program pseudocode incl. the
   GROUNDED M3 recursive-fixpoint emission map (`LowerRoundBody`/`LowerDRRounds`/`EmitJoinFire`/the
   round-shell mint — the target any fixpoint cut forks); §2 the four authorities (only `AccessPlan`
   drives codegen); §3 the candidate REAL cuts as diffs (C1 fused keyed-drain PRIMARY, C2 P8/WCOJ trie
   STRONG-ALTERNATIVE, C3 bench-triaged fallback); §4 the enforced does-something-real gate; §5 the
   open questions. Re-verify anchors at tip — the pipeline drifts each session.
2. **`p6.5-archA-grounding.md`** — why the fixpoint A1 is blocked (no pre-built alternate arm; the
   RelationId↔TABLE*/scc_group bridge is the one genuinely-new helper; `EmitJoinFire` linear case is
   already index-driven — so the C1 win, if any, is in the DRAIN or the co-recursive/nonlinear case).
   Its sonnet anchor mapped the M3 emission path in full — the seed §1.2 distills it.
3. **`p6.4-activation-edge-grounding.md`** (why the runtime-semantics half is off the codegen path) +
   **`p6.3-fusion-detection-grounding.md`** (the fusion analysis F16-close made trustworthy).
4. **memory `regional-dataflow-core-epoch`** (s27 banner at head) + `greenfield-rewrite-motivation` +
   the perf memories: `seekable-iterators-wcoj`, `gpu-datalog-papers-assessment`, `perf-guiding-oracles`
   (McSherry/COST honesty referee), `cost-scenario-family` (judge a win across scenarios, never over-fit).
5. **`bench/BASELINE.md` + `bench/README.md`** — the COST instrument that enforces gate (3). Never time
   bench concurrently with the suite.

## Step 0 — TRIAGE (prove the win exists BEFORE building)
The mandate is to build something REAL, so the FIRST workflow phase must PROVE a bench-measurable,
answer-equal win exists, then pick the cut:
- **C1 probe:** on the kFused carriers (key_corecursion_1, corecursion_1, tc_nonlinear_diff), inspect the
  M3-emitted fixpoint (`.ir`/`.h`) — does it FULL-SCAN/full-drain where a `binding_prefix`-KEYED seek
  (reusing P7's `GetOrCreateIndex`/`Index::First/Next`) would be answer-equal and touch fewer rows? If M3
  is already keyed on the prefix everywhere → NO C1 win → pivot.
- **C2 probe:** scan the corpus for a genuine WCOJ/leapfrog join win (a cyclic/triangle join where M3's
  binary hash-join is measurably beaten). If found, C2 is the real cut (heavier, slice minimally).
- Pick the candidate with a demonstrable win. If neither is clean, C3 (bench finds the worst bottleneck).

## Method — the grounding loop, run via WORKFLOWS with opus + sonnet (the loop that landed P2–F16)
Thin orchestrator; sequential single-phase workflows; watch the `(await parallel(...)).filter(...)`
precedence. DOCS-ONLY until the owner green-lights execution. Produce, in order:
1. **As-is pseudocode** — build out the algorithm/architecture pseudocode from the seed §1 + the chosen
   candidate's target (the M3 emission path for C1, the join/runtime path for C2); **re-verify every
   anchor at tip** (sonnet reads; the seed's map is a starting point, not gospel).
2. **Design-goal diffs** at hunk grain on that pseudocode (opus) — the chosen real cut; each with a
   DISCRIMINATING exit gate that INCLUDES the bench delta + answer byte-equality + codegen-golden move.
3. **Critique adversarially** (opus 3-refuter panel) — and **VERIFY EMPIRICALLY**: a throwaway-worktree
   spike (apply, build, run the FULL suite for answer-invariance, run `bench/` for the delta). The
   refuters must confirm the cut DOES SOMETHING REAL (codegen moves + bench delta), is answer-equal
   (no miscompile), and is minimal (no swamp / no drag-in of P8 if C1, or of a full trie if C2's first
   slice). P6.5's grounding was refuted-as-shadow — do not repeat that; the win must be demonstrated.
4. **IR desired output states** (predict-then-verify) — the exact `.ir`/`.h`(/`.rel`) BEFORE/AFTER on
   the carrier, which goldens MOVE, and the bench-carrier's measured delta. Decide the goldens precisely.

Model tiering (memory `subagent-model-tiering`): **sonnet** = anchor re-verification, baseline `.ir`/`.h`
dumps, bench baselines; **opus** = the triage judgment, design diffs, refuter panel, IR desired-states.
Keep the orchestrator thin. Present the design + exit gate and **STOP for the execution go/no-go.**

On green-light: execute as one coherent commit (SUITE PASS, ctest 5/5, answer goldens byte-equal ×4
modes, codegen goldens re-blessed after review, bench delta recorded per `bench/BASELINE.md`), then
update CLAUDE.md + memory + write the session-29 seed.

## Gotchas (carried)
- clangd diagnostics are NOISE (no include paths) — trust the real build only.
- macOS bash 3.2 / zsh word-splitting: use `${=var}` when a var holds multiple CLI args. `.dr` ASCII-only.
- Run builds/suite SILENT on success; the full OptDiff suite takes ~3–4 min — run BACKGROUNDED, await.
- `runall.sh --bless <workroot> [filter]` takes NO jobs arg (PROMOTES from an existing workroot; run the
  suite into it FIRST, with any new `.irgold` step added). Bless ONLY after reviewing the delta; never
  through a symlink golden; never to make a red case green. A new carrier needs `.dr`+`.main.cpp`(+
  `.irgold` for codegen goldens); a `.batches` sidecar adds the oracle/monotone/behavioral answer nets.
- Bench builds are `-O2 -DNDEBUG`; NEVER time bench concurrently with the suite; comparisons key on
  (case, mode, knobs) semantics, never generated-text hashes.
- The answer-equality gate is the FIRST real one in this arc — a fixpoint/join re-emission that changes
  an answer is a MISCOMPILE. Gate hard: oracle + behavioral + I0 RefInterp across all 4 modes.
- If the triage (Step 0) shows NO real minimal win in C1 or C2, SAY SO plainly and put the re-sequence
  decision (P8-first, or a different real lever) to the owner — do not manufacture a shadow to have
  something to land.

## A closing word
The discipline that got this project here — predict-then-verify, adversarial critique, honest
value-accounting, never blessing a shadow — is exactly the discipline that will land the real win. Trust
it. When the bench harness shows fewer rows scanned and every answer golden stays byte-identical, that is
the greenfield vision made concrete: the model earning its keep, the generated code getting genuinely
better, correctness never in doubt. You have the map, the tools, and the judgment to do it. Make it real —
and enjoy it.
