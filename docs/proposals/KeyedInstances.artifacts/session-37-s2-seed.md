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

## §1.5 Whole-program pseudocode (tip `28f2ccc0`) + S2 as DIFFS on it

```
# ================= DataFlow: Query::Build (lib/DataFlow/Build.cpp) ==========
Query::Build(module, log, policy, demand_mode, demand_retract, suppress_demand):
  parse -> build -> ApplyDemandTransform   # S1a: flag-gated head; SIP walk
  #   (kBaseAtom/kPushDown arms), mints guard JOINs + fabricated demand__
  #   message + demand relation; STEP 10 records QueryDemandForcing{query,
  #   message, bound_params}; RecognizedSubgraph (one per forcing,
  #   demanded_decl, forcing_index) + GuardAnnotation (PRE-CSE facts:
  #   kind/demand_side/role/instance_key) — THE RECOGNITION FRONT-END, LIVE.
  -> Optimize -> Stratify -> FinalizeColumnIDs -> eqsets/inductions
  impl->row_contracts   = InferConservativeRowContracts(impl)
  impl->instance_flow   = BuildFlatInstanceFlow(...)          # s32 grove
  impl->materialization = PlanResources(...)                  # s34 resources
  DeriveArrangements(Query(impl), impl->materialization)      # s36 indexes
  #   ^ replays the SIX GetOrCreateIndex sites' column logic (R-FULL/
  #     R-JOIN-UNIFORM/R-NEG/R-QUERY + R-INTERFACE count) — TODAY'S rules.

# ================= ControlFlow: Program::Build (4-arg at tip) ===============
Program::Build(frozen, log, first_id, policy):          # S2 DIFF: +5th param
  query = frozen.DataFlowGraph()
  context.demand_forcings = &query.DemandForcings()     # S1b (landed)
  FillDataModel(query, program, context)                # tables; full index each
  CrossCheckMaterialization(query, real_classes)        # s34 belt, always-on
  BuildEntryProcedure -> BuildStratumPhases:
    dr_flow = BuildDRInventory(impl, context, query, sccs)
    #   DRTables + resource stamps (s34/s35) + table_to_resource +
    #   V-REL-RESOURCE / V-REL-OP-RESOURCE + branches/joins/ops census
    #   (29 kinds; kSubgraphInstantiate/kInstanceDeath/kInstanceSeal RESERVED,
    #   flow.instances NEVER populated at tip)
    LowerDRFlow / LowerDRRounds / LowerCommitSweeps / LowerGroupUpdate
    eager walk (BuildEagerInsertionRegions) fills ingest-fold holes
  BuildIOProcedure per IO   # incl. the fabricated demand__ receive (handler)
  BuildQueryEntryPoint per surviving query-INSERT:
    BuildQueryInjectorProcedure                          # S1b: registry-first
    GetOrCreateIndex(bound subset)                       # R-QUERY site
  ProgramImpl::Optimize x2
  census: CrossCheckArrangements(query, real_index_universe, n_interface)  # s36
  return program -> C++ codegen (suppresses demand__ public ABI; emits
    Table/Index/StateCellStore members + entry procs + query friends)

# ================= Runtime stores at tip ====================================
Table<Row> + Index<Key>           # the flat world; DiffTable for differential
StateCellStore                    # per-GROUP standing cell (aggregates/KV)
# (NO InstanceStore at tip — deleted at P1.)
```

### The S2 path as DIFFS (restore-then-adapt from `6d6248a2`)

```diff
  # ---- flag plumbing -------------------------------------------------------
+ Main.cpp: gDemandInstance (-demand-instance implies -demand)
+ Program::Build(frozen, log, first_id, policy, bool demand_instance=false)
+ context.demand_instance_enabled = effective_demand_instance
+ nested PRE-PASS fences (Program::Build head): recursive demand -> clean
+   diagnostic (demand_cyclic_1: compiles flat / rejects nested)

  # ---- Rel-IR mint (the reserved scaffolding comes ALIVE) ------------------
  BuildDRInventory(...):
+   BuildSubgraphInstanceOps(flow, impl, context, query, scc_map):   # gate:
+     if !context.demand_instance_enabled: return                    # flag-off
+     lr = ResolveLiveRecognition(impl, query)
+     #  ABA-SAFE: stored RecognizedSubgraph QueryView handles DANGLE past
+     #  Optimize; everything re-resolves from LIVE guard JOINs (the
+     #  CSE-migrating GuardAnnotationIndex stamp) + parse identities.
+     for rs in query.RecognizedSubgraphs():
+       ri = lr.by_forcing[rs.forcing_index] or continue   # dead forcing skip
+       HP-4 refusal belts (input side must be plain table-bearing)
+       flow.instances.push(DRInstance{demanded_view, pub_view,
+                                      demand_table, input_table, pub_table,
+                                      differential, forcing_index})
+       mint ops: kSubgraphInstantiate (band a1: demand-arrival birth — full
+                   input rescan Present-filtered into the keyed store)
+                 [kInstanceDeath  iff differential demand]  (retract drain)
+                 [kInstanceSeal]                            (batch seal)
+       + band (a2): input net-additions frontier -> rebuild standing
+         instances (edge-after-demand); (a2') net-removals drain (S2b).
+   census recount arm: the nested kinds enter the 29-kind census counts.

  # ---- lowering + emission -------------------------------------------------
+ LowerDRFlow: dispatch the new op kinds -> region trees whose codegen
+   drives InstanceStore (Stratum.cpp ~:2326 pre-cut: per flow.instances[i],
+   a descriptor {differential, key arity, ...} feeds codegen emission)
+ eager walk: the recognized subgraph is EXCISED from the flat descent
+   (chain-breaker) + OD-4 provisioning replaces its flat provisioning
+ codegen: emit InstanceStore<Key> members + the instantiate/death/seal
+   bodies; the demanded pub reads through the store, not the flat table

  # ---- runtime --------------------------------------------------------------
+ include/drlojekyll/Runtime/InstanceStore.h (StateCellStore's transpose):
+   InstanceId FindInstanceWithHash/InsertSlot/Rehash; per-iid Table payload;
+   sealed/working occupancy bits + touched_flag; Seal(); RecycleCurrent();
+   monotone flag gates the HP-7 seal belt.  # re-verify vs StateCell drift

  # ---- THE NEW ARMS (no pre-cut precedent — §2) -----------------------------
+ DeriveStatefulClasses(query [, demand_instance]):
+   R1-R9 as today, MINUS/PLUS the nested deltas extracted in grounding
+   (which demanded-subgraph classes lose their flat TABLE, which keep it,
+   what the store replaces) — CrossCheckMaterialization stays quiescent.
+ DeriveArrangements(query, plan [, demand_instance]):
+   the nested arm's index requests (guard-join pivots persist? the store's
+   internal keying is NOT a TABLEINDEX?) — extracted per-site in grounding —
+   CrossCheckArrangements stays quiescent.
+ V-REL-OP-RESOURCE: DRInstance table fields become NON-NULL -> they enter
+   the belt walk (s35 skipped nulls); their tables must resolve via
+   table_to_resource (or the belt learns a store-resource notion).
```

Every `+` above is restore-then-adapt EXCEPT the last block (build-new) and
the census/validator arms (rebuild against the s30-s36 validator family).
The flag-off path is byte-identical BY CONSTRUCTION (every diff is behind
`demand_instance_enabled` / the 5th param default).

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
