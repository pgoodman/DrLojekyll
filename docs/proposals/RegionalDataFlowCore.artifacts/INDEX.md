# RegionalDataFlowCore.artifacts — directory map

## Current resumption authority

Start with **`next-session-prompt.md`**. It contains the current code-grounded
review and roadmap for relation-local `@key` semantics, ordered access paths,
canonical relation facts, exact request ownership, rooted lifecycle for cyclic
binding-state dependencies, partial-binding agreement, and lazy trie/COLT work.
It supersedes older session charters and the assumption that `@key` is owned by
query-demand adornments. Re-verify its code anchors against the branch tip
before implementation.

Historical session provenance: DESIGN-GROUNDING session 2026-08-02 at tip
f0c913e0 (branch `keyed-instances`), phases 1–6. The target at that time was
`../RegionalDataFlowCore.md`. The current resumption prompt is the semantic and
execution-roadmap authority for keyed-instance continuation. The older proposal
still supplies the retained invariants named below; it is not an independent
authority to use when the two documents disagree. The remaining artifacts
preserve grounding, staged-cutover, and critique evidence.

READ ORDER for keyed-instance continuation: this INDEX →
`next-session-prompt.md` → `../RegionalDataFlowCore.md` → older phase reports
only when the current prompt references them. The historical adjudication
artifacts remain evidence, not current semantic authority.

## Supersession matrix

| Topic in `RegionalDataFlowCore.md` | Status for keyed-instance continuation |
| --- | --- |
| Semantic member identity, explicit projection, and derivation support | RETAIN. Canonical regional facts and `SemanticMemberKey` remain the logical truth authority. |
| Exact `RequestEdgeId`, multiple owners, late attachment, caller-qualified results, and drain-before-retire | RETAIN. Counts and runtime handles remain derived implementation aids. |
| Pure-region/effect boundary and epoch results independent of queue order | RETAIN. |
| Region ownership/call forest is acyclic | RETAIN for lexical parent/child region ownership and external request routing. |
| No cyclic or key-changing regional request graph | REFINE. Region ownership stays acyclic, but typed intra-region `RuleActivationEdge` dependencies between binding states may cycle and use rooted SCC liveness. They are not request-owner edges. |
| Recursive evaluation remains inside one instance | SUPERSEDE. Prefix-preserving recursion may stay in one binding state; key-changing recursion may run a joint fixpoint over several binding states. |
| Regional demand has no source annotation | SUPERSEDE. `@key` is a source-level ordered specialization-path contract, not a request, member key, query adornment, or physical-layout promise. |
| Physical layout research excluded | RETAIN AS SEQUENCING. Hash/trie/COLT/Free Join planning follows semantic and lifecycle cutover; it does not define the semantic model. |
| Original Stage A–D implementation order | SUPERSEDE. Use the phases in `next-session-prompt.md`. Historical diffs remain evidence for retained invariants and deletion obligations. |

## Historical session deliverables

- **owner-adjudication-brief.md** — the historical consolidated,
  deduplicated DECISION QUEUE in three tiers (T1 blocks the stage sequence, T2
  blocks a stage's exit gate, T3 standalone pre-Stage-A), each item mapped across
  its source labels, with the panel's recommendation where one exists, a fast-path
  summary, and six session errata.
- **ledger-entry-AW-draft.md** — DRAFT of KeyedInstances.md §20(AW) (the regional
  epoch open). The owner decides whether/where it lands; KeyedInstances.md is
  untouched (its ledger stops at (AV)).
- **next-session-prompt.md** — the current keyed-instance review and execution
  roadmap. It supersedes the session-8/K5 successor charter formerly stored at
  this path.
- **INDEX.md** — this file.

## Historical architecture pseudocode

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

## Session 3 (2026-08-03) — region-model grounding + I0 landed

- **regional-arch-pseudocode.md Part R** — the fleet-verified region-model
  current-architecture pseudocode (R.1.1-R.1.7 + the drift ledger: 1 broken,
  10 drifts; the kInstanceDeath gate, LowerDRRounds loop-carried state, and
  InstanceStore two-buffer model corrections are load-bearing). Supersedes
  region-model-pseudocode-seed.md Part 1.
- **region-model-diffs.md** — DIFF-R1..R6 formalized (implementer-grade, on
  Part R) + per-diff 4-lens panel records with code-refuted verdicts and
  normative amendments. Supersedes the seed's Part 2. Panel survival rate
  flagged un-triaged.
- **region-model-desired-states.md** — the desired region-model IR output
  states (.rel request-edge family, G1 -region-out blocks, DOT cluster_region
  twins, D2.6-paused header stub) authored from fresh carrier dumps, with the
  determinism-critique ledger applied. The Stage-C predict-then-verify targets.
- **stage-i0-interpreter.md §7/§8** — the dated amendments (OG1-parsed /
  OG2-tool / OG4-carve ratified; D1.3 .probes contract; the plain-compile CBF
  adjudication) and the EXIT-GATE RECORD: I0 LANDED, 63/63, suite 190 green
  with run_refinterp live, F29 promoted+fixed.
- **owner-adjudication-record.md** (appended) — the adornment-fuzzing owner
  direction (placement-enumeration harness + bracket parser obligations).

## Later landed-state and implementation evidence (2026-08-03–05)

These files describe intermediate compiler states and the tests that landed
with them. They are useful for locating current code and deciding which tests
to rewrite, but `next-session-prompt.md` supersedes their key/adornment,
fallback, and stage-sequencing semantics.

- **stage-b-seed.md** and **stage-b-landed-seed.md** — pre-landing and landed
  whole-pipeline views of the degenerate `FrozenRegionalProgram` layer.
- **key-pragma-landed-seed.md** — landed architecture after `@demand` became
  `@key` and pragma-selected nested lowering existed.
- **k1-multikey.md** — grounded implementation record for repeated `@key`
  pragmas, unordered set-of-sets validation, multi-adornment demand coupling,
  and its golden corpus. Its parser/code census remains evidence; its semantic
  coupling is superseded.
- **k6-riders.md** and **k6-landed-seed.md** — redeclaration, diagnostic-range,
  DOT, and referee riders plus the post-K6 pipeline. The immediately-previous-
  redeclaration implementation documented here is the source of the remaining
  keyed/unkeyed/keyed consistency gap.
- **k5-provenance.md** and **k5-landed-seed.md** — Tier-2 origin-declaration
  provenance design, landed evidence, and the post-K5 pipeline.
- **s9-landed-seed.md** and **s9-mint-sloc-diag-formulation.md** — post-session-9
  compiler/referee architecture and deferred diagnostic-source-location work.
- **s5-mint-tags.md**, **s5-mint-tags-table.md**, and
  **s5-mint-tags-desired-states.md** — the stable DataFlow mint-tag design,
  complete site table, and predicted dump deltas. Mint tags remain debugging
  metadata, never semantic identity.
- **running-example-disassembler.md** — recursive-disassembler design terrain,
  including pivot splits and activation-call-graph examples. Re-evaluate its
  old bracket/request terminology through the supersession matrix.
- **region-model-pseudocode-seed.md** — superseded seed for the region-model
  architecture. `regional-arch-pseudocode.md` is the later executable census;
  neither overrides the current target semantics.
