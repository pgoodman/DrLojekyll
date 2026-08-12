<!-- Copyright 2026, Peter Goodman. All rights reserved. -->
# Session-32 grounding — KEYED-MONOTONE slice (adjudicated, empirically anchored)

> Produced by the session-32 grounding workflow (13 agents: 6 sonnet extractors over the
> pipeline + the pre-cut reference at `dc965d3c^`, opus design, a 4-refuter opus panel,
> opus IR-desired-states, opus adjudicator). **Empirically anchored:** an agent BUILT the
> pre-cut compiler at `dc965d3c^` and ran the canonical witness, so the `.rel`/`.ir`/`.h`
> golden targets below are VERBATIM compiler output, not prediction. The dumps are preserved
> under `session-32-keyed-monotone-grounding.artifacts/empirical-{nested,flat,cpp-nested}/`.
>
> Orchestrator verifications this pass (not taken on faith): (1) tip `IsCutSuccessorDR`
> (`lib/Rel/Rel.cpp:1131`) is `(void) context; return CanReceiveDeletions()||IsAggregate()||
> IsKVIndex();` — the demand clause IS deleted (BLOCKING finding confirmed real); the pre-cut
> reference has exactly `context.demand_instance_enabled && succ.GuardAnnotationIndex() !=
> QueryView::kNoGuardAnnotation`. (2) `demand_instance_enabled` exists NOWHERE at tip. (3) the
> mint slot at `Rel.cpp:1618` (before `// ---- branches/joins`) is correct. (4) `DRInstance` +
> the three `DROpKind` values + `DROp` instance members + `DRFlowGraph::instances` all survive
> in `Rel.h` unused. (5) `Program::Build` is 4-arg at tip. Design/refuter/IR raw transcripts
> preserved in the `.artifacts/raw-*.md` companions.

---

Both contested claims are now resolved against actual code. Decisive findings:

- **Tip `IsCutSuccessorDR` (Rel.cpp:1132)** = `(void) context; return CanReceiveDeletions() || IsAggregate() || IsKVIndex();` — the pre-cut `demand_instance_enabled && GuardAnnotationIndex() != kNoGuardAnnotation` clause is **confirmed deleted**. Triple-refuter consensus verified by direct diff. `Build.cpp:937` still calls through it, so restoring the one clause re-arms the eager cut + monotone-frontier provisioning generically.
- **Refuter 1 Finding 1 is REFUTED by empirical ground truth**: the pre-cut resolver requires `role==kBody` to bind `input_table` (Rel.cpp:979), and the ground-truth `-demand-instance` compile of the exact witness produced `input=%table:11` / `kSubgraphInstantiate=1`. The surviving guard JOIN for a 1-hop body demand carries `kBody`, not `kQueryProjection`. The resolver works; only its defensive belt is worth folding.

Here is the authoritative grounding document.

---

# Keyed-Monotone Slice — FINAL Grounding (Adjudicated)

**Branch** `keyed-instances`. **Reference** pre-cut tip `dc965d3c^` (commit `6d6248a2`). **Scope** slice 1 = birth + edge-after-demand rebuild + publish + seal, MONOTONE only (no `kInstanceDeath`, no retraction, no counters/queues). Restore the deleted keyed-instance lowering so a bound `#query` lowers to a keyed sub-database (`InstanceStore`) that **replaces** the flat guard-web.

**Adjudication basis**: the design + 4 refuter panels + 5 empirically-built IR desired-states (compiled at `dc965d3c^`, verbatim compiler output, not prediction) + 2 in-session code verifications this pass (`IsCutSuccessorDR` diff; pre-cut resolver role-loop).

---

## 1. VERDICT

**Keyed-monotone SURVIVES the panel as a buildable, answer-preserving slice — the substrate is intact and the target output is empirically proven (the pre-cut compiler produces the exact `.rel`/`.ir`/`.h` we are rebuilding toward). It does NOT survive "as framed" on three axes, all fixable without abandoning the approach:**

1. **BLOCKING — the file plan omits the cut-successor restore (triple consensus: R2-F1, R3-F1, R4-F1; confirmed by direct diff this pass).** The design's 10-file plan never touches `IsCutSuccessorDR` (`Rel.cpp:1132`) or wires `context.demand_instance_enabled` (zero hits at tip). Without the guard-annotation clause: (a) the monotone demand/input `kNetAddition` frontiers the `kVecDrain` effects consume are **never minted** (`MonotoneIngestRoleDR` returns `kEmpty` → `DRFlowGraph::TableVec` asserts, `Rel.cpp:598-605` → hard abort / NDEBUG corruption); **and** (b) the flat guard-web pub-INSERT is **not suppressed** → the keyed store publishes *beside* a still-live flat web → `.stdout`-invisible double-emission that moves zero codegen. **Both failures collapse to one ~2-line restore** (the clause + the `Context` field); `Build.cpp:937` already calls through, so the eager-walk cut and `AnyCutSuccessorDR` frontier provisioning re-arm automatically. This is **mandatory and mechanical**.

2. **BLOCKING as a FRAMING error — "answer-equal to the non-keyed full-materialization `.stdout`" is category-confused (R1-F2).** Demand materializes the pub relation *only for demanded keys*; that equals full materialization **only when the demanded relation is observed solely through the bound `#query` cursor**. Any non-query consumer (a downstream rule that publishes a message off the demanded relation) set-diverges (and per-epoch diverges) from a full-mat golden by construction. **Re-scope the validation**: primary equality is the **eqgate (flat `-demand` == nested `-demand-instance`)**, pulled INTO slice 1 (both paths are live and monotone, so they are answer-equal by construction); full-mat equality holds only for a query-only-observed witness. This *also* closes the R2-F2 census-tautology gap (below) by making a real answer witness mandatory in slice 1.

3. **MAJOR — the validator surface cannot self-prove count or ownership (R2-F2, R2-F3, R4-F3).** The census recount is a tautology (mint and recount both call `ResolveLiveRecognition`), and `V-INST-SOLE` counts only instance ops, not eager derivers on the same pub. **Fold**: an independent recount source + a sole-deriver structural validator + a structural golden proving the flat region vanished.

**REFUTED (does not survive scrutiny): Refuter 1 Finding 1 (zero-mint).** Its premise — the surviving guard carries `kQueryProjection` so `input_table` stays null — is contradicted by ground truth: the pre-cut resolver binds `input_table` off a `kBody` guard (`Rel.cpp:979-983`), and the empirical `-demand-instance` compile of the exact 1-hop witness produced `input=%table:11` / `kSubgraphInstantiate=1`. The 1-hop body guard JOIN carries `kBody`. The resolver, restored entire, resolves this witness. **Only R1-F1's defensive belt is folded** (loud-fail when `RecognizedSubgraphs()` non-empty yet every forcing resolves `!ok`), as cheap insurance and because R2-F2 independently wants it.

**No slice re-scope is forced.** Slice 1 stays monotone birth+rebuild+publish+seal. The corrections are (a) one mandatory file-plan addition, (b) a validation-framing swap, (c) five loud fences + one structural validator + one structural golden, (d) three ground-truth pseudocode corrections. Total surface ~1200-1500 lines across **11-12 files**, all byte-recoverable from `dc965d3c^`.

---

## 2. FINAL BUILT-OUT PSEUDOCODE (corrected)

`[NEW]` = re-authored; `[SCAF]` = surviving scaffolding reused; `[CUT]` = the mandatory cut-successor restore (was omitted); `[GT]` = ground-truth correction from the empirical dumps.

```
=== STAGE 0: Query::Build (lib/DataFlow, POST-Optimize) — RECOGNITION (LIVE, S1a) ===
Build.cpp:2601  ApplyDemandTransform(impl)      // mints guard JOINs + demand__ msg
  → per bound #query: impl->recognized_subgraphs += RecognizedSubgraph{...}
  → per bound #query: impl->demand_forcings += QueryDemandForcing{query,message,bound_params}
  → guard JOIN stamped guard_annotation_index; GuardAnnotation{role∈{kBody,kQueryProjection},
       demand_side, instance_key(pivot positions), forcing_index, guarded_read, demanded_view}
       (VERIFIED tip Query.h:1001-1032 — role/instance_key names correct; R2/R2-F6 discharged)
Build.cpp:2622  impl->Optimize()   // CSE folds/migrates guard_annotation_index; VIEW* keys DANGLE

=== STAGE 1: FrozenRegionalProgram::Build (lib/Regional) — NO KEYED CODE ===
  Tier-1 names the demanded-interior relation as a row-contract (driven by demanded_decl,
  independent of whether keyed LOWERING runs). V-REGION-CENSUS unaffected.
  [GT] .region dump byte-IDENTICAL flat↔nested (empirically verified). No keyed token here.

=== STAGE 2: Program::Build(frozen, log, first_id, policy, demand_instance) — DR pipeline ===
  [NEW] 5th arg demand_instance → context.demand_instance_enabled  (default false in header)

  [CUT] lib/Rel/Rel.cpp:1132  IsCutSuccessorDR(context, succ):
      return succ.CanReceiveDeletions() || succ.IsAggregate() || succ.IsKVIndex()
          || (context.demand_instance_enabled
              && succ.GuardAnnotationIndex() != QueryView::kNoGuardAnnotation);
    ▶ This ONE clause: (a) makes the guard JOIN a cut → AnyCutSuccessorDR (Rel.cpp:158)
      → MonotoneIngestRoleDR provisions the monotone demand+input kNetAddition frontiers
      the kVecDrain effects consume (without it: TableVec assert → abort);
      (b) stops the eager walk at the guard JOIN (Build.cpp:937) → the flat guard-web
      pub-INSERT is NOT emitted → keyed REPLACES flat (closes design Risk R6 as REPLACE);
      (c) [GT] the two emergent kCommitSweep ops on demand/input fall out of this
      (the section-walk rescan reads a committed+sealed table).

  (Stratum.cpp:2149) DRFlowGraph dr_flow = BuildDRInventory(impl, context, query, sccs)
     ... crossovers, products, GROUP_UPDATE (KVIndices :1615) ...
     [NEW @ Rel.cpp:1618]  if (context.demand_instance_enabled)
                             BuildSubgraphInstanceOps(flow, impl, context, query, scc_map);
       │
       ├─ LiveRecognition lr = ResolveLiveRecognition(impl, query)     [NEW, restore ENTIRE]
       │    re-buckets LIVE guard JOINs by forcing_index; per forcing binds:
       │      demand_table = model_table(join.in0);    input_table = model_table(join.in1)
       │        WHERE annots[ai].role == GuardAnnotation::kBody  (VERIFIED works: 1-hop
       │        body guard is kBody; empirical input=%table:11 — R1-F1 REFUTED);
       │      input_key_cols = annots[ai].instance_key;
       │      pub_table = model of live INSERT whose Declaration().Id()==forcing.query decl Id
       │        (Id() embeds name AND arity — arity-safe; R4-adjacent);
       │      ri.ok = demand_table && input_table && pub_table && all three handles resolve.
       │
       └─ for rs in query.RecognizedSubgraphs():
            ri = lr.by_forcing[rs.forcing_index]
            if (!ri.ok) continue;                       // ABA-safe skip (fully-dead forcing)
            [FENCE R1-F1/R2-F2]  see V-INST-RESOLVED below (loud if ALL forcings !ok)
            [FENCE R1-F3]  ValidatorFail if resolved >1 distinct kBody input for one forcing
            [FENCE HP-4 + R1-F3]  ValidatorFail if ri.input_view Is{Map,Negate,Agg,KVIndex,Join,Merge}
            [FENCE R1/R4-F2/R5]  ValidatorFail if TableIsDifferential(pub|demand|input)  // slice-1
            sid  = flow.instances.size()                                              [SCAF]
            flow.instances.push_back(DRInstance{demanded_view, pub_view}              [SCAF Rel.h:856]
                 .differential=false .pub_table .demand_table .input_table
                 .key_cols=rs.key_cols .row_cols=(pub positions ∉ key_cols)
                 .forcing_name=forcings[fi].query.NameAsString())
            [R1-F4] assert types(pub_cols[key_cols]) == types(input_cols[input_key_cols])
            DROp inst(kSubgraphInstantiate) .ctx=kSeed .table_op_table=pub_table .table_op_sign=+1
                 .demand_table .input_table .demanded_view=*ri.demanded_view .instance_store_id=sid
                 .reads = { kFlagRead(input_table, kPresent, kSeed) }   [GT: reads channel, NOT effects]
                 .effects = InstantiateEffects_MONO(pub,demand,input)   // §3(a) mono arm
                 .context_cols/.context_col_sources = per pub pos → kInstanceKeySlot | kRowSlot
                 .arms = [ DRArm(+1, kAccess(input_table, kSectionWalk) bound=input_key_cols
                                    -> kFold(pub_table, +1, kNonRecursive)) ]
            flow.ops.push_back(inst)
            // NO kInstanceDeath (mono demand — slice 2)
            DROp seal(kInstanceSeal) .ctx=kSeed .table_op_table=pub_table .instance_store_id=sid
                 .effects=[ kStateFold(pub_table, sign=0) ];  flow.ops.push_back(seal)
            flow.instance_stratum[sid] = 1 + max(drain_stratum(demand), drain_stratum(input))
                 ⚠ drain_stratum EMPTY at mint → both 0 → instance_stratum=1 (CORRECT mono-acyclic;
                   empirically op.0 stratum=1, R1 benign for this witness) — see Risk R1 + belt below

  (Stratum.cpp:2156) DeriveDRStrata(dr_flow)     // does NOT touch instance_stratum
     [R1/R2-F5 belt] DEBUG assert ∀sid: instance_stratum[sid] > drain_stratum[demand|input]
  (Stratum.cpp:2185) ValidateDRInventory / ValidateDROps / LinearizeAndValidateDRFlow
     DROpStratum keyed arm [NEW]:
        kSubgraphInstantiate|kInstanceDeath → flow.instance_stratum.at(sid)
             else ValidatorFail(...); __builtin_unreachable();   [R2-F4: no fallthrough to 0u]
        kInstanceSeal → 0u  (trailing band; kStateSeal twin; add to V-READY skip set)
     Census recount [NEW, INDEPENDENT source — NOT ResolveLiveRecognition]:  see §5
     V-INST-{EFFECT,SOLE,SOLE-DERIVER,PAIR-mono,DRAIN,RESOLVED}  [NEW]  see §5

=== STAGE 3: ControlFlow lowering — LowerSubgraphInstances (Procedure.cpp ~326) ===
  inside `if (context.dr_flow) {`, BEFORE LowerCommitSweeps:
    for op in dr_flow.SubgraphInstances():        [NEW accessor = OpsOfKind(kSubgraphInstantiate)]
      inst = dr_flow.instances[op.instance_store_id]
      demand_front = TableDeltaVector(demand_table, kNetAdditions)   // band-a1 source
      input_front  = TableDeltaVector(input_table,  kNetAdditions)   // band-a2 source
      si = operation_regions.CreateDerived<SUBGRAPHINSTANCE>(seq, sid, /*diff=*/false)
      si->demand_frontier/input_frontier/input_table/pub_table (Emplace)
      si->key_positions=inst.key_cols; row_positions=inst.row_cols
      si->input_key_cols=op.arms[0].body->bound_cols; input_row_cols=(input arity ∉ input_key_cols)
      [belt] assert si->input_key_cols.size() == inst.key_cols.size()
      context.emitted_instance_ops += {sid,kSubgraphInstantiate},{sid,kInstanceSeal}   [NEW]
    V-INST-EMITTED: emitted_instance_ops multiset == enrolled (op.kind,sid) multiset
    LowerCommitSweeps(...)  (unchanged)
  Stratum.cpp (after state_cells descriptor loop):
    [R1-F5 CORRECTED] impl->instance_stores build loop: per DRInstance derive
       key_types = inst.pub_table column-projection[key_cols].Type()     // DURABLE TABLE*, not pub_view
       row_types = inst.pub_table column-projection[row_cols].Type()
       → ProgramInstanceStore{id, key_types, row_types, differential=false}
  Procedure.cpp ClassifyVector: [NEW] case kSubgraphInstance → mark demand/input frontier vecs used

=== STAGE 4: C++ codegen — EmitSubgraphInstance (Database.cpp), mono-collapsed ===
  header: #include <drlojekyll/Runtime/InstanceStore.h>  gated on !InstanceStores().empty()
  per store: Key_<id> / Row_<id> hash structs (EmitHashStruct); NO Reduce_/driver ABI
  member:  ::hyde::rt::InstanceStore<Key_<id>, Row_<id>> instance_<id>;  (ctor: allocator)
           threaded as ref-param into every touching proc (mirror StateCell Database.cpp:883-932)
  region emitter:
    band-a1 (demand net-adds): for k in demand_front: iid=FindOrAddInstance(Key{k});
        if(!TouchedFlag(iid)) { assert !WorkingOccupied; cur=TouchCurrent; rescan(k) }
    band-a2 (input net-adds):  for e in input_front: iid=FindInstance(Key{e[in_key]});
        if(iid!=kNoInstance && !TouchedFlag(iid)) { ...; cur=TouchCurrent; rescan(e[in_key]) }
    rescan(keyexprs): for s in input.NumRows(): ir=input.RowAt(s);
        if(ir.<in_key>==keyexprs) cur.TryAdd(Row{ir.<in_row>})     // NO .Present() conjunct (mono)
    band-b (publish diff): for iid in Touched(): cur=Current(iid); frz=Frozen(iid); key=KeyAt(iid);
        for r in cur.NumRows(): row=cur.RowAt(r);
          if(frz.Find(row)==kNoRow) { ins=pub.TryAdd(pub_exprs(key,row));
                                      if(ins.added) EmitIndexAdds(...) }   // partial idx on pub
    seal: instance_<id>.Seal(); #ifndef NDEBUG instance_<id>.DebugValidate(); #endif
  [GT] the bound-#query cursor reads the PUB table (neighborhood_4) via the partial-key
    index (idx_41, P7 seek) after inject_37 seeds demand→flow→store→pub. The cursor
    does NOT read the store; the store is the intermediate band-b materializes into pub.
```

---

## 3. FINAL KEYED-MONOTONE DIFF (each fix tagged to its finding)

### (a) `lib/Rel/Rel.h` — un-reserve enum + no new fields

Re-add three enumerators immediately after `kStateSeal (145)`, before `kEagerForward (148)` so the `(18)` comment stays truthful:
```cpp
  kStateSeal,
  kSubgraphInstantiate,  // (15) D2.b keyed instance: birth/rebuild + publish
  kInstanceDeath,        // (16) D2.b DIFFERENTIAL-ONLY (slice 2; declared, unminted)
  kInstanceSeal,         // (17) D2.b frozen↔current swap (trailing band)
  kEagerForward,         // (18) ...
```
Declaring `kInstanceDeath` now (never minted in slice 1) stabilizes every exhaustive switch for slice 2. **All payload fields already exist** (`Rel.h:695-707`; R3-F4 verified — no new field needed; HP-3 "pub rides `table_op_table`" holds at tip).

### (b) `lib/Rel/Rel.cpp` — cut-successor restore + mint + stratum

- **[R2-F1/R3-F1/R4-F1 — MANDATORY, verified this pass]** `IsCutSuccessorDR` (`:1132`): add the `context.demand_instance_enabled && succ.GuardAnnotationIndex() != QueryView::kNoGuardAnnotation` disjunct (verbatim from `dc965d3c^`). This is the single load-bearing omission; it re-arms the monotone-frontier provisioning AND the flat-web suppression through the already-live `Build.cpp:937` / `AnyCutSuccessorDR` / `MonotoneIngestRoleDR` call chain.
- **[NEW]** `ResolveLiveRecognition` + `struct ResolvedInstance`/`LiveRecognition` — restore **entire** from `dc965d3c^:Rel.cpp:924-1025`. Verified correct: binds `input_table` off the `kBody` guard's `join.in1`, `demand_table` off `join.in0`, `pub_table` off the arity-exact `Declaration().Id()` INSERT match. **R1-F1 mechanism REFUTED** — this resolves the canonical witness (empirical `input=%table:11`).
- **[NEW]** `InstantiateEffects_MONO(pub,demand,input)` — the `!diff` arm only:
  `{ kVecDrain(demand,kNetAddition), kVecDrain(input,kNetAddition), kInstanceDemand(demand), kInstanceRebuild(pub,+1), kStateEmit(pub), kStateOld(pub), kCounter(pub,+1,kNonRecursive) }`. **[GT correction]** `kFlagRead(input,kPresent,kSeed)` goes on the op's **`reads` channel, NOT the effects multiset** (empirical `.rel`: `reads: Present(%table:11)`). Drop the `if(input_diff)` removal-drain and the whole `if(diff)` two-sign counter/crossing/append block. **Verified byte-exact against the empirical `op.0` effect set.**
- **[NEW]** `SealEffect(pub)` = single `kStateFold(pub, sign=0)` (verbatim).
- **[NEW]** `BuildSubgraphInstanceOps` — restore minus the `if(TableIsDifferential(demand_table))` `kInstanceDeath` block. Fold the fences: **[R1-F3]** `>1 kBody input ⇒ ValidatorFail`; **[R1-F3+HP-4]** refuse `input_view` Is{Map,Negate,Agg,KVIndex,**Join,Merge**} (extend HP-4 to Join/Merge — the single-table rescan cannot model a join body in slice 1); **[R1/R4-F2/R5]** `TableIsDifferential(pub|demand|input) ⇒ ValidatorFail`; **[R1-F4]** assert `types(pub[key_cols]) == types(input[input_key_cols])`.
- **[NEW]** `DROpStratum` (`:~3964`): keyed arms with **[R2-F4]** `ValidatorFail(...); __builtin_unreachable();` (no fallthrough to `kInstanceSeal: return 0u`). Miss on `instance_stratum.at(sid)` is a loud fail (E3-mandated; unlike `kGroupUpdate`'s silent 0u).
- **[NEW]** `SubgraphInstances()` = one-liner `OpsOfKind(kSubgraphInstantiate)` (generic `OpsOfKind` survives `Rel.h:936`).
- **[R1/R2-F5 belt]** post-`DeriveDRStrata` DEBUG assert `instance_stratum[sid] > drain_stratum[demand|input]`.

### (c) `lib/ControlFlow/Build/Build.h` + `Build.cpp` — Context wiring (was OMITTED)

- **[R3-F1/R4-F1]** `Build.h Context`: `bool demand_instance_enabled{false};` and `std::vector<struct{unsigned store_id; uint8_t kind;}> emitted_instance_ops;` (mirror `emitted_ingest_folds` `:237`).
- **[R4-F1]** `Build.cpp`: restore `effective_demand_instance` selection + the `context.demand_instance_enabled` assignment. The eager-walk cut at `Build.cpp:937` already calls `IsCutSuccessorDR` — no arm-specific Build.cpp code beyond the field is needed (refinement over R3/R4's "restore the Build.cpp arm": the arm is generic in the predicate).

### (d) `lib/ControlFlow/Program.h` + `Build/Procedure.cpp` + `Build/Stratum.cpp` — region family

- **[NEW/R3-F6]** `Program.h`: `ProgramOperation::kSubgraphInstance`; `ProgramSubgraphInstanceRegionImpl` / `using SUBGRAPHINSTANCE` — mono field subset (`demand_frontier`, `input_frontier`, `input_table`, `pub_table`, `key_positions`, `row_positions`, `input_key_cols`, `input_row_cols`, `store_id`, `const bool differential`); keep the differential `UseRef`s declared-but-null (no slice-2 header churn); `struct ProgramInstanceStore`; `ProgramImpl::instance_stores`; `ProgramVisitor::AsSubgraphInstance`; `FROM_OP`. Byte-templated by `GROUPUPDATE`; empty default `Visit` bodies mean no visitor subclass breaks (R3-F6 verified).
- **[NEW]** `Procedure.cpp`: `LowerSubgraphInstances` mono-collapsed (drop `death_by_sid`, the `if(inst.differential)` block, `input_removal_frontier`). Call site + `V-INST-EMITTED` before `LowerCommitSweeps`. `ClassifyVector` `case kSubgraphInstance`.
- **[R1-F5 CORRECTED]** `Stratum.cpp` `instance_stores` build loop: derive `key_types`/`row_types` from **`inst.pub_table`'s durable column projection**, NOT `inst.pub_view.Columns()` (the QueryView handle dangles by the time this loop runs).

### (e) Runtime — restore `InstanceStore.h` verbatim (§4).

### (f) `lib/CodeGen/CPlusPlus/Database.cpp` — `EmitSubgraphInstance` mono-collapsed

Restore from `dc965d3c^` §7 minus band-(a0) death, band-(a2'), and the whole `if(diff)` publish arm. Keep band-a1/a2/b + seal + `DebugValidate()`. All helpers verified present at tip (R3-F7: `VecName`, `EmitHashStruct`, `EmitIndexAdds`, `RowExpr`, `table_member`, `col_field`, `index_member`). **[GT]** the `#query` cursor is a plain pub-table partial-key hash seek (P7 `idx.First/Next`), *not* a store read.

### (g) `bin/drlojekyll/Main.cpp` + `lib/ControlFlow/Build.cpp` — flag + 4→5-arg

- Re-add `-demand-instance` → `gDemandInstance`; thread `Program::Build(*frozen, log, gFirstId, policy, /*demand_instance=*/gDemandInstance)`; default the param `false` in `Program.h:1379` (R3-F8: one real call site, trivial). **`-demand-instance` self-gates on `gDemand`** — if `gDemand` false, `RecognizedSubgraphs()` is empty and the mint is a no-op (R6 half).
- **[R4-F2]** reject `-demand-instance` composed with `-demand-retract` at flag-parse (differential demand is slice 2).

---

## 4. RUNTIME STORE DECISION — RESTORE `InstanceStore.h` (final)

**Restore `git show dc965d3c^:include/drlojekyll/Runtime/InstanceStore.h` verbatim. Do NOT adapt StateCell.** Decisive, in priority order:

1. **Empirically proven output.** The exact target `.h` (member `InstanceStore<Key_0, Row_0> instance_0`, band-a1/a2/b, `Seal`/`DebugValidate`) was produced by the InstanceStore-based pre-cut compiler running the exact witness. Restoring it reproduces a known-good artifact; any adaptation walks away from the proof.
2. **Verified to compile against current `Table.h` (R3-F3).** Every member InstanceStore depends on is present, semantics unchanged post-data-structures-epoch: `RowStore(Allocator)` ctor, `Find→kNoRow`, `RowAt`/`NumRows`, `TryAdd`, `Reset`, `Seal`. InstanceStore never calls the compaction path, so dead-row renumbering does not perturb its `frozen ⊆ current` monotone belt. **Risk R8 discharged.**
3. **Different abstraction, not different clothes.** StateCell's value slot is a *scalar reduction* (`Recompute::Working = {Vec<Summary>*, Vec<int32_t>*}`, value-equality `Fold`); the instance value slot is a *whole relation* (`Table<RowT>` per iid, `TryAdd`/`Find`/`NumRows`). StateCell `Seal` is a value copy; instance `Seal` is an O(1) frozen↔current pointer swap — forcing StateCell's copy would clone every nested row every epoch (the exact cost the frozen-pair design exists to avoid). Occupancy is self-reported (`current->NumRows()>0`) vs StateCell's out-of-band `working_count` (a second source of truth = a correctness surface, not a saving).

Construct with `monotone_=true` everywhere; `RecycleCurrent` stays inert dead code for slice 2. Keep the `#ifndef NDEBUG` `Seal()` "frozen ⊆ current" belt (lines 188-204) — a **positive** monotone regression guard. **[R1-F4 add]** extend `DebugValidate` with a band-(a) belt asserting every row in instance K has `input.<in_key> == K` (catches a position bug the arity belt passes).

---

## 5. VALIDATOR OBLIGATIONS (executor checklist)

| Validator | Slice-1 obligation | Source finding |
|---|---|---|
| **[CUT] `IsCutSuccessorDR` clause** | guard-annotation disjunct restored; without it `TableVec` aborts AND flat double-emits | R2-F1/R3-F1/R4-F1 |
| **Census recount — INDEPENDENT source** | `exp_instance` from a structurally-disjoint walk (count live `query.Inserts()` whose `Declaration().Id()` matches a `DemandForcings()[i].query` decl), **not** `ResolveLiveRecognition`. `expect(kSubgraphInstantiate,exp)`, `expect(kInstanceSeal,exp)`, `expect(kInstanceDeath,0)` | R2-F2 (anti-tautology, E-27) |
| **V-INST-RESOLVED** | if `RecognizedSubgraphs()` non-empty and every forcing resolves `!ok` ⇒ loud `ValidatorFail` (turn the silent `continue` into a covered cross-check) | R1-F1 (defensive) / R2-F2 |
| **V-INST-EFFECT** | `kSubgraphInstantiate` effect multiset == `InstantiateEffects_MONO` exactly; **`kFlagRead Present` counted on the `reads` channel, not effects**; forbid appends/crossings/second counter. `kInstanceSeal` = single `kStateFold(pub,0)` | design §2f + **[GT]** |
| **V-INST-SOLE** | `input_table` non-differential AND ≠ `pub_table`; exactly one `kSubgraphInstantiate` per pub | design §2f |
| **V-INST-SOLE-DERIVER (NEW)** | a pub owned by a `kSubgraphInstantiate` appears in **zero** `kEagerInsert`/eager-branch markers — makes "keyed replaces flat" a checked invariant, not an emergent one | R2-F3/R4-F3 |
| **V-INST-PAIR (mono)** | ops grouped by `instance_store_id` == `{instantiate, seal}` exactly (no death) | design §2f |
| **V-INST-DRAIN** | `table_delta_vecs[demand_table][kNetAdditions] != nullptr` AND `[input_table][kNetAdditions] != nullptr` (catches silent zero-birth; the cut clause is what makes these non-null) | design §2f / R2-F1 |
| **Differential fences (loud)** | `TableIsDifferential(pub|demand|input) ⇒ ValidatorFail`; reject `-demand-instance` + `-demand-retract` at parse | R1/R4-F2/R5 |
| **Input-arity/multiplicity fences** | `>1 kBody input per forcing ⇒ ValidatorFail`; refuse `input_view` Is{Join,Merge,Map,Negate,Agg,KVIndex}; `input_key_cols.size()==key_cols.size()` | R1-F3 |
| **`DROpStratum` loud-fail** | miss ⇒ `ValidatorFail(); __builtin_unreachable();` (no fallthrough to band 0) | R2-F4 |
| **instance_stratum belt** | DEBUG assert `instance_stratum[sid] > drain_stratum[demand|input]` post-`DeriveDRStrata` | R1/R2-F5 |
| **V-INST-EMITTED / DIFF-COHERENCE** | `emitted_instance_ops` multiset == enrolled; diff-coherence vacuous-mono | design §2f |
| **V-LINEAR/LOOP/READY/BAND-HAZARD** | auto once `effects` honest + `key_of` correct; add `kInstanceSeal` to the V-READY skip set | design §2f |
| **NOT built (vacuous slice-1)** | V-INST-ORDER (no death op), V-INST-PARTITION (no diff publish) | slice 2 |

**Non-perturbed (verified, no action):** RowContract/ProjectionRole (upstream, `Query::Build` tail), K5 `origin_decls` (upstream, DataFlow), V-REGION-CENSUS (upstream, Regional — `.region` byte-identical flat↔nested, empirically). The keyed mint is Rel/ControlFlow-layer and creates no Query views. (R2 "safe validators" analysis.)

---

## 6. EXECUTION PLAN (ordered, checkpointed)

**Witness (pin first, before any code):** `nbhd.dr` = `#message add_edge(u64 From,u64 To). #local edge(u64 From,u64 To). edge(F,T):add_edge(F,T). #query neighborhood(bound u64 Start,free u64 Node):edge(Start,Node).` — single-body-predicate, monotone, **no non-query consumer** (R1-F2 contract). Golden targets are the empirically-built dumps at `/private/tmp/.../scratchpad/out_nested/{w.rel,w.ir}` + `cpp_nested/datalog.h` and the flat twins in `out_flat/`.

**CP0 — un-reserve + wire the cut (compiles, codegen byte-unchanged flag-off).**
Rel.h enum (3 tags); `IsCutSuccessorDR` clause; `Build.h` Context fields + `Build.cpp` assignment; `Main.cpp` flag; `Program::Build` 5th arg. Audit **all ~131 exhaustive switches** (`grep -n 'case DROpKind::\|case ProgramOperation::'` in Rel.cpp/Format.cpp/Program.cpp/Procedure.cpp) — **[R3-F2]** `-Werror` is OFF, so a missed arm is a silent runtime abort, not a compile error; do a throwaway `-DWARNINGS_AS_ERRORS=ON` build as a hard gate. Add the `DROpKind` name-map arms (`Format.cpp:79`) and `DROpStratum` arms. **Exit CP0**: full build green; OptDiff `SUITE: PASS` **byte-identical** flag-off (orthogonality proof); ctest 5/5. → **WIP commit "CP0: un-reserve + cut-successor + flag, flag-off byte-identical".**

**CP1 — mint + validators (`.rel` matches golden).**
`ResolveLiveRecognition`, `InstantiateEffects_MONO`, `SealEffect`, `BuildSubgraphInstanceOps` (+ all fences), `DROpStratum` keyed arms, census independent recount, the V-INST-* battery, Format.cpp `.rel` instance render. **Exit CP1**: `drlojekyll nbhd.dr -demand -demand-instance -rel-out` byte-equals `out_nested/w.rel` (census `kSubgraphInstantiate=1 kInstanceSeal=1 kInstanceDeath=0 kCommitSweep=2 kEagerInsert=0 kEagerJoin=0`); all validators pass; flag-off still byte-identical. → **WIP commit "CP1: keyed mint + validators, .rel golden green".**

**CP2 — lower (`.ir` matches golden).**
`Program.h` region family; `LowerSubgraphInstances`; `instance_stores` build loop (pub_table-durable types); `ClassifyVector`; `.ir` render. **Exit CP2**: `-cf-out` byte-equals `out_nested/w.ir` (`subgraph-instance i#0 ... key@{0} row@{1} seal`, no `join-tables`). → **WIP commit "CP2: subgraph-instance region, .ir golden green".**

**CP3 — codegen + runtime (`.h` matches golden, runs).**
Restore `InstanceStore.h`; `EmitSubgraphInstance`; header wiring. **Exit CP3**: `-cpp-out` produces the `instance_0` member + bands + seal; compiles + links + runs; `datalog.h` matches `cpp_nested/datalog.h`. → **WIP commit "CP3: InstanceStore codegen, runs".**

**CP4 — witness + goldens + eqgate (answer correctness).**
Add `nbhd` to the corpus. **[R1-F2/R2-F2]** the **eqgate** (`.eqgate` sidecar, flat `-demand` vs nested `-demand-instance`, byte-compared per mode against ONE `.stdout` golden) is **mandatory in slice 1** (not deferred). **[R4-F4]** add `.rel`/`.ir`/`.h` `.irgold` structural goldens pinning *both* the `SUBGRAPHINSTANCE`/`instance_0` presence *and* the absence of the flat guard-JOIN→pub eager-insert region (a green `.stdout` alone is consistent with the F1 shadow). `.batches` + oracle + monotone goldens for the query-only-observed answer.

**EXIT GATE:** full build (incl. `bin/Oracle`); OptDiff `SUITE: PASS` byte-identical flag-off; ctest 5/5; `nbhd` under all 4 modes green; **eqgate flat==nested==golden**; the 4 always-on validator surfaces pass on the keyed graph; the structural golden proves the flat region vanished (double-emission ruled out). → **squash to the landing commit.**

---

## 7. THE FORK (owner STOP) — InstanceStore vs InstanceFlow

**RECOMMENDATION: build slice 1 on `InstanceStore` (the pragmatic runtime store). Defer `InstanceFlow` (the new context-family IR) until monotone AND differential land.** Frame as owner STOP — this is the one decision that gates the session's direction.

Decisive reasons:

1. **Only InstanceStore has proven output.** The exact `.rel`/`.ir`/`.h` we are rebuilding toward were emitted by the InstanceStore-based pre-cut compiler on the exact witness (empirically dumped this grounding). `InstanceFlow` is a new IR with **zero** produced artifact — choosing it means designing *and* proving from scratch, not restoring a known-good.
2. **The entire InstanceStore substrate is byte-recoverable from `dc965d3c^` and verified-buildable at tip** (R3-F3/F4/F5/F6/F7/F8 all discharged): the runtime store compiles against current `Table.h`, the reserved `DROp` fields + `PlanNode`/`DRArm` spine suffice, the resolver compiles, the region family is mechanical GROUPUPDATE-templated boilerplate, the codegen retargets cleanly. This is a ~1200-1500-line **restoration**, feasible in one focused session.
3. **`InstanceFlow` is a multi-session research bet with no runtime.** It is a new IR family (context-carrying flow objects) with no store, no lowering, no codegen, and no empirical anchor — it contradicts the session directive ("DO SOMETHING REAL this session"). Its value proposition (a cleaner context model for multi-adornment / nested keys) is a slice-2/3 concern (D3.a territory), not a slice-1 blocker.
4. **The panel's blocking issues are InstanceStore-plan omissions, not InstanceStore-abstraction faults.** The cut-successor restore, the framing re-scope, and the validator gaps apply identically to any lowering; none argues for a different IR. Switching substrate would not retire a single finding while discarding the proof.

**What would flip this** (state to the owner): if multi-adornment (N-stores-one-pub, OD-15) or nested/recursive keys surface a context-plumbing wall in InstanceStore during slice 2, reconsider InstanceFlow **then**, with the monotone slice landed as the equivalence backstop. Do not pay the research cost before the pragmatic path is proven insufficient.

---

## 8. RESIDUAL RISKS carried into execution

1. **`instance_stratum` self-seed reads empty `drain_stratum` at mint (R1).** Sound for mono-acyclic (empirically `op.0 stratum=1`), but latently wrong if a demand/input table ever drains ≥1. The DEBUG belt (§5) converts it from silent to loud on the first offender. If a mono witness ever violates it, promote the self-seed to a post-`DeriveDRStrata` step (mirror JOIN_EMIT delta enrollment at `Stratum.cpp:2170`).
2. **Two emergent `kCommitSweep` ops [GT].** Not hand-minted — they fall out of demand/input becoming cut successors. If the cut-successor restore is subtly incomplete, these vanish and the section-walk rescan reads an uncommitted table. The `.rel` census golden (`kCommitSweep=2`) pins them; a drop is caught at CP1.
3. **Census independence is only as good as the disjoint walk (R2-F2).** The recommended `query.Inserts()`-decl-match recount shares no code with `ResolveLiveRecognition`, but if a future refactor routes both through one helper the tautology returns. The eqgate/oracle is the ultimate backstop — keep it mandatory.
4. **Sole-deriver validator depends on the cut holding across CSE (R4-F3).** `guard_annotation_index` migrates on CSE (`Link.cpp:223`, `IdentityJoin.cpp:154` note the coupling); a migration bug could drop the stamp, un-cut the guard, and re-enable the flat web. V-INST-SOLE-DERIVER catches the *result* (an eager deriver on a keyed pub) structurally, independent of the stamp path.
5. **Slice-1 monotone invariant is program-wide, not witness-local (R5/F9).** The only end-to-end-proven mono witness is `nbhd`. Any bound `#query` over a `@differential` target silently needs the unbuilt differential arm — the loud `TableIsDifferential(pub|demand|input)` fence makes this an abort, not a miscompile. Keep it from day one.
6. **Multi-body / join-body demand is out of slice 1 (R1-F3).** `Demand.cpp` *recognizes* join bodies (traverses JOINs, `:704-716`), but the single-table rescan cannot model them. The extended HP-4 fence (refuse `input_view` Is{Join,Merge}) turns a would-be under-materialization into a clean diagnostic. A two-hop witness is slice-2+ territory.