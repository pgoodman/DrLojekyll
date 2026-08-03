# Next Session: RegionalDataFlowCore — ground the region model, then I0

Continue in the Dr. Lojekyll repository, branch `keyed-instances`. Governing
discipline unchanged: reality grounding, byte-exact goldens, explicit negative
witnesses, predict-then-verify at the single-transform grain, the evil-monkey
rule, complexity guilty until proven inherent (smaller-not-larger). THE
STANDING METHOD applies to every slice: (a) fleet re-verify + EXTEND the
pseudocode against the tip; (b) express the slice as DIFFS on that pseudocode;
(c) adversarial critique panel (correctness/lifecycle, termination/confluence,
testability/oracle, NECESSITY — refute-verified) over any reworked diff;
(d) author/refresh the DESIRED IR OUTPUT STATES (.df/.contract/.rel/
-region-out/generated header/DOT) as diffs from freshly collected dumps,
critique them (plus the determinism lens), use them as predict-then-verify
targets. Implementation only when (a)-(d) are green.

RUN FOLLOW-ON WORK THROUGH THE WORKFLOW TOOL — deterministic orchestration
(pipeline / fan-out / adversarial-verify), a thin orchestrator, agents return
structured findings (never paste whole files between them). MODEL TIERING:
sonnet for mechanical extraction (anchor verification, function maps, dump
collection, census counts, suite runs); opus for judgment (pseudocode
authorship, diff formulation, adjudication, critique/refute panels). SILENT
tests (capture to files, print only failures). NEVER bless a golden to make
anything pass; derive expectations independently first, bless only via explicit
`runall.sh --bless` after review. Do not commit/push unless asked; identity
`Peter Goodman <peter.goodman@gmail.com>`; keep tool/model/provider details out
of commits and artifacts.

## State at session open (base commit 6eb39b05, branch keyed-instances)

- COMMITTED (6eb39b05): all `docs/proposals/RegionalDataFlowCore.artifacts/`
  design work — the adjudication brief/record, five stage diffs, desired-state
  docs, critique/coverage/necessity reports, `regional-arch-pseudocode.md`, the
  test matrix + evidence, the ledger + prompt drafts, `running-example-
  disassembler.md` (the region-model design thread), and `region-model-
  pseudocode-seed.md` (current architecture + path-forward diffs, THE seed this
  session extends).
- UNCOMMITTED working tree (Stage A + Branch A code — verify still present with
  `git status`): `lib/DataFlow/Identity.h`, `RowContract.{h,cpp}`, the Stage-A
  edits (Query.h, Tuple.cpp, Build.cpp, Optimize.cpp, Format.{cpp,h},
  Aggregate.cpp), the F26/F27 fixes (DeadFlowElimination.cpp, Stratify.cpp,
  Optimize.cpp, ControlFlow/Build/Build.cpp, PassPolicy.h, bin/Oracle),
  `-contract-out` + DOT extension, the D3.3 corpus cases + blessed goldens,
  `tests/DataFlowIdentity/`, FINDINGS.md (F26-F29), runall.sh. Suite:
  `SUITE: PASS (190 cases)`, ctest 7/7. Scratch `*.dot` at repo root are
  throwaway — delete.
- Re-snapshot baselines from the tip; artifact line-anchors are session-local.

## Phase 0 (housekeeping — before design work)

1. `git status` / `git log -1`. Rebuild debug; run the suite SILENT
   (`SUITE: PASS (190)`) and ctest to confirm the uncommitted tree is intact.
2. COMMIT GATE (owner-interactive): propose committing the uncommitted code as
   its own commit(s) — suggested split: (i) F26/F23 df.dfe hygiene split +
   V-SCC-SEAM + gate audit + FINDINGS F26/F23; (ii) the D3.3 landing set +
   F27/F28 fixes + F29 record; (iii) Stage A (identity + contracts +
   `-contract-out` + DOT) — or the owner's preferred shape. Do NOT push unless
   asked.
3. CLAUDE.md REFRESH (it is STALE — describes pre-Stage-A, suite 180): suite
   190; the F26 split (df.dfe picks WHICH dead-flow pass runs; always-on
   V-SCC-SEAM); the Stage-A layer (Identity.h, ProjectionRole-in-Equals-only,
   row contracts, `-contract-out` + 3 `.contract` goldens, the new validators,
   lint deleted); F27's dedup-table rule for table-less monotone taps; the DOT
   clusters/annotations; the new diagnostic-case names.
4. Ask the owner whether `ledger-entry-AW-draft.md` ((AW)+(AX)) lands in
   KeyedInstances.md now.

## The MAIN work — ground the region model (workflow-driven, standing method)

The 2026-08-03 design thread (`running-example-disassembler.md` +
`owner-adjudication-record.md` addenda + `region-model-pseudocode-seed.md`)
produced a rich region model but only a SEED grounding. Build it out rigorously
under the standing method. Concretely:

1. **(a) Fleet re-verify + EXTEND the pseudocode.** Run a sonnet fleet to
   verify every code anchor in `region-model-pseudocode-seed.md` Part 1 (the
   current-architecture pseudocode: ApplyDemandTransform, BuildSubgraphInstance
   Ops / InstanceStore, LowerDRRounds, the Stage-A contract pass) against the
   tip — report drifted/broken. Then extend Part 1 to whole-program fidelity
   where the seed is thin (the forcer/injector path, the commit-sweep/Seal
   epoch boundary, the eager-web reachability). Merge into (or supersede
   toward) `regional-arch-pseudocode.md` so ONE pseudocode authority holds.

2. **(b) Formalize the path-forward DIFFS.** Take `region-model-pseudocode-
   seed.md` Part 2 (DIFF-R1..R6: request-edge node, syntactic pivot split /
   activation-SCC, declared regions, lazy/force barrier, non-prefix
   logical/physical keys, matrix frame) and turn each into a rigorous diff on
   the verified pseudocode — exact node kinds, the sites touched, the soundness
   obligation, the owner decision it depends on. Opus authors.

3. **(c) Adversarial critique panel (refute-verified) over the diffs.** The
   four lenses per diff; opus panels; refute each surviving finding against the
   code. Priority scrutiny: DIFF-R2's activation-SCC termination (the
   coupled-fixpoint restatement of V-CW's disjoint-union lemma — the X≠F
   cross-instance-read discriminator); DIFF-R4's per-key quiescence detection
   (the force-complete barrier soundness); DIFF-R3's flat==declared==nested
   answer-identity (needs I0).

4. **(d) DESIRED IR OUTPUT STATES for the region model.** Author, from freshly
   collected current dumps, the desired `.rel` / `-region-out` / DOT / generated
   header states for the region-model nodes (the `rel[Bound...](Free...)`
   request-edge rendering; the region/cluster dump; the edge_kind LAZY vs
   FORCE_COMPLETE token). Critique with the determinism lens. These become the
   predict-then-verify targets for the eventual Stage-C implementation.

Use the disassembler + non-linear TC as the running carriers throughout
(`running-example-disassembler.md` is the anchor). Where a diff needs an owner
decision not yet ratified (declared-regions hint-vs-mandate + sequence slot;
the §6-vs-§11 request-edge routing rule; D2.6 reader-handle schema), STOP and
write an adjudication finding — do not guess.

## The buildable slice — I0 (the reference interpreter, ranked #2)

I0 remains the next slice that produces RUNNING code, and it is the referee
every region-model equivalence claim (flat == declared == nested) depends on.
Under the standing method, apply its Phase-3 amendments first (A-corr-1 is
RESOLVED by D1.3 = probe-restricted via a per-case `.probes` sidecar; OG1/OG2
are I0-authoring-time decisions now due; H8/OG3 is SATISFIED by
`demand_diff_pub_1`). Note F29 (idx_43 codegen scope bug) blocks compiling
bound-query-over-KV shapes I0 will want — fix-or-scope it in the amended doc.
Exit gate: I0 validated against the CURRENT compiler over the corpus
(`.batches` = final membership + sorted published deltas; bound queries via
`.probes`), zero disagreements or each adjudicated as a finding.

## Paused behind owner input (do NOT author their code)

Stage C authoring stays paused for the re-brief: D2.6 reader-handle schema (=
the shared-arrangement handle question); the §6-vs-§11 bound-query routing
rule; the declared-regions calls (DIFF-R3); the lazy/force barrier (DIFF-R4);
the logical-origin-provenance direction; role-inheritance-through-proxies. All
are in `owner-adjudication-record.md` and the seed doc.

## Stop conditions (write an adjudication finding, do not improvise)

An unratified decision becomes load-bearing; a predicted dump diverges
unexpectedly; the covering array goes red outside carve-out A; any impulse to
author Stage C/D code before the re-brief.

## Deliverables

Committed (owner-shaped) code tree; refreshed CLAUDE.md; (AW)/(AX) landed or
deferred; the extended region-model pseudocode (one authority) + the formalized
critiqued DIFF-R1..R6 + the desired region-model IR states, all standing-method
green; the I0 amendments applied and I0 validated against the current compiler;
and an updated version of THIS prompt pointing at the ranked-next slice (Stage
B planning-tier canonicalization, the declared-regions surface, or the Stage-C
re-brief — whichever the owner ranks).
