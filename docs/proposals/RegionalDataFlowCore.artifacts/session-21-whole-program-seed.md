# Keyed-instance rewrite — POST-P5 whole-program pseudocode + path forward as diffs (session-21 seed)

Session 20 close (2026-08-09). Branch `keyed-instances`, **tip `375e8713`** ("P5: the partial-binding
DAG (declared @key first specializes)"). **P1 + P2 + P3 + P4 + P5 ARE LANDED** (compile-clean, OptDiff
**SUITE: PASS (223)**, ctest **5/5**). This seed is the START-HERE whole-program backbone for **P6**. It
supersedes `session-20-whole-program-seed.md` §1 (POST-P4 pipeline) for the Regional layer — P5 added
the LOGICAL-ACCESS-PATH authority (`DeclaredAccessPath`) + the per-relation binding-schema DAG.

**GREENFIELD RULING (owner), still governing.** The compiler is not in use → delete-then-rebuild.
Every post-P1 gate is STRUCTURAL, never answer-equality (the full-materialization backend answers
correctly with the regional model still a compile-time observer). Memory `greenfield-rewrite-motivation`.

## §0. Status — what P5 changed, what is grounded for P6

**Landed at P5 (tip `375e8713`; full record `p5-grounding.md`):**
- The ORDERED `DeclaredAccessPath{KeyPathId, relation, ordered_fields}` authority + pure
  `InternDeclaredPaths` (KeyPathId sorted by ordered_fields → F21) in `RegionInstance.h`.
- The per-relation ORDER-FREE binding-schema DAG on `RegionInstanceRelations`: `RelSchemaLocalId`
  (DISTINCT from the region-global `BindingStateSchemaId`), `schema_table` keyed on
  `(RelationId, sorted ordinal set)`, `binding_edges`, `InternBindingSchema`, lazy
  `MaterializePrefixChain` (declared-or-visited only, never the power set).
- `RelationSchema.declared_access_paths` (Regional.h), populated from `decl.InstanceKeys()` in BOTH
  freeze arms; the freeze materializes the prefix chains; the always-on `HasInstanceKey()`-tied
  **V-PREFIX-CHAIN** belt (interned nodes == exactly the declared-prefix UNION).
- A golden-pinned `-region-out` `declared-key` block (Regional/Format.cpp); the DOT badge kept.
- **MODEL + RENDER ONLY — no codegen** (`key_partial_1` `.rel`/`datalog.h`/`.cpp` byte-identical to the
  pre-P5 baseline; the physical partial-key seek is P7). `BindingStateId.vals` stays `{}` (value-free).
- NEW carrier `key_partial_1` (the first positive `@key` case post-P1) + the mandatory ctest
  `RegionInstanceP5` GateD–GateI DAG battery.

**Post-P5 honest baseline for P6 (all grep-verified at tip):**
- `RegionTemplate.recursive_components` (`std::vector<RecursiveComponent>`) and `rules`
  (`std::vector<RuleRoutingProjection>`) and `inherited_symbolic_fields`
  (`std::vector<SymbolicFieldId>`) are ALL **RESERVED-EMPTY** (Regional.h:159-176); `RecursiveComponent`
  and `RuleRoutingProjection` are empty stub structs. **P6.1 is the sole populator of
  `recursive_components`; P6.2 of `rules`/symbolic fields** (comments say so).
- `RegionInstanceRelations.activation_edges` is **RESERVED-EMPTY**; `RootedReachability` has NO
  activation propagation (a single topo sweep, "activation_edges is empty at P3 → no propagation");
  `AddDerivation`/`RouteResults`/`DeriveActivationEdge` have **NO real-compile callers** — the whole
  derivation/route half is exercised ONLY by the `RegionInstance` ctest with synthetic rows (P3-style,
  reaffirmed unchanged at P4/P5). This is "the DORMANT half."
- The actual recursive evaluation is delivered by the retained **M3 full-materialization backend**:
  DataFlow `Stratify` already computes the view-graph SCC condensation (`view->stratum`; equal ids ⟺
  same SCC; public accessor `QueryView::Stratum()`, Query.h:367); the differential fixpoint +
  OVERDELETE→REDERIVE→INSERT over split `C_nr`/`C_r` lives in **`lib/ControlFlow/Build/Stratum.cpp`**
  (the `reconstruction-diffs` "Stratum.cpp:1796-1841" anchor is THIS file, re-verify the exact lines).
- `@key` is now LIVE-but-compile-time: it shapes the P5 model + render, drives no codegen, and does not
  yet participate in SCC/routing (that is P6.2 — `@key` paths seed the destination binding prefixes).

## §1. The whole program TODAY (POST-P5) — pseudocode

### §1.1 Pipeline (bin/drlojekyll/Main.cpp) — anchors current at tip
```
compile(module, flags):
    query   = Query::Build(module, log, gPassPolicy)              # DataFlow IR (3-arg since P1)
    frozen  = FrozenRegionalProgram::Build(query, log)            # Regional model: ports + P3 request/
                                                                   #   derivation model + P4 AccessPlan
                                                                   #   + P5 DeclaredAccessPath/schema DAG
    SetRelDumpStream(gRelStream)
    program = Program::Build(frozen, log, gFirstId, gPassPolicy)  # ControlFlow IR (reads frozen: P4 PlanFor)
    GenerateDatabaseCode(program, h, cc)                          # C++ codegen (M3 backend)
```

### §1.2 DataFlow — Query::Build (unchanged by P2–P5; the SCC source for P6.1)
```
Query::Build(module, log, policy):                               # lib/DataFlow/Build.cpp
    build SELECT/…/INSERT; Simplify?; Connect; Optimize?; LinkViews; IdentifyInductions;
    Finalize*; BuildEquivalenceSets; Stratify(log)               # Stratify computes view->stratum (SCC)
    impl->row_contracts = InferConservativeRowContracts(impl)
    return Query(impl)
# @key is parsed metadata on ParsedDeclaration; it does NOT participate in Query::Build.
```

### §1.3 Regional — FrozenRegionalProgram::Build (P2 typed owner + P3 model + P4 plan + P5 DAG) [POST-P5]
```
Build(query, log):                                               # lib/Regional/Planning.cpp
    out.dataflow_graph = query; R = out.region; RR = out.instances
    input/result ports + ABIs (declaration order)                # P2
    for parsed_query in dedup-by-Id(walk):                       # BuildRequestPorts (P3 + P4 plan)
        for redecl in dedup-by-BindingPattern(...):
            if HasBoundParam: plan=ComputeQueryAccessPlan; R.request_ports+={…,plan}; RR.AddRequestEdge(RootLease,…)
            else: R.permanent_roots+={redecl}; RR.AddRequestEdge(PermanentRoot,…)
    for (decl,ins) in CollectContractInserts:  R.relation_schemas += BuildRelationSchemaFromInsert(...)   # +P5 paths
    for decl in CollectOriginInteriorDecls:    R.relation_schemas += BuildRelationSchemaFromOrigin(...)   # +P5 paths
    for schema in R.relation_schemas:                            # ---- P5 DAG (NEW) ----
        for p in schema.declared_access_paths.paths: RR.MaterializePrefixChain(p)
    # rules / recursive_components stay RESERVED EMPTY  <-- P6.1/P6.2 POPULATE THESE
    out.census = DeriveRegionalCensus(query); RECOUNT belt
    RunFreezeValidators: V-FROZEN-NO-OPEN-PORT, V-OWNERSHIP-ACYCLIC, V-PLAN-HONEST(P4), V-PREFIX-CHAIN(P5)
    return out
```

### §1.3a THE MODEL as it stands (the P6 launch point)
```
# Compile-time authorities on the frozen program:
#   #1 RegionalFactId (fact id) ...................... P3
#   #2 per-relation binding-schema DAG (order-free) ... P5 (RelSchemaLocalId/schema_table/binding_edges)
#   #3 DeclaredAccessPath (ORDERED) .................. P5
#   #4 AccessPlan (physical) ......................... P4 (the ONLY one that drives codegen)
# DORMANT for real compiles (ctest-only, synthetic rows): AddDerivation / RouteResults /
#   DeriveActivationEdge / RootedReachability-with-propagation / EvaluateEpoch. activation_edges,
#   rules, recursive_components all EMPTY. <-- P6 first makes the RECURSIVE-analysis half real.
```

### §1.4 ControlFlow + codegen — unchanged since P4
```
Program::Build(frozen,…): query=frozen.DataFlowGraph(); context.frozen=&frozen (P4 PlanFor read)
    …build DR flow, induction fixpoint (Stratum.cpp), commit sweeps…   # the retained M3 backend
    BuildQueryEntryPointImpl: plan=frozen.PlanFor(redecl); withhold index iff kFullScanFilter (P4)
# P5 added NOTHING here. Codegen is the M3 backend; the regional model is a compile-time observer.
```

---

## §2. The target — four authorities + two edges (carried, unchanged)
```
Logical fact          RegionalFactRelation : RegionalFactId -> RegionalFact  (SINGLE fact authority)
Residual              BindingState (schema + typed values + FactDerivation ids)  (NOT a 2nd fact owner)
Logical access path   DeclaredAccessPath (ORDERED; [A,B] != [B,A])           (P5 LIVE)
Physical structure    AccessPlan (FullScanFilter | FullKeyHashLookup | …trie) (P4 live; P7 extends)
RequestEdge   exact ownership, ACYCLIC forest        RuleActivationEdge  derivation dep, MAY cycle (P6.4)
Retained invariants: member identity, exact request ownership, caller-qualified results,
  drain-before-retire, single-fact-authority, counts-are-caches, liveness re-derived NOT refcounted.
```
Full semantic authority: `next-session-prompt.md` (Phase 6, "Avoid these false starts", the target rep).

---

## §3. The path forward as diffs (POST-P5 altitude)

### P6 — recursive regional execution  [THE NEXT STEP — LARGEST PHASE; MUST BE SUB-SLICED]

Authority: `reconstruction-diffs.md §3-P6` (P6.1–P6.6) + §5.7/§5.8 (B4/B5/H7/M4/M5/M6/L6) +
`next-session-prompt.md` Phase 6. **Re-verify every anchor** (they predate P1–P5; the Stratum.cpp DRed
line numbers especially, and `RegionInstanceRelations` has moved to RegionInstance.h). This is where the
DORMANT derivation/route half first does real work.

**THE HEADLINE OPEN DECISION the P6 grounding loop MUST settle (do NOT presume):** does P6 stay a
COMPILE-TIME model addition (like P3/P4/P5 — populate `recursive_components`/`rules`/activation edges as
a compile-time analysis + validators + render + ctest, codegen UNCHANGED, the M3 backend still evaluates)
OR does P6 begin REAL RUNTIME EVALUATION (the model DRIVES the fixpoint, a step toward replacing M3)?
The P3/P4/P5 cadence + the greenfield staging argue for a COMPILE-TIME first cut; `next-session-prompt`
Phase 6 + `reconstruction-diffs §3-P6 EvaluateEpoch` describe a RUNTIME loop. Reconcile: **the runtime
engine is the eventual goal, but P6 should be sub-sliced** so the first cut is a compile-time model
addition and the runtime fixpoint/DRed is a later, separately-gated cut.

**Recommended sub-slice (the next actionable, compile-time, codegen-unchanged):**

```
# P6.1 — query-independent SCC -> populate RegionTemplate.recursive_components (currently EMPTY).
+ RecursiveComponent { std::vector<RelationId> members; }        # replace the empty stub struct
+ ComputeRecursiveComponents(query, R):                          # M4 SAFER ARM: project, don't re-Tarjan
+     # DataFlow Stratify already computed the view-graph SCC (view->stratum; equal ids <=> same SCC).
+     # Project it onto RELATIONS: group each relation-schema's producing view(s) by Stratum(); a
+     # component = a stratum with >1 relation OR a self-recursive relation (a back-edge within a stratum).
+     for schema in R.relation_schemas: bucket[stratum_of(schema)].add(schema.id)
+     R.recursive_components = [ RecursiveComponent{sorted members} for bucket b if recursive(b) ]
+     # DEBUG belt: cross-check against a relation-projected condensation (F19 positive assertion).
# Render: a `-region-out` `recursive-component C<k> members=(rel…)` block (deterministic order). Census? decide.

# P6.2 — typed edge-local field projections + SymbolicFieldId promotion -> populate rules/inherited fields.
+ RuleRoutingProjection { RuleId; vector<pair<SymbolicFieldId body_pos, SymbolicFieldId head_pos>> }  # replace stub
+ PromoteSharedSymbolicField(f_a, f_b): union ONLY when EVERY producer rule agrees on the mapping (F16).
+ # @key paths (P5) seed the destination binding PREFIX a routed fact carries (the P5 DAG feeds P6.2).
+ # SymbolicFieldId becomes the region-global field identity the P5 A3 note reserved (P5's per-relation
+ #   (RelationId, ordinal) is REBUILT here, not migrated).
# Exit gate: co-recursive p(K,X)@key(K), q(X,K)@key(X) with NO #query -> recursive_components=={p,q}
#   one component, byte-identical under add/remove #query; a hand-built all-producers-agree case promotes,
#   co-occurrence-only does not; cross-checked against Stratify's projected condensation.
```

**Deferred to a later P6 cut (RUNTIME evaluation — likely touches codegen/runtime, separately gated):**
```
# P6.3 fusion (FusedFixpoint vs JointFixpoint; L6 order from declared @key path; fusion answer-checked vs joint)
# P6.4 DeriveActivationEdge (cross-state, MAY cycle, NEVER AddRequestEdge; F18 no self-loops in the acyclic slice)
# P6.5 EvaluateEpoch TWO passes: (A) within-epoch support-loss DRed per-FACT on RegionalFactId (B4 — NOT a
#      per-BindingState C_nr/C_r mirror), mirroring the landed ControlFlow/Build/Stratum.cpp OVERDELETE->
#      REDERIVE->INSERT; interleaved with (B) joint rooted-reachability + semi-naive worklist as ONE
#      worklist carrying signed frontiers across component boundaries (B5 — else cross-component transitive
#      retraction is dropped).
# P6.6 RetireUnreachableSCCs: liveness RE-DERIVED (never refcount — a self-supporting cycle keeps support>0
#      forever), drain-before-retire, H7 cascade-retract every FactDerivation sourced from the SCC's states.
```

**Exit gate for the recommended first cut (STRUCTURAL, discriminating):** `recursive_components` populated
correctly (a hand-built co-recursion witness — one component spanning the cycle, byte-identical under
add/remove `#query`), cross-checked against Stratify's projected condensation (F19 positive assertion);
`rules`/symbolic fields populated only where all producers agree (both arms of PromoteSharedSymbolicField);
a `-region-out` render pin + a new/extended ctest gate; **codegen byte-stable** (`.rel`/`datalog.h`/`.stdout`
unchanged — decide + pin this, as P5 did). A stub that leaves `recursive_components` empty FAILS.

**Open design questions the P6 grounding loop must settle:**
- Compile-time-only first cut vs runtime evaluation (the headline above) — and if compile-time, whether
  P6.1+P6.2 is one commit or two.
- Does P6 change codegen? (If P6.1/P6.2 are compile-time model+render, NO — pin it, P3/P5-style.)
- Where the SCC/routing render lives (`-region-out` `recursive-component`/`rule` blocks) + census impact.
- The SymbolicFieldId promotion: is the P5 per-relation DAG rebuilt into a region-global one now, or does
  P6.2 add a parallel region-global layer leaving the P5 DAG for the render only? (A3 said "rebuilt at P8"
  — reconcile: P6.2 promotes symbolic fields for ROUTING; P8 rebuilds the trie spine.)
- The M4 arm: project Stratify's condensation (reuse `QueryView::Stratum()`) vs a fresh GenericTarjan
  (F30 extraction obligation) — the projection arm is safer + already computed; confirm it covers
  message-mediated recursion (F7 — the message seam is a stratum edge in Stratify already?).

### P7 — physical access planning (AccessPlan its own domain; real hash/trie) — where a narrow key first PAYS
`p7p9-diffs.md`. `kRetainedIndexScan`/`kFullKeyHashLookup`/trie arms become COST decisions; the intra-
relation prefix seek is P7 `GetOrCreateIndex(subset)` (p7p9-critique headline). AccessPlan dual-homes.
### P8 — the cross-relation ORDERED trie / COLT / Free Join (rebuilds the P5 binding-schema spine region-global)
### P9 — access-path inference (additive, logical-only, PRE-Optimize) + Minimize/DeterminedBy

```
  P7–P9 operational diffs + exit gates: p7p9-diffs.md + p7p9-critique.md (standing realizations).
```

---

## §4. What session 21 should do (the grounding loop, weighted to P6)

Enough is persisted to resume cold (this seed + `p5-grounding.md` + `reconstruction-diffs.md §3-P6/§5.7/§5.8`
+ `next-session-prompt.md` Phase 6 + `p7p9-diffs.md` + memory `regional-dataflow-core-epoch`). P6 is the
next ACTIONABLE step and the LARGEST phase — sub-slice it. Run the **build-pseudocode → design-goal diffs
→ critique → IR-desired-states** loop (the method that landed P2–P5), on the POST-P5 codebase:

1. **Ground the whole program at the POST-P5 tip** (this seed is the backbone — keep it current). Run the
   P6 analog of the P1–P5 symbol grep: enumerate every site P6 touches/reuses — the RESERVED-EMPTY
   `recursive_components`/`rules`/`inherited_symbolic_fields` + their stub structs; `RegionInstanceRelations`
   `activation_edges`/`AddDerivation`/`RouteResults`/`DeriveActivationEdge`/`RootedReachability`; DataFlow
   `Stratify` `view->stratum`/`QueryView::Stratum()`; the ControlFlow/Build/Stratum.cpp DRed machinery
   (re-anchor the OVERDELETE→REDERIVE lines); the message-seam handling (F7). Ground every anchor. **SETTLE
   the headline open decision (compile-time first cut vs runtime engine) with the owner.**
2. **Formulate design-goal diffs** at hunk grain with DISCRIMINATING STRUCTURAL exit gates (co-recursion →
   one component, byte-identical under add/remove `#query`; producers-agree promotion both arms; the DRed
   deletion path if in scope — B0/F1 a still-rooted self-supporting cycle IS retracted). Answer-equality is
   a LOST CHECK. Keep the four authorities separate; RequestEdge acyclic vs RuleActivationEdge may-cycle.
3. **Critique adversarially** (opus refuter panel) against the real POST-P5 code + retained invariants +
   the "Avoid these false starts" checklist (esp. "do not use reference counts to collect a cyclic
   activation graph"; "do not let an internal activation edge become a request owner"; "do not implement
   recursive keyed regions by extending only InstanceStore"). Special scrutiny: is the SCC projection from
   Stratify sound + does it close message seams (F7/M4)? Does DRed (if in scope) mirror the landed
   Stratum.cpp per-FACT machinery (B4) and cover cross-component transitive retraction (B5)? Is retirement
   re-derived, never refcount (P6.6)? VERIFY, don't assert; rank survivors; record refuted diffs as certs.
4. **Author/extend the desired IR output states** (predict-then-verify, STRUCTURAL pins): the `-region-out`
   `recursive-component`/`rule` render (where it lives + deterministic order), any census impact, whether
   `.rel`/`datalog.h` move (likely NOT if the first cut is compile-time model+render — pin that prediction),
   and a NEW positive co-recursion carrier (there is none). Sonnet pulls current carrier dumps as the
   baseline; opus authors the desired states.

METHOD: WORKFLOWS (opus for diff-authoring/critique/judgment; sonnet for mechanical census/carrier-dumps/
anchor re-verification). Several sequential single-phase workflows beat one mega-workflow; keep the
orchestrator thin. **Docs-only unless the owner green-lights P6 execution** (the [OWNER STOP]). All
anchors are grounded at tip `375e8713` (re-verify any before trusting; the `reconstruction-diffs §3-P6`
anchors PREDATE P1–P5 and have moved).
