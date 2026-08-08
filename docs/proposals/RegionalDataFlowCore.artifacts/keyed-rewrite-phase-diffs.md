# Keyed-instance rewrite — P1–P9 hunk-grained phase diffs

Session 12 (2026-08-06, branch `keyed-instances`, tip `46a404d4` + uncommitted Phase-0).
This is §4 of the rewrite grounding, deepened to hunk grain. Each phase is a diff
on the pseudocode of `keyed-rewrite-pseudocode-seed.md` §1 (current) → §2/§3
(target). Anchors are from `keyed-rewrite-current-pseudocode.md` §1.0 (re-verified
@ tip). Authored by an opus panel (P1 cut / P2-P3 regional authority / P4-P5
specialization+DAG / P6 recursive / P7-P9 physical), reconciled by the orchestrator.
The adversarial critique + ranked findings are in `keyed-rewrite-critique.md`.

**Phase 1 is OWNER-GATED and destructive.** These diffs are design grounding, not
an instruction to implement.

> ⚠️ **SUPERSEDED IN PART — read `keyed-rewrite-whole-program.md` §3 FIRST.** The
> session-12 adversarial critique (`keyed-rewrite-critique.md`) proved the P1
> deletion inventory below is INCOMPLETE (the atomic cut would not compile) and
> found blocking gaps in P4/P6.5. The corrected P1 inventory, the P2/P3/P4/P5
> amendments, and the open sequencing decisions D1–D4 live in
> `keyed-rewrite-whole-program.md` §3. Apply those before treating any P1.x
> inventory here as final.

## Design-goal → phase coverage

| # | Design goal | Resolving phase(s) |
|---|---|---|
| 1 | Four-authority separation | P1 (dissolve collapse), P2 (typed records) |
| 2 | Order-significant paths + order-free binding identity | P0 (parser), P1 (delete Demand sort), P5 |
| 3 | RequestEdge vs RuleActivationEdge | P3 (introduce both), P6 (activation cycles) |
| 4 | Rooted-reachability liveness | P3 (roots), P6 (cyclic drain) |
| 5 | Honest FullScanFilter | P1 (retire kSectionWalk), P4 (honest label), P7 (real plans) |
| 6 | Partial-binding DAG | P5 |
| 7 | Co-recursive key flow | P6 |

---

## Cluster: Phase 1 — the destructive demand-authority cut (owner-gated). Decomposed into seven ordered sub-cuts P1.1–P1.7 that LAND ATOMICALLY as one commit (the corpus is re-run only at the tail); the ordering is the recommended implementation/review sequence and is dictated by reintroduction obligations (you cannot delete GuardAnnotation/RecognizedSubgraph until the Planning.cpp + Rel.cpp consumers that read them are stubbed to the demand-free baseline). Net effect: ApplyDemandTransform, the fabricated demand__ messages, QueryDemandForcing/GuardAnnotation/RecognizedSubgraph, the Rel kSubgraphInstantiate/kInstanceDeath/kInstanceSeal nested lowering, the InstanceStore runtime leaf + its full-scan codegen mold, and the -demand/-demand-retract/-demand-instance flags all cease to exist. Post-cut honest baseline: @key is INERT parsed metadata (paths carried on the decl, consumed by nobody until P4); a bound #query reads the canonical, fully-materialized relation through the ordinary query cursor; there is ONE fact authority (the materialized pub table) and no mislabeled access — the kSectionWalk placeholder is retired, not relabeled. Keyed/residual evaluation is NON-FUNCTIONAL from here until P4; this is the single honest baseline, not a fallback. Anchors re-verified at tip 46a404d4 + the uncommitted Phase-0 worktree.

### P1.1 — Delete ApplyDemandTransform and its call site; drop demand params from Query::Build

**Diff**

```diff
@@ lib/DataFlow/Build.cpp  QueryImpl::Query::Build (:2524) @@
-Query::Build(module, log, policy, demand_mode, demand_retract, suppress_demand):   # Query.h:1102 decl
+Query::Build(module, log, policy):
   ... build graph ...
   proxy_view_to_decl = {}                         # Connect out-param, Connect.cpp:164
   if !ConnectInsertsToSelects(log, proxy_view_to_decl): return nullopt
-  # THE DEMAND CUT (call @~2561):
-  if !impl.ApplyDemandTransform(module, log, demand_mode, demand_retract,
-                                suppress_demand, proxy_view_to_decl):
-    return nullopt
-  if errors: return nullopt
   if policy.AnyBodyOptionalEnabled(kDataFlow): impl.Optimize(log, policy)
   ... IdentifyInductions / FinalizeDepths / Stratify / InferConservativeRowContracts unchanged ...

@@ lib/DataFlow/Demand.cpp  — DELETE THE ENTIRE TRANSLATION UNIT @@
-QueryImpl::ApplyDemandTransform(...)                 # :388, body :397-1487
-  the activation gate (demand_key_decls scan, pragma_activated) # :411-430
-  the re-entry guard DemandMessagesFabricated()      # :434-439
-  Loop 1 per-adornment locate/fence                  # :531
-  Tier-1 decl resolve + RP-6 realization             # :858-882
-  V-DECLARED-KEY RP-10 bijection + the sort() collapse # :892-959 (sort @905)
-  Step-4 stray-consumer union                        # :974
-  Loop 2 fabricate/mint/guard/register               # :1014
-  R-DUP grouped rewire                               # :1374
-  OWN-3 census belts                                 # :1481
-Query::DemandForcings/GuardAnnotations/RecognizedSubgraphs accessor bodies # :298-316 (move to P1.6 with the decls)

@@ lib/DataFlow/Connect.cpp  ConnectInsertsToSelects (:164) @@
-  out param proxy_view_to_decl; write proxy_view_to_decl[insert_proxy]=decl  # :1307 (sole reader was ApplyDemandTransform)
   insert_proxy.origin_decls = [decl]                 # :1309 KEEP — K5 Tier-2 seed, demand-independent
```

**Invariant established.** Query::Build is a pure parse->dataflow lowering with NO demand parameter and NO demand mutation: for every module the built graph is byte-identical to the pre-cut demand-OFF, pragma-free build. `impl->demand_forcings`/`guard_annotations`/`recognized_subgraphs` are never populated (they are always empty until their storage is deleted in P1.6).

**Exit gate.** Grep proves zero references to ApplyDemandTransform outside its deleted TU. build/debug builds. Every pre-cut FLAGLESS, pragma-free corpus case (the 166 pre-demand cases) produces a byte-identical `.df`/`.rel`/`.ir` dump (the demand-OFF no-op was already byte-identical, so this is provable by diff against the pre-cut golden). Any case whose ONLY prior signal came from the transform now compiles as a plain program.

**Design goals resolved.**
- Goal 2 (order-significant paths): DELETES the Demand.cpp:905 order-collapsing sort — one of the two SameKeySetOfSets copies (Parser.cpp:1477 is the other, Phase-0 scope) — removing the site that treated [A,B] and [B,A] as one declaration in key/adornment validation.
- Goal 1 (four-authority separation): removes the top-down authority that collapsed a relation-local key declaration into a query-demand adornment; @key stops force-fabricating query-demand relations.

**Deletion obligations.**
- lib/DataFlow/Demand.cpp — whole TU: QueryImpl::ApplyDemandTransform (:388) and all helpers
- the demand_mode/demand_retract/suppress_demand parameters of Query::Build (include/drlojekyll/DataFlow/Query.h:1102; def lib/DataFlow/Build.cpp:2524)
- the proxy_view_to_decl out-param write at lib/DataFlow/Connect.cpp:1307 (Tier-1 feed; sole reader deleted)
- ParsedModule::DemandMessagesFabricated/MarkDemandFabricated (include/drlojekyll/Parse/Parse.h:937-938; def lib/Parse/Parse.cpp) — only the re-entry guard used them

**Reintroduction obligations.**
- Query::Build callers must drop the demand args: bin/drlojekyll/Main.cpp:76 (gDemand/gDemandRetract), bin/Oracle/Main.cpp:749-753 (was suppress_demand=true — trivial, Oracle stays demand-blind by construction).
- Connect.cpp keeps the origin_decls seed (:1309) intact — it is the demand-free Tier-2 provenance that P1.6 leans on for interior naming after Tier-1 is deleted.

**Anchors.** `lib/DataFlow/Build.cpp:2524`, `lib/DataFlow/Build.cpp:2561`, `lib/DataFlow/Demand.cpp:388`, `lib/DataFlow/Demand.cpp:905`, `lib/DataFlow/Connect.cpp:164`, `lib/DataFlow/Connect.cpp:1307`, `lib/DataFlow/Connect.cpp:1309`, `include/drlojekyll/DataFlow/Query.h:1102`, `bin/Oracle/Main.cpp:749`

---

### P1.2 — Delete the demand__ fabrication half (FabricateDemandMessage/Local + registry)

**Diff**

```diff
@@ lib/Parse/Demand.cpp — DELETE THE ENTIRE TRANSLATION UNIT @@
-ParsedModule::FabricateDemandMessage(name, param_types, differential)   # :170
-    mint ParsedMessageImpl under reserved `demand__` prefix; stamp @differential synthetic (:1497)
-ParsedModule::FabricateDemandLocal(name+"_local", param_types)          # :226
-    mint ParsedLocalImpl under `demand__`
-FabricateParams / DemandFabricationWouldCollide / the demand__ collision scan (F30) # :50,149

@@ include/drlojekyll/Parse/Parse.h — DELETE decls @@
-ParsedModule::FabricateDemandMessage / FabricateDemandLocal
-ParsedModule::DemandFabricationWouldCollide
-Query::IsDemandMessage(ParsedMessage)                                    # used only by codegen suppression, killed in P1.5

@@ lib/Parse/Parse.cpp — DELETE @@
-the `demand__` reserved-prefix accessors and the kind-scoped collision scan (F30)
```

**Invariant established.** No compiler path mints a synthetic ParsedMessageImpl/ParsedLocalImpl. The `demand__` prefix reverts to an ordinary (now unreserved) identifier namespace; a user relation named demand__* is no longer special-cased. Every ParsedDeclaration in a built module traces to real source text.

**Exit gate.** Grep proves zero references to Fabricate*/DemandFabricationWouldCollide/IsDemandMessage anywhere. The two reserved-prefix reject cases (the demand__ collision witnesses, F30) are deleted from the rejects corpus with their runall.sh entries. build passes.

**Design goals resolved.**
- Goal 1 (four-authority separation): removes the fabrication that turned an internal storage declaration into query-demand messages — the clearest instance of the collapsed authority (a relation-local key manufacturing an external calling-convention message).

**Deletion obligations.**
- lib/Parse/Demand.cpp — whole TU (FabricateDemandMessage :170, FabricateDemandLocal :226, FabricateParams/collision scan :50,149)
- ParsedModule::FabricateDemandMessage/FabricateDemandLocal/DemandFabricationWouldCollide decls (include/drlojekyll/Parse/Parse.h)
- the F30 kind-scoped demand__ collision scan (lib/Parse/Parse.cpp)
- the @differential synthetic-attribute stamp on fabricated messages (Parse/Demand.cpp:1497 in the current-pseudocode numbering)

**Reintroduction obligations.**
- None — FabricateDemand* had exactly one caller (the P1.1-deleted ApplyDemandTransform). Query::IsDemandMessage is left declared-but-unused until P1.5 deletes its codegen consumer, or delete it here and defer the codegen edit to P1.5 (either order is safe since IsDemandMessage over an empty message set returns false).

**Anchors.** `lib/Parse/Demand.cpp:170`, `lib/Parse/Demand.cpp:226`, `lib/Parse/Demand.cpp:50`, `lib/Parse/Demand.cpp:149`, `include/drlojekyll/Parse/Parse.h:937`

---

### P1.3 — Delete the driver flags and the Program::Build flat/nested selector + keyed feature-gap fences

**Diff**

```diff
@@ bin/drlojekyll/Main.cpp @@
-static bool gDemand=false, gDemandInstance=false, gDemandRetract=false;   # :49-51
   query = Query::Build(module, error_log, gPassPolicy)                    # :76 (args dropped in P1.1)
-program = Program::Build(*frozen, log, first_id, policy, gDemandInstance) # :114
+program = Program::Build(*frozen, log, first_id, policy)
-  arg parse: -demand / --demand (:608), -demand-instance (:615), -demand-retract (:625)
-  usage text: -demand-instance / -demand-retract (:267-268)

@@ lib/ControlFlow/Build/Build.cpp  Program::Build (:1333) @@
-Program::Build(frozen, log, first_id, policy, demand_instance):
+Program::Build(frozen, log, first_id, policy):
   query = frozen.Query()                                                   # :1337 KEEP (H4 unwrap)
   ... C-2 feature-gap pre-pass (agg/kv/map/product-in-scc) KEEP :1345-1437 ...
-  # ---- D2.b keyed-instance feature-gap fences, per forcing ----   :1439-1519
-  any_forcing=false; all_forcings_admissible=true
-  annots = query.GuardAnnotations()                                        # :1466
-  query.ForEachView: bucket views with GuardAnnotationIndex()!=kNoGuardAnnotation
-  per forcing: compute cyclic_demand / recursive_content; strict reject arm  # :1490-1517
-  # ---- RP-9 silent pragma->nested selection ----                   :1532-1549
-  effective_demand_instance = demand_instance
-  if !demand_instance && any_forcing && all_forcings_admissible &&
-     subgraphs[0].demanded_decl.HasInstanceKey(): effective_demand_instance=true
-  context.demand_instance_enabled = effective_demand_instance              # :1568 (THE selector bit)
-  context.demand_forcings = &query.DemandForcings()                        # :1560
   ... region building continues — now UNCONDITIONALLY the full-materialization arm ...
```

**Invariant established.** There is exactly ONE lowering arm. `context.demand_instance_enabled` no longer exists; every downstream call site gated on it (Rel.cpp:1573/2067/4002, the eager-walk chain-breaker excision, OD-4 provisioning) reduces to its flag-off branch. A bound #query lowers to a plain read over its fully-materialized pub table. No -demand* flag is accepted on the command line.

**Exit gate.** `drlojekyll -demand foo.dr` errors as an unknown flag (or the flag is simply gone from usage). Program::Build has no demand_instance parameter. The four RP-6 demand-fence diagnostics (demand_cyclic_1, demand_recursive_content_1, demand_two_queries_1, demand_multi_adorn_1) and the two multi-adorn/all-free key rejects no longer emit a diagnostic — they compile — and are removed from runall.sh's expected-diagnostic list (verified in P1.7). Suite still ends SUITE: PASS after the P1.7 migration.

**Design goals resolved.**
- Goal 1 (four-authority separation): removes the selector that made @key a demand opt-in and the silent flat/nested fallback the next-session-prompt explicitly forbids ('Do not silently fall back to the old flat architecture').
- Goal 5 (honest FullScanFilter): removes the -demand-instance path that ROUTED to the kSectionWalk mold; after this the only access a bound query performs is the ordinary materialized-relation read (label-honest).

**Deletion obligations.**
- bin/drlojekyll/Main.cpp: gDemand/gDemandInstance/gDemandRetract (:49-51), the -demand/--demand/-demand-instance/-demand-retract arg-parse arms (:608-628), usage lines (:267-268)
- lib/ControlFlow/Build/Build.cpp: the per-forcing keyed feature-gap fence block (:1439-1519), the RP-9 selection (:1532-1549), context.demand_forcings (:1560), context.demand_instance_enabled (:1568), the demand_instance parameter of Program::Build (:1333)
- the `-demand-retract` differential-demand entry point + RETRACT hidden friend (lib/CodeGen/CPlusPlus/Database.cpp:1685-1722) — nothing sets @differential on a (now nonexistent) demand message

**Reintroduction obligations.**
- Program::Build must UNCONDITIONALLY take the full-materialization arm — re-provide by deleting the branch, not selecting it: the flat guard-web arm was already the demand_instance_enabled==false path, so 'the flat arm becomes the sole arm' is the re-provided capability. Confirm the eager-walk chain-breaker excision + OD-4 net-additions-frontier provisioning (previously gated on demand_instance_enabled) are the ONLY consumers and drop cleanly to their flag-off (no-op) branch.
- bin/drlojekyll/Main.cpp Program::Build call (:114) drops its last arg.
- The bound-query codegen entry point (Database.cpp forcing-function arm, spec.forcing_function branch ~1652/1673) must fall through to the plain-cursor read, since no query now carries a forcing function (that registry dies in P1.6).

**Anchors.** `bin/drlojekyll/Main.cpp:49`, `bin/drlojekyll/Main.cpp:114`, `bin/drlojekyll/Main.cpp:608`, `lib/ControlFlow/Build/Build.cpp:1333`, `lib/ControlFlow/Build/Build.cpp:1439`, `lib/ControlFlow/Build/Build.cpp:1532`, `lib/ControlFlow/Build/Build.cpp:1568`, `lib/CodeGen/CPlusPlus/Database.cpp:1685`

---

### P1.4 — Delete the Rel nested keyed lowering (mint ops, shape recovery, and their validators)

**Diff**

```diff
@@ lib/Rel/Rel.cpp @@
-ResolveLiveRecognition(impl, query) -> map<forcing_index, ResolvedInstance>  # :936-1025
-BuildSubgraphInstanceOps(flow, impl, context, query, scc_map)                # :1038-1200
-  gated call site: if context.demand_instance_enabled: BuildSubgraphInstanceOps(...) # :2067
-  mints: DROp(kSubgraphInstantiate) :1112, kInstanceDeath :~1678, kInstanceSeal :~1688
-  access.lowering = kSectionWalk (THE LIE) on the Rederive arm             # Rel.cpp:1146 / Rel.h:483
-  V-ALPHA arm A/B/B-i/B-ii/B-iii                                            # :4413-4493
-  V-INST-SOLE / V-INST-EFFECT / V-INST-PAIR                                 # :4283-4407 (CheckInstanceSolePub :4386)
-  V-INST-DRAIN (regime-split)                                              # :4493-4528
-  CheckInstanceDeathFrontier / CheckInstanceRebuildDeriver                 # Rel.h:1226,1250
-  the keyed-instance census recount arm                                    # :3994-4021 (kSubgraphInstantiate expect count)
-  op-kind switch arms for kSubgraphInstantiate/kInstanceDeath/kInstanceSeal in the DR op walk :591

@@ lib/Rel/Rel.h @@
-enum DROpKind:: kSubgraphInstantiate (:148), kInstanceDeath (:153), kInstanceSeal (:159)
-Lowering::kSectionWalk (:483)   # RETIRE the placeholder value entirely (kFullScan/kPointTest/kSeek remain)
-DRInstance descriptor, DRFlowGraph::instances / instance_stratum / SubgraphInstances() (:949)
-CheckInstanceDeathFrontier / CheckInstanceRebuildDeriver / V-INST-* decls (:1226-1250)
```

**Invariant established.** The DR-IR has no keyed-instance vocabulary: no kSubgraphInstantiate/kInstanceDeath/kInstanceSeal op, no DRInstance descriptor, no kSectionWalk Lowering value, no V-INST-*/V-ALPHA validators. Every DR op the lowering emits corresponds to code codegen actually emits; the census recount no longer expects a nonzero instantiate count. The label-vs-emission divergence (V-PRED-XCHECK's keyed sibling) is structurally impossible because the mislabeled op is gone.

**Exit gate.** RelValidators ctest passes with the V-INST-*/V-ALPHA families deleted (they were VACUOUS knob-off already, so their removal changes no live assertion). `-rel-out` on every surviving case shows zero instance ops; the 11 .rel goldens that carried census lines drop their kSubgraphInstantiate/kInstanceDeath rows (re-blessed in P1.7). No `.rel` dump contains the token `section-walk`.

**Design goals resolved.**
- Goal 5 (honest FullScanFilter): FULLY retires the kSectionWalk placeholder at Rel.cpp:1146 and the Lowering::kSectionWalk enum value — the exact exit-gate deliverable. The dishonest 'labelled walk over a full scan' is deleted rather than relabeled; P4 will reintroduce an honest FullScanFilter label that matches emitted code.
- Goal 1 (four-authority separation): deletes the DR-IR layer that reduced facts/derivations/requests/forcing-indexes/store-handles to DRInstance side-fields + raw integers (the §1.5 identity gap at the Rel layer).

**Deletion obligations.**
- lib/Rel/Rel.cpp: ResolveLiveRecognition (:936-1025), BuildSubgraphInstanceOps (:1038-1200) + its gated call (:2067), the kSubgraphInstantiate census-recount arm (:3994-4021), V-ALPHA (:4413-4493), V-INST-SOLE/EFFECT/PAIR + CheckInstanceSolePub (:4283-4407), V-INST-DRAIN (:4493-4528), the three instance op-kind switch arms (:591)
- lib/Rel/Rel.h: DROpKind::kSubgraphInstantiate/kInstanceDeath/kInstanceSeal (:148-159), Lowering::kSectionWalk (:483), DRInstance + DRFlowGraph::SubgraphInstances (:949), CheckInstanceDeathFrontier/CheckInstanceRebuildDeriver/V-INST decls (:1226-1250)
- the InstantiateEffects/DeathEffects/SealEffect effect-set builders (Rel.cpp:780 region)

**Reintroduction obligations.**
- The DR lowering for a bound query must ride the ordinary full-materialization eager web — re-provide: BuildStratumPhases already lowers the pub relation's INSERT/SELECT/JOIN web (the flat arm) independent of the instance ops, so removing BuildSubgraphInstanceOps leaves a complete, correct lowering of the materialized relation. Confirm no surviving op references DRFlowGraph::instances (V-PRED-XCHECK, DeriveDRStrata band-key tie-break).
- The DR census (DeriveDRStrata / V-ONE-FOLD family) must recount without the instance ops — they simply drop out of the census multiset.

**Anchors.** `lib/Rel/Rel.cpp:936`, `lib/Rel/Rel.cpp:1038`, `lib/Rel/Rel.cpp:1146`, `lib/Rel/Rel.cpp:2067`, `lib/Rel/Rel.cpp:4002`, `lib/Rel/Rel.cpp:4386`, `lib/Rel/Rel.cpp:4413`, `lib/Rel/Rel.h:148`, `lib/Rel/Rel.h:483`

---

### P1.5 — Delete the InstanceStore runtime leaf, its codegen mold, and the ControlFlow instance region

**Diff**

```diff
@@ include/drlojekyll/Runtime/InstanceStore.h — DELETE THE HEADER @@
-template<Key,RowT> class InstanceStore  # :54-219 (FindOrAddInstance/TouchCurrent/WorkingOccupied/RecycleCurrent/Seal/DebugValidate)

@@ include/drlojekyll/ControlFlow/Program.h @@
-class ProgramSubgraphInstanceRegion(Impl)   # :847-909
-ProgramRegion::IsSubgraphInstance() (:119), ProgramRegion ctor (:90), friend (:156)
-ProgramInstanceStoreInfo (:1432-1472), Program::InstanceStores()
-ProgramVisitor::Visit(ProgramSubgraphInstanceRegion) (:1539)

@@ lib/CodeGen/CPlusPlus/Database.cpp @@
-EmitSubgraphInstance(region)                         # decl :310, def :2352-2787
-  emit_instance_rescan (THE FULL-SCAN MOLD)          # :2434-2494 (honesty note :2339-2341)
-  band-(a0) death :2511 / (a1) birth :2532 / (a2·a2') emit_edge_drain :2577 / (b) publish :2661
-region dispatch arm: else if region.IsSubgraphInstance() { ...EmitSubgraphInstance... }  # :766-767
-Key_<id>/Row_<id> struct emission                    # :1251-1290
-InstanceStore member decl / ctor-forward / proc-param # :913-917, :966-967
-fabricated-demand entry-point suppression: if program.Query().IsDemandMessage(*m): continue  # :~3693

@@ tests/InstanceStore — DELETE the ctest target @@
```

**Invariant established.** No generated database contains an InstanceStore member, a Key_/Row_ instance struct, a SUBGRAPH_INSTANTIATE region, or the full-scan rescan mold. The one place codegen physically performed the O(NumRows(input)) scan-and-filter is gone. The ProgramRegion visitor closure is total without the instance arm.

**Exit gate.** Grep proves zero references to InstanceStore / ProgramSubgraphInstanceRegion / EmitSubgraphInstance / emit_instance_rescan. No generated datalog.h emits `InstanceStore<`. The InstanceStore ctest target is removed from CMake and `ctest` no longer lists it. Every surviving corpus case's generated code compiles and links against the runtime WITHOUT InstanceStore.h.

**Design goals resolved.**
- Goal 5 (honest FullScanFilter): deletes the emit_instance_rescan full-scan mold (Database.cpp:2434-2494) and its in-code honesty caveat (:2339-2341) — the physical scan the kSectionWalk label lied about is removed; the honest baseline read is the ordinary materialized-relation cursor.
- Goal 4 (rooted-reachability liveness): deletes the InstanceStore occupancy = current[iid].NumRows()>0 proxy (a per-key presence cache) — the leaf-cache liveness notion the prompt forbids ('neither a count nor an allocated runtime handle is the semantic owner'). Rooted-reachability is BUILT in P6; here the wrong owner is retired.

**Deletion obligations.**
- include/drlojekyll/Runtime/InstanceStore.h — whole header (:54-219)
- include/drlojekyll/ControlFlow/Program.h: ProgramSubgraphInstanceRegion(+Impl) (:847-909), IsSubgraphInstance (:119), ProgramInstanceStoreInfo/InstanceStores (:1432-1472), the visitor Visit overload (:1539)
- lib/CodeGen/CPlusPlus/Database.cpp: EmitSubgraphInstance decl+def (:310, :2352-2787), the region-dispatch arm (:766), Key_/Row_ struct emission (:1251-1290), InstanceStore member/ctor/param plumbing (:913-917, :966-967), the IsDemandMessage entry-point suppression (:~3693)
- tests/InstanceStore/* + its CMake target and ctest registration
- lib/ControlFlow/Build/*: the ProgramSubgraphInstanceRegionImpl builder that lowered a kSubgraphInstantiate DR op into the region (now unreachable — no such op is minted after P1.4)

**Reintroduction obligations.**
- The region-dispatch switch (Database.cpp:766) must remain total after the IsSubgraphInstance arm is removed — re-provide: no op emits the region, so the arm is dead; confirm the ProgramVisitor pure-virtual set no longer declares Visit(ProgramSubgraphInstanceRegion) (else every visitor breaks).
- A bound-query entry point must read the fully-materialized pub relation via the ordinary query cursor — re-provide: the plain (non-forcing) query entry point already exists for undemanded #query names (the cursor over the pub table); every bound query now takes that path since the forcing-function registry is empty (killed in P1.6). The fabricated-message ABI suppression (IsDemandMessage) is deleted because there are no fabricated messages to suppress.

**Anchors.** `include/drlojekyll/Runtime/InstanceStore.h:54`, `include/drlojekyll/ControlFlow/Program.h:847`, `include/drlojekyll/ControlFlow/Program.h:1432`, `include/drlojekyll/ControlFlow/Program.h:1539`, `lib/CodeGen/CPlusPlus/Database.cpp:766`, `lib/CodeGen/CPlusPlus/Database.cpp:2352`, `lib/CodeGen/CPlusPlus/Database.cpp:2434`, `lib/CodeGen/CPlusPlus/Database.cpp:3693`

---

### P1.6 — Delete the DataFlow demand side-record types and re-provide interior naming from the demand-free tiers

**Diff**

```diff
@@ include/drlojekyll/DataFlow/Query.h @@
-struct QueryDemandForcing (:985), GuardAnnotation (:1007), RecognizedSubgraph (:1045)
-using GuardAnnotationIndex (:25); QueryView::GuardAnnotationIndex()/kNoGuardAnnotation (:452-453)
-Query::DemandForcings()/GuardAnnotations()/RecognizedSubgraphs() accessors (:1093-1102)
-QueryImpl fields: demand_forcings / guard_annotations / recognized_subgraphs
-QueryViewImpl::guard_annotation_index field + folded-count bookkeeping

@@ lib/Regional/Planning.cpp @@
-CollectDemandInteriorDecls(query)            # :210 — reads RecognizedSubgraphs()->demanded_decl (Tier-1)
-ResolveInteriorSupport(query, decl)          # :249 — reads GuardAnnotations()+RecognizedSubgraphs()+live (Tier-1)
 DeriveRegionalCensus(query):                  # :384
-  request_ports = |query.DemandForcings()|    # :388
+  request_ports = 0                            # no forcings exist post-cut
   row_contracts = |CollectContractInserts|                       # R-STORE, KEEP
-               + |CollectDemandInteriorDecls|                    # Tier-1, DELETE
               + |CollectOriginInteriorDecls|                     # Tier-2 (OriginDecls), KEEP
 Build(query,log):                             # :439
-  REQUEST PORTS + region-internal lines: 1 per forcing (:445-...)   # demand__ internals gone
-  QUERY ABI routing: match redecl vs forcings[fi].query -> '-> R0 via P<k>' | permanent-root (:...)
+  every #query redecl routes '-> permanent-root' (no forcing to match)
-  CollectContractInserts skip rule: starts_with('demand__')     # :190 — no demand__ decls exist; drop the skip
-  Tier-1 contract loop (:628-634, ResolveInteriorSupport)
   Tier-2 contract loop (:649-655, ResolveOriginSupport)           # KEEP — demand-free naming authority

@@ lib/Regional/Format.cpp @@
-RegionalInternal (fabricated demand__ lines) render (:84-171)      # internals vector now always empty -> delete
```

**Invariant established.** The DataFlow IR owns no demand side-tables. Interior relation naming is exactly R-STORE (insert-materialized decls, CollectContractInserts) ∪ Tier-2 (OriginDecls-carried, demand-independent) — both already correct and demand-free. request_ports is a structural 0; every #query is a permanent observation root. V-REGION-CENSUS re-derives from this demand-free authority and still holds.

**Exit gate.** Grep proves zero references to GuardAnnotation/RecognizedSubgraph/QueryDemandForcing/GuardAnnotationIndex/DemandForcings across the tree. DataFlowValidators ctest passes. Every `-region-out` golden re-derives with request-ports=0 and row-contracts = |R-STORE|+|Tier-2| (the demand cases lose their Tier-1 E-line + request port; re-blessed in P1.7). V-REGION-CENSUS (Rel.cpp:4634) does not abort on any surviving case.

**Design goals resolved.**
- Goal 1 (four-authority separation): completes the removal of the collapsed authority — GuardAnnotation/RecognizedSubgraph were the side-records that reduced facts/derivations/requests to raw integers (§1.5). After this, the only fact authority a bound query sees is the materialized pub relation; nothing infers key semantics from demand messages, guard annotations, or query adornments (the prompt's post-Phase-1 requirement).
- Goal 3 (RequestEdge vs RuleActivationEdge): deletes QueryDemandForcing, the collapsed proto-'request' notion, clearing the ground for the typed RequestEdgeId/RuleActivationEdgeId that P3 introduces.

**Deletion obligations.**
- include/drlojekyll/DataFlow/Query.h: QueryDemandForcing (:985), GuardAnnotation (:1007), RecognizedSubgraph (:1045), GuardAnnotationIndex alias+kNoGuardAnnotation (:25,:452-453), the three Query accessors (:1093-1102), the QueryImpl demand vectors, QueryViewImpl::guard_annotation_index
- lib/DataFlow/Query.cpp: the deleted accessors' definitions (:298-316)
- lib/Regional/Planning.cpp: CollectDemandInteriorDecls (:210), ResolveInteriorSupport (:249), the request-port/region-internal minting loop (:445), the QUERY-ABI forcing-match routing, the demand__ skip in CollectContractInserts (:190), the Tier-1 contract loop (:628-634)
- lib/Regional/Format.cpp: the RegionalInternal (fabricated demand__) render path (:84-171); Regional.h RegionalInternal struct + FrozenRegionalProgram::internals vector

**Reintroduction obligations.**
- Tier-1 interior naming (the merge-materialized demanded relation that had no INSERT) is DELETED — re-provide is UNNECESSARY: post-cut nothing merge-materializes a demand relation, so that naming CATEGORY is vacuously empty. Interior naming reduces to R-STORE ∪ Tier-2, both pre-existing and demand-free; ResolveOriginSupport (:346, OriginDecls-based) is the surviving support-bit authority in place of the deleted ResolveInteriorSupport.
- DeriveRegionalCensus.request_ports must become the constant 0 (was |DemandForcings()|); the Build() request-port/internal loops delete; every #query ABI routes to permanent-root (re-provide the routing default).
- V-REGION-CENSUS (Rel.cpp:4634) and its stored context.frozen_census stay wired — they now recount a demand-free census; confirm no field reads DemandForcings().
- The K5 conservation belt (Build.cpp NDEBUG, 'every demanded interior's decl is origin-reachable') is now vacuous — delete or leave as a no-op; origin_decls (Tier-2 seed, Connect.cpp:1309) remains the live provenance.

**Anchors.** `include/drlojekyll/DataFlow/Query.h:985`, `include/drlojekyll/DataFlow/Query.h:1007`, `include/drlojekyll/DataFlow/Query.h:1045`, `include/drlojekyll/DataFlow/Query.h:452`, `lib/Regional/Planning.cpp:190`, `lib/Regional/Planning.cpp:210`, `lib/Regional/Planning.cpp:249`, `lib/Regional/Planning.cpp:384`, `lib/Regional/Planning.cpp:445`, `lib/Rel/Rel.cpp:4634`

---

### P1.7 — Test migration — repurpose answer witnesses, delete coupling/equivalence/registry tests, update runall.sh

**Diff**

```diff
@@ tests/OptDiff — REPURPOSE (were demand/RP-6 rejects) -> ordinary successful programs @@
-reject_key_no_bound_query_1 (rejects/)   # keyed local, no bound query -> compiles
-key_undemanded_1 / key_mismatch_1 / key_over_adorn_1 / key_multi_adorn_1  # were all-4-mode diagnostics
-key_multi_adorn_allfree_1                # key-triggered reject removed
+key_fenced_1  -> reused as a NEGATION/COMPLETENESS witness (not a demand-body feature-gap reject)

@@ tests/OptDiff/cases — SURVIVE as ANSWER witnesses (reuse .dr dataset; delete demand-shape goldens) @@
 key_tc_witness / key_neighborhood_witness / key_multi_adorn_witness:
-  DELETE their symlinked df/rel/ir/h/region/stdout/oracle/monotone goldens' PRAGMA-ACTIVATION role
-  DELETE .irgold/.probes demand-shape pins
+  keep .dr + .main.cpp; re-bless stdout as the FULL-MATERIALIZATION logical answer
+  new assertions: exact request ownership / canonical fact identity (per next-session-prompt tests 3,4,5,14)
 demand_tc_witness / demand_neighborhood_witness / demand_neighborhood_mono_witness /
 demand_diff_input_1 / demand_diff_pub_1 / demand_diff_neighborhood_witness / demand_multi_adorn_witness:
-  DELETE .drflags (-demand gone), .eqgate (flat==nested==golden meaningless), .irgold/.rel/.region/.behavioral demand goldens
+  answer under full materialization is IDENTICAL -> keep .dr as an answer witness OR delete as redundant with key_* twin

@@ tests/OptDiff — DELETE OUTRIGHT (feature-gap rejects of a deleted transform; equivalence/registry/census) @@
-demand_cyclic_1 / demand_recursive_content_1 / demand_two_queries_1 / demand_multi_adorn_1 /
-demand_multi_adorn_allfree_1 / demand_mutual_content_1 / demand_beside_mutual_1 / demand_beside_recursion_1 /
-demand_agg_body_1 / demand_kv_body_1 / demand_config_agg_body_1   # + all sidecars
-the two demand__ reserved-prefix collision rejects (F30 witnesses)

@@ tests/OptDiff — KEEP UNCHANGED (parser-shape, P1-untouched; Phase-0 owns them) @@
 key_unknown_1 / key_anon_1 / key_wildcard_1 / key_dup_1
 reject_key_double_1 / reject_key_empty_1 / reject_key_eof_1 / reject_key_literal_1 /
 reject_key_on_message_1 / reject_key_on_query_1 / reject_key_redecl_1 /
 reject_key_trailing_comma_1 / reject_key_unclosed_1

@@ tests/OptDiff/runall.sh — expected-diagnostic list @@
-drop: demand_multi_adorn_1, demand_cyclic_1, demand_recursive_content_1, demand_two_queries_1,
-      demand_agg_body_1, demand_kv_body_1, demand_config_agg_body_1, demand_mutual_content_1,
-      demand_multi_adorn_allfree_1, key_mismatch_1, key_multi_adorn_1, key_over_adorn_1,
-      key_multi_adorn_allfree_1, key_fenced_1, key_undemanded_1
 keep:  key_unknown_1, key_anon_1, key_dup_1, key_wildcard_1 (parser @key rejects), + non-key diagnostics
```

**Invariant established.** The suite pins only lasting language invariants (parser-shape @key obligations, ordered-path duplicate rejection, redeclaration consistency) and logical-ANSWER witnesses. No test pins flat-vs-nested equivalence, demand-mode IR shape, fabricated-message registries, or recognizer census — those are deleted as architectural requirements. runall.sh ends SUITE: PASS.

**Exit gate.** `DR=build/debug/bin/drlojekyll tests/OptDiff/runall.sh <workroot>` ends SUITE: PASS. The six RP-6 @key cases (key_undemanded/mismatch/over_adorn/multi_adorn/multi_adorn_allfree, and key_fenced repurposed) and the deleted demand_* diagnostics no longer appear in the expected-diagnostic list. key_tc_witness / key_neighborhood_witness / key_multi_adorn_witness produce the same logical answer as before (full materialization is answer-preserving vs demand) and their stdout goldens are re-blessed via runall.sh --bless after review. The four parser @key rejects (unknown/anon/wildcard/dup) and the reject_key_* corpus still exit 1 cleanly. InstanceStore ctest is gone.

**Design goals resolved.**
- Goal 2 (order-significant paths): reject_key_double_1 (exact-duplicate-path) is KEPT and reject_key_redecl_1 keeps the extended keyed/unkeyed/keyed sequence — the surviving witnesses that [A,B] and [B,A] are distinct while an exact repeat rejects (parser-side; the demand-side collapse witness set is deleted with the transform).
- Goal 1 (four-authority separation): reclassifies key_tc/neighborhood/multi_adorn witnesses from byte-equivalence/pragma-activation tests to logical-answer + canonical-fact-identity tests, matching the honest baseline (one fact authority, @key inert).

**Deletion obligations.**
- tests/OptDiff/cases + goldens for: demand_cyclic_1, demand_recursive_content_1, demand_two_queries_1, demand_multi_adorn_1, demand_multi_adorn_allfree_1, demand_mutual_content_1, demand_beside_mutual_1, demand_beside_recursion_1, demand_agg_body_1, demand_kv_body_1, demand_config_agg_body_1 (all sidecars: .dr/.drflags/.main.cpp/.batches/.probes/.eqgate/.irgold)
- the .drflags/.eqgate/.irgold/.rel/.region/.behavioral demand-shape goldens of demand_tc_witness, demand_neighborhood_witness, demand_neighborhood_mono_witness, demand_diff_input_1, demand_diff_pub_1, demand_diff_neighborhood_witness, demand_multi_adorn_witness
- the pragma-activation symlink goldens (df/rel/ir/h/region/stdout/oracle/monotone) of key_tc_witness/key_neighborhood_witness/key_multi_adorn_witness
- tests/OptDiff/rejects/reject_key_no_bound_query_1.dr + the two demand__ reserved-prefix collision rejects (F30)
- runall.sh expected-diagnostic entries for every deleted/repurposed demand_*/key_* case; the InstanceStore ctest registration

**Reintroduction obligations.**
- key_fenced_1's negation/completeness assertion must be re-provided as a plain program (it was a demand-body feature-gap reject; the dataset's negation semantics still evaluate under full materialization — re-author its .main.cpp to assert the complete relational answer).
- key_tc/neighborhood/multi_adorn .main.cpp drivers must be re-provided to assert logical answers + canonical fact identity (next-session-prompt tests 3,4,5,14,15) instead of flat==nested==golden; their stdout re-blessed only after review (never bless a red case green).
- reject_key_redecl_1 must retain the extended keyed/unkeyed/keyed sequence (Phase-0 landed it; P1 must not regress it).

**Anchors.** `tests/OptDiff/cases/demand_cyclic_1.dr`, `tests/OptDiff/cases/key_tc_witness.dr`, `tests/OptDiff/cases/key_fenced_1.dr`, `tests/OptDiff/cases/key_undemanded_1.dr`, `tests/OptDiff/rejects/reject_key_no_bound_query_1.dr`, `tests/OptDiff/rejects/reject_key_double_1.dr`, `tests/OptDiff/runall.sh`, `tests/InstanceStore`

---

## Cluster: P2 (make FrozenRegionalProgram authoritative) + P3 (exact RequestEdge / RuleActivationEdge relations over the full-materialization backend). Both diff the seed pseudocode (§1 current → §2/§3 target) and the companion §1 subsystem blocks. They assume the P1 destructive cut has already landed: ApplyDemandTransform / GuardAnnotation / RecognizedSubgraph / QueryDemandForcing / DRInstance / InstanceStore lowering / the -demand* flags are GONE, so at P2 entry Planning.cpp's demand-coupled naming (CollectDemandInteriorDecls, ResolveInteriorSupport) and the request-port/ABI-routing loops that read DemandForcings() are BROKEN and keyed evaluation is NON-FUNCTIONAL until P4. P2 discharges P1's naming/classification reintroduction obligation with typed planning records; P3 re-introduces exact request ownership (which P1 zeroed by deleting forcings) as a first-class RequestEdge relation over the canonical fully-materialized relations.

### P2 — Make FrozenRegionalProgram the typed semantic owner; retire the 5 render-string vectors, the Query-as-owner pass-through, the row_contracts friend leak, and the census re-derivation from Query

**Diff**

```diff
@@ Regional.h — the owned state of FrozenRegionalProgram (Regional.h:37-158) @@
 # typed id domains stay; the render-string vectors and the pass-through-as-owner go.
-struct RegionalAbi     { Kind kind; string decl_text; string route_text; }        // Regional.h:75
-struct RegionalPort    { Kind kind; unsigned port_index; string head_text; string fields_text; }  // :82
-struct RegionalInternal{ string text; }                                            // :97  (fabricated demand__ lines — already dead post-P1)
-struct RegionalPermanentRoot { string text; }                                      // :102
-struct RegionalContract{ unsigned edge_index; string rel_name; string member_key_text;
-                          string support_text; bool declared_key; }                 // :108 — member_key/support already TEXT
+// Four-authority-separated typed records. NO pre-rendered strings; Format.cpp derives text from these.
+struct RelationSchema {
+    RelationId               relation;              // decl.Id()-derived stable id
+    vector<SymbolicFieldId>  fields;                // AllFields, schema order (from decl params)
+    SemanticMemberKey        member_key;            // COPIED from RowContract at freeze (§Build), typed FieldIds not text
+    DeclaredAccessPathSet    declared_access_paths; // ORDERED paths from decl.InstanceKeys() (P0 item 4 identity)
+    DerivationSupport        support;               // {monotone|differential} — a classification, not a string
+}
+struct ResultPort  { PortId id; RelationId relation; vector<SymbolicFieldId> fields; }   // published surface
+struct InputPort   { PortId id; MessageId message; vector<SymbolicFieldId> fields; }      // received messages
+struct PermanentRootRecord { RelationId relation; }   // unforced query / undemanded observation root (TYPED, no text)
+// request_ports is EMPTY at P2 (forcings deleted by P1); P3 re-provides it from the RequestEdge relation.
+struct RegionTemplate {
+    RegionId                 id;                    // only RegionId(0) exists until Stage-C fan-out
+    vector<SymbolicFieldId>  inherited_fields;      // empty at R0
+    vector<RelationSchema>   relation_schemas;      // one per distinct non-demand INSERT/interior decl (3 tiers, typed)
+    RuleSet                  rules;                  // the region's dataflow slice, referenced not copied (see DataFlowGraph())
+    vector<RecursiveComponent> recursive_components; // predicate SCCs (from Stratify), typed — replaces the census-only view
+    vector<InputPort>        input_ports;
+    vector<ResultPort>       result_ports;
+    vector<PortId>           request_ports;         // EMPTY until P3
+    vector<PermanentRootRecord> permanent_roots;
+}
@@ Regional.h — class FrozenRegionalProgram (Regional.h:131-158) @@
-  const ::hyde::Query &Query(void) const;            // <<< the pass-through masquerading as semantic owner
-  const vector<RegionalAbi>          &Abis(void)  const;
-  const vector<RegionalPort>         &Ports(void) const;
-  const vector<RegionalInternal>     &Internals(void) const;
-  const vector<RegionalPermanentRoot>&PermanentRoots(void) const;
-  const vector<RegionalContract>     &Contracts(void) const;
- private:
-  ::hyde::Query query;                               // Regional.h:152 — the owner
-  vector<RegionalAbi> abis; vector<RegionalPort> ports; vector<RegionalInternal> internals;
-  vector<RegionalPermanentRoot> permanent_roots; vector<RegionalContract> contracts;   // :154-158
+  const vector<RegionTemplate> &Regions(void) const;   // THE semantic owner now
+  const RegionalCensus         &Census(void)  const;   // kept — a PROJECTION of Regions() (see DeriveRegionalCensus)
+  // Query is DEMOTED to an explicitly-named dataflow-graph carrier for the base
+  // dataflow->controlflow lowering; it is NO LONGER 'the semantic owner'. Named so
+  // grep proves no consumer treats it as a god-object.
+  const ::hyde::DataFlowGraph &DataFlowGraph(void) const;
+ private:
+  vector<RegionTemplate> regions;                     // exactly one (RegionId(0)) at Stage B
+  RegionalCensus census;                              // derived-from-regions cache
+  ::hyde::Query dataflow_graph;                        // renamed field; base lowering only

@@ Planning.cpp — FrozenRegionalProgram::Build (Planning.cpp:439) @@
 # Build STOPS emitting render strings and STOPS reaching demand side-tables; it
 # BUILDS one RegionTemplate of typed records. The RowContract friend read happens
 # ONCE here (the freeze hand-off), converted to typed member_key, then never again.
 Build(query, log):
-    out = FrozenRegionalProgram(query)                 // stores the Query as owner
-    forcings = query.DemandForcings()                  // DELETED type (P1)
-    for fi, entry in enumerate(forcings):              // request-port + demand__ internal loop
-        out.ports.push(RegionalPort{kRequest, fi, "query="+name, BoundParamNames})
-        out.internals.push(RegionalInternal{ ... "[fabricated, driver-suppressed]" })
+    R = RegionTemplate{ id = RegionId(0) }
+    // --- request ports: EMPTY. Forcings are gone (P1); requests are a P3 relation. ---
+    // (no demand__ internals: fabricated messages no longer exist)
     (received, published) = CollectMessages(query)     // UNCHANGED — real msgs, never demand-coupled
-    for m in received:  mint kInput  port + input_abi  "-> R0 via P<k>"
-    for m in published: mint kResult port + output_abi "-> R0 via P<k>"
+    for m in received:  R.input_ports.push(InputPort{ PortId(next++), m.Id(), FieldsOf(m) })
+    for m in published: R.result_ports.push(ResultPort{ PortId(next++), m.Id(), FieldsOf(m) })
-    // --- QUERY ABIs + permanent roots: routing matched redecl.BindingPattern against forcings ---
-    for parsed_query, redecl in UniqueRedeclarations:
-        matched = index fi s.t. forcings[fi].query==redecl AND BindingPattern matches
-        if matched>=0: abi.route_text = "-> R0 via P<matched>"          // demand-coupled routing
-        else:          abi.route_text="-> permanent-root"; permanent_roots.push(...)
+    // Post-P1 every bound query is just a materialized-relation reader => a permanent root.
+    // (P3 re-splits: a bound query becomes a RootLease owning a RequestEdge => a request port.)
+    for parsed_query in dedup-by-Id(sub-module walk):
+        R.permanent_roots.push(PermanentRootRecord{ RelationOf(parsed_query) })
     // --- ROW CONTRACTS -> typed RelationSchema, 3 tiers, ONE edge counter ---
-    row_contracts_map = query.impl->row_contracts       // Planning.cpp:574 — THE FRIEND LEAK
-    for (decl, ins) in CollectContractInserts(query):
-        rc = row_contracts_map[QueryView(ins).impl]      // abort if missing
-        key_text = "("+[decl.NthParameter(i).Name() ... i in rc.member_key]+")"
-        out.contracts.push(RegionalContract{edge++, decl.Name(), key_text,
-            view.CanReceiveDeletions()?"differential":"monotone", decl.HasInstanceKey()})
-    for decl in CollectDemandInteriorDecls(query):       // Tier-1: reads RecognizedSubgraphs (DELETED P1)
-        out.contracts.push(RegionalContract{edge++, decl.Name(), AllParamNames(decl),
-            ResolveInteriorSupport(query,decl)?"differential":"monotone", ...})
-    for decl in CollectOriginInteriorDecls(query):       // Tier-2: OriginDecls, dedups vs Tier-1
-        out.contracts.push(RegionalContract{edge++, decl.Name(), AllParamNames(decl),
-            ResolveOriginSupport(query,decl)?"differential":"monotone", ...})
+    rc_map = query.PublicRowContracts()                 // NEW public accessor replaces the impl-> friend reach
+    for (decl, ins) in CollectContractInserts(query):   // Tier R-STORE: UNCHANGED walk
+        rc = rc_map[QueryView(ins)]                      // abort (FROZEN-REGIONAL) if missing
+        R.relation_schemas.push(RelationSchema{
+            relation = RelationId(decl.Id()), fields = FieldsOf(decl),
+            member_key = rc.member_key,                  // TYPED SemanticMemberKey, no string render
+            declared_access_paths = InternDeclaredPaths(decl, decl.InstanceKeys()),  // ordered, P0-identity
+            support = view.CanReceiveDeletions() ? kDifferential : kMonotone })
+    for (decl, view) in CollectOriginInteriorDecls(query):   // Tier-2 SUBSUMES Tier-1 (see reintroduction note)
+        R.relation_schemas.push(RelationSchema{ RelationId(decl.Id()), FieldsOf(decl),
+            AllFieldsKey(decl), InternDeclaredPaths(decl, decl.InstanceKeys()),
+            ResolveOriginSupport(query,decl) ? kDifferential : kMonotone })
     // --- recursive components: TYPED, from Stratify (was census-only) ---
+    for scc in query.MultiViewStrata(): R.recursive_components.push(RecursiveComponent{scc})
     out.regions = [R]
     out.census = DeriveRegionalCensus(out.regions)      // <<< now over TYPED records, not the Query
     self-recount + V-FROZEN-NO-OPEN-PORT + V-OWNERSHIP-ACYCLIC   // UNCHANGED belts, retargeted at R
     return out

@@ Planning.cpp — DeriveRegionalCensus (Planning.cpp:384) @@
 # THE CENSUS BECOMES A PROJECTION OF THE TYPED RECORDS, not a re-walk of the Query.
-DeriveRegionalCensus(query):
-    request_ports = |query.DemandForcings()|                       // DELETED type
-    (received,published) = CollectMessages(query); input=|recv|; result=|pub|
-    row_contracts = |CollectContractInserts| + |CollectDemandInteriorDecls|   // demand-coupled
-                                              + |CollectOriginInteriorDecls|
-    return RegionalCensus{regions=1, child_calls=0, program_roots=1, ...}
+DeriveRegionalCensus(regions):                                     // <<< SIGNATURE CHANGE: takes typed regions
+    R = regions[0]
+    return RegionalCensus{ regions=|regions|, child_calls=0, program_roots=1,
+        request_ports=|R.request_ports|, input_ports=|R.input_ports|,
+        result_ports=|R.result_ports|, row_contracts=|R.relation_schemas| }

@@ Planning.cpp — DELETE the demand-coupled naming + support functions @@
-CollectDemandInteriorDecls(query)  // :210, reads query.RecognizedSubgraphs()->demanded_decl — TYPE GONE
-ResolveInteriorSupport(query,decl) // :249, reads GuardAnnotations()+RecognizedSubgraphs() — TYPES GONE
 # Tier-2 CollectOriginInteriorDecls (:298) + ResolveOriginSupport (:346) SURVIVE and
 # SUBSUME Tier-1: with demand deleted, every former demand-interior decl is now an
 # ordinary undemanded #local/#export interior, already named by OriginDecls(). Drop
 # the Tier-1 dedup arm inside CollectOriginInteriorDecls (nothing left to dedup against).

@@ Build.cpp — Program::Build unwrap (Build.cpp:1333-1337, 1572) @@
-  const ::hyde::Query &query = frozen.Query();          // :1337 — treats Query as semantic owner
+  const RegionTemplate &region = frozen.Regions()[0];   // regional semantics come from the typed record
+  const ::hyde::DataFlowGraph &dfg = frozen.DataFlowGraph();  // base dataflow->controlflow lowering ONLY
-  context.frozen_census = &frozen.Census();             // :1572
+  context.frozen_regions = &frozen.Regions();           // Rel recounts the TYPED records, not Query

@@ Rel.cpp — V-REGION-CENSUS-IDENTITY (Rel.cpp:4634-4662) @@
-  if (context.frozen_census) {
-    RegionalCensus expect = DeriveRegionalCensus(query);   // :4639 — RE-DERIVES FROM THE QUERY
-    for f in 7 fields: stored[f] != expect[f] -> abort()
-  }
+  if (context.frozen_regions) {
+    RegionalCensus expect = DeriveRegionalCensus(*context.frozen_regions);  // recount TYPED records
+    for f in 7 fields: stored[f] != expect[f] -> abort()
+  }

@@ seed §1.5 — the identity gap (RESOLVED for the regional layer) @@
-FrozenRegionalProgram is a RegionalCensus (7 counts) + RegionalAbi render-ready
-strings + a Query pass-through — it OWNS no regional semantics.
+FrozenRegionalProgram OWNS a vector<RegionTemplate> of typed records: RelationSchema
+(member_key: SemanticMemberKey, declared_access_paths: ordered DeclaredAccessPathSet,
+support: DerivationSupport), input/result ports, permanent roots, recursive_components.
+The census is a projection of these. Query is a named dataflow-graph carrier, not an owner.
+(Facts/derivations/requests remain integers/absent until P3.)
```

**Invariant established.** FrozenRegionalProgram is the sole owner of the regional planning semantics: every regional fact a downstream consumer needs (relation schema, member key, declared access paths, support classification, input/result ports, permanent roots, recursive components) is a typed record on a RegionTemplate, and the census is a pure projection of those records. No consumer re-derives regional shape from the optimized Query graph, reaches a demand side-table, or reads the query.impl->row_contracts friend field; formatting is the only thing that turns typed records into text, and it does so downstream of the freeze.

**Exit gate.** (1) The 180+ .region.<mode> goldens are byte-identical across the P2 refactor (they were re-blessed at P1 when demand vanished; P2 changes only HOW the same bytes are produced) — proven by `runall.sh` SUITE: PASS with no --bless. (2) `grep -rn 'frozen.Query()\|query.impl->row_contracts\|RecognizedSubgraph\|GuardAnnotation\|DemandForcings\|RegionalAbi\|RegionalPort\|RegionalContract' lib/Regional lib/Rel lib/ControlFlow` returns zero hits. (3) V-REGION-CENSUS-IDENTITY (Rel.cpp:4634) recounts the typed RegionTemplate vectors and still aborts a stubbed shell. (4) IdentityTypes ctest passes with RelationSchema.member_key typed as SemanticMemberKey (no string round-trip).

**Design goals resolved.**
- Goal 1 (four-authority separation) — PARTIAL/foundational: RelationSchema splits what a single RegionalContract render-string previously fused. Logical relation truth is named by member_key (SemanticMemberKey) and support (DerivationSupport); logical access path is declared_access_paths (ordered DeclaredAccessPathSet, order-significant per P0). The physical-access authority is deliberately ABSENT here (no index/scan field) so it cannot be accidentally collapsed into the schema — it is introduced only when codegen emits it (P4/P7). Residual specialization arrives in P3/P4.
- Goal 3 (RequestEdge vs RuleActivationEdge) — foundational only: request_ports becomes a typed, initially-empty field distinct from result_ports and permanent_roots, reserving the seam so P3 can populate it from the RequestEdge relation without reshaping the record. The two edge RELATIONS themselves are P3.
- Goal 7 (co-recursive key flow) — foundational: recursive_components is now a typed vector<RecursiveComponent> derived from Stratify's predicate SCCs (query.MultiViewStrata()) rather than a bare census integer, giving P6 a typed home for prefix-preserving-vs-key-changing SCC classification.

**Deletion obligations.**
- struct RegionalAbi (Regional.h:75) — deleted
- struct RegionalPort (Regional.h:82) — deleted (replaced by typed InputPort/ResultPort; request ports deferred to P3)
- struct RegionalInternal (Regional.h:97) — deleted (fabricated demand__ lines no longer exist post-P1)
- struct RegionalPermanentRoot (Regional.h:102) — deleted (replaced by typed PermanentRootRecord)
- struct RegionalContract (Regional.h:108) — deleted (replaced by RelationSchema; the declared_key bool folds into declared_access_paths non-emptiness)
- FrozenRegionalProgram::Query() accessor + the `::hyde::Query query` owner field (Regional.h:152) — deleted; the field is renamed dataflow_graph and exposed only via DataFlowGraph()
- the five owned vectors abis/ports/internals/permanent_roots/contracts (Regional.h:154-158) — deleted
- CollectDemandInteriorDecls (Planning.cpp:210) — deleted (reads the P1-deleted RecognizedSubgraphs())
- ResolveInteriorSupport (Planning.cpp:249) — deleted (reads the P1-deleted GuardAnnotations()+RecognizedSubgraphs())
- the friend leak `query.impl->row_contracts` (Planning.cpp:574) — deleted; replaced by the public query.PublicRowContracts() accessor
- the DeriveRegionalCensus(const Query&) signature (Planning.cpp:384 / Regional.h:127) — replaced by DeriveRegionalCensus(const vector<RegionTemplate>&)
- the Query re-derivation inside V-REGION-CENSUS (Rel.cpp:4639 `DeriveRegionalCensus(query)`) — replaced by a recount of context.frozen_regions

**Reintroduction obligations.**
- DISCHARGES P1's obligation: P1 deleted GuardAnnotation/RecognizedSubgraph/QueryDemandForcing, breaking interior-relation NAMING (CollectDemandInteriorDecls) and support CLASSIFICATION (ResolveInteriorSupport) and the request-port/ABI routing that read DemandForcings(). P2 re-provides naming via RelationSchema built from the surviving OriginDecls()-based Tier-2 path (which now subsumes the former Tier-1 demand-interior set, since every former demand interior is an ordinary undemanded interior post-P1) and classification via RelationSchema.support = ResolveOriginSupport(...). Request-port routing is NOT re-provided here — it was deleted with forcings and is re-introduced by P3's RequestEdge relation.
- P2 itself creates a gap it defers: request_ports is left empty and every bound query is rendered a permanent root, so the request/result ownership split (P1 removed by deleting forcings) is only STRUCTURALLY reserved. P3 must re-provide request-port population by deriving it from the RequestEdge relation (a bound query becomes a RootLease owning a RequestEdge), re-splitting permanent-root vs request-port.

**Anchors.** `include/drlojekyll/Regional/Regional.h:37-158`, `include/drlojekyll/Regional/Regional.h:127`, `lib/Regional/Planning.cpp:210`, `lib/Regional/Planning.cpp:249`, `lib/Regional/Planning.cpp:298`, `lib/Regional/Planning.cpp:346`, `lib/Regional/Planning.cpp:384`, `lib/Regional/Planning.cpp:439`, `lib/Regional/Planning.cpp:574`, `lib/Regional/Planning.cpp:576-687`, `lib/DataFlow/RowContract.h:38`, `lib/DataFlow/RowContract.h:51`, `lib/DataFlow/Identity.h:52`, `lib/ControlFlow/Build/Build.cpp:1333`, `lib/ControlFlow/Build/Build.cpp:1337`, `lib/ControlFlow/Build/Build.cpp:1572`, `lib/Rel/Rel.cpp:4634-4662`, `lib/Rel/Rel.cpp:4639`

---

### P3 — Introduce the RequestEdge / RuleActivationEdge / FactDerivation / RoutedResult relations over full materialization; one empty-binding state per live region instance; activation restricted to the acyclic slice

**Diff**

```diff
@@ Regional.h — new typed identity domains + the two edge relations (after the P2 records) @@
 # Dense handles are lowered artifacts; these typed ids are the identity authority (Medium finding).
+struct RootLeaseId    { unsigned v; auto operator<=>(...) = default; }   // a bound-query / external root
+struct PermanentRootId{ unsigned v; ... }                                 // an unforced observation root
+struct BindingStateId { RegionInstanceId region_instance; BindingStateSchemaId schema;
+                        SortedBoundFieldValues canonical_bindings; }       // P3: only the EMPTY-binding state exists
+struct RegionalFactId { RegionInstanceId region_instance; RelationId relation;
+                        SemanticMemberIdentity member; }                   // member = schema-order typed member key
+struct RequestOwnerId = RootLease(RootLeaseId) | PermanentRoot(PermanentRootId)
+                      | RegionalMember(RegionalFactId)                     // parent-member ownership (cross-region, P6+)
+struct RequestEdgeId  { RequestOwnerId owner; CallSiteId call_site; BindingStateId destination_state; }
+struct RuleActivationEdgeId { BindingStateId source_state; RegionalFactId source_fact;
+                             RuleId rule; BindingStateId destination_state; }
+struct FactDerivation { DerivationId id; BindingStateId source_binding_state;
+                        RegionalFactId fact; DerivationSupportCount support; }  // split signed support
+struct RoutedResultId { RequestEdgeId request_edge; RegionalFactId fact; }        // CALLER-QUALIFIED copy
 # The two edge relations are NEVER unified: request_edges is an acyclic forest keyed
 # by exact owner; activation_edges is a derivation dependency (may cycle — but P3
 # populates only the acyclic slice; P6 lifts it to SCCs).
+struct RegionInstanceRelations {   // hangs off the single live RegionInstanceId(RegionId(0))
+    map<RequestEdgeId>        request_edges;      // EXACT ownership; forest
+    map<RuleActivationEdgeId> activation_edges;   // acyclic slice only in P3
+    map<RegionalFactId, FactDerivation> derivations;
+    set<RoutedResultId>      routed_results;
+}

@@ RegionTemplate.request_ports — populated (was empty at P2) @@
 # A bound query is a RootLease owning a RequestEdge to the region's empty-binding state;
 # its result port re-splits from the P2 permanent-root list.
-    for parsed_query: R.permanent_roots.push(PermanentRootRecord{ RelationOf(parsed_query) })   // P2
+    for parsed_query in dedup-by-Id(sub-module walk):
+        if IsBound(parsed_query):
+            lease = RootLeaseId(next_lease++)
+            R.request_ports.push(PortId(next_port++))                 // request port re-provided (P2 gap closed)
+            seed_request_edge(lease, EmptyBindingState(R))            // exact owner -> empty state
+        else:
+            R.permanent_roots.push(PermanentRootRecord{ RelationOf(parsed_query) })

@@ seed §2.2 — the two edge relations, made real over full materialization @@
 # §2.2's target pseudocode was cyclic-complete; P3 realizes it RESTRICTED to the
 # acyclic slice, over the P1 full-materialization backend (no InstanceStore).
 AddRequestEdge(owner, call_site, dest_state):                 // owner in {RootLease|PermanentRoot|RegionalMember}
     e = intern(request_edges, (owner, call_site, dest_state))
+    // EXACT OWNERSHIP: a 2nd owner requesting dest_state interns a DISTINCT edge; facts are NOT re-derived.
     return e
 RemoveRequestEdge(e):
-    request_edges.erase(e)                                     // liveness re-derived, not refcounted
+    request_edges.erase(e)
+    // CALLER-QUALIFIED retract (retained RegionalDataFlowCore.md invariant): drop ONLY this owner's
+    // routed copies; the shared RegionalFact and every other owner's RoutedResult survive.
+    for rr in routed_results where rr.request_edge == e: routed_results.erase(rr)
 DeriveActivationEdge(source_state, source_fact, rule, dest_state):
-    intern_or_drop(activation_edges, (source_state, source_fact, rule, dest_state))
+    // P3 RESTRICTION: assert dest_state is not back-reachable to source_state in activation_edges
+    //   (acyclic slice — a cyclic derivation is a P6 obligation, rejected/deferred here, NOT silently cycled).
+    assert !ActivationReachable(dest_state, source_state)
+    intern_or_drop(activation_edges, (source_state, source_fact, rule, dest_state))
 RootedReachability(request_edges, activation_edges):
     live = { e.dest_state for e in request_edges if RootAlive(e.owner) }
-    repeat to fixpoint:                                         // §2.2 cyclic form
-        for a in activation_edges:
-            if a.source_state in live and Present(a.source_fact): live.add(a.dest_state)
+    // P3: activation_edges is a DAG, so a single topological sweep suffices (no fixpoint yet).
+    for a in ActivationTopoOrder(activation_edges):
+        if a.source_state in live and Present(a.source_fact): live.add(a.dest_state)
     return live

@@ seed §2.3 — AddDerivation over canonical full-materialization facts @@
 AddDerivation(source_binding_state, fact_id, delta):
     d = derivations_of(fact_id)
-    d.support += delta                                         // split signed support
+    d.support += delta                                         // split signed support (SUPPORT != OWNERSHIP)
     if d.support crosses 0->>0: publish born
     if d.support crosses >0->0: publish dies
+    // Two binding maps may BOTH support one RegionalFactId; RegionalFactRelation still holds ONE member.
+    // FullScanFilter over the P1-materialized relation supplies the facts (no path-private row copy).

@@ seed §3 — EvaluateEpoch, restricted to the acyclic slice @@
 EvaluateEpoch(input_deltas, request_deltas):
     old = SnapshotCommittedOutputs()
     ApplyInputDeltas(input_deltas)
     for rd in request_deltas: Add/RemoveRequestEdge(rd)       // exact ownership deltas
-    repeat to JOINT LEAST FIXPOINT:                            // §3 cyclic form (P6)
-        live = RootedReachability(request_edges, activation_edges)
-        for st in live: ... DeriveActivationEdge ... RouteResults(st)
+    // P3: acyclic. One rooted-reachability sweep, then one topo pass of rule evaluation.
+    live = RootedReachability(request_edges, activation_edges)
+    for st in ActivationTopoOrder-restricted(live):
+        for rule in rules_of(region_of(st)):
+            for delta in EvaluateRule(rule, st): AddDerivation(st, delta.fact, delta.sign)
+        for f in new_facts(st): DeriveActivationEdge(st, f, matching_rule(f), dest_state_of(f))
+        RouteResults(st)
     RetractRoutedResults(all_states \ live)
-    RetireUnreachableSCCs()                                    // §3 cyclic form (P6)
+    // RetireUnreachableSCCs is a NO-OP in P3 (no activation cycles exist yet); the seam is
+    // reserved for P6 and MUST drain routed removals before retiring — never a refcount.
     Publish(Difference(old, CurrentCommittedOutputs())); Seal()
 RouteResults(st):
     for e in request_edges where e.dest_state == st:
         for f in canonical_facts_of(st):
-            emit RoutedResult(e, f)                             // caller-qualified
+            routed_results.insert(RoutedResultId{e, f})        // CALLER-QUALIFIED (retained invariant):
+            // a LATE requester (a 2nd AddRequestEdge to an already-derived st) receives the
+            // already-derived facts as its OWN RoutedResults; its later RemoveRequestEdge retracts
+            // only those copies (the RemoveRequestEdge arm above), never the shared fact.

@@ seed §1.5 — the identity gap (further RESOLVED) @@
-(Facts/derivations/requests remain integers/absent until P3.)
+RegionalFactId/FactDerivation/RequestEdgeId/RuleActivationEdgeId/RoutedResultId are typed
+identities on RegionInstanceRelations; dense runtime handles are lowered artifacts of these,
+never the identity. Support (FactDerivation.support) is distinct from ownership (RequestEdgeId);
+one requester's removal retracts only its RoutedResults. Only the empty-binding state exists
+(keyed BindingStateIds are P4/P5); activation is the acyclic slice (cyclic SCCs are P6).
```

**Invariant established.** Exact request ownership and derivation dependency are two distinct relations that never collapse: request_edges is an acyclic ownership forest whose edges are keyed by an exact RequestOwnerId (RootLease | PermanentRoot | RegionalMember), and activation_edges is a derivation-dependency DAG (acyclic slice in P3). Membership of a RegionalFact is decided by live FactDerivations (split signed support), which is provably distinct from who owns the request for it. A second requester of an already-live state attaches a distinct RequestEdge and receives its own caller-qualified RoutedResults without duplicating a single RegionalFact; removing one requester erases only that requester's RoutedResults and its RequestEdge, leaving the shared fact and every other requester's results intact. Dense runtime handles are lowerings of these typed ids, never the identity.

**Exit gate.** A directed test battery over the full-materialization backend (repurposed key_* datasets per next-session-prompt Test migration items 3,4,5,14,15,18,19) passes: (a) every root lease, permanent observation, and (P6-reserved) parent member resolves to exactly one RequestEdge owner — assert |owners(dest_state)| matches the distinct AddRequestEdge count, no fact duplicated (RegionalFactRelation holds one member per RegionalFactId regardless of requester count); (b) issuing a 2nd AddRequestEdge to a live state adds RoutedResults but adds zero FactDerivations (item 14/18); (c) RemoveRequestEdge on one of two requesters retracts only that edge's RoutedResults and leaves the fact present (item 15/19 — the late-requester/caller-qualified property); (d) support-vs-ownership: a fact with live derivations but no live request root is retained as a fact yet routes nowhere; a requested state with no derivation routes nothing — assert the two are independently observable; (e) a DeriveActivationEdge that would close a cycle trips the P3 acyclic-slice assert (proving cyclic activation is deferred, not silently admitted). No InstanceStore, no scan-labelled-as-walk (P1 deleted them; P4 adds honest FullScanFilter).

**Design goals resolved.**
- Goal 3 (RequestEdge vs RuleActivationEdge distinct) — FULLY: the two are separate typed relations (RequestEdgeId keyed by exact RequestOwnerId forming an acyclic forest; RuleActivationEdgeId a derivation dependency), never unified into one edge set, discharging the 'do not let an internal activation edge become a request owner' false-start and the retained exact-RequestEdgeId-ownership invariant (multiple owners, late attachment, caller-qualified results, drain-before-retire).
- Goal 4 (rooted-reachability liveness, not refcount) — PARTIAL: RootedReachability computes liveness from live request roots through activation edges (a topological sweep in P3's acyclic slice); RemoveRequestEdge re-derives liveness rather than decrementing a count, and RetireUnreachableSCCs is a reserved no-op that MUST drain routed removals in P6 — establishing the non-refcount discipline before cycles exist. The full cyclic rooted least-fixpoint is P6.
- Goal 1 (four-authority separation) — advances: FactDerivation.support (support authority) is held distinct from RequestEdgeId (ownership authority) and from RegionalFactId (logical-truth authority); a residual specialization is represented as a BindingStateId (here only the empty state), reserving the residual-specialization authority as its own typed domain rather than a physical structure.

**Deletion obligations.**
- seed §3 EvaluateEpoch 'repeat to JOINT LEAST FIXPOINT' loop — replaced (in P3) by a single rooted-reachability sweep + one topo evaluation pass; the fixpoint form is a P6 obligation, not deleted permanently but explicitly out-of-slice
- seed §3 RetireUnreachableSCCs active body — reduced to a reserved no-op in P3 (no activation cycles exist); the SCC-retirement-after-routed-removals-drain logic is a P6 obligation
- seed §2.2 RootedReachability 'repeat to fixpoint' — replaced by ActivationTopoOrder single pass in the acyclic slice
- the P2 blanket 'every bound query -> permanent root' arm (Planning.cpp Build permanent-root loop) — a bound query now becomes a RootLease + RequestEdge + request port

**Reintroduction obligations.**
- RE-PROVIDES the request-port authority that P1 deleted (by removing DemandForcings) and that P2 left as an empty typed field: request_ports is now populated from the RequestEdge relation (a bound query is a RootLease owning a RequestEdge to the region's empty-binding state), re-splitting the P2 permanent-root list into request-owning roots vs permanent observation roots — closing the gap P2's reintroduction note flagged.
- P3 itself defers (and MUST be re-provided later): keyed/non-empty BindingStateIds (only the empty-binding state exists here) are a P4/P5 obligation — until then a keyed @key read routes through the empty state's FullScanFilter (P4); cyclic RuleActivationEdges and the joint rooted least-fixpoint + SCC retirement are a P6 obligation; the physical AccessPlan selection over these facts is P7.

**Anchors.** `include/drlojekyll/Regional/Regional.h:37-158`, `lib/Regional/Planning.cpp:439`, `lib/DataFlow/Identity.h:52`, `docs/proposals/RegionalDataFlowCore.artifacts/keyed-rewrite-pseudocode-seed.md:249-273`, `docs/proposals/RegionalDataFlowCore.artifacts/keyed-rewrite-pseudocode-seed.md:277-292`, `docs/proposals/RegionalDataFlowCore.artifacts/keyed-rewrite-pseudocode-seed.md:311-350`, `include/drlojekyll/Runtime/InstanceStore.h:54-219`, `lib/CodeGen/CPlusPlus/Database.cpp:2434-2494`

---

## Cluster: Phase 4 (honest complete-path specialization) + Phase 5 (partial-binding DAG)

### P4 — Honest complete-path specialization — retire the kSectionWalk lie, re-provide keyed relation-local evaluation over one canonical fact relation with a FullScanFilter label that matches emitted code

**Diff**

```diff
@@ §2.4 SelectAccessPlan — the physical-access-structure AUTHORITY (goal 1), NOT a reuse of the join Lowering enum @@
-SelectAccessPlan(req) -> AccessPlan ∈ {FullScanFilter | FullKeyHashLookup |
-                                       ExistingTriePrefix | EnumeratePrefix |
-                                       BuildLazyOrdering}:
-    # FullScanFilter is ALWAYS a correct realization (a capability guarantee,
-    # not a perf promise). A non-scan plan is chosen ONLY when codegen EMITS it.
-    # INVARIANT (Phase 4): the label MUST match the emitted code — a scan is
-    # never labelled a walk. This retires the kSectionWalk placeholder.
+SelectAccessPlan(req) -> AccessPlan:            # NEW Rel-IR domain (lib/Rel/Rel.h,
+    # sibling of, DISTINCT from, the join-scan Lowering{kPointTest,kSectionWalk,
+    # kFullScan,kSeek} at Rel.h:483 — that enum STAYS honest for real
+    # idx.First/Next join range walks, Rel.cpp:2432). AccessPlan is the physical
+    # access-structure authority; members {kFullScanFilter, kFullKeyHashLookup,
+    # kExistingTriePrefix, kEnumeratePrefix, kBuildLazyOrdering}.
+    assert req.completeness == CompleteRelation or req.op is a named ActiveSubset
+    plan = kFullScanFilter                       # the ONLY realized member @P4
+    # kFullScanFilter is a capability guarantee, never a perf promise; a non-scan
+    # member is returned ONLY when codegen EMITS it (deferred to P7).
+    return plan
+# V-PLAN-HONEST (NEW always-on belt at the ValidateDROps tail, the emission-
+# matching SUCCESSOR to the retired label-only V-ALPHA arm A, Rel.cpp:4413-4490):
+#   every keyed-access node's AccessPlan label == its codegen emission —
+#   kFullScanFilter <=> the full-scan+key-equality-filter mold; a
+#   kFullKeyHashLookup node exists ONLY beside an emitted Find()+hash.
+#   @P4: assert count(kFullKeyHashLookup)==0 and NO keyed access carries
+#   Lowering::kSectionWalk. (V-ALPHA validated the label was self-consistent,
+#   never that it matched emission — the exact hole this belt closes.)

@@ §3 EvaluateRule — a keyed relation-local read lowers THROUGH the honest plan @@
     for st in live:                              # semi-naive, per binding state
         for rule in rules_of(region_of(st)):
-            for delta in EvaluateRule(rule, st): # AccessPlan per read (§2.4)
-                AddDerivation(st, delta.fact, delta.sign)        # §2.3
+            for read in reads_of(rule):
+                req  = AccessRequirement(read.relation, st.bindings,
+                                         read.required_fields, CompleteRelation)
+                read.plan = SelectAccessPlan(req)          # @P4 == kFullScanFilter
+            for delta in EvaluateRule(rule, st): # each read drives its labelled plan
+                AddDerivation(st, delta.fact, delta.sign)        # §2.3 ONE authority

@@ §4 Phase 4 — honest complete-path specialization (relation-local, NONRECURSIVE slice) @@
-request R through declared path P:
-    st = BindingStateId from P's field/value bindings
-    evaluate st with AccessPlan = FullScanFilter        # honest label, no kSectionWalk lie
-    add FactDerivations → canonical RegionalFactIds
-    route distinct canonical facts through exact RequestEdges
-one canonical fact relation regardless of path count; FullKeyHashLookup only
-when codegen emits one.
+# RE-PROVIDE the keyed evaluation P1 deleted (BuildSubgraphInstanceOps
+# Rel.cpp:1038-1200 = kSubgraphInstantiate/kInstanceDeath/kInstanceSeal;
+# emit_instance_rescan Database.cpp:2434-2494; the InstanceStore leaf), now
+# driven by Phase-2 RelationSchema.declared_access_paths + Phase-3 RequestEdges/
+# FactDerivations, NEVER recovered guard JOINs.
+EvaluateKeyedRequest(RequestEdge e):             # e.dest_state came from declared path P
+    st   = e.dest_state                          # a BindingStateId (order-free values)
+    req  = AccessRequirement(st.relation, st.bindings, all_fields, CompleteRelation)
+    plan = SelectAccessPlan(req)                 # == kFullScanFilter
+    for row in FullScanFilter(canonical_input(st.relation), st.bindings):
+        fid = RegionalFactId(st.region_instance, st.relation, MemberKey(row)) # §2.3
+        AddDerivation(st, fid, +1)               # into the ONE RegionalFactRelation
+    RouteResults(st)                             # distinct canonical facts -> e (§3)
+# INVARIANTS: exactly ONE RegionalFactRelation per relation regardless of
+# |declared paths| — paths/plans REFER to RegionalFactIds, never a path-private
+# Row copy (retires the per-complete-key InstanceStore double buffer as a fact
+# owner). The keyed access is minted with AccessPlan::kFullScanFilter at the
+# successor of the deleted Rel.cpp:1146 site; Format renders "full-scan-filter";
+# the Database.cpp:2339-2341 caveat ("a full scan with a key filter — the keyed
+# index is a deferred perf refinement; the DR spine already tags section-walk")
+# is DELETED — the label is now literally true. kFullKeyHashLookup is selected
+# only when P7 codegen emits one.
```

**Invariant established.** Every keyed-access node's physical-plan label equals its codegen emission (label==emission), enforced by the always-on V-PLAN-HONEST belt; the keyed-access label domain is AccessPlan (whose sole realized member is kFullScanFilter, mapping exactly to the full-scan-with-key-filter mold), disjoint from the join-scan Lowering enum; and there is exactly ONE RegionalFactRelation per relation regardless of how many @key access paths reference it.

**Exit gate.** The nonrecursive relation-local keyed slice (key_tc_witness / key_neighborhood_witness datasets repurposed as ANSWER witnesses, not equivalence/pragma witnesses) evaluates to the correct canonical answer in all 4 optimization modes; a structural grep over every .rel and -region-out golden shows NO keyed access rendered `section-walk` and every keyed access rendered `full-scan-filter`; V-PLAN-HONEST asserts count(kFullKeyHashLookup)==0; a relation carrying N declared @key paths emits exactly ONE fact Table_ in generated C++ (fact-count-invariant test, roadmap test 14 first half); and the Database.cpp:2339-2341 honesty caveat no longer exists in the tree.

**Design goals resolved.**
- Goal 5 (honest FullScanFilter) — FULLY: the reintroduced keyed access carries AccessPlan::kFullScanFilter whose codegen emission IS the full-scan+key-equality-filter mold (Database.cpp:2434-2494 shape preserved); Lowering::kSectionWalk is retired from the keyed domain (kept only for honest idx.First/Next join walks at Rel.cpp:2432); V-PLAN-HONEST upgrades the referee from label-self-consistency (old V-ALPHA arm A) to label==emission; FullKeyHashLookup is selectable only when codegen emits a hash lookup, which it does not yet, so it is never chosen.
- Goal 1 (four-authority separation) — PARTIALLY: introduces AccessPlan as its OWN physical-access-structure domain, kept distinct from the logical-access-path (declared_access_paths) and from the join Lowering enum, so the physical structure never masquerades as the logical path or as canonical fact identity.

**Deletion obligations.**
- Lowering::kSectionWalk as a KEYED-access label — the reintroduced keyed access node (successor of lib/Rel/Rel.cpp:1146) must set AccessPlan::kFullScanFilter, never Lowering::kSectionWalk; the enum value itself survives ONLY for honest join range walks at lib/Rel/Rel.cpp:2432-2433.
- The honesty-caveat comment at lib/CodeGen/CPlusPlus/Database.cpp:2339-2341 ("a full scan with a key filter — the keyed index is a deferred perf refinement; the DR spine already tags section-walk").
- The label-ONLY self-consistency belt V-ALPHA arm A (lib/Rel/Rel.cpp:4413-4490, the kInstanceKeySlot⇒{kPointTest,kSectionWalk} check) — retired by P1; it must NOT be re-provided as a label-only check, it is replaced by the emission-matching V-PLAN-HONEST.
- The per-complete-key InstanceStore double-buffered Row cache (frozen[iid]/current[iid], include/drlojekyll/Runtime/InstanceStore.h:141-145) as a FACT OWNER — any residual after P1 must not be reintroduced; facts live solely in RegionalFactRelation.

**Reintroduction obligations.**
- P1 deleted keyed relation-local evaluation entirely (BuildSubgraphInstanceOps Rel.cpp:1038-1200; emit_instance_rescan Database.cpp:2434-2494; the special InstanceStore lowering), leaving @key inert and bound queries reading full materialized relations. P4 MUST re-provide keyed evaluation, now driven by Phase-2 declared_access_paths + Phase-3 RequestEdges/FactDerivations (NOT recovered GuardAnnotation/RecognizedSubgraph JOINs, which no longer exist), minting into ONE canonical RegionalFactRelation.
- P1 deleted the physical full-scan emission (the emit_instance_rescan mold). P4 MUST re-provide the full-scan-with-key-filter emission (byte-shape may be preserved) but labelled AccessPlan::kFullScanFilter and rendered "full-scan-filter".
- P1 deleted the InstanceStore-as-second-fact-owner. P4 MUST re-provide the routing of distinct canonical facts through exact Phase-3 RequestEdges so results reach requesters WITHOUT a path-private fact copy, preserving the one-canonical-relation invariant across any declared-path count.

**Anchors.** `lib/Rel/Rel.h:483`, `lib/Rel/Rel.cpp:1146`, `lib/Rel/Rel.cpp:2432`, `lib/Rel/Rel.cpp:4413`, `lib/Rel/Format.cpp:325`, `lib/CodeGen/CPlusPlus/Database.cpp:2339`, `lib/CodeGen/CPlusPlus/Database.cpp:2434`

---

### P5 — The partial-binding DAG — order-significant BindingEdge over order-free BindingStateId, prefix sharing (@key(A) ⊂ @key(A,B)), convergence ([A,B]/[B,A] → one {A,B} state), lazy declared-or-visited materialization

**Diff**

```diff
@@ §2.1 InternDeclaredPaths — derive the ordered prefix chain, lazily (goal 6) @@
 InternDeclaredPaths(relation, raw_paths):            # written pragma order irrelevant
     seen = set()
     for p in raw_paths:
         canon = tuple(p.ordered_fields)              # NO sort — order IS identity
         if canon in seen: REJECT "exact duplicate access path"   # P0 item 4
         seen.add(canon)
     assign KeyPathId deterministically AFTER the full set is validated
+    # P5: materialize the DECLARED prefix DAG. Each ordered path induces its
+    # prefix chain empty -> {f0} -> {f0,f1} -> ...; convergent paths share the
+    # schemas/states/edges. LAZY: intern only DECLARED (here) or VISITED
+    # (EvaluateRule/DeriveActivationEdge below) states — never the power set.
+    for p in raw_paths:
+        prev = BindingStateSchema(region, EMPTY)              # region-empty binding
+        acc  = []
+        for f in p.ordered_fields:                            # ORDER significant HERE
+            acc.append(f)
+            cur = BindingStateSchema(region, SET(acc))        # order-FREE key -> SHARED
+            InternBindingEdge(parent=prev, added_field=f, child=cur)  # ordered nav edge
+            prev = cur
+    # @key(A) and @key(A,B): both start f=A -> the SAME BindingStateSchema{A}
+    #   and the SAME edge (empty --A--> {A}); @key(A,B) only ADDS ({A}--B-->{A,B}).
+    # [A,B] and [B,A]: DISTINCT edge chains (empty--A-->{A}--B-->{A,B} vs
+    #   empty--B-->{B}--A-->{A,B}) both terminating at the SAME schema {A,B}.

@@ §2.1 BindingStateSchema / BindingStateId / BindingEdge — two authorities, never collapsed (goal 2) @@
-BindingStateId(region_instance, bound_field_value_map):
-    schema = BindingStateSchema(region_of(region_instance),
-                                canonical_field_SET(bound_field_value_map))
-    return intern(binding_state_table,
-                  (region_instance, schema, sort_by_field(bound_field_value_map)))
-    # [A=a,B=b] reached via path [A,B] and via [B,A] intern to the SAME id.
+BindingStateSchema(region, field_set):           # ORDER-FREE, values-FREE
+    return intern(schema_table, (region, sorted(field_set)))   # the field SET is the key
+BindingStateId(region_instance, bound_field_value_map):
+    schema = BindingStateSchema(region_of(region_instance),
+                                canonical_field_SET(bound_field_value_map))
+    return intern(binding_state_table,
+                  (region_instance, schema, sort_by_field(bound_field_value_map)))
+    # ORDER-FREE endpoint: [A=a,B=b] via [A,B] and via [B,A] -> SAME id.
+InternBindingEdge(parent, added_field, child):   # ORDER-SIGNIFICANT navigation
+    return intern(binding_edge_table, (parent, added_field, child))
+    # The ordered edge is the ONLY authority carrying traversal order; order
+    # NEVER keys a schema or a state. This SUPERSEDES the path-level order-
+    # collapse of SameKeySetOfSets (Parser.cpp:1477-1488, sorts intra-key AND
+    # inter-key) and its Demand.cpp:902-905 twin (dead post-P1): order is lifted
+    # to the EDGE authority, convergence is kept at the STATE authority — the two
+    # are never collapsed (goal 2).

@@ §2.4 AccessRequirement — an unbound read requires the COMPLETE specialization, never enumerate-only-active @@
 AccessRequirement(relation, available_ordered_bindings, required_fields,
                   completeness ∈ {CompleteRelation | ActiveSubset})
     # An ordinary unbound read REQUIRES CompleteRelation. ActiveSubset is legal
     # ONLY for an explicitly-named active-subset operation (retained: "do not
     # implement an ordinary unbound read by enumerating only active states").
+    # P5: a CompleteRelation read is satisfied by DESCENDING the prefix DAG to a
+    # materialized parent state and scanning the residual, OR by FullScanFilter
+    # over canonical facts (§2.4/P4) — NEVER by treating the set of already-
+    # materialized (visited) binding states AS the relation. The visited-state
+    # set is a cache of navigation, not the membership oracle.

@@ §4 Phase 5 — the partial-binding DAG (lazy, prefix-sharing, convergent) @@
-intern BindingState by (region_instance, canonical field set, values);
-create ordered BindingEdge only for DECLARED or VISITED paths.
+# RE-PROVIDE the "same binding SET converges" recognition that P0 removed with
+# SameKeySetOfSets's order-collapse (Parser.cpp:1477) — now at the STATE
+# authority, not the PATH authority — and GENERALIZE P4's single terminal
+# BindingStateId into the full prefix DAG P4 deferred.
+intern BindingStateSchema by (region, field SET)                 # order-free (goal 2)
+intern BindingStateId     by (region_instance, schema, sorted values)
+intern BindingEdge        by (parent_state, ordered added_field, child_state)  # order-signif.
+materialize a schema/state/edge ONLY when a DeclaredAccessPath names it (the
+  prefix chain above) OR evaluation VISITS it (a subset/non-prefix read the
+  worklist reaches); unvisited subsets stay UNMATERIALIZED (no power-set closure,
+  no eager subset lattice).
+all states/edges REFER to canonical RegionalFactIds (§2.3) — multiple physical
+  indexes/paths NEVER duplicate a fact or a derivation.
```

**Invariant established.** Binding-state identity is order-free (BindingStateSchema keyed on the field SET; BindingStateId on schema + sorted values) while access-path navigation is order-significant (BindingEdge keyed on the ordered (parent_state, added_field)) — the two authorities are never collapsed. Declared prefixes and convergent paths share schemas/states/edges; only declared-or-visited states materialize; and every state/edge refers to a canonical RegionalFactId so no physical index or path duplicates a fact.

**Exit gate.** A binding-state dump (-region-out extension) shows @key(A) and @key(A,B) sharing ONE {A} BindingStateSchema id and ONE empty--A-->{A} BindingEdge; [A,B] and [B,A] show TWO ordered edges (A-added and B-added) into ONE {A,B} BindingStateId; a @key(A,B,C) relation never evaluated at {A,B} shows NO {A,B} state materialized (unvisited-subset test, roadmap test 16); an unbound read before any keyed state is materialized returns the complete relational answer (test 13); two convergent paths do not duplicate facts/derivations and retracting one path's derivation preserves a fact still supported by another (tests 14, 15); reordering two @key pragmas produces a byte-identical declaration contract (test 7).

**Design goals resolved.**
- Goal 2 (order-significant paths WITH order-free binding-state identity) — FULLY: BindingEdge is interned on the ORDERED (parent, added_field) so [A,B] and [B,A] are distinct edge chains, while BindingStateSchema/BindingStateId are interned on the order-free field SET (+ sorted values) so both chains terminate at ONE {A,B} state. Order lives only on the edge authority; it never keys a state. This is the correct replacement for the SameKeySetOfSets sort-collapse (Parser.cpp:1477, Demand.cpp:902) that P0-item-4 removed — convergence moves from the path layer (wrong) to the state layer (right).
- Goal 6 (partial-binding DAG) — FULLY: DeriveDeclaredPrefixChain interns a schema+state+edge per PREFIX of each declared path, so @key(A) shares the {A} prefix of @key(A,B); materialization is LAZY — a state/edge exists only if DECLARED or VISITED, so unvisited subsets never materialize and the power set is never closed; subset/non-prefix declared paths add only the alternate edges evaluation visits; and all states/edges REFER to canonical RegionalFactIds, so multiple physical indexes never duplicate facts.

**Deletion obligations.**
- The order-collapsing set-of-sets equality as the IDENTITY mechanism: SameKeySetOfSets's canon lambda (lib/Parse/Parser.cpp:1477-1488) sorts intra-key (std::sort, destroying [A,B]!=[B,A]) and inter-key (std::set). P0-item-4 already removes the intra-key sort from the parser dup-check; P5 must additionally forbid any order-collapsing SET from ever keying a BindingStateSchema/BindingStateId at the regional layer.
- The Demand.cpp:902-905 canon copy (the RP-10 bijection's sorted std::set<std::vector<unsigned>>) — already dead after P1's demand cut; P5 must NOT resurrect a sorted set-of-sets as a binding-state key anywhere.
- The InstanceStore complete-key-only namespace: keys[iid] = the COMPLETE α-bound key tuple with NO prefix states (include/drlojekyll/Runtime/InstanceStore.h:141, 54-219) — replaced by the interned prefix DAG (schema_table / binding_state_table / binding_edge_table).
- Any eager power-set or subset-lattice closure over possible bindings (forbidden by the roadmap "do not eagerly materialize the power set") — the interning is strictly declared-or-visited.

**Reintroduction obligations.**
- P0-item-4 deleted SameKeySetOfSets's order-collapse (lib/Parse/Parser.cpp:1477), which was the ONLY mechanism recognizing [A,B] and [B,A] as reaching the same endpoint — but it did so wrongly, at the PATH layer, discarding order. P5 MUST re-provide that convergence recognition at the binding-STATE authority (the order-free BindingStateId) while keeping the ordered paths distinct via BindingEdge, so [A,B]!=[B,A] as paths yet both reach one {A,B} state.
- P4 provided only the TERMINAL (complete-path) BindingStateId for the nonrecursive relation-local slice, deliberately deferring prefixes and partial states. P5 MUST re-provide the partial-binding layer P4 deferred: the prefix schemas/states, prefix sharing (@key(A) reusing @key(A,B)'s {A}), and the ordered navigation edges that converge — without introducing a second fact owner (P4's one-canonical-relation invariant is preserved).

**Anchors.** `lib/Parse/Parser.cpp:1477`, `lib/DataFlow/Demand.cpp:902`, `include/drlojekyll/Runtime/InstanceStore.h:54`, `lib/Regional/Planning.cpp:201`, `lib/Regional/Format.cpp:908`

---

## Cluster: P6 — recursive regional execution (resolves design goals 4 rooted-reachability-liveness and 7 co-recursive-key-flow). Six coherent sub-cuts P6.1–P6.6 on the §2/§3 target pseudocode of keyed-rewrite-pseudocode-seed.md, built on P2 typed RegionTemplate/RuleRoutingProjection/recursive_components, P3 RequestEdge/RuleActivationEdge/FactDerivation, P4 FullScanFilter specialization, P5 partial-binding DAG. Retains next-session-prompt invariants: RequestEdge acyclic forest ≠ RuleActivationEdge (may cycle), drain-before-retire, caller-qualified routed results, epoch result independent of queue order, Stratify unstratified-negation/aggregation rejects (Stratify.cpp:272-344) UNWEAKENED. Supersedes: recursion-stays-in-one-instance, no-cyclic-binding-routing.

### P6.1 — Query-independent predicate SCC / recursive-component computation

**Diff**

```diff
@@ §2.1 identity construction — ADD ComputeRecursiveComponents (populate P2's RegionTemplate.recursive_components) @@
+ # Predicate SCCs over the REGIONAL RULE GRAPH, with ZERO query input. Nodes =
+ # RelationSchema (per RegionTemplate.relation_schemas); edges = rule body->head
+ # deps: a rule `H :- ... B_i ...` contributes edge B_i.relation -> H.relation.
+ # This REPLACES the query-COUPLED recursion recognition the deleted demand path
+ # used (ViewSelfReachable on a guard-JOIN lead side + forcing_index bucketing,
+ # Build.cpp:1439-1519; gone in P1).
+ ComputeRecursiveComponents(region_template):
+     g = rule_dependency_graph(region_template.rules)          # RelationSchema-level
+     sccs = TarjanCondense(g)                                  # REUSE the iterative
+         # frame-stack Tarjan of QueryImpl::Stratify (Stratify.cpp:178-232),
+         # SOURCES-FIRST pop = topological condensation, but over RelationSchema
+         # nodes, NOT VIEW* — a SEPARATE graph from the DataFlow-view SCC that
+         # Stratify keeps for negation/aggregation stratification.
+     region_template.recursive_components =
+         [ scc for scc in sccs if len(scc)>1 or has_self_edge(scc) ]
+     # An SCC MAY contain DIFFERENTLY-KEYED siblings (p@key(K) with q@key(X)):
+     # legal (goal 7). Query adornments contribute NOTHING to this partition.
@@ §2.1 — NOTE the two SCC domains never merge @@
+ # DataFlow-view SCC (Stratify.cpp:124) => stratification + induction MERGEs
+ #   (Induction.cpp / InductionGroupId): UNCHANGED, still the negation/agg
+ #   soundness authority (Stratify.cpp:272-344 rejects UNWEAKENED).
+ # Regional recursive_components (this fn) => keyed residual recursion planning.
```

**Invariant established.** RegionTemplate.recursive_components is a pure function of RegionTemplate.rules — byte-identical under any, or no, bound query; the DataFlow-view SCC (Stratify) and the regional predicate SCC are distinct graphs and the negation/aggregation stratification rejects are unweakened.

**Exit gate.** For the co-recursive example (p(K,X)@key(K), q(X,K)@key(X), p:-q, q:-p) with NO bound #query, ComputeRecursiveComponents returns {p,q} as one nontrivial component; a directed unit assertion shows adding OR removing a bound query leaves the partition byte-identical; Stratify.cpp:272-344 still rejects an SCC-crossing negate/aggregate (existing agg_in_scc_1/evm_func_parse goldens unchanged).

**Design goals resolved.**
- Goal 7 (co-recursive key flow): the SCC partition ADMITS differently-keyed predicates in one component and is computed with no query adornment input, satisfying 'differently keyed predicates in one recursive SCC are legal ... do not need to agree with a query adornment'.
- Goal 3 (edge-relation distinctness, groundwork): recursive_components is a predicate-dependency partition, kept separate from RequestEdge ownership.

**Deletion obligations.**
- The P3 latent assumption that RegionTemplate.recursive_components is always empty (P3 restricted activation to the nonrecursive/acyclic slice and never populated it) — no live symbol, but the acyclic-only path in P4/P5 planning that reads recursive_components==∅ is retired here.
- (already P1-deleted, must stay deleted) the query-coupled recursion detection: ViewSelfReachable-on-guard-JOIN + forcing_index bucketing at lib/ControlFlow/Build/Build.cpp:1439-1519.

**Reintroduction obligations.**
- Re-provides recursion DETECTION for keyed evaluation that the deleted demand path performed via ViewSelfReachable/forcing buckets — now as a query-free RelationSchema-level Tarjan condensation feeding recursive_components.

**Anchors.** `docs/proposals/RegionalDataFlowCore.artifacts/keyed-rewrite-pseudocode-seed.md:§2.1 (SymbolicFieldId block)`, `lib/DataFlow/Stratify.cpp:124 (Stratify entry)`, `lib/DataFlow/Stratify.cpp:178-232 (iterative Tarjan to reuse)`, `lib/DataFlow/Stratify.cpp:272-344 (unstratified negation/agg rejects — retained)`, `lib/DataFlow/Induction.cpp (InductionGroupId / induction MERGE)`, `lib/ControlFlow/Build/Build.cpp:1439-1519 (query-coupled fences, deleted P1)`, `RegionTemplate.recursive_components (P2 typed record)`

---

### P6.2 — Typed edge-local RuleRoutingProjection + schema-symbolic promotion (keep relation fields distinct)

**Diff**

```diff
@@ §2.1 SymbolicFieldId — implement the DEFERRED promotion (the comment's 'ONLY in Phase 6') @@
  SymbolicFieldId(relation, ordinal) -> intern per (RelationId, ordinal)
-     # promotion to a shared symbolic identity happens ONLY in Phase 6 rule
-     # analysis, and ONLY when EVERY producer of both fields proves the mapping.
+ # DEFAULT = per-rule EDGE-LOCAL projection. A shared variable spelling in a rule
+ # proves an edge-local VALUE equality, NOT schema equivalence (retained:
+ # 'parameter spelling is not proof of agreement').
+ RuleRoutingProjection(rule) -> { (src: SymbolicFieldId, dst: SymbolicFieldId) }
+     # POPULATE P2's typed record directly from head/body variable occurrences of
+     # `rule`. Fields of DISTINCT relations keep DISTINCT ids (retained: 'keep
+     # relation fields distinct'). This is the ONLY bridge across an activation
+     # edge unless promotion proves a global identity.
+ PromoteSharedSymbolicField(f_a, f_b, region_template):
+     producers = producers_of(rel_of(f_a)) ∪ producers_of(rel_of(f_b))
+     if producers nonempty and every rule in producers carries the identical
+        (f_a <-> f_b) pair in its RuleRoutingProjection:
+         union(symbolic_field_classes, f_a, f_b)     # ONE schema-level symbol
+     else:
+         keep distinct                                # edge-local projection only
@@ §2.2 DeriveActivationEdge — re-key through the DESTINATION relation's projection @@
  DeriveActivationEdge(source_state, source_fact, rule, dest_state):
+     # dest_state's bound field/values are the RuleRoutingProjection image of
+     # source_fact's fields under `rule` — NOT a raw column-position copy.
      intern_or_drop(activation_edges, (source_state, source_fact, rule, dest_state))
```

**Invariant established.** Two relations' fields share one SymbolicFieldId iff EVERY producer rule of both relations proves the mapping; otherwise cross-relation routing is carried edge-locally by per-rule RuleRoutingProjection, and mere shared-variable co-occurrence never collapses field identities.

**Exit gate.** For the co-recursive example the routing p.K↔q.K and p.X↔q.X across p:-q / q:-p is materialized as per-rule RuleRoutingProjections; a directed unit assertion shows p's and q's SymbolicFieldId classes stay DISTINCT (co-occurrence alone does not union them) while a hand-constructed all-producers-agree case DOES promote — proving both arms of PromoteSharedSymbolicField.

**Design goals resolved.**
- Goal 1 (four-authority separation): logical-fact field identity (SymbolicFieldId) is kept distinct from the edge-local access projection; a rule's typed source->dest mapping is the residual-specialization bridge, never a fact-schema merge.
- Goal 7 (co-recursive key flow): the typed edge-local projection is exactly what carries key-CHANGING routing (p keyed on K into q keyed on X) between binding states.

**Deletion obligations.**
- Any spelling/position-based field-collapse heuristic implied by the raw-integer column model that P1-P5 inherited — replaced by RuleRoutingProjection + the all-producers-agree promotion gate.

**Reintroduction obligations.**
- Re-provides cross-relation field routing that the deleted demand SIP walk performed implicitly via guard-JOIN pivot positions (Demand.cpp locate-guard-sites), now as an explicit typed per-rule projection.

**Anchors.** `docs/proposals/RegionalDataFlowCore.artifacts/keyed-rewrite-pseudocode-seed.md:§2.1 (SymbolicFieldId), §2.2 (DeriveActivationEdge)`, `RuleRoutingProjection (P2 typed record; next-session-prompt.md:524-528)`, `next-session-prompt.md:200-206 ('parameter spelling is not proof of agreement')`, `lib/DataFlow/Demand.cpp (locate-guard-sites pivot positions, deleted P1)`

---

### P6.3 — Prefix-preserving fusion into one binding-state-local fixpoint (same-key co-recursion)

**Diff**

```diff
@@ §3 evaluation — ADD PlanRecursiveComponent (fusion arm) @@
+ PlanRecursiveComponent(scc, region_template):
+     routes = [ RuleRoutingProjection(r) for r in rules_within(scc) ]   # P6.2
+     P = CommonPreservedPrefix(routes)                                  # bound-field set
+     if P nonempty and every route's dest binding prefix == P:
+         return FusedFixpoint(binding_prefix = P)
+             # ALL scc relations evaluate INSIDE ONE BindingStateId keyed on P
+             # (BindingStateId interning from §2.1 / P5). The semi-naive frontier
+             # is PREFIX-LOCAL: one fixpoint spans the whole SCC, not one per
+             # relation — REUSE the induction MERGE frontier machinery
+             # (Induction.cpp) restricted to the P-keyed BindingState.
+     else:
+         return JointFixpoint(scc)                                      # P6.4
+ CommonPreservedPrefix(routes):
+     return intersection over routes of the bound-field set carried UNCHANGED
+            source_state -> dest_state
@@ §3 EvaluateEpoch — dispatch each recursive component @@
      repeat to JOINT LEAST FIXPOINT:
+         for scc in region_of(st).recursive_components:
+             plan = PlanRecursiveComponent(scc, region_template)   # Fused | Joint
          live = RootedReachability(request_edges, activation_edges)
```

**Invariant established.** A recursive SCC whose every RuleRoutingProjection preserves a common bound-field prefix P evaluates as ONE binding-state-local semi-naive fixpoint keyed on P — the SCC's relations share a single BindingStateId per P-value, never a per-relation residual store.

**Exit gate.** Same-key co-recursion (p(K,X)@key(K), r(K,Y)@key(K), p:-r, r:-p) produces ONE BindingStateId per K value spanning BOTH p and r; a `.batches` witness asserts (a) exactly one fixpoint frontier per K (not two), and (b) the residual answer byte-equals the full-materialization baseline established at P1 across all 4 opt modes and the oracle.

**Design goals resolved.**
- Goal 7 (co-recursive key flow): implements 'if recursive edges preserve a common binding prefix, fuse the SCC into one binding-state-local residual fixpoint' — the same-key arm — replacing the SUPERSEDED recursion-stays-in-one-instance rule with true prefix fusion.

**Deletion obligations.**
- The P1-deleted single-full-scan InstanceStore rebuild (lib/Rel/Rel.cpp:1038-1200 BuildSubgraphInstanceOps + Database.cpp:2434-2494 emit_instance_rescan) must NOT be reintroduced — fusion runs a semi-naive frontier, not one O(NumRows) rescan per touched key.
- The SUPERSEDED 'recursion stays in one instance' constraint (next-session-prompt supersession matrix).

**Reintroduction obligations.**
- Re-provides residual recursive evaluation for the prefix-preserving keyed shape that the deleted cyclic_demand/recursive_content fences (Build.cpp:1439-1519) REJECTED — e.g. the key_tc_witness recursive transitive-closure dataset now evaluates residually rather than falling back to flat full materialization.

**Anchors.** `docs/proposals/RegionalDataFlowCore.artifacts/keyed-rewrite-pseudocode-seed.md:§3 (EvaluateEpoch)`, `BindingStateId (§2.1; P5 partial-binding DAG)`, `lib/DataFlow/Induction.cpp (semi-naive frontier machinery to reuse)`, `lib/Rel/Rel.cpp:1038-1200 (single-scan rebuild, deleted P1)`, `lib/CodeGen/CPlusPlus/Database.cpp:2434-2494 (full-scan mold, deleted P1)`, `tests/OptDiff/cases/key_tc_witness.*`

---

### P6.4 — Key-changing recursion as a cyclic RuleActivationEdge graph between binding states

**Diff**

```diff
@@ §2.2 RuleActivationEdge — LIFT the Phase-3 acyclic restriction @@
  DeriveActivationEdge(source_state, source_fact, rule, dest_state):
-     # [Phase 3] activation restricted to the nonrecursive/acyclic slice.
+     # activation edges MAY CYCLE inside one region (SUPERSEDES
+     # no-cyclic-binding-routing). A key-CHANGING recursive edge routes
+     # source_state's fact THROUGH the destination relation's declared key
+     # (via RuleRoutingProjection, P6.2) into a DIFFERENT BindingStateId.
      intern_or_drop(activation_edges, (source_state, source_fact, rule, dest_state))
+     # INVARIANT (retained goal 3): BOTH endpoints share ONE lexical
+     # RegionInstanceId; this edge is NEVER a RequestEdge — RequestEdges stay an
+     # ACYCLIC ownership forest (§2.2 AddRequestEdge), disjoint from this cycle.
@@ §3 — ADD JointFixpoint (the key-changing arm from PlanRecursiveComponent) @@
+ JointFixpoint(scc):
+     # Build the activation graph over BINDING STATES, not relations. For the
+     # co-recursive example p(K,X)@key(K), q(X,K)@key(X), p:-q, q:-p the routing
+     # graph is
+     #     (p,{K=k}) --q:-p--> (q,{X=x}) --p:-q--> (p,{K=k'}) --> ...
+     # each edge RE-KEYS the head tuple through the dest relation's declared path
+     # (goal 2: [K] and [X] are ordered paths reaching order-free {K}/{X} states).
+     # These states form a CYCLE in activation_edges — legal. Evaluate by the
+     # joint worklist (P6.5).
```

**Invariant established.** A key-changing recursive SCC is a cycle of RuleActivationEdges connecting DISTINCT BindingStateIds (one per (relation, declared-key value)); RequestEdge ownership remains an acyclic forest and no RuleActivationEdge is ever consulted as a RequestEdge.

**Exit gate.** For the co-recursive example the activation graph contains the cycle (p,{K})→(q,{X})→(p,{K}); a directed unit assertion checks (a) the RequestEdge relation stays acyclic (V-OWNERSHIP-ACYCLIC-style, Planning.cpp:711 spirit), and (b) an activation edge is never accepted by AddRequestEdge — the two relations are type-disjoint at the API.

**Design goals resolved.**
- Goal 7 (co-recursive key flow): implements 'if edges change bindings, run a fixpoint across a graph of binding states' and 'represent key-changing recursion as RuleActivationEdge dependencies between binding states'.
- Goal 3 (RequestEdge vs RuleActivationEdge distinct): the cycle lives ONLY in activation edges; ownership stays acyclic — retained invariant explicitly preserved.
- Goal 2 (order-significant paths, order-free states): each re-key uses the ordered declared path yet lands on the order-free binding-state endpoint.

**Deletion obligations.**
- Phase-3's acyclic-slice guard on DeriveActivationEdge (the '[Phase 3] activation restricted to the nonrecursive/acyclic slice' precondition).
- (already P1-deleted, must stay deleted) the cyclic_demand fence (Build.cpp:1439-1519) and the demand-body-walk reject of un-witnessed recursive shapes (Demand.cpp Step-3).

**Reintroduction obligations.**
- Re-provides the key-changing recursive capability the demand body walk and cyclic_demand fence rejected (demand_cyclic_1) — now as legal cyclic activation between differently-keyed binding states, not a diagnostic.

**Anchors.** `docs/proposals/RegionalDataFlowCore.artifacts/keyed-rewrite-pseudocode-seed.md:§2.2 (DeriveActivationEdge, AddRequestEdge), §3 (JointFixpoint site)`, `next-session-prompt.md:208-235 (co-recursive key flow, the p/q example)`, `lib/ControlFlow/Build/Build.cpp:1439-1519 (cyclic_demand, deleted P1)`, `lib/Regional/Planning.cpp:711 (V-OWNERSHIP-ACYCLIC — RequestEdge-forest referee to keep)`

---

### P6.5 — Joint rooted-reachability + semi-naive worklist to least fixpoint

**Diff**

```diff
@@ §3 EvaluateEpoch — make the JOINT fixpoint concrete and interleaved @@
-     repeat to JOINT LEAST FIXPOINT:
-         live = RootedReachability(request_edges, activation_edges)   # §2.2
-         for st in live:                              # semi-naive, per binding state
-             for rule in rules_of(region_of(st)):
-                 for delta in EvaluateRule(rule, st): # AccessPlan per read (§2.4)
-                     AddDerivation(st, delta.fact, delta.sign)        # §2.3
-             for f in new_facts(st):
-                 DeriveActivationEdge(st, f, matching_rule(f), dest_state_of(f))
-             RouteResults(st)
+     # JOINT because liveness and derivation are MUTUALLY recursive: a new fact
+     # in a live state creates an activation edge that makes a NEW state live,
+     # which derives more facts. One semi-naive worklist drives BOTH.
+     worklist = { seed states of live RequestEdges }
+     repeat to LEAST FIXPOINT (worklist empty):
+         live = RootedReachability(request_edges, activation_edges)   # cyclic-safe §2.2
+         st = worklist.pop()
+         if st not in live: continue                  # ROOTED-GATED: derive only while rooted
+         for rule in rules_of(region_of(st)):
+             for delta in EvaluateRule(rule, st):     # SEMI-NAIVE: new-frontier tuples only
+                 changed = AddDerivation(st, delta.fact, delta.sign)  # §2.3
+                 if changed:
+                     for (rule2, dest) in RouteFactForward(delta.fact, st):  # P6.2 projection
+                         DeriveActivationEdge(st, delta.fact, rule2, dest)
+                         worklist.push(dest)          # newly-activatable binding state
+         RouteResults(st)
+     # RESULT == fresh evaluation from committed inputs + live request roots
+     # (the DELETION CONTRACT, seed §3). Support counts are CACHES, never the
+     # membership/reachability oracle.
@@ §2.2 RootedReachability — annotate cyclic-safety @@
  RootedReachability(request_edges, activation_edges):
      live = { e.dest_state for e in request_edges if RootAlive(e.owner) }
-     repeat to fixpoint:
+     repeat to fixpoint:                              # cyclic-safe: monotone add-only
          for a in activation_edges:
              if a.source_state in live and Present(a.source_fact):
                  live.add(a.dest_state)
      return live
```

**Invariant established.** The epoch result is the LEAST fixpoint of (rooted-reachability ∘ semi-naive derivation), computed by one monotone worklist, and is independent of worklist/queue order; a binding state derives facts only while it is rooted-reachable.

**Exit gate.** Different-key co-recursion (the p/q example) CONVERGES to the published surface of the full-materialization baseline (P1), byte-identical across all 4 opt modes AND the bin/Oracle derivation-counter oracle AND the I0 RefInterp definitional evaluator (a `.batches` + `.probes` witness); a worklist-order-perturbation variant produces identical output (order-independence assertion).

**Design goals resolved.**
- Goal 4 (rooted-reachability liveness): the fixpoint is gated on RootedReachability every step — a state that loses its root stops deriving, and liveness is re-derived, not counted.
- Goal 7 (co-recursive key flow): 'run the joint rooted-reachability and semi-naive fact worklist to a least fixpoint' — different-key co-recursion converges.

**Deletion obligations.**
- The P3/P4/P5 single-pass topological evaluation of activation edges (which assumed an acyclic DAG of binding states) — replaced by the fixpoint worklist; the acyclic-DAG traversal must not survive.
- The seed's non-interleaved 'for st in live' outer loop that separated reachability from derivation into two phases (unsound once activation edges cycle).

**Reintroduction obligations.**
- Re-provides fixpoint termination that the differential UPDATECOUNT zero-crossing machinery gave the old flat lowering (StackSafeNegation.md §5.1) — here as monotone rooted-reachability + semi-naive least-fixpoint over binding states.

**Anchors.** `docs/proposals/RegionalDataFlowCore.artifacts/keyed-rewrite-pseudocode-seed.md:§3 (EvaluateEpoch), §2.2 (RootedReachability)`, `bin/Oracle/Main.cpp (derivation-counter oracle referee)`, `bin/RefInterp (I0 definitional least-fixpoint referee)`, `docs/proposals/StackSafeNegation.md §5.1 (fixpoint termination discipline to mirror)`

---

### P6.6 — Deletion: retire unreachable activation SCCs only after routed removals drain (NOT refcount)

**Diff**

```diff
@@ §3 EvaluateEpoch — DRAIN before RETIRE @@
      RetractRoutedResults(all_states \ live)
-     RetireUnreachableSCCs()                          # ONLY after routed removals drain
+     drained = DrainRoutedRemovals(all_states \ live)
+         # emit EVERY RoutedResult retraction for now-unreachable states' facts
+         # FIRST. A routed copy is CALLER-QUALIFIED (retained): a late
+         # requester's removal retracts ONLY its own routed copies, never the
+         # shared RegionalFact while another owner still requests it.
+     RetireUnreachableSCCs(drained)                   # then collect the dead SCC
      Publish(Difference(old, CurrentCommittedOutputs()))
      Seal()
@@ §3 RetireUnreachableSCCs — reachability, never refcount @@
  RetireUnreachableSCCs():
      # A self-supporting activation SCC with no live root: retire the WHOLE SCC,
-     # but ONLY after every routed removal for its facts drains. NEVER via a
-     # reference count (retained).
+     # but ONLY after DrainRoutedRemovals for its facts completes. Liveness is
+     # RE-DERIVED (RootedReachability, P6.5), NEVER counted: an internal
+     # activation cycle keeps its OWN support counts > 0 (each edge supports the
+     # next), yet is DEAD once no live RequestEdge reaches it. A refcount scheme
+     # would DEADLOCK — mutual support pins both counts positive forever
+     # (goal 4). Removing the last root => RootedReachability drops the whole
+     # cycle => drain => retire.
@@ §3 — the removing-the-sole-root scenario (co-recursive cycle) @@
+ # p:-q, q:-p with ONE RootLease requesting (p,{K=k}). p supports q supports p;
+ # internal FactDerivation.support stays > 0 on both. Remove the RootLease:
+ #   RootedReachability(request_edges', activation_edges) = {}  (no live root)
+ #   DrainRoutedRemovals({(p,{K=k}),(q,{X=x})}) -> retract routed copies
+ #   RetireUnreachableSCCs -> collect the {p,q} activation SCC
+ # A counter-based collector never fires here — that is the bug goal 4 forbids.
```

**Invariant established.** An unrooted self-supporting activation SCC is retired ONLY after its routed removals fully drain (drain-before-retire, retained); retirement is driven by re-derived rooted-reachability, and a positive internal support count is NEVER sufficient to keep a cycle live.

**Exit gate.** A `.batches`/oracle witness on the co-recursive cycle: after landing the SCC under one RootLease, removing that sole lease drives the published surface to empty (the whole {p,q} cycle drains); a directed assertion shows FactDerivation.support on both relations is still > 0 at removal time (proving reachability, not the counter, decided retirement); a late-second-requester variant confirms removing one requester leaves the shared child alive (caller-qualified drain).

**Design goals resolved.**
- Goal 4 (rooted-reachability liveness, NOT refcount): implements 'a self-supporting cycle with no live root is inactive' via reachability re-derivation, with the explicit refcount-deadlock counter-scenario as the witness; honors the retained drain-before-retire ordering.

**Deletion obligations.**
- The InstanceStore leaf's append-only 'an iid, once minted, is retained for the program's life' MONOTONE-FOREVER retention model (include/drlojekyll/Runtime/InstanceStore.h:54-219) — recursive retirement requires real drain+collect, so the leaf-cache retention cannot be the liveness authority.
- (already P1-deleted) the acyclic single-key kInstanceDeath death arm (lib/Rel/Rel.cpp) which handled only non-cyclic single-key removal.

**Reintroduction obligations.**
- Re-provides deletion/retraction for CYCLIC keyed data that the old kInstanceDeath (-1) arm never handled — the acyclic death arm only retracted a single-key InstanceStore's frozen set (Database.cpp band-(a0)); P6.6 re-provides retraction across a self-supporting binding-state cycle.

**Anchors.** `docs/proposals/RegionalDataFlowCore.artifacts/keyed-rewrite-pseudocode-seed.md:§3 (RetireUnreachableSCCs, RetractRoutedResults, RouteResults)`, `include/drlojekyll/Runtime/InstanceStore.h:54-219 (append-only leaf, retention model deleted)`, `lib/Rel/Rel.cpp (kInstanceDeath -1 arm, deleted P1)`, `lib/CodeGen/CPlusPlus/Database.cpp:2511-2530 (band-(a0) single-key death, deleted P1)`, `next-session-prompt.md:232-235 (removing the last root retires a self-supporting activation cycle after routed removals drain)`

---

## Cluster: Phases 7-9 — physical access planning (P7) / lazy tries + induced orderings (P8) / automatic path inference (P9): the forward-looking backend that consumes the P2-P6 typed regional model and turns AccessRequirement into honest, codegen-honored physical access.

### P7 — Separate physical access planning: AccessRequirement -> explicit, codegen-honored AccessPlan

**Diff**

```diff
@@ §2.4 SelectAccessPlan (seed L302-309) — retire the P4 unconditional stub @@
 SelectAccessPlan(req) -> AccessPlan in {FullScanFilter | FullKeyHashLookup |
                                        ExistingTriePrefix | EnumeratePrefix |
                                        BuildLazyOrdering}:
-    # FullScanFilter is ALWAYS a correct realization (a capability guarantee,
-    # not a perf promise). A non-scan plan is chosen ONLY when codegen EMITS it.
-    # INVARIANT (Phase 4): the label MUST match the emitted code.
-    # (P4 backend leaves this a STUB: returns FullScanFilter unconditionally.)
-    return FullScanFilter
+    # P7: real dispatch, gated on what codegen can EMIT. NEVER selects a plan
+    # whose kind is absent from CodegenPlanCapabilities -> falls back to scan.
+    if req.completeness == CompleteRelation and req.available_ordered_bindings == []:
+        return FullScanFilter                      # honest: emits the linear scan
+    s = LookupPhysicalStructure(req.relation, req.available_ordered_bindings)
+    if s is HashArrangement and s.key_fields == set(req.available_ordered_bindings)
+       and FullKeyHashLookup in CodegenPlanCapabilities:
+        return FullKeyHashLookup(s)                 # emits arr.Find(key), O(1)
+    if s is TriePrefixIndex and is_prefix(req.available_ordered_bindings, s.ordered_prefix)
+       and ExistingTriePrefix in CodegenPlanCapabilities:
+        return ExistingTriePrefix(s)                # emits idx.First()/Next() range walk
+    return FullScanFilter                           # capability-guarantee fallback
+
+# P7 NEW — the honesty seam made ENFORCEABLE: the set of plan kinds codegen
+# can actually render. Opens with scan only; FullKeyHashLookup/ExistingTriePrefix
+# are added in LOCKSTEP with their EmitAccessPlan arms — never label-ahead-of-code.
+CodegenPlanCapabilities : set<AccessPlanKind> = { FullScanFilter }
+
+# P7 NEW — physical structures are their OWN authority (goal-1's 4th domain),
+# lazily interned from AccessRequirements. They REFER to canonical RegionalFactIds
+# (P4); they never own facts or define liveness.
+PhysicalAccessStructure =
+    HashArrangement{ relation, key_fields:SymbolicFieldSet }
+  | TriePrefixIndex { relation, ordered_prefix:[SymbolicFieldId] }
+ProvisionPhysicalStructure(req):                    # idempotent intern, first keyed use
+    if req wants a keyed read and its selected plan kind in CodegenPlanCapabilities:
+        intern matching HashArrangement/TriePrefixIndex over req.relation
+
+# P7 NEW — EmitAccessPlan: label == emission, refereed by V-PLAN-HONEST.
+EmitAccessPlan(plan, req):
+    match plan:
+      FullScanFilter:     emit `for s in 0..rel.NumRows(): if key==...: use row`
+      FullKeyHashLookup:  emit `iid = arr.Find(key); if iid: use rows(iid)`  # NO 0..NumRows
+      ExistingTriePrefix: emit `for r in trie.Range(prefix): use r`          # NO 0..NumRows
+
@@ replace the label-only V-ALPHA belt (Rel.cpp~4413-4490) @@
-V-ALPHA (label self-consistency only): a kInstanceKeySlot column pairs with
-    lowering in {kPointTest, kSectionWalk}. Validates the LABEL is internally
-    consistent, NEVER that the label matches what codegen emits.
+V-PLAN-HONEST (always-on, fprintf+abort, survives NDEBUG): for every emitted
+    read, the emitted loop shape == plan.kind's shape. A FullKeyHashLookup /
+    ExistingTriePrefix emission contains NO whole-table `0..req.relation.NumRows()`
+    scan of req.relation. This is the standing referee the old V-ALPHA never was.
+
@@ §3 EvaluateEpoch — per-read planning (seed L327-329) @@
         for delta in EvaluateRule(rule, st):
-            AddDerivation(st, delta.fact, delta.sign)   # AccessPlan per read (§2.4)
+            # each read resolves req = AccessRequirement (P2); P7 lowers it:
+            #   plan = SelectAccessPlan(req); ProvisionPhysicalStructure(req); EmitAccessPlan(plan, req)
+            AddDerivation(st, delta.fact, delta.sign)   # facts stay canonical (P4)
```

**Invariant established.** V-PLAN-HONEST: the AccessPlan label is a standing always-on guarantee that the emitted code shape matches the label — a non-scan plan (FullKeyHashLookup / ExistingTriePrefix) provably emits no whole-table `0..relation.NumRows()` scan, and a plan kind is selectable only if it is in CodegenPlanCapabilities (label can never run ahead of emission). Physical access structure becomes its own authority, distinct from the logical access path.

**Exit gate.** Structural test (Test-migration #17): compile a keyed relation reached by a bound path where FullKeyHashLookup/ExistingTriePrefix is in CodegenPlanCapabilities; grep the generated datalog.h and assert it contains ZERO `for (uint32_t s = 0; s < <rel>.NumRows(); ++s)` full-scan loops over that relation, and that a `.Find(`/`.Range(` keyed emission is present. Companion: the same relation under an unbound read still emits the honest FullScanFilter scan and returns the complete answer. V-PLAN-HONEST aborts the build if any emitted non-scan read contains a whole-table loop.

**Design goals resolved.**
- Goal 5 (honest FullScanFilter / retire the kSectionWalk placeholder): COMPLETED here as anti-regression — P4 made the scan label honest (scan==scan); P7 adds the FIRST genuine non-scan emission and the V-PLAN-HONEST referee proving the label discriminates, so a scan can never again hide behind a keyed label (the old V-ALPHA at Rel.cpp:4413-4490 only checked label self-consistency).
- Goal 1 (four-authority separation): resolves the 4th domain — physical access structure (HashArrangement/TriePrefixIndex) is now a first-class authority provisioned from AccessRequirement, never defining semantic identity or liveness (facts stay the P4 canonical RegionalFactRelation).

**Deletion obligations.**
- P4's stub `SelectAccessPlan := return FullScanFilter` (unconditional single-arm), seed §2.4 L302-309 — replaced by the codegen-gated dispatcher
- the label-only V-ALPHA belt at lib/Rel/Rel.cpp:4413-4490 — replaced by V-PLAN-HONEST (label==emission)
- the in-code honesty apology at lib/CodeGen/CPlusPlus/Database.cpp:2339-2341 ('a full scan with a key filter — deferred perf refinement') — the deferral is now discharged and the comment must be deleted, not left as a standing excuse
- obligation to keep the kSectionWalk enum value (lib/Rel/Rel.h:481) HONEST: post-P1 no kSectionWalk-labelled node may lower to a full scan; P7's V-PLAN-HONEST is the standing guarantee that reuse of kSectionWalk means an actual idx.First()/Next() range walk

**Reintroduction obligations.**
- ORDERING: consumes P2's per-read AccessRequirement records and P4's BindingStateId + canonical RegionalFactRelation; P7 cannot run before P2 (no typed AccessRequirement to lower) or P4 (no canonical fact target for a keyed read to refer to).
- P1 DELETED the only non-full-scan keyed physical read the system had (the kSectionWalk-labelled DRInstance rescan at Rel.cpp:1146 + InstanceStore hash lookup); P4 re-provided only FullScanFilter. P7 RE-PROVIDES the actual indexed/keyed physical access capability P1 removed (FullKeyHashLookup / ExistingTriePrefix) — this time with label==emission enforced, closing the capability gap P1 opened for keyed evaluation.

**Anchors.** `docs/proposals/RegionalDataFlowCore.artifacts/keyed-rewrite-pseudocode-seed.md:296-309`, `docs/proposals/RegionalDataFlowCore.artifacts/keyed-rewrite-pseudocode-seed.md:318-335`, `lib/Rel/Rel.h:480`, `lib/Rel/Rel.cpp:4413`, `lib/CodeGen/CPlusPlus/Database.cpp:2339`

---

### P8 — Lazy tries + induced orderings: shared-prefix navigation with convergent semantic endpoints (Free Join / COLT)

**Diff**

```diff
@@ §2.4b PhysicalAccessStructure (P7) — promote the flat TriePrefixIndex to a lazy shared trie @@
 PhysicalAccessStructure =
     HashArrangement{ relation, key_fields:SymbolicFieldSet }
-  | TriePrefixIndex { relation, ordered_prefix:[SymbolicFieldId] }   # ONE flat path
+  | LazyPathTrie  { relation, root:TrieNode }        # P8: many ordered paths, shared prefixes
+TrieNode{ binding_state:BindingStateId,               # P5's ORDER-FREE endpoint id
+          edges: map<SymbolicFieldId -> TrieNode> }    # one child per NEXT field of some path
+# A declared/inferred ordered path [f0,f1,...] is a CHAIN of TrieNodes; two paths
+# sharing a prefix share the prefix nodes; two paths fixing the SAME field SET
+# converge on ONE TrieNode whose binding_state is P5's single BindingStateId.
+CompilePathIntoTrie(trie, ordered_fields):           # LAZY: nodes minted on first visit
+    node = trie.root; bound = {}
+    for f in ordered_fields:
+        bound = bound + {f}
+        node = node.edges.get_or_mint(f,
+                 TrieNode{ binding_state = BindingStateId(region_instance,
+                                                          canonical_field_SET(bound)) })
+    return node   # @key(A)&@key(A,B) share the {A} node; [A,B]&[B,A] share the {A,B} leaf
+
@@ §3 EvaluateRule — rule/join traversal INDUCES additional orders (P8) @@
+InducedOrdering(rule, seed_fields) -> [SymbolicFieldId]:
+    # Free Join / COLT: the join variable order visited FROM seed_fields is an
+    # access order. r(A,C):-p(A,B),q(B,C) seeded at {A} induces [A,B,C]; a
+    # different seed induces a different order (=> a different lazy chain).
+    order = topo-order body vars from seed_fields along shared-var join edges
+                                                     # P6 edge-local projections give the join graph
+    CompilePathIntoTrie(trie_of(rule.head.relation), order)   # materialize VISITED path ONLY
+    return order
+EvaluateRule(rule, st):                              # P8 refinement of the P4/P7 read loop
+    order = InducedOrdering(rule, bound_fields(st))
+    for read in rule.body walked in `order`:
+        req  = AccessRequirement(read.relation, bindings-so-far, ...)   # P2
+        plan = SelectAccessPlan(req); ProvisionPhysicalStructure(req); EmitAccessPlan(plan, req)  # P7
+        yield derivations into canonical RegionalFactIds                # P4
+
@@ §2.2/§2.3 — the trie NODE is not a fact owner (retained invariant, restated) @@
+V-TRIE-CONVERGE (always-on, fprintf+abort): any two chains fixing the same
+    canonical field SET terminate at ONE TrieNode.binding_state. A TrieNode
+    holds fixpoint frontiers + FactDerivation ids (P5 BindingState), NEVER a
+    path-private copy of facts (retained: 'binding state is not a second owner
+    of relation facts'; 'multiple physical indexes never duplicate facts').
```

**Invariant established.** V-TRIE-CONVERGE: the physical navigation trie realizes P5's partial-binding DAG so that (a) declared/inferred paths sharing a prefix share TrieNodes, (b) any two ordered chains fixing the same field set terminate at ONE TrieNode.binding_state (the order-free endpoint), and (c) only VISITED path prefixes are materialized (nodes minted on first visit) — no eager power-set closure, no path-private fact store.

**Exit gate.** Compile `#local p(u64 A,u64 B) @key(A) @key(A,B).` and assert the trie for p has a SINGLE {A} TrieNode shared by both paths (Test #8). Compile `@key(A,B) @key(B,A).` and assert the two chains terminate at ONE {A,B} leaf with one BindingStateId (Test #9). Compile `r(A,C):-p(A,B),q(B,C).` seeded at {A}: assert the induced order is [A,B,C], that only the visited prefix nodes exist and unvisited subsets (e.g. {C}, {B,C} for this seed) are ABSENT from the trie (Test #16), and that a fact reachable via two convergent chains appears exactly once in RegionalFactRelation (Test #14). V-TRIE-CONVERGE aborts if any two same-field-set chains reach distinct nodes.

**Design goals resolved.**
- Goal 6 (partial-binding DAG / prefix sharing / intern only visited): PHYSICALLY realized — LazyPathTrie is the on-demand physical structure over P5's logical BindingState DAG; CompilePathIntoTrie mints nodes on first visit only, so unvisited subsets stay unmaterialized and multiple indexes never duplicate facts.
- Goal 2 (order-significant paths WITH order-free binding identity): physically realized — each ordered path is a distinct TrieNode chain ([A,B] != [B,A] as navigation), yet both converge on the single {A,B} TrieNode.binding_state (P5's order-free endpoint), enforced by V-TRIE-CONVERGE.
- Goal 7 (co-recursive key flow / induced orderings): rule/join traversal induces access orders across the P6 SCC — InducedOrdering uses P6's edge-local projections so a differently-keyed co-recursive predicate can induce a prefix-preserving order that fuses, or a key-changing order that opens a new chain, without duplicating facts.

**Deletion obligations.**
- P7's flat `TriePrefixIndex{ relation, ordered_prefix }` single-path variant (seed §2.4b as introduced by P7) — subsumed by LazyPathTrie{relation, root:TrieNode}
- any eager 'one physical index per declared path' provisioning from P7's ProvisionPhysicalStructure — replaced by lazy per-visit CompilePathIntoTrie so unvisited paths cost nothing
- the InstanceStore leaf-row-cache as the residual home (include/drlojekyll/Runtime/InstanceStore.h:54-219) is fully retired by P8: its complete-key double-buffered Table pair becomes a TrieNode leaf keyed on the order-free BindingStateId, so the 'one row set per complete key, no prefix states' shape (already deleted logically in P1) has its physical replacement here

**Reintroduction obligations.**
- ORDERING: consumes P5's BindingStateId interning + BindingEdge DAG (TrieNodes ARE the DAG's physical nodes; the trie cannot exist before P5 provides order-free endpoint ids) and P6's rule SCCs + edge-local field projections (InducedOrdering needs the join graph and must respect co-recursive key flow). P8 must not run before P5 or P6.
- consumes P7's PhysicalAccessStructure catalog + EmitAccessPlan + V-PLAN-HONEST; the ExistingTriePrefix emission P7 stubbed as a flat range walk is here backed by real LazyPathTrie.Range(prefix) navigation, so P8 must extend CodegenPlanCapabilities' ExistingTriePrefix arm to emit the shared-prefix trie walk (still V-PLAN-HONEST: no whole-table scan).

**Anchors.** `docs/proposals/RegionalDataFlowCore.artifacts/keyed-rewrite-pseudocode-seed.md:227-246`, `docs/proposals/RegionalDataFlowCore.artifacts/keyed-rewrite-pseudocode-seed.md:311-350`, `docs/proposals/RegionalDataFlowCore.artifacts/next-session-prompt.md:702-718`, `include/drlojekyll/Runtime/InstanceStore.h:54`

---

### P9 — Automatic path inference: a query is one binding source; declared @key preserved as a logical guarantee alongside inferred paths

**Diff**

```diff
@@ §2.1 access-path identity — declared -> declared UNION inferred, with provenance (P9) @@
-DeclaredAccessPath(relation, ordered_fields):
-    assert no_repeated_field(ordered_fields)         # a path repeats no field
-    return record{ relation, ordered_fields }        # [A,B] != [B,A]
-InternDeclaredPaths(relation, raw_paths):            # written pragma order irrelevant
-    seen = set()
-    for p in raw_paths:
-        canon = tuple(p.ordered_fields)              # NO sort — order IS identity
-        if canon in seen: REJECT "exact duplicate access path"
-        seen.add(canon)
-    assign KeyPathId deterministically AFTER the full set is validated
+AccessPath(relation, ordered_fields, provenance in {kDeclared|kInferred}):
+    assert no_repeated_field(ordered_fields)
+    return record{ relation, ordered_fields, provenance }   # [A,B] != [B,A]
+InternAccessPaths(relation, declared_raw, inferred_raw):
+    seen = set(); paths = []
+    for p in declared_raw:                           # DECLARED FIRST — a guaranteed capability
+        canon = tuple(p.ordered_fields)              # NO sort — order IS identity
+        if canon in seen: REJECT "exact duplicate access path"   # retained P0 item 4
+        seen.add(canon); paths += AccessPath(relation, canon, kDeclared)
+    for p in inferred_raw:                           # INFERRED — additive, never overrides
+        canon = tuple(p.ordered_fields)
+        if canon in seen: continue                    # already declared: keep the declared one
+        seen.add(canon); paths += AccessPath(relation, canon, kInferred)
+    assign KeyPathId deterministically AFTER the full set is validated
+
@@ §2.1 NEW — infer useful access sequences (P9) @@
+InferAccessPaths(region):
+    # A QUERY is only ONE source of initial bindings; rules/joins/active
+    # bindings are the others (retained: 'a query is one possible source').
+    cand = []
+    for rule in region.rules:                         # from rule/join structure (P6 edge-local)
+        for seed in useful_seed_field_sets(rule):
+            cand += InducedOrdering(rule, seed)        # P8 hook
+    for st in active_binding_states(region):          # from LIVE bindings (P6 rooted reachability)
+        cand += orderings_visited_from(st)
+    return dedup(cand)
+# population: InternAccessPaths(rel, declared = rel.@key paths, inferred = InferAccessPaths(region))
+
@@ §2.4 SelectAccessPlan (P7) — inferred structures are eligible too (P9) @@
     s = LookupPhysicalStructure(req.relation, req.available_ordered_bindings)
+    # s may now be provisioned from an INFERRED path, not only a declared @key.
+    # A DECLARED path with NO physical structure still returns FullScanFilter:
+    # a declared path promises a LOGICAL specialization capability, NOT a layout.
+
@@ §2.1 — the preservation referee (P9) @@
+V-DECLARED-PATH-PRESERVED (always-on, fprintf+abort): every kDeclared AccessPath
+    survives InferAccessPaths unchanged and remains a selectable logical
+    specialization even when it provisions NO physical structure; an inferred
+    path never shadows, reorders, or removes a declared one.
```

**Invariant established.** V-DECLARED-PATH-PRESERVED: declared @key paths are guaranteed LOGICAL specialization capabilities that survive inference verbatim (never shadowed/removed by an inferred path and never obligated to a physical layout), while inferred paths are purely additive optimization hints drawn from rules/joins/active bindings — a query is one binding source among several, not the authority for keys.

**Exit gate.** Compile a program with NO @key whose rule `r(A,C):-p(A,B),q(B,C).` induces [A,B,C]: assert an kInferred AccessPath is interned and (if ExistingTriePrefix is in CodegenPlanCapabilities) a non-scan plan is selected — with zero @key pragmas (a query is not required, Test-migration #4/#5). Compile `#local rel(...) @key(A).` used by a query whose binding pattern differs from {A}: assert the {A} specialization is still available (Test #5, query bindings need not equal relation keys). Compile a declared @key that provisions no physical structure: assert the unbound/declared read still answers the COMPLETE relation via FullScanFilter (Test #13). V-DECLARED-PATH-PRESERVED aborts if any kDeclared path is missing or altered after InferAccessPaths.

**Design goals resolved.**
- Goal 1 (four-authority separation — logical access path vs physical access structure): finalized — an inferred path is a LOGICAL access-path fact that does not promise a physical arrangement, and a declared path is a logical guarantee that may still lower to FullScanFilter; SelectAccessPlan keeps the physical-structure choice strictly downstream of the logical-path set.
- Goal 2 (order-free binding-state identity across all path sources): inferred and declared ordered paths both intern their endpoints through P8's LazyPathTrie, so a declared [A,B] and an inferred [B,A] still converge on the single {A,B} BindingStateId — order-free identity holds regardless of whether a path was declared or inferred.
- Goal 7 (co-recursive key flow): active_binding_states(region) feeds inference from the P6 rooted-reachability set, so orderings actually visited during recursive evaluation become inferred paths — inference is driven by real co-recursive key flow, not just static query adornments.

**Deletion obligations.**
- `InternDeclaredPaths` and the bare `DeclaredAccessPath(relation, ordered_fields)` record (seed §2.1 L227-236) — replaced by provenance-tagged AccessPath + InternAccessPaths(declared, inferred)
- the 'declared-only' assumption that DeclaredAccessPathSet is the sole population source of a relation's paths (seed §2.1; next-session-prompt.md:433-442 DeclaredAccessPathSet) — the set is now declared UNION inferred
- any residual coupling that treats a query adornment / bound query as the authority or precondition for a relation's access paths (final removal of the P0/P1-era assumption; the declared-path source at lib/Parse/Parse.h:387,392 is now ONE input to InternAccessPaths, not the whole set)

**Reintroduction obligations.**
- ORDERING: consumes P8's InducedOrdering + LazyPathTrie (inference emits induced orders as trie chains) and P6's active_binding_states (rooted reachability) + edge-local projections + RuleRoutingProjection from P2 (typed source->dest field mapping needed to infer sound cross-relation orders). P9 must not run before P8 or P6.
- RE-PROVIDES the generalization P0/P1 stripped: when @key was decoupled from query adornments (P0) and the demand transform deleted (P1), the ONLY binding source (the query adornment) was removed and nothing replaced it as an access-path generator. P9 makes rules, joins, and active bindings first-class binding sources so keyed/inferred access is driven by program structure, not by whether a query happens to bind the relation — completing the 'a keyed relation is valid independently of how queries use it' contract.

**Anchors.** `docs/proposals/RegionalDataFlowCore.artifacts/keyed-rewrite-pseudocode-seed.md:227-236`, `docs/proposals/RegionalDataFlowCore.artifacts/next-session-prompt.md:433-442`, `docs/proposals/RegionalDataFlowCore.artifacts/next-session-prompt.md:719-725`, `lib/Parse/Parse.h:387`

---

