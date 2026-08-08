# Keyed-instance greenfield rewrite — whole-program pseudocode + path forward as diffs

Session 11 seed (2026-08-06, branch `keyed-instances`, tip `46a404d4` + the
uncommitted Phase-0 working tree). This is the WHOLE-PROGRAM grounding for the
rewrite described in `next-session-prompt.md`. It is a SEED: the next session
is asked to deepen every pseudocode block, formulate the per-phase diffs at
hunk grain, critique them, and author the desired IR output states.

## §0. Orientation and supersession

The authority chain is `INDEX.md` → `next-session-prompt.md` → this file →
`keyed-rewrite-current-pseudocode.md` (NEW, session 12: the implementer-grain
deepening of §1 below — 8 per-subsystem pseudocode blocks, a fully re-verified
anchor table superseding §5, and a 10-item drift-correction ledger) →
`regional-arch-pseudocode.md` (deep current-architecture pseudocode; §1–§7 +
Part R) for line-level current-state detail.

SESSION-12 STATUS: §1 deepened + anchors re-verified (companion doc); §2/§3
rendered as operational pseudocode below; §4 P1–P9 refined to hunk grain with
exit gates + design-goal resolution; adversarial critique in
`keyed-rewrite-critique.md`; desired IR output states in
`keyed-rewrite-ir-desired-states.md`. Phase 1 remains OWNER-GATED (destructive);
this session is grounding only — no production code touched beyond the already-
landed uncommitted Phase-0 parser witnesses.

The central re-framing (supersedes the sessions 5–7 "@key selects the nested
lowering / force-activates demand" narrative recorded in memory):

- `@key(A, B, ...)` is a **relation-local, ordered access-path declaration**
  on `#local`/`#export`. It is NOT a query adornment, NOT a demand opt-in, NOT
  a physical-index selector, and NOT a member-key.
- **Order matters**: `[A,B] ≠ [B,A]` as paths. But the **semantic binding
  state** they reach is order-free (both converge on `{A,B}` fixed).
- Four separate authority domains (never collapsed): logical relation truth
  (canonical facts) · residual specialization (a lazy binding context) ·
  logical access path (ordered fields) · physical access structure (scan /
  hash / trie). Plus two edge relations: `RequestEdge` (exact ownership) vs
  `RuleActivationEdge` (derivation dependency; may cycle inside a region).

The rewrite DELETES the current demand machinery (Phase 1) and rebuilds keyed
evaluation on a real regional representation (Phases 2–9). There is no
compatibility mode; each phase is a coherent cut with an explicit exit gate.

---

## §1. The current whole-program pipeline as pseudocode (what we cut FROM)

Faithful at whole-program altitude; see `regional-arch-pseudocode.md` §1–§4 +
Part R.1.1–R.1.7 for the line-level version.

```
# bin/drlojekyll/Main.cpp — the compile pipeline
compile(module, flags):
    query  = Query::Build(module, log, passpolicy,
                          demand = flags.gDemand,          # -demand
                          demand_retract = flags.gDemandRetract)
    # origin_decls final here (public accessor)
    frozen = FrozenRegionalProgram::Build(query, log)      # Stage B (nominal)
    SetRelDumpStream(rel_stream)                            # -rel-out sink
    program = Program::Build(frozen, log, first_id, passpolicy,
                             demand_instance = flags.gDemandInstance)  # -demand-instance
    GenerateDatabaseCode(program, h, cc)                    # C++ codegen
```

### §1.1 Query::Build — where demand is injected (lib/DataFlow/Build.cpp:2524)

```
Query::Build(module, log, policy, demand_mode, demand_retract):
    build SELECT/TUPLE/JOIN/MERGE/CMP/MAP/NEGATE/AGG/KVINDEX/INSERT graph
    ConnectInsertsToSelects(log, proxy_view_to_decl)   # wire producers→consumers;
                                                        # records insert_proxy → decl
                                                        # in a Build-SCOPED map
    ApplyDemandTransform(module, log, demand_mode, demand_retract,
                         suppress_demand, proxy_view_to_decl)   # ← THE demand cut
    if not all-passes-gated: Optimize(log, policy)      # CSE, canon, DFE
    Stratify(log)                                        # SCC + neg/agg stratification
    row_contracts = InferConservativeRowContracts(impl) # pure, post-Stratify
    return Query(impl)
```

### §1.2 ApplyDemandTransform — the top-down authority (lib/DataFlow/Demand.cpp:388)

```
ApplyDemandTransform(...):
    if suppress_demand: return ok                        # bin/Oracle demand-blind
    demand_key_decls = scan PARSED module for HasInstanceKey()   # RP-6 activation
    pragma_activated = not empty(demand_key_decls)
    if not demand_mode and not pragma_activated: return ok       # containment gate
    bound_queries = [rel for rel in relations if IsQuery and any bound param]
    if empty(bound_queries):
        if pragma_activated: REJECT "@key but no bound query to seed"   # RP-6
        return ok
    if len(bound_queries) > 1: REJECT "multi bound query unsupported"
    q = bound_queries[0]
    # Phase 1: per-adornment locate/check (SIP walk backward from bound cols)
    for adornment in UniqueRedeclarations(q):
        p_bound = SIP-infer bound column set at the demanded relation p
        run left-linear / stray-consumer / neg-agg-in-body fences
        record PerAdornment{p_bound, guard sites, p_merge, ...}
    p_demanded_decl = proxy_view_to_decl[plan[0].p_merge]        # Tier-1 naming
    for d in demand_key_decls:
        if d != p_demanded_decl: REJECT "@key on non-demanded relation"  # RP-6
    if p_demanded_decl.HasInstanceKey():
        V-DECLARED-KEY: declared set-of-sets == inferred p_bound set-of-sets
                        (order-free bijection; Arm A / Arm B rejects)
    Step 4: stray-consumer union accounting (once, between loops)
    # Phase 2: mint per adornment
    for adornment in plan:
        FabricateDemandMessage/Local(module)   # real demand__<n> ParsedMessageImpl
        mint demand relation d_p, push-down guard JOIN (d_p ⋈ p)
        stamp GuardAnnotation (CSE-migrating) + RecognizedSubgraph
        register QueryDemandForcing (BindingPattern-keyed)
    if multi-adornment: mint MERGE UNION of guards (R-DUP) into one shared pub
```

### §1.3 Program::Build — the flat/nested selector (lib/ControlFlow/Build/Build.cpp:1333)

```
Program::Build(frozen, log, first_id, policy, demand_instance):
    query = frozen.Query()                               # H4: unwrap
    feature-gap pre-pass (agg/kv/product-in-scc rejects)
    # keyed-instance fences, per forcing, from LIVE guard JOINs:
    for forcing in query.GuardAnnotations grouped by forcing_index:
        cyclic_demand   = any guard JOIN lead self-reachable
        recursive_content = summarized input induction-owned / self-reachable
        if demand_instance and (cyclic_demand or recursive_content):
            REJECT (strict override)                     # demand_cyclic_1 etc.
        all_forcings_admissible &= not (cyclic or recursive_content)
    effective_demand_instance = demand_instance
    if not demand_instance and any_forcing and all_forcings_admissible
       and subgraphs[0].demanded_decl.HasInstanceKey():
        effective_demand_instance = true                 # RP-9 pragma → nested, SILENT
    build regions; nested arm lowers via Rel DRInstance, flat arm via guard web
```

### §1.4 Rel nested lowering + runtime leaf (lib/Rel/Rel.cpp, Database.cpp, InstanceStore.h)

> DRIFT (session 12): the mint enumerator is `kSubgraphInstantiate` (Rel.h:148),
> NOT `kInstanceInstantiate` as written below. The `kSectionWalk` label is set at
> `Rel.cpp:1146` with NO inline caveat; the honest "full scan with a key filter —
> deferred perf refinement" acknowledgment lives at codegen `Database.cpp:2339-2341`.
> See `keyed-rewrite-current-pseudocode.md` §1.1 for the full drift ledger.

```
# Rel.cpp ~936-1025 ResolveLiveRecognition: ABA-SAFE shape recovery. Stored
# RecognizedSubgraph QueryView handles DANGLE after Optimize, so recover
# everything from LIVE guard JOINs + parse identity:
ResolveLiveRecognition(query):
    for live view v with a GuardAnnotationIndex: bucket v under its forcing_index
    for each forcing bucket, for each guard JOIN v (joined jl, |jl|>=2):
        demand_table = model_table(jl[0])                 # fabricated demand side
        if annot.role == kBody:
            input_table = model_table(jl[1]); input_key_cols = annot.instance_key
        pub_table/pub_view = the live INSERT whose decl.Id() == forcing query decl
    return { forcing_index -> ResolvedInstance{demand,input,pub, key_cols, ok} }

# Rel.cpp ~1038-1179 BuildSubgraphInstanceOps (gated on demand_instance_enabled):
for RecognizedSubgraph rs (skip if dead / !ri.ok):        # ABA-safe skip
    REFUSE if input_view is MAP/NEGATE/AGG/KVIndex (HP-4)
    mint kInstanceInstantiate:
        per published pos p: source = kInstanceKeySlot if p in key_cols else kRowSlot
        Rederive arm(+1): ACCESS input_table, pred=kPresent,
            lowering = kSectionWalk,  # ← THE LIE: labelled walk, generates a full scan
            bound_cols = input_key_cols, child = FOLD(+1) into pub_table
    if demand_table differential: mint kInstanceDeath(-1)
    mint kInstanceSeal (always)

# CodeGen Database.cpp ~2434-2494 emit_instance_rescan (the generated mold):
emit_instance_rescan(key):
    assert !store.WorkingOccupied(iid)                    # V-INST-FRESH
    cur = store.TouchCurrent(iid)
    for s in 0..input.NumRows():                          # ← FULL SCAN of input table
        if input.RowAt(s).keyfields == key                # key filter
           and (input_diff ? input.Present(s) : true):
            cur.TryAdd(Row{ row-cols })                   # residual rebuild
# drivers: band(a0) death Recycle · band(a1) demand birth · band(a2/a2') edge
#          add/remove — all call the ONE rescan mold, once per touched iid.
# band(b) publish: set-difference frozen↔current → (T,F) drop first, (F,T) born.

# Runtime InstanceStore<Key, RowT> (InstanceStore.h:54-219):
#   Key = the COMPLETE α-bound key tuple; slots[] open-addressed Hash()->iid
#   keys[]/hashes[] iid-indexed append-only; frozen[iid]/current[iid] double-
#   buffered Table<RowT>; sealed_occupied[]/touched_flag[]/touched set.
#   Seal(): per touched iid swap current->frozen, Reset, recompute occupancy.
# → a LEAF ROW CACHE keyed by the COMPLETE key. No prefix states, no partial
#   bindings, no recursion, no request routing, no shared canonical facts.
```

### §1.5 The identity gap today

`FieldId`, `SemanticMemberKey`, `DeltaSign`, support-count domains exist in
DataFlow (Identity.h). But keyed lowering reduces facts, derivations,
requests, forcing indexes, and runtime handles to `GuardAnnotation` /
`RecognizedSubgraph` side-records + raw integers. `FrozenRegionalProgram` is
a `RegionalCensus` (7 counts) + `RegionalAbi` render-ready strings + a Query
pass-through — it OWNS no regional semantics.

---

## §2. The target authority model as OPERATIONAL pseudocode (what we cut TO)

The four authorities, turned from the typed records of `next-session-prompt.md`
"Target semantic representation" into the OPERATIONS that construct and query
them. Names may change; the ALGEBRA is the contract. Retained
`RegionalDataFlowCore.md` invariants named inline.

### §2.1 Identity construction (the interning algorithms)

```
# Symbolic fields — a relation's fields are DISTINCT identities. A shared
# spelling in a rule proves an EDGE-LOCAL projection, NOT schema equivalence
# (retained: "parameter spelling is not proof of agreement").
SymbolicFieldId(relation, ordinal) -> intern per (RelationId, ordinal)
    # promotion to a shared symbolic identity happens ONLY in Phase 6 rule
    # analysis, and ONLY when EVERY producer of both fields proves the mapping.

# Region instance identity — lexical scope never collapses on visible values
# (retained: "two parent instances cannot collapse facts just because their
# visible values match").
RegionInstanceId(region, lexical_owner, inherited_bindings):
    return intern(region_instance_table,
                  (region, lexical_owner, sort_by_field(inherited_bindings)))

# Access-path identity — ORDER-SIGNIFICANT within a path, ORDER-FREE across the
# declared set. THIS is what the current sort-collapse (Parser.cpp:1477,
# Demand.cpp:902) violates.
DeclaredAccessPath(relation, ordered_fields):
    assert no_repeated_field(ordered_fields)         # a path repeats no field
    return record{ relation, ordered_fields }        # [A,B] != [B,A]
InternDeclaredPaths(relation, raw_paths):            # written pragma order irrelevant
    seen = set()
    for p in raw_paths:
        canon = tuple(p.ordered_fields)              # NO sort — order IS identity
        if canon in seen: REJECT "exact duplicate access path"   # P0 item 4
        seen.add(canon)
    assign KeyPathId deterministically AFTER the full set is validated

# Binding-state identity — ORDER-FREE endpoint. Every path fixing the same
# field/value map interns to ONE state.
BindingStateId(region_instance, bound_field_value_map):
    schema = BindingStateSchema(region_of(region_instance),
                                canonical_field_SET(bound_field_value_map))
    return intern(binding_state_table,
                  (region_instance, schema, sort_by_field(bound_field_value_map)))
    # [A=a,B=b] reached via path [A,B] and via [B,A] intern to the SAME id.
```

### §2.2 The two edge relations (never collapsed)

```
# RequestEdge — EXACT ownership; forms an ACYCLIC forest (retained: "region
# ownership/call forest is acyclic"; "exact RequestEdgeId ... multiple owners").
AddRequestEdge(owner, call_site, dest_state):     # owner ∈ {RootLease|PermanentRoot|RegionalMember(fact)}
    return intern(request_edges, (owner, call_site, dest_state))
    # A 2nd owner requesting dest_state ATTACHES a new edge; it never dups facts.
RemoveRequestEdge(e): request_edges.erase(e)      # liveness RE-DERIVED, not refcounted

# RuleActivationEdge — derivation dependency; MAY CYCLE inside one region.
DeriveActivationEdge(source_state, source_fact, rule, dest_state):
    # BOTH endpoints share ONE lexical RegionInstanceId; cross-region parent/
    # child uses a RequestEdge on the ownership forest (retained refinement).
    intern_or_drop(activation_edges, (source_state, source_fact, rule, dest_state))

# Liveness = ROOTED REACHABILITY, not counting (retained refinement:
# "internally referenced cycle with no live root is inactive").
RootedReachability(request_edges, activation_edges):
    live = { e.dest_state for e in request_edges if RootAlive(e.owner) }
    repeat to fixpoint:
        for a in activation_edges:
            if a.source_state in live and Present(a.source_fact):
                live.add(a.dest_state)
    return live
```

### §2.3 Canonical facts (the SINGLE fact authority)

```
RegionalFactRelation : RegionalFactId -> RegionalFact
    # RegionalFactId = (region_instance, relation, SemanticMemberIdentity)
    # SemanticMemberIdentity = the RowContract member key (Stage-A), schema order.
    # DISTINCT facts only, derived from LIVE FactDerivations (retained:
    # "derivation support decides membership"; "member identity").
AddDerivation(source_binding_state, fact_id, delta):
    d = derivations_of(fact_id)
    d.support += delta                               # split signed support (retained)
    if d.support crosses 0→>0: publish born
    if d.support crosses >0→0: publish dies
    # Two binding maps may BOTH support one RegionalFactId; the relation still
    # holds ONE member. Access paths / physical arrangements REFER to these ids;
    # they never store a path-private copy (retained: "binding state is not a
    # second owner of relation facts").
```

### §2.4 Access requirement vs access plan (the HONESTY seam)

```
AccessRequirement(relation, available_ordered_bindings, required_fields,
                  completeness ∈ {CompleteRelation | ActiveSubset})
    # An ordinary unbound read REQUIRES CompleteRelation. ActiveSubset is legal
    # ONLY for an explicitly-named active-subset operation (retained: "do not
    # implement an ordinary unbound read by enumerating only active states").
SelectAccessPlan(req) -> AccessPlan ∈ {FullScanFilter | FullKeyHashLookup |
                                       ExistingTriePrefix | EnumeratePrefix |
                                       BuildLazyOrdering}:
    # FullScanFilter is ALWAYS a correct realization (a capability guarantee,
    # not a perf promise). A non-scan plan is chosen ONLY when codegen EMITS it.
    # INVARIANT (Phase 4): the label MUST match the emitted code — a scan is
    # never labelled a walk. This retires the kSectionWalk placeholder.
```

## §3. The evaluation / lifecycle contract as OPERATIONAL pseudocode

Defined by relational RESULT, not queue order (retained: "epoch results
independent of queue order"). Counts are caches, never the membership/
reachability oracle.

```
EvaluateEpoch(input_deltas, request_deltas):
    old = SnapshotCommittedOutputs()                 # published surface only
    ApplyInputDeltas(input_deltas)                   # canonical facts move
    for rd in request_deltas: Add/RemoveRequestEdge(rd)   # exact ownership deltas

    repeat to JOINT LEAST FIXPOINT:
        live = RootedReachability(request_edges, activation_edges)   # §2.2
        for st in live:                              # semi-naive, per binding state
            for rule in rules_of(region_of(st)):
                for delta in EvaluateRule(rule, st): # AccessPlan per read (§2.4)
                    AddDerivation(st, delta.fact, delta.sign)        # §2.3
            for f in new_facts(st):
                DeriveActivationEdge(st, f, matching_rule(f), dest_state_of(f))
            RouteResults(st)                         # canonical facts -> RequestEdges

    RetractRoutedResults(all_states \ live)
    RetireUnreachableSCCs()                          # ONLY after routed removals drain
    Publish(Difference(old, CurrentCommittedOutputs()))
    Seal()

RouteResults(st):
    for e in request_edges where e.dest_state == st:
        for f in canonical_facts_of(st):
            emit RoutedResult(e, f)                  # CALLER-QUALIFIED (retained):
                # a late requester receives already-derived facts; its removal
                # retracts ONLY its routed copies, not the shared fact.

RetireUnreachableSCCs():
    # A self-supporting activation SCC with no live root: retire the WHOLE SCC,
    # but ONLY after every routed removal for its facts drains. NEVER via a
    # reference count (retained: "do not use reference counts to collect a
    # cyclic activation graph").
```

DELETION CONTRACT (retained, restated over the four-authority model): any
incremental scheme (differential / DRed / affected-SCC recompute) MUST compute
the same least fixpoint as a fresh evaluation from committed inputs + live
request roots.

---

## §4. THE PATH FORWARD AS DIFFS (on §1 → §2/§3)

Each phase is a diff on the pseudocode above with an exit gate. The next
session refines these to hunk grain and critiques them.

### Phase 0 — lock the language contract (parser-local)  [PARTIALLY LANDED]

```
  DONE (uncommitted, suite 251 PASS):
+   reject `@key` on `#query` (Query.cpp state 6, kPragmaKey branch)
+   redecl consistency across FULL context (Parser.cpp: first key-bearing
      canonical decl, not immediate-previous)  → reject_key_redecl_1 rewritten,
      reject_key_on_query_1 added
  REMAINING (entangled with P1 — the demand bijection at Demand.cpp:905 sorts):
~   InstanceKeySet → ordered DeclaredAccessPath; decl paths = unordered unique
~   exact-duplicate path rejects; [A,B] and [B,A] both accepted (item 4)
~   define prefix / same-binding-different-order behavior (items 5,6)
```
Exit gate: parser + AST carry ordered paths; `[A,B]`/`[B,A]` distinct; no
demand coupling in key validation. Tests: keep parser-shape rejects; drop
key/adornment-coupling tests.

### Phase 1 — delete the old demand authority  [DESTRUCTIVE; owner-gated]

```
- ApplyDemandTransform, FabricateDemandMessage/Local, demand__ registry
- QueryDemandForcing, GuardAnnotation, RecognizedSubgraph, GuardAnnotationIndex
- DRInstance mint + post-optimization shape recovery (Rel.cpp:936-1059,1137)
- special InstanceStore lowering + Database.cpp scan/filter path (2434-2493)
- flags: -demand, -demand-retract, -demand-instance; pragma_activated;
  the flat/nested selector (Build.cpp:1439-1549)
Query::Build(...):
-   ApplyDemandTransform(...)
Program::Build(...):
-   feature-gap keyed fences; effective_demand_instance selection
```
Post-P1 state: `@key` is inert metadata; bound queries read the canonical
fully-materialized relations (one honest baseline, not a fallback).
Exit gate: whole corpus compiles with demand DELETED; ~50 demand_*/key_* cases
deleted or repurposed per `next-session-prompt.md` "Test migration"; the
`key_tc_witness`/`key_neighborhood_witness`/`key_multi_adorn_witness` datasets
survive as ANSWER witnesses, not equivalence/pragma-activation witnesses.
CAPABILITY NOTE: keyed/demand evaluation is NON-FUNCTIONAL from here until P4.

### Phase 2 — make FrozenRegionalProgram authoritative  [pure refactor otherwise]

```
FrozenRegionalProgram:
-   RegionalAbi render-ready strings; Query pass-through as the semantic owner
+   RegionTemplate{ RegionId, inherited_fields, RelationSchema[], rules,
+                   recursive_components, request_ports, result_ports }
+   RelationSchema{ RelationId, fields, member_key (from RowContract),
+                   declared_access_paths }
+   RuleRoutingProjection (typed source→dest field map, per rule)
+   AccessRequirement per read
Program/Rel/ControlFlow: consume ONLY the frozen typed records (formatting
derives from them). Rel stops recovering shapes from the optimized Query.
```
Exit gate: 180+ region goldens re-derive from typed records; no consumer reads
demand side-tables.

### Phase 3 — exact request + activation relations (over full materialization)

```
+ RequestEdgeId, RuleActivationEdgeId, FactDerivation, RoutedResultId
  each live region instance has ONE empty-binding state; activation restricted
  to the nonrecursive/acyclic slice
```
Exit gate properties: every root/permanent/parent-member has an exact owner;
a 2nd requester attaches without duplicating facts; removing one requester
retracts only its routed results; support ≠ ownership; dense handles ≠ identity.

### Phase 4 — honest complete-path specialization (deliberately simple backend)

```
request R through declared path P:
    st = BindingStateId from P's field/value bindings
    evaluate st with AccessPlan = FullScanFilter        # honest label, no kSectionWalk lie
    add FactDerivations → canonical RegionalFactIds
    route distinct canonical facts through exact RequestEdges
one canonical fact relation regardless of path count; FullKeyHashLookup only
when codegen emits one.
```
Exit gate: nonrecursive relation-local slice evaluates; the IR label matches
what codegen does (no scan-labelled-as-walk).

### Phase 5 — the partial-binding DAG

```
intern BindingState by (region_instance, canonical field set, values);
create ordered BindingEdge only for DECLARED or VISITED paths.
```
Exit gate: `@key(A)`/`@key(A,B)` share `{A}`; `[A,B]`/`[B,A]` converge on
`{A,B}`; unvisited subsets unmaterialized; multiple indexes never duplicate facts.

### Phase 6 — recursive regional execution

```
compute predicate SCCs independent of queries; typed edge-local projections;
fuse prefix-preserving recursion into one binding-state-local fixpoint;
key-changing recursion = RuleActivationEdge graph; joint rooted-reachability +
semi-naive worklist to least fixpoint; retire unreachable activation SCCs only
after routed removals drain (NOT via refcount).
```
Exit gate: same-key co-recursion fuses; different-key co-recursion converges;
removing the sole root drains a self-supporting cycle.

### Phase 7 — separate physical access planning

```
lower AccessRequirement → explicit AccessPlan; add hash/trie paths only after
codegen honors them; STRUCTURAL test proving a trie plan does no whole-table scan.
```

### Phase 8 — lazy tries + induced orderings (Free Join / COLT territory)
### Phase 9 — automatic path inference (queries are one source of bindings)

---

## §5. Grounding anchors (re-verify against tip before editing)

| Concern | Anchor (drifts; re-derive) |
| --- | --- |
| demand activation / rejects | `lib/DataFlow/Demand.cpp:401,473,868,892` |
| flat/nested selector | `lib/ControlFlow/Build/Build.cpp:1439-1549` |
| Program::Build unwrap | `lib/ControlFlow/Build/Build.cpp:1333-1338` |
| Guard/RecognizedSubgraph records | `include/drlojekyll/DataFlow/Query.h:1007-1058` |
| Rel shape recovery / kSectionWalk | `lib/Rel/Rel.cpp:936-1059, 1137-1158` |
| codegen full scan | `lib/CodeGen/CPlusPlus/Database.cpp:2434-2493` |
| runtime leaf | `include/drlojekyll/Runtime/InstanceStore.h:54-218` |
| regional planning on demand records | `lib/Regional/Planning.cpp:201-285` |
| parser @key surface | `lib/Parse/Parser.cpp` (~793 parse, ~1656 redecl), `lib/Parse/Query.cpp` (state 6) |
| Query::Build stage order | `lib/DataFlow/Build.cpp:2524-2665` |

## §6. Open decisions carried for the next session

- Phase ordering vs the old Stage A→I0→B→C→D plan: the greenfield P0–P9 plan
  supersedes it; `stage-*-diff.md` remain evidence for retained invariants.
- Whether Phase 0 item 4 (ordered paths) lands standalone or with the P1 cut
  (blocked today by the sorting bijection in demand validation).
- The desired IR output states (`.df`/`.contract`/`-region-out`/`.rel`/header)
  after each phase are UNWRITTEN — the next-session task authors them.
```
