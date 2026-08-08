# Keyed-instance rewrite — consolidated whole-program pseudocode + path forward as diffs (post-s13 seed)

Session 13 close (2026-08-06, branch `keyed-instances`, tip `46a404d4` + the uncommitted s11
Phase-0 worktree). This is the SINGLE grounded whole-program view for session 14: it
re-explains the entire compiler pipeline as pseudocode (§1), states the target four-authority
model (§2), and expresses the path forward as P1–P9 diffs at whole-program altitude **with the
session-13 corrections folded in** (D1–D4 resolved, the 5 blocking re-critique findings B1–B5,
and the certified mechanisms) (§3). It is the START-HERE grounding; the hunk-grained detail
hangs off it (`keyed-rewrite-reconstruction-diffs.md`).

State: docs only; **suite 251 PASS unchanged; nothing blessed; ZERO production code touched.**
Anchors re-verified at tip this session (drift table: `reconstruction-diffs.md` §0.1).
Phase 1 is OWNER-GATED and destructive.

Authority chain: `INDEX.md` → `next-session-prompt.md` (semantic authority: `@key` is a
relation-local ORDERED access-path declaration, NOT a query adornment) → **this file**
(whole-program view) → `keyed-rewrite-reconstruction-diffs.md` (P1 compile-clean + P2–P6
operational + §5 amendments) → `keyed-rewrite-reconstruction-critique.md` (the 23 survivors +
20 certifications) → `keyed-rewrite-ir-desired-states.md` (predict-then-verify IR + the 2
no-baseline carriers) → `keyed-rewrite-pseudocode-seed.md` §2/§3 (target algebra).

---

## §1. The whole program TODAY — pseudocode (what we cut FROM)

Compact; per-subsystem implementer-grain pseudocode is in `keyed-rewrite-current-pseudocode.md`.
All `file:line` re-verified at tip.

```
# bin/drlojekyll/Main.cpp — the pipeline
compile(module, flags):
    query   = Query::Build(module, log, policy, demand=flags.gDemand,
                           demand_retract=flags.gDemandRetract)          # Build.cpp:2524
    frozen  = FrozenRegionalProgram::Build(query, log)                   # Planning.cpp:439 (degenerate)
    program = Program::Build(frozen, log, first_id, policy,
                             demand_instance=flags.gDemandInstance)       # Build.cpp:1333
    GenerateDatabaseCode(program, h, cc)                                 # Database.cpp

# ---- DataFlow: Query::Build ----  (THE demand authority lives here)
Query::Build:
    build SELECT/TUPLE/JOIN/MERGE/CMP/MAP/NEGATE/AGG/KVINDEX/INSERT graph
    ConnectInsertsToSelects(log, proxy_view_to_decl)                     # Connect.cpp:164
    ApplyDemandTransform(module, log, demand_mode, demand_retract, ...): # Demand.cpp:388  <== THE CUT
        activation gate: scan PARSED module for HasInstanceKey() -> pragma_activated   # :411-430
        !demand_mode && !pragma_activated -> return                       # containment gate
        bound_queries = [q : IsQuery && any param kBound]                 # :459-471 (RP-6 rejects if empty+pragma)
        per-adornment SIP walk (UniqueRedeclarations); fences
        V-DECLARED-KEY RP-10 bijection: declared set-of-sets == inferred  # :892-959; canon SORTS :905-908
        FabricateDemandMessage/Local under `demand__`                     # Parse/Demand.cpp:170,226
        mint d_p relation + push-down guard JOIN (d_p ⋈ p)
        stamp GuardAnnotation (CSE-migrating) + RecognizedSubgraph; register QueryDemandForcing
    Optimize -> CSE migrates GuardAnnotation via View/Join/IdentityJoin/Link.cpp
    Stratify (iterative Tarjan over VIEW*; neg/agg rejects; V-SCC-SEAM)  # Stratify.cpp:124,168,272-344
    row_contracts = InferConservativeRowContracts (2-phase, PURE)        # RowContract.cpp:365

# ---- Regional: FrozenRegionalProgram::Build (degenerate planner) ----
Build(query, log):                                                       # Planning.cpp:439
    out = FrozenRegionalProgram(query)   # <== stores the Query BY VALUE as the real owner
    request ports/internals: 1 per query.DemandForcings()               # :445 (+ NumForcingsOfName :142)
    input/result ABIs from CollectMessages (filters query.IsDemandMessage) # :164
    row contracts, 3 tiers: R-STORE (friend leak query.impl->row_contracts :574)
        + Tier-1 CollectDemandInteriorDecls (:210, reads RecognizedSubgraphs)
        + Tier-2 CollectOriginInteriorDecls (:298, reads OriginDecls)
    out.census = DeriveRegionalCensus(query)  # :384/661 — pure fn of query; V-REGION-CENSUS recount Rel.cpp:4638

# ---- ControlFlow: Program::Build (flat/nested selector) ----
Program::Build(frozen, ..., demand_instance):                           # Build.cpp:1333
    query = frozen.Query()                                               # H4 unwrap :1337
    feature-gap pre-pass (agg/kv/map/product-in-scc rejects)            # :1345-1437
    per-forcing keyed fences from LIVE GuardAnnotations (cyclic_demand/recursive_content) # :1439-1519
    RP-9 silent pragma->nested; context.demand_instance_enabled          # :1532-1568 (THE selector bit)
    context.demand_forcings = &query.DemandForcings()                    # :1560 (Build.h:127 field)
    BuildQueryInjector* from the QueryDemandForcing registry             # :413-527 (ForcingMessage arm :522-527 KEEP)
    IsCutSuccessorDR: demand_instance_enabled && GuardAnnotationIndex()  # Rel.cpp:1571-1576 (eager-walk chain-break)

# ---- Rel: nested keyed lowering (gated on demand_instance_enabled) ----
ResolveLiveRecognition(query)          # Rel.cpp:936 — ABA-safe shape recovery from live guard JOINs
BuildSubgraphInstanceOps(...)          # Rel.cpp:1038 — mint kSubgraphInstantiate/kInstanceDeath/kInstanceSeal
    Rederive arm ACCESS input_table lowering=kSectionWalk               # Rel.cpp:1146 (THE label the scan makes true)
# ControlFlow lowering: LowerSubgraphInstances (Procedure.cpp:279) -> ProgramSubgraphInstanceRegion
# CodeGen: EmitSubgraphInstance (Database.cpp:2352) -> emit_instance_rescan FULL-SCAN mold (:2434)
#   dispatch: CollectEffects arm (:766) + EmitRegion emit arm (:1930); caveat (:2339-2341)
# Runtime: InstanceStore<Key,RowT> (InstanceStore.h:54) — a LEAF ROW CACHE keyed by the COMPLETE key:
#   no prefix states, no recursion, no request routing, no shared canonical facts

# THE IDENTITY GAP: keyed lowering reduces facts/derivations/requests/forcing-indexes/handles
# to GuardAnnotation/RecognizedSubgraph side-records + raw ints; FrozenRegionalProgram is a
# 7-count census + 5 render-string vectors + a Query pass-through — it OWNS no regional semantics.
```

---

## §2. The target — four authorities + two edges + EvaluateEpoch (what we cut TO)

Operational algebra in `keyed-rewrite-pseudocode-seed.md` §2/§3. In one screen:

```
FOUR AUTHORITIES, never collapsed:
  Logical fact          RegionalFactRelation : RegionalFactId -> RegionalFact  (SINGLE fact authority)
  Residual              BindingState (owns frontiers + FactDerivation ids)      (NOT a 2nd fact owner)
  Logical access path   DeclaredAccessPath (ORDERED fields; [A,B] != [B,A])
  Physical structure    AccessPlan (FullScanFilter | FullKeyHashLookup | trie | ...)
TWO EDGE RELATIONS, never unified:
  RequestEdge        exact ownership, ACYCLIC forest (RootLease/PermanentRoot/RegionalMember)
  RuleActivationEdge derivation dep, MAY CYCLE in a region; liveness = ROOTED reachability
IDENTITY:
  BindingStateSchema keyed on the field SET (order-FREE, compile-time); BindingStateId adds sorted VALUES
  (runtime); DeclaredAccessPath ORDERED; [A,B]&[B,A] converge on ONE {A,B} schema; @key(A) shares the
  {A} prefix of @key(A,B). SemanticMemberIdentity = the RowContract member_key projection (schema order).
EvaluateEpoch = least fixpoint of (rooted-reachability ∘ semi-naive derivation), order-independent;
  deletion computes the SAME least fixpoint as fresh-from-committed (DRed / affected-SCC recompute);
  drain routed removals BEFORE retiring an unreachable activation SCC; counts are CACHES, never the oracle.
```

RETAINED (do not break): SemanticMemberKey identity; exact RequestEdge ownership + multiple
owners + late attach + caller-qualified results + drain-before-retire; pure-region/effect
boundary + epoch order-independence; ownership forest acyclic. SUPERSEDED:
recursion-stays-in-one-instance; demand-has-no-source-annotation; no-cyclic-binding-routing.

---

## §3. The path forward as diffs (whole-program altitude, post-s13)

Each phase is a diff on §1 → §2. Hunk grain is in `keyed-rewrite-reconstruction-diffs.md`; the
s13 corrections (D1–D4, B1–B5, certifications) are folded here. `⊗` = a session-13 correction.

### P0 — lock the language contract (parser-local) [partially landed; item 4 remains]

```
  DONE (uncommitted): reject @key on #query (Query.cpp state 6); full-context redecl consistency
⊗ REMAINING item 4 (order-significant paths) is a HARD PREREQUISITE of P5 (H6, empirically
  confirmed): the parser dup rejects SORT at BOTH Parser.cpp:974-995 (same-decl) AND
  Parser.cpp:1477-1488 (cross-redecl), upstream of demand — so @key(A,B) @key(B,A) rejects AT
  PARSE today. Flip both to order-significant ([A,B] != [B,A], exact-tuple repeat still rejects);
  re-bless/retire reject_key_double_1's order-free claim. Landable AHEAD of P1.
```

### P1 — delete the demand authority [DESTRUCTIVE, owner-gated]

```
- DELETE: ApplyDemandTransform + Demand.cpp TU; FabricateDemand*/demand__ registry; QueryDemandForcing;
  GuardAnnotation/RecognizedSubgraph/GuardAnnotationIndex; the Rel kSubgraphInstantiate/kInstanceDeath/
  kInstanceSeal nested lowering + ResolveLiveRecognition/BuildSubgraphInstanceOps; the keyed kSectionWalk
  USE at Rel.cpp:1146 (KEEP the enum value — join pivots at Rel.cpp:2432-2433 need it); InstanceStore.h +
  EmitSubgraphInstance + the full-scan mold; -demand/-demand-retract/-demand-instance + the flat/nested selector.
⊗ B1 — the atomic cut STILL won't compile without these un-enumerated consumers (all folded, diffs §5.1):
    BuildQueryInjectorFromRegistry (Build.cpp:413-503) + registry branch (:507-519) [PRESERVE the
      ForcingMessage fall-through :522-527]; Context::demand_forcings field (Build.h:127);
    DeriveRegionalCensus's query.DemandForcings() (Planning.cpp:388 -> literal 0); the request-port loop
      (Planning.cpp:445-468) + NumForcingsOfName (:142); QueryDemandForcing (Query.h:985) + DemandForcings()
      decl (:1093) + demand_forcings member (:1222);
    IsCutSuccessorDR (Rel.cpp:1571-1576) drop the demand_instance_enabled && GuardAnnotationIndex() conjunct;
    the declared-key renderer is the WHOLE block DataFlow/Format.cpp:1745-1798 (-contract-out, not .df);
    QueryView::GuardAnnotationIndex() DEF is Query.cpp:353; IsDemandMessage's 3 callers
      (Database.cpp:1522/:3692, Planning.cpp:164); the demanded-interior belt Build.cpp:2677-2678.
⊗ B1 golden fallout (diffs §5.2): re-bless key_tc_witness.contract + key_multi_adorn_witness.contract to
  drop the declared-key lines at the P1 tail (the SIP inferred= half is unreconstructable).
  Post-P1 honest baseline: @key inert; a bound #query reads the canonical materialized relation via the
  plain cursor. Keyed/residual eval NON-FUNCTIONAL until P4. STRONGEST anti-silent-pass belt = the .rel
  census multiset (kSubgraphInstantiate/kInstanceSeal/kIngestFold exact per carrier).
⊗ s14 re-grep — the cut is STILL NOT compile-clean: 7 MORE un-enumerated consumers survive (verified at
  tip, reconstruction-diffs §6): (§6-1) the Rel-IR dump emitter Rel/Format.cpp:114-116/865-903/1131-1132
  (L2 caught only the DataFlow renderer twin); (§6-2) Rel.cpp DROpStratum :4794-4807 + key_of :5363
  enumerator arms; (§6-3) the ProgramSubgraphInstanceRegion/ProgramInstanceStore DEFINITION TUs
  ControlFlow/{Operation,Program,Format,Visitor}.cpp (P1.5 enumerated only header+Database.cpp+Build/*);
  (§6-4) the LIVE CALL Planning.cpp:304-306 CollectOriginInteriorDecls(RETAINED)->CollectDemandInteriorDecls(DELETED);
  (§6-5) tests/DataFlowValidators/GuardAnnotationFoldTest.cpp; (§6-6) the five tests/RelValidators/Instance*Test.cpp.
  DURABLE FIX: a symbol-driven `git grep -l <deleted-symbol>` P1 pre-commit acceptance gate (each must
  return ONLY inventory files), not range-anchored review. B2–B5 CERTIFIED internally consistent (§6.1).
```

### P2 — FrozenRegionalProgram becomes the typed semantic owner

```
+ RegionTemplate{RelationSchema[], rules(RuleRoutingProjection), recursive_components(RESERVED EMPTY -> P6.1),
  input/result/request ports, permanent_roots}; RelationSchema{member_key(SemanticMemberKey typed),
  visible_fields, declared_key_positions(PRECOMPUTED at freeze so Format renders w/o the value-id/ordinal
  bridge — F13), declared_access_paths, support}.
⊗ H1 — do NOT retype Context::frozen_census (keep const RegionalCensus*; its reader Rel.cpp:4638-4662 reads
  a RegionalCensus; F12 keeps the census query-derived). Add a separate frozen_regions field only later.
⊗ H2 — V-REGION-CENSUS at the Rel tail is TAUTOLOGICAL (f(query)==f(query)); re-provide the in-Build
  internal recount reading the NEW typed RegionTemplate (was Planning.cpp:663-687, deleted with the vectors).
⊗ M1 — query.PublicRowContracts() does NOT retire the friend leak (RowContractMap/QueryViewImpl are private);
  keep the friend access (contained to lib/Regional) OR return an opaque {decl,visible_fields,member_key} vector.
  D2: recursive_components has ONE producer (P6.1); P2 leaves it empty (CERTIFIED: no consumer).
```

### P3 — RequestEdge / RuleActivationEdge / FactDerivation / RoutedResult (acyclic slice, over full materialization)

```
+ typed ids (RootLeaseId/PermanentRootId/BindingStateId/RegionalFactId/RequestEdgeId/RuleActivationEdgeId/
  FactDerivation/RoutedResultId) on RegionInstanceRelations; one empty-binding state; activation = acyclic slice.
⊗ B2 (BLOCKING) — permanent roots MUST be request owners: AddRequestEdge(PermanentRoot(id), ..., EmptyBindingState)
  with RootAlive(PermanentRoot)=true, else RootedReachability leaves live={} and the ~73% no-bound-query corpus
  publishes NOTHING. Add a no-bound-query publish probe to the exit gate.
⊗ H3 — AddDerivation's fact identity projects the row through declared_key_positions (the F13 positional bridge),
  NOT raw member_key value-ids (FieldId is "not an array index"). F29 and F13 share ONE positional projection.
⊗ M3 — P3 EvaluateEpoch LAYERS request/activation tracking over the RETAINED full-materialization backend
  (the landed induction fixpoint); the abstract topo-sweep evaluator is scoped to the acyclic/keyed slice.
  F18: P3 mints NO intra-state activation edges (the rule DAG carries order); the acyclic assert becomes live
  only once cross-state edges exist post-P4/P5. R5: caller-qualified retract holds (CERTIFIED).
```

### P4 — honest complete-path specialization (FullScanFilter, label==emission by construction)

```
+ REUSE ProgramTableScanRegion (public handle Program.h:1097-1140, Impl :1601) with index=nullopt: the
  EXISTING region.IsTableScan()->EmitScan dispatch (Database.cpp:1943->3374-3418) emits the honest
  NumRows-scan + key-equality filter (:3403-3418) — NO new dispatch arm, NO relabeled walk. D4 Option-1:
  label==emission by construction (codegen is label-blind, grep -c lowering Database.cpp=0 — CERTIFIED).
⊗ B3 (BLOCKING) — the exit gate MUST discriminate against the P1 baseline (whose cursor at Database.cpp:1768-1799
  ALREADY emits a NumRows scan + key filter): key on the region-cursor s<id> shape (vs the query-cursor pos)
  AND a -region-out compile-time RequestEdge/RoutedResult assertion (RoutedResult has no datalog.h surface at P4);
  drop the "caveat gone" probe (that line is deleted at P1.5, not P4).
⊗ P4 hunk must populate out_vars (one VAR per table column, per Join.cpp:268-270) or EmitScan's bind_outputs
  emits nothing and the body's free-column refs are unbound; document IndexedColumns/InputVariables are populated
  for the index-less shape (add a belt IndexedColumns().size()==InputVariables().size()).
  D1 — pull the minimal terminal BindingStateSCHEMA interner (NOT the DAG) into P4 (H4): the schema id is
  compile-time; the value-bearing BindingStateId is minted at runtime in EvaluateEpoch.
```

### P5 — the partial-binding DAG (order-free schema, order-significant edge)

```
+ BindingStateSchema(region, field_set) order-free; BindingStateId(ri, schema, sorted values) runtime;
  BindingEdge(parent, ordered added_field, child) order-significant; MaterializePrefixChain lazy (declared-or-visited).
⊗ H6 — REQUIRES P0-item-4 (the parser flip) first, else @key(A,B) @key(B,A) never reaches P5.
⊗ F8 corrected exit gate — for @key(A,B,C): declared prefix chain {A}⊂{A,B}⊂{A,B,C} PRESENT; genuine NON-prefix
  subsets {A,C},{B},{C} ABSENT. F20: after materializing a keyed state, an unbound read returns the COMPLETE answer.
  F21: canonicalize the -contract-out declared-key render (DataFlow/Format.cpp:1745-1798) by KeyPathId (inter-path
  order-free, intra-path significant). M7/F11: a TrieNode keys on BindingStateSchemaId, never the value-bearing id.
  M5: name ONE BindingStateSchema interner owner (keyed on region+sorted-field-set), shared by P4's D1 pull and P5.
```

### P6 — recursive regional execution (SCC + routing + fusion + joint fixpoint + DRed deletion)

```
+ P6.1 ComputeRecursiveComponents (RelationSchema-level Tarjan — a FRESH GenericTarjan extraction, F30);
  P6.2 RuleRoutingProjection + PromoteSharedSharedField (union only if every producer agrees);
  P6.3 PlanRecursiveComponent (FusedFixpoint on a common preserved prefix, else JointFixpoint);
  P6.4 key-changing recursion = cyclic RuleActivationEdges (RequestEdge stays acyclic); P6.5 joint worklist;
  P6.6 retire unreachable SCCs after routed removals drain (NEVER refcount).
⊗ F7/M4 — ComputeRecursiveComponents must close SCCs through message publish->receive seams; MessageSeamsOf
  can't use ForEachInsertToSelectSeam endpoints directly (both carry the message decl) — resolve each to its
  producing/consuming RELATION, OR run the SCC over the DataFlow VIEW graph and project Stratify's condensation.
⊗ B4 (BLOCKING) — DRed is a per-FACT REBUILD over FactDerivation.support (recursive vs non-recursive split ON the
  FactDerivation, aggregated per fact_id), NOT a per-BindingState mirror of the landed per-row C_nr/C_r (that makes
  a binding state a 2nd fact owner). Mirror the ALGEBRA (Stratum.cpp:1799-1841 OVERDELETE->REDERIVE->INSERT,
  EmitRederive:660-691 = a C_r counter read; CERTIFIED landed), not the storage.
⊗ B5 (BLOCKING) — (A)-once-then-(B) drops cross-component TRANSITIVE retraction. Make (A) the transitive closure
  over all affected SCCs interleaved with (B) as ONE worklist carrying signed frontiers across component boundaries
  (mirror Stratum.cpp:2447->2490, :1856-1869), OR wrap (A)+(B) in an outer repeat-to-fixpoint (the seed §3 joint loop).
⊗ H7 — RetireUnreachableSCCs must CASCADE-RETRACT every FactDerivation sourced from the SCC's states BEFORE freeing
  storage; guard against the retracted-fact set, not routed removals (an interior helper is never routed).
  L6: give CommonPreservedPrefix an order source (declared @key path order); make the "one frontier" check advisory.
```

### P7–P9 — physical access planning / lazy tries / path inference [THIN — the ranked-next grounding target]

```
+ P7 SelectAccessPlan real dispatch gated on CodegenPlanCapabilities; PhysicalAccessStructure
  (HashArrangement/TriePrefixIndex); EmitAccessPlan; V-PLAN-HONEST (structural, at the emission site per D4).
+ P8 LazyPathTrie (shared prefixes, convergent endpoints, minted-on-visit); InducedOrdering (Free Join/COLT).
+ P9 InferAccessPaths (rules/joins/active bindings; a query is ONE binding source); declared paths preserved as
  logical guarantees alongside inferred (V-DECLARED-PATH-PRESERVED).
⊗ s14 DEEPENED — P7–P9 now have hunk-grained operational diffs (keyed-rewrite-p7p9-diffs.md),
  an adversarial critique (keyed-rewrite-p7p9-critique.md: 3 blocking + 5 high + 18 certs), and
  desired IR states (ir-desired-states §9–§12). KEY GROUNDED RESULTS:
    P7: AccessPlan is its own Rel domain (NOT the join Lowering enum); kFullScanFilter≡index=nullopt,
      kFullKeyHashLookup≡index=Some — SAME ProgramTableScanRegion, so at P7 the kind→region map STOPS
      being injective (D4 Option-1→Option-2): V-PLAN-HONEST MOVES to the EmitScan emission site on a
      plan_kind field with a kUnplanned sentinel (else the belt aborts the pre-existing join scans —
      critique B-P7). The query-path discriminator is CURSOR-SHAPE (region s<id> vs <name>_cursor
      pos/id), NOT loop-shape (an index-bearing relation's baseline cursor already emits First/Next).
    P8: ~100% GREENFIELD (grep-confirmed zero trie/sorted/prefix). Whole-key First/Next REUSES Index
      verbatim; .Range partial-prefix seek is the ONE new codegen surface. Trie nodes intern on
      BindingStateId=(schema, sorted values) — value-partitioned AND convergent (critique B-P8; F11
      is a P5 compile-lattice rule, not the P8 runtime rule). GetOrCreateIndex (Data.cpp:350 SortAndUnique)
      is order-free — the ordered-trie hook; dedup must gain a kind tag (critique H-P8).
    P9: inference is ADDITIVE over the DeclaredAccessPath authority; declared NEVER overridden
      (V-DECLARED-PATH-PRESERVED replaces the deleted V-DECLARED-KEY bijection). Order must come from a
      DataFlow signal (out_to_in/decl order), NEVER the file-static ControlFlow SortedPredecessors
      (critique B-P9, layer + authority violation). The lift walk (§3.2) runs on the POST-Optimize
      frozen graph whose forwarding TUPLEs Optimize DELETES — a BLOCKING prerequisite, run pre-Optimize
      or rewrite over post-Optimize invariants (critique H-P9). The -contract render SPLITS into
      declared-key/inferred-key lines and MOVES DataFlow→Regional (the DataFlow renderer reads P1-deleted
      RecognizedSubgraphs). Physical-layer exit gates are STRUCTURAL-only (answer-equality is a lost check).
```

---

## §4. Session-14 CLOSE — what landed + the next ranked step

Session 14 executed all five charter items as a GROUNDING round (docs only; suite 251 PASS by
construction — production untouched; nothing blessed). What landed:

1. **B1–B5 gate RE-VERIFIED — NOT compile-clean.** A whole-tree re-grep found 7 MORE un-enumerated
   P1 consumers (reconstruction-diffs §6, all verified at tip); B2–B5 CERTIFIED internally consistent
   (§6.1). The P1 green-light now additionally requires §6-1..§6-6 + a symbol-driven `git grep -l`
   pre-commit acceptance gate.
2. **P7–P9 DEEPENED** into hunk-grained operational diffs (keyed-rewrite-p7p9-diffs.md §1–§6).
3. **Design-goal diffs FORMULATED** with discriminating (structural, never-answer) exit gates (same doc §4/§5).
4. **CRITIQUED** (keyed-rewrite-p7p9-critique.md: 3 blocking + 5 high + 5 medium + 2 low + 18 certs);
   all amendments folded into p7p9-diffs §7.
5. **IR DESIRED STATES authored** (ir-desired-states §9–§12) with the critique corrections.

**NEXT RANKED STEP (owner-gated cadence unchanged — P1 is still destructive + owner-gated):**
- (a) **P0-item-4 is the one LANDABLE-AHEAD production change** (the parser order-free→order-significant
  flip at Parser.cpp:974-995 + 1477-1488) — a HARD P5 prerequisite (H6), independent of the P1 cut.
  Owner call to land it ahead.
- (b) **Resolve the 3 P7–P9 blocking corrections into the diffs** at the same grain s13 gave P2–P6
  (they are folded as §7 amendments; the next pass would rewrite §1–§3 IN PLACE to absorb them,
  esp. re-sourcing P9's order from a DataFlow signal and the pre-vs-post-Optimize lift decision).
- (c) **The two remaining least-grounded residues** (p7p9-diffs §6): the `.Range` partial-prefix
  codegen contract (P8's single biggest new surface) and the InducedOrdering↔P6-joint-fixpoint
  structural belt.
- (d) Then, owner-gated: green-light the destructive P1 cut (now with the §6 seven-consumer additions).

Do the follow-on with WORKFLOWS (opus for diff-authoring/critique/judgment, sonnet for mechanical
census/extraction/carrier-dumps). Keep the orchestrator thin; subagents return distilled
pseudocode/findings. Re-verify every anchor at tip before trusting it.
