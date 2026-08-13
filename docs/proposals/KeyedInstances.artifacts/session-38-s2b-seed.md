<!-- Copyright 2026, Peter Goodman. All rights reserved. -->
# Session-38 seed — S2b: the differential regimes (`-demand-retract`, kInstanceDeath, the full eqgate family)

> START-HERE for session 38 (owner ratified S2a at the s37 close: "commit and
> push, then seed the S2b charter"). Written at the s37 close (branch
> `keyed-instances`, tip `637e7a61`, pushed). S2a is COMPLETE and RATIFIED:
> the `-demand-instance` keyed InstanceStore nested lowering is LIVE,
> answer-identical to flat `-demand` (eqgate ×4 on both S2a carriers), all
> four s34-s36 belts taught the nested rules, SUITE PASS (232), ctest 6/6.
> Full record: `session-37-s2a-grounding.md` (§8 panel record, §9 landed
> state + the named S2b residue this seed expands).

## §0 What S2b IS (the pre-cut D3.a arc, re-landed)

The DIFFERENTIAL regimes of the nested lowering: `-demand-retract` makes the
fabricated demand message `@differential` (standing demands can be RETRACTED;
bound queries gain the generated `<name>_<bindings>_retract` entry point), so
a keyed instance can DIE (band-(a0): drain the demand net-removals frontier,
`FindInstance` + `RecycleCurrent`, the full (T,F) retract riding the generic
drop scan) and be REBORN. Plus the full diff-input × diff-demand composition
(`demand_diff_input_1`) and the R-DIFF flagship witness
(`demand_neighborhood_witness`: birth → retract → death → rebirth phases +
the `@differential` `nbhd_out` tap upgrading the eqgate to sorted
published-delta identity). ANSWER-IDENTICAL to flat by the same eqgate
mechanism; the flat `-demand -demand-retract` arm is itself restored-but-
unexercised at tip and gets re-proven first (the S1b/S2a flat-first
precedent).

## §1 Inventory at tip `637e7a61` (verified during s37 — the head start is LARGE)

ALREADY LIVE (landed or restored in S2a, dormant only for want of the flag):
- **The ENTIRE kInstanceDeath machinery, restored VERBATIM in S2a**: the mint
  arm (`BuildSubgraphInstanceOps`, gated `TableIsDifferential(demand_table)`
  — never true without `-demand-retract`), `DeathEffects` (the zero-counter
  death signature), `LowerSubgraphInstances`' death-band wiring
  (`removal_frontier` + V-INST-DEATH-COHERENCE + the orphan-mint fences),
  `ClassifyVector`'s removal/del/add vec arms, codegen band-(a0) +
  OVERDELETE-first band-(b) del/add-queue publish + the generated
  V-INST-PARTITION belt, and the whole validator family
  (`CheckInstanceDeathFrontier` / `CheckInstanceInputArm` /
  `CheckInstantiateEffects` / `CheckInstanceSolePub` / `CheckInstanceOrder` +
  V-INST-DRAIN's regime split). NONE of it needs restoring — it needs
  REACHING and WITNESSING.
- **The a2' input-removals machinery is LIVE AND WITNESSED** (S2a landed the
  e5 carrier `demand_diff_neighborhood_witness` precisely because
  `input_diff` derives from the input table, not the retract flag).
- **The `_retract` EmitQueryFriends arm is LIVE at tip** (S1b-era; the s37
  panel proved it was never deleted — `spec.retract_function` gated).
- **The S1b injector dispatcher's retract arm** ("retract arm gated on
  message differentialness") — in the restored registry-first
  `BuildQueryInjectorProcedure`; unexercised.
- **`Query::Build` is 6-arg** with `demand_retract` already a parameter; the
  S1a-restored `lib/DataFlow/Demand.cpp` carries the retract arm (fabricated
  message minted `@differential`) — restored from `6d6248a2`, UNEXERCISED
  since (no flag wires it).
- **The nested rel/region/ir surfaces**: the flags-twin pins the R-MONO
  shape; the death ops' render cases (`kInstanceDeath` op-render, census
  token, `DROpStratum`/`key_of` arms) all restored in S2a.
- **The eqgate harness** handles N carriers; `bless_copy` symlink protection
  proven again in s37.

MISSING (the actual S2b work):
- **`Main.cpp`**: `gDemandRetract` + the `-demand-retract` argv arm (implies
  `-demand`) + threading as `Query::Build`'s 5th argument (pre-cut
  `6d6248a2:bin/drlojekyll/Main.cpp:49-76,620-628` — extracted verbatim in
  scratchpad e4 during s37; trivially re-derivable from `git show`).
- **The witnesses** (restoration source `6d6248a2`, goldens FRESH-BLESSED):
  `demand_neighborhood_witness` (`.drflags` `-demand -demand-retract` +
  `.eqgate`; the R-DIFF flagship: retract/death/rebirth phases, HP-5
  over-materialization sentinel, the `@differential` tap) and
  `demand_diff_input_1` (diff-input × diff-demand, the full composition
  carrier). Eqgate family target after S2b: FOUR live `.eqgate` cases (mono,
  e5, flagship, diff_input — re-derive as `ls tests/OptDiff/cases/*.eqgate`,
  never propagate the constant).
- **The fence-diagnostic corpus stragglers** (all pre-cut, all
  clean-diagnostic, fences already live): `demand_recursive_content_1` (bare
  `-demand` — the upstream body-walk reject; ALSO the recorded SOUNDNESS
  PRECONDITION of the S2a arrangements skip, so its restoration is
  belt-relevant, not just corpus hygiene), `demand_agg_body_1` /
  `demand_kv_body_1` / `demand_config_agg_body_1` (demand-sink/R-MAT rejects,
  `.batches` + oracle/monotone goldens pinning definitional answers),
  `demand_mutual_content_1`, `demand_two_queries_1` (R-1BOUND). Verify each
  still draws its diagnostic at tip before adding to the runall.sh list.
- **`tests/RelValidators/`** (deleted at P1): `DeathFrontierTest` /
  `InputArmTest` / `InstanceEffectsTest` / `InstanceOrderTest` /
  `InstanceSolePubTest` — the PURE-validator negative-space death tests the
  restored `CheckInstance*` family was FACTORED for (their doc comments still
  promise "death-testable in tests/RelValidators"). Restore + CMake +
  ctest (6/6 → 7/7 or however the target splits; state the real number).
- **The R-DIFF skip-discriminator live-fire** (s37 panel, deferred to S2b by
  design): a deletion-capable guard join under `-demand-instance
  -demand-retract` — the UNCONDITIONAL R-JOIN-UNIFORM skip must FIRE
  CrossCheckArrangements (real universe keeps the delta-path pivot indexes)
  while the landed `!CanReceiveDeletions()`-gated skip stays QUIESCENT. This
  is the empirical proof the panel-fixed predicate earns its teeth.

## §2 KNOWN OBLIGATIONS with no S2a precedent (ground these FIRST)

1. **Frontier provisioning under differential demand.** Under R-MONO the
   demand net-additions frontier is minted by the eager cut-successor
   boundary append; under DIFFERENTIAL demand both ± frontiers are
   COMMIT-BAND products minted by the frontier-filter lowering (F-b1-3), and
   `LowerSubgraphInstances` deliberately FENCES mint-on-miss (the A2.6
   orphan-mint idiom, restored in S2a). Ground: does tip's
   `LowerDRFlow`/frontier-filter path provision `(demand, kNetRemovals)` +
   `(demand, kNetAdditions)` for a differential demand table exactly as
   pre-cut? (It should — Stratum.cpp is byte-identical to `6d6248a2` — but
   the composition with the S1b injector is NEW: pre-cut's injector predates
   the registry-first dispatcher.)
2. **The arrangements arm under diff-demand.** The S2a skip is gated
   `!CanReceiveDeletions()`; under `-demand-retract` the guard joins BECOME
   deletion-capable → the skip self-disables → derived == real requires the
   delta path to actually mint every pivot index (the panel's direct
   measurement said it does). Verify empirically on the flagship BEFORE
   blessing anything; if a divergence appears it is a REAL S2b finding, not
   a bless-over.
3. **The S1c Tier-1 residue.** `-region-out`'s demanded-interior
   `support=` byte is the OR over live guard JOINs' `CanReceiveDeletions()`;
   under retract it flips to `differential` — S1c proved the K5 Tier-2 path
   sufficient on the FLAT arm with MONOTONE demand only. Byte-diff the
   pre-cut `demand_neighborhood_witness.region.*` goldens (if any) / the
   Tier-2 output at tip and decide: still subsumed, or is this where the
   retired Tier-1 machinery (`CollectDemandInteriorDecls` /
   `ResolveInteriorSupport`) finally earns re-adding? DO NOT assume — the
   S1c note names this exact regime as the open observable.
4. **Behavioral goldens under retract (F32 policy).** The behavioral binary
   is the PLAIN compile (never `.drflags`); demand-gated publish taps
   legitimately diverge demand-lowered. The four differential-regime demand
   behavioral goldens were re-blessed to the definitional CBF pre-cut — the
   restored flagship's behavioral golden must come out the same way (byte
   vs `6d6248a2` expected EQUAL; verify like s37 did, where every answer
   surface matched pre-cut bytes exactly).
5. **Netting at the message boundary.** The FIVE-WAY INPUT-QUIESCENCE
   COUPLING comment (restored EmitSubgraphInstance head) leans on handler
   NetBatch netting; the retract path adds demand-side ± in one batch.
   The witness's batches exercise it; the coupling block is the review
   checklist for ANY touch of the band code.

## §3 Slicing

- **S2b-i (flat retract first, the S1b precedent):** wire `-demand-retract`
  (Main.cpp only), restore `demand_neighborhood_witness` +
  `demand_diff_input_1`, prove the FLAT `-demand -demand-retract` arm
  end-to-end (diffrun ×4 + oracle/interp/behavioral referees), bless flat
  goldens — the fixed byte targets.
- **S2b-ii (the nested arm):** eqgate the two carriers (the kInstanceDeath /
  death-band machinery goes LIVE for the first time); belts verification per
  §2.1/§2.2 + the discriminator live-fire; the region-out §2.3 decision.
- **S2b-iii (hygiene):** RelValidators death tests; the fence-diagnostic
  stragglers; records (CLAUDE.md, memory, FINDINGS if anything fired).
  If s2b-i surfaces real drift (the retract path is the LEAST-exercised
  restored surface — S1a restored it blind), land the largest honest
  sub-slice and say so.

## §4 Anchors

| Fact | Source |
|---|---|
| S2a landed state + S2b residue | `session-37-s2a-grounding.md` §9 |
| Panel record (folded findings incl. the skip predicate + V-INST-COHERE) | `session-37-s2a-grounding.md` §8 |
| Restoration tip | `6d6248a2` (unchanged; the s37 extractions in the session-37 scratchpad are gone — re-extract from git) |
| Pre-cut flagship witness | `git show 6d6248a2:tests/OptDiff/cases/demand_neighborhood_witness.*` (+ goldens) |
| Pre-cut diff_input carrier | `git show 6d6248a2:tests/OptDiff/cases/demand_diff_input_1.*` |
| Pre-cut Main.cpp retract arm | `git show 6d6248a2:bin/drlojekyll/Main.cpp` (:49-76, :620-628) |
| Pre-cut RelValidators tests | `git show 6d6248a2:tests/RelValidators/` + its CMakeLists |
| The death-band code (restored, dormant) | `lib/ControlFlow/Build/Procedure.cpp` `LowerSubgraphInstances` (death arm), `lib/CodeGen/CPlusPlus/Database.cpp` `EmitSubgraphInstance` band-(a0) |
| The regime-split validators (restored) | `lib/Rel/Rel.cpp` `CheckInstance*` + V-INST-DRAIN; decls + doc contracts `lib/Rel/Rel.h` tail |
| The arrangements arm + its S2b behavior | `lib/DataFlow/Materialization.{h,cpp}` `DeriveArrangements` demand_instance arm (self-disabling under deletion-capability BY DESIGN) |
| D3.a design corpus | `KeyedInstances.artifacts/` d3a0-d3a3 + `d3-instance-store-target.md` |
| S1c Tier-1 residue statement | CLAUDE.md S1b/S1c note ("observable only under -demand-retract") |

## §5 Open questions (name, don't assume)

1. Does the S1a-restored DataFlow retract arm (fabricated `@differential`
   demand message) still compose at tip? It has NEVER compiled since the
   restore — the first `-demand -demand-retract` compile is the real test.
2. §2.1 frontier provisioning composition with the S1b injector.
3. §2.2 the deletion-capable guard-join index universe (measure, then
   discriminator-fire).
4. §2.3 the Tier-1 support-OR decision (re-add vs still-subsumed).
5. Does the flagship need `-demand-retract` in its `.eqgate` compile line?
   (run_eqgate appends `-demand-instance` to `flags_of`, which already folds
   the `.drflags` — so `-demand -demand-retract -demand-instance` composes
   automatically. Verify, don't assume.)
6. `demand_diff_pub_1` (the sixth pre-cut eqgate case) — S2b or S2c? It is
   multi-adornment-free; check its pre-cut shape and slot it honestly.
