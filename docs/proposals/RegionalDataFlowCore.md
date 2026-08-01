# Regional DataFlow core: identity-safe acyclic demand regions

> **Status: proposed replacement architecture.** This document is normative
> about the target design. It supersedes
> [RegionalDataFlow.md](RegionalDataFlow.md), which remains only as a design
> record. The implementation is greenfield: there is one Regional DataFlow
> representation and one Regional-to-Rel lowering, with no old/new execution
> selector or compatibility path.

## 0. Decision

DrLojekyll should represent demand as compiler-owned, identity-safe dataflow
regions with exact request ownership.

A regional program contains an effect-owning `ProgramRoot` and a forest of pure
region templates. Region ownership and region calls follow the same acyclic
parent-to-child relation. A region's local relational graph may be cyclic;
recursive relation evaluation remains an instance-qualified local fixpoint.
Region calls never form recursive strongly connected components.

The compiler pipeline is:

```text
Parsed program
    -> normalized LogicalProgram
    -> PlanningRegionalProgram
    -> FrozenRegionalProgram
    -> RelProgram
    -> ControlFlow Program
```

The authorities are disjoint:

- Logical normalization owns source meaning and explicit set projection.
- Planning Regional DataFlow owns region selection and open port negotiation.
- Frozen Regional DataFlow owns semantic identity, lexical scope, and ports.
- Rel owns differential state, local fixpoints, lifecycle scheduling, and
  caller-qualified result maintenance.
- ControlFlow mechanically realizes the Rel schedule.
- Runtime storage implements required capabilities; it does not define member
  identity or demand ownership.
- `ProgramRoot` owns external commands, observable effects, and committed
  publication. Pure regions do not contain or transport effects.

The first production slice supports root regions and acyclic child regions.
Key-changing recursive region calls, recursive port-schema SCCs, in-region
events, speculative region duplication, cost-ranked extraction, and physical
layout research are not part of this architecture.

## 1. Split-brain methodology audit

This design was derived from executable code first. Comments, tests, and the
superseded proposal were then used as evidence, not as architectural authority.

### 1.1 Executable architecture map

The current compiler behaves approximately as follows:

```text
input boundary:
  parsed clauses/messages/queries
      -> QueryView graph

logical construction [bottom-up, normalization]:
  build clauses
  connect inserts to selects
  infer differential properties

demand path [top-down, syntactic forcing, mode-selected]:
  if -demand:
      find one supported bound-query shape
      fabricate parser message declarations
      insert demand readers and guard joins
      stamp GuardAnnotation records
      register QueryDemandForcing
      register RecognizedSubgraph

logical optimization [fixpoint, graph rewriting]:
  canonicalize / CSE / simplify the ordinary and fabricated graph together
  finalize pointer-backed views, columns, induction groups, and storage sets

operational discovery [bottom-up, shape recovery]:
  BuildDRInventory(global QueryView graph)
  if -demand-instance:
      recover live guarded shapes from RecognizedSubgraph + annotations
      mint DRInstance operations
  else:
      retain the flat guarded execution

lowering [eager, imperative scheduling]:
  build ControlFlow procedures, tables, vectors, state cells, and instance stores
  query entry injects a fabricated ADD before returning a table cursor
  optional separate entry injects a REMOVE

runtime state [incremental plus eager rescan]:
  DiffTable presence = packed derivation counters > 0
  demand liveness = fabricated demand-row presence
  instance lookup = append-only Key -> uint32_t iid
  instance content = full-rescan current/frozen table buffers

output boundary:
  query cursor scans shared backing storage
  message publication exposes epoch deltas
```

Executable anchors include `Query::Build` in `lib/DataFlow/Build.cpp`,
`QueryImpl::ApplyDemandTransform` in `lib/DataFlow/Demand.cpp`, the demand and
recognition records in `include/drlojekyll/DataFlow/Query.h`,
`BuildSubgraphInstanceOps` and `BuildDRInventory` in `lib/Rel/Rel.cpp`, query
injection in `lib/ControlFlow/Build/Build.cpp`, instance lowering in
`lib/ControlFlow/Build/Procedure.cpp`, and `Runtime/InstanceStore.h`.

### 1.2 Algorithm labels

| Region | Current strategy | Authority problem |
| --- | --- | --- |
| Clause construction | Bottom-up normalization | Correct owner of logical source meaning |
| Demand transform | Top-down forcing plus syntactic rejection | Decides semantic scope by mutating ordinary graph shape |
| DataFlow optimization | Convergent graph rewrite | Optimizes semantic nodes and demand scaffolding as peers |
| Recognized-subgraph registry | Cached duplicate description | Records a second description of the graph before optimization |
| Rel inventory | Bottom-up operational discovery | Re-infers an optional nested meaning from optimized shape |
| Flat versus instance lowering | Mode-selected fork | Two execution meanings survive for one demand relation |
| Instance maintenance | Eager full rescan on touch | Physical strategy is embedded in the exceptional lowering |
| Query cursor | Lazy scan after eager forcing | Cursor lifetime and demand lifetime are separate APIs |
| Recursive relations | Differential local fixpoint | Sound mechanism worth preserving inside a region |

The contradiction is not merely duplicate code. A top-down semantic decision
is encoded as ordinary graph surgery, then a bottom-up operational pass tries to
recover a nested object from the result. Neither representation is authoritative
enough to compose with identity, effects, multiple request owners, or recursive
planning.

### 1.3 Quantity and qualia audit

| Semantic quantity | Current representation | Invalid or ambiguous operation | Target type/owner |
| --- | --- | --- | --- |
| Logical node identity | `QueryView` handle plus pointer-derived identity and separate deterministic order | Equality, ordering, and origin are not one contract | `LogicalNodeId`, then `(RegionId, LocalNodeId)` |
| Field identity | View-relative positions and unsigned ids | Position, semantic field, and physical column can be conflated | `FieldId`, `PortFieldId`, `PhysicalColumnId` |
| Binding key | `vector<unsigned>` positions | Used as adornment, route, storage key, and identity projection | `InstanceKeySchema` of `FieldExpression` |
| Semantic member identity | Equality of surviving physical row fields | Dropping a value can silently merge members | `SemanticMemberKey` in `RowContract` |
| Derivation support | Two signed `int32_t` counters packed into `uint64_t` | Presence arithmetic can be mistaken for demand ownership | `DerivationSupportCount` owned by Rel |
| Demand ownership | Presence of a fabricated message row | Multiple owners and owner-specific result removal are erased | `RequestEdgeId` and `RequestEdgeRelation` |
| Demand support summary | Implied row presence or raw counters | A count is treated as the owner rather than a derived summary | `DemandSupportCount`, derived from exact edges |
| Runtime instance identity | Dense `uint32_t iid` keyed only by generated `Key` | Allocation identity, lexical scope, and liveness are conflated | `ChildInstanceId`; `InstanceId` is its runtime handle |
| Region qualification | Repeated key columns in ordinary rows | Removing a constant key changes equality unless scope is explicit | lexical `InstancePath` |
| Differential change | Boolean add/remove arms and signed raw folds | Sign, presence crossing, derivation count, and ownership are easy to mix | `DeltaSign` plus named support algebras |
| Epoch state | Current/frozen buffers, flags, touched ids | Queue order can become observable without a net-state contract | `EpochId`, old/new relation snapshots in Rel |
| Observable effect | Impure map or message publication mixed with flow | A speculative or retractable membership update can execute an irreversible action | `RootEffectOp` and stored result relation owned by `ProgramRoot` |

The target keeps each quantity named across compiler boundaries. Conversion to
raw positions, integers, and storage rows occurs once in ControlFlow/runtime
lowering.

### 1.4 Findings

#### F1: demand has two semantic authorities

`ApplyDemandTransform` decides where demand flows, but `BuildSubgraphInstanceOps`
decides whether that flow is really a keyed database. An annotation synchronizes
the two descriptions without making either canonical.

#### F2: physical row equality stands in for semantic identity

Optimizer liveness determines which values remain, while runtime hashing treats
the remaining physical tuple as the member. A projection can therefore change
multiplicity without an explicit logical set boundary.

#### F3: demand has two lifecycle models

The fabricated demand table owns membership, while `InstanceStore` owns an
append-only key-to-iid map and current/frozen result buffers. Key existence is
not liveness, and liveness is not exact caller ownership.

#### F4: a request owner is absent from the executable contract

Repeated demand for the same key collapses to one demand row. The compiler can
keep a callee alive, but it cannot attach an existing result set to a new caller
or retract only a departing caller's results.

#### F5: effects do not compose with speculative regional evaluation

An effect occurrence is not a differential relation member. Cloning, replaying,
netting, or retracting an impure operation requires a transaction contract the
regional model does not have.

#### F6: semantic architecture and physical research were coupled

Recursive region calls, cost-ranked extraction, redundant arrangements,
columnar layouts, and LSM maintenance were specified before exact acyclic
request ownership had a terminating model. Those subjects obscured the minimal
semantic replacement.

### 1.5 Architectural pseudocode diff

```diff
 Query::Build(parsed):
     logical = Normalize(parsed)
-    if demand_flag:
-        RewriteToFabricatedMessagesAndGuardJoins(logical)
     Optimize(logical)
-    RecordRecognizedDemandShapes(logical)
-    BuildStorageEquivalenceSets(logical)
-    return logical
+    InferRowContracts(logical)
+    planning = BuildPlanningRegionalProgram(logical)
+    ExtractAcyclicPureRegions(planning)
+    SolveOpenPortsBottomUp(planning)
+    frozen = FreezeAndValidate(planning)
+    return frozen

 Rel::Build(program):
-    flow = DiscoverGlobalOperations(program.query_views)
-    if demand_instance_flag:
-        RecoverDRInstances(program.annotations, flow)
-    return LowerFlatAndExceptionalPaths(flow)
+    rel = LowerProgramRoot(program.root)
+    for region in program.regions.postorder():
+        LowerFrozenRegion(region, rel)
+    DeriveExactRequestEdgeMaintenance(rel)
+    ValidateLifecycleClosure(rel)
+    return rel

 QueryEntry(bound_args):
-    InjectDemandMessageADD(bound_args)
-    return SharedTableCursor(bound_args)
+    lease = AcquireRootRequestLease(bound_args)
+    EvaluateEpochToNetFixpoint()
+    return RegionalCursor(move(lease), bound_args)

 RegionCall(requester_member, key):
-    EnsureDemandRowPresent(key)
-    MaybeRebuildInstanceByFullRescan(key)
+    edge = RequestEdgeId(requester_member, call_site, ChildInstanceId(key))
+    Maintain(edge in RequestEdgeRelation)
+    Maintain(RoutedResult = RequestEdgeRelation JOIN ChildResultRelation)
```

### 1.6 Composability result

| Check | Result |
| --- | --- |
| Authority | Frozen Regional DataFlow is the only semantic region and port owner |
| Type preservation | Member, request, instance, support, and delta identities remain distinct |
| Algebra | Derivation support, demand ownership, and delta sign are never added or compared interchangeably |
| Composition | No annotation recognizer, alternate lowering, or generated parser declaration remains |
| Algorithm fit | Top-down acyclic planning feeds bottom-up Rel lowering through a frozen contract |
| Termination | Finite region depth plus local relation fixpoints; no recursive request graph |
| Effects | Pure regions never schedule observable actions; `ProgramRoot` remains the sole effect authority |
| Test impact | Behavioral witnesses remain; tests pinning flags, recognizers, and exceptional operations are replaced |

## 2. Scope and exclusions

### 2.1 In scope

- A canonical Regional DataFlow program for every compilation.
- Acyclic observation roots and parent-to-child region calls.
- Local cyclic relational graphs and instance-qualified local recursion.
- Explicit member identity and explicit distinct projection.
- Open planning ports and frozen lowering ports as different types.
- Exact request-edge ownership and caller-qualified results.
- Move-only root query leases and permanent published-output roots.
- Differential input, request, and result membership across epochs.
- One Regional-to-Rel-to-ControlFlow production path.
- Deterministic rendering, validation, and semantic property testing.

### 2.2 Excluded from this architecture

- Recursive or key-changing region calls.
- Recursive port-schema SCC solving.
- Event ports between regions.
- Impure functors inside regions.
- Speculative duplicate regions or arrangements.
- Cost scores, calibration, concurrency selection, or layout selection.
- Columnar tries, LSM compaction, WCOJ, and other physical storage research.
- Declaration-level `@first`, `@only`, selection policy, or bounded-consumption
  semantics; [BoundedObservation.md](BoundedObservation.md) owns those decisions.

An excluded feature is not implemented through a fallback path. If the source
program contains an effect, it remains in the explicit `ProgramRoot`. If no
child extraction is legal, the program still lowers through the same frozen
regional representation with fewer region templates.

## 3. Core representations

Planning and frozen IRs are different types so a partially negotiated port
cannot reach Rel.

```text
PlanningRegionalProgram {
    root: OpenProgramRoot
    regions: RegionId -> OpenRegionTemplate
    logical_origins: (RegionId, LocalNodeId) -> LogicalNodeId
}

OpenRegionTemplate {
    id: RegionId
    owner: ProgramRoot | ParentRegion(RegionId)
    parameters: ordered RegionParameter list
    request_port: OpenRequestPort
    input_ports: ordered OpenInputDeltaPort list
    result_ports: ordered OpenResultDeltaPort list
    body: PureRegionalGraph
    children: ordered OpenChildCall list
    row_contracts: EdgeId -> RowContract
    requirements: EdgeId -> UseRequirements
}

FrozenRegionalProgram {
    root: FrozenProgramRoot
    roots: ordered RegionId list
    regions: RegionId -> FrozenRegionTemplate
    logical_origins: (RegionId, LocalNodeId) -> LogicalNodeId
}

FrozenRegionTemplate {
    id: RegionId
    owner: ProgramRoot | ParentRegion(RegionId)
    parameters: ordered RegionParameter list
    request_port: FrozenRequestPort
    input_ports: ordered FrozenInputDeltaPort list
    result_ports: ordered FrozenResultDeltaPort list
    body: FrozenPureRegionalGraph
    children: ordered FrozenChildCall list
    row_contracts: EdgeId -> RowContract
    requirements: EdgeId -> UseRequirements
}
```

`FreezeAndValidate` consumes a `PlanningRegionalProgram` and produces a
`FrozenRegionalProgram`. There is no mutator from a frozen port back to an open
port. Rel accepts only `FrozenRegionalProgram`.

### 3.1 ProgramRoot

`ProgramRoot` is not a region template. It owns:

- sealed declared query and message ABIs;
- root request leases and permanent observations;
- external input delta ingestion;
- effectful or ordered operations that are ineligible for a pure region;
- publication of committed result deltas.

Every program uses this root. Keeping an effect in `ProgramRoot` is a typed
placement rule, not an alternate execution mode.

### 3.2 Region structure

There are exactly two graph structures:

```text
region ownership/call forest       acyclic; a call targets a direct child
local relational graph             may contain relation SCCs
```

Every `FrozenChildCall` targets a region whose `owner` is the caller region.
Cross-tree, ancestor, sibling, and self calls are invalid. This makes lexical
instance ownership total and makes request propagation terminate by finite
region depth.

One logical node may be cloned into multiple observation roots when sealed
external outputs require independent scopes. Each clone retains its
`LogicalNodeId` origin. Speculative cost-driven cloning is outside this design.

## 4. Identity and row contracts

### 4.1 Named identities

At minimum, introduce distinct types for:

```text
FieldId
FieldExpression
LogicalNodeId
RegionId
LocalNodeId
EdgeId
PortId
CallSiteId
RootLeaseId
PermanentRootId
ProgramRootInstanceId
RegionOwnerInstanceId
InstanceId
ChildInstanceId
InstanceKey
InstancePath
SemanticMemberKey
RegionalRequesterId
RequestOwnerId
RequestEdgeId
DerivationSupportCount
DemandSupportCount
DeltaSign
EpochId
```

These types must not be aliases that allow free arithmetic or comparison across
domains. Dense runtime indices are introduced only by ControlFlow lowering.

### 4.2 Row and use contracts

```text
RowContract {
    visible_fields: ordered FieldId list
    candidate_member_keys: antichain of FieldExpression sets
    derivation_support: SupportAlgebra
}

UseRequirements {
    values: FieldExpression set
    distinctions: DistinctionRequirement set
    presence: PresenceRequirement set
    routing: RoutingRequirement set
}
```

Effects are absent because region bodies are pure. Effect requirements belong
to `ProgramRoot` placement validation.

The conservative initial member key for a declared relation is all declared
fields. A proof may replace it with a smaller candidate key. Failure to prove
erasure safe retains the field or sequesters it into `InstancePath`.

### 4.3 Projection is explicit

There are two different logical operators:

```text
MemberProjection:
    may hide payload fields
    preserves SemanticMemberKey

DistinctProjection:
    creates set members keyed by the visible output tuple
    intentionally merges equal projected values
```

Normalization decides which operator source syntax means. Optimizer liveness
may never turn one into the other. Aggregate input identity is the member key of
the explicit input relation, not whichever physical payload columns survive.

### 4.4 Transfer rules

| Operator | Member identity rule |
| --- | --- |
| Declared relation | Declared key, otherwise all declared fields |
| Filter/compare | Preserve input key; predicate fields are value/presence requirements |
| Member projection | Preserve hidden input key |
| Distinct projection | Visible output tuple becomes the member key |
| Merge/union | Output member key; arm collisions contribute derivation support |
| Join | Union contributor keys, minimized by proven equalities and dependencies |
| Negation | Positive member key; negative key is a presence requirement |
| Pure one-to-one functor | Preserve input key unless injective output proves an equivalent key |
| Pure one-to-many functor | Input key plus emitted output member key |
| Aggregate | Group key for output; explicit input member key for membership |
| Key/value state | Explicit key, value/member identity, and update algebra |

## 5. Lexical instances and exact requests

### 5.1 Lexical instance identity

```text
ChildInstanceId {
    owner_instance: RegionOwnerInstanceId
    region: RegionId
    key: InstanceKey
}

RegionOwnerInstanceId =
    ProgramRootInstance(ProgramRootInstanceId)
    | ParentInstance(InstanceId)

InstancePath =
    ProgramRootInstance
    | InstancePath + ChildInstanceId
```

The path follows the ownership forest, never the dynamic request stack. Since a
call targets only a direct child, the owning instance is always known.

### 5.2 Request ownership

```text
RegionalRequesterId {
    caller_instance: InstanceId
    member: SemanticMemberKey
}

RequestOwnerId =
    RootLease(RootLeaseId)
    | PermanentRoot(PermanentRootId)
    | RegionalMember(RegionalRequesterId)

RequestEdgeId {
    owner: RequestOwnerId
    call_site: CallSiteId
    child: ChildInstanceId
}
```

`RequestEdgeRelation` is the semantic authority for demand. Multiple requester
members may demand the same child. Multiple derivations of the same requester
member affect derivation support for one request edge; they do not fabricate
new anonymous owners.

```text
ActiveInstanceRelation =
    DistinctProjection(RequestEdgeRelation.child)

DemandSupportCount(child) =
    count live RequestEdgeId values for child
```

`DemandSupportCount` is a derived implementation aid. A count can answer whether
an instance is active, but it cannot replace the edge relation because it cannot
route or retract caller-specific results.

### 5.3 Caller-qualified results

```text
ChildResultRelation {
    child: ChildInstanceId
    member: SemanticMemberKey
    payload: visible result fields
}

RoutedResultRelation =
    RequestEdgeRelation
        JOIN ChildResultRelation ON child
```

The routed result's identity includes `RequestEdgeId` and the child result member
key until an explicit parent projection changes it. Therefore:

- adding a request edge attaches all existing child results to that owner;
- removing one edge retracts only that owner's routed results;
- child result changes fan out to every live edge;
- child state is retired only after its last edge disappears and all routed
  removals are visible.

This relation, rather than a special subscribe/catch-up branch, gives births,
late subscribers, detachments, and result fanout one differential meaning.

## 6. Root observations and leases

Root observations are a closed sum:

```text
RootObservation =
    LeasedQueryRoot(FrozenDeclaredQueryPort)
    | PermanentOutputRoot(FrozenDeclaredOutputPort)
```

A permanent output owns one `PermanentRootId` for the program lifetime. A bound
query call creates a distinct `RootLeaseId` and corresponding request edge.

Generated cursors own a move-only lease:

```text
RegionalCursor<Row> {
    database: BorrowedDatabase
    lease: RootRequestLease
    position: CursorPosition

    move: transfer lease ownership
    copy: forbidden
    close/destruct: enqueue removal of the exact root request edge
}
```

The database must outlive every cursor it created. A database entry point may
not start another query or mutating epoch while a cursor over mutable backing
state is open. The first implementation enforces this existing drain-before-
next-entry contract. Closing the cursor records its exact lease removal; the
next entry point nets all pending lease removals before doing other regional
work. Idle retired leases may leave physical cache state allocated, but they are
not observable and are processed before the database can be observed again.

There is no nullable release callback and no separate user-invoked demand
retract function. Lease destruction owns the exact pending removal.

## 7. Epoch and lifecycle semantics

The semantic epoch transition is defined by old and new relations, not queue
visit order:

```text
EvaluateEpoch(external_input_deltas, lease_deltas):
    old_observable = SnapshotCommittedOutputs()

    root_input_deltas = ProgramRoot.ApplyExternalEpoch(external_input_deltas)
    NetRegionalInputs(root_input_deltas)
    NetRootLeaseEdges()

    repeat until all regional and local frontiers are empty:
        maintain RequestEdgeRelation
        maintain ActiveInstanceRelation
        maintain region-local relations to local fixpoint
        maintain ChildResultRelation
        maintain RoutedResultRelation

    new_observable = CurrentCommittedOutputs()
    publication_delta = Difference(old_observable, new_observable)
    ProgramRoot.PublishCommitted(publication_delta)
    RetireInactiveStateAfterRemovals()
    SealEpoch()
```

Rel may schedule these operations incrementally, but validation proves that the
schedule computes this transition. Same-epoch request and input permutations
must produce the same final relations and publication delta.

### 7.1 Termination

Termination relies on two independent measures:

1. Region requests move only from a region to a proper child in a finite
   ownership forest. There is no regional request cycle.
2. Recursive relation edges stay within one region and use the existing Rel
   differential fixpoint, qualified by `(RegionId, InstanceId)`.

```text
for each affected (RegionId, InstanceId):
    while local recursive frontier is non-empty:
        execute next Rel induction round
```

No `InstancePath` growth participates in recursive relation evaluation.

### 7.2 Effects

Pure regions expose only request, input-delta, and result-delta ports. An
observable message or impure functor remains in `ProgramRoot`.

`ProgramRoot` executes an authoritative effect according to the root schedule
and stores any relational result it produces. A region may consume only the
stored result delta through an input port. Region extraction cannot move, clone,
replay, or retract the effect itself. Publications similarly consume committed
regional deltas in `ProgramRoot` after regional quiescence. The regional
architecture therefore composes with the root's effect contract without
inventing a second effect model.

## 8. Planning and freezing

The initial planner is deterministic and legality-driven. Profitability is not
a semantic input.

```text
BuildPlanningRegionalProgram(logical):
    contracts = InferConservativeRowContracts(logical)
    planning = BuildProgramRootAndObservationRoots(logical, contracts)

    for region in stable ownership order:
        OptimizeLocalGraphToFixpoint(region)
        SolveBackwardRequirements(region)

        while candidate = FirstStableAdmissibleChild(region):
            child = ExtractPureChild(region, candidate)
            OptimizeLocalGraphToFixpoint(child)
            SolveBackwardRequirements(child)
            ReplaceSliceWithOpenChildCall(region, child)
            ReoptimizeParent(region)

    SolvePortsInOwnershipPostorder(planning)
    ValidatePlanningProgram(planning)
    return FreezeAndValidate(planning)
```

Because calls are acyclic, child ports are solved in ownership postorder. No
recursive port-schema fixpoint exists. Extraction never clones a non-root
logical node and never reopens an opaque child, so each successful iteration
strictly reduces the parent's unassigned local slice and the loop terminates.

### 8.1 Admissibility

A child slice is admissible only when:

```text
all operators in the slice are pure
the callee is a newly owned direct child
all member distinctions have a realizable local or InstancePath-qualified key
every request and result removal has an exact identity
every crossing input has a differential maintenance strategy
sealed declared ABIs remain in ProgramRoot and unchanged
no region parameter escapes except through an explicit port mapping
local stratification and recursion remain valid
```

Failure means the slice remains in its current regional scope. It does not mean
the compiler chooses the obsolete flat demand transform; the entire program
still lowers from one `FrozenRegionalProgram`.

### 8.2 Port minimization

An open internal port field may be removed only if no requirement needs its:

```text
value
member distinction
presence test
route, group, index, or retraction lookup
```

If the field is a region parameter, `InstancePath` must still qualify every
affected local identity. Declared external port fields are sealed in
`ProgramRoot` and are never minimized.

## 9. Rel and ControlFlow lowering

```text
RelProgram {
    root: RelProgramRoot
    regions: ordered RelRegion list
    request_edges: RelRequestEdgeRelation
    routed_results: ordered RelRoutedResult list
}

RelRegion {
    id: RegionId
    owner: ProgramRoot | ParentRegion(RegionId)
    ports: FrozenRegionPorts
    instance_layout: InstancePathLayout
    scope: RelScope
    lifecycle: RelRegionLifecycle
}

RelRegionLifecycle {
    request_edge_add
    request_edge_remove
    input_delta
    local_fixpoint
    child_result_add
    child_result_remove
    routed_result_add
    routed_result_remove
    retire_inactive
    seal_epoch
}
```

Rel validates that every frozen port has one lowering, every request edge has a
balanced add/remove path, every child result participates in routed fanout, and
all local recursive queues belong to the instance-qualified fixpoint.

ControlFlow uses typed ids to map Rel objects to dense runtime slots. It does not
look up `QueryView *`, guard annotations, parser declarations, or recognized
shapes. Runtime may implement a region with per-instance tables, a shared keyed
index, or recomputation, provided the chosen implementation satisfies the same
capability and lifecycle contract. This proposal does not select among them.

## 10. Language boundary

Regional demand is compiler-owned and has no source annotation.

- Remove query-body `@first message(...)` forcing and its generated-message
  behavior when the regional path lands.
- Do not use declaration-level `@first` or `@only` to define demand, member
  identity, or storage.
- Declaration-level bounded consumption is owned exclusively by
  [BoundedObservation.md](BoundedObservation.md). This document supersedes that
  proposal only where it previously preserved query-body forcing. Bounded
  consumption must decide its result types without changing regional request
  semantics.
- A client operation that performs a write is an explicit command/message entry,
  not an observational query with hidden forcing.

## 11. Required validators

Compilation fails on:

```text
unrealized semantic member identity
accidental collapse outside DistinctProjection
symbolic parameter escape
open port reaching FrozenRegionalProgram
parent/child frozen-port disagreement
mutation of a sealed external ABI
region call not targeting a direct child
cycle in the region ownership/call graph
effectful operator inside a region
sequestered key absent from InstancePath qualification
ambiguous RequestOwnerId or RequestEdgeId
request edge without caller-qualified result maintenance
result removal without exact routed-result identity
inactive state cleared before routed removals
non-converged local relational fixpoint
FrozenRegionalProgram/Rel operation census mismatch
Rel/ControlFlow lifecycle census mismatch
```

No validator warns and continues into another execution model.

## 12. Testing strategy

### 12.1 Unit and structural tests

- Row-contract and requirement transfer for every logical operator.
- `MemberProjection` versus `DistinctProjection` collision behavior.
- Candidate-key reduction under equality, constants, and region parameters.
- Planning-to-frozen type transition and immutable frozen ports.
- Ownership postorder port solving.
- Rejection of cross-tree, ancestor, self, and cyclic region calls.
- Exact request-edge derivation from distinct requester members.
- Move-only root lease construction, transfer, close, pending-removal drain,
  and destruction.
- Rel/ControlFlow lifecycle and operation census validators.

### 12.2 Semantic property tests

A small reference relational interpreter compares normalized logical meaning
with regional execution over generated databases and delta batches. Tests
permute independent request and input updates within an epoch and compare:

```text
final relation membership
net published deltas
live RequestEdgeId set
live ChildInstanceId set
caller-qualified routed results
```

Internal queue order, dense runtime ids, and physical enumeration order are not
semantic outputs.

### 12.3 Directed witnesses

| Area | Required witness |
| --- | --- |
| Identity | Equal visible payload from distinct semantic members |
| Projection | Hidden member preservation versus explicit distinct collapse |
| Aggregate | Equal contributions from distinct members and exact retraction |
| Multiple owners | Two requester members for one child; retract either one first |
| Multiple requester members | Same caller instance and call site request one key from two members |
| Late subscriber | Second edge attaches already-maintained child results |
| Detachment | Removing one edge retracts only its routed results |
| Request/data order | Request-before-data, data-before-request, and same-epoch flaps |
| Nested regions | Two acyclic depths with lexical key aliases |
| Local recursion | Linear, nonlinear, and mutual relation recursion inside one instance |
| Permanent root | Published observation stays live without a cursor lease |
| Effects | Effect stays in ProgramRoot; region consumes only its stored result delta |
| Rejection | Key-changing recursive region request fails compilation |

### 12.4 Tests to replace or delete

Delete tests whose only purpose is to preserve:

- `-demand` versus `-demand-instance` output equivalence;
- demand-mode and demand-retract flags;
- fabricated parser messages or query forcing registries;
- guard-annotation survival through CSE;
- `RecognizedSubgraph` or `DRInstance` census and ordering;
- full-rescan `InstanceStore` internals as the semantic keyed-instance model;
- a separate user-invoked query retract entry point.

Keep their datasets when they express useful behavior. Rewrite those witnesses
against request edges, lease lifetime, caller-qualified results, local
recursion, and net epoch deltas.

## 13. Direct implementation sequence

Each cut leaves one production authority.

### Stage A: make identity explicit

1. Add named field, member-key, support, delta, and requirement types.
2. Normalize payload-preserving and distinct projections into separate nodes.
3. Infer conservative row contracts through the existing logical graph.
4. Make aggregates consume an explicit member key.
5. Replace advisory projection warnings with precise contract validation.

Exit: projection and aggregate collision witnesses have explicit semantics.

### Stage B: make the regional representation canonical

1. Add `PlanningRegionalProgram`, `FrozenRegionalProgram`, and typed ids.
2. Represent the existing whole program as `ProgramRoot` plus observation-root
   region templates without child extraction.
3. Move local optimization, stratification, formatting, and validation through
   the regional representation.
4. Make Rel consume only `FrozenRegionalProgram`.

Exit: every program follows the same Regional-to-Rel path with unchanged
logical results and deterministic dumps.

### Stage C: replace forcing with complete acyclic lifecycle

Land the following as one coherent cutover:

1. Add `RequestEdgeRelation`, `ActiveInstanceRelation`, `ChildResultRelation`,
   and `RoutedResultRelation`.
2. Add move-only root leases and permanent root owners.
3. Implement one-level pure child extraction and lexical `ChildInstanceId`.
4. Implement edge birth, late attachment, detachment, fanout, result removal,
   inactive retirement, and same-epoch netting.
5. Lower the lifecycle through Rel and ControlFlow.
6. Remove `ApplyDemandTransform`, fabricated demand declarations,
   `QueryDemandForcing`, guard annotations, `RecognizedSubgraph`, `DRInstance`,
   special instance lowering, and all demand selector flags.
7. Remove query-body forcing syntax and the separate retract entry point.

Exit: every supported bound query uses an exact regional request lease; source
and executable searches find no obsolete demand authority.

### Stage D: deepen the acyclic forest and qualify local recursion

1. Implement deterministic nested child extraction.
2. Solve ports in ownership postorder and reoptimize parents after child freeze.
3. Qualify local Rel tables, state, and rounds by `(RegionId, InstanceId)`.
4. Support existing relation recursion entirely inside a region.
5. Add the full nested, differential, and local-recursion witness matrix.

Exit: nested acyclic regions and local recursive relations match the semantic
oracle across update permutations.

Physical cost models, redundant arrangements, columnar layouts, and any future
recursive request architecture require separate proposals after this exit.

## 14. Deletions

The completed replacement deletes rather than deprecates:

- `-demand`, `-demand-retract`, and `-demand-instance`;
- parser-object fabrication by DataFlow optimization;
- generated demand-message entry suppression;
- query forcing and separate demand-retract procedures;
- demand guard annotations and graph-shape recognizers;
- `RecognizedSubgraph` and `DRInstance`;
- flat-versus-instance lowering selection;
- special `InstanceStore` semantics recovered from a global graph;
- tests and comments whose only subject is those paths.

The superseded proposal remains documentation only. It is not referenced by
source comments, tests, implementation plans, or runtime flags.

## 15. Acceptance invariants

The architecture is complete only while all of these hold:

1. Every compilation produces one `FrozenRegionalProgram`.
2. Rel consumes only frozen regional contracts and never discovers regions.
3. Region ownership and calls are the same acyclic forest.
4. Recursive relation evaluation stays local to one region instance.
5. `InstancePath` follows lexical ownership, never a dynamic call stack.
6. `RequestEdgeRelation` is the sole semantic demand authority.
7. Every requester member and root lease has an exact `RequestOwnerId`.
8. Every request edge participates in caller-qualified routed results.
9. A second owner receives existing results; one owner's removal affects no
   other owner.
10. Child state is cleared only after the last edge dies and routed removals are
    visible.
11. Member projection and distinct projection are different logical operators.
12. A sequestered key remains in `InstancePath` until a proof removes every
    value, distinction, presence, and routing need.
13. Pure regions contain no observable effect.
14. Same-epoch request/data ordering cannot change final membership or net
    publication.
15. There is no demand mode, alternate lowering, recognizer, or fallback.
16. Every regional, Rel, and ControlFlow artifact is deterministic and
    validator-complete.

The resulting architecture is intentionally smaller than the superseded one.
It establishes the semantic object that later physical optimization can safely
accelerate: a pure, lexically owned database region whose lifetime and results
are derived from exact acyclic request edges.
