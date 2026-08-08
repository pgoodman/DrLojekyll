# Keyed-instance rewrite — whole-program grounding + amended path forward

Session 12 close (2026-08-06, branch `keyed-instances`, tip `46a404d4` + the
uncommitted s11 Phase-0 worktree). This is the CONSOLIDATED whole-program view: it
re-explains the architecture as pseudocode at whole-program altitude, states the
target as operational pseudocode, and expresses the path forward as diffs **with the
session-12 adversarial-critique amendments folded in**. It is the single grounding
backbone; the detailed artifacts hang off it.

## §0. Persistence map — where everything lives

| Need | Artifact |
| --- | --- |
| Semantic authority (why `@key` is a relation-local ordered path) | `next-session-prompt.md` |
| This whole-program backbone (start here) | **this file** |
| Deepened current pseudocode (8 subsystems) + RE-VERIFIED anchor table + drift ledger | `keyed-rewrite-current-pseudocode.md` |
| Seed: §1 pipeline, §2/§3 OPERATIONAL target model + eval contract, §4 P0–P9 | `keyed-rewrite-pseudocode-seed.md` |
| P1–P9 hunk-grained diffs (P1.1–P1.7, P6.1–P6.6) | `keyed-rewrite-phase-diffs.md` — **s12 hunks; SUPERSEDED for P1 + P2–P6 by the session-13 corrected set below** |
| **CORRECTED P1 inventory + deepened P2–P6 operational diffs + D1–D4 resolutions + §5 re-critique amendments (session 13)** | **`keyed-rewrite-reconstruction-diffs.md` — the current actionable authority for P1 + P2–P6** |
| s12 adversarial critique (33 findings + 7 refuted) | `keyed-rewrite-critique.md` (folded into reconstruction-diffs) |
| **s13 re-critique of the corrected diffs (23 survivors + 20 certifications)** | **`keyed-rewrite-reconstruction-critique.md`** |
| Desired post-cut IR states for the carriers + the 2 no-baseline carriers (P6 co-recursive, P5 convergence) | `keyed-rewrite-ir-desired-states.md` (§6/§7 authored session 13) |
| Retained vs superseded invariants | `INDEX.md` supersession matrix + `../RegionalDataFlowCore.md` |

State: docs only this session; **suite 251 PASS unchanged; nothing blessed; no
production code touched** (the tree's Parser.cpp/Query.cpp/reject_key_redecl_1.dr are
the pre-existing s11 Phase-0 witnesses).

---

## §1. Whole-program pipeline — CURRENT (what we cut FROM)

Compact; `keyed-rewrite-current-pseudocode.md` has the implementer-grain per-subsystem
pseudocode with verified `file:line`.

```
# bin/drlojekyll/Main.cpp
compile(module, flags):
    query  = Query::Build(module, log, policy, demand=flags.gDemand,
                          demand_retract=flags.gDemandRetract)      # Build.cpp:2524
    frozen = FrozenRegionalProgram::Build(query, log)              # Planning.cpp:439 (degenerate)
    program= Program::Build(frozen, log, first_id, policy,
                            demand_instance=flags.gDemandInstance)  # Build.cpp:1333
    GenerateDatabaseCode(program, h, cc)                           # Database.cpp

# THE demand authority (Build.cpp:2524 -> Demand.cpp:388)
Query::Build:
    build SELECT/TUPLE/JOIN/MERGE/CMP/MAP/NEGATE/AGG/KVINDEX/INSERT graph
    ConnectInsertsToSelects -> proxy_view_to_decl                  # Connect.cpp:164
    ApplyDemandTransform(...):                                     # Demand.cpp:388  <-- THE CUT
        activation gate: scan parsed module for HasInstanceKey()   # :411-430 (pragma_activated)
        containment: !demand_mode && !pragma_activated -> return
        locate bound query; per-adornment SIP walk (fences)
        V-DECLARED-KEY RP-10 bijection (declared set-of-sets == inferred)  # :892-959
            canon = sort(each set)   # <-- ORDER-COLLAPSE, dup of Parser.cpp:1477
        FabricateDemandMessage/Local (demand__ prefix)            # Parse/Demand.cpp:170,226
        mint d_p relation + push-down guard JOIN (d_p ⋈ p)
        stamp GuardAnnotation + RecognizedSubgraph; register QueryDemandForcing
    Optimize (CSE migrates GuardAnnotation via View/Join/IdentityJoin/Link.cpp)
    Stratify (Tarjan + neg/agg rejects + V-SCC-SEAM)              # Stratify.cpp:124
    row_contracts = InferConservativeRowContracts (pure, 2-phase) # RowContract.cpp:365

# Flat/nested selector (Build.cpp:1439-1568)
Program::Build:
    query = frozen.Query()                                        # H4 unwrap :1337
    feature-gap pre-pass (agg/kv/map/product rejects)             # :1345-1437
    per-forcing fences from LIVE GuardAnnotations (view-identity,  # :1466 bucket by forcing_index
        ABA-safe): cyclic_demand = ViewSelfReachable(guard lead);
        recursive_content = kBody input induction-owned/self-reach
    demand_instance -> STRICT reject on inadmissible; else RP-9 SILENT
        pragma->nested if all_forcings_admissible && HasInstanceKey  # :1532
    context.demand_instance_enabled = effective_demand_instance    # :1568

# Rel nested lowering (Rel.cpp), gated on demand_instance_enabled
ResolveLiveRecognition(query)          # :936-1025 ABA-safe shape recovery from live guard JOINs
BuildSubgraphInstanceOps(...)          # :1038-1200 mint kSubgraphInstantiate (:1146 kSectionWalk
    "lie"), kInstanceDeath, kInstanceSeal
# CodeGen (Database.cpp:2352-2787): emit_instance_rescan = FULL SCAN of input,
#   key-filter, TryAdd (labelled section-walk; honesty acknowledged in-code :2339)
# Runtime (InstanceStore.h:54-219): keys[iid] complete key -> two Table<RowT>
#   (frozen/current); Touch/Seal/RecycleCurrent. A LEAF ROW CACHE — no prefixes,
#   no recursion, no request routing, no shared canonical facts.
```

THE IDENTITY GAP: keyed lowering reduces facts/derivations/requests/forcing-indexes/
runtime-handles to `GuardAnnotation`/`RecognizedSubgraph` side-records + raw ints.
`FrozenRegionalProgram` is a 7-count census + 5 render-string vectors + a Query
pass-through — it owns no regional semantics.

---

## §2. Target — the four authorities + EvaluateEpoch (what we cut TO)

Operational pseudocode is in `keyed-rewrite-pseudocode-seed.md` §2 (identity interning,
the two edge relations, canonical facts, access-requirement-vs-plan) and §3
(`EvaluateEpoch`). The contract in one screen:

```
FOUR AUTHORITIES, never collapsed:
  Logical fact          RegionalFactRelation : RegionalFactId -> RegionalFact   (SINGLE fact authority)
  Residual              BindingState (owns frontiers + FactDerivation ids)       (NOT a 2nd fact owner)
  Logical access path   DeclaredAccessPath (ORDERED fields; [A,B] != [B,A])
  Physical structure    AccessPlan (FullScanFilter | FullKeyHashLookup | trie | ...)
TWO EDGE RELATIONS:
  RequestEdge        exact ownership, ACYCLIC forest (roots/permanent/parent-member)
  RuleActivationEdge derivation dep, MAY CYCLE in a region; liveness = ROOTED reachability
IDENTITY:
  BindingStateSchema keyed on the field SET (order-FREE); DeclaredAccessPath ORDERED;
  [A,B] and [B,A] converge on ONE {A,B} schema; @key(A) shares the {A} prefix of @key(A,B).
EvaluateEpoch = least fixpoint of (rooted-reachability ∘ semi-naive derivation),
  order-independent; deletion computes the SAME least fixpoint as fresh-from-committed;
  drain routed removals BEFORE retiring an unreachable activation SCC; counts are caches.
```

RETAINED (do not break): semantic member identity + SemanticMemberKey; exact
RequestEdge ownership + multiple owners + late attach + caller-qualified results +
drain-before-retire; pure-region/effect boundary + epoch order-independence; ownership
forest acyclic. SUPERSEDED: recursion-stays-in-one-instance; demand-has-no-source-
annotation; no-cyclic-binding-routing.

---

## §3. Path forward as diffs — AMENDED (critique folded in)

The phase sequence and per-phase diffs are in `keyed-rewrite-phase-diffs.md`. Below is
the whole-program **amendment layer**: the corrections the s12 critique proved are
REQUIRED, plus the two genuine sequencing decisions the owner must make. Treat the
phase-diffs doc's P1 deletion inventory as SUPERSEDED by §3.1.

### §3.1 P1 (the destructive cut) — CORRECTED deletion inventory

The atomic P1 as authored **would not compile**. Corrections (all CONFIRMED against
real code):

```diff
  P1.4 (Rel keyed-lowering deletion):
-   RETIRE Lowering::kSectionWalk enum value entirely
+   DELETE ONLY the keyed use at Rel.cpp:1146 + the V-ALPHA validator (Rel.cpp:4431,
+   Rel.h:497). KEEP the kSectionWalk enum value — it is load-bearing for ordinary
+   join pivots at Rel.cpp:2433 (acc->lowering = pivot_inputs.empty()?kFullScan:kSectionWalk,
+   ctx=kFixpoint). The "lie" is confined to the keyed Rederive access, not the enum.

  P1.5 (codegen deletion) — ADD the ControlFlow/Build consumers, and fix anchors:
+   delete LowerSubgraphInstances + call (Procedure.cpp:279,588)
+   delete kSubgraphInstance/kSubgraphInstantiate DR-op-walk case arms (Procedure.cpp:196,499,601-604)
+   delete Context::EmittedInstanceOp/emitted_instance_ops + V-INST-EMITTED cross-check (Build.h:264-272)
+   delete ProgramInstanceStore (Stratum.cpp:2328)
+   delete BOTH IsSubgraphInstance arms: effects collector (Database.cpp:766) AND
+     the EmitSubgraphInstance emit-dispatch (Database.cpp:1930)  # anchor was wrong (:766 only)
+   delete the second IsDemandMessage codegen arm (Database.cpp:1522), not just :3692

  P1.6 (DataFlow side-record deletion) — ADD the CSE-migration + render consumers:
+   lib/DataFlow/{View,Join,IdentityJoin,Link}.cpp: excise GuardAnnotationsCompatible/
+     PromoteSurvivorToBody/CheckGuardAnnotationFold/PrintGuardAnnotation + every
+     guard_annotation_index / guard_annotations / guard_annotation_folded_count read-write
+     (the whole CSE-migration block; the field is permanently kNoGuardAnnotation post-cut)
+   the free-function decls at lib/DataFlow/Query.h:1268-1290
+   lib/DataFlow/Format.cpp:1747-1765 (the .df RecognizedSubgraphs render)
+   lib/DataFlow/Build.cpp:2677-2678 (the demanded-interior belt — DELETE, not "no-op")
+   remove the !query.IsDemandMessage(m) conjunct at Planning.cpp:164 (post-cut a no-op)
+   delete the IsDemandMessage decl at DataFlow/Query.h:1108 after all 3 callers are gone

  P1.6 exit gate (V-REGION-CENSUS) — NOT tautological-safe as stated:
~   the census recount must keep an INDEPENDENT path (walk the DataFlowGraph carrier:
~   CollectContractInserts/CollectMessages over query) vs the projected census, so a
~   stub that under/over-populates still aborts. The strongest anti-silent-pass belt
~   for P1 is the .rel CENSUS MULTISET (kSubgraphInstantiate/kInstanceSeal/kIngestFold
~   deltas are exact integers per carrier — see keyed-rewrite-ir-desired-states.md §5).
```

Post-P1 honest baseline (unchanged from the plan): `@key` inert metadata; a bound
`#query` reads the canonical fully-materialized relation via the plain cursor; keyed/
residual evaluation is NON-FUNCTIONAL until P4/P5. `demand_*` twins deleted; `key_*`
datasets survive as ANSWER witnesses (need STRUCTURAL `.rel` pins added — see §3.3).

### §3.2 P2/P3 (regional authority + request/activation) — amendments

```diff
  P2 (FrozenRegionalProgram authoritative):
+   RelationSchema must carry visible_fields (ordered view-relative FieldId list) OR a
+   precomputed positional member-key projection, so Format.cpp renders member-key
+   WITHOUT the value-id->position bridge that P2 otherwise drops.
~   V-REGION-CENSUS: source the two sides of the belt from DIFFERENT places (typed
~   records vs a DataFlowGraph re-walk) — a census that projects the records it should
~   recount is a tautology (silent-pass).
!   recursive_components has ONE producer (see §3.4 decision D2): do NOT populate it from
    the nonexistent query.MultiViewStrata().

  P3 (RequestEdge/RuleActivationEdge/FactDerivation over full materialization):
~   With "only the empty-binding state exists", do NOT mint intra-state activation edges
    that degenerate to self-loops. EITHER reserve RuleActivationEdge for the cross-
    binding-state hops introduced in P4/P5, materializing the acyclic rule chain
    directly here, OR define the P3 acyclic guard over the relation/fact dependency DAG,
    not over binding-state reachability.
```

### §3.3 P4/P5 (honest specialization + partial-binding DAG) — amendments

```diff
  P4 (honest FullScanFilter) — must be a REAL emission, not prose (BLOCKING):
+   (a) a ProgramRegion subclass (or reused scan region) for keyed FullScanFilter access
+   (b) a Database.cpp region-dispatch arm + Emit fn re-provisioning the full-scan+key-filter mold
+   (c) the DR-op -> ControlFlow lowering
+   (d) honesty realized AT CODEGEN: either codegen branches on the AccessPlan label, or
+       V-PLAN-HONEST checks the ProgramRegion SHAPE (codegen is currently label-blind, so
+       a DR-tail belt can only assert label ∈ CodegenPlanCapabilities, NOT label==emission)
+   P4 exit gate: add a STRUCTURAL clause — grep the generated code for the key-filter
+     loop bound to the declared key, AND a differential test where keyed routing
+     observably differs from a blind full read (a late 2nd requester adds RoutedResults
+     but ZERO new fact Tables). Answer-equality alone is a LOST CHECK post-P1.

  P5 (partial-binding DAG):
+   exit gate: for @key(A,B,C) assert genuine NON-prefix subsets {A,C},{B},{C} are ABSENT
+     and the declared-prefix chain {A}⊂{A,B}⊂{A,B,C} IS present (the old "no {A,B}" clause
+     was wrong — {A,B} is a declared prefix that IS materialized).
+   BindingStateSchema (order-free, values-free) is the interned schema; BindingStateId
+     (with values) is runtime-only. A trie node keys on the SCHEMA, never re-sorting into
+     a values-bearing id (the P8 regression).
```

### §3.4 Open sequencing/design decisions (owner)

- **D1 — P4/P5 ordering.** As written, P4's "keyed evaluation" is really a full-
  materialization no-op UNLESS the minimal `BindingStateId`-with-values interning is
  pulled forward from P5, because the bound query's RequestEdge is seeded to the EMPTY
  binding state and nothing re-seeds it to the terminal keyed state. Choose: (i) pull
  the terminal-`BindingStateId` interning into P4 (P4 becomes the first real keyed
  filter), or (ii) keep P4 a labelled full-materialization step and land the first real
  keyed filter at P5 (re-scope P4's claim + exit gate accordingly). Recommend (i) — it
  makes P4's honesty claim real and testable.
- **D2 — recursive_components producer.** P2 (typed field) vs P6.1 (RelationSchema-level
  Tarjan) must not both populate it from different SCC domains. Recommend: P2 leaves it a
  reserved empty typed field; P6.1 is the sole populator AND closes SCCs through the
  message publish→receive seams (`ForEachInsertToSelectSeam`) so it agrees with
  `Stratify`'s condensation (else message-mediated recursion is mis-partitioned).
- **D3 — P6.5 deletion semantics.** Root-loss (P6.6, rooted-reachability) and input-
  driven retraction of a still-rooted fact (P6.5) are SEPARATE and BOTH required. P6.5
  needs a real OVERDELETE→REDERIVE (DRed) pass over the semi-naive frontier (or fresh-
  from-committed per affected SCC). The seed §3 deletion contract already mandates the
  same-least-fixpoint result; P6.5's diff must show the mechanism, not just add-only
  semi-naive.
- **D4 — P7 honesty seam location.** Put the label==emission guarantee at the codegen
  emission site (EmitAccessPlan dispatches on plan.kind) OR make codegen read the label
  as the sole scan/index selector. The DR-IR-tail belt cannot enforce label==emission.

---

## §4. Status after session 13 + the immediate next step

**Session 13 (2026-08-06) DONE (grounding; docs only, suite 251 PASS, nothing blessed):**
§3's amendments were enacted into `keyed-rewrite-reconstruction-diffs.md` (compile-clean P1
inventory, D1–D4 resolutions, deepened P2–P6 operational diffs, design-goal diffs); the
reconstruction phases were deepened to implementer grain via a sonnet fan-out (anchors
re-verified at tip); a fresh opus panel re-critiqued the corrected diffs
(`keyed-rewrite-reconstruction-critique.md`: 23 survivors + 20 certifications) and its
blocking/high `Fix:`es were folded into reconstruction-diffs §5; the IR desired-states were
extended with both no-baseline carriers (P6 co-recursive, P5 convergence), each grounded by
its VERIFIED CURRENT REJECT.

**D1–D4 resolved** (reconstruction-diffs §2): D1 = pull the minimal terminal
`BindingStateSchema` interner (not the DAG) into P4 (§5.8); D2 = P6.1 sole populator of
`recursive_components` (owner-free); D3 = two mechanisms (per-fact DRed support-loss + rooted
worklist), corrected by B4/B5 to a per-FACT rebuild interleaved to a joint fixpoint; D4 =
label==emission by construction via `ProgramTableScanRegion` reuse (codegen is label-blind).

**Before the owner green-lights the destructive P1 cut, the s13 re-critique's 5 BLOCKING items
must be settled in the plan** (all folded, reconstruction-diffs §5): B1 (P1 injector + census
consumers of `QueryDemandForcing` — the cut still won't compile without them), B2 (P3 must root
permanent-root programs), B3 (P4 exit gate must discriminate against the baseline), B4 (DRed is
a per-FACT rebuild, not a per-BindingState mirror), B5 (cross-component transitive retraction
needs a joint fixpoint). H6 makes Phase-0 item 4 (the parser order-free→order-significant flip
at Parser.cpp:974-995 + 1477-1488) a HARD prerequisite of P5. The next grounding round (if any)
hardens P4/P6 emission at codegen grain; otherwise the owner green-lights P1.
