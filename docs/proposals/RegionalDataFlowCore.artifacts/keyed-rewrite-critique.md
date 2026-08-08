# Keyed-instance rewrite — adversarial critique of the P1–P9 diffs (ranked)

Session 12 (2026-08-06, tip `46a404d4`). An opus refuter panel (5 per-cluster
refuters + 1 cross-phase completeness critic) tried to BREAK the diffs in
`keyed-rewrite-phase-diffs.md` against real code and the retained
`RegionalDataFlowCore.md` invariants. Every CONFIRMED finding was verified by
opening the cited `file:line`. REFUTED = a refuter tried and the diff HELD
(certification, listed last).

**Tally:** 33 surviving (6 blocking, 12 high, 11 medium, 4 low; 29 CONFIRMED, 4 PLAUSIBLE) + 7 REFUTED (diffs that held).

## Owner headline

The P1 deletion inventory as authored is INCOMPLETE — the atomic cut would not
compile (findings B4/B5/B6, H13, H14, M23, M24). Two blocking SOUNDNESS/reintro
gaps sit in the reconstruction phases: P4 re-provides keyed FullScanFilter
EMISSION only in prose (B1), and P6.5's worklist has no OVERDELETE→REDERIVE path
so a retraction of support for a still-rooted recursive fact is never retracted
(B0 — violates the retained deletion contract). Several exit gates are vacuous
against the P1 baseline (B2, H8/H9/H11, M18/M22/M25). None invalidate the
DIRECTION; all are addressable amendments, enumerated below as `Fix:`.

---

## Surviving findings (ranked)

### F1 · BLOCKING · CONFIRMED · P6.5 · `soundness-gap`

**P6.5's monotone add-only worklist has no OVERDELETE→REDERIVE path, so an input retraction that removes support for a still-rooted recursively-derived fact is never retracted — a self-supporting cycle masks the deletion, violating the retained DELETION CONTRACT.**

- *Failure scenario:* p:-q, q:-p under one live RootLease requesting (p,{K=k}), plus an extensional edge that (transitively) supports the p/q cycle. Retract that edge. The state (p,{K=k}) stays rooted (its RootLease is untouched), so RootedReachability keeps it live. The P6.5 loop only ever does AddDerivation(...,delta.sign) then, on `changed`, DeriveActivationEdge + worklist.push — it never removes an activation edge nor shrinks `live`. The negative delta cannot propagate because RootedReachability is explicitly 'monotone add-only' and the worklist is additive. The cycle's mutual support keeps both facts Present, so they are never retracted — a phantom pair. Fresh evaluation from committed inputs would NOT derive them.
- *Evidence:* keyed-rewrite-phase-diffs.md:1164-1179 (worklist loop: only AddDerivation + push, no edge removal / live shrink) and :1184 (RootedReachability annotated 'monotone add-only'); seed §3 DELETION CONTRACT keyed-rewrite-pseudocode-seed.md:352-355 requires the same least fixpoint as fresh eval; the old flat lowering used differential OVERDELETE→REDERIVE→INSERT with UPDATECOUNT zero-crossings (StackSafeNegation §5.1) which P6.5's 'Reintroduction obligations' (:1203-1204) reduces to 'termination', not retraction. Verified the reintro note only re-provides termination, not overdelete.
- *Fix:* P6.5 must model within-epoch support loss for rooted states: either a real OVERDELETE→REDERIVE (DRed) pass over the semi-naive frontier that shrinks derivations when support crosses to 0 (independent of root liveness), or a from-committed-inputs re-derivation of each affected SCC each epoch. The 'monotone add-only' reachability handles ROOT loss (P6.6) but not INPUT-driven retraction of a fact whose state remains rooted; both paths are required.

### F2 · BLOCKING · CONFIRMED · P4 · `reintro-gap`

**P4 re-provisions the keyed FullScanFilter EMISSION only in prose; no hunk re-adds the ControlFlow region, the Database.cpp region-dispatch arm, an Emit function, or a DR-op lowering that P1.5 deleted — so V-PLAN-HONEST has no emission to match a label against.**

- *Failure scenario:* After P1.5 deletes EmitSubgraphInstance (Database.cpp:2352-2787), the region-dispatch IsSubgraphInstance arm (~765), ProgramSubgraphInstanceRegion, and the visitor overload, a bound @key query has NO codegen path. P4's diff touches ONLY seed pseudocode (§2.4 SelectAccessPlan, §3 EvaluateRule, §4). Its Anchors (phase-diffs:842) are Rel.h:483 / Rel.cpp:1146,2432,4413 / Format.cpp:325 / Database.cpp:2339,2434 — every one a DELETED line, none a Program.h region class, dispatch arm, or new Emit template. So the 'keyed access minted with kFullScanFilter at the successor of Rel.cpp:1146' is a DR-IR op with no ControlFlow lowering and no C++ emission defined anywhere; either it is never lowered (V-PRED-XCHECK aborts on an unmatched DR op) or it is never minted and P4 is inert. Compounding: codegen reads the `lowering` label ZERO times (grep -c 'lowering' Database.cpp = 0) — emission is driven by ProgramRegion KIND, so 'label==emission' also requires codegen to branch on the label, which nothing does.
- *Evidence:* lib/CodeGen/CPlusPlus/Database.cpp: region dispatch is an if/else-if chain on region.IsX() (lines 555-618, plus the IsSubgraphInstance used-table arm at ~765); grep -c 'lowering' Database.cpp = 0 (codegen never consults the DR Lowering/AccessPlan label). P1.5 deletion inventory phase-diffs.md:248,267 removes EmitSubgraphInstance + the dispatch arm. P4 Anchors phase-diffs.md:842 list no Program.h / dispatch / Emit hunk.
- *Fix:* Add explicit hunks to P4: (a) a new ProgramRegion subclass (or reuse of a scan region) for keyed FullScanFilter access, (b) a Database.cpp region-dispatch arm + Emit function re-provisioning the full-scan+key-filter mold, (c) the DR-op → ControlFlow lowering, and (d) either make codegen branch on the AccessPlan label or re-scope V-PLAN-HONEST to check the ProgramRegion shape, since codegen is currently label-blind.

### F3 · BLOCKING · CONFIRMED · P4 · `silent-pass-test`

**Every clause of P4's exit gate is satisfied by the P1 full-materialization baseline with @key inert — the gate cannot distinguish real keyed FullScanFilter evaluation from doing nothing.**

- *Failure scenario:* P4's exit gate: (1) 'evaluates to the correct canonical answer in all 4 modes' — P1.7 itself states full materialization is answer-preserving vs demand and key_tc_witness survives as an ANSWER witness, so the P1 baseline already passes this. (2) 'every keyed access rendered full-scan-filter' and (3) 'count(kFullKeyHashLookup)==0' — both VACUOUSLY true when there are ZERO keyed-access nodes (the stub emits none). (4) 'N declared @key paths emit exactly ONE fact Table_' — the P1 baseline already emits exactly one pub Table (no keyed store exists). Thus an implementation that does NOTHING keyed (reads the full pub table, @key cosmetic) passes the entire P4 exit gate. The gate proves answer-correctness, which P1 already delivers, not that keyed evaluation was reintroduced.
- *Evidence:* phase-diffs.md:825 (exit gate clauses) vs phase-diffs.md:380/395 (P1.7: 'full materialization is answer-preserving vs demand', key_tc_witness kept as ANSWER witness). Confirmed codegen emits one pub Table for a plain relation (no InstanceStore after P1).
- *Fix:* Add a discriminating clause: assert the generated code CONTAINS the full-scan+key-equality-filter loop bound to the declared key (a positive structural grep for the key-filter emission), and a differential test where keyed routing observably differs from a blind full-table read (e.g. a late second requester adds RoutedResults but zero new fact Tables). Answer-equality alone is a lost check post-P1.

### F4 · BLOCKING · CONFIRMED · P1.4/P4 · `missed-deletion`

**P1.4 declares 'RETIRE Lowering::kSectionWalk the placeholder value entirely' but kSectionWalk has a live NON-keyed consumer at Rel.cpp:2433 (ordinary join pivots); the deletion inventory omits it, and P4/P7 directly contradict P1.4 by keeping the value 'for join walks'.**

- *Failure scenario:* Rel.cpp:2433 sets `acc->lowering = pivot_inputs.empty() ? kFullScan : kSectionWalk` for every JOIN with a pivot — the ordinary corpus join path (join_1 etc.), nothing to do with @key. If P1.4 deletes the enum value 'entirely (kFullScan/kPointTest/kSeek remain)' the join path (2433), the V-ALPHA reference (4431), and Format.cpp:325 fail to compile. So the value CANNOT be retired at P1.4. P4 line 832 ('the enum value survives ONLY for honest join range walks at Rel.cpp:2432-2433') and P7 line 1345 ('keep the kSectionWalk enum value HONEST') both assume it is KEPT — a straight contradiction of P1.4's inventory. The mis-framing ('keyed-only placeholder / the lie') also mislabels the join-pivot use, which is not a keyed-instance lie.
- *Evidence:* lib/Rel/Rel.cpp:2433 `pivot_inputs.empty() ? Lowering::kFullScan : Lowering::kSectionWalk` (join pivot, non-keyed); lib/Rel/Rel.h:483 enum; lib/Rel/Rel.cpp:4431 V-ALPHA reads kSectionWalk; lib/Rel/Format.cpp:325 renders it. P1.4 inventory phase-diffs.md:204,226 vs P4 phase-diffs.md:832 vs P7 phase-diffs.md:1345.
- *Fix:* Amend P1.4 to KEEP Lowering::kSectionWalk (only delete its keyed use at Rel.cpp:1146 and the V-ALPHA validator), reconciling with P4/P7. Note the enum value stays load-bearing for join pivots; the 'lie' is confined to the keyed Rederive access, not the enum.

### F5 · BLOCKING · CONFIRMED · P1.6 · `missed-deletion`

**Deleting the GuardAnnotation struct + QueryViewImpl::guard_annotation_index field breaks the guard-annotation CSE-migration machinery in four DataFlow optimizer TUs that P1.6's deletion inventory never enumerates.**

- *Failure scenario:* P1.6 removes struct GuardAnnotation, GuardAnnotationIndex, kNoGuardAnnotation, guard_annotations/guard_annotation_folded_count, and QueryViewImpl::guard_annotation_index, listing only include/DataFlow/Query.h, lib/DataFlow/Query.cpp, Planning.cpp and Regional/Format.cpp. But those symbols are read/written by the CSE fold path in lib/DataFlow/View.cpp (GuardAnnotationsCompatible/PromoteSurvivorToBody/CheckGuardAnnotationFold/PrintGuardAnnotation + the migration block that indexes query->guard_annotations[...] and increments guard_annotation_folded_count), lib/DataFlow/Join.cpp:289, lib/DataFlow/IdentityJoin.cpp:160-162, lib/DataFlow/Link.cpp:230-236 (the edde6c82 ProxyMergedViews guard-preserve fix), plus the free-function DECLS in lib/DataFlow/Query.h:1268-1290. None of these files/decls are in the P1.6 inventory, so the atomic P1 commit will not compile.
- *Evidence:* lib/DataFlow/View.cpp:588-764 (GuardAnnotationsCompatible/PromoteSurvivorToBody/CheckGuardAnnotationFold + CSE migration reading query->guard_annotations[guard_annotation_index] and ++guard_annotation_folded_count); lib/DataFlow/Join.cpp:289; lib/DataFlow/IdentityJoin.cpp:160-162; lib/DataFlow/Link.cpp:230-236; decls at lib/DataFlow/Query.h:1268-1290. P1.6 inventory (phase-diffs.md:320-324) names only include/Query.h, Query.cpp, Regional/Planning.cpp, Regional/Format.cpp.
- *Fix:* Add lib/DataFlow/{View,Join,IdentityJoin,Link}.cpp and the free-function decls in lib/DataFlow/Query.h:1268-1290 to the P1.6 deletion inventory: excise GuardAnnotationsCompatible/PromoteSurvivorToBody/CheckGuardAnnotationFold/PrintGuardAnnotation and every guard_annotation_index/guard_annotations/guard_annotation_folded_count read-write (the whole CSE-migration block), since with the transform gone the field is permanently kNoGuardAnnotation and the machinery is dead.

### F6 · BLOCKING · CONFIRMED · P1.4/P1.5 · `missed-deletion`

**The ControlFlow/Build consumers of the keyed-instance DR vocabulary (LowerSubgraphInstances, the DR-op-walk switch arms, the V-INST-EMITTED cross-check struct, and the Stratum ProgramInstanceStore) are omitted from P1.5's inventory, which hand-waves them as 'lib/ControlFlow/Build/*: the ProgramSubgraphInstanceRegionImpl builder'.**

- *Failure scenario:* P1.4 deletes DROpKind::kSubgraphInstantiate/kInstanceDeath/kInstanceSeal (Rel.h:148-159) and P1.5 deletes ProgramSubgraphInstanceRegion. But lib/ControlFlow/Build/Procedure.cpp:601-604 switches on those exact deleted enumerators inside the V-INST-EMITTED cross-check, LowerSubgraphInstances (Procedure.cpp:279, called :588) iterates dr_flow.SubgraphInstances() (deleted in P1.4 per Rel.h:949) and reads op->AsSubgraphInstance() (Procedure.cpp:196-197), Context::emitted_instance_ops / struct EmittedInstanceOp (Build.h:262-272) stores a DROpKind-cast kind, and Stratum.cpp:2328 constructs ProgramInstanceStore. Since P1 lands atomically as ONE commit, every one of these dangling references to a P1.4/P1.5-deleted symbol fails to compile; the generic 'the builder' line does not enumerate them, and no per-cluster refuter looked at the ControlFlow/Build side (they covered only Database.cpp:766/1930 and Program.h).
- *Evidence:* lib/ControlFlow/Build/Procedure.cpp:196-197 (op->AsSubgraphInstance()), :279 (LowerSubgraphInstances), :588 (its call), :601-604 (switch on kSubgraphInstantiate/kInstanceDeath/kInstanceSeal); lib/ControlFlow/Build/Build.h:264-272 (struct EmittedInstanceOp / emitted_instance_ops, comment names all three deleted kinds); lib/ControlFlow/Build/Stratum.cpp:2328 (ProgramInstanceStore desc). P1.5 inventory only says 'lib/ControlFlow/Build/*: the ProgramSubgraphInstanceRegionImpl builder' at phase-diffs.md:269.
- *Fix:* Enumerate the ControlFlow/Build deletions in P1.5 (and cross-reference the P1.4 enum cut): delete LowerSubgraphInstances + its call (Procedure.cpp:279,588), the kSubgraphInstance/kSubgraphInstantiate DR-op-walk case arms (Procedure.cpp:196,499,601-604), Context::EmittedInstanceOp/emitted_instance_ops (Build.h:264-272) and its V-INST-EMITTED cross-check block, and ProgramInstanceStore (Stratum.cpp:2328) — since after P1.4 no DR op carries those kinds and the cross-check is dead.

### F7 · HIGH · CONFIRMED · P6.1 · `soundness-gap`

**ComputeRecursiveComponents builds SCCs over RelationSchema nodes with edges = rule body→head only, but Stratify closes SCCs through message publish→receive (INSERT-to-stream) seams; a recursion that closes only through a message seam is classified non-recursive and routed to the acyclic P4/P5 single-pass evaluator.**

- *Failure scenario:* A relation whose recursion closes through a publish/receive message round-trip (the case Stratify explicitly names) forms a genuine dataflow SCC. P6.1's node set is RegionTemplate.relation_schemas = 'one per distinct non-demand INSERT/interior decl' (P2), which EXCLUDES messages (they are InputPort/ResultPort, not relation_schemas), and its edge set is only rule body→head deps with no message-seam edge. So the cross-message cycle has no representative edge/node → the component is absent from recursive_components → PlanRecursiveComponent (P6.3) never sees it → it is planned by the acyclic single-pass topological path (P6.5 deletion-obligation :1200 removes the acyclic traversal only for components IN recursive_components), yielding non-termination or an incomplete fixpoint.
- *Evidence:* lib/DataFlow/Stratify.cpp:168 adds `sources[select]<-insert` for every INSERT→SELECT seam unconditionally (and :169-171 records stream seams), so message seams close Tarjan SCCs; Stratify.cpp:110-112 comment names 'SCCs closed only through message publish->receive seams'. keyed-rewrite-phase-diffs.md:961-967 defines nodes=relation_schemas, edges=rule body→head, no seam edge; P2 RelationSchema (phase-diffs :421-424,505-513) is per-INSERT/interior decl, messages are separate InputPort/ResultPort records.
- *Fix:* ComputeRecursiveComponents must add message publish→receive seam edges (the ForEachInsertToSelectSeam edges Stratify already uses) between the relations bridged by a message, or derive recursive_components directly from the Stratify VIEW-level SCC condensation (projected to relations) rather than an independent relation-only rule graph, so the two SCC domains cannot disagree on what is recursive.

### F8 · HIGH · CONFIRMED · P5 · `exit-gate-weak`

**P5's 'unvisited-subset' exit gate checks that @key(A,B,C) shows NO {A,B} state — but {A,B} is a DECLARED PREFIX of [A,B,C] that InternDeclaredPaths explicitly materializes; the test names a prefix (must be present) instead of a non-prefix subset, so it rejects the correct impl and fails to catch lattice closure.**

- *Failure scenario:* P5's InternDeclaredPaths (phase-diffs:863-870) interns the full prefix chain of each declared path: for @key(A,B,C) it materializes schemas/edges for {A}, {A,B}, {A,B,C}. So a CORRECT implementation materializes the {A,B} schema+edge. The exit gate (phase-diffs:932) asserts '@key(A,B,C) never evaluated at {A,B} shows NO {A,B} state materialized' — a referee enforcing that would ABORT the correct implementation. Meanwhile a WRONG lattice-closing impl (materializing {A,C},{B,C},{B},{C}) is NOT caught, because the discriminating subsets are the NON-prefix ones the gate never names. The gate also conflates BindingStateSchema (compile-time, materialized for prefixes) with BindingStateId (value-bearing) within the same sentence, so it cannot cleanly referee either.
- *Evidence:* phase-diffs.md:863-870 (prefix chain materializes every prefix incl. {A,B}) vs exit gate phase-diffs.md:932 (' @key(A,B,C) ... NO {A,B} state materialized'). {A,B} is a strict prefix of [A,B,C].
- *Fix:* Rewrite the unvisited-subset clause to name a genuine NON-prefix subset that must be absent (e.g. for @key(A,B,C): assert {A,C}, {B}, {C} schemas are ABSENT) and assert the declared-prefix chain {A}⊂{A,B}⊂{A,B,C} IS present. Separate the schema-level (compile-time) claim from any value-state claim.

### F9 · HIGH · CONFIRMED · P7 · `soundness-gap`

**V-PLAN-HONEST is anchored at the ValidateDROps tail (pre-codegen) and the access label is advisory, so it structurally cannot enforce label==emission — it degrades to the exact label-self-consistency check it claims to supersede.**

- *Failure scenario:* A codegen arm (or a legacy mold) emits a whole-table scan while the Rel-IR op carries AccessPlan::kFullKeyHashLookup / ExistingTriePrefix. V-PLAN-HONEST, running inside Program::Build's ValidateDROps (Stratum.cpp:2186) BEFORE GenerateDatabaseCode (Main.cpp:146), sees only that the label is in CodegenPlanCapabilities and passes; the dishonest emission ships uncaught. The belt provably cannot 'contain NO whole-table 0..NumRows scan' check because the emitted C++ text does not exist yet at that point.
- *Evidence:* grep -c 'lowering|Lowering|AccessPlan' lib/CodeGen/CPlusPlus/Database.cpp = 0: codegen never reads the label. The Lowering enum is read ONLY at lib/Rel/Format.cpp:324-327 (dump text) and lib/Rel/Rel.cpp:4430-4431 (V-ALPHA). Codegen chooses scan-vs-index from its own maybe_index availability (Database.cpp:3179/3386 emit .Next() vs 3304/3395 emit for(...<NumRows())). ValidateDROps (Stratum.cpp:2186) runs strictly before GenerateDatabaseCode (Main.cpp:146). The P4 diff (phase-diffs.md:770) explicitly anchors V-PLAN-HONEST 'at the ValidateDROps tail'.
- *Fix:* Either move the honesty referee into the codegen emission site (EmitAccessPlan dispatches on plan.kind, making label==emission true by construction and the Rel-tail belt redundant) OR make codegen actually READ the AccessPlan label as the sole scan/index selector so the label is causal; state that V-PLAN-HONEST at the DR-IR tail can only assert label∈CodegenPlanCapabilities, not label==emission.

### F10 · HIGH · CONFIRMED · P7 · `exit-gate-weak`

**Test #17 keys on one exact scan-loop SYNTAX and on mere presence of '.Find('/'.Range(' — a whole-relation ('degenerate') trie scans everything yet passes both halves; a full-scan-labelled-trie is not excluded.**

- *Failure scenario:* A relation reached by a keyed path provisions a TriePrefixIndex whose prefix range covers every row (or an idx.First()/Next() walk over a section equal to the whole table). Codegen emits `for (r : trie.Range(prefix)) ...` — no `for (uint32_t s = 0; s < rel.NumRows(); ++s)` substring — so the grep for the scan pattern finds nothing, and '.Range(' is trivially present, so Test #17 PASSES while the read still visits all NumRows rows. The test proves loop shape, never that fewer than NumRows rows are touched.
- *Evidence:* phase-diffs.md:1335 defines Test #17 as grep for the literal `for (uint32_t s = 0; s < <rel>.NumRows(); ++s)` and 'a .Find(/.Range( keyed emission is present'. Database.cpp:2445 is the only place that exact pattern is emitted (the deleted mold); any other whole-table iteration (range-for, .Next() over a covering index at 3179/3386) evades it. Generated datalog.h contains many unrelated .Find( calls (e.g. 1767, index Next loops), so the presence half is near-vacuous.
- *Fix:* Make the gate cost-structural: assert the keyed read's induced iteration count / provisioned structure key_fields is a strict subset of the relation, or instrument a row-visit counter and assert visited < NumRows for the bound read; require the .Find/.Range emission to be the one FOR the keyed read (tie it to the relation+bindings), not any substring in the file.

### F11 · HIGH · CONFIRMED · P8 · `retained-invariant-violation`

**P8's TrieNode.binding_state:BindingStateId (and CompilePathIntoTrie's BindingStateId(region_instance, canonical_field_SET(bound))) re-collapses the order-free schema vs valued-state split P5 just separated — the trie is a compile-time schema DAG whose nodes are BindingStateSchema, not the values-keyed BindingStateId.**

- *Failure scenario:* CompilePathIntoTrie runs at structure-build time with no runtime values, yet mints TrieNode.binding_state = BindingStateId, which P5 keys on (region_instance, schema, sorted VALUES). With no values there is no well-defined BindingStateId; if instead a distinct BindingStateId is used per runtime value tuple, every value produces a different trie node and the shared-prefix/convergence structure evaporates (goal-6 prefix sharing lost). Either way the compile-time navigation authority (schema) and the runtime identity authority (valued state) are conflated — the very two-authority collapse P5 forbids.
- *Evidence:* phase-diffs.md:1365 (TrieNode{binding_state:BindingStateId}) and :1375 (BindingStateId(region_instance, canonical_field_SET(bound))). P5 seed §2.1 (seed:240-245) + P5 diff:883-889 define BindingStateSchema(region, field_set) as order-free/VALUES-FREE and BindingStateId(region_instance, value_map) as schema+sorted VALUES, and P5 diff:868 builds the compile-time edge DAG over BindingStateSchema nodes. Exit-gate Test #9 (phase-diffs.md:1405) asserts 'one BindingStateId' at compile time — untestable, no values exist.
- *Fix:* TrieNode.binding_state must be a BindingStateSchema (or BindingStateSchemaId) — the order-free, values-free endpoint; CompilePathIntoTrie interns BindingStateSchema(region, SET(bound)); Tests #8/#9 assert one BindingStateSchema id. Reserve BindingStateId (with values) for runtime derivation only.

### F12 · HIGH · CONFIRMED · P2 · `exit-gate-weak`

**P2 collapses V-REGION-CENSUS into a tautology: the census becomes a projection of the very typed records it is supposed to independently recount, so a stubbed/empty planner passes.**

- *Failure scenario:* A stub FrozenRegionalProgram::Build that emits an empty (or arbitrarily-shaped-but-internally-consistent) RegionTemplate sets census = DeriveRegionalCensus(regions) = |R.relation_schemas|=0, |R.input_ports|=0, etc. V-REGION-CENSUS at Rel.cpp:4638 recomputes DeriveRegionalCensus(*frozen_regions) over the SAME records, yielding the same zeros -> equal -> no abort. The stub ships. Today the same stub is CAUGHT: the current DeriveRegionalCensus(query) re-walks the Query independently (CollectContractInserts/CollectMessages/DemandForcings) and finds the real interior contracts, so 0 != real_count aborts.
- *Evidence:* lib/Regional/Planning.cpp:384-398 — DeriveRegionalCensus(const Query&) recounts by re-walking the Query (CollectContractInserts, CollectMessages, DemandForcings), NOT by reading out.ports/out.contracts; this independence is what makes it anti-stub. P2 diff (phase-diffs.md:529-533,556) redefines it as DeriveRegionalCensus(regions){ row_contracts=|R.relation_schemas|, input_ports=|R.input_ports| ... } and has V-REGION-CENSUS recompute the identical projection over the same *frozen_regions. Both the in-Build self-recount (Planning.cpp:665-687) and the Rel re-derive then read the same R vectors.
- *Fix:* Keep a recount path INDEPENDENT of the built vectors: re-derive the census in V-REGION-CENSUS from the DataFlowGraph carrier (the surviving CollectContractInserts/CollectMessages walks over query) and compare against the R-projected census, so a planner that under/over-populates R.relation_schemas vs the actual materialized-insert set still aborts. Do not source both sides of the belt from the same typed records.

### F13 · HIGH · CONFIRMED · P2 · `reintro-gap`

**RelationSchema drops rc.visible_fields, the load-bearing positional bridge the member_key text render requires; member_key alone (view-relative value-ids) cannot be rendered against decl positions, so the P2 byte-identity gate cannot hold.**

- *Failure scenario:* Format.cpp is asked to reproduce member_key_text from RelationSchema{ member_key=rc.member_key (a set of QueryColumnImpl value-ids), fields=FieldsOf(decl) (decl-ordinal SymbolicFieldIds) }. To print the key it must know, for each decl parameter position i, whether that column's value-id is in member_key — but that position->value-id map lived only in rc.visible_fields, which RelationSchema no longer carries. value-id and decl-ordinal are disjoint id domains, so the membership test is unreconstructable; member_key_text renders wrong (or empty), breaking the '.region goldens byte-identical' exit gate P2 leans on as its correctness proof, and denying P3 the schema-order SemanticMemberIdentity it needs.
- *Evidence:* lib/Regional/Planning.cpp:591-608 — render iterates i over rc.visible_fields, skips i>=decl.Arity(), tests rc.visible_fields[i] in rc.member_key, emits decl.NthParameter(i).Name(); needs BOTH visible_fields and member_key. lib/DataFlow/RowContract.h:32-46 — member_key/visible_fields are SemanticMemberKey of view-relative column VALUE ids. phase-diffs.md:505-509 stores member_key=rc.member_key and fields=FieldsOf(decl) but no visible_fields.
- *Fix:* Add visible_fields (the ordered view-relative FieldId list) to RelationSchema, or precompute and store the positional member-key projection (the list of decl ordinals in the key) at freeze so Format.cpp can render without the value-id->position bridge.

### F14 · HIGH · CONFIRMED · P1.2/P1.5/P1.6 · `missed-deletion`

**IsDemandMessage's definition dies with lib/DataFlow/Demand.cpp (P1.1) but two of its three callers are left live; P1.2 mis-states it as 'used only by codegen suppression'.**

- *Failure scenario:* P1.1 deletes lib/DataFlow/Demand.cpp whole, removing the IsDemandMessage definition at :313. P1.5 deletes only one codegen caller (cites Database.cpp:~3693) and omits the second at Database.cpp:1522 (the F2-B(ii) suppression twin). P1.6 explicitly annotates CollectMessages(query) as 'UNCHANGED', but that function calls query.IsDemandMessage(m) at Planning.cpp:164 to filter received messages. After the definition is gone, Database.cpp:1522 and Planning.cpp:164 fail to link/compile in the atomic P1 commit.
- *Evidence:* Definition lib/DataFlow/Demand.cpp:313. Live callers: lib/CodeGen/CPlusPlus/Database.cpp:1522 and :3692, lib/Regional/Planning.cpp:164 (inside CollectMessages, which P1.6 diff marks UNCHANGED at phase-diffs.md:475). P1.2 claim 'used only by codegen suppression, killed in P1.5' at phase-diffs.md:104 is false. Decl is at include/drlojekyll/DataFlow/Query.h:1108, not Parse.h as P1.2 states.
- *Fix:* In P1.5 also delete Database.cpp:1522 arm; in P1.6 remove the !query.IsDemandMessage(m) conjunct at Planning.cpp:164 (post-cut every received message is real, so the filter is a no-op) and correct P1.2 to delete the IsDemandMessage decl from DataFlow/Query.h:1108 after all three callers are gone.

### F15 · HIGH · CONFIRMED · P1.4 · `missed-deletion`

**kSectionWalk is not exclusively the keyed-instance 'lie'; it labels a legitimate bound-pivot access in the SURVIVING flat fixpoint-join lowering, so 'retire the enum value entirely' breaks/relabels a live path.**

- *Failure scenario:* P1.4 declares Lowering::kSectionWalk 'THE LIE' only on the Rederive arm (Rel.cpp:1146) and retires the enum value whole. But Rel.cpp:2433 assigns acc->lowering = pivot_inputs.empty() ? kFullScan : kSectionWalk inside the general JOIN pivot-side access-plan spine builder (acc->ctx = Ctx::kFixpoint), a core flat recursive-join path independent of BuildSubgraphInstanceOps (:1038-1200). Removing the enum value breaks compile at :2433, or — worse for Goal 5 — forces the honest 'bound-pivot section scan' label to be conflated with kFullScan.
- *Evidence:* lib/Rel/Rel.cpp:2433 (acc->lowering = pivot_inputs.empty()? kFullScan : kSectionWalk) with acc->ctx=Ctx::kFixpoint and pivot_inputs built from jv.NthInputPivotSet at :2413-2415 — a general join path. kSectionWalk enum at lib/Rel/Rel.h:483. P1.4 inventory (phase-diffs.md:204,219) cites only :1146 and :483.
- *Fix:* Do not retire the kSectionWalk enum value in P1; delete only the instance-lowering mint at Rel.cpp:1146. Rel.cpp:2433 is a legitimate access label the flat join path still needs. If the value is to be renamed for honesty, that is a P4/P7 concern, and Rel.cpp:2433 (+ the V-ALPHA reference at :4431 / Rel.h:497) must be part of that edit.

### F16 · HIGH · CONFIRMED · P2/P6.1 · `reintro-gap`

**recursive_components is populated TWICE from two DIFFERENT SCC domains — P2 fills it from the nonexistent query.MultiViewStrata() (DataFlow VIEW-level strata), P6.1 overwrites it from a RelationSchema-level Tarjan — and no phase reconciles the element type or deletes the P2 population.**

- *Failure scenario:* P2 (phase-diffs:515) does `for scc in query.MultiViewStrata(): R.recursive_components.push(RecursiveComponent{scc})`, seeding VIEW*-level SCC elements; P6.1 (phase-diffs:966-976) computes recursive_components from a SEPARATE RelationSchema-level rule-dependency Tarjan and explicitly declares the two SCC domains 'never merge'. The RecursiveComponent element shape (a VIEW set at P2 vs a RelationSchema set at P6.1) is incompatible, yet P6.1's deletion inventory (phase-diffs:993) only retires 'the P3 latent assumption that recursive_components is always empty' — it never lists deleting P2's MultiViewStrata population. So either P2's wrong-domain content survives into P3/P4/P5 (which P2's exit gate never validates as P6-usable) or P6.1 silently double-fills. Worse, query.MultiViewStrata() does not exist: the only real accessors are QueryView::Stratum() (Query.h:374) and Query::NumStrata() (Query.h:1114) — so P2's own hunk cannot compile against tip.
- *Evidence:* phase-diffs.md:515 (query.MultiViewStrata()) and :577 (calls it Stratify's predicate SCCs) vs :966-976 (ComputeRecursiveComponents over RelationSchema nodes) and :977-981 ('the two SCC domains never merge'); grep for MultiViewStrata across include/ lib/ returns zero hits — real accessors are Query.h:374 (Stratum) / Query.h:1114 (NumStrata). P6.1 deletion inventory phase-diffs.md:993 omits the P2 population.
- *Fix:* Pick ONE producer of recursive_components: either P2 leaves it empty (a reserved typed field) and P6.1 is the sole populator via the RelationSchema-level Tarjan, or P2 populates it from the real relation-level graph (not query.MultiViewStrata, which does not exist). Fix the P2 anchor to a real accessor, define the RecursiveComponent element type once (RelationSchema-set), and add 'delete the P2 recursive_components population' to P6.1's deletion inventory.

### F17 · HIGH · CONFIRMED · P3/P4/P5 · `ordering-hazard`

**P3 seeds every request edge to the EMPTY binding state, P4's EvaluateKeyedRequest consumes a keyed dest_state with st.bindings, but no phase re-wires the request-edge seed from empty to a keyed terminal BindingStateId — the machinery that interns non-empty binding states is P5, so P4's keyed-filter path is structurally unreachable.**

- *Failure scenario:* P3 mints `seed_request_edge(lease, EmptyBindingState(R))` (phase-diffs:640) for every bound query and states keyed reads 'route through the empty state's FullScanFilter (P4)' (phase-diffs:738). P4's EvaluateKeyedRequest (phase-diffs:804-811) takes `st = e.dest_state` and filters `FullScanFilter(canonical_input(st.relation), st.bindings)`. But every e.dest_state is still the EMPTY state (P4's diff contains no hunk re-seeding the P3 request edge to a keyed terminal state), so st.bindings is empty and the 'key filter' degenerates to a bare full scan — the bound query's actual bound column VALUES never enter a binding state. Non-empty BindingStateId interning (schema + sorted values) is not introduced until P5 (phase-diffs:885-889). Thus P4's central deliverable ('re-provide keyed relation-local evaluation') cannot filter by key at all; this is the structural mechanism behind the separately-reported 'P4 exit gate is answer-vacuous' finding — the keyed path is dead, not merely untested.
- *Evidence:* phase-diffs.md:640 (seed_request_edge to EmptyBindingState), :738 (P3 defers keyed dest_states to P4/P5), :804-811 (EvaluateKeyedRequest reads e.dest_state/st.bindings) with no re-seed hunk; P5 BindingStateId-with-values interning at phase-diffs.md:885-889; P5 reintro note :946 confirms 'P4 provided only the TERMINAL complete-path BindingStateId' — but P4's diff never actually builds it on the request edge.
- *Fix:* P4 must include a hunk that re-seeds the bound query's RequestEdge from EmptyBindingState to the terminal keyed BindingStateId built from the query's bound field/value bindings (the seed §4 'st = BindingStateId from P's field/value bindings' step), and P4 must therefore pull the minimal BindingStateId-with-values interning forward from P5 (or P5 must move ahead of P4). Otherwise state explicitly that P4 is a full-materialization no-op and the first real keyed filter lands at P5, and re-scope P4's 'keyed evaluation' claim + exit gate accordingly.

### F18 · HIGH · PLAUSIBLE · P3 · `soundness-gap`

**P3's acyclic-slice activation assert is incoherent with 'only the empty-binding state exists': every intra-region derivation becomes a self-loop on the one state, so the assert misfires on ordinary acyclic multi-relation programs.**

- *Failure scenario:* For a nonrecursive program (edge; q:-edge; p:-q) P3 has one region instance with one empty-binding state st. EvaluateEpoch runs DeriveActivationEdge(st, f, rule, dest_state_of(f)) for each new fact (phase-diffs.md:696); with a single state, source_state==dest_state==st for q and p. The acyclic guard assert !ActivationReachable(dest_state, source_state) (phase-diffs.md:661) is ActivationReachable(st,st): reflexively true, or true after the first self-edge is interned — so it aborts on the second (or first) derivation, though the program is a DAG. Alternatively, if activation edges are suppressed to avoid this, RootedReachability (phase-diffs.md:663-671) never grows past the seeded roots and multi-hop RouteResults is unreachable.
- *Evidence:* phase-diffs.md:601 (title 'one empty-binding state per live region instance'), :610 (BindingStateId 'only the EMPTY-binding state exists'), :657-662 (DeriveActivationEdge acyclic assert), :693-697 (EvaluateEpoch mints activation edges per new fact). seed §2.2 keyed-rewrite-pseudocode-seed.md:258-262 says both activation endpoints share ONE RegionInstanceId — so activation runs over binding states, of which P3 has exactly one.
- *Fix:* Either (a) do not mint intra-state activation edges in P3 (materialize the acyclic rule chain directly and reserve activation edges for cross-binding-state hops introduced in P4/P5), or (b) define the P3 acyclic guard over the relation/fact dependency DAG, not over binding-state reachability, so a single-state region does not degenerate to self-loops.

### F19 · MEDIUM · CONFIRMED · P6.1 · `silent-pass-test`

**P6.1's exit gate 'adding OR removing a bound query leaves the partition byte-identical' is vacuous after P1, because P1 deletes all demand machinery so a bound query can no longer influence the dataflow/rule graph at all — the test certifies nothing about the new SCC computation.**

- *Failure scenario:* A completely stubbed ComputeRecursiveComponents (e.g. returns everything, or nothing) still passes 'byte-identical under add/remove query', because post-P1 the graph the SCC is computed over is provably query-independent by construction (demand transform gone). The gate that is supposed to witness 'query adornments contribute NOTHING to the partition' can never fail regardless of implementation correctness.
- *Evidence:* keyed-rewrite-phase-diffs.md:986 exit gate; P1.1 invariant keyed-rewrite-phase-diffs.md:67 ('graph is byte-identical to the pre-cut demand-OFF build' — no query influence remains); the substantive claim (SCC admits differently-keyed siblings, :975) is the part that actually needs a directed witness and is not what the byte-identical gate checks.
- *Fix:* Replace the vacuous invariance gate with a positive directed assertion on the co-recursive example: recursive_components == {p,q} as one component AND each of p,q is correctly marked recursive, cross-checked against the Stratify VIEW-level SCC projected to relations (so a stub that over- or under-groups fails).

### F20 · MEDIUM · CONFIRMED · P5 · `soundness-gap`

**P5's completeness exit gate ('unbound read BEFORE any keyed state is materialized returns the complete answer') never exercises the ActiveSubset trap — an unbound read AFTER some binding states are visited that enumerates only active states.**

- *Failure scenario:* The retained invariant is 'do not implement an ordinary unbound read by enumerating only active states'. The dangerous case is: materialize state {A=a}, then perform an unbound read — a buggy impl returns only rows with A=a instead of the complete relation. P5's exit gate (phase-diffs:932) tests completeness ONLY 'before any keyed state is materialized', the one configuration where no active states exist to wrongly enumerate. An implementation that correctly returns the complete answer when nothing is visited but UNDER-answers post-materialization passes the gate. The AccessRequirement diff (phase-diffs:906-910) states the rule in prose but the gate provides no test that binds it.
- *Evidence:* phase-diffs.md:900-910 (AccessRequirement prose: 'NEVER by treating the set of visited states AS the relation') vs exit gate phase-diffs.md:932 ('before any keyed state is materialized'); seed §2.4 retained invariant phase-diffs seed:299-301.
- *Fix:* Add an exit-gate clause: after materializing at least one keyed binding state, perform an unbound read of the SAME relation and assert it returns the COMPLETE relational answer (strictly a superset of any single active state's rows).

### F21 · MEDIUM · CONFIRMED · P5 · `reintro-gap`

**P5 exit-gate test 7 (reorder @key pragmas → byte-identical declaration contract) requires a canonical inter-path render order, but -contract-out currently renders in written-pragma order and no P2/P5 hunk re-sorts it.**

- *Failure scenario:* CLAUDE.md documents '-contract-out emits ONE declared-key line per set (written-pragma order)' and 'the decl formatter round-trips all N pragmas' — so today reordering `@key(A) @key(B)` to `@key(B) @key(A)` produces DIFFERENT contract output. P5's InternDeclaredPaths only makes path-set IDENTITY order-free ('written pragma order irrelevant', assign KeyPathId after validation) but provides no hunk changing the -contract-out / declared-key RENDER order (Regional/Format.cpp:183 badge, RowContract render). So exit-gate test 7 ('byte-identical declaration contract' under pragma reorder) fails against the unchanged written-order renderer.
- *Evidence:* lib/Regional/Format.cpp:183 (declared_key badge render); CLAUDE.md 'declared-key ... written-pragma order'; P5 diff phase-diffs.md:852-858 changes identity only, exit gate phase-diffs.md:932 test 7 demands byte-identical render; no render-canonicalization hunk in P2 or P5.
- *Fix:* Add a hunk canonicalizing the inter-path RENDER order (e.g. sort declared paths by their KeyPathId / lexicographic ordered-field tuple) in the contract emitter — inter-path order is order-free (goal 2 constrains only intra-path order), so sorting the SET of paths is sound and makes test 7 achievable.

### F22 · MEDIUM · CONFIRMED · P9 · `ordering-hazard`

**InferAccessPaths feeds the compile-time deterministic KeyPathId catalog (InternAccessPaths) from active_binding_states(region) — P6.5's RUNTIME rooted-reachability live set — a producer/consumer inversion that makes inferred paths epoch-dependent and KeyPathId assignment non-deterministic.**

- *Failure scenario:* active_binding_states is the set of states actually rooted-reachable during EvaluateEpoch (runtime, P6.5 worklist). InternAccessPaths assigns KeyPathId 'deterministically AFTER the full set is validated' — a compile-time catalog step. Epoch 1 visits {A}; epoch 2 (more input) visits {A},{B}; the inferred-path set and thus every declared/inferred KeyPathId shifts between epochs. Any -region-out/.rel golden that renders inferred paths, or any downstream keyed on KeyPathId, diverges across epochs/opt-modes — a determinism break the golden-master suite would surface as flakiness or an un-blessable golden.
- *Evidence:* phase-diffs.md:1464 ('for st in active_binding_states(region): ... from LIVE bindings (P6 rooted reachability)') feeding :1467 InternAccessPaths; goal-7 resolution :1489 ('orderings actually VISITED during recursive evaluation become inferred paths'). active_binding_states is produced by P6.5's runtime worklist (phase-diffs.md:1164-1176, seed §3 EvaluateEpoch). The ordering note (:1497) lists P8/P6 deps but never resolves the compile-vs-runtime inversion.
- *Fix:* Drive compile-time inference from a STATIC over-approximation (rule/join structure + declared paths + statically-reachable binding schemas), not the runtime live set; if runtime-visited orders are wanted they must feed a lazy runtime trie mint (no compile-time KeyPathId), keeping the compile-time catalog a pure function of program text.

### F23 · MEDIUM · CONFIRMED · P2 · `exit-gate-weak`

**P2 exit gate asserts '180+ .region.<mode> goldens byte-identical'; only 24 exist and roughly half are demand/pragma cases P1.7 deletes or re-blesses.**

- *Failure scenario:* The gate's confidence rests on a phantom corpus. A regression in typed-record rendering that only touches, say, key_tc_witness (a case P1.7 re-blesses/repurposes) could be masked because the '180+' framing implies broad coverage that does not exist; the true surviving region-golden set after P1 is ~12 (join_1, merge_2, tc_nonlinear_diff x4).
- *Evidence:* ls tests/OptDiff/goldens/*.region* = 24 files: 6 cases (demand_multi_adorn_witness, demand_tc_witness, join_1, key_tc_witness, merge_2, tc_nonlinear_diff) x 4 modes. CLAUDE.md itself says '20 .region.<mode> goldens'. phase-diffs.md:572 and seed §4 P2 both say '180+'. demand_*/key_tc are deleted/re-blessed by P1.7 (phase-diffs.md:347-355).
- *Fix:* Restate the gate with the real count (~24 pre-P1, ~12 surviving), and enumerate which specific region goldens must be byte-identical vs re-blessed, so the byte-identity claim is checkable.

### F24 · MEDIUM · CONFIRMED · P1.6 · `missed-deletion`

**P1.6 deletes struct RecognizedSubgraph + Query::RecognizedSubgraphs() but omits the DataFlow-side .df render that reads them (lib/DataFlow/Format.cpp), naming only the Regional Format.cpp.**

- *Failure scenario:* The .df dump's demand-adornment render loops over qc.query.RecognizedSubgraphs() and reads rs.demanded_decl at lib/DataFlow/Format.cpp:1747-1765. P1.6 lists lib/Regional/Format.cpp in its inventory but not lib/DataFlow/Format.cpp, so deleting the struct/accessor breaks this TU. The demanded-interior belt at lib/DataFlow/Build.cpp:2677-2678 (reads impl->recognized_subgraphs) is only mentioned as a 'delete or leave as a no-op' reintro note, but it cannot be a no-op since it dereferences the deleted field.
- *Evidence:* lib/DataFlow/Format.cpp:1747-1765 (for RecognizedSubgraph rs : qc.query.RecognizedSubgraphs(); rs.demanded_decl); lib/DataFlow/Build.cpp:2677-2678 (for rs : impl->recognized_subgraphs; assert reachable.count(rs.demanded_decl.Id())). P1.6 inventory at phase-diffs.md:320-324 omits lib/DataFlow/Format.cpp.
- *Fix:* Add lib/DataFlow/Format.cpp (the .df RecognizedSubgraphs render, :1747-1765) and lib/DataFlow/Build.cpp:2677-2678 (the demanded-interior belt — delete outright, not no-op) to the P1.6/P1.1 deletion inventories.

### F25 · MEDIUM · CONFIRMED · P1.5 · `anchor-wrong`

**P1.5 names the wrong IsSubgraphInstance dispatch arm: the EmitSubgraphInstance emit-dispatch is at Database.cpp:1930, while :766 (cited as 'the region-dispatch arm') is actually the used-table/effects collector arm.**

- *Failure scenario:* Two IsSubgraphInstance() arms must be deleted: the effects/used-state collector at Database.cpp:766 and the actual EmitSubgraphInstance dispatch at :1930. P1.5's diff and Anchors cite only :766 and label it 'region dispatch arm (:766-767)', but the region that actually calls EmitSubgraphInstance is at :1930. A literal follow of the inventory leaves the :1930 emit arm (which calls the deleted EmitSubgraphInstance) in place → dangling call to a deleted function.
- *Evidence:* lib/CodeGen/CPlusPlus/Database.cpp:766 (used-table collector: si.InputTable()) vs :1930 (EmitSubgraphInstance(ProgramSubgraphInstanceRegion::From(region))). P1.5 text/anchors at phase-diffs.md:248,267,275 cite :766 only.
- *Fix:* P1.5 must delete BOTH IsSubgraphInstance arms: the effects collector at Database.cpp:766 AND the emit dispatch at :1930 (the one calling EmitSubgraphInstance); fix the anchor.

### F26 · MEDIUM · CONFIRMED · P1.6 · `silent-pass-test`

**The V-REGION-CENSUS exit gate P1.6 leans on is tautological post-cut: both the stored census and the Rel recount call the identical DeriveRegionalCensus(query), and request-ports collapses to a 0==0 comparison a stubbed planner also passes.**

- *Failure scenario:* P1.6 sets request_ports = 0 as a constant inside DeriveRegionalCensus and cites 'V-REGION-CENSUS (Rel.cpp:4634) does not abort on any surviving case' as evidence the cut is sound. But Planning.cpp:661 stores out.census = DeriveRegionalCensus(query) and Rel.cpp:4640 recomputes DeriveRegionalCensus(query) and compares — same function, same input, so the identity is trivially true regardless of correctness. The only non-trivial referee (Planning.cpp:683 check_count built-vs-derived) also degenerates to 0==0 for request-ports once the minting loop is deleted. An empty-shell planner passes.
- *Evidence:* lib/Regional/Planning.cpp:661 (out.census = DeriveRegionalCensus(query)) and :683-687 (check_count); lib/Rel/Rel.cpp:4640 (const RegionalCensus expect_census = DeriveRegionalCensus(query)) comparing to context.frozen_census, itself the same call.
- *Fix:* Do not treat V-REGION-CENSUS as a P1 soundness gate for request-ports; it only regains teeth at P2 when DeriveRegionalCensus(regions) counts the typed records independently of Build's minting. Note this transient vacuity explicitly, or bring the P2 signature change forward so the recount and the built counts have independent derivations.

### F27 · MEDIUM · CONFIRMED · P2/P5/P7/P8/P9 · `silent-pass-test`

**The desired-IR-output-states doc (keyed-rewrite-ir-desired-states.md) and the critique doc (keyed-rewrite-critique.md) that the seed and phase-diffs cite as existing are ABSENT, so the new IR surfaces introduced across phases have no pinned desired state and no exit gate byte-pins them.**

- *Failure scenario:* Multiple phases introduce or mutate IR surfaces: P5 adds a '-region-out extension' binding-state dump (phase-diffs:932), P7 changes .rel/generated-C++ via AccessPlan render + EmitAccessPlan (phase-diffs:1310-1335), P8 adds trie structure, P9 adds kDeclared/kInferred provenance to the -contract-out declared-key lines (contradicting CLAUDE.md's 'ONE declared-key line per set, written-pragma order'). Each phase's exit gate describes these surfaces only in prose ('a binding-state dump shows...', 'grep the generated datalog.h') with no committed golden and no desired-state reference. The seed (line 21) and phase-diffs (line 9) both point to keyed-rewrite-ir-desired-states.md / keyed-rewrite-critique.md as the pinning authority, but neither file exists in the artifacts directory — so the repo's own predict-then-verify / golden-master discipline has no target for any post-P1 IR surface. A stubbed or drifting renderer passes every exit gate because nothing byte-compares its output.
- *Evidence:* ls of docs/proposals/RegionalDataFlowCore.artifacts/ shows neither keyed-rewrite-critique.md nor keyed-rewrite-ir-desired-states.md; seed keyed-rewrite-pseudocode-seed.md:21 references the latter as authored, phase-diffs.md:9 references the former. New-surface exit gates with no golden: phase-diffs.md:932 (P5 binding-state dump), :1335 (P7 grep-only), :1405 (P8 trie tests), :1484 (P9 inferred-path assertions).
- *Fix:* Author keyed-rewrite-ir-desired-states.md (the D4 desired-states) with a per-phase predicted -region-out/.rel/.contract/generated-C++ shape for every new surface (P2 typed-record render, P5 binding-state dump, P7 AccessPlan/EmitAccessPlan, P8 trie, P9 kInferred contract lines), and make each phase exit gate cite a specific committed golden or predicted dump rather than prose; also reconcile P9's inferred-path render with CLAUDE.md's -contract-out declared-key format and fix the two dangling doc references.

### F28 · MEDIUM · PLAUSIBLE · P6.3 · `ordering-hazard`

**PlanRecursiveComponent's fuse-vs-joint choice depends on CommonPreservedPrefix computed over SymbolicFieldId classes that P6.2's PromoteSharedSymbolicField mutates; the P6.3 exit gate ('exactly one fixpoint frontier per K, not two') is a structural claim that a non-promoting/promotion-incomplete implementation fails while still producing the correct answer, so the structural and answer checks can pass/fail independently.**

- *Failure scenario:* Same-key co-recursion p(K,X)@key(K), r(K,Y)@key(K), p:-r, r:-p. By P6.2 default, p.K and r.K are DISTINCT SymbolicFieldIds; CommonPreservedPrefix is nonempty ONLY if PromoteSharedSymbolicField has unioned them. If promotion fires partially or after CommonPreservedPrefix reads the classes, the intersection is empty → JointFixpoint (two frontiers) → the 'exactly one frontier per K' gate FAILS even though the residual answer still byte-equals the baseline. Conversely a fusion that fires without a genuinely preserved prefix would collapse two binding states — a correctness risk the answer-only gate might miss.
- *Evidence:* keyed-rewrite-phase-diffs.md:1060-1072 (CommonPreservedPrefix over routes' 'bound-field set carried UNCHANGED'), :1015-1026 (P6.2 default distinct ids, promotion gate), :1082 exit gate demands one frontier AND answer equality as separate assertions; field identity is the promotion-mutated class, making the prefix computation order-sensitive to P6.2.
- *Fix:* Specify that CommonPreservedPrefix runs strictly AFTER PromoteSharedSymbolicField reaches its fixpoint, and make fusion a proven-safe optimization (fusion and joint must be asserted answer-identical on the SAME dataset) so the structural 'one frontier' claim never gates correctness — only performance.

### F29 · MEDIUM · PLAUSIBLE · P2/P3/P4 · `retained-invariant-violation`

**P4 mints RegionalFactId member identity via a bare MemberKey(row) that is not tied to P2's RelationSchema.member_key (SemanticMemberKey = which columns), risking collapse or spurious splitting of semantic member identity across the P2->P4 seam.**

- *Failure scenario:* The retained invariant is 'semantic member identity + SemanticMemberKey'. P2 stores RelationSchema.member_key : SemanticMemberKey = std::vector<FieldId> (Identity.h:52) — the COLUMN SET that identifies a member. P3's RegionalFactId carries a SemanticMemberIdentity (per-fact VALUES, a new type). P4 computes `RegionalFactId(st.region_instance, st.relation, MemberKey(row))` (phase-diffs:809) but MemberKey(row) is a free function with no reference to the relation's RelationSchema.member_key projection. If MemberKey(row) projects on the wrong columns (e.g. AllFields for a keyed relation, or the key columns for a monotone AllFields relation), two rows that are the same semantic member get distinct RegionalFactIds (member identity split) or two distinct members collide (identity collapse) — exactly the invariant P2 typed member_key was meant to carry. No phase's diff wires MemberKey(row) to RelationSchema.member_key.
- *Evidence:* lib/DataFlow/Identity.h:52 (SemanticMemberKey = vector<FieldId>, a column projection); lib/DataFlow/RowContract.h:40,46 (visible_fields/member_key both SemanticMemberKey); phase-diffs.md:421,507 (P2 member_key COPIED from RowContract) vs :613 (P3 SemanticMemberIdentity 'schema-order typed member key') vs :809 (P4 MemberKey(row) with no member_key argument).
- *Fix:* Specify that P4's MemberKey(row) projects the row through the relation's RelationSchema.member_key (the P2 SemanticMemberKey column list) to produce the SemanticMemberIdentity, and add an exit-gate assertion that two rows agreeing on the member_key columns intern to ONE RegionalFactId while rows differing there do not — binding the P3/P4 fact identity to the P2 schema member key.

### F30 · LOW · CONFIRMED · P6.1 · `anchor-wrong`

**The diff says to 'REUSE the iterative frame-stack Tarjan of QueryImpl::Stratify (Stratify.cpp:178-232)', but that Tarjan is inlined inside QueryImpl::Stratify over VIEW*/local `sources` with function-local TarjanState/TarjanFrame — there is no extractable TarjanCondense(g) to reuse.**

- *Failure scenario:* An implementer following the diff looks for a reusable condensation routine and finds none; the cited code is a monolithic method mutating member state (num_strata, views[i]->stratum). 'Reuse' actually requires extracting a generic templated Tarjan first, which the diff does not budget as an obligation.
- *Evidence:* grep shows TarjanCondense does not exist anywhere; TarjanState/TarjanFrame are file-local structs (Stratify.cpp:19,31) and the loop (Stratify.cpp:184-232) reads/writes locals `sources`,`views`,`index_of` and members — not parameterized over an arbitrary graph.
- *Fix:* Add an explicit obligation to extract a generic iterative Tarjan condensation (templated over node/edge accessors) shared by Stratify and ComputeRecursiveComponents, or state that a fresh Tarjan is written for the RelationSchema graph; do not describe it as a drop-in reuse.

### F31 · LOW · CONFIRMED · P2 · `missed-deletion`

**P2 deletion inventory omits the Context::frozen_census field declaration (Build.h:230), which must change type when context.frozen_census becomes context.frozen_regions.**

- *Failure scenario:* The diff retargets Build.cpp:1572 and Rel.cpp:4638 from frozen_census to frozen_regions but the Context struct still declares `const RegionalCensus *frozen_census`; Rel.cpp:4640 still reads `*context.frozen_census` for `stored`. If the field is not re-typed and a new stored-census source wired, the V-REGION-CENSUS block fails to compile or reads a dangling/removed member.
- *Evidence:* lib/ControlFlow/Build/Build.h:226,230 declare `const RegionalCensus *frozen_census{nullptr}`; lib/Rel/Rel.cpp:4638-4640 read context.frozen_census as `stored`. P2 deletion obligations (phase-diffs.md:579-591) list Planning/Regional/Rel sites but not Build.h:230.
- *Fix:* Add Build.h:230 (Context::frozen_census) to the P2 deletion/retarget inventory, and specify where `stored` (frozen.Census()) is now sourced for the V-REGION-CENSUS comparison.

### F32 · LOW · CONFIRMED · P1.1 · `anchor-wrong`

**The ApplyDemandTransform call-site anchor is off by ~40 lines and the proxy_view_to_decl is a Build.cpp local, not a Connect out-write at the cited line.**

- *Failure scenario:* P1.1 cites 'THE DEMAND CUT (call @~2561)' and Build.cpp:2524/2561 anchors, but at tip Query::Build is at Build.cpp:2524 while the actual impl->ApplyDemandTransform(...) call is at Build.cpp:2601 (map declared at :2582, ConnectInsertsToSelects at :2584). Line 2561 is unrelated (RemoveUnusedViews/ClearGroupIDs). A reviewer navigating to :2561 finds the wrong hunk.
- *Evidence:* lib/DataFlow/Build.cpp:2601 (if (!impl->ApplyDemandTransform(module, log, demand_mode, demand_retract, suppress_demand, proxy_view_to_decl))), :2582 (std::unordered_map<VIEW*,ParsedDeclaration> proxy_view_to_decl), :2560-2566 unrelated. Diff cites 2561 at phase-diffs.md:41,85.
- *Fix:* Re-anchor the P1.1 demand-cut call to Build.cpp:2601 (map local at :2582); the proxy_view_to_decl is a Build.cpp stack local passed by reference, not a Connect.cpp:1307 out-write.

### F33 · LOW · PLAUSIBLE · P7 · `anchor-wrong`

**P7 claims Lowering::kSectionWalk 'stays honest for real idx.First()/Next() join walks at Rel.cpp:2432', but codegen never reads Lowering, so whether the join path emits an index walk or a full scan is independent of the label — the honesty is an unverified assumption outside V-PLAN-HONEST's (AccessPlan-only) domain.**

- *Failure scenario:* The join-path access at Rel.cpp:2432 is stamped kSectionWalk whenever pivot_inputs is non-empty, but codegen's actual scan-vs-index choice is made by its own maybe_index availability, not the label. If a pivot join lowers to a full NumRows scan (no usable index provisioned) while wearing kSectionWalk, the label is dishonest and V-PLAN-HONEST — scoped to the disjoint AccessPlan domain — never inspects the Lowering enum, so nothing catches it.
- *Evidence:* phase-diffs.md:828/832 assert kSectionWalk 'kept only for honest idx.First/Next join walks at Rel.cpp:2432'. Rel.cpp:2432-2433 sets lowering=kSectionWalk purely from pivot_inputs.empty(); codegen reads Lowering 0 times (grep -c in Database.cpp = 0) and picks index vs scan via maybe_index (Database.cpp:3179/3386 vs 3304/3395). I did not trace the specific pivot-join emission to confirm it is always an index walk, hence PLAUSIBLE.
- *Fix:* Either bring the surviving Lowering::kSectionWalk join label under an emission-matching check too, or make codegen dispatch on the label; do not claim the residual kSectionWalk is honest without a referee tying it to the emitted idx.First/Next.

---

## REFUTED (diffs that held under attack — certification)

### R1 · P6.6 · `retained-invariant-violation`

**REFUTED — tried to break drain-before-retire and the no-refcount cyclic-liveness rule; the diff holds. Retirement is correctly re-derived from RootedReachability (not support counts), and drain is sequenced strictly before retire.**

- *Why it holds:* None needed for the invariant; but clarify the redundant pair RetractRoutedResults(all_states\live) at :1216 vs DrainRoutedRemovals(all_states\live) at :1218 — the two operations over the identical set have unspecified distinct semantics and should be defined or merged.
- *Evidence:* keyed-rewrite-phase-diffs.md:1218-1223 sequences drained=DrainRoutedRemovals(...) THEN RetireUnreachableSCCs(drained); P6.5 RootedReachability (:1181-1188) seeds `live` only from rooted RequestEdges and closes via activation edges guarded on source_state∈live — after root removal source_state is not live so the cycle is excluded regardless of Present(source_fact)/support>0 (:1232-1237 states exactly this). Ownership stays acyclic and support is a cache, matching the retained invariant.

### R2 · P6.4 · `retained-invariant-violation`

**REFUTED — tried to make the key-changing recursion cycle contaminate the RequestEdge ownership forest; the diff keeps them type-disjoint and acyclic. Both cycle endpoints resolve to one region instance, so no binding state becomes a second fact owner via a request cycle.**

- *Why it holds:* None; the ownership/activation separation is preserved. Recommend the exit gate additionally assert the two endpoints share one RegionInstanceId (currently only asserted in prose at :1111-1113).
- *Evidence:* keyed-rewrite-phase-diffs.md:1110-1113 keeps the edge in activation_edges only, never AddRequestEdge (type-disjoint API, exit gate :1128); the differently-keyed states differ in bound_field_value_map (BindingStateId, seed §2.1:240-245) but share the same region_instance (region R0 + inherited prefix, §2.1:220-222) since key-changing recursion has empty preserved prefix; RequestEdge acyclicity is refereed by the retained Planning.cpp:711 V-OWNERSHIP-ACYCLIC.

### R3 · P5 · `ordering-hazard`

**Attack refuted: P5 does NOT reintroduce a sort that collapses [A,B]/[B,A]; convergence is correctly moved to the state authority and distinction kept at the edge authority.**

- *Why it holds:* None — the diff holds on this axis. Certified: order lives only on BindingEdge, convergence only on BindingStateSchema/Id; no reintroduced path-collapsing sort.
- *Evidence:* phase-diffs.md:883-898 (BindingStateSchema sorts field SET; BindingEdge keyed ordered; no path sort) and :852-858 (tuple(ordered_fields), NO sort). Verified against the two real sort sites Parser.cpp:1477 / Demand.cpp:902 (both order-collapsing) which P5 replaces, not re-uses.

### R4 · P9 · `soundness-gap`

**V-DECLARED-PATH-PRESERVED holds: I could not construct an inferred path that shadows, reorders, or removes a declared @key path.**

- *Why it holds:* Optionally assign KeyPathId to declared paths in a namespace independent of inferred paths so a declared path's id is stable regardless of what inference adds; otherwise no change needed.
- *Evidence:* phase-diffs.md:1444-1454 (InternAccessPaths declared-first, inferred `continue` on dup) and :1469-1473 (declared-with-no-structure returns FullScanFilter). Order-as-identity preserved by :1448 'NO sort — order IS identity'.

### R5 · P3 · `retained-invariant-violation`

**REFUTED: caller-qualified retract holds — two requesters' routed copies of one fact cannot alias, and removing one owner leaves the shared fact and the other owner's copies intact.**

- *Why it holds:* None needed; keep RoutedResultId keyed by (request_edge, fact) and keep derivations keyed by RegionalFactId so the two never alias.
- *Evidence:* phase-diffs.md:621 (RoutedResultId{RequestEdgeId,RegionalFactId}), :651-656 (RemoveRequestEdge erases only rr where rr.request_edge==e), :673-681 (derivations keyed by RegionalFactId, independent of requests). Matches retained RegionalDataFlowCore.md caller-qualified-results invariant.

### R6 · P3 · `retained-invariant-violation`

**REFUTED: activation edges never become request owners, and one-empty-binding-state does not collapse member identity.**

- *Why it holds:* None needed; keep RequestOwnerId's RegionalMember arm carrying a RegionalFactId (not an activation edge) and keep fact identity on RegionalFactId.
- *Evidence:* phase-diffs.md:614-618 (RequestOwnerId variants; RequestEdge vs RuleActivationEdge separate structs), :625-630 (RegionInstanceRelations holds request_edges and activation_edges as distinct maps), :612 (RegionalFactId includes SemanticMemberIdentity). Consistent with retained ownership-acyclic + SemanticMemberKey invariants.

### R7 · P1.3/P1.5 · `reintro-gap`

**REFUTED: the claim that a bound #query falls through to a plain cursor once the forcing registry is emptied holds — the query-entry emitter already no-ops the forcing call when spec.forcing_function is null.**

- *Why it holds:* No change needed; certifies the P1.3/P1.5 reintroduction obligation. (Keep the spec.forcing_function/retract_function fields present-but-null rather than deleting the struct members, so the :1652/:1685 branches stay valid dead code.)
- *Evidence:* lib/CodeGen/CPlusPlus/Database.cpp:1652 (if (spec.forcing_function)), :1673-1676 (emit_forcing_call returns early if !spec.forcing_function), :1685 (retract arm guarded on spec.retract_function, deleted by P1.3).

