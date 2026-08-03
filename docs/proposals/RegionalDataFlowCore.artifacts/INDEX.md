# RegionalDataFlowCore.artifacts — directory map

Session provenance: DESIGN-GROUNDING session 2026-08-02 at tip f0c913e0 (branch
`keyed-instances`), phases 1–6. No production code, no goldens, no commits this
session. The normative target is `../RegionalDataFlowCore.md` (the proposed
replacement architecture: request edges, typed identity, deletes the `-demand`
machinery); these artifacts ground it against landed code, diff it into a staged
cutover, critique it adversarially, and enumerate the owner decisions it owes.

READ ORDER for a fresh session: this INDEX → `owner-adjudication-brief.md` (the
decision queue) → `next-session-prompt.md` (the charter) → the phase reports and
stage diffs as the brief references them.

## Start-here deliverables (the session's outputs)

- **owner-adjudication-brief.md** — THE central deliverable. The consolidated,
  deduplicated DECISION QUEUE in three tiers (T1 blocks the stage sequence, T2
  blocks a stage's exit gate, T3 standalone pre-Stage-A), each item mapped across
  its source labels, with the panel's recommendation where one exists, a fast-path
  summary, and six session errata. START HERE.
- **ledger-entry-AW-draft.md** — DRAFT of KeyedInstances.md §20(AW) (the regional
  epoch open). The owner decides whether/where it lands; KeyedInstances.md is
  untouched (its ledger stops at (AV)).
- **next-session-prompt.md** — the successor charter (SUPERSEDES the prior
  design-grounding charter of the same name). Next session: owner adjudication
  first, then the pre-Stage-A cleanup slice OR the Stage A implementation slice.
- **INDEX.md** — this file.

## Architecture pseudocode (the grounding authority)

- **regional-arch-pseudocode.md** — whole-program pseudocode of the CURRENT
  demand/keyed-instance architecture (the layer the proposal replaces) with Stages
  A–D as diffs. Fleet-verified 2026-08-02; §4b (runtime epoch path) and §4c
  (generated cursor lifetime — the F3 two-lifetimes gap) are new this pass. §6's
  Stage-A hunk was amended IN PLACE 2026-08-02 to the enum-in-identity realization
  (brief Errata-3 RESOLVED; O-A1 stays owner-gated) and §6/§7 now point at the
  stage docs / the brief as the authorities.

## The five stage diffs (Phase 2)

- **stage-a-diff.md** — typed identity + explicit projections + row contracts;
  lint→contract-validation. Verdict: BLOCKED-ON T-conf-1/T-conf-2 (cyclic-graph
  contract soundness). O-A1/O-A2/O-A3, E-A2/E-A3 escalations live here.
- **stage-i0-interpreter.md** — the reference relational interpreter (review's
  inserted step, ranked #2). Verdict: BLOCKED-ON A-corr-1 (bound-query probe-
  enumeration contract). OG1-4 engine/emitter decisions; H8/OG3 = the R-DIFF
  witness.
- **stage-b-diff.md** — the planning regional program becomes canonical (pure
  refactor, 180 goldens pin it). Verdict: SOUND-WITH-AMENDMENTS (the only clean
  stage). ADJ-2/ADJ-3 carried-open; the `-region-out` grammar is left open.
- **stage-c-diff.md** — request edges replace forcing (THE cutover). Verdict:
  BLOCKED-ON corr-1 (force.dr `@first`); corr-3 (permanent-root) required. H-J
  Variant A/B = the Concern-2 decision; E1–E6 escalations.
- **stage-d-diff.md** — deep forest + instance-qualified local recursion. Verdict:
  BLOCKED-ON T-oracle-4 (reject-vs-silent-full-materialize, = Concern 2 at Stage
  D); A-corr-3 (recursion×detach) required. V-PI vs V-CW realization choice.

## The four desired-state docs (Phase 4)

- **df-stage-a-desired-states.md** — the `.df`/`.contract` dump after Stage A.
  SOUND-WITH-AMENDMENTS. §4.2 census line corrected 2026-08-02 per RDA-C1
  (`views=20 contracts=20 na=9`; brief Errata-4 RESOLVED). AI-1 = in-`.df` vs
  separate-sink decision.
- **regional-dump-stage-b-desired-states.md** — the new `-region-out` dump
  (grammars G1/G2/G3 all rendered on join_1). SOUND-WITH-AMENDMENTS (the row-
  contract set/count needs a specified oracle). AI-2 = the grammar decision.
- **rel-stage-c-desired-states.md** — the `.rel` dump after Stage C (request-edge/
  lifecycle ops). SOUND-WITH-AMENDMENTS on the body; BLOCKED-ON the tc `.rel` pin
  (C2/AI-4 = Concern 2). N1 double-write is a required fix; AI-5 = the
  `differential=` flag.
- **header-stage-c-desired-states.md** — the generated header after Stage C
  (RegionalCursor, move-only lease, deleted retract). SOUND-WITH-AMENDMENTS. AI-6 =
  the pivotal owner-identity-vs-refcount schema decision; AI-7 = A2–A5.

## The critique + audit reports (Phase 3)

- **phase3-critique-report.md** — per-stage findings (59 surviving / 23 refuted),
  refuter-adjusted severities, amendments per finding, and the cross-stage roll-up
  (5 blocking findings; Stage B the only clean stage).
- **phase3-coverage-audit.md** — the completeness ledger: §11 validators (5
  ORPHAN), §15 invariants (3 caveated), §12.3 witnesses (2 UNHOMED), §14 deletions
  (0 ordering violations), X1–X4 cross-stage inconsistencies.
- **necessity-audit.md** — the complexity/necessity lens: mechanism verdicts + 5
  ranked simplification candidates. Headline: NO legacy-two-authority or
  unnecessary mechanism survived; every candidate folds into a stage diff (smaller-
  not-larger holds).

## The desired-state critique (Phase 4)

- **phase4-critique-report.md** — refute-verified critique of the four desired-
  state docs, with the collected owner-facing ADJUDICATION INPUTS AI-1..AI-8.

## The test matrix (Phase 5)

- **test-matrix-proposal.md** — the gate×mode covering array (13 configs, complete
  strength-2 CA over 8 PassPolicy gates) + the feature-mixing directed corpus (14
  crossings / 15 cases), EMPIRICALLY verified at tip. Carve-out A (kvindex_1/canon-
  off), carve-out B (df.dfe SIGABRT, PREDICTION-FAILED), PF-1..PF-4, the 9-case
  pre-Stage-A landing set.

## Persisted evidence (promoted from session scratch, 2026-08-02)

- **passpolicy-gates.md** — the authoritative PassPolicy gate table (8 gates:
  registry name, gated site, what disabling skips, default, mode composition;
  df.sink vestigial; ApplyDemandTransform deliberately un-gated).
- **covering-array-verified.md** — the full covering-array design WITH per-claim
  empirical verification (12 VERIFIED / 2 PREDICTION-FAILED / 6 unverifiable-
  today), incl. the df.dfe SIGABRT evidence and the kvindex_1 canon-off
  uniqueness sweep. `test-matrix-proposal.md` is the synthesized deliverable;
  this is its evidence base.
- **feature-mixing-verified.md** — the 15-case feature-mixing design with
  per-case compile/reject verification and the reject-site corrections (PF-3/
  PF-4 evidence).
- **probes-feature-mixing/** — the 15 verified `.dr` probe programs exactly as
  compiled during verification; the starting point for the pre-Stage-A landing
  set (brief D3.3).

## The standing review (Phase 0 / prior session)

- **fable-review-2026-08-01.md** — the first review: the four concerns
  (interpreter-before-cutover, tagged-binary oracle, inadmissible-extraction
  semantics, extract-always-is-a-cost-policy, routed-result realization) + the
  recommended ranking. The seed the whole session grounds.
</content>
