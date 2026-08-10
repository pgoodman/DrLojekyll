# Session 21 prompt — keyed-instance rewrite: P6 (recursive regional execution) grounding + execution-readiness

Continue in the Dr. Lojekyll repo on branch `keyed-instances`. **Re-check the branch tip and worktree
first.** The tip should be `375e8713` ("P5: the partial-binding DAG (declared @key first specializes)").
**P1 + P2 + P3 + P4 + P5 ARE LANDED** — compile-clean, OptDiff `SUITE: PASS (223 cases)`, ctest 5/5 (the
`RegionInstance` gate now carries the P5 GateD–GateI DAG battery). The working tree should be clean.

## Context you must load (the greenfield rewrite state)

- **P5 shipped the partial-binding DAG** (`p5-grounding.md`): the ORDERED `DeclaredAccessPath` authority +
  pure `InternDeclaredPaths` (KeyPathId sorted → F21); the per-relation ORDER-FREE binding-schema DAG on
  `RegionInstanceRelations` (`RelSchemaLocalId`, `schema_table`, `binding_edges`, lazy
  `MaterializePrefixChain` — never the power set); `RelationSchema.declared_access_paths` populated in both
  freeze arms; the `HasInstanceKey()`-tied always-on **V-PREFIX-CHAIN** belt; a golden-pinned `-region-out`
  `declared-key` block; the first positive `@key` carrier `key_partial_1`; the mandatory ctest
  `RegionInstanceP5` DAG gates. **Model + render only — no codegen movement** (verified byte-identical).
- **The regional model's DERIVATION/ROUTE half is STILL DORMANT for real compiles** (P3-style,
  ctest-only): `RegionInstanceRelations.activation_edges` / `AddDerivation` / `RouteResults` /
  `DeriveActivationEdge` have no real-compile callers; `RegionTemplate.recursive_components` / `rules` /
  `inherited_symbolic_fields` are RESERVED-EMPTY; the actual recursive evaluation is the retained M3
  full-materialization backend (DataFlow `Stratify` computes the view-graph SCC; the differential
  fixpoint + OVERDELETE→REDERIVE→INSERT live in `lib/ControlFlow/Build/Stratum.cpp`).
- **P6 is where that dormant recursive-analysis half first does real work.** It is the LARGEST phase and
  MUST be sub-sliced. **The headline open decision: does P6 stay a COMPILE-TIME model addition (like
  P3/P4/P5 — populate `recursive_components`/`rules`/activation edges + validators + render + ctest,
  codegen unchanged) OR begin REAL RUNTIME EVALUATION (the model drives the fixpoint, toward replacing
  M3)?** The P3/P4/P5 cadence argues for a compile-time first cut; `next-session-prompt` Phase 6 +
  `reconstruction-diffs §3-P6 EvaluateEpoch` describe a runtime loop. The recommended reconciliation
  (session-21 seed §3): sub-slice P6 so the FIRST cut is compile-time (P6.1 SCC → `recursive_components`,
  projected from Stratify's condensation per M4; P6.2 typed edge-local routing + SymbolicFieldId
  promotion → `rules`), deferring the runtime fixpoint/DRed (P6.3–P6.6) to a later, separately-gated cut.
- **CRITICAL: there is NO co-recursion carrier in the corpus** for P6. P6 must AUTHOR one (the P3/P4/P5
  precedent: a new golden a stub cannot pass) — e.g. co-recursive `p(K,X)@key(K), q(X,K)@key(X)`.
- The rewrite is **greenfield delete-then-rebuild**. Every post-P1 gate is STRUCTURAL, never
  answer-equality (the full-materialization baseline already answers correctly). Motivation: memory
  `greenfield-rewrite-motivation`.

## Read in this order

1. `RegionalDataFlowCore.artifacts/INDEX.md` (read-order + supersession matrix).
2. **`session-21-whole-program-seed.md`** — START HERE. The POST-P5 whole-program view: §0 status + what
   is grounded for P6 (incl. the RESERVED-EMPTY fields + the DORMANT half + the Stratify SCC source), §1
   the CURRENT pipeline pseudocode grounded in real post-P5 code (§1.3a the model as the P6 launch point),
   §2 the four-authority target, §3 the path forward P6–P9 as diffs (P6 sub-sliced, with its headline open
   decision + the recommended first cut + the deferred runtime half), §4 what this session must do. **All
   anchors are grounded at tip `375e8713`** — §0 lists what to re-verify.
3. `p5-grounding.md` — the P5 record + the METHOD TEMPLATE (§5 layer-site inventory, §6 "what P6
   inherits", §7 drift table, §8 refuter survivors A1–A7, §9 IR states; the freeze-model + always-on-belt
   + mandatory-ctest-gate + new-carrier pattern to reuse).
4. `next-session-prompt.md` — the SEMANTIC authority (Phase 6, the "Avoid these false starts" checklist —
   esp. "do not use reference counts to collect a cyclic activation graph", "do not let an internal
   activation edge become a request owner", "do not implement recursive keyed regions by extending only
   InstanceStore"; the "Target semantic representation" typed records — `RuleActivationEdgeId`,
   `RecursiveComponent`, `SymbolicFieldId`).
5. `keyed-rewrite-reconstruction-diffs.md §3-P6` (P6.1–P6.6) + §5.7 (B4/B5/H7/M4/M5/M6/L6 — the DRed
   per-FACT rebuild, the cross-component transitive retraction, the cascade-retract, the safer SCC arm)
   + §5.8 (H4/D1) + §4 (design-goal diffs, goals 3/4/7). **Re-verify every anchor — they predate P1–P5
   and have MOVED** (`RegionInstanceRelations` now lives in `RegionInstance.h`; the Stratum.cpp DRed line
   numbers; `GenericTarjan` may be superseded by projecting `QueryView::Stratum()`).
6. `keyed-rewrite-reconstruction-critique.md` + `keyed-rewrite-p7p9-critique.md` for standing realizations.

## The decision this session opens on

**[OWNER STOP] Is P6 execution green-lit, or stay in grounding? And if green-lit, which first cut
(compile-time P6.1/P6.2 vs the runtime engine)?**

- **If grounding (default):** run the grounding loop below on the POST-P5 codebase — deepen toward P6
  execution-readiness. Settle the headline compile-time-vs-runtime decision. Do NOT edit production code
  toward P6 without an explicit owner go-ahead. Confirm via git you touched only docs.
- **If green-lit:** execute the agreed P6 first cut against `session-21-seed §3` + the grounded diffs.
  For the recommended compile-time cut: replace the `RecursiveComponent`/`RuleRoutingProjection` stub
  structs with real records; populate `recursive_components` (SCC projected from Stratify's condensation,
  M4 safer arm) + `rules`/symbolic fields (typed edge-local routing, PromoteSharedSymbolicField both arms);
  add an always-on belt (the P4/P5 idiom — cross-check against Stratify's projected condensation, F19);
  render the `-region-out` `recursive-component`/`rule` blocks; author a NEW positive co-recursion carrier
  golden + the mandatory ctest gates; keep codegen byte-stable (pin `.rel`/`datalog.h`/`.stdout`). Land it
  with the STRUCTURAL exit gate green (co-recursion → one component byte-identical under add/remove
  `#query`; producers-agree promotion; a stub that leaves `recursive_components` empty FAILS). OptDiff
  `SUITE: PASS`; ctest ≥5/5; any golden movement reviewed and blessed only as an explicit, reviewed P6
  consequence. One coherent commit.

## This session's task (the grounding loop — do this whether or not P6 lands)

Run the **build-pseudocode → design-goal diffs → critique → IR-desired-states** loop, keeping
`session-21-whole-program-seed.md` current as the whole-program backbone.

1. **Build out / refresh the whole-program pseudocode** at implementer altitude, weighted to P6:
   re-express the key algorithms as pseudocode grounded in the REAL post-P5 code — the Stratify SCC
   condensation (`view->stratum`/`QueryView::Stratum()`) and how to PROJECT it onto relations (M4 safer
   arm) vs a fresh `GenericTarjan` (F30); `ComputeRecursiveComponents`; the typed `RuleRoutingProjection`
   + `PromoteSharedSymbolicField` (SymbolicFieldId promotion, region-global — reconcile with the P5 A3
   "rebuilt at P8" note); how `@key` paths (the P5 DAG) seed the destination binding prefix a routed fact
   carries; and — if in scope — the P6.5 EvaluateEpoch two-pass (DRed pass (A) mirroring
   `lib/ControlFlow/Build/Stratum.cpp` + the joint rooted-reachability worklist (B)) and P6.6 retirement.
   Run the P6 analog of the P1–P5 symbol grep — enumerate every site P6 must reuse or touch. Ground every
   anchor. **Settle the headline open decision (compile-time first cut vs runtime engine; one commit or
   two; where the render lives; census impact; the new carrier).**
2. **Formulate design-goal diffs** on the deepened pseudocode at hunk grain, with DISCRIMINATING
   STRUCTURAL exit gates (co-recursion → one `recursive_components` component, byte-identical under
   add/remove `#query`; PromoteSharedSymbolicField both arms — all-producers-agree promotes,
   co-occurrence-only does not; if DRed is in scope, B0/F1 — a still-rooted self-supporting cycle IS
   retracted). Answer-equality is a LOST CHECK. Keep the four authorities separate; RequestEdge stays an
   acyclic forest, RuleActivationEdge may cycle but is NEVER accepted by `AddRequestEdge` (F18/goal 3).
3. **Critique the diffs adversarially.** Run an opus refuter panel against the real POST-P5 code + the
   retained `RegionalDataFlowCore.md` invariants + the "Avoid these false starts" checklist. Special
   scrutiny: is the SCC projection from Stratify sound and does it close message seams (F7/M4)? Does the
   SymbolicFieldId promotion only union where all producers agree (F16), and does it reconcile with the
   P5 per-relation DAG (rebuild vs parallel layer)? If DRed/runtime is in scope: does it mirror the landed
   per-FACT `Stratum.cpp` machinery (B4), cover cross-component transitive retraction as ONE worklist (B5),
   and retire by RE-DERIVED reachability never refcount (P6.6/H7)? Is the new co-recursion carrier's gate
   discriminating (a stub that leaves `recursive_components` empty must FAIL)? VERIFY findings against the
   codebase — do not merely assert. Rank survivors; record refuted diffs as certifications.
4. **Author/extend the desired IR output states.** Predict-then-verify the target IR dumps with
   STRUCTURAL pins only: the `-region-out` `recursive-component`/`rule` render (WHERE it lives +
   deterministic order); any census-line impact; whether `.rel`/generated `datalog.h` move at all (likely
   NOT if the first cut is model+render — pin that prediction); the NEW positive co-recursion carrier
   golden(s). Sonnet pulls the current carrier dumps as the baseline (read-only compiles into a scratch
   dir); opus authors the desired states.

## How to do the work (workflows + model tiering)

Use the **Workflow tool** for the fan-out/critique structure — the owner has opted into multi-agent
orchestration for this grounding. Several sequential single-phase workflows, staying in the loop between
them, beat one mega-workflow. Suggested shape (adapt freely):

- **Ground + inventory (fan-out, mostly sonnet):** one reader runs the P6 layer-site enumeration (the
  RESERVED-EMPTY fields + stub structs / the DORMANT `RegionInstanceRelations` half / the Stratify SCC
  source / the ControlFlow/Build/Stratum.cpp DRed machinery / the message-seam handling); one reader per
  P6 sub-area (SCC projection `ComputeRecursiveComponents`; typed routing + `PromoteSharedSymbolicField`;
  the DRed/retirement runtime half if in scope; the new-carrier design). Each returns first-cut
  operational pseudocode + a drift table (grounded at `375e8713` — re-verify before trusting). Sonnet for
  mechanical census/extraction; opus for genuine judgment readers.
- **Diff + amend (opus, effort high):** author the P6 diffs + the compile-clean inventory into a new
  `p6-grounding.md` (sibling of `p5-grounding.md`). Judgment work — opus.
- **Critique (adversarial panel, opus, effort high):** per-area refuters + a cross-phase
  completeness/invariants critic, each prompted to BREAK the diffs against real code + the retained
  invariants + the false-starts checklist; verify, don't assert; rank; fold survivors into
  `p6-grounding.md §8`.
- **IR desired-states (opus author, sonnet carrier-dump extraction):** sonnet pulls the current
  `-region-out`/`.rel` dumps for a hand-written co-recursion probe as the diff baseline; opus authors the
  desired P6 states.

Keep the orchestrator thin: subagents return distilled pseudocode/findings, not raw file dumps.
Front-load cheap artifacts (dump the current dumps once into scratch; embed paths in agent prompts).
Land results as updates to the existing docs + a new `p6-grounding.md`; keep
`session-21-whole-program-seed.md` current as the whole-program backbone.

## Guardrails

- Re-verify every cited anchor before trusting it. Treat the `session-21-whole-program-seed.md` §1 line
  numbers as current-at-`375e8713` and re-grep any older doc's anchor (the `reconstruction-diffs §3-P6`
  anchors moved at P1–P5 — `RegionInstanceRelations` is now in `RegionInstance.h`; the Stratum.cpp DRed
  lines; `GenericTarjan` may be superseded by projecting `QueryView::Stratum()`).
- Do NOT edit production code toward P6 without an explicit owner go-ahead. If green-lit, use a
  symbol/shape-driven acceptance check + the STRUCTURAL discriminating gate (a stub that leaves
  `recursive_components` empty must fail), not a review read; keep the region goldens + the
  `RegionInstance` ctest as the anti-stub belt; author the NEW co-recursion carrier before claiming green.
- The recommended P6 first cut MAY be model+render only (answer-invariant, no codegen golden movement) —
  decide + pin that in the grounding. The runtime evaluation half (P6.3–P6.6) is a larger, separately-
  gated cut that likely touches codegen/runtime (where M3 begins to be replaced) — do NOT fold it into the
  first cut without an explicit owner decision. Any golden movement must be an explicit, reviewed
  consequence — never blessed to make a red case green, never automatically on failure. If you compile
  probes for baseline dumps, that is read-only; the suite must stay `SUITE: PASS` in grounding.
- The bless loop mechanizes the symlink rule; a NEW carrier golden is a real content add. Prefer
  silent-on-success builds/tests (capture to file, surface only on failure); tokens matter.
- Never rebuild the compiler while a suite/ctest run is in flight (a mid-run rebuild corrupts the run).
- Update memory (`regional-dataflow-core-epoch.md`, `MEMORY.md`) at session close with what landed and
  the next ranked step.
```
```
