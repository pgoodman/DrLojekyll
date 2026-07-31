# Bounded Consumption and Exact-One Evaluation

> **Status.** Architecture proposal, reviewed against the executable Query,
> Rel, ControlFlow, code-generation, and runtime paths on 2026-07-31. The
> proposal is intentionally greenfield: it introduces a typed vocabulary for
> consumer intent, one logical authority for each meaning, and one physical
> authority for choosing how that meaning executes. It does not preserve a
> full-materialization path beside a bounded path merely for compatibility.

## 0. Decision

Dr. Lojekyll needs a first-class logical `ONLY` operator whose result for each
group is the sole distinct input row when the group contains exactly one row,
and no row otherwise.

`ONLY` is not `FIRST`, `LIMIT 1`, an estimated cardinality, or a generic
integer tuning knob. Its exact state space is:

```text
Empty
One(row)
Many
```

Within one stable observation, the second distinct row settles the result as
`Many`, so evaluation stops immediately. Across epochs, however, `Many` is
permanent only for insert-only input. A differential group can move from
`Many` back to `One(row)` after removals. The original motivating sets are
sets of *uncommitted* candidates, so this differential case is central rather
than exceptional.

The architectural ownership is:

```text
Query IR:
  owns internal ChooseOne and ONLY relational meaning
  owns external read intent at the query boundary
  keeps group, payload, policy, objective, ties, and distinct identity typed
  derives internal demand from ONLY output

Rel IR:
  owns one physical decision for every applicable consumption work scope
  proves access shape, lifecycle, effects, and early-termination scope

ControlFlow:
  mechanically lowers the selected Rel plan
  does not rediscover boundedness or choose an alternative plan

Runtime:
  provides typed selection/exact-one state and bounded probe primitives
  does not infer semantics from cursor behavior, functor names, or raw limits

Cost analysis:
  reports actual row touches versus the semantically permitted bound
  never authorizes a rewrite
```

This adds one semantic quantity to the existing cardinality/work split:

```text
Cardinality:         how many rows exist
Decision state:      Empty | One(row) | Many
Physical work:       how many rows the chosen plan touches
```

The number two is derived from `ONLY`'s state transition. It is not stored as
a configurable `limit`, `threshold`, or bare integer.

### 0.1 Broader goal: preserve how much a consumer needs

`ONLY` is the first complete implementation slice, not the only bounded
consumption shape. Algorithms need to distinguish these intents:

| Intent | Result | Decisive event |
|---|---|---|
| `CHOOSE_ONE<Row, Policy>` | One qualifying row selected by a policy | Policy-dependent |
| `EXISTS` | Whether a qualifying row exists | First witness accepts; exhaustion rejects |
| `ONLY<Row>` | The row iff exactly one distinct row exists | Second witness rejects; exhaustion with one accepts |
| `COMPLETE<Row>` | Every qualifying row | Exhaustion |

These are semantic variants, not spellings of `LIMIT 1`:

- `ChooseOne(Arbitrary)` deliberately permits any qualifying row and accepts
  the first witness produced by its physical plan.
- `ChooseOne(ByObjective(...))` promises an optimum under an objective and
  tie policy. It accepts the first physical witness only when Rel proves that
  the access path is ordered by that objective.
- `ONLY` cannot accept the first row because success requires exhaustion.
- `EXISTS` does not carry a row payload.
- “The producer has at most one row” is a producer cardinality fact, not a
  consumer request to use one row.

The compiler should derive a typed, use-scoped summary from the live Query
graph:

```text
ConsumptionIntent =
  ChooseOne(GroupKey, PayloadProjection, SelectionPolicy)
  | Exists(QualifyingProjection)
  | Only(GroupKey, PayloadProjection, DistinctKey)
  | Complete

SelectionPolicy =
  Arbitrary
  | ByObjective(ObjectiveExpression, Direction, TiePolicy)
```

This summary is analysis, not a second semantic graph. `QueryOnly` remains the
authority for exact-one relational meaning; `QueryChooseOne` remains the
authority for selection meaning; an external query read is one possible
consumer boundary rather than the only place these operators can occur. Rel
consumes the derived summary and must make four answers visible in its dumps
and cost report:

```text
What did the consumer ask for?
Which physical scope became bounded: read, producer, or downstream demand?
Which structural proof made that legal?
If work stayed complete, which typed blocker required it?
```

This is the general architecture into which `@first` and `@only` fit.

### 0.2 Surface placement does not define IR placement

The initial surface can support `@first` and `@only` on `#query` declarations,
but the semantic operators must also compose inside a rule graph:

```text
surface @first over candidates grouped by g
  -> QueryChooseOne(group_key=g, policy=Arbitrary)

surface @first(objective(candidate)) over candidates grouped by g
  -> QueryChooseOne(
       group_key=g,
       policy=ByObjective(
         expression=objective(candidate),
         direction=declared direction,
         ties=declared tie policy))

surface @only over candidates grouped by g
  -> QueryOnly(group_key=g, distinct_key=candidate identity)
```

The final grammar should make objective direction and ties explicit; it must
not infer them from a functor name. An objective functor must be pure and its
result must have a named ordered domain. A floating-point score is not
interchangeable with probability, rank, weight, likelihood, or confidence.

An unparameterized `@first` and an objective-bearing `@first` have the same
maximum output cardinality but different work laws:

```text
ChooseOne(Arbitrary):
  may accept the first qualifying witness

ChooseOne(ByObjective):
  must inspect every candidate
  unless an ordered index, monotone bound, or equivalent structural proof
  establishes that no unseen candidate can win
```

This is why `returns_at_most_one` is insufficient as an optimization fact.

## 1. Executable architecture today

### 1.1 Pipeline map

```text
source boundary [top-down parsing]
  source text
    -> ParsedModule / ParsedClause / ParsedAggregate / ParsedQuery

logical compilation [bottom-up construction + convergent rewrites]
  ParsedModule
    -> Query graph
       SELECT / TUPLE / JOIN / MAP / AGG / MERGE / CMP / NEGATE / INSERT
    -> demand transform for supported bound-query shapes
    -> CSE / canonicalization / dead-flow elimination
    -> linked and stratified Query

physical compilation [hybrid discovery + scheduling + lowering]
  Query
    -> DataTable / DataIndex ownership
    -> DRFlowGraph inventory and access-plan spines
       ACCESS / GATE / FOLD
    -> checked schedule
    -> ControlFlow region trees

output boundary [structural code generation]
  ControlFlow + ProgramQuery descriptors
    -> generated C++ procedures, tables, indexes, and query cursors

runtime [incremental, stateful, fixpoint]
  message delta
    -> table folds and frontiers
    -> eager and recursive propagation
    -> aggregate state cells
    -> commit
  host query
    -> scan already-materialized table or index
```

### 1.2 What each layer cannot currently say

| Layer | Current useful fact | Missing fact |
|---|---|---|
| Parse | Two overloaded `@first` spellings, functor ranges, aggregating functor shape | A semantic selection/exact-one operator |
| Query | Complete relational dependencies and grouping columns | Typed `ChooseOne` / `Only` consumer intent and decisive event |
| Rel | Point test, section walk, full scan, effects, schedule | A selected bounded plan or a typed reason work stays complete |
| ControlFlow | Loops, gates, folds, procedure return | Structured termination of an observer-owned loop nest |
| Runtime | Complete tables/indexes and aggregate cells | Typed selection/exact-one results and bounded probes |

The absence is structural:

- `QueryView` has no exact-one node.
- Rel's `PlanKind` is only `kAccess`, `kGate`, and `kFold`.
- `ProgramOperation` has loops and procedure returns, but no observer-owned
  early exit.
- Query entry points are emitted directly from `ProgramQuery`; they do not
  pass through a Rel query-read plan.
- `@recompute` aggregate state stores the whole live membership multiset and
  rescans it for reduction.

### 1.3 Current bad behavior can be demonstrated

There is no committed `@only` case yet, so the proof must distinguish a direct
exact-one witness from adjacent evidence.

Direct whole-world evidence:

1. A count or uniqueness computation expressed through a generic aggregate
   becomes `QueryAggregate`.
2. An undeclared aggregate algebra becomes `@recompute`.
3. `Recompute::Fold` linearly searches and retains every distinct summarized
   value.
4. `Recompute::Emit` passes the complete values/counts vectors to the driver
   reducer.
5. A downstream comparison with one cannot change this because the aggregate
   functor's meaning is opaque and no bounded observer exists in Query.

Adjacent bounded-fact loss:

1. Parse exposes `ParsedQuery::ReturnsAtMostOneResult()` for `@first`.
2. The property is used to reformat source.
3. It is not converted into a Query, Rel, or ControlFlow fact.
4. Generated free-result queries always expose a cursor whose `next` follows
   the backing index/table until exhaustion.

`FIRST` and `ONLY` have different semantics. This is not a proposal to lower
`ONLY` as `FIRST`; the `@first` path is evidence that an explicitly bounded
consumer property can currently remain trapped at the parse/presentation
boundary.

### 1.4 `@first` representation census

Dr. Lojekyll currently gives the same token two unrelated parse meanings:

```text
#query q(...) @first
  = a declared return-stream limiter
  = ParsedQueryImpl.first_attribute
  = exposed as ParsedQuery::ReturnsAtMostOneResult()

#query q(...) :- @first message(...), ...
  = send a forcing message before evaluating the query body
  = ParsedClauseImpl.forcing_predicates
  = lowered to ProgramQuery.forcing_function
```

The second form is sequencing/demand injection, not row selection. It should
not be treated as evidence for `ChooseOne`. Reusing the spelling is a surface
category collision that the semantic IR must not preserve.

For declaration-level `@first`, the executable path is:

```text
Parse:
  first_attribute                       PRESENT
  ReturnsAtMostOneResult()              PRESENT

Formatting:
  prints @first                         PRESENT

DataFlow Query:
  QueryChooseOne / consumption intent   ABSENT

Rel:
  selected first-row realization        ABSENT

ControlFlow:
  observer-owned early exit             ABSENT

ProgramQuery:
  retains ParsedQuery wholesale         INDIRECTLY AVAILABLE, NOT TYPED

C++ query codegen:
  emits an ordinary cursor whose next() advances until exhaustion
```

Therefore `@first` is not explicitly represented in any optimization IR
today. Its syntax survives transitively in `ProgramQuery`, but no pass can ask
whether the request bounded only the host read, the production of the backing
relation, or a demanded subgraph. The generated cursor does not enforce the
declared stream limit.

The replacement path is:

```text
declaration @first
  -> QueryReadAnyOne
  -> Rel ConsumptionRealization
       read = BoundedRead(FirstLiveRowProbe)
       producer = BoundedProducer(FusedChooseOne)
                  | CompleteProducer(CompleteProducerPlan,
                                     BoundedPlanBlocker)
       downstream = KeyedDemand(KeyedChooseOne)
                    | CompleteDownstream(TypedBlocker)
                    | NoDownstreamScope
  -> ProgramQueryRead tagged plan
  -> single-result codegen API returning Empty | One(row)
```

The bounded read is always legal: its own consumer asked for no more than one
row. Bounded *production* is separate. If the backing relation is shared,
standing, recursive, or effectful, Rel may retain complete production while
still stopping the query read after its first live qualifying row. Dumps must
show both scopes rather than reporting the result cap as though all upstream
work were capped.

### 1.5 Reproducible current-state proof

`data/examples/force.dr` contains both declaration `@first` and query-body
forcing `@first` on `get_next_id`. The current debug compiler was run as:

```sh
mkdir -p /private/tmp/drl-bounded-current/cpp
build/debug/bin/drlojekyll \
  -df-out /private/tmp/drl-bounded-current/force.df \
  -rel-out /private/tmp/drl-bounded-current/force.rel \
  -ir-out /private/tmp/drl-bounded-current/force.ir \
  -cpp-out /private/tmp/drl-bounded-current/cpp \
  data/examples/force.dr
```

Observed on 2026-07-31:

```text
force.df:   no first/choose/only/observe consumption fact
force.rel:  no first/choose/only/observe consumption fact
force.ir:   forcing receive procedure and call are present
            no return-stream consumption fact is present
datalog.h:  get_next_id_bf_cursor is emitted with bool next(...)
            next advances the backing index and remains callable to exhaustion
```

The source anchors explain the result:

| Evidence | Current anchor |
|---|---|
| Parse recognizes declaration `@first` | `lib/Parse/Parse.cpp:1136-1138` |
| Formatting is its only direct consumer | `lib/Parse/Format.cpp:113-117` |
| Rel plan vocabulary has only access/gate/fold | `lib/Rel/Rel.h:460-483` |
| `ProgramQuery` carries raw `ParsedQuery`, table, index, and forcing procedures but no typed read plan | `include/drlojekyll/ControlFlow/Program.h:1362-1397` |
| Codegen unconditionally defines the free-result cursor loop | `lib/CodeGen/CPlusPlus/Database.cpp:1726-1796` |
| Generic recompute retains all distinct summaries and passes the full vectors to reduction | `include/drlojekyll/Runtime/StateCell.h:229-259` |

This is a direct proof of declaration-level `@first` intent loss and an
adjacent proof of the whole-world substrate that an opaque exact-one aggregate
would use. It is not falsely presented as proof that an unimplemented
`@only` already lowers badly.

## 2. Motivating algorithm

The whole-world spelling is:

```text
for each committed function/source pair (F, S):
  B = all uncommitted binary references of F
  R = all uncommitted source globals of S

  for cls in {ZERO, NONZERO}:
    Bc = materialize every b in B where init(b) == cls
    Rc = materialize every r in R where init(r) == cls

    if cardinality(Bc) == 1 and cardinality(Rc) == 1:
      emit sole(Bc) -> sole(Rc)
```

The required algorithm is:

```text
function observe_only(rows, qualifies):
  state = Empty

  for row in rows:
    if not qualifies(row):
      continue

    match state:
      Empty:
        state = One(row)
      One(_):
        return Many
      Many:
        unreachable

  return state

for each committed function/source pair (F, S):
  for cls in {ZERO, NONZERO}:
    b_state = observe_only(binary_refs(F), init(_) == cls)
    if b_state is not One(b):
      continue

    r_state = observe_only(source_globals(S), init(_) == cls)
    if r_state is not One(r):
      continue

    emit b -> r
```

The per-key work is:

```text
Work(F, cls) = ScanToSecond(B(F, cls))
             + IsOne(B(F, cls)) * ScanToSecond(R(S, cls))
```

`ScanToSecond(X)` counts rows examined until the second qualifying distinct
row or exhaustion. It is not generally `min(2, |X|)`: an unindexed filter may
examine nonqualifying rows, and downstream joins may produce duplicates that
must be removed before they count as witnesses.

## 3. Quantity and type audit

| Quantity | Meaning | Required representation | Invalid collapse |
|---|---|---|---|
| `ConsumptionIntent` | What settles one use of a relation | Tagged semantic summary derived from its owning Query operator/boundary | Raw result limit shared by `FIRST`, `ONLY`, and objective selection |
| `SelectionPolicy` | How one row is chosen | `Arbitrary | ByObjective(expression, direction, ties)` | Boolean `first`, functor-name convention, or untyped comparator callback |
| `ObjectiveValue` | Domain ordered by a selection objective | Named ordered domain returned by a pure expression | Bare float mixing score, probability, weight, and confidence |
| `OnlyState<Row>` | Exact observation result | Sum type `Empty | One(Row) | Many` | Nullable row, where zero and many become indistinguishable inside the operator |
| `GroupKey` | Partition within which exact-one is tested | Named ordered column projection | Bare vector of column ids reused as an index key without a role |
| `DistinctKey` | Identity deciding whether two witnesses differ | Named row projection | Assuming payload equality is complete row equality |
| `InputChangeMode` | Snapshot, insert-only, or differential lifecycle | Enum derived from Query facts | Boolean `monotone` attached ad hoc by a lowering |
| `OnlyLowering` | Selected physical strategy | Enum chosen once by Rel | Independent flags such as `bounded`, `materialized`, `incremental` |
| `PlanDecision` | Whether and where bounded work was realized | `Selected(PhysicalPlan) | Complete(CompletePlan, TypedBlocker)` | Silent fallback to complete work |
| `MemberCount` | Exact live members of a differential group | Named signed/count domain | Saturating count reused across removals |
| `RowTouches` | Physical work | Cost-domain value | Relation cardinality treated as work |

There should be no generic `ObservationHorizon{2}` field on `QueryOnly`.
`ONLY` defines the transition `One -> Many` on the second distinct witness;
the decision bound is derived from the operator kind. A future `AT_LEAST`,
`TOP_K`, or thresholded count must introduce its own semantic variant rather
than repurpose a raw limit.

## 4. First split-brain review of the initial proposal

### Finding 1: `FIRST` and `ONLY` were conflated

`FIRST` can emit after one witness. `ONLY` cannot emit until the input is
exhausted with one witness, though it can reject after the second. Treating
both as a generic row cap erases the proof obligation that exhaustion is
required for success.

**Repair:** make `QueryOnly` an exact semantic node with `OnlyState`, not a
`LIMIT 2` annotation.

### Finding 2: logical meaning and physical early exit had two owners

The initial proposal placed a bounded property on Query edges, an observer in
Rel, a loop break in ControlFlow, and specialized query C++ generation. If
each can decide boundedness independently, the compiler acquires four
partially overlapping sources of truth.

**Repair:** Query alone owns semantics. Rel alone selects a lowering.
ControlFlow and codegen consume that selection mechanically. The query-read
surface must receive a Rel-authored physical descriptor instead of inspecting
`ParsedQuery` or rediscovering `ONLY` directly.

### Finding 3: a generic backward demand and `QueryOnly` duplicated authority

If an edge annotation can request two rows while a `QueryOnly` node also says
exact-one, optimizer rewrites can preserve one and lose the other.

**Repair:** `QueryOnly` is the semantic source. Any demand facts are derived
analysis results keyed by the node and its use edge; they are recomputed after
structural rewrites and never serialized as a second semantic graph.

### Finding 4: snapshot and standing-state lifecycles were mixed

A snapshot scan may forget all rows after observing the second. A standing
differential observer may later receive removals and return from `Many` to
`One`. Saturation is sound only within the snapshot or permanently for an
insert-only group.

**Repair:** Rel chooses one named `OnlyLowering` whose lifecycle is explicit.
No lowering contains a fallback branch to another lifecycle.

### Finding 5: bounded observation and conditional evaluation were mixed

Stopping `Bc` after two witnesses does not itself skip computation of `Rc` in
a bottom-up engine. If `Rc` is maintained eagerly elsewhere, it has already
been computed.

**Repair:** model the second optimization as internal data-dependent demand:
the `One(b)` output of B's `QueryOnly` is the demand relation for R. There is
no parallel eager R path after this rewrite.

### Finding 6: cost was at risk of becoming a correctness oracle

Cardinality estimates can choose which of two safe observers to run first,
but cannot prove an early exit correct.

**Repair:** structural recognition and lifecycle analysis produce a proof
certificate. Cost only ranks already-valid physical plans and reports missed
opportunities.

### Finding 7: the query entry surface bypasses the proposed physical owner

External query cursors are generated directly from `ProgramQuery`, outside
Rel plan spines and ControlFlow. Adding special `@only` code to the C++
generator would preserve this architectural split.

**Repair:** Rel must author a query-read descriptor for external `ONLY` just as
it authors internal access plans. `ProgramQuery` carries that descriptor to
codegen; codegen does not inspect parse attributes to choose behavior.

## 5. Logical architecture

### 5.1 Boundary observation versus relational selection

A `#query @first` read and an internal `@first` do not have the same owner:

```text
QueryReadAnyOne:
  observes one row from an otherwise unchanged relation
  permits the host read to stop
  does not by itself change standing relational truth

QueryChooseOne:
  selects one row per group as relational truth
  may add/retract a chosen row as membership or the objective changes
  participates in Query optimization, stratification, and differential state
```

Both derive `ConsumptionIntent::ChooseOne`, but the use scope records whether
the intent belongs only to a read edge or to a relational operator. This
prevents an optimizer from truncating a shared producer merely because one
host read asks for one row.

### 5.2 `QueryChooseOne`

```text
QueryChooseOne:
  input: QueryView
  group_key: GroupKey
  payload: PayloadProjection
  policy: SelectionPolicy

SelectionPolicy =
  Arbitrary
  | ByObjective(ObjectiveExpression, Direction, TiePolicy)

TiePolicy =
  AnyOptimum
  | ByStableKey(StableKeyProjection)
  | RejectTies
```

Semantics:

```text
for each group g:
  candidates = DISTINCT rows(input, g)

  match policy:
    Arbitrary:
      emit any one member, if present

    ByObjective(expr, direction, AnyOptimum):
      evaluate expr for every member
      emit any member having the optimum value

    ByObjective(expr, direction, ByStableKey(key)):
      emit the unique optimum under (expr, key)

    ByObjective(expr, direction, RejectTies):
      emit the objective optimum only when exactly one member has that value
```

An internal `Arbitrary` choice deliberately introduces plan-permitted choice:
the selected row can change when a plan changes or when the current choice is
removed. If that is not an acceptable language property, unparameterized
internal `@first` must be rejected and only boundary `#query @first` may use
`Arbitrary`. The surface decision must be explicit before implementation; the
compiler must not quietly pretend physical iteration order is logical order.

Objective evaluation and row observation are separate work quantities. A
result cardinality of one does not prove one row touch:

```text
function choose_by_objective(rows, objective):
  best = Empty

  for row in rows:                      # complete unless access is ordered
    value = objective(row)
    best = improve(best, value, row)

  return best
```

Rel may replace that loop with one ordered probe only when the index order
matches the objective, direction, and tie policy. `RejectTies` may need the
second row in the optimum-value section even when the first index row proves
the optimum value.

### 5.3 `QueryOnly`

```text
QueryOnly:
  input: QueryView
  group_key: GroupKey
  payload: PayloadProjection
  distinct_key: DistinctKey

semantics for each group g:
  members = DISTINCT project(distinct_key, rows(input, g))

  if cardinality(members) == 1:
    emit project(group_key, payload, sole(members))
  else:
    emit nothing
```

`QueryOnly` is aggregate-like and nonmonotone under insertions: the first
member adds output and the second retracts it. It therefore participates in
stratification and differential tracking as a first-class node rather than
masquerading as a TUPLE or optimizer hint.

### 5.4 Recognition

Recognition is a semantics-preserving Query rewrite, not a cost-guided
runtime peephole.

Initial authoritative source:

```text
surface @only
  -> QueryOnly
```

Recognizable whole-world exact-one form:

```text
candidates(g, x)

nonunique(g) :-
  candidates(g, x),
  candidates(g, y),
  x != y

result(g, x) :-
  candidates(g, x),
  !nonunique(g)
```

Recognizer pseudocode:

```text
function recognize_only(result_view):
  require result_view is NEGATE(candidate_side, nonunique_keys)
  require nonunique_keys is projection of a self-join
  require both self-join inputs are equivalent reads of candidate_side
  require join equality is exactly the group key
  require inequality proves distinct(candidate_identity_left,
                                      candidate_identity_right)
  require result payload is projected from candidate_side
  require no additional predicate changes either candidate set
  require no observable consumer depends on nonunique_keys itself

  return QueryOnly(
      input=candidate_side,
      group_key=proved_group_key,
      payload=proved_payload,
      distinct_key=proved_candidate_identity)
```

A `count(...) == 1` form is recognizable only when `count` is a compiler-known
semantic operator. A user-supplied aggregate called `count_i32` remains opaque;
its name and driver implementation are not proof.

### 5.5 Use-scoped demand derivation

Bounded work is use-scoped. The same candidate producer may feed an `ONLY` and
a consumer requiring the complete relation.

```text
enum CompletionNeed:
  Complete
  OnlyWitnesses(QueryOnlyId)

function derive_completion_need(use_edge):
  if user is QueryOnly and edge is its observed input:
    return OnlyWitnesses(user.id)
  return Complete

function combine_needs(all_uses):
  if any use needs Complete:
    return Complete
  if all uses refer to the same QueryOnly semantics:
    return that OnlyWitnesses
  return Complete
```

This analysis does not truncate a shared producer. Rel may still choose a
bounded probe over a complete backing table, or clone an ephemeral pure
subplan if its cost model prefers that plan and the structural proof allows
it.

### 5.6 Internal conditional demand

For the motivating B-then-R computation:

```text
BOnly(F, cls, b) = ONLY B candidates grouped by (F, cls)

DemandR(S, cls) :-
  committed_source(F, S),
  BOnly(F, cls, _)

ROnly(S, cls, r) = ONLY R candidates grouped by (S, cls),
                   instantiated only for DemandR(S, cls)

result(F, cls, b, r) :-
  committed_source(F, S),
  BOnly(F, cls, b),
  ROnly(S, cls, r)
```

`BOnly` owns truth. `DemandR` owns activation of R's subgraph instance, not
the truth of R candidates. When B moves from `One` to `Many`, the derived
demand is retracted; when later removals return B to `One`, demand is added
again. The keyed-instance lifecycle is therefore the natural physical
mechanism, but the existing query-root-only demand transform must be
generalized rather than duplicated with a special ONLY scheduler.

## 6. Physical architecture

### 6.0 Rel plans each work scope separately

“Returns one row” is not a physical plan. Rel records whether boundedness was
realized at each relevant scope:

```text
ConsumptionRealization:
  read: ReadDecision
  producer: ProducerDecision
  downstream: DownstreamDecision

ReadDecision =
  BoundedRead(ReadPlan)
  | CompleteRead
  | NoReadScope

ProducerDecision =
  BoundedProducer(ProducerPlan)
  | CompleteProducer(CompleteProducerPlan, BoundedPlanBlocker)
  | NoProducerScope

DownstreamDecision =
  KeyedDemand(DemandPlan)
  | CompleteDownstream(BoundedPlanBlocker)
  | NoDownstreamScope

BoundedPlanBlocker =
  SharedCompleteConsumer
  | MissingCompatibleIndex
  | UnorderedObjective
  | DifferentialRecoveryUnavailable
  | RecursiveOwnership
  | EffectfulPath
```

These tagged decisions are required output, not warnings. A supported
bounded intent may not silently disappear into a complete plan. `No*Scope`
means that scope does not exist for this use; it is not nullable state.

Examples:

```text
#query @first over a shared materialized table:
  read       = BoundedRead(FirstLiveRowProbe)
  producer   = CompleteProducer(StandingMaterialization,
                                SharedCompleteConsumer)
  downstream = NoDownstreamScope

internal @first(objective(...)) with no ordered index:
  read       = NoReadScope
  producer   = CompleteProducer(CompleteObjectiveChoice,
                                UnorderedObjective)
  downstream = NoDownstreamScope

BOnly controlling R activation:
  read       = NoReadScope
  producer   = BoundedProducer(DifferentialIndexState)
  downstream = KeyedDemand(ROnlyInstances)
```

### 6.1 Rel operator

Rel gains one operation referencing the semantic Query node:

```text
DROpKind::kObserveOnly

ObserveOnlyOp:
  query_only: QueryOnly
  decision: OnlyPlanDecision
```

The concrete representation should avoid nullable strategy-specific fields.
Use a tagged strategy payload:

```text
OnlyLowering =
  SnapshotSectionProbe(SnapshotProbePlan)
  | DifferentialIndexState(DifferentialOnlyPlan)
  | MonotoneFused(MonotoneOnlyPlan)

OnlyPlanDecision =
  Selected(OnlyLowering)
  | Complete(CompleteStandingOnlyPlan, BoundedPlanBlocker)
```

Each strategy payload owns all and only its required table, index, access,
lifecycle, and effect-proof fields. Rel selects exactly one decision.
Unsupported shapes retain the semantically correct `QueryOnly` with one
complete standing implementation and a typed blocker; they do not enter a
warning-and-continue half-plan. Once a specialized plan is selected, no
complete duplicate path survives.

Selection gets a parallel operator and tagged strategy family:

```text
DROpKind::kChooseOne

ChooseOneLowering =
  FirstLiveRowProbe(ArbitraryProbePlan)
  | OrderedObjectiveProbe(ObjectiveProbePlan)
  | DifferentialChoiceState(DifferentialChoicePlan)

ChooseOnePlanDecision =
  Selected(ChooseOneLowering)
  | Complete(CompleteChoiceState(CompleteChoicePlan), BoundedPlanBlocker)
```

`FirstLiveRowProbe` is valid for arbitrary boundary observation.
`OrderedObjectiveProbe` additionally proves that its access order implements
the objective, direction, and tie policy. `CompleteChoiceState` still caps the
result relation at one but advertises that candidate production/evaluation is
complete.

### 6.2 Lowering selection

```text
function plan_choose_one(query_choose, use_scope, physical_context):
  facts = derive {
    selection_policy,
    compatible_order,
    input_change_mode,
    recursive_ownership,
    use_completion_needs,
    effect_summary
  }

  if use_scope is BoundaryRead
     and selection_policy is Arbitrary:
    read = BoundedRead(FirstLiveRowProbe(...))
  else:
    read = NoReadScope

  if selection_policy is ByObjective
     and compatible_order proves expression, direction, and tie policy:
    producer = BoundedProducer(OrderedObjectiveProbe(...))

  else if selection_policy is Arbitrary
          and input_change_mode is InsertOnly
          and use_completion_needs permits exclusive bounded execution
          and effect_summary is Pure
          and recursive_ownership is Acyclic:
    producer = BoundedProducer(FusedChooseOne(...))

  else:
    producer = CompleteProducer(
        CompleteChoiceState(...),
        derive_bounded_plan_blocker(facts))

  downstream = derive_downstream_decision(query_choose, physical_context)
  return ConsumptionRealization(read, producer, downstream)

function choose_only_lowering(query_only, physical_context):
  facts = derive {
    source_table,
    group_index,
    input_change_mode,
    recursive_ownership,
    use_completion_needs,
    effect_summary,
    distinctness_location
  }

  if source_table is complete
     and group_index supports query_only.group_key
     and query_only is evaluated against a stable snapshot:
    return Selected(SnapshotSectionProbe(...))

  if input_change_mode is Differential
     and source_table is complete
     and group_index supports query_only.group_key:
    return Selected(DifferentialIndexState(...))

  if input_change_mode is InsertOnly
     and use_completion_needs permits exclusive bounded execution
     and effect_summary is Pure
     and recursive_ownership is Acyclic:
    return Selected(MonotoneFused(...))

  return Complete(
      CompleteStandingOnly(...),
      derive_bounded_plan_blocker(facts))
```

`CompleteStandingOnly` is the baseline implementation of the *new semantic
operator*, not retention of the old relational encoding. The old self-join /
negation or opaque aggregate graph is deleted after recognition.

### 6.3 Snapshot section probe

```text
function probe_only(index, group_key, is_present): OnlyResult<Row>:
  id = index.first(group_key)

  while id exists and not is_present(id):
    id = index.next(id)

  if id does not exist:
    return Empty

  first_row = table.row(id)
  id = index.next(id)

  while id exists and not is_present(id):
    id = index.next(id)

  if id exists:
    return Many

  return One(first_row)
```

This visits at most two *present qualifying* rows. Differential tombstones may
add index-chain visits; those are physical work and belong in the Rel cost,
not in the logical cardinality.

### 6.4 Differential standing state

The observer must be able to recover the survivor when a group changes from
two live candidates to one.

```text
state per group:
  member_count: MemberCount
  published: NoPublishedRow | Published(Row)

on distinct presence change (group, row, entered_or_exited):
  old_count = member_count[group]
  new_count = old_count + delta(entered_or_exited)
  member_count[group] = new_count

  if old_count == 0 and new_count == 1:
    survivor = source_index.first_present(group)
    publish_add(group, survivor)

  else if old_count == 1 and new_count == 2:
    publish_remove(group, previously_published_row)

  else if old_count == 2 and new_count == 1:
    survivor = source_index.first_present(group)
    publish_add(group, survivor)

  else if old_count == 1 and new_count == 0:
    publish_remove(group, previously_published_row)

  else:
    publish_nothing
```

The exact count is not saturated. Candidate membership remains authoritative
in the source table/index; the observer does not retain a second full
membership multiset. The observer receives only distinct presence transitions:
an additional derivation of an already-present identity does not increment
`member_count`, and removal of one remaining claim does not decrement it.
Batch ordering must ensure the source table's current presence is visible
before the `0/2 -> 1` survivor lookup.

### 6.5 Monotone fused lowering

```text
state per group = Empty | One(row) | Many

on candidate addition:
  Empty:
    state = One(row)
    publish_add(row)

  One(old):
    state = Many
    publish_remove(old)
    cancel remaining exclusive work for this group

  Many:
    ignore
```

This strategy is illegal when removals can occur, when another consumer needs
complete input, or when cancellation would suppress side effects or recursive
facts required elsewhere.

### 6.6 ControlFlow and runtime lowering

ControlFlow gains structured consumer regions rather than free-floating
`break` statements:

```text
ProgramChooseOneRegion:
  owns its selection state and qualifying-output loop nest
  records arbitrary, complete-objective, or ordered-objective lowering
  exits on the first witness only for arbitrary or proved-ordered access

ProgramObserveOnlyRegion:
  owns observer state variables
  owns the qualifying-output loop nest
  has on_empty, on_one, on_many continuations
  terminates its owned loop nest on transition One -> Many
```

For bounded probes, ControlFlow or an external query read calls shared runtime
primitives returning typed results. For generic fused subplans, the region
owns the qualifying outputs and exits only its own loop scope. Codegen never
reconstructs a rule from `@first`, `@only`, parse attributes, objective-functor
names, or aggregate names.

External query reads need a Rel-authored descriptor:

```text
ProgramQueryRead =
  CompleteCursor(CompleteCursorPlan)
  | AnyOneProbe(AnyOneProbePlan)
  | ObjectiveOneProbe(ObjectiveOneProbePlan)
  | OnlyProbe(OnlyProbePlan)
```

`ProgramQuery` carries exactly one variant. The C++ generator dispatches on
that physical variant and does not independently inspect `ParsedQuery`.
Bounded variants return named results such as `Empty | One(row)` or
`Empty | One(row) | Many`; they do not expose an ordinary cursor that callers
can accidentally continue to exhaustion.

## 7. Recognition and lowering pseudocode diff

```diff
- ParsedQuery.first_attribute survives only inside ParsedQuery
- generated host API returns an ordinary exhaustible cursor
+ QueryReadAnyOne owns declaration-level @first intent
+ Rel records read=FirstLiveRowProbe
+ Rel separately records bounded or complete producer work
+ ProgramQueryRead::AnyOneProbe returns Empty | One(row)
```

```diff
- @first(objective(row))
-   -> retain at most one result after complete, opaque evaluation
+ QueryChooseOne(ByObjective(expression, direction, tie_policy))
+   -> matching ordered access: OrderedObjectiveProbe
+   -> otherwise: CompleteChoiceState + UnorderedObjective blocker
+   -> dumps and counters expose which path was selected
```

```diff
- candidate rows
-   -> self-join to find a distinct pair
-   -> project nonunique group keys
- candidate rows AND-NOT nonunique keys
-   -> materialize result
+ candidate rows
+   -> QueryOnly(group_key, payload, distinct_key)
+   -> materialize exact-one result
```

```diff
- QueryOnly input
-   -> complete standing materialization
-   -> host/internal consumer scans materialized rows
+ QueryOnly input
+   -> Rel chooses exactly one OnlyLowering
+      snapshot table/index -> two-witness probe
+      differential table/index -> exact count + survivor lookup
+      exclusive insert-only flow -> fused Empty/One/Many observer
+   -> ControlFlow mechanically lowers the chosen plan
```

```diff
- compute B candidates completely
- compute R candidates completely
- reduce B to exact-one
- reduce R to exact-one
- join the two reduced relations
+ compute/maintain BOnly
+ derive DemandR from BOnly's One output
+ instantiate and compute ROnly only for DemandR keys
+ join BOnly and ROnly
```

## 8. Soundness boundaries

| Context | Stop after second now? | Forget tail across epochs? | Skip R while B is not one? |
|---|---:|---:|---:|
| Query over stable complete table | Yes | Not applicable | Yes, for this query invocation |
| Insert-only exclusive subgraph | Yes | Yes | Yes |
| Differential standing relation | Yes for a snapshot probe | No | Yes, with retractable internal demand |
| Shared producer with complete consumer | Probe may stop; producer may not | No | Only if R is otherwise unneeded |
| Recursive closure already materialized | Yes | Not applicable | Yes for query read |
| Closure currently being computed | Only with a separate exclusivity/absorption proof | Generally no | Not in the initial implementation |
| Impure/effectful path | No | No | No |

The first implementation excludes recursive fused cancellation. It may still
probe an already-complete recursive result. This is a semantic boundary, not a
feature flag or warning fallback.

## 9. Cost model integration

Correctness is structural. Cost chooses among certified physical plans and
reports when a certified bound was not realized.

Query authors:

```text
Card(input)
GroupCard(input, group_key)
OnlyOutputCard = number of groups whose group cardinality is exactly one
```

Rel authors:

```text
Touches(FirstLiveRowProbe(g)) =
  index_or_table_visits_until_first_present_qualifying_row(g)

Touches(CompleteObjectiveChoice(g)) =
  all_candidate_visits(g) + all_objective_evaluations(g)

Touches(OrderedObjectiveProbe(g)) =
  first optimum row
  + any additional optimum-section rows required by the tie policy

Touches(SnapshotSectionProbe(g)) =
  index_chain_visits_until_second_present_or_exhaustion(g)

Touches(DifferentialIndexState(delta)) =
  candidate_delta_rows
  + survivor_lookups_on_transitions_to_one

Touches(MonotoneFused(g)) =
  candidate_rows_until_second_or_exhaustion(g)
```

For B then R:

```text
Touches = Touches(BOnly)
        + Pr[B group is One] * Touches(ROnly | demanded)
```

The static report should distinguish:

```text
semantic opportunity:
  QueryOnly proves second-witness rejection

selected realization:
  complete standing | snapshot probe | differential index state | monotone fused

estimated work:
  actual plan row touches

missed-bound diagnostic:
  QueryOnly is backed by a complete materialization even though a certified
  probe/fused plan exists
```

### 9.1 Optimization visibility contract

The compiler must expose intent preservation and physical realization at
every level. Suggested stable dump forms are:

```text
Query:
  choose-one q#17 group=(F, cls) policy=arbitrary scope=query-read
  choose-one q#23 group=(F, cls)
    policy=objective(score, minimize, stable-key=Address)
  only q#31 group=(F, cls) payload=(B) distinct=(BinaryRefId)

Rel:
  consume q#17
    read=first-live-row-probe
    producer=complete blocker=shared-complete-consumer
  consume q#23
    producer=complete-objective-choice blocker=unordered-objective
  consume q#31
    producer=snapshot-section-probe decisive=second-distinct-or-exhaustion
    downstream=keyed-demand target=ROnly

ControlFlow:
  observe scope=read(q#17) exit=first-live-row
  observe scope=producer(q#31) reject=second-distinct accept=exhausted-one

Cost:
  q#17 intent_rows<=1 read_touches=1 producer_touches=complete
  q#23 intent_rows<=1 objective_evals=group-cardinality ordered=false
  q#31 qualifying_touches<=2 downstream_R_touches=0_when_B_not_one
```

The exact syntax can follow existing formatters, but the fields are an
architectural contract. A validator requires every non-`COMPLETE` intent to
have:

```text
the owning semantic Query node or boundary
the selected physical decision for every applicable work scope
the decisive event or exhaustion obligation
the proof facts used by a bounded plan
the typed blocker used by a complete plan
```

This makes three common failures mechanically visible:

1. **Intent loss:** `@first` appears in Parse but no `ChooseOne` intent appears
   in Query.
2. **False optimization:** result cardinality is one, but objective evaluation
   and producer work remain complete.
3. **Missed realization:** Query carries a bounded intent, but Rel selects
   complete work despite an available certified access path.

The cost model reports these states and measures row touches; it does not
change a blocker into a proof.

## 10. Validation and proof plan

### 10.1 Cross-level preservation

```text
V-CONSUMPTION-PRESERVE:
  every non-COMPLETE surface intent has one semantic Query owner or boundary
  every such use has one ConsumptionRealization in Rel
  every applicable work scope is Selected or Complete with a typed blocker
  every selected decision has one mechanical ControlFlow/codegen lowering
  no codegen path selects behavior directly from parse attributes

V-FIRST-PRESERVE:
  declaration @first becomes QueryReadAnyOne
  query-body @first forcing remains sequencing and never becomes ChooseOne
  a bounded query read cannot expose an exhaustible cursor

V-CHOOSE-PRESERVE:
  group, payload, policy, objective, direction, and ties survive unchanged
  an objective probe's physical order proves the logical order

V-ONLY-PRESERVE:
  every live QueryOnly has exactly one Rel ObserveOnlyOp
  every ObserveOnlyOp references the same group/payload/distinct projections
  every ObserveOnlyOp lowers to exactly one physical implementation
```

### 10.2 Strategy validators

```text
V-CHOOSE-ARBITRARY:
  scope is a boundary read or the language admits internal arbitrary choice
  selected read terminates at its first live qualifying row

V-CHOOSE-OBJECTIVE:
  objective is pure and returns its declared ordered domain
  direction and tie policy are explicit
  an OrderedObjectiveProbe matches objective, direction, and stable tie key
  otherwise producer decision is complete with UnorderedObjective blocker

V-ONLY-SNAPSHOT:
  source table is complete
  index key equals the QueryOnly group key
  probe checks current presence for differential tables

V-ONLY-DIFF:
  member_count is exact, not saturated
  source membership is table/index authoritative
  transitions to one happen after source presence updates
  published row is removed on transitions away from one

V-ONLY-MONOTONE:
  input cannot receive deletions
  observed work is exclusive or every other use is independently complete
  path is pure
  cancellation does not cross an SCC boundary

V-ONLY-DEMAND:
  internal demand is produced only by QueryOnly One output
  demand removal tears down or invalidates the keyed R instance
  no ungated eager R path survives the transform
```

### 10.3 Semantic tests

Selection and boundary tests:

- declaration `#query @first` appears as `QueryReadAnyOne` in the Query dump;
- query-body `@first message(...)` appears only as forcing/sequencing;
- an arbitrary boundary read returns `Empty` or one live qualifying row and
  cannot be advanced again;
- an unordered objective returns the correct optimum after evaluating every
  candidate and reports `UnorderedObjective`;
- a compatible ordered index returns the same optimum with a bounded probe;
- `AnyOptimum`, `ByStableKey`, and `RejectTies` have distinct tie behavior;
- an internal arbitrary-choice fixture is withheld until its nondeterministic
  semantics are accepted explicitly.

Per group:

- zero candidates -> no output;
- one candidate -> that row;
- two candidates -> no output;
- many candidates -> no output;
- duplicate derivations of one distinct row -> one output under set semantics;
- two rows with equal payload but distinct identity -> behavior follows the
  declared `DistinctKey`.

Differential traces:

- `0 -> 1 -> 2 -> 3 -> 2 -> 1 -> 0` publishes and retracts at the exact
  transitions;
- same-batch add/remove netting leaves output equal to the post-batch set;
- compaction and tombstones do not cause a stale row to become the survivor.

Composition tests:

- B has two candidates -> R receives no demand and records zero row touches;
- B becomes one after a removal -> R demand appears and ROnly is evaluated;
- B returns to many -> R demand retracts;
- a second full consumer preserves complete candidate state;
- recursive input may use snapshot probing but cannot select monotone fused
  lowering.

### 10.4 Performance proof

Use runtime row-touch counters, not functor side effects.

```text
query @first, first physical row qualifies:
  read qualifying visits = 1
  producer visits may remain complete and are reported separately

objective @first, unordered input of N candidates:
  objective evaluations = N
  output rows <= 1

objective @first, compatible ordered index:
  objective candidate visits = 1
  plus tie-section visits required by the declared tie policy

many B, arbitrary R:
  B qualifying visits = 2
  R visits = 0

singleton B, many R:
  B scan exhausts
  R qualifying visits = 2

empty/singleton group:
  scan exhausts because success/emptiness requires proof of exhaustion
```

Pin Query, Rel, and ControlFlow dumps for the fixtures so a future pass cannot
silently retain a bounded semantic intent while dropping its physical decision
or falsely claim that one result meant one row touch.

## 11. Implementation sequence

1. Add `ConsumptionIntent`, work-scope decisions, typed blockers, dumps, and
   validators without changing execution; use current `#query @first` as the
   preservation witness that fails before the plumbing lands.
2. Lower declaration `@first` to `QueryReadAnyOne`, add a Rel-authored
   `ProgramQueryRead::AnyOneProbe`, and replace its exhaustible generated cursor
   with an `Empty | One(row)` result.
3. Add row-touch accounting that separately reports read, producer, objective,
   and downstream-demand work.
4. Add `OnlyState<Row>` / `OnlyResult<Row>`, `QueryOnly`, and an `@only` query
   surface as the exact-one semantic oracle.
5. Add Query formatting, hashing, equality, canonicalization, differential
   tracking, stratification, and dead-flow rules for `QueryOnly`.
6. Add the exact-one recognizer and delete the matched self-join/negation graph
   after replacement.
7. Add `kObserveOnly`, its tagged decisions, `SnapshotSectionProbe`, and
   `DifferentialIndexState` with exact member count and survivor lookup.
8. Generalize demand from query-root seeds to internal relation-produced
   demand; use `BOnly` output to own R instance activation and retraction.
9. Implement `MonotoneFused` for pure, acyclic, exclusive inputs.
10. Decide whether unparameterized internal `@first` admits arbitrary choice.
    Then add `QueryChooseOne` and the corresponding internal surface; do not
    expose the surface before this semantic decision.
11. Add objective-bearing `@first`, explicit direction/ties, complete objective
    evaluation, and `OrderedObjectiveProbe` only with a matching order proof.
12. Add missed-bound diagnostics after all correctness validators are in
    place.

Do not retain the recognized self-join/negation graph as a compatibility path.
Do not add a mode flag selecting old versus new exact-one semantics. Tests that
pin complete materialization of a recognized `ONLY` shape preserve an accident
and should be replaced with semantic, IR-preservation, differential, and
row-touch assertions.

## 12. Composability audit

### Authority

- Query is the sole owner of exact-one truth.
- Query operators own internal selection truth; query-read boundaries own only
  observation intent.
- Rel is the sole owner of physical strategy.
- Source tables/indexes remain the sole owner of candidate membership.
- Internal demand owns activation only, never candidate truth.

### Type preservation

- `ConsumptionIntent`, `SelectionPolicy`, objective values, `OnlyState`,
  `GroupKey`, `DistinctKey`, `InputChangeMode`, physical decisions,
  `MemberCount`, and `RowTouches` remain distinct quantities.
- No raw `int` simultaneously means a cardinality, witness threshold, and work
  estimate.

### Algebra

- Arbitrary choice, ordered optimum, and exact-one are distinct algebras even
  though each emits at most one row per group.
- Objective selection is complete unless an access-order proof excludes every
  unseen candidate from winning.
- Snapshot early rejection relies only on seeing two distinct live rows.
- Insert-only `Many` is absorbing.
- Differential `Many` is not absorbing; the exact count and backing index
  restore `One` correctly.
- Set semantics are explicit through `DistinctKey` rather than accidental
  payload comparison.

### Control composition

- Bounded observation and conditional R activation are separate operators that
  compose through a real relation.
- Bottom-up truth production remains authoritative; demand controls which
  keyed subgraphs are active.
- Recursive cancellation is excluded until it has its own proof rather than
  being smuggled through the acyclic lowering.

### State and termination

- Each lowering states whether the first witness, ordered first witness,
  exhaustion, second witness, or an epoch transition terminates work.
- No snapshot-local saturation leaks into differential standing state.
- No eager R computation survives beside demanded R computation.

### Tests

- Semantic tests separately establish arbitrary choice, objective selection,
  and exact-one truth.
- Validators establish cross-IR preservation and strategy preconditions;
- row-touch tests establish that the selected bounded plan actually does less;
- differential traces establish that doing less does not forget future
  transitions.

## 13. Second split-brain review

The written design was re-derived from its pseudocode after adding the general
consumption vocabulary and compared again with the executable Parse, Query,
Rel, ControlFlow, query-codegen, demand, and runtime paths.

### Finding A: “at most one result” still collapsed unlike algorithms

The first draft centered `ONLY` and treated `FIRST` mostly as evidence of lost
metadata. That did not encode the larger goal. Arbitrary selection, objective
selection, existence, exact-one, and completeness can share an output bound
while having different decisive events and exhaustion obligations.

**Repair:** `ConsumptionIntent` is tagged by semantics, never by a raw limit.
`QueryChooseOne` and `QueryOnly` are different operators.

### Finding B: surface placement was mistaken for semantic placement

Treating `@first` as only a query-codegen feature would optimize host reads but
could not express internal choice or compose choice with demand. Conversely,
turning every query `@first` into a relational truncation could corrupt a
shared standing relation.

**Repair:** boundary `QueryReadAnyOne` and internal `QueryChooseOne` are
separate owners that derive a common use-scoped intent.

### Finding C: the existing `@first` spelling is split-brained

One parse use means “return at most one row”; another means “send this forcing
message before evaluating the query.” The latter does lower into operational
machinery, while the former never becomes an optimization-IR fact.

**Repair:** lower by typed parse role, never by token spelling. Dumps use
`read-any-one`, `choose-one`, and `forcing` rather than the ambiguous word
`first` alone.

### Finding D: result bound and work bound were still too easy to conflate

An objective-bearing selection returns one row but normally evaluates every
candidate. A boundary read may stop after one row while its shared producer
still materializes the whole relation. A B-only observer may bound its own
probe and independently gate all R work.

**Repair:** Rel records required read, producer, and downstream decisions
separately. Cost reports each scope separately.

### Finding E: objective selection needs more than a functor call

`@first(functor_call(...))` does not by itself say whether lower or higher is
better, how ties behave, whether the objective is pure, or whether its result
domain has a total order. Guessing from names or raw floats would create a
quantity/type category error.

**Repair:** `SelectionPolicy::ByObjective` carries a named objective domain,
direction, and tie policy. An ordered probe requires an exact physical-order
proof; otherwise the complete plan carries `UnorderedObjective` visibly.

### Finding F: arbitrary internal choice is a language decision

At a host boundary, accepting any live row is a clear observation contract.
Inside relational truth, arbitrary choice can make the selected row depend on
the physical plan and can churn under deletions.

**Repair:** the architecture supports the typed variant, but implementation of
unparameterized internal `@first` is gated on explicitly accepting that
semantics. Objective-bearing internal selection is not blocked by this choice.

### Finding G: strategy-specific fields reintroduced optionality

The first physical sketch placed a source index on the generic observer even
though not every strategy needs the same state.

**Repair:** each tagged strategy payload owns its required fields;
`ObserveOnlyOp` owns one non-null `OnlyPlanDecision`.

### Finding H: complete execution could become a silent fallback

A plan that preserves results but loses bounded work is semantically correct,
so ordinary equivalence tests cannot detect it.

**Repair:** complete decisions require a typed blocker, IR dumps pin that
blocker, and row-touch tests distinguish a true bounded realization from a
one-row result produced by whole-world work.

### Verdict

The revised architecture is coherent if these invariants remain hard gates:

```text
one semantic owner per consumption meaning
one Rel decision per applicable work scope
no raw row cap standing in for FIRST, objective choice, or ONLY
no objective early exit without an ordering/bound proof
no producer cancellation inferred from a boundary read
no silent complete plan for a non-COMPLETE intent
no eager R path beside internally demanded R
no differential Many saturation that prevents recovery to One
```

The recommended first vertical proof is the existing declaration-level
`@first`: demonstrate that its parse fact disappears today, preserve it as
`QueryReadAnyOne`, select a bounded read, and verify one read touch while
reporting producer work independently. The first new semantic slice is
`@only`, because its second-witness rejection and exhaustion-for-success rule
exercise the architecture more strongly than arbitrary selection. Internal
choice and objective selection then branch from the same substrate without
being disguised as exact-one.
