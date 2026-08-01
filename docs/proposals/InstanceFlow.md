# InstanceFlow: contextual computation families between Query and Rel

> Status: architecture proposal, 2026-07-31. This document is intentionally
> greenfield: it defines one target pipeline and the deletions required to get
> there. It does not propose a permanent legacy/InstanceFlow mode split.

## 0. Decision

Add a new compiler IR, **InstanceFlow**, after the optimized logical Query IR
and before storage planning and Rel. InstanceFlow represents a deterministic
grove of context-parameterized computation families built backward from
consumer uses. A family may bind columns such as a join key once, remove them
from every residual row inside the family, and expose push, probe, and standing
interest interfaces. Different families may refer to the same Query origin.
That overlap is deliberate and does not duplicate logical derivations.

The target pipeline is:

```text
Parse
  -> Query                         logical truth and boundary intent
  -> InstanceFlow                  contextual topology and use coverage
  -> MaterializationPlan           state identity, replicas, arrangements
  -> Rel                           delta algorithms, effects, rounds, schedule
  -> ControlFlow                   mechanical executable regions
  -> C++/runtime
```

Rel belongs exactly where it was already trying to go: after logical planning
and before ControlFlow, as the sole owner of temporal and differential
execution. InstanceFlow does not replace Rel. It gives Rel a better input than
the combination of a shared Query DAG, already allocated `TABLE *` objects,
and narrowly recognized demand subgraphs.

The ownership rule is:

```text
Query:
  What facts are true? What does a boundary consumer mean?

InstanceFlow:
  Which contextual occurrence satisfies each logical use?
  Which columns are family context rather than residual row payload?
  Where may pure computation be duplicated, shared, or activated lazily?

MaterializationPlan:
  Which logical collections and family occurrences use which state resources?
  Which resources are authoritative and which are read replicas?
  Which projections, orders, columns, and partitions does each arrangement hold?

Rel:
  How do signed changes, visibility changes, interest changes, effects,
  aggregates, and recursive rounds execute in time?
  Which one certified physical algorithm realizes each work scope?

ControlFlow:
  What loops, regions, variables, calls, and branches implement that Rel plan?
```

This is not WCOJ. The first useful specialization is much smaller: bind an
equality key as family context, keep residual columns in sorted grouped
arrangements, and merge/probe those groups with the existing binary and
differential join algorithms.

## 1. Why a new IR is necessary

The current Query graph is a shared logical DAG. A `QueryView` has one identity
even when two consumers want materially different physical uses of it. The
current demand transform works around that mismatch by fabricating Query
relations and guard joins before CSE, then attaching recognition annotations
so later lowering can rediscover a special subgraph. Current storage sharing
similarly puts a physical decision, `EquivalenceSetId`, directly on Query
views.

Neither representation can say all of the following at once:

```text
the same Query origin appears in two consumer-local computations
one occurrence carries K as a row column
another treats K as an instance parameter
the two occurrences may share an authoritative collection
each may have a different read arrangement
only one occurrence owns each logical emission obligation
```

Mutating or cloning Query nodes is the wrong repair. It would make logical CSE
and physical occurrence identity fight each other. Keeping more annotations on
Query would create the same split brain as the demand annotations, only larger.

InstanceFlow is therefore a derived occurrence IR. It refers back to Query but
does not change Query truth. Query optimization finishes first. InstanceFlow
then expands one logical node into zero, one, or many contextual occurrences
according to its uses.

The result is called a **grove**, not literally a forest, because:

- each root grows backward in a tree-like way;
- two trees may contain occurrences of the same Query origin;
- exact `(origin, context)` subfamilies may be shared;
- activation and materialization references can make the physical result a
  DAG even though its consumer-oriented explanation remains tree-like.

## 2. Ground truth: the executable architecture today

The current build is approximately:

```text
function BuildQuery(module):
  graph = BuildClauses(module)
  RemoveUnused(graph)
  TrackDifferential(graph)
  Simplify(graph)
  ConnectInsertsToSelects(graph)
  ApplyDemandTransform(graph)         # fabricates demand relations/guards
  OptimizeAndCSE(graph)
  ConvertConstantsAndProxyInserts(graph)
  LinkViews(graph)
  IdentifyInductions(graph)
  FinalizeDepthsColumnsAndDiff(graph)
  BuildEquivalenceSets(graph)         # physical storage sharing on Query
  Stratify(graph)
  return graph

function BuildProgram(query):
  data_models = BuildDataModel(query.EquivalenceSetIds)
  FillDataModel(query, data_models)    # decides which views need TABLEs
  tables = AllocateTablesAndIndexes(data_models)

  for each stratum:
    rel = BuildDRInventory(query, tables, recognized_demand_subgraphs)
    DeriveRelStrata(rel)
    ValidateAndLinearize(rel)
    LowerRelFlowRoundsSweepsGroupsInstances(rel, control_flow)

  BuildEntryProcedure(query, tables, control_flow)
  return control_flow
```

The Query vocabulary is already the logical language: `SELECT`, `TUPLE`,
`JOIN`, `MERGE`, `MAP`, `COMPARE`, `NEGATE`, `AGGREGATE`, `KVINDEX`, and
`INSERT`. Rel already contains much of the right temporal vocabulary:
membership predicates, signed effects, access spines, claim/retire/rederive
operations, group state, fixpoint rounds, keyed instance operations, and an
effect-derived linearization.

The remaining architectural inversions are important:

1. `BuildEquivalenceSets` says which logical views can share backing storage,
   so Query owns a physical decision.
2. ControlFlow tables are allocated before Rel; `DRTable`, `DRBranch`,
   `DRJoin`, `PlanNode`, and `DRInstance` carry `TABLE *` and reconstruct Query
   paths around those tables.
3. The demand pass creates a second, physical-looking Query graph, while Rel
   recognizes only selected shapes inside it.
4. ControlFlow still has construction knowledge that should be mechanical
   lowering from Rel.
5. `Query::Build` wraps finalization in `catch (...)`, cleans up some dead
   views, and continues. A contextual IR cannot be allowed to consume a graph
   whose failed invariant was hidden this way.

The last point is a high-severity muted-exception finding. The protected block
mutates topology, identities, differential facts, and column numbering. The
catch does not restore a documented complete state and does not know which call
failed. The target repair is:

```diff
- try:
-   ConvertConstantsAndProxyInserts()
-   LinkViews()
-   IdentifyInductions()
-   FinalizeDepthsColumnsAndDiff()
- catch (...):
-   assert(false)
-   remove_some_dead_views_and_continue()
+ ConvertConstantsAndProxyInserts()
+ LinkViews()
+ IdentifyInductions()
+ FinalizeDepthsColumnsAndDiff()
+ # invariant failures reach the compilation boundary with their cause intact
```

### 2.1 Reproducible source anchors

| Fact | Current source |
|---|---|
| Query construction, demand placement, finalization, and catch | `lib/DataFlow/Build.cpp:2518-2650` |
| Storage equivalence is built on Query | `lib/DataFlow/Build.cpp:2297` |
| Demand mutates the Query graph | `lib/DataFlow/Demand.cpp:385` |
| Rel is built inside stratum construction after data models exist | `lib/ControlFlow/Build/Stratum.cpp:2117-2149` |
| Rel inventories `impl->tables` | `lib/Rel/Rel.cpp:1803-1825` |
| `DRTable`, `DRInstance`, and `DRFlowGraph` | `lib/Rel/Rel.h:412`, `:870`, `:896` |
| Join pivots are already sorted/uniqued before their loop | `lib/ControlFlow/Build/Join.cpp:575-588` |
| Generated join body | `lib/CodeGen/CPlusPlus/Database.cpp:3084` |
| Authoritative rows are AoS plus an open-addressing full-row hash set | `include/drlojekyll/Runtime/Table.h:54` |
| Exact-key indexes are hash slots plus per-row linked chains | `include/drlojekyll/Runtime/Table.h:749` |
| Current keyed runtime allocates per-instance current/frozen state | `include/drlojekyll/Runtime/InstanceStore.h:63` |

These are navigation anchors for the proposal's commit, not permanent line
contracts. The executable types and call order are the authority when lines
move.

## 3. Where the architecture was going before InstanceFlow

The Rel/DeltaRel work has a coherent prior trajectory. It was not merely adding
another dump. It was moving the compiler from two emission authorities toward
one:

```text
Query DAG + tables
  -> derive complete Rel inventory
  -> derive effects and dependencies
  -> linearize all eager, differential, group, and recursive work in Rel
  -> mechanically lower Rel to ControlFlow
  -> delete ControlFlow's independent Query walk and old discovery
```

In complete pseudocode, the intended endpoint was:

```text
function BuildProgram_PreInstanceFlowTarget(query):
  storage = ChooseSharedDataModels(query)
  tables = AllocateTables(storage)

  rel = RelProgram()
  rel.tables = Inventory(tables, query.members)
  rel.branches = DeriveBranches(query, tables)
  rel.joins = DeriveJoins(query, tables)
  rel.ops += DeriveEagerOps(query, tables)
  rel.ops += DeriveDifferentialOps(query, tables)
  rel.ops += DeriveAggregateOps(query, tables)
  rel.ops += DeriveRecognizedKeyedInstances(query, tables)
  rel.rounds = DeriveRecursiveRounds(query, tables)
  rel.effects = DeriveEffects(rel)
  rel.order = Linearize(rel.effects, rel.rounds)
  ValidateRelIsComplete(rel)

  return LowerRelMechanically(rel)
```

That direction remains correct. InstanceFlow changes the inputs and separates
two decisions that the prior target still conflated:

- deriving consumer-local computation topology from a shared logical DAG;
- selecting temporal/differential execution for the resulting topology.

Without InstanceFlow, Rel would eventually become the sole physical authority
but would still have to rediscover occurrence topology from Query and still
receive storage decisions made too early. With InstanceFlow, the Rel cutover
can finish on a cleaner contract.

### 3.1 Diff from the executable system to the prior target

```diff
- ControlFlow eagerly walks Query and emits some regions directly
- Rel mirrors/replays portions and cross-checks the two authorities
+ Rel inventories every executable operation
+ ControlFlow lowers only Rel operations and regions
+ old walk/discovery and cross-authority equivalence belts are deleted

- Rel is partly a shadow graph around already allocated TABLE * objects
+ Rel becomes the complete temporal execution graph

- scheduling facts are rediscovered in several builders
+ effects, hazards, SCC rounds, and one pinned order are authored in Rel
```

### 3.2 Diff from the prior target to the InstanceFlow target

```diff
- Query owns EquivalenceSetId storage sharing
- Rel rediscovers branches, joins, and keyed subgraphs from Query
- demand is a Query-graph rewrite plus later recognition
- one QueryView implies one default physical occurrence
+ Query owns only logical truth, semantic SCCs, and boundary intent
+ InstanceFlow maps every logical use to a contextual occurrence
+ MaterializationPlan solely owns state/replica/arrangement identity
+ Rel consumes InstanceFlow + MaterializationPlan; it never rediscovers them
+ demand is standing activation between family templates, not fabricated truth
+ one Query origin may have many occurrence-specific physical forms
```

The proposed architecture is therefore an insertion into the prior direction,
not a reversal of it.

## 4. Typed semantic quantities

These quantities must not collapse into raw integers, booleans, column lists,
or generic IDs:

| Type | Meaning | Must not be confused with |
|---|---|---|
| `QueryOriginId` | Stable logical Query node | family occurrence |
| `OriginUseId` | One producer-to-consumer edge or boundary obligation | Query node |
| `LogicalCollectionId` | One set-valued logical truth collection | storage resource |
| `DerivationSiteId` | One logical rule/head contribution | consumer use |
| `FamilyId` | One parameterized computation template | live key instance |
| `ActivationSccId` | One atomic component of the family activation graph | Query SCC |
| `FamilyInstanceKey` | Runtime binding of a family context | residual row |
| `FamilyNodeId` | One Query-origin occurrence inside a family | Query origin |
| `ContextSlotId` | One symbolic parameter coordinate | Query column id |
| `ContextSignature` | Canonical column-equivalence-to-slot mapping | runtime key value |
| `ResidualSchema` | Columns physically carried per row | logical full schema |
| `SourceRowToken` | Stable identity of a source row/fact | payload equality |
| `SettledSupportCount` | Nonnegative logical derivation multiplicity at a visibility boundary | interest or replicas |
| `WorkingDerivationCounter` | Rel-local signed overdelete/rederive state inside an epoch | settled support |
| `VisibilityDelta` | Zero-crossing change of set membership | support delta |
| `InterestCount` | Number of live standing consumers of a key | derivations |
| `ReplicaCount` | Number of physical copies | any semantic count |
| `CoverageDomain` | Context-key subset owned by an occurrence | table partition |
| `EmissionAuthorityId` | Sole writer for one derivation coverage cell | replica owner |
| `StateResourceId` | Mutable authoritative state allocation | logical collection |
| `ArrangementId` | Query-oriented physical access form | authoritative truth |
| `SnapshotGeneration` | Stable rebuild/read frontier | differential epoch |
| `RowTouches` | Physical work estimate/counter | cardinality |

Use domain structs/enums for these contracts. In particular, do not encode
context as `vector<unsigned>`, absence as a null collaborator, or plan choice
as several independent booleans.

```text
InputChangeMode =
  StableSnapshot
  | InsertOnly
  | Differential
```

Change mode is per family port. A join may have an insert-only left input and a
differential right input; collapsing that to one family-level boolean would
either reject valid plans or select an unsound lifecycle.

## 5. The algebra of a keyed family

For a binary join:

```text
left(K, A)
right(K, B)
out(K, A, B) :- left(K, A), right(K, B)
```

the family interpretation is:

```text
Relation<K, A>  ~=  Family<K, Relation<A>>

family JoinAtK[K = k]:
  left_residual(A)  = left(k, A)
  right_residual(B) = right(k, B)
  out_residual(A,B) = left_residual(A) x right_residual(B)
```

Inside `JoinAtK`, `K` is a symbolic constant. It is not copied through every
row, compared again at the join, or stored in every residual segment. Logical
rows remain `(k, A)`, `(k, B)`, and `(k, A, B)`; only their physical coordinate
system changed.

This transformation needs explicit proof objects:

```text
ContextTransfer =
  Preserve(slot, input_column, output_column)
  | Bind(slot, logical_column)
  | Derive(slot, pure_expression, input_slots)
  | Rekey(old_slots, new_slots, certified_function)
  | Forget(slot, fold_supports_by_remaining_identity)
  | Block(ContextBlocker)

ContextBlocker =
  EffectfulOrigin
  | MissingColumnLineage
  | NonInvertibleKeyMap
  | ContextForgetNeedsSupportFold
  | IncompleteNegation
  | CrossKeyAggregate
  | PartialRecursiveScc

GrowthBlocker =
  Context(ContextBlocker)
  | IncompatibleActivationCycle
  | UnprovedCoveragePartition
  | NonPositiveEstimatedBenefit
```

`Bind` is injective when the family instance key remains part of logical
identity. `Forget` is potentially many-to-one and must consolidate support
counts. It is never a free projection.

### 5.1 Canonical context signatures

The left and right spellings of `K` become one slot only because Query column
lineage and the join equality prove them equal:

```text
ContextSignature:
  slots:
    k0:
      domain_type: JoinKey
      logical_columns: {left.K, right.K, out.K}
      proof: EqualityClassProof(join_origin, pivot_pairs)
```

Canonicalization sorts slots by their smallest stable logical-column identity,
then records every equivalent column and its proof. Two candidates can share a
family only when their canonical signatures, change modes, SCC ownership, and
effect boundaries match. Equal runtime types or equal column names are not
sufficient.

This is the precise sense in which left `K` and right `K` are both
“bound-like”: they are references to one family coordinate, not two residual
values that happen to compare equal.

### 5.2 Join delta semantics inside the family

Binding `K` does not change incremental join algebra:

```text
on batch (delta_left, delta_right) for key k:
  emit delta_left x right_before
  emit left_after x delta_right
```

or an equivalent ordered two-arm formulation already certified by Rel. What
changes is access:

```diff
- carry K in every left/right row
- hash K to find an index chain
- re-test or reconstruct K at downstream sites
+ dispatch the batch to family instance K
+ merge/probe sorted residual groups for that K
+ reconstruct K only at a boundary whose logical schema includes it
```

If projection causes multiple source pairs to produce the same output row,
Rel applies signed derivation support to the output authority. Pair identity
may use `(LeftSourceRowToken, RightSourceRowToken)` internally; payload equality
alone is not a multiplicity proof.

### 5.3 A family does not imply one heap object per key

`Family<K, Relation<A>>` is the semantic and planning view. MaterializationPlan
may choose a grouped, ephemeral, or keyed-state substrate, and Rel may operate
that substrate in several ways:

```text
VirtualGrouped:
  one sorted arrangement containing many K segments

BatchedEphemeral:
  sort a batch by K and process each contiguous group

StandingStore:
  one lifecycle record per live interested K

PartitionedFixpoint:
  one round-state partition per active recursive K
```

The ordinary join-pivot optimization should normally use `VirtualGrouped` or
`BatchedEphemeral`, not allocate an `InstanceStore` object for every distinct
database key. Standing per-key state is for lifecycle-bearing demand,
aggregates, and recursion. This distinction is how the design gains the
“just sort some things” simplification without turning every join into a
subscription system.

## 6. InstanceFlow object model

```text
InstanceFlowProgram:
  origins: QueryOriginCatalog
  uses: OriginUseCatalog
  families: ordered map FamilyId -> FamilyTemplate
  activations: ordered list ActivationEdge
  coverage: ordered list UseCoverage
  authorities: ordered list EmissionAuthority
  requirements: ordered list ArrangementRequirement
  barriers: ordered list ContextBlock
  provenance: TraceCatalog

FamilyTemplate:
  id: FamilyId
  root_use: OriginUseId
  context: ContextSignature
  key_schema: FamilyKeySchema
  input_changes: ordered map FamilyInputPortId -> InputChangeMode
  output_changes: ordered map FamilyOutputPortId -> InputChangeMode
  activation: ActivationRequirement
  nodes: ordered map FamilyNodeId -> FamilyNode
  edges: ordered list FamilyEdge
  ports: FamilyPorts
  scc_ownership: Acyclic | WholeQueryScc(QuerySccId)

ActivationRequirement =
  CompleteDataDriven
  | StandingInterest(InterestPortId)
  | BoundaryProbe(QueryReadId)

FamilyNode:
  id: FamilyNodeId
  origin: QueryOriginId
  occurrence: OccurrenceRole
  residual_schema: ResidualSchema
  transfers: ordered list ContextTransfer
  output_collection: LogicalCollectionId

FamilyPorts:
  pushes: ordered list DeltaNotificationPort
  probes: ordered list ProbePort
  interests: ordered list InterestPort
  publications: ordered list PublicationPort

UseCoverage:
  use: OriginUseId
  domain: CoverageDomain
  occurrence: FamilyNodeId | FamilyEdgeId
  authority: EmissionAuthorityId

EmissionAuthority:
  id: EmissionAuthorityId
  derivation_site: DerivationSiteId
  domain: CoverageDomain
  writer: FamilyNodeId
```

Every sum above is a real tagged variant. A family does not have nullable
`probe`, `interest`, `scc`, or `arrangement` collaborators. Empty port lists
mean no port of that kind. Strategy-specific state lives in its strategy
payload.

### 6.1 Relation-service interpretation

Any state-backed family port can be understood as a relation service:

```text
push(DeltaNotification):
  accept an input support/visibility change

probe(ProbeRequest) -> SnapshotRows:
  answer a transient exact-key/range lookup against a named generation

interest(InterestDelta):
  add or remove a standing subscription to a context key

publish(VisibilityDelta):
  notify downstream consumers when logical membership changes
```

This makes explicit a property already latent in the dataflow. A join receives
a pushed pivot from one side and queries the other side for matching rows. The
new IR can push that query context farther backward through pure operators and
can turn it into a standing interest when maintaining a keyed region is cheaper
than repeated transient probes.

Push, probe, and interest are not interchangeable:

- push reports that truth changed;
- probe observes truth without changing lifecycle;
- interest changes which contextual computation must stay live.

## 7. Building the grove top down

“Top down” means from consumers toward producers. The algorithm starts with
obligations close to outputs and queries, then walks Query predecessor edges.
It does not destructively remove a Query node when one tree admits it.

### 7.1 Inputs

```text
BuildInstanceFlow(
  optimized_query,
  query_scc_condensation,
  boundary_consumption_intents,
  query_cardinality_and_provenance,
  planning_policy
)
```

The pass runs after Query structural optimization, column identity, differential
analysis, and semantic SCC discovery. It runs before storage equivalence or
ControlFlow table allocation. `ApplyDemandTransform` and
`BuildEquivalenceSets` are not part of the target Query build.

### 7.2 Root obligations

Roots include:

1. each `INSERT`, publication, message, and external query-read use;
2. each internal consumption operator such as `ONLY`, `EXISTS`, or
   `CHOOSE_ONE`;
3. each join pivot equality as a candidate family context, even without an
   external bound parameter;
4. each aggregate group key as a candidate family context;
5. each explicit standing query/adornment.

Items 3 and 4 are why redundancy can be useful without externally driven
demand. A per-pivot or per-group family can remove repeated key columns, enable
independent work partitions, and justify a narrower columnar arrangement.

### 7.3 Baseline and candidates

There is one semantic baseline, not an old compatibility mode. It mirrors the
optimized Query DAG with maximal sharing of exact
`(QueryOriginId, empty ContextSignature)` occurrences:

```text
FlatFamily:
  context = empty
  residual schema = full logical schema
  shared structure follows the original Query DAG
  every outgoing OriginUse still has its own coverage record
  state/effect boundaries terminate backward growth
```

This does not recompute the whole Query prefix once per root. A shared flat
occurrence publishes once to its distinct covered uses, just as one current
Query node can have several successors. Consumer-local duplication appears
only when an explicitly selected contextual candidate splits that sharing, and
the score includes the duplicate work/state.

Every specialization is a certified coordinate change from that baseline.
Unsupported or unprofitable boundaries remain in an empty-context family with
a typed blocker in the dump.

Candidate seeds are canonical context signatures:

```text
CandidateSeed =
  BoundaryBinding(root_use, bound_columns)
  | JoinPivot(join_origin, equality_classes)
  | AggregateGroup(aggregate_origin, group_columns)
  | ConsumptionGroup(consumer_origin, group_columns)
```

### 7.4 Backward transfer

```text
function transfer_context(use, desired_context): TransferDecision:
  match producer operator:
    SELECT / exact TUPLE forwarding / MERGE arm:
      Preserve context through proven column lineage

    COMPARE:
      Preserve if predicate is pure and does not redefine context

    MAP(functor):
      Preserve source slots
      Derive output slots only with a pure deterministic expression certificate
      Rekey backward only with a declared inverse/bidirectional certificate

    JOIN:
      distribute equality-class slots to every side containing that class
      bind the common pivot once in the enclosing family

    NEGATE:
      preserve only when completeness can be established per context key

    AGGREGATE / KVINDEX / CHOOSE / ONLY:
      preserve group slots; stop or create a nested family at value state

    recursive SCC member:
      admit the entire SCC only if every cycle preserves the signature

    INSERT / message / impure forcing / unknown functor:
      Block with a typed reason
```

### 7.5 Deterministic global greedy selection

Growing one root completely before considering the next would make results
depend on root iteration order and allow the first root to capture every
shareable region. Use a global best-improvement loop over the SCC-condensed
Query graph:

```text
function build_grove(query):
  obligations = enumerate_all_origin_uses(query)
  plan = mirror_query_dag_with_shared_empty_context_occurrences(obligations)
  frontier = seed_all_candidate_contexts(query)

  while frontier not empty:
    candidates = []
    for item in frontier:
      decision = certify_one_backward_growth(item, plan)
      if decision is Admissible:
        candidates.push(score(decision, plan))
      else:
        plan.record_block(decision.blocker)

    if no candidate has positive certified benefit:
      break

    chosen = max(candidates,
                 key=(benefit, stable_context_key, stable_origin_use_id))
    plan = apply_growth_transactionally(plan, chosen)
    ValidateCoverageAndContext(plan)
    frontier = update_affected_frontiers(frontier, chosen)

  canonicalize_ids(plan)
  ValidateInstanceFlow(plan)
  return plan
```

“Greedy” is only the cost selection. Correctness comes from transfer
certificates and validators. The score may compare:

```text
benefit = avoided_row_bytes
        + avoided_key_hashes_and_comparisons
        + pruned_derivation_row_touches
        + locality_and_parallelism_credit
        - duplicate_state_bytes
        - activation_and_rebuild_touches
        - extra_arrangement_maintenance
```

Unknown estimates cannot prove a transformation safe. They can select the flat
candidate or leave two certified candidates tied. Tie-breaking is stable and
semantic-ID based, never pointer or discovery-order based.

### 7.6 SCC atomicity

The greedy unit is a Query SCC, not an arbitrary recursive node:

```text
function certify_recursive_growth(scc, context):
  require every recursive edge preserves each context slot
  require every seed entering the SCC can be partitioned by the same context
  require no operator inside forgets or merges context keys
  require all negative/aggregate dependencies obey stratification
  return WholeSccFamily(context) or Block(reason)
```

A family never contains half of a recursive SCC. This prevents a local-looking
binding from turning a global fixpoint into several incomplete fixpoints.

### 7.7 Activation graph cycles

Query recursion and family activation are different graphs. Acyclic Query may
still produce a family cycle if `A`'s output acquires `B` and `B`'s output
acquires `A`. Naive reference counting would make such a cycle self-sustaining
after its external root disappeared.

Build and condense the activation graph after family selection:

```text
function validate_activation_graph(flow):
  for component in strongly_connected_components(flow.activations):
    if component is singleton without self-edge:
      continue

    if all families are context-compatible and can be coalesced:
      replace component with one atomic family region
    else:
      reject the contextual split and retain the certified flat coverage

  require condensation(flow.activations) is acyclic
```

The initial architecture does not ship cyclic interest reference counting.
Recursive logical cycles live inside `WholeQueryScc` family regions; interest
enters those regions from the acyclic activation condensation. A future cyclic
activation algebra would need root-supported reachability/fixpoint semantics,
not ordinary `InterestCount`, and should be a new explicit design.

## 8. Coverage, sharing, redundancy, and emission ownership

The central invariant is about **uses**, not nodes:

```text
for every OriginUseId u:
  union(coverage domains assigned to u) == complete domain of u
  pairwise_intersection(coverage domains assigned to u) == empty
```

One Query origin may therefore occur in many families while every consumer
obligation is still covered exactly once.

`CoverageDomain` is a static proof domain, not the current set of interested
keys:

```text
CoverageDomain =
  All(ContextSignature)
  | StaticKeys(ContextSignature, sorted distinct literals)
  | HashPartition(ContextSignature, partition_index, partition_count)
  | ProvedPredicatePartition(ContextSignature, PartitionProofId)
```

The first implementation should use `All` almost everywhere and
`HashPartition` only for explicit worker partitioning. Arbitrary predicate
partitions are illegal without a proof object establishing exhaustiveness and
pairwise disjointness. A standing family whose live interest set currently
contains only three keys still covers `All`: inactivity is lifecycle state,
not permission for a second occurrence to cover the supposedly inactive keys.
This avoids dynamic handoff between a flat writer and a keyed writer.

### 8.1 Three kinds of “same”

Replace the overloaded current storage equivalence with three relations:

```text
LogicalCollectionId:
  must denote the same set-valued truth

ForwardingEquivalence:
  bijective row-identity-preserving coordinate transform

StorageCompatibility:
  physical schemas/lifecycles permit a shared resource
```

Only the first is semantic equality. The second may justify zero-copy aliases.
The third is a materialization opportunity, not a union operation on Query.

### 8.2 When redundancy is good

Redundancy is justified when it buys a distinct use-specific property:

- one arrangement is ordered by `(K, A)` and another by `(K, B)`;
- one consumer needs full rows while another needs only two payload columns;
- per-key segments avoid carrying `K` through a hot join body;
- read replicas allow independent concurrent scans without a shared mutable
  cursor or lock;
- immutable columnar runs are cheaper to scan while a row/counter store remains
  the differential authority;
- a deep derived region is active for a small set of standing keys;
- rebuilding a cheap pure region is less expensive than retaining its global
  output.

Redundancy is not justified merely because two planner roots happened to grow
separately. Every replica has an explicit consumer set, cost, lifecycle, and
source authority.

### 8.3 Multiplicity laws

Keep these counters separate:

```text
SettledSupportCount(logical_row):
  number of live logical derivations

WorkingDerivationCounter(logical_row, phase):
  signed transient state used by overdelete/rederive/insert

InterestCount(family_key):
  number of live consumers requiring the instance

ReplicaCount(arrangement):
  number of physical copies
```

Only settled support at the owning visibility boundary determines logical
visibility:

```text
apply_support_delta(row, d):
  before = support[row]
  after = before + d
  require after >= 0
  support[row] = after

  if before == 0 and after > 0: publish VisibilityDelta::Enter(row)
  if before > 0 and after == 0: publish VisibilityDelta::Exit(row)
```

Rel's split/working counters may temporarily enter states that are not settled
logical support. They do not feed replicas or interests directly. Rel first
completes the required overdelete/rederive/insert phase and then authors the
net visibility transition according to the table's existing differential
contract.

Set-relational join bodies consume distinct visible input rows. If one visible
left row has three upstream derivations and one visible right row has two, the
join body sees one left/right pair, not six. The output's support changes when
distinct source-row pairs or distinct rule sites appear/disappear. Upstream
support changes that do not cross visibility zero do not enter a
visibility-fed arrangement.

`SourceRowToken` denotes the full logical source-row identity needed after a
non-injective projection. It is never a bare physical row slot or linked-index
position, because compaction may move rows and slot reuse may name a different
row. A content/full-key identity may be reused when the same logical row is
later reborn; any cached physical handle paired with it carries a
`SnapshotGeneration` and is invalid outside that generation.

Read replicas and family caches receive visibility changes, not raw derivation
support, unless they are themselves the designated support authority for an
internal residual collection. Creating a replica never increments support.
Adding a second interest never emits the row again. Rebuilding an instance
from a snapshot never replays logical insertions into the authoritative
collection.

### 8.4 Exact-one emission authority

For every `(DerivationSiteId, CoverageDomain)` there is exactly one writer:

```text
V-EMISSION-UNIQUE:
  authorities partition each derivation site's domain
  every emitting FamilyNode names one authority
  no two authorities overlap
  every required domain is covered
```

Non-authoritative occurrences may:

- read the authoritative logical collection;
- maintain a visibility-fed replica;
- compute a consumer-local residual result whose next edge has its own unique
  use coverage.

They may not fold the same logical derivation into the same collection.

## 9. Materialization and the Datatoad lessons

Datatoad's relevant design, as of commit `9caaa13`, is:

- `Forest`: sorted, deduplicated column layers connected by bounds;
- each column is a sequence of sorted lists extending a prefix;
- layer boundaries are type-erased ranges, permitting per-column
  specialization;
- `FactLSM`: immutable forests in geometrically sized levels;
- `Relations::Forms`: action-specific projections/permutations maintained from
  one base relation;
- column-at-a-time sorting carries grouping/order metadata to the next column.

The implementation is in `src/facts/{mod.rs,trie.rs}` and the design is
described in the project's [columnar tries](https://github.com/frankmcsherry/datatoad/blob/main/mdbook/src/chapter_1/chapter_1_1.md),
[columnar sorting](https://github.com/frankmcsherry/datatoad/blob/main/mdbook/src/chapter_1/chapter_1_2.md),
and [LSM tries](https://github.com/frankmcsherry/datatoad/blob/main/mdbook/src/chapter_1/chapter_1_4.md)
chapters.

The direct adaptation is an occurrence-specific physical form:

```text
ArrangementSpec:
  source: LogicalCollectionId | FamilyNodeId
  context_key: ordered ContextSlotIds
  residual_columns: ordered logical columns
  residual_order: ordered sort columns
  identity: SourceRowTokenPolicy
  lifecycle: MonotoneRuns | DifferentialRuns | VisibilityReplica
  partitioning: PartitionSpec
  consumers: non-empty set OriginUseId
```

The first runtime shape should be simpler than a general trie:

```text
GroupedArrangement<K, Columns...>:
  key_directory: sorted distinct K
  segment_bounds: offsets from each K to its residual rows
  residual_columns: one contiguous vector per needed column
  row_tokens: stable identity column when projection is non-injective
  generations: immutable sorted runs or epoch segments
```

For a batch of pivot keys, the join can sort/unique the pivots—as it already
does—and merge-align them with `key_directory`. Matching segments are then
scanned without a hash probe or linked row-ID chain per key.

### 9.1 Immutable runs and differential authority

Do not make a read-optimized columnar replica the accidental owner of signed
truth. Use explicit strategies:

```text
StateLayout =
  AuthoritativeRowSupportStore(RowSupportPlan)
  | AuthoritativeDifferentialRuns(DifferentialRunPlan)
  | MonotoneGroupedRuns(MonotoneRunPlan)

ReplicaLayout =
  VisibilityFedGroupedColumns(GroupedColumnPlan)
  | VisibilityFedHashIndex(HashReplicaPlan)
```

A pragmatic first slice keeps the current counter-capable row store as
authority and maintains `VisibilityFedGroupedColumns` only on zero crossings.
Later, signed sorted runs can become authoritative if compaction, survivor
lookup, and support consolidation are proven.

Immutable runs should follow a geometric merge discipline. New epoch runs are
sorted and deduplicated; similarly sized runs merge; readers either search the
small logarithmic run set or a compacted generation. The plan must measure
write amplification and tombstone/dead-row pressure rather than assume the
columnar form wins universally.

### 9.2 What not to import from Datatoad

Datatoad's WCOJ planner introduces terms one at a time, asks every atom to
`count`, selects the smallest proposer per prefix, then has other atoms
validate proposals. Its incremental evaluation also builds one plan per
source atom. Those are coherent with its interpreter and immutable
insertion-only facts, but importing them now would replace Dr's binary join,
differential scheduling, and generated-code architecture.

Therefore:

```diff
- adopt count/propose/validate and term-at-a-time Generic Join
- redesign every functor and relation as a WCOJ atom
+ retain Dr's current binary/product/negation/aggregate semantics
+ adopt sorted grouped physical forms and action-specific arrangements
+ let Rel choose hash probe, grouped merge, point test, or scan per access
```

## 10. MaterializationPlan

Materialization is a separate deterministic plan so topology and storage do not
become one untestable decision.

```text
MaterializationPlan:
  resources: ordered map StateResourceId -> StateResource
  arrangements: ordered map ArrangementId -> Arrangement
  bindings: FamilyPortOrNode -> ResourceBinding
  aliases: ordered list ForwardingAlias
  replicas: ordered list ReplicaFeed

StateResource:
  authority_for: LogicalCollectionId | InternalResidualCollectionId
  schema: PhysicalSchema
  support_policy: SupportPolicy
  lifecycle: ResourceLifecycle
  partitioning: PartitionSpec

ResourceBinding =
  Authoritative(StateResourceId)
  | Alias(StateResourceId, ForwardingEquivalence)
  | Replica(ArrangementId, ReplicaFeedId)
  | Ephemeral(EphemeralBufferPlan)
```

Planning is constraint-first:

```text
function plan_materialization(instance_flow):
  requirements = collect_arrangement_requirements(instance_flow)
  resources = allocate_one_authority_per_required_stateful_collection(requirements)
  aliases = coalesce_only_proven_forwarding_equivalents(resources)
  replicas = choose_compatible_use_specific_forms(requirements, cost_model)
  validate_no_replica_is_authoritative(resources, replicas)
  return canonical_plan(resources, aliases, replicas)
```

This replaces `BuildEquivalenceSets`. There is no second storage-sharing pass
on Query and no permanent compatibility bridge.

The current equivalence sets are useful migration evidence, not future
authority. If a set exists because an `INSERT`/`SELECT` pair denotes one
logical relation, its members map to one `LogicalCollectionId`. If it exists
because a TUPLE is a bijective pass-through, it becomes
`ForwardingEquivalence`. Those facts strongly encourage backward growth to
absorb the forwarding chain and strongly encourage one shared authority, which
is the principled version of “if the old graph says share, suck it into the
tree.” They do not force two consumer-local residual schemas to use one
physical arrangement.

## 11. Rel after InstanceFlow

Rel consumes abstract IDs and fully specified occurrence topology:

```text
BuildRel(
  query,
  instance_flow,
  materialization_plan,
  query_sccs,
  cost_facts
) -> RelProgram
```

It does not accept `ProgramImpl *`, `Context &`, or preallocated `TABLE *` as
the source of truth. Rel objects refer to:

```text
QueryOriginId
OriginUseId
FamilyId / FamilyNodeId
LogicalCollectionId
StateResourceId / ArrangementId
EmissionAuthorityId
```

ControlFlow later maps resource IDs to `Table`, `DiffTable`,
`GroupedArrangement`, `StateCellStore`, and `FamilyStore` allocations.

### 11.1 What Rel keeps

Preserve and generalize:

- old/new membership predicates and context;
- signed folds and recursive/nonrecursive derivation classes;
- `Access/Gate/Fold` plan spines;
- point, section, full-scan, and future seek lowerings;
- effect sets and RAW/WAR/WAW hazard derivation;
- claim, retire, rederive, filter, and commit operations;
- group/state-cell algorithms;
- recursive round shells and quiescence tests;
- explicit keyed-family birth, rebuild, death, and seal operations;
- one checked linearization.

### 11.2 What Rel deletes

Replace:

- `DRTable::model` and all semantic use of `TABLE *`;
- branch and join rediscovery from Query paths;
- recognition of demand-fabricated subgraphs;
- `DRInstance` as a narrow special case tied to a recognized `MERGE`;
- physical table identity inherited from Query equivalence sets;
- any ControlFlow path that independently chooses an eager or differential
  algorithm.

### 11.3 Complete lowering pseudocode

```text
function BuildRel(query, flow, materialization):
  rel = RelProgram(trace_parent=flow.id)

  for resource in materialization.resources:
    rel.resources += DeriveResourceLifecycle(resource)

  for family in flow.families:
    rel.regions += PlanFamilyLifecycle(family)

    for node in family.nodes:
      rel.ops += PlanOccurrence(node,
                                flow.coverage_for(node),
                                materialization.binding_for(node))

    for port in family.ports.pushes:
      rel.ops += PlanPushIngest(port)

    for port in family.ports.probes:
      rel.ops += PlanProbe(port)

    for port in family.ports.interests:
      rel.ops += PlanInterestLifecycle(port)

  rel.ops += PlanReplicaFeeds(materialization.replicas)
  rel.rounds += PlanWholeSccFamilyRounds(flow.recursive_families)
  rel.effects = DeriveEffects(rel)
  rel.dependencies = DeriveHazards(rel.effects)
  rel.order = LinearizeEpochAndRoundScopes(rel)
  ValidateRelPreservesInstanceFlow(rel, flow, materialization)
  return rel
```

### 11.4 Rel plan decisions remain single-owner decisions

InstanceFlow can require “exact-key probe,” “standing interest,” or “grouped
iteration.” It does not pick the delta algorithm. Rel records exactly one
decision per applicable work scope:

```text
AccessDecision =
  HashPointProbe(HashProbePlan)
  | SortedSectionProbe(SortedSectionPlan)
  | SortedBatchMerge(SortedBatchMergePlan)
  | FullScan(FullScanPlan, TypedBlocker)

FamilyLifecycleDecision =
  SnapshotProbe(SnapshotFamilyPlan)
  | StandingDifferential(StandingFamilyPlan)
  | ExclusiveMonotone(ExclusiveMonotonePlan)
  | CompleteGlobal(CompletePlan, TypedBlocker)
```

This preserves the bounded-observation rule: Query owns meaning, InstanceFlow
owns contextual scope, and Rel owns the one physical temporal realization.

## 12. ControlFlow and traceback

ControlFlow becomes a mechanical structured lowering:

```text
function LowerRel(rel, materialization):
  allocations = AllocateRuntimeResources(materialization)
  program = Program(allocations)

  for region_or_op in rel.pinned_order:
    program.append(LowerExactlyOne(region_or_op, allocations))

  ValidateEveryRelOpLoweredExactlyOnce(rel, program)
  return program
```

Every lower-level object preserves a non-null origin chain:

```text
TraceRef:
  query_origin: QueryOriginId
  origin_use: OriginUseId
  family: FamilyId
  family_node: FamilyNodeId
  rel_op: RelOpId
  control_flow_node: ProgramNodeId
```

Not every layer needs every field in its primary object. Store a typed trace
record in a side catalog keyed by the layer's ID. A ControlFlow diagnostic can
then say:

```text
program node cf#91
  <- rel op r#44 sorted-section-probe
  <- family node if#12.7 right_residual
  <- origin use u#38 join.6:right
  <- query origin q#17 select edge
```

Codegen never inspects Query shape, parse annotations, functor names, or
demand-recognition metadata to select an algorithm.

## 13. Differential and incremental lifecycle

### 13.1 The required event distinction

```text
InputEvent =
  SupportDelta(logical_row, signed_amount)
  | VisibilityDelta(logical_row, Enter | Exit)
  | InterestDelta(family_key, Acquire | Release)
  | SnapshotReady(family_key, generation)
  | EpochComplete(epoch)
```

Conflating these events causes the hardest bugs. In particular, an interest
addition is not a fact insertion, and a replica rebuild is not a replay of
derivations.

### 13.2 Birth without missed deltas

Standing family birth must join a stable snapshot with a delta cursor:

```text
on first Acquire(key):
  generation = authority.pin_generation()
  instance = family_store.create(key, generation)
  rebuild instance from probe(key, generation)
  replay visibility deltas after generation
  mark instance Live only after replay catches the current frontier
```

The authority must retain the post-generation delta window until the instance
acknowledges it. “Scan then start listening” without this handshake loses rows.

### 13.3 Release and rebirth

```text
on final Release(key):
  mark instance Retiring at current frontier
  finish or cancel only work owned by that instance
  publish required output exits through its emission authorities
  wait until downstream exits are acknowledged
  retire local resources and release pinned generations
```

If an `Acquire` arrives during retirement, the lifecycle makes one explicit
choice—cancel retirement or create a later generation. It does not run two
uncoordinated instances for the same `(FamilyId, key, generation)`.

### 13.4 Batch ordering

Rel derives the exact schedule, but the semantic partial order is:

```text
1. net external support changes
2. update authoritative support state
3. publish visibility zero crossings
4. apply interest changes caused by those visibility changes
5. rebuild newly born instances from a pinned snapshot
6. process family-local deltas and recursive rounds
7. publish family outputs through unique authorities
8. seal resources, replicas, and generations
9. retire zero-interest instances whose exits are complete
```

Some independent steps may execute concurrently. Rel's effect graph proves
which ones.

### 13.5 Differential hazards

Validators must catch:

- applying support changes twice through overlapping families;
- publishing raw support to a visibility replica;
- snapshot/delta gaps or duplicate replay;
- treating `Many` as absorbing in a differential exact-one observer;
- forgetting a context slot without consolidating support;
- retiring an instance before its negative outputs drain;
- probing a generation while compaction invalidates its segment handles;
- processing new and old join sides in an order inconsistent with the signed
  delta formula;
- reusing a `SourceRowToken` after logical identity has died and been reborn;
- applying a replica's row order as though it were semantic order.

## 14. Operator and feature matrix

### 14.1 SELECT, TUPLE, MERGE, and COMPARE

Pure forwarding nodes are the easiest context carriers. Preserve context only
through proven column lineage. A TUPLE that drops or duplicates a context
coordinate needs `Forget` or an explicit derived-coordinate proof. A MERGE can
share one family only if all arms present the same canonical signature; other
arms enter through adapters or remain outside. COMPARE is a pure filter when
its operands and predicate have no effects.

### 14.2 MAP and functors

Classify functors by declared contract:

```text
FunctorContextCapability =
  PurePreserving(column_lineage)
  | PureDeriving(output_slot, expression_certificate)
  | Bidirectional(forward_certificate, inverse_certificate)
  | PureOpaque
  | Effectful
```

- `PurePreserving` can sit inside multiple families.
- `PureDeriving` can create a new context slot in the forward direction.
- `Bidirectional` can push a bound result backward only through its certified
  inverse and multiplicity contract.
- `PureOpaque` is executable but a context-growth barrier when the required
  coordinate crosses it.
- `Effectful` is both a growth and duplication barrier. It has one execution
  authority.

Do not infer purity, order, inverse behavior, or domains from a functor's name.
Named result types must keep addresses, scores, ranks, weights, and counts from
becoming interchangeable raw primitives.

### 14.3 Joins and products

For equijoins, unify equal key columns into context slots and keep the current
binary incremental arms. A no-pivot product has no natural key to bind; it
stays flat unless a surrounding context partitions every side. Never treat a
product as cost-free. The cost model propagates its multiplicative cardinality.

Multiple join consumers may request different arrangements of the same side.
That is a primary valid use of replicas.

### 14.4 Negation

Absence is not answerable from partial demand alone. A per-key negative gate is
legal only with a completeness proof:

```text
NegativeReady(key, epoch) =
  positive driving scope for key is complete at epoch
  AND negated collection's key snapshot includes all changes through epoch
```

Interest in a key may activate storage and computation, but absence cannot be
published until `NegativeReady`. On later insertions, the output retracts; on
later removals, it may reappear. Recursive negation remains governed by Query
stratification and cannot be pulled into the lower SCC.

An existing negation hint such as `@never` may select its Query membership
semantics, but it does not waive context completeness, generation ordering, or
the unique-emission proof.

### 14.5 Aggregates, KV indexes, and state cells

An aggregate group key is a natural family context:

```text
family AggregateAtGroup[G = g]:
  residual inputs omit G
  one StateCell holds the aggregate algebra for g
  publication reconstructs G at the boundary
```

The aggregate algorithm remains a Rel decision:

```text
AggregateLifecycle =
  InvertibleIncremental(InverseFoldPlan)
  | RecomputeFromAuthoritativeMembers(RecomputePlan)
  | MergeablePartial(PartialMergePlan)
```

`MergeablePartial` requires a declared associative merge and identity; a
generic aggregate cannot be split across families and merged speculatively.
Recompute state needs authoritative member access and survivor/rebuild
generation rules. Configuration columns may be family context but remain
distinct from true group columns in the reduction ABI.

### 14.6 `ONLY`, `EXISTS`, `CHOOSE_ONE`, and bounded reads

These are semantic Query operators or boundary intents, not raw row limits.
InstanceFlow turns their group keys and downstream dependencies into work
scopes. Rel still selects snapshot, standing differential, ordered-probe, or
complete implementations.

For a B-then-R exact-one computation:

```text
BOnly emits One(F, cls, b)
  -> InterestDelta::Acquire ROnly[(S, cls)]

BOnly retracts One because it becomes Empty or Many
  -> InterestDelta::Release ROnly[(S, cls)]
```

There is no parallel eager R path. Read boundedness, producer boundedness, and
downstream activation remain separate decisions and dumps.

### 14.7 Recursive queries

Recursive families host a fixpoint per context key only after the whole SCC is
admitted:

```text
family RecursiveAtKey[K = k]:
  seed_frontier = seeds restricted to k
  accumulated = empty residual set

  until frontier empty:
    derived = apply every SCC rule to frontier and accumulated
    next = derived not visible in accumulated
    fold next into accumulated
    frontier = next
```

The existing keyed instance `RESCAN(k)` is one pass and is insufficient for
recursive content. The new Rel region must represent instance-scoped round
shells, loop-carried frontiers, quiescence, and unique output authority.

For differential recursion, each family instance owns its overdelete,
rederive, and insert phases. Interest death is not the same as logical input
retraction: death retracts the instance's published coverage; input retraction
updates the live fixpoint for an instance that remains demanded.

Initial recursive admission requires:

- context preservation around every cycle;
- seed completeness for the key;
- no cross-key aggregate or forget operation inside the SCC;
- a cost witness showing the keyed closure is preferable to the complete
  closure for the target workload;
- equivalence against the flat recursive oracle over insertion and retraction
  traces.

### 14.8 Messages, inputs, publications, and queries

External messages are push-driven and generally must persist completely. A
family may prune derived computation downstream of them; it cannot refuse an
input row merely because no key is currently interesting unless the language
boundary explicitly defines a lossy subscription.

Publications and inserts are logical effect boundaries. They have one emission
authority and are duplication barriers. External queries may use transient
probes without creating standing interests; subscriptions use interests and
therefore participate in birth/death lifecycle.

A query-body forcing message is sequencing/effect intent, not a bounded-read
intent despite the current shared `@first` spelling. It remains a single
execution boundary and blocks duplication. A declaration-level first-row read
is represented by its typed Query boundary intent and may receive a bounded
probe plan.

### 14.9 Conditions and zero-arity relations

A condition is a real zero-arity logical collection, not a null key. Its
context signature is empty and its residual identity is the unit token.
Broadcast/partition decisions must preserve its global presence semantics.

## 15. Concurrency and partitioning

Family keys provide an explicit parallelism boundary:

```text
partition = hash(FamilyId, FamilyInstanceKey) % workers
```

Independent keys may rebuild, scan, and run fixpoints concurrently when their
effects touch disjoint resource partitions. Shared authoritative collections,
global aggregates, zero-arity conditions, and publications introduce effect
edges or barriers.

Columnar redundancy can improve concurrency because consumers scan immutable
segments rather than sharing mutable index-chain cursors. It can also worsen
memory bandwidth by maintaining too many replicas. MaterializationPlan records
the partitioning and Rel's cost model prices both maintenance and reads.

Determinism requirements:

- logical final state is independent of worker/interleaving order;
- observable delta streams obey the language's batch contract;
- arbitrary choice is allowed only where Query says it is allowed;
- debug IDs and plan selection are deterministic independent of pointer values;
- replay from the same input batches yields the same canonical state hash.

## 16. Debugging and rendering

Add two first-class outputs:

```text
-instanceflow-out <file>      canonical textual IR
-instanceflow-dot <file>      provenance-aware grove rendering
```

The text dump is the semantic review surface. A family renders:

```text
family if#3 root=u#41 context=(k0:q#17.K) activation=standing-interest
  covers u#41 domain=all authority=ea#7
  node if#3.0 origin=q#22 join role=root residual=(A,B)
  node if#3.1 origin=q#17 left role=input residual=(A,token)
  node if#3.2 origin=q#19 right role=input residual=(B,token)
  interest in key=(k0) from=if#1.publish
  require arrange source=lc#4 key=(k0) columns=(A,token) order=(A)
  blocked at q#12 reason=effectful-functor
```

The Materialization dump adds:

```text
resource sr#5 authority=lc#4 support=signed-row-store partition=hash(k0)
arrange ar#8 replica-of=sr#5 form=grouped-columns
  key=(k0) columns=(A,token) order=(A) consumers=(u#41,u#57)
```

Rel renders the selected realization and its parent:

```text
op r#44 parent=if#3.2/u#41 access=sorted-batch-merge arrange=ar#8
region rgn#6 parent=if#3 lifecycle=standing-differential
```

### 16.1 DOT conventions

- cluster each `FamilyTemplate`;
- show context slots in the cluster label;
- use solid edges for residual data push;
- use blue edges for probes;
- use orange edges for standing interest;
- use dashed gray origin links from occurrences to Query nodes;
- show authoritative resources as cylinders and replicas as double-outline
  cylinders;
- mark emission authority at publication edges;
- render blockers at the stopped backward frontier;
- render overlapping Query origins without visually merging their occurrences.

The current pointer-derived DOT identities are not acceptable for this surface.
Canonical IDs derive from stable Query order, canonical context signatures,
root uses, and stable tie-breakers.

### 16.2 Debug queries

The compiler should answer:

```text
why-family(if#3)
  root, growth steps, benefit terms, blockers, coverage

why-arrangement(ar#8)
  consumers, required columns/order, maintenance cost, sharing decision

trace-query-use(u#41)
  Query -> InstanceFlow -> materialization -> Rel -> ControlFlow

who-emits(lc#4, domain)
  exactly one EmissionAuthorityId
```

These may initially be information in dumps rather than an interactive CLI,
but the underlying trace catalog is a required IR contract.

## 17. Validators

Run validators unconditionally in compiler builds used for correctness, not
only when a dump or optimization flag is enabled.

```text
V-IF-ORIGIN:
  every FamilyNode references a live QueryOriginId with compatible schema

V-IF-CONTEXT:
  context transfers preserve logical rows and typed slot domains
  residual schema plus context reconstructs the logical schema

V-IF-COVERAGE:
  every OriginUse domain is covered exactly once

V-IF-EMISSION:
  every derivation coverage cell has exactly one emission authority

V-IF-MULTIPLICITY:
  support, interest, and replica operations occur only in their own domains

V-IF-SCC:
  a recursive Query SCC is wholly inside one compatible family region or wholly outside

V-IF-ACTIVATION:
  the family activation condensation is acyclic
  every nontrivial activation SCC was coalesced into one atomic family region

V-IF-NEGATION:
  every absence publication has a per-key completeness source

V-IF-EFFECT:
  effectful Query origins have one execution occurrence and cannot be duplicated

V-MAT-AUTHORITY:
  every state-backed LogicalCollectionId has exactly one authoritative support resource
  every ephemeral LogicalCollectionId has no persistent support resource

V-MAT-REPLICA:
  replicas consume visibility transitions and cannot emit logical support

V-MAT-SCHEMA:
  every arrangement contains its access key, required payload, and identity columns

V-REL-PRESERVE:
  every family port/node requiring execution has exactly one Rel realization

V-REL-TRACE:
  every Rel op and ControlFlow region has a valid origin chain

V-REL-HAZARD:
  effect-derived order satisfies epoch, generation, round, and retirement hazards
```

Validation is transactional: construction either returns one complete valid IR
or compilation fails. There is no catch-and-clean partial plan.

## 18. Testing strategy

### 18.1 Canonical goldens

Use small, reviewable golden families:

```text
<case>.df                 logical Query dump
<case>.instanceflow       contextual topology/coverage dump
<case>.materialization    resources and arrangements
<case>.rel                temporal plan
<case>.ir                 ControlFlow
```

Goldens should pin stable semantic fields and decisions, including typed
blockers. They should not pin pointer addresses, allocation order, incidental
container iteration, or raw estimates that are intentionally scenario inputs.
Exact text goldens are appropriate once canonical IDs and ordering exist.

Do not use goldens alone. They make architecture visible but cannot establish
multiplicity or differential correctness.

### 18.2 Structural unit tests

- context canonicalization across equality classes;
- bind/preserve/derive/rekey/forget transfer rules;
- exact OriginUse coverage with shared origins;
- overlapping-domain rejection;
- unique emission authority;
- storage compatibility versus logical equality;
- effectful duplication rejection;
- SCC all-or-nothing admission;
- arrangement required-column/identity closure;
- stable IDs under unrelated insertion order.

Follow Datatoad's strongest recent testing pattern for each columnar
arrangement: compare it with a transparent row-set model. Build, merge,
project/permute, exact-key section, join, semijoin, and antijoin should extract
to the same `set<FullLogicalRow>` as the model. Run the same cases with
fixed-width and mixed-width column domains, duplicate-heavy inputs, and both
large and deliberately tiny merge/compaction budgets so specialized and
staged kernels share one semantic oracle.

### 18.3 Semantic oracle tests

For every supported family shape, run the same input trace through:

```text
reference Query interpreter or flat complete lowering
InstanceFlow lowering
```

Compare final logical collections and the specified batch-relative delta
streams. The flat execution is a test oracle, not a shipped compatibility path.

Required join-key traces include:

- left then right insertion;
- right then left insertion;
- multiple derivations of one left/right row;
- projected duplicate join outputs;
- removal and rebirth;
- two keys interleaved;
- two consumers sharing one origin with different arrangements;
- one contextual and one flat consumer of the same origin.

### 18.4 Differential property tests

Generate short signed traces subject to nonnegative support:

```text
insert derivation
insert duplicate derivation
remove one derivation
remove final derivation
acquire interest
release interest
compact
reacquire
```

After every batch assert:

```text
visible(authority) == reference_set
each live replica == projection(reference_set)
each family output == reference_query restricted to live coverage
sum of emitted support deltas == reference derivation delta
no zero-interest family retains unretired output after its frontier drains
```

### 18.5 Feature witnesses

At minimum:

- monotone pivot-bound join;
- differential pivot-bound join;
- product blocked without common context;
- pure map preserving a key;
- invertible/bidirectional map rekey;
- effectful functor boundary;
- negate with completeness and a rejected incomplete case;
- invertible aggregate per group;
- recompute aggregate per group with removals;
- `ONLY` transition `0 -> 1 -> 2 -> 1 -> 0`;
- boundary first-row probe over a complete producer;
- nonrecursive standing family birth/death/rebirth;
- recursive transitive closure per key;
- differential recursive closure with edge removal;
- multi-adornment families sharing one publication authority;
- zero-arity condition;
- external input retained while derived computation is inactive.

### 18.6 Perturbation tests

Every major validator needs a test-only construction that violates precisely
one invariant before normal lowering and proves compilation fails loudly:

- duplicate coverage;
- duplicate emission authority;
- support event sent to replica twice;
- missing snapshot generation;
- half-SCC family;
- missing negation completeness;
- context forget without fold;
- Rel op without parent trace;
- ControlFlow double-lowering.

These are direct unit fixtures, not runtime feature flags or production
fallback paths.

### 18.7 Performance proof

Extend the cost model to four layers:

```text
Query:
  logical cardinality and keyset provenance

InstanceFlow:
  number of live keys, coverage fraction, rebuilds, duplicate derivation work

MaterializationPlan:
  authoritative bytes, replica bytes, sort/merge maintenance, write amplification

Rel:
  row touches, probes, segment scans, rounds, survivor lookups, effects
```

Use scenario families rather than one nominal cardinality. Counters should
measure:

- carried key bytes removed;
- hash probes versus sorted-directory comparisons;
- segment rows scanned;
- replica visibility updates;
- instance births/rebuilds/deaths;
- duplicate pure computation;
- aggregate recompute members;
- recursive rounds per key;
- peak and steady-state bytes.

The first acceptance comparison is current exact-key hash/index-chain join
versus pivot-sorted `GroupedArrangement`, without WCOJ or changed join
semantics.

## 19. Full pipeline pseudocode

```text
function Compile(module, options):
  parsed = Parse(module)

  query = BuildLogicalQuery(parsed)
  OptimizeLogicalQuery(query)
  FinalizeColumnLineage(query)
  AnalyzeDifferentials(query)
  query_sccs = StratifyAndIdentifySccs(query)
  ValidateQuery(query)

  query_analysis = AnalyzeCardinalityProvenanceAndConsumption(query)

  instance_flow = BuildInstanceFlow(
      query,
      query_sccs,
      query_analysis.consumption,
      query_analysis.cost,
      options.planning_policy)
  ValidateInstanceFlow(instance_flow, query)

  materialization = PlanMaterialization(
      query,
      instance_flow,
      query_analysis.cost)
  ValidateMaterialization(materialization, instance_flow)

  rel = BuildRel(
      query,
      instance_flow,
      materialization,
      query_sccs,
      query_analysis.cost)
  DeriveRelEffectsDependenciesAndOrder(rel)
  ValidateRel(rel, instance_flow, materialization)

  control_flow = LowerRel(rel, materialization)
  ValidateControlFlowCoverage(control_flow, rel)

  return GenerateCode(control_flow)
```

No later phase walks Query to choose an executable algorithm. Query remains
available for diagnostics, logical types, and semantic provenance.

## 20. Implementation sequence and deletion plan

### Phase A: identities and fail-loud foundations

1. Add stable `QueryOriginId`, `OriginUseId`, `LogicalCollectionId`, and
   `DerivationSiteId` catalogs after Query optimization.
2. Add complete column-lineage and equality-class queries needed by context
   transfer.
3. Remove the broad `catch (...)` finalization path; make invariant failures
   reach the compilation boundary.
4. Canonicalize Query dump/DOT identities needed for traceback.

Exit gate: every Query use and derivation site is stable and total across the
corpus.

### Phase B: flat InstanceFlow and abstract resources

1. Build a flat empty-context family plan covering every origin use.
2. Add InstanceFlow text/DOT rendering and validators.
3. Add MaterializationPlan with abstract resource IDs, initially selecting
   layouts equivalent to the current runtime requirements.
4. Refactor Rel to consume resource and occurrence IDs instead of `TABLE *`.
5. Allocate runtime resources only in ControlFlow lowering.

The temporary implementation may cross-check old/new inventories inside a
single development commit series, but the landed phase deletes the old
authority and its mode flags. Cross-check scaffolding is not architecture.

Exit gate: normal compilation is byte- or behavior-equivalent where ordering
is semantic, every Rel/ControlFlow object traces through InstanceFlow, and
`BuildEquivalenceSets` no longer owns storage.

### Phase C: complete the Rel authority cutover

1. Lower all eager, join, differential, group, query-read, and round work from
   Rel.
2. Delete ControlFlow Query-shape dispatch and old branch/join discovery.
3. Delete Rel demand-subgraph recognition.
4. Make codegen consume only typed ControlFlow descriptors.

Exit gate: there is one executable-plan authority and exact-one lowering
coverage from Rel to ControlFlow.

### Phase D: first contextual specialization—join pivots

1. Seed `JoinPivot` contexts for monotone acyclic equijoins.
2. Bind K across left and right; retain residual row tokens.
3. Add `VisibilityFedGroupedColumns` with sorted key directory and residual
   columns.
4. Add Rel `SortedSectionProbe` and `SortedBatchMerge`.
5. Compare against current hash/index-chain access with semantic, structural,
   and row-touch tests.

This is the first optimization slice. It deliberately does not add Generic
Join.

### Phase E: standing interest and differential lifecycle

1. Add interest-counted birth/rebuild/seal/retire regions.
2. Add generation pin + delta replay handshake.
3. Feed replicas only with visibility transitions.
4. Add signed join and rebirth traces.
5. Replace the current query-root demand transform with InstanceFlow boundary
   and internal activation roots; delete fabricated demand guards.

Exit gate: flat demand, keyed instances, multi-adornment, demand retraction,
and differential input all lower from the same family vocabulary.

### Phase F: aggregates, negation, and bounded consumption

1. Admit group-key aggregate families with invertible and recompute strategies.
2. Add per-key completeness tokens for negation.
3. Lower `ONLY`, `EXISTS`, `CHOOSE_ONE`, and bounded reads through family work
   scopes and Rel decisions.
4. Delete recognized whole-world encodings after semantic Query operators
   replace them.

### Phase G: recursive families

1. Represent whole-SCC family regions and instance-scoped round shells in Rel.
2. Admit context-preserving monotone recursion.
3. Add differential overdelete/rederive/insert per family instance.
4. Lift recursive demand fences only after equivalence, lifecycle, and cost
   gates pass.

### Phase H: deeper columnar forms

1. Evaluate immutable LSM grouped runs as authoritative monotone state.
2. Add multi-layer prefix-compressed arrangements only where more than one
   residual key depth pays for the bounds structure.
3. Specialize fixed-width columns at compile time/runtime without creating a
   Cartesian product of row-specialized code.
4. Consider GPU/radix or broader column kernels only after measured sort cost
   dominates.

## 21. Hazards and explicit rulings

| Hazard | Ruling |
|---|---|
| First root captures a shared region | Global deterministic candidate selection; ownership is use coverage |
| Two trees emit the same fact | Exact-one `EmissionAuthority` over derivation coverage |
| Replica changes multiplicity | Replicas consume visibility only; replica count is nonsemantic |
| Bound K disappears logically | Context remains part of full row identity and traceback |
| Forgetting K merges rows | Mandatory support fold by remaining logical identity |
| Projection hides distinct sources | Preserve `SourceRowToken` until non-injectivity is resolved |
| New interest misses concurrent input | Pin generation, rebuild, replay post-generation delta |
| Release races rebirth | Explicit Retiring state and generation rule |
| Differential `Many` later becomes `One` | Exact member authority/survivor lookup; no permanent saturation |
| Negation answers from partial state | Per-key completeness token required |
| Half a recursive cycle is instantiated | SCC-atomic admission |
| Per-key recursion repeats global work | Cost gate and context-preservation proof |
| Effectful functor runs in replicas | Effectful origin is a duplication barrier with one authority |
| Storage sharing changes logical equality | Separate logical, forwarding, and compatibility relations |
| Columnar replica has stale/dead rows | Generation and visibility-feed contract; compaction effect hazards |
| Plan changes with pointer order | Canonical semantic IDs and deterministic tie-breaks |
| Correct result hides complete work | Work-scope decisions, blockers, and row-touch counters |
| Invalid partial Query reaches new IR | Remove broad catch-and-continue finalization |

## 22. Composability audit

### Authority

- Query has one owner for logical truth.
- InstanceFlow has one owner for contextual occurrence/use coverage.
- MaterializationPlan has one owner for state and arrangement identity.
- Rel has one owner for temporal algorithms and schedule.
- ControlFlow has no planning authority.

No decision is represented as both an annotation and an independently
rediscovered graph.

### Algebra

- Binding context is a coordinate change, not selection.
- Interest controls activation, not truth.
- Replication controls physical access, not support.
- Visibility is the zero-crossing of support.
- Context forgetting is an aggregation over supports.
- Recursive specialization distributes a fixpoint only when the context is an
  invariant of the complete SCC.

### State

- One authoritative support resource exists per logical collection.
- Every replica has one feed and a declared generation.
- Every live family instance has one lifecycle state.
- Every derived publication has one emission authority.
- Snapshot and standing differential state use different typed plans.

### Control

- The grove is selected globally and deterministically.
- Rel effects derive the only execution order.
- Birth, replay, publish, seal, and death are explicit events.
- No codegen path recovers intent from Parse or Query syntax.

### Cost

- Query cardinality is not row touches.
- family instance count is not relation cardinality.
- replica bytes are not authoritative bytes.
- cost ranks certified alternatives but never supplies a correctness proof.

### Testability

- every cross-layer transition has an exact coverage validator;
- every major plan has a canonical dump;
- semantic oracles cover final truth and delta traces;
- perturbation tests prove validators are live;
- performance counters prove the selected specialization actually does less.

## 23. Second review: failure modes in the first formulation

This architecture was re-read once as a Query designer, once as a
differential runtime designer, and once as a storage designer. The following
repairs are incorporated above.

### Finding 1: “the tree owns a node” was the wrong unit

A shared Query node can have several consumer uses, and node ownership would
either prohibit useful overlap or duplicate output folds.

**Repair:** cover `OriginUseId × CoverageDomain`; assign emission separately by
`DerivationSiteId × CoverageDomain`.

### Finding 2: family context risked becoming hidden row loss

Simply dropping K from residual rows is unsound at a boundary that merges
instances or projects away K.

**Repair:** context transfer is explicit; logical identity retains the family
key; `Forget` performs a support fold.

### Finding 3: redundancy risked multiplying truth

An independently maintained replica can look like another derivation source.

**Repair:** authoritative support, visibility-fed replicas, and local residual
support are different resource roles. Replica feeds cannot publish support.

### Finding 4: InstanceFlow and Rel could both become physical planners

The early draft let InstanceFlow select too much lifecycle/access detail.

**Repair:** InstanceFlow selects contextual topology and required capabilities;
MaterializationPlan selects resource/layout identity; Rel selects the temporal
algorithm and schedule.

### Finding 5: a literal forest hid sharing and activation cycles

Consumer trees may share family templates and recursive families contain
cycles.

**Repair:** the review surface is tree-oriented, but the formal IR is a grove
of family DAGs with SCC-owned regions and explicit activation edges.

### Finding 6: external demand was treated as the only source of binding

That misses the user's key observation. A join already uses a pivot as though
it were bound on the probed side.

**Repair:** join equality classes and aggregate group keys are internal
candidate roots even without a bound query.

### Finding 7: sorted storage was too quickly equated with columnar authority

Differential support, survivor lookup, and compaction make authoritative
columnar state a larger change.

**Repair:** the first slice is a visibility-fed grouped-column replica. Move
authority only after signed-run semantics and lifecycle tests exist.

### Finding 8: recursive demand was described as a bigger rescan

A single keyed rescan cannot compute a recursive closure.

**Repair:** recursive family admission requires an instance-scoped Rel round
shell for the whole SCC and later a differential three-phase fixpoint.

### Finding 9: successful bounded results could still hide whole-world work

Returning one row says nothing about producer or downstream effort.

**Repair:** read, producer, and downstream activation are distinct work scopes,
with selected plans or typed blockers.

### Finding 10: migration could leave two permanent authorities

A passive InstanceFlow mirror plus the old Query/ControlFlow path would repeat
the long Rel cutover problem.

**Repair:** temporary cross-checks are deleted at each phase exit. There is no
shipped old/new mode, compatibility graph, or warning-and-continue fallback.

### Finding 11: ordinary family parameterization looked heap-resident

If every distinct join key implied an `InstanceStore` allocation, the first
slice could cost more than the hash/index-chain implementation it replaces and
would miss the sorting simplification.

**Repair:** distinguish the family template from its realization. Ordinary
joins use virtual grouped or batch-ephemeral keys; only standing demand,
stateful aggregates, and recursive closures require keyed lifecycle records.

### Finding 12: the family activation graph could contain non-Query cycles

Reference-counted interests in a cycle can keep one another alive after the
last root disappears. Query SCC analysis alone does not see this graph.

**Repair:** build activation SCCs separately, coalesce compatible cyclic
components into one atomic family, and otherwise retain flat coverage. The
shipped activation condensation is acyclic.

### Finding 13: a root-per-tree flat baseline could duplicate the whole DAG

A naive Christmas-tree implementation might clone every shared Query prefix
once per output before it has selected any useful context.

**Repair:** the flat baseline mirrors and maximally shares the original Query
DAG. Only a scored contextual split introduces consumer-local duplication.

## 24. Third review: implementation go/no-go gates

The revised plan was reviewed again from the proposed phase order rather than
from the conceptual model. It is ready to implement only if each transition
can answer these questions mechanically:

```text
Query -> InstanceFlow:
  Can every origin use, derivation site, equality proof, effect boundary, and
  Query SCC be named without pointer or traversal-order identity?

InstanceFlow -> MaterializationPlan:
  Can every resource requirement identify its consumers, full logical schema,
  residual schema, context reconstruction, identity columns, and authority?

MaterializationPlan -> Rel:
  Can Rel select one lifecycle/access algorithm without walking Query to infer
  missing topology or consulting an already allocated TABLE *?

Rel -> ControlFlow:
  Can every Rel op lower exactly once, and can ControlFlow allocate every
  resource without choosing semantics or strategy?

Runtime -> diagnostics:
  Can every incorrect count, stale generation, duplicate emission, or missed
  retirement be traced to one Query use and one family occurrence?
```

The go/no-go rulings are:

1. Do not land contextual specialization before the flat InstanceFlow path is
   the sole input to Rel.
2. Do not land physical replicas before visibility/support separation is
   validated.
3. Do not delete a carried context column before reconstruction and `Forget`
   tests exist.
4. Do not enable standing interest before generation-safe birth and ordered
   death are represented in Rel.
5. Do not enable recursive families before activation condensation and
   instance-scoped rounds are explicit.
6. Do not call a plan columnar merely because it stores columns separately;
   require sorted grouped access to reduce measured touches or bytes.
7. Do not call a one-row result optimized unless its read, producer, and
   downstream work scopes say what stopped and why.

Under those gates, the architecture composes. Without any one of them, the new
IR would merely move an existing split brain to a new file.

## 25. Final recommendation

Proceed, but define InstanceFlow narrowly and insist on the layer boundaries:

1. insert stable origin/use identities and a maximally shared flat
   InstanceFlow at the existing Query-to-Rel seam;
2. move storage sharing out of Query into an abstract MaterializationPlan;
3. complete the one-authority Rel cutover against those inputs and delete the
   remaining ControlFlow/Query planning paths;
4. prove the model first with maximally shared flat families;
5. specialize ordinary left/right join pivots into `K`-context families;
6. add sorted grouped column replicas before attempting a general columnar
   authoritative store;
7. unify current demand/keyed-instance behavior as standing activation;
8. admit aggregates and recursion only through their explicit algebra and SCC
   proofs;
9. do not make WCOJ part of this project.

The key conceptual simplification is real: `K` can be constant with respect to
`left'(A)` and `right'(B)`. The hard part is not representing that constant. It
is preserving coverage, support, lifecycle, and emission ownership when many
such coordinate systems overlap. InstanceFlow earns its place if—and only
if—it makes those four obligations first-class and leaves Rel as the single
temporal authority beneath them.
