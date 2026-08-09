# Keyed-instance rewrite — POST-P3 whole-program pseudocode + path forward as diffs (session-19 seed)

Session 18 close (2026-08-08). Branch `keyed-instances`, **tip `63573a67`** ("P3: RequestEdge /
FactDerivation acyclic slice (compile-time model)"). **P1 + P2 + P3 ARE LANDED** (compile-clean,
OptDiff SUITE: PASS 222, ctest 5/5). This seed is the START-HERE whole-program backbone for P4. It
supersedes `session-18-whole-program-seed.md` §1 (POST-P2 pipeline) for the Regional/model layer —
the P3 request/derivation model now exists.

**GREENFIELD RULING (owner), still governing.** The compiler is not in use → delete-then-rebuild.
P1 removed the demand authority; P2 typed the regional owner; P3 added the request/derivation MODEL
(compile-time, codegen unchanged); **P4–P9 make it drive real keyed evaluation.** Every post-P1 gate
is STRUCTURAL, never answer-equality (the full-materialization baseline already answers correctly
with `@key` inert). Motivation: memory `greenfield-rewrite-motivation`.

## §0′. P4 LANDED (session 19) — read `p4-grounding.md`

**P4 is LANDED** (honest complete-path specialization / FullScanFilter). The `AccessPlan` fourth
authority (`RegionInstance.h`: `kFullScanFilter`/`kFullKeyHashLookup`/`kRetainedIndexScan` +
`AccessRequirement` + `SelectAccessPlan`) is SELECTED at freeze (`BuildRequestPorts` stores it on
`RequestPortRecord.plan`) and READ at codegen (`BuildQueryEntryPointImpl` / the empty-query arm read
`frozen->PlanFor(redecl)` and WITHHOLD the index for `kFullScanFilter`, guarded by the always-on
V-PLAN-HONEST belt). A bound+free `#query` now lowers to `EmitQueryFriends`' honest full-scan-filter
cursor (`while pos<NumRows` + `if row.<f>!=<param>`, `pos=0`, index elided) instead of the retained
`idx.First/Next` seek; an all-bound query keeps `.Find` (`kFullKeyHashLookup`). `-region-out` renders
`plan=…`. The compile-time request/derivation model half stays P3-style (ctest-only); P4 drives a
codegen CHANGE via the compile-time authority read (NOT a runtime evaluation — the "drives evaluation"
framing was retracted, `p4-grounding.md §8-S3`). Gate GREEN: OptDiff **SUITE: PASS (222)**, ctest 5/5
(extended `RegionInstance` P4 gates), NEGATIVE WITNESS verified (plan flip → header reverts to the
seek), only `booleans.region.*` goldens moved (the `plan=` token, re-blessed). The recursion gate the
critique proposed was DROPPED (premise corrected — every corpus query relation is a non-recursive
projection; full-scan is answer-correct over recursive relations too; `§8-S2`). NEXT actionable = **P5**
(the partial-binding DAG: order-free `BindingStateSchema`, order-significant `BindingEdge`, where a
DECLARED `@key` first specializes). Full record: **`p4-grounding.md`**.

## §0. Status — what P3 changed, what is grounded for P4 [PRE-P4 — historical below]

**Landed at P3 (tip `63573a67`):** `include/drlojekyll/Regional/RegionInstance.h` (new public leaf) —
all regional typed-id domains (Stage-B skeleton ids MOVED here from `Regional.h` + the P3 residual/
ownership/support ids) + `RegionInstanceRelations` with inline ops (`AddRequestEdge`/
`RemoveRequestEdge`/`AddDerivation`/`RouteResults`/`RootedReachability`). Stored BY VALUE on
`FrozenRegionalProgram` (`Instances()`). `Planning.cpp`'s `BuildRequestPorts` splits the query loop:
bound `#query` → `RootLease` request port (`RouteKind::kRequestPort`); all-free → `PermanentRoot`;
both mint a `RequestEdge`. Census `request_ports` re-derived (`CountBoundQueryRedecls`) in lockstep.
`-region-out` renders `-> request-port P<k>` + `request-port P<k> query=…/… bound=(…)`. Design +
critique + IR states: `p3-grounding.md` (§8 critique survivors, §9 IR desired-states).

**Post-P3 honest baseline:** the P3 model is COMPILE-TIME only. For real programs `BuildRequestPorts`
populates `request_edges` + `requested_relation_of_owner`; the `derivations`/`routed_results` half
stays EMPTY (no rule sweep — `RegionTemplate.rules` RESERVED-EMPTY). A bound `#query` STILL evaluates
via the retained full-materialization backend (`BuildQueryEntryPointImpl`, Build.cpp:413 — scan the
canonical table via `scanned_index`). `BindingStateId` interns to EXACTLY the empty state. Keyed
evaluation is still NON-FUNCTIONAL; P4 is the first phase where the model DRIVES evaluation.

**Grounded for P4 (verified at tip this session):** the P4 diff `reconstruction-diffs.md` §3-P4 (lines
417-487) + §5.5 (B3, lines 745-760). **Its anchors have DRIFTED — re-verify all:**
- `ProgramTableScanRegion` handle: `include/drlojekyll/ControlFlow/Program.h:1028` (diff says
  1096-1140 — MOVED). Impl `ProgramTableScanRegionImpl`, `AsTableScan` at Operation.cpp:1202-1203.
- `EmitScan`: `lib/CodeGen/CPlusPlus/Database.cpp:2778` (diff says 3334-3396 — MOVED); the
  `region.IsTableScan()` → `EmitScan` dispatch is Database.cpp:1846-1847 (diff says 1943 — MOVED).
- The bound-query backend P4 layers over: `BuildQueryEntryPointImpl` (Build.cpp:413) already computes
  `col_indices` = the bound decl-ordinals (Build.cpp:422-426) and `scanned_index =
  GetOrCreateIndex(col_indices)` (Build.cpp:439-443), then `impl->queries.emplace_back(query, table,
  scanned_index, …)`. THIS is the existing keyed-scan seam — P4's FullScanFilter question is whether to
  reuse it or mint a fresh `ProgramTableScanRegion` (see §3-P4 open Q).

---

## §1. The whole program TODAY (POST-P3) — pseudocode

### §1.1 Pipeline (bin/drlojekyll/Main.cpp) — anchors verified at tip
```
compile(module, flags):
    query   = Query::Build(module, log, gPassPolicy)              # Main.cpp:73  (3-arg)
    frozen  = FrozenRegionalProgram::Build(query, log)            # Main.cpp:89  (P3 model built here)
    SetRelDumpStream(gRelStream)                                  # Main.cpp:106  (-rel-out seam)
    program = Program::Build(frozen, log, gFirstId, gPassPolicy)  # Main.cpp:110
    GenerateDatabaseCode(program, h, cc)                          # Main.cpp:142
```

### §1.2 DataFlow — Query::Build (unchanged by P3)
```
Query::Build(module, log, policy):                               # lib/DataFlow/Build.cpp:2524
    build SELECT/TUPLE/JOIN/MERGE/CMP/MAP/NEGATE/AGG/KVINDEX/INSERT; Simplify?; Connect; Optimize?;
    LinkViews; IdentifyInductions; Finalize*; BuildEquivalenceSets; Stratify(log)
    impl->row_contracts = InferConservativeRowContracts(impl)    # AllFields on multi-view SCCs (see NOTE)
    return Query(impl)
# NOTE (session-18 finding): the conservative contract keys recursive-SCC views on AllFields — a SOUND
# but USELESS-for-specialization key. Proving a NARROW key (a functional dependency) is P9 +
# the Minimize/DeterminedBy provability widening (O-R3.5); a DECLARED narrow key is @key (P5). P4 does
# NOT narrow keys — it binds ALL of a query's bound columns (complete-path) over a full scan.
```

### §1.3 Regional — FrozenRegionalProgram::Build (P2 typed owner + the P3 model)  [REFRESHED for P3]
```
Build(query, log):                                               # lib/Regional/Planning.cpp:306
    out.dataflow_graph = query
    R = out.region  (RegionTemplate); RR = out.instances (RegionInstanceRelations)
    # ---- input/result ports + ABIs (declaration order)
    for m in received:  R.ports += {kInput, i++, m};  input_abis += {kInput, m, kToPortP, i}
    for m in published: R.ports += {kResult,i++, m};  output_abis += {kOutput,m, kToPortP, i}
    if no published:    output_abis += {kOutput, monostate, kNone, 0}          # the `<none>` line
    # ---- BuildRequestPorts (P3): split the query loop by binding                Planning.cpp:343-386
    for parsed_query in dedup-by-Id(sub-module walk): decl=…; requested=RelationId{decl.Id()}   # L5
        for redecl in dedup-by-BindingPattern(decl.UniqueRedeclarations()):
            cs = CallSiteId{next++}
            if HasBoundParam(redecl):                                            # Build.cpp:422-426 idiom
                lease=RootLeaseId{next++}; R.request_ports += {port=next_port++, redecl, lease, cs}
                query_abis += {kQuery, redecl, kRequestPort, port}
                RR.AddRequestEdge(RootLease(lease), cs, EmptyBindingState(ri{0}), requested)
            else:
                perm=PermanentRootId{next++}; R.permanent_roots += {redecl}
                query_abis += {kQuery, redecl, kPermanentRoot, 0}
                RR.AddRequestEdge(PermanentRoot(perm), cs, EmptyBindingState(ri{0}), requested)
    R.abis = input ++ query ++ output
    # ---- relation schemas (R-STORE insert arm + Tier-2 origin arm) — unchanged from P2
    for (decl,ins) in CollectContractInserts: R.relation_schemas += BuildFromInsert(rc_map, decl, ins)
    for decl in CollectOriginInteriorDecls:   R.relation_schemas += BuildFromOrigin(query, decl)
    # RR.derivations / RR.routed_results stay EMPTY (no rule sweep at P3, M3)
    out.census = DeriveRegionalCensus(query)     # request_ports = CountBoundQueryRedecls(query)
    RECOUNT: built R.request_ports.size()/ports vs census (check_count belt) ; RunFreezeValidators
    return out
```

### §1.3a THE P3 MODEL, as it stands (the P4 launch point)
`RegionInstanceRelations` (RegionInstance.h) is populated for real programs ONLY on the ownership
side: `request_edges` (one per dedup'd redecl, RootLease|PermanentRoot owner) +
`requested_relation_of_owner`. The evaluation side is DEFINED but dormant:
- `AddDerivation(src_state, relation, member_key_positions, row, delta)` — projects the row through the
  positional mask (H3), interns a `RegionalFactId`, folds per-fact `support`, records
  `contributing_states`. NEVER CALLED for a real program at P3 (no rule sweep).
- `RootedReachability(ri)` — seeds `{EmptyBindingState(ri)}` (ProgramRoot, F1) ∪ live request-edge
  dests. At P3 always `{EmptyBindingState}`.
- `RouteResults(st)` — routes support>0 facts of `st` to request edges (dest==st) filtered by
  requested relation (L5). Empty at P3 (no derivations).
- `BindingStateId.vals` is ALWAYS `{}` (the empty state). `RootAlive(RootLease)≡true` (F2).

### §1.4 ControlFlow + codegen — the retained backend a bound query STILL uses (M3)
```
Program::Build(frozen, …):                                      # lib/ControlFlow/Build.cpp:1206
    query = frozen.DataFlowGraph(); context.frozen_census = &frozen.Census()
    …build DR flow, induction fixpoint, commit sweeps…
    for insert in query.Inserts() where IsQuery:  BuildQueryEntryPoint(decl, insert)   # :1378-1385
        BuildQueryEntryPointImpl(decl, insert):                 # Build.cpp:413
            col_indices = [p.Index() for p in decl.Parameters() if Binding()==kBound]  # :422-426
            scanned_index = table->GetOrCreateIndex(col_indices)                        # :439-443
            impl->queries.emplace_back(query, table, scanned_index, forcer, retract)   # the ENTRY record
GenerateDatabaseCode:                                           # a bound query lowers to a cursor
    EmitScan(ProgramTableScanRegion) 3 arms on Index()          # Database.cpp:2778 (VERIFY line)
      index=nullopt -> full-scan + key-equality filter          #   the honest FullScanFilter mold
      index=Some    -> keyed_chain / keyed_probe hash lookup
    IsTableScan()->EmitScan dispatch                            # Database.cpp:1846-1847 (VERIFY line)
```

---

## §2. The target — four authorities + two edges (carried, unchanged)
```
Logical fact          RegionalFactRelation : RegionalFactId -> RegionalFact  (SINGLE fact authority)
Residual              BindingState (frontiers + FactDerivation ids)          (NOT a 2nd fact owner)
Logical access path   DeclaredAccessPath (ORDERED; [A,B] != [B,A])           (P5 seeds, P9 extends)
Physical structure    AccessPlan (FullScanFilter | FullKeyHashLookup | …trie) (P4 introduces FullScanFilter)
RequestEdge   exact ownership, ACYCLIC forest        RuleActivationEdge  derivation dep, MAY cycle
CROSS-CUT: inference (P9) picks NO physical structure; planning (P7) picks NO logical path;
answer identity holds because FullScanFilter is always a correct realization.
```
Full semantic authority: `next-session-prompt.md` (four authorities, "Avoid these false starts",
"Target semantic representation"). Retained invariants: member identity, exact request ownership,
caller-qualified results, drain-before-retire, single-fact-authority, counts-are-caches.

---

## §3. The path forward as diffs (POST-P3 altitude)

### P4 — honest complete-path specialization (FullScanFilter)  [THE NEXT STEP]

Authority: `reconstruction-diffs.md` §3-P4 (417-487) + §5.5 (B3). **Re-verify every anchor (§0 lists
the drift).** Touches the PHYSICAL-access authority (`AccessPlan`), introduced as its own domain,
kept distinct from logical access path and fact identity. **P4 does NOT narrow keys** — it binds ALL
of a query's bound columns (complete-path) and realizes them with `FullScanFilter` (scan + key filter,
`index=nullopt`). It makes the P3 model DRIVE evaluation for the acyclic/complete-path slice.

```
# The design question P4 must settle (grounded, not in the pre-P1 diff):
#   Does P4 (a) LAYER over the existing BuildQueryEntryPointImpl keyed-scan seam (Build.cpp:413,
#   which ALREADY emits index=Some keyed_probe for a bound query), or (b) mint a fresh
#   ProgramTableScanRegion with index=nullopt (the honest FullScanFilter mold)? The diff assumes (b)
#   for label==emission honesty; but (a) already exists and already answers. The B3 gate distinguishes
#   them by the region-cursor `s<id>` naming vs the query-cursor `pos` shape (Database.cpp scan body).
#
+ AccessPlan = { kFullScanFilter }                       # P4: the only realized arm (P7 adds hash/trie)
+ AccessRequirement { relation; available_bindings; required_fields; completeness }
+ SelectAccessPlan(req) = kFullScanFilter                # the single arm
EvaluateEpoch (P4 extension of the P3 acyclic sweep):
    for st in {EmptyBindingState(R)} ∩ live:
        for each requested relation r at st:             # from request_edges' requested_relation
            for (row, sign) in FullScanFilter(model_table_of(r), bound_cols, bound_vals):
                AddDerivation(st, r, member_key_positions_of(r), row, sign)   # H3 projection
            RouteResults(st)                             # L5-filtered; now NON-EMPTY
LowerAccessRequirement(kFullScanFilter, ctx):            # ControlFlow: reuse ProgramTableScanRegion
    table = ctx.model_table_of(req.relation); index = nullopt
    return CreateTableScanRegion(table, index, indexed_columns=bound_cols, input_variables=bound_vars,
                                 body=…)                  # Database.cpp EmitScan emits the honest mold
```
**B3 discriminating gate (STRUCTURAL, not answer-equality — §5.5):** the four §3-P4 probes are
non-discriminating (a no-op P4 on the plain post-P1 cursor passes — the baseline already emits a
NumRows scan + key filter). The corrected gate: carry the terminal `BindingStateId` VALUES into the
keyed scan and assert the emitted filter constant equals the bound value threaded through a
`ProgramTableScanRegion` — distinguishable by the region-cursor `s<id>` naming vs the query-cursor
`pos`/`_cursor` shape; move the 2nd-requester/RoutedResult check to a `-region-out` (or the
`RegionInstance` unit test) compile-time assertion; re-anchor the caveat check to a line P4 changes.
**D1 (open):** P4 needs the terminal `BindingStateId` (non-empty `vals`) — pull the compile-time
`BindingStateSchema` interner forward (a SCHEMA id, not value-bearing). P3 interned only the empty
state; P4 is where a state carries real bound values.

### P5 — the partial-binding DAG (order-free schema, order-significant edge)
`reconstruction-diffs.md` §3-P5 (489-538). `@key(A)` reuses the `{A}` prefix of `@key(A,B)`;
`[A,B]`/`[B,A]` converge on one `{A,B}` schema. `BindingStateSchema` keyed on the field SET
(order-free); `BindingEdge` order-significant. This is where a DECLARED `@key` first specializes.

### P6 — recursive regional execution (SCC + routing + joint fixpoint + DRed deletion)
B4 (per-FACT DRed on `RegionalFactId`, never per-BindingState) + B5 (cross-component transitive
retraction = ONE joint signed-frontier fixpoint). The `RuleActivationEdge` cross-state arm goes live.

### P7 — physical access planning (AccessPlan its own domain; real hash/trie)  — where a narrow key first PAYS
### P8 — the cross-relation ORDERED trie / COLT / Free Join
### P9 — access-path inference (additive, logical-only, PRE-Optimize) + Minimize/DeterminedBy (O-R3.5)  — where AllFields keys are PROVEN narrow

```
  P5–P9 operational diffs + exit gates: reconstruction-diffs.md §3 (P5-P6) + p7p9-diffs.md (P7-P9).
  The "AllFields key is useless" observation (session 18) targets P7 (physical) + P9/O-R3.5 (proving
  the FD) — NOT P4. P4/P5 make a DECLARED/complete key drive evaluation; P9/O-R3.5 prove an
  UNDECLARED narrow key. Both are downstream of P4.
```

---

## §4. What session 19 should do (the grounding loop, weighted to P4)

Enough is persisted to resume cold (this seed + `p3-grounding.md` + `reconstruction-diffs.md`
§3-P4/§5.5 + `p7p9-diffs.md` + `next-session-prompt.md` + memory `regional-dataflow-core-epoch`). P4
is the next ACTIONABLE constructive step. Run the **build-pseudocode → design-goal diffs → critique →
IR-desired-states** loop (the method that worked for P3), on the POST-P3 codebase, weighted to P4:

1. **Ground the whole program at the POST-P3 tip** (this seed is the backbone — keep it current). Run
   the P4 analog of the P1/P2/P3 symbol grep: enumerate every current site P4 must touch or reuse —
   the `ProgramTableScanRegion` handle/impl + `CreateTableScanRegion` mint sites, `EmitScan` + the
   `IsTableScan` dispatch, the `BuildQueryEntryPointImpl` keyed-scan seam P4 either reuses or replaces,
   and where `AddDerivation`/`RouteResults` first get INVOKED for a real program. Ground every anchor
   (the §0 drift shows the diff's anchors have moved). Settle the (a)-reuse-vs-(b)-fresh-scan question.
2. **Formulate design-goal diffs** at hunk grain with DISCRIMINATING STRUCTURAL exit gates
   (region-cursor `s<id>` vs query-cursor `pos`; 2nd-requester routed-result-not-derivation; the
   terminal-BindingStateId-value threaded into the emitted filter constant). Answer-equality is a LOST
   CHECK. Keep the four authorities separate — `AccessPlan` (physical) must not conflate with the
   logical access path or fact identity.
3. **Critique adversarially** (refuter panel) against the real POST-P3 code + retained invariants +
   the "Avoid these false starts" checklist. Special scrutiny: does P4 keep `FullScanFilter` a correct
   realization (answer identity holds)? Does the B3 gate actually DISCRIMINATE a real P4 from the
   post-P1 baseline cursor (which already emits a NumRows scan + key filter)? Does D1 (terminal
   BindingStateId values) land without dragging P5 machinery in? VERIFY, don't assert; rank survivors.
4. **Author/extend the desired IR output states** (predict-then-verify): the P4 `-region-out` (does it
   change? — request ports already render; P4 may add a plan annotation), the `.rel` `kAccessKeyedRelation`
   op family + `plan=FullScanFilter`, the generated `datalog.h` keyed-scan mold (the region-cursor
   `s<id>` shape), and any `RegionInstance` unit-test extension. STRUCTURAL pins only. Sonnet pulls
   current carrier dumps as the baseline; opus authors the desired states.

METHOD: WORKFLOWS (opus for diff-authoring / critique / judgment; sonnet for mechanical census /
carrier-dumps / anchor re-verification). Several sequential single-phase workflows beat one
mega-workflow; keep the orchestrator thin. **Docs-only unless the owner green-lights P4 execution**
(the [OWNER STOP]). All anchors are byte-current at tip `63573a67` (re-verify any before trusting).
```
```
