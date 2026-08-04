# Next Session: post-K6 (owner ranks: K5 Tier-2 provenance vs K4 fuzz arm vs K3 Stage-C ownership flip vs S4 residuals; K3's two STOPs standing)

Continue in the Dr. Lojekyll repository, branch `keyed-instances` (tip =
the session-7 close docs commit atop `b95d3d29` [K6] and `e641be46` [K1],
PUSHED; clean tree expected). Governing discipline unchanged: reality
grounding, byte-exact goldens, explicit negative witnesses,
predict-then-verify at the single-transform grain, the evil-monkey rule,
complexity guilty until proven inherent.

THE ENTRY SEED is `k6-landed-seed.md` (session-7 close): Part 1 the
whole-program pipeline pseudocode at the post-K6 tip, Part 2 the path
forward as DIFF-NEXT-S1..S4, Part 3 verify-first. It supersedes
`key-pragma-landed-seed.md` at the whole-program grain; the fine-grain
authorities are `k1-multikey.md` (K1) and `k6-riders.md` (K6), each with
its panel record + adjudications. The normative design record is
`region-model-diffs.md` (RP-1..RP-10, SESSION-7 RATIFICATIONS, and THE
SUBGRAPH-AUTHORITY FRAMING — the owner-adjudicated normative input to any
K3 work). Seeds are single-pass orchestrator-authored: FLEET-VERIFY their
anchors before building on them.

THE STANDING METHOD is the session's shape — for whichever slice the
owner ranks first:
(a) BUILD OUT THE PSEUDOCODE: fleet re-verify + extend the architecture/
    algorithm pseudocode against the tip, starting from the seed's Part 1;
    ONE pseudocode authority per slice (k1-multikey.md / k6-riders.md are
    the shape: Part A grounded reality, verification ledger).
(b) FORMULATE THE DIFFS: express the ranked slice as rigorous dated diffs
    on that verified pseudocode. Opus authors; TIGHTEN THE AUTHORING PASS
    first — the orchestrator adjudicates open sub-questions into the
    prompt (the ADJ-K1/ADJ-K6 pattern), and runs CHEAP EMPIRICAL PROBES
    inline before/while panels run (the s7 shadow probe found a live bug
    the panel then compounded — probes and panels compose).
(c) CRITIQUE THE DIFFS: adversarial panel (correctness/lifecycle,
    termination/confluence, testability/oracle, necessity),
    REFUTE-VERIFIED against code — no finding without a concrete failure
    scenario; refuters default to REFUTED; orphaned findings (a dead
    refuter) are ORCHESTRATOR-verdicted from the journal, never dropped.
    CALIBRATION (s5-s7): the panel is a FILTER, the suite+probes are the
    REFEREES — s7's panel found F31 (a forever-dead check family) only
    because a refuter empirically exercised the "working precedent";
    negative witnesses must be able to LOSE the check they pin
    (ADJ-K1-F: a witness that stays green when the check is deleted is a
    lost check, reshape it).
(d) DESIRED IR OUTPUT STATES: author the desired dump bytes as diffs from
    FRESHLY COLLECTED dumps (-df-out, -contract-out, -rel-out,
    -region-out, the DOT twins), determinism-critiqued, COMPUTED from the
    emitters' padding rules — collect dumps inline while panels run (they
    become the implementation's predict-then-verify gates). Definitional
    referee goldens (oracle/monotone/behavioral) are GENERATED-then-
    reviewed via cross-referee agreement, never hand-predicted.
Implementation only when (a)-(d) are green and the owner ratifies; the
implementer runs gated (byte-match predictions BEFORE golden install,
finding-never-fudge), and the orchestrator re-runs build+suite+ctest as
an INDEPENDENT referee after.

RUN FOLLOW-ON WORK THROUGH THE WORKFLOW TOOL — deterministic
orchestration (pipeline / fan-out / adversarial-verify), a thin
orchestrator, structured findings (complete text in structured-output
fields; check the failures line; recover orphans from journal.jsonl).
MODEL TIERING: sonnet for mechanical extraction (anchor fleets, dump
collection, censuses, format exemplars); opus for judgment (pseudocode
authorship, diff formulation, panels/refuters, adjudication-heavy
amendment passes). SILENT tests (capture to files, print only failures).
Bless only via `runall.sh --bless` after review — `bless_copy` now
mechanically refuses symlink write-through, but the review discipline
stands. Any demand-blind `Query::Build` consumer passes
`suppress_demand=true`. Do not commit/push unless asked; identity
`Peter Goodman <peter.goodman@gmail.com>`; keep tool/model/provider
details out of commits and artifacts.

## State at session open (session-7 close — K1 + K6 LANDED)

- Multi-`@key` (RP-10 total bijection, K1) + the seven K6 riders: N
  pragmas with parse-time dup-set reject and per-pragma DisplayRanges
  (Step-2b Arm A anchors at the offending @key token); IDENTICAL-OR-
  ABSENT cross-redeclaration consistency on the F31-REVIVED check site
  (F31: prev_decl aliased self — every redecl type/name check had been
  dead forever; fixed, zero corpus fallout); context-resolving accessors
  (a pragma on any redeclaration activates — F-K6-SHADOW dead); the
  collision-advice fork; "instance key" wording throughout; per-set
  declared-key contract lines; `bless_copy`; the `-rel-dot-out` Rel-IR
  DOT twin + region declared-key badge + no `_MissingVar`; the
  multi-adornment family is .batches-refereed (oracle/monotone/
  behavioral; key twin symlinks oracle/monotone, real behavioral).
- Suite PASS (250 = 203 cases + 47 rejects); ctest 7/7; eqgate 6.
- The RP-8 auto-sweep STOP is RESOLVED strict-for-now (re-open only on
  an explicit owner call). The injected forcing seam STILL EXISTS —
  deleting it is the Stage-C ownership flip.

## Phase 0 (housekeeping)

1. `git status` / `git log -5` — expect clean at the close tip. Rebuild
   debug; suite SILENT (expect `SUITE: PASS (250)`); ctest (7/7).
2. Read, in order: `k6-landed-seed.md` (the charge), the ranked slice's
   authorities (`region-model-diffs.md` sections; for K3 additionally
   `stage-c-diff.md` + the DIFF-R1 panel record +
   `owner-adjudication-record.md` + THE SUBGRAPH-AUTHORITY FRAMING).
3. Fleet re-verify the seed's Part-1 anchors before building.

## Owner decisions to surface (STOP where load-bearing, do not guess)

- THE RANKING over the seed's Part 2:
  1. **S1 / K5 Tier-2 provenance** — origin decl-sets on models + D2.9
     as one slice; unblocks the ADJ-R3-C belt + undemanded interior
     naming; the panel must litigate the maintained-satellite question.
  2. **S2 / K4 key-subset fuzz arm** — small, independent; its oracle
     must model the K6-widened reject surface (dup-set, identical-or-
     absent, revived F31 checks).
  3. **S3 / K3 Stage-C re-brief — the OWNERSHIP FLIP** (request edges;
     RecognizedSubgraph demotes to a handoff; injector seam dies).
     DESIGN-ONLY until the owner answers: D2.6 SHARPENED (readers hand
     onto the REGION or the STORE?) and the §6-vs-§11 routing rule; plus
     the DIFF-R1 15-amendment fold into stage-c-diff.md.
  4. **S4 residuals** — request-port declared badge; the behavioral
     "PLAIN vs .drflags" comment/code contradiction (recorded s7,
     unreconciled); the missing oracle-vs-behavioral cross-family check.

## Stop conditions

An unratified decision becomes load-bearing (S3's two owner STOPs); a
predicted dump diverges unexpectedly; ANY golden beyond the predicted set
diverges; behavioral-golden divergence is a HARD STOP; any impulse to
author code for an unranked slice. An unexpected corpus abort/divergence
is a FINDING to adjudicate and record (F31 is the standing exemplar) —
amend the design record, never fudge.

## Deliverables

The ranked slice through (a)-(d) with its one-authority artifact; if the
owner ratifies implementation, the slice landed with every golden
byte-identical except the predicted set; an updated version of THIS
prompt pointing at the ranked-next slice.
