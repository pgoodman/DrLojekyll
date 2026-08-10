# Keyed-instance rewrite — POST-P4 whole-program pseudocode + path forward as diffs (session-20 seed)

> **P5 LANDED (session 20, 2026-08-09).** The partial-binding DAG shipped: the ORDERED
> `DeclaredAccessPath` authority (RegionInstance.h) + the per-relation order-free binding-schema DAG
> (`RelSchemaLocalId`/`schema_table`/`binding_edges` + `MaterializePrefixChain`, lazy — no power set),
> interned at freeze from `decl.InstanceKeys()`, made load-bearing by the `HasInstanceKey()`-tied
> **V-PREFIX-CHAIN** belt, rendered as a golden-pinned `-region-out` `declared-key` line (KeyPathId-
> sorted → F21). **P5 moves NO codegen** (`.rel`/`datalog.h`/`.stdout` byte-stable — verified). Gate
> GREEN: OptDiff **SUITE: PASS (223)** (new carrier `key_partial_1` — the first positive `@key` case
> post-P1), ctest **5/5** (RegionInstance GateD–GateI DAG battery). Full record + the refuter-panel
> survivors (A1–A7) + IR states: **`p5-grounding.md`**. NEXT actionable = **P6** (recursive regional
> execution — the P3/P4 model's dormant derivation/route half first runs). The body below is the
> POST-P4 view; §3-P5 is now the LANDED record.

Session 19 close (2026-08-08). Branch `keyed-instances`, **tip `c546a6a4`** ("P4: honest complete-path
specialization (AccessPlan / FullScanFilter)"). **P1 + P2 + P3 + P4 ARE LANDED** (compile-clean,
OptDiff SUITE: PASS 222, ctest 5/5). This seed is the START-HERE whole-program backbone for **P5**. It
supersedes `session-19-whole-program-seed.md` §1 (POST-P3 pipeline) for the Regional/codegen layer —
P4 added the `AccessPlan` fourth authority and a real freeze→codegen data dependency.

**GREENFIELD RULING (owner), still governing.** The compiler is not in use → delete-then-rebuild.
Every post-P1 gate is STRUCTURAL, never answer-equality (the full-materialization backend answers
correctly with `@key` inert). Motivation: memory `greenfield-rewrite-motivation`.

## §0. Status — what P4 changed, what is grounded for P5

**Landed at P4 (tip `c546a6a4`; full record `p4-grounding.md`):** the `AccessPlan` fourth authority
(physical structure) in `include/drlojekyll/Regional/RegionInstance.h`
(`AccessPlan{kFullScanFilter,kFullKeyHashLookup,kRetainedIndexScan}` + `AccessRequirement` +
`SelectAccessPlan`). SELECTED at freeze (`BuildRequestPorts` computes + stores it on
`RequestPortRecord.plan`; `FrozenRegionalProgram::PlanFor(redecl)` reads it back) and CONSUMED at
codegen (`Program::Build` threads `frozen` into `Context`; `BuildQueryEntryPointImpl` + the
empty-query arm withhold the index for `kFullScanFilter`, always-on **V-PLAN-HONEST** belt). A
bound+free `#query` now lowers to `EmitQueryFriends`' honest full-scan-filter cursor (`while
pos<NumRows` + `if row.<f>!=<param>`, `pos=0`, index elided); an all-bound query keeps `.Find`
(`kFullKeyHashLookup`). `-region-out`/`-region-dot-out` render `plan=`. **P4 drives a codegen CHANGE
via a compile-time authority READ — the model's derivation/route half stays P3-style (ctest-only).**

**Post-P4 honest baseline for P5:**
- `BindingStateSchemaId` (RegionInstance.h:115) exists but interns ONLY the empty schema
  (`BindingStateSchemaId{0u}`); `BindingStateId.vals` is always `{}`. There is NO `BindingEdge`, NO
  `DeclaredAccessPath`, NO prefix DAG yet — **P5 introduces the residual binding-state DAG.**
- `@key` is INERT parsed metadata that SURVIVED P1: `ParsedDeclaration::InstanceKeys()`
  (`std::vector<InstanceKeySet>`, ordered paths) / `HasInstanceKey()` / `InstanceKeyRanges()`
  (Parse.h:456-462, Parse.cpp:858-881); parsed in `ParseLocalExport` (Parser.cpp:357, the `@key(...)`
  pragma tail states 21/22 fill `instance_key_param_index_sets` per set). The **order-significant**
  path semantics (P0-item-4) survived: same-decl dup compares ordered vectors directly (Parser.cpp:977),
  `SameKeySetOfSets` is order-free ACROSS paths / order-significant WITHIN each (Parser.cpp:1469-1475,
  used at :1669). So `@key(A,B)` and `@key(B,A)` are DISTINCT paths that both pass parse.
- **@key currently has ZERO semantic effect** — it does not shape the binding model, the plan, or
  codegen. Its only live surfaces are the parse-rejects (`key_anon_1`/`key_dup_1`/`key_unknown_1`/
  `key_wildcard_1` — arg-list obligations) and the advisory `-region-dot-out` `declared-key` badge
  (Regional/Format.cpp:384, keyed on `schema.decl.HasInstanceKey()`).
- **CRITICAL for P5: there is NO positive `@key` carrier in the corpus.** The semantic `@key`
  witnesses (`key_tc_witness`, `key_neighborhood_witness`, `key_multi_adorn_witness`, …) were DELETED
  at P1 with the demand machinery. P5 must AUTHOR a new positive carrier (the P3 precedent: a new
  golden that a stub cannot pass), e.g. `#local rel(u64 A, u64 B) @key(A). #query rel(bound A, free B).`
- **P5 anchors that predate P4 (re-verify all):** `reconstruction-diffs.md §3-P5` (489-538) +
  §5.6 (H6/M7 parser flip); `next-session-prompt.md` Phase 5 + the "Target semantic representation"
  (`BindingStateSchema`/`BindingEdge`/`DeclaredAccessPath`). Its F21 render target
  (DataFlow/Format.cpp:1745-1798, `RenderDeclaredKeyLines`) MAY have moved/been deleted at P1 — the
  `-contract-out` row-contract dump is at DataFlow/Format.cpp:1531; re-locate the declared-key line.

---

## §1. The whole program TODAY (POST-P4) — pseudocode

### §1.1 Pipeline (bin/drlojekyll/Main.cpp) — anchors current at tip
```
compile(module, flags):
    query   = Query::Build(module, log, gPassPolicy)              # DataFlow IR (3-arg since P1)
    frozen  = FrozenRegionalProgram::Build(query, log)            # Regional model: ports + P3 request/
                                                                   #   derivation model + P4 AccessPlan
    SetRelDumpStream(gRelStream)                                  # -rel-out seam
    program = Program::Build(frozen, log, gFirstId, gPassPolicy)  # ControlFlow IR (reads frozen)
    GenerateDatabaseCode(program, h, cc)                          # C++ codegen
```

### §1.2 DataFlow — Query::Build (unchanged by P2–P4)
```
Query::Build(module, log, policy):                               # lib/DataFlow/Build.cpp
    build SELECT/TUPLE/JOIN/MERGE/CMP/MAP/NEGATE/AGG/KVINDEX/INSERT; Simplify?; Connect; Optimize?;
    LinkViews; IdentifyInductions; Finalize*; BuildEquivalenceSets; Stratify(log)
    impl->row_contracts = InferConservativeRowContracts(impl)    # AllFields on multi-view SCCs
    return Query(impl)
# @key is parsed metadata on the ParsedDeclaration; it does NOT participate in Query::Build.
```

### §1.3 Regional — FrozenRegionalProgram::Build (P2 typed owner + P3 model + P4 plan)  [POST-P4]
```
Build(query, log):                                               # lib/Regional/Planning.cpp
    out.dataflow_graph = query
    R = out.region (RegionTemplate); RR = out.instances (RegionInstanceRelations)
    input/result ports + ABIs (declaration order)                # unchanged
    # ---- BuildRequestPorts (P3 split by binding + P4 plan)      Planning.cpp
    for parsed_query in dedup-by-Id(sub-module walk): decl=…; requested=RelationId{decl.Id()}
        for redecl in dedup-by-BindingPattern(decl.UniqueRedeclarations()):
            cs = CallSiteId{next++}
            if HasBoundParam(redecl):                            # bound -> RootLease request port
                plan = ComputeQueryAccessPlan(decl, redecl)      # P4 (§1.3a): has_free -> scan;
                                                                 #   all-bound -> hash; store on record
                R.request_ports += {port, redecl, RootLeaseId, cs, plan}
                RR.AddRequestEdge(RootLease, cs, EmptyBindingState(ri{0}), requested)
            else:                                                # all-free -> PermanentRoot
                R.permanent_roots += {redecl}
                RR.AddRequestEdge(PermanentRoot, cs, EmptyBindingState(ri{0}), requested)
    relation schemas (R-STORE insert arm + Tier-2 origin arm)    # unchanged from P2
    # RR.derivations / RR.routed_results stay EMPTY for real compiles (ctest-only; §1.3a)
    out.census = DeriveRegionalCensus(query); RECOUNT belt; RunFreezeValidators
    return out
```

### §1.3a THE P3/P4 MODEL, as it stands (the P5 launch point)
```
# P4 AccessPlan (RegionInstance.h) — the PHYSICAL-STRUCTURE authority, DISTINCT from fact id /
# binding state / logical access path:
SelectAccessPlan(req):                                           # req: {relation, has_free, avail_bindings}
    if not req.has_free: return kFullKeyHashLookup               # all-bound -> .Find (unchanged emit)
    return kFullScanFilter                                       # bound+free -> full scan (index withheld)
# ComputeQueryAccessPlan is a PURE freeze function of the redecl's binding arity (NO recursion gate —
# a query relation is always a materialized-then-scanned target, §8-S2). Stored on RequestPortRecord.

# The P3 request/derivation model — DORMANT for real compiles (exercised only by the RegionInstance
# ctest): BindingStateId interns to EXACTLY EmptyBindingState; AddDerivation/RouteResults have no
# real-compile callers. `@key` does NOT touch any of this yet (that is P5).
```

### §1.4 ControlFlow + codegen — the P4 query-read consumption (M3 backend elsewhere)
```
Program::Build(frozen, …):                                      # lib/ControlFlow/Build/Build.cpp
    query = frozen.DataFlowGraph(); context.frozen_census = &frozen.Census()
    context.frozen = &frozen                                    # P4: so entry-point builders read PlanFor
    …build DR flow, induction fixpoint, commit sweeps…          # the retained M3 backend (unchanged)
    for insert in query.Inserts() where IsQuery: BuildQueryEntryPoint(decl, insert)
        for redecl in dedup(decl.UniqueRedeclarations()):
            BuildQueryEntryPointImpl(redecl, insert):
                col_indices = bound decl-ordinals
                plan = context.frozen->PlanFor(redecl)           # P4: READ the model's plan
                scanned_index = (plan==kFullScanFilter) ? nullopt
                                : GetOrCreateIndex(col_indices)  # withhold for full scan
                assert(plan!=kFullScanFilter or !scanned_index)  # V-PLAN-HONEST
                impl->queries.emplace_back(query, table, scanned_index, forcer, retract)
GenerateDatabaseCode:                                           # EmitQueryFriends (Database.cpp:1527)
    all-bound  -> `.Find(key)` existence (ignores index)
    bound+free & scanned_index==nullopt -> `while(pos<NumRows){ if(row.<f>!=<param>) continue; }` scan
    bound+free & scanned_index==Some    -> `idx.First/Next` seek (P7 cost-based; not emitted at P4)
```

---

## §2. The target — four authorities + two edges (carried, unchanged)
```
Logical fact          RegionalFactRelation : RegionalFactId -> RegionalFact  (SINGLE fact authority)
Residual              BindingState (schema + typed values + FactDerivation ids)  (NOT a 2nd fact owner)
Logical access path   DeclaredAccessPath (ORDERED; [A,B] != [B,A])           (P5 SEEDS this authority)
Physical structure    AccessPlan (FullScanFilter | FullKeyHashLookup | …trie) (P4 live; P7 extends)
RequestEdge   exact ownership, ACYCLIC forest        RuleActivationEdge  derivation dep, MAY cycle
CROSS-CUT: inference (P9) picks NO physical structure; planning (P7) picks NO logical path;
answer identity holds because FullScanFilter is always a correct realization.
```
Full semantic authority: `next-session-prompt.md` ("Avoid these false starts", "Target semantic
representation", Phase 5). Retained invariants: member identity, exact request ownership,
caller-qualified results, drain-before-retire, single-fact-authority, counts-are-caches.

---

## §3. The path forward as diffs (POST-P4 altitude)

### P5 — the partial-binding DAG (order-free schema, order-significant edge)  [THE NEXT STEP]

Authority: `reconstruction-diffs.md §3-P5` (489-538) + §5.6. **Re-verify every anchor (§0).** Touches
the LOGICAL-ACCESS-PATH authority (`DeclaredAccessPath`, ordered) + the residual binding-state
identity (`BindingStateSchema`, order-free). This is where a DECLARED `@key` FIRST specializes — it
stops being inert and shapes the binding-state model. Like P3, P5 is largely a COMPILE-TIME modeling
addition (intern the schema DAG from declared `@key` paths + render it); its codegen impact (a
partial-key seek) is a P7 cost decision, so P5 may be STRUCTURAL-only (model + `-region-out`/render),
answer-invariant — DECIDE this in the grounding loop.

```
# (a) The logical access-path authority — ORDERED paths, order-significant identity.
+ DeclaredAccessPath { KeyPathId id; RelationId relation; vector<FieldOrdinal> ordered_fields }
+ InternDeclaredPaths(relation, raw_paths):                  # raw_paths = decl.InstanceKeys() (ordered)
+     seen={}; for p in raw_paths: canon=tuple(p)            # NO within-path sort — order IS identity
+         if canon in seen: (already a parse reject, §0)      #   (the P0-item-4 flip already enforces)
+         seen.add(canon)
+     assign KeyPathId in a DETERMINISTIC order (sort by ordered_fields) — F21
+     return DeclaredAccessPathSet{ unique by (relation, ordered_fields) }

# (b) The residual binding-state schema DAG — ORDER-FREE schema, ORDER-SIGNIFICANT edge.
+ BindingStateSchema(region, field_set) = intern(schema_table, (region, sort_by_ordinal(field_set)))
+     # @key(A) and @key(A,B)'s first hop BOTH compute {A} -> SAME BindingStateSchemaId (prefix share)
+ BindingEdge { parent_schema; added_field; child_schema }  # navigation, order-significant
+ MaterializePrefixChain(region, path):                     # LAZY: declared-or-visited, never power set
+     prev = BindingStateSchema(region, {})
+     for f in path.ordered_fields: cur = BindingStateSchema(region, prev.fields ∪ {f});
+         BindingEdge(prev, f, cur); prev = cur
+     # [A,B]: edge({},A,{A}), edge({A},B,{A,B});  [B,A]: edge({},B,{B}), edge({B},A,{A,B})
+     #   -> ONE {A,B} schema reached by TWO ordered edge chains

# (c) Cross-redecl consistency: the P5 model uses the SAME order-free-across / order-significant-within
#     semantics the parser already enforces (SameKeySetOfSets, Parser.cpp:1469-1475) — no new reject.

# (d) Render (F21): one `declared-key rel=… declared=(<ordered fields>)` line per path, KeyPathId-sorted
#     so pragma reorder -> byte-identical. RE-TARGET: the surface moved DataFlow->Regional at earlier
#     sessions; the live declared-key surface today is the -region-dot-out badge (Regional/Format.cpp:384).
#     DECIDE where the golden-pinned declared-key render lives (-region-out vs -contract-out).
```
**Exit gate (STRUCTURAL, discriminating — never answer-equality; §3-P5 F8/F20/F21):**
- (1) F8 prefix chain: for `@key(A,B,C)` the DECLARED chain `{A}⊂{A,B}⊂{A,B,C}` is present AND
  genuine non-prefix subsets `{A,C},{B},{C}` are ABSENT (compile-time schema claim; a covering-array
  probe). (2) `@key(A)`/`@key(A,B)` share ONE `{A}` schema id + one `{}--A-->{A}` edge; `[A,B]`/`[B,A]`
  are TWO ordered edges into ONE `{A,B}` schema. (3) F20: after materializing ≥1 keyed schema, an
  UNBOUND read of the same relation still returns the COMPLETE answer (⊇ any active subset). (4) F21:
  reordering two `@key` pragmas → byte-identical declared-key render. (5) A NEW positive `@key` carrier
  golden (there is none post-P1 — §0). A stub that interns nothing / sorts within a path fails these.
**Open design questions the grounding loop must settle:**
- Does P5 change codegen at all, or is it model + render only (answer-invariant, P3-style)? (If the
  binding-state DAG is compile-time structure the plan does not yet consume, P5 moves no codegen
  golden — decide + pin.)
- Where does the golden-pinned declared-key render live now (post-P1 surface migration)?
- The NEW positive `@key` carrier(s): shape, the discriminating pins, and whether they need a driver.
- D1 revisited: does P5 need non-empty `BindingStateId.vals` at compile time, or only the SCHEMA DAG
  (value-free)? (P4 kept vals symbolic; P5's schema interning is value-free by design — confirm.)

### P6 — recursive regional execution (SCC + routing + joint fixpoint + DRed deletion)
`reconstruction-diffs §3-P6` + B4/B5/H7. The `RuleActivationEdge` cross-state arm goes live; per-FACT
DRed on `RegionalFactId`; cross-component transitive retraction = ONE joint signed-frontier fixpoint.
This is where the P3/P4 model's DORMANT derivation/route half first runs for a real compile.

### P7 — physical access planning (AccessPlan its own domain; real hash/trie)  — where a narrow key first PAYS
`p7p9-diffs.md`. `kRetainedIndexScan` / `kFullKeyHashLookup` / trie arms become COST decisions;
AccessPlan dual-homes (Regional model for query reads + a Rel `plan_kind` for interior/join scans).
### P8 — the cross-relation ORDERED trie / COLT / Free Join (reuses the P5 binding-schema spine)
### P9 — access-path inference (additive, logical-only, PRE-Optimize) + Minimize/DeterminedBy — proves an UNDECLARED narrow key

```
  P6–P9 operational diffs + exit gates: reconstruction-diffs.md §3 (P6) + p7p9-diffs.md (P7–P9).
  P4/P5 make a DECLARED key drive the model; P9/O-R3.5 prove an UNDECLARED narrow key. Both downstream.
```

---

## §4. What session 20 should do (the grounding loop, weighted to P5)

Enough is persisted to resume cold (this seed + `p4-grounding.md` + `reconstruction-diffs.md §3-P5/§5.6`
+ `next-session-prompt.md` Phase 5 + `p7p9-diffs.md` + memory `regional-dataflow-core-epoch`). P5 is
the next ACTIONABLE constructive step. Run the **build-pseudocode → design-goal diffs → critique →
IR-desired-states** loop (the method that worked for P2/P3/P4), on the POST-P4 codebase, weighted to P5:

1. **Ground the whole program at the POST-P4 tip** (this seed is the backbone — keep it current). Run
   the P5 analog of the P1–P4 symbol grep: enumerate every current site P5 touches or reuses — the
   `@key` parse surface (`InstanceKeys`/`HasInstanceKey`/`ParseLocalExport` states 21/22), the
   `BindingStateSchemaId`/`BindingStateId` substrate + `EmptyBindingState`, the declared-key render
   surface (re-locate post-P1: `-region-out`/`-region-dot-out`/`-contract-out`), and the P4
   `RequestPortRecord`/`ComputeQueryAccessPlan`/`PlanFor` seam P5 layers on. Ground every anchor.
   Settle the OPEN questions in §3-P5 (does P5 change codegen; where the render lives; the new carrier).
2. **Formulate design-goal diffs** at hunk grain with DISCRIMINATING STRUCTURAL exit gates (F8 prefix
   present + non-prefix absent; `@key(A)`⊂`@key(A,B)` share one schema id; `[A,B]`/`[B,A]` two edges one
   schema; F20 unbound-read completeness; F21 render-reorder byte-identity). Answer-equality is a LOST
   CHECK. Keep the four authorities separate — the ORDERED `DeclaredAccessPath` must not conflate with
   the ORDER-FREE `BindingStateSchema` (that IS the P5 headline distinction).
3. **Critique adversarially** (opus refuter panel) against the real POST-P4 code + retained invariants +
   the "Avoid these false starts" checklist. Special scrutiny: does the order-free schema / order-sig
   edge split hold end to end (no within-path sort leaks; the `[A,B]`/`[B,A]` convergence real)? Does an
   unbound read still see the complete relation (F20 — the "do not enumerate only active states" false
   start)? Does P5 avoid power-set materialization (only declared/visited edges)? Is the new `@key`
   carrier's gate discriminating (a stub that ignores `@key` must fail)? VERIFY, don't assert; rank.
4. **Author/extend the desired IR output states** (predict-then-verify, STRUCTURAL pins): the P5
   declared-key render (where it lives + KeyPathId-sorted), any `-region-out` binding-schema block, and
   whether `.rel`/`datalog.h` move (likely NOT if P5 is model+render only — pin that). A NEW positive
   `@key` carrier golden. Sonnet pulls current carrier dumps as the baseline; opus authors the desired
   states.

METHOD: WORKFLOWS (opus for diff-authoring / critique / judgment; sonnet for mechanical census /
carrier-dumps / anchor re-verification). Several sequential single-phase workflows beat one
mega-workflow; keep the orchestrator thin. **Docs-only unless the owner green-lights P5 execution**
(the [OWNER STOP]). All anchors are grounded at tip `c546a6a4` (re-verify any before trusting; the
`reconstruction-diffs §3-P5` anchors PREDATE P1–P4 and have moved).
