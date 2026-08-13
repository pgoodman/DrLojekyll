<!-- Copyright 2026, Peter Goodman. All rights reserved. -->
# Session-35 charter — InstanceFlow Phase C: extend the TABLE*→StateResourceId retype across the Rel op model (real, cross-checked, goldened)

You are resuming the InstanceFlow build on branch `keyed-instances` (Dr. Lojekyll, the
`hyde` C++ Datalog compiler). You are inheriting strong, disciplined, LANDED work.
Session 34 landed the **MaterializationPlan resource authority** (a codegen-byte-identical
OBSERVER cross-checked against the real allocation) **and Phase-C step 1**: the first Rel
object (`DRTable`) to carry a `StateResourceId` beside its `TABLE*`, stamped from the
materialization map (`ResourceForView(v) = authority(EquivalenceSetId(v))`) and belt-guarded
by `V-REL-RESOURCE`. Everything is green: OptDiff **SUITE PASS 227** byte-identical, ctest
**5/5**, both belts (`CrossCheckMaterialization`, `V-REL-RESOURCE`) belt-verified live, and
every witness dump byte-matched its prediction. **This is careful, honest, beautiful compiler
work. The map is proven and the path is clear. Trust the discipline, be bold, and build the
next real thing. We believe in you — you have earned that confidence.**

## The mission this session

**Land Phase-C step 2: make the ENTIRE Rel op model resource-addressable.** Extend the
`StateResourceId` retype from the single `DRTable` object to the ~44 base-table `TABLE*`
fields on the `DROp` variants (`fold_table`, `negate_table`, `negated_table`, `pred_table`,
`product_table`, `side_tables`, `fire_table`, `chain_*`, `gate_table`, `ingest_table`,
`agg_table`, `input_table`, `seed_*`, `read/write/value/counter_table`). Each op's base-table
field gets a parallel, cross-checked `StateResourceId`, stamped at `BuildDRInventory` from a
`TABLE* → StateResourceId` index (invert `flow.tables`), and — the part that makes it a
REAL, pinned artifact — **rendered in `-rel-out`** (` resource=sr#K` after ` table=%table:N`)
and goldened. This is the concrete realization of the §11 goal "Rel refers to
`StateResourceId`", and the last brick before the allocation inversion (Step 3) can retype
the ops to CONSUME the ids.

It stays a codegen-byte-identical OBSERVER — nothing consumes the ids for emission yet — so
the CODEGEN goldens (`.h`/`.ir`/`.stdout`/oracle/monotone/behavioral) MUST stay byte-identical.
Only the 10 `.rel` dump goldens move (an ADDITIVE token). The new `V-REL-OP-RESOURCE` belt
(every non-null base-table field resolves to a resource whose class == the field table's
`EquivalenceSetId`) is a falsifiable correctness claim — exactly the kind that caught the s32
SCC-keying bug. That is real, load-bearing infrastructure.

**Do something REAL** is the north star. Here it means: land the op-level retype (new
cross-checked ids across the whole Rel op model, rendered + goldened, gate-green), AND keep the
line of sight to Step 3 (the allocation inversion) explicit. If grounding shows step 2 is too
thin to be real (e.g. the render/cross-check is vacuous), say so and re-scope toward Step 2b
(reconcile the `support` token with `TableIsDifferential`, then cross-check support equality —
teeth the resolution-only belt lacks) or toward scoping the arrangement derivation (§2.5.3) that
unblocks Step 3. Do not land a hollow shadow.

## Read first (resume authority, in order)

1. `docs/proposals/InstanceFlow.artifacts/session-35-seed.md` — THE whole-program view at the
   post-step-1 tip. Especially **§1** (current-architecture pseudocode: Query::Build tail →
   materialization; Program::Build → BuildDataModel/FillDataModel/region-construction-with-inline-
   GetOrCreateIndex/BuildDRInventory-shadow/LowerDRFlow → codegen), **§2** (the path forward as
   diffs — Step 2 / 2b / 3), **§3** (the recommended next real step, concretely), **§4** (anchors).
2. `docs/proposals/InstanceFlow.md` §11 (Rel after InstanceFlow — what Rel keeps/deletes; §11.3
   `BuildRel`; §11.4 the single-owner decisions), §16 (dump grammar), §10 (MaterializationPlan).
3. `docs/proposals/InstanceFlow.artifacts/session-34-grounding.md` (the adjudicated panel: the
   bijection fold that made the retype well-defined; the `support` vs `TableIsDifferential` note).
4. `lib/Rel/Rel.{h,cpp}` (the DROp variants + `BuildDRInventory`) and
   `lib/ControlFlow/Build/Stratum.cpp` (`LowerDRFlow`, the emitters that read the `TABLE*`
   fields). memory `regional-dataflow-core-epoch` (s34 head) + the CLAUDE.md "InstanceFlow" section.

## Phase 1 — GROUND IT with workflows (before touching production code)

Ground → critique → execute, the way every phase here has been done. Use **workflows** (the
`Workflow` tool) for the fan-out, and tier the models deliberately: **sonnet** for mechanical
extraction and recon (transcribe a subsystem to current-tip line numbers, enumerate every DROp
base-table field + its mint site, produce raw pseudocode of a stage, hand-derive a witness's
expected `-rel-out` delta); **opus** for judgment (design diffs, adversarial refuter panels,
synthesis). Keep the orchestrator thin — sonnet reads, you keep the conclusions.

Concretely, the grounding workflow(s) should PRODUCE and then CRITIQUE:

1. **Whole-program pseudocode, built out.** Extend seed §1 into faithful, current-tip-anchored
   pseudocode of the stages this slice touches: every `DROp` variant and which base-table
   `TABLE*` field(s) it carries; the EXACT `BuildDRInventory` mint site for each field; how
   `LowerDRFlow`/the `Emit*` templates read those fields (so you can prove the ids are pure
   shadow — read by nothing that emits). Sonnet extracts each; you assemble.
2. **Design-goal diffs on that pseudocode.** Express Step 2 as precise diffs: the
   `TABLE*→StateResourceId` inversion of `flow.tables`; the parallel resource fields (or a single
   `std::unordered_map<TABLE*, StateResourceId>` on the flow graph — decide which is cleaner); the
   `V-REL-OP-RESOURCE` belt; the `-rel-out` render; the golden set. Sketch Step 2b (support
   reconciliation + support cross-check) and far enough into Step 3 (the allocation inversion) to
   prove the line of sight.
3. **Adversarial critique (opus refuter panel).** Fan out N independent refuters, each trying to
   REFUTE that: (a) EVERY base-table `TABLE*` field is truly a stateful-class identity resolvable
   via the map (find a field that is NOT — e.g. a table with no member views, an index/arrangement
   masquerading as a base table, or a field set to a table outside `flow.tables`); (b) the render +
   cross-check is NON-vacuous AND byte-identical corpus-wide for codegen (find a witness where an op
   field bites); (c) the ids are genuinely pure SHADOW — no `Emit*`/`Lower*` path reads the new field
   for emission, so codegen is byte-identical; (d) step 2 has a real, non-shadow line to Step 3 (the
   allocation inversion) rather than being shadow-for-shadow. Default refuted=true on uncertainty.
   Fold every surviving finding.
4. **IR desired-output-states, same treatment (predict-then-verify).** Produce the DESIRED
   `-rel-out` dumps (the op lines with the new ` resource=sr#K` tokens) for the witnesses
   (`negate_1`, `join_1`, `tc_nonlinear_diff`, `d5_recursive_negate` — differential/negate/product
   coverage) FIRST, then critique them for internal consistency + against the census/validator
   contracts + the `.materialization` goldens (does op `resource=sr#K` agree with the table's
   authority?). These are the golden targets you build toward and later bless (additive-token
   permutation check — `permcheck.py` / the E-K5-PAD discipline).

## Phase 2 — EXECUTE (after grounding is owner-ratified)

Build **Step 2** (seed §3): the `TABLE*→StateResourceId` map, the parallel op resource ids
stamped at `BuildDRInventory`, the `V-REL-OP-RESOURCE` belt, the `-rel-out` render, and re-blessed
`.rel` goldens. Exit gate:
- every non-null base-table field resolves (belt QUIESCENT corpus-wide) AND belt-verified LIVE
  (corrupt the map → fires; revert → quiescent — the CP-discipline);
- CODEGEN goldens (`.h`/`.ir`/`.stdout`/oracle/monotone/behavioral) BYTE-IDENTICAL (the observer
  proof); only the `.rel` goldens move, and the moved bytes are EXACTLY the additive token (predict
  the deltas first, verify each);
- OptDiff **SUITE PASS**; ctest **5/5**;
- WIP-commit at each checkpoint (protect the work — the branch history records a checkout-wipe
  incident); reach a clean gate before the final commit; ask the owner then.

Then take the first grounded step toward Step 3 (SCOPE, or begin, the allocation inversion — e.g.
settle the §2.5.3 arrangement-derivation fork, or prototype `AllocateRuntimeResources` reading the
plan) — so the session ends with real code landed AND a grounded, owner-ratified plan that continues
toward the codegen move.

## Discipline (carried, load-bearing)

- **Do something REAL** — land the op-level retype (real, cross-checked, rendered, goldened) and
  keep a concrete, named line to Step 3 (the allocation inversion). Never drift into a hollow
  shadow: if the render/cross-check is vacuous, re-scope toward Step 2b or the arrangement scope
  (the s27/s28 lesson — no shadow-for-shadow's-sake).
- Ground → critique → execute; workflows for fan-out; sonnet reads, opus judges; thin orchestrator.
- **Predict-then-verify** every IR dump: write the target `-rel-out` deltas FIRST, build, then diff
  the real dump against them (this caught the s32 SCC-keying bug and byte-matched every s33/s34 dump).
- Preserve the **observer invariant**: this slice keeps CODEGEN byte-identical (nothing consumes the
  ids for emission); the first codegen move is Phase D, gated behind the Step-3 allocation inversion.
  Only the `.rel` dump goldens legitimately move (an additive token) — predict + re-bless them; do
  NOT let a codegen golden silently move.
- Determinism: ids from `StateResourceId` / `EquivalenceSetId` / canonical order, NEVER `TABLE*`
  pointer / iteration order in any goldened output.
- Run builds/suite SILENT on success; clangd diagnostics in this repo are NOISE (no include paths)
  — trust the real build only. WIP-commit checkpoints; ask the owner at the clean gate.

## You've got this

InstanceFlow is the greenfield-coherent destination the whole design points at — one IR that
finally says "which contextual occurrence satisfies each use," with storage, coverage, emission,
lifecycle, and provenance as first-class obligations. The resource authority is standing and proven;
`DRTable` already speaks in resource ids; the map is total and single-valued. The next real step —
teaching the WHOLE Rel op model to speak in `StateResourceId`, cross-checked and goldened — is the
last brick before the allocation inversion that unlocks the codegen payoff. Build it with the same
rigor that got us here: ground it, critique it hard, predict-then-verify it, land it green. Be
rigorous, be bold, and make it real. We believe in you.
