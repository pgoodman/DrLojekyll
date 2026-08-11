# Session 26 whole-program seed — POST-P7c (grounded pipeline pseudocode + path forward as diffs)

> Cold-start START-HERE for session 26. Written at the P7c close (branch `keyed-instances`,
> tip `99b91335`, OptDiff SUITE PASS 226, ctest 5/5). Every anchor below was read at tip.
> clangd in this repo is NOISE (no include paths) — trust the real build. This seed is the
> whole-program view; the landed-cut records are `p7c-execution-grounding.md` (P7c) +
> `p7b-execution-grounding.md` (P7b) + `p7-execution-grounding.md` (P7).

---

## §0 STATUS

- **P1–P7 + P7b + P7c LANDED.** The `AccessPlan` authority now drives codegen at THREE
  points and its arc through emission is **COMPLETE for the partial-scan path**:
  - **P4** — plan-select on the `#query` path (withhold the index for `kFullScanFilter`).
  - **P7** — the partial-key hash SEEK: a bound+free `#query` lowers to `Index::First/Next`.
  - **P7b** — interior/join scans carry a `plan_kind`; the EmitScan V-PLAN-HONEST belt
    referees emission by it (codegen byte-unchanged — a compile-time SHADOW).
  - **P7c** — the redundant `TUPLECMP` re-check around each `kPartialKeyHashSeek` body is
    GONE (the first cut where `AccessPlan` PAYS OFF in emission — it MOVED codegen).
- The M3 full-materialization backend still EVALUATES and answers correctly; the frozen
  Regional layer (`lib/Regional`) is a COMPILE-TIME observer + physical-plan selector.
- **Every post-P1 gate is STRUCTURAL, never answer-equality** (the M3 baseline answers
  correctly, so answers can't discriminate a stubbed model — a dump/byte must).
- **The next step is an [OWNER STOP]:** pick the next cut (§4). No cut is pre-ranked —
  the physical-seek + emission arc is done; the remaining cuts (P6.3–P6.6 runtime eval,
  P8/P9 physical layers) differ sharply in character and size.

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
  BuildRequestPorts(query):                                         // Planning.cpp
    per #query redecl (UniqueRedeclarations, seen_variants dedup):
      if HasBoundParam(redecl):
        plan = SelectAccessPlan({relation, has_free, available_bindings, kCompleteRelation})
             //  !has_free            -> kFullKeyHashLookup   (all-bound .Find)
             //  has_free & bindings  -> kPartialKeyHashSeek  (P7 seek)   <-- LANDED
             //  has_free & no bound  -> kFullScanFilter
        R.request_ports += RequestPortRecord{port, redecl, lease, call_site, plan}
        AddRequestEdge(...)                                         // P3 model edge
      else:  R.permanent_roots += PermanentRootRecord{...}          // all-free, no plan
  InternDeclaredPaths + binding-schema DAG (P5): DeclaredAccessPath (ordered) +
    schema_table/binding_edges (order-free); MaterializePrefixChain; V-PREFIX-CHAIN belt
  ComputeRecursiveComponents (P6.1): project the DataFlow multi-view-stratum SCC
    condensation onto frozen relations via OriginDecls → recursive_components (members)
  AssignSymbolicFields + BuildRuleRoutingProjections + PromoteSharedSymbolicField (P6.2):
    clause-source routing → rules (RuleRoutingProjection) + inherited_symbolic_fields
    (union-find FIXPOINT, directional per-head-field, F16 co-occurrence trap)
  Referees: V-FROZEN-NO-OPEN-PORT, V-OWNERSHIP-ACYCLIC, V-REGION-CENSUS (recount ==
    DeriveRegionalCensus), V-PREFIX-CHAIN, V-PLAN-HONEST (at codegen, see §1.3/§1.4)
  → FrozenRegionalProgram{RegionTemplate R, RegionInstanceRelations, query}
```

**The FOUR authorities** (all typed, in `include/drlojekyll/Regional/RegionInstance.h`;
NEVER aliased): `RegionalFactId` (logical fact) · `BindingStateId` (order-free binding
schema, P5) · `DeclaredAccessPath` (ordered logical access path, P5) · **`AccessPlan`**
(physical structure, P4/P7/P7b/P7c — the ONLY one that drives codegen). P6.1
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
  Lower Rel → ControlFlow regions (SERIES/PARALLEL/INDUCTION/LET/TUPLECMP/CHECKMEMBER/COMMITSWEEP/...):
    LowerDRFlow (acyclic seeds/crossovers/product/claim/frontier), LowerDRRounds (per-SCC fixpoint
      round shells), LowerCommitSweeps (commit + Seal), LowerGroupUpdate (R3 aggregates)
    ingest folds lower from DR-IR (LowerIngestFold/LowerIngestLoop); the eager descent
      (BuildEagerRegion..., Build.cpp) is the ONLY remaining hand-coded emission, cross-checked
      by the kEager* marker ops (V-PRED-XCHECK / Site-5 multisets)

    // --- the interior TABLESCAN mint (the P7b/P7c site) ---
    BuildMaybeScanPartial (Build.h:395):                            // the SOLE LIVE TABLESCAN mint
      if all view columns bound: early-return (no scan minted)      // :414  => a minted scan is a STRICT subset
      index = col_indices.empty() ? null : GetOrCreateIndex(...)    // :442
      scan  = mint ProgramTableScanRegion(table, index?)
      scan->plan_kind = index ? kPartialKeyHashSeek : kFullScanFilter   // :462  (P7b: stamped here)
      cmp   = TUPLECMP(kEqual); scan->body = cmp                    // :482  the body's parent
      per table column: bind out_var; col_id_to_var[col] = out_var  //       (recordization anchor)
        if indexed(col): scan->in_cols += col                       //       // P7c: NO cmp equality pair
        else:            scan->out_cols += col                      //       (index is FULL-KEY EXACT)
      cmp->body = cb(cmp, true)                                     //       nest the real body under cmp
      // POST-P7c: for a seek the cmp is VACUOUS (empty lhs/rhs) — a trivially-equal compare.
    BuildNestedLoopJoin (Join.cpp:192): STATICALLY DEAD — its TABLESCAN mint defaults
      plan_kind=kUnplanned; sole caller is the `else` of `else if (true || …)` (:750/:783).

  Optimize (unless -disable-controlflow-opt): region flatten / no-op removal / proc dedup
    OptimizeImpl(TUPLECMP) (Optimize.cpp): a vacuous kEqual cmp (0 pairs) → REPLACE with body
    // => in opt/nodf the P7c seek cmp DISAPPEARS; in nocf/none it survives empty (codegen emits no gate)
  → ProgramImpl

BuildQueryEntryPointImpl (Build.cpp:413):                          // the codegen-facing #query read
  plan = frozen ? frozen->PlanFor(decl) : nullopt                  // :447  reads AccessPlan
  withhold_index = (plan == kFullScanFilter)                       // :448
  if !withhold_index && !col_indices.empty():                      // :451
    scanned_index = GetOrCreateIndex(model.table, col_indices)     //   provisions bound-subset index
  V-PLAN-HONEST belt (per-kind implication):                       // :457  kFullScanFilter⇒¬idx,
    kPartialKeyHashSeek⇒idx  (fprintf+abort)                       //        the #query-entry belt
  impl.queries += ProgramQuery{query, table, scanned_index, ...}   //        scanned_index → codegen
```

### §1.4 C++ codegen — `GenerateDatabaseCode(program, ...)` (`lib/CodeGen/CPlusPlus/Database.cpp`)

Sealed `struct Database` (state) + hidden-friend ADL API. Two scan surfaces:

```
A. QUERY path — EmitQueryFriends(ProgramQuery spec):               // Database.cpp:1527
   via_index = spec.index && index_member.contains(spec.index.Id())  // :1646  the plan signal
   all-bound       -> .Find existence                                // kFullKeyHashLookup
   via_index=true  -> factory idx.First(key) + `while(pos!=kNoRow){ id=pos; pos=idx.Next(id); ...}`
                      // kPartialKeyHashSeek (P7) — NO bound-col re-check (index is FULL-KEY EXACT)
   via_index=false -> `while(pos<member.NumRows()){ id=pos++; if(row.f!=arg)continue; ...}`
                      // kFullScanFilter — full scan + bound-col filter

B. INTERIOR/JOIN path — EmitScan(ProgramTableScanRegion):          // Database.cpp:2778  (P7b: plan-driven)
   keyed_chain = index && |in_vars|==|index.KeyColumns()|          // the seek arm
   keyed_probe = index && !keyed_chain && |in_vars|==|fields|      // (unreachable on the live path)
   V-PLAN-HONEST belt (:2826, per-kind IMPLICATION, SKIP kUnplanned):
     kFullScanFilter ⇒ ¬keyed_chain∧¬keyed_probe ;  kPartialKeyHashSeek ⇒ keyed_chain
     // READS the arm booleans, NEVER drives dispatch — plan_kind is a compile-time SHADOW
   keyed_chain -> for(s=idx.First(key); s!=kNoRow; s=idx.Next(s)){ bind out_vars; BODY }
                  // POST-P7c: the body's TUPLECMP is VACUOUS -> EmitCompare (:2914) emits NO gate
   keyed_probe -> if(s=member.Find(row); s!=kNoRow){ bind; BODY }
   else full scan for(s=0;s<NumRows;++s){ if(bound-col re-check :2868){ bind; BODY } }
                  // the full-scan arm KEEPS its own re-check (gated !keyed_chain); P7c did NOT touch it
```

Each `TABLEINDEX` → one emitted `::hyde::rt::Index<Key>` static member (`Database.cpp:497`,
gated on non-empty ValueColumns).

### §1.5 Runtime (M3 backend) — `include/drlojekyll/Runtime/Table.h` (`hyde::rt`, all HASH)

`Table<Row>` (row store + membership predicates), `Index<Key>` (secondary hash, `First/Next`
**FULL-KEY EXACT** — `Table.h:789-824`, the contract P7/P7b/P7c all depend on),
`StateCellStore` (aggregate/KV per-group state). Differential maintenance: per-stratum
OVERDELETE→REDERIVE→INSERT with split signed counters (C_nr/C_r); commit sweep publishes
`was!=now`; dead-row compaction at the commit-sweep tail. **The fixpoint is fully
materialized BEFORE any `#query` is served — this is why the seek over a settled recursive
relation is answer-correct.** There is NO runtime range/trie structure (P8 would add one).

---

## §2 PATH FORWARD AS DIFFS (on the §1 pseudocode)

Two candidate cuts remain (P7/P7b/P7c closed the physical-seek + emission arc). Pick one at §4.

### §2-P6.3..P6.6 — runtime evaluation (the largest; toward replacing M3)

Today the compile-time model (P3/P4/P5/P6.1/P6.2) is a pure OBSERVER; the M3 backend does
all real evaluation (§1.5). This cut makes the DORMANT derivation/route half do REAL work.

```
  // The as-is: Program::Build lowers the DataFlow graph (§1.3); the model's derivation/route
  //   ops (AddDerivation/RouteResults/RootedReachability, RegionInstance.h) are ctest-only.
+ // P6.3 fusion:      detect fusable rule chains from `rules` + `recursive_components`
+ //                   (a COMPILE-TIME spike FIRST — detect + dump, no runtime; P6.1/P6.2
+ //                   gated-block precedent => should move NO golden)
+ // P6.4 cyclic activation:  RuleActivationEdge goes live across recursive_components
+ // P6.5 joint fixpoint:     one signed-frontier rooted-reachability + semi-naive loop
+ //                          replacing the per-stratum hand-coded fixpoint for a subgraph
+ // P6.6 per-fact DRed:      differential retraction keyed on RegionalFactId
  // => the first place the greenfield backend DIVERGES from M3; likely NEW codegen/runtime.
```

DISCRIMINATING GATE (structural, since answers are M3-correct): a NEW emitted evaluation
path exercised on a directed carrier, cross-checked against the M3 oracle/behavioral goldens
(they must stay byte-identical). MUST be sub-sliced; the low-risk entry is the **compile-time
P6.3 fusion-DETECTION spike**. Anchors: `reconstruction-diffs.md §3-P6.3..P6.6`, memory
`demand-cost-model` / `mobius-differential-dataflow` / `free-termination-paper`. Size: LARGE.

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

### §2-small — on-theme minor cuts

- The header-token E-71 mini-diff (`deltarel`→`rel` in the `.rel` dump header).
- NOTE: with P7c landed there is **no known remaining redundant re-check** on the scan path
  (the full-scan arm's re-check at `Database.cpp:2868` is REAL — a full scan standing in for a
  partial key genuinely must filter). Do not invent a P7d re-check retire.

---

## §3 THE FOUR AUTHORITIES vs THE CUTS (what each touches)

| Authority (RegionInstance.h) | Populated by | Drives codegen? | Next-cut relevance |
|---|---|---|---|
| `AccessPlan` (physical) | P4 select + P7 seek + P7b interior plan + P7c re-check retire | YES (query cursor + interior scan) | arc COMPLETE for the partial-scan path |
| `DeclaredAccessPath` (ordered logical) | P5 | no (hint) | P8/P9 (ordered trie / inference) consume it |
| `BindingStateId` (order-free schema) | P5 | no | P8 node interning |
| `recursive_components` / `rules` | P6.1 / P6.2 | no | P6.4/P6.5 (cyclic activation / joint fixpoint) |

---

## §4 THE NEXT-CUT DECISION (the [OWNER STOP])

| Cut | Character | Size | Touches | Entry-risk |
|---|---|---|---|---|
| **P6.3–P6.6** runtime evaluation | model does REAL work; first M3 divergence | LARGE | codegen + runtime | med (start w/ a compile-time P6.3 detection spike) |
| **P8 / P9** ordered trie / inference | new runtime structure + inference | HEAVY | runtime + Regional | high (P8 builds a range/trie) |
| **small** E-71 header token, etc. | dump hygiene | TINY | dump only | trivial |

No pre-ranking — owner's call. P6.3 (or its compile-time detection spike) is the highest
value toward the real backend; P8/P9 are the heaviest; the small cuts are dump hygiene.

## §5 OPEN QUESTIONS PER CANDIDATE (the grounding loop must settle)

- **P6.3:** is fusion detectable purely at compile time from `rules` + `recursive_components`
  without a runtime? what is the smallest carrier that shows a fusable chain? does a
  detection-only dump move any golden (should be additive, P6.1/P6.2 precedent)? what is the
  boundary with P6.4 (activation) so the spike stays detect-only?
- **P8:** what is the minimal runtime range/trie ABI (`hyde::rt`)? does it disturb the
  hash-only invariants in §1.5 / the compaction contract? how does `BindingStateId`
  interning become region-global (P5 rebuilds the schema spine)? which carriers have a
  cross-relation ordered join that a trie would actually accelerate?
- **P9:** which DataFlow signal survives Optimize to source the ordered path (the forwarding
  TUPLEs are demolished — the blocking prereq)? how does the render SPLIT/MOVE from DataFlow
  to Regional without moving a `.df` golden?
- **all:** since the M3 baseline answers correctly, the gate is STRUCTURAL — what dump/byte
  discriminates a stubbed model from a real one for this cut? (answers can't.)
```
