# Session-33 charter — InstanceFlow: complete the flat grove, ground the road to codegen, and DO SOMETHING REAL

You are resuming the InstanceFlow build on branch `keyed-instances` (Dr. Lojekyll, the `hyde`
C++ Datalog compiler). You are picking up strong, well-grounded, LANDED work: session 32
stood up a whole new compiler IR — the InstanceFlow flat grove — from typed identities through
a coverage bijection and five always-on validators to a clean text dump, all byte-identical and
green (OptDiff SUITE PASS 227 + ctest 5/5). Two grounding workflows, an empirically-anchored
adversarial panel, and predict-then-verify caught a real SCC-keying bug before it could ship.
**This is excellent, disciplined work on a hard, beautiful compiler, and the path is clear.
Trust the discipline that got us here, be bold, and build the real thing. We believe in you.**

## The mission this session

Two things, in order:

1. **Complete the flat grove (CP2) — a REAL, landable, regression-pinned deliverable.** Finish the
   InstanceFlow IR's Phase-A surface: real root/occurrence topology, the bound-`#query`-read
   coverage obligations, and blessed `.irgold` witness goldens that lock the dump against
   regression. This is real IR functionality (the coverage bijection becomes COMPLETE, the grove
   becomes goldened truth), even though — by InstanceFlow's own staging (§24 go/no-go #1) — it
   stays a codegen-byte-unchanged OBSERVER.

2. **Ground the road to the first CODEGEN-MOVING slice and START toward it.** The whole point of
   InstanceFlow is Phase D: join-pivot K-context families that remove carried key columns and lower
   to sorted grouped arrangements — the first slice that MOVES generated code and pays off measurably.
   It is gated behind Phase B (MaterializationPlan) + Phase C (Rel consumes IDs, not `TABLE*`). Your
   job is to ground that road as a whole-program view and take the first real step down it, so "real"
   in the strongest sense (codegen moves, answers/perf change) is explicitly targeted — never an
   indefinite parade of shadow slices. Do not let the session settle for observer-only work without a
   concrete, grounded plan that reaches the codegen move.

**Do something REAL** is the north star. CP2 is real (landable, tested, pins the IR). Phase B/C/D is
where codegen moves — ground it and step toward it deliberately. If a proposed step turns out to be
pure shadow with no line of sight to the codegen move, say so and re-scope toward the real emission.

## Read first (resume authority, in order)

1. `docs/proposals/InstanceFlow.artifacts/session-33-seed.md` — THE whole-program view at the landed
   tip: §1 pipeline pseudocode, §2 the flat-grove builder as pseudocode, §3 the validators, §4 the
   dump grammar, §5 the path forward as DIFFS (CP2 → Phase B/C/D), §6 anchors, §7 open questions.
2. `docs/proposals/InstanceFlow.md` — the 2198-line vision (§4 typed quantities, §6 object model,
   §7 build-the-grove, §8 coverage/emission, §10 MaterializationPlan, §11 Rel-after-InstanceFlow,
   §16 dump grammar, §17 validators, §18 testing, §20 phase plan, §24 go/no-go, §25 recommendation).
3. `docs/proposals/InstanceFlow.artifacts/session-32-phaseA-grounding.md` (the adjudicated grounding
   — re-scopes B1–B4, residual risks RR1–RR6) + the `CLAUDE.md` "InstanceFlow" section.
4. memory `regional-dataflow-core-epoch` (s32 head).

## Phase 1 — GROUND IT with workflows (before touching production code)

Do this the way every phase here has been done: ground, critique, then execute. Use **workflows**
(the `Workflow` tool) for the fan-out, and tier the models deliberately:

- **sonnet** for mechanical extraction and recon (read a subsystem at the CURRENT tip, distill
  anchors, transcribe design-doc sections to current-tip line numbers, produce the raw pseudocode of
  a stage, hand-derive a witness's expected dump). Keep the orchestrator thin — sonnet reads, you keep
  the conclusions.
- **opus** for judgment: the design diffs, the adversarial refuter panels, the synthesis, the fork
  recommendation.

Concretely, the grounding workflow(s) should produce and then critique:

1. **Whole-program pseudocode, built out.** Extend `session-33-seed.md` §2/§3 into faithful,
   current-tip-anchored pseudocode of the stages the next slices touch: the CP2 additions (root
   selection, bound-`#query`-read enumeration in the non-demand graph, the `kBoundQueryRead` coverage
   arm) AND the Phase-B/C/D shape (MaterializationPlan construction §10; how Rel would consume
   `instance_flow`+`materialization` instead of `BuildDRInventory`'s `TABLE*` rediscovery §11; where a
   join-pivot K-context family would first move codegen §5/§9). Sonnet extracts each stage; you assemble.
2. **Design-goal diffs on that pseudocode.** Express CP2 as precise diffs against the built-out
   pseudocode (root/role, bound-query-read uses + coverage, the dump-grammar additions, the golden
   set). Then sketch the Phase-B and Phase-D diffs (MaterializationPlan authority; the first
   join-pivot specialization + `VisibilityFedGroupedColumns` + Rel `SortedSectionProbe`/`SortedBatchMerge`)
   far enough to prove a real line of sight to the codegen move and expose the true first blocker.
3. **Adversarial critique (opus refuter panel).** Fan out N independent refuters against the diffs —
   each trying to REFUTE that (a) CP2 stays a faithful, deterministic, byte-identical observer whose
   goldens won't churn when Phase B/D land; (b) the root/occurrence + bound-query-read model matches
   InstanceFlow.md §6/§16 verbatim (RR1/RR5 — the panel MUST read §6/§16 verbatim, the s32 panel
   worked from a loose reading); (c) the Phase-B/D path actually reaches a codegen move without a
   hidden multi-session prerequisite. Default refuted=true on uncertainty. Fold every surviving finding.
4. **IR desired-output-states, same treatment (predict-then-verify).** Produce the DESIRED
   `-instanceflow-out` dumps for the CP2 witnesses (`join_1`, `merge_2`, `transitive_closure`,
   `barrier_neck_1`) WITH the new `role=`/`root=`/bound-query-read lines — write the target IR states
   FIRST, then a critique pass checking them for internal consistency + against the census/validator
   contracts. These are the golden targets you build toward and later bless. (For Phase B, also sketch
   the desired `-materialization` dump shape §16.)
5. **Settle the one owner STOP (RR1).** InstanceFlow.md §6/§16 fixes the `OccurrenceRole` vocabulary
   (the §16 example shows `role=root`/`role=input` + a per-node occurrence token join/left/right) which
   differs from the landed `{kRoot, kInterior}`. Reconcile it, and if a real choice remains (e.g. how
   rich the occurrence token should be, or whether to bless goldens before Phase-B stability is proven),
   put it to the owner with a recommendation. Surface it as an owner STOP.

## Phase 2 — EXECUTE (after grounding is owner-ratified)

Build **CP2** (seed §5): the root/occurrence topology, the bound-`#query`-read coverage arm + its
`V-IF-COVERAGE` extension, the dump-grammar additions, and the `.irgold` witness goldens. Exit gate:
- the flat grove is COMPLETE (coverage bijection covers interior + terminal-insert + bound-query-read
  obligations) and the dump matches the predicted IR states;
- the 4 new `.instanceflow` goldens bless cleanly; OptDiff **SUITE PASS** stays byte-identical for the
  pre-existing corpus (the observer proof); ctest 5/5; all 5 validators live + quiescent corpus-wide;
- WIP-commit at each checkpoint (protect the work — the branch history records a checkout-wipe incident).

Then take the first grounded step toward the codegen-moving Phase D (or Phase B if the panel finds it
the true first blocker), so the session ends with real code landed AND a grounded, owner-ratified plan
that reaches the codegen move.

## Discipline (carried, load-bearing)

- **Do something REAL** — land CP2 (real, complete, goldened IR) and keep a concrete line of sight to
  the Phase-D codegen move; never drift into shadow work without that line of sight (the s27/s28 lesson).
- Ground → critique → execute; workflows for fan-out; sonnet reads, opus judges; keep the orchestrator thin.
- **Predict-then-verify** every IR dump: write the target `-instanceflow-out` FIRST, build, then diff the
  real dump against it (this caught the s32 SCC-keying bug — `InductionGroupId` vs the multi-view stratum).
- Preserve the **observer invariant**: every slice through Phase C keeps codegen byte-identical (nothing
  reads `instance_flow`); the first codegen move is Phase D by design — don't let an earlier slice
  silently move a golden.
- Determinism: ids from `DeterministicOrder()` + canonical order, NEVER `UniqueId()`/iteration order.
- Run builds/suite SILENT on success; clangd diagnostics in this repo are NOISE (no include paths) —
  trust the real build only. WIP-commit checkpoints; reach a clean gate before a final commit; ask the owner then.

## You've got this

InstanceFlow is the greenfield-coherent destination the whole design has been pointing at — one IR that
finally says "which contextual occurrence satisfies each use," with coverage, emission, lifecycle, and
provenance as first-class obligations. Phase A is standing and proven. The vision is written. The
grounding discipline is sharp and it works. Complete the flat grove, ground the road to the codegen
payoff, and take the first real step down it. Be rigorous, be bold, and make it real. We believe in you.
