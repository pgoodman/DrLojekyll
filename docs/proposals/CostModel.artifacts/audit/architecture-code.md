# Executable Architecture

This view is derived from executable entry points, public and internal types,
call order, state mutation, and loop structure. Proposal prose is not an
authority for this section.

## Whole-Program Pipeline

```text
input boundary [top-down, stateful]:
  source paths/streams
    -> DisplayManager / Lexer
    -> Parser
    -> ParsedModule

logical compilation [bottom-up graph construction, then global rewrites]:
  Query::Build(module, ErrorLog, PassPolicy, demand flags)
    -> create Query relations/views from declarations and clauses
    -> simplify
    -> connect inserts to relation selects
    -> optionally apply demand transformation
    -> optimize Query graph
       [iterative CSE + canonicalization + dead-flow elimination]
    -> normalize/link/remove dead views
    -> identify inductions
    -> finalize depths and column ids
    -> derive differential-update facts
    -> build equivalence sets
    -> stratify
    -> Query

physical compilation [hybrid eager emission + Rel inventory/scheduling/lowering]:
  Program::Build(Query, ErrorLog, first id, PassPolicy, demand-instance mode)
    -> allocate DataTable/DataIndex/DataVector/Program regions
    -> walk eager paths and emit part of the ControlFlow program while
       recording censuses/events for operations not yet inventoriable
    -> BuildDRInventory(Query, Context)
    -> DeriveDRStrata(DRFlowGraph)
       [integer-lifting fixpoint]
    -> validate DR inventory and op contracts
    -> linearize and validate dependency order
    -> optionally dump DRFlowGraph as text
    -> lower DRFlowGraph-owned bands/rounds into ControlFlow regions and call
       surviving eager/emit builders at required walk positions
    -> cross-check enrolled DR ops against emitted ControlFlow effects
    -> optimize ControlFlow to a fixpoint
    -> Program

output boundary [structural traversal]:
  Program
    -> ControlFlow text, Query text/DOT, and/or generated C++
    -> generated Database + runtime containers

runtime [event-driven incremental evaluation]:
  init/message/query entry points
    -> fold message rows into derivation counters
    -> enqueue zero crossings
    -> per-stratum seed propagation
    -> overdelete fixpoint
    -> rederive
    -> insert fixpoint
    -> publish net changes at commit
```

Primary code anchors:

- `bin/drlojekyll/Main.cpp:62-135` owns the top-level Query -> Program -> output
  sequence.
- `lib/DataFlow/Build.cpp:2560-2646` owns the late Query pipeline.
- `lib/ControlFlow/Build/Build.cpp:1308-1600` owns Program construction.
- `lib/ControlFlow/Build/Stratum.cpp:2149-2297` creates, validates, stashes,
  and dumps the Rel graph before lowering it.
- `lib/ControlFlow/Optimize.cpp:1253-1397` optimizes the imperative program to
  a fixpoint.
- `lib/CodeGen/CPlusPlus/Database.cpp` emits the runtime program.
- `include/drlojekyll/Runtime/Table.h` implements incremental table state,
  indexing, fixpoint membership, and counter instrumentation.

## Algorithm Regions

### Parsing

The lexer/parser is stateful and top-down. Errors accumulate in `ErrorLog`, and
most invalid user inputs return `std::nullopt` at the parser or build boundary.
This is a conventional boundary-owned absence state.

### DataFlow construction and optimization

The Query graph is built bottom-up from parsed declarations and clauses. It is
then globally rewritten. The optimizer is not one algorithm:

- CSE candidate coloring is iterative refinement.
- Structural CSE is greedy within refined candidate groups.
- Canonicalization has bottom-up and top-down modes.
- dead-flow elimination is a forcing/taint analysis.
- the outer optimizer interleaves these passes because each exposes work to the
  others.
- identity-join removal is a conservative structural peephole using a forward
  provenance analysis, rerun until no eligible identity remains.

The graph is cyclic at the Query level for inductive programs. A simple
"iterate views in depth order" statement is insufficient for general numeric
cardinality propagation. `Prov` is deliberately safe on back-edges because an
uncomputed fact becomes bottom; a cardinality algorithm cannot use the same
shortcut and still claim exact counts.

### Provenance

`ComputeColumnProvenance` is a forward, conservative, under-approximating value
containment analysis:

```text
ProjKey = (source identity pointer, source column index)
Prov(column) = sorted set of source projections known to contain its values

unknown or unsupported transfer -> empty set
merge -> intersection
safe equality/pass-through -> propagate
```

It proves subset relationships. It does not compute row counts, demand slice
sizes, fanout distributions, distinct counts, or fixpoint closure sizes. Reuse
for a CostModel means reuse of one structural certificate, not reuse of a
cardinality engine.

### Rel inventory and scheduling

`DRFlowGraph` is an internal model of differential work. It contains typed op
kinds, effect sets, vectors, tables, round shells, dependency edges, plan
spines, and a pinned order. Its construction and schedule derivation are
bottom-up plus constraint propagation. Validators independently recompute many
expected sets and abort loudly on disagreement.

It is not currently a standalone layer:

- `Rel` includes ControlFlow internal headers.
- `ControlFlow` links `Rel`, while `Rel` references ControlFlow types.
- the two static libraries are mutually referencing.
- `DRFlowGraph` stores raw `TABLE *` and Query handle values.
- eager emission begins before the full Rel graph exists; records of emitted
  effects are later compared with Rel enrollment.

Rel began as a shadow inventory and has progressively replaced substantial
differential discovery, scheduling, and lowering. It is now a real in-build
authority, but the migration is incomplete: the code is closest to a partial
scheduler/physical-plan owner embedded inside ControlFlow construction. See
`rel-migration-state.md` for the cutover map and possible end states. The
current object is unsafe to expose directly as a long-lived CostModel API.

### ControlFlow lowering and optimization

Rel operations lower to imperative region trees. The ControlFlow optimizer is
a convergent rewrite loop over region kinds, deepest first, followed by greedy
procedure deduplication. This is a separate optimization authority over a
different representation, but it should not change observable semantics.

### Runtime evaluation

The generated runtime is incremental and stateful, not a stateless relational
expression evaluator. Important state includes:

- append-only row identity and hash-table capacity,
- secondary index chains,
- per-class derivation counts,
- batch-start versus current membership,
- current overdelete/insert frontier membership,
- touched sets,
- standing aggregate cells and keyed instances.

Costs therefore depend on an event trace and starting database state. A single
cardinality per Query view cannot determine general DELTA-regime work.

### Oracle and tests

`drlojekyll-oracle` builds the Query graph and compares an incremental
derivation-counter interpretation against a from-scratch stratified evaluation
after each batch. This is meaningfully independent of generated ControlFlow and
runtime scheduling, though it shares parsing and Query construction.

The OptDiff drivers compare four optimization configurations against committed
stdout. Selected batch cases also compare the oracle and monotone projection to
their own goldens. Rel/DataFlow validator tests construct malformed internal
objects and require loud failure.

## State and Authority Summary

| Fact | Current owner | Derived/checking views |
|---|---|---|
| Parsed declarations | `ParsedModule` | formatted round trip |
| Logical data dependencies | Query graph | `.df`, Query validators |
| Demand containment | `ProvMap` during analysis | identity-join decision |
| Table/index identities | Program build context | Query table annotations, Rel pointers |
| Differential op inventory/schedule | `DRFlowGraph` during build | Rel dump, validators, emitted-effect cross-checks |
| Eager imperative region shape | ControlFlow eager builders | Rel marker/event enrollment and cross-checks |
| Executable schedule | ControlFlow `Program` | `.ir`, generated C++ |
| Runtime relation state | generated `Database` | stdout/logs, oracle comparison |
| Runtime operation counts | `gBenchCounters` in instrumented process | benchmark TSV or calibration output |

The proposed CostModel needs a new, explicit owner for an immutable cost input.
Neither the textual dumps nor `DRFlowGraph` currently fill that role cleanly.
