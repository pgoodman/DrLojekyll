# Keyed-instance rewrite — POST-P2 whole-program pseudocode + path forward as diffs (session-18 seed)

Session 17 close (2026-08-08). Branch `keyed-instances`, **tip `ae207c36`** ("P2:
FrozenRegionalProgram becomes the typed semantic owner"). **P1 + P2 ARE LANDED** (compile-clean,
OptDiff SUITE: PASS 222, ctest 4/4). This seed SUPERSEDES `session-17-whole-program-seed.md`
§1.3/§1.3a — that file describes the PRE-P2 render-string shell, which no longer exists.

**GREENFIELD RULING (owner, 2026-08-08), still governing.** The compiler is not in use → the
rewrite is delete-then-rebuild. P1 removed the demand authority; P2 made the regional program a
typed semantic owner; **P3–P9 build the residual/edge/physical authorities.** The P1→P4 window
(bound queries fall back to full materialization) costs nothing with no users; every post-P1 gate
is STRUCTURAL, never answer-equality. Full motivation: memory `greenfield-rewrite-motivation`.

## §0. Status — what P2 changed, what is grounded for P3

**Landed at P2 (tip `ae207c36`):** `FrozenRegionalProgram` stores ONE typed `RegionTemplate`
(`include/drlojekyll/Regional/Regional.h`) — `relation_schemas` (positional `member_key_positions`
masks + `support`), `abis` (typed `AbiRecord` with a `variant<monostate,ParsedMessage,
ParsedDeclaration>`), `ports` (`PortRecord`), `permanent_roots` (`PermanentRootRecord`), and
`rules`/`recursive_components` **RESERVED-EMPTY** structs. The 5 render-string shells
(RegionalAbi/Port/Internal/PermanentRoot/Contract) are DELETED; `Format.cpp` derives all text from
`p.Region()`. Owner field `query`→`dataflow_graph`; accessor `Query()`→`DataFlowGraph()`; `Region()`
added. Friend leak `query.impl->row_contracts` KEPT (read once in `Build`, threaded into the R-STORE
schema arm). Design + critique: `p2-typed-owner-grounding.md`.

**Post-P2 honest baseline (unchanged from P1 semantically):** `@key` is INERT parsed metadata; a
bound `#query` is a PERMANENT-ROOT observation served by `BuildQueryEntryPoint`
(lib/ControlFlow/Build/Build.cpp:504) reading the canonical fully-materialized relation via the
plain cursor. `request_ports = 0` for every program. Keyed/residual evaluation is NON-FUNCTIONAL.

**Grounded for P3 (verified at tip this session):** the P3 operational diff
(`reconstruction-diffs.md` §3-P3 lines 347-416 + §5.4 lines 728-744 B2/H3/M3/L5) + the standing
B-panel realizations. `RegionInstance.h` does NOT exist yet (P3 creates it, sibling of Regional.h's
typed-id idiom). The positional member key P3 needs is ALREADY realized as
`RelationSchema.member_key_positions` (P2 shipped it — H3's projection mask). The four-authority
target + the two-edge distinction live in `next-session-prompt.md` "Target semantic representation".

---

## §1. The whole program TODAY (POST-P2) — pseudocode

### §1.1 Pipeline (bin/drlojekyll/Main.cpp)

```
compile(module, flags):
    query   = Query::Build(module, log, gPassPolicy)               # Main.cpp:73  (3-arg; demand gone)
    frozen  = FrozenRegionalProgram::Build(query, log)             # Main.cpp:89  (TYPED owner now)
    SetRelDumpStream(gRelStream)                                   # -rel-out seam
    program = Program::Build(frozen, log, gFirstId, gPassPolicy)   # Main.cpp:109
    GenerateDatabaseCode(program, h, cc)
```

### §1.2 DataFlow — Query::Build (unchanged by P2)

```
Query::Build(module, log, policy):                                # lib/DataFlow/Build.cpp:2524
    build SELECT/TUPLE/JOIN/MERGE/CMP/MAP/NEGATE/AGG/KVINDEX/INSERT graph
    Simplify?; ConnectInsertsToSelects; Optimize? (CSE/canon/DFE); LinkViews; ...
    IdentifyInductions; Finalize*; BuildEquivalenceSets; Stratify(log)
    impl->row_contracts = InferConservativeRowContracts(impl)     # PURE 2-phase; the friend-leaked side-table
    ValidateRowContracts; return Query(impl)
```

### §1.3 Regional — FrozenRegionalProgram::Build (the P2 TYPED OWNER)  [REFRESHED for P2]

```
Build(query, log):                                                # lib/Regional/Planning.cpp:312
    out.dataflow_graph = query          # the retained DataFlow graph (Program::Build consumes it)
    R = out.region  (RegionTemplate)
    # ---- ABIs + ports (declaration order; request_ports=0 post-cut)
    for m in received:  R.ports += PortRecord{kInput, i++, m};  R.abis += AbiRecord{kInput,  m, kToPortP, i}
    for m in published: R.ports += PortRecord{kResult,i++, m};  R.abis += AbiRecord{kOutput, m, kToPortP, i}
    if no published:    R.abis += AbiRecord{kOutput, monostate, kNone, 0}      # the `<none>` line
    for redecl in dedup(query.Queries()):                        # BindingPattern dedup
        R.abis += AbiRecord{kQuery, redecl, kPermanentRoot, 0};  R.permanent_roots += PermanentRootRecord{redecl}
    # ---- relation schemas: TWO arms on one dense edge counter (E0..En)
    rc_map = query.impl->row_contracts                           # the ONE friend leak (M1)
    for (decl, ins) in CollectContractInserts(query):            # R-STORE insert arm
        R.relation_schemas += BuildRelationSchemaFromInsert(rc_map, decl, QueryView(ins))
                                                                 #   positions[i]=(vis[i] in member_key); support=CanReceiveDeletions
    for decl in CollectOriginInteriorDecls(query):               # Tier-2 origin arm (NO RowContract)
        R.relation_schemas += BuildRelationSchemaFromOrigin(query, decl)
                                                                 #   positions=all-true; support=ResolveOriginSupport (OR over carriers)
    R.rules = []; R.recursive_components = []                    # RESERVED EMPTY (P6.1/P6.2 populate)
    out.census = DeriveRegionalCensus(query)                     # PURE fn of query (F12); request_ports==0
    RECOUNT typed R (ports by kind, relation_schemas.size()) vs out.census  # H2 anti-stub belt, fprintf+abort
    RunFreezeValidators (V-FROZEN-NO-OPEN-PORT / V-OWNERSHIP-ACYCLIC)
    return out
```

### §1.3a WHAT IS STILL DEGENERATE (the P3 motivation, grounded in real post-P2 code)

The RegionTemplate is now a typed LOGICAL-FACT owner (member keys are positional masks, not
strings), but it owns NONE of the RESIDUAL / EDGE / physical authorities:
- **No RequestEdge / RuleActivationEdge.** `permanent_roots` is a flat list of `ParsedDeclaration`;
  there is no ownership forest, no derivation-dependency graph. `rules`/`recursive_components` are
  reserved-empty — no rule routing exists.
- **No FactDerivation / BindingState.** Nothing tracks per-fact support or residual specialization.
- **Every bound `#query` is a full-materialization read.** `Program::Build` unwraps
  `frozen.DataFlowGraph()` (Build.cpp:1209) and `BuildQueryEntryPoint` (Build.cpp:504) serves each
  redeclaration by scanning the canonical materialized table — the P1→P4 fallback. No request
  routes, no keyed specialization.
**P3 introduces the RequestEdge/RuleActivationEdge/FactDerivation/RoutedResult authorities (acyclic
slice) and roots the permanent roots as request-edge owners.**

### §1.4 ControlFlow + Rel — Program::Build (flat lowering; Rel-IR is the stratum authority)

```
Program::Build(frozen, log, first_id, policy):                   # lib/ControlFlow/Build.cpp:1206
    query = frozen.DataFlowGraph()                               # P2 rename (was frozen.Query())
    context.frozen_census = &frozen.Census()                     # const RegionalCensus* (H1)
    feature-gap pre-pass (agg/kv/map/product-in-scc rejects)
    BuildQueryEntryPoint per redeclaration (full-materialization cursor read)   # the retained backend
    build typed DRVecs + DROps; DeriveDRStrata; LinearizeAndValidateDRFlow (Kahn)
    IsCutSuccessorDR: succ.CanReceiveDeletions() || IsAggregate() || IsKVIndex()
    LowerDRFlow / LowerDRRounds / LowerCommitSweeps -> ControlFlow regions
    V-REGION-CENSUS (Rel.cpp:3879): DeriveRegionalCensus(query) == *frozen_census (tautological belt)
```

### §1.5 CodeGen + Runtime — unchanged

```
EmitScan(ProgramTableScanRegion): 3 arms on Index() — keyed_chain / keyed_probe / full-scan+filter
    (Database.cpp; LABEL-BLIND; cursor = "s"+id — the P4/P7 s<id> discriminator)
GetOrCreateIndex(cols): SortAndUnique -> ORDER-FREE identity (hash indexes only; no trie/sorted)
Runtime: Table/DiffTable + Index<Key> whole-key hash.
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

Typed-record target: `next-session-prompt.md` "Target semantic representation". Retained
`RegionalDataFlowCore.md` invariants: member identity, exact request ownership, caller-qualified
results, drain-before-retire, single-fact-authority, counts-are-caches.

---

## §3. The path forward as diffs (POST-P2 altitude)

### P3 — RequestEdge / RuleActivationEdge / FactDerivation / RoutedResult (acyclic slice)  [THE NEXT STEP]

Authority: `reconstruction-diffs.md` §3-P3 (lines 347-416) + §5.4 (728-744). New file
`lib/Regional/RegionInstance.h` (sibling of Regional.h's typed-id idiom). Touches the OWNERSHIP
authority (RequestEdge) + the SUPPORT authority (FactDerivation), kept distinct from logical-fact
identity (RegionalFactId) and each other.

```
+ RootLeaseId / PermanentRootId / RegionInstanceId typed ids
+ BindingStateId { RegionInstanceId ri; BindingStateSchemaId schema; SortedBoundFieldValues vals }
+   # P3: interns to EXACTLY the empty state (vals={})
+ RegionalFactId { RegionInstanceId ri; RelationId relation; SemanticMemberIdentity member }
+ RequestOwnerId = variant<RootLeaseId, PermanentRootId, RegionalFactId>   # RegionalMember is P6+
+ RequestEdgeId { RequestOwnerId owner; CallSiteId call_site; BindingStateId dest }
+ RuleActivationEdgeId { BindingStateId src_state; RegionalFactId src_fact; RuleId rule; BindingStateId dst }
+ FactDerivation { DerivationId id; BindingStateId src_state; RegionalFactId fact; DerivationSupportCount support }
+ RoutedResultId { RequestEdgeId request_edge; RegionalFactId fact }

BuildRequestPorts(region_template):                 # re-provides the query-entry loop, typed
    for redecl in dedup(query.Queries()):
        if any bound param:
            lease = RootLeaseId(next); AddRequestEdge(RootLease(lease), CallSite(redecl), EmptyBindingState(ri))
            request_ports.push(...)
        else:                                        # B2: an all-free query is a PERMANENT ROOT owner
            pr = PermanentRootId(next)
            AddRequestEdge(PermanentRoot(pr), CallSite(redecl), EmptyBindingState(ri))   # RootAlive(PermanentRoot)≡true
            permanent_roots.push(pr)
AddDerivation(src_state, relation_schema, row, delta):        # F29/H3: member via the POSITIONAL mask
    member = ProjectRow(row, relation_schema.member_key_positions)   # <-- P2's mask, reused directly
    fact = RegionalFactId(src_state.ri, relation_schema.id, member); d.support += delta; publish on 0<->>0
EvaluateEpoch(input_deltas, request_deltas):                  # ACYCLIC (P3): one topo sweep, no fixpoint
    apply inputs/requests; live = RootedReachability(request_edges, activation_edges)
    for st in {EmptyBindingState} ∩ live: for rule in topo_order(rules): AddDerivation(...); RouteResults(st)
    RetractRoutedResults(dead states); Publish(diff); Seal()
```

STANDING B-PANEL BLOCKERS (from `reconstruction-critique.md`, verify against real code):
- **B2 (blocking):** permanent roots MUST become `AddRequestEdge(PermanentRoot,...)` owners with
  `RootAlive≡true`, else ~73% of corpus programs (no bound `#query`) get `live={}` and publish
  nothing. Exit-gate probe: a no-bound-query program still publishes its full answer.
- **B4:** per-FACT DRed over `FactDerivation.support` keyed on `RegionalFactId` (single fact
  authority), NOT a per-BindingState C_nr/C_r mirror.
- **H3 (FOLDED at P2):** fact identity projects the row through the POSITIONAL member key —
  now realized as `RelationSchema.member_key_positions`; P3 reuses it directly (no new bridge).
- **M3:** P3 `EvaluateEpoch` LAYERS request/activation tracking over the RETAINED
  full-materialization backend (the landed induction fixpoint + `BuildQueryEntryPoint`) — it does
  NOT replace codegen. Add a recursive baseline probe.
- **L5:** `RouteResults` must filter by the request edge's requested relation, else a `#query` on p
  receives edge/q facts in the single-empty-state world.
- **D1 (open):** pull the compile-time `BindingStateSchema` interner into P4 (a SCHEMA id, not a
  value-bearing id). P3 interns only the empty state.

EXIT GATE (structural, discriminating — NOT answer-equality): directed battery over repurposed
key_* datasets — (a) exact owner count == distinct AddRequestEdge, no fact duplicated; (b) a 2nd
requester adds RoutedResults, ZERO new FactDerivations; (c) RemoveRequestEdge on one of two
retracts only its routed copies, fact stays present; (d) support-vs-ownership independently
observable; (e) F29: two rows agreeing on member-key columns intern to ONE RegionalFactId.

### P4 — honest complete-path specialization (FullScanFilter, reuse ProgramTableScanRegion)
### P5 — the partial-binding DAG (order-free schema, order-significant edge — P0-item-4 done)
### P6 — recursive regional execution (SCC + routing + fusion + joint fixpoint + DRed deletion)
### P7 — physical access planning (AccessPlan its own domain)
### P8 — the cross-relation ORDERED trie / COLT / Free Join
### P9 — access-path inference (additive, logical-only, PRE-Optimize)

```
  P4–P9 operational diffs + exit gates: reconstruction-diffs.md §3 (P4-P6) + §5.5, p7p9-diffs.md (P7-P9).
  B3 P4 discriminating gate: region-cursor s<id> vs query-cursor pos (structural, not answer).
  B5 cross-component transitive retraction = ONE joint signed-frontier fixpoint (P6).
  Two no-baseline carriers (ir-desired-states §6 P6 corecursive p@key(K)/q@key(X); §7 P5 convergence)
  were grounded by VERIFIED CURRENT REJECTS — those rejects are now GONE (@key inert), so re-ground
  them as post-P2 COMPILE witnesses (structural pins), not reject→compile deltas.
```

---

## §4. What session 18 should do (the grounding loop, now on the POST-P2 codebase, weighted to P3)

Enough is persisted to resume cold (this seed + reconstruction-diffs §3-P3/§5.4 + p7p9-diffs +
next-session-prompt.md target + memory `regional-dataflow-core-epoch`). P3 is the next ACTIONABLE
constructive step. Ranked:

1. **Re-ground the whole program as pseudocode at the POST-P2 tip** (this seed is the backbone —
   keep it current). Weight toward P3 EXECUTION-READINESS: is `reconstruction-diffs §3-P3`
   operational enough to BUILD `RegionInstance.h` (the typed ids + AddRequestEdge/AddDerivation/
   EvaluateEpoch) and LAYER it over the retained backend without replacing codegen (M3)? Run the P3
   analog of the P1/P2 symbol grep — enumerate every current site that P3 must touch or root: the
   query-entry loop (`BuildQueryEntryPoint`/`BuildEmptyQueryEntryPoint`, Build.cpp:481-516), the
   publish path, and where request/activation tracking layers in. Confirm B2 (permanent-root owners)
   is realizable without answer regression.
2. **Formulate design-goal diffs** on the deepened pseudocode with DISCRIMINATING structural exit
   gates (owner-count / routed-result-vs-derivation / drain-before-retire / member-key-intern /
   compile-abort — answer-equality is a LOST CHECK). Keep the four authorities separate + the two
   edges distinct. Special scrutiny: B4 (per-FACT DRed on RegionalFactId, never per-BindingState);
   the ownership forest stays ACYCLIC (RequestEdge), while RuleActivationEdge MAY cycle.
3. **Critique adversarially** (refuter panel) against the real POST-P2 code + retained
   `RegionalDataFlowCore.md` invariants + the "Avoid these false starts" checklist. Does P3 keep
   the RequestEdge forest acyclic? Does it avoid a second fact authority (BindingState is NOT a fact
   owner)? Is member identity projected through `member_key_positions` (the P2 mask), never a new
   value-id bridge? Does the all-free permanent-root owner (B2) preserve every current publish?
   VERIFY, don't assert; rank survivors; record refuted as certifications.
4. **Author/extend the desired IR output states** (ir-desired-states) for P3: predict-then-verify
   the typed `-region-out` (request-edge/permanent-root-owner render) + the `.rel` request-edge/
   lifecycle op family + the recount belt, STRUCTURAL pins only. Sonnet pulls the current carrier
   dumps as baselines; opus authors the desired states.

METHOD: WORKFLOWS (opus for diff-authoring / critique / judgment; sonnet for mechanical census /
carrier-dumps / string-shell-consumer enumeration). Several sequential single-phase workflows beat
one mega-workflow; keep the orchestrator thin. All anchors are byte-current at tip `ae207c36`
(re-verify any before trusting). Docs-only unless the owner green-lights P3 execution.
```
```
