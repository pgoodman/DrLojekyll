# Session 17 prompt — keyed-instance rewrite: P2 (typed FrozenRegionalProgram owner) grounding + execution-readiness

Continue in the Dr. Lojekyll repo on branch `keyed-instances`. **Re-check the branch tip and worktree
first.** The tip should be `dc965d3c` ("P1 greenfield cut: delete the -demand / keyed-instance authority").
**P1 IS LANDED** — the destructive demand/keyed-instance deletion is committed (compile-clean, OptDiff
`SUITE: PASS (222 cases)`, ctest 4/4). The working tree should be clean.

## Context you must load (the greenfield rewrite state)

- **`@key` is now INERT parsed metadata** — parse-surface rejects still fire, no semantic/activation effect.
  A bound `#query` is a permanent-root observation reading the fully-materialized relation. `-demand*` flags,
  the demand transform, and the keyed-instance nested lowering are DELETED. `Query::Build` is 3-arg.
- The rewrite is **greenfield delete-then-rebuild** (compiler not in use): P1 removed the demand authority;
  **P2–P9 rebuild the typed regional model.** The P1→P4 window (full-materialization fallback) costs nothing;
  every post-P1 gate is STRUCTURAL, never answer-equality. Motivation: memory `greenfield-rewrite-motivation`.

## Read in this order

1. `RegionalDataFlowCore.artifacts/INDEX.md` (read-order + supersession matrix).
2. **`session-17-whole-program-seed.md`** — START HERE. The POST-P1 CONSOLIDATED whole-program view: §0
   status (what P1 changed / what's grounded), §1 the CURRENT pipeline pseudocode grounded in real post-cut
   code (§1.3a is the P2 motivation), §2 the four-authority target, §3 the path forward P2–P9 as diffs (P2 is
   the next actionable), §4 what this session must do. **All anchors are byte-current at tip `dc965d3c`** — the
   pre-P1 "current pipeline" docs (session-16 seed §1 and earlier) are now HISTORICAL; do not ground on them.
3. `next-session-prompt.md` — the SEMANTIC authority (four authorities, two edges, ordered access paths,
   the "Avoid these false starts" checklist, the "Target semantic representation" typed records).
4. `keyed-rewrite-reconstruction-diffs.md` §3-P2 (the P2 operational diff — §5.3 H1/H2/M1 already folded in as
   primary text + the s16 anchor re-verification + the false-start certification) and §3 P3–P6 for the sequel.
5. `keyed-rewrite-p7p9-diffs.md` (P7–P9) + the two critique files (`keyed-rewrite-reconstruction-critique.md`
   B1–B5, `keyed-rewrite-p7p9-critique.md`) for the standing blocking realizations (B2–B5, D1).
6. `keyed-rewrite-ir-desired-states.md` (predict-then-verify IR — §6/§7 no-baseline carriers now need
   re-grounding as POST-P1 compile witnesses, since the rejects they leaned on are gone).

## The decision this session opens on

**[OWNER STOP] Is P2 execution green-lit, or stay in grounding?**

- **If grounding (default):** do the grounding loop below on the POST-P1 codebase — deepen toward P2
  execution-readiness. Do NOT edit production code toward P2 without an explicit owner go-ahead. Confirm via
  git you touched only docs.
- **If green-lit:** execute P2 against `reconstruction-diffs §3-P2` — introduce the typed RegionTemplate /
  RelationSchema records, rename the `query` owner field to `dataflow_graph` (accessor-only), re-point the
  in-`Build` recount at the typed `R`, derive all render from the typed records, and land it with the
  structural exit gate green (surviving region goldens re-derive with no --bless; grep-zero for downstream
  `frozen.Query()` re-parse / `member_key_text` string re-parse; IdentityTypes ctest; OptDiff SUITE: PASS).
  Keep it one coherent commit; the region goldens are the anti-stub belt.

## This session's task (the grounding loop — do this whether or not P2 lands)

Run the **build-pseudocode → design-goal diffs → critique → IR-desired-states** loop, keeping
`session-17-whole-program-seed.md` current as the whole-program backbone.

1. **Build out / refresh the whole-program pseudocode of the architecture and algorithms** at implementer
   altitude, weighted toward the NEXT step (P2): re-express the key algorithms as pseudocode grounded in the
   REAL post-P1 code, and run the P2 analog of the P1 symbol-driven grep — **enumerate every current consumer
   that re-parses the render-STRING shells or unwraps `frozen.Query()`** (`RegionalAbi.decl_text`/`route_text`,
   `RegionalPort.head_text`/`fields_text`, `RegionalContract.member_key_text`/`support_text`, and
   `Program::Build`'s `query = frozen.Query()` unwrap), so the P2 typed-owner cut has a compile-clean inventory.
   Ground every anchor in real code (all are byte-current at tip `dc965d3c`).

2. **Formulate design-goal diffs** on the deepened pseudocode at hunk grain, with DISCRIMINATING STRUCTURAL
   exit gates (census-token / cursor-shape region-`s<id>`-vs-query-`pos` / node-count / provenance-survival /
   compile-abort). Answer-equality is a LOST CHECK for the whole post-P1 layer. Keep the four authorities
   separate (logical fact / residual / logical path / physical structure) and the two edges distinct
   (RequestEdge vs RuleActivationEdge).

3. **Critique the diffs adversarially.** Run a refuter panel against the real POST-P1 code + the retained
   `RegionalDataFlowCore.md` invariants + the "Avoid these false starts" checklist. Special scrutiny: does the
   P2 typed-owner move keep the ownership forest ACYCLIC (B2 permanent-root-as-RequestEdge-owner)? Does it
   accidentally re-introduce a DataFlow-mutation-plus-post-optimization-recognition authority (it must NOT —
   freeze is a pure post-Optimize read)? Is the B4 per-FACT DRed keyed on `RegionalFactId`, never on
   `BindingStateId`? VERIFY findings against the codebase — do not merely assert. Rank survivors; record
   refuted diffs (and refuter claims that don't survive verification) as certifications.

4. **Author/extend the desired IR output states.** As P2 grounds, predict-then-verify the target IR dumps
   (`-region-out` / `-contract-out` / `.rel` / generated C++) with STRUCTURAL pins only (typed records rendered;
   the recount belt; no string re-parse). Sonnet pulls the current carrier dumps as the baseline (read-only
   compiles into a scratch dir); opus authors the desired states.

## How to do the work (workflows + model tiering)

Use the **Workflow tool** for the fan-out/critique structure — the owner has opted into multi-agent
orchestration for this grounding. Several sequential single-phase workflows, staying in the loop between them,
beat one mega-workflow. Suggested shape (adapt freely):

- **Ground + inventory (fan-out, mostly sonnet):** one reader runs the P2 string-shell / `frozen.Query()`
  consumer enumeration and returns a compile-clean re-point inventory; one reader per P2 sub-area (the typed
  RegionTemplate/RelationSchema records; the render-derives-from-typed-records mandate; the re-pointed recount
  belt; the friend-leak `row_contracts` access it must keep per M1). Each returns first-cut operational
  pseudocode + a drift table (everything is byte-current at `dc965d3c` — re-verify before trusting). Sonnet for
  the mechanical census/extraction; opus for any genuine judgment reader.
- **Diff + amend (opus, effort high):** author the P2 typed-owner diffs + the compile-clean re-point inventory
  into the authority docs in place. Judgment work — opus.
- **Critique (adversarial panel, opus, effort high):** per-area refuters + a cross-phase completeness/invariants
  critic, each prompted to BREAK the diffs against real code + the retained invariants; verify, don't assert; rank.
- **IR desired-states (opus author, sonnet carrier-dump extraction):** sonnet pulls current `-region-out`/
  `-contract-out`/`.rel` dumps as the diff baseline; opus authors the desired typed-render states.

Keep the orchestrator thin: subagents return distilled pseudocode/findings, not raw file dumps. Front-load
cheap artifacts (dump the current carrier IRs once into scratch; embed paths in agent prompts). Land results as
updates to the existing docs; keep `session-17-whole-program-seed.md` current as the whole-program backbone.

## Guardrails

- Re-verify every cited anchor before trusting it. Since P1 landed, MANY anchors moved — treat the
  `session-17-whole-program-seed.md` §1 line numbers as current-at-`dc965d3c` and re-grep any older doc's anchor.
- Do NOT edit production code toward P2 without an explicit owner go-ahead. If green-lit, use a symbol/shape-driven
  acceptance check (the string-shell re-point inventory), not a review read, and keep the region goldens as the
  anti-stub belt.
- Do not bless any golden except as an explicit, reviewed consequence of a green-lit P2 cut. If you compile
  carriers for baseline dumps, that is read-only; the suite must stay `SUITE: PASS`. Confirm via git you touched
  only docs unless P2 is green-lit.
- Prefer silent-on-success builds/tests (capture to file, surface only on failure); tokens matter.
- Update memory (`regional-dataflow-core-epoch.md`, `greenfield-rewrite-motivation.md`, `MEMORY.md`) at session
  close with what landed and the next ranked step.
