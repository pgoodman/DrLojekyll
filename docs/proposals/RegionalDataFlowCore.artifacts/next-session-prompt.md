# Next Session: RegionalDataFlowCore — commit gate, then I0 (the reference interpreter)

Continue work in the Dr. Lojekyll repository, branch `keyed-instances`.
The governing discipline is unchanged: reality grounding, byte-exact
goldens, explicit negative witnesses, predict-then-verify at the
single-transform grain, the evil-monkey rule, and complexity guilty until
proven inherent. THE STANDING METHOD (owner directive) applies to every
slice: (a) fleet re-verify + extend the pseudocode against the tip;
(b) express the slice as diffs on the stage docs; (c) adversarial
critique panel (four lenses + determinism where dumps are involved),
refute-verified; (d) desired-IR-output-state diffs from freshly collected
dumps as the predict-then-verify targets. Implementation only when
(a)-(d) are green. Run fleets through the Workflow tool (sonnet
mechanical / opus judgment); silent tests; never bless a golden to make
anything pass.

## Where the prior session (2026-08-02/03) left the tree

EVERYTHING IS UNCOMMITTED working-tree state on top of f0c913e0. The
session executed: full 23-decision owner adjudication
(`owner-adjudication-record.md` — THE decision authority now, including
two 2026-08-03 design-conversation addenda), Branch A in full (F26 df.dfe
split + V-SCC-SEAM; the gate audit — carve-out A is the only remaining
coupling; F23 promoted + `product_in_scc_diff_1`; the 8-case D3.3 landing
set which caught and fixed F27 phantom re-publish + F28 oracle KV algebra,
recorded F29 idx_43 codegen scope; errata 2/5/6), and Stage A in full
(Identity.h typed ids + F4 battery + ctest IdentityTypes; ProjectionRole
in Equals ONLY — see the Hash-fold adjudication; InferConservativeRowContracts
flat-key two-phase post-Stratify; V-CONTRACT-CENSUS / V-MEMBERKEY-REALIZED /
V-AGG-INPUT-KEY always-on, V-NO-COLLAPSE belt-only; lint deleted;
`-contract-out` + three blessed `.contract.opt.golden`s; H-A9 witnesses
proven unconstructible; the DOT stratum-cluster/role/key extension).
Suite: `SUITE: PASS (190 cases)`, ctest 7/7, zero existing-golden churn.
Ledger drafts: `ledger-entry-AW-draft.md` now carries BOTH (AW) and (AX);
KeyedInstances.md still stops at (AV).

## Start here (mandatory, in order)

1. `git status` / `git log -1` — expect the uncommitted session tree; if
   the owner has committed, re-snapshot baselines from the new tip
   (binaries and line anchors in the artifacts are session-local).
2. `CLAUDE.md` — NOTE: it still describes the pre-Stage-A state (suite
   180/181, no contracts layer). Updating it for F26/F27/Stage-A/190 is a
   FIRST-CLASS deliverable this session (see Phase 0).
3. `RegionalDataFlowCore.artifacts/INDEX.md`, then
   `owner-adjudication-record.md` (ratified state + the two 2026-08-03
   design addenda), then `ledger-entry-AW-draft.md` (AW+AX).
4. `stage-i0-interpreter.md` — the next stage's doc (BLOCKED-ON A-corr-1
   was RESOLVED by D1.3: probe-restricted via a per-case `.probes`
   sidecar; OG1/OG2 are I0-authoring-time decisions, now due).

## Phase 0 — housekeeping gates (before any I0 work)

1. **Commit gate (owner-interactive if the tree is still dirty).** The
   session deliberately committed nothing. Propose a commit split to the
   owner (suggested: (1) F26+F23 cleanup + gate-audit artifacts, (2) the
   D3.3 landing set + F27/F28 fixes + F29 record, (3) Stage A + DOT
   extension, (4) docs/artifacts), or take their preferred shape. Do not
   push unless asked.
2. **CLAUDE.md refresh**: suite count 190; the F26 split (df.dfe picks
   WHICH dead-flow pass runs; V-SCC-SEAM always-on); the Stage-A layer
   (Identity.h, ProjectionRole-in-Equals, row contracts, `-contract-out`,
   the three .contract goldens, the new validators, lint deleted —
   agg_distinct_1's lint note is STALE); F27's dedup-table rule for
   table-less monotone taps; the DOT clusters/annotations; the new
   diagnostic-case names.
3. **§20 ledger**: ask the owner whether (AW)+(AX) land in
   KeyedInstances.md now (they are drafted final in
   `ledger-entry-AW-draft.md`).

## The main slice — I0, the reference relational interpreter (ranked #2)

Entry condition: Phase 0 done. Follow `stage-i0-interpreter.md` UNDER the
standing method — its Phase-3 amendments must be applied first:
- **A-corr-1 is decided (D1.3)**: probe-restricted answers via a per-case
  `.probes` sidecar; amend the doc's answer-contract sections to that
  shape and design the sidecar grammar (mirror `.batches` style).
- **OG1 (engine)** and **OG2 (tagged-binary emitter)** are I0-internal
  decisions now due — decide them in the amended doc with rationale, or
  present a short owner question if genuinely taste-bound.
- H8/OG3 (the R-DIFF witness) is SATISFIED by `demand_diff_pub_1` — the
  arm is proven live; cite it.
- The F29 idx_43 codegen bug blocks compiling bound-query-over-KV shapes;
  I0 wants that shape eventually — either fix F29 first (its promotion
  trigger names I0) or scope it out explicitly in the amended doc.
Exit gate: I0 validated against the CURRENT compiler over the corpus
(`.batches` cases: final membership + sorted published deltas; bound
queries via `.probes`), zero disagreements or each disagreement
adjudicated as a finding.

## Paused / queued behind owner input

- **Stage C re-brief queue** (do NOT author Stage C): D2.6 concurrency
  requirement (now sharpened: it is the shared-arrangement READER-HANDLE
  question — owner-bearing = individually retractable capabilities,
  refcount = anonymous count); the §6-vs-§11 bound-query routing rule;
  A2/A4 (ride D2.6); the logical-origin-provenance-on-models direction
  (arrangement/split-vs-share planning input, candidate home = Stage-B
  planning tier); the role-inheritance-through-proxies question (should
  proxy mints inherit kDistinct so set-boundary provenance survives to
  the final graph — zero behavioral risk, changes dump semantics).
- **Stage B**: G1 ratified; ADJ-2/ADJ-3 ratified; the `-region-out` DOT
  twin (cluster_region_<id>) is an owner directive recorded in
  stage-b-diff.md.

## Stop conditions (unchanged)

Write an adjudication finding instead of improvising when: an unratified
decision becomes load-bearing; a predicted dump diverges unexpectedly;
the covering array goes red outside carve-out A; or any impulse arises to
author Stage C/D before the re-brief.

## Deliverables

Committed (or owner-shaped) tree; refreshed CLAUDE.md; §20(AW)/(AX)
landed or explicitly deferred; the amended I0 doc (a)-(d) green; the I0
implementation validated against the current compiler; an updated version
of THIS prompt pointing at Stage B (planning-tier canonicalization) or
the Stage-C re-brief, whichever the owner ranks next.
