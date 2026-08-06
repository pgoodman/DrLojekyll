# Resume: relation-local keyed instances and lazy residual regions

Continue in the Dr. Lojekyll repository on branch `keyed-instances`. At the
time this prompt was written, the reviewed tip was `4c0deb7d`; re-check the
branch tip and worktree before acting.

This prompt is the current resumption authority for keyed-instance work. It
supersedes the earlier session-oriented charter in this file and corrects a
central category error in the current implementation and some older proposal
artifacts: `@key` is a relation-local declaration, not a spelling of a query
adornment and not an opt-in to the old demand transform.

## Objective

Build toward relation-local residual databases whose declared key variables
are symbolic constants within each residual instance. Ultimately, evaluation
should be able to materialize instance prefixes lazily, infer useful access
paths automatically, and support trie/COLT-like physical layouts and Free Join
planning without changing the logical keyed-relation semantics.

The target is not merely to replace the hash table inside `InstanceStore` with
a trie. The compiler needs a regional semantic representation that owns local
relations, recursive components, partial bindings, and routing between
instances. Ordered tries are physical access structures over that semantic
state.

Treat the codebase as greenfield. Do not preserve the current flat/nested dual
architecture, silent fallback, compatibility selectors, or tests that pin an
accidental coupling once the coherent replacement owns their responsibilities.

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

`@key` declares how an internal relation is decomposed into residual
instances. A query's `bound` attributes describe an external calling
convention. A demand optimizer may exploit a keyed relation, but a query must
not activate, define, or validate a relation's keys.

A keyed relation is valid when it:

- has no bound query;
- is used only by other rules;
- is recursive or mutually recursive;
- is not the relation reached by a query;
- is read at a point where its key is not already bound; or
- has a key that differs from every query binding pattern.

The parser already routes `@key` through `ParseLocalExport` rather than query
parsing (`lib/Parse/Parser.cpp:353`). Preserve that separation and add an
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

A semantic residual instance is identified by its region, canonical bound
field set, and canonical values. Traversal order is not part of that identity.
Different ordered indexes may converge on the same residual state.

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

For one predicate, multiple access paths index one relation state. For
different predicates, contents remain distinct, but co-recursive analysis may
place them in a shared symbolic environment when rule-variable flow proves
their key fields carry the same values.

Parameter spelling is not proof of agreement. Map fields through rule
variables, equality constraints, and typed lineage into SCC- or region-level
symbolic parameter identities.

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

If recursive edges preserve a common binding prefix, the compiler may fuse
the SCC into one instance-local residual fixpoint. If edges change keys, run a
fixpoint across a graph of residual instances. Different keys are not an error
and do not need to agree with a query adornment.

A read whose key is not bound remains semantically valid. It may enumerate
existing key instances, scan an outer trie level, use another index, or induce
a new access path. This is an access-planning decision, not a language reject.

## Current executable architecture

The current branch implements a narrower and incorrectly coupled system:

```text
parse @key
    -> vector<vector<unsigned>> parameter positions
    -> equality canonicalizes each key by sorting

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

### Critical: semantic binding identity and access order are collapsed

Parser and demand validation sort key fields, erasing source order. Runtime
then stores only complete-key instances, with no prefix states. The system has
neither a correct ordered path model nor a partial-binding model.

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

### Medium: typed identity is incomplete

Parameter positions, forcing indexes, store ids, key columns, and row columns
remain interchangeable integers. `RegionId`, `PortId`, and `EdgeId` exist but
regional records still store raw indexes. Introduce domain types at the new
regional boundary rather than wrapping the old side tables further.

### Medium: comments describe incompatible architectures

Production comments still say the demand transform runs only under `-demand`,
that the code is a single-adornment slice, that no `-demand-instance` flag
exists, and that the runtime performs a section walk. The keyed core also
contains extensive session/plan provenance instead of present invariants.
Rewrite comments as each authority is replaced; do not retain the historical
ledger in production code.

`docs/proposals/RegionalDataFlowCore.md:753` says regional demand has no source
annotation. That conflicts with the clarified language design: explicit
`@key` remains a source-level relation contract even when additional paths are
inferred automatically.

## Target semantic representation

Use typed records along these lines; exact names may change after grounding:

```text
RegionTemplate {
    RegionId
    inherited_symbolic_fields
    local_relations
    rules
    recursive_components
    request_ports
    result_ports
}

RelationSchema {
    RelationId
    fields
    declared_key_paths
}

KeyPathSchema {
    KeyPathId
    relation
    ordered_fields: [SymbolicFieldId]
}

BindingStateSchema {
    BindingStateSchemaId
    region
    canonical_bound_fields: SymbolicFieldSet
}

BindingState {
    schema
    canonical_values
    local_relation_state
    local_fixpoint_state
}

BindingEdge {
    parent_state
    added_field
    child_state
}

RuleRoutingEdge {
    source_region
    destination_relation
    destination_key_projection
}

AccessRequirement {
    relation
    bound_fields
    required_fields
}

AccessPlan =
    FullScanFilter
  | FullKeyHashLookup
  | ExistingTriePrefix
  | EnumeratePrefix
  | BuildLazyOrdering
```

Semantic state is canonical by bound field set and values. Access paths and
physical indexes refer to it. Do not duplicate relation truth merely because
two orders reach it.

## Roadmap

### Phase 0: lock the language contract

1. Preserve key order in parse data and equality.
2. Reject `@key` on `#query` with a direct parser witness.
3. Permit keys without queries and on internally used relations.
4. Define repeated keys as access paths over one logical relation.
5. Define prefix and same-binding/different-order behavior.
6. Fix redeclaration checking across the complete declaration context.

Do not retain tests whose only purpose is to preserve key/adornment coupling.

### Phase 1: decouple `@key` from demand

Delete from `ApplyDemandTransform`:

- keyed-declaration scanning;
- `pragma_activated`;
- the no-bound-query rejection;
- the demanded-target restriction;
- key/adornment bijection validation; and
- pragma-selected flat/nested fallback.

After this phase, `@key` alone fabricates no demand messages. `-demand` may
temporarily remain an independent query optimization, but it has no authority
over relation keys.

### Phase 2: make `FrozenRegionalProgram` authoritative

Replace render-ready strings and the `Query` pass-through with typed semantic
regions, relation schemas, rule routing, recursive components, and access
requirements. Formatting must derive from typed records.

Migrate consumers to this program and delete `GuardAnnotation`,
`RecognizedSubgraph`, and post-optimization live-shape recovery when their last
responsibility moves. Do not maintain the old and new authorities in parallel.

### Phase 3: implement honest complete-key routing

Start with a deliberately simple backend:

```text
derive tuple for relation R
    -> project R's declared key
    -> find/create complete-key instance
    -> insert residual row
```

Support a nonrecursive relation-local slice first. Store one logical relation
regardless of access path count. Call the physical implementation
`FullScanFilter` or `FullKeyHashLookup` according to what it actually emits.

### Phase 4: add the partial-binding DAG

Intern binding states by canonical field set and values. Create ordered edges
only for declared or visited paths.

Required properties:

- `@key(A)` reuses the `A` prefix of `@key(A,B)`;
- `[A,B]` and `[B,A]` converge on one `{A,B}` residual state;
- subset/non-prefix paths create only the alternate edges evaluation visits;
- unvisited subsets do not materialize; and
- multiple physical indexes never duplicate semantic facts.

### Phase 5: add recursive regional execution

1. Compute predicate SCCs independently of queries.
2. Derive canonical symbolic parameters through rule-variable lineage.
3. Fuse components whose recursive flow preserves a common binding prefix.
4. Represent key-changing recursion as dependencies between residual states.
5. Run a semi-naive worklist over fact deltas and newly activated instances.
6. Track ownership/reference state where several parents share a child
   specialization, especially for differential updates.

An instance must own all local relation and fixpoint state required by its
region, not only one published row set.

### Phase 6: separate and implement physical access planning

Lower `AccessRequirement` to an explicit access plan. Add real hash and trie
paths only after code generation can honor them. Structural tests must prove a
trie plan does not execute the old whole-table rescan.

### Phase 7: lazy tries and induced orderings

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

### Phase 8: infer paths automatically

Infer useful access sequences from rules, joins, and active bindings. A query
is only one possible source of initial bindings. Preserve explicit `@key` as a
guaranteed user-declared path while permitting inferred paths in addition.

After the canonical regional lowering owns demand-driven evaluation, delete
`-demand-instance` and the permanent flat-versus-nested equivalence contract.

## Test migration and acceptance criteria

Cases currently expected to reject that should become ordinary successful
programs include:

- `reject_key_no_bound_query_1`;
- `key_undemanded_1`;
- key/query mismatch;
- key over-adornment; and
- partial key/adornment declaration.

Add or repurpose tests for:

1. `@key` on a bound `#query` rejects directly.
2. `@key` alone fabricates no `demand__...` declarations.
3. A keyed local with no query compiles and evaluates normally.
4. A keyed relation reached only through another local evaluates normally.
5. Query bindings need not equal relation keys.
6. `@key(A)` and `@key(A,B)` share one runtime `A` state.
7. `[A,B]` and `[B,A]` share their final residual state.
8. Same-key co-recursive predicates fuse into an instance-local fixpoint.
9. Differently keyed co-recursive predicates reach the correct global
   fixpoint.
10. Key-changing recursion creates and drains the expected instance graph.
11. An unbound read of a keyed relation remains semantically valid.
12. Two convergent paths do not duplicate facts or derivations.
13. Unvisited prefixes remain unmaterialized.
14. A selected trie access performs no whole-table scan.
15. Removing one requester does not destroy a shared child still owned by
    another requester.

Keep answer/oracle witnesses where useful, but delete flat-versus-nested
equivalence tests as architectural requirements after cutover. Tests are
evidence of intended semantics, not a reason to preserve the old split.

## Immediate resumption checklist

1. Read the repository instructions and inspect the current branch/worktree.
2. Re-derive the cited anchors from current code; do not trust old session
   prose as authority.
3. Read `docs/proposals/RegionalDataFlowCore.md`, but treat its deletion of the
   source annotation as superseded by this prompt.
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

## Verification state when this prompt was written

The review independently derived executable and comment-only architectures
using Clang preprocessing/token extraction. The focused targets were rebuilt,
and these tests passed:

```text
DataFlowValidators
RelValidators
InstanceStore
```

The full OptDiff suite was not run because the review made no production
changes. The worktree was clean before this documentation update.

## Avoid these false starts

- Do not make `@key` another spelling of `bound`.
- Do not require any query to justify a keyed relation.
- Do not compare keys only as unordered sets.
- Do not equate semantic binding identity with traversal order.
- Do not build one independent database per access ordering.
- Do not eagerly materialize the power set of possible bindings.
- Do not implement recursive keyed regions by extending only `InstanceStore`.
- Do not label a full scan as a section or trie walk.
- Do not let DataFlow mutation plus post-optimization recognition remain the
  authority once the regional program exists.
- Do not silently fall back to the old flat architecture when a keyed shape is
  unsupported; either implement the semantic path or reject it explicitly
  during the staged cutover.
