<!-- Copyright 2026, Peter Goodman. All rights reserved. -->
# Session-35 seed — InstanceFlow after Phase-C step 1: the TABLE*→StateResourceId retype path, whole-program

> START-HERE for the next session. Written at the close of session 34 (branch
> `keyed-instances`, tip `fc268683`). Session 34 landed TWO things, both green
> (OptDiff **SUITE PASS 227** byte-identical + ctest **5/5**):
> **(A)** the resources-first **MaterializationPlan** RESOURCE authority (a
> codegen-byte-identical OBSERVER on `QueryImpl::materialization`, cross-checked
> against the real `view_to_model` allocation — `CrossCheckMaterialization`), and
> **(B) Phase-C step 1**: the first Rel object (`DRTable`) to carry a
> materialization-map identity beside its `TABLE*` — a cross-checked
> `StateResourceId resource`, stamped at `BuildDRInventory` via
> `MaterializationPlanOf(query)`, belt-guarded by `V-REL-RESOURCE`. Both are
> SHADOWS (consumed by nothing). Authority for (A): `session-34-seed.md` §2.5 +
> `session-34-grounding.md`.

## §0 Where we are — do NOT re-litigate

- **The map exists and is proven.** `ResourceForView(v) = authority(EquivalenceSetId(v))`
  is total and single-valued (V-MAT-BIJECTION: one authoritative `StateResource`
  per stateful physical class; co-recursive shared stores resolve through one
  canonical authority + `ForwardingAlias`). `DRTable.resource` is the first
  consumer of that map inside Rel. Belt-verified live.
- **The destination (owner-ratified, s33/s34):** the BIG UNIFICATION via
  InstanceFlow-the-separate-IR; `Rel` refers to `StateResourceId`/`ArrangementId`
  instead of `TABLE*` (InstanceFlow.md §11.2). Phase C is the megaproject that
  gets us there; Phase D (the first codegen move) is strictly after it.
- **The TRUE blocker (named s33, unchanged):** runtime-resource allocation is
  INTERLEAVED into `lib/ControlFlow/Build/*` (~8272 LOC) and Query-driven — tables
  by `FillDataModel`, indexes by `GetOrCreateIndex` INLINE mid-region-build. Rel
  is a POST-HOC SHADOW census (`BuildDRInventory` runs DURING region construction,
  after allocation). Nothing downstream of the allocation inversion moves until it
  lands.

## §1 Whole-program pseudocode (current architecture, tip `fc268683`)

```
# ============ DataFlow: Query::Build (lib/DataFlow/Build.cpp) ============
Query::Build(module, log, policy):
  ... parse -> build -> Optimize -> Stratify -> FinalizeColumnIDs ...
  IdentifyInductions(impl)                      # :2633  induction_info
  TrackDifferentialUpdates(impl)                # :2640  CanReceive/ProduceDeletions
  BuildEquivalenceSets(impl)                    # :2645  view.EquivalenceSetId()
  impl->row_contracts = InferConservativeRowContracts(impl)          # :2654
  impl->instance_flow = BuildFlatInstanceFlow(Query(impl))           # :2664  the grove
  impl->materialization = PlanResources(Query(impl), instance_flow)  # :2675  s34 (A)
  #   -> one StateResource per stateful EquivalenceSetId class (bijection);
  #      collection authorities + internal residues + forwarding aliases.
  return Query(impl)                             # the FROZEN graph (immutable hereafter)

# ============ ControlFlow: Program::Build (lib/ControlFlow/Build/Build.cpp) ======
Program::Build(frozen, first_id, policy):
  query = frozen.DataFlowGraph()                 # :1245  the same frozen QueryImpl
  BuildDataModel(query, program)                 # :1363  one DataModel per view,
  #   union-find'd by view.EquivalenceSetId()  -> view_to_model classes ARE the
  #   EquivalenceSetId partition. NO table yet.
  FillDataModel(query, program, context)         # :1397  R1-R9 TABLE-need rules:
  #   TABLE::GetOrCreate(view) gives that view's CLASS a `TABLE`. The ONLY table
  #   creation site. => the stateful class set.
  CrossCheckMaterialization(query, {EqSetId(v): table-backed v})     # :1413  s34 (A) belt
  BuildEntryProcedure / BuildIOProcedure / per-SCC region construction:  # :1422+
    #   THE INTERLEAVING: the eager walk + BuildStratumPhases mint regions AND
    #   call GetOrCreateIndex(col_indices) INLINE where a seek/join/scan needs an
    #   index (Data.cpp:205, Build.h:443, Join.cpp:272/408, Build.cpp:432/490/526).
    #   Index allocation is a SIDE EFFECT of emission, not a planned decision.
    BuildStratumPhases(...):
      dr_flow = BuildDRInventory(impl, context, query, recursive_sccs)  # Stratum.cpp:2149
      #   POST-HOC SHADOW census over ALREADY-allocated tables:
      #     for TABLE* table in impl->tables:            # Rel.cpp:1385 (map :1374)
      #       DRTable t; t.model = table
      #       t.differential = TableIsDifferential(table)   # CanProduce ∨ agg/kv
      #       t.member_views = table->views
      #       t.resource = eqset_to_resource[member_views[0].EquivalenceSetId()]  # s34 (B)
      #       V-REL-RESOURCE: all member_views share one class; class has one resource
      #     ... DRBranch / DRJoin / DROps (each carrying raw TABLE* fields:
      #         fold_table, negate_table, negated_table, pred_table, product_table,
      #         side_tables, fire_table, chain_*, gate_table, ingest_table, agg_table,
      #         input_table, seed_*, read/write/value/counter_table — ~44 in Rel.h) ...
      context.dr_flow = dr_flow                    # Stratum.cpp:2290
      LowerDRFlow / LowerDRRounds / LowerCommitSweeps(impl, context, dr_flow, ...)
      #   Stratum.cpp:1442+ — turn DR ops into ControlFlow regions; the Emit*
      #   templates read the ops' raw TABLE* fields to emit table ops/indexes.
  ProgramImpl::Optimize(...)                       # region flatten/dedup
  return program                                   # -> C++ codegen (Database.cpp)
```

Key truths (all verified this session):
- `impl->tables` exists ONLY because `FillDataModel` ran; every `TABLE*` is one
  stateful `EquivalenceSetId` class; `table->views` are that class's registered
  feeders. So `DRTable.resource` is derivable with no new state.
- The DR ops' ~44 `TABLE*` fields are ALL base-table (resource) identities — none
  is an index/arrangement identity (indexes live on `PlanNode::bound_cols` +
  `GetOrCreateIndex`, minted inline). So a `TABLE*→StateResourceId` retype is the
  base-table half; arrangements are a SEPARATE later concern.
- BuildDRInventory is a SHADOW census (Rel does not decide allocation; it reads it).

## §2 The path forward as DIFFS (Phase C, smallest-first, each its own gate)

### Step 1 — LANDED (s34): DRTable.resource shadow (above).

### Step 2 (RECOMMENDED NEXT, REAL, one-session): extend the retype across the DROp base-table fields + make it VISIBLE + goldened

```diff
  class DROp {                          # and the per-op variant structs
-   TABLE *fold_table; TABLE *negate_table; ... (the ~44 base-table fields)
+   TABLE *fold_table; ...              # KEEP for now (shadow step)
+   # NEW parallel resource ids, one per base-table field, stamped at the SAME
+   # BuildDRInventory site the TABLE* is set, via the table->DRTable->resource
+   # map (build a TABLE*->StateResourceId index once from flow.tables):
+   StateResourceId fold_resource; StateResourceId negate_resource; ...
+ # V-REL-OP-RESOURCE belt: every non-null TABLE* field has a resolved resource
+ #   whose class == the field table's EquivalenceSetId. (Reuses the table->resource
+ #   map DRTable already carries — O(tables) precompute, O(1) per field.)
+ # RENDER in -rel-out: each `args: table=%table:N` op line gains ` resource=sr#K`
+ #   (predict-then-verify; re-bless the 10 .rel goldens — ADDITIVE token, all
+ #   other bytes identical; codegen .h/.ir/.stdout/behavioral BYTE-IDENTICAL).
```
WHY real: it extends the resource map from the ONE `DRTable` object to the WHOLE
Rel op model (the concrete "Rel refers to StateResourceId" §11 goal), and the
goldened `-rel-out` render makes the coverage a pinned, reviewable artifact. Still
a SHADOW (nothing consumes the ids for emission), so codegen stays byte-identical.

### Step 2b (bundle or separate): reconcile the `support` token with the physical table

```diff
- PlanResources support = differential iff any class member CanReceiveDeletions()
+ support = differential iff any class member (CanProduceDeletions() ∨ IsAggregate()
+           ∨ IsKVIndex())            # == TableIsDifferential's notion, view-side
+ # THEN add to V-REL-RESOURCE: resource.support == (DRTable.differential ? diff : mono)
+ #   — a REAL cross-check tying the two authorities' differential-flavor notions.
+ # May move the 5 .materialization goldens (re-bless); measure first.
```
The current `support` copies the Regional row-contract convention
(`CanReceiveDeletions`), which is a DIFFERENT notion from `TableIsDifferential`
(`CanProduceDeletions ∨ agg/kv`). Aligning them lets the retype belt cross-check
support equality — teeth the resolution-only belt lacks today.

### Step 3 — the allocation inversion (THE MEGAPROJECT, multi-session)

```diff
- Program::Build allocates tables (FillDataModel) + indexes (GetOrCreateIndex INLINE
-   during region construction), then BuildDRInventory SHADOWS the result.
+ A Rel authority decides ALL runtime resources from the materialization plan
+   BEFORE region construction:
+     AllocateRuntimeResources(materialization):   # the ONLY allocation site
+       for resource in plan.resources: mint its TABLE (or successor) up front
+       for arrangement in plan.arrangements: mint its index up front   # needs §2.5.3
+   ControlFlow region builders then LOWER (consume ids; never GetOrCreateIndex).
- delete: BuildDRInventory's TABLE* walk; the inline GetOrCreateIndex sites; the
-   Query-shape join/branch rediscovery (Join.cpp BuildJoin + Rel.cpp re-derivation).
```
This needs the ARRANGEMENT half (the §2.5.3 `collect_arrangement_requirements`
gap — no standing index-requirements pass today; requirements only materialize at
the inline `GetOrCreateIndex` call sites). So arrangements must be DERIVED before
this lands. Step 2/2b do not need arrangements; Step 3 does.

### Step 4 — Phase D, the FIRST codegen move (past the blocker). Not this arc yet.

## §3 The recommended next REAL step (one-session, byte-identical-except-.rel)

**Do Step 2 (+ Step 2b if grounding shows it's clean).** Concretely:
1. Precompute `TABLE* -> StateResourceId` once in `BuildDRInventory` (invert
   `flow.tables`: `table->resource`), right after the DRTable loop.
2. At each op-mint site that sets a base-table field, ALSO stamp the parallel
   `StateResourceId` from that map; assert resolution (V-REL-OP-RESOURCE).
3. Render ` resource=sr#K` after ` table=%table:N` on the `.rel` op lines; predict
   the 10 `.rel` golden deltas FIRST, then re-bless (additive-token permutation
   check — see `permcheck.py` / the E-K5-PAD discipline).
4. Gate: codegen goldens (.h/.ir/.stdout/oracle/monotone/behavioral) BYTE-IDENTICAL;
   only `.rel` goldens move; belt quiescent corpus-wide + belt-verified live; ctest 5/5.

This is REAL: it makes the entire Rel op model resource-addressable and goldens the
proof — the last brick before the allocation inversion can retype the ops to
CONSUME ids (Step 3). If grounding finds Step 2 too thin, the fallback with more
teeth is Step 2b's support cross-check, or scoping the arrangement derivation
(§2.5.3) so Step 3 is unblocked.

## §4 Anchors (re-verify at next tip — the pipeline drifts)

| Fact | Source (tip fc268683) |
|---|---|
| Resource authority + bijection | `lib/DataFlow/Materialization.{h,cpp}` (`PlanResources`, `DeriveStatefulClasses`) |
| The map accessor (Query friend) | `MaterializationPlanOf(Query)` — `Materialization.cpp`; decl `Materialization.h`; friend `include/drlojekyll/DataFlow/Query.h` |
| Step-1 retype (DRTable) | `lib/Rel/Rel.h:400` (`DRTable::resource`); map `Rel.cpp:1374`, mint+belt `Rel.cpp:1385` (`V-REL-RESOURCE`) |
| The ~44 Rel `TABLE*` fields | `lib/Rel/Rel.h` (grep `TABLE *\*`); DROp variants §1.4+ |
| BuildDRInventory (shadow census) | `lib/Rel/Rel.cpp:1363` (`BuildDRInventory`), called `lib/ControlFlow/Build/Stratum.cpp:2149` |
| DR lowering (reads TABLE*) | `LowerDRFlow` `lib/ControlFlow/Build/Stratum.cpp:1442`+ |
| Table allocation | `FillDataModel` `Build.cpp:37/1397`; `BuildDataModel` `Build.cpp:233/1363` |
| Index allocation (inline, the gap) | `DataTableImpl::GetOrCreateIndex` `lib/ControlFlow/Data.cpp:348`; call sites `Data.cpp:205`, `Build.h:443`, `Join.cpp:272/408`, `Build.cpp:432/490/526` |
| `TableIsDifferential` | `lib/ControlFlow/Build/Build.cpp:673` (CanProduceDeletions ∨ agg/kv) |
| `.rel` goldens (would move on render) | `tests/OptDiff/goldens/*.rel.opt.golden` (10) |
| Program::Build sequence | `lib/ControlFlow/Build/Build.cpp:1245-1440` |
| Vision / §11 (Rel after InstanceFlow) | `docs/proposals/InstanceFlow.md` §10, §11, §16 |
| s34 grounding + record | `session-34-grounding.md`; CLAUDE.md "InstanceFlow" section; memory `regional-dataflow-core-epoch` |

## §5 Open questions carried forward
- **Arrangement derivation (§2.5.3):** no standing `collect_arrangement_requirements`
  pass; Step 3 needs one. Options (i) pre-pass replicating the inline index logic,
  (ii) post-Program read (cross-check only), (iii) defer. Settle when Step 3 nears.
- **`support` reconciliation (Step 2b):** CanReceiveDeletions vs TableIsDifferential
  — measure golden impact before adopting.
- **Discriminating kBoundQueryRead witness** (from s33): still not in corpus; add if
  the arm ever becomes load-bearing.
