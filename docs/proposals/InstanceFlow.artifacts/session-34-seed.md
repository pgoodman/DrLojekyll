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

## §3 Anchors (re-verify at next tip — the pipeline drifts)

| Fact | Source (tip 693717f1) |
|---|---|
| Grove builder + role/root + bound-read arm | `lib/DataFlow/InstanceFlow.cpp` (`BuildFlatInstanceFlow`) |
| Validators (+ kBoundQueryRead arm) | `lib/DataFlow/InstanceFlow.cpp` (`ValidateInstanceFlow`) |
| Dump (root=/role=/bound-read covers) | `lib/DataFlow/Format.cpp` (`operator<<(QueryInstanceFlow)`) |
| 4 CP2 goldens | `tests/OptDiff/goldens/{join_1,merge_2,transitive_closure,barrier_neck_1}.instanceflow.opt.golden` |
| Phase-C blocker anchors | `lib/Rel/Rel.{h,cpp}` (TABLE*), `lib/ControlFlow/Build/*` (inline alloc), `Procedure.cpp:749` |
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
