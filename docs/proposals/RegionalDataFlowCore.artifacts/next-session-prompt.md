# Next Session: RegionalDataFlowCore — Stage B (the canonical planning regional program)

Continue in the Dr. Lojekyll repository, branch `keyed-instances`. Governing
discipline unchanged: reality grounding, byte-exact goldens, explicit negative
witnesses, predict-then-verify at the single-transform grain, the evil-monkey
rule, complexity guilty until proven inherent (smaller-not-larger). THE
STANDING METHOD applies to every slice: (a) fleet re-verify + EXTEND the
pseudocode against the tip; (b) express the slice as DIFFS on that pseudocode;
(c) adversarial critique panel (refute-verified) over any reworked diff;
(d) author/refresh the DESIRED IR OUTPUT STATES as diffs from freshly
collected dumps, critique them (determinism lens), use them as
predict-then-verify targets. Implementation only when (a)-(d) are green.

RUN FOLLOW-ON WORK THROUGH THE WORKFLOW TOOL (pipeline / fan-out /
adversarial-verify; thin orchestrator; structured findings). MODEL TIERING:
sonnet mechanical, opus judgment. SILENT tests. NEVER bless to make red green;
bless only via explicit `runall.sh --bless` after review. Do not commit/push
unless asked; identity `Peter Goodman <peter.goodman@gmail.com>`; keep
tool/model/provider details out of commits and artifacts.

## State at session open (session 3 closed 2026-08-03; re-snapshot from tip)

- COMMITTED at session-3 open: b61797cd (F26 dfe split + V-SCC-SEAM + F23 pin),
  bd694541 (D3.3 landing set + F27/F28 + F29 record), 1d28a74c (Stage A),
  70cc60fd (docs: ledger (AW)/(AX), CLAUDE.md refresh, adornment-fuzzing
  direction). NOT pushed.
- Session-3 NEW WORK (verify with `git status` whether the owner committed it;
  if not, the commit gate below): the I0 SLICE (bin/RefInterp — OG1-parsed
  definitional evaluator; bin/RefHarness — OG2-tool behavioral emitter;
  bin/CMakeLists.txt targets; runall.sh run_refinterp step + bless line; 59
  blessed `goldens/*.behavioral.stdout`; 8 `cases/*.probes`; the F29 codegen
  FIX in lib/CodeGen/CPlusPlus/Database.cpp commit-sweep used-state collector;
  FINDINGS.md F29 promoted) + the DESIGN artifacts (regional-arch-pseudocode
  Part R; region-model-diffs.md; region-model-desired-states.md;
  stage-i0-interpreter.md §7/§8; owner-adjudication-record.md appends;
  INDEX.md; this prompt).
- EXIT STATE verified at close: SUITE: PASS (190) with run_refinterp LIVE,
  ctest 7/7, zero non-behavioral golden churn. The I0 exit gate is MET 63/63
  (59 compiling MATCH incl. 4-mode behavioral agreement; 4 diagnostics
  interp-clean; zero carve-outs — negation_flap moot under the netting
  adjudication).

## Phase 0 (housekeeping)

1. `git status` / rebuild / suite SILENT (expect PASS 190) / ctest.
2. COMMIT GATE (owner-interactive) for the session-3 tree if uncommitted —
   suggested split: (i) the F29 codegen fix + FINDINGS promotion; (ii) the I0
   slice (tools + runall wiring + goldens + probes + stage-doc amendments);
   (iii) the region-model design artifacts. Or the owner's shape.
3. CLAUDE.md refresh: add the I0 referee to the Test section (run_refinterp,
   the behavioral golden family, the CBF, the plain-compile rule, the .probes
   sidecar) + FINDINGS status line (F29 fixed).

## The MAIN work — Stage B, the canonical planning regional program (owner-ranked next)

`stage-b-diff.md` is the stage doc (the ONLY clean stage in the phase-3
critique). Under the standing method:

1. (a) Fleet re-verify stage-b-diff.md's anchors against the tip (Stage A
   landed since its authoring; Part R of regional-arch-pseudocode.md is the
   pseudocode authority; the H9 validator handoff was re-pointed by Errata-5).
2. (b) Express Stage B as diffs on Part R; fold in the Stage-B-relevant
   normative amendments from region-model-diffs.md's panel records (esp.
   DIFF-R5's logical/physical key split — the Stage-B Minimize/FieldExpression
   deferral is where flat-key grows real minimal keys) and the Stage-B inputs
   recorded in owner-adjudication-record.md (logical-origin provenance on
   model tables; the demand-areas-vs-SCCs framing; role-inheritance-through-
   proxies is an OPEN owner item).
3. (c) Critique panel, refute-verified. NOTE the session-3 panel-yield lesson:
   the refuters confirmed ~95% (vs Stage A's 22%) — tighten the refuter charge
   (demand concrete code evidence per confirmation; re-triage severities).
4. (d) Desired -region-out G1 states: region-model-desired-states.md §2/§3 are
   the seed (its §5/§6 hazard ledger lists what needs owner adjudication
   first: H2 the kRequestEdgeAdd-vs-mono-arm-collapse contradiction with
   rel-stage-c-desired-states.md H-G.3; H1 the key-in-output convention).
   Stage B's dump grammar is ratified G1 (D2.2); the DOT twin directive
   applies (cluster_region, advisory).
5. Exit gate: Stage B is a PURE REFACTOR — all 190 goldens byte-identical,
   all 59 behavioral goldens byte-identical (I0 now referees the builder
   tail INDEPENDENTLY — the OG1-parsed independence argument, E2, is live).

## Standing referees (use them)

- I0: `run_refinterp` in every suite run; `drlojekyll-refinterp <dr> <batches>
  [probes]` ad hoc. The CBF gate compiles the PLAIN program (never .drflags).
- The adornment-fuzzing direction (owner 2026-08-03) awaits DIFF-R3's slice —
  keep it in view when Stage B touches the parser-adjacent surfaces.

## Paused behind owner input (do NOT author their code)

D2.6 reader-handle schema (Stage-C header authoring paused); the §6-vs-§11
routing rule; declared-regions hint-vs-mandate + sequence slot (DIFF-R3);
the lazy/force barrier variants (DIFF-R4); the desired-states H1/H2
adjudications; role-inheritance-through-proxies; the region-model-diffs.md
OPEN OWNER ITEMS lists (per diff).

## Stop conditions

An unratified decision becomes load-bearing; a predicted dump diverges
unexpectedly; any golden (bespoke OR behavioral) diverges during the Stage-B
refactor; any impulse to author Stage C/D code before the re-brief.

## Deliverables

Committed (owner-shaped) session-3 tree; CLAUDE.md I0 refresh; Stage B
standing-method green (verified anchors, diffs, refuted critiques, desired
G1 states) and — if the owner ratifies the implementation — the Stage-B
refactor landed with every golden byte-identical; an updated version of THIS
prompt pointing at the next ranked slice (Stage-C re-brief resolution or the
declared-regions surface).
