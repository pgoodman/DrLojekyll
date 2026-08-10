# Session 24 whole-program seed — POST-P7 (grounded pipeline pseudocode + path forward as diffs)

> Cold-start START-HERE for session 24. Written at the P7 close (branch `keyed-instances`,
> tip `6e81cfa1`, OptDiff SUITE PASS 226, ctest 5/5). Every anchor below was read at tip.
> clangd in this repo is NOISE (no include paths) — trust the real build. This seed is the
> whole-program view; the P7 record is `p7-execution-grounding.md`, the P7-scoped as-is
> pseudocode is `p7-grounding-seed.md §1`.

---

## §0 STATUS

- **P1–P7 LANDED.** The `keyed-instances` namesake arc (a bound `#query` over a keyed
  relation → an index seek) is CLOSED. The M3 full-materialization backend still
  EVALUATES and answers correctly; the frozen Regional layer (`lib/Regional`) is a
  COMPILE-TIME observer that now steers codegen at TWO points: P4's plan-select (withhold
  index) and P7's partial-key hash SEEK.
- **Every post-P1 gate is STRUCTURAL, never answer-equality** (the M3 baseline answers
  correctly, so answers can't discriminate a stubbed model — a dump/byte must).
- **The next step is an [OWNER STOP]:** pick the next cut (§4). No cut is pre-ranked —
  P7 completed the physical-seek arc and the three candidates differ in character.

---

## §1 WHOLE-PROGRAM PSEUDOCODE (as-is, grounded at tip)

### §1.0 The compile driver (`bin/drlojekyll/Main.cpp`)

```
main(argv):
  module = ParseAndCombineModules(...)                              // lib/Lex + lib/Parse
  query  = Query::Build(module, log, gPassPolicy)                   // :73   DataFlow IR
  frozen = FrozenRegionalProgram::Build(query, log)                 // :89   Regional model
  SetRelDumpStream(gRelStream)                                      // :106  Rel dump sink
  program = Program::Build(frozen, log, gFirstId, gPassPolicy)      // :110  ControlFlow(+Rel)
  GenerateDatabaseCode(program, h_os, cc_os, ...)                   // :142  C++ codegen
  // dump sinks: -df-out (DataFlow), -contract-out, -rel-out, -region-out, -ir-out, -cpp-out
```

Four IRs, each with its own text dump: DataFlow `.df`, Rel `.rel`, ControlFlow `.ir`,
Regional `.region` (+ `.contract`, `.origin`, DOT twins). Codegen → `datalog.h`/`.cpp`
against `lib/Runtime` (`hyde::rt`).

### §1.1 DataFlow IR — `Query::Build(module, log, policy)` (`lib/DataFlow`)

```
Query::Build:
  BuildClause per rule → node graph (SELECT/TUPLE/JOIN/MERGE/CMP/MAP/NEGATE/AGG/KVINDEX/INSERT)
    zero-arity preds desugar to unit relations; every inter-view dep is a COLUMN edge
  Optimize (unless -disable-dataflow-opt):                          // lib/DataFlow/Optimize.cpp
    Simplify → Canonicalize fixpoint → CSE → dead-flow (EliminateDeadFlows | CollectDeadCycles)
  Stratify: multi-view-stratum SCC condensation (QueryView::Stratum(), closes msg seams)
  ConnectInsertsToSelects: mint insert-proxies; seed OriginDecls (K5) + the
    Query::Build-scoped insert_proxy→decl map (Tier-1 naming)
  InferConservativeRowContracts (RowContract.{h,cpp}): member-key contracts per relation
  // mint tags (S5'): every Def<T> carries a stable "pass/what" creation-site literal
  → QueryImpl (the frozen post-Optimize graph; the SOLE input to freeze)
```

Key invariants: no view is its own user; a source-less forwarding cycle is unsatisfiable;
`QueryImpl` owns no conditions (unit relations); CSE never folds a unit SELECT into a
non-unit one; group_ids/InsertSetsOverlap is a correctness guard not an opt target.

### §1.2 Regional freeze — `FrozenRegionalProgram::Build(query, log)` (`lib/Regional`)

A DEGENERATE planner (ONE ProgramRoot + ONE observation-root region R0, no child calls,
no re-optimize) that reads the post-Optimize graph and builds a TYPED compile-time model.
It is layered OVER the M3 backend — `Program::Build`'s first act is `query = frozen.Query()`.

```
FrozenRegionalProgram::Build(query):
  R = RegionTemplate{}
  BuildRelationSchemas(query):                                      // Planning.cpp
    per distinct relation-INSERT decl → RelationSchema{member_key_positions, support}  (R-STORE)
    per Tier-2 origin-interior decl (CollectOriginInteriorDecls, K5) → RelationSchema (no rc)
  BuildRequestPorts(query):                                         // Planning.cpp:687
    per #query redecl (UniqueRedeclarations, seen_variants dedup):
      if HasBoundParam(redecl):                                     // :689
        plan = ComputeQueryAccessPlan(decl, redecl)                // :691  builds AccessRequirement
             = SelectAccessPlan({relation, has_free, available_bindings, kCompleteRelation})
             //  !has_free            -> kFullKeyHashLookup   (all-bound .Find)
             //  has_free & bindings  -> kPartialKeyHashSeek  (P7 seek)   <-- LANDED
             //  has_free & no bound  -> kFullScanFilter
        R.request_ports += RequestPortRecord{port, redecl, lease, call_site, plan}
        AddRequestEdge(...)                                         // P3 model edge
      else:  R.permanent_roots += PermanentRootRecord{...}         // all-free, no plan
  InternDeclaredPaths + binding-schema DAG (P5): DeclaredAccessPath (ordered) +
    schema_table/binding_edges (order-free); MaterializePrefixChain; V-PREFIX-CHAIN belt
  ComputeRecursiveComponents (P6.1): project the DataFlow multi-view-stratum SCC
    condensation onto frozen relations via OriginDecls → recursive_components (members)
  AssignSymbolicFields + BuildRuleRoutingProjections + PromoteSharedSymbolicField (P6.2):
    clause-source routing → rules (RuleRoutingProjection) + inherited_symbolic_fields
    (union-find FIXPOINT, directional per-head-field, F16 co-occurrence trap)
  Referees: V-FROZEN-NO-OPEN-PORT, V-OWNERSHIP-ACYCLIC, V-REGION-CENSUS (recount ==
    DeriveRegionalCensus), V-PREFIX-CHAIN, V-PLAN-HONEST (at codegen, see §1.4)
  → FrozenRegionalProgram{RegionTemplate R, RegionInstanceRelations, query}
```

**The FOUR authorities** (all typed, in `include/drlojekyll/Regional/RegionInstance.h`;
NEVER aliased): `RegionalFactId` (logical fact) · `BindingStateId` (order-free binding
schema, P5) · `DeclaredAccessPath` (ordered logical access path, P5) · **`AccessPlan`**
(physical structure, P4/P7 — the ONLY one that drives codegen). P6.1
`recursive_components` + P6.2 `rules`/`inherited_symbolic_fields` are the compile-time
recursion/routing analyses; codegen does not read them.

### §1.3 ControlFlow + Rel — `Program::Build(frozen, first_id, optimize)` (`lib/ControlFlow`, `lib/Rel`)

```
Program::Build(frozen):
  query = frozen.Query()                                            // the byte-preserving seam
  Build the Rel (delta-relational) IR from the DataFlow graph:      // lib/Rel, the stratum authority
    typed DRVecs (queues/frontiers/pivots) + DROps (sign/position/claim-context + 10 membership
      predicates + effect sets + access-plan spines); DeriveDRStrata (monotone lift);
      LinearizeAndValidateDRFlow (Kahn, band-key tie-break)
    Always-on graph validators (fprintf+abort): V-XOVER-ONE/V-PROD-*/V-JOIN-ONE, the census,
      V-LINEAR/V-LOOP/V-READY/V-BAND-HAZARD, V-PRED-XCHECK (ties DR model to Emit* templates)
  Lower Rel → ControlFlow regions (SERIES/PARALLEL/INDUCTION/LET/CHECKMEMBER/COMMITSWEEP/CLAIM/...):
    LowerDRFlow (acyclic seeds/crossovers/product/claim/frontier), LowerDRRounds (per-SCC fixpoint
      round shells), LowerCommitSweeps (commit + Seal), LowerGroupUpdate (R3 aggregates)
    ingest folds lower from DR-IR (LowerIngestFold/LowerIngestLoop); the eager descent
      (BuildEagerRegion..., Build.cpp) is the ONLY remaining hand-coded emission, cross-checked
      by the kEager* marker ops (V-PRED-XCHECK / Site-5 multisets)
  BuildQueryEntryPointImpl (Build.cpp:437):                         // the codegen-facing query read
    plan = frozen.PlanFor(decl)                                     // :444  reads AccessPlan
    withhold_index = (plan == kFullScanFilter)                      // :446
    if !withhold_index && !col_indices.empty():                     // :449
      scanned_index = GetOrCreateIndex(model.table, col_indices)    //   provisions bound-subset index
    V-PLAN-HONEST belt (per-kind implication, P7 §3-D6):           // :458  kFullScanFilter⇒¬idx,
      kPartialKeyHashSeek⇒idx  (fprintf+abort)                      //        both entry points
    impl.queries += ProgramQuery{query, table, scanned_index, ...}  // :466  scanned_index → codegen
  Optimize (unless -disable-controlflow-opt): region flatten / no-op removal / proc dedup
  → ProgramImpl
```

### §1.4 C++ codegen — `GenerateDatabaseCode(program, ...)` (`lib/CodeGen/CPlusPlus/Database.cpp`)

Sealed `struct Database` (state) + hidden-friend ADL API. Two scan surfaces:

```
A. QUERY path — EmitQueryFriends(ProgramQuery spec):               // Database.cpp:1527
   via_index = spec.index && index_member.contains(spec.index.Id())  // :1646  the ONE plan signal
   all-bound       -> .Find existence                                // kFullKeyHashLookup
   via_index=true  -> `while (pos!=kNoRow){ id=pos; pos=idx.Next(id); ...}` + factory idx.First(key)
                      // kPartialKeyHashSeek (P7) — NO bound-col re-check (index is FULL-KEY EXACT)
   via_index=false -> `while (pos<member.NumRows()){ id=pos++; if(row.f!=arg)continue; ...}`
                      // kFullScanFilter — full scan + bound-col filter

B. INTERIOR/JOIN path — EmitScan(ProgramTableScanRegion):          // Database.cpp:2778  <-- NOT plan-driven
   RE-DERIVES the arm from index-presence × arity (NO stored plan on the region node):
     keyed_chain = index && |in_vars|==|index.KeyColumns()|  -> for(s=idx.First; ...; s=idx.Next)
     keyed_probe = index && |in_vars|==|fields|              -> if(s=member.Find(row); ...)
     else full scan for(s=0; s<NumRows; ++s) + bound-col re-check belt
   Two mint sites, plan_kind threaded at NEITHER (0/2):
     BuildMaybeScanPartial (Build.h:448) — index conditional; BuildNestedLoopJoin (Join.cpp:254) — index always Some
```

Each `TABLEINDEX` → one emitted `::hyde::rt::Index<Key>` static member (`Database.cpp:497`,
gated on non-empty ValueColumns).

### §1.5 Runtime (M3 backend) — `include/drlojekyll/Runtime/Table.h` (`hyde::rt`, all HASH)

`Table<Row>` (row store + membership predicates), `Index<Key>` (secondary hash, `First/Next`
FULL-KEY EXACT), `StateCellStore` (aggregate/KV per-group state). Differential maintenance:
per-stratum OVERDELETE→REDERIVE→INSERT with split signed counters (C_nr/C_r); commit sweep
publishes `was!=now`; dead-row compaction at the commit-sweep tail. **The fixpoint is fully
materialized BEFORE any `#query` is served — this is why P7's seek over a settled recursive
relation is answer-correct.** There is NO runtime range/trie structure (P8 would add one).

---

## §2 PATH FORWARD AS DIFFS (on the §1 pseudocode)

Three candidate cuts, each a diff on the as-is. Pick one at §4.

### §2-P7b — interior/join plan-driven scans (the smallest, on-theme continuation)

P7 is #query-path ONLY (§1.4-A). The interior/join `EmitScan` (§1.4-B) still re-derives its
arm from index-presence × arity; `plan_kind` is threaded at NEITHER mint. P7b closes the
deferred B-P7/D4 — thread a plan onto the region node and referee emission by it.

```
  // ControlFlow: Program.h ProgramTableScanRegionImpl  ADD:
+ AccessPlan plan_kind{AccessPlan::kUnplanned};   // EXCLUDED from Hash/Equals/MergeEqual (S5' precedent)
+ AccessPlan PlanKind() const;

  // Mint site 1 — BuildMaybeScanPartial (Build.h:448):
-   mint ProgramTableScanRegion(table, out_cols, index?, ...)
+   plan_kind = index ? kPartialKeyHashSeek : kFullScanFilter   // (an interior partial scan)
+   mint ProgramTableScanRegion(..., plan_kind)
  // Mint site 2 — BuildNestedLoopJoin (Join.cpp:254):
+   plan_kind = kUnplanned   // a join pivot is not a #query access; the belt SKIPS it (B-P7)

  // Codegen: EmitScan(ProgramTableScanRegion) head:
+ V-PLAN-HONEST (moved/duplicated here, per-kind IMPLICATION, kUnplanned skip):
+   if region.plan_kind != kUnplanned:
+     kFullScanFilter    ⇒ full-scan arm     kPartialKeyHashSeek ⇒ keyed_chain (First/Next)
```

DISCRIMINATING GATE: an interior `kPartialKeyHashSeek` region that emitted a full scan
ABORTS; join scans (kUnplanned) never trip it; codegen byte-stable (this is a referee, the
emission already matches). SEQUENCING: thread plan_kind (D4) BEFORE moving the belt (D6) or
the corpus aborts. Anchors + the s14–15 design: `keyed-rewrite-p7p9-diffs.md` §1.4/§1.5,
`p7-execution-grounding.md` §2-Q5/Q6. Size: SMALL, compile-time + codegen, structural gates.

### §2-P6.3..P6.6 — runtime evaluation (the largest; toward replacing M3)

Today the compile-time model (P3/P4/P5/P6.1/P6.2) is a pure OBSERVER; the M3 backend does
all real evaluation (§1.5). This cut makes the DORMANT derivation/route half do REAL work.

```
  // The as-is: Program::Build lowers the DataFlow graph; the model's derivation/route ops
  //   (AddDerivation/RouteResults/RootedReachability, RegionInstance.h) are ctest-only.
  // P6.3 fusion:      detect fusable rule chains (a COMPILE-TIME spike first — no runtime)
+ // P6.4 cyclic activation:  RuleActivationEdge goes live across recursive_components
+ // P6.5 joint fixpoint:     one signed-frontier rooted-reachability + semi-naive loop
+ //                          replacing the per-stratum hand-coded fixpoint for a subgraph
+ // P6.6 per-fact DRed:      differential retraction keyed on RegionalFactId
  // => the first place the greenfield backend DIVERGES from M3; likely NEW codegen/runtime.
```

DISCRIMINATING GATE (structural, since answers are M3-correct): a NEW emitted evaluation
path exercised on a directed carrier, cross-checked against the M3 oracle/behavioral goldens
(they must stay byte-identical). MUST be sub-sliced; the low-risk entry is a **compile-time
P6.3 fusion-DETECTION spike** (detect + dump, no runtime). Anchors:
`reconstruction-diffs.md §3-P6.3..P6.6`, memory `demand-cost-model` /
`mobius-differential-dataflow` / `free-termination-paper`. Size: LARGE.

### §2-P8 / §2-P9 — the remaining physical layers (heaviest)

```
  // P8 cross-relation ordered TRIE (Free Join / COLT):
+ //   NEW runtime range/trie structure (none exists — §1.5 all-hash);
+ //   BindingStateId node interning (region-global pool); ordered .Range subtree DFS; EmitJoin
+ //   NB: the intra-relation prefix seek is ALREADY P7 (kPartialKeyHashSeek) — P8 is PURELY
+ //       the cross-relation ordered case (the s15 "P8a dissolved" ruling).
  // P9 access-path INFERENCE:
+ //   infer an ordered path from a DataFlow signal (NOT file-static ControlFlow order — B-P9);
+ //   the P9 lift walks the POST-Optimize-demolished forwarding TUPLEs (a blocking prereq)
```

Anchors: `keyed-rewrite-p7p9-diffs.md §2/§3`, `keyed-rewrite-p7p9-critique.md`. Size: HEAVY
(P8 is a real runtime-structure build, not a select-and-read).

---

## §3 THE FOUR AUTHORITIES vs THE CUTS (what each touches)

| Authority (RegionInstance.h) | Populated by | Drives codegen? | Next-cut relevance |
|---|---|---|---|
| `AccessPlan` (physical) | P4 select + P7 seek | YES (query cursor) | P7b extends it to interior scans |
| `DeclaredAccessPath` (ordered logical) | P5 | no (hint) | P8/P9 (ordered trie / inference) consume it |
| `BindingStateId` (order-free schema) | P5 | no | P8 node interning |
| `recursive_components` / `rules` | P6.1 / P6.2 | no | P6.4/P6.5 (cyclic activation / joint fixpoint) |

---

## §4 THE NEXT-CUT DECISION (the [OWNER STOP])

| Cut | Character | Size | Touches | Entry-risk |
|---|---|---|---|---|
| **P7b** interior/join plan-driven scans | continuation of the AccessPlan authority | SMALL | compile-time + codegen | low (referee; byte-stable) |
| **P6.3–P6.6** runtime evaluation | model does REAL work; first M3 divergence | LARGE | codegen + runtime | med (start w/ a compile-time P6.3 detection spike) |
| **P8 / P9** ordered trie / inference | new runtime structure + inference | HEAVY | runtime + Regional | high (P8 builds a range/trie) |

No pre-ranking — owner's call. P7b is the cleanest small continuation; P6.3 (or its spike)
is the highest value toward the real backend; P8/P9 are the heaviest.

## §5 OPEN QUESTIONS PER CANDIDATE (the grounding loop must settle)

- **P7b:** does any interior scan actually WANT a seek plan today, or is it purely a
  referee move (byte-stable)? which of the 3 BuildMaybeScanPartial callers
  (Stratum.cpp:1033/1215/1333) produce a keyed interior scan? does moving V-PLAN-HONEST to
  EmitScan interact with the join-pivot scans (must SKIP kUnplanned)?
- **P6.3:** is fusion detectable purely at compile time from `rules` + `recursive_components`
  without a runtime? what is the smallest carrier that shows a fusable chain? does a
  detection-only dump move any golden (should be additive, P6.1/P6.2 precedent)?
- **P8:** what is the minimal runtime range/trie ABI (`hyde::rt`)? does it disturb the
  hash-only invariants in §1.5 / the compaction contract? how does `BindingStateId`
  interning become region-global (P5 rebuilds the schema spine)?
```
