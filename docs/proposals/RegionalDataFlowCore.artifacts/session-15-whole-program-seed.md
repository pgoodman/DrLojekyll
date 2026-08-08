# Keyed-instance rewrite — consolidated whole-program pseudocode + path forward as diffs (post-s14 seed)

Session 14 close (2026-08-07, branch `keyed-instances`, tip `46a404d4` + the uncommitted s11
Phase-0 worktree). The SINGLE grounded whole-program view for session 15. It supersedes
`session-14-whole-program-seed.md` on two axes: (1) **§1 now includes the PHYSICAL layer**
(ControlFlow lowering → CodeGen `EmitScan` → `DataIndex`/`GetOrCreateIndex` → runtime
`Table`/`Index`) that s14 grounded and that P7–P9 build on — the s13 seed §1 stopped at the
Rel/InstanceStore layer; (2) **§3 folds the s14 corrections into the actual diffs** (the 7 new
P1 consumers from reconstruction-diffs §6; the 3 P7–P9 blocking corrections from
p7p9-critique). Everything anchor below re-verified at tip this session unless marked "carried".

State: docs only; **suite 251 PASS by construction — production untouched; nothing blessed.**
Phase 1 is OWNER-GATED and destructive.

**s15 CLOSE (2026-08-07):** the three P7–P9 blocking corrections + the two least-grounded residues are now
FOLDED into `keyed-rewrite-p7p9-diffs.md §1–§3` as primary text (no longer §7 amendments), and re-critiqued.
§1 anchors re-verified at tip (EmitScan 3334-3423, `cursor="s"+id`@3346, `#query` cursor `pos`@1750 — HOLD;
only minor coordinate refinements). Three s15 outcomes to carry:
- **B-P7's "sole mint site" was WRONG** — there are TWO `ProgramTableScanRegion` mints (Join.cpp:254 index=Some
  AND Build.h:469 `BuildMaybeScanPartial`, one arm index=None). The `kUnplanned` sentinel skip is confirmed right.
- **B-P9 SETTLED → Arm A (pre-Optimize inference), EMPIRICALLY** — the `demand_tc_witness` `.df` collapses 19
  forwarding TUPLEs + 1 single-source MERGE under Optimize, forcing the walk to the Build.cpp:2601 slot + a
  `QueryImpl` member `inferred_access_paths` (NOT the destroyed `proxy_view_to_decl`); order = adornment ordinal.
- **The `.Range` residue DISSOLVED "P8a"** — the intra-relation prefix seek is P7 `kFullKeyHashLookup` (a
  bound-subset hash index + keyed_chain), not a new structure. P8 is PURELY the cross-relation ORDERED trie
  (Free Join / COLT), where `BindingStateId` node interning + the ordered `.Range` DFS + `EmitJoin` live.
Re-critique record: `keyed-rewrite-p7p9-critique.md` "Session-15 re-critique" (1 blocking + 8 survivors, all
folded; the blocking + 3 mediums dissolved by the P8a→P7 fold). IR states: `keyed-rewrite-ir-desired-states.md`
§9–§12 (s15 corrections in §12 items 13–15).

Authority chain: `INDEX.md` → `next-session-prompt.md` (semantic authority: `@key` is a
relation-local ORDERED access-path declaration, NOT a query adornment) → **this file**
(whole-program view) → `keyed-rewrite-reconstruction-diffs.md` (P1 compile-clean §1 + §6 +
P2–P6 §3) → `keyed-rewrite-p7p9-diffs.md` (P7–P9 §1–§7) → the two critiques
(`keyed-rewrite-reconstruction-critique.md`, `keyed-rewrite-p7p9-critique.md`) →
`keyed-rewrite-ir-desired-states.md` (§1–§12 predict-then-verify IR).

---

## §1. The whole program TODAY — pseudocode (what we cut FROM)

### §1.1 The pipeline (bin/drlojekyll/Main.cpp — verified at tip)

```
compile(module, flags):
    query   = Query::Build(module, log, gPassPolicy, gDemand, gDemandRetract)   # Main.cpp:76
    frozen  = FrozenRegionalProgram::Build(query, log)                          # Main.cpp:92  (degenerate)
    SetRelDumpStream(gRelStream)                                                # Main.cpp:109 (the -rel-out seam)
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
        !demand_mode && !pragma_activated -> return
        bound_queries = [q : IsQuery && any param kBound]              # Demand.cpp:459-471
        per-adornment SIP walk (UniqueRedeclarations); fences          # Demand.cpp:531-838
        V-DECLARED-KEY RP-10 bijection: declared set-of-sets == inferred  # Demand.cpp:892-960 (canon SORTS)
        FabricateDemandMessage/Local under `demand__`; mint d_p + guard JOIN (d_p ⋈ p)
        stamp GuardAnnotation (CSE-migrating) + RecognizedSubgraph; register QueryDemandForcing
    impl->Optimize(log, policy)                                        # Build.cpp:2622  <== AFTER the demand transform!
        # CSE migrates GuardAnnotation via View/Join/IdentityJoin/Link.cpp; TUPLE canonicalization
        # (Tuple.cpp:60-125) DELETES forwarding TUPLEs + single-source MERGEs  (the P9 lift hazard)
    Stratify (iterative Tarjan over VIEW*; neg/agg rejects; V-SCC-SEAM)
    row_contracts = InferConservativeRowContracts (2-phase, PURE)      # RowContract.cpp
```

### §1.3 Regional — FrozenRegionalProgram::Build (degenerate planner; carried)

```
Build(query, log):                                                    # lib/Regional/Planning.cpp:439
    out = FrozenRegionalProgram(query)          # stores the Query BY VALUE as the real owner
    request ports/internals: 1 per query.DemandForcings()
    input/result ABIs from CollectMessages (filters query.IsDemandMessage)
    row contracts, 3 tiers: R-STORE (friend leak query.impl->row_contracts)
        + Tier-1 CollectDemandInteriorDecls (reads RecognizedSubgraphs)
        + Tier-2 CollectOriginInteriorDecls (reads OriginDecls; CALLS CollectDemandInteriorDecls @:304 — §6-4)
    out.census = DeriveRegionalCensus(query)     # pure fn of query; V-REGION-CENSUS recount at Rel tail
```

### §1.4 ControlFlow + Rel — Program::Build (flat/nested selector; the Rel-IR is the stratum authority)

```
Program::Build(frozen, ..., demand_instance):                        # lib/ControlFlow/Build.cpp:1333
    query = frozen.Query()                                            # H4 unwrap
    feature-gap pre-pass (agg/kv/map/product-in-scc rejects)
    per-forcing keyed fences from LIVE GuardAnnotations
    RP-9 silent pragma->nested; context.demand_instance_enabled       # THE selector bit
    context.demand_forcings = &query.DemandForcings()
    BuildQueryInjector* from the QueryDemandForcing registry          # Build.cpp:413-527
    # ---- Rel-IR (lib/Rel): the SOLE stratum authority ----
    build typed DRVecs + DROps; DeriveDRStrata; LinearizeAndValidateDRFlow (Kahn)
    IsCutSuccessorDR: demand_instance_enabled && GuardAnnotationIndex() # Rel.cpp:1571 (eager-walk chain-break)
    # nested keyed lowering (gated on demand_instance_enabled):
    ResolveLiveRecognition(query); BuildSubgraphInstanceOps(...)      # Rel.cpp:936,1038
        mint kSubgraphInstantiate/kInstanceDeath/kInstanceSeal; Rederive ACCESS lowering=kSectionWalk # Rel.cpp:1146
    LowerDRFlow / LowerDRRounds / LowerCommitSweeps                    # Stratum.cpp — emit ControlFlow regions
    LowerSubgraphInstances -> ProgramSubgraphInstanceRegion           # Procedure.cpp:279
```

### §1.5 CodeGen + Runtime — THE PHYSICAL LAYER (new in this seed; verified at tip)

This is the layer P7–P9 build on. It was absent from the s13 seed §1.

```
# ---- lib/CodeGen/CPlusPlus/Database.cpp: region dispatch (THREE walk chains) ----
GenerateDatabaseCode: WalkRegion (:551-650) + CollectEffects (:678-991) + EmitRegion (:1868-1985)
EmitRegion(region):                                                   # Database.cpp:1868
    ... else if region.IsTableScan(): EmitScan(From(region))          # :1942-1943  (the P4/P7 seam)
    ... else if region.IsSubgraphInstance(): EmitSubgraphInstance(...) # :1930-1931 (deleted at P1)

# ---- THE scan dispatch — ONE function, THREE arms, keyed on region.Index() (verified :3334-3426) ----
EmitScan(region : ProgramTableScanRegion):
    maybe_index = region.Index()                    # std::optional<DataIndex>
    input_vars  = region.InputVariables();  out_vars = region.OutputVariables()  # one VAR per column
    if maybe_index && index_member.contains(idx.Id()) && |input_vars|==|idx.KeyColumns()|:   # keyed_chain
        for (s = idx.First(RowExpr(input_vars)); s != kNoRow; s = idx.Next(s)) { r=RowAt(s); bind; body }
    elif maybe_index && |input_vars|==|fields|:                                               # keyed_probe
        if (s = member.Find(RowExpr(input_vars)); s != kNoRow) { r=RowAt(s); bind; body }
    else:                                                                                     # full-scan+filter
        for (s = 0; s < member.NumRows(); ++s) { r=RowAt(s);
            if (!input_vars.empty()) { assert(|IndexedColumns()|==|input_vars|);              # :3405
                if (⋀ r.field[IndexedColumns[k].Index()] == input_vars[k]) { bind; body } }
            else { bind; body } }
    # index=nullopt  => the else arm = the honest "FullScanFilter"; index=Some => keyed_chain/keyed_probe.
    # LABEL-BLIND: no Rel Lowering / AccessPlan token is read here (grep -c lowering Database.cpp = 0).

# ---- the #query cursor (the P1/P4/P7 baseline comparand; Database.cpp:1735-1829) ----
<name>_<bf>_cursor : a STRUCT keyed on `pos`/`id`; via_index arm emits idx.First/.Next when an index
    exists, else a NumRows scan + `if (row.<field> != <param>) continue;`  # keys on pos, NOT a region s<id>

# ---- DataModel / DataTable / DataIndex (the index substrate; verified) ----
FillDataModel: per view, model->table = tables.Create(); ALWAYS GetOrCreateIndex(all columns)  # Data.cpp:205
    # the all-column index is EXCLUDED from index_member (ValueColumns().empty(), Database.cpp:501) —
    # the table's built-in whole-row Find IS the unique index; the all-col DataIndex is structurally dead.
GetOrCreateIndex(impl, cols):                                         # Data.cpp:348-372 — SOLE index-mint path
    SortAndUnique(cols)                                               # :350  => index identity is ORDER-FREE
    dedup by column_spec string (e.g. "0:1")                          # :357  => [A,B] and [B,A] alias today
    # callers: FillDataModel (:205, all-col), Join pivots (Join.cpp:264/400), the #query cursor selector
DataModel : DisjointSet { TABLE *table }  # view_to_model[view]->FindAs<DataModel>()->table  (Program.h:152)
ProgramTableScanRegion (Program.h:1097-1140; Impl lib/ControlFlow/Program.h:1601):
    Table(), optional<DataIndex> Index(), Body(), IndexedColumns(), InputVariables(), OutputVariables()
    # sole ctor site today: Join.cpp:254-297 (index=Some over pivot cols)

# ---- Runtime (include/drlojekyll/Runtime/): ALL HASH, NO TRIE (grep-confirmed zero) ----
Table<Row>/DiffTable<Row> over RowStore<Row>: NumRows(), RowAt(id), Find(row)=whole-key hash # Table.h:53-737
Index<Key> (Table.h:748-888): open-addressing hash; First(key)/Next(id) FULL-KEY EXACT (Table.h:791-803)
    # no .Range / partial-prefix seek; the whole-key probe IS the equality authority (no per-row re-check)
InstanceStore<Key,RowT> (deleted at P1): keyed leaf cache, dense iid per bound key, double-buffered
    # capability lost at P1 = keyed localization; P8 trie re-provides prefix NAVIGATION over the canonical
    # DiffTable (no double-buffer: kInI/InNew already carry frozen/current, Table.h:385-395)

# THE IDENTITY GAP (unchanged): keyed lowering reduces facts/derivations/requests/forcing-indexes/handles
# to GuardAnnotation/RecognizedSubgraph side-records + raw ints; FrozenRegionalProgram OWNS no regional
# semantics; the physical layer knows only hash indexes + full scans (no ordered/trie structure).
```

---

## §2. The target — four authorities + two edges + EvaluateEpoch (carried, tight)

```
FOUR AUTHORITIES, never collapsed:
  Logical fact          RegionalFactRelation : RegionalFactId -> RegionalFact  (SINGLE fact authority)
  Residual              BindingState (owns frontiers + FactDerivation ids)      (NOT a 2nd fact owner)
  Logical access path   DeclaredAccessPath (ORDERED fields; [A,B] != [B,A])     (P5 seeds, P9 extends)
  Physical structure    AccessPlan (FullScanFilter | FullKeyHashLookup | trie)  (P7 introduces, P8 extends)
TWO EDGE RELATIONS, never unified:
  RequestEdge        exact ownership, ACYCLIC forest (RootLease/PermanentRoot/RegionalMember)
  RuleActivationEdge derivation dep, MAY CYCLE in a region; liveness = ROOTED reachability
IDENTITY: BindingStateSchema keyed on the field SET (order-FREE, compile-time); BindingStateId adds sorted
  VALUES (runtime); DeclaredAccessPath ORDERED; [A,B]&[B,A] converge on ONE {A,B} schema.
EvaluateEpoch = least fixpoint of (rooted-reachability ∘ semi-naive derivation), order-independent;
  deletion computes the SAME least fixpoint as fresh-from-committed; drain routed removals BEFORE retiring
  an unreachable SCC; counts are CACHES, never the oracle.
CROSS-CUT (s14): inference (P9) picks NO physical structure; planning (P7) picks NO logical path;
  answer identity holds at every step because FullScanFilter is always a correct realization of any path.
```

---

## §3. The path forward as diffs (whole-program altitude, s14 corrections folded)

`⊗` = an s14 correction now folded into the diff itself (not merely an amendment).

### P0 — lock the language contract [LANDED — all items complete]

```
  DONE (uncommitted s11): reject @key on #query; full-context redecl consistency.
✅ item 4 (order-significant paths) — LANDED s15 (2026-08-07, owner-gated go-ahead). Flipped BOTH parser
  dup-reject sort sites to order-significant: same-decl (Parser.cpp ~974, removed the two std::sort ->
  exact ordered-vector compare) AND cross-redecl SameKeySetOfSets (~1477, dropped the within-set sort,
  kept the std::set across-pragma order-freedom). Predict-then-verify CONFIRMED: `@key(A,B) @key(B,A)`
  now passes the parser dup-check (distinct paths — rejects only DOWNSTREAM at demand, an unbuilt-P5
  concern); `@key(A,B) @key(A,B)` still rejects (exact ordered dup, new message); reordered pragmas across
  redecls stay one contract (test 7); a redecl declaring a DIFFERENT ordered path draws "differs". Within-
  @key dup-column check (@key(A,A)) unchanged (separate order-free obligation). reject_key_double_1 comment
  updated (its @key(A)@key(A) is exact-dup either way); NEW reject_key_double_2 (@key(A,B)@key(A,B))
  guards the multi-column ordered compare. No golden moved (no corpus case used order-permuted keys).
```

### P1 — delete the demand authority [DESTRUCTIVE, owner-gated]

```
- DELETE: ApplyDemandTransform + Demand.cpp TU; FabricateDemand*/demand__; QueryDemandForcing;
  GuardAnnotation/RecognizedSubgraph/GuardAnnotationIndex; the Rel kSubgraphInstantiate/kInstanceDeath/
  kInstanceSeal nested lowering + ResolveLiveRecognition/BuildSubgraphInstanceOps; the keyed kSectionWalk
  USE at Rel.cpp:1146 (KEEP the enum — join pivots at Rel.cpp:2432-2433 need it); InstanceStore.h +
  EmitSubgraphInstance; -demand/-demand-retract/-demand-instance + the flat/nested selector.
  B1 additions (reconstruction-diffs §5.1): BuildQueryInjectorFromRegistry + registry branch (KEEP the
    ForcingMessage fall-through); Context::demand_forcings (Build.h:127); DeriveRegionalCensus's
    DemandForcings() (-> literal 0) + request-port loop + NumForcingsOfName; IsCutSuccessorDR conjunct;
    IsDemandMessage's 3 callers; the whole -contract-out declared-key renderer Format.cpp:1745-1798.
⊗ §6 additions (s14 re-grep — the cut is STILL NOT compile-clean without these SEVEN; all verified at tip):
    §6-1 the Rel-IR DUMP EMITTER Rel/Format.cpp:114-116 (DROpKindName) + :865-903 (render arms) +
      :1131-1132 (census array) — L2 caught only the DataFlow renderer twin, NOT this one.
    §6-2 Rel.cpp DROpStratum :4794-4807 (instance_stratum/store_id case arms) + key_of :5363 (kInstanceSeal
      band-11 tie-break) — outside every P1.4 range; fold to the default 0u.
    §6-3 the ProgramSubgraphInstanceRegion/ProgramInstanceStore DEFINITION TUs: ControlFlow/Operation.cpp
      :152-155,507-543 + Program.cpp:233-256,414,752-809 + Format.cpp:659-666,979 + Visitor.cpp:38 +
      the AsSubgraphInstance virtual Program.h:577/:1170 (P1.5 enumerated only header+Database.cpp+Build/*).
    §6-4 the LIVE CALL Planning.cpp:304-306: CollectOriginInteriorDecls (RETAINED) calls the DELETED
      CollectDemandInteriorDecls — delete the loop (post-cut no Tier-1 decls exist).
    §6-5 tests/DataFlowValidators/GuardAnnotationFoldTest.cpp (whole TU + CMake).
    §6-6 tests/RelValidators/{InstanceSolePub,InstanceEffects,InstanceOrder,InputArm,DeathFrontier}Test.cpp.
⊗ DURABLE ACCEPTANCE GATE: a symbol-driven `git grep -l <deleted-symbol>` P1 pre-commit check (each deleted
  symbol returns ONLY inventory files) — the range-anchored review keeps missing sibling TUs. Run it as the
  gate, not a read.
  Golden fallout: re-bless key_tc_witness.contract + key_multi_adorn_witness.contract to drop declared-key.
  STRONGEST anti-silent-pass belt = the .rel census multiset (kSubgraphInstantiate/kInstanceSeal/kIngestFold
  exact per carrier); the V-REGION-CENSUS request-ports check degenerates to 0==0 at P1 (§1.2).
```

### P2 — FrozenRegionalProgram becomes the typed semantic owner
### P3 — RequestEdge/RuleActivationEdge/FactDerivation/RoutedResult (acyclic slice)
### P4 — honest complete-path specialization (FullScanFilter, reuse ProgramTableScanRegion)
### P5 — the partial-binding DAG (order-free schema, order-significant edge)
### P6 — recursive regional execution (SCC + routing + fusion + joint fixpoint + DRed deletion)

```
  P2–P6 operational diffs + exit gates are in reconstruction-diffs §3; the 5 blocking B1–B5 + the s14
  §6.1 consistency certification gate the P1 green-light. UNCHANGED by s14 except: B2 permanent-roots as
  request owners; B3 discriminating P4 gate (region-cursor s<id> vs query-cursor pos); B4 per-FACT DRed
  over FactDerivation.support keyed on RegionalFactId (NOT a per-BindingState C_nr/C_r mirror); B5
  cross-component transitive retraction as ONE joint signed-frontier fixpoint. D1 pulls the compile-time
  BindingStateSchema interner (not the DAG) into P4.
```

### P7 — physical access planning (introduce the AccessPlan domain)

```
+ AccessPlan is its OWN Rel domain, DISJOINT from the join Lowering enum (goal 1). CodegenPlanCapabilities
  = a compile-time SET literally enumerating the EmitScan arms that exist (kFullScanFilter ALWAYS in;
  kFullKeyHashLookup in; kTriePrefixWalk OUT until P8). SelectAccessPlan reuses GetOrCreateIndex;
  LowerAccessRequirement mints a ProgramTableScanRegion (reuse, no new codegen arm; MUST populate out_vars
  one-per-column or bind_outputs emits nothing).
⊗ B-P7 (blocking): the V-PLAN-HONEST belt CANNOT sit unconditionally at EmitScan's head — the pre-existing
  join-pivot mint (Join.cpp:254-266, index=Some) never sets plan_kind, so the bijection assert aborts every
  join. FIX: add a `kUnplanned` sentinel default the belt SKIPS (keeps P7 a pure addition), OR thread
  plan_kind at EVERY ProgramTableScanRegion mint. plan_kind is EXCLUDED from Hash/Equals (mint_tag precedent).
⊗ B-P7 corollary: the D4 map STOPS being injective at P7 (both kFullScanFilter=index=nullopt and
  kFullKeyHashLookup=index=Some lower to ProgramTableScanRegion) — the D4 Option-1 -> Option-2 transition:
  V-PLAN-HONEST MOVES to the EmitScan emission site (branch on plan_kind), NEVER a DR-tail belt.
⊗ H-P7: bound_field_positions is an order-FREE SET; EmitScan's keyed_chain trusts input_vars in KeyColumns
  (ascending) order — §1.4 MUST sort bound_cols ascending or First() is mis-keyed (empty results).
⊗ M-P7: the query-path discriminator is CURSOR-SHAPE (region s<id> vs <name>_cursor pos/id), NOT loop-shape
  — an index-bearing relation's baseline #query cursor already emits First/Next (verified in the neighborhood
  generated header). Loop-shape survives only for the interior/rule-body path.
```

### P8 — lazy shared tries / COLT / Free Join (~100% GREENFIELD)

```
+ LazyPathTrie NAVIGATES the canonical DiffTable (never owns rows); whole-key First/Next REUSES Index
  verbatim; .Range partial-prefix seek is the ONE new codegen surface (an EmitAccessPlan branch);
  GetOrCreateIndex (Data.cpp:350 SortAndUnique) is the order-free hook -> add an ordered GetOrCreateOrderedIndex.
⊗ B-P8 (blocking): the trie as first-drafted is VALUE-keyed (edges on prefix_values_hash) so [A,B]/[B,A]
  reach TWO leaves — no convergence. FIX: intern trie nodes in a region-global pool keyed by BindingStateId
  = (schema, sorted value-map): value-partitioned (so TrieFirst returns ONE value's rows, honoring full-key-
  exact) AND convergent (sort-equal value maps share ONE node). F11 "node keys on SCHEMA" is a P5
  compile-lattice rule, NOT the P8 runtime-node rule.
⊗ H-P8: GetOrCreateIndex dedup is kind-BLIND (column_spec only) — an ordered "0:1" trie index aliases a join
  hash "0:1". FIX: add a `kind:{kHash,kTriePrefix}` field to DataIndexImpl; dedup on (column_spec, kind) in BOTH minters.
⊗ M-P8: FreeJoinSpine has NO codegen consumer (joins emit nested First/Next); add the EmitJoin lock-step
  rewrite to the residue + gate BuildLazyOrdering behind a caps handshake (else label!=emission).
⊗ M-P8: the "3 not 4 nodes" pin is runtime-unstable (mint-on-visit); SCOPE it to the eager
  MaterializePrefixChain schema spine (compile-time), fold a declared-vs-visited origin tag, observe via the
  P5 -region-out binding-schema block (the one-to-one TrieNode<->BindingStateSchema makes it reusable).
```

### P9 — access-path inference (additive, logical-only)

```
+ InferAccessPaths(region) collects logical paths from {query adornments, join pivots, contextual bindings},
  UNIONED with declared paths (declared authoritative); V-DECLARED-PATH-PRESERVED replaces the deleted
  V-DECLARED-KEY bijection (Demand.cpp:892-960). P9 feeds the LOGICAL authority ONLY.
⊗ B-P9 (blocking): the inferred-path ORDER must NOT derive from SortedPredecessors — it is file-STATIC
  (ControlFlow/Build/Join.cpp:142), unreachable from Regional freeze (layer cycle), does not EXIST at freeze,
  and collapses the planning/logical authorities. FIX: derive order from a DataFlow-visible signal reachable
  from Regional (out_to_in Query.h:832 projected through decl parameter order / SIP arrival order).
⊗ H-P9 (blocking prereq): LiftBoundColumnsToRelation walks forwarding TUPLEs + single-source MERGE that
  Optimize (Build.cpp:2622, AFTER the demand transform :2601) DELETES (Tuple.cpp:60-125). FIX: run inference
  over the pre-Optimize graph OR rewrite the walk over post-Optimize invariants; predict-then-verify against
  a real post-Optimize .df dump.
⊗ H-P9: the -contract-out declared/inferred render MOVES DataFlow->Regional — the current renderer
  (Format.cpp:1745-1798) reads P1-DELETED RecognizedSubgraphs and cannot see the Regional schema.access_paths.
  SPLIT into distinct declared-key/inferred-key provenance lines (owner-adjudicated golden-shape change:
  key_multi_adorn_witness + key_tc_witness contracts move).
```

---

## §4. Session 15 — DONE (this grounding round). What session 16 should do.

Session 15 completed items 1–5 below (all GROUNDING; production untouched):
1. ✅ **Absorbed the 3 P7–P9 blocking corrections into p7p9-diffs §1–§3 in place** — B-P9 order re-sourced
   from a DataFlow signal (adornment ordinal), pre-Optimize decision SETTLED (Arm A, empirically); `kUnplanned`
   plan_kind seam at the mints; `BindingStateId` trie-node interning (now in `Navigate`/`intern`, §2.1).
2. ✅ **Deepened the two residues** — the `.Range` (dissolved: intra-rel seek = P7 kFullKeyHashLookup; the
   ordered `.Range` DFS is the sole new P8 surface) and the `V-INDUCEDORDER-SYMBOLIC-CONSISTENT` belt (§2.5).
3. ✅ **Design-goal diffs** with discriminating exit gates (§4 table; the InducedOrdering belt replaced the
   answer-equality LOST CHECK).
4. ✅ **Critiqued adversarially** — 4-refuter panel, 1 blocking + 8 survivors all folded, the blocking + 3
   mediums dissolved by the P8a→P7 fold; record in `keyed-rewrite-p7p9-critique.md` "Session-15 re-critique".
5. ✅ **Extended IR states** §9–§12 (§12 items 13–15 carry the s15 corrections).

**What session 16 should do (ranked):**
- ✅ **P0-item-4 LANDED + COMMITTED this session** (owner go-ahead) — the parser order-significance flip;
  the hard P5 prerequisite is now in place. Committed at tip `6d6248a2` ("Lock the @key Phase-0 language
  contract") together with the prior safe s11 Phase-0 work (reject @key-on-#query + full-context redecl
  consistency — they were entangled in Parser.cpp and form one coherent Phase-0 unit). SUITE: PASS (252
  cases), no golden moved. P0 (lock the language contract) is now COMPLETE.
- **[OWNER STOP] Green-light the destructive P1 cut** — gated on the 5 B1–B5 + the §6 seven-consumer inventory
  (reconstruction-diffs §6) + the symbol-driven `git grep -l` acceptance gate. Compile-clean only after all fold.
- **Continue grounding P2–P6** (reconstruction-diffs §3) toward compile-clean, if no green-light yet.
- **Predict-then-verify residuals**: dump a real post-P0-item-4 `.contract`/`-region-out` once the parser flips,
  to confirm the §11 split render + §10.1 spine predictions against actual bytes.

P0-item-4 (the parser order-significance flip) was LANDED this session on the owner's go-ahead — see §3 P0.

METHOD: WORKFLOWS (opus for diff-authoring/critique/judgment, sonnet for mechanical census/extraction/
carrier-dumps). Keep the orchestrator thin; subagents return distilled pseudocode/findings, not file dumps.
Front-load carrier dumps once into scratch. Re-verify EVERY anchor at tip before trusting it (they drift).
Do NOT touch production toward P1 (owner-gated). Confirm via git you touched only docs (251 PASS holds by
construction). Update memory at close.
```
