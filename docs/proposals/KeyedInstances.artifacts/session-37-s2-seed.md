<!-- Copyright 2026, Peter Goodman. All rights reserved. -->
# Session-37 seed — S2: the keyed InstanceStore nested lowering (`-demand-instance` rebuilt)

> START-HERE for session 37 (owner call at s36 close: "do S2 next session").
> Written at the s36 close (branch `keyed-instances`, tip `28f2ccc0`). The S1
> arc is COMPLETE this session: S1a (flat `-demand` compiles, landed s30),
> S1b (the injector — `-demand` programs ANSWER-CORRECT ×4 modes vs four
> independent referees; the pruning MEASURED in-compiler and in-harness:
> selective `idx_hops` 580,080→1,192 ≈ 487×, non-selective 4.7× regression ⇒
> cost-gated; BASELINE.md run 12), and S1c (the ADJ-2 `region-internal` line
> restored; Tier-1 naming proven subsumed by K5 Tier-2 via pre-cut golden
> byte-diff). Gates green: OptDiff **SUITE PASS (228)**, ctest **5/5**,
> flag-off corpus byte-identical throughout.

## §0 What S2 IS (the namesake feature's second half)

`-demand-instance` (implies `-demand`): lower a RECOGNIZED demanded subgraph
to a KEYED InstanceStore — standing per-key nested-relation state, one
instance per demanded key — instead of the flat guard web. ANSWER-IDENTICAL
to flat `-demand` by construction; the eqgate mechanism proves it
(flat==nested==golden, byte-compared per mode). Deleted WHOLE at the P1 cut;
S1a deliberately did NOT restore it ("STILL REMOVED S2+"). The runtime store
concept survives as `StateCellStore`'s transpose (pre-cut
`Runtime/InstanceStore.h` header comment names exactly that relationship).

## §1 Inventory at tip `28f2ccc0` (verified this session)

SURVIVES (the reintegration assets):
- **The recognition front-end is LIVE**: S1a's restored `Demand.cpp`
  populates `RecognizedSubgraph` (one per forcing, `demanded_decl`,
  `forcing_index`) + `GuardAnnotation` (kind/demand_side/role/instance_key,
  PRE-CSE recorded facts) + `Query::DemandForcings()`. The S1b injector
  (`BuildQueryInjectorFromRegistry`) seeds any lowering.
- **`lib/Rel/Rel.h` scaffolding RESERVED**: `DRInstance` (:868-901, one per
  RecognizedSubgraph, `flow.instances` — never populated at tip),
  `DROp::demand_table`/`input_table` + the
  `kSubgraphInstantiate`/`kInstanceDeath`/`kInstanceSeal` op-kind family
  (:702-708) — s35 verified all assignment-site-free (the belt skips nulls).
- **`run_eqgate` SURVIVES in runall.sh** (:522): a `.eqgate` sidecar
  re-compiles the case with the sidecar's flags + the SAME driver ×4 modes
  and byte-compares each stdout against the case's own blessed golden. Zero
  `.eqgate` cases at tip — the mechanism is armed, witness-less.
- **The design corpus** (`KeyedInstances.artifacts/`, 71 files): d1/d2a/d2b/
  d2c designs + desired-states (the recognition/lowering/eqgate designs),
  d3a0-d3a3 (differential regimes + multi-adornment, per-band designs),
  `d3-instance-store-target.md`, `rel-arch-pseudocode.md`, `rfinal-*`.
- **The s32 grounding** (`RegionalDataFlowCore.artifacts/
  session-32-keyed-instances-seed.md` + `session-32-keyed-monotone-grounding.md`
  + its `.artifacts/` dir): the whole-program pseudocode + the keyed-MONOTONE
  first-slice grounding done BEFORE the owner pivoted s32 to InstanceFlow —
  re-verify against tip (five sessions of drift) but the shape work exists.
- **`demand_tc_witness`** (S1b): the flat-arm recursive-TC witness with 13
  goldens — the recursive shape FENCES under `-demand-instance`
  (`demand_cyclic_1`'s pre-cut reject), so S2's witnesses are the
  NEIGHBORHOOD shapes, not this one.

DELETED at P1 (restoration source = `6d6248a2`, the S1a-proven
last-before-deletion tip — NOT `48cd0a4f`):
- `include/drlojekyll/Runtime/InstanceStore.h` (the standing per-instance
  nested store, StateCellStore's transpose).
- `Program::Build`'s 5th param `bool demand_instance = false`
  (pre-cut `Program.h:1463`; tip is 4-arg) + Main.cpp `gDemandInstance` +
  `context.demand_instance_enabled` + the nested pre-pass fences (recursive
  demand reject) + `BuildSubgraphInstanceOps` (the DR-IR mint) + the census
  recount arm + the eager-walk chain-breaker excision + OD-4 provisioning +
  the band lowerings (a1 birth / a2 edge-adds rescan / a2' edge-removals
  drain / kInstanceDeath) + codegen's InstanceStore emission.
- The witness corpus: `demand_neighborhood_witness` (R-DIFF flagship,
  `-demand -demand-retract` + `.eqgate` `-demand-instance`),
  `demand_neighborhood_mono_witness` (R-MONO, bare `-demand`),
  `demand_diff_neighborhood_witness` (e5: diff-input × mono-demand,
  kInstanceDeath=0 beside kSubgraphInstantiate=1),
  `demand_diff_input_1`, `demand_multi_adorn_witness`
  (kSubgraphInstantiate=2 — two stores, one pub), and the fence diagnostics
  (`demand_cyclic_1` compiles flat / rejects nested;
  `demand_recursive_content_1`; `demand_agg_body_1` etc.).

## §2 THE NEW INTEGRATION OBLIGATION (did not exist pre-cut — name it FIRST)

The pre-cut nested lowering predates the s34–s36 derived-authority belts.
Under `-demand-instance` the PHYSICAL ALLOCATION CHANGES (a demanded
subgraph's tables/indexes are replaced/augmented by keyed instance state),
and FOUR always-on cross-checks now encode TODAY'S allocation:

1. `CrossCheckMaterialization` (s34): `DeriveStatefulClasses`' R1-R9 replay
   == the real table-backed class set. A nested lowering that changes which
   classes get TABLEs FIRES it.
2. `CrossCheckArrangements` (s36): the R-FULL/R-JOIN-UNIFORM/R-NEG/R-QUERY
   replay == the real index universe. Nested-arm index deltas FIRE it.
3. `V-REL-OP-RESOURCE` / `V-REL-RESOURCE` (s35): every op base-table field
   resolves through `table_to_resource`; `DRInstance`'s `demand_table`/
   `input_table`/`pub_table` fields, once POPULATED, enter the belt's walk
   (s35 deliberately skipped them as always-null).
4. `V-REGION-CENSUS` + V-INGEST-XCHECK + the eager-web census: the pre-cut
   nested arm had its OWN census recount arm — it must be rebuilt against
   the CURRENT (much larger) validator family.

THE RULE (non-negotiable, the belts exist to catch exactly this): teach the
DERIVATIONS the nested rules — a `demand_instance` arm in
`DeriveStatefulClasses`/`DeriveArrangements` replaying the nested
FillDataModel/GetOrCreateIndex deltas — never gate a belt off. This is ALSO
the honest InstanceFlow convergence: the plan learning "which storage does a
demanded subgraph need under the nested lowering" is precisely the §10
material the parked Stage C consumes later. Ground this integration FIRST —
it is the one part of S2 with no pre-cut precedent.

## §3 Slicing (mirror the pre-cut D-sequence; each slice its own gate)

- **S2a (the one-session target): R-MONO nested end-to-end.** Restore:
  `Runtime/InstanceStore.h`, the `-demand-instance` flag + 5-arg
  `Program::Build` + fences, `BuildSubgraphInstanceOps` + `flow.instances`
  population + band-(a1) birth + band-(a2) edge-after-demand rescan +
  codegen emission — adapted to five sessions of drift (Rel op-model growth,
  the s35 resource stamps, the s36 belts per §2). Witness:
  `demand_neighborhood_mono_witness` restored from `6d6248a2` (bare
  `-demand` flat golden + `.eqgate` `-demand-instance`) — the eqgate
  (flat==nested==golden ×4 modes) is the answer gate. Fences restored as
  all-4-modes diagnostics (`demand_cyclic_1` MODE-SPLIT: flat-compiles /
  nested-rejects).
- **S2b: the differential regimes** (d3a0-d3a2): `-demand-retract` (instance
  death/rebirth, kInstanceDeath), diff-input (band-(a2') removals drain, the
  P-STORE∧¬P-DEATH e5 carrier), the four eqgate family members.
- **S2c: multi-adornment** (d3a3): N disjoint stores over ONE shared pub
  (kSubgraphInstantiate=2), R-DUP union, V-INST-SOLE re-key.

## §4 Anchors (re-verify at tip — five sessions of drift since 6d6248a2)

| Fact | Source |
|---|---|
| Restoration tip | `6d6248a2` (S1a-proven: tip-K1 API + Mint tags) |
| Pre-cut InstanceStore | `git show 6d6248a2:include/drlojekyll/Runtime/InstanceStore.h` |
| Pre-cut 5-arg Build | `6d6248a2:include/drlojekyll/ControlFlow/Program.h:1463` |
| Rel.h reserved scaffolding | `lib/Rel/Rel.h:702-708` (op fields/kinds), `:868-901` (`DRInstance`, `flow.instances`) |
| Recognition front-end (live) | `lib/DataFlow/Demand.cpp` (`RecognizedSubgraph`, `GuardAnnotation`, STEP 10 forcings) |
| Injector (live, S1b) | `lib/ControlFlow/Build/Build.cpp` `BuildQueryInjectorFromRegistry` |
| run_eqgate (live, witness-less) | `tests/OptDiff/runall.sh:522` |
| s32 keyed-monotone grounding | `RegionalDataFlowCore.artifacts/session-32-keyed-monotone-grounding.md` (+ `.artifacts/`), `session-32-keyed-instances-seed.md` |
| Design suite | `KeyedInstances.artifacts/` d1/d2*/d3a* + `d3-instance-store-target.md` + `rel-arch-pseudocode.md` |
| The s34-s36 belts (§2) | `lib/DataFlow/Materialization.{h,cpp}` (both cross-checks), `lib/Rel/Rel.cpp` (V-REL-*), CLAUDE.md InstanceFlow § |
| Pre-cut witnesses | `git show 6d6248a2:tests/OptDiff/cases/demand_neighborhood_mono_witness.*` (+ diff/multi-adorn twins, fences) |
| Cost context | `-demand-instance` inherits the flat cost-gate (BASELINE run 12: selective ~487× / non-selective 4.7× regression); nested changes the CONSTANT, not the gate |

## §5 Open questions for grounding (name, don't assume)

1. **§2 belts integration** — the derivation arms' exact rules (what does
   the nested FillDataModel/index minting actually allocate at `6d6248a2`?
   Extract per-site, THEN design the `demand_instance` arms).
2. **InstanceStore vs StateCellStore drift**: has `StateCell.h` changed
   since `6d6248a2` in ways the transposed store must mirror (config-agg
   SealOne landed post-fork)?
3. **The s32 fork echo**: s32 grounded keyed-monotone as a BUILD-NEW; S1a's
   lesson was RESTORE-THEN-ADAPT (byte-faithful from `6d6248a2`, then fix
   drift). Recommend restore-then-adapt (it worked twice: S1a, S1b), with
   the s32 grounding as the review lens — but let the grounding panel rule.
4. **Eqgate goldens under drift**: the pre-cut witnesses' flat goldens must
   be RE-BLESSED against tip (S1b precedent: 13 goldens re-blessed clean);
   never copy pre-cut golden bytes.
5. **`-region-out` under nested**: S1c's `region-internal` line + the
   Tier-1-subsumption argument were verified on the FLAT arm; re-verify the
   nested arm's dump surface (the pre-cut region goldens for the nested
   witnesses are the reference).
