# Next Session: RegionalDataFlowCore — Stage B (the regional representation becomes canonical)

Continue in the Dr. Lojekyll repository, branch `keyed-instances`. Governing
discipline unchanged: reality grounding, byte-exact goldens, explicit negative
witnesses, predict-then-verify at the single-transform grain, the evil-monkey
rule, complexity guilty until proven inherent (smaller-not-larger).

THE STANDING METHOD is the session's shape — for the Stage-B slice:
(a) BUILD OUT THE PSEUDOCODE: fleet re-verify + extend the architecture/
    algorithm pseudocode against the tip — `stage-b-seed.md` Part 1 is the
    Stage-B-grain seed, `regional-arch-pseudocode.md` (§1–§7 + Part R) the
    deep authority; extend where Stage B needs finer grain (the Query::Build
    tail and every caller of Query; the Rel/ControlFlow read surface of
    QueryImpl that "consumes only FrozenRegionalProgram" must wrap; the dump
    sinks). Merge so ONE pseudocode authority holds.
(b) FORMULATE THE DIFFS: re-express `stage-b-diff.md` H1–H9 as rigorous
    diffs on that verified pseudocode, folding in `stage-b-seed.md` Part 2's
    DELTA-1..6 (the freeze point AFTER the Stage-A contract pass; the
    190+63+63+59+5 exit gate with I0 live; ratified-G1 + predicted bytes;
    the panel amendments; the new owner inputs). Opus authors.
(c) CRITIQUE THE DIFFS: adversarial panel (correctness/lifecycle,
    termination/confluence, testability/oracle, necessity), REFUTE-VERIFIED
    against code — and apply DELTA-6: refuters demand concrete code evidence
    per confirmation, re-triage severities, no finding without a failure
    scenario (session-3's ~95% survival was too permissive; Stage A's ~22%
    is the calibration).
(d) DESIRED IR OUTPUT STATES: author/refresh the desired `-region-out` G1
    bytes + DOT twin + any `.df`/`.contract`/`.rel` deltas for the ONE-region
    Stage-B program, as diffs from freshly collected dumps
    (`region-model-desired-states.md` §2/§3 seed it; honor its §5/§6
    determinism ledger and do not prejudge the H1/H2 owner items); critique
    with the determinism lens. These become the predict-then-verify targets.
Implementation only when (a)–(d) are green and the owner ratifies.

RUN FOLLOW-ON WORK THROUGH THE WORKFLOW TOOL — deterministic orchestration
(pipeline / fan-out / adversarial-verify), a thin orchestrator, agents return
structured findings (never paste whole files between them; put the COMPLETE
text in structured-output fields — a session-3 author lost its section to a
placeholder). MODEL TIERING: sonnet for mechanical extraction (anchor
verification, dump collection, census counts, suite runs); opus for judgment
(pseudocode authorship, diff formulation, adjudication, critique/refute
panels). SILENT tests (capture to files, print only failures). NEVER bless a
golden to make anything pass; bless only via explicit `runall.sh --bless`
after review. Do not commit/push unless asked; identity
`Peter Goodman <peter.goodman@gmail.com>`; keep tool/model/provider details
out of commits and artifacts.

## State at session open (session-3 close = tip 423b8139 + the seed commit; pushed)

- Everything is COMMITTED and PUSHED: b61797cd/bd694541/1d28a74c (F26 split /
  D3.3+F27+F28 / Stage A), 70cc60fd (ledger (AW)/(AX) + CLAUDE.md), a9a11f7b
  (F29 FIXED — commit-sweep used-state collector), 1137b4f6 (I0 LANDED — the
  exit gate met 63/63, run_refinterp live in runall.sh, 59 frozen behavioral
  goldens, 8 .probes), 423b8139 (Part R + region-model-diffs.md +
  region-model-desired-states.md), + the stage-b-seed/prompt docs commit.
- Verified at close: SUITE: PASS (190) with the I0 referee live, ctest 7/7,
  working tree clean.
- Re-snapshot baselines from the tip; artifact line-anchors are session-local.

## Phase 0 (housekeeping)

1. `git status` / `git log -5`. Rebuild debug; suite SILENT (expect
   `SUITE: PASS (190)`); ctest (expect 7/7).
2. Read, in order: `stage-b-seed.md` (the charge), `stage-b-diff.md` (the
   hunks it amends), `regional-arch-pseudocode.md` Part R (the pseudocode
   authority), the `region-model-diffs.md` panel records that name Stage-B
   homes, `region-model-desired-states.md` §2/§3/§5/§6.

## The MAIN work — Stage B under (a)–(d) above

Charge: the regional representation becomes canonical as a PURE REFACTOR —
`PlanningRegionalProgram` built at the Query::Build tail (AFTER the Stage-A
contract pass; contracts/models/strata/guard-annotations/RecognizedSubgraphs/
forcings are its inputs), FROZEN, one ProgramRoot + one observation-root
region, zero child calls, everything downstream consuming the frozen object;
the G1 `-region-out` dump + DOT twin; the H9-as-amended scaffold validators;
the DIFF-R5 logical-key canonicalization IF the owner pulls Minimize into
this slice (ask — D3.4 deferred it to B as an option, not a mandate).

EXIT GATE: 190 bespoke + 63 oracle + 63 monotone + 59 behavioral + 5 eqgate
+ the .irgold pins ALL byte-identical, suite WITH run_refinterp live — the
I0 referee (no shared code with Query::Build) checks the builder-tail
refactor independently. A NEW `-region-out` surface pinned deterministic for
the witness set (blessed only after review). Any behavioral-golden divergence
is a hard stop (they are FROZEN through the Stage-C cutover).

## Owner decisions to surface (STOP where load-bearing, do not guess)

- H1 vs H1-ALT: freeze at the Query::Build tail (return-type ripple) vs a
  main-level step between Query::Build and Program::Build.
- Minimize/FieldExpression (real minimal keys) into this slice or stay
  deferred (D3.4; DIFF-R5's Stage-B hook).
- The desired-states H1 (key-in-output convention) + H2 (kRequestEdgeAdd vs
  H-G.3 mono-arm-collapse) adjudications — Stage-C-gating, but the G1 census
  field-order choice must not prejudge them.
- Role-inheritance-through-proxies (D2.9 default rule); logical-origin
  provenance on planning table nodes (direction, unratified).

## Paused behind owner input (do NOT author their code)

Stage C entirely: D2.6 reader-handle schema, the §6-vs-§11 routing rule, the
request-edge mint (DIFF-R1), the lazy/force barrier (DIFF-R4), declared
regions + the adornment-fuzzing harness (DIFF-R3 — but keep the G1 grammar
generator-friendly).

## Stop conditions

An unratified decision becomes load-bearing; a predicted dump diverges
unexpectedly; ANY golden (bespoke, oracle, behavioral, eqgate, irgold)
diverges during the refactor; any impulse to author Stage C/D code.

## Deliverables

The verified+extended Stage-B pseudocode (one authority); the reworked
critiqued Stage-B diffs (amendments applied as dated diffs on
stage-b-diff.md); the desired G1/DOT states, determinism-critiqued; and —
if the owner ratifies implementation — Stage B landed with every golden
byte-identical and the new dump surface blessed after review; an updated
version of THIS prompt pointing at the ranked-next slice (the Stage-C
re-brief resolution, or the declared-regions/DIFF-R3 surface, as the owner
ranks).
