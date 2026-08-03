# Stage C — request edges replace forcing: the cutover, as diff hunks

Diff target: `regional-arch-pseudocode.md` (fleet-verified 2026-08-02 at tip
f0c913e0). Every hunk below names the pseudocode section it modifies and the
code anchor it corresponds to. Normative target: `RegionalDataFlowCore.md`
§13 Stage C, §5, §7, §9, §11, §12, §14, §15. Review inputs:
`fable-review-2026-08-01.md` (Concerns 1–4).

SINGLE-PASS RULE (house precedent): the implementing fleet re-verifies every
anchor here against code before cutting — line numbers drift, structure should
not.

This is THE cutover. Stages A (identity types + explicit projection),
B (`FrozenRegionalProgram` canonical, 180 goldens byte-identical), and I0
(reference relational interpreter, corpus-validated against the tip compiler)
are PREREQUISITES and are assumed landed. Stage C deletes the entire demand
layer of §2/§3/§4/§4c and replaces it with the request-edge lifecycle. Nothing
in this stage introduces a runtime old/new selector (proposal §0); the tagged
pre-cutover binary is a TEST-TIME referee only (Concern 1.2).

---

## 0. Deletion manifest (each row names its replacement's landing stage)

| # | Deleted thing | Pseudocode anchor | Code anchor | Replacement | Lands |
|---|---|---|---|---|---|
| D1 | `ApplyDemandTransform` (all of it) + the two failure classes (clean rejects; Step-9 tripwire + OWN-3 census aborts) | §2 | `lib/DataFlow/Demand.cpp:385` | planner `ExtractPureChild`/`FirstStableAdmissibleChild` + admissibility validation | **C** (H-E) |
| D2 | `FabricateDemandMessage` / `FabricateDemandLocal` + the `demand__` fabrication registry | §2 Records | `lib/Parse/Demand.cpp:163/:219` | no fabricated parser objects; demand is compiler-owned request edges | **C** (H-F) |
| D3 | `kMessageHandler` demand suppression site + `IsDemandMessage` predicate | §2 read seam; §4c `:1511-1513` | `Query.h:1074`; `Database.cpp:1511-1513` | absence — no demand message exists to suppress; the demand seed is a lease/edge | **C** (H-F, H-H) |
| D4 | `GuardAnnotation`, `RecognizedSubgraph`, `QueryDemandForcing` + `guard_annotation_index` carrier + the `DemandForcings/GuardAnnotations/RecognizedSubgraphs` read seam | §2 Records; carrier | `Query.h:966-1032`; private `Query.h:479`; `Demand.cpp:295-313` | `RequestEdgeRelation` + `FrozenRegionTemplate` records | **C** (H-F) |
| D5 | `ResolveLiveRecognition` + `BuildSubgraphInstanceOps` + the `demand_instance_enabled` fork + the census recount + `CheckInstance*` (both `ValidateDROps` and `CheckInstanceOrder`) | §3 | `Rel.cpp:934, :1036, :2065, :3999-4021, ~:4280-4392, :4993` | `LowerFrozenRegion` lifecycle lowering + lifecycle census + V-* lifecycle validators | **C** (H-G, H-I) |
| D6 | `BuildQueryInjectorProcedure` (force + retract) + `retract_proc` + parse-level `ForcingMessage` fallback (`@first` body-forcing) | §4; §4c | `Build.cpp:389-506, :526-532`; `Query::ForcingMessage()` | move-only `RootRequestLease` acquire/release lowering (one lifetime API) | **C** (H-H) |
| D7 | `LowerSubgraphInstances` band vocabulary (a0/a1/a2/a2'/b) + `DRInstance` + the op kinds `kSubgraphInstantiate`/`kInstanceDeath`/`kInstanceSeal` + eff kinds `kInstanceRebuild`/`kInstanceDemand` + `kInstanceKeySlot` | §3 op vocab; §4 | `Rel.h:148-159, :86-90, :478`; `Procedure.cpp:279` | lifecycle ops `request_edge_add/remove`, `input_delta`, `local_fixpoint`, `child_result_add/remove`, `routed_result_add/remove`, `retire_inactive`, `seal_epoch` | **C** (H-G) |
| D8 | `InstanceStore<Key,RowT>` as the SEMANTIC keyed-instance model (double-buffered full-rescan) | §4 | `include/drlojekyll/Runtime/InstanceStore.h:62` | `RequestEdgeRelation` (small edge table) + the ORDINARY pub `DiffTable` (results stored once — shared-pub) | **C** (H-G). Physical per-instance store, if any, is a Stage-D ADDITION, not a deferred replacement (escalation E5) |
| D9 | flags `-demand`, `-demand-instance`, `-demand-retract` + the three globals + the ad-hoc double-set | §1 `:48-50, :471-491` | `Main.cpp` | unconditional compiler-owned regional path; no mode gate | **C** (H-A) |

Every replacement lands in **Stage C** or earlier (Stage B added the
`BuildPlanningRegionalProgram` shell that H-E extends; Stage A added the identity
types that H-F/H-G consume). No deletion outruns its replacement.

---

## H-A. Pipeline: retire the three flags and the mode gate (§1)

```diff
 main(argv):                              # bin/drlojekyll/Main.cpp
-  gDemand (:48), gDemandInstance (:49, implies gDemand),
-  gDemandRetract (:50)               # three globals, ad-hoc double-set (:471-491)
-  query   = Query::Build(module, log, gPassPolicy, gDemand, gDemandRetract)
+  query   = Query::Build(module, log, gPassPolicy)
   SetRelDumpStream(gRelStream)
-  program = Program::Build(query, log, gFirstId, gPassPolicy, gDemandInstance)
+  program = Program::Build(query, log, gFirstId, gPassPolicy)
```

```diff
 Query::Build                             # lib/DataFlow/Build.cpp:2518
   ...
   ConnectInsertsToSelects
-  ApplyDemandTransform(module, log, demand_mode, demand_retract)   # :2587
+  # (D1: deleted here; the regional planner runs in the Build tail — see H-E)
   Optimize [gate AnyBodyOptionalEnabled(kDataFlow)]
   ...
   Stratify(log)
-  return Query(impl)
+  planning = BuildPlanningRegionalProgram(impl)   # Stage-B shell, H-E extends
+  frozen   = FreezeAndValidate(planning)          # distinct TYPE (Stage B)
+  return frozen
```

- `demand_mode == false` head-return (§2:393) is GONE — there is no mode.
  Every program takes the regional path. The `gPassPolicy.bisect_counter`
  threading (§1:35) is untouched (Stage B already carried it across the
  `FrozenRegionalProgram` boundary).
- **Golden consequence:** the ~38 corpus cases that today carry a bound
  `#query` but compile flag-OFF (byte-identical to no-demand) now receive
  regional treatment. Extraction is answer-neutral (Concern 3), so their
  `.stdout` stays byte-identical; their SHAPE goldens (`.rel`/`.df`/`.ir`/`.h`),
  where pinned, change. See Exit Gate §EG.

---

## H-B. Delete the demand transform (§2, all of it)

```diff
-# lib/DataFlow/Demand.cpp:385  QueryImpl::ApplyDemandTransform
-ApplyDemandTransform(module, log, demand_mode, demand_retract):
-  if not demand_mode: return true                  # mode gate
-  if module.DemandMessagesFabricated(): reject      # re-entry guard G2
-  # Step 1  locate THE bound #query relation (reject >1 / ≠1 materialization)
-  # Loop 1 / PHASE 1  trace, classify GuardSite {kReadAtTuple|kBaseAtom|kPushDown}
-  # Step 4  stray-consumer union (CollectColUsers)
-  # Loop 2 / PHASE 2  mint per adornment:
-  #   FabricateDemandMessage/Local; mint demand seed; MintGuardJoin; stamp
-  #   GuardAnnotation; MintRestoringTuple; the query-projection DOUBLE JOIN;
-  #   push RecognizedSubgraph; push QueryDemandForcing
-  # Step 9  root-seed reachability TRIPWIRE (fprintf+abort)
-  # Step 11 deferred rewires grouped by (consumer,read): SINGLETON | MULTI (R-DUP)
-  # Step 11b OWN-3 census (fprintf+abort)
-  module.MarkDemandFabricated()
```

Deleted with it:
- Both failure classes. The clean `reject(...)` diagnostics become the planner's
  admissibility outcomes (H-E, H-J). The Step-9 tripwire + OWN-3 census aborts
  become the lifecycle-closure validators (H-I: V-EDGE-BALANCE, V-LIFECYCLE-CENSUS).
- The `GuardSite`/`GuardAnnotation` value-locked enum static_assert
  (`Demand.cpp:145`) — no enum survives.
- The `DemandMessagesFabricated`/`MarkDemandFabricated` re-entry guard — no
  fabrication, no re-entry hazard.

**Grep gate:** no `ApplyDemandTransform`, no `GuardSite`, no `MintGuardJoin`,
no `DEMAND-GUARD`/`DEMAND-RESTORE`/`DEMAND-RAW-SEED`/`DEMAND-GUARD-UNION`
string literals.

---

## H-C. Delete the parser fabrication + records + carrier + read seam (§2 Records)

```diff
-FabricateDemandMessage(name, types, demand_retract)   # Parse/Demand.cpp:163
-  # real ParsedMessageImpl via CreateDerived; synthetic @differential stamp
-FabricateDemandLocal(name, types)                      # Parse/Demand.cpp:219
```
```diff
-# include/drlojekyll/DataFlow/Query.h — PUBLIC header
-QueryDemandForcing {query, message, bound_params}      # :966
-GuardAnnotation    {Kind, DemandSide, Role, is_instance_key,
-                    instance_key, guarded_read, demanded_view, forcing_index}  # :988
-RecognizedSubgraph {forcing_index, demanded_view, key_cols,
-                    pub_view, guard_annotation_indices}  # :1026
-# private lib/DataFlow/Query.h:479
-QueryViewImpl::guard_annotation_index (unsigned {~0u})  # the CSE-surviving stamp
-# CSE stamp migration (View.cpp:684-724) + kNoGuardAnnotation accessor (:443-444)
-# read seam:
-Query::DemandForcings() / GuardAnnotations() / RecognizedSubgraphs()
-Query::IsDemandMessage(m)
```

Replacement (H-F introduces the data; here we only note the accessor deletion):
the ONLY cross-pass surface that survives is the frozen region's public
contract — `FrozenRegionTemplate.request_port` / `.result_ports` and the
`RequestEdgeRelation`. No downstream pass re-walks live views through a
CSE-migrating stamp (the F1 hazard is DELETED, not re-guarded).

**Grep gate:** no `GuardAnnotation`, `RecognizedSubgraph`, `QueryDemandForcing`,
`guard_annotation_index`, `IsDemandMessage`, `DemandForcings`, `demand__`.

---

## H-D. Delete recognition recovery + instance-op discovery (§3)

```diff
-ResolveLiveRecognition(impl, query)                    # Rel.cpp:934
-  # ABA-safe re-resolution of stale RecognizedSubgraph handles by re-bucketing
-  # LIVE guard JOINs on GuardAnnotationIndex()
-BuildSubgraphInstanceOps(flow, impl, context, query, scc_map)  # Rel.cpp:1036
-  # HP-4 refusal belt; P-STORE/P-DEATH divergence; InstantiateEffects;
-  # DRInstance push; rescan spine; kInstanceDeath/kInstanceSeal mint; strata
-BuildDRInventory: if context.demand_instance_enabled:  # Rel.cpp:2065 THE FORK
-    BuildSubgraphInstanceOps(...)
```

Reason this is a whole-function delete, not a rewrite: the entire pass exists to
RE-DISCOVER a nested object the compiler already destroyed by graph surgery
(proposal F1). Under Stage C the region is authoritative in
`FrozenRegionalProgram` — there is nothing to recover. `BuildDRInventory` loses
the `:2065` fork; the `(void) context; (void) scc_map;` inert plumbing at
`Rel.cpp:1039-1040` goes with it.

Replacement lands in H-G (`LowerFrozenRegion` walks the frozen region's local
graph — no discovery).

---

## H-E. The planner: one-level pure child extraction (new; extends the Stage-B `BuildPlanningRegionalProgram` shell)

Diff target: §1 Stage B tail (`BuildPlanningRegionalProgram`, added as a
no-extraction shell in Stage B). This is the REPLACEMENT for D1. Anchor:
proposal §8.

```diff
 BuildPlanningRegionalProgram(logical):        # Stage B: shell only
   contracts = InferConservativeRowContracts(logical)   # Stage A
   planning  = BuildProgramRootAndObservationRoots(logical, contracts)
+  for region in stable ownership order:
+    OptimizeLocalGraphToFixpoint(region)
+    SolveBackwardRequirements(region)
+    while candidate = FirstStableAdmissibleChild(region):
+      if not ExtractionPolicy(candidate):     # <-- NAMED PROVISIONAL SEAM
+        continue-past candidate               # legal but declined this pass
+      child = ExtractPureChild(region, candidate)      # ONE level in Stage C
+      OptimizeLocalGraphToFixpoint(child)
+      SolveBackwardRequirements(child)
+      ReplaceSliceWithOpenChildCall(region, child)
+      ReoptimizeParent(region)
   SolvePortsInOwnershipPostorder(planning)
   ValidatePlanningProgram(planning)
   return planning
```

- `FirstStableAdmissibleChild` enumerates candidate slices in a DETERMINISTIC
  ownership order and returns the first whose slice passes the §8.1 admissibility
  gate (all-pure, newly-owned direct child, realizable member key, exact
  request/result removal identity, differential crossing strategy, sealed ABIs
  untouched, no parameter escape, local stratification valid).
- **ONE LEVEL in Stage C** (the `while` extracts direct children of an
  observation root only; the extracted child is NOT itself descended). Nested
  extraction is Stage D.
- **`ExtractionPolicy(candidate) -> bool` is a NAMED PROVISIONAL SEAM** (review
  Concern 3). Stage C implementation: `return true` (extract-whenever-legal).
  This is the exact point the demand cost model
  ([[demand-cost-model]]) meets the planner — declared here so the later cost
  work modifies ONE function rather than rediscovering that the planner made
  cost decisions. Known failure mode of always-true: message-rooted trivial
  shapes get a region template that never pays for itself; behavior is correct,
  only shape/perf is affected. Marked ESCALATION E3.

**Determinism note:** admissibility ordering must not depend on pointer-derived
ids (HP-9). `FirstStableAdmissibleChild` derives candidate order from a
`FrozenRegionalProgram` node walk (Stage A `LogicalNodeId` order), never from
handle identity or vector append order.

---

## H-F. Request-edge relations replace the demand records (new; proposal §5)

Diff target: §5 (the identity story) → the request-edge model. Types come from
Stage A; Stage C wires the RELATIONS.

```diff
-demand ownership = presence of a fabricated demand-message row
-                   (multiple requesters collapse; no per-owner retraction — F4)
-demand support   = the row's derivation counters (a count as owner set)
+RequestEdgeRelation {                        # THE demand authority (proposal §5.2)
+  owner: RequestOwnerId                       # RootLease | PermanentRoot | RegionalMember
+  call_site: CallSiteId
+  child: ChildInstanceId                      # { owner_instance, region, key }
+}
+ActiveInstanceRelation = DistinctProjection(RequestEdgeRelation.child)   # §5.2
+DemandSupportCount(child) = count live RequestEdgeId for child           # DEMOTED
+  # a derived summary: answers "is this instance active", CANNOT route/retract
+ChildResultRelation { child, member: SemanticMemberKey, payload }        # §5.3
+RoutedResultRelation = RequestEdgeRelation JOIN ChildResultRelation ON child
```

- `DemandSupportCount` is DEMOTED to a derived integer (proposal §5.2): it may
  never stand in for the edge set. The Stage-A `SupportAlgebra` static_assert
  battery (H-A1, the F4 deliverable) forbids using it as an owner at the type
  level — there is NO named `V-DEMAND-SUPPORT-DERIVED` runtime validator at
  Stage A (X3 reconciled 2026-08-02, brief Errata-5: the name was a miscite
  of the battery).
- Multiple requester members demanding the same child produce DISTINCT
  `RequestEdgeId` values; multiple derivations of ONE requester member affect
  derivation support for ONE edge (they do not fabricate anonymous owners) —
  this is the direct fix for F4, and it makes the D3.a.3 refcounted-union-pub
  and the V-INST-SOLE `(pub_table, forcing_index)` re-key OBSOLETE (both were
  approximations of `RequestEdgeRelation`; Concern review "F4 workarounds").

---

## H-G. The shared-pub RoutedResult realization + lifecycle ops (diff on §4b epoch path + §3 op vocab)

This is the FIRST PHYSICAL REALIZATION (review Concern 4): results stored ONCE,
per-owner routing/retraction DERIVED from the edge relation. It is the D3.a.3
shared-pub shape generalized — **never** a literal O(edges × results)
materialization.

### H-G.1 — op vocabulary swap (§3)

```diff
-DROpKind += kSubgraphInstantiate  kInstanceDeath  kInstanceSeal
-EffKind  += kInstanceRebuild  kInstanceDemand
-BindingSource::kInstanceKeySlot
+# lifecycle ops (proposal §9 RelRegionLifecycle):
+kRequestEdgeAdd    kRequestEdgeRemove
+kInputDelta        kLocalFixpoint
+kChildResultAdd    kChildResultRemove
+kRoutedResultAdd   kRoutedResultRemove
+kRetireInactive    kSealEpoch
```

### H-G.2 — band → op mapping (§4 `LowerSubgraphInstances` → `LowerFrozenRegion`)

Diff target: §4 band doc-comment order (a0/a1/a2/a2'/b) + §4b Step 6 tail.

```diff
-LowerSubgraphInstances:                          # Procedure.cpp:279
-  death_by_sid = OpsOfKind(kInstanceDeath)
-  for op in flow.SubgraphInstances():   # ONE SUBGRAPHINSTANCE region
-    band a1: birth  (orphan-mint fence on demand kNetAdditions)
-    band a2: rebuild (memoized input net-additions frontier)
-    band a2': rebuild (input net-removals iff diff input)
-    band b : signed publish  (del_queue/add_queue; si->demand_table Present-probe)
-    band a0: death drain (demand kNetRemovals) iff sid in death_by_sid
-    si->input_key_cols / input_row_cols; enroll {instantiate, seal, death?}
+LowerFrozenRegion(region, rel):                  # replaces the band vocabulary
+  for each observation root / extracted child (ONE level):
+    # request births/deaths (replaces a1-birth-fence AND a0-death-drain):
+    kRequestEdgeAdd    from RootLease/RegionalMember edge net-additions
+    kRequestEdgeRemove from edge net-removals    (lease destructor source — H-H)
+    # input crossing (replaces a2 / a2'):
+    kInputDelta        from the input net-add AND net-removal frontiers
+                       (ONE op, signed; the a2/a2' split collapses)
+    kLocalFixpoint     the §7.1 Rel differential fixpoint over the region body
+                       (SURVIVES unchanged — rel-arch §7(B); UNSTRATIFIED at the
+                        epoch tail in Stage C, exactly as SUBGRAPHINSTANCE is
+                        today — see §4b Step 6 asymmetry; Stage D stratifies)
+    # result maintenance (replaces band-b signed publish):
+    kChildResultAdd / kChildResultRemove  into the ORDINARY pub DiffTable
+                       (results stored ONCE — the shared pub; NO per-instance store)
+    kRoutedResultAdd / kRoutedResultRemove  = RequestEdgeRelation ⋈ ChildResult
+                       DERIVED per-owner; rides the SAME pub-table
+                       kDeleteQueue/kAddQueue + commit sweep band-b uses today
+    kRetireInactive    when child's last edge dies AND routed removals visible
+    kSealEpoch         (replaces kInstanceSeal)
```

Key realization facts (Concern 4, made concrete against §4b):
- The pub table shape is UNCHANGED. Band-(b) already "rides the ORDINARY
  pub-table `kDeleteQueue`/`kAddQueue` vectors + commit sweep" (§4b Step 6);
  the shared-pub realization keeps exactly that. What changes is the
  demand-liveness GATE: the E8d `si->demand_table` Present-probe is REPLACED by
  `RoutedResultRelation` membership (an edge-join), so a result is published to
  an owner iff a live `RequestEdgeId` routes it.
- `RoutedResultRelation` is NEVER stored as `edges × results`. It is a
  differential JOIN whose add/remove deltas are the published deltas. Adding an
  edge attaches all existing child results to that owner; removing one edge
  retracts only that owner's routed results; child-result changes fan out to
  every live edge — one differential meaning for birth / late subscriber /
  detachment / fanout (proposal §5.3, the review's "genuine consolidation").
- The `si->input_key_cols` rescan spine (§4 `:462-464`) and the full-rescan
  `InstanceStore` double-buffer are GONE (D8). The region body is maintained by
  `kLocalFixpoint` (the ordinary Rel fixpoint), not by a rescan-on-touch.

### H-G.3 — the R-MONO / R-DIFF divergence collapses

Today P-STORE (`TableIsDifferential(pub)`) and P-DEATH
(`TableIsDifferential(demand)`) diverge by design (the e5 carrier). Under
Stage C, demand is UNIFORMLY retractable (a `kRequestEdgeRemove` exists for
every owner), so the "soundness by irrevocability" MONO special case disappears
— the review's headline consolidation. The `if (diff)` pub arm
(`Rel.cpp:1072`, corpus-UNEXERCISED today) becomes the ONLY arm. This is the
load-bearing reason the exit gate depends on a differential-pub oracle that
does NOT exist pre-cutover — see ESCALATION E1.

### H-G.4 — new lifecycle census (§4b Step 5/6 + §3 census recount)

```diff
-# ValidateDROps census recount (Rel.cpp:3999-4021):
-#   per RecognizedSubgraph ++exp_instance (+exp_death if P-DEATH)
-#   expect kSubgraphInstantiate/kInstanceSeal == exp_instance, kInstanceDeath == exp_death
+# lifecycle census (proposal §9/§11 — V-LIFECYCLE-CENSUS, H-I):
+#   every FrozenChildCall / observation root => exactly one kSealEpoch
+#   every RequestEdgeId => balanced kRequestEdgeAdd / kRequestEdgeRemove path
+#   every ChildResult => participates in kRoutedResult{Add,Remove} fanout
+#   FrozenRegionalProgram op census == Rel op census == ControlFlow op census
```

**Lifecycle asymmetry preserved:** in Stage C the region lifecycle runs ONCE,
outside all strata, at the epoch tail — same asymmetry as §4b Step 6's
SUBGRAPHINSTANCE (unstratified). The `LowerFrozenRegion` call sits where
`LowerSubgraphInstances` sat (`Procedure.cpp:588`, inside
`PublishDifferentialMessageVectors`, after `LowerDRRounds` quiesces). Stage D is
what qualifies the fixpoint by `(RegionId, InstanceId)` and stratifies per
instance — the direct inverse of today's recursive-content fences (see the
§7(6) open decision; NOT a Stage C obligation).

---

## H-H. Move-only `RootRequestLease` replaces the injector/retract pair (diff on §4c; resolves F3)

Diff target: §4c (`EmitQueryFriends`) + §4 (`BuildQueryEntryPointImpl`).

```diff
-BuildQueryEntryPointImpl:                        # Build.cpp:509-543
-  forcer_proc  = BuildQueryInjectorProcedure(is_retract=false)   # :526
-  retract_proc = BuildQueryInjectorProcedure(is_retract=true)    # :531
-  # dispatcher matches (query, BindingPattern); retract gates IsDifferential;
-  # fallback = parse-level query.ForcingMessage() (@first body-forcing)
-  queries.emplace_back(query, table, scanned_index, forcer_proc, retract_proc)
+BuildQueryEntryPointImpl:
+  # one lowering: the root request edge (proposal §6). No injector, no retract proc.
+  queries.emplace_back(query, pub_table, scanned_index,
+                       root_lease_acquire /*kRequestEdgeAdd*/,
+                       /* release is the cursor destructor, not a proc */)
```

```diff
 EmitQueryFriends(spec):                          # Database.cpp:1610-1820
-  emit_forcing_call(args):
-    if spec.forcing_function:
-      DetailName(*forcing_function)(state..., args...)   # inject demand ADD
-  if spec.retract_function:                              # void name_retract(...)
-    emit `void name_retract(db, ..., bound)`; DetailName(*retract_function)
+  # THE forcing call becomes lease acquisition:
+  emit_acquire_lease(args):
+    lease = AcquireRootRequestLease(db, spec.root_edge, args)   # enqueues the
+                                                                # exact edge ADD
+  # NO name_retract. Lease destruction owns the exact pending removal.
   if !has_free:  existence check (Find/Present)          # unchanged shape
   else: nested struct name_cursor {
-    Database&; bound fields; uint32_t pos;
+    Database&; bound fields; uint32_t pos;
+    RootRequestLease lease;             # move-only: copy = deleted
     bool next(free out-refs) {...}
+    // destructor: lease enqueues removal of the exact root RequestEdgeId
   }
-  factory: emit_forcing_call(bound); return { db, bound..., First/0 }
+  factory: lease = emit_acquire_lease(bound);
+           return { db, bound..., First/0, move(lease) }
```

- ONE lifetime API (F3 fix). "Demand lifetime" and "cursor lifetime" — the two
  STRUCTURALLY UNCONNECTED APIs of §4c — become one move-only token. The cursor
  OWNS the lease; the lease owns the exact `RequestEdgeId`. Close/destruct
  enqueues the edge removal; the next entry point nets ALL pending lease
  removals before other regional work (proposal §6).
- `-demand-retract` and the separate `name_retract` entry point are DELETED
  (D6). No nullable release callback, no user-invoked retract (proposal §14,
  §10).
- **Drain-before-next-entry** stays a CONVENTION in the first implementation
  (proposal §6: "The first implementation enforces this existing drain-before-
  next-entry contract"). The lease makes RETRACTION type-safe; it does not yet
  make CURSOR staleness type-safe against an intervening `CompactDead()`
  renumber (§4c). That residual is explicitly deferred by the proposal — record
  it so the exit gate does not over-claim.
- `@first` query-body forcing surface removed (proposal §10; the `ForcingMessage`
  fallback of §4c). If any corpus `.dr` uses `@first` in a query body, its
  witness is rewritten (§12.4).

**Grep gate:** no `BuildQueryInjectorProcedure`, `kQueryMessageInjector`,
`inject_<id>`, `retract_function`, `name_retract`, `ForcingMessage`.

---

## H-I. Validators: retire the instance census, add lifecycle closure (§3 validators + proposal §11)

```diff
-# retired with the instance op kinds (D5/D7):
-CheckInstantiateEffects / V-INST-EFFECT                 # Rel.cpp:4301
-V-INST-SOLE  (CheckInstanceSolePub, (pub_table,forcing_index))  # :4384
-V-INST-PAIR / V-INST-DIFF-COHERENCE / V-INST-INPUT-COHERENCE / V-INST-DEATH-COHERENCE
-V-ALPHA (kInstanceKeySlot placement)                    # ~:4411-4488
-CheckInstanceDeathFrontier / CheckInstanceInputArm      # :4551-4552
-CheckInstanceOrder / V-INST-ORDER (LinearizeAndValidateDRFlow :5887)
-# the keyed-instance census recount                     # :3999-4021
-# the Demand.cpp Step-9 tripwire + OWN-3 census (fprintf+abort)
+# added (proposal §11; always-on fprintf+abort, survive NDEBUG):
+V-REGION-ACYCLIC   region call targets a DIRECT child; no ownership/call cycle
+V-OWNER-EXACT      no ambiguous RequestOwnerId / RequestEdgeId
+V-EDGE-BALANCE     every request edge has a balanced add/remove path
+V-ROUTED-FANOUT    every child result participates in routed fanout
+V-ROUTE-EXACT      every result removal has an exact routed-result identity
+V-INACTIVE-AFTER   inactive state cleared ONLY after routed removals visible
+V-PORT-CLOSED      no open port reaches FrozenRegionalProgram (Stage B; asserts here)
+V-LIFECYCLE-CENSUS FrozenRegionalProgram / Rel / ControlFlow op census agree
+# AMENDED 2026-08-02 (owner-ratified D2.1; closes brief Errata-5 / X1/X2 —
+# Stage B H9 defers these HERE, so H-I must land them by name):
+V-PORT-AGREE       every FrozenChildCall mapping matches the child's frozen
+                   port schema (proposal §11 line 5, parent/child frozen-port
+                   disagreement)
+V-PURE-REGION      no effectful operator inside an extracted pure region
+                   (proposal §11 line 9; co-arrives with real extraction —
+                   ProgramRoot co-hosts effects, children may not; grounds
+                   §15.13, plus the §12.3 row-12 directed effects witness)
```

AMENDED 2026-08-02 (owner-ratified D2.1 sub-pick): §11 lines 3
(symbolic-parameter-escape) and 6 (sealed-ABI mutation) are NOT named
validators — their §11 "compilation fails" semantics is explicitly SOFTENED
to "declines extraction into full materialization" (the admissibility gate's
behavior); the proposal text must state the softening. Also ratified
(corr-3): `PermanentRoot` is a `kRequestEdgeAdd` SOURCE with add-only
permanent edges CARVED OUT of `V-EDGE-BALANCE` (or given a defined teardown
removal), plus the §12.3 permanent-root witness.

- The four intrinsic B-3 validators (V-XOVER-ONE/V-PROD-MONO/V-PROD-CLASS/
  V-JOIN-ONE) and the eager-web census (V-PRED-XCHECK, the Site-5 multiset
  cross-checks, etc.) SURVIVE unchanged — they validate the local-graph
  lowering, which the region body still is (review "greenfield at the DEMAND
  layer only").
- `V-INACTIVE-AFTER` is the typed form of the F3 "key existence ≠ liveness"
  hazard: state retires only after the last edge dies AND its routed removals
  are published (proposal §15.10).

---

## H-J. OWNER-GATED — the fate of today's clean diagnostics (review Concern 2)

§8.1 says an inadmissible slice "remains in its current regional scope" and the
program still lowers: the queried relation is fully materialized in the
observation root, the lease just scopes a cursor (today's flag-off behavior +
lease bookkeeping). That is a BEHAVIOR CHANGE from today's rejects. **The owner
must choose.** Both variants laid out; do not silently pick.

Cases in question (today's demand diagnostics):

| case | today | maps to |
|---|---|---|
| `demand_cyclic_1` | reject under `-demand-instance`; compiles under `-demand` | recursive demand = key-changing recursive region call (§2.2 excluded) |
| `demand_recursive_content_1` | reject under plain `-demand` (upstream body-walk) | recursive-content region body (§2.2 excluded) |
| `demand_multi_adorn_allfree_1` | reject all 4 modes (all-free sibling) | an all-free observation root that would read a demand-guarded pub and under-answer |
| `demand_multi_adorn_1` | reject (left-linear; the per-name reject MOVED here at D3.a.3) | left-linear propagation gap |

### Variant A — uniform full materialization (§8.1 literal)

Inadmissible extraction ⇒ the slice stays in the observation root, fully
materialized; the lease scopes a cursor. NO rejects except the one the proposal
ITSELF still wants (§12.3: "Key-changing recursive region request fails
compilation").

- `demand_cyclic_1`, `demand_recursive_content_1`, `demand_multi_adorn_allfree_1`,
  `demand_multi_adorn_1` all FLIP diagnostic → COMPILING full-materialization
  programs. Their goldens become behavioral `.stdout` (answer-correct: the whole
  relation is materialized, the cursor filters).
- **Tension:** pure Variant A conflicts with §12.3, which requires at least one
  rejection witness. Resolve by AUTHORING a NEW `key-changing recursive region
  request` witness (a Stage-D-facing shape) distinct from `demand_cyclic_1`.

### Variant B — retain the recursion feature-gap rejects (hybrid)

`ExtractionPolicy`/admissibility keeps a REJECT for shapes that map to
out-of-scope recursive region calls; only the non-recursive shapes fall back to
full materialization.

- `demand_cyclic_1` → REJECT retained; becomes the §12.3 rejection witness
  (rewritten against region calls: "key-changing recursive region request").
- `demand_recursive_content_1` → REJECT retained (recursive region body,
  §2.2 excluded).
- `demand_multi_adorn_allfree_1` → full materialization (non-recursive;
  answer-correct — the all-free root just materializes the relation).
- `demand_multi_adorn_1` → full materialization (non-recursive).

### Recommendation for the owner (not a decision)

Variant B is the smaller behavior change and it satisfies §12.3 WITHOUT
authoring a synthetic witness: `demand_cyclic_1`'s dataset already exercises the
recursive shape. Variant A is cleaner ("no rejects, one path") but owes a new
witness and silently changes four documented diagnostics to compiles. **OWNER
DECISION OWED.** The exit gate below is written to bind under EITHER variant
(the diagnostic-vs-behavioral golden set is variant-parameterized).

---

## EG. Exit gate (golden-master terms)

The referees: byte-compare, `tests/OptDiff/permcheck.py` (permutation referee),
the I0 reference interpreter, the PRE-CUTOVER TAGGED BINARY (behavioral goldens:
final membership + sorted published deltas), new directed witnesses, and grep
gates. Per surface:

### EG.1 — `.stdout` behavioral goldens (all 180 cases, 4 modes each)

- **Stays byte-identical:** every case's `.stdout` — extraction is answer-neutral
  and the lifecycle preserves answers. The 4-mode cross-agreement is preserved.
  Referee: byte-compare + I0 agreement + the tagged pre-cutover binary.
- **Rewritten drivers (§12.4):** the demand witness `.main.cpp` drivers
  (`demand_neighborhood_witness`, `demand_neighborhood_mono_witness`,
  `demand_diff_input_1`, `demand_diff_neighborhood_witness`,
  `demand_multi_adorn_witness`, `demand_tc_witness`) are rewritten against
  leases / request edges (no `name_retract`, no `-demand*` flags). Their
  `.stdout` is re-blessed to the rewritten driver's output and adjudicated by
  I0 + the tagged binary (final membership + sorted deltas must match the
  pre-cutover run). If the rewritten driver prints the same logical rows in the
  same order, the `.stdout` is byte-identical; only the harness changes.

### EG.2 — oracle / monotone goldens (`.oracle.stdout` / `.monotone.stdout`)

- **Stays byte-identical:** `demand_diff_input_1`, `demand_diff_neighborhood_witness`,
  `demand_neighborhood_witness`, `demand_neighborhood_mono_witness`,
  `demand_tc_witness` all recompute definitionally (`bin/Oracle` +
  monotone projection). Answers are invariant → goldens invariant. Referee:
  `bin/Oracle` byte-compare + permcheck.py for the published-delta permutation
  arm.

### EG.3 — shape goldens (`.rel` / `.df` / `.ir` / `.h`) — LEGITIMATELY CHANGE

- **Changes shape (re-blessed):** the demand-touched shape pins —
  `demand_neighborhood_mono_witness.rel.{opt,nocf,nodf,none}.golden`,
  `demand_tc_witness.{rel.opt,df.opt,ir.opt,h.opt}.golden` — lose the
  `kSubgraphInstantiate`/`kInstanceDeath`/`kInstanceSeal` op family and gain the
  lifecycle op family + the lifecycle census line. Re-blessed via
  `runall.sh --bless` after review. Referee: byte-compare (post-bless) +
  permcheck.py where published-delta order is order-free per epoch.
- **New directed `.rel` goldens:** author at least one lifecycle carrier pinning
  the request-edge / routed-result op surface and the new census (the successor
  to the eleven eager-marker `.rel` pins). Naming: a mono carrier
  (successor to `demand_neighborhood_mono_witness`) and a differential carrier
  (successor to `demand_neighborhood_witness`).
- **The ~38 flag-off bound-query cases:** `.stdout` byte-identical (EG.1); any
  SHAPE pin among them changes and is re-blessed. Verify via I0 that none
  silently changes BEHAVIOR (that would be a finding, not a bless).

### EG.4 — sidecars

- **DELETED:** all `.drflags` (no flags) and all `.eqgate` (no flat/nested
  distinction — the eqgate family of five retires once BOTH referees pass, per
  proposal §12.4). `.batches` datasets are KEPT (rewired witnesses reuse them).

### EG.5 — diagnostic cases (VARIANT-PARAMETERIZED, H-J)

- Under the OWNER-chosen variant, the four demand diagnostics either flip to
  behavioral `.stdout` goldens (Variant A / non-recursive under B) or stay
  expected-diagnostic (Variant B recursive shapes). `runall.sh`'s
  expected-diagnostic list is updated accordingly. Referee: the diagnostic
  byte-compare for retained rejects; I0 + tagged binary for flipped compiles.

### EG.6 — grep gates (deletion-completeness referee)

The tree contains NONE of: `demand__`, `-demand`, `-demand-instance`,
`-demand-retract`, `GuardAnnotation`, `RecognizedSubgraph`, `QueryDemandForcing`,
`guard_annotation_index`, `IsDemandMessage`, `ResolveLiveRecognition`,
`BuildSubgraphInstanceOps`, `kSubgraphInstantiate`, `kInstanceDeath`,
`kInstanceSeal`, `kInstanceRebuild`, `kInstanceDemand`, `kInstanceKeySlot`,
`FabricateDemand*`, `BuildQueryInjectorProcedure`, `name_retract`,
`ForcingMessage`, `InstanceStore` (D8; see E5). No comment or test names those
paths (proposal §14).

### EG.7 — MISSING-ORACLE dependency (do not proceed on vibes)

The differential-pub lifecycle arm (H-G.3) has NO pre-cutover behavioral
witness (`DRInstance::differential == false` program-wide today; the `if (diff)`
pub arm is corpus-unexercised). The tagged pre-cutover binary therefore CANNOT
adjudicate the arm that Stage C makes the only arm. **Exit gate depends on I0
carrying a differential-published-answer demand-instance directed case, OR one
authored at the I0 stage.** This is not optional — it is the oracle for the
single most behavior-bearing hunk. ESCALATION E1.

---

## MECH. Mechanisms carried forward / introduced (necessity-audit input)

### Carried forward (survive Stage C unchanged)

- Rel differential fixpoint — claim gates `TryClaimDel`/`TryClaimAdd`, semi-naive
  rounds, OVERDELETE→REDERIVE→INSERT — is `kLocalFixpoint`'s implementation.
- Ordinary pub `DiffTable` + `kDeleteQueue`/`kAddQueue` + commit sweep — the
  shared-pub result store (results stored once); `RoutedResult` rides it.
- `EmitCommitSweep` / `CompactDead` — dead-row compaction + index rebuild; the
  cursor drain-before-next contract still rests on it (§4c residual).
- Ingest folds (`MakeStageOneIngestFolds` / `MakeMonotoneIngestFold` /
  `MakeIngestLoopOp`) + eager web + 29-kind eager census + Site-5 cross-checks.
- B-3 validators (V-XOVER-ONE/V-PROD-MONO/V-PROD-CLASS/V-JOIN-ONE), V-PRED-XCHECK,
  the `.rel` dump surface — validate the region-body local-graph lowering.
- `StateCellStore` + `GROUP_UPDATE` (aggregates / KV) — untouched by Stage C.
- `ExtractPrimaryProcedure` two-procedure split (§4b Step 9) — Stage C's
  lifecycle lowering rides whatever Stage B decided (open decision §7(6)).
- `NetBatch` SET-semantics ingest; `gPassPolicy.bisect_counter` threading;
  `permcheck.py` permutation referee; `bin/Oracle` derivation-counter oracle;
  monotone-projection oracle.

### Introduced in Stage C

- `RequestEdgeRelation` — THE demand authority (replaces demand-row presence).
- `ActiveInstanceRelation = DistinctProjection(RequestEdgeRelation.child)`.
- `ChildResultRelation` — results stored once (the shared pub).
- `RoutedResultRelation = RequestEdge ⋈ ChildResult` — derived, never
  materialized edges×results.
- `DemandSupportCount` — DEMOTED to a derived summary (never an owner).
- `RootRequestLease` — move-only, one lifetime API; cursor-owned;
  destructor enqueues the exact edge removal (F3 fix).
- Lifecycle ops: `kRequestEdgeAdd`/`kRequestEdgeRemove`, `kInputDelta`,
  `kLocalFixpoint`, `kChildResultAdd`/`kChildResultRemove`,
  `kRoutedResultAdd`/`kRoutedResultRemove`, `kRetireInactive`, `kSealEpoch`.
- Planner: `ExtractPureChild`, `FirstStableAdmissibleChild`, one-level extraction.
- `ExtractionPolicy(candidate)->bool` — NAMED PROVISIONAL SEAM (always-true;
  the cost-model attach point).
- `ChildInstanceId` + lexical `InstancePath` (one level).
- Lifecycle census + validators: V-REGION-ACYCLIC, V-OWNER-EXACT, V-EDGE-BALANCE,
  V-ROUTED-FANOUT, V-ROUTE-EXACT, V-INACTIVE-AFTER, V-PORT-CLOSED,
  V-LIFECYCLE-CENSUS.
- I0 reference interpreter (from the I0 stage) + tagged pre-cutover binary — the
  cutover referees.
- Grep gates (EG.6) — deletion-completeness referee.

---

## ESC. Escalations

- **E1 (MISSING ORACLE, blocking).** The differential-pub lifecycle arm (H-G.3)
  is corpus-unexercised pre-cutover; the tagged binary cannot adjudicate it. I0
  MUST carry a differential-published-answer demand-instance directed case (or
  one must be authored at the I0 stage) before Stage C goldens reference this
  arm. Sharpens proposal §7(1) / review Concern 1.
- **E2 (OWNER-GATED, blocking H-J).** Inadmissible-extraction semantics:
  Variant A (uniform full materialization; owes a new §12.3 rejection witness)
  vs Variant B (retain recursion feature-gap rejects). Enumerated fates in H-J.
  Owner decides; the exit gate binds under either. Review Concern 2 / proposal
  §7(2).
- **E3 (named seam, non-blocking).** `ExtractionPolicy` is provisionally
  always-true; failure mode = message-rooted trivial shapes extracted uselessly
  (answer-correct, shape/perf only). The demand cost model
  ([[demand-cost-model]]) modifies THIS function later. Review Concern 3 /
  proposal §7(3).
- **E4 (ratified, recorded).** `RoutedResult` first realization = shared-pub
  (results stored once, per-owner routing derived — H-G.2), NEVER literal
  edges×results. Review Concern 4 / proposal §7(4); made a concrete hunk here.
- **E5 (Stage-D handoff).** `InstanceStore.h` is deleted as the SEMANTIC model
  (D8). Whether a PHYSICAL per-instance store returns at Stage D (per-
  `(RegionId, InstanceId)` tables) is a Stage-D decision, not a deferred Stage-C
  replacement. The Stage-C one-level shared-pub realization needs no such store.
- **E6 (inherited Stage-B constraint).** `LowerFrozenRegion` must ride whatever
  Stage B decided for the `ExtractPrimaryProcedure` two-procedure split
  (§4b Step 9) under `FrozenRegionalProgram`. Proposal §7(6).
- **E7 (residual, deferred by proposal).** The cursor-vs-`CompactDead`-renumber
  staleness (§4c) is NOT type-closed by the lease — only RETRACTION is. The
  first implementation keeps drain-before-next-entry as CONVENTION (proposal
  §6). Do not let the exit gate over-claim "F3 fully closed": the lease closes
  the demand-vs-cursor SPLIT; cursor staleness against an intervening epoch
  remains conventional.
