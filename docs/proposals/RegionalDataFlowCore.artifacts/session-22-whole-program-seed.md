# Keyed-instance rewrite — POST-P6.1 whole-program pseudocode + path forward as diffs (session-22 seed)

Session 21 close (2026-08-09). Branch `keyed-instances`, **tip `734712c0`** ("P6.1: query-independent
recursive components (compile-time, codegen byte-unchanged)"). **P1 + P2 + P3 + P4 + P5 + P6.1 ARE
LANDED** (compile-clean, OptDiff **SUITE: PASS (224)**, ctest **5/5**). This seed is the START-HERE
whole-program backbone for **P6.2** — the second (and final) compile-time slice of P6. It supersedes
`session-21-whole-program-seed.md` §1 (POST-P5 pipeline) for the Regional layer — P6.1 added the
`recursive_components` SCC-projection authority + its `-region-out` render.

**GREENFIELD RULING (owner), still governing.** The compiler is not in use → delete-then-rebuild.
Every post-P1 gate is STRUCTURAL, never answer-equality (the full-materialization M3 backend answers
correctly with the regional model still a compile-time observer). Memory `greenfield-rewrite-motivation`.

**OWNER DECISIONS carried from session 21 (still governing):** P6 is a **COMPILE-TIME first cut**
(P6.1 + P6.2 populate `recursive_components`/`rules` as a compile-time analysis; codegen UNCHANGED;
the M3 backend still evaluates). Real RUNTIME evaluation (P6.3–P6.6: EvaluateEpoch / per-fact DRed /
cyclic activation) is a later, SEPARATELY-gated cut. Session mode was **ground-then-execute**; confirm
the owner's mode for session 22 at the [OWNER STOP].

## §0. Status — what P6.1 changed, what is grounded for P6.2

**Landed at P6.1 (tip `734712c0`; full record `p6-grounding.md` §6 = the v2 that shipped):**
- `struct RecursiveComponent { std::vector<RelationId> members; }` (Regional.h — replaced the empty
  stub). `RegionTemplate.recursive_components` is now POPULATED by `ComputeRecursiveComponents`
  (Planning.cpp, the SOLE populator).
- The projection: bucket the frozen relations by the DataFlow **multi-view-stratum** SCC condensation
  (`QueryView::Stratum()`, a Tarjan condensation that CLOSES message publish→receive seams — the F7
  question, decisive over the seam-blind `InductionGroupId`), mapping relations via `OriginDecls`. A
  multi-view stratum IS an SCC cycle (self-recursion = size-1 `members`, co-recursion = size-N);
  `OriginDecls` is narrower than rows-flow-through so a base relation (edge) is excluded from tc's
  component. **Reads NO `rules`** ⟹ P6.1 was independent of P6.2 (and P6.2 is independent of it).
- MODE-FAITHFUL, not mode-invariant (a per-compile observer of the actual per-mode graph): un-optimized
  modes surface vacuous `p:-p` self-loops that canonicalization strips. No mode-uniform strip exists
  short of dead-flow analysis (opt-only). Pinned by `recursion.dr` (0 opt/nocf vs 5 nodf/none).
- Render: a gated OWN-WIDTH `-region-out` block `recursive-component  C<k>  members=(…)` after the
  declared-key block, **NO census count** (the P5 declared-key precedent — only recursive dumps move;
  16 unrelated region goldens byte-identical). Codegen BYTE-UNCHANGED.
- Carriers: `corecursion_1` (NEW co-recursion witness), `two_inductions`/`recursion` region `.irgold`s,
  `tc_nonlinear_diff` +1 line. The grounding loop's 3-refuter opus panel EMPIRICALLY refuted a v1
  insert-arm formulation (all corpus recursion carriers are Tier-2 origin-interiors) — the origin
  projection is the survivor.

**Post-P6.1 honest baseline for P6.2 (grep-verified at tip):**
- `RegionTemplate.rules` (`std::vector<RuleRoutingProjection>`) and `inherited_symbolic_fields`
  (`std::vector<SymbolicFieldId>`) are STILL **RESERVED-EMPTY** (Regional.h:175/178); **`struct
  RuleRoutingProjection {};` is still an EMPTY stub** (Regional.h:159) — **P6.2 is the sole populator
  of BOTH** (the comments say so). `recursive_components` is now populated; `rules` is the last
  RESERVED-EMPTY field of the compile-time-first-cut.
- `struct SymbolicFieldId { uint32_t v; }` (RegionInstance.h:79) EXISTS but is RESERVED — "P6.2 is the
  sole consumer." There is NO `SymbolicFieldId` minting anywhere yet.
- **`RelationSchema` has NO `fields` vector today** (Regional.h:69-94: `id`, `decl`,
  `member_key_positions`, `support`, `declared_access_paths`). The reconstruction-diffs P2 pseudocode
  once showed `RelationSchema.fields : vector<SymbolicFieldId>` but it was DROPPED at P2 — **P6.2
  introduces the symbolic-field machinery FRESH.** A relation's fields today ARE its
  `decl.NthParameter(i)` (i ∈ [0, decl.Arity())).
- The P5 per-relation binding-schema DAG keys on `(RelationId, sorted ordinal set)` — the honest
  stand-in for `(region, SymbolicFieldSet)` that "P6.2 promotes, REBUILT not migrated at P8" (the A3
  note, RegionInstance.h + p5-grounding). So P6.2's `SymbolicFieldId` is the region-global field
  identity the P5 per-relation `(RelationId, ordinal)` becomes when producers agree.
- There is **no `RuleId`** assigned anywhere yet (reconstruction-diffs names it; P6.2 mints it).
- The DORMANT derivation/route half (`activation_edges`, `AddDerivation`, `RouteResults`,
  `RootedReachability`-with-propagation) is STILL ctest-only synthetic-rows; `DeriveActivationEdge`
  does not exist as a method (a prose comment). **P6.2 does NOT touch this half** (it is P6.4+ runtime).
- The `ParsedClause` walk API is available to the Regional layer: `ParsedDeclaration::Clauses()` /
  `ParsedModule::Clauses()`, `ParsedClause::PositivePredicates(group)` / `NegatedPredicates(group)` +
  the head, `ParsedPredicate::Arguments()` → `UsedNodeRange<ParsedVariable>`, and `ParsedVariable`
  identity for matching body-position ↔ head-position by the SAME variable (Parse.h:213/312/355/482).
- The actual recursive EVALUATION is still the retained M3 backend (ControlFlow/Build/Stratum.cpp
  OVERDELETE→REDERIVE→INSERT over split `C_nr`/`C_r`, `LowerDRRounds` :1794-1872). P6.2 does not touch it.

## §1. The whole program TODAY (POST-P6.1) — pseudocode

### §1.1 Pipeline (bin/drlojekyll/Main.cpp) — anchors current at tip
```
compile(module, flags):
    query   = Query::Build(module, log, gPassPolicy)              # DataFlow IR (3-arg since P1)
    frozen  = FrozenRegionalProgram::Build(query, log)            # Regional model: P2 typed owner +
                                                                   #   P3 request/derivation model + P4
                                                                   #   AccessPlan + P5 DeclaredAccessPath/
                                                                   #   schema DAG + P6.1 recursive_components
    SetRelDumpStream(gRelStream)
    program = Program::Build(frozen, log, gFirstId, gPassPolicy)  # ControlFlow IR (reads frozen: P4 PlanFor)
    GenerateDatabaseCode(program, h, cc)                          # C++ codegen (M3 backend)
```

### §1.2 DataFlow — Query::Build (unchanged by P2–P6.1; the SCC source for P6.1, the clause source for P6.2)
```
Query::Build(module, log, policy):                               # lib/DataFlow/Build.cpp
    build SELECT/…/INSERT; Simplify?; Connect; Optimize?; LinkViews; IdentifyInductions;
    Finalize*; BuildEquivalenceSets; Stratify(log)               # Stratify computes view->stratum (SCC),
                                                                   #   closing INSERT->SELECT message seams
    impl->row_contracts = InferConservativeRowContracts(impl)
    return Query(impl)
# @key is parsed metadata on ParsedDeclaration; ParsedClause structure is the P6.2 rule source.
```

### §1.3 Regional — FrozenRegionalProgram::Build (P2..P6.1) [POST-P6.1]
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
    for schema in R.relation_schemas:                            # ---- P5 DAG ----
        for p in schema.declared_access_paths.paths: RR.MaterializePrefixChain(p)
    R.recursive_components = ComputeRecursiveComponents(query, R.relation_schemas)   # ---- P6.1 (LANDED) ----
    # rules / inherited_symbolic_fields stay RESERVED EMPTY  <-- P6.2 POPULATES THESE (the LAST empties)
    out.census = DeriveRegionalCensus(query); RECOUNT belt
    RunFreezeValidators: V-FROZEN-NO-OPEN-PORT, V-OWNERSHIP-ACYCLIC, V-PLAN-HONEST(P4), V-PREFIX-CHAIN(P5)
    return out

ComputeRecursiveComponents(query, relation_schemas):            # P6.1, the SOLE populator
    rel_ids = { schema.id.v for schema in relation_schemas }
    views_per_stratum = histogram over query.ForEachView of V.Stratum()
    members : map<stratum, set<RelationId>>                      # ascending stratum
    query.ForEachView(V):
        s = V.Stratum(); if !s or views_per_stratum[s] <= 1: continue     # only MULTI-VIEW (recursive) strata
        for decl in V.OriginDecls(): if decl.Id() in rel_ids: members[s].insert(RelationId{decl.Id()})
    return [ RecursiveComponent{sorted(mem)} for (s,mem) in members if !mem.empty() ]
```

### §1.3a THE MODEL as it stands (the P6.2 launch point)
```
# Compile-time authorities on the frozen program:
#   #1 RegionalFactId (fact id) ...................... P3
#   #2 per-relation binding-schema DAG (order-free) ... P5 (RelSchemaLocalId/schema_table/binding_edges)
#   #3 DeclaredAccessPath (ORDERED) .................. P5
#   #4 AccessPlan (physical) ......................... P4 (the ONLY one that drives codegen)
#   #5 recursive_components (SCC structure) .......... P6.1 (compile-time observer, no codegen)
# STILL RESERVED-EMPTY: rules (RuleRoutingProjection stub), inherited_symbolic_fields, SymbolicFieldId
#   (no minting). <-- P6.2 makes the ROUTING half real. activation_edges / AddDerivation / RouteResults
#   remain DORMANT (ctest-only synthetic rows) — that is P6.4+ (runtime), NOT P6.2.
```

### §1.4 ControlFlow + codegen — unchanged since P4
```
Program::Build(frozen,…): query=frozen.DataFlowGraph(); context.frozen=&frozen (P4 PlanFor read)
    …build DR flow, induction fixpoint (Stratum.cpp), commit sweeps…   # the retained M3 backend
    BuildQueryEntryPointImpl: plan=frozen.PlanFor(redecl); withhold index iff kFullScanFilter (P4)
# P5/P6.1 added NOTHING here. Codegen is the M3 backend; the regional model is a compile-time observer.
```

---

## §2. The target — four authorities + two edges (carried, unchanged)
```
Logical fact          RegionalFactRelation : RegionalFactId -> RegionalFact  (SINGLE fact authority)
Residual              BindingState (schema + typed values + FactDerivation ids)  (NOT a 2nd fact owner)
Logical access path   DeclaredAccessPath (ORDERED; [A,B] != [B,A])           (P5 LIVE)
Physical structure    AccessPlan (FullScanFilter | FullKeyHashLookup | …trie) (P4 live; P7 extends)
RequestEdge   exact ownership, ACYCLIC forest        RuleActivationEdge  derivation dep, MAY cycle (P6.4)
SymbolicFieldId  region-global field identity, promoted only when EVERY producer agrees (P6.2)
Retained invariants: member identity, exact request ownership, caller-qualified results,
  drain-before-retire, single-fact-authority, counts-are-caches, liveness re-derived NOT refcounted.
```
Full semantic authority: `next-session-prompt.md` (Phase 6, "Avoid these false starts", the target rep).

---

## §3. The path forward as diffs (POST-P6.1 altitude)

### P6.2 — typed edge-local field projections + SymbolicFieldId promotion  [THE NEXT STEP]

Authority: `keyed-rewrite-reconstruction-diffs.md §3-P6.1/P6.2` (lines 540-577 — the
`RuleRoutingProjection`/`PromoteSharedSymbolicField` formulation + exit gate) + `p6-grounding.md §4`
+ `next-session-prompt.md` Phase 6 step 2. **Re-verify every anchor** (they predate P1–P6.1; the
reconstruction-diffs `ComputeRecursiveComponents-after-rules` ordering is DEAD — P6.1 landed first via
the origin projection and reads no rules; P6.2 now follows P6.1 and is independent of it).

**Recommended shape (compile-time, codegen-unchanged — the P6.1 cadence):**
```
# P6.2a — a region-global SymbolicFieldId per (relation, ordinal), promoted across agreeing rules.
+ struct RuleRoutingProjection {                                  # replace the empty stub
+     RuleId id;
+     std::vector<std::pair<SymbolicFieldId, SymbolicFieldId>> body_to_head;   # per shared variable
+ }
+ AssignSymbolicFields(R):                                        # seed: one fresh SymbolicFieldId per
+     for schema in R.relation_schemas:                           #   (relation, ordinal) field position
+         for i in [0, schema.decl.Arity()):  intern (schema.id, i) -> SymbolicFieldId
+ BuildRuleRoutingProjections(query, R):                          # per-CLAUSE var-position matching
+     for decl in R.relation_schemas' decls:
+         for clause in decl.Clauses():                           # ParsedClause; head=decl, body=predicates
+             for body_pred in clause.PositivePredicates(0):
+                 for (body_i, var) in enumerate(body_pred.Arguments()):
+                     for (head_j, hvar) in enumerate(head args) if hvar is the SAME variable as var:
+                         rule.body_to_head += ( SymFld(body_pred.decl, body_i), SymFld(decl, head_j) )
+     R.rules = [ RuleRoutingProjection{RuleId(clause), map} for clause ]
+ PromoteSharedSymbolicField(f_a, f_b, R):                        # union classes iff EVERY producer agrees
+     producers = ProducerRulesOf(rel_of(f_a)) ∪ ProducerRulesOf(rel_of(f_b))
+     if producers and all((f_a<->f_b) ∈ rule.body_to_head for rule in producers): union(classes, f_a, f_b)
+ R.inherited_symbolic_fields = the promoted class representatives (region-global frame)
# @key seed: a P5 DeclaredAccessPath's ordered fields (relation, ordinal) map through the promotion to
#   the destination binding PREFIX a routed fact would carry (P6.2 feeds the P6.4 routing; NO routing yet).
# Render: a -region-out `rule R<k> ...`/`routing ...` block (deterministic order) + census? DECIDE
#   (P6.1 chose NO census count — follow it unless the rule count is a first-class skeleton count).
# HEADLINE OPEN (settle in the grounding loop, do NOT presume — the P6.1 lesson):
#   Source the rule graph from PARSED CLAUSES (identity-preserving — P6.1 proved DataFlow loses
#   per-relation identity via CSE model-sharing) BUT the frozen relations are POST-Optimize; a clause
#   whose relations were folded away has no schema. Reconcile: build rules only among R.relation_schemas'
#   decls; a folded interior rule is out of the frozen model by construction (like P6.1's rel_ids filter).
#   Confirm this covers the corpus + does not double-count multi-clause relations.
```

**Exit gate (STRUCTURAL, discriminating — reconstruction-diffs:567-572).** Co-recursive
`p(K,X)@key(K), q(X,K)@key(X)` with NO `#query`: `rules` populated with the per-clause field maps;
a hand-constructed **all-producers-agree** case PROMOTES a shared `SymbolicFieldId` (both relations'
K-position unify into one class); a **co-occurrence-only** case does NOT promote (both arms of
`PromoteSharedSymbolicField`); `recursive_components` byte-identical under add/remove `#query`
(re-assert P6.1's property holds); **codegen byte-stable** (`.rel`/`datalog.h`/`.stdout` unchanged —
decide + pin, P6.1-style). A stub that leaves `rules` empty FAILS. Needs a NEW positive carrier
(there is none — `corecursion_1` has no `@key`; author a `@key`-bearing co-recursion witness, likely
building on `key_partial_1`'s `#local … @key(…)` + distinct-`#query` shape).

**Open design questions the P6.2 grounding loop must settle:**
- The HEADLINE clause-vs-DataFlow rule source (above) — and whether the frozen-relation filter is sound.
- Does P6.2 change codegen? (If compile-time model+render, NO — pin it, P6.1-style.)
- Where the `rule`/`routing` render lives + census impact (P6.1 chose no census count; decide).
- Is the P5 per-relation `(RelationId, ordinal)` REBUILT region-global now (A3 said "rebuilt at P8" —
  reconcile: P6.2 promotes symbolic fields for ROUTING; P8 rebuilds the trie spine). Where does the
  `SymbolicFieldId` interner live (a region-global pool on `RegionInstanceRelations`, peer of P5's
  `schema_table`)?
- Negation/aggregate/@product body atoms: do they participate in the routing projection, or only
  positive relation predicates? (`NegatedPredicates` exists; F18 says P3 mints no intra-state edges —
  keep P6.2 to positive relation predicates for the first cut? decide.)
- The `RuleId` identity: keyed on the `ParsedClause` (UniqueId is pointer-order — HP-9 forbids it as a
  golden key; use a deterministic clause ordinal per decl, like P6.1's decl.Id ordering).

**Deferred to the RUNTIME cut (P6.3–P6.6 — separately gated, likely touches codegen/runtime):**
```
# P6.3 fusion (FusedFixpoint vs JointFixpoint; L6 order from declared @key path; fusion answer-checked)
# P6.4 DeriveActivationEdge (cross-state, MAY cycle, NEVER AddRequestEdge; F18 no self-loops acyclic slice)
# P6.5 EvaluateEpoch TWO passes: (A) within-epoch support-loss DRed per-FACT on RegionalFactId (B4),
#      mirroring ControlFlow/Build/Stratum.cpp OVERDELETE->REDERIVE->INSERT; interleaved with (B) joint
#      rooted-reachability + semi-naive worklist as ONE signed-frontier worklist across components (B5).
# P6.6 RetireUnreachableSCCs: liveness RE-DERIVED (never refcount), drain-before-retire, H7 cascade-retract.
```

### P7 — physical access planning (AccessPlan its own domain; real hash/trie) — where a narrow key first PAYS
`keyed-rewrite-p7p9-diffs.md`. `kRetainedIndexScan`/`kFullKeyHashLookup`/trie arms become COST decisions;
the intra-relation prefix seek is P7 `GetOrCreateIndex(subset)`. AccessPlan dual-homes.
### P8 — the cross-relation ORDERED trie / COLT / Free Join (rebuilds the P5 binding-schema spine region-global)
### P9 — access-path inference (additive, logical-only, PRE-Optimize) + Minimize/DeterminedBy
```
  P7–P9 operational diffs + exit gates: keyed-rewrite-p7p9-diffs.md + keyed-rewrite-p7p9-critique.md.
```

---

## §4. What session 22 should do (the grounding loop, weighted to P6.2)

Enough is persisted to resume cold (this seed + `p6-grounding.md` §4/§6 + `keyed-rewrite-reconstruction-diffs.md
§3-P6.2` + `next-session-prompt.md` Phase 6 + memory `regional-dataflow-core-epoch`). P6.2 is the next
ACTIONABLE step and the LAST compile-time slice of P6. Run the **build-pseudocode → design-goal diffs
→ critique → IR-desired-states** loop (the method that landed P2–P6.1), on the POST-P6.1 codebase:

1. **Ground the whole program at the POST-P6.1 tip** (this seed is the backbone — keep it current). Run
   the P6.2 analog of the P1–P6.1 symbol grep: enumerate every site P6.2 touches/reuses — the
   RESERVED-EMPTY `rules`/`inherited_symbolic_fields` + the `RuleRoutingProjection` stub +
   `SymbolicFieldId` (no minting); the `ParsedClause`/`ParsedPredicate`/`ParsedVariable` walk API + how
   to match body-pos↔head-pos by variable identity; the P5 `(RelationId, ordinal)` schema_table (what
   P6.2 promotes region-global); the `RelationSchema` shape (no `fields` vector today). Ground every
   anchor. **SETTLE the headline open decision (clause-vs-DataFlow rule source; the P6.1 identity
   lesson) with the owner.**
2. **Formulate design-goal diffs** at hunk grain with DISCRIMINATING STRUCTURAL exit gates
   (all-producers-agree PROMOTES a shared SymbolicFieldId; co-occurrence-only does NOT — both arms of
   `PromoteSharedSymbolicField`, F16; `recursive_components` byte-identical under add/remove `#query`).
   Answer-equality is a LOST CHECK. Keep the four authorities separate; the SymbolicFieldId promotion is
   for ROUTING (P6.2), the trie spine is P8. RequestEdge acyclic vs RuleActivationEdge may-cycle — but
   P6.2 mints NO activation edges (that is P6.4).
3. **Critique adversarially** (opus refuter panel; VERIFY EMPIRICALLY by compiling throwaway carriers —
   this is what caught the P6.1 blocking inversion) against the real POST-P6.1 code + retained invariants
   + the "Avoid these false starts" checklist. Special scrutiny: is the clause-based rule source sound
   given CSE / folded interiors (does the frozen-relation filter cover the corpus)? Does
   `PromoteSharedSymbolicField` reach a FIXPOINT (F28) and is the "every producer agrees" test right for
   multi-clause relations? Is the RuleId deterministic (HP-9, no pointer/UniqueId golden key)? Does P6.2
   accidentally touch codegen or the dormant runtime half? VERIFY, don't assert; rank survivors; record
   refuted diffs as certs.
4. **Author/extend the desired IR output states** (predict-then-verify, STRUCTURAL pins): the
   `-region-out` `rule`/`routing` render (where it lives + deterministic order), any census impact,
   whether `.rel`/`datalog.h` move (likely NOT if compile-time model+render — pin that prediction,
   P6.1-style), and a NEW `@key`-bearing co-recursion carrier (there is none). Sonnet pulls current
   carrier dumps as the baseline; opus authors the desired states.

METHOD: WORKFLOWS (opus for diff-authoring/critique/judgment; sonnet for mechanical census/carrier-dumps/
anchor re-verification). Several sequential single-phase workflows beat one mega-workflow; keep the
orchestrator thin. **Docs-only unless the owner green-lights P6.2 execution** (the [OWNER STOP]). All
anchors are grounded at tip `734712c0` (re-verify any before trusting; the `reconstruction-diffs §3-P6.2`
anchors PREDATE P1–P6.1 and the ComputeRecursiveComponents-after-rules ordering is DEAD).
