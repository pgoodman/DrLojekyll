<!-- Copyright 2026, Peter Goodman. All rights reserved. -->
# Session-36 charter — InstanceFlow Phase C: derive the ARRANGEMENT (index) requirements — the true Step-3 unblocker

You are resuming the InstanceFlow build on branch `keyed-instances` (Dr. Lojekyll,
the `hyde` C++ Datalog compiler). You are inheriting strong, disciplined, LANDED
work, and you are exactly the right agent to carry it forward. Session 35 landed
**Phase-C step 2**: the WHOLE Rel op model is now resource-addressable
(`DRFlowGraph::table_to_resource`, a `TABLE*→StateResourceId` index), rendered in
`-rel-out` and belt-guarded (`V-REL-OP-RESOURCE`), gate-green (OptDiff **SUITE
PASS 227** codegen byte-identical, ctest **5/5**, belt verified LIVE). We were
honest that step 2 is a regression fence + goldened coverage artifact whose one
forward-load-bearing output is the map itself — and we named exactly where that
map stops being a shadow: **the arrangement derivation.** That is this session.
**This is careful, honest, beautiful compiler work, and you have earned real
confidence. The resource half is done and proven; the path is clear. Trust the
discipline, be bold, and build the next real thing. We believe in you.**

## The mission this session

**Derive the ARRANGEMENT (index) requirements as a first-class, cross-checked,
goldened authority** — the missing half of the runtime-resource plan and the true
blocker before the Step-3 allocation inversion. Today indexes materialize ONLY as
a side effect of emission (six inline `GetOrCreateIndex` sites); there is NO
standing requirement set, and `-materialization-out` prints a HARDCODED
`arrangements=0`. Change that.

Follow the s34→s35 cadence that got the resource half here safely (observer →
cross-check → invert):

- **Stage A (the recommended landing this session):** the arrangement CENSUS — a
  post-`Program::Build` observer that walks the real `impl->tables[*]->indices`,
  tags each by `table_to_resource` (the s35 map's FIRST real consumer), emits one
  deterministic `Arrangement` per `(resource, sorted column-set)`, and RENDERS
  them in `-materialization-out` (replacing the hardcoded `arrangements=0`,
  cross-referencing the resource `sr#K` id space). Codegen byte-identical; only
  the 5 `.materialization` goldens move (additive).
- **Stage B (bundle it if — and only if — grounding shows Stage A alone is a
  hollow shadow):** the PURE `collect_arrangement_requirements` derivation
  (pure-`QueryView`-API, the `DeriveStatefulClasses` precedent) that replays the
  six sites' column logic from the FINAL graph, CROSS-CHECKED byte-for-byte
  against Stage A's census (the falsifiable `derived == real` claim — the real
  teeth). This is where the landed/deferred P7/P7b/P9 access-path analyses
  finally earn a consumer.

**Do something REAL is the north star.** Here it means: land a derived,
cross-checked, rendered, goldened arrangement authority — not a census that
merely re-prints what emission already decided with no independent derivation.
Apply the s35 lesson without flinching: if grounding shows Stage A alone is
shadow-for-shadow (a census with no derivation to check it against), SAY SO and
bundle Stage B's cross-check so the slice has teeth. If grounding shows the whole
arrangement notion is premature (e.g. the six sites' logic can't be faithfully
replayed pure-side yet), say so and re-scope toward the smallest REAL step that
advances the allocation inversion. Do not land a hollow shadow.

## Read first (resume authority, in order)

1. `docs/proposals/InstanceFlow.artifacts/session-36-seed.md` — THE whole-program
   view at the post-step-2 tip. Especially **§1** (current-architecture
   pseudocode incl. the index universe + the gap), **§2** (the path forward as
   diffs — Stage A/B/C), **§3** (the recommended next step + the named DECISION
   POINTS), **§4** (anchors, re-verify at tip).
2. `docs/proposals/InstanceFlow.artifacts/session-35-arrangement-scope.md` — the
   staged arrangement plan (A/B/C) + why it's the honest continuation of s35.
3. `docs/proposals/InstanceFlow.artifacts/session-35-grounding.md` — the step-2
   panel + the HONEST verdict (why a census-without-teeth is thin; the Step-2b
   view-set-equality defer).
4. `docs/proposals/InstanceFlow.md` §10 (`plan_materialization` :1038,
   `ArrangementSpec` :936, `collect_arrangement_requirements` :1039), §11/§12
   (Rel after InstanceFlow / `BuildRel`). `lib/DataFlow/Materialization.{h,cpp}`,
   `lib/ControlFlow/Data.cpp` (`GetOrCreateIndex`), `lib/ControlFlow/Program.h`
   (`TABLEINDEX`/`indices`/`column_spec`). memory `regional-dataflow-core-epoch`
   (s35 head) + the CLAUDE.md "InstanceFlow" section.

## Phase 1 — GROUND IT with workflows (before touching production code)

Ground → critique → execute, the way every phase here has been done. Use
**workflows** (the `Workflow` tool) for the fan-out, and tier the models
deliberately: **sonnet** for mechanical extraction and recon (transcribe a
subsystem to current-tip line numbers; enumerate every `GetOrCreateIndex` call
site + the exact column set it requests + which graph fact determines it;
hand-derive a witness's expected `-materialization-out` arrangement block);
**opus** for judgment (design diffs, adversarial refuter panels, synthesis). Keep
the orchestrator thin — sonnet reads, you keep the conclusions.

Concretely, the grounding workflow(s) should PRODUCE and then CRITIQUE:

1. **Whole-program pseudocode, built out.** Extend seed §1 into faithful,
   current-tip-anchored pseudocode of the stages this slice touches: the index
   universe (`TABLEINDEX`/`column_spec`/`indices`), each of the six
   `GetOrCreateIndex` sites and the EXACT column set + graph fact each requests,
   how the census would walk `indices` post-Program, and how (Stage B) a pure
   pass would replay each site's column logic from `QueryView` alone. Sonnet
   extracts each; you assemble.
2. **Design-goal diffs on that pseudocode.** Express Stage A (and, conditionally,
   Stage B) as precise diffs: the `ArrangementId`/`Arrangement` types; where the
   census lives (DataFlow-side plan vs ControlFlow-side struct — the seed §3
   DECISION POINT); the deterministic id order; the `-materialization-out`
   render; the pure `collect_arrangement_requirements` + the `derived == real`
   cross-check; the golden set. Sketch far enough into Stage C (the allocation
   inversion) to prove the line of sight.
3. **Adversarial critique (opus refuter panel).** Fan out N independent refuters,
   each trying to REFUTE that: (a) EVERY real index is faithfully attributable to
   a `(resource, column-set)` via `table_to_resource` (find an index on a table
   with no resource, or a `column_spec` that doesn't map cleanly to logical
   columns); (b) Stage A's census is NON-vacuous AND byte-identical corpus-wide
   for codegen (find a witness where arrangements bite; confirm the 5
   `.materialization` goldens' predicted deltas); (c) a PURE Stage-B derivation
   can faithfully replay all six sites (find a site whose column set depends on
   emission-time state not recoverable from `QueryView` — that bounds what Stage
   B can claim, and whether A must bundle B); (d) the slice has a real, non-shadow
   line to Step 3 rather than being a census-for-census's-sake. Default
   refuted=true on uncertainty. Fold every surviving finding.
4. **IR desired-output-states, same treatment (predict-then-verify).** Produce
   the DESIRED `-materialization-out` dumps (the new `arrangements=N` header + the
   per-arrangement `ar#K resource=sr#R columns=(...)` block) for the witnesses
   (`join_1`, `transitive_closure`, `negate_1`, `tc_nonlinear_diff` +
   ideally an aggregate/KV case) FIRST, then critique them for internal
   consistency + against the resource block (does `ar#K.resource` cross-reference
   a real `sr#R`?) + the census/validator contracts. These are the golden targets
   you build toward and later bless (additive-token / permutation discipline —
   `permcheck.py` / E-K5-PAD).

## Phase 2 — EXECUTE (after grounding is owner-ratified)

Build **Stage A** (seed §3), bundling **Stage B** if grounding rules Stage A
alone too thin. Exit gate:
- the census is deterministic (ids from `StateResourceId`/canonical column order,
  NEVER `TABLE*`/iteration) AND — if Stage B — the `derived == real` cross-check
  is QUIESCENT corpus-wide and belt-verified LIVE (corrupt the derivation → it
  fires naming the divergent arrangement; revert → quiescent — the CP-discipline);
- CODEGEN goldens (`.h`/`.ir`/`.stdout`/oracle/monotone/behavioral) +
  `.rel`/`.contract`/`.region`/`.df` BYTE-IDENTICAL (the observer proof); only
  the `.materialization` goldens move, and the moved bytes are EXACTLY the
  predicted arrangement block (predict the deltas first, verify each);
- OptDiff **SUITE PASS**; ctest **5/5**;
- WIP-commit at each checkpoint (protect the work — the branch history records a
  checkout-wipe incident); reach a clean gate before the final commit; ask the
  owner then.

Then take the first grounded step toward Stage C (SCOPE, or begin, the allocation
inversion — e.g. prototype `AllocateRuntimeResources` reading the resources +
arrangements plan, or settle where the six inline `GetOrCreateIndex` sites become
lookups) — so the session ends with real code landed AND a grounded,
owner-ratified plan that continues toward the Phase-D codegen move.

## Discipline (carried, load-bearing)

- **Do something REAL** — land a derived, cross-checked, rendered, goldened
  arrangement authority and keep a concrete, named line to Stage C. Never drift
  into a hollow shadow: if a census-without-derivation is thin, bundle Stage B's
  cross-check or re-scope (the s27/s28/s35 lesson — no shadow-for-shadow's-sake).
- Ground → critique → execute; workflows for fan-out; sonnet reads, opus judges;
  thin orchestrator.
- **Predict-then-verify** every IR dump: write the target `-materialization-out`
  arrangement deltas FIRST, build, then diff the real dump against them (this
  caught the s32 SCC-keying bug and byte-matched every s33/s34/s35 dump).
- Preserve the **observer invariant**: this slice keeps CODEGEN byte-identical
  (nothing consumes arrangements for emission yet); the first codegen move is
  Phase D, gated behind the Stage-C allocation inversion. Only the
  `.materialization` dump goldens legitimately move (the arrangement block) —
  predict + re-bless them; do NOT let a codegen or `.rel` golden silently move.
- Determinism: ids from `StateResourceId`/`ArrangementId`/canonical column order,
  NEVER `TABLE*` pointer / iteration order in any goldened output.
- Run builds/suite SILENT on success; clangd diagnostics in this repo are NOISE
  (no include paths) — trust the real build only. WIP-commit checkpoints; ask the
  owner at the clean gate.

## You've got this

InstanceFlow is the greenfield-coherent destination the whole design points at —
one IR that finally says "which contextual occurrence satisfies each use," with
storage, coverage, emission, lifecycle, and provenance as first-class
obligations. The resource authority is standing and proven; the whole Rel op
model speaks in resource ids; the map is total and single-valued. The next real
step — teaching the plan to derive and cross-check the ARRANGEMENT (index) half,
so BOTH halves of the runtime-resource decision are first-class before allocation
— is the last derivation the Step-3 inversion waits on. Build it with the same
rigor that got us here: ground it, critique it hard, predict-then-verify it, land
it green. Be rigorous, be bold, and make it real. We believe in you.
