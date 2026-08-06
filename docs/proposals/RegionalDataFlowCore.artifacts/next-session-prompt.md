# Resume: relation-local keyed instances and lazy residual regions

Continue in the Dr. Lojekyll repository on branch `keyed-instances`. At the
time this revision was written, the code anchors were re-verified at
`1339124d`; that tip differs from the earlier reviewed `4c0deb7d` only by the
documentation commit that introduced this prompt. Re-check the branch tip and
worktree before acting.

This prompt is the current resumption authority for keyed-instance work. It
supersedes the earlier session-oriented charter in this file and corrects a
central category error in the current implementation and some older proposal
artifacts: `@key` is a relation-local declaration, not a spelling of a query
adornment and not an opt-in to the old demand transform.

## Objective

Build relation-local residual evaluation without making residual state a
second owner of relation truth. A declared key path specializes a relation by
binding fields to symbolic constants. The compiler may materialize those
specializations lazily, infer additional paths, and select trie/COLT-like or
Free Join physical plans without changing logical relation membership.

The target is not merely to replace the hash table inside `InstanceStore` with
a trie. The compiler needs a regional semantic representation that owns local
relations, semantic member identity, derivation support, recursive components,
partial bindings, exact request ownership, and routing between binding states.
Binding states and ordered arrangements refer to canonical relation facts;
they do not become competing fact stores.

Treat the codebase as greenfield. Do not preserve the current flat/nested dual
architecture, silent fallback, compatibility selectors, or tests that pin an
accidental coupling once the coherent replacement owns their responsibilities.

## Authority model

Keep four domains separate:

```text
Logical relation truth
    Canonical regional facts, named by lexical region instance, relation, and
    SemanticMemberKey. Derivation support decides membership.

Residual specialization
    A lazy evaluation context named by a canonical field/value binding map.
    It owns fixpoint frontiers and support contributions, not a private copy of
    logical relation truth.

Logical access path
    An ordered sequence of fields by which a specialization can be reached.
    Declared and inferred paths may converge on the same specialization.

Physical access structure
    A full scan, hash arrangement, trie, COLT, or other implementation over
    canonical facts. It never defines semantic identity or liveness.
```

Request ownership and recursive derivation are also different relations:

```text
RequestEdge
    Exact root or parent-member ownership of a requested binding state.

RuleActivationEdge
    A derivation-qualified dependency from a source fact/state to a destination
    binding state. These edges may be cyclic inside a region.

ActiveBindingState
    Rooted reachability from live RequestEdges through RuleActivationEdges.
    An internally referenced cycle with no live root is inactive.
```

Counts may cache whether a state has incoming support, but neither a count nor
an allocated runtime handle is the semantic owner. Cyclic liveness is a rooted
least-fixpoint problem; ordinary reference counting is insufficient.

## Non-negotiable language semantics

### Declaration ownership

The declaration domains are separate:

```text
#local / #export
    may declare @key(...)

#query
    may declare bound/free parameters
    may not declare @key(...)

#message / #functor
    may not declare @key(...)
```

`@key` declares an ordered, relation-local specialization path. It does not
declare semantic member identity, select a physical index, activate the old
demand transform, or require a matching query adornment. A query's `bound`
attributes describe an external calling convention. Query lowering may request
a compatible specialization, but it must not define or validate relation keys.

A keyed relation remains valid independently of how queries happen to use it.
In particular, it may have no bound query, be used only by rules, participate
in direct or mutual recursion, be unreachable from a query, be read before its
key is bound, or declare paths different from every query binding pattern.

Honoring a declared path is a semantic capability guarantee, not a performance
promise: `FullScanFilter` is a correct initial realization. If a future language
feature promises a physical layout, give that feature a separate contract.

The parser already routes `@key` through `ParseLocalExport` rather than query
parsing (`lib/Parse/Parser.cpp:357`). Preserve that separation and add an
explicit rejection witness for `@key` on a `#query`.

### Key order and partial bindings

`@key(A, B)` is an ordered access path. It induces the prefix sequence:

```text
empty -> {A} -> {A, B}
```

It is not the same path as `@key(B, A)`. However, both paths reach the same
semantic specialization after the same values are bound:

```text
                 {A=a}
                /     \
empty                           {A=a, B=b}
                \     /
                 {B=b}
```

Therefore distinguish three layers:

```text
Semantic specialization
    Which symbolic fields have fixed values?

Logical access path
    In which order were those fields bound?

Physical access structure
    Which full scan, hash index, trie, or lazy COLT realizes the path?
```

A semantic binding state is identified by its lexical `RegionInstanceId` and a
canonical map from symbolic fields to typed values. Traversal order is not part
of that identity. Different ordered paths may converge on the same state. The
state contributes derivations to canonical regional facts; it does not own a
path-private copy of those facts.

### Agreement between keys

After mapping declaration fields to canonical symbolic parameters, classify
key relationships rather than asking only whether two vectors are equal:

| Classification | Example | Consequence |
| --- | --- | --- |
| Exact path | `[A,B]`, `[A,B]` | Same navigation path |
| Prefix | `[A]`, `[A,B]` | Direct parent/child residual |
| Same binding set, different order | `[A,B]`, `[B,A]` | Different paths, common endpoint |
| Proper subset, not prefix | `[A,C]`, `[A,B,C]` | Semantic containment; another navigation edge is needed |
| Partial overlap | `[A,B]`, `[B,C]` | May share a contextual binding, not a direct trie path |
| Disjoint | `[A,B]`, `[C,D]` | Independent paths |

A prefix is immediately reusable in one trie. An unordered subset is semantic
agreement but not necessarily physical reuse. Do not eagerly close the full
subset lattice; intern only declared or actually visited binding states and
edges.

Repeated `@key` pragmas on one predicate declare access paths over one logical
relation. They must not create competing semantic stores. For example,
`@key(A) @key(A,B)` shares the `A` state, while `@key(A,B) @key(B,A)` uses two
navigation paths that converge at the full binding.

Each path is ordered, but the collection of declared paths is not: reversing
the order of two `@key` pragmas does not change the contract. Repeating the
same ordered path is an error. `[A,B]` and `[B,A]` are different paths and are
therefore both legal on one declaration.

### Contextual paths

A key may be relative to bindings inherited from an enclosing region:

```text
inherited binding: {X=x}
local key path:    [A, B]
effective path:    [X, A, B]
```

Thus a sequence that is a suffix or substring globally may be a prefix of the
remaining path inside an already-specialized region. Represent inherited and
local bindings explicitly rather than copying outer parameters into every
declaration key.

### Same and different predicates

For one predicate, multiple access paths index one canonical fact relation. For
different predicates, fact identities and contents remain distinct, though
co-recursive analysis may evaluate them in one binding environment.

Parameter spelling is not proof of agreement. A shared variable in one rule
proves an edge-local value projection, not a schema-global equivalence between
relation fields. Keep relation fields distinct and represent each rule's typed
source-to-destination projection explicitly. Promote fields to one shared
symbolic identity only when every producer of both fields proves that invariant.

### Co-recursive key flow

Differently keyed predicates in one recursive SCC are legal:

```datalog
#local p(u64 K, u64 X) @key(K).
#local q(u64 X, u64 K) @key(X).

p(K, X) : q(X, K).
q(X, K) : p(K, X).
```

Every derived head tuple is projected through the destination relation's key,
so the recursive execution graph can contain:

```text
(p, K) -> (q, X) -> (p, K)
```

If recursive edges preserve a common binding prefix, the compiler may fuse the
SCC into one binding-state-local residual fixpoint. If edges change bindings,
run a fixpoint across a graph of binding states. Different keys are not an error
and do not need to agree with a query adornment.

The recursive routing graph is not the region ownership forest and not the
request-owner relation. `RuleActivationEdge` may cycle inside one region;
`RequestEdge` retains exact external or parent-member ownership. A state is live
only when reachable from a live request root. Removing the last root must retire
an otherwise self-supporting activation cycle after its routed removals drain.

A read whose key is not bound remains semantically valid. Its
`AccessRequirement` must say that complete relation membership is required. A
planner may activate the empty or a shallower specialization, enumerate a
complete outer domain, scan canonical relation facts, use another complete
arrangement, or induce a new path. Enumerating only already-materialized states
is valid only for an explicitly named `ActiveSubset` operation; it is not a
correct implementation of an ordinary relational read.

## Current executable architecture

The current branch implements a narrower and incorrectly coupled system:

```text
parse @key
    -> vector<vector<unsigned>> parameter positions
    -> retain each path in written order
    -> duplicate/redeclaration equality sorts copies and treats paths as sets

Query::Build / ApplyDemandTransform
    -> any @key force-activates demand transformation
    -> collect bound query adornments
    -> require the keyed declaration to be the one demanded relation
    -> require declared key sets == inferred adornment sets
    -> fabricate demand__... messages and guard joins
    -> record GuardAnnotation and RecognizedSubgraph

global Query optimization
    -> may replace the recorded views

FrozenRegionalProgram::Build
    -> derive census and render-ready strings from the final Query
    -> retain the Query itself
    -> always one region and zero child calls

row contracts
    -> name visible fields and SemanticMemberKey after optimization
    -> remain a Query-owned side table keyed by live view identity

Program::Build
    -> unwrap the Query
    -> explicit -demand-instance rejects unsupported recursion
    -> implicit @key silently falls back to flat demand lowering

Rel lowering
    -> rediscover recognized guards from the optimized graph
    -> mint one DRInstance per forcing

generated runtime
    -> for each touched complete key, scan the entire global input table
    -> filter rows by key
    -> rebuild one residual Row table
    -> compare frozen/current snapshots and publish deltas
```

Important anchors at the reviewed tip:

- `lib/DataFlow/Demand.cpp:401-430`: `@key` force-activates demand.
- `lib/DataFlow/Demand.cpp:473-484`: a key without a bound query rejects.
- `lib/DataFlow/Demand.cpp:868-880`: every keyed declaration must be the
  demanded target.
- `lib/DataFlow/Demand.cpp:892-959`: key/adornment set-of-sets bijection.
- `lib/ControlFlow/Build/Build.cpp:1439-1548`: strict flag reject versus silent
  pragma fallback.
- `include/drlojekyll/DataFlow/Query.h:1007-1058`: `GuardAnnotation` and
  `RecognizedSubgraph` side records.
- `lib/Regional/Planning.cpp:201-285`: regional planning depends on those old
  records to name and classify demanded interiors.
- `lib/ControlFlow/Build/Build.cpp:1331-1338`: `Program::Build` unwraps the
  `Query` from `FrozenRegionalProgram`.
- `lib/Rel/Rel.cpp:936-1059`: post-optimization shape recovery.
- `lib/Rel/Rel.cpp:1137-1158`: the plan claims `kSectionWalk`.
- `lib/CodeGen/CPlusPlus/Database.cpp:2434-2493`: code generation performs a
  full table scan and key filter.
- `include/drlojekyll/Runtime/InstanceStore.h:54-218`: flat complete-key map,
  append-only instance ids, and two row tables per instance.

## Prioritized findings

### Critical: `@key` is owned by the wrong compiler phase

`@key` currently acts as an explicit query-demand adornment. This rejects
valid relation-local uses and makes an internal storage declaration fabricate
query-demand relations.

### Critical: semantic binding identity and access-path equality are collapsed

The parser retains written order, but duplicate detection, cross-redeclaration
equality, and demand validation sort path copies and therefore treat `[A,B]`
and `[B,A]` as the same declaration. Runtime keys come from query adornments
rather than declared path order and store only complete-key instances, with no
prefix states. The system carries the source spelling but has neither the
correct path contract nor a partial-binding model.

### Critical: the regional layer is nominal

`FrozenRegionalProgram` is a census/rendering shell around `Query`, not the
semantic owner of regions, local relations, recursion, or instance routing.
Rel still recovers shapes from the old demand mutation.

### Critical: the IR claims a physical access path that does not exist

Rel labels the instance input access `SectionWalk`; generated code scans every
row. This must become an honest `FullScanFilter` until a real keyed access path
is selected and emitted.

### High: `InstanceStore` is a leaf row cache, not a residual database

It owns one double-buffered output row set per complete key. Recursive SCCs,
auxiliary local relations, request routing, and prefix sharing remain global or
absent. Extending this class alone into a trie would optimize the wrong owner.

### High: exact fact, derivation, and request identities do not reach runtime

`FieldId`, `SemanticMemberKey`, `DeltaSign`, and support-count domains exist in
the DataFlow layer, but keyed lowering reduces facts, derivations, requests,
forcing indexes, and runtime store handles to unrelated side records and raw
integers. Convergent access paths therefore have no shared member/support
authority, and one requester's removal cannot be represented independently of
another's by `InstanceStore` alone.

### High: co-recursive keyed relations are structurally unsupported

The demand body walker expects one demanded relation and narrow
key-preserving recursion. Mutual recursion and key-changing recursive flow
reject or fall back before regional execution can represent them.

### High: redeclaration consistency has a gap

The parser compares a new key only with the immediately previous
redeclaration. `@key(A)`, then an unkeyed redeclaration, then `@key(B)` can
bypass the comparison; `InstanceKeys()` later returns the first nonempty key.
Compare every key-bearing redeclaration against one canonical declaration
contract.

### Medium: typed identity remains incomplete across boundaries

Parameter positions, forcing indexes, store ids, key columns, and row columns
remain interchangeable integers. `FieldId`, `SemanticMemberKey`, `DeltaSign`,
support domains, `RegionId`, `PortId`, and `EdgeId` exist, but regional records
still store raw indexes and render-ready strings. Carry the existing semantic
types into the authoritative regional program and add the missing binding,
path, fact, request, and activation identities there rather than wrapping the
old side tables further.

### Medium: comments describe incompatible architectures

Production comments still say the demand transform runs only under `-demand`,
that the code is a single-adornment slice, that no `-demand-instance` flag
exists, and that the runtime performs a section walk. The keyed core also
contains extensive session/plan provenance instead of present invariants.
Rewrite comments as each authority is replaced; do not retain the historical
ledger in production code.

`docs/proposals/RegionalDataFlowCore.md:753` says regional demand has no source
annotation. Retain its separation of exact request ownership from derivation
support, but supersede that sentence: explicit `@key` is a source-level logical
specialization-path contract even when additional paths are inferred. The
annotation does not itself create a request or choose a physical layout.

## Target semantic representation

Use typed records along these lines; exact names may change after grounding:

```text
RegionTemplate {
    RegionId
    inherited_symbolic_fields
    relation_schemas
    rules
    recursive_components
    request_ports
    result_ports
}

RelationSchema {
    RelationId
    fields
    member_key: SemanticMemberKeySchema
    declared_access_paths: DeclaredAccessPathSet
}

BoundFieldValue {
    field: SymbolicFieldId
    value: TypedValue
}

RegionOwnerInstanceId =
    ProgramRootInstance(ProgramRootInstanceId)
  | ParentRegionInstance(RegionInstanceId)

RegionInstanceId {
    region: RegionId
    lexical_owner: RegionOwnerInstanceId
    inherited_bindings: sorted [BoundFieldValue]
}

DeclaredAccessPath {
    KeyPathId
    relation
    ordered_fields: [SymbolicFieldId]
}

DeclaredAccessPathSet {
    unordered set, unique by (relation, ordered_fields)
    KeyPathId assigned deterministically after validation
}

SemanticMemberIdentity {
    schema: SemanticMemberKeySchema
    values: typed tuple in schema order
}

RegionalFactId {
    region_instance: RegionInstanceId
    relation
    member: SemanticMemberIdentity
}

RegionalFact {
    id: RegionalFactId
    visible_values
}

FactDerivation {
    DerivationId
    source_binding_state
    fact: RegionalFactId
    support: DerivationSupportCount
}

RegionalFactRelation {
    RegionalFactId -> RegionalFact for distinct facts derived from live
    FactDerivations
}

BindingStateSchema {
    BindingStateSchemaId
    region
    canonical_bound_fields: SymbolicFieldSet
}

BindingStateId {
    region_instance: RegionInstanceId
    schema
    canonical_bindings: sorted [BoundFieldValue]
}

BindingState {
    id: BindingStateId
    local_fixpoint_frontiers
    fact_derivation_ids
}

BindingEdge {
    parent_state
    added_field
    child_state
}

RequestOwnerId =
    RootLease(RootLeaseId)
  | PermanentRoot(PermanentRootId)
  | RegionalMember(RegionalFactId)

RequestEdgeId {
    owner: RequestOwnerId
    call_site: CallSiteId
    destination_state: BindingStateId
}

RuleActivationEdgeId {
    source_state
    source_fact: RegionalFactId
    rule: RuleId
    destination_state
}

ActiveBindingStateRelation {
    binding states reachable from live RequestEdgeIds through
    RuleActivationEdgeIds whose source state is reachable
}

RoutedResultId {
    request_edge: RequestEdgeId
    fact: RegionalFactId
}

RuleRoutingProjection {
    source_relation
    destination_relation
    typed_field_mapping
}

AccessRequirement {
    relation
    available_bindings: ordered [BoundFieldValue]
    required_fields: SymbolicFieldSet
    completeness: CompleteRelation | ActiveSubset
}

AccessPlan =
    FullScanFilter
  | FullKeyHashLookup
  | ExistingTriePrefix
  | EnumeratePrefix
  | BuildLazyOrdering
```

`RegionalFactRelation` is the only logical fact authority. `RegionInstanceId`
preserves lexical parent scope; two parent instances cannot collapse facts just
because their visible values match. Binding states own evaluation frontiers and
derivation contributions; access paths and physical arrangements refer to
canonical facts. Two paths reaching one binding map in one region instance
share one `BindingStateId`. Two different binding maps may both support one
`RegionalFactId`, but `RegionalFactRelation` still contains one semantic member.

`RequestEdgeId` is the exact ownership authority. `RuleActivationEdgeId` is a
derivation dependency and may participate in a cycle; it does not keep that
cycle alive without rooted request reachability. A dense runtime id is only a
lowered handle for one of these typed identities. Both endpoints of a
`RuleActivationEdgeId` belong to the same lexical `RegionInstanceId`;
cross-region parent/child activation uses an exact `RequestEdgeId` along the
acyclic region ownership forest.

## Evaluation and lifecycle contract

Define each epoch by its relational result, not queue order:

```text
EvaluateEpoch(input_deltas, request_deltas):
    old_outputs = SnapshotCommittedOutputs()
    apply input_deltas
    apply exact RequestEdge additions/removals

    repeat to a joint least fixpoint:
        live_states = RootedReachability(RequestEdges, RuleActivationEdges)
        evaluate fact deltas in live_states with semi-naive regional rules
        maintain FactDerivations and RegionalFactRelation
        derive/retract RuleActivationEdges from source facts
        route canonical result facts through exact RequestEdges

    retract routed results of unreachable states
    retire unreachable binding-state SCCs
    publish Difference(old_outputs, CurrentCommittedOutputs())
    seal epoch
```

On deletion, an implementation may use differential maintenance, DRed, or
affected-SCC recomputation, but it must compute the same least fixpoint as a
fresh evaluation from committed inputs and live request roots. Incoming counts
are permitted caches; they are not the reachability or membership oracle.

## Roadmap

### Phase 0: lock the language contract

1. Replace `InstanceKeySet` with an ordered `DeclaredAccessPath`; represent a
   declaration's paths as an unordered unique collection.
2. Reject `@key` on `#query` with a direct parser witness.
3. Permit keys without queries and on internally used relations.
4. Reject an exact duplicate path but accept `[A,B]` and `[B,A]` as distinct.
5. Define repeated paths over one canonical relation fact authority.
6. Define prefix and same-binding/different-order behavior.
7. Fix redeclaration checking against one canonical declaration contract across
   the complete redeclaration context. Pragma order is semantically irrelevant;
   order inside each path is significant.

Do not retain tests whose only purpose is to preserve key/adornment coupling.

### Phase 1: delete the old demand authority

Remove `ApplyDemandTransform`, fabricated demand declarations,
`QueryDemandForcing`, `GuardAnnotation`, `RecognizedSubgraph`, `DRInstance`,
post-optimization demand-shape recovery, special `InstanceStore` lowering, and
the `-demand`, `-demand-retract`, and `-demand-instance` selectors. Delete the
keyed-declaration scan, `pragma_activated`, key/query diagnostics, and
pragma-selected flat/nested fallback with them.

After this phase, `@key` is inert metadata until the regional specialization
phases consume it, and bound queries read the canonical fully materialized
relations. That is one honest semantic baseline, not a compatibility mode. No
consumer may infer key semantics from demand messages, guard annotations, or
query adornments.

### Phase 2: make `FrozenRegionalProgram` authoritative

Replace render-ready strings and the `Query` pass-through with typed semantic
regions, relation schemas, existing `RowContract` member identities, rule
routing projections, recursive components, and access requirements. Formatting
must derive from typed records. Program, Rel, and ControlFlow consume only this
frozen representation.

### Phase 3: add exact request and activation relations

Introduce `RequestEdgeId`, `RuleActivationEdgeId`, `FactDerivation`, and
caller-qualified routed results over the full-materialization backend.
At this phase, each live region instance has one empty-binding state, and rule
activation is restricted to the nonrecursive/acyclic slice. Phase 4 adds keyed
states; Phase 6 lifts activation routing to cyclic SCCs.

Required properties:

- every root lease, permanent observation, and parent member has an exact
  request owner;
- adding a second requester attaches existing results without duplicating
  semantic facts;
- removing one requester retracts only its routed results;
- derivation support is distinct from request ownership; and
- dense runtime handles never become semantic identity.

### Phase 4: implement honest complete-path specialization

Start with a deliberately simple backend:

```text
request relation R through declared path P
    -> build canonical BindingStateId from P's field/value bindings
    -> evaluate the state with FullScanFilter
    -> add FactDerivations pointing at canonical RegionalFactIds
    -> route distinct canonical facts through exact RequestEdges
```

Support a nonrecursive relation-local slice first. All declared paths are legal,
but the initial physical realization may be `FullScanFilter`. Store one
canonical fact relation regardless of access path count. Use
`FullKeyHashLookup` only when code generation emits one.

### Phase 5: add the partial-binding DAG

Intern binding states by lexical region instance, canonical field set, and
typed values. Create ordered edges only for declared or visited paths.

Required properties:

- `@key(A)` reuses the `A` prefix of `@key(A,B)`;
- `[A,B]` and `[B,A]` converge on one `{A,B}` residual state;
- subset/non-prefix paths create only the alternate edges evaluation visits;
- unvisited subsets do not materialize; and
- multiple physical indexes never duplicate semantic facts.

An ordinary unbound read must activate or access a complete specialization; it
must not silently enumerate only already-materialized states.

### Phase 6: add recursive regional execution

1. Compute predicate SCCs independently of queries.
2. Build typed edge-local field projections. Promote a schema-level symbolic
   identity only when every producer proves the same mapping.
3. Fuse components whose recursive flow preserves a common binding prefix.
4. Represent key-changing recursion as `RuleActivationEdge` dependencies
   between binding states.
5. Run the joint rooted-reachability and semi-naive fact worklist to a least
   fixpoint.
6. On deletion, retire unreachable activation SCCs only after exact routed
   removals drain. Do not use reference counts as the liveness oracle.

A binding state owns all fixpoint frontier and derivation state required by its
region, not a private copy of logical relation membership.

### Phase 7: separate and implement physical access planning

Lower `AccessRequirement` to an explicit access plan. Add real hash and trie
paths only after code generation can honor them. Structural tests must prove a
trie plan does not execute the old whole-table rescan.

### Phase 8: lazy tries and induced orderings

Compile declared paths into lazy navigation structures with shared prefixes
and convergent semantic endpoints. Let rule/join traversal induce additional
orders.

For example:

```datalog
p(A, B) @key(A, B).
q(B, C) @key(B, C).
r(A, C) : p(A, B), q(B, C).
```

Starting from `A` induces `[A,B,C]`. Starting from another binding may induce
another order and therefore another lazy index. This is the layer where Free
Join variable ordering and COLT-like on-demand construction belong.

### Phase 9: infer paths automatically

Infer useful access sequences from rules, joins, and active bindings. A query
is only one possible source of initial bindings. Preserve explicit `@key` as a
guaranteed logical specialization path while permitting inferred paths in
addition. A declared path does not promise a particular physical arrangement.

## Test migration and acceptance criteria

Cases currently expected to reject that should become ordinary successful
programs include:

- `reject_key_no_bound_query_1`;
- `key_undemanded_1`;
- `key_mismatch_1`;
- `key_over_adorn_1`;
- `key_multi_adorn_1`; and
- `key_fenced_1` and the key-triggered reject behavior in
  `key_multi_adorn_allfree_1`.

Keep the parser-shape obligations in `key_unknown_1`, `key_anon_1`,
`key_wildcard_1`, `key_dup_1`, `reject_key_empty_1`,
`reject_key_eof_1`, `reject_key_literal_1`,
`reject_key_on_message_1`, `reject_key_trailing_comma_1`, and
`reject_key_unclosed_1`. Keep exact duplicate-path rejection in
`reject_key_double_1`. Keep redeclaration consistency, but extend
`reject_key_redecl_1` with the currently missed keyed/unkeyed/keyed sequence.

Reuse the datasets from `key_tc_witness`, `key_neighborhood_witness`, and
`key_multi_adorn_witness`, but delete their role as byte-equivalence or
pragma-activation witnesses. They should test logical answers, exact request
ownership, specialization, and canonical fact identity instead.
Reuse `key_fenced_1` as a negation/completeness witness rather than a demand-
body feature-gap reject.

Add or repurpose tests for:

1. `@key` on any `#query` rejects directly.
2. `@key` alone fabricates no `demand__...` declarations.
3. A keyed local with no query compiles and evaluates normally.
4. A keyed relation reached only through another local evaluates normally.
5. Query bindings need not equal relation keys.
6. Exact duplicate paths reject, while `[A,B]` and `[B,A]` are both accepted.
7. Reordering separate `@key` pragmas does not change the declaration contract.
8. `@key(A)` and `@key(A,B)` share one `{A}` `BindingStateId`.
9. `[A,B]` and `[B,A]` share their final `BindingStateId`.
10. Same-key co-recursive predicates fuse into a binding-state-local fixpoint.
11. Differently keyed co-recursive predicates reach the correct global
   fixpoint.
12. Removing the sole root of a cyclic activation SCC drains that SCC; internal
    edges do not keep it live.
13. An unbound read before any keyed state is materialized returns the complete
    relational answer.
14. Two convergent paths do not duplicate facts or derivations.
15. Retracting one path's derivation preserves a fact supported by another.
16. Unvisited prefixes remain unmaterialized.
17. A selected trie access performs no whole-table scan.
18. Removing one requester does not destroy a shared child still owned by
    another requester.
19. A late requester receives already-derived canonical results, and its
    removal retracts only its routed copies.

Keep answer/oracle witnesses where useful, but delete flat-versus-nested
equivalence tests, demand-mode goldens, fabricated-message registries, and
recognizer census tests as architectural requirements after cutover. Tests are
evidence of intended semantics, not a reason to preserve the old split.

## Immediate resumption checklist

1. Read the repository instructions and inspect the current branch/worktree.
2. Re-derive the cited anchors from current code; do not trust old session
   prose as authority.
3. Read `docs/proposals/RegionalDataFlowCore.md` through `INDEX.md`'s
   supersession matrix. Retain its member identity, exact request ownership,
   caller-qualified result, effect, and epoch invariants; supersede its source-
   annotation and no-cyclic-binding-routing claims as stated there.
4. Inspect the key-related OptDiff cases and classify each as a lasting
   language invariant, an implementation witness, or an obsolete
   key/adornment-coupling test.
5. Produce an exact Phase 0/Phase 1 diff plan before editing production code.
6. Implement a coherent cut rather than adding another flag, nullable
   collaborator, compatibility shim, or fallback path.
7. Update comments in touched areas to state current invariants only; remove
   session labels and historical plan narration.
8. Run focused parser, DataFlow, Rel, and runtime validators, then the relevant
   OptDiff cases. Do not bless failures into acceptance.

## Verification state when this prompt was revised

The preceding code-grounding review independently derived executable and
comment-only architectures using Clang preprocessing/token extraction. At the
earlier reviewed tip, the focused targets were rebuilt and these tests passed:

```text
DataFlowValidators
RelValidators
InstanceStore
```

This architecture rewrite re-verified the cited source paths but did not rerun
binaries because it changed documentation only. The full OptDiff suite was not
run. The worktree was clean before this documentation update.

## Avoid these false starts

- Do not make `@key` another spelling of `bound`.
- Do not require any query to justify a keyed relation.
- Do not compare keys only as unordered sets.
- Do not equate semantic binding identity with traversal order.
- Do not build one independent database per access ordering.
- Do not let a binding state become a second owner of relation facts.
- Do not let an internal activation edge become a request owner.
- Do not use reference counts to collect a cyclic activation graph.
- Do not implement an ordinary unbound read by enumerating only active states.
- Do not eagerly materialize the power set of possible bindings.
- Do not implement recursive keyed regions by extending only `InstanceStore`.
- Do not label a full scan as a section or trie walk.
- Do not let DataFlow mutation plus post-optimization recognition remain the
  authority once the regional program exists.
- Do not silently fall back to the old flat architecture when a keyed shape is
  unsupported; either implement the semantic path or reject it explicitly
  during the staged cutover.
