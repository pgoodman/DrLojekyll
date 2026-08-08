# Keyed-instance rewrite — POST-P1 whole-program pseudocode + path forward as diffs (session-17 seed)

Session 16 close (2026-08-08). Branch `keyed-instances`, **tip `dc965d3c`** ("P1 greenfield cut:
delete the -demand / keyed-instance authority"). **P1 IS LANDED** (compile-clean, OptDiff SUITE:
PASS 222 cases, ctest 4/4, one atomic commit). This seed SUPERSEDES `session-16-whole-program-
seed.md` §1 — that file's "whole program TODAY" describes the PRE-cut pipeline (demand +
instance lowering), which no longer exists.

**GREENFIELD RULING (owner, 2026-08-08), still governing.** The compiler is not in use → the
rewrite is delete-then-rebuild. P1 removed the demand authority; P2–P9 rebuild the typed regional
model. The P1→P4 regression window (bound queries fall back to full materialization) costs
nothing with no users; every post-P1 gate is STRUCTURAL, never answer-equality. Full motivation:
memory `greenfield-rewrite-motivation` + this file's §0.

## §0. Status — what P1 changed, and what is grounded for P2

**Deleted at P1 (do NOT reference as live):** `ApplyDemandTransform` + `lib/DataFlow/Demand.cpp`;
`FabricateDemand*`/`demand__`/`lib/Parse/Demand.cpp`; `QueryDemandForcing`/`GuardAnnotation`/
`RecognizedSubgraph`/`GuardAnnotationIndex`; the Rel `kSubgraphInstantiate`/`kInstanceDeath`/
`kInstanceSeal` nested lowering + `ResolveLiveRecognition`/`BuildSubgraphInstanceOps`; the
ControlFlow `ProgramSubgraphInstanceRegion`/`ProgramInstanceStore` region family + the
`ProgramOperation::kSubgraphInstance` enumerator; CodeGen `EmitSubgraphInstance`; Runtime
`InstanceStore.h`; the `-demand`/`-demand-instance`/`-demand-retract` flags. `Query::Build` is
now `(module, log, policy)`. `IsCutSuccessorDR` SURVIVES reduced (diff/agg/KV cut authority):
`return succ.CanReceiveDeletions() || succ.IsAggregate() || succ.IsKVIndex();`.

**Post-cut honest baseline:** `@key` is INERT parsed metadata (parse rejects still fire:
wildcard/anon/dup/unknown/ordered-dup/redecl/on-query/on-message; NO semantic/activation effect).
A bound `#query` is a PERMANENT-ROOT observation — reads the canonical fully-materialized relation
via the plain cursor. `request_ports = 0` for every program. Keyed/residual evaluation is
NON-FUNCTIONAL until P2–P9.

**Grounded for P2 (verified at tip):** the P2 operational diff (`reconstruction-diffs.md` §3-P2,
with §5.3 H1/H2/M1 folded into the primary text) + its false-start certification + the anchor
re-verification (Planning.cpp `Build` :312, `CollectContractInserts` :150, `CollectOriginInteriorDecls`
:176, `DeriveRegionalCensus` :259, the in-`Build` recount :476-497). The typed-target records live
in `next-session-prompt.md` "Target semantic representation".

---

## §1. The whole program TODAY (POST-P1) — pseudocode

### §1.1 Pipeline (bin/drlojekyll/Main.cpp)

```
compile(module, flags):
    query   = Query::Build(module, log, gPassPolicy)               # Main.cpp:73  (3-arg; demand gone)
    frozen  = FrozenRegionalProgram::Build(query, log)             # Main.cpp:89  (degenerate planner)
    SetRelDumpStream(gRelStream)                                   # -rel-out seam
    program = Program::Build(frozen, log, gFirstId, gPassPolicy)   # Main.cpp:109 (no demand_instance)
    GenerateDatabaseCode(program, h, cc)
```

### §1.2 DataFlow — Query::Build (no demand pass; pure build+optimize+contracts)

```
Query::Build(module, log, policy):                                # lib/DataFlow/Build.cpp:2524
    build SELECT/TUPLE/JOIN/MERGE/CMP/MAP/NEGATE/AGG/KVINDEX/INSERT graph  (BuildClause per clause)
    RemoveUnusedViews; ClearGroupIDs; TrackDifferentialUpdates
    if policy.Gate("df.simplify"): impl->Simplify(log)
    proxy_view_to_decl = {}                                        # VESTIGIAL now (was the demand map)
    ConnectInsertsToSelects(log, proxy_view_to_decl)              # still wires producers->consumers
    if policy.AnyBodyOptionalEnabled(kDataFlow): impl->Optimize(log, policy)   # CSE/canon/DFE
    ConvertConstantInputsToTuples; RemoveUnusedViews; ProxyInsertsWithTuples; LinkViews; ...
    IdentifyInductions; FinalizeDepths; FinalizeColumnIDs; TrackDifferentialUpdates(final)
    BuildEquivalenceSets; Stratify(log)                           # neg/agg rejects; V-SCC-SEAM
    impl->row_contracts = InferConservativeRowContracts(impl)     # PURE 2-phase; a QueryImpl member
    ValidateRowContracts
    return Query(impl)
```

### §1.3 Regional — FrozenRegionalProgram::Build (the degenerate planner; the P2 TARGET)

```
Build(query, log):                                                # lib/Regional/Planning.cpp:312
    out = FrozenRegionalProgram(query)          # stores `::hyde::Query query` BY VALUE (the real owner);
                                                #   a census/RENDER-STRING shell (see §1.3a)
    for msg in CollectMessages(query):          # real messages only (IsDemandMessage filter GONE)
        push RegionalPort{ kInput | kResult, decl_text, ... }     # ABI ports
    request_ports = 0                            # (the demand request-port loop is DELETED)
    for (decl, redecl) in query ABIs (decl order):
        every redeclaration -> RegionalPermanentRoot (route_text "-> permanent-root")   # POST-CUT
    for (decl, ins) in CollectContractInserts(query):             # R-STORE: one per non-demand INSERT decl
        push RegionalContract{ rel_name, member_key_text, support_text }   # member key POSITIONAL from
                                                                          #   query.impl->row_contracts (friend leak)
    for decl in CollectOriginInteriorDecls(query):               # Tier-2 origin residue (SUBSUMES old Tier-1)
        push RegionalContract{ ... }
    out.census = DeriveRegionalCensus(query)     # PURE fn of query; request_ports==0
    RECOUNT built ports/contracts vs out.census (check_count, abort on mismatch)   # the anti-stub belt
    RunFreezeValidators (V-FROZEN-NO-OPEN-PORT / V-OWNERSHIP-ACYCLIC)
    return out
```

### §1.3a WHAT IS WRONG WITH §1.3 (the P2 motivation, grounded in Regional.h:75-157)

`FrozenRegionalProgram` is a **render-ready-strings + Query-passthrough shell**, NOT a semantic
owner. Concretely (Regional.h): it stores `::hyde::Query query` as the real authority, and its
"records" are mostly `std::string` render text — `RegionalAbi{decl_text, route_text}`,
`RegionalPort{head_text, fields_text}`, `RegionalContract{rel_name, member_key_text,
support_text}`. Downstream (`Program::Build` first statement `query = frozen.Query()`) unwraps
straight back to the Query. So the "frozen regional program" owns no typed regional semantics; it
is a dump formatter. **P2 makes it the typed semantic owner.**

### §1.4 ControlFlow + Rel — Program::Build (flat lowering only; Rel-IR is the stratum authority)

```
Program::Build(frozen, log, first_id, policy):                   # lib/ControlFlow/Build.cpp
    query = frozen.Query()                                        # H4 unwrap (still Query-passthrough)
    feature-gap pre-pass (agg/kv/map/product-in-scc rejects)      # (keyed fences DELETED)
    BuildQueryInjectorProcedure -> ForcingMessage @first fall-through ONLY   # (registry branch DELETED)
    build typed DRVecs + DROps; DeriveDRStrata; LinearizeAndValidateDRFlow (Kahn)
    IsCutSuccessorDR: succ.CanReceiveDeletions() || IsAggregate() || IsKVIndex()   # (demand conjunct DELETED)
    LowerDRFlow / LowerDRRounds / LowerCommitSweeps -> ControlFlow regions
    # (LowerSubgraphInstances + the nested keyed lowering are GONE)
```

### §1.5 CodeGen + Runtime — unchanged by P1 except the deleted instance emission

```
EmitScan(ProgramTableScanRegion): 3 arms on Index() — keyed_chain / keyed_probe / full-scan+filter
    (Database.cpp; LABEL-BLIND; cursor = "s"+id — the P4/P7 s<id> discriminator)
GetOrCreateIndex(cols): SortAndUnique -> ORDER-FREE identity (hash indexes only; no trie/sorted)
Runtime: Table/DiffTable + Index<Key> whole-key hash. (InstanceStore DELETED.)
```

---

## §2. The target — four authorities + two edges (carried, unchanged)

```
FOUR AUTHORITIES, never collapsed:
  Logical fact          RegionalFactRelation : RegionalFactId -> RegionalFact  (SINGLE fact authority)
  Residual              BindingState (owns frontiers + FactDerivation ids)      (NOT a 2nd fact owner)
  Logical access path   DeclaredAccessPath (ORDERED fields; [A,B] != [B,A])     (P5 seeds, P9 extends)
  Physical structure    AccessPlan (FullScanFilter | FullKeyHashLookup | ...trie) (P7 introduces, P8 extends)
TWO EDGE RELATIONS, never unified:
  RequestEdge        exact ownership, ACYCLIC forest (RootLease/PermanentRoot/RegionalMember)
  RuleActivationEdge derivation dep, MAY CYCLE in a region; liveness = ROOTED reachability
IDENTITY: BindingStateSchema keyed on the field SET (order-FREE); BindingStateId adds sorted VALUES;
  DeclaredAccessPath ORDERED; [A,B] & [B,A] converge on ONE {A,B} schema.
EvaluateEpoch = least fixpoint of (rooted-reachability ∘ semi-naive derivation), order-independent;
  deletion computes the SAME least fixpoint; drain routed removals BEFORE retiring an unreachable SCC;
  counts are CACHES. CROSS-CUT: inference (P9) picks NO physical structure; planning (P7) picks NO
  logical path; answer identity holds because FullScanFilter is always a correct realization.
```

Typed-record target: `next-session-prompt.md` "Target semantic representation" (RegionTemplate /
RelationSchema / BindingState / RequestEdge / RuleActivationEdge / FactDerivation / AccessRequirement /
AccessPlan). Retained `RegionalDataFlowCore.md` invariants: member identity, exact request ownership,
caller-qualified results, drain-before-retire, single-fact-authority, counts-are-caches.

---

## §3. The path forward as diffs (POST-P1 altitude)

### P2 — FrozenRegionalProgram becomes the TYPED semantic owner  [THE NEXT STEP; first constructive phase]

Authority: `reconstruction-diffs.md` §3-P2 (operational, §5.3 folded) + its false-start certification.
Touches the logical-fact authority (member_key) + the render/naming plumbing. Must NOT fabricate
residual/access-path/physical authorities (P3/P4/P5/P7).

```
+ struct RelationSchema { RelationId id; vector<SymbolicFieldId> fields;
+   SemanticMemberKey visible_fields;   // rc.visible_fields COPIED verbatim (F13)
+   SemanticMemberKey member_key;       // rc.member_key COPIED verbatim
+   vector<bool> declared_key_positions;// size==arity; precomputed at freeze (render off THIS, no bridge)
+   DeclaredAccessPathSet declared_access_paths;  // decl.InstanceKeys() carried (inert @key metadata)
+   bool support; }                     // view.CanReceiveDeletions()
+ struct RegionTemplate { RegionId id; vector<SymbolicFieldId> inherited_symbolic_fields;
+   vector<RelationSchema> relation_schemas; vector<RuleRoutingProjection> rules;  // rules RESERVED at P2
+   vector<RecursiveComponent> recursive_components;   // RESERVED EMPTY (P6.1 sole populator)
+   request_ports / result_ports / permanent_roots; }
~ FrozenRegionalProgram: `::hyde::Query query` owner field -> renamed `dataflow_graph`, exposed only
    via DataFlowGraph(); delete the 5 owned render-STRING vectors (RegionalAbi/Port/Internal/Contract
    are string shells); store ONE RegionTemplate.
~ BuildRelationSchema(query, decl, view, rc_map): rc_map = query.impl->row_contracts (KEEP the
    friend-class access, M1 — a public accessor WIDENS the leak); declared_key_positions precomputed
    from the SAME test as the current Planning.cpp:591-608 loop.
~ Format/render DERIVES from the typed records (RenderMemberKeyText reads declared_key_positions; no
    RowContract re-lookup, no value-id/ordinal bridge).
~ frozen_census KEPT `const RegionalCensus *` (H1 — its reader reads a RegionalCensus; add a typed
    `frozen_regions` field only when a later phase needs it). DeriveRegionalCensus(query) KEPT (F12).
= EXIT GATE (structural, discriminating — NOT answer-equality, a lost check post-P1):
    (1) the surviving ~region goldens re-derive FROM `R` with no --bless;
    (2) the in-`Build` recount RE-POINTED at typed `R` (R.relation_schemas.size() etc.) vs
        DeriveRegionalCensus(query) — the real anti-stub belt (H2; V-REGION-CENSUS alone is tautological);
    (3) grep zero: frozen.Query() re-parse / member_key_text string re-parse downstream;
    (4) IdentityTypes ctest passes with member_key typed.
  FALSE-START CERTIFIED: P2 is a PURE post-Optimize READ; no DataFlow mutation, no recognition pass.
    Standing obligation: R.rules must derive from the frozen read (P6.2 fills it; reserved at P2).
```

### P3 — RequestEdge / RuleActivationEdge / FactDerivation / RoutedResult (acyclic slice)
### P4 — honest complete-path specialization (FullScanFilter, reuse ProgramTableScanRegion)
### P5 — the partial-binding DAG (order-free schema, order-significant edge — P0-item-4 done)
### P6 — recursive regional execution (SCC + routing + fusion + joint fixpoint + DRed deletion)
### P7 — physical access planning (AccessPlan its own domain)
### P8 — the cross-relation ORDERED trie / COLT / Free Join
### P9 — access-path inference (additive, logical-only, PRE-Optimize)

```
  P3–P9 operational diffs + exit gates: reconstruction-diffs.md §3 (P3-P6), p7p9-diffs.md (P7-P9).
  Blocking realizations still standing from the B-panel:
    B2 permanent roots become AddRequestEdge owners (ACYCLIC forest) or the ~all no-bound-query corpus
       publishes nothing — TODAY already realized as permanent-roots in Planning.cpp (P3 formalizes the edge).
    B3 P4 discriminating gate: region-cursor s<id> vs query-cursor pos (structural, not answer).
    B4 per-FACT DRed over FactDerivation.support keyed on RegionalFactId (single fact authority),
       NOT a per-BindingState C_nr/C_r mirror.
    B5 cross-component transitive retraction = ONE joint signed-frontier fixpoint.
    D1 pull the compile-time BindingStateSchema interner into P4 (a SCHEMA id, not a value-bearing id).
  Two no-baseline carriers (ir-desired-states §6 P6 corecursive p@key(K)/q@key(X); §7 P5 convergence)
  were grounded by VERIFIED CURRENT REJECTS — those rejects are now GONE (@key inert), so re-ground
  them as post-P1 COMPILE witnesses (structural pins), not reject→compile deltas.
```

---

## §4. What session 17 should do (the grounding loop, now on the POST-P1 codebase)

Enough is persisted to resume cold (this seed + reconstruction-diffs §3-P2 + p7p9-diffs + next-session-
prompt.md target). P2 is the next ACTIONABLE constructive step. Ranked:

1. **Re-ground the whole program as pseudocode at the POST-P1 tip** (this seed is the backbone — keep it
   current; §1 is now grounded in real code). Weight toward P2 EXECUTION-READINESS: is `reconstruction-
   diffs §3-P2` operational enough to BUILD the typed RegionTemplate/RelationSchema records + re-point the
   recount + derive render from typed records? Enumerate every current `frozen.Query()` / string-shell
   consumer that P2 must re-point (the analog of the P1 symbol-driven grep, now for the STRING shells).
2. **Formulate design-goal diffs** on the deepened pseudocode with DISCRIMINATING structural exit gates
   (census-token / cursor-shape region-s<id>-vs-query-pos / node-count / provenance-survival / compile-
   abort — answer-equality is a LOST CHECK). Keep the four authorities separate + the two edges distinct.
3. **Critique adversarially** (refuter panel) against the real POST-P1 code + retained RegionalDataFlowCore.md
   invariants + the "Avoid these false starts" checklist. Special scrutiny: does the P2 typed-owner move keep
   the ownership forest ACYCLIC (B2 permanent-root-as-RequestEdge-owner)? Does it re-introduce any
   mutate-then-recognize authority (it must not — freeze is a pure read)? VERIFY, don't assert; rank survivors;
   record refuted as certifications.
4. **Author/extend the desired IR output states** (ir-desired-states) for P2: predict-then-verify the typed
   `-region-out` / `-contract-out` dumps + the recount belt, STRUCTURAL pins only. Sonnet pulls current carrier
   dumps as baselines; opus authors the desired states.

METHOD: WORKFLOWS (opus for diff-authoring / critique / judgment; sonnet for mechanical census / carrier-dumps /
string-shell-consumer enumeration). Several sequential single-phase workflows beat one mega-workflow; keep the
orchestrator thin. All anchors are byte-current at tip `dc965d3c` (re-verify any before trusting). Docs-only
unless the owner green-lights P2 execution.
```
