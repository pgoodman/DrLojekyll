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

Sonnet extracts, opus judges, thin orchestrator (the s36 pattern: 4-sonnet
recon + 4-opus refuters caught a wrong join rule before it entered code).
The grounding must PRODUCE and then CRITIQUE:

1. **The restoration manifest** (the S1a recipe): per-file diff of
   `6d6248a2`'s nested-arm code vs tip — what restores byte-faithful, what
   must adapt to drift (the Rel op-model growth s30→s36, the s35 resource
   stamps, kJoinEmit/kIngestLoop, the InstanceFlow observers at the
   Query::Build tail). Enumerate EVERY nested-arm allocation site (tables,
   indexes, stores) — this feeds item 2.
2. **The §2 belts-integration design** (the crux, no pre-cut precedent):
   the exact `demand_instance` arms `DeriveStatefulClasses` and
   `DeriveArrangements` need so `CrossCheckMaterialization` /
   `CrossCheckArrangements` stay quiescent on nested compiles — derived
   from item 1's allocation-site extraction, NEVER by gating a belt off.
   Also: `DRInstance` field population vs the V-REL-OP-RESOURCE walk (s35
   skipped always-null fields — populated fields enter the belt), and the
   census/eager-web validators the nested arm must satisfy.
3. **Adversarial refuter panel (opus)**: (a) the restoration manifest's
   drift claims (find a restored line that silently no-longer-composes);
   (b) the belts-integration arms (find a nested allocation the derivation
   arm misses — the cross-check fires on a valid program); (c) the eqgate
   answer-identity claim under tip semantics (find a divergence source
   between flat and nested at tip the pre-cut equivalence didn't face);
   (d) scope honesty (is R-MONO-first still the right slice, or does drift
   force a smaller/larger first cut?). Default refuted=true on uncertainty.
4. **Predict-then-verify targets**: the witness's expected `-rel-out`
   census line (`kSubgraphInstantiate=1`, the DRInstance render), the
   `.eqgate` flow, and the flat-arm goldens re-blessed against tip FIRST
   (the S1b precedent) so the nested arm has a fixed target.

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
