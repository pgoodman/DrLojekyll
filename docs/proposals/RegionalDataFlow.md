# Regional dataflow: identity-safe demand and keyed instances

> **Status: superseded.** [Regional DataFlow Core](RegionalDataFlowCore.md)
> replaces this proposal. This file is retained as a non-normative design
> record; its recursive region calls, event ports, lifecycle model, language
> decisions, implementation sequence, and acceptance criteria must not guide
> implementation. The remainder is intentionally unedited so the rejected
> design and its evidence remain reviewable.

## 1. Decision summary

DrLojekyll should represent demand as nested, compiler-owned dataflow regions.
A region is a keyed database instance with a scoped symbolic key, a cyclic local
dataflow graph, and typed request, input, result, and event ports. Regions are
chosen top down from observations, optimized, recursively subdivided, and then
lowered through Rel to ControlFlow. The region ownership relation is a forest;
the graph inside each region and the graph of recursive region calls may be
cyclic.

This replaces two incompatible authorities in the current compiler:

1. `ApplyDemandTransform` mutates the logical DataFlow graph into a manual
   magic-set-shaped graph by fabricating parser objects and guard joins.
2. Rel later attempts to recognize part of that mutation and optionally lowers
   it as a `DRInstance` instead of as the flat guarded graph.

The replacement has one authority. Regional DataFlow decides semantic scope,
symbolic parameters, and port contracts. Rel decides incremental schedules,
state, effects, and rounds within those frozen contracts. ControlFlow realizes
the Rel schedule. Runtime storage is a physical choice, never the semantic
definition of a keyed instance.

The immediate language decision is:

- Remove declaration-level `@first`. It is parsed and printed but is not carried
  into `ProgramQuery`; generated query code always exposes a cursor. Its
  observable meaning would also depend on an unspecified physical order.
- Remove body-position `@first message(...)` as a demand mechanism. Today it is
  a manual query-time seed: generated query entry code injects an ADD message,
  which wakes a guarded dataflow before the query table is scanned. That was an
  explicit opt-in to demand transformation before demand transformation had a
  proper compiler representation. A regional request port subsumes it.
- Keep an explicitly invoked command/message operation if clients need a query
  that also performs a write. Do not disguise that effect as `first` or as an
  ordinary observational query.
- Define `@only`, if retained, over semantic members rather than visible
  payload tuples. It is coherent only after row/member identity is explicit.

The first implementation prerequisite is therefore not the region planner. It
is an identity contract that prevents a projection, constant substitution, or
region boundary from silently changing multiplicity.

## 2. What the executable compiler does now

The following is re-derived from executable code, not from proposal prose.

### 2.1 Current top-level pipeline

`Query::Build` in `lib/DataFlow/Build.cpp` approximately performs:

```text
build clauses
remove views
track differential properties
simplify
connect relation inserts to selects

if demand flag:
    ApplyDemandTransform(query)

Optimize(query)
convert constants
remove views
proxy inserts
link uses
identify inductions
finalize depths and column ids
track differential and post-init constants
build storage equivalence sets
stratify
```

ControlFlow construction then creates the global data model, eager message and
query entry procedures, and initialization. Rel builds a global inventory of
operations, vectors, state cells, and recognized instances, derives strata,
linearizes and validates their flow, and lowers stratum phases to ControlFlow.
ControlFlow optimization and code generation follow.

The semantic and physical responsibilities are interleaved:

- DataFlow graph shape drives logical optimization.
- `BuildEquivalenceSets` decides some storage sharing after optimization.
- `FillDataModel` adds imperative special cases for joins, differentials,
  impure maps, aggregates, and recognized demand shapes.
- Rel is authoritative for differential scheduling but receives keyed instances
  as a recognized special form inside a global graph.
- ControlFlow construction still creates eager structures around that Rel
  authority.

### 2.2 Current demand transform

`lib/DataFlow/Demand.cpp` implements a narrow syntactic graph rewrite:

```text
for a bound query adornment:
    trace its projection to one relation
    classify supported body guard sites
    reject unsupported shapes
        multi-clause and some self-join forms
        negation and aggregation
        sideways dataflow forms
        mixed all-free consumers

    fabricate ParsedMessage and ParsedLocal declarations
    create query IO and receive nodes
    create demand merge/projection nodes
    create guard joins at selected consumers
    rewire consumers through guards
    guard the query projection
    attach annotations describing the recognized shape
    register query-demand forcing
```

This is a magic-set-like transformation expressed as parser-shaped graph
surgery. It makes optimization reason about implementation scaffolding rather
than about demand as a semantic scope. The annotations then become a second,
fragile description of the graph they annotate.

### 2.3 Current keyed-instance lowering

With the keyed-instance option enabled, Rel resolves live annotated guard joins
and constructs a `DRInstance` roughly equivalent to:

```text
DRInstance {
    demanded_view
    published_view
    forcing_message
    demand_table
    input_table
    publication_table
    key_columns
    row_columns = published columns minus key_columns
    differential
}
```

`BuildSubgraphInstanceOps` creates the operation. `InstanceStore<Key, Row>`
maintains dense instance ids and current/next buffers; the lowering rescans an
instance and publishes born/dropped rows around a seal. The supported graph
shape is intentionally narrow and presently fences maps, negation,
aggregation, key/value input, and substantial recursive cases.

The prior next step was to extend this `DRInstance` with a per-instance
fixpoint/round dimension, replacing full rescan with incremental recursive
maintenance and relaxing those fences. That path would deepen the split:
DataFlow would keep constructing flat guards while Rel increasingly treated a
recognized subset as a nested database.

### 2.4 What `@first` actually does

There are two distinct spellings that should not be conflated.

Declaration-level query `@first` is dead metadata:

```text
parse annotation
format annotation
discard before ProgramQuery
generate the same query cursor as an unannotated query
```

If a scalar query API is wanted later, make cardinality a checked contract:
prove at-most-one from keys, or define an explicit deterministic order/reducer.
“Return whichever physical row is encountered first” is not a relational
semantics and cannot safely drive storage or region planning.

Body-position `@first msg(K)` participates in forcing:

```text
query(arguments):
    inject ADD msg(bound_arguments)
    run the message-driven dataflow
    scan the query result table
```

The message is simultaneously an input/guard in DataFlow and a query-entry
side effect in ControlFlow. The automatic demand builder is a thin sibling of
this path: generated demand forcing is preferred, with parsed `@first` forcing
as a fallback. Parsed `@first` is force-only: unlike the newer generated demand
path, it has no matching retract entry and therefore no scoped query lifetime.
That is precisely a split-brain seam. Demand is alternately source-authored and
compiler-inferred, but both depend on a message whose meaning is broader than
an ordinary declared message.

Regional request ports eliminate the need for either spelling. A bound query
enters a root region with an `InstanceKey`; the resulting request birth drives
the region. There is no fallback and no synthetic language declaration.

### 2.5 Split-brain audit

The current contradictions are architectural, not merely duplicated code:

| Concern | Current authority A | Current authority B | Category mismatch |
| --- | --- | --- | --- |
| Demand | Top-down, syntactic `ApplyDemandTransform` | Post-hoc Rel recognizer over optimized graph shape | Semantic scope versus incidental shape |
| Forcing | Parsed `@first` message side effect | Compiler `QueryDemandForcing` registry | User-authored event versus inferred query request |
| Keyed instance | Flat guard joins in ordinary DataFlow | Optional nested `DRInstance` lowering | Two permanent execution meanings for one object |
| Row identity | Greedy value-column liveness and attached-column heuristics | Equality of all surviving physical table fields | Needed values versus semantic distinctions |
| Storage | Post-optimization equivalence sets | Imperative `FillDataModel` operator exceptions | Global logical sharing versus local physical necessity |
| Scheduling | Eager ControlFlow walk creates structures/markers | Rel claims sole differential schedule authority | Construction traversal versus operational dependency graph |
| Aggregate multiplicity | Set of projected `over` payload tuples | User expectation of contributing source members | Value equality versus member identity |

The target does not reconcile these pairs with synchronization annotations. It
deletes one side of each pair: Regional DataFlow owns scope and contracts, Rel
owns operational scheduling, and post-freeze arrangement planning owns storage.
Typed ids and validators connect those authorities without letting either
re-infer another's decisions.

### 2.6 The intended architecture immediately before this proposal

The keyed-instance work was heading toward a capable but still post-hoc nested
lowering. In pseudocode, its intended extension was:

```text
graph = BuildOrdinaryDataFlow(program)
if demand enabled:
    graph = RewriteToFlatMagicGuards(graph)
graph = Optimize(graph)

recognized = RecoverEligibleGuardedSubgraphs(graph.annotations)
rel = BuildGlobalRelFlow(graph)

for subgraph in recognized:
    if cost prefers nested and operator fences pass:
        instance = DRInstance(fixed_key_columns, residual_publication_row)
        if subgraph is recursive:
            add per-instance Rel round/fixpoint
        choose rescan or indexed incremental maintenance
    else:
        retain ordinary flat guarded lowering

lower rel and any instance special procedures to ControlFlow
```

The planned advances—recursive instance rounds, indexed rescans, differential
demand/input handling, more adornments, and cost selection—are individually
useful. The problem is their home. A fixed `DRInstance` recovered after global
optimization cannot express recursive regions within regions, re-optimize a
parent after a child interface shrinks, negotiate mutable internal message
schemas, or distinguish a sequestered key from an erased identity column. Each
new operator would add a recognition fence and another special lowering case.

The regional design keeps the useful runtime and Rel mechanisms but moves the
semantic decision earlier. It therefore changes the intended architecture, not
just the implementation sequence.

## 3. The category error: physical columns are not row identity

The current compiler does not have a first-class answer to “which fields keep
these two semantic rows distinct?” Runtime table hashing and equality use the
physical row fields. Optimizer liveness asks whether a column value is used.
Attached columns and keep-last-edge rules preserve enough physical shape for
particular operators. Provenance tracks value containment forward. None of
these is a backward semantic identity analysis.

This makes the following rewrite unsafe without more information:

```text
outside: left(K, A), right(K, B)

inside instance K = κ:
    left'(A), right'(B)
```

The intended equality is not `(K, A) == (A)`. It is:

```text
global_row(K, A) == qualified_row(instance_path(K), local_row(A))
```

`K` is sequestered into the instance path. It is constant inside the region and
need not be passed through every local operator, but it still qualifies local
identity, storage, retractions, and result publication.

A Boolean “identity-tainted” bit is insufficient. Identity can have alternative
candidate keys, can be minimized by equalities and constants, can gain witness
components at joins, and can be transformed by functors. The compiler needs a
structured contract.

### 3.1 Named semantic quantities

At minimum, use distinct domain types for:

```text
FieldId
LogicalNodeId
RegionId
LocalNodeId
PortId
CallSiteId
InstanceId
InstanceKey
InstancePath
SemanticMemberKey
DerivationSupportCount
DemandSupportCount
DeltaSign
```

In particular, these quantities must never share a bare integer contract:

- semantic membership: whether a member exists in a relation or aggregate;
- derivation support: how many derivations currently support that member;
- demand support: how many callers currently require an instance;
- differential sign/count: the batch change being applied.

### 3.2 Row and use contracts

Each logical edge carries a row contract:

```text
RowContract {
    visible_fields: ordered FieldId set
    candidate_member_keys: antichain of FieldExpression sets
    derivation_support: SupportAlgebra
}

UseRequirements {
    values: FieldExpression set
    distinctions: DistinctionRequirement set
    presence: PresenceRequirement set
    effects: EffectRequirement set
    routing: RoutingRequirement set
}
```

The five requirements answer different questions:

- `values`: which values a consumer computes or publishes;
- `distinctions`: which differences between input members remain observable;
- `presence`: which matches or non-matches affect existence;
- `effects`: which invocations must happen exactly once or in a defined order;
- `routing`: which fields select an instance, shard, index, group, or port.

Candidate keys form an antichain because two incomparable keys may both be
valid. Region parameters can reduce a key; an equijoin can establish a
functional dependency; an injective functor can offer an alternative key. A
single transitive taint bit cannot express any of these.

The conservative initial contract is “all source columns distinguish.” Later
passes may prove a smaller key. Failing to prove erasure safe retains the
field or moves it into the `InstancePath`; it never silently collapses rows.

### 3.3 Operator transfer rules

The backward requirement pass and forward row-contract pass meet at each
operator. Their first normative rules are:

| Operator | Semantic member identity |
| --- | --- |
| Declared relation/message | Declared key, or all declared fields until explicit keys exist |
| Filter/compare | Inherits input member key; predicate fields are value/presence requirements |
| Tuple/map projection | Preserves hidden member key unless this node is an explicit semantic set projection |
| Merge/union | Output tuple key; arm identity contributes only to derivation support |
| Join | Union of contributor member keys, minimized by proven equality, constants, and functional dependencies |
| Negation | Positive member key; negative fields are presence requirements |
| One-to-one pure functor | Input member key, unless declared injective outputs provide an equivalent key |
| One-to-many pure functor | `(input member key, free output tuple key)` |
| Impure functor | Authoritative keyed invocation and stored result identity; never cloned speculatively |
| Aggregate | Group key for output; explicit member key for inputs |
| Key/value state | Explicit key, value/member identity, and update algebra |

An operator may discard a visible value while preserving a hidden member key.
An explicit relation-head projection may intentionally collapse equal projected
tuples; that is a semantic set boundary and must be represented as such rather
than inferred from physical storage shape.

### 3.4 Aggregates and `@only`

Aggregate inputs need four roles:

```text
AggregateInput {
    group_key: FieldExpression set
    member_key: SemanticMemberKey
    value_fields: FieldExpression set
    configuration_fields: FieldExpression set
}
```

For an input whose proven semantic key is exactly `{A, B}`, moving `A` into a
region key may reduce the *local* residual key to `{B}`. The proof is not “A is
unused”; it is:

```text
global member key {A, B}
    == qualified member key {InstancePath(A), B}
```

The compiler must not use `{B}` globally or share its aggregate state across
instances without the path qualification. If the aggregate ranges over source
members with an additional key `S` while `B` is only the fold payload, then
`S` must also survive invisibly and two equal payloads contribute twice:

```text
member (A=7, S=1, payload B=9) -> +9
member (A=7, S=2, payload B=9) -> +9
```

Within instance `A=7`, the state is keyed by the residual semantic member key,
which is `{B}` in the first case and `{S}` (or another proven key containing it)
in the second. It is never chosen merely by reusing the fold payload columns.
Only an explicit distinct projection before the aggregate collapses equal
payload values.

`@only` has the same dependency. It asks whether a semantic member is the sole
member in a group. It must not count visible payload encodings. Its incremental
implementation tracks a member set (or an equivalent exact state), so transitions
between zero, one, and many members publish the correct add/remove deltas.

There is no executable `@only` surface on the researched branch today, so it is
not a compatibility constraint. If introduced, it must be one ordinary regional
cardinality operator with one Rel lowering. Implementing it once as aggregate
payload logic and again as query/control-flow selection would create the same
split-brain failure as `@first`.

Aggregate execution itself only became end-to-end in commit `d6e31b79` on
2026-07-16. Projected-tuple distinctness was documented in `1ccc9be3` and given
a lint/witness in `c8888e44` on 2026-07-27. Those commits are valuable evidence
of current behavior, not a mature semantic constraint that overrides row-count
meaning. Phase A intentionally replaces “the `over` payload projection is the
member” with an explicit choice between hidden member preservation and an
explicit distinct projection.

## 4. Target IR: regional DataFlow

Introduce a semantic IR between normalized logical DataFlow and Rel. It may
reuse the current `QueryView` operator vocabulary initially, but it must have
new ownership, ids, ports, and contracts rather than using annotations on the
old graph.

```text
Parsed program
    -> normalized LogicalQuery                 existing cyclic DataFlow meaning
    -> RegionalQuery                           new canonical semantic IR
    -> RelProgram                              incremental operational IR
    -> ControlFlow Program                     executable schedule
```

Regional DataFlow is canonical even when no profitable nested region exists. A
program initially enters the IR as one bootstrap root region containing the full
normalized graph and all sealed observations. Once forest planning lands, its
first transform replaces that wrapper with stable observation-rooted regions
(coalescing observations when sharing wins), then recursively extracts children.
These are successive states of the same IR, not old/new execution paths. This
allows existing local simplification and constant propagation passes to operate
through one interface from the first landing.

### 4.1 Core types

```text
RegionalQuery {
    roots: ordered RegionId list
    regions: RegionId -> RegionTemplate
    logical_origins: (RegionId, LocalNodeId) -> LogicalNodeId
}

RegionTemplate {
    id: RegionId
    parent: RegionId or ProgramRoot
    parameters: ordered RegionParameter list
    request_ports: ordered FrozenRequestPort list
    input_ports: ordered FrozenInputPort list
    result_ports: ordered FrozenResultPort list
    event_ports: ordered FrozenEventPort list
    body: cyclic RegionalGraph
    calls: ordered RegionCall list
    row_contracts: EdgeId -> RowContract
    requirement_solution: EdgeId -> UseRequirements
}

RegionCall {
    call_site: CallSiteId
    callee: RegionId
    arguments: parent FieldExpression -> child RegionParameter
    request_identity: RequestIdentity
    result_mapping: child PortField -> parent FieldExpression
}

InstancePath = parent InstancePath + (RegionId, InstanceKey)
```

The region ownership relation is a forest. `RegionalGraph` is not a tree and is
not required to be acyclic. Calls may form strongly connected components for
recursive queries. “Christmas tree” describes top-down ownership and shared
prefixes of scoped keys, not the shape of the executable relation graph.

A logical node may be cloned into multiple regions. Each clone has a stable
`LocalNodeId` and the same `LogicalNodeId` origin. The source node is not deleted
merely because one dependent admitted it into a region.

### 4.2 Typed boundary contracts

Do not represent all boundaries as a generic message. Use distinct types:

```text
DeclaredMessagePort   // sealed external delta/event ABI
DeclaredQueryPort     // sealed external bound/free query ABI
OpenRegionPort        // compiler negotiates internal schema
FrozenRegionPort      // finalized internal request/result/delta ABI
```

Declared ports are sealed. The planner may not remove or reorder their fields,
even if a particular region could reconstruct one. Internal ports remain open
while a parent and child solve requirements. They freeze only after value,
distinction, presence, effect, routing, and differential needs reach a fixpoint.

The port kinds are not synonyms:

| Port | Direction and meaning |
| --- | --- |
| Request | Parent asks that a keyed child instance exist; carries reference-counted lifecycle support |
| Input delta | Predecessor supplies relation membership changes used by live instances |
| Result delta | Region publishes maintained membership changes to its callers/successors |
| Event | Ordered or effectful message whose occurrence is itself observable |

This formalizes the useful “every backed node is a small database” intuition.
A successor can probe/request a keyed view while predecessors continue to push
updates. It does not collapse those interactions into one generic message: a
request changes demanded lifetime, an input changes data, a result reports
maintained data, and an event performs an effect.

Symbolic parameters cannot leak across a frozen boundary. The parent has a
value expression; the child has a scoped `RegionParameter`; the call explicitly
maps between them. Inside the child, substitutions can treat the parameter as a
constant. Across the boundary, ordinary constant propagation stops.

The same rule applies to a predecessor or successor of a request or event port:
no optimizer may push a child symbolic constant into an unrelated parent edge,
or pull an external value into the child without an explicit port field.

### 4.3 Port mutability and field erasure

An internal port field can be removed only if all of these are true:

```text
no consumer needs its value
no semantic member key needs its distinction
no presence test needs it
no effect identity/order needs it
no route, group, index, or retraction lookup needs it
and, if it is a region parameter, InstancePath still qualifies local identity
```

This makes internal request/message shapes mutable during planning. Declared
message shapes remain sealed. The distinction is explicit in the type system,
not a convention checked at scattered call sites.

## 5. Top-down recursive region planning

The planner begins at observations, not at sources. Roots are declared queries
with their adornments and externally published message/relation outputs.

### 5.1 Planning algorithm

```text
PlanRegions(logical_query):
    normalize and connect the logical graph
    logical_sccs = ComputeSCCs(logical_query)
    contracts = InferConservativeRowContracts(logical_query)

    regional = CanonicalBootstrapRoot(logical_query)
    regional = SeedObservationForest(regional)

    for root in stable observation order:
        PlanRegion(root, logical_sccs, contracts)

    SolveRecursivePortSCCs(regional)
    ValidateRegionalQuery(regional)
    return regional

PlanRegion(region, logical_sccs, contracts):
    SubstituteScopedParameters(region)
    OptimizeRegionToFixpoint(region)
    requirements = SolveBackwardRequirements(region)
    MinimizeOpenPorts(region, requirements)

    loop:
        candidates = EnumerateAdmissibleChildSlices(region, logical_sccs)
        if candidates is empty:
            break

        candidate = StableBestCandidate(candidates, RegionCost)
        child = ExtractChildWithoutDeletingLogicalOrigins(region, candidate)
        PlanRegion(child, logical_sccs, contracts)
        FreezeChildInterfaceOrJoinRecursiveSCC(child)
        ReplaceSliceWithOpaqueRegionCall(region, child)

        // The child's smaller port and new opaque boundary change the parent.
        SubstituteScopedParameters(region)
        OptimizeRegionToFixpoint(region)
        requirements = SolveBackwardRequirements(region)
        MinimizeOpenPorts(region, requirements)

    FreezeRegionWhenAllChildAndRecursiveContractsAreStable(region)
```

`SeedObservationForest` preserves one logical-origin pool and may clone a
logical node into several roots; it removes the bootstrap wrapper after every
sealed observation is covered. It may coalesce compatible observations under
one root when their contracts and update stream are genuinely shared. There is
never a runnable bootstrap plan beside a runnable forest plan.

The important ordering is “form one level, optimize, recurse, return a frozen
child interface, then optimize the parent again.” Region extraction is not a
single graph partition followed by cleanup. The result of optimization at each
level changes which inputs and distinctions the next level needs and may expose
new profitable children.

For determinism and debuggability, choose one candidate at a time using stable
ids and a total tie-break. Independent future planning may run concurrently,
but its committed result must be equivalent to this serial ordering.

### 5.2 Admissibility before profitability

A candidate slice is admissible only when:

```text
all cloned effects are clone-safe, or the effect stays in one authoritative region
all required semantic distinctions have a realizable local or qualified key
all request and result retractions have an exact lookup identity
sealed declared ABIs remain unchanged
the slice respects stratification and recursive SCC rules
a differential maintenance strategy exists for every crossing edge
no symbolic parameter escapes except through an explicit port mapping
```

Cost never legalizes an inadmissible transform. It ranks legal alternatives.

### 5.3 Profitability and useful redundancy

The planner should be permitted to create redundant regions and arrangements.
One logical subgraph may be materially useful under two adornments or sort
orders. Duplication can reduce row width, isolate concurrency, make storage
columnar, and remove cross-instance synchronization.

A region key need not originate in a declared bound query. An internal join can
turn each incoming pivot `K` into a request for a keyed view of another input;
an aggregate group or key/value probe can do the same. In the useful operational
model, an incoming join row both pushes its own delta and changes demand for
matching instances on the other arms. The planner may therefore create a keyed
child under an all-free root, or duplicate the same logical input into two
consumer-specific keyed regions, when the arrangement/locality benefit repays
the request and update fanout. External adornment is one source of symbolic
keys, not an eligibility requirement.

Use a named cost expression, not raw incomparable numbers:

```text
RegionBenefit =
    PrunedWork
  + LocalityBenefit
  + ArrangementReuseBenefit
  + ConcurrencyBenefit
  - RequestTrafficCost
  - DuplicatedStorageCost
  - UpdateFanoutCost
  - LifecycleCost
  - FixpointCost
```

Every term has a named unit or calibrated estimate. The cost model chooses
among semantically equivalent regional plans; it does not determine semantics.

Current storage equivalence sets are not an authority during region planning.
Existing equivalence may be a cohesion hint—if two nodes must share state, a
candidate that keeps them together is likely attractive—but regionization
invalidates and recomputes physical sharing. Sharing decisions occur after
ports and qualified identities are frozen.

### 5.4 Relationship to magic sets and SLDMagic

The planner is a symbolic, top-down demand transformation in the same broad
family as magic sets: a bound query creates a representation of relevant calls,
and those calls restrict bottom-up derivation. It is closer in spirit to
SLDMagic than to a conventional adorn-and-rewrite pass because it recursively
specializes calling contexts and tries to avoid materializing intermediate
lemmas that exist only to join a subquery back to its caller.

The analogy is a design guide, not an IR definition:

| Literature concept | Regional DataFlow counterpart |
| --- | --- |
| Adorned predicate | Region template plus typed parameter/result contract |
| Magic fact | Reference-counted region request |
| Bound query constant | Scoped symbolic `RegionParameter` mapped from a caller value |
| Sideways information passing | Parent/child port requirements and local optimization |
| SLD goal/calling context | `InstancePath` plus `CallSiteId` |
| Bottom-up rewritten rules | Rel operations within an instance-qualified scope |

Three differences are load-bearing. First, constants remain symbolic until a
runtime request instantiates them. Second, request and result lifetimes are
differential and must support retraction and multiple owners. Third, region
selection is interleaved with ordinary optimization and recursive port
minimization rather than performed as a one-shot source rewrite. These are why
fabricating magic predicates in the existing DataFlow graph is too weak a
representation even though its mathematical intuition is relevant.

## 6. Cycles and recursive trees within trees

There are three distinct structures:

```text
region ownership forest            acyclic nesting and scoped keys
local relation graph               may contain dataflow SCCs
region call graph                  may contain recursive call SCCs
```

Conflating them is the source of many false DAG assumptions.

### 6.1 Key-invariant recursion

If every recursive edge preserves the region key, keep the SCC inside one
region and run an instance-qualified fixpoint:

```text
for (RegionId, InstanceId):
    while local recursive delta queues are non-empty:
        execute next Rel round
```

The existing Rel induction/round model is reused, but all vectors, state cells,
and queues are qualified by region and instance.

### 6.2 Key-changing recursion

If recursion derives a new key and recursively demands another instance, model
an explicit recursive region call. Request identity includes caller path,
callsite, and callee key. Calls in the same recursive SCC share a port-schema
fixpoint and are frozen together.

The conservative first implementation keeps an SCC atomic unless a dedicated
conversion proves key-changing recursion safe. No heuristic may cut arbitrary
back-edges and hope Rel reconstructs the intended recursion.

### 6.3 Stratification

Negation and aggregation retain their stratification requirements in two
places: inside a region and across the region-call SCC graph. A recursive
aggregate is accepted only when lower-stratum membership is complete at the
frontier where its group update occurs. An unstratified regional plan is
rejected even if the original flat graph happened to pass a shape-based check.

## 7. Rel owns operations, not region discovery

Rel consumes a frozen `RegionalQuery`. It never rediscovers regions from graph
annotations, pointer adjacency, fabricated messages, or guard-join shapes.

Replace the global graph plus exceptional `DRInstance` model with:

```text
RelProgram {
    root_scope: RelRootScope
    regions: ordered RelRegion list
}

RelRegion {
    id: RegionId
    parent: RegionId or ProgramRoot
    port_schemas: FrozenRegionPorts
    instance_path_layout: InstancePathLayout
    scope: RelScope
    lifecycle: RegionLifecycleOps
}

RelScope {
    tables
    vectors
    operations
    state_cells
    induction_rounds
    dependency_order
    effect_summary
}

RegionLifecycleOps {
    request_add
    request_remove
    input_delta
    evaluate_to_fixpoint
    result_add
    result_remove
    seal_epoch
}
```

The concrete operation names may change, but every lifecycle effect must be
explicit and enumerable. Rel validates that request births can cause evaluation,
request deaths can retract all published results before state is cleared, input
deltas can reach every live instance, and all recursive queues participate in
the region fixpoint.

ControlFlow lowering uses stable typed ids to map Rel tables, vectors, ports,
and instance stores. It does not look up `QueryView *` identities. A runtime
`InstanceStore` is one physical implementation of a `RelRegion`; an indexed
shared input plus per-key state or a recompute-on-touch strategy are equivalent
alternatives under the same lifecycle contract.

## 8. Differential and lifecycle semantics

Keyed demand is not Boolean. Two callers may require the same callee instance.
Dropping one request must not destroy the other caller's results.

```text
RequestIdentity = (CallerInstancePath, CallSiteId, CalleeInstanceKey)

on request ADD:
    increment DemandSupportCount for callee key and request identity
    if total support changes 0 -> 1:
        create/activate instance
        subscribe or seed its inputs
        evaluate to fixpoint
        publish net results

on request REMOVE:
    decrement the matching support
    if total support changes 1 -> 0:
        emit removals for every published result
        unsubscribe inputs
        clear state only after removals are visible
```

### 8.1 Input changes

An input delta arriving after demand exists must update every subscribed
instance. A request arriving after input exists must observe the complete
current input. Rel may choose among:

```text
IncrementalPerRow
RecomputeOnTouch
SharedInputIndex
```

All strategies implement the same birth/death and net-delta semantics. Strategy
selection is physical and can vary by region.

### 8.2 Epoch ordering

Demand and data changes in the same batch must commute at the observable
boundary. An epoch drains request, input, recursive, and publication queues to a
fixpoint, then publishes the net result. Temporary add/remove flaps do not leak
merely because one queue happened to run first.

Do not reuse an `InstanceId` initially. Compaction can be added only after an
id-generation scheme proves that delayed deltas cannot alias a new instance.

### 8.3 Differential recursion

The operational product is:

```text
(RegionId, InstanceId, InductionRound)
```

Existing DRed-style born/dropped accounting is qualified by the first two
components. Duplicate regional materializations receive the same logical input
delta but maintain independent derivation and demand support. Child result
deltas become parent input deltas; a child death therefore cascades through the
same ordinary dependency graph rather than through a special cleanup path.

## 9. Aggregate lowering inside regions

Aggregate state is scoped by both the instance and group:

```text
AggregateStateKey = (InstancePath, GroupKey)
AggregateMemberMap = SemanticMemberKey -> CurrentContribution
```

For an invertible aggregate:

```text
on member support 0 -> 1:
    apply +contribution exactly once
on member support 1 -> 0:
    apply -contribution exactly once
```

For a recompute aggregate, retain exact members and rebuild the group value.
Configuration fields may be region parameters or group-qualified fields, but
their role must not be confused with a member value or identity field.

The aggregate output member key is the group key qualified by `InstancePath`.
Removing a key field from the local group tuple is safe only because the path
retains it. Removing an input identity field is safe only when a residual
`SemanticMemberKey` still distinguishes every contributing member.

This contract also repairs existing aggregate projection ambiguity. A payload
projection and a distinct-member projection become different nodes. Projection
collision is intentional only at the latter.

## 10. Functors and effects

Functor declarations need semantic contracts independent of their C++ calling
shape:

```text
FunctorContract {
    purity
    cardinality: one_to_one | zero_or_one | one_to_many
    injectivity: proven mappings from inputs to outputs
    effect_identity
    retraction_strategy
}
```

- A pure non-injective function may compute a value but cannot replace an input
  member key with that value.
- A one-to-many function extends member identity with the emitted free-output
  identity.
- An impure function is not cloned into redundant regions. It runs once in an
  authoritative keyed scope, and its result is stored so retraction does not
  invoke an unrelated second effect.
- Until an impure function supplies an exact effect and retraction contract, it
  is inadmissible inside an extracted child region.

These rules should be enforced by types and validators. Warning-and-continue or
fallback-to-flat paths would recreate the architectural split.

## 11. What to adapt from Datatoad

Research baseline: `frankmcsherry/datatoad` commit `9caaa13`. Datatoad combines
columnar sorted storage, redundant relation forms, seed-bound planning, and a
uniform queryable relation abstraction. Its worst-case-optimal join executor is
not the relevant prerequisite here.

### 11.1 Adopt now

1. **Seed-bound planning.** Datatoad plans from grounded terms and grows a stage
   as terms become available. Region planning similarly starts at a bound query
   or observation and works backward from its symbolic parameters.
2. **Sequester bound columns.** Datatoad temporarily removes irrelevant columns
   from the active join prefix and reassembles them later. A region moves bound
   key columns into `InstancePath`, reducing repeated local row width while
   preserving qualified identity.
3. **Redundant read forms.** A relation may maintain multiple read-optimized
   forms, synchronized from one logical update stream. DrLojekyll can maintain
   per-adornment/per-region arrangements with different key-prefix sort orders.
4. **Columnar sorted prefixes.** Store instance-key columns, residual member-key
   columns, payload columns, and support columns by role. A sorted prefix trie is
   a natural layout for repeated probes under a bound key.
5. **LSM-style immutable batches.** Sorted-distinct batches with size-doubling
   merges provide a simple ingestion path and make redundant forms practical.
6. **Queryable relation capabilities.** Region inputs/results should expose
   typed key-probe, enumerate, and count capabilities. Rel can choose a stored or
   computed implementation without changing the caller contract.
7. **Backward payload liveness.** Datatoad retains only columns needed by later
   work. DrLojekyll should do this using the richer five-part requirements so
   value liveness never erases member identity.

### 11.2 Important adaptation

Datatoad forms intentionally deduplicate collisions after projection. That is
correct for a set-valued transformed relation, but it must not become an
accidental identity rule in DrLojekyll. A regional arrangement may project away
payload columns for reading while carrying a hidden member key or support count.
Collision means semantic collapse only at an explicit distinct projection.

### 11.3 Concrete regional layouts

For a logical relation `T(K, A, B)` used inside a region keyed by `K`, keep the
semantic descriptor separate from any one layout:

```text
RegionalArrangementContract {
    instance_key = [K]
    residual_member_key = [A, B] or another proven key
    payload = fields needed only as values
    support = derivation/demand counters required by the regime
    capabilities = {probe, enumerate, count, apply_delta}
}
```

Then measure three physical families:

1. **Global prefix forest.** Store sorted `(K, residual member key, payload)`
   columns in one Datatoad-like trie. An instance is a prefix range. This has
   low per-instance overhead, naturally supports batched merge, and shares
   storage across cold keys. Updates and compaction touch global structures.
2. **Instance directory.** Map `K -> InstanceId -> residual columnar forest`.
   `K` is absent from every residual row. Hot instances can update and evaluate
   concurrently, but many tiny instances pay directory/allocation overhead and
   cross-instance scans are weaker.
3. **Hybrid key-partitioned LSM.** Ingest immutable sorted
   `(K, residual key, payload)` batches, retain prefix ranges per `K`, and compact
   hot keys into dedicated residual forests. This preserves batch efficiency
   while allowing selective per-instance isolation.

Do not bake one family into `RelRegion`. Rel requests capabilities and records a
chosen strategy; ControlFlow/runtime implement it. The same semantic region may
have redundant read forms, for example:

```text
form 0: [K, A, B]    request/probe by K then enumerate A/B
form 1: [K, B, A]    join validation by K/B
form 2: [K] + prefix_count
```

All forms are read-only derivatives of one authoritative logical update stream.
On an input delta, derive each form's keyed batch, preserve or count hidden
member collisions according to its `RowContract`, then advance `recent` to
`stable`. A form is never independently writable.

Keep column roles physically distinct:

- instance-key columns participate in the directory or leading trie prefix;
- residual member-key columns establish local uniqueness and retraction lookup;
- payload columns can be late-materialized or omitted from count-only forms;
- derivation and demand support use separate typed side arrays;
- epoch flags/round membership are transient side structures, not tuple fields.

Prefix counts can answer aggregate cardinality or join appraisal without
enumerating payload. Late materialization lets a join align narrow key/member
columns before fetching wide payload. Repeated or low-cardinality columns can
later use dictionary/RLE encoding, but only after the role split is correct;
compression must not become another source of semantic deduplication.

The arrangement cost model measures at least bytes per live member, number and
size distribution of instances, probe fanout, merge write amplification,
duplicate-form update fanout, payload materialization rate, and contention. It
should be calibrated on the actual generated operations rather than chosen from
row-count estimates alone.

### 11.4 Defer

Do not adopt worst-case-optimal multiway joins as part of this change. WCOJ would
replace too much of the current builder while DataFlow, Rel, aggregates, and
differentials are moving. The useful near-term lesson is “sort, prefix, project,
and keep redundant forms,” not “replace binary join planning.” Binary joins can
probe region arrangements through the same queryable capability and later share
the storage work of a WCOJ implementation if one becomes worthwhile.

## 12. Pseudocode architecture diff

### 12.1 Semantic construction

```diff
 Query::Build(parsed):
     graph = BuildClauses(parsed)
     Normalize(graph)
     ConnectInsertsToSelects(graph)
-    if options.demand:
-        ApplyDemandTransform(graph)
     Optimize(graph)
-    BuildEquivalenceSets(graph)
-    Stratify(graph)
-    return graph
+    InferRowContracts(graph)
+    regional = BuildCanonicalRegionalQuery(graph)
+    regional = PlanRegionsTopDown(regional)
+    OptimizeAndFreezePortsToFixpoint(regional)
+    ValidateRegionalQuery(regional)
+    StratifyRegionalQuery(regional)
+    return regional
```

### 12.2 Query entry and forcing

```diff
 BuildQueryEntry(query):
-    if generated_demand_forcing(query):
-        inject fabricated demand message ADD
-    else if parsed_first_forcing(query):
-        inject declared @first message ADD
-    scan query result table
+    request = RootRequest(query.bound_arguments)
+    acquire request support
+    evaluate affected region scopes to epoch fixpoint
+    expose result cursor qualified by request
+    release request according to the query/session lifetime contract
```

An API whose result cursor outlives the call needs an explicit request lease.
The lease is part of the generated API/runtime contract, not an optional
callback and not an implicit `@first` message.

### 12.3 Region extraction

```diff
-RecognizeGuardAnnotations(flat_graph):
-    find demand JOINs and fabricated messages
-    recover demanded/publication views
-    construct optional DRInstance
+PlanRegion(parent):
+    solve contracts and optimize parent
+    candidate = best admissible backward slice
+    child = clone slice with stable logical origins
+    map bound expressions to child symbolic parameters
+    recursively plan and optimize child
+    freeze child request/result/delta ports
+    replace slice with typed RegionCall
+    re-optimize parent against the frozen interface
```

### 12.4 Rel lowering

```diff
-BuildDRInventory(global_dataflow):
-    derive global operations, vectors, cells, strata
-    recover recognized DRInstance special operations
-    lower instance stores through a separate path
+BuildRelProgram(regional_query):
+    for region in stable ownership/SCC order:
+        consume frozen port and row contracts
+        build instance-qualified tables, vectors, cells, and rounds
+        build explicit request/input/result lifecycle operations
+        choose a physical maintenance strategy
+        validate effects and differential closure
+    derive cross-region dependency schedule
+    lower one coherent RelProgram to ControlFlow
```

### 12.5 Storage planning

```diff
-BuildEquivalenceSets(pre-regional QueryViews)
-FillDataModel with operator-specific sharing exceptions
+for each frozen RelRegion:
+    enumerate required probe/enumerate/count capabilities
+    select shared or redundant arrangements by measured cost
+    lay out InstanceKey, MemberKey, payload, and support separately
+    validate every arrangement against the same RowContract
```

## 13. Rendering and debugging

The regional structure must be inspectable before its first optimization is
enabled. Add a deterministic textual regional dump and a DOT rendering.

Suggested text form:

```text
region r0 root query reachable/bf
  params [p0: Node]
  request q0 sealed-in [Node]
  result  o0 sealed-out values=[Dst]
             distinct=[path(p0), Dst]
  local-scc s0 recursive
    n0 origin=v17 SELECT edge
      row values=[Src,Dst] keys=[{Src,Dst}]
    call c0 -> r1 args=[p0 := n0.Dst]

region r1 parent=r0
  params [p0: Node]
  request i0 open/frozen [p0]
  result  o0 [Dst] distinct=[path(p0),Dst]
  ...
```

Every dump records:

- stable region, node, edge, port, callsite, and SCC ids;
- logical origin ids for clones;
- parameters and instance-path layout;
- sealed/open/frozen state and complete port schemas;
- fields by value, distinction, presence, effect, and routing role;
- candidate member keys and support algebra;
- child calls and recursive call SCCs;
- rejected candidate reason codes and the chosen cost terms when requested.

DOT uses region clusters and explicit port nodes. Cyclic local edges remain
visible; nested clusters must not visually imply the local graph is a tree.

The Rel dump adds physical arrangements, lifecycle operations, effect sets,
state-cell group/member keys, instance-qualified round ids, and the selected
maintenance strategy. The ControlFlow dump shows the corresponding procedures,
vectors, tables, request leases, and instance maps.

All ordering is by stable typed id. Pointer order and unordered-container order
are forbidden on dump and code-generation paths.

### 13.1 Required validators

Fail compilation with a precise invariant violation for:

```text
unrealized semantic member identity
accidental member collapse at a physical projection
symbolic parameter escape
parent/child frozen-port disagreement
mutation of a sealed declared ABI
sequestered key absent from InstancePath qualification
functor cardinality/injectivity mismatch
unbalanced or ambiguous request ownership
missing member identity for aggregate retraction
missing lookup identity for a result removal
non-converged recursive port-schema SCC
RegionalQuery/Rel operation and effect census mismatch
Rel/ControlFlow lifecycle effect mismatch
```

Validator failures need directed death tests. No validator should warn and
continue into a flat or partially regional fallback.

## 14. Testing and golden strategy

Behavioral equivalence alone is insufficient because the main failures are
wrong identity, hidden forcing, and alternate architectural paths. Tests must
cover semantics, structure, lowering, and determinism.

### 14.1 Unit tests

- Forward `RowContract` and backward `UseRequirements` transfer for every
  operator.
- Candidate-key minimization under constants, equalities, and region params.
- Port negotiation and freeze, including recursive call SCC least fixpoints.
- Request support births/deaths with multiple callers.
- Aggregate member identity independent of fold payload.
- Functor cardinality and injectivity contracts.
- Storage arrangements preserving hidden member keys.

### 14.2 Semantic property tests

Build a small reference relational interpreter and compare flat logical meaning
against regional execution over generated small databases and delta batches.
Permute independent updates within an epoch. Compare the final relation and the
net published deltas, not an incidental internal scheduling order.

### 14.3 Directed feature matrix

| Area | Required witnesses |
| --- | --- |
| Join identity | Equal visible output from different left/right witnesses; key sequestered on both inputs |
| Aggregate | Duplicate values from different members; constant group key; invertible and recompute; configuration fields; `@only` transitions |
| Projection | Explicit distinct collapse versus payload-only physical projection |
| Functors | Non-injective one-to-one; one-to-many duplicate-looking outputs; impure rejection/authoritative placement |
| Adornments | Multiple bound patterns, shared originals, all-free query beside bound queries |
| Nested regions | Tree within tree; parent reoptimization after child freeze; key alias at two depths |
| Redundancy | Two regions clone one logical subgraph with different arrangements and identical semantics |
| Sealed ports | Declared message fields remain intact while an internal port shrinks |
| Demand lifecycle | Two owners then retract one; request before data; data before request; same-epoch add/remove flap; result removal before instance clear |
| Recursion | Linear, nonlinear, mutual, key-invariant, key-changing, and nested recursive demand |
| Differential recursion | Instance-qualified born/dropped rounds and demand death |
| Stratification | Negation and stratified aggregate across region calls; unstratified rejection |
| Other operators | Key/value state, conditions, products, inserts, and published messages |

### 14.4 Structural goldens

Curated cases carry mode-specific sidecars:

```text
case.region.golden    semantic regions, ports, keys, origins, SCCs
case.rel.golden       lifecycle, arrangements, cells, rounds, effects
case.ir.golden        final ControlFlow
```

Regenerate only through the existing explicit bless discipline. Run each
selected compiler invocation repeatedly and byte-compare dumps and generated
headers. Include witnesses whose construction order and first allocated id vary
while their canonical output remains identical.

The old force witness is replaced by two tests: parsing body-position `@first`
as forcing is rejected, and an otherwise identical bound query obtains results
through a regional request without source annotations.

### 14.5 End-to-end gates

Run the full corpus in every supported optimization mode, debug and release,
plus sanitizer configurations. Compare against the semantic oracle for every
new regional witness. A temporary shadow validator may compare plans during a
landing, but production execution has one canonical regional path and no flag
that selects the old demand transform.

## 15. Direct implementation sequence

Each step ends with one architectural authority. Do not retain the previous
path “for safety”; the preceding test oracle is the safety mechanism.

### Phase A: make identity expressible

1. Add typed field, row, member-key, support-count, and requirement contracts.
2. Initially infer all declared source fields as a candidate member key.
3. Thread contracts through existing DataFlow optimization and current lowering.
4. Add validators that prohibit silent physical projection collapse.
5. Split aggregate input roles into group, member, value, and configuration.
6. Repair aggregate and `@only` lowering to use semantic member identity.
7. Delete the advisory aggregate-projection warning. The type/IR contract either
   preserves members or represents an explicit distinct projection; warning and
   continuing with ambiguous semantics is not a supported third state.

Exit gate: aggregate duplicate-value witnesses and projection property tests
pass without relying on all physical payload fields for identity.

### Phase B: make RegionalQuery canonical

1. Introduce stable ids, region types, typed ports, and `InstancePath`.
2. Wrap every normalized graph in a single root `RegionalQuery` unconditionally.
3. Port existing local optimization passes to operate inside a region.
4. Add the regional textual/DOT dump and all boundary validators.
5. Move stratification and SCC census to the regional representation.

Exit gate: the whole corpus goes through one-root RegionalQuery with identical
semantics and deterministic new goldens.

### Phase C: replace forcing and the alternate lowering

1. Make bound query entry acquire a typed root request lease.
2. Implement one-level acyclic child extraction and frozen compiler ports.
3. Lower regions canonically through `RelProgram` and explicit lifecycle ops.
4. Remove `ApplyDemandTransform`, fabricated parser declarations,
   `QueryDemandForcing`, guard annotations, recognized-subgraph recovery,
   `DRInstance`, and the alternate keyed-instance flag.
5. Remove both `@first` language meanings and their fallback code/tests.
6. Recompute storage arrangements only after regional contracts freeze.

Exit gate: all non-recursive supported demand witnesses use regional requests,
and searches find no semantic dependency on the removed forcing vocabulary.

### Phase D: recurse structurally

1. Implement optimize/extract/recurse/freeze/reoptimize planning.
2. Add mutable open ports and recursive requirement minimization.
3. Add recursive region-call SCC schema fixpoints.
4. Support key-invariant local recursion, then intentional key-changing calls.
5. Qualify existing Rel rounds and state by region/instance.

Exit gate: recursive feature-matrix witnesses match the flat semantic oracle,
including nested and mutual recursion.

### Phase E: complete differential lifecycle

1. Add exact request identities and `DemandSupportCount` accounting.
2. Make input subscriptions and result removals explicit Rel operations.
3. Drain same-epoch request/data/recursive queues to net fixpoint.
4. Add instance-qualified DRed and aggregate member retractions.
5. Keep instance ids stable; add compaction only with a generation proof.

Exit gate: every lifecycle ordering permutation produces the same net deltas.

### Phase F: optimize physical forms

1. Add per-region key-prefix arrangements and capability-based probes.
2. Separate key, residual member identity, payload, and support columns.
3. Add sorted immutable batches and size-tiered merges where measurements
   justify them.
4. Permit cost-ranked redundant regions/forms for multiple adornments.
5. Add columnarization and concurrency measurements to the calibrated cost
   model.

Exit gate: measured witnesses show benefit without semantic or structural
fallbacks. WCOJ remains a separate future decision.

## 16. Deletions and documentation cleanup

The completed transition deletes rather than deprecates:

- source-level `@first` query and body forcing syntax;
- generated-demand versus parsed-first fallback selection;
- parser-object fabrication by a DataFlow optimization;
- demand guard annotations and graph-shape recognizers;
- the `-demand`/keyed-instance choice between permanent lowerings;
- `DRInstance` as a special operation recovered from a global graph;
- pre-region storage-sharing authority and its operator-specific exceptions;
- the aggregate-projection warning once explicit member/distinct contracts make
  ambiguity impossible;
- tests whose only purpose is to preserve the obsolete alternate paths.

Update comments and architecture documentation in the same slices. Comments
must state current invariants—qualified identity, sealed ports, lifecycle
effects, and regional ownership—not phase names, migration history, proposal
citations, or stories about the removed graph shape. `docs/Architecture.md`
must show RegionalQuery and Rel explicitly; source comments claiming that tuple
or insert projections preserve multiset meaning must be checked against the new
member contract and rewritten or removed.

## 17. Acceptance invariants

The design is complete only when all of these are continuously true:

1. There is one semantic demand representation: typed regional requests.
2. There is one production lowering: RegionalQuery to RelProgram to
   ControlFlow.
3. `K` may leave a local row only by remaining in `InstancePath` or by a proof
   that it is irrelevant to value, distinction, presence, effect, and routing.
4. Aggregates and `@only` count semantic members, not payload encodings.
5. A declared message/query ABI is sealed; an internal port freezes by fixpoint.
6. Region ownership is a forest, while local and call graphs may be cyclic.
7. Child optimization can change its port, and the parent is reoptimized after
   that port freezes.
8. Multiple callers are reference-counted by exact request identity.
9. Request/data ordering within an epoch cannot change net observable deltas.
10. Effects are neither cloned nor retracted without an explicit contract.
11. Physical redundancy is allowed only behind one logical contract and one
    update stream.
12. Every regional, Rel, and ControlFlow artifact is deterministic and
    validator-complete.

This architecture turns the user-level intuition into a precise rule: inside
`left(K, ...)` and `right(K, ...)` for a keyed instance, `K` is a scoped symbolic
constant. It stops being repeated payload, not part of meaning. Demand can then
push recursively into increasingly specialized regions without leaking
constants, losing multiplicity, or requiring source-authored forcing.

## 18. Executable grounding and research references

These anchors are the review map for implementation. Line numbers will move;
the named functions and contracts are the durable references.

### 18.1 DrLojekyll anchors

| Concern | Executable anchor |
| --- | --- |
| Pipeline ordering | `lib/DataFlow/Build.cpp`, `Query::Build`, including `ApplyDemandTransform`, `IdentifyInductions`, `BuildEquivalenceSets`, and `Stratify` |
| Demand graph rewrite | `lib/DataFlow/Demand.cpp`, `QueryImpl::ApplyDemandTransform` |
| Parsed forcing predicate enters body first | `lib/DataFlow/Build.cpp`, clause construction around `ParsedClause::ForcingMessage` |
| Generated-forcing-first fallback | `lib/ControlFlow/Build/Build.cpp`, `BuildQueryInjectorProcedure` |
| Dead declaration-level first accessor | `ParsedQuery::ReturnsAtMostOneResult`; its only executable consumer is `lib/Parse/Format.cpp` |
| Forcing and recognition metadata | `include/drlojekyll/DataFlow/Query.h`, `QueryDemandForcing`, guard annotations, and recognized subgraphs |
| Keyed operation recovery | `lib/Rel/Rel.cpp`, `BuildSubgraphInstanceOps` |
| Global Rel inventory | `lib/Rel/Rel.cpp`, `BuildDRInventory` |
| Rel-to-ControlFlow strata | `lib/ControlFlow/Build/Stratum.cpp`, `BuildStratumPhases` |
| Special keyed lowering | `lib/ControlFlow/Build/Procedure.cpp`, `LowerSubgraphInstances` |
| Physical model and sharing exceptions | `lib/ControlFlow/Build/Build.cpp`, `BuildDataModel` and `FillDataModel` |
| Current physical row equality | `include/drlojekyll/Runtime/Table.h` |
| Current aggregate state | `include/drlojekyll/Runtime/StateCell.h` and Rel group-update construction |

Reviewers should repeat three searches at every cutover: all source-language
`@first` handling, all demand/guard recognition metadata, and all optional
`DRInstance` dispatch. A phase is not complete while one of those searches
still reaches production semantics.

### 18.2 Datatoad anchors

At researched commit `9caaa13`:

| Lesson | Source anchor |
| --- | --- |
| Redundant read-only transformed forms | `src/facts/mod.rs`, `Relations`, `Forms`, `ensure_action`, and `advance` |
| Projection collisions are deduplicated | `src/facts/mod.rs` module contract and transformed-form maintenance |
| Sorted columnar prefix trie | `src/facts/trie.rs`, `Forest` and `Layer` |
| Size-tiered immutable layers | `src/facts/mod.rs`, `FactLSM::tidy` |
| Grounded seed planning | `src/rules/plan.rs`, `PlanAtom::ground`, `plan_rule`, and `plan_body` |
| Backward projection targets | `src/rules/plan.rs`, final stage projection-target pass |
| Sequestering and reassembly | `src/rules/exec.rs`, multi-atom execution around `wco_join_inner` |
| Uniform count/propose/validate access | `src/rules/exec.rs`, executable atom protocol |

The public research sources are:

- Frank McSherry, [Datatoad](https://github.com/frankmcsherry/datatoad).
- Stefan Brass, [A Framework for Bottom-Up Simulation of
  SLD-Resolution](https://arxiv.org/abs/1405.4021).
- Stefan Brass, [Implementation Alternatives for Bottom-Up
  Evaluation](https://doi.org/10.4230/LIPIcs.ICLP.2010.44).

The literature supports the goal-directed and calling-context analogy. It does
not supply DrLojekyll's identity, differential lifecycle, regional port, or Rel
contracts; those are requirements derived from this compiler's semantics.
