# Rel Migration State

This document treats the user's recollection as a hypothesis and checks it
against current code and repository history. The hypothesis is correct: Rel is
a work-in-progress insertion between DataFlow and ControlFlow. It is not yet a
clean compiler stage in the conventional ownership sense.

## Historical Shape

Before the Rel work, the effective pipeline was:

```text
ParsedModule -> Query/DataFlow -> direct ControlFlow builders -> Program -> C++
```

The first Rel commit, `bdcea2ff` on 2026-07-15, explicitly introduced a
construct-alongside shadow inventory. Its purpose was to derive differential
operations independently from Query and compare them with the old discovery
and emission machinery. Subsequent commits cut over operation families and
deleted portions of the old discovery/scheduling code. The library was renamed
from `DR` to `DeltaRel` and finally to `Rel`.

That history explains the current topology, but it does not justify preserving
the topology. Current code remains the authority.

## Current Execution Order

The present `Program::Build` is approximately:

```text
Program::Build(Query):
  allocate ProgramImpl and ControlFlow build Context
  build table/index data model
  start building ControlFlow entry procedure
    eagerly walk Query nodes
      mint ControlFlow regions immediately
      record censuses/events for later Rel enrollment
    complete eager work lists
    BuildStratumPhases
      build local DRFlowGraph from Query + Context
      derive Rel strata and dependency order
      validate Rel inventory
      cross-check earlier eager emissions
      move graph into Context::dr_flow
      optionally dump it
      lower Rel-owned stratum and recursive bands to ControlFlow
    lower keyed-instance and commit-sweep tails from Context::dr_flow
  build remaining IO/init/query procedures
  optimize ControlFlow
  return Program
  destroy Context and DRFlowGraph
```

Rel therefore participates *inside* ControlFlow construction. It is not an
input-complete stage that runs before any ControlFlow node exists, and its graph
does not survive in the returned `Program`.

## What Rel Owns Today

The strongest current ownership claims are narrower than "Rel owns lowering":

- Rel derives the differential operation inventory, vector/effect model,
  dependency edges, strata, rounds, and pinned schedule.
- Rel-owned schedules drive acyclic differential bands, recursive round shells,
  commit sweeps, aggregate/state-cell work, and keyed-instance work.
- ingest fold/loop and join-emission records have Rel operations, but several
  are lowered at their original eager-walk positions to preserve region/id
  ordering.
- validators recompute expected operation families and compare enrollment with
  emitted effects.

This is real authority. It is also incomplete authority because the graph is
constructed from, and lowered through, ControlFlow-internal objects.

## What ControlFlow Still Owns

The eager descent remains ControlFlow code:

- `BuildEagerRegion` dispatches Query view kinds.
- `BuildEagerJoinRegion`, `BuildEagerProductRegion`,
  `BuildEagerGenerateRegion`, `BuildEagerCompareRegions`, and peers create the
  imperative region structure.
- eager marker enrollment is reconstructed from a walk-side census.
- join/product emission events are recorded before the Rel graph exists and
  replayed into Rel operations later.
- `LowerJoinEmit` still calls the shared `BuildJoin`; `LowerProductEmit` only
  records that an inline ControlFlow emission happened.
- lowerers in `Stratum.cpp` consume Rel operations but directly allocate
  ControlFlow regions and call surviving `Emit*` helpers.

The honest statement is: Rel is the scheduling and inventory authority for a
large differential subset, while ControlFlow is still the object owner and the
imperative emission implementation. Some eager families are modeled and
cross-checked rather than authored entirely by Rel.

## Structural Evidence of an Incomplete Boundary

1. `Rel` includes private ControlFlow headers and uses `TABLE`, `VECTOR`,
   `ProgramImpl`, `Context`, and ControlFlow builder helpers.
2. `ControlFlow` links `Rel` and includes `Rel.h`; the static libraries are
   mutually referencing.
3. `DRFlowGraph` stores raw table pointers and Query handles.
4. the graph is local to `Program::Build` and is destroyed with its `Context`.
5. `Context::dr_flow` is a nullable `shared_ptr`, even though the current entry
   procedure calls `BuildStratumPhases` unconditionally and that function
   always installs a graph before its early return. The null check in the
   commit path and comments saying "null when no stratum phases ran" reflect an
   older control-flow shape.
6. comments disagree about whether eager wrapper paths are retired, modeled,
   or the only remaining hand-coded surface.

## Split-Brain Assessment

The hybrid is understandable as a migration technique. The problem is leaving
its end state implicit:

```text
Rel as checker:
  Query -> ControlFlow
       \-> Rel inventory validates selected decisions

Rel as physical plan:
  Query -> Rel plan -> generic ControlFlow lowering

Rel as partial scheduler:
  Query -> shared Program-build context
             Rel schedules differential bands
             eager builders author the rest
```

All three can be coherent. Combining their claims is not. The code is closest
to the third today, while proposal prose often claims the second and some
cross-check structure still resembles the first.

## End-State Decision Required

Before CostModel takes Rel as its primary input, choose one of these directions:

### A. Finish Rel as the physical plan

Make Rel value-semantic and independent of ControlFlow internals. Build it from
Query plus an explicit physical-schema input, then lower the complete supported
operation vocabulary through a one-way `Rel -> ControlFlow` dependency. Remove
walk-side production authorities after their Rel replacement is tested.

This is the cleanest foundation for a plan-based CostModel, but it is a broad
compiler refactor.

### B. Keep Rel as an internal differential scheduler

State the narrower ownership contract. Stop claiming a universal intermediate
representation. Cost analysis must snapshot facts from Query, Rel, and the
finished Program because no one representation is complete.

This is the smallest honest change, but it makes a complete static cost model
harder and exposes duplicated semantic identities.

### C. Retire Rel after extracting its proven algorithms

Move its inventory/scheduling algorithms into a single physical-planning owner
or directly into a redesigned Program builder, then delete the circular layer.

This may be preferable if a generic Rel representation is not useful outside
the migration. It should not be dismissed merely because much Rel code exists.

## CostModel Consequence

Do not add a post-`Program::Build` `DRFlowGraph` accessor. The graph does not
outlive the build and contains unstable pointers. Do not make its diagnostic
text dump the implicit core API either.

The first CostModel experiment should either:

- consume a pointer-free `CostProgramSnapshot` constructed while Query, Rel,
  and ControlFlow are simultaneously alive; or
- wait for the Rel end-state decision and use the resulting stable plan type.

The snapshot is an experiment boundary, not an endorsement of today's hybrid.
Its fields should record where each fact came from so contradictions between
Query, Rel, and Program remain observable rather than silently reconciled.

## Migration Completion Tests

Whichever direction is chosen, use tests to prove authority transfer:

- perturb the alleged old authority and show output cannot change, or compilation
  fails because no caller remains;
- perturb the new authority and show a targeted structural/runtime test fails;
- delete the old path in the same change as the cutover;
- compare generated behavior across all four optimization modes;
- retain independent Rel/Program cross-checks only when they derive the same
  invariant from genuinely different inputs;
- avoid byte-identity as the only acceptance condition when the new architecture
  deliberately changes identifiers or schedule representation.

The goal is not to preserve a dual implementation forever. The dual period is
useful only while it produces evidence for deletion.
