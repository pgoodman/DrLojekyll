# Keyed-instance rewrite — whole-program pseudocode + path forward as diffs (session-16 seed)

Session 15 close (2026-08-07). Branch `keyed-instances`, **tip `6d6248a2`** ("Lock the @key
Phase-0 language contract"). Supersedes `session-15-whole-program-seed.md`: **P0 is now COMPLETE
and COMMITTED** (was the last landable-ahead item), so the next actionable step is the destructive
**P1 cut** (owner-gated).

**ANCHOR-VALIDITY (read this first — it saves a re-verification pass).** The two Phase-0 commits
(`46a404d4 → 6d6248a2`) changed **ONLY `lib/Parse/Parser.cpp` + `Query.cpp`** (plus `docs/` and
`tests/OptDiff/rejects/`). `git diff --name-only 46a404d4 HEAD` outside those paths is EMPTY.
Therefore **every anchor the P1/P2–P6/P7–P9 authority docs cite in DataFlow / Rel / ControlFlow /
Regional / CodeGen / Runtime is byte-identical at HEAD** — those docs reference tip `46a404d4` but
need NO re-anchoring below the parser. Only re-verify a `lib/Parse/*` line before trusting it (P0
shifted them). Spot-verified at HEAD this session: `ApplyDemandTransform` Build.cpp:2601, `Optimize`
Build.cpp:2622, `BuildQueryInjectorFromRegistry` ControlFlow/Build.cpp:413, `demand_forcings` :510,
`kSubgraphInstantiate` Rel.cpp:591, `ResolveLiveRecognition` :936, `CollectDemandInteriorDecls`
Planning.cpp:210.

State: production is P0-complete (committed) + otherwise untouched; suite `SUITE: PASS (252 cases)`;
nothing blessed. **Phase 1 is OWNER-GATED and destructive.**

Authority chain: `INDEX.md` → `next-session-prompt.md` (SEMANTIC authority: `@key` is a
relation-local ORDERED access-path declaration, NOT a query adornment / demand opt-in; "Avoid these
false starts" is a checklist) → **this file** (whole-program view) → `keyed-rewrite-reconstruction-
diffs.md` (P1 compile-clean §1 + §6 + P2–P6 §3) + `keyed-rewrite-reconstruction-critique.md` (the
B1–B5 gate) → `keyed-rewrite-p7p9-diffs.md` (P7–P9 §1–§7, s15-amended) + `keyed-rewrite-p7p9-
critique.md` (s14 + s15 re-critique) → `keyed-rewrite-ir-desired-states.md` (§1–§12 predict-then-
verify IR).

---

## §1. The whole program TODAY — pseudocode (what we cut FROM)

### §1.1 Pipeline (bin/drlojekyll/Main.cpp)

```
compile(module, flags):
    query   = Query::Build(module, log, gPassPolicy, gDemand, gDemandRetract)   # Main.cpp:76
    frozen  = FrozenRegionalProgram::Build(query, log)                          # Main.cpp:92  (degenerate planner)
    SetRelDumpStream(gRelStream)                                                # Main.cpp:109 (-rel-out seam)
    program = Program::Build(frozen, log, gFirstId, gPassPolicy, gDemandInstance) # Main.cpp:113
    GenerateDatabaseCode(program, h, cc)                                        # Main.cpp:146
    # -contract-out is a DataFlow-layer dump AFTER Program::Build                # Main.cpp:166 (moves in P9)
```

### §1.2 DataFlow — Query::Build (THE demand authority; the P1 cut target)

```
Query::Build:                                                          # lib/DataFlow/Build.cpp
    build SELECT/TUPLE/JOIN/MERGE/CMP/MAP/NEGATE/AGG/KVINDEX/INSERT graph
    ConnectInsertsToSelects(log, proxy_view_to_decl)                   # Connect.cpp
    impl->ApplyDemandTransform(module, log, demand_mode, demand_retract, proxy_view_to_decl)  # Build.cpp:2601  <== CUT
        activation gate: scan PARSED module for HasInstanceKey() -> pragma_activated
        !demand_mode && !pragma_activated -> return                   # the containment gate (pragma-free = byte-identical)
        bound_queries; per-adornment SIP walk (UniqueRedeclarations); fences   # Demand.cpp
        V-DECLARED-KEY RP-10 bijection: declared set-of-sets == inferred p_bound # Demand.cpp:892-960 (SORTS -> P9 replaces)
        FabricateDemandMessage/Local under `demand__`; mint d_p + guard JOIN (d_p ⋈ p)
        stamp GuardAnnotation (CSE-migrating) + RecognizedSubgraph; register QueryDemandForcing
    impl->Optimize(log, policy)                                        # Build.cpp:2622  <== AFTER the demand transform
        # CSE migrates GuardAnnotation; TUPLE canonicalization DELETES forwarding TUPLEs + single-source MERGEs
        # (VERIFIED s15: demand_tc_witness .df collapses 19 TUPLEs + 1 MERGE nodf->opt — the P9 lift hazard)
    Stratify (iterative Tarjan over VIEW*; neg/agg rejects; V-SCC-SEAM)
    row_contracts = InferConservativeRowContracts (2-phase, PURE)      # RowContract.cpp  (a QueryImpl member)
```

### §1.3 Regional — FrozenRegionalProgram::Build (degenerate planner; the P2 target)

```
Build(query, log):                                                    # lib/Regional/Planning.cpp:439
    out = FrozenRegionalProgram(query)          # stores the Query BY VALUE as the real owner (a census/render shell)
    request ports/internals: 1 per query.DemandForcings()
    input/result ABIs from CollectMessages (filters query.IsDemandMessage)
    row contracts, 3 tiers: R-STORE (friend-leak query.impl->row_contracts) + Tier-1
        CollectDemandInteriorDecls (reads RecognizedSubgraphs) + Tier-2 CollectOriginInteriorDecls (reads OriginDecls)
    out.census = DeriveRegionalCensus(query)     # pure fn of query; V-REGION-CENSUS recount at Rel tail
```

### §1.4 ControlFlow + Rel — Program::Build (flat/nested selector; Rel-IR is the stratum authority)

```
Program::Build(frozen, ..., demand_instance):                        # lib/ControlFlow/Build.cpp:1333
    query = frozen.Query()                                            # H4 unwrap (the byte-preserving thin seam)
    feature-gap pre-pass (agg/kv/map/product-in-scc rejects)
    per-forcing keyed fences from LIVE GuardAnnotations; RP-9 silent pragma->nested
    context.demand_instance_enabled = the selector bit; context.demand_forcings = &query.DemandForcings()
    BuildQueryInjector* from the QueryDemandForcing registry           # Build.cpp:413-527  (B1 consumer)
    # ---- Rel-IR (lib/Rel): the SOLE stratum authority ----
    build typed DRVecs + DROps; DeriveDRStrata; LinearizeAndValidateDRFlow (Kahn)
    IsCutSuccessorDR: demand_instance_enabled && GuardAnnotationIndex() # Rel.cpp:1571 (eager-walk chain-break)
    # nested keyed lowering (gated on demand_instance_enabled):
    ResolveLiveRecognition(query); BuildSubgraphInstanceOps(...)       # Rel.cpp:936,1038
        mint kSubgraphInstantiate/kInstanceDeath/kInstanceSeal; Rederive ACCESS lowering=kSectionWalk  # Rel.cpp:1146
    LowerDRFlow / LowerDRRounds / LowerCommitSweeps                     # Stratum.cpp — emit ControlFlow regions
    LowerSubgraphInstances -> ProgramSubgraphInstanceRegion            # Procedure.cpp:279
```

### §1.5 CodeGen + Runtime — the physical layer (the P7–P9 substrate; verified s15)

```
EmitScan(region : ProgramTableScanRegion):                            # Database.cpp:3334-3423 — ONE fn, 3 arms on Index()
    keyed_chain  (index in index_member, |in|==|KeyColumns|): for(s=idx.First(in);s!=kNoRow;s=idx.Next(s))  # :3381
    keyed_probe  (index present, |in|==|fields|):             if(s=member.Find(in); s!=kNoRow)             # :3387 (all-cols)
    else full-scan+self-filter:                              for(s=0;s<NumRows;++s){ if(⋀ r.field[k]==in[k]) } # :3392
    # LABEL-BLIND: grep -c lowering Database.cpp = 0. cursor = "s"+id (:3346) — the P7 s<id> discriminator.
GetOrCreateIndex(cols): SortAndUnique(cols) -> ORDER-FREE identity (column_spec dedup, kind-BLIND).   # Data.cpp:348-372
Runtime: Table/DiffTable + Index<Key> whole-key hash (Table.h:748-888). ALL HASH, NO TRIE/sorted (grep=0).
InstanceStore<Key,RowT> (deleted at P1): keyed double-buffered leaf cache.
# TWO ProgramTableScanRegion MINT sites (s15 R2): Join.cpp:254 (index=Some) + Build.h:469 BuildMaybeScanPartial
#   (index=Some or index=None crossover full-scans @3 Stratum.cpp sites). Neither sets a plan_kind.
```

**THE IDENTITY GAP:** keyed lowering reduces facts / derivations / requests / forcing-indexes / handles
to `GuardAnnotation`/`RecognizedSubgraph` side-records + raw ints; `FrozenRegionalProgram` OWNS no
regional semantics; the physical layer knows only hash indexes + full scans.

---

## §2. The target — four authorities + two edges + EvaluateEpoch (carried, tight)

```
FOUR AUTHORITIES, never collapsed:
  Logical fact          RegionalFactRelation : RegionalFactId -> RegionalFact  (SINGLE fact authority)
  Residual              BindingState (owns frontiers + FactDerivation ids)      (NOT a 2nd fact owner)
  Logical access path   DeclaredAccessPath (ORDERED fields; [A,B] != [B,A])     (P5 seeds, P9 extends)
  Physical structure    AccessPlan (FullScanFilter | FullKeyHashLookup | ...trie) (P7 introduces, P8 extends)
TWO EDGE RELATIONS, never unified:
  RequestEdge        exact ownership, ACYCLIC forest (RootLease/PermanentRoot/RegionalMember)
  RuleActivationEdge derivation dep, MAY CYCLE in a region; liveness = ROOTED reachability
IDENTITY: BindingStateSchema keyed on the field SET (order-FREE, compile-time); BindingStateId adds sorted
  VALUES (runtime); DeclaredAccessPath ORDERED; [A,B] & [B,A] converge on ONE {A,B} schema.
EvaluateEpoch = least fixpoint of (rooted-reachability ∘ semi-naive derivation), order-independent;
  deletion computes the SAME least fixpoint as fresh-from-committed; drain routed removals BEFORE retiring
  an unreachable SCC; counts are CACHES, never the oracle.
CROSS-CUT: inference (P9) picks NO physical structure; planning (P7) picks NO logical path; answer identity
  holds at every step because FullScanFilter is always a correct realization of any path.
```

Full typed-record target: `next-session-prompt.md` "Target semantic representation" (RegionTemplate /
RelationSchema / BindingState / RequestEdge / RuleActivationEdge / FactDerivation / AccessRequirement /
AccessPlan). Retained `RegionalDataFlowCore.md` invariants: member identity, exact request ownership,
caller-qualified results, effect/epoch invariants, drain-before-retire, single-fact-authority,
counts-are-caches.

---

## §3. The path forward as diffs (whole-program altitude)

### P0 — lock the language contract [✅ COMPLETE, committed 6d6248a2]

```
✅ reject @key on #query; full-context redecl consistency (s11); order-significant paths (P0-item-4, s15):
   both parser dup-reject sort sites flipped — same-decl exact-vector compare + SameKeySetOfSets dropped the
   within-path sort (kept the across-pragma std::set). [A,B] != [B,A] both legal; exact ordered dup rejects.
   reject_key_double_2 guards the multi-column ordered compare. SUITE: PASS (252), no golden moved.
```

### P1 — delete the demand authority [GREEN-LIT + EXECUTING 2026-08-08; DESTRUCTIVE]

> **GREENFIELD RULING (owner, 2026-08-08).** The DrLojekyll compiler is NOT in production use —
> treat the keyed-instance rewrite as greenfield. This is the load-bearing justification for the
> delete-then-rebuild sequencing (P1 destroys before P2–P9 rebuild): with no users, the P1→P4
> regression window (bound queries fall back to full materialization until the typed regional path
> is rebuilt) costs nothing, correctness is preserved (full materialization is always a correct
> realization), and every post-P1 gate is STRUCTURAL not answer-equality. Strangler/toggle REJECTED
> (no working system to keep alive; the demand transform mutates the shared Query graph; a long-lived
> dual-authority period is itself the forbidden false-start). Branch-isolation UNNECESSARY (no
> operational `main` to protect) — land directly on `keyed-instances`. The ONLY residual risk is
> rebuild completion (P2–P9), which is just "is the project going to happen." Discipline: ONE atomic,
> compile-clean, suite-green commit — never half-cut.


The compile-clean inventory + the 5 blocking findings are the gate. Authority: `keyed-rewrite-
reconstruction-diffs.md` §1 (base inventory) + §6 (the 7 re-grep consumers) + §5.1 (B1); B-findings in
`keyed-rewrite-reconstruction-critique.md`.

```
- DELETE: ApplyDemandTransform + Demand.cpp TU; FabricateDemand*/demand__; QueryDemandForcing;
  GuardAnnotation/RecognizedSubgraph/GuardAnnotationIndex; the Rel kSubgraphInstantiate/kInstanceDeath/
  kInstanceSeal nested lowering + ResolveLiveRecognition/BuildSubgraphInstanceOps; the keyed kSectionWalk USE
  at Rel.cpp:1146 (KEEP the enum — join pivots Rel.cpp:2432-2433 need it); InstanceStore.h +
  EmitSubgraphInstance; -demand/-demand-retract/-demand-instance + the flat/nested selector.
- B1 (reconstruction-diffs §5.1): BuildQueryInjectorFromRegistry + registry branch (KEEP ForcingMessage
  fall-through); Context::demand_forcings (Build.h:127); DeriveRegionalCensus DemandForcings()->0 +
  request-port loop + NumForcingsOfName; IsCutSuccessorDR conjunct; IsDemandMessage's 3 callers; the whole
  -contract-out declared-key renderer Format.cpp:1745-1798.
- §6 NINE un-enumerated consumers (VERIFIED at tip — the cut is NOT compile-clean without them):
  §6-1 Rel/Format.cpp:114-116 + :865-903 + :1131-1132 (dump emitter arms + census).
  §6-2 Rel.cpp DROpStratum :4794-4807 + key_of :5363 (instance band-key) -> fold to default 0u.
  §6-3 ControlFlow class-def sibling TUs: Operation.cpp:152-155,507-543 + Program.cpp:233-256,414,752-809 +
    Format.cpp:659-666,979 + Visitor.cpp:38 + the AsSubgraphInstance virtual Program.h:577/:1170.
  §6-4 LIVE CALL Planning.cpp:304-306: CollectOriginInteriorDecls (RETAINED) calls the DELETED
    CollectDemandInteriorDecls — delete the loop.
  §6-5 tests/DataFlowValidators/GuardAnnotationFoldTest.cpp (whole TU + CMake).
  §6-6 tests/RelValidators/{InstanceSolePub,InstanceEffects,InstanceOrder,InputArm,DeathFrontier}Test.cpp.
  §6-7 [s16] bin/drlojekyll/Main.cpp: gDemand/gDemandInstance/gDemandRetract globals :49-51 + the 3 arg-parse
    arms :610/:617-618/:627-628 + drop the demand args at Query::Build :76 and Program::Build :113-114.
  §6-8 [s16] bin/Oracle/Main.cpp:749-753 — THE 8TH CONSUMER, first in a DISJOINT BINARY: drop the 3 demand
    args (demand_mode/demand_retract/suppress_demand, all false/true) → Query::Build(mod, log, DisableDFOpt()).
    BEHAVIOR-PRESERVING: suppress_demand=true only defeated flagless RP-6 @key so the oracle sees the full
    closure — post-cut there IS no transform to suppress. Query::Build reverts to 3-arg (Query.h:1081-1086).
+ DURABLE ACCEPTANCE GATE: a symbol-driven `git grep -l <deleted-symbol>` P1 pre-commit check (each deleted
  symbol returns ONLY {inventory ∪ allowlist} files), run COMMENT-STRIPPED (`| grep -vE ':[0-9]+:\s*(//|\*)'`)
  — the s16 DRY found 7 comment-only stale refs (Prov/Regional.h/Optimize/IdentityJoin/Link/Table.h/Connect)
  that the raw -l gate false-flags; allowlist retained parse-surface (HasInstanceKey/InstanceKeys/ForcingMessage)
  + test-DATA (.dr/.golden). Run it as the gate, NOT a range-anchored read (the recurring miss). The three
  structural blind spots it mechanizes: dump-emitter pair (§6-1 vs L2), header-vs-def TU (§6-3 vs P1.5), and
  now libraried-vs-disjoint-binary (§6-8 Oracle vs every lib consumer).
  Golden fallout: re-bless key_tc_witness.contract + key_multi_adorn_witness.contract to drop declared-key.
  STRONGEST anti-silent-pass belt = the .rel census multiset (kSubgraphInstantiate/kInstanceSeal exact per
  carrier); V-REGION-CENSUS request-ports degenerates to 0==0 at P1.
POST-CUT baseline (one honest semantic, NOT a compatibility mode): @key is inert metadata; bound queries read
  the canonical fully-materialized relations. No consumer may infer key semantics from demand messages /
  guard annotations / query adornments.
```

### P2 — FrozenRegionalProgram becomes the typed semantic owner  [first CONSTRUCTIVE phase after the cut]
### P3 — RequestEdge / RuleActivationEdge / FactDerivation / RoutedResult (acyclic slice)
### P4 — honest complete-path specialization (FullScanFilter, reuse ProgramTableScanRegion)
### P5 — the partial-binding DAG (order-free schema, order-significant edge — P0-item-4 UNBLOCKED it)
### P6 — recursive regional execution (SCC + routing + fusion + joint fixpoint + DRed deletion)

```
  P2–P6 operational diffs + exit gates: reconstruction-diffs §3. The 5 blocking B1–B5 gate the P1 green-light:
  B1 (P1 not compile-clean w/o the injector/census consumers — folded); B2 (permanent roots become
  AddRequestEdge owners or the ~73% no-bound-query corpus publishes nothing); B3 (P4 discriminating gate:
  region-cursor s<id> vs query-cursor pos); B4 (per-FACT DRed over FactDerivation.support keyed on
  RegionalFactId, NOT a per-BindingState C_nr/C_r mirror); B5 (cross-component transitive retraction = ONE
  joint signed-frontier fixpoint). D1 pulls the compile-time BindingStateSchema interner into P4. The two
  no-baseline carriers (ir-desired-states §6 P6 corecursive p@key(K)/q@key(X); §7 P5 convergence) are grounded
  by VERIFIED CURRENT REJECTS.
```

### P7 — physical access planning (AccessPlan its own domain)  [s15-amended, p7p9-diffs §1]
### P8 — the cross-relation ORDERED trie / COLT / Free Join  [s15-amended, p7p9-diffs §2]
### P9 — access-path inference (additive, logical-only, PRE-Optimize)  [s15-amended, p7p9-diffs §3]

```
  P7: AccessPlan a NEW Rel domain (kUnplanned default the V-PLAN-HONEST emission belt SKIPS — TWO legacy scan
    mints; plan_kind EXCLUDED from Hash/Equals, safe because emission label-blind). kFullKeyExactProbe carries
    index=SOME (the dead all-col index -> keyed_probe.Find; NOT nullopt). Discriminator = CURSOR-SHAPE (pos vs
    s<id>), not loop-shape.
  P8: the intra-relation prefix seek is P7 kFullKeyHashLookup (NOT a new structure — "P8a" dissolved). P8 =
    ONLY the cross-relation ORDERED trie: interior nodes interned on BindingStateId (Navigate/intern
    region-global pool), the ordered .Range subtree DFS (distinct from hash First/Next), the kind:{kHash,
    kTriePrefix} DataIndex tag, the EmitJoin Free Join rewrite, kTriePrefixWalk gated OUT of caps until it lands.
  P9: InferAccessPaths runs PRE-Optimize (Build.cpp:2601 slot — the walk's forwarding-TUPLE chain is intact
    there; VERIFIED by the demand_tc_witness .df collapse) and deposits a QueryImpl MEMBER inferred_access_paths
    keyed by (relation_decl_id, PathSourceKey). Order = adornment ordinal (out_to_in stays a keyed .find(); its
    iteration order is canonicalization-DISCLAIMED). V-DECLARED-PATH-PRESERVED replaces V-DECLARED-KEY. Render
    (-contract-out) MOVES DataFlow->Regional, split declared-key/inferred-key. Structural pins only.
```

---

## §4. What session 16 should do

Enough is persisted to resume cold (this seed + the ~3.7k-line authority docs). Ranked:

1. **[OWNER STOP] Decide P1.** Either (a) green-light the destructive cut — then execute it against the
   reconstruction-diffs §1+§6 inventory, driven by the symbol-driven `git grep -l` acceptance gate, re-bless
   the two `.contract` goldens, and land it compile-clean + SUITE: PASS; or (b) stay in grounding and deepen.
2. **If grounding: re-ground the whole program as pseudocode + path-forward-as-diffs** at the current tip
   (this seed is the backbone — keep it current). Weight toward P1 EXECUTION-READINESS (is the §6 inventory
   now exhaustive? run the symbol-driven grep DRY to find any 8th consumer) and P2 (make FrozenRegionalProgram
   the typed owner — the first constructive phase; is reconstruction-diffs §3 P2 operational enough to build?).
3. **Formulate design-goal diffs** on the deepened pseudocode with DISCRIMINATING exit gates (structural /
   census-token / cursor-shape / provenance-survival — answer-equality is a LOST CHECK past P1). Keep the four
   authorities separate.
4. **Critique adversarially** against real code + the retained RegionalDataFlowCore.md invariants + the "Avoid
   these false starts" checklist. VERIFY, don't assert; rank survivors; record refuted as certifications.
5. **Author/extend the desired IR output states** as each phase grounds (ir-desired-states). Predict-then-verify;
   STRUCTURAL pins only.

METHOD: WORKFLOWS (opus for diff-authoring / critique / judgment; sonnet for mechanical census / extraction /
carrier-dumps). Several sequential single-phase workflows beat one mega-workflow; keep the orchestrator thin
(subagents return distilled pseudocode/findings, not file dumps); front-load carrier dumps once into scratch.
Re-verify only `lib/Parse/*` anchors (everything else is byte-identical to the doc baseline 46a404d4 — see the
ANCHOR-VALIDITY note at the top). Do NOT begin the destructive P1 cut without an explicit owner go-ahead.
Confirm via git you touched only docs unless P1 is green-lit. Update memory at close.
```
