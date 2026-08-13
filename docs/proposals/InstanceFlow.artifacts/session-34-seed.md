<!-- Copyright 2026, Peter Goodman. All rights reserved. -->
# Session-34 seed — InstanceFlow after CP2: the big-unification path, blocker named

> START-HERE for the next session. Written at the close of session 33 (branch
> `keyed-instances`, tip `693717f1`). CP2 is LANDED: the flat empty-context grove
> dump is COMPLETE (node `role=`, family `root=none`, the kBoundQueryRead coverage
> arm) and regression-pinned by 4 `.instanceflow` goldens. It remains a
> codegen-byte-identical OBSERVER. **Owner ratified this session: the destination is
> the BIG UNIFICATION** (demand + keyed + multi-adornment + carried-column elision
> all lowering from one family vocabulary, §20-E), via InstanceFlow as a SEPARATE
> derived IR — NOT nested/annotated dataflow (which re-tangles the physical/logical
> split-brain that P1 deleted; §1 verbatim + the fork discussion below). The
> near-term join-pivot codegen win was ALREADY harvested by P7/P7b/P7c outside this
> chain, so InstanceFlow's payoff is the UNIFICATION, not a quick perf win — accept
> the consumer-less runway.

## §0 Where we are (session-33 decisions — do NOT re-litigate)

- **Direction (owner, s33):** big unification via InstanceFlow-the-separate-IR. The
  "why a new IR / why not nested dataflow" question was raised + answered: nesting
  forces a containment tree where the real structure is a maximal-sharing coverage
  DAG with exactly-one-emission, and context-on-dataflow re-creates the annotate-Query
  split-brain P1 deleted. Recorded in `session-33-grounding.md` + memory.
- **CP2 LANDED (`71d64004` + `693717f1`):** role/root topology + bound-query-read
  coverage + 4 goldens. Gate GREEN: OptDiff **SUITE PASS (227) byte-identical**
  (observer proof), 4 new `.instanceflow` goldens (belt-verified live), ctest 5/5,
  all 5 V-IF validators quiescent corpus-wide.
- **RR1 SETTLED (owner + panel):** flat family `root_use` stays `std::optional`,
  renders `root=none` (the seeded plural `roots=` contradicted §6 `root_use`
  singular / §16 `root=u#41` — panel CLAIM-2 F2). Node `role=root|interior`, root =
  emission/read OUTPUT obligation (INSERT writer / bound-read target); candidate
  join/agg CONTEXTS stay the separate `seeds` catalog, never node roles. `role=input`
  (a Phase-D port concept) deferred — the flat grove reifies no inter-family ports.

## §1 The panel-adjudicated line-of-sight (session-33 grounding, all 3 claims REFUTED)

The Phase-D "first codegen move" is REAL but GATED behind a genuine multi-session
prerequisite. This is the load-bearing finding — do not mis-sell a shadow slice as
"a step toward codegen."

- **Phase C is XL, cross-library.** `TABLE*` is baked into Rel's object model (144
  occurrences; `lib/Rel/Rel.h:24` hard-includes ControlFlow `Program.h`). Rel is a
  POST-HOC SHADOW census: `BuildDRInventory` (`Rel.cpp:1363-2452`, ~1100 LOC) walks
  ALREADY-allocated ControlFlow tables (`for (TABLE *table : impl->tables)`), and
  runs LAST (`Procedure.cpp:749`), after every `TABLE*`/index was allocated inline
  during region construction (`GetOrCreateIndex` at `Join.cpp:272/408`, `Build.h:443`).
- **THE TRUE FIRST BLOCKER (name it, own it):** runtime-resource allocation is
  INTERLEAVED into `lib/ControlFlow/Build/{Join,Induction,Negate,Product,Insert,
  Procedure}.cpp` (~8272 LOC) and Query-driven. Phase D's `SortedSectionProbe`/
  `SortedBatchMerge` (§11.4 `AccessDecision`) can only DRIVE emission once a Rel
  authority OWNS the access decision — which needs allocation MOVED OUT of ControlFlow
  region construction to AFTER Rel decides (Phase B item 5 / Phase C exit gate, §20
  L1886/L1889). That inversion is the megaproject; nothing downstream of it moves
  until it lands.
- **Phase B splits at item 4** (draw the line here):
  - Items 1-3 (flat MaterializationPlan as a certified OBSERVER; abstract resource/
    arrangement IDs; layouts == current runtime requirements) are ONE-SESSION
    reachable via the established shadow-pass pattern (Phase A / P6.3 / P7b). M-size.
  - Items 4-5 (Rel consumes IDs not `TABLE*`; allocation moves to ControlFlow) ARE
    the blocker above — they belong WITH Phase C. XL, multi-session.
- **Three of six §11.2 deletion bullets are already vestigial** (demand-subgraph
  recognition is DataFlow-side in `RecognizedSubgraph`, not Rel; `DRInstance`/
  `demand_table` at `Rel.h:696/850` are reserved-unpopulated). The two REAL bullets
  (`TABLE*` object model; table identity from equivalence sets) each independently
  touch most of `lib/Rel` (6097 LOC) AND most of `lib/ControlFlow/Build` (8272 LOC).

## §2 The path forward as DIFFS (post-CP2)

### Next slice (RECOMMENDED): Phase-B items 1-3 as a certified OBSERVER — MaterializationPlan shadow

Honest labeling: this is a SHADOW (codegen byte-identical; nothing consumes it yet),
NOT a codegen move. It is the first real step of the big unification and it de-risks
the Phase-C inversion by proving the resource/arrangement model can be DERIVED from
the flat grove and MATCHES what ControlFlow allocates today.

```diff
+ MaterializationPlan PlanMaterialization(query, instance_flow, cost):   # §10
+   resources    = one StateResourceId per stateful LogicalCollectionId
+   arrangements = one ArrangementId per (resource, required column-set/order)
+                  seeded = the CURRENT runtime requirement set
+   bindings     = per FamilyNode -> (resource, arrangement) it reads/writes
+   ValidateMaterialization (V-MAT-AUTHORITY / V-MAT-SCHEMA / V-MAT-REPLICA)
+ # CROSS-CHECK (the shadow contract, deleted before the phase lands per §20-B):
+ #   resources   vs the real view_to_model / EquivalenceSet union-find
+ #   arrangements vs the real GetOrCreateIndex column_spec set
+ #   ABORT on divergence — proves the derived plan == today's allocation
+ -materialization-out dump (§16 resource/arrange grammar); .irgold-pinned observer.
```
- Storage: by-value on `QueryImpl` beside `instance_flow` (the RowContract/grove
  precedent). Built in the `Query::Build` tail AFTER `BuildFlatInstanceFlow`.
- Determinism: ids from `LogicalCollectionId`/canonical column order, never `TABLE*`
  pointer / iteration order.
- Exit gate: OptDiff SUITE PASS byte-identical (observer) + the cross-check quiescent
  corpus-wide + a `-materialization-out` golden or two + ctest 5/5.
- WHY this and not a JoinPivot-seed slice: the seed slice (activate `CandidateSeed`)
  is also a valid shadow, but MaterializationPlan is on the CRITICAL PATH to Phase C
  (it is Phase C's input, §11.3 `BuildRel(query, flow, materialization)`), whereas
  the seed slice only pays off at Phase D which is past the blocker. Do the
  critical-path shadow first.

### Then: Phase C = the allocation inversion (the megaproject, multi-session)

```diff
- Program::Build allocates every TABLE*/DataModel, then ControlFlow region builders
-   pick pivots + call GetOrCreateIndex INLINE, then BuildDRInventory shadows it
+ BuildRel(query, instance_flow, materialization, sccs, cost) inventories every op
+   from IDs + occurrence topology (no TABLE*, no Query re-walk)
+ ControlFlow LOWERS Rel ops; AllocateRuntimeResources(materialization) is the ONLY
+   allocation site, BEFORE region lowering consumes it
- delete: BuildDRInventory's TABLE* walk; the Query-shape join/branch rediscovery
-   (Join.cpp BuildJoin + Rel.cpp's re-derivation); the reserved DRInstance/demand_table
```
Sequence inside Phase C (smallest-first, each its own gate): (1) retype ONE Rel
struct family off `TABLE*` onto `StateResourceId` behind the materialization map,
cross-checked; (2) move index allocation out of `GetOrCreateIndex` inline sites into
a post-Rel `AllocateRuntimeResources`; (3) delete the shadow re-derivation once Rel
is the sole authority. Each step keeps codegen byte-identical until the last.

### Then: Phase D = the FIRST codegen move (join-pivot K-context families, §20-D)

Only reachable after Phase C. `JoinPivot` contexts bind K across left/right, elide
carried key columns, add `VisibilityFedGroupedColumns` (sorted key directory +
residual columns), Rel `SortedSectionProbe`/`SortedBatchMerge`. THIS is where a
golden legitimately moves (predict-then-verify, re-bless the CP2 goldens).

## §2.5 The materialization/allocation side as pseudocode (what the next slice observes)

The recommended next slice is a certified-OBSERVER `MaterializationPlan`, and it
must cross-check against TODAY's storage/index decisions. Those decisions live in
THREE places, all read-verified at tip `693717f1`:

```
# (A) storage sharing — DataFlow (lib/DataFlow/Build.cpp:2303)
BuildEquivalenceSets(query):                 # runs INSIDE Query::Build, before freeze
  # union-finds QueryViews that must share ONE backing store (INSERT/SELECT of the
  # same relation; bijective pass-through TUPLEs). Result: view.EquivalenceSetId().
  # THIS is the physical decision §10 pulls out of Query ("useful migration
  # evidence, not future authority").

# (B) resource allocation — ControlFlow (lib/ControlFlow/Build/Build.cpp)
BuildDataModel(query, program):              # :233
  for view in query.ForEachView:            # ONE DataModel per view...
    model = new DataModel ; view_to_model[view] = model
    eq_classes[view.EquivalenceSetId()] = model
  for view in query.ForEachView:            # ...union-find'd by EquivalenceSetId
    DisjointSet::Union(view_to_model[view], eq_classes[view.EquivalenceSetId()])
  # => view_to_model: a union-find whose CLASSES are the stateful-storage candidates.

FillDataModel(query, program, context):      # :37  — WHICH classes get a real TABLE
  for view where view.CanReceiveDeletions():
    for pred in view.Predecessors(): TABLE::GetOrCreate(pred)   # differential needs persisted preds
  for insert in query.Inserts() where insert.IsRelation():
    TABLE::GetOrCreate(insert)                                  # materialized relations
    if insert.NumAttachedColumns(): TABLE::GetOrCreate(pred[0]) # condition-witness setter
  for merge where NeedsInduction{Cycle,Output}Vector(merge): TABLE::GetOrCreate(merge)
  for join: for pred in join.JoinedViews(): TABLE::GetOrCreate(pred)   # join inputs
            if join.CanReceiveDeletions() and a successor drops a pivot: TABLE::GetOrCreate(join)
  # => the set of views backed by a persistent TABLE = today's "stateful collections".

# (C) arrangement allocation — SCATTERED, inline (the GAP, §4)
DataTableImpl::GetOrCreateIndex(cols):        # lib/ControlFlow/Data.cpp:348
  SortAndUnique(cols) ; spec = ColumnSpec(cols)   # ORDER-FREE key (so [A,B]==[B,A])
  return the existing DataIndex with column_spec==spec, else Create one
  # CALLED INLINE mid-region-build: Data.cpp:205, Build.h:443, Join.cpp:272/408,
  # Build.cpp:432/490/526/1495. There is NO standing requirements pass — an
  # arrangement requirement only becomes visible at the call site that needs it.
```

**The InstanceFlow↔materialization mapping (the observer's contract):**
- `LogicalCollectionId` (an INSERT-target decl, CP2) ↔ the `view_to_model` CLASS its
  writer views fall into. A collection is STATEFUL iff FillDataModel gives that class
  a TABLE. Cross-check: two collections whose writer views share a model ⟺ one
  `EquivalenceSet` (co-recursive relations may share a store yet be two collections —
  the `LogicalCollectionId ≠ EquivalenceSetId` warning in InstanceFlow.h).
- `ArrangementId` ↔ a `DataIndex` (`column_spec`) on that table. Order-free key.

### §2.5.1 The observer-slice diffs — RESOURCES-ONLY FIRST (smallest real slice)

The §4 gap (no `collect_arrangement_requirements` pass) means deriving ARRANGEMENTS
up-front is hard — they are minted inline. So split the slice: derive + cross-check
RESOURCES first (tractable: FillDataModel's TABLE-need rules are a bounded set over
the grove), DEFER arrangements to a follow-on.

```diff
+ MaterializationResources PlanResources(query, instance_flow):   # pure, observer
+   for lc in instance_flow.collections:
+     stateful = any writer view of lc is TABLE-backed per the FillDataModel rules
+                (re-derived from the grove, NOT read from program->tables)
+     if stateful: resources += StateResource{ new StateResourceId, authority_for=lc,
+                                              schema = lc.writer0 residual }
+   # plus InternalResidualCollectionId resources for TABLE-backed NON-collection
+   # views (join preds, induction merges) — the residue outside insert-named LCs,
+   # exactly the Tier-2 provenance shape (CollectOriginInteriorDecls precedent).
+   ValidateMaterialization: V-MAT-AUTHORITY (>=1 authority per stateful collection;
+                            ephemeral collections have none).
+ # CROSS-CHECK (the shadow contract, deleted before the phase lands, §20-B):
+ #   the set {authority_for collections} ∪ {internal residual resources}, mapped back
+ #   to views, MUST equal the TABLE-backed view set in program->view_to_model after
+ #   Program::Build — ABORT on divergence. Proves the grove-derived resource set ==
+ #   today's allocation. (Runs as a DEBUG/belt cross-check, like the K5 conservation
+ #   assert; the plan itself is built pre-Program from the grove.)
```
Storage: by-value on `QueryImpl` beside `instance_flow` (built in the `Query::Build`
tail, AFTER `BuildFlatInstanceFlow`). Determinism: ids from `LogicalCollectionId` /
canonical view order, never `TABLE*` / iteration order. Codegen byte-identical.

### §2.5.2 Desired `-materialization-out` dump (§16 grammar) — resources first

```
materialization  resources=R arrangements=A
resources
  sr#0 authority=lc#0 schema=(From,To) support=differential
  sr#1 authority=lc#2 schema=(Node) support=monotone
  sr#2 authority=internal#0 schema=(X,From,To) support=differential   # a join-pred residue
# arrangements — DEFERRED to the follow-on slice (the collect_arrangement_requirements gap)
```
Predict-then-verify the dump FIRST (as with CP2), then bless a `.materialization`
golden or two (transitive_closure = the differential+monotone+internal-residue
witness; join_1 = the simplest).

### §2.5.3 The open call to settle before building (the §4 gap, sharpened)

`collect_arrangement_requirements` (§10) has no analog today — arrangements are minted
inline. Three ways, decide in grounding: (i) replicate the inline index-need logic in a
pre-pass over the grove+Query (faithful but duplicates scattered logic); (ii) build
arrangements as a POST-`Program::Build` read of each table's `DataIndex` set (easy but
INVERTS the intended order — observing the output, not deriving it — acceptable ONLY as
a cross-check, never as the authority); (iii) RESOURCES-ONLY first (§2.5.1), arrangements
deferred. Recommend (iii): it is the smallest slice that is genuinely REAL (a new typed
authority, cross-checked, dumped, goldened) with a clean line to Phase C, and it does not
force resolving the arrangement-derivation question before there is a consumer for it.

## §3 Anchors (re-verify at next tip — the pipeline drifts)

| Fact | Source (tip 693717f1) |
|---|---|
| Grove builder + role/root + bound-read arm | `lib/DataFlow/InstanceFlow.cpp` (`BuildFlatInstanceFlow`) |
| Validators (+ kBoundQueryRead arm) | `lib/DataFlow/InstanceFlow.cpp` (`ValidateInstanceFlow`) |
| Dump (root=/role=/bound-read covers) | `lib/DataFlow/Format.cpp` (`operator<<(QueryInstanceFlow)`) |
| 4 CP2 goldens | `tests/OptDiff/goldens/{join_1,merge_2,transitive_closure,barrier_neck_1}.instanceflow.opt.golden` |
| Phase-C blocker anchors | `lib/Rel/Rel.{h,cpp}` (TABLE*), `lib/ControlFlow/Build/*` (inline alloc), `Procedure.cpp:749` |
| Storage sharing (resources) | `BuildEquivalenceSets` `lib/DataFlow/Build.cpp:2303`; `BuildDataModel` `lib/ControlFlow/Build/Build.cpp:233` (`view_to_model` union-find) |
| Stateful-table decision | `FillDataModel` `lib/ControlFlow/Build/Build.cpp:37` (which views get a `TABLE`) |
| Arrangement dedup (scattered) | `DataTableImpl::GetOrCreateIndex` `lib/ControlFlow/Data.cpp:348` (`ColumnSpec`, order-free); inline call sites `Data.cpp:205`,`Build.h:443`,`Join.cpp:272/408` |
| Internal-residue precedent | `CollectOriginInteriorDecls` (Tier-2 provenance, `lib/Regional/Planning.cpp`) |
| MaterializationPlan spec | `docs/proposals/InstanceFlow.md` §10 (L1008), §11.3 (L1116) |
| Grounding (this session) | `docs/proposals/InstanceFlow.artifacts/session-33-grounding.md` |
| Vision | `docs/proposals/InstanceFlow.md` §1 (why a new IR), §6, §16, §20 |

## §4 Open questions carried forward

- **Discriminating kBoundQueryRead witness:** the CP2 witnesses vacuously exercise
  the arm (writer0 == the INSERT node == role=root either way — panel R4). A bound
  `#query` over a relation with ≥2 non-mergeable INSERT sites would make the
  read-vs-insert coverage arms diverge; no such case is in the corpus. Add one if the
  arm ever becomes load-bearing.
- **`role=` token aliasing (panel F6, accepted):** node lines use `role=root|interior`
  (OccurrenceRole); covers lines use `role=copied|pivot|...` (InputColumnRole). Distinct
  line-kinds; not renamed. Revisit only if a parser consumes the dump.
- **MaterializationPlan arrangement enumeration (anchors scout):** there is no standing
  `collect_arrangement_requirements` pass today — requirements only materialize at the
  inline `GetOrCreateIndex` call sites mid-walk. The observer plan must either replicate
  that logic in a pre-pass or defer construction; resolve before building §2's slice.
