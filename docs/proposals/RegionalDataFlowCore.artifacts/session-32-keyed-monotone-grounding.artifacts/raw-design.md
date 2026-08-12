All anchors verified against tip. Synthesizing the design.

---

# Keyed-Monotone Slice — Whole-Program Design (current-tip anchored)

**Branch** `keyed-instances`. **Scope**: slice 1 = birth + edge-after-demand rebuild + publish + seal, MONOTONE only (no `kInstanceDeath`, no retraction, no drop-scan, no counters/queues). Restore the deleted keyed-instance lowering so a bound `#query` lowers to a keyed sub-database (`InstanceStore`) instead of the flat guard-web.

**Verified tip facts** (grep/sed this session, not from extracts):
- `DROpKind` at tip ends `…kStateSeal (145), kEagerForward (18-comment, 148) … kProductEmit (28, 244)` — the three Instance tags are **gone**; only the dangling comment at `Rel.h:690` survives. Must re-add.
- Mint slot: `Rel.cpp:1605` KVIndices loop → `BuildGroupUpdateOps` at `:1615` → `:1620` `// ---- branches/joins`. **Insert the keyed arm between :1618 and :1620.**
- Surviving scaffolding (all confirmed present, unused): `DROp::{demanded_view(695), demand_table(696), input_table(697), instance_store_id(698), forcing_index(699), context_cols, context_col_sources}`; `class DRInstance` (`Rel.h:856-882`, full field set incl. `differential`, `key_cols`, `row_cols`, ctor `(demanded_view, pub_view)`); `DRFlowGraph::instances (889)`, `::instance_stratum (890)`, `::drain_stratum (928)`, `::OpsOfKind (936)`.
- `DROpStratum(const DRFlowGraph&, const DROp&)` = `Rel.cpp:~3964`; `kGroupUpdate` arm reads `flow.group_update_stratum` else `return 0u`; `kStateSeal` → `return 0u`. **No instance arms** — must add.
- DR pipeline driver = `lib/ControlFlow/Build/Stratum.cpp:2149-2189`: `BuildDRInventory (2149)` → `DeriveDRStrata (2156)` → JOIN_EMIT delta enroll (2170) → `ValidateDRInventory/ValidateDROps/LinearizeAndValidateDRFlow (2185-2189)`.
- `Program::Build` = **4-arg** `(frozen, log, first_id, policy)` (`Program.h:1379`). `Main.cpp`: `gDemand (50)`, `Query::Build(module, log, policy, gDemand) (74)`, `Program::Build(*frozen, log, gFirstId, policy) (111)`. **No `gDemandInstance`, no `-demand-instance` flag** at tip.
- **Load-bearing caveat (E1, empirically confirmed)**: `RecognizedSubgraph::{demanded_view,pub_view}` and every `GuardAnnotation` `QueryView` DANGLE after `Optimize`. `BuildDRInventory` runs post-`Optimize`, so the mint **must not** deref them — it re-resolves live off the CSE-migrating `GuardAnnotationIndex` stamp (`ResolveLiveRecognition`). Only `forcing_index`, `key_cols`, `demanded_decl` are read off the stored struct.

---

## 1. BUILT-OUT PSEUDOCODE

Seed §1 pipeline, expanded for exactly the stages keyed instances touch. `→` = data flow; `[NEW]` = re-authored; `[SCAF]` = surviving-but-unused scaffolding this reuses.

```
=== STAGE 0: Query::Build (lib/DataFlow, POST-Optimize) — RECOGNITION (already live, S1a) ===
Build.cpp:2601  ApplyDemandTransform(impl)              // mints guard JOINs + demand__ msg
  → per bound #query, appends to impl->recognized_subgraphs (Query.h:1214):
      RecognizedSubgraph{ forcing_index, demanded_view*, key_cols, pub_view*,
                          guard_annotation_indices, demanded_decl }
      *dangle after Optimize — ONLY forcing_index/key_cols/demanded_decl durable
  → per bound #query, appends to impl->demand_forcings:
      QueryDemandForcing{ query:ParsedQuery, message:ParsedMessage, bound_params }
  → guard JOINs stamped QueryViewImpl::guard_annotation_index (View.cpp:724 migrates on CSE)
Build.cpp:2622  impl->Optimize()                        // CSE folds guard sites; VIEW* keys dangle
Accessors: Query::RecognizedSubgraphs()/DemandForcings()/GuardAnnotations() (Demand.cpp:298-311)

=== STAGE 1: FrozenRegionalProgram::Build (lib/Regional) ===
  Names the demanded-interior relation as a Tier-1 row-contract (already works).
  V-REGION-CENSUS unaffected — keyed mint adds no region.  (No keyed code here.)

=== STAGE 2: Program::Build (lib/ControlFlow) → the DR pipeline (Stratum.cpp:2149) ===
  gate: context.demand_instance_enabled  [NEW field on Context]

  (2149) DRFlowGraph dr_flow = BuildDRInventory(impl, context, query, recursive_sccs)
     ... crossovers, products, GROUP_UPDATE (KVIndices at :1615) ...
     [NEW ARM @ Rel.cpp:1618]  if (context.demand_instance_enabled)
                                 BuildSubgraphInstanceOps(flow, impl, context, query, scc_map);
       │
       ├─ [NEW] LiveRecognition lr = ResolveLiveRecognition(impl, query)   // ABA-safe
       │     re-buckets LIVE guard JOINs by GuardAnnotationIndex → forcing_index;
       │     per forcing resolves: demand_table (joined[0] model),
       │       input_table (kBody guard joined[1] model), input_key_cols (annot.instance_key),
       │       pub_table (live INSERT whose Declaration().Id() == forcing.query decl Id),
       │       + live demanded_view/pub_view/input_view HANDLES (for descriptor render only)
       │
       └─ for rs in query.RecognizedSubgraphs():
            ri = lr.by_forcing[rs.forcing_index];  if (!ri.ok) continue;   // fully-dead forcing
            HP-4 refusal belt: abort if ri.input_view is Map/Negate/Agg/KVIndex
            diff = TableIsDifferential(ri.pub_table)   // slice-1 INVARIANT: always false
            sid  = flow.instances.size()                                        [SCAF]
            flow.instances.push_back(DRInstance{demanded_view, pub_view}         [SCAF Rel.h:856]
                 .differential=false, .pub_table, .demand_table, .input_table,
                 .key_cols=rs.key_cols, .row_cols=(pub positions ∉ key_cols),
                 .forcing_name=forcings[fi].query.NameAsString())
            DROp inst(kSubgraphInstantiate)                                     [NEW enum tag]
                 .ctx=kSeed .table_op_table=pub_table .table_op_sign=+1  (HP-3: no new pub field)
                 .demand_table .input_table .demanded_view=*ri.demanded_view
                 .instance_store_id=sid
                 .effects = InstantiateEffects_MONO(pub,demand,input)   // §2a mono arm only
                 .context_cols/.context_col_sources = per pub pos → kInstanceKeySlot|kRowSlot
                 .arms = [ DRArm(+1, PlanNode kAccess kSectionWalk over input_table
                                       bound=ri.input_key_cols → PlanNode kFold pub +1) ]
            flow.ops.push_back(inst)
            // NO kInstanceDeath (mono demand)                                  [slice-2]
            DROp seal(kInstanceSeal) .table_op_table=pub_table .instance_store_id=sid
                 .effects=[SealEffect(pub)]; flow.ops.push_back(seal)           [NEW enum tag]
            flow.instance_stratum[sid] = 1 + max(drain_stratum(demand),         [SCAF Rel.h:890]
                                                  drain_stratum(input))
            // NB: at mint time inside BuildDRInventory, drain_stratum is EMPTY  ⚠ see §2b + Risk R1

  (2156) DeriveDRStrata(dr_flow, ...)      // populates drain_stratum etc. — does NOT touch instance_stratum
  (2185) ValidateDRInventory / ValidateDROps / LinearizeAndValidateDRFlow
       │  DROpStratum keyed arm [NEW]: kSubgraphInstantiate/kInstanceDeath →
       │     flow.instance_stratum.at(sid) else ValidatorFail (loud, unlike kGroupUpdate's 0u);
       │     kInstanceSeal → 0u (trailing band, kStateSeal twin)
       │  Census recount [NEW]: exp_instance from ResolveLiveRecognition (NOT flow.instances.size)
       │  V-INST-EFFECT/SOLE/PAIR(mono)/DRAIN [NEW]  (§2f)

=== STAGE 3: ControlFlow lowering — LowerSubgraphInstances (Procedure.cpp) ===
  Procedure.cpp ~326, inside `if (context.dr_flow) {`, BEFORE LowerCommitSweeps:
    [NEW] LowerSubgraphInstances(impl, context, *context.dr_flow, seq)
       for op in dr_flow.SubgraphInstances():          [NEW accessor = OpsOfKind(kSubgraphInstantiate)]
         inst = dr_flow.instances[op.instance_store_id]
         V-INST-DIFF-COHERENCE belt (vacuous mono)
         demand_front = TableDeltaVector(demand_table, kNetAdditions)     // band-a1 source
         input_front  = TableDeltaVector(input_table,  kNetAdditions)     // band-a2 source
         SUBGRAPHINSTANCE si = operation_regions.CreateDerived<SUBGRAPHINSTANCE>(seq, sid, /*diff=*/false)
         si->demand_frontier / input_frontier / input_table / pub_table  (Emplace)
         si->key_positions=inst.key_cols; row_positions=inst.row_cols
         si->input_key_cols=op.arms[0].body->bound_cols;  input_row_cols=(input arity ∉ input_key_cols)
         context.emitted_instance_ops += {sid,kSubgraphInstantiate},{sid,kInstanceSeal}   [NEW field]
    [NEW] V-INST-EMITTED: emitted_instance_ops multiset == enrolled (op.kind,sid) multiset
    LowerCommitSweeps(...)  (unchanged)
  Stratum.cpp (after state_cells descriptor loop):
    [NEW] impl->instance_stores build loop:  per DRInstance → ProgramInstanceStore
          {key_types = pub_cols[key_cols].Type(), row_types = pub_cols[row_cols].Type(), differential=false}
  Procedure.cpp ClassifyVector: [NEW] case kSubgraphInstance → mark demand/input frontier vecs used

=== STAGE 4: C++ codegen — EmitSubgraphInstance (Database.cpp) ===
  header:  #include <drlojekyll/Runtime/InstanceStore.h>   [RESTORE FILE verbatim, E5 verdict]
  per store: emit Key_<id> hash struct (key_types), Row_<id> hash struct (row_types)
  Database struct member:  InstanceStore<Key_<id>, Row_<id>> instance_<id>;   (ctor: allocator)
  threaded as ref-param into procs that touch it (mirror StateCell :883-932)
  region emitter (mono collapse of pre-cut §7, ~120-150 lines):
    band-a1:  for k in demand_front: iid=FindOrAddInstance(Key{k});
                if(!TouchedFlag(iid)) emit_instance_rescan(k)
    band-a2:  for e in input_front: iid=FindInstance(Key{e[in_key]});
                if(iid!=kNoInstance && !TouchedFlag(iid)) emit_instance_rescan(e[in_key])
    emit_instance_rescan(keyexprs): assert !WorkingOccupied(iid); cur=TouchCurrent(iid);
        for s in input.NumRows(): ir=input.RowAt(s);
          if(ir.<in_key>==keyexprs...) cur.TryAdd(Row{ir.<in_row>...})   // NO Present() conjunct (mono)
    band-b:   for iid in Touched(): cur=Current(iid); frz=Frozen(iid); key=KeyAt(iid);
        for r in cur.NumRows(): row=cur.RowAt(r);
          if(frz.Find(row)==kNoRow) pub_member.TryAdd(pub_exprs(key,row))   // [+ EmitIndexAdds if indexed]
    seal:     instance_<id>.Seal();  #ifndef NDEBUG instance_<id>.DebugValidate(); #endif
```

---

## 2. KEYED-MONOTONE DIFF (against tip)

### (a) `BuildDRInventory` keyed mint arm — `lib/Rel/Rel.cpp`

**Prereq (do first)**: `lib/Rel/Rel.h` — re-add the 3 enumerators at their historical slot, **immediately after `kStateSeal` (145), immediately before `kEagerForward` (148)** so `kEagerForward`'s `(18)` comment stays truthful:

```cpp
  kStateSeal,            // (…) R3 ...
  kSubgraphInstantiate,  // (15) D2.b keyed instance: birth/rebuild + publish
  kInstanceDeath,        // (16) D2.b keyed instance: DIFFERENTIAL-ONLY (slice 2; declared, unminted)
  kInstanceSeal,         // (17) D2.b keyed instance: frozen↔current swap (trailing band)
  kEagerForward,         // (18) R1: ...
```
Declaring all three now (even `kInstanceDeath`, never minted in slice 1) keeps the enum stable for slice 2 and lets every exhaustive `switch` add its arm once. Payload fields (`demanded_view`/`demand_table`/`input_table`/`instance_store_id`/`context_cols`/`context_col_sources`) already exist (`Rel.h:695-707`).

**New static helpers** (hand-picked from E4, re-targeted). All symbols below are dead at tip and re-authored verbatim except the noted mono restriction:

- `ResolveLiveRecognition` + `struct ResolvedInstance`/`LiveRecognition` — restore **entire** (E4 §1). Pure graph-walk, no differential content. Re-target check: `GuardAnnotation::kBody` and `GuardAnnotation::instance_key` — verify field names against `Query.h:1001` before wiring (E4 flagged unverified; struct exists).
- `InstantiateEffects_MONO(pub,demand,input)` — E4 §2a **`else`/`!diff` arm only**: `{ kVecDrain(demand,kNetAddition), kVecDrain(input,kNetAddition), kInstanceDemand(demand), kFlagRead(input,kPresent,kSeed), kInstanceRebuild(pub,+1), kStateEmit(pub), kStateOld(pub), kCounter(pub,+1,kNonRecursive) }`. **Drop** the `if(input_diff)` removal-drain, drop the whole `if(diff)` two-sign counter/crossing/append block.
- `SealEffect(pub)` — restore verbatim (E4 §2b): single `kStateFold(pub, sign=0)`.
- `BuildSubgraphInstanceOps` — restore E4 §2c **minus** the `if (demand_table && TableIsDifferential(demand_table))` `kInstanceDeath` mint block. Keep the HP-4 refusal belt, the `DRInstance` push, the instantiate DROp, the `context_cols` α-tagging, the single `DRArm` rescan spine, the `kInstanceSeal` DROp, and the `flow.instance_stratum[sid]` self-seed. `DeathEffects` is not restored.

**Insertion at `Rel.cpp:1618`** (between KVIndices loop close and `// ---- branches/joins`):
```cpp
  // ------------------------------------------------------- keyed instances
  if (context.demand_instance_enabled) {
    BuildSubgraphInstanceOps(flow, impl, context, query, scc_map);
  }
```

### (b) `DeriveDRStrata` / Linearize enrollment — `lib/Rel/Rel.cpp`

The keyed family is **self-seeded, not `DeriveDRStrata`-enrolled** (unlike `group_update_stratum`). Two edits:

1. **`DROpStratum` (`Rel.cpp:~3964`)** — add three arms alongside `kGroupUpdate`/`kStateSeal`:
```cpp
  case DROpKind::kSubgraphInstantiate:
  case DROpKind::kInstanceDeath:
    if (auto it = flow.instance_stratum.find(op.instance_store_id);
        it != flow.instance_stratum.end()) { return it->second; }
    ValidatorFail("DROpStratum: instance op has no instance_stratum entry");  // loud, deliberate
  case DROpKind::kInstanceSeal:
    return 0u;  // trailing commit band (kStateSeal twin; V-READY-exempt)
```
The **loud fail on miss** (vs `kGroupUpdate`'s silent `0u`) is the E3-mandated strengthening — a missing entry is always a mint bug for this family.

2. **V-READY exemption** — wherever `kStateSeal`/`kCommitSweep` are exempted from the V-READY "must have a producer" check in `LinearizeAndValidateDRFlow`, add `kInstanceSeal` to the same skip set.

⚠ **`instance_stratum` timing (Risk R1)**: `BuildSubgraphInstanceOps` runs inside `BuildDRInventory` where `flow.drain_stratum` is still **empty** (`DeriveDRStrata` runs at `Stratum.cpp:2156`, after). So `ready_after()` returns 0 for both tables → every `instance_stratum[sid] = 1`. For the acyclic monotone slice this is correct (demand/input drains sit at base stratum 0, instances at 1). **Decision**: keep the self-seed at mint (faithful to pre-cut) for slice 1, and add a `DEBUG`-only assert that no `instance_stratum` value under-orders its demand/input drain *after* `DeriveDRStrata`. If a demand/input table ever drains at stratum ≥1 (never in the mono acyclic corpus), promote the self-seed to a post-`DeriveDRStrata` step mirroring the JOIN_EMIT delta enrollment at `Stratum.cpp:2170`.

### (c) `LowerSubgraphInstance` region emission — `lib/ControlFlow/Build/Procedure.cpp` + `Program.h`

**`Program.h` type re-adds** (E4 §6): `ProgramOperation::kSubgraphInstance` (after `kGroupUpdate`, `Program.h:441`); `class ProgramSubgraphInstanceRegionImpl` / `using SUBGRAPHINSTANCE` — **mono field subset only**: keep `demand_frontier`, `input_frontier`, `input_table`, `pub_table`, `key_positions`, `row_positions`, `input_key_cols`, `input_row_cols`, `store_id`, `const bool differential`. Keep the differential `UseRef`s (`removal_frontier`/`del_queue`/`add_queue`/`demand_table`/`input_removal_frontier`) **declared but unset** (null in slice 1) so slice 2 needs no header churn. Add `struct ProgramInstanceStore {id, key_types, row_types, differential}`, `ProgramImpl::instance_stores`, `ProgramVisitor::AsSubgraphInstance`, forward-decl.

**`Procedure.cpp`**: restore `LowerSubgraphInstances` (E4 §4) **mono-collapsed**: drop `death_by_sid` and its wiring, drop the `if(inst.differential){…del/add queue…}` block, drop the `input_removal_front`/`input_removal_frontier`, drop the death-lookup block. Keep: `SubgraphInstances()` iteration, the `V-INST-DIFF-COHERENCE` belt (vacuous), `demand_front`/`input_front` fetch, the `SUBGRAPHINSTANCE` mint + Emplaces, the key/row partition, `input_key_cols` from `op.arms[0].body->bound_cols`, the arity-coherence belt, and the `emitted_instance_ops` enrollment (instantiate + seal, no death). Call site + `V-INST-EMITTED` cross-check at `Procedure.cpp:~326`, before `LowerCommitSweeps`. Add `ClassifyVector` `case kSubgraphInstance` (mark demand/input frontier vecs used).

**`lib/ControlFlow/Build/Build.h`** — add to `Context`: `bool demand_instance_enabled{false};` and `std::vector<struct{unsigned store_id; uint8_t kind;}> emitted_instance_ops;` (mirror `emitted_ingest_folds`, `Build.h:237`).

**`Stratum.cpp`** — after the surviving `impl->state_cells.clear(); for(...)` descriptor loop, add the `impl->instance_stores` build loop (E4 §5, 100% mono-safe — `differential` is a stored bit).

**Re-targeting map (dead symbol → tip)**: `SUBGRAPHINSTANCE`/`ProgramInstanceStore`/`kSubgraphInstance`/`ProgramImpl::instance_stores`/`AsSubgraphInstance` = **re-author** (all deleted). `TableDeltaVector`/`HasTableDeltaVector`/`operation_regions.CreateDerived`/`UseRef::Emplace` = **stable, present**. `DRFlowGraph::SubgraphInstances()` = **re-author as** `OpsOfKind(kSubgraphInstantiate)` one-liner (generic `OpsOfKind` survives, `Rel.h:936`).

### (d) Runtime store choice — **RESTORE `InstanceStore.h`, do NOT adapt StateCell** (E5 verdict)

`git show dc965d3c^:include/drlojekyll/Runtime/InstanceStore.h > include/drlojekyll/Runtime/InstanceStore.h`, unchanged. Rationale (E5, decisive):

- **Different abstraction, not different clothes.** StateCell's value slot is a *scalar reduction* (`Recompute::Working` = `{Vec<Summary>*, Vec<int32_t>*}`, linear-scanned by value-equality `Fold`); the keyed-instance value slot is a *whole relation* (`Table<RowT>` per iid, with `TryAdd`/`Find`/`NumRows`).
- **Seal cost.** StateCell `Seal` does a value copy `sealed := Emit(working)` (O(1) scalar). Instance `Seal` is an O(1) **pointer swap** `frozen ↔ current`; forcing a StateCell-style copy would copy every nested row every epoch — exactly what the frozen-pair design (d3-instance-store-target.md R-A, judge-hardened) exists to avoid.
- **Occupancy.** Instance occupancy = `current->NumRows()>0` (self-reported by the table); StateCell needs an out-of-band `working_count`. Two sources of truth is a correctness surface, not a saving.
- **Publish diff.** Band-(b) is a table-vs-table set difference (`frz.Find(row)==kNoRow`), not a scalar `new!=old`.
- **Shared skeleton is cheap to hand-write twice** (dense-id-by-hashed-key, `Touched()`, `Seal`-at-tail, `DebugValidate`) and **risky to abstract** — StateCell.h's own header frames itself as a *peer* of a future instance store, never its superset.

File is self-contained (`Allocator.h`/`BenchCounters.h`/`Table.h`/`Vec.h`, all present). Construct with default `monotone_=true` everywhere; never call `RecycleCurrent` in slice 1 (it stays as inert dead code for slice 2). The `#ifndef NDEBUG` `Seal()` "frozen ⊆ current" belt (lines 188-204) is a **positive** monotone regression guard — keep it.

### (e) Codegen emission — `lib/CodeGen/CPlusPlus/Database.cpp`

Restore `Generator::EmitSubgraphInstance` **mono-collapsed** from E4 §7 (~120-150 lines net):
- **Drop** band-(a0) death (`region.RemovalFrontier()` always nullopt), band-(a2') (`input_removal` always nullopt), the entire `if(diff)` publish arm (no `born`/`carried`/`dropped` counters, no drop-scan, no `AddDerivation`/`SubDerivation`, no queue `.Add`, no `V-INST-PARTITION` belt).
- **Keep**: the `emit_instance_rescan` lambda **without** the `if(input_diff)` `.Present(s)` conjunct (a monotone `Table::RowAt` never enumerates a dead row); band-a1 `FindOrAddInstance`+`!TouchedFlag`→rescan; band-a2 `emit_edge_drain` **`!diff` arm** (`iid!=kNoInstance && !TouchedFlag` — sound by irrevocability, no demand-liveness probe); band-b `for(iid:Touched()) for(r:cur.NumRows()) if(frz.Find(row)==kNoRow) pub.TryAdd(...)` with the `EmitIndexAdds` branch when the pub table has indexes; `Seal()` + `DebugValidate()`.
- **Header wiring** (all mono-essential): `#include <drlojekyll/Runtime/InstanceStore.h>` gated on `!program.InstanceStores().empty()`; per-store `Key_<id>`/`Row_<id>` `EmitHashStruct` (no `Reduce_<id>` policy struct, no driver ABI — unlike StateCell); `InstanceStore<Key_<id>,Row_<id>> instance_<id>;` member ctor'd with allocator; ref-param threading into touching procs (mirror StateCell `Database.cpp:883-932`).

**Re-targeting**: `EmitSubgraphInstance`/`Key_<id>`/`Row_<id>`/`instance_<id>` = re-author. `EmitHashStruct`/`EmitIndexAdds`/`RowExpr`/`VecName`/`table_member`/`col_field`/`cc.Indent` = stable, present.

### (f) Always-on validator obligations (E3) — minimal slice-1 surface

Each new op MUST satisfy, in `lib/Rel/Rel.cpp` `ValidateDROps`/census + `LinearizeAndValidateDRFlow`:

| Validator | Slice-1 obligation | Build now? |
|---|---|---|
| **Census recount** | `exp_instance` from `ResolveLiveRecognition(impl,query)` (NOT `flow.instances.size()` — E-27 anti-tautology, mirror the `exp_group_update` comment at `Rel.cpp:3450`). `expect(kSubgraphInstantiate, exp_instance)`, `expect(kInstanceSeal, exp_instance)`, `expect(kInstanceDeath, 0)`. | **YES** |
| **V-INST-EFFECT** | Each `kSubgraphInstantiate` carries exactly the mono multiset `{drain==2(demand+input kNetAddition), demand==1(kInstanceDemand), leaf==1(kFlagRead Present), rebuild==1(kInstanceRebuild +1), emit==1(kStateEmit), old==1(kStateOld), counter==1(sign+1)}` and **forbids** appends/crossings/second counter. `kInstanceSeal` = single sign-0 `kStateFold`. | **YES** |
| **V-INST-SOLE** | (a) `input_table` non-differential AND ≠ `pub_table`; (b) exactly one `kSubgraphInstantiate` per pub table. | **YES** |
| **V-INST-PAIR (mono arm)** | Grouped by `instance_store_id`: op-set == `{instantiate, seal}` exactly (no death). | **YES** |
| **V-INST-DRAIN** | `context.table_delta_vecs[demand_table][kNetAdditions] != nullptr` (catches silent zero-birth). | **YES** |
| **`DROpStratum` loud-fail** | §2b — miss ⇒ `ValidatorFail`. | **YES (mandatory)** |
| **V-LINEAR/V-LOOP/V-READY/V-BAND-HAZARD** | Free once `effects` honest + `key_of` correct. | auto |
| **V-INST-DIFF-COHERENCE / V-INST-EMITTED** | Vacuous-true drift catchers (`Procedure.cpp`). | **YES** (cheap, house discipline) |
| **V-INST-ORDER** | Death-before-instantiate ordering. | **NO** (vacuous — no death op; slice 2) |
| **V-ALPHA** | α-slot binding tags. | Arm A (generic PlanNode `kInstanceKeySlot`→`kSectionWalk`) rides existing PlanNode validation; Arm B (context_cols totality) worth adding, low cost. |

**Program::Build / Main wiring** (E4 §9-10): re-add `-demand-instance` flag → `gDemandInstance` (help text already at `Main.cpp:264`); thread as `Program::Build(*frozen, log, gFirstId, policy, /*demand_instance=*/gDemandInstance)` → `context.demand_instance_enabled`. Signature goes 4-arg → 5-arg at `Program.h:1379`.

---

## 3. KEYED-DIFFERENTIAL DIFF (slice 2, sketch)

What differential (`kInstanceDeath` + retraction) adds on top of slice 1:

- **Mint** (`BuildSubgraphInstanceOps`): un-drop the `if(TableIsDifferential(demand_table))` block → `kInstanceDeath` DROp (`table_op_sign=-1`, shares `pub_table` so OD-2 sorts it *before* the +1 instantiate) with `DeathEffects(pub,demand)` (restore E4 §2b). Un-drop `InstantiateEffects`'s `if(input_diff)` removal-drain arm and the `if(diff)` two-sign counter/crossing/append triple. `DRInstance::differential = TableIsDifferential(pub_table)` becomes truthy.
- **Lowering** (`LowerSubgraphInstances`): un-drop `death_by_sid`, `si->removal_frontier`, the `del_queue`/`add_queue`/`demand_table` Emplace block, `input_removal_frontier`; enroll `kInstanceDeath` into `emitted_instance_ops`.
- **Codegen** (`EmitSubgraphInstance`): un-drop band-(a0) `RecycleCurrent` on demand net-removals; band-(a2') removal-drain with the `.Present(s)` rescan conjunct + the differential demand-liveness probe (`demand_member.Find(...).Present(...)`) in `emit_edge_drain`; the `if(diff)` publish arm (`born`/`carried`/`dropped`, drop-scan `frz` vs `cur` → `SubDerivation`+`del_queue.Add`, `AddDerivation`+`add_queue.Add`, `V-INST-PARTITION` belt).
- **Validators re-arm**: `V-INST-EFFECT` R-DIFF fork (`counters==2, counter_signs==0, crossings==2, appends==2`; death shape `{drain,demand,old,rebuild@-1}` forbidding emit/counter/crossing/append); `V-INST-PAIR` R-DIFF arm (`{death?,instantiate,seal}`, death≤1); **V-INST-ORDER** (new `CheckInstanceOrder` off `flow.pinned_order`, death≺instantiate per store); `V-INST-SOLE` drops the "input non-differential" clause for the D3.a.2 diff-input arm; `V-INST-DRAIN` both-sign frontier presence.
- **Runtime**: `RecycleCurrent` goes live (already present in restored `InstanceStore.h`); `differential` ctor arg threaded true.
- **Equivalence gate**: the `.eqgate` sidecar family (flat `-demand` vs `-demand-instance` nested, byte-compared per mode against ONE golden) — the D3.a house pattern; `demand_neighborhood_witness` is the canonical carrier.

Slice 2 is D3.a.1-.3 territory (retract channel, diff-input a2' arm, multi-adornment N-stores-one-pub). Out of slice-1 scope.

---

## 4. OPEN RISKS (for the refuter panel)

1. **`instance_stratum` self-seed reads empty `drain_stratum` at mint (§2b).** Confirmed: `BuildSubgraphInstanceOps` runs inside `BuildDRInventory` (`Stratum.cpp:2149`), `DeriveDRStrata` runs after (`:2156`), so `flow.drain_stratum` is empty at mint → every `instance_stratum[sid]=1`. Sound for the acyclic mono corpus (base drains at 0) but **latently wrong** if a demand/input table ever drains ≥1. Refuter should confirm no mono-acyclic witness violates this, or force the post-`DeriveDRStrata` re-seed. *This is the single most likely place a subtle mis-ordering slips past every other validator (E3 warns: a wrong `key_of` fails no other check).*

2. **`GuardAnnotation::kBody` / `::instance_key` field names unverified at tip (E1/E4).** `ResolveLiveRecognition` reads `annots[ai].role == GuardAnnotation::kBody` and `annots[ai].instance_key`. Struct exists (`Query.h:1001`) but the enumerator/field names weren't independently re-checked. If S1a restored a drifted `GuardAnnotation`, the re-resolution mis-buckets. **Must grep `Query.h:1001-1032` before wiring.**

3. **`ResolveLiveRecognition` robustness when CSE eliminates all guards for a forcing.** E1's witness shows CSE folds two guard sites into one surviving JOIN and elides 1-arm MERGEs. `ri.ok` gates on all three tables + all three handles resolving; a `!ri.ok` forcing is `continue`-skipped (silent). Risk: a forcing that *should* mint but whose guards were over-eliminated silently produces zero instances → the census recount (also `ResolveLiveRecognition`-sourced) would agree with the (wrong) zero, masking it. **The census can't catch a recognizer that mis-resolves — E6/A7 explicitly flags this; the oracle/eqgate is the backstop.** Refuter: is there a mono witness where CSE eliminates a needed guard?

4. **`pub_table` INSERT identity match under multi-arity siblings.** `ResolveLiveRecognition` matches `ins.Declaration().Id() == forcing.query decl Id` (full name+arity). E4 warns a name-only match binds a legal same-name/different-arity sibling and partitions a mismatched pub row. Confirm `.Id()` embeds arity at tip (extract asserts it; verify).

5. **Slice-1 invariant `TableIsDifferential(pub_table)==false` program-wide.** `DRInstance::differential` comment (`Rel.h:867`) asserts pub is monotone "for every demanded subgraph today." If any bound-`#query` target is differential (e.g. a `@differential` demanded relation), the mono-only `InstantiateEffects_MONO` under-builds and the `!diff` codegen arm mis-publishes. **Add a `ValidatorFail` if `TableIsDifferential(pub_table)` in slice 1** rather than silently taking the mono arm — fail loud until slice 2.

6. **Interaction with the existing flat `-demand` guard-web.** `-demand-instance` implies `-demand` pre-cut; at tip `gDemand` is separate. If `-demand-instance` is set but `gDemand` is false, `ApplyDemandTransform` never runs and `RecognizedSubgraphs()` is empty → `BuildSubgraphInstanceOps` mints nothing (self-gates). If both set, the flat guard-web AND the keyed store both lower for the same forcing → double publish. **Decision needed**: does `-demand-instance` *replace* the flat lowering for recognized forcings (pre-cut behavior — the guard JOINs are recognized-and-consumed, not independently lowered), or compose? The pre-cut recognizer "consumed" the guard subgraph; confirm the flat guard JOINs don't *also* emit an independent publish path when the keyed store owns the pub table (V-INST-SOLE "exactly one instantiate per pub" partially guards this, but not against a non-instance flat deriver on the same table).

7. **`Program::Build` 4→5-arg ripples.** Every caller (Main.cpp:111, plus any test harness / `bin/Oracle` / `bin/RefHarness`) needs the new arg. Grep all `Program::Build` call sites; default the param to `false` in the header to minimize churn.

8. **Runtime `InstanceStore.h` restore vs current `Table.h`/`Vec.h`.** File deleted at `dc965d3c`; `Table.h` has since evolved (data-structures epoch: dead-row compaction, `CompactRowsInPlace`, id renumbering). E5 claims the includes are "present unchanged" but the *semantics* of `Table<RowT>::Find`/`RowAt`/`NumRows` under compaction must still match InstanceStore's `frozen ⊆ current` monotone assumption. **Verify `InstanceStore.h`'s nested `Table` usage compiles and its `DebugValidate` belt holds against the current `Table.h` before trusting the verbatim restore.**