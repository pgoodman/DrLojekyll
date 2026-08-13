<!-- Copyright 2026, Peter Goodman. All rights reserved. -->
# Session-36 seed — InstanceFlow after Phase-C step 2: the ARRANGEMENT derivation, whole-program

> START-HERE for the next session. Written at the close of session 35 (branch
> `keyed-instances`, tip `c8ed5bd0`). Session 35 landed **Phase-C step 2**, green
> (OptDiff **SUITE PASS 227** codegen byte-identical + ctest **5/5**): the WHOLE
> Rel op model is now resource-addressable via `DRFlowGraph::table_to_resource`
> (a `TABLE*→StateResourceId` index), rendered in `-rel-out` (` resource=sr#K`)
> and belt-guarded (`V-REL-OP-RESOURCE`). It is a codegen-byte-identical OBSERVER
> — honestly a regression fence + goldened coverage artifact whose ONE
> forward-load-bearing output is the map itself. Authority for step 2:
> `session-35-grounding.md`; the forward plan: `session-35-arrangement-scope.md`.

## §0 Where we are — do NOT re-litigate

- **The RESOURCE (table) half is DONE.** Resources are derived + cross-checked
  (s34 `MaterializationPlan`, `PlanResources`/`CrossCheckMaterialization`),
  op-addressable (s35 `table_to_resource`), and rendered/goldened. The
  `TABLE*→StateResourceId` map is total (every `impl->table` has a resource) and
  single-valued (V-MAT-BIJECTION).
- **The ARRANGEMENT (index) half does NOT exist.** There is NO derivation of the
  index-requirement set from the graph — indexes materialize ONLY as a side
  effect of emission, at six inline `GetOrCreateIndex` sites. `-materialization-out`
  prints a HARDCODED `arrangements=0` (`lib/DataFlow/Format.cpp:2066`).
- **The TRUE blocker (named s33, unchanged):** runtime-resource allocation is
  INTERLEAVED into `lib/ControlFlow/Build/*` and Query-driven — tables by
  `FillDataModel`, indexes by `GetOrCreateIndex` INLINE mid-region-build. Rel is
  a POST-HOC SHADOW census. The allocation inversion (Step 3) needs BOTH halves
  derived up front; the arrangement half is the missing input.
- **The destination (owner-ratified s33/s34):** the BIG UNIFICATION —
  InstanceFlow-the-separate-IR; Rel refers to `StateResourceId`/`ArrangementId`
  instead of `TABLE*`/inline indexes (InstanceFlow.md §11). Phase C gets us
  there; Phase D (first codegen move) is strictly after.

## §1 Whole-program pseudocode (current architecture, tip `c8ed5bd0`)

```
# ============ DataFlow: Query::Build (lib/DataFlow/Build.cpp) ============
Query::Build(module, log, policy):
  ... parse -> build -> Optimize -> Stratify -> FinalizeColumnIDs ...
  IdentifyInductions / TrackDifferentialUpdates / BuildEquivalenceSets
  impl->row_contracts   = InferConservativeRowContracts(impl)
  impl->instance_flow   = BuildFlatInstanceFlow(Query(impl))         # the grove
  impl->materialization = PlanResources(Query(impl), instance_flow)  # s34 (A)
  #   -> RESOURCES: one StateResource per stateful EquivalenceSetId class
  #      (bijection), collection authorities + internal residues + aliases.
  #      ARRANGEMENTS: EMPTY (the gap this session addresses).
  return Query(impl)                             # FROZEN graph

# ============ ControlFlow: Program::Build (lib/ControlFlow/Build/Build.cpp) ======
Program::Build(frozen, first_id, policy):
  query = frozen.DataFlowGraph()
  BuildDataModel(query, program)                 # :1363  one DataModel per view,
  #   union-find'd by EquivalenceSetId. NO table yet.
  FillDataModel(query, program, context)         # :1397  R1-R9 TABLE-need rules;
  #   TABLE::GetOrCreate(view) — the ONLY table creation site (bar the empty-
  #   query table at :517, minted AFTER inventory). Also mints a keyed model's
  #   key index INLINE (Data.cpp:205 GetOrCreateIndex).
  CrossCheckMaterialization(query, {EqSetId(v): table-backed v})     # :1413 belt
  per-SCC region construction (BuildEntryProcedure -> BuildStratumPhases):
    #   THE INTERLEAVING: the eager walk + BuildStratumPhases mint regions AND
    #   call GetOrCreateIndex INLINE where a seek/join/scan needs an index
    #   (Build.h:443 partial scan, Join.cpp:272/408 pivots, Build.cpp:432/490/526
    #   query entry points). Index allocation is a SIDE EFFECT of emission.
    BuildStratumPhases(...):
      dr_flow = BuildDRInventory(impl, context, query, recursive_sccs) # :2149
      #   POST-HOC SHADOW census over ALREADY-allocated tables:
      #     for TABLE* table in impl->tables:            # Rel.cpp:1524+
      #       DRTable t; t.model=table; t.differential=TableIsDifferential(table)
      #       t.member_views=table->views
      #       t.resource = eqset_to_resource[views[0].EquivalenceSetId()]  # s34(B)
      #     flow.table_to_resource[t.model] = t.resource   # s35 step 2, Rel.cpp:1584
      #     ... DRBranch / DRJoin / DROps (raw TABLE* base-table fields) ...
      #     ValidateOpResources(flow)     # s35 V-REL-OP-RESOURCE, Rel.cpp:2669
      #        every non-null base-table field resolves via table_to_resource;
      #        class cross-check vs the partner view where one exists.
      LowerDRFlow / LowerDRRounds / LowerCommitSweeps
      #   Emit* templates read the ops' raw TABLE* fields (NOT the resource ids)
      #   to emit table ops + read the inline-minted indexes.
  ProgramImpl::Optimize(...)
  return program                                   # -> C++ codegen

# ============ The index universe TODAY (the gap) ============
# DataTableImpl::indices : DefList<TABLEINDEX>       # Program.h:137
#   each TABLEINDEX has a `column_spec` (sorted-unique col set, Data.cpp:357)
# GetOrCreateIndex(table, col_indexes):              # Data.cpp:348 — the ONLY
#   SortAndUnique(col_indexes); dedup by column_spec; else mint a TABLEINDEX.
#   Called from 6 inline emission sites. NO standing requirement set exists.
```

Key truths (verified s35):
- Every non-null base-table `TABLE*` an op holds is in `impl->tables`
  (single `GetOrCreate` choke-point; inventory runs before the empty-query
  table). So `table_to_resource` is total over op fields.
- The op TABLE* fields are the BASE-TABLE (resource) half; indexes are a
  SEPARATE universe (`TABLEINDEX` on `column_spec`, never a `TABLE*`).
- Rel is a SHADOW census: it reads allocation, never decides it.

## §2 The path forward as DIFFS — the arrangement derivation (Step 3 unblocker)

Mirror the s34→s35 cadence (observer → cross-check → invert). Three stages, each
its own gate; A is a codegen-byte-identical observer, C is the inversion.

### Stage A (RECOMMENDED NEXT, REAL, one-session): the arrangement CENSUS

```diff
  struct MaterializationResources {
    std::vector<StateResource> resources;
    std::vector<ForwardingAlias> aliases;
+   std::vector<Arrangement>    arrangements;   # NEW (empty until Stage A)
  }
+ struct ArrangementId { uint32_t v; ... };     # typed id, Identity.h idiom
+ struct Arrangement {
+   ArrangementId id;
+   StateResourceId resource;      # the owning table's resource (reuse s35 map!)
+   std::vector<unsigned> columns; # the index column_spec (sorted, canonical)
+ };
+ # A POST-Program observer (peer of CrossCheckMaterialization): walk the real
+ # impl->tables[*]->indices, tag each by table_to_resource[table], emit one
+ # Arrangement per (resource, sorted column-set), deterministic id order
+ # (by StateResourceId then ascending column-set).
- os << ... << " arrangements=0\n";              # Format.cpp:2066 HARDCODED
+ os << ... << " arrangements=" << plan.arrangements.size() << "\n";
+ # render each: `  ar#K resource=sr#R columns=(...)` — the sr#R cross-refs the
+ # resources block + the -rel-out resource token (ONE id space).
+ # bless the 5 .materialization goldens (additive; MOST are 0->N arrangements).
```
WHY real: it makes the currently-INVISIBLE index set a DERIVED, id-addressed,
goldened artifact — the arrangement analog of the s34 resources slice. It REUSES
`table_to_resource` (the s35 map's first real consumer). Still an OBSERVER
(nothing drives allocation) ⇒ codegen byte-identical. NOTE: this reads the real
`indices` post-Program, so it is a lib/ControlFlow-side census (like
`CrossCheckMaterialization`), NOT the pure DataFlow-side `PlanResources`.

### Stage B (next): the PURE `collect_arrangement_requirements` + cross-check

```diff
+ # A pure-QueryView-API derivation (the DeriveStatefulClasses precedent, NO
+ # lib/ControlFlow dep) that REPLAYS the six sites' column logic from the FINAL
+ # graph:
+ #   - keyed-model key columns          (Data.cpp:205 rule)
+ #   - per bound #query, the SIP bound subset (Build.cpp:432 — P7 SelectAccessPlan)
+ #   - per pivot JOIN side, pivot columns     (Join.cpp:272/408 rule)
+ #   - per interior partial scan, bound subset (Build.h:443 — P7b AccessPlan)
+ std::set<(ResourceId, ColumnSet)> collect_arrangement_requirements(query);
+ # CROSS-CHECK byte-for-byte against Stage A's census (the falsifiable claim,
+ # the CrossCheckMaterialization analog): derived == real, abort on divergence.
```
This is where P7/P7b/P9 (landed/deferred access-path analyses) finally earn a
consumer. The cross-check is the REAL teeth Stage A's census lacks.

### Stage C — the inversion (the megaproject payoff, multi-session)

```diff
+ AllocateRuntimeResources(materialization):     # the ONLY allocation site
+   for r in plan.resources:     mint its TABLE up front
+   for a in plan.arrangements:  mint its index up front
- FillDataModel + the 6 inline GetOrCreateIndex sites
+ region builders LOOK UP (resource,col-set) -> ArrangementId -> index handle
- delete: BuildDRInventory's TABLE* walk; the inline GetOrCreateIndex; the
-   Query-shape join/branch rediscovery.
```
Needs Stages A+B. Then Phase D (first codegen move) rides on it.

## §3 The recommended next REAL step (one-session, byte-identical-except-.materialization)

**Do Stage A** (the arrangement census). Concretely:
1. Add `ArrangementId` (typed id) + `Arrangement` struct + the `arrangements`
   vector on `MaterializationResources` (or a sibling plan object — DECIDE in
   grounding: DataFlow-side plan vs a ControlFlow-side census struct, since the
   real `indices` live in lib/ControlFlow).
2. Build the census as a POST-`Program::Build` observer (peer of
   `CrossCheckMaterialization`): walk `impl->tables[*]->indices`, tag by
   `table_to_resource`, emit deterministic `Arrangement`s.
3. Render in `-materialization-out` (replace the hardcoded `arrangements=0`);
   predict the 5 `.materialization` golden deltas FIRST, then re-bless (additive).
4. Gate: codegen goldens (.h/.ir/.stdout/oracle/monotone/behavioral) +
   `.rel`/`.contract`/`.region`/`.df` BYTE-IDENTICAL; only `.materialization`
   goldens move; belt/census live-verified; ctest 5/5.

DECISION POINTS for grounding (name them, don't assume):
- WHERE the census lives (the `indices` are lib/ControlFlow — so is it a
  `MaterializationResources.arrangements` populated late, or a new
  ControlFlow-side struct cross-referencing sr# ids?).
- WHETHER Stage A alone is "real enough" or must bundle Stage B's cross-check to
  avoid a hollow shadow (the s35 lesson: a census with no derivation is thin;
  Stage B's cross-check is the teeth — consider bundling A+B if grounding shows
  A alone is a shadow-for-shadow).

## §4 Anchors (re-verify at next tip — the pipeline drifts)

| Fact | Source (tip c8ed5bd0) |
|---|---|
| s35 map | `DRFlowGraph::table_to_resource` `lib/Rel/Rel.h:939`; populate `Rel.cpp:1584` |
| s35 belt | `ValidateOpResources` `lib/Rel/Rel.cpp:1381`; call `Rel.cpp:2669` |
| s35 render | `res`/`tidr` `lib/Rel/Format.cpp:408/415`; 18 args-line sites |
| Resource authority | `lib/DataFlow/Materialization.{h,cpp}` (`PlanResources`, `DeriveStatefulClasses`) |
| Materialization dump (arrangements=0 HARDCODED) | `lib/DataFlow/Format.cpp:2066` |
| CrossCheckMaterialization (the census precedent) | `Build.cpp:1413`; friend on `Query.h` |
| Index universe | `TABLEINDEX` on `DataTableImpl::indices` `lib/ControlFlow/Program.h:137`; `column_spec` `:100` |
| GetOrCreateIndex (the gap) | `lib/ControlFlow/Data.cpp:348`; 6 sites: `Data.cpp:205`, `Build.h:443`, `Join.cpp:272/408`, `Build.cpp:432/490/526` |
| Table allocation | `FillDataModel` `Build.cpp:37/1397`; `BuildDataModel` `Build.cpp:233/1363` |
| BuildDRInventory | `lib/Rel/Rel.cpp:1506`; called `Stratum.cpp:2149` |
| Access-path analyses (Stage-B inputs) | P7 `SelectAccessPlan` (RegionInstance.h), P7b `AccessPlan` (Program.h) — see CLAUDE.md RegionalDataFlowCore notes |
| Vision §10/§11/§12 | `docs/proposals/InstanceFlow.md` (`plan_materialization` :1038, `ArrangementSpec` :936, `BuildRel` :1065) |
| s35 record | `session-35-grounding.md`, `session-35-arrangement-scope.md`; CLAUDE.md InstanceFlow §; memory `regional-dataflow-core-epoch` |

## §5 Open questions carried forward
- **`support` reconciliation (Step 2b, DEFERRED):** materialization `support`
  (OR `CanReceiveDeletions` over ALL class views) vs `TableIsDifferential`
  (OR `CanProduceDeletions ∨ agg/kv` over `table->views`). Diverges on agg/KV
  (measured s35); a `resource.support == DRTable.differential` cross-check needs
  the VIEW-SET-EQUALITY premise proven first (all-class-views vs table->views).
- **Discriminating kBoundQueryRead witness** (s33): still not in corpus.
- **P9 firewall relaxation:** the access-path inference is consumer-less until
  Stage B reads it — Stage B is its long-awaited consumer.
