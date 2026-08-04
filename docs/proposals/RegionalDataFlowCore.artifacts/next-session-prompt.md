# Next Session: post-@key (owner ranks: multi-@key repetition vs Stage-C re-brief vs fuzz arm vs Tier-2; two STOPs standing)

Continue in the Dr. Lojekyll repository, branch `keyed-instances` (tip
`48cd0a4f`, PUSHED; clean tree expected). Governing discipline unchanged:
reality grounding, byte-exact goldens, explicit negative witnesses,
predict-then-verify at the single-transform grain, the evil-monkey rule,
complexity guilty until proven inherent (smaller-not-larger).

THE ENTRY SEED is `key-pragma-landed-seed.md` (session-6 close): Part 1 is
the whole-program pipeline pseudocode at the @key-era tip, Part 2 the path
forward as DIFF-NEXT-K1..K6, Part 3 the verify-first list. The seed is
single-pass orchestrator-authored — fleet-verify its anchors before
building on them. `regional-arch-pseudocode.md` Part R3 is
STALE-BY-SUPERSESSION (bracket-era); the normative design record is
`region-model-diffs.md` (RP-1..RP-9, RES-1..6, the session findings).

THE STANDING METHOD is the session's shape — for whichever slice the owner
ranks first:
(a) BUILD OUT THE PSEUDOCODE: fleet re-verify + extend the architecture/
    algorithm pseudocode against the tip — start from the seed's Part 1;
    re-ground Part R3's fine grain (the @key parse states 8/21/22 with
    immediate resolution, the activation gate's module-decl scan, the
    Step-2b post-Loop-1 slot, the Program::Build admissibility fork) where
    the ranked slice needs it. Merge so ONE pseudocode authority holds.
(b) FORMULATE THE DIFFS: express the ranked slice as rigorous dated
    amendments on that verified pseudocode (K1: the repetition surface +
    set-of-sets reconciliation semantics; K3: stage-c-diff.md AMENDED by
    the DIFF-R1 panel's 15 normative amendments — no implementation before
    dated diffs; K5: DIFF-NEXT-3 Tier 2 + D2.9 as ONE slice). Opus
    authors; TIGHTEN THE AUTHORING PASS before the panel.
(c) CRITIQUE THE DIFFS: adversarial panel (correctness/lifecycle,
    termination/confluence, testability/oracle, necessity), REFUTE-VERIFIED
    against code — refuters demand concrete code evidence per confirmation,
    re-triage severities, no finding without a failure scenario.
    CALIBRATION (sessions 5-6): a probe-seeded low-volume finder pass ran
    11/11 survival AND the corpus still caught three things the panel
    missed (T1-IMPL-1 CSE-fragility, R3A-IMPL-1 the impossible eqgate,
    S6-IMPL-1 the demand-blind oracle). The panel is a FILTER; the SUITE is
    the referee — corpus-wide runs adjudicate, never fudge (finding docs).
(d) DESIRED IR OUTPUT STATES: author/refresh the desired dump bytes for the
    slice as diffs from FRESHLY COLLECTED dumps (-region-out, -contract-out,
    -rel-out, -df-out), critiqued with the determinism lens.
    `regional-dump-stage-b-desired-states.md` §9/§10 are the landed-bytes
    authority. LESSON (RES-4): predicted bytes must be COMPUTED from the
    EMITTER's padding rules (max-over-contracts column widths re-pad
    EXISTING lines), never assumed line-local.
Implementation only when (a)–(d) are green and the owner ratifies.

RUN FOLLOW-ON WORK THROUGH THE WORKFLOW TOOL — deterministic orchestration
(pipeline / fan-out / adversarial-verify), a thin orchestrator, agents
return structured findings (COMPLETE text in structured-output fields,
never placeholders; check the failures line and adjudicate orphaned
findings). MODEL TIERING: sonnet for mechanical extraction (anchor
verification, dump collection, census counts, suite runs); opus for
judgment (pseudocode authorship, diff formulation, adjudication,
critique/refute panels). SILENT tests (capture to files, print only
failures). NEVER bless a golden to make anything pass; bless only via
explicit `runall.sh --bless` after review — and NEVER bless the SYMLINKED
surfaces of `key_tc_witness` / `key_neighborhood_witness` directly (bless
writes THROUGH symlinks into the twins' goldens; verify byte-identity
first). Any demand-blind consumer of `Query::Build` MUST pass
`suppress_demand=true` (the S6-IMPL-1 rule; bin/Oracle is the precedent).
Do not commit/push unless asked; identity
`Peter Goodman <peter.goodman@gmail.com>`; keep tool/model/provider
details out of commits and artifacts.

## State at session open (session-6 close — @key LANDED end-to-end)

- `@key(Col, ...)` on `#local`/`#export` (RP-5/RP-9): the declared INSTANCE
  KEY, a flagless strict FORCE-OPT-IN (RP-6) that SELECTS the nested
  keyed-instance lowering where every forcing admits it and falls back
  SILENTLY to the flat guard web where not (recursive shapes);
  `-demand-instance` stays the STRICT override (fences stay diagnostics).
  Placement declaration-only (RP-7); `-demand` = the auto layer (RP-8).
  Checking: parse-time arg obligations; Step-2b V-DECLARED-KEY post-Loop-1
  (fence-first; mismatch/unseeded/undemanded = stable hard rejects, RP-3);
  Minimize/DeterminedBy is the provability lift (O-R3.5 — the @key-for-FD
  reservation dissolved by convergence).
- Suite PASS (246 = 200 golden cases + 46 driverless rejects in
  `tests/OptDiff/rejects/`, both mode extremes, rc=0 = lost check, >=124 =
  crash finding); ctest 7/7. Witness spine: `key_tc_witness`
  (activation-equivalence via 12 symlinks; flat fallback),
  `key_neighborhood_witness` (flagless NESTED selection,
  kSubgraphInstantiate=1 pinned in its own rel.opt golden), the 8 flagless
  `key_*` diagnostics, F30's collision pair.
- Tier-1 interior naming (session 5) unchanged: -region-out names demanded
  interiors (`rel=path`/`rel=edge`, census row-contracts=2 on the demand
  pins); support= is the role-blind OR over live guard JOINs.
- The injected forcing seam (the query entry point publishing the
  fabricated `demand__` message through the synthesized injector) STILL
  EXISTS in both lowerings — deleting it is Stage C's request-edge work,
  the owner's stated end-state.

## Phase 0 (housekeeping)

1. `git status` / `git log -5` — expect clean at `48cd0a4f`. Rebuild debug;
   suite SILENT (expect `SUITE: PASS (246)`); ctest (expect 7/7).
2. Read, in order: `key-pragma-landed-seed.md` (the charge), the ranked
   slice's sections in `region-model-diffs.md`, and for K3 additionally
   `stage-c-diff.md` + the DIFF-R1 panel record + `owner-adjudication-record.md`.

## Owner decisions to surface (STOP where load-bearing, do not guess)

- THE RANKING (owner states it in the session-opening message), over the
  seed's Part 2:
  1. **K1 multi-@key repetition** — `@key(A) @key(B)` -> N keyed stores;
     surface+checking only (the D3.a.3 N-store machinery exists). OPEN
     semantics inside it: total-bijection vs subset coverage of the
     adornments (recommend total).
  2. **K2 the `-demand` auto-sweep STOP** — strict vs BEST-EFFORT
     (recommended endpoint; FLIPS the demand_*_body_1 reject goldens whose
     oracle pins were built anticipating exactly this). OWNER STOP — do not
     implement before the explicit call.
  3. **K3 Stage-C re-brief** — request edges + lifecycle ops; kills the
     injected forcing seam; `@key`'s declared bit becomes a render-only
     provenance marker on the request port. BLOCKED on the D2.6
     reader-handle schema + the §6-vs-§11 routing rule (both STOPs) + the
     DIFF-R1 15-amendment fold.
  4. **K4 the key-subset covering-array fuzz arm** (small, independent;
     feeds rejects/).
  5. **K5 Tier-2 provenance** (origin decl-sets + D2.9 as one slice;
     unblocks the ADJ-R3-C belt; the panel must litigate the post-F1
     maintained-satellite question).
  - Riders under any ranking (seed K6): cross-redeclaration @key
    consistency; pragma-anchored Step-2b error ranges; the DOT `declared`
    badge + a DR-IR DOT twin (advisory, never goldened); the
    `KEY (_MissingVar)` render wart.

## Stop conditions

An unratified decision becomes load-bearing (K2 and K3 carry explicit
owner STOPs); a predicted dump diverges unexpectedly; ANY golden (bespoke,
oracle, monotone, behavioral, eqgate, irgold incl. `.region`) diverges
beyond the predicted set; behavioral-golden divergence is a HARD STOP
(frozen through the Stage-C cutover); any impulse to author code for an
unranked slice. Precedent: an unexpected corpus abort/divergence is a
FINDING to adjudicate and record (the T1-IMPL-1 / S6-IMPL-1 shape) —
amend the design record, never fudge.

## Deliverables

The verified+extended pseudocode (one authority; Part R3 re-grounded if
touched); the ranked slice's dated, panel-critiqued diffs; the desired IR
states, determinism-critiqued; and — if the owner ratifies implementation —
the slice landed with every golden byte-identical except the predicted set
(blessed after review); an updated version of THIS prompt pointing at the
ranked-next slice.
