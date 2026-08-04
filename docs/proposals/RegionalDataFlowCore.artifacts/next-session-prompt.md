# Next Session: post-DIFF-R3 (owner ranks: fuzz arm vs Stage-C re-brief vs Tier-2 provenance)

Continue in the Dr. Lojekyll repository, branch `keyed-instances`. Governing
discipline unchanged: reality grounding, byte-exact goldens, explicit negative
witnesses, predict-then-verify at the single-transform grain, the evil-monkey
rule, complexity guilty until proven inherent (smaller-not-larger).

THE STANDING METHOD is the session's shape — for whichever slice the owner
ranks first:
(a) BUILD OUT THE PSEUDOCODE: fleet re-verify + extend the architecture
    pseudocode against the tip — `regional-arch-pseudocode.md` (§1–§7,
    Part R, Part B, **Part R3**) is the deep authority; session-5 landed
    code SHIFTS Part-R3 line anchors (Demand.cpp gained the Step-2b block,
    Parser.cpp gained state 20) — re-verify before building.
(b) FORMULATE THE DIFFS as dated amendments on the slice's hunk doc
    (Stage-C: stage-c-diff.md + the DIFF-R1 panel's 15 normative
    amendments; Tier-2: stage-b-landed-seed.md DIFF-NEXT-3 Tier 2 + D2.9
    as ONE slice). Opus authors; tighten the authoring pass BEFORE the
    panel.
(c) CRITIQUE THE DIFFS: adversarial panel (correctness/lifecycle,
    termination/confluence, testability/oracle, necessity), REFUTE-VERIFIED
    against code, no finding without a failure scenario. Session-5
    calibration: 11/11 survived a probe-seeded low-volume finder pass — AND
    the corpus still caught two things the panel missed (T1-IMPL-1
    CSE-fragility, R3A-IMPL-1 the impossible eqgate spec). The panel is a
    filter, never the referee; the suite is the referee.
(d) DESIRED IR OUTPUT STATES as diffs from FRESHLY COLLECTED dumps,
    determinism-critiqued (regional-dump-stage-b-desired-states.md §9/§10
    are the landed-bytes authority — §10's byte predictions verified exact,
    incl. the max-over-contracts re-padding lesson: predicted bytes must be
    computed from the EMITTER's padding rules, not assumed line-local).
Implementation only when (a)–(d) are green and the owner ratifies.

RUN FOLLOW-ON WORK THROUGH THE WORKFLOW TOOL — deterministic orchestration,
thin orchestrator, structured findings (COMPLETE text in fields). MODEL
TIERING: sonnet for mechanical extraction, opus for judgment. SILENT tests
(capture to files, print only failures). NEVER bless a golden to make
anything pass; bless only via explicit `runall.sh --bless` after review —
and NEVER bless `demand_key_tc_witness`'s symlinked surfaces directly
(bless writes THROUGH symlinks into demand_tc_witness's goldens; verify
byte-identity BEFORE any bless that touches it). Do not commit/push unless
asked; identity `Peter Goodman <peter.goodman@gmail.com>`; keep
tool/model/provider details out of commits and artifacts.

## SESSION-6 DELTA (2026-08-04, read FIRST): the @demand surface + force-activation LANDED

Session 6 (owner-driven design conversation, ratified live): the declared
demand key is now the flagless post-paramlist pragma `@demand(K...)` on
`#local`/`#export` (RP-5), a strict FORCE-OPT-IN to the demand transform
(RP-6 — no `-demand` needed; unseeded/undemanded/mismatched pragmas all
hard-reject); `-demand` is reframed as the AUTO layer (RP-8) with the
STRICT-vs-BEST-EFFORT sweep question an OPEN STOP; placement is
declaration-only (RP-7). The R3a bracket surface is RETIRED (lexemes kept,
pointed redirect diagnostic). Witness family renamed demand_key_* (all
flagless); demand_key_tc_witness is the ACTIVATION-EQUIVALENCE witness
(symlinked goldens: pragma-compile == flag-compile byte-for-byte). ALSO
landed: tests/OptDiff/rejects/ (46 should-fail cases: 30 adopted from the
ToB parse_errors branch + 16 modern; found+fixed F30, the kind-scoped
demand__ collision scan) and S6-IMPL-1 (`suppress_demand` on Query::Build
— bin/Oracle is demand-blind by contract; ANY new definitional consumer
must pass true). Suite PASS (245), ctest 7/7. Authority:
region-model-diffs.md "THE @DEMAND SURFACE + FORCE-ACTIVATION (session
6)"; CLAUDE.md's DIFF-R3 section; ledger §20(BB). NEW ranked-next
candidates joining the list below: multi-@demand REPETITION (`@demand(A)
@demand(B)` → N stores — the multi-adornment surface lift) and the RP-8
auto-sweep STOP.

## State at session open (session-5 close — DIFF-R3 LANDED)

- Session 5 landed, all committed (tip 7f96f7c5): Stage-B commit split
  (b0155bcd/dc29c4c2/a42579f8), the Tier-1 naming lift (d0911997: interior
  relations nameable in -region-out; 8 .region goldens re-blessed,
  row-contracts 1→2), and R3a (7f96f7c5: the `rel[K...](...)` bracket
  surface + Step-2b V-DECLARED-KEY lint + 9 witnesses, suite 190→199).
- RATIFIED POLICY (recorded in region-model-diffs.md session-5 AMENDMENTS):
  hint-not-mandate with R3b DEAD; unprovable brackets REJECT;
  Minimize/DeterminedBy is the provability lift candidate (O-R3.5).
- Authority docs: region-model-diffs.md "DIFF-R3 AMENDMENTS (session 5)"
  + PANEL RECORD + ADJUDICATED RESOLUTIONS (RES-1..6) + the two
  implementation-finding sections; regional-arch-pseudocode.md Part R3;
  regional-dump-stage-b-desired-states.md §10; CLAUDE.md's new DIFF-R3
  section.
- Standing limitations recorded: V-REGION-CENSUS's row-contracts arm is
  stored-vs-rederived (self-equal by construction — it referees a stubbed
  derive, never emitted-vs-derived; the emitted tie is Planning.cpp's
  freeze recount); the ADJ-R3-C column-survival belt is Tier-2-gated.

## Phase 0 (housekeeping)

1. `git status` / `git log -5`; expect a clean tree at the session-6 tip.
   Rebuild debug; suite SILENT (expect `SUITE: PASS (245)` — 199 cases +
   46 rejects); ctest (expect 7/7).
2. Read: the session-5 AMENDMENTS + RESOLUTIONS + findings sections in
   region-model-diffs.md (the landed contract), Part R3, desired-states
   §10, and the ranked slice's own docs.

## Owner decisions to surface (STOP where load-bearing, do not guess)

- THE RANKING (owner states it in the session-opening message):
  1. **The key-SUBSET covering-array fuzz arm** (the R3a-slotted follow-on:
     placements over (relation × key-subset × redeclaration), verdict =
     accept-iff-exact-match + clean-reject-otherwise + NEVER abort; ORDER
     dropped as inert; slots beside the existing covering-array machinery;
     small, independent).
  2. **Stage-C re-brief** (request-edge node + lifecycle ops) — blocked on
     the D2.6 reader-handle schema + the §6-vs-§11 routing rule (both
     STOPs) + folding the DIFF-R1 panel's 15 normative amendments into
     stage-c-diff.md.
  3. **Tier-2 provenance mini-slice** (origin decl-sets on models, stamped
     at the Connect erasure site, union-only on CSE folds; ONE slice with
     D2.9 proxy-role inheritance; unblocks the ADJ-R3-C belt + undemanded
     interior naming; the first sanctioned post-F1 maintained satellite —
     the panel must litigate that explicitly).
  - Smaller riders available under any ranking: the declared×nested eqgate
    witness on a NON-recursive base (a bracketed
    demand_neighborhood_mono_witness variant — R3A-IMPL-1's follow-on);
    the DIFF-NEXT-4 zero-arity empty-`rel=` parser wart.
- If Stage C: D2.6 and §6-vs-§11 MUST be answered before authoring the
  request-edge row schema or any recursive-demand routing.

## Stop conditions

An unratified decision becomes load-bearing; a predicted dump diverges
unexpectedly; ANY golden (bespoke, oracle, behavioral, eqgate, irgold incl.
`.region`) diverges beyond the predicted set; behavioral-golden divergence
is a HARD STOP (frozen through the Stage-C cutover); any impulse to author
code for an unranked slice. Session-5 precedent: an unexpected corpus
abort/divergence is a FINDING to adjudicate and record (T1-IMPL-1 /
R3A-IMPL-1 shape) — amend the diff doc, never fudge.

## Deliverables

The verified+extended pseudocode (one authority); the reworked critiqued
slice diffs (dated amendments); the desired IR states, determinism-
critiqued; and — if the owner ratifies implementation — the slice landed
with every golden byte-identical except the predicted set (blessed after
review); an updated version of THIS prompt pointing at the ranked-next
slice.
