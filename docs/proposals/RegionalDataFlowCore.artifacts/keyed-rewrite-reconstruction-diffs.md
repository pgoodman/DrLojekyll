# Keyed-instance rewrite — corrected P1 inventory + deepened P2–P6 reconstruction diffs

Session 13 (2026-08-06, branch `keyed-instances`, tip `46a404d4` + the uncommitted
s11 Phase-0 parser worktree). This is the **corrected + deepened** successor to
`keyed-rewrite-phase-diffs.md` for the reconstruction phases. It:

1. gives the **compile-clean** P1 deletion inventory (folds every s12 missed-deletion /
   wrong-anchor finding, all re-verified at tip this session — §1);
2. resolves the four open sequencing/design decisions **D1–D4** with a recommendation +
   rationale each (§2);
3. deepens **P2–P6** into operational algorithms at implementer grain, each carrying its
   critique `Fix:` amendments and a corrected exit gate (§3); and
4. formulates the **design-goal diffs** the s12 critique found under-resolved (§4).

Authority chain: `INDEX.md` → `next-session-prompt.md` (semantic authority) →
`keyed-rewrite-whole-program.md` (backbone; its §3 is the amendment layer this doc
enacts) → **this file** (P1 corrected + P2–P6 deepened) → `keyed-rewrite-phase-diffs.md`
(the s12 hunk diffs; **P1.x inventory and P2–P6 exit gates here SUPERSEDE it where they
disagree**) → `keyed-rewrite-pseudocode-seed.md` §2/§3 (target algebra). The fresh
adversarial critique of THIS doc's diffs is `keyed-rewrite-reconstruction-critique.md`;
the desired post-cut IR states are `keyed-rewrite-ir-desired-states.md`.

**State: docs only. Suite 251 PASS unchanged; nothing blessed; no production code touched.**
Phase 1 remains **OWNER-GATED and destructive** — this is grounding, not implementation.

### §0.1 Anchors re-verified this session (drift corrections vs the s12 tables)

Every anchor below was re-derived at tip by a per-subsystem reader pass. Corrections
that matter for the diffs:

| Concern | s12 said | verified @ tip | 
| --- | --- | --- |
| `CollectDemandInteriorDecls` | Planning.cpp:201 | **Planning.cpp:210** |
| `RowContract.member_key` | RowContract.h:51 | **RowContract.h:46** (`:51` is `using RowContractMap`) |
| `RowContract.visible_fields` | — | **RowContract.h:40** |
| `Context::frozen_census` field | (unlocated) | **Build.h:230** `const RegionalCensus *frozen_census{nullptr}` |
| query-ABI/permanent-root routing loop | Planning.cpp:201-285 | **Planning.cpp:512-562** (Build starts :439; :201-285 is `CollectMessages`/`NumForcingsOfName`) |
| bound-`#query` recognition | (implied Planning) | **Demand.cpp:459-471** (`decl.IsQuery() && any param kBound`) |
| Demand `canon` sort twin | Demand.cpp:902 | **Demand.cpp:905-908** (used :920-927; whole fn dies with P1) |
| `EmitSubgraphInstance` emit-dispatch | (only :766 cited) | **Database.cpp:1930-1931** (inside `EmitRegion`); :766 is the `CollectEffects` collector arm — **BOTH** exist (critique F25 right) |
| Database.cpp region-dispatch | "one chain" | **THREE chains**: `WalkRegion` 551-650, `CollectEffects` 678-991, `EmitRegion` 1868-1985 |
| scan-vs-index selector | (unlocated) | **`Generator::EmitScan` Database.cpp:3334-3396** (`ProgramTableScanRegion`), keyed on `region.Index()`/`InputVariables()`, **never** the Rel `Lowering` label |
| `grep -c lowering Database.cpp` | =0 (claim) | **=0, re-verified** — codegen is label-blind; `Lowering` read only at Format.cpp:322-327,558 + V-ALPHA Rel.cpp:4429-4431 |
| keyed `kSectionWalk` mint | Rel.cpp:1146 | **Rel.cpp:1146** (TRUE) |
| non-keyed join-pivot `kSectionWalk` | Rel.cpp:2433 | **Rel.cpp:2432-2433** `pivot_inputs.empty()?kFullScan:kSectionWalk`, `Ctx::kFixpoint` (TRUE — F4/F15 confirmed) |
| `ProgramTableScanRegion` (reuse target) | — | **Program.h:1096-1140** (`Table`/`Index`/`Body`/`IndexedColumns`/`InputVariables`); Visit @1545 |
| `LowerSubgraphInstances` + switch arms | (hand-waved) | **Procedure.cpp:279 (call :588), :196-197, :431-501, :601-604**; `Context::EmittedInstanceOp` **Build.h:268-272**; `ProgramInstanceStore` **Stratum.cpp:2328** (F6 confirmed) |
| `MultiViewStrata()` | (P2 hunk calls it) | **ABSENT** — real accessors `QueryView::Stratum()` Query.h:374 (`optional<unsigned>`), `Query::NumStrata()` Query.h:1114 (F16 confirmed: the P2 hunk would not compile) |
| generic `TarjanCondense` | "reuse Stratify's" | **ABSENT** — Tarjan is inlined in `QueryImpl::Stratify` over `VIEW*`, file-local `TarjanState`/`TarjanFrame` (Stratify.cpp:19/:31); reuse is an **extraction obligation** (F30) |
| message publish→receive seam edge | Stratify.cpp:168 | **Stratify.cpp:162-172** `sources[select]<-insert` for **every** INSERT→SELECT seam incl. message streams; `io_seams` :169-171; iterator `ForEachInsertToSelectSeam` **Query.h:1149 / Differential.cpp:18-41** |
| OVERDELETE→REDERIVE→INSERT | (design doc only) | **LANDED**: Stratum.cpp:1796-1841 (round-shell pairing), :257-277 (`DerivClass`), :660-680 (`EmitRederive` = a `C_r` counter read, not a search); split `C_nr`/`C_r` Table.h:21-43,344-377,427,656-714 |
| `Induction.cpp` "semi-naive frontier to reuse" | phase-diffs:1102 | **DRIFT of MEANING**: Induction.cpp is the VIEW-level *identification* pass (`MergeSet`/`InductionGroupId` :89-292); the literal frontier-loop template P6.5 mirrors is **Stratum.cpp**, not Induction.cpp |
| `F21` render target | Regional/Format.cpp:183 | **DataFlow/Format.cpp:1745-1798** (`:183` is just the `declared-key` boolean badge, no per-path order) |

---

## §1. P1 — the compile-clean deletion inventory (corrected)

The whole-program §3.1 amendment layer + the seven s12 sub-cuts P1.1–P1.7, with EVERY
missed deletion and wrong anchor from the critique folded in and re-verified. P1 lands
atomically as one commit; the corpus is re-run only at the tail. Ordering P1.1→P1.7 is
the recommended review sequence (you cannot delete a side-record until its consumers are
stubbed to the demand-free baseline).

### §1.1 The additions the atomic cut needs to COMPILE (critique B4/B5/B6, H13/H14, F5/F6/F14/F24/F25)

These are the deletions the s12 P1.x inventory OMITTED. Without them the atomic commit
does not build:

```diff
  P1.1 (call-site anchor, F32):
~   re-anchor the demand cut to lib/DataFlow/Build.cpp:2601 (impl->ApplyDemandTransform call);
    proxy_view_to_decl is a Build.cpp stack local (:2582), passed by ref — NOT a Connect.cpp:1307 out-write.

  P1.2/P1.6 (IsDemandMessage's THREE callers, F14):
+   IsDemandMessage def dies with lib/DataFlow/Demand.cpp (P1.1). Its callers:
+     - Database.cpp:1522 (2nd codegen suppression arm) — DELETE (P1.5 missed it; only :3692 was cited)
+     - Database.cpp:3692 (1st codegen suppression arm) — DELETE
+     - Planning.cpp:164 (inside CollectMessages, which P1.6 marked "UNCHANGED") — remove the
+       !query.IsDemandMessage(m) conjunct (post-cut every received message is real; a no-op filter)
+   THEN delete the decl at DataFlow/Query.h:1108 (P1.2 mis-stated it as Parse.h + "codegen-only").

  P1.4 (KEEP the enum value, F4/F15):
-   RETIRE Lowering::kSectionWalk enum value entirely
+   DELETE ONLY the keyed use at Rel.cpp:1146 + V-ALPHA arm A (Rel.cpp:4422-4442, the
+   kInstanceKeySlot⇒{kPointTest,kSectionWalk} clause). KEEP the enum value — it is
+   load-bearing for ordinary join pivots at Rel.cpp:2432-2433 (Ctx::kFixpoint) and is
+   rendered at Format.cpp:322-327,558. The "lie" is confined to the keyed Rederive access.

  P1.5 (BOTH IsSubgraphInstance arms + the ControlFlow/Build consumers, F6/F25):
+   Database.cpp: delete the CollectEffects collector arm (:766) AND the EmitRegion emit
+     dispatch (:1930-1931, calls EmitSubgraphInstance) — two arms, not one.
+   ControlFlow/Build: delete LowerSubgraphInstances (Procedure.cpp:279) + its call (:588),
+     the AsSubgraphInstance()/kSubgraphInstantiate/kInstanceDeath/kInstanceSeal DR-op-walk
+     arms (Procedure.cpp:196-197,431-501,601-604), Context::EmittedInstanceOp/
+     emitted_instance_ops + the V-INST-EMITTED cross-check (Build.h:268-272; check
+     Procedure.cpp:592-611), and ProgramInstanceStore (Stratum.cpp:2328).

  P1.6 (the GuardAnnotation CSE-migration TUs + the .df render, F5/F24):
+   lib/DataFlow/{View,Join,IdentityJoin,Link}.cpp: excise GuardAnnotationsCompatible/
+     PromoteSurvivorToBody/CheckGuardAnnotationFold/PrintGuardAnnotation + every
+     guard_annotation_index / guard_annotations / guard_annotation_folded_count read-write
+     (the whole CSE-migration block — the field is permanently kNoGuardAnnotation post-cut).
+     Concrete sites: View.cpp:588-764, Join.cpp:289, IdentityJoin.cpp:160-162,
+     Link.cpp:230-236 (the edde6c82 ProxyMergedViews guard-preserve fix), free-function
+     decls Query.h:1268-1290.
+   lib/DataFlow/Format.cpp:1747-1765: the .df RecognizedSubgraphs render (reads rs.demanded_decl).
+   lib/DataFlow/Build.cpp:2677-2678: the demanded-interior belt — DELETE OUTRIGHT (it
+     dereferences impl->recognized_subgraphs; it cannot be a "no-op").

  P2 dependency (F31 — fold into P2, not P1): Context::frozen_census (Build.h:230) must
    change type when the census recount moves to typed records; see §3 P2.
```

### §1.2 P1.6 exit gate — NOT tautology-safe as stated (F12/F26)

The V-REGION-CENSUS belt at Rel.cpp:4638-4662 recomputes `DeriveRegionalCensus(query)`
and compares to the stored census — **but P1.6 sets `request_ports = 0` as a constant and
the stored census is ALSO `DeriveRegionalCensus(query)`**, so the request-ports check
degenerates to `0 == 0` regardless of correctness. For P1 the census belt is transiently
weak on request-ports; the **strongest anti-silent-pass belt for P1 is the `.rel` census
multiset** (`kSubgraphInstantiate`/`kInstanceSeal`/`kIngestFold` are exact integers per
carrier — see `keyed-rewrite-ir-desired-states.md` §5): a stub that leaves any instance op
raises the multiset and fails. State the census vacuity explicitly at P1; it regains teeth
at P2 when the recount is sourced independently (§3 P2, F12 fix).

### §1.3 Post-P1 honest baseline (unchanged)

`@key` is inert parsed metadata; a bound `#query` reads the canonical fully-materialized
relation via the plain cursor. `demand_*` twins deleted; `key_*` datasets survive as
ANSWER witnesses (need STRUCTURAL `.rel` pins added — the answer-equality-alone gate is a
lost check post-P1, critique B2/F3). Keyed/residual evaluation is NON-FUNCTIONAL until P4.

---

## §2. Open decisions D1–D4 — resolutions

Each is grounded in the re-verified code above. D2 is a clean owner-free call; D1/D3/D4
have a strong recommendation but D1 and D4 remain genuine owner sequencing/architecture
choices — framed crisply for `AskUserQuestion` at implementation time.

### D1 — P4/P5 ordering (the empty-state seed hazard, F17)

**Problem.** P3 seeds every RequestEdge to the EMPTY binding state; P4's
`EvaluateKeyedRequest` reads `e.dest_state.bindings`, but nothing re-seeds the edge to a
keyed terminal `BindingStateId`, and the values-bearing interning is P5. As written, P4's
keyed filter is **structurally unreachable** — `st.bindings` is empty, the "filter"
degenerates to a bare scan.

**Recommendation: (i) pull the MINIMAL terminal-`BindingStateId`-with-values interning
forward into P4.** P4 becomes the first *real* keyed filter. Rationale: it makes P4's
honesty claim (label==emission of a *key-filtered* scan) actually testable — otherwise
P4's exit gate is answer-vacuous (already flagged B2/F3), and "P4 does keyed evaluation"
is false. The minimal pull-forward is narrow: `BindingStateId(region_instance, schema,
sorted values)` for the single terminal complete-path state per bound query, plus the
request-edge re-seed hunk (`dest_state = BindingStateId(query's bound field/value map)`).
It does NOT require P5's full prefix DAG / BindingEdge / convergence machinery — those stay
P5. P4 gets the endpoint; P5 generalizes to the partial-binding lattice.

**Owner call framing (if rejected):** the alternative (ii) is to keep P4 an explicitly
labelled full-materialization step (route through the empty state) and land the first real
keyed filter at P5 — re-scoping P4's claim + exit gate to "honest FullScanFilter *label*
over the complete relation" only. Choose (ii) only if you want P4 to ship with zero
BindingStateId-with-values machinery; the cost is that P4 proves nothing keyed.

### D2 — recursive_components producer (F16) — RESOLVED, owner-free

**P2 leaves `recursive_components` a reserved EMPTY typed field; P6.1 is the sole
populator.** P2 must NOT call `query.MultiViewStrata()` (it does not exist — the hunk would
not compile) or any SCC accessor. P6.1's `ComputeRecursiveComponents` runs the
RelationSchema-level Tarjan **after** `RegionTemplate.rules` is populated, and **closes SCCs
through message publish→receive seams** (`MessageSeamsOf`, mirroring Stratify.cpp:162-172 /
`ForEachInsertToSelectSeam`) so its partition agrees with Stratify's condensation projected
to relations (F7). Element type defined once: `RecursiveComponent.members : set<RelationSchema>`.
Recommended belt: a DEBUG assertion that the two SCC domains never disagree on *which*
relations are mutually recursive.

### D3 — P6.5 deletion semantics (the BLOCKING soundness gap, B0/F1)

**Root-loss (P6.6, rooted-reachability) and input-driven support-loss of a still-rooted
fact (P6.5) are SEPARATE and BOTH required.** P6.5 must carry a **real
OVERDELETE→REDERIVE (DRed) pass** — which already exists, landed, as the flat differential
engine at `Stratum.cpp:1796-1841` (round-shell pairing), `Stratum.cpp:660-680`
(`EmitRederive` = a `C_r` counter read, not a search), over the split `C_nr`/`C_r` counters
(`Table.h:21-43,344-377,427,656-714`).

**Resolution (mechanism, grounded — the owner does not "decide" soundness, only ratifies
the shape):** `EvaluateEpoch` runs TWO explicit passes:
- **(A) within-epoch support-loss**, per `AffectedRecursiveComponents(input_deltas)`,
  **independent of root liveness** — the exact StackSafeNegation §5.2 OVERDELETE→REDERIVE→
  INSERT algebra, keyed **per BindingState** instead of per global table. A fact whose only
  support was a retracted input is retracted here even if its binding state is still rooted.
- **(B) the joint rooted-reachability + semi-naive worklist** — the root-loss path (P6.6),
  monotone add-only for *reachability*, but `AddDerivation` in the worklist accepts negative
  `delta.sign` so (A)'s consequences propagate forward through the same worklist.

This satisfies the seed §3 DELETION CONTRACT (same least fixpoint as fresh-from-committed)
with two distinct mechanisms rather than one add-only loop. The choice for the owner is only
**which incremental scheme** (per-SCC DRed vs fresh-from-committed re-derivation per affected
SCC) — both are contract-compliant; DRed reuses the landed machinery and is recommended.

### D4 — P7 honesty seam location (label==emission; F2/F9/F33)

**Codegen is label-blind (`grep -c lowering Database.cpp = 0`, re-verified).** The DR-tail
V-PLAN-HONEST can only assert `label ∈ CodegenPlanCapabilities`, never `label == emission`.

**Recommendation: Option 1 — make label==emission a compile-time invariant of the lowering
function, by REUSING `ProgramTableScanRegion` as the FullScanFilter emission shape** (see §3
P4). Because `kFullScanFilter → mint a ProgramTableScanRegion with index=nullopt` and a
future `kFullKeyHashLookup → index=Some(idx)`, and codegen's `EmitScan` (Database.cpp:3334-
3396) *already* selects scan-vs-index from `region.Index()`, the AccessPlan label and the
emitted C++ shape become the **same thing** — there is no separate label left to drift, and
V-PLAN-HONEST is redundant *for the P4 slice* (the referee is `LowerAccessRequirement`'s own
exhaustive per-AccessPlan-arm switch, checked by `-Wswitch`/`assert(unreachable)`).

**Owner call framing (P7, when a 2nd plan kind arrives):** Option 2 (a DR-tail belt) becomes
necessary the moment two DIFFERENT AccessPlan values could lower to the SAME region kind
(e.g. `FullKeyHashLookup` and `ExistingTriePrefix` both reusing `ProgramTableScanRegion`
distinguished only by `Index().has_value()`) — the label→shape map stops being injective and
a real emission-fidelity check is needed. Recommend: keep Option 1's injective map as long as
possible (one region shape per AccessPlan kind); only when it breaks, move the referee into
`EmitScan`/`EmitAccessPlan` (branch on `plan.kind`), never the DR-IR tail. **F33 corollary:**
the surviving join-pivot `kSectionWalk` label sits in the same epistemic position (a V-ALPHA
DR-only badge; `EmitJoin` never reads it) — fixing its honesty is the same Option-1/Option-2
choice applied to the join lowering site, out of P4/P7's scope but flagged.

---

## §3. Deepened P2–P6 — operational diffs (implementer grain)

Each phase: corrected anchors → operational pseudocode (real type/function names where they
will live) → corrected exit gate → critique fixes folded. The four authorities stay
separate throughout (logical fact / residual specialization / logical access path / physical
structure) — each phase names which authority it touches and which it must NOT prematurely
fabricate.

### P2 — FrozenRegionalProgram becomes the typed semantic owner

**Touches:** logical-fact authority (member_key) + the render/naming plumbing. Must NOT
fabricate residual/access-path/physical authorities (they are P3/P4/P5/P7).

```
struct RelationSchema {
    RelationId              id
    fields                  : vector<SymbolicFieldId>    # decl-ordinal identity, one per param
    visible_fields          : SemanticMemberKey          # rc.visible_fields, COPIED verbatim (F13)
    member_key              : SemanticMemberKey          # rc.member_key, COPIED verbatim
    declared_key_positions  : vector<bool>               # size==arity; [i] = (visible_fields[i] in
                                                          #   member_key). PRECOMPUTED at freeze — the
                                                          #   exact bool the old Planning.cpp:591-608
                                                          #   loop computed inline. Format renders off
                                                          #   THIS, no value-id/ordinal bridge (F13).
    declared_access_paths   : DeclaredAccessPathSet       # decl.InstanceKeys() carried through (P0/P5)
    support                 : bool                        # view.CanReceiveDeletions()
}
struct RegionTemplate {
    RegionId  id                                          # RegionId(0) at Stage B
    inherited_symbolic_fields : vector<SymbolicFieldId>   # empty at Stage B
    relation_schemas          : vector<RelationSchema>
    rules                     : vector<RuleRoutingProjection>   # NEW (P6.2 fills the field map)
    recursive_components      : vector<RecursiveComponent>       # RESERVED EMPTY at P2 (D2/F16)
    request_ports / result_ports / permanent_roots
}

BuildRelationSchema(query, decl, view, row_contracts):        # the ONE friend-leak read
    rc = row_contracts.at(view.impl)                          # SAME site as Planning.cpp:574
    positions = [ (i < rc.visible_fields.size() && rc.visible_fields[i] in rc.member_key)
                  for i in [0, decl.Arity()) ]                # SAME test as Planning.cpp:600-604
    return RelationSchema{ RelationId(decl.Id()), FieldsOf(decl), rc.visible_fields,
                           rc.member_key, positions,
                           InternDeclaredPaths(decl.Id(), decl.InstanceKeys()),
                           view.CanReceiveDeletions() }

FrozenRegionalProgram::Build(query, log):                     # Planning.cpp:439
    R = RegionTemplate{ id: RegionId(0) }
    rc_map = query.impl->row_contracts                        # KEEP the friend-class access (M1): RowContractMap/
                                                              #   RowContract/QueryViewImpl are PRIVATE lib types;
                                                              #   a public accessor would WIDEN the leak, not close it.
                                                              #   lib/Regional already includes lib/DataFlow/Query.h.
                                                              #   (Alt: return an opaque by-value {decl,visible,member}.)
    for (decl, ins) in CollectContractInserts(query):         # R-STORE, unchanged walk
        R.relation_schemas.push(BuildRelationSchema(query, decl, QueryView(ins), rc_map))
    for (decl, view) in CollectOriginInteriorDecls(query):    # Tier-2 SUBSUMES the deleted Tier-1
        R.relation_schemas.push(BuildOriginSchema(query, decl, view))
    R.recursive_components = []                                # RESERVED — P6.1 is sole populator (D2)
    R.request_ports = BuildRequestPorts(query); R.result_ports = BuildResultPorts(query)
    R.rules = BuildRuleRoutingProjections(query)
    out = FrozenRegionalProgram(R)
    out.census = DeriveRegionalCensus(query)                  # STILL a function of query (F12)
    RunFreezeValidators(out)                                   # V-FROZEN-NO-OPEN-PORT / V-OWNERSHIP-ACYCLIC
    return out

# Format.cpp render, no RowContract re-lookup:
RenderMemberKeyText(schema, decl):
    return "(" + join([decl.NthParameter(i).Name() for i in [0,decl.Arity())
                       if schema.declared_key_positions[i]], ", ") + ")"
```

**Deletions/retargets (corrected):** RegionalAbi/Port/Internal/PermanentRoot/Contract
structs (Regional.h:75/82/97/102/108); the `::hyde::Query query` owner field → renamed
`dataflow_graph`, exposed only via `DataFlowGraph()`; the five owned render vectors;
`CollectDemandInteriorDecls` (Planning.cpp:210) + `ResolveInteriorSupport` (:249) — both
read P1-deleted demand side-tables; the friend leak (:574); `Context::frozen_census`
(**Build.h:230**) is KEPT as `const RegionalCensus *` — **NOT retyped** (superseded by §5 H1:
its sole reader V-REGION-CENSUS reads a `RegionalCensus`, and F12 keeps the census
query-derived; add a separate `frozen_regions` field only when a later phase needs the typed
record); `DeriveRegionalCensus(const Query&)` signature is KEPT (F12).

**Exit gate (corrected, F12/F23).** (1) The **~24** `.region.<mode>` goldens (6 cases × 4
modes at tip — NOT "180+"; the surviving post-P1 set is ~12: join_1, merge_2,
tc_nonlinear_diff×4, plus re-blessed key_* carriers) re-derive from typed records with no
`--bless`. (2) `grep -rn 'frozen.Query()\|query.impl->row_contracts\|RecognizedSubgraph\|
GuardAnnotation\|DemandForcings\|RegionalAbi\|RegionalPort\|RegionalContract' lib/Regional
lib/Rel lib/ControlFlow` returns zero. (3) **The anti-hollow-`R` belt is TWO referees (H2 —
NOT V-REGION-CENSUS, which is fully tautological: `DeriveRegionalCensus(query)` vs a STORED
`DeriveRegionalCensus(query)`, all 7 fields):** (a) the region goldens render FROM `R`, so a
hollow `R` diverges them; and (b) **P2 must RE-POINT the in-`Build` recount** — today
Planning.cpp:663-687 counts `out.ports`/`out.contracts.size()` (the render vectors P2
deletes/retypes) against `DeriveRegionalCensus(query)`; P2 re-provides it reading the NEW typed
`R` (`R.relation_schemas.size()`, `R.request_ports.size()`, `R.result_ports.size()`), still
vs `DeriveRegionalCensus(query)`. **Census is always a function of `query`, never of
`RegionTemplate` (F12)** — so the recount is a genuine cross-check (typed `R` vs query re-walk),
not a tautology, and a stub `Build` emitting a hollow `R` aborts. (4) IdentityTypes ctest
passes with `member_key` typed.

**Critique fixes folded:** F12 (independent recount), F13 (carry visible_fields AND
precompute declared_key_positions — render needs no bridge), F16/F31 (recursive_components
reserved-empty; frozen_census KEPT `const RegionalCensus *` at Build.h:230 per H1 — NOT
retyped; its reader Rel.cpp:4638-4662 reads a `RegionalCensus`, add a separate `frozen_regions`
field only when a later phase needs the typed record), F23 (real golden count), M1 (keep the
friend-class access, drop the "public accessor retires the leak" claim), H2 (the in-`Build`
recount re-pointed at typed `R` is the real anti-stub belt).

**s16 anchor re-verification + false-start certification.** All P2 Planning.cpp anchors hold at
tip `6d6248a2` (below the parser → byte-identical to baseline `46a404d4`; spot-verified this
session): `Build` :439-441, the friend-leak `query.impl->row_contracts` :574, the
`declared_key_positions` loop :591-608 (incl. the `decl.Arity() <= i` unit-relation break F13
relies on), the in-`Build` recount :663-687. **CERTIFICATION vs the "Avoid these false starts"
checklist item "Do not let DataFlow mutation plus post-optimization recognition remain the
authority once the regional program exists":** P2 does NOT re-introduce it. Freeze is a PURE
post-Optimize READ of the already-built/optimized `query` to construct typed records; it mutates
no DataFlow node and runs no recognition pass (that WAS `ApplyDemandTransform` + `RecognizedSubgraph`,
both deleted in P1). The one standing obligation: `R.rules`/`BuildRuleRoutingProjections(query)`
must derive from the frozen read, never a re-recognition — at P2 the field is only structurally
reserved (P6.2 is its sole populator), so the certification holds by construction.

### P3 — RequestEdge / RuleActivationEdge / FactDerivation / RoutedResult (acyclic slice)

**Touches:** ownership authority (RequestEdge) + support authority (FactDerivation), kept
distinct from logical-fact identity (RegionalFactId) and each other. New file
`lib/Regional/RegionInstance.h` (sibling of Regional.h's typed-id idiom).

```
struct RootLeaseId{uint32_t v;<=>}; struct PermanentRootId{uint32_t v;<=>};
struct BindingStateId { RegionInstanceId ri; BindingStateSchemaId schema; SortedBoundFieldValues vals; }
                                                    # P3: interns to EXACTLY the empty state (vals={})
struct RegionalFactId { RegionInstanceId ri; RelationId relation; SemanticMemberIdentity member; }
using RequestOwnerId = variant<RootLeaseId, PermanentRootId, RegionalFactId /*RegionalMember, P6+*/>;
struct RequestEdgeId       { RequestOwnerId owner; CallSiteId call_site; BindingStateId dest; }
struct RuleActivationEdgeId{ BindingStateId src_state; RegionalFactId src_fact; RuleId rule; BindingStateId dst; }
struct FactDerivation      { DerivationId id; BindingStateId src_state; RegionalFactId fact; DerivationSupportCount support; }
struct RoutedResultId      { RequestEdgeId request_edge; RegionalFactId fact; }
struct RegionInstanceRelations { map request_edges; map activation_edges; map<RegionalFactId,FactDerivation> derivations; set<RoutedResultId> routed_results; }

BuildRequestPorts(region_template):                 # RE-PROVIDES Planning.cpp:512-562's forcing loop
    for parsed_query in dedup-by-Id(sub_module_walk):
        for redecl in decl.UniqueRedeclarations():
            if any(p.Binding()==kBound for p in redecl.Parameters()):   # Demand.cpp:459-471's test
                lease = RootLeaseId(next++)
                e = AddRequestEdge(RootLease(lease), CallSiteId(redecl), EmptyBindingState(ri))
                request_ports.push(PortId(next++), lease, e)
            else:
                permanent_roots.push(PermanentRootId(next++), RelationOf(redecl))

AddRequestEdge(owner, cs, dest) = intern(request_edges,(owner,cs,dest))   # 2nd owner = DISTINCT edge, no re-derive
RemoveRequestEdge(e): request_edges.erase(e); erase routed_results where request_edge==e   # caller-qualified

DeriveActivationEdge(src_state, src_fact, rule, dst):        # F18 FIX (a):
    if src_state == dst: return NONE                          #   P3 mints NO intra-state (self-loop) edge;
    assert !ActivationReachable(dst, src_state)               #   the acyclic RULE DAG carries the order.
    return intern_or_drop(activation_edges,(src_state,src_fact,rule,dst))  # edges reserved for cross-state (P4/P5)

AddDerivation(src_state, relation_schema, row, delta):        # F29 FIX: member via the P2 schema key
    member = SemanticMemberIdentity{ relation_schema.member_key, ProjectRow(row, relation_schema.member_key) }
    fact_id = RegionalFactId(src_state.ri, relation_schema.id, member)
    d = derivations.get_or_create(fact_id, src_state); d.support += (delta==kAdd?+1:-1)
    on 0->>0 publish born; on >0->0 publish dies; return fact_id

EvaluateEpoch(input_deltas, request_deltas):                  # P3: acyclic
    old = SnapshotCommittedOutputs(); ApplyInputDeltas(input_deltas)
    for rd in request_deltas: Add/RemoveRequestEdge(rd)
    live = RootedReachability(request_edges, activation_edges)     # ONE topo sweep (DAG, no fixpoint)
    for st in {EmptyBindingState(R)} ∩ live:
        for rule in topo_order(rules_of(region_of(st))):          # F18: rule DAG order, not self-edges
            for (row,sign) in EvaluateRule(rule, st, AccessPlan=FullScanFilter):  # P4 supplies plan
                AddDerivation(st, RelationSchemaOf(rule.head), row, sign)
        RouteResults(st)
    RetractRoutedResults(all_states \ live)
    # RetireUnreachableSCCs: reserved no-op in P3 (no activation cycles yet)
    Publish(Difference(old, CurrentCommittedOutputs())); Seal()
```

**Exit gate (unchanged from phase-diffs P3 + F29 clause).** Directed battery over
repurposed key_* datasets: (a) exact owner count matches distinct `AddRequestEdge`, no fact
duplicated; (b) 2nd requester adds RoutedResults, ZERO new FactDerivations; (c)
`RemoveRequestEdge` on one of two retracts only its routed copies, fact stays present; (d)
support-vs-ownership independently observable; (e) a cycle-closing `DeriveActivationEdge`
trips the acyclic assert — **only reachable once cross-state edges exist (post-P4/P5), so it
does not misfire on P3 DAG programs (F18)**; (f) **F29:** two rows agreeing on `member_key`
columns intern to ONE `RegionalFactId`, rows differing there do not.

**Critique fixes folded:** F18 (Fix (a): no intra-state activation edges; rule DAG carries
order; assert becomes live only post-P4/P5), F29 (AddDerivation threads
`RelationSchema.member_key`), F17 (grounded for D1 — see §2 D1; **P3 does NOT re-seed to a
keyed state; that is D1's call**).

### P4 — honest complete-path specialization (FullScanFilter, label==emission by construction)

**Touches:** physical-access authority (AccessPlan) — introduced as its own domain, kept
distinct from logical access path and canonical fact identity. **Key realization: reuse
`ProgramTableScanRegion` — no new ProgramRegion subclass, no new Database.cpp dispatch arm.**

```
# (a) ControlFlow: REUSE ProgramTableScanRegion (Program.h:1096-1140) as the FullScanFilter shape.
#     A keyed complete-path read IS "scan Table, filter IndexedColumns==InputVariables, run Body" —
#     exactly its contract. Table() = the ONE canonical RegionalFactRelation model table (NOT an
#     InstanceStore leaf). Index() = nullopt at P4 (Phase 7 populates it for hash/trie plans).
LowerAccessRequirement(dr_op /*kAccessKeyedRelation, plan==kFullScanFilter*/, ctx):
    table      = ctx.model_table_of(dr_op.relation)
    bound_cols = dr_op.binding_state.bound_field_positions()          # decl-ordinal
    bound_vars = [ctx.VarFor(f,v) for (f,v) in dr_op.binding_state.canonical_bindings]  # D1: needs
                                                                       #   the terminal BindingStateId
    return ctx.program.CreateTableScanRegion(table, index=nullopt,
             indexed_columns=bound_cols, input_variables=bound_vars, body=LowerRegion(dr_op.body))

# (b) Database.cpp: NO new dispatch arm. The EXISTING region.IsTableScan()->EmitScan (Database.cpp:1943)
#     + EmitScan body (3334-3396) already emit, with index=nullopt, the honest mold:
#       for (uint32_t s = 0; s < <table>.NumRows(); ++s) {
#         const auto r = <table>.RowAt(s);
#         if (r.<key0> == bound0 && ...) { <body> }
#       }
#     label==emission because there is no separate DR token to drift from the region kind.

# (c) Rel: mint kAccessKeyedRelation with plan = SelectAccessPlan(AccessRequirement).
SelectAccessPlan(req): return AccessPlan::kFullScanFilter          # P4: the only realized arm

# (d) Honesty seam: D4 Option 1 — label==emission is a compile-time invariant of
#     LowerAccessRequirement's per-AccessPlan-arm switch; V-PLAN-HONEST redundant at P4.
```

**Deletions folded:** the keyed `kSectionWalk` mint (Rel.cpp:1146) + V-ALPHA arm A only
(NOT the enum value — F4/F15); the Database.cpp:2339-2341 honesty caveat; the InstanceStore
double-buffer as a fact owner (facts live solely in RegionalFactRelation).

**Exit gate (corrected — STRUCTURAL, not answer-only; B2/F3).** Answer-equality is a LOST
CHECK post-P1 (the full-materialization baseline already answers correctly with @key inert).
Add discriminating probes: **(1) positive structural** — grep generated `datalog.h` for the
key-filter scan loop tied to the ACTUAL bound var:
`for (uint32_t s… < <table>.NumRows()…) { … if (r.<key0> == <bound0> …) }`; **(2)
differential** — a late 2nd RequestEdge to the same BindingStateId adds a RoutedResult with
ZERO new fact `Table_<n>` (a blind-full-read no-op passes probe 1 but cannot fake the
caller-qualified RoutedResult behavior — that requires real RequestEdge/BindingState
plumbing); **(3)** N declared @key paths emit exactly ONE fact `Table_`; **(4)** the
Database.cpp:2339-2341 caveat no longer exists.

**Critique fixes folded:** B1/F2 (concrete hunks reusing ProgramTableScanRegion — no
dangling symbol, no new dispatch arm), F3 (discriminating structural + differential probes),
F4/F15 (keep the enum value), F33 (join label honesty = same D4 choice, flagged), F17/D1
(P4 needs the terminal BindingStateId — see §2 D1).

### P5 — the partial-binding DAG (order-free schema, order-significant edge)

**Touches:** logical-access-path authority (ordered paths) + residual-specialization
identity (binding states), kept distinct.

```
InternDeclaredPaths(relation, raw_paths):                 # raw_paths dup-free intra-path, order-kept
    seen={}; paths=[]
    for p in raw_paths:                                    # pragma order irrelevant to identity
        canon = tuple(p)                                   # NO SORT — order IS identity (replaces the
        if canon in seen: REJECT "exact duplicate access path"   #   ADJ-K1-A sort at Parser.cpp:978-984)
        seen.add(canon); paths.append(DeclaredAccessPath{relation, p})
    for i,p in enumerate(sorted(paths, key=p.ordered_fields)): p.id = KeyPathId(i)  # DETERMINISTIC (F21)
    return DeclaredAccessPathSet{paths}                    # unordered set, unique by (relation, ordered_fields)

BindingStateSchema(region, field_set) = intern(schema_table,(region, sort_by_field_id(field_set)))
    # ORDER-FREE, VALUE-FREE. @key(A) and @key(A,B)'s first hop both compute {A} -> SAME id (prefix sharing).
BindingEdge{ parent_schema, added_field, child_schema }   # ORDER-SIGNIFICANT navigation
BindingStateId(ri, schema, sorted values) = intern(binding_state_table,(ri, schema, sort_by_field(values)))
    # runtime peer, minted only in EvaluateEpoch (F11: a trie node keys on the SCHEMA, never this)

MaterializePrefixChain(region, path):                     # LAZY: declared-or-visited only, never power set
    prev = BindingStateSchema(region, {})
    for f in path.ordered_fields:
        cur = BindingStateSchema(region, prev.fields ∪ {f}); BindingEdge(prev, f, cur); prev = cur
    # [A,B]: edge({},A,{A}),edge({A},B,{A,B});  [B,A]: edge({},B,{B}),edge({B},A,{A,B}) -> ONE {A,B} schema

# Cross-redecl consistency REPLACES SameKeySetOfSets (Parser.cpp:1477): order-free ACROSS paths,
# order-SIGNIFICANT within each path — { tuple(p) for p in a } == { tuple(p) for p in b }, NO per-path sort.

# F21 render fix — the REAL target is lib/DataFlow/Format.cpp:1745-1798 (NOT Regional/Format.cpp:183):
RenderDeclaredKeyLines(decl):                              # replaces the for(dset:decl.InstanceKeys()) @1762
    for p in sorted(InternDeclaredPaths(decl.relation, decl.InstanceKeys()).paths, key=p.id):
        emit "declared-key rel=<name> declared=(<p.ordered_fields, intra-order KEPT>)"
    # inter-path order now order-free (pragma reorder -> byte-identical); intra-path order stays significant
```

**Exit gate (corrected, F8/F20/F21).** (1) **F8:** for `@key(A,B,C)` assert the declared
prefix chain `{A}⊂{A,B}⊂{A,B,C}` IS present AND genuine NON-prefix subsets `{A,C},{B},{C}`
are ABSENT (the s12 "no {A,B}" clause was wrong — {A,B} is a declared prefix). Schema-level
(compile-time) claim, separated from any value-state claim. (2) `@key(A)`/`@key(A,B)` share
ONE `{A}` schema id + one `empty--A-->{A}` edge; `[A,B]`/`[B,A]` show two ordered edges into
ONE `{A,B}` schema. (3) **F20 (ActiveSubset trap):** after materializing ≥1 keyed state, an
unbound read of the SAME relation returns the COMPLETE answer (⊇ any active state's rows),
NOT the active subset. (4) **F21:** reordering two `@key` pragmas → byte-identical
`-contract-out` (via the KeyPathId-sorted render).

**Critique fixes folded:** F8 (prefix-present + non-prefix-absent), F20 (post-materialization
completeness clause), F21 (canonicalize render at DataFlow/Format.cpp:1745-1798), F11
(TrieNode keys on BindingStateSchemaId, never BindingStateId — named here, enforced at P8).

### P6.1/P6.2 — query-independent SCC + typed edge-local routing

```
GenericTarjan<Node, EdgeFn>(nodes, out_edges):            # EXTRACTION OBLIGATION (F30) — no
    ... iterative index/lowlink/on_stack/scc_stack, sources-first pop ...   # TarjanCondense exists today

RuleDependencyEdges(region_template):
    edges = { body_rel -> head_rel for rule in rules for body_rel in rule.body }
    for (pub_decl, recv_decl) in MessageSeamsOf(region_template):     # F7: close message seams like
        edges[RelationOf(pub_decl)].add(RelationOf(recv_decl))        #   Stratify.cpp:162-172 / ForEachInsertToSelectSeam
    return edges

ComputeRecursiveComponents(region_template):              # SOLE populator (D2); runs after rules exist
    sccs = GenericTarjan(region_template.relation_schemas, RuleDependencyEdges(region_template))
    region_template.recursive_components = [RecursiveComponent{members: scc}
                                            for scc in sccs if len(scc)>1 or has_self_edge(scc)]
    # DEBUG belt: project Stratify's VIEW-level condensation onto relations; assert it never
    # disagrees with this partition on WHICH relations are mutually recursive.

RuleRoutingProjection(rule) = { (SymbolicFieldId(body_rel,i), SymbolicFieldId(head_rel,j))
                                 for each var V at body pos i and head pos j of rule }
PromoteSharedSymbolicField(f_a, f_b, rt):                 # union ONLY when EVERY producer agrees
    producers = ProducerRulesOf(rel_of(f_a)) ∪ ProducerRulesOf(rel_of(f_b))
    if producers and all((f_a<->f_b) in RuleRoutingProjection(r) for r in producers):
        union(symbolic_field_classes, f_a, f_b)
```

**Exit gate.** Co-recursive `p(K,X)@key(K), q(X,K)@key(X)` with NO `#query`:
`recursive_components == {p,q}` as one component, byte-identical under add/remove `#query`;
each of p,q marked recursive, cross-checked against Stratify's projected condensation (F19 —
a positive directed assertion, NOT the vacuous byte-identical-under-query gate); a
hand-constructed all-producers-agree case promotes, co-occurrence-only does not (both arms of
PromoteSharedSymbolicField); Stratify.cpp:272-344 rejects unchanged.

**Critique fixes folded:** F7/H6 (MessageSeamsOf closes message-mediated recursion), F16
(sole producer, element type once), F30 (GenericTarjan is an extraction obligation, not
reuse), F19 (positive directed SCC assertion replaces the vacuous invariance gate), F28
(PromoteSharedSymbolicField reaches fixpoint before P6.3's CommonPreservedPrefix).

### P6.3–P6.6 — fusion / cyclic activation / joint fixpoint / DRed deletion

```
PlanRecursiveComponent(scc, rt):                          # AFTER PromoteSharedSymbolicField fixpoint (F28)
    P = CommonPreservedPrefix([RuleRoutingProjection(r) for r in rules_within(scc)])  # promoted classes
    if P nonempty and all(route.dest_prefix == P): return FusedFixpoint(binding_prefix=P)
    else: return JointFixpoint(scc)
    # F28 SAFETY CLAUSE: fusion is a PROVEN-SAFE optimization — a FusedFixpoint plan is
    # cross-checked against JointFixpoint on the SAME dataset; the "one frontier per K"
    # STRUCTURAL check is asserted SEPARATELY from answer-equality, never substituted for it.

DeriveActivationEdge(src_state, src_fact, rule, dst):     # P6.4: LIFT the P3 acyclic restriction
    # dst bindings = RuleRoutingProjection(rule) image of src_fact (P6.2), never a raw column copy.
    intern_or_drop(activation_edges,(src_state,src_fact,rule,dst))   # MAY cycle; NEVER an AddRequestEdge

EvaluateEpoch(input_deltas, request_deltas):              # P6.5 — the D3/B0/F1 fix: TWO passes
    old = SnapshotCommittedOutputs(); ApplyInputDeltas(input_deltas)   # incl. RETRACTIONS
    for rd in request_deltas: Add/RemoveRequestEdge(rd)

    # (A) WITHIN-EPOCH SUPPORT-LOSS — per AffectedRecursiveComponents, INDEPENDENT of root liveness.
    #     The landed StackSafeNegation §5.2 algebra (Stratum.cpp:1796-1841 / EmitRederive :660-680),
    #     keyed PER BindingState over split C_nr/C_r (Table.h): OVERDELETE (TryClaimDel claim-round
    #     on C_nr down-crossings) -> REDERIVE (a C_r>0 COUNTER READ, not a search: re-enqueue rows
    #     still cyclically supported) -> INSERT (TryClaimAdd mirror). A fact whose only support was a
    #     retracted input is ACTUALLY retracted here even if its state is still rooted.
    for scc in AffectedRecursiveComponents(input_deltas): OVERDELETE; REDERIVE; INSERT per LiveBindingStatesOf(scc)

    # (B) JOINT ROOTED-REACHABILITY + SEMI-NAIVE WORKLIST — the root-loss path (P6.6).
    worklist = { seed states of live RequestEdges }
    repeat to LEAST FIXPOINT:
        live = RootedReachability(request_edges, activation_edges)   # monotone add-only (correct for its job)
        st = worklist.pop(); if st not in live: continue
        for rule in rules_of(region_of(st)):
            for delta in EvaluateRule(rule, st):                     # semi-naive; delta.sign<0 ACCEPTED
                if AddDerivation(st, delta.fact, delta.sign):        #   (propagates (A)'s consequences)
                    for (rule2,dest) in RouteFactForward(delta.fact, st):
                        DeriveActivationEdge(st, delta.fact, rule2, dest); worklist.push(dest)
        RouteResults(st)

    drained = DrainRoutedRemovals(all_states \ live)      # R1: ONE operation (was two names, same set)
    RetireUnreachableSCCs(drained)                        # P6.6: assert scc.facts ⊆ drained THEN collect
    Publish(Difference(old, CurrentCommittedOutputs())); Seal()

RetireUnreachableSCCs(drained):                           # liveness RE-DERIVED, never refcount
    for scc in { s : s ⊆ (all_states \ live) }:           # a self-supporting cycle keeps support>0 on
        assert scc.facts ⊆ drained                        #   every member — a refcount collector DEADLOCKS
        FreeBindingStateStorage(scc)                       # real drain+collect (NOT InstanceStore's forever-iid)
```

**Exit gate.** Same-key co-recursion (`p@key(K),r@key(K),p:-r,r:-p`) produces ONE
BindingStateId per K spanning both, and byte-equals the P1 baseline across 4 modes + oracle
(fusion answer-checked vs joint — F28). Different-key co-recursion (`p@key(K)/q@key(X)`)
CONVERGES to the P1 published surface, byte-identical across 4 modes + oracle + I0 RefInterp,
worklist-order-perturbation invariant. **B0/F1:** retract an edge supporting a self-supporting
still-rooted `path`/`p` cycle → the fact IS retracted (pass (A)); a directed assertion shows
`FactDerivation.support > 0` at removal yet the fact drops (reachability/DRed decided it, not
a counter). **P6.6:** removing the sole RootLease drives the `{p,q}` cycle's published surface
to empty; a late-2nd-requester variant leaves the shared child alive (caller-qualified drain).

**Critique fixes folded:** B0/F1 (pass (A) DRed, independent of root liveness, mirroring the
landed Stratum.cpp machinery — the single biggest gap), R1 (Drain/Retract = one operation),
F28 (promotion-fixpoint ordering + fusion-as-proven-safe-optimization with separate
structural/answer checks).

---

## §4. Design-goal diffs (the goals the s12 critique found under-resolved)

| # | Goal | Resolving phases (this doc) | The load-bearing correction |
|---|------|-----------------------------|------------------------------|
| 1 | Four-authority separation | P2 (member_key), P4 (AccessPlan domain), P6.2 (SymbolicFieldId), P7 (physical structure) | AccessPlan is its own Rel domain, NOT a reuse of the join `Lowering` enum |
| 2 | Order-significant paths + order-free binding identity | P5 (BindingEdge ordered / BindingStateSchema order-free) | convergence moves to the STATE authority; the SameKeySetOfSets sort is deleted, not re-used |
| 3 | RequestEdge vs RuleActivationEdge | P3 (both introduced, type-disjoint), P6.4 (activation may cycle) | F18: P3 mints NO intra-state activation edges (no self-loops); RequestEdge stays an acyclic forest |
| 4 | Rooted-reachability liveness (not refcount) | P3 (roots), P6.5 (rooted worklist), P6.6 (drain-before-retire) | a self-supporting cycle keeps support>0 forever — a refcount deadlocks; liveness is re-derived |
| 5 | Honest FullScanFilter | P1 (delete the keyed kSectionWalk use only), P4 (reuse ProgramTableScanRegion), P7 (real plans) | label==emission by construction (D4 Option 1); codegen is label-blind, so a DR-tail belt cannot enforce it |
| 6 | Partial-binding DAG | P5 (declared prefix chain, lazy) | F8: declared prefixes ARE materialized; only NON-prefix subsets must be absent |
| 7 | Co-recursive key flow | P6.1 (SCC), P6.2 (routing), P6.3 (fusion), P6.4 (cyclic activation), P6.5 (DRed) | F7: SCC must close message seams; B0/F1: the DRed deletion path is mandatory |

**Goal 5 (honest FullScanFilter) — REAL emission, not prose (B1).** The four hunks are §3
P4 (a)–(d): reuse `ProgramTableScanRegion` (Program.h:1096-1140) so the existing
`region.IsTableScan()→EmitScan` dispatch (Database.cpp:1943→3334-3396) emits the honest
key-filter scan with zero new dispatch code; `index=nullopt` lands EmitScan's existing
full-scan arm. Exit gate is STRUCTURAL (grep the key-filter loop tied to the bound var) +
differential (2nd requester adds RoutedResults, zero new fact Tables), never answer-only.

**Goals 2+6 (partial-binding DAG + order-free schema identity) — §3 P5.** BindingStateSchema
keyed on the field SET (order-free); BindingEdge on the ordered `(parent, added_field)`;
both `[A,B]` and `[B,A]` edge chains terminate at ONE `{A,B}` schema. The corrected exit gate
(F8) asserts declared prefixes present + non-prefix subsets absent + post-materialization
completeness (F20). Contract render canonicalized at DataFlow/Format.cpp:1745-1798 (F21).

**Goals 4+7 (co-recursive key flow + rooted reachability + real DRed) — §3 P6.** The
critical correction is B0/F1: P6.5 carries pass (A) within-epoch support-loss (DRed,
independent of root liveness) beside pass (B) rooted-reachability, mirroring the LANDED
`Stratum.cpp:1796-1841` OVERDELETE→REDERIVE→INSERT over split `C_nr`/`C_r`. Retirement is
re-derived reachability + drain-before-retire, never refcount (P6.6).

**Goal 3 (RequestEdge vs RuleActivationEdge distinct) — §3 P3/P6.4.** Two type-disjoint
relations; RequestEdge is an acyclic ownership forest, RuleActivationEdge a derivation
dependency that may cycle (P6.4) but is NEVER accepted by `AddRequestEdge`. F18: P3's acyclic
slice mints no self-loops (the rule DAG carries order); the acyclic assert becomes live only
once cross-state edges exist post-P4/P5.

---

## §5. Session-13 re-critique amendments (folded)

An independent opus refuter panel attacked §1–§4 against real code
(`keyed-rewrite-reconstruction-critique.md`, 23 survivors + 20 certifications). The direction
held; the amendments below are REQUIRED and supersede the corresponding §1–§4 text. Anchors
re-verified at tip.

### §5.1 P1 additions the corrected inventory STILL missed (B1/L1/L2/L3/L4 — compile-clean, take 2)

The §1 cut references three more P1-deleted-symbol consumers. Add to the atomic commit:

```diff
+ ControlFlow injector (B1): excise BuildQueryInjectorFromRegistry (Build.cpp:413-503) + the
+   registry branch of BuildQueryInjectorProcedure (Build.cpp:507-519, reads *context.demand_forcings
+   / QueryDemandForcing) — but PRESERVE BuildQueryInjectorProcedure's ForcingMessage fall-through
+   (:522-527, serves user @first, still called :551/:556). Delete Context::demand_forcings field
+   decl at Build.h:127 (P1.3 deleted only the :1560 assignment).
+ Regional census (B1): retarget DeriveRegionalCensus so request_ports no longer calls
+   query.DemandForcings() (Planning.cpp:388 -> literal 0); delete the request-port/internal loop
+   (Planning.cpp:445-468) and NumForcingsOfName (Planning.cpp:142, takes QueryDemandForcing&).
+ The struct/accessor themselves: QueryDemandForcing (Query.h:985), DemandForcings() decl
+   (Query.h:1093), demand_forcings member (Query.h:1222).
~ P1.6 renderer (L2): delete the WHOLE block DataFlow/Format.cpp:1745-1798 (not 1747-1765 — that
+   orphans 1766-1798) and relabel it "-contract-out declared-key render" (NOT ".df").
~ P1.6 accessor (L3): the QueryView::GuardAnnotationIndex() DEFINITION is Query.cpp:353 (not
+   :298-316, which are the Demand.cpp Query:: accessors killed with the P1.1 TU).
+ IsCutSuccessorDR (L4, Rel.cpp:1571-1576): a SURVIVING live reader of BOTH deleted symbols —
+   drop the whole `context.demand_instance_enabled && succ.GuardAnnotationIndex()...` conjunct so
+   it returns only `succ.CanReceiveDeletions() || IsAggregate() || IsKVIndex()` (it references
+   deleted symbols; "reduces to flag-off branch" cannot compile).
```

### §5.2 P1 golden fallout the tail re-run must schedule (H5)

Deleting the declared-key renderer reds two REAL `.contract` goldens
(`key_tc_witness.contract.opt.golden`, `key_multi_adorn_witness.contract.opt.golden`, which
pin `declared-key … inferred=…` lines). At the P1 tail, re-bless both to DROP the declared-key
line(s) (the SIP `inferred=` half is unreconstructable once @key is inert); re-add a
`declared=`-only line at P5 and re-bless again. Also: the ~24-golden P2 gate double-counts —
honest surviving set is **12 real** (join_1/merge_2/tc_nonlinear_diff ×4) **+ key_tc_witness's
4 region goldens re-blessed** from now-dangling symlinks (M2).

### §5.3 P2 — do NOT retype frozen_census; re-provide the anti-stub recount (H1/H2/M1)

- **H1:** keep `Context::frozen_census` as `const RegionalCensus *` (its reader
  Rel.cpp:4638-4662 reads a `RegionalCensus`; F12 keeps the census query-derived). Add a
  separate `frozen_regions` field only when a later phase needs the typed record.
- **H2 (the real teeth):** V-REGION-CENSUS at the Rel tail is FULLY tautological
  (`DeriveRegionalCensus(query)` vs a stored `DeriveRegionalCensus(query)` — all 7 fields, not
  just request_ports). The genuine anti-stub belt today is the in-`Build` recount
  (Planning.cpp:663-687) that reads the BUILT vectors — which P2 deletes. **P2 MUST re-provide
  an internal recount reading the NEW typed `RegionTemplate`** (`R.relation_schemas.size()`,
  `R.request_ports.size()`, `R.result_ports.size()`) against `DeriveRegionalCensus(query)`,
  replacing the deleted check. Exit-gate §3-P2 clause (3) is corrected: the anti-hollow-R
  referees are (a) the region goldens rendering FROM `R` and (b) this new internal recount —
  NOT the Rel-tail belt.
- **M1:** `query.PublicRowContracts()` does NOT retire the friend leak (`RowContractMap`/
  `RowContract`/`QueryViewImpl` are private lib types; exposing them on public `Query` is a
  WIDER leak). Keep the friend-class access (contained to lib/Regional, which already includes
  the private lib/DataFlow/Query.h) OR return an opaque pre-materialized `{decl, visible_fields,
  member_key}` vector by value. Drop the "retires the friend leak" claim.

### §5.4 P3 — root the permanent roots; fix the fact-identity projection; clarify the backend (B2/H3/M3/L5)

- **B2 (blocking):** `BuildRequestPorts` must ALSO `AddRequestEdge(PermanentRoot(id),
  CallSiteId(redecl), EmptyBindingState(ri))` with `RootAlive(PermanentRoot)≡true` — else the
  ~73% of corpus programs with no bound `#query` get `live={}` and publish nothing. Add a P3
  exit-gate probe: a no-bound-query program still publishes its full answer.
- **H3:** `AddDerivation`'s fact identity must project the row through the **positional**
  member-key (`declared_key_positions`, the F13 bridge), NOT raw `member_key` value-ids
  (RowContract.h: FieldId is "not an array index"). F29 and F13 share ONE positional projection.
- **M3:** P3 `EvaluateEpoch` LAYERS request/activation tracking over the RETAINED
  full-materialization backend (the landed induction fixpoint) — it does NOT replace codegen;
  the abstract topo-sweep evaluator is scoped to the acyclic/keyed slice. Add a recursive
  baseline probe.
- **L5:** `RouteResults` must filter by the request edge's requested relation (from
  CallSiteId/owner), or state consumers filter downstream — else a `#query` on p receives
  edge/q facts in the single-empty-state world.

### §5.5 P4 — discriminating exit gate (B3)

All four §3-P4 probes are non-discriminating (a no-op P4 on the plain post-P1 cursor passes;
the baseline cursor at Database.cpp:1768-1799 already emits a `NumRows` scan + key filter;
RoutedResult has no codegen surface at P4; the "caveat gone" probe targets a P1.5-deleted
line). Corrected gate: adopt D1(i) so the keyed scan carries terminal-BindingStateId VALUES,
then assert the emitted filter constant equals the bound value threaded through a
`ProgramTableScanRegion` — distinguishable by the region-cursor `s<id>` naming vs the
query-cursor `pos`/`_cursor` shape (Database.cpp:1768); move the 2nd-requester/RoutedResult
check to a `-region-out` compile-time assertion (not a datalog.h grep); re-anchor the
caveat-gone check to a line P4 actually changes. (The `ProgramTableScanRegion` reuse mechanism
is CERTIFIED sound — EmitScan with `index=nullopt` + non-empty `InputVariables` emits the
honest key filter at Database.cpp:3403-3418; the public handle is Program.h:1097-1140, its Impl
`ProgramTableScanRegionImpl` at lib/ControlFlow/Program.h:1601. The cross-phase refuter's
"that range is GroupUpdateImpl" claim was REFUTED — GroupUpdate is at Program.h:808. But the
P4 hunk must also populate `out_vars` — one VAR per table column, mirroring Join.cpp:268-270 —
or EmitScan's `bind_outputs` (Database.cpp:3349-3356) emits nothing and the body's free-column
refs are unbound; and it must document that IndexedColumns/InputVariables are populated for the
index-less FullScanFilter shape too, adding a codegen belt `IndexedColumns().size() ==
InputVariables().size()` since Program.h:1116/1128 say they're "empty if an index isn't used".)

### §5.6 P5 — the parser flip is a hard Phase-0 prerequisite (H6/M7)

- **H6 (empirically confirmed):** the order-free parser dup rejects fire at PARSE
  (Parser.cpp:974-995 same-decl + 1477-1488 cross-redecl, both sort), upstream of
  `InternDeclaredPaths`; `@key(A,B) @key(B,A)` never reaches P5 (the `order_converge` probe
  rejects at parse — ir-desired-states §7B). Move the order-free→order-significant flip of BOTH
  parser sites into **Phase-0 item 4** as an explicit P5 prerequisite; re-bless/retire
  `reject_key_double_1`'s order-free claim; record the RP-10 set-of-sets semantic reversal.
- **M7:** the F8 "non-prefix subsets absent" gate needs a compile-vs-runtime origin
  distinction (both intern into one order-free schema table). Assert over
  `MaterializePrefixChain`'s DECLARED output BEFORE any `EvaluateEpoch`, or tag schemas with a
  declared-vs-visited origin.

### §5.7 P6 — DRed is a per-fact REBUILD, not a per-BindingState mirror; retirement cascade-retracts (B4/B5/H7/M4/M5/M6/L6)

- **B4 (blocking):** DROP "mirror the landed per-row `C_nr`/`C_r` keyed per BindingState" —
  that makes a binding state a second fact owner. Specify DRed over the **per-FACT**
  `FactDerivation.support` in the single `RegionalFactRelation` (the recursive-vs-non-recursive
  support split lives ON the FactDerivation, aggregated per `fact_id`) — a real REBUILD of the
  counter model, an obligation, not a reuse of Stratum.cpp/Table.h.
- **B5/M6 (blocking):** the (A)-once-then-(B) split drops cross-component TRANSITIVE
  retraction (a C2 affected only via C1's retracted fact is covered by neither pass) and can
  settle on a fixed point ≠ fresh-from-committed. Make (A) the transitive closure over all
  affected SCCs, interleaved with (B) as ONE worklist carrying signed frontiers across
  component boundaries (mirroring the landed per-stratum frontier flow, Stratum.cpp:2447→2490,
  :1856-1869), OR wrap (A)+(B) in an outer repeat-to-fixpoint (the seed §3 single joint loop).
- **H7:** `RetireUnreachableSCCs` must CASCADE-RETRACT every `FactDerivation` sourced from the
  SCC's states (dropping per-fact support so membership dies and Publish emits removals) BEFORE
  freeing storage; re-type the guard to compare against the retracted-fact set, NOT routed
  removals (an interior helper `q` is never routed, so `assert scc.facts ⊆ drained` aborts).
- **M4:** `MessageSeamsOf` cannot use `ForEachInsertToSelectSeam` endpoints directly (both
  carry the same message decl). Resolve each endpoint to its producing/consuming RELATION, OR
  run the SCC over the DataFlow VIEW graph and project Stratify's condensation (the safer arm —
  guarantees agreement by construction, not by a DEBUG belt).
- **M5:** name ONE `BindingStateSchema` interner owner keyed on `(region, sorted field-set)`,
  available from P3's empty-schema site; P4's D1 pull-forward and P5 both call it (else the
  terminal id P4 mints diverges from P5's and the convergence gate fails).
- **L6:** `CommonPreservedPrefix`/`dest_prefix` have no order source (RuleRoutingProjection is
  a SET; promotion yields order-free classes). Define the ordering from the declared @key path
  order on the destination relation; make the "one frontier" structural check advisory (fall
  back to Joint) until the ordering is well-founded.

### §5.8 D1 — the pull-forward is a compile-time SCHEMA id, not a value-bearing id (H4)

D1(i)'s "minimal" pull-forward is incoherent as stated: a value-bearing `BindingStateId` is
runtime-only and embeds a `BindingStateSchemaId` whose interner is P5. Corrected: P4's request
port scopes to a compile-time `BindingStateSchemaId` (which bound fields); the value-bearing
`BindingStateId` is minted at runtime in `EvaluateEpoch`; P4 pulls the `BindingStateSchema`
interner (NOT the prefix DAG/BindingEdge) forward — or adopt D1 option (ii). This resolves
D1's "does not require P5's machinery" contradiction: P4 requires the schema interner (a
narrow slice of P5), not the DAG.

---

## §6. Session-14 re-grep — the P1 cut is STILL NOT compile-clean (7 new un-enumerated consumers)

A session-14 compile-gate refuter re-greped the WHOLE tree (lib/ include/ bin/ tests/) for
every P1-deleted symbol and diffed the hit set against §1 + §5.1. **VERDICT: NOT-CLEAN —
7 un-enumerated compile-breaking consumer groups survive the atomic cut.** Every one below was
**verified at tip by opening the file:line** (not asserted). These are REQUIRED additions to the
P1 atomic inventory; they are the s13→s14 analog of the B1 injector/census catch (a corrected
inventory is still incomplete — the pattern is that each pass finds the *sibling* TU the prior
pass's anchor range did not span). Add all seven to P1:

```diff
+ §6-1 Rel-IR dump emitter (analog of L2, which caught only the DataFlow renderer):
+   lib/Rel/Format.cpp:114-116 (DROpKindName string arms) + :865-903 (the three `.rel` render
+   case-groups kSubgraphInstantiate/kInstanceDeath/kInstanceSeal) + :1131-1132 (the census kind
+   array entries). All reference the P1.4-deleted Rel.h:148-159 enumerators. VERIFIED: the
+   render arms are at Format.cpp:865/885/897. Hunk: delete the three DROpKindName arms, the three
+   render case-groups, and the three census-array entries.
+ §6-2 Rel.cpp DROpStratum + key_of enumerator arms (outside every P1.4 range):
+   lib/Rel/Rel.cpp:4794-4795 (`case kSubgraphInstantiate:`/`case kInstanceDeath:` reading
+   flow.instance_stratum/instance_store_id) + :4807 (`case kInstanceSeal:`) — the DR-strata
+   derivation; AND :5363 (`if (op.kind==kInstanceSeal) return Key{...,11,...}` band-key tie-break
+   in key_of; the :5370 kSubgraphInstantiate/kInstanceDeath fall-through is only a comment). Both
+   sit outside the P1.4 ranges (census 3994-4021, V-INST 4283-4407, V-ALPHA 4413-4493). Hunk:
+   delete the three DROpStratum case arms (fold to the default 0u) + the kInstanceSeal band-11 arm.
+ §6-3 ControlFlow class-definition sibling TUs (P1.5 enumerated only the header + Database.cpp +
+   Build/*): the ProgramSubgraphInstanceRegion(Impl)/ProgramInstanceStore(Info) method defs +
+   visitor macros + render live in FOUR un-enumerated TUs:
+     lib/ControlFlow/Operation.cpp:152-155,507-543 (AsSubgraphInstance override + FROM_OP);
+     lib/ControlFlow/Program.cpp:233-256 (ProgramInstanceStore accessor defs), :414, :752-809;
+     lib/ControlFlow/Format.cpp:659-666,979 (operator<< render);
+     lib/ControlFlow/Visitor.cpp:38 (MAKE_VISITOR);
+     + the AsSubgraphInstance virtual in lib/ControlFlow/Program.h:577 (Impl) and :1170 (public).
+   Hunk: delete these defs + the virtual/override + the FROM_OP/MAKE_VISITOR lines.
+ §6-4 The retained-fn→deleted-fn LIVE CALL (the subtlest): lib/Regional/Planning.cpp:304-306 —
+   `CollectOriginInteriorDecls` (RETAINED by P1.6 as the Tier-2 authority) calls the P1.6-DELETED
+   `CollectDemandInteriorDecls(query)` to dedup Tier-1 decls out of the Tier-2 residue. A live call
+   to a deleted function INSIDE a kept function. VERIFIED at Planning.cpp:304. Hunk: delete the
+   :304-306 loop (post-cut there ARE no Tier-1 demand-interior decls, so the subtraction is empty).
+ §6-5 tests/DataFlowValidators/GuardAnnotationFoldTest.cpp (whole TU, :38-144) — consumes
+   GuardAnnotation / PromoteSurvivorToBody / CheckGuardAnnotationFold (all P1.6-deleted). P1.7
+   migrated only tests/OptDiff + the InstanceStore ctest; this DataFlowValidators unit TU breaks
+   the DataFlowValidators build. Hunk: delete the TU + its CMake/ctest registration.
+ §6-6 tests/RelValidators/{InstanceSolePub,InstanceEffects,InstanceOrder,InputArm,DeathFrontier}Test.cpp
+   — five dedicated unit TUs that build DROp(kSubgraphInstantiate/kInstanceDeath) and call the
+   P1.4-deleted validators (CheckInstantiateEffects/CheckInstanceOrder/CheckInstanceDeathFrontier/
+   CheckInstanceSolePub). P1.4's exit gate ASSERTS "RelValidators ctest passes with V-INST-* deleted"
+   but never DELETES these TUs — they break the RelValidators build. Hunk: delete the five TUs +
+   their CMake/ctest registration. (All six test files confirmed present at tip.)
```

### §6.0 Session-16 DRY re-grep — TWO MORE un-enumerated consumers (§6-7, §6-8), both in `bin/`

The session-16 grounding ran the symbol-driven `git grep -l` acceptance gate DRY across the WHOLE
tree at tip `6d6248a2` (code lines only — comment refs stripped), diffing the hit set against
§1 + §5.1 + §6-1..§6-6. **VERDICT: the `lib/` + `include/` + `tests/` inventory is now EXHAUSTIVE
(no 8th consumer there — every `lib`/`include`/`tests` code hit maps to an inventory file, incl.
`Procedure.cpp`/`Stratum.cpp` under P1.5's "Build/\*"). BUT the gate surfaced TWO compile-breaking
consumers in `bin/` — a build family the §1+§6 anchor set never spanned.** Both VERIFIED at tip by
opening the file:line. Add both to P1:

```diff
+ §6-7 bin/drlojekyll/Main.cpp — the CLI flag family + its call-site threads (the "delete the flags"
+   prose never named a file — exactly the omission the gate exists to catch):
+     :49-51  `static bool gDemand/gDemandInstance/gDemandRetract = false;` (the three flag globals);
+     :610,:617-618,:627-628  the `-demand`/`-demand-instance`/`-demand-retract` arg-parse arms
+       (`-demand-instance` sets gDemand+gDemandInstance; `-demand-retract` sets gDemand+gDemandRetract);
+     :76   `Query::Build(module, error_log, gPassPolicy, gDemand, gDemandRetract)` — drop the 2 args;
+     :113-114  `Program::Build(*frozen_opt, ..., gPassPolicy, gDemandInstance)` — drop gDemandInstance.
+   Hunk: delete the 3 globals + the 3 arg-parse arms; revert both Build calls to their pre-demand arity.
+ §6-8 bin/Oracle/Main.cpp:749-753 — THE 8TH CONSUMER, and the first in a DISJOINT BINARY (not even
+   libraried with drlojekyll): `drlojekyll-oracle` calls
+     `hyde::Query::Build(*module_opt, error_log, PassPolicy::DisableDataFlowOpt(),
+                         /*demand_mode=*/false, /*demand_retract=*/false, /*suppress_demand=*/true)`.
+   When P1 reverts `Query::Build` to its 3-arg form the `drlojekyll-oracle` TARGET fails to build.
+   Hunk: drop the three trailing args → `Query::Build(*module_opt, error_log, DisableDataFlowOpt())`.
+   BEHAVIOR-PRESERVING BY CONSTRUCTION: `suppress_demand=true` existed solely to defeat flagless
+   RP-6 `@key` activation so the oracle referees the FULL closure (Oracle:745-748 comment) — post-cut
+   there is no demand transform to suppress, so the oracle naturally referees the full closure. The
+   `suppress_demand` param itself is deleted from the signature (its only true-passing caller was here).
```

**Query::Build signature is the shared root (VERIFIED Query.h:1081-1086):** `Build(module, log, policy,
bool demand_mode=false, bool demand_retract=false, bool suppress_demand=false)` — P1 deletes all THREE
demand params → `Build(module, log, policy)`. Its COMPLETE non-comment caller set at tip is exactly
three lines: the definition (`lib/DataFlow/Build.cpp:2524`, in-inventory), `bin/drlojekyll/Main.cpp:76`
(§6-7), `bin/Oracle/Main.cpp:749` (§6-8). RefInterp/RefHarness mention `Query::Build` in COMMENTS only
(they parse raw clauses — no DataFlow), so they are NOT consumers. `DemandForcings()` (Query.h:1093)
and the `QueryDemandForcing` type delete with the accessor (B1 already routes the census off it).

### §6.0.1 The acceptance gate needs a code-vs-comment discriminator (7 stale-prose refs)

The DRY also found SEVEN files that reference a deleted symbol in a COMMENT only (not compile-breaking,
but the `git grep -l` gate as literally specified — "returns ONLY inventory files" — flags them as
false violations): `lib/DataFlow/Prov.cpp:83` (`demand__`), `include/drlojekyll/Regional/Regional.h:95`
(`demand__` example-dump string), `lib/DataFlow/Optimize.cpp:912` (`ApplyDemandTransform`),
`lib/DataFlow/IdentityJoin.cpp:154` + `lib/DataFlow/Link.cpp:223` (`IsCutSuccessorDR` — the code hits
in these two files are ALL comment; their real GuardAnnotation-CSE code excision is §1.1),
`include/drlojekyll/Runtime/Table.h:301` (`InstanceStore` double-buffer note),
`lib/DataFlow/Connect.cpp:270` (`// the single reader is ApplyDemandTransform`). These are a stale-
comment CLEANUP pass (update/delete the prose), NOT compile-blockers. Connect.cpp additionally deserves
a look: `proxy_view_to_decl`'s sole reader is the deleted `ApplyDemandTransform`, so the map plumbing
becomes dead — a follow-on cleanup, not compile-clean-blocking.

**GATE REFINEMENT (durable):** run the acceptance gate as `git grep -n <sym> -- 'lib/*' 'include/*'
'bin/*' | grep -vE ':[0-9]+:\s*(//|\*|/\*)'` (comment-line strip) and diff the FILE set against the
inventory. Retained parse-surface symbols (`HasInstanceKey`/`InstanceKeys`/`ForcingMessage`) and
test-DATA files (`.dr`/`.golden`) legitimately keep hits and belong on a documented allowlist; a
code-line hit outside {inventory ∪ allowlist} is the real violation.

**Method note for the owner (predict-then-verify precedent):** the recurring failure mode is an
inventory anchored by RANGE ("delete View.cpp:588-764") that silently omits a SIBLING TU holding
a *definition* or a *render arm* of the same deleted symbol. The durable fix is symbol-driven, not
range-driven: **before the P1 commit, the acceptance gate is `git grep -l` for each deleted symbol
returning ONLY files already in the inventory** — run it as the P1 pre-commit check, not a review
read. The `.rel`/DataFlow dump-emitter pair (§6-1 vs L2), the header-vs-definition-TU pair
(§6-3 vs P1.5), and now the **libraried-vs-disjoint-binary pair (§6-8: the Oracle target vs every
`lib` consumer)** are the three structural blind spots this makes mechanical.

### §6.1 B2–B5 internal-consistency confirmation (CERTIFIED against the seed §2/§3 algebra)

The session-14 gate also re-checked whether the B2/B3/B4/B5 amendments are internally consistent
with the seed §2 four-authority algebra and the retained `RegionalDataFlowCore.md` invariants.
**Verdict: CONSISTENT — no contradiction.** Load-bearing points:
- **B2** (permanent roots become `AddRequestEdge` owners with `RootAlive(PermanentRoot)≡true`)
  realizes the `RequestOwnerId::PermanentRoot` arm and keeps the forest ACYCLIC (a root→empty-state
  edge is a forest root, not a cycle). It is required BY the algebra: without it `EvaluateEpoch`
  yields `live={}` for the ~73% no-bound-query corpus, violating "deletion computes the SAME least
  fixpoint as fresh-from-committed."
- **B3** (region-cursor `s<id>` shape + `-region-out` RequestEdge/RoutedResult assertion) is a
  test-discrimination amendment orthogonal to the algebra; consistent with AccessPlan-as-physical-
  authority + label==emission.
- **B4** (per-FACT DRed over `FactDerivation.support`, recursive/non-recursive split ON the
  FactDerivation aggregated per `fact_id`) is EXACTLY the §2 razor: it upholds "RegionalFactRelation
  = SINGLE fact authority" + "BindingState is NOT a 2nd fact owner" + "counts are CACHES." It retains
  the landed C_nr/C_r SPLIT *concept* while rejecting per-BindingState *storage* — "mirror the
  algebra, not the storage."
- **B5** (make (A) the transitive closure over all affected SCCs interleaved with (B) as one
  signed-frontier worklist, or an outer repeat-to-fixpoint) is precisely what honors §2's
  "EvaluateEpoch = least fixpoint … order-independent; deletion == fresh-from-committed"; the
  rejected (A)-once-then-(B) split would violate that guarantee. H7's cascade-retract-before-free
  matches "drain routed removals BEFORE retiring an unreachable SCC."
- **One watch-point (not a contradiction):** §2 line 100 says a BindingState "owns FactDerivation
  ids" while B4 aggregates support per `fact_id`. The two coexist only if support aggregation keys on
  `RegionalFactId` (the single fact authority) — which B4 explicitly states ("aggregated per
  fact_id"). It lands on the correct side of the razor; keep the `FactDerivation.fact` key on
  `RegionalFactId`, never on `BindingStateId`.

**Net P1 green-light status:** the 5 blocking findings B1–B5 hold as folded, AND B2–B5 are
internally consistent — but P1 is NOT yet compile-clean until the §6 seven-consumer additions land
in the atomic inventory. The owner's green-light checklist is now: (1) fold §6-1..§6-6; (2) adopt
the symbol-driven `git grep -l` pre-commit acceptance gate; (3) the B1 injector/census + B2–B5
amendments already folded. Only then is the destructive cut compile-clean.
