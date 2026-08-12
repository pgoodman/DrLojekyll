# Session-32 seed — KEYED INSTANCES: whole-program pseudocode + the path-forward diffs

> Written at the close of session 31 (branch `keyed-instances`, tip after `87224857`). This is the
> START-HERE whole-program view for the keyed-instance build (the branch namesake). It records the
> session-31 decisions, re-expresses the pipeline as pseudocode, and states the path forward as DIFFS
> on that pseudocode so the next session is grounded in a whole-program view before touching code.
>
> **The next session must GO DEEPER on this** (that is its charter, `session-32-prompt.md`): build out
> the pseudocode via workflows, formulate + critique the design-goal diffs, and do the same for the IR
> desired-output-states — then execute a REAL first slice. This doc is the seed, not the final grounding.

## §0 Where we are (session-31 decisions — do NOT re-litigate)

- **S1a LANDED (s30):** flat `-demand` is live, flag-only, corpus byte-identical (SUITE 227). It is the
  RECOGNITION FRONT-END: `ApplyDemandTransform` mints the demand relation + guard-joins and records a
  `RecognizedSubgraph` per bound `#query` (bound key + demanded view + pub view + guard indices + decl).
- **DEMANDCONST: DROPPED.** It was a device to de-spook the FLAT graph (localize instance-identity as a
  CSE-carried value instead of a smeared tag). Committing to keyed instances carries instance-identity
  STRUCTURALLY (the store key), so DEMANDCONST is subsumed, not a prerequisite. It was a good thinking
  tool — its conclusion IS keyed instances.
- **Flat back-end polish (the old S1b injector/witness/bench): DEPRIORITIZED.** No more flat
  intermediates. The flat guard-web lowering stays as-is (it compiles) as an optional dev reference,
  but we invest in the keyed back-end instead.
- **Target: KEYED INSTANCES** — the nested, trie-inducing store (owner: "boldly move into the target
  feature space, stop pussyfooting with intermediate flat approaches").
- **Validation = two EXISTING goldens** (no new infra): `.stdout` (non-keyed / full materialization =
  answer-equality ground truth) + `.monotone.stdout` (the non-differential / add-only monotone
  projection, already run by `bin/Oracle`). Keyed answers must byte-equal these.
- **Vestigial `IsConditional` control-dependence analysis: REMOVED (s31, `87224857`)** — kept
  `is_condition` (the real unit-relation / condition-table marker); deleted the dead
  `IsConditional`/`IntroducesControlDependency`/`IsConstantAfterInitialization`/`TrackConstAfterInit`
  cluster (no-op consumers, #240/#242 scaffolding never built, inverted name). SUITE byte-identical.

## §1 The whole-program pipeline, as pseudocode (current tip)

```
CompileModule(module):                                    # bin/drlojekyll/Main.cpp
  query   = Query::Build(module, log, policy, demand_mode=gDemand)   # lib/DataFlow
  frozen  = FrozenRegionalProgram::Build(query, log)                 # lib/Regional
  program = Program::Build(frozen, first_id, policy)                 # lib/ControlFlow (+ lib/Rel)
  emit C++ (program)                                                 # lib/CodeGen/CPlusPlus/Database.cpp

Query::Build(module, log, policy, demand_mode, ...):      # lib/DataFlow/Build.cpp:2524
  impl = build initial dataflow graph from clauses (SELECT/TUPLE/JOIN/MERGE/CMP/MAP/NEGATE/AGG/KV/INSERT)
  impl.ConnectInsertsToSelects(proxy_view_to_decl)        # wire producers↔consumers; record proxy→decl
  impl.ApplyDemandTransform(module, demand_mode, ..., proxy_view_to_decl)   # <-- S1a, see §2
  if !policy.skip: impl.Optimize()                        # CSE + Canonicalize fixpoint + dead-flow
  impl.ConvertConstantInputsToTuples()                    # invariant: only TUPLE takes all-constants
  impl.LinkViews(); impl.IdentifyInductions()
  impl.FinalizeDepths(); impl.FinalizeColumnIDs()
  BuildEquivalenceSets(impl); impl.Stratify()             # multi-view stratum = an SCC cycle
  impl.row_contracts = InferConservativeRowContracts(impl)   # Stage-A member keys
  return Query(impl)

FrozenRegionalProgram::Build(query, log):                 # lib/Regional/Planning.cpp
  # a degenerate planner: ONE ProgramRoot + ONE region R0; a compile-time typed model beside codegen
  CollectMessages(query, received, published)             # demand__ msgs filtered out (§6a)
  BuildRequestPorts(query)                                # bound #query → request port; all-free → permanent root
  row_contracts   = R-STORE (insert-materialized decls) ∪ Tier-1 (demand interiors) ∪ Tier-2 (origin)
  recursive_comps = ComputeRecursiveComponents(query)     # P6.1 SCC projection
  rules/routing   = BuildRuleRoutingProjections + PromoteSharedSymbolicField   # P6.2
  ClassifyFusableComponents(...)                          # P6.3 fused-vs-joint (dump-only)
  V-REGION-CENSUS(query) recount                          # always-on anti-stub belt
  return frozen

Program::Build(frozen, first_id, policy):                 # lib/ControlFlow/Build/
  query = frozen.Query()                                  # H4 byte-preserving seam
  flow  = BuildDRInventory(query, context)                # lib/Rel/Rel.cpp:1363 — the typed DR flow graph
  DeriveDRStrata(flow); LinearizeAndValidateDRFlow(flow)  # monotone lift + Kahn schedule (CHECKED)
  regions = LowerDRFlow(flow) ⊕ LowerDRRounds ⊕ LowerCommitSweeps ⊕ LowerGroupUpdate   # Stratum.cpp
  eager web = BuildEagerInsertionRegions(...)             # the still-hand-coded eager descent (Build.cpp)
  program.Optimize()                                      # region flatten / dedup
  return program

ApplyDemandTransform(...):                                # lib/DataFlow/Demand.cpp  (S1a)
  if !demand_mode: return true                            # flag-off containment gate → byte-identical
  for each bound #query q:                                # single-adornment slice
    SIP walk backward from q's bound cols:
      kBaseAtom : bound col reaches a message-SELECT with no intervening JOIN  → guard the base atom
      kPushDown : bound col exits a JOIN reading p itself (IsFullWidthReaderOf) → recursive push-down guard
    fabricate demand__q message + demand relation d_p     # ParsedModule::FabricateDemand*
    mint guard-JOINs (d_p ⋈ read) gating every producer of p on the demanded key
    record RecognizedSubgraph{demanded_view, key_cols, pub_view, guard_indices, demanded_decl}
  return true
```

## §2 The recognition→lowering seam (where keyed diverges — the load-bearing fact)

`ApplyDemandTransform` records `impl.recognized_subgraphs`, but at tip **nothing LOWERS it** — the only
reader is a debug conservation belt (`Build.cpp:2676`). The flat guard-web "just works" because the
guard-JOINs are ordinary JOINs that lower through the ordinary `BuildDRInventory` path. So:

```
RecognizedSubgraph  --(today)-->  [debug assert only]          # flat lowering ignores it
                    --(S2 KEYED)-->  BuildDRInventory: mint kSubgraphInstantiate per RecognizedSubgraph
```

**The keyed lowering is a NEW ARM in `BuildDRInventory`/`Stratum.cpp` that consumes `RecognizedSubgraphs`
and emits a keyed InstanceStore instead of leaving the demanded relation to materialize flat.** The
recognition front-end (S1a) is unchanged; the demand graph's flat guard-web either (a) is bypassed for
recognized subgraphs, or (b) stays as the dev reference while the keyed arm is validated beside it.

## §3 The path forward, as DIFFS on the pseudocode

### Slice 1 — keyed-MONOTONE (birth only; no death, no retraction)
```diff
 Program::Build(frozen, ...):
   query = frozen.Query()
   flow  = BuildDRInventory(query, context)
+  # NEW keyed arm (monotone): one instance per demanded key, keyed by the bound tuple.
+  for rs in query.RecognizedSubgraphs():
+    inst = DRInstance(rs.demanded_view, rs.pub_view)          # reserved scaffold, lib/Rel/Rel.h:870
+    inst.demand_table = model_table_of(rs.demand relation)
+    flow.instances.push(inst)
+    flow.ops.push(DROp{kind=kSubgraphInstantiate, inst, key=rs.key_cols})  # BIRTH/REBUILD, band-(a)
+    # NO kInstanceDeath in the monotone slice.
   DeriveDRStrata(flow); LinearizeAndValidateDRFlow(flow)
   regions = LowerDRFlow(flow) ⊕ ...
+  # NEW: LowerSubgraphInstance(op) emits against a keyed store (StateCellStore-shaped, keyed by tuple)
   ...
```
Runtime: reuse the `StateCellStore` shape (`include/drlojekyll/Runtime/StateCell.h`) — dense keyed
slots, occupancy, monotone (never compacts). One slot per demanded key; the slot holds that key's
demanded sub-database (its trie level).
**Gate:** keyed answers byte-equal `.monotone.stdout` (add-only oracle) AND `.stdout` on monotone
programs. Codegen goldens MOVE (a keyed store replaces the flat guard-web tables). Witness precedent:
the deleted `demand_neighborhood_mono_witness` (bare `-demand`, R-MONO nested lowering).

### Slice 2 — keyed-DIFFERENTIAL (add death + retraction)
```diff
   for rs in query.RecognizedSubgraphs():
     ...
     flow.ops.push(DROp{kind=kSubgraphInstantiate, inst, key=rs.key_cols})
+    if TableIsDifferential(rs.pub) or demand can retract:
+      flow.ops.push(DROp{kind=kInstanceDeath, inst, key=rs.key_cols})   # whole-instance DEATH
+      # death MUST precede a re-instantiate on the same key (V-INST-DRAIN, §18(B))
```
Runtime: the `StateCellStore` sealed/working two-word cell already models the differential snapshot;
sub-database retraction = overdelete→rederive at the instance level. Re-arm the deleted V-INST-*
validators (`CheckInstanceDeathFrontier`, `CheckInstantiateEffects`).
**Gate:** keyed answers byte-equal the full differential `.stdout`.

## §4 The assets (this is a RE-INTEGRATION, not greenfield — like S1a was)

- **Runtime store:** `include/drlojekyll/Runtime/StateCell.h` — `StateCellStore` keyed-per-group store
  (dense ids, occupancy, sealed/working cell, non-aliasing compaction story). Adapt: key by bound tuple.
- **Rel-IR ops (reserved, present):** `kSubgraphInstantiate` (birth/rebuild), `kInstanceDeath`, the
  `DRInstance` descriptor (`demand_table` + demanded/pub views) — `lib/Rel/Rel.h:855-903`. Un-reserve.
- **Deleted validators to re-arm:** `CheckInstanceDeathFrontier`, `CheckInstantiateEffects`, V-INST-*.
- **Recognition front-end:** S1a `Query::RecognizedSubgraphs()` (bound key + demanded/pub views).
- **Surviving design docs (blueprint intact; only code was deleted):**
  `docs/proposals/DemandSeeds.artifacts/d3-instance-store-target.md` (the InstanceStore paper-first
  spec: store §1-§3, witness §4, census/validators §5), `docs/proposals/InstanceFlow.md` (the deeper
  vision — a NEW IR of context-parameterized computation families; see §6), and the
  `docs/proposals/KeyedInstances.artifacts/` D1/D2/D3 design set.
- **Emission entry:** `BuildDRInventory` (`lib/Rel/Rel.cpp:1363`); lowering in `lib/Rel/Stratum.cpp`.

## §5 The validation strategy (both oracles already exist)

- **Answer-equality:** keyed `.stdout` == non-keyed `.stdout` on the demanded subset (full materialization
  is ground truth). Enforced per the precedented `.eqgate`/`diffrun` pattern.
- **Monotone cross-check:** keyed `.monotone.stdout` == the existing monotone projection (add-only).
- **Disjoint referees unchanged:** RefInterp / oracle / behavioral are demand-blind and referee the
  definitional closure; the keyed binary is refereed by its own blessed `.stdout` + 4-mode + human bridge.
- **Flag-off corpus stays byte-identical** (keyed is `-demand`-gated via `.drflags`, orthogonal to the 4
  modes). The keyed-monotone/differential witnesses are NEW cases; no existing golden moves except the
  demand carriers'.

## §6 The one real fork the next session must settle first

**InstanceStore (pragmatic) vs InstanceFlow (ambitious):**
- **InstanceStore** (`d3-instance-store-target.md`): a keyed store back-end bolted onto the EXISTING Rel
  pipeline (the §3 diffs above). Smaller, re-integration, reuses `StateCellStore` + reserved ops. This
  is the tractable path and what §3 sketches.
- **InstanceFlow** (`InstanceFlow.md`): a NEW IR between the optimized Query and Rel — "a deterministic
  grove of context-parameterized computation families built backward from consumer uses," each family
  binding a key once, removing it from residual rows, exposing push/probe/standing-interest. This is
  literally the "subgraphs of computation that induce trie-like structures via nested databases" vision.
  Bigger, more greenfield-coherent, the real destination.
- **Recommendation to the owner:** build InstanceStore-monotone FIRST (real, gated, reuses assets) as the
  concrete beachhead, and treat InstanceFlow as the architecture it generalizes into once one keyed
  lowering works end-to-end. Do NOT start with the new-IR rewrite cold. But the next session should
  re-read both and put the fork explicitly to the owner with a recommendation.

## §7 Anchors (re-verify at next tip — the pipeline drifts)

- Recognition: `lib/DataFlow/Demand.cpp` (`ApplyDemandTransform`; SIP arms kBaseAtom/kPushDown);
  `Query::RecognizedSubgraphs()`/`DemandForcings()` in `include/drlojekyll/DataFlow/Query.h`.
- Seam: `RecognizedSubgraph` consumed only at `lib/DataFlow/Build.cpp:2676` (debug belt) — the keyed hook.
- Rel-IR: `BuildDRInventory` `lib/Rel/Rel.cpp:1363`; reserved ops/`DRInstance` `lib/Rel/Rel.h:855-903`;
  lowering `lib/Rel/Stratum.cpp`.
- Runtime store: `include/drlojekyll/Runtime/StateCell.h`.
- Goldens: `tests/OptDiff/goldens/*.monotone.stdout` (add-only oracle) + `*.stdout` (full).
- Surviving design: `d3-instance-store-target.md`, `InstanceFlow.md`, `KeyedInstances.artifacts/`.
- Prior seeds SUPERSEDED by this: `session-31-s1b-seed.md` (flat back-end polish — deprioritized).
```
