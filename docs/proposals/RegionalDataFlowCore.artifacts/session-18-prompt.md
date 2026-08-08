# Session 18 prompt — keyed-instance rewrite: P3 (RequestEdge / FactDerivation acyclic slice) grounding + execution-readiness

Continue in the Dr. Lojekyll repo on branch `keyed-instances`. **Re-check the branch tip and
worktree first.** The tip should be `ae207c36` ("P2: FrozenRegionalProgram becomes the typed
semantic owner"). **P1 + P2 ARE LANDED** — the demand/keyed-instance cut (P1) and the typed-owner
retype (P2) are committed (compile-clean, OptDiff `SUITE: PASS (222 cases)`, ctest 4/4). The working
tree should be clean.

## Context you must load (the greenfield rewrite state)

- **P2 shipped the typed semantic owner.** `FrozenRegionalProgram` stores ONE typed `RegionTemplate`
  (`include/drlojekyll/Regional/Regional.h`): `relation_schemas` (positional `member_key_positions`
  masks + `support`), typed `abis`/`ports`/`permanent_roots`, and `rules`/`recursive_components`
  RESERVED-EMPTY. Render derives from `p.Region()`; accessor is `DataFlowGraph()` (not `Query()`).
  The friend leak `query.impl->row_contracts` is kept. Design/critique: `p2-typed-owner-grounding.md`.
- **`@key` is still INERT parsed metadata**; a bound `#query` is a permanent-root observation served
  by `BuildQueryEntryPoint` reading the fully-materialized relation. `request_ports = 0`. The
  RESIDUAL / EDGE / physical authorities do NOT exist yet.
- The rewrite is **greenfield delete-then-rebuild**. P3–P9 build the typed edge/residual/physical
  model. The P1→P4 window (full-materialization fallback) costs nothing; every post-P1 gate is
  STRUCTURAL, never answer-equality. Motivation: memory `greenfield-rewrite-motivation`.

## Read in this order

1. `RegionalDataFlowCore.artifacts/INDEX.md` (read-order + supersession matrix).
2. **`session-18-whole-program-seed.md`** — START HERE. The POST-P2 CONSOLIDATED whole-program view:
   §0 status, §1 the CURRENT pipeline pseudocode grounded in real post-P2 code (§1.3 is the refreshed
   typed owner; §1.3a is the P3 motivation), §2 the four-authority target, §3 the path forward P3–P9
   as diffs (P3 is the next actionable), §4 what this session must do. **All anchors are byte-current
   at tip `ae207c36`** — the pre-P2 "current pipeline" docs (session-17 seed §1.3 and earlier) are now
   HISTORICAL for the Regional layer.
3. `p2-typed-owner-grounding.md` — the P2 record (the typed records P3 builds on; note
   `RelationSchema.member_key_positions` IS the H3 positional member key P3 reuses for fact identity).
4. `next-session-prompt.md` — the SEMANTIC authority (four authorities, two edges, ordered access
   paths, the "Avoid these false starts" checklist, the "Target semantic representation" typed records).
5. `keyed-rewrite-reconstruction-diffs.md` §3-P3 (lines 347-416, the P3 operational diff) + §5.4
   (lines 728-744, the B2/H3/M3/L5 amendments) and §3 P4–P6 for the sequel.
6. `keyed-rewrite-reconstruction-critique.md` (B1–B5) + `keyed-rewrite-p7p9-critique.md` for the
   standing blocking realizations (B2/B4/B5, D1).
7. `keyed-rewrite-ir-desired-states.md` (predict-then-verify IR — the P3 `.rel` request-edge/lifecycle
   family + the request-edge `-region-out` render; §6/§7 no-baseline carriers need re-grounding as
   POST-P2 compile witnesses since the rejects they leaned on are gone).

## The decision this session opens on

**[OWNER STOP] Is P3 execution green-lit, or stay in grounding?**

- **If grounding (default):** run the grounding loop below on the POST-P2 codebase — deepen toward P3
  execution-readiness. Do NOT edit production code toward P3 without an explicit owner go-ahead.
  Confirm via git you touched only docs.
- **If green-lit:** execute P3 against `reconstruction-diffs §3-P3` + §5.4 — introduce
  `lib/Regional/RegionInstance.h` (the typed ids: RootLeaseId/PermanentRootId/RegionInstanceId/
  BindingStateId/RegionalFactId/RequestEdgeId/RuleActivationEdgeId/FactDerivation/RoutedResultId), the
  `AddRequestEdge`/`AddDerivation`/`EvaluateEpoch` acyclic-slice logic LAYERED over the retained
  full-materialization backend (M3 — do NOT replace codegen), and root the permanent roots as
  request-edge owners (B2). Land it with the STRUCTURAL exit gate green (the directed key_* battery:
  exact owner count, 2nd-requester adds routed-results-not-derivations, RemoveRequestEdge retracts only
  routed copies, member-key intern via `member_key_positions`; a no-bound-query program still publishes
  its full answer; OptDiff SUITE: PASS; ctest 4/4). Keep it one coherent commit.

## This session's task (the grounding loop — do this whether or not P3 lands)

Run the **build-pseudocode → design-goal diffs → critique → IR-desired-states** loop, keeping
`session-18-whole-program-seed.md` current as the whole-program backbone.

1. **Build out / refresh the whole-program pseudocode** at implementer altitude, weighted toward the
   NEXT step (P3): re-express the key algorithms (RequestEdge ownership forest, RuleActivationEdge
   derivation graph, FactDerivation support counting, EvaluateEpoch rooted-reachability sweep) as
   pseudocode grounded in the REAL post-P2 code, and run the P3 analog of the P1/P2 symbol grep —
   **enumerate every current site P3 must root or layer over**: the query-entry loop
   (`BuildQueryEntryPoint`/`BuildEmptyQueryEntryPoint`, lib/ControlFlow/Build/Build.cpp:481-516), the
   publish path, the retained induction backend M3 wraps, and where request/activation tracking
   attaches. Ground every anchor in real code.
2. **Formulate design-goal diffs** on the deepened pseudocode at hunk grain, with DISCRIMINATING
   STRUCTURAL exit gates (owner-count / routed-result-vs-derivation / drain-before-retire /
   member-key-intern / compile-abort). Answer-equality is a LOST CHECK. Keep the four authorities
   separate (logical fact / residual / logical path / physical structure) and the two edges distinct
   (RequestEdge vs RuleActivationEdge).
3. **Critique the diffs adversarially.** Run a refuter panel against the real POST-P2 code + the
   retained `RegionalDataFlowCore.md` invariants + the "Avoid these false starts" checklist. Special
   scrutiny: does P3 keep the RequestEdge ownership forest ACYCLIC while allowing RuleActivationEdge to
   cycle (rooted SCC liveness)? Does it avoid a SECOND fact authority (BindingState owns frontiers +
   FactDerivation ids, NOT facts)? Is B4 per-FACT DRed keyed on `RegionalFactId`, never on
   `BindingStateId`? Does the B2 all-free permanent-root owner preserve every current publish? Is fact
   identity projected through `member_key_positions` (the P2 mask), never a new value-id bridge?
   VERIFY findings against the codebase — do not merely assert. Rank survivors; record refuted diffs as
   certifications.
4. **Author/extend the desired IR output states.** As P3 grounds, predict-then-verify the target IR
   dumps (`-region-out` request-edge/permanent-root-owner render, the `.rel` request-edge/lifecycle op
   family, generated C++) with STRUCTURAL pins only. Sonnet pulls the current carrier dumps as the
   baseline (read-only compiles into a scratch dir); opus authors the desired states.

## How to do the work (workflows + model tiering)

Use the **Workflow tool** for the fan-out/critique structure — the owner has opted into multi-agent
orchestration for this grounding. Several sequential single-phase workflows, staying in the loop
between them, beat one mega-workflow. Suggested shape (adapt freely):

- **Ground + inventory (fan-out, mostly sonnet):** one reader runs the P3 layer-site enumeration
  (query-entry loop / publish path / backend M3 wraps) and returns a compile-clean inventory; one
  reader per P3 sub-area (the typed RegionInstance ids; AddRequestEdge/AddDerivation; EvaluateEpoch
  rooted-reachability; the B2 permanent-root-owner rooting). Each returns first-cut operational
  pseudocode + a drift table (byte-current at `ae207c36` — re-verify before trusting). Sonnet for
  mechanical census/extraction; opus for genuine judgment readers.
- **Diff + amend (opus, effort high):** author the P3 typed-edge diffs + the compile-clean inventory
  into the authority docs in place. Judgment work — opus.
- **Critique (adversarial panel, opus, effort high):** per-area refuters + a cross-phase completeness/
  invariants critic, each prompted to BREAK the diffs against real code + the retained invariants;
  verify, don't assert; rank.
- **IR desired-states (opus author, sonnet carrier-dump extraction):** sonnet pulls current
  `-region-out`/`.rel` dumps as the diff baseline; opus authors the desired request-edge render states.

Keep the orchestrator thin: subagents return distilled pseudocode/findings, not raw file dumps.
Front-load cheap artifacts (dump the current carrier IRs once into scratch; embed paths in agent
prompts). Land results as updates to the existing docs; keep `session-18-whole-program-seed.md` current
as the whole-program backbone.

## Guardrails

- Re-verify every cited anchor before trusting it. Treat the `session-18-whole-program-seed.md` §1 line
  numbers as current-at-`ae207c36` and re-grep any older doc's anchor (many moved at P2 — the Regional
  layer especially).
- Do NOT edit production code toward P3 without an explicit owner go-ahead. If green-lit, use a
  symbol/shape-driven acceptance check (the layer-site inventory + the directed key_* structural
  battery), not a review read, and keep the region goldens as the anti-stub belt.
- Do not bless any golden except as an explicit, reviewed consequence of a green-lit P3 cut. If you
  compile carriers for baseline dumps, that is read-only; the suite must stay `SUITE: PASS`. Confirm via
  git you touched only docs unless P3 is green-lit.
- Prefer silent-on-success builds/tests (capture to file, surface only on failure); tokens matter.
- Update memory (`regional-dataflow-core-epoch.md`, `MEMORY.md`) at session close with what landed and
  the next ranked step.
