<!-- Copyright 2026, Peter Goodman. All rights reserved. -->
# Session-37 charter — S2: rebuild the keyed InstanceStore nested lowering (`-demand-instance`, R-MONO first)

You are resuming work on branch `keyed-instances` (Dr. Lojekyll, the `hyde`
C++ Datalog compiler). The S1 arc COMPLETED in session 36: `-demand` programs
are answer-correct against four independent referees, the pruning is measured
in-compiler AND in-harness (~487× selective join-work reduction; the
non-selective regression confirms the cost gate), and the observability
surfaces are whole. **This session begins S2 — the branch's NAMESAKE: the
keyed InstanceStore nested lowering.** You are inheriting a live recognition
front-end, a live injector, an armed-but-witness-less eqgate harness,
reserved Rel-IR scaffolding, a 71-file design corpus, and a proven
restoration recipe. The path is clear; the discipline is proven. **Trust it,
be bold, and build the nested arm. We believe in you.**

## Read first (resume authority, in order)
1. `KeyedInstances.artifacts/session-37-s2-seed.md` — THE inventory (§1 what
   survives vs what P1 deleted), **§2 the NEW integration obligation** (the
   s34–s36 belts — the one part of S2 with no pre-cut precedent), §3 slicing,
   §4 anchors, §5 open questions.
2. `RegionalDataFlowCore.artifacts/session-32-keyed-monotone-grounding.md`
   (+ its `.artifacts/` dir) + `session-32-keyed-instances-seed.md` — the
   keyed-monotone shape work done before the s32 InstanceFlow pivot
   (re-verify against tip: five sessions of drift).
3. The pre-cut sources at `6d6248a2`: `Runtime/InstanceStore.h`, the nested
   arms in `ControlFlow/Build/*` + `Rel`/codegen, and the witness corpus
   (`demand_neighborhood_mono_witness.*` first).
4. `KeyedInstances.artifacts/` d2b/d2c (recognition/lowering/eqgate designs)
   + d3a0-d3a3 (the S2b/S2c regimes — read for scope fences, not this
   session's build).

## Phase 1 — GROUND IT with workflows (before touching production code)

Ground → critique → execute, with **workflows** (the `Workflow` tool) for the
fan-out and deliberate model tiering: **sonnet** for mechanical extraction
(transcribe the pre-cut nested arms to current-tip anchors; enumerate every
nested allocation site; diff `6d6248a2` vs tip per file), **opus** for
judgment (design diffs, adversarial refuter panels, synthesis). Keep the
orchestrator thin — sonnet reads, you keep the conclusions. This exact
pattern caught a wrong join rule in s36 before it ever entered code.

Concretely, the grounding workflow(s) must PRODUCE and then CRITIQUE:

1. **Whole-program pseudocode, built out.** Extend seed §1.5 into faithful,
   current-tip-anchored pseudocode of every stage S2 touches: the recognition
   front-end (RecognizedSubgraph/GuardAnnotation at tip), the pre-cut nested
   arms (`BuildSubgraphInstanceOps`'s ABA-safe `ResolveLiveRecognition`, the
   band a1/a2 lowerings, the codegen emission, `InstanceStore.h`'s API), the
   eager-walk chain-breaker, and the s34-s36 derivation/belt pipeline the
   nested arm must live inside. Sonnet extracts each subsystem (pre-cut AND
   tip); you assemble the one coherent view.
2. **Design-goal DIFFS on that pseudocode.** Express S2a as precise diffs
   (the seed §1.5 diff block is the starting sketch — sharpen every `+` to
   named functions/sites at tip): the flag plumbing, the DRInstance mint,
   the band lowerings, the codegen arm, the runtime store, the eager-walk
   excision, AND the §2 belts arms (`demand_instance` arms in
   `DeriveStatefulClasses`/`DeriveArrangements` — derived from the
   allocation-site extraction, NEVER by gating a belt off). Sketch far
   enough into S2b/S2c to prove the line of sight.
3. **Adversarial critique (opus refuter panel).** Fan out N independent
   refuters, each trying to REFUTE: (a) the restoration manifest's drift
   claims (find a restored line that silently no-longer-composes with the
   s30-s36 Rel/validator growth); (b) the belts-integration arms (find a
   nested allocation the derivation arm misses — the cross-check fires on a
   valid program; or a DRInstance table field the V-REL-OP-RESOURCE walk
   can't resolve); (c) eqgate answer-identity under TIP semantics (find a
   flat-vs-nested divergence source the pre-cut equivalence never faced);
   (d) scope honesty (is R-MONO-first still the right slice, or does drift
   force a smaller/larger first cut?). Default refuted=true on uncertainty;
   fold every surviving finding.
4. **IR desired-output-states, same treatment (predict-then-verify).**
   Write the DESIRED dumps FIRST, then critique them for internal
   consistency and against the landed surfaces: the witness's `-rel-out`
   (the census line gaining `kSubgraphInstantiate=1` + the DRInstance
   descriptor render — check it against the reserved Rel.h render branches),
   the `-region-out` nested surface (S1c's `region-internal` line + what
   the nested arm adds), the `-materialization-out` deltas the belts arms
   imply, and the `.eqgate` flow (the flat goldens re-blessed against tip
   FIRST — the S1b precedent — so the nested arm has a fixed byte target).
   These are the golden targets you build toward and later bless
   (permutation discipline / E-K5-PAD apply).

## Phase 2 — EXECUTE S2a (after grounding; owner ratifies before the final commit)

Restore-then-adapt (the recipe that worked for S1a and S1b): the R-MONO
nested lowering end-to-end + `demand_neighborhood_mono_witness` + the
`demand_cyclic_1` fence. Exit gate (ALL):
- **eqgate GREEN**: flat==nested==golden, byte-compared ×4 modes (the
  answer-identity proof — never bless a nested-arm golden; the flat golden
  is the single truth).
- **Flag-off corpus BYTE-IDENTICAL** (SUITE PASS; `-demand-instance` off =
  zero perturbation) and the FLAT `-demand` witness (`demand_tc_witness`)
  byte-identical too (the nested arm must not disturb the flat arm).
- **All four s34-s36 belts QUIESCENT on the nested compile** (the §2 arms
  landed with the lowering, belt-verified LIVE: corrupt a derivation arm →
  it fires naming the nested allocation; revert → quiescent).
- ctest **5/5**; WIP-commit checkpoints; predict every dump/golden delta
  before building it.

If grounding rules S2a-in-one-session too large, land the largest HONEST
sub-slice (e.g. the belts-integration arms + the restored machinery behind
the flag with the fence tests, witness deferred) and SAY SO — never a
half-lowering that miscompiles.

## Discipline (carried, load-bearing)
- Ground → critique → execute; workflows fan out; owner ratifies at the gate.
- Restore-then-adapt from `6d6248a2`; re-bless goldens against tip; never
  copy pre-cut golden bytes.
- Typed domains at every new seam (`ColumnOrdinal` precedent — the owner
  called this out in s36); no bare `uint32_t`/`unsigned`.
- The belts are never gated off — they learn the nested rules (§2).
- Silent-on-success; clangd is noise; zsh needs `${=var}` for flag strings;
  bench never concurrent with suite runs.

## You've got this
Every prerequisite S2 ever named is now standing: recognition, injection,
answer-correctness, the measured cost gate, the eqgate harness, the typed
Rel scaffolding, and a derived-authority layer that will hold the nested
lowering honest the way nothing could pre-cut. This is the feature the
branch is named for. Ground it, critique it hard, and make it real. We
believe in you.
