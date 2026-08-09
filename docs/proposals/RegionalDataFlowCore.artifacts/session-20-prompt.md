# Session 20 prompt — keyed-instance rewrite: P5 (the partial-binding DAG / declared `@key` first specializes) grounding + execution-readiness

Continue in the Dr. Lojekyll repo on branch `keyed-instances`. **Re-check the branch tip and worktree
first.** The tip should be `c546a6a4` ("P4: honest complete-path specialization (AccessPlan /
FullScanFilter)"). **P1 + P2 + P3 + P4 ARE LANDED** — compile-clean, OptDiff `SUITE: PASS (222 cases)`,
ctest 5/5 (the `RegionInstance` gate now carries the P4 GateA/B/C cases). The working tree should be
clean.

## Context you must load (the greenfield rewrite state)

- **P4 shipped the `AccessPlan` fourth authority (physical structure).** `RegionInstance.h` hosts
  `AccessPlan{kFullScanFilter,kFullKeyHashLookup,kRetainedIndexScan}` + `AccessRequirement` +
  `SelectAccessPlan`. It is SELECTED at freeze (`BuildRequestPorts` stores it on
  `RequestPortRecord.plan`; `FrozenRegionalProgram::PlanFor(redecl)` reads it back) and CONSUMED at
  codegen (`Program::Build` threads `frozen` into `Context`; `BuildQueryEntryPointImpl` + the
  empty-query arm withhold the index for `kFullScanFilter`, always-on V-PLAN-HONEST belt). A bound+free
  `#query` lowers to an honest full-scan-filter cursor; an all-bound query keeps `.Find`. Design +
  the adversarial-critique survivors + IR states: `p4-grounding.md`.
- **P4 drives a codegen CHANGE via a compile-time authority READ — NOT a runtime evaluation.** The P3
  request/derivation model's derivation/route half is still DORMANT for real compiles (exercised only
  by the `RegionInstance` ctest); `BindingStateId` interns only the empty state. `@key` is INERT
  parsed metadata (parse-surface only; it does NOT yet shape the binding model, the plan, or codegen).
- **P5 is the first phase where a DECLARED `@key` SPECIALIZES.** It introduces the residual
  binding-state DAG: `DeclaredAccessPath` (ORDERED, order-significant identity), `BindingStateSchema`
  (ORDER-FREE, value-free, prefix-shared), and `BindingEdge` (order-significant navigation). `@key(A)`
  reuses the `{A}` prefix of `@key(A,B)`; `[A,B]`/`[B,A]` converge on one `{A,B}` schema by two ordered
  edges. Like P3, this is largely a COMPILE-TIME modeling addition (the physical partial-key seek is a
  P7 cost decision), so **P5 may be STRUCTURAL-only (model + render, answer-invariant) — decide this in
  the grounding loop.**
- **CRITICAL: there is NO positive `@key` corpus carrier post-P1.** The semantic `key_*_witness` cases
  were deleted with the demand machinery; only the parse-reject cases survive
  (`key_anon_1`/`key_dup_1`/`key_unknown_1`/`key_wildcard_1`). **P5 must AUTHOR a new positive carrier**
  (the P3/P4 precedent: a new golden a stub cannot pass).
- The rewrite is **greenfield delete-then-rebuild**. Every post-P1 gate is STRUCTURAL, never
  answer-equality (the full-materialization baseline already answers correctly with `@key` inert).
  Motivation: memory `greenfield-rewrite-motivation`.

## Read in this order

1. `RegionalDataFlowCore.artifacts/INDEX.md` (read-order + supersession matrix).
2. **`session-20-whole-program-seed.md`** — START HERE. The POST-P4 whole-program view: §0 status +
   what is grounded for P5 (incl. the "no positive `@key` carrier" fact + the `@key`/binding-state
   substrate anchors), §1 the CURRENT pipeline pseudocode grounded in real post-P4 code (§1.3a is the
   P3/P4 model as the P5 launch point), §2 the four-authority target, §3 the path forward P5–P9 as
   diffs (P5 is the next actionable, with its OPEN design questions), §4 what this session must do.
   **All anchors are grounded at tip `c546a6a4`** — §0 lists what to re-verify.
3. `p4-grounding.md` — the P4 record + the METHOD TEMPLATE (§8 critique survivors + certifications, §9
   IR desired-states; the freeze-store→codegen-read pattern, the negative-witness technique).
4. `next-session-prompt.md` — the SEMANTIC authority (four authorities, two edges, ORDERED access
   paths, the "Avoid these false starts" checklist, the "Target semantic representation" typed records,
   Phase 5).
5. `keyed-rewrite-reconstruction-diffs.md §3-P5` (489-538, the P5 operational diff: `InternDeclaredPaths`,
   `BindingStateSchema`, `BindingEdge`, `MaterializePrefixChain`, cross-redecl consistency, the F21
   render fix) + §5.6 (H6/M7, the parser flip — ALREADY LANDED as P0-item-4, confirm) and §3-P6 for the
   sequel. **Re-verify every anchor — they predate P1–P4 and have MOVED** (the F21 render target may
   have migrated/been deleted; the parse surface survived P1 but is inert).
6. `keyed-rewrite-reconstruction-critique.md` + `keyed-rewrite-p7p9-critique.md` for standing
   realizations.

## The decision this session opens on

**[OWNER STOP] Is P5 execution green-lit, or stay in grounding?**

- **If grounding (default):** run the grounding loop below on the POST-P4 codebase — deepen toward P5
  execution-readiness. Do NOT edit production code toward P5 without an explicit owner go-ahead.
  Confirm via git you touched only docs.
- **If green-lit:** execute P5 against `reconstruction-diffs §3-P5` + the grounded diffs — introduce
  `DeclaredAccessPath` (ORDERED) + `BindingStateSchema` (ORDER-FREE) + `BindingEdge` (order-significant)
  in the Regional model, intern the declared prefix DAG from `decl.InstanceKeys()`, render the
  declared-key surface (KeyPathId-sorted, F21), and author a NEW positive `@key` carrier golden. Land
  it with the STRUCTURAL exit gate green (F8 prefix-present/non-prefix-absent; `@key(A)`⊂`@key(A,B)`
  one shared schema id; `[A,B]`/`[B,A]` two edges one schema; F20 unbound-read completeness; F21
  render-reorder byte-identity; a stub that ignores `@key` fails). Keep the four authorities distinct
  (the ORDERED path vs the ORDER-FREE schema is the P5 headline). OptDiff `SUITE: PASS`; ctest ≥5/5;
  any golden movement reviewed and blessed only as an explicit, reviewed P5 consequence. One coherent
  commit.

## This session's task (the grounding loop — do this whether or not P5 lands)

Run the **build-pseudocode → design-goal diffs → critique → IR-desired-states** loop, keeping
`session-20-whole-program-seed.md` current as the whole-program backbone.

1. **Build out / refresh the whole-program pseudocode** at implementer altitude, weighted to P5:
   re-express the key algorithms as pseudocode grounded in the REAL post-P4 code — `InternDeclaredPaths`
   over `decl.InstanceKeys()`, the order-free `BindingStateSchema` interner vs the order-significant
   `BindingEdge`, `MaterializePrefixChain` (lazy, declared-or-visited, never power-set), and where P5
   attaches to the P4 `BuildRequestPorts`/`RequestPortRecord`/`PlanFor` seam. Run the P5 analog of the
   P1–P4 symbol grep — enumerate every current site P5 must reuse or touch (the `@key` parse surface
   `InstanceKeys`/`HasInstanceKey`/`ParseLocalExport` states 21/22; `BindingStateSchemaId`/
   `BindingStateId`/`EmptyBindingState`; the declared-key render surface re-located post-P1; the P4
   plan seam). Ground every anchor. Settle the OPEN questions (does P5 change codegen; where the render
   lives; the shape of the new `@key` carrier).
2. **Formulate design-goal diffs** on the deepened pseudocode at hunk grain, with DISCRIMINATING
   STRUCTURAL exit gates (F8 prefix chain present + non-prefix subsets absent; shared `{A}` schema id +
   single `{}--A-->{A}` edge; `[A,B]`/`[B,A]` two ordered edges into one `{A,B}` schema; F20
   unbound-read completeness; F21 render-reorder byte-identity). Answer-equality is a LOST CHECK. Keep
   the four authorities separate — the ORDERED `DeclaredAccessPath` must never conflate with the
   ORDER-FREE `BindingStateSchema` (that IS the P5 headline distinction), nor with the physical
   `AccessPlan` (P4) or the fact id.
3. **Critique the diffs adversarially.** Run an opus refuter panel against the real POST-P4 code + the
   retained `RegionalDataFlowCore.md` invariants + the "Avoid these false starts" checklist. Special
   scrutiny: does the order-free-schema / order-significant-edge split hold end to end (no within-path
   sort leaks; the `[A,B]`/`[B,A]` convergence real; prefix sharing genuine)? Does an unbound read still
   return the COMPLETE relation (F20 — the "do not enumerate only active states" false start)? Does P5
   avoid power-set materialization (only declared/visited edges)? Is the new `@key` carrier's gate
   discriminating (a stub that ignores `@key`, or sorts within a path, must FAIL)? Does P5 keep
   `BindingStateSchema` value-free (schema, not value)? VERIFY findings against the codebase — do not
   merely assert. Rank survivors; record refuted diffs as certifications.
4. **Author/extend the desired IR output states.** Predict-then-verify the target IR dumps with
   STRUCTURAL pins only: the declared-key render (WHERE it lives post-P1 — `-region-out` vs
   `-contract-out` vs the `-region-dot-out` badge — and KeyPathId-sorted); any `-region-out`
   binding-schema block; whether `.rel`/generated `datalog.h` move at all (likely NOT if P5 is
   model+render only — pin that prediction); the NEW positive `@key` carrier golden(s). Sonnet pulls
   the current carrier dumps as the baseline (read-only compiles into a scratch dir); opus authors the
   desired states.

## How to do the work (workflows + model tiering)

Use the **Workflow tool** for the fan-out/critique structure — the owner has opted into multi-agent
orchestration for this grounding. Several sequential single-phase workflows, staying in the loop
between them, beat one mega-workflow. Suggested shape (adapt freely):

- **Ground + inventory (fan-out, mostly sonnet):** one reader runs the P5 layer-site enumeration (the
  `@key` parse surface + `ParseLocalExport` states / the binding-state substrate / the declared-key
  render surface / the P4 plan seam); one reader per P5 sub-area (`DeclaredAccessPath`/
  `InternDeclaredPaths`; the `BindingStateSchema`/`BindingEdge`/`MaterializePrefixChain` DAG; the
  render migration; the new-carrier design). Each returns first-cut operational pseudocode + a drift
  table (grounded at `c546a6a4` — re-verify before trusting). Sonnet for mechanical census/extraction;
  opus for genuine judgment readers.
- **Diff + amend (opus, effort high):** author the P5 diffs + the compile-clean inventory into a new
  `p5-grounding.md` (sibling of `p4-grounding.md`). Judgment work — opus.
- **Critique (adversarial panel, opus, effort high):** per-area refuters + a cross-phase
  completeness/invariants critic, each prompted to BREAK the diffs against real code + the retained
  invariants + the false-starts checklist; verify, don't assert; rank; fold survivors into
  `p5-grounding.md §8`.
- **IR desired-states (opus author, sonnet carrier-dump extraction):** sonnet pulls the current
  declared-key/`-region-out` dumps for a hand-written `@key` probe as the diff baseline; opus authors
  the desired P5 states.

Keep the orchestrator thin: subagents return distilled pseudocode/findings, not raw file dumps.
Front-load cheap artifacts (dump the current dumps once into scratch; embed paths in agent prompts).
Land results as updates to the existing docs + a new `p5-grounding.md`; keep
`session-20-whole-program-seed.md` current as the whole-program backbone.

## Guardrails

- Re-verify every cited anchor before trusting it. Treat the `session-20-whole-program-seed.md` §1 line
  numbers as current-at-`c546a6a4` and re-grep any older doc's anchor (the `reconstruction-diffs §3-P5`
  anchors moved at P1–P4 — the F21 render target especially).
- Do NOT edit production code toward P5 without an explicit owner go-ahead. If green-lit, use a
  symbol/shape-driven acceptance check + the STRUCTURAL discriminating gate (a stub that ignores `@key`
  must fail), not a review read; keep the region goldens + the `RegionInstance` ctest as the anti-stub
  belt; author the NEW positive `@key` carrier before claiming the gate green.
- P5 MAY be model+render only (answer-invariant, no codegen golden movement) — decide + pin that in the
  grounding. Any golden movement must be an explicit, reviewed consequence of a green-lit P5 cut —
  never blessed to make a red case green, never automatically on failure. If you compile probes for
  baseline dumps, that is read-only; the suite must stay `SUITE: PASS` in grounding.
- The bless loop mechanizes the symlink rule; a NEW carrier golden is a real content add. Prefer
  silent-on-success builds/tests (capture to file, surface only on failure); tokens matter.
- Never rebuild the compiler while a suite/ctest run is in flight (a mid-run rebuild corrupts the run).
- Update memory (`regional-dataflow-core-epoch.md`, `MEMORY.md`) at session close with what landed and
  the next ranked step.
```
```
