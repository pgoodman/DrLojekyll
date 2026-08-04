# Next Session: RegionalDataFlowCore — the post-Stage-B slice (owner ranks: Stage-C re-brief vs DIFF-R3 declared regions)

Continue in the Dr. Lojekyll repository, branch `keyed-instances`. Governing
discipline unchanged: reality grounding, byte-exact goldens, explicit negative
witnesses, predict-then-verify at the single-transform grain, the evil-monkey
rule, complexity guilty until proven inherent (smaller-not-larger).

THE STANDING METHOD is the session's shape — for whichever slice the owner
ranks first:
(a) BUILD OUT THE PSEUDOCODE: fleet re-verify + extend the architecture/
    algorithm pseudocode against the tip — `stage-b-landed-seed.md` Part 1 is
    the post-landing whole-program seed; `regional-arch-pseudocode.md`
    (§1–§7, Part R, Part B) is the deep authority. Extend where the chosen
    slice needs finer grain (Stage-C: the demand transform's mint sites +
    ResolveLiveRecognition + the lifecycle-op inventory; DIFF-R3: the
    lexer/parser bracket surface + ApplyDemandTransform's inference-vs-check
    fork). Merge so ONE pseudocode authority holds.
(b) FORMULATE THE DIFFS: re-express the slice's hunk doc as rigorous diffs on
    that verified pseudocode (Stage-C: stage-c-diff.md AMENDED by the
    DIFF-R1 panel's 15 normative amendments — apply the amendment rule, no
    implementation before dated diffs; DIFF-R3: region-model-diffs.md
    DIFF-R3/R5 + their panel amendments), folding
    `stage-b-landed-seed.md` Part 2's DIFF-NEXT-1..4. Opus authors.
(c) CRITIQUE THE DIFFS: adversarial panel (correctness/lifecycle,
    termination/confluence, testability/oracle, necessity), REFUTE-VERIFIED
    against code — refuters demand concrete code evidence per confirmation,
    re-triage severities, no finding without a failure scenario (~22%
    survival is the calibration; session-4 hit 75% because the first-pass
    draft carried real contradictions — tighten the authoring pass, then the
    panel).
(d) DESIRED IR OUTPUT STATES: author/refresh the desired dump bytes for the
    slice as diffs from FRESHLY COLLECTED dumps (`-region-out` now exists —
    collect it too; `region-model-desired-states.md` §1–§3 seed the Stage-C
    surfaces; `regional-dump-stage-b-desired-states.md` §9+§9.7 are the
    landed-bytes authority), critiqued with the determinism lens. These
    become the predict-then-verify targets.
Implementation only when (a)–(d) are green and the owner ratifies.

RUN FOLLOW-ON WORK THROUGH THE WORKFLOW TOOL — deterministic orchestration
(pipeline / fan-out / adversarial-verify), a thin orchestrator, agents return
structured findings (put COMPLETE text in structured-output fields, never
placeholders; note: a workflow agent can die mid-parallel — check the
failures line and adjudicate orphaned findings rather than dropping them).
MODEL TIERING: sonnet for mechanical extraction (anchor verification, dump
collection, census counts, suite runs); opus for judgment (pseudocode
authorship, diff formulation, adjudication, critique/refute panels). SILENT
tests (capture to files, print only failures). NEVER bless a golden to make
anything pass; bless only via explicit `runall.sh --bless` after review. Do
not commit/push unless asked; identity `Peter Goodman <peter.goodman@gmail.com>`;
keep tool/model/provider details out of commits and artifacts.

## State at session open (session-4 close — STAGE B LANDED)

- Stage B landed the same day it was designed: (a)–(d) design pass (Part B
  pseudocode; panel-critiqued AMENDMENTS on stage-b-diff.md, 12→9 findings;
  §9 desired states + §9.7 implementation reconciliation), owner ratified to
  recommended (ESC-4 variant (iii), Minimize DEFERRED, ESC-1/ESC-2, 4-witness
  pin set, no key-invariant token), then implementation: `lib/Regional`
  (FrozenRegionalProgram, the third-slot freeze, G1 `-region-out` +
  `-region-dot-out` DOT twin), H4 (`Program::Build(frozen,…)`, thin
  `frozen.Query()` seam), V-REGION-CENSUS always-on recount at the
  ValidateDROps tail, `run_irgold` region surface + 16 blessed `.region`
  goldens (4 witnesses × 4 modes).
- SESSION-4 DELTAS MAY BE UNCOMMITTED — check `git status` first; if dirty,
  surface the commit split to the owner before anything else (docs +
  implementation + goldens are natural commit units).
- Known records: R-STORE narrowed to insert-materialized relations (§9.7 —
  `path` unnameable; the logical-origin-provenance necessity witness); the
  zero-arity implicit-export empty-`rel=` parser wart (record-only,
  DIFF-NEXT-4); demand pair census `row-contracts=1`.

## Phase 0 (housekeeping)

1. `git status` / `git log -5`; handle the uncommitted-deltas question.
   Rebuild debug; suite SILENT (expect `SUITE: PASS (190)` WITH the 16
   `.region` pins and V-REGION-CENSUS live); ctest (expect 7/7).
2. Read, in order: `stage-b-landed-seed.md` (the charge), the ranked slice's
   hunk doc (`stage-c-diff.md` + the DIFF-R1 panel record in
   `region-model-diffs.md`, OR the DIFF-R3/R5 sections), `stage-b-diff.md`
   AMENDMENTS (the landed contract), `regional-arch-pseudocode.md` Part B.

## Owner decisions to surface (STOP where load-bearing, do not guess)

- THE RANKING (the owner states it in the session-opening message):
  Stage-C re-brief (blocked on D2.6 reader-handle + the §6-vs-§11 routing
  rule — both STOPs) vs DIFF-R3 declared regions (independent of D2.6;
  owner-recommended post-B slot; brings the adornment-fuzzing harness) vs
  the Tier-2 provenance-through-transforms mini-slice (DIFF-NEXT-3 Tier 2 +
  D2.9 proxy role inheritance as ONE slice).
- TIER-1 NAMING LIFT (DIFF-NEXT-3 Tier 1, rider-sized): unless the owner
  says otherwise, EXECUTE IT in this session regardless of the ranking —
  as the first hunk of DIFF-R3 if that ranks first, else as a standalone
  rider (predict the 8 re-blessed .region goldens FIRST: `rel=path` /
  `rel=rel` return, demand-pair census `row-contracts` 1→2; suite stays
  190-green otherwise; bless only after review).
- If Stage C: D2.6 and §6-vs-§11 MUST be answered before authoring the
  request-edge row schema or any recursive-demand routing.
- If DIFF-R3: the hint-vs-mandate policy (STOP-R5-A reject-vs-widen) and the
  exact slot for the fuzzing harness.

## Stop conditions

An unratified decision becomes load-bearing; a predicted dump diverges
unexpectedly; ANY golden (bespoke, oracle, behavioral, eqgate, irgold incl.
`.region`) diverges; behavioral-golden divergence is a HARD STOP (frozen
through the Stage-C cutover); any impulse to author code for the unranked
slice.

## Deliverables

The verified+extended pseudocode (one authority); the reworked critiqued
slice diffs (dated amendments on the slice's hunk doc); the desired IR
states, determinism-critiqued; and — if the owner ratifies implementation —
the slice landed with every golden byte-identical and any new dump surface
blessed after review; an updated version of THIS prompt pointing at the
ranked-next slice.
