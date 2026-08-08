# Session 16 prompt — keyed-instance rewrite: P1-execution-readiness + continued whole-program grounding

Continue in the Dr. Lojekyll repo on branch `keyed-instances`. **Re-check the branch tip and worktree
first.** The tip should be `6d6248a2` ("Lock the @key Phase-0 language contract") — P0 is COMPLETE and
COMMITTED (the parser order-significance flip + the safe s11 Phase-0 rejects, suite `SUITE: PASS (252
cases)`). The session-15 grounding docs are added but UNCOMMITTED (docs only): `keyed-rewrite-p7p9-diffs.md`
(s15-amended), `keyed-rewrite-p7p9-critique.md` (s14 + s15 re-critique), `keyed-rewrite-ir-desired-states.md`
(§9–§12), `session-15-whole-program-seed.md`, `session-16-whole-program-seed.md`, plus the modified
`INDEX.md`. Nothing was blessed.

## Read in this order

1. `RegionalDataFlowCore.artifacts/INDEX.md` (read-order + supersession matrix).
2. `next-session-prompt.md` — the SEMANTIC authority: `@key` is a relation-local, ORDERED access-path
   declaration, NOT a query adornment / demand opt-in. Its "Avoid these false starts" list (lines 823-842) is
   a checklist. Retain its member-identity / exact-request-ownership / caller-qualified-result / epoch /
   drain-before-retire invariants.
3. **`session-16-whole-program-seed.md`** — START HERE. The CONSOLIDATED whole-program view: §1 current
   pipeline pseudocode (incl. the physical layer), §2 the four-authority target, §3 the path forward P0–P9 as
   diffs (P0 ✅ complete; **P1 is the next actionable step**), §4 what this session must do. **Read its
   ANCHOR-VALIDITY note first** — HEAD differs from the doc-anchor baseline `46a404d4` ONLY in `lib/Parse/*` +
   tests, so every DataFlow/Rel/ControlFlow/Regional/CodeGen/Runtime anchor the authority docs cite is
   byte-identical at HEAD; only re-verify a `lib/Parse/*` line before trusting it.
4. `keyed-rewrite-reconstruction-diffs.md` §1 (P1 base inventory) + §6 (the 7 re-grep consumers) + §3 (P2–P6
   operational diffs) — the P1+P2–P6 authority. `keyed-rewrite-reconstruction-critique.md` — the B1–B5 gate.
5. `keyed-rewrite-p7p9-diffs.md` (P7–P9, s15-amended in §1–§3) + `keyed-rewrite-p7p9-critique.md` (§ "Session-15
   re-critique": the P8a→P7 dissolution + all survivors folded).
6. `keyed-rewrite-ir-desired-states.md` §1–§12 (predict-then-verify IR; §12 items 13–15 = the s15 corrections).

## The decision this session opens on

**[OWNER STOP] Is the destructive P1 cut green-lit?**

- **If YES:** execute the P1 cut against the `reconstruction-diffs.md` §1 + §6 inventory. Before deleting,
  run the **symbol-driven `git grep -l <deleted-symbol>` acceptance gate** for EVERY deleted symbol
  (`ApplyDemandTransform`, `FabricateDemand*`, `demand__`, `QueryDemandForcing`, `GuardAnnotation`,
  `RecognizedSubgraph`, `GuardAnnotationIndex`, `kSubgraphInstantiate`, `kInstanceDeath`, `kInstanceSeal`,
  `ResolveLiveRecognition`, `BuildSubgraphInstanceOps`, `InstanceStore`, `EmitSubgraphInstance`,
  `ProgramSubgraphInstanceRegion`, `CollectDemandInteriorDecls`, …) — each must return ONLY files already in
  the inventory; a hit outside it is the 8th (9th, …) un-enumerated consumer the range-anchored reads keep
  missing. Land it compile-clean, re-bless `key_tc_witness.contract` + `key_multi_adorn_witness.contract` (drop
  the declared-key line — the `@key`-inert post-cut baseline), and confirm `SUITE: PASS`. The `.rel` census
  multiset (kSubgraphInstantiate/kInstanceSeal exact per carrier) is the strongest anti-silent-pass belt.
- **If NO (default):** stay in GROUNDING — deepen the whole-program view toward P1 execution-readiness and P2
  (the first constructive phase). Do NOT begin the destructive cut. Confirm via git you touched only docs.

## This session's task (the grounding loop — do this whether or not P1 lands)

Continue the **build-pseudocode → diff → critique → IR-desired-states** loop, keeping
`session-16-whole-program-seed.md` current as the whole-program backbone.

1. **Build out / refresh the whole-program pseudocode of the architecture and algorithms.** Re-express the key
   algorithms as pseudocode at implementer altitude, weighted toward the NEXT step: (a) the P1 cut as an atomic
   diff — run the symbol-driven grep DRY and reconcile it against `reconstruction-diffs.md` §1+§6 (is the
   inventory now exhaustive? name any new consumer); (b) P2 — making `FrozenRegionalProgram` the TYPED semantic
   owner (replace the render-ready-strings/Query-passthrough shell with typed RegionTemplate / RelationSchema /
   RowContract / AccessRequirement records; Program/Rel/ControlFlow consume only the frozen representation).
   Ground every anchor in real code (only `lib/Parse/*` can have drifted).

2. **Formulate design-goal diffs** on the deepened pseudocode at hunk grain, with DISCRIMINATING exit gates.
   Answer-equality is a LOST CHECK for the whole post-P1 layer (the full-materialization baseline answers
   correctly), so every pin must be STRUCTURAL: census-token / cursor-shape (region `s<id>` vs query-cursor
   `pos`) / node-count / provenance-survival / compile-abort. Keep the four authorities separate (logical fact /
   residual / logical path / physical structure) and the two edges distinct (RequestEdge vs RuleActivationEdge).

3. **Critique the diffs adversarially.** Run a refuter panel against the real code + the retained
   `RegionalDataFlowCore.md` invariants + the "Avoid these false starts" checklist. VERIFY findings against the
   codebase — do not merely assert. Rank survivors; record refuted diffs (and refuter claims that don't survive
   verification) as certifications. Special scrutiny: does any P2 typed-owner move accidentally re-introduce a
   DataFlow-mutation-plus-post-optimization-recognition authority (the last false-start)? Does the B2 permanent-
   root-as-RequestEdge-owner realization keep the ownership forest ACYCLIC? Is the B4 per-FACT DRed keyed on
   `RegionalFactId` (single fact authority), never on `BindingStateId`?

4. **Author/extend the desired IR output states.** As each phase grounds, predict-then-verify the target IR
   dumps (`.df` / `.rel` / `-region-out` / `-contract-out` / generated C++) with STRUCTURAL pins only. Sonnet
   pulls the current carrier dumps as the baseline (read-only compiles into a scratch dir); opus authors the
   desired states.

## How to do the work (workflows + model tiering)

Use the **Workflow tool** for the fan-out/critique structure — the owner has opted into multi-agent
orchestration for this grounding. Several sequential single-phase workflows, staying in the loop between them,
beat one mega-workflow. Suggested shape (adapt freely):

- **Ground + anchor-reconcile (fan-out, mostly sonnet):** one reader runs the symbol-driven P1 `git grep -l`
  DRY and reconciles against reconstruction-diffs §1+§6 (returns any new consumer + a compile-clean verdict);
  one reader per P2 sub-area (the typed RegionTemplate/RelationSchema records; the formatting-derives-from-typed
  -records mandate; the Program/Rel/ControlFlow consume-only-frozen seam). Each returns first-cut operational
  pseudocode + a drift table (only `lib/Parse/*` can drift). Sonnet is right for the mechanical census/extraction;
  use opus for any genuine judgment reader.
- **Diff + amend (opus, effort high):** author the P1-atomic-diff refinement + the P2 typed-owner diffs into the
  authority docs in place. Judgment work — opus.
- **Critique (adversarial panel, opus, effort high):** per-phase refuters + a cross-phase completeness/invariants
  critic, each prompted to BREAK the diffs against real code + the retained invariants; verify, don't assert; rank.
- **IR desired-states (opus author, sonnet carrier-dump extraction):** sonnet pulls current carrier dumps as the
  diff baseline; opus authors the desired states.

Keep the orchestrator thin: subagents return distilled pseudocode/findings, not raw file dumps. Front-load cheap
artifacts (dump the current carrier IRs once into scratch; embed paths in agent prompts). Land results as updates
to the existing docs; keep `session-16-whole-program-seed.md` current as the whole-program backbone.

## Guardrails

- Re-verify every cited anchor before trusting it — but only `lib/Parse/*` anchors can have drifted since the
  doc baseline `46a404d4` (see the seed's ANCHOR-VALIDITY note); everything below the parser is byte-identical.
- Do NOT edit production code toward Phase 1 without an explicit owner go-ahead. The destructive demand deletion
  is owner-gated. If green-lit, use the symbol-driven `git grep -l` gate as the acceptance check, not a review read.
- Do not bless any golden except as an explicit, reviewed consequence of a green-lit P1 cut (the two `.contract`
  goldens). If you compile carriers for baseline dumps, that is read-only; the suite must stay `SUITE: PASS`.
  Confirm via git that you touched only docs unless P1 is green-lit.
- Prefer silent-on-success builds/tests (capture to file, surface only on failure); tokens matter.
- Update memory (`regional-dataflow-core-epoch.md`, `MEMORY.md`) at session close with what landed and the next
  ranked step.
