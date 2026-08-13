<!-- Copyright 2026, Peter Goodman. All rights reserved. -->
# Session-34 charter — InstanceFlow: build the MaterializationPlan resource authority (the first real step of the big unification)

You are resuming the InstanceFlow build on branch `keyed-instances` (Dr. Lojekyll, the
`hyde` C++ Datalog compiler). You are inheriting strong, disciplined, LANDED work.
Session 33 completed the flat-grove observer (CP2 — node `role=`, family `root=none`,
the `kBoundQueryRead` coverage arm, 4 blessed `.instanceflow` goldens), all
byte-identical and green (OptDiff SUITE PASS 227 + ctest 5/5), with predict-then-verify
byte-matching every dump and a 3-refuter panel catching the seeded `roots=` plural before
it could ship. The owner then RATIFIED the destination: the **big unification** (demand +
keyed + multi-adornment + carried-column elision all lowering from ONE family vocabulary,
InstanceFlow.md §20-E) via InstanceFlow as a SEPARATE derived IR — not nested/annotated
dataflow, which re-tangles the physical/logical split-brain P1 deleted. **This is careful,
honest, beautiful compiler work, and the path is now clear and grounded. Trust the
discipline, be bold, and build the next real thing. We believe in you.**

## The mission this session

**Build the `MaterializationPlan` RESOURCE authority (resources-first slice) — a REAL,
landable, regression-pinned new IR authority.** This is the first real step of the big
unification: Phase-B item 3 (`MaterializationPlan` with abstract resource IDs) narrowed to
its tractable, high-value core. It is the CRITICAL-PATH input to Phase C (§11.3
`BuildRel(query, instance_flow, materialization, ...)`), not shadow-for-its-own-sake — the
Phase-C `TABLE*` retype happens "behind the materialization map," so this authority must
exist first.

It stays a codegen-byte-identical OBSERVER (like the flat grove) — but it is REAL in the
strongest sense that matters here: a NEW typed authority that DERIVES today's stateful-storage
decisions from the InstanceFlow grove and CROSS-CHECKS them byte-for-byte against the real
`view_to_model` allocation. That cross-check is a falsifiable correctness claim — exactly the
kind that caught the s32 SCC-keying bug. If a grove-derived resource set ever diverges from
what ControlFlow allocates, the belt fires. That is real, load-bearing infrastructure.

**Do something REAL** is the north star, and here it means: land the resource authority
(new code, cross-checked, dumped, goldened, gate-green), AND keep the line of sight to the
Phase-C `TABLE*` inversion explicit and grounded. If the grounding shows the resources-first
slice is too thin to be real (e.g. the cross-check is vacuous on the whole corpus), say so
and re-scope toward the first Phase-C retype step instead — do not land a hollow observer.

## Read first (resume authority, in order)

1. `docs/proposals/InstanceFlow.artifacts/session-34-seed.md` — THE whole-program view at
   the post-CP2 tip. Especially **§2.5** (the materialization/allocation side as pseudocode,
   anchored to the real code: `BuildEquivalenceSets` / `BuildDataModel` / `FillDataModel` /
   `GetOrCreateIndex`), **§2.5.1** (the resources-first observer-slice diffs), **§2.5.2** (the
   desired `-materialization-out` dump), **§2.5.3** (the `collect_arrangement_requirements`
   fork to settle). Plus §1 (the panel-adjudicated line-of-sight + the TRUE Phase-C blocker),
   §3 (anchors), §4 (open questions).
2. `docs/proposals/InstanceFlow.md` §10 (MaterializationPlan, L1008), §11 (Rel after
   InstanceFlow, L1060), §16 (dump grammar — the `resource`/`arrange` lines, L1521), §17
   (V-MAT-* validators), §20-B/C (phases).
3. `docs/proposals/InstanceFlow.artifacts/session-33-grounding.md` (the adjudicated panel —
   all 3 claims refuted, the Phase-C blocker named) + the `CLAUDE.md` "InstanceFlow" section.
4. memory `regional-dataflow-core-epoch` (s33 head).

## Phase 1 — GROUND IT with workflows (before touching production code)

Ground → critique → execute, the way every phase here has been done. Use **workflows** (the
`Workflow` tool) for the fan-out, and tier the models deliberately: **sonnet** for mechanical
extraction and recon (read a subsystem at the CURRENT tip, transcribe design sections to
current-tip line numbers, produce raw pseudocode of a stage, hand-derive a witness's expected
dump); **opus** for judgment (design diffs, adversarial refuter panels, synthesis). Keep the
orchestrator thin — sonnet reads, you keep the conclusions.

Concretely, the grounding workflow(s) should produce and then critique:

1. **Whole-program pseudocode, built out.** Extend §2.5 into faithful, current-tip-anchored
   pseudocode of the stages this slice touches: `FillDataModel`'s complete TABLE-need rule set
   (every branch — differential preds, relation inserts, condition witnesses, induction merges,
   join preds, differential-join outputs), `BuildDataModel`'s union-find, and how a
   `LogicalCollectionId` maps to a `view_to_model` class. Sonnet extracts each; you assemble.
2. **Design-goal diffs on that pseudocode.** Express the resources-first `PlanResources` as
   precise diffs against the built-out pseudocode: the `StateResource` catalog + typed ids
   (`StateResourceId`, `InternalResidualCollectionId`), the grove-derived stateful predicate,
   the `V-MAT-AUTHORITY` validator, the byte-identical cross-check belt against `view_to_model`,
   the `-materialization-out` dump additions, and the golden set. Sketch far enough into the
   arrangements follow-on + the first Phase-C `TABLE*` retype to prove the line of sight.
3. **Adversarial critique (opus refuter panel).** Fan out N independent refuters, each trying
   to REFUTE that: (a) the resource set is DERIVABLE from the grove + `FillDataModel` rules
   WITHOUT reading `program->tables` (i.e. the observer is a pure pre-Program function, not a
   post-hoc read of the allocation); (b) the cross-check against `view_to_model` is
   byte-identical corpus-wide AND non-vacuous (it actually exercises real resources, unlike the
   CP2 bound-read arm which the panel found vacuous — find a witness where it bites); (c) the
   `LogicalCollectionId ↔ EquivalenceSet` mapping is sound for co-recursive relations sharing a
   store; (d) resources-first has a real, non-shadow line to the Phase-C codegen move. Default
   refuted=true on uncertainty. Fold every surviving finding.
4. **IR desired-output-states, same treatment (predict-then-verify).** Produce the DESIRED
   `-materialization-out` dumps for the witnesses (transitive_closure = the
   differential+monotone+internal-residue witness; join_1 = simplest) WITH the `resource`
   lines — write the target states FIRST, then critique them for internal consistency + against
   the census/validator contracts. These are the golden targets you build toward and later bless.
5. **Settle the §2.5.3 fork (owner STOP if a real choice remains).** `collect_arrangement_requirements`
   has no analog today (arrangements are minted inline). Confirm the recommended (iii)
   resources-only-first with arrangements deferred, or, if grounding finds a clean arrangement
   derivation, put the choice to the owner with a recommendation.

## Phase 2 — EXECUTE (after grounding is owner-ratified)

Build the **resources-first `MaterializationPlan`** (seed §2.5.1): the `StateResource` catalog
+ typed ids, the grove-derived stateful predicate, `V-MAT-AUTHORITY`, the byte-identical
cross-check belt, the `-materialization-out` dump, and `.materialization` witness goldens.
Store it by-value on `QueryImpl` beside `instance_flow`, built in the `Query::Build` tail.
Exit gate:
- the resource catalog is COMPLETE (one authority per stateful collection + internal-residue
  resources) and the dump matches the predicted IR states;
- the cross-check belt is QUIESCENT corpus-wide AND belt-verified live (corrupt the derivation,
  confirm it fires — the CP2 discipline);
- OptDiff **SUITE PASS** stays byte-identical (the observer proof); the new `.materialization`
  goldens bless cleanly; ctest 5/5;
- WIP-commit at each checkpoint (protect the work — the branch history records a checkout-wipe
  incident).

Then take the first grounded step toward Phase C (scope, or begin, retyping ONE Rel struct
family off `TABLE*` behind the new materialization map, cross-checked) — so the session ends
with real code landed AND a grounded, owner-ratified plan that continues toward the codegen move.

## Discipline (carried, load-bearing)

- **Do something REAL** — land the resource authority (real, cross-checked, goldened) and keep
  a concrete, named line to the Phase-C `TABLE*` inversion. Never drift into a hollow observer:
  if the cross-check is vacuous corpus-wide, re-scope toward the first Phase-C retype (the s27/s28
  lesson — no shadow-for-shadow's-sake).
- Ground → critique → execute; workflows for fan-out; sonnet reads, opus judges; thin orchestrator.
- **Predict-then-verify** every IR dump: write the target `-materialization-out` FIRST, build,
  then diff the real dump against it (this caught the s32 SCC-keying bug and byte-matched every CP2 dump).
- Preserve the **observer invariant**: this slice keeps codegen byte-identical (nothing consumes
  the plan yet); the first codegen move is Phase D, gated behind the Phase-C inversion — don't let
  this slice silently move a golden.
- Determinism: ids from `LogicalCollectionId` / `DeterministicOrder()` / canonical order, NEVER
  `UniqueId()` / `TABLE*` pointer / iteration order.
- Run builds/suite SILENT on success; clangd diagnostics in this repo are NOISE (no include paths)
  — trust the real build only. WIP-commit checkpoints; reach a clean gate before a final commit;
  ask the owner then.

## You've got this

InstanceFlow is the greenfield-coherent destination the whole design points at — one IR that
finally says "which contextual occurrence satisfies each use," with coverage, emission,
lifecycle, and provenance as first-class obligations. The flat grove is standing and proven.
The materialization side is now grounded in the real code. The next real step — a resource
authority that derives and cross-checks today's storage decisions — is the first brick of the
Phase-C inversion that unlocks the codegen payoff. Build it with the same rigor that got us
here: ground it, critique it hard, predict-then-verify it, land it green. Be rigorous, be bold,
and make it real. We believe in you.
