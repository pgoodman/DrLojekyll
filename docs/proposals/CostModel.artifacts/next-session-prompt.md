# NEXT-SESSION PROMPT — the cost-model epoch (steps 2 + 3)

> Copy the block below to start the next session. It continues the cost-model epoch
> opened 2026-07-31 (the identity-join recognizer + Prov substrate LANDED; the cost
> model reshaped from symbolic to numeric-simulator-calibrated-against-bench).

---

Continue Dr. Lojekyll on branch `keyed-instances` (verify tip with `git log` — it is the
identity-join-recognizer landing: Prov substrate + `df.ident_join` + the mono `.irgold`
mode-split pin). The recognizer is DONE and committed. This session builds the NUMERIC
COST SIMULATOR — the reshape (step 2) + the tool (step 3).

PRIMARY TARGET: the cost model as a NUMERIC SIMULATOR whose predictions are VALIDATED
against `gBenchCounters` (owner ruling: numbers, not symbols; "how are you evaluating these
/ is it make-believe" — the answer is predicted-vs-measured). NOT symbolic `N·M` algebra.
Two deliverables: (2) RESHAPE `docs/proposals/CostModel.md` from symbolic to numeric; (3)
BUILD `bin/Cost` (`drlojekyll-cost`) — L1 cardinality + L2 op-cost → predicted counters,
V-COST-CALIB against measured `gBenchCounters`, over a `.cost` SCENARIO FAMILY.

BOOTSTRAP (read END TO END before any code): `docs/proposals/CostModel.artifacts/
cost-simulator-seed.md` (the SINGLE-PASS whole-program pseudocode + path-forward-as-diffs
r0–r6 + the ten... eight anchors) FIRST; then `measured-calibration-1.md` (the reshape
rationale + the measured law ΔidxAdds=F·K), `grounding-double-join.md` (the double-join
grounding), `prov-recognizer-impl.md` (the landed Prov/recognizer the L1 reuses); then
`CostModel.md` (KEEP §0 two-layer / §2.8 spine-Lowering / §4 scenario-family; RESHAPE §1–§3
symbolic → numeric). Then the recalled memories `[[cost-scenario-family]]`,
`[[predict-then-verify-ir]]`, `[[demand-cost-model]]`. The seed is SINGLE-PASS, ONE session,
NO fleet — treat every claim as UNVERIFIED until your fleet re-derives it from CODE.

ORDER OF WORK:
0. RE-SNAPSHOT baselines from the CURRENT tip before any code: build debug+release+ASAN
   (error-grep every build — the stale-binary trap); ctest 6/6 both trees; OptDiff SUITE
   PASS (incl. the recognizer's mono `.irgold` byte-identical); ASAN standing per §19(F).
   Snapshot the mono bench calibration (ΔidxAdds=F·K) as the frozen predicted-vs-measured
   anchor.
1. RE-VERIFY THE SEED with a WORKFLOW FLEET (house precedent): seed-UNREAD opus derivation
   lanes that rebuild the cost-simulator architecture from CODE ONLY (lane A: the `.rel`
   op vocabulary + `Lowering` spine, what L2 costs; lane B: the `gBenchCounters` fields, the
   prediction target + how a driver snapshots them; lane C: L1 cardinality over the `.df`
   dump + how Prov's keyset reasoning ports) + a seed-READ adversarial verifier (hunt FALSE
   claims in cost-simulator-seed.md, measure drift) + an xhigh consolidator minting errata.
   Orchestrator personally re-verifies the §4 anchors + the frozen calibration.
2. STAGE (a) — BUILD OUT THE PSEUDOCODE of the algorithms + architecture (seed-unread opus
   lanes; whole-program, grounded in anchors): the numeric simulator's L1 (cardinality),
   L2 (op-cost → counter fields), the calibration harness (predicted≈measured), and the
   scenario-family judge(). Produce a stage-(a) substrate doc.
3. STAGE (b) — FORMULATE THE DIFFS on that pseudocode for the key design goals (r0 reshape
   CostModel.md; r1 bin/Cost skeleton reading `.df`/`.rel` OFFLINE; r2 L1; r3 L2; r4 the
   calibration harness; r5 scenario family + goldens), one opus lane per coherent sub-goal,
   each with code-anchored PRE-REGISTERED predictions. CRITIQUE: a fresh adversarial critic
   per design lane + an xhigh adjudicator producing the binding design doc.
4. DESIRED OUTPUT STATES — do the SAME (pseudocode + diffs + critique) for every touched
   OUTPUT surface VERSUS REAL: the `.cost` sidecar grammar (scenario family), the `bin/Cost`
   stdout format (per-scenario predicted + measured + spread), and the predicted-vs-measured
   golden shape. A desired-states author lane hand-predicts the goldens; a BLIND worktree
   prototype lane implements from the design alone (hand-provisioned worktree, tip-verified,
   own build, WIP-commit at every lane boundary); three-way convergence; bless via the ritual.
5. Implement in pristine after convergence; review at pre-commit (opus — Fable may be over
   quota); land the ledger record + the design contracts + push.

METHOD: use WORKFLOWS for ALL thought-heavy work. OPUS for derivation/design/judgment/
critique lanes; SONNET for mechanical extraction/audit/golden-regen lanes; xhigh effort for
designers/adjudicators/consolidators. Keep the orchestrator thin but personally execute any
gate named in a binding pin (E-77). Agents write partial progress to disk with incremental
edits; recover a capped lane from its disk file, never respawn.

STANDING GATES per landing: SUITE PASS; the recognizer's mono `.irgold` byte-identical vs
the frozen baseline; V-COST-CALIB predicted≈measured on the mono ΔidxAdds=F·K (the
actionability gate — a cost model that can't reproduce a measured count is make-believe);
ctest 6/6 debug + ASAN; config-invariance. TRAPS: after every build grep 'error:';
background shells reset cwd — absolute paths; the bench build is SEPARATE
(`-DDRLOJEKYLL_BENCH_COUNTERS -O2 -DNDEBUG`, never the timed binary); the bin/Cost SEAM IS A
FORK to resolve, NOT offline-by-assumption — bin/Oracle links internal libs + builds the Query
graph in-memory (so Prov is reusable in-process), but the Rel graph has only the text
SetRelDumpStream seam (see seed r1 / anchor 6); RUN TESTS SILENT (capture to a file, print only
the sentinel + on-failure detail — tokens matter); GOLDEN-MASTER EVIL-MONKEY test every
transform on/off in all 4 modes + a negative witness; macOS bash 3.2 (no `declare -A`); zsh
needs `${=var}`; `PATH="/Users/pag/Code/.brew/bin:$PATH"`. Push to
`git@github.com:pgoodman/DrLojekyll`.

---
