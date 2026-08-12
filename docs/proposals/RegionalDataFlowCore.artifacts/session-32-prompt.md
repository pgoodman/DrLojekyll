# Session-32 charter — GROUND then BUILD keyed instances (the namesake), and DO SOMETHING REAL

You are resuming the keyed-instance greenfield rewrite on branch `keyed-instances` (Dr. Lojekyll, the
`hyde` C++ Datalog compiler). You are picking up strong, well-grounded work — the last several sessions
landed real, verified progress (S1a resurrected flat `-demand` flag-off-byte-identical; the vestigial
`IsConditional` analysis was removed byte-clean; the architecture direction was sharpened with the owner).
**You are doing good work on a hard, beautiful compiler, and the path is clear. Trust the discipline that
got us here, be bold, and build the real thing.**

## The mission this session

The owner has committed to the branch's namesake: **keyed instances** — the nested, trie-inducing store
where a bound `#query` lowers to a keyed sub-database per demanded key, instead of a flat guard-web that
just adds tables. No more intermediate flat approaches. Validate against two goldens that ALREADY exist
(`.stdout` non-keyed = answer ground truth; `.monotone.stdout` = the add-only monotone projection).

**Your deliverable is REAL, not a shadow.** By the end of this arc a `-demand` program must lower to a
keyed instance store end-to-end, produce answers byte-equal to the non-keyed golden, and MOVE codegen
goldens (a keyed store replacing flat guard-web tables). The monotone slice is the first real cut.

## Read first (resume authority, in order)

1. `session-32-keyed-instances-seed.md` — THE whole-program seed: §0 the settled decisions, §1 the
   pipeline as pseudocode, §2 the recognition→lowering seam (`RecognizedSubgraph`), §3 the path forward
   as DIFFS (keyed-monotone slice 1, keyed-differential slice 2), §4 the assets (this is a
   RE-INTEGRATION, not greenfield), §5 the two-golden gate, §6 the InstanceStore-vs-InstanceFlow fork.
2. memory `regional-dataflow-core-epoch` (s30/s31 head) + `demand-subgraph-unification` +
   `shared-arrangements-mapping` (trie/sorted-layout) + `seekable-iterators-wcoj`.
3. The surviving blueprints: `docs/proposals/DemandSeeds.artifacts/d3-instance-store-target.md` (the
   InstanceStore paper-first spec), `docs/proposals/InstanceFlow.md` (the deeper context-family IR
   vision), `docs/proposals/KeyedInstances.artifacts/` (D1/D2/D3).
4. `include/drlojekyll/Runtime/StateCell.h` (the keyed store you reuse) + `lib/Rel/Rel.h:855-903` (the
   reserved `kSubgraphInstantiate`/`kInstanceDeath`/`DRInstance` scaffolding) + `lib/Rel/Rel.cpp:1363`
   (`BuildDRInventory`, the mint site).

## Phase 1 — GROUND IT with workflows (before touching production code)

Do this the way every phase here has been done: ground, critique, then execute. Use **workflows**
(the `Workflow` tool) for the fan-out, and tier the models deliberately:

- **sonnet** for mechanical extraction and recon (read a subsystem, distill anchors, transcribe existing
  design docs to current-tip line numbers, produce the raw pseudocode of a stage). Keep the orchestrator
  thin — sonnet does the reading, you keep the conclusions.
- **opus** for judgment: the design diffs, the adversarial refuter panels, the synthesis.

Concretely, the grounding workflow should produce and then critique:

1. **Whole-program pseudocode, built out.** Extend `session-32-keyed-instances-seed.md §1` into faithful,
   current-tip-anchored pseudocode of the pipeline stages that keyed instances touches: `ApplyDemandTransform`
   (recognition), `BuildDRInventory` + `Stratum.cpp` lowering (the mint/lower site), the `StateCellStore`
   runtime contract, and the codegen emission for a keyed store. Sonnet extracts each stage; you assemble.
2. **Design-goal diffs on that pseudocode.** Express the keyed-monotone lowering (seed §3 slice 1) as
   precise diffs against the built-out pseudocode — the new `BuildDRInventory` arm that consumes
   `RecognizedSubgraphs` and mints `kSubgraphInstantiate`, the `LowerSubgraphInstance` region emission,
   and the runtime store binding. Then the keyed-differential diff (slice 2, `kInstanceDeath` + retraction).
3. **Adversarial critique (opus refuter panel).** Fan out N independent refuters against the diffs — each
   trying to REFUTE that the monotone slice is (a) answer-equal to the non-keyed golden, (b) sound against
   the four always-on abort-validators (RowContract, origin_decls, Rel eager-web cross-checks, Regional
   census), (c) buildable by re-integrating `StateCellStore` + the reserved Rel ops. Default refuted=true
   on uncertainty. Fold every surviving finding into the diffs before executing.
4. **IR desired-output-states, same treatment.** Produce the DESIRED `.df` / `.rel` / `.ir` / `.h` /
   `.region` dumps for the keyed-monotone witness (predict-then-verify: write the target IR states FIRST),
   then a critique pass that checks them for internal consistency + against the census/validator contracts.
   This is the golden target you build toward and later bless.
5. **Settle the fork (seed §6): InstanceStore vs InstanceFlow.** Re-read both surviving designs, put the
   fork to the owner with a recommendation (default: InstanceStore-monotone first as the beachhead,
   InstanceFlow as the generalization once one keyed lowering works). This is an owner STOP — surface it.

## Phase 2 — EXECUTE the first real slice (keyed-monotone)

After the grounding is owner-ratified, build **keyed-monotone** (seed §3 slice 1): reuse `StateCellStore`,
un-reserve `kSubgraphInstantiate`, add the `BuildDRInventory` keyed arm + `LowerSubgraphInstance`, and a
NEW witness (`.dr` + `.drflags=-demand` + `.probes` + `.batches` + monotone/oracle/behavioral goldens; the
deleted `demand_neighborhood_mono_witness` is the shape precedent). Exit gate:
- keyed answers byte-equal `.monotone.stdout` (add-only) AND `.stdout` on monotone programs (×4 modes),
- codegen goldens MOVE (keyed store replaces flat guard-web tables) — bless the new witness,
- OptDiff **SUITE PASS** still byte-identical for the flag-off corpus, ctest 5/5,
- WIP-commit checkpoints on the branch (the memory records a checkout-wipe incident — protect the work).

## Discipline (carried, load-bearing)

- **Do something REAL** — the monotone slice must lower end-to-end and be answer-equal, not a
  codegen-byte-unchanged shadow. If a proposed step turns out to be observer-only, say so and re-scope
  toward the real emission (the s27/s28 lesson: model-drives-codegen must actually move codegen).
- Ground → critique → execute; workflows for fan-out; sonnet reads, opus judges; keep the orchestrator thin.
- Predict-then-verify the IR dumps; run builds/suite SILENT on success; clangd is NOISE.
- Hand-pick hunks from the surviving design/deleted code — do NOT mechanically reapply a whole pre-cut
  diff (the InstanceStore family drifted through P2–P9; re-target the current typed model). Keep
  `Program::Build` 4-arg.
- WIP-commit checkpoints; reach a clean gate before proposing a final commit; ask the owner then.

## You've got this

This branch has a real, measured win at its core (demand pruning: `idx_hops` 40000→11 selective) and a
clean target (keyed instances = the nested trie the whole design has been pointing at). The hard
recognition front-end is DONE. The runtime store EXISTS. The Rel ops are RESERVED. The design docs
SURVIVE. The goldens are ALREADY THERE. This is a re-integration of a deeply-designed system onto a solid
typed foundation — exactly the kind of work that's gone well here before. Be rigorous, be bold, and make
it real. We believe in you.
