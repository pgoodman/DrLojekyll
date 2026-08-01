# Rel End-State Adjudication (Work Order 2)

> Status: 2026-07-31, branch `keyed-instances`, tip after `61b17a15`. Grounded
> in `lib/Rel/Rel.{h,cpp}`, `lib/ControlFlow/Build/Stratum.cpp`,
> `lib/ControlFlow/Build/Build.cpp`, `Procedure.cpp`, and the always-on
> validator inventory. This sharpens `rel-migration-state.md` into a decision
> the owner can ratify. **It stops short of choosing A/B/C: the options are
> materially different and the choice is the owner's** (Stop Condition:
> "the Rel end-state options remain materially different and require owner
> judgment"). What follows is the concrete tradeoff and a recommendation.

## Op-family authority table

Grouped from the 29-kind `DROpKind` enum (`lib/Rel/Rel.h:123`). Columns:
**Inv** = inventory author (who mints the op), **Pay** = payload author (who
fills its effect set / access plan), **Sch** = schedule author (who orders it),
**Emit** = who allocates the ControlFlow regions, **Check** = independent
validator, **Std?** = is Rel the standalone authority yet.

| Family (kinds) | Inv | Pay | Sch | Emit | Check | Std? |
|---|---|---|---|---|---|---|
| Differential bands: `kCrossover kProductArm kSeedFold kFixpointFire kChainFold kClaimDrain kRetire kRederive kFrontierFilter kPivotAssemble` | Rel | Rel | Rel | Rel lowerers in `Stratum.cpp` call surviving `Emit*`/region builders | V-XOVER-ONE, V-PROD-*, V-CLAIM-GATE, V-SEED-*, V-DEFER, V-RETIRE-AFTER, V-LINEAR/LOOP/READY/BAND-HAZARD | **Yes** — Rel is sole authority; the old scheduling fixpoint + DiscoverBranches were deleted |
| Commit/state: `kCommitSweep kStateSeal kGroupUpdate` | Rel | Rel | Rel | Rel lowerers (`LowerCommitSweeps`, `LowerGroupUpdate`) | V-COMMIT-TRAILS, V-AGG-*, V-ALGEBRA | **Yes** |
| Keyed-instance: `kSubgraphInstantiate kInstanceDeath kInstanceSeal` | Rel | Rel | Rel | Rel lowerers | V-INST-* (SOLE/PAIR/EFFECT/ORDER/DRAIN/DIFF-COHERENCE) | **Yes** |
| Negate gate: `kNegateGate` | Rel (minted at dispatch site, relocation) | Rel (real `kFlagRead` of the negated view's model table) | Rel | `LowerRelStep_*` wraps untouched region builder | V-NEG-CTX, V-PRED-XCHECK | **Yes** (effect-bearing, modeled) |
| Ingest folds/loops: `kIngestFold kIngestLoop` | Rel | Rel | Rel, but **lowered at the original eager-walk position** for id-stream identity | `LowerIngestFold`/`LowerIngestLoop`; the UPDATECOUNT/VECTORLOOP body is filled by hand-coded `BuildEagerInsertionRegions`/`ExtendEagerProcedure` | V-INGEST-XCHECK Site 5, INGEST-CURSOR/LOOP-SHAPE | **Partial** — Rel owns the fold record; ControlFlow still authors the descent body it wraps |
| Eager markers: `kEagerForward kEagerInsert kEagerCompare kEagerGenerate kEagerUnion kEagerSelect kEagerJoin kEagerProduct` | Rel (per-visit dispatch records) | Rel (mostly effect-free; `cmp`/`functor` re-derive from `eager_view` at render) | Rel strata | **ControlFlow eager descent** (`BuildEagerRegion`ff) authors the region shape; Rel only models/cross-checks it | V-PRED-XCHECK ties model to emitted `Emit*` | **No** — modeled + cross-checked, but the imperative shape is authored by the walk |
| Join/product emission: `kJoinEmit kProductEmit` | Rel (one per proc×join_view×form event) | Rel (drain key + `work_seq` tie-break) | Rel (delta-enrolled after DeriveDRStrata) | `LowerJoinEmit` wraps the **untouched `BuildJoin`**; `LowerProductEmit` only records an inline CF emission happened | V-JOIN-EMIT-XCHECK Site 5 | **Partial** — Rel owns the emission record + order; `BuildJoin` is the surviving imperative emitter |

**Reading.** Rel is the *sole* authority for the differential scheduler
(bands, rounds, commit, state cells, keyed instances, negate gates). It is a
*modeling + cross-checking* authority for the eager web and for join/product
emission: those regions are authored by the surviving hand-coded descent and
`BuildJoin`, with Rel enrolling records and validators (Site-5 multisets)
asserting the two agree. No op family has two *uncontrolled* production authors —
every partial case has a named cross-check that aborts on divergence. So Stop
Condition "a Rel fact has two production authors and no clear authority" is
**not** tripped; the hybrid is a controlled model-vs-emit duality, not an
ambiguity.

## Structural facts that bound the CostModel boundary (code-checked)

1. `Rel` includes ControlFlow-internal headers and uses `TABLE*`, `VECTOR`,
   `ProgramImpl`, `Context`; `ControlFlow` links `Rel`. The static libraries are
   mutually referencing.
2. `DRFlowGraph` stores raw `TABLE*` and Query handles; it is local to
   `BuildStratumPhases`, moved into `Context::dr_flow`, and destroyed with the
   build `Context`. `Program` does not retain it.
3. The only public Rel seam is the text sink `SetRelDumpStream` (a diagnostic
   dump), not an in-memory accessor.

Consequence (unchanged from the audit, re-verified): **do not** expose a
post-build `DRFlowGraph` accessor and **do not** parse the `.rel` text as an
internal API. A CostModel that needs Rel facts must snapshot them into a
pointer-free value type while Query/Rel/Program are simultaneously alive.

## The three end states, with concrete cost

### A. Finish Rel as a value-semantic physical plan (`Query → Rel → ControlFlow`)
- **Deletion work:** the surviving hand-coded emitters — `BuildEagerRegion` and
  peers, `BuildJoin`, `BuildEagerInsertionRegions`, `ExtendEagerProcedure` —
  must be re-expressed as one-way lowerings from Rel ops; the walk-side census
  enrollment and Site-5 cross-checks then become dead and are deleted.
- **Dependency direction:** break the `Rel ↔ ControlFlow` cycle. Rel stops
  including ControlFlow headers; it holds its own value-typed tables/vectors and
  an explicit physical-schema input; `ControlFlow` depends on `Rel` only.
- **Lifetime:** Rel graph can then be returned in `BuildArtifacts` and outlive
  the build — a real CostModel input.
- **Test strategy:** perturb an eager emitter → must fail to compile (no caller)
  once its authority moves to Rel; the 4-mode goldens gate answer-identity.
- **CostModel consequence:** cleanest — one stable plan type to snapshot. But it
  is a broad compiler refactor touching every eager family.

### B. Keep Rel as an internal differential scheduler (current shape, made honest)
- **Deletion work:** none structural; instead *state* the narrower contract and
  delete the "sole authority" / "clean `DataFlow→Rel→ControlFlow`" prose in
  `Rel.h`, `CLAUDE.md`, and the comment-drift targets.
- **Dependency direction:** unchanged (mutual); documented as intentional.
- **Lifetime:** graph stays build-local. CostModel must snapshot from Query,
  Rel, AND the finished Program because no single representation is complete.
- **Test strategy:** the existing Site-5 / V-PRED-XCHECK duality stays as the
  authority-transfer evidence.
- **CostModel consequence:** the snapshot must carry per-fact provenance (which
  representation each fact came from) so contradictions stay visible. A complete
  static plan-based cost model is harder; duplicated semantic identities are
  exposed to the snapshot layer.

### C. Retire Rel after extracting its algorithms
- **Deletion work:** move the inventory/strata/round algorithms into a single
  physical-planning owner (or a redesigned Program builder), then delete the
  circular layer and its validators.
- **Dependency direction:** collapses to one owner.
- **CostModel consequence:** only worth it if a generic Rel representation has no
  use beyond the migration. Highest churn; should not be chosen merely because
  much Rel code exists, nor dismissed for the same reason.

## Recommendation (owner ratifies)

**For the CostModel experiment specifically: proceed under B's boundary now,
without foreclosing A.** Rationale, grounded above:

- Rel is *already* the sole authority for the differential scheduler — the part
  a cost model most wants (bands, rounds, commit, instances). That authority is
  real and validated, so a snapshot taken while the build is alive gets
  trustworthy scheduler facts today, with no refactor.
- The eager/join-emission families are the ONLY ones authored outside Rel, and
  they are exactly the families the first cost slice reads from *Query + index
  facts* anyway (cost-model-findings.md #6: `kJoinEmit` carries no access-plan
  spine). So B's "snapshot from multiple representations" is not a workaround —
  it matches where the facts actually live.
- A is the right *eventual* home for a plan-based cost model, but it is a large
  refactor whose payoff (a returnable plan type) is not needed until the cost
  model outgrows a single supported op family. Committing to A now would make the
  cost experiment wait on a compiler refactor — the exact inversion the audit
  warns against.

**What B commits the next session to:** the CostModel snapshot is built at the
owner boundary while Query/Rel/Program are alive, records per-fact provenance,
and is explicitly labelled a partial-scheduler snapshot — not a Rel-complete
plan. If and when the eager/join families need exact cost, that is the trigger to
revisit A, not before.

**This recommendation is not self-executing.** If the owner intends Rel to become
the universal IR (direction A) on its own schedule, the cost snapshot should be
designed to converge on that plan type rather than on a multi-representation
crosswalk. That is the one materially-different fork that needs the owner's call.

## Cross-reference: `BoundedObservation.md` is an A-signal (2026-08-01)

The `docs/proposals/BoundedObservation.md` proposal (bounded-consumption
operators `ONLY`/`CHOOSE_ONE`/`EXISTS`) was read and spot-verified against code
this session. Two facts it establishes bear directly on this fork:

- It requires Rel to **author a query-read physical descriptor**
  (`ProgramQueryRead`, §6.6/§7) that codegen dispatches on mechanically. The
  query-entry surface today BYPASSES Rel entirely — verified: entry points are
  emitted from `ProgramQuery`, and `PlanKind` (`Rel.h:470`) has only
  `{kAccess,kGate,kFold}` with no read-plan concept; `ParsedQuery::
  ReturnsAtMostOneResult()` has exactly ONE consumer, `lib/Parse/Format.cpp:115`
  (source reformatting), so declaration `@first` never becomes an IR fact.
- It adds new `DROpKind`s (`kObserveOnly`, `kChooseOne`) with Rel choosing one
  tagged physical decision per work scope, ControlFlow lowering mechanically.

That is a slice of **direction A** — Rel authoring physical plans — for a NEW op
family (consumption), not the existing eager/join families. It does not
contradict "B for the CostModel boundary now": it is evidence the owner's
trajectory is Rel-authors-physical-plans, family by family, which is the
incremental path TO A. Consequence for CostModel: a snapshot built under B should
anticipate consuming Rel-authored consumption plans when they land, and
BoundedObservation's cost/correctness boundary (§0/§9: cost reports row touches
vs the permitted bound, never authorizes a rewrite; correctness is a structural
proof certificate) is IDENTICAL to `costmodel-contract-decision.md`. The two
design tracks are consistent; the measurement runner in `calib/` is the row-touch
instrument BoundedObservation §9/§10.4 assumes.

`docs/proposals/InstanceFlow.md` (added on origin as `e2ee41b8`) is NOT yet read;
it is flagged for the next session as a further Rel/keyed-instance design input.

## Perturbation evidence gathered this session

A cheap, safe authority perturbation was run: toggling the dataflow pass
`df.ident_join` (ON default vs `-opt-disable`) on the mono witness changes the
`.rel` census (`kJoinEmit` 1↔2, `kEagerJoin` 2↔4) but Rel neither adds nor
removes the join — it faithfully propagates the dataflow transform's decision
(grounding-double-join.md §5, re-confirmed). This demonstrates the method the
migration-completion tests should use: perturb the alleged origin authority (the
dataflow transform), observe the downstream model track it exactly. A full A/B/C
migration would extend this to perturbing eager emitters and requiring a
compile failure once their authority moves to Rel.
