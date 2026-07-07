# The Data-Flow IR ("Query" IR)

The data-flow IR is the middle layer of the compiler pipeline:

    ParsedModule  --Query::Build-->  Query (data-flow IR)  --Program::Build-->  Program (control-flow IR)

`Query::Build(module, log, optimize)` (`lib/DataFlow/Build.cpp`) lowers every
clause of a parsed module into one shared graph; `Program::Build`
(`lib/ControlFlow/Build/Build.cpp`) consumes that graph to emit the
control-flow IR. The public, immutable-facade API lives in
`include/drlojekyll/DataFlow/Query.h` (`Query`, `QueryView`, `QueryColumn`,
`QueryTuple`, ...); the mutable implementation classes live in
`lib/DataFlow/Query.h` (`QueryImpl`, `QueryViewImpl`, `QueryColumnImpl`, ...).
The implementation file also defines the short aliases used throughout the
pass code: `VIEW`, `COL`, `COND`, `TUPLE`, `JOIN`, `MERGE`, `CMP`, `MAP`,
`AGG`, `KVINDEX`, `SELECT`, `INSERT`, `NEGATION`, `REL`, `IO`, `CONST`, `TAG`.

The IR models each Datalog program as a dataflow graph. Data enters through
message RECEIVEs and constant streams, flows through relational operators,
and leaves through INSERTs (into relations, query materializations, or
published messages). Every interior node is a *view*: a `QueryViewImpl` that
owns an ordered list of output columns (`DefList<QueryColumnImpl> columns`)
and describes the multiset of tuples those columns carry. Edges are
column-level uses: a view's `input_columns` (and `attached_columns`) are
`UseList`s of the *output* columns of its predecessor views. `QueryImpl`
owns all nodes in per-kind `DefList`s (`selects`, `tuples`, `joins`,
`merges`, `maps`, `compares`, `negations`, `kv_indices`, `aggregates`,
`inserts`) plus the non-view entities: `relations`, `ios`, `constants`,
`tags`, and `conditions`.

## The column model

`QueryColumnImpl` (a `Def<QueryColumnImpl>`) represents all values that can
inhabit one position of a view's output tuples. Key fields:

- `view`: the owning view; `index`: the column's position within
  `view->columns` (`Index()` recomputes it when unset).
- `var` / `type`: the `ParsedVariable` (if any) and `TypeLoc` the column
  carries. Dumps label columns with the variable name (`X`, `AutoVar_2`).
- `id`: during building, roughly the smallest `ParsedVariable::Order` in the
  clause that produced the column. `QueryImpl::FinalizeColumnIDs` reassigns
  IDs after optimization so that two columns with the same ID hold the same
  runtime value.
- `referenced_constant`: a `UseRef` to a real constant column when this
  column is a *constant reference* -- a column known to always hold that
  constant at runtime. The indirection matters: dataflow edges sometimes
  encode control dependencies, so a constant-ref column keeps flowing
  through the graph instead of being blindly replaced by its constant.
  `IsConstant()` is true only for columns of a CONST stream SELECT;
  `IsConstantRef()` for columns with an attached constant;
  `TryResolveToConstant()` returns the constant when
  `OptimizationContext::can_replace_inputs_with_constants` permits the
  substitution. `CopyConstantFrom` propagates the attachment from an input
  column to an output column.
- `IsUsed()` / `IsUsedIgnoreMerges()`: whether any view reads the column.

### Group IDs

`VIEW::group_ids` guards against over-merging during CSE. The canonical
hazard is a cross-product:

    node_pairs(A, B) : node(A), node(B).

Both SELECTs read the same relation and are structurally identical, but
merging them collapses the cross-product into the diagonal.
`QueryImpl::RelabelGroupIDs` (`lib/DataFlow/Optimize.cpp`) assigns a fresh
`group_id` to every JOIN, AGGREGATE, and KVINDEX, then propagates ID *sets*
from users back toward sources to a fixpoint: each view's `group_ids`
accumulates the group IDs of every view that uses its columns. `Equals`
refuses to unify two views whose group ID sets overlap
(`VIEW::InsertSetsOverlap`), so the two `node` SELECTs above -- both tainted
by the same JOIN's group ID -- stay distinct. The propagation loop asserts
`view != user` for every column use: **no view is ever its own direct
user**. `ClearGroupIDs` erases the sets for passes that must not be
influenced by them.

### "Canonical"

`VIEW::is_canonical` marks whether a node may have changed and needs
re-inspection by its `Canonicalize` method. A canonical view is one in its
locally "most optimal" form: constants propagated to outputs, duplicate
outputs collapsed onto one representative, unused columns dropped (when
permitted), and inputs read from as far up the graph as possible.
`is_locked` forbids further canonicalization of a node; `is_dead` and
`is_unsat` (see below) exclude it.

## Node kinds

Each view class overrides `KindName()`, which is the name printed in
`-dot-out` dumps. Several classes print different names depending on their
role:

| Class (lib/DataFlow/Query.h) | Alias | Dump name(s) | Semantics |
|---|---|---|---|
| `QuerySelectImpl` | `SELECT` | `RECEIVE`, `CONST`, `PUSH` | Source of tuples from a stream or relation |
| `QueryTupleImpl` | `TUPLE` | `TUPLE` | Column projection/forwarding |
| `QueryKVIndexImpl` | `KVINDEX` | `KVINDEX` | Key-value index with merge functors |
| `QueryJoinImpl` | `JOIN` | `JOIN`, `PRODUCT` | Equi-join over pivots; 0 pivots = cross-product |
| `QueryMapImpl` | `MAP` | `MAP`, `FUNCTION`, `PREDICATE`, `FILTER` | Functor application |
| `QueryAggregateImpl` | `AGG` | `AGGREGATE` | Aggregating functor over groups |
| `QueryMergeImpl` | `MERGE` | `UNION` | Union of same-arity views |
| `QueryCompareImpl` | `CMP` | `COMPARE eq/gt/lt/neq` | Filter by comparing two columns |
| `QueryNegateImpl` | `NEGATION` | `AND-NOT`, `AND-NEVER` | Absence check against another view |
| `QueryInsertImpl` | `INSERT` | `INSERT`, `INCREMENT`, `MATERIALIZE`, `TRANSMIT` | Sink into a relation, condition, query, or message |

**SELECT** (`QuerySelectImpl`) sources tuples and has no `input_columns`.
It reads either a `stream` (`QueryIOImpl` message input, printed `RECEIVE`;
or `QueryConstantImpl`/`QueryTagImpl`, printed `CONST`) or a `relation`
(printed `PUSH`, because INSERTs push data to it). Its weak `inserts` list
names the INSERT views that can feed it;
`QueryImpl::ConnectInsertsToSelects` links them when the relation's full
state need not be visible for point queries.

**TUPLE** (`QueryTupleImpl`) forwards its `input_columns` to its output
columns, one to one, possibly reordering, duplicating, or narrowing.
TUPLEs are the IR's only pure forwarding views, and the normalization
passes lean on that: guard tuples isolate a view's external interface
(`GuardWithTuple` / `GuardWithOptimizedTuple` in `lib/DataFlow/View.cpp`),
`QueryImpl::ConvertConstantInputsToTuples` makes TUPLE the only view kind
whose inputs may all be constants, `ExtractConditionsToTuples` moves tested
conditions onto TUPLEs, and `ProxyInsertsWithTuples` puts a TUPLE before
every INSERT so input column indices line up positionally downstream.

**KVINDEX** (`QueryKVIndexImpl`) treats `input_columns` as keys and
`attached_columns` as values; `merge_functors` (one `ParsedFunctor` per
value column) combine an old value with a proposed new one when a key is
re-inserted. It always has `can_produce_deletions` set: an update to a key
produces a deletion of the old value tuple.

**JOIN** (`QueryJoinImpl`) equi-joins `joined_views`. `out_to_in` maps each
output column to the input columns it merges: the first `num_pivots` output
columns are *pivots*, each merging one column from every joined view (all
must hold equal values); the remaining outputs forward exactly one input
column each. When `num_pivots` is zero the node is a cross-product and
prints as `PRODUCT`.

**MAP** (`QueryMapImpl`) applies `functor`. `input_columns` feed the
functor's `bound` parameters; the first `functor.Arity()` output columns
mirror the parameters in declaration order (both `bound` copies and `free`
results); `attached_columns` carry extra lexical context through unchanged.
The dump name encodes the flavor: with free parameters, `MAP` (pure) or
`FUNCTION` (impure); without, `PREDICATE` (pure) or `FILTER` (impure).

**AGG** (`QueryAggregateImpl`) applies an aggregating `functor`. Its three
input lists are `group_by_columns` (partition the input, invisible to the
functor), `config_columns` (bound parameters "configuring" the functor),
and `aggregated_columns` (the summarized values). Outputs are the groups,
configs, and the functor's summary columns.

**MERGE** (`QueryMergeImpl`, printed `UNION`) unions the same-arity
`merged_views` positionally. MERGEs are where dataflow cycles close:
recursive clauses produce a MERGE whose `induction_info`
(`InductionInfo`) records inductive vs. non-inductive predecessors and
successors, the `merge_set_id`, and `merge_depth`. `IsInductive()` is true
only for such MERGEs.

**CMP** (`QueryCompareImpl`) filters its predecessor's rows by comparing
two input columns with `op`. An equality CMP fuses the compared pair into a
single output (`[eq, attached...]`); any other operator keeps both
(`[lhs, rhs, attached...]`); `attached_columns` pass through.

**NEGATION** (`QueryNegateImpl`) forwards `input_columns` plus
`attached_columns` only when no matching tuple exists in `negated_view`
(a `UseRef` to the checked view, which gets `is_used_by_negation` set).
`is_never` distinguishes `@never` negations (`AND-NEVER`) from ordinary
ones (`AND-NOT`).

**INSERT** (`QueryInsertImpl`) is a sink: it has `input_columns` but *no
output columns* (asserted in `GuardWithTuple`). It targets either a
`relation` or a `stream`, and prints as `MATERIALIZE` (query),
`TRANSMIT` (message publication), `INSERT` (ordinary relation), or
`INCREMENT` (zero-arity declaration, i.e. a condition setter).

## Conditions (zero-arity predicates)

`QueryConditionImpl` (`COND`) models a zero-argument predicate: either a
user-defined `ParsedExport` (`declaration` is set) or an anonymous condition
invented by optimization, e.g. when canonicalization drops a view's last
data edge to a predecessor and must keep the "predecessor holds data"
dependency as control. A condition tracks `setters` (views that produce it;
a zero-arity INSERT is an `INCREMENT` setter) and `positive_users` /
`negative_users` -- all weak lists. On the view side,
`positive_conditions` / `negative_conditions` are the conditions a view
tests: its tuples flow only if every positive condition holds and no
negative one does, and `sets_condition` (a `WeakUseRef`) is the condition
the view sets. Testing a condition wraps a view in the dump with a `COND`
cell listing the tested predicates (`!` prefix for negative), linked to the
condition node with purple edges. `QueryImpl::ShrinkConditions` collapses
chains of constant-input TUPLEs that each set a condition tested by the
next; `ExtractConditionsToTuples` then confines tested conditions to TUPLE
nodes for the control-flow builder.

## Def-use machinery

Everything sits on `include/drlojekyll/Util/DefUse.h`:

- `Def<T>` (CRTP) is a definition holding its `uses`. `Use<T>` records
  `(user, def_being_used)`. `Def::ReplaceAllUsesWith(that)` retargets every
  use and notifies each `User` via `Update(timestamp)`; timestamps
  (`User::gNextTimestamp`) let views invalidate cached hashes and depths
  when operands change.
- `UseList<T>` is an owning list of uses held by one `User` (e.g.
  `input_columns`, `merged_views`, `positive_conditions`);
  `WeakUseList<T>` / `WeakUseRef<T>` are non-owning variants whose entries
  are nulled when the def dies -- used for references that must not keep
  nodes alive (`joined_views` -- a JOIN's strong uses are the columns in
  `out_to_in` -- plus `predecessors`, `successors`, condition user/setter
  lists, `sets_condition`, and `QuerySelectImpl::inserts`).
- `DefList<T>` owns the nodes themselves (`QueryImpl::tuples` etc.);
  `RemoveUnused()` reclaims defs with no uses.

Views are both `Def<QueryViewImpl>` (usable as whole nodes, e.g. by a
MERGE) and `User` (they use columns and conditions). A view `IsUsed()` if
its columns are read, it is used as a node, or it sets a condition.

Core invariants:

- **No view is its own direct user** (asserted during
  `RelabelGroupIDs`). CSE additionally refuses merges that would create
  one (`DirectlyUsesColumnsOf` checks in `lib/DataFlow/Optimize.cpp`).
- **TUPLEs are the only forwarding views**, and normalization funnels
  structure through them (guards, condition carriers, INSERT proxies).
- **Unsatisfiable views** (`is_unsat`) provably produce no tuples --
  e.g. a CMP of two distinct unique constants, or a UNION whose operands
  are all unsatisfiable. `MarkAsUnsatisfiable` propagates the property so
  dead-flow elimination can prune the dependent subgraph.

### Dead-flow elimination

`QueryImpl::EliminateDeadFlows` (`lib/DataFlow/DeadFlowElimination.cpp`)
is a mark-and-sweep taint pass: a view is dead unless derived from input.
Taint seeds are message RECEIVEs, stream SELECTs, and constants (an
all-constant input set reports a null incoming view, which is pre-seeded).
Taint propagates forward to a fixpoint with per-kind rules: TUPLE, INSERT,
CMP, MAP, and KVINDEX are tainted when their incoming view is; a MERGE when
*any* live operand is; a JOIN only when *all* joined views are; a NEGATE
only when both its predecessor *and* its negated view are; a relation
SELECT when any live INSERT into the relation is. The sweep unlinks
untainted views, prunes untainted operands out of surviving MERGEs, and
deletes TUPLEs forming trivial self-cycles through a MERGE
(`IsTrivialCycle`: the TUPLE's only user is the MERGE that feeds it and it
forwards columns 1:1, so it routes a subset of the MERGE's data back into
the MERGE). Finally, conditions left with no setters resolve to a
fixpoint: negative tests of them are vacuously true and are dropped, while
positive testers are unsatisfiable and are deleted.

This is what collects **source-less forwarding cycles**: a recursive
clause set whose only "sources" are its own outputs (e.g. a relation
defined solely in terms of itself) forms a cycle of TUPLEs/MERGEs that
keeps itself alive under use-count reclamation, but never receives taint
from the input boundary, so the whole cycle is deleted as unsatisfiable.
`PullDataFromBeyondTrivialTuples` similarly guards its upward chase with a
visited set so walks around such cycles terminate.

## Building and the optimization driver

`Query::Build` runs, in order: `BuildClause` per clause;
`RemoveUnusedViews`; `TrackDifferentialUpdates`; `Simplify`;
`ConnectInsertsToSelects`; `Optimize` (only when `optimize` is true -- the
`-disable-dataflow-opt` CLI flag turns it off); then the normalization tail
consumed by the control-flow builder: `ConvertConstantInputsToTuples`,
`ExtractConditionsToTuples`, `ProxyInsertsWithTuples`, `LinkViews`
(fills the public `predecessors`/`successors`), `IdentifyInductions`,
`FinalizeDepths`, `FinalizeColumnIDs`, `TrackDifferentialUpdates`,
`TrackConstAfterInit`, the forwards/backwards column taint analyses, and
`BuildEquivalenceSets` (groups views that can share one data model).

`QueryImpl::Simplify` runs even in unoptimized builds: CSE over SELECTs
only, two conservative canonicalization sweeps over JOINs (dropping useless
pivots and rewriting single-source JOINs into TUPLEs), then
`RemoveUnusedViews`.

`QueryImpl::Optimize` (`lib/DataFlow/Optimize.cpp`) interleaves three
global passes because each exposes work for the others:

    do_cse():  while CSE(all views) merges something:
                 RemoveUnusedViews(); TrackDifferentialUpdates()
    do_sink(): UNION-sinking re-canonicalization (body disabled)

    do_sink(); do_cse()
    Canonicalize(default: bottom-up, conservative)
    do_sink(); do_cse(); do_sink()
    for up to max INSERT depth, while EliminateDeadFlows() changes:
      Canonicalize(top-down, +can_remove_unused_columns,
                   +can_replace_inputs_with_constants)
      do_sink()
      if ShrinkConditions(): Canonicalize(same opts)
      RemoveUnusedViews()
    do_cse(); RemoveUnusedViews()

**CSE** buckets views by `HashInit()` (a shallow kind/shape hash that
tolerates cycles), proves deep equivalence pairwise with `Equals` over an
`EqualitySet` (so checks over cyclic graphs converge), and applies merges
in priority order (matching `UpHash`es first, shallower views first),
refreshing group IDs after each merge.

**Canonicalize** drives every view's virtual `Canonicalize(query, opt,
log)` to a fixpoint in depth order (bottom-up from SELECTs, or reverse
when `opt.bottom_up` is false), guarded by a pass cap of about
`max(2N, N^2)` and an eight-entry history of per-pass change hashes that
breaks cyclic non-progress. Per-node `Canonicalize` methods are local,
relation-preserving rewrites; the shared repertoire (see each node's
`.cpp` for the full doc comment) is:

- *Guard tuples*: interpose a TUPLE over a view's users so the view can
  reorder, deduplicate, or drop columns beneath a stable facade
  (`GuardWithTuple`, `GuardWithOptimizedTuple`).
- *Constant propagation*: mark outputs of constant inputs as constant
  refs (`CanonicalizeColumn`), and resolve constant-ref inputs to their
  constants when `can_replace_inputs_with_constants` is set.
- *Column deduplication and unused-column removal*: redirect users of
  duplicated outputs to one representative; drop unused outputs when
  `can_remove_unused_columns` permits (an input used two or more times is
  always removable).
- *Structure folding*: bypass trivial forwarding TUPLEs and single-source
  UNIONs (`PullDataFromBeyondTrivialTuples`); flatten and deduplicate
  MERGE operands; fold trivially true/false CMPs; convert single-view
  JOINs to TUPLEs; propagate `is_unsat`.
- *Condition compensation*: when a rebuild drops the last data edge to a
  predecessor, keep the row-presence dependency by making the predecessor
  set a condition the view tests (`CreateDependencyOnView`).

`OptimizationContext` (`lib/DataFlow/Optimize.h`) carries the switches:
`can_replace_inputs_with_constants`, `can_remove_unused_columns`,
`can_sink_unions` (plus the per-kind `can_sink_unions_through_tuples` /
`_maps` / `_negations` / `_joins`), and `bottom_up`. **Union sinking is
disabled**: `do_sink`'s body is commented out and nothing else sets
`can_sink_unions`, so `QueryMergeImpl::SinkThroughTuples` /
`SinkThroughMaps` / `SinkThroughNegations` / `SinkThroughJoins` in
`lib/DataFlow/Merge.cpp` are unreachable.

## Reading a dump

The CLI (`bin/drlojekyll/Main.cpp`) emits the data-flow graph with
`-dot-out <PATH>` in GraphViz DOT, one HTML-table label per node. (Note:
`-ir-out` emits the *control-flow* IR, not this graph.) Compiling this
program:

    #message in(u64 X).
    #query out(free u64 A, free u64 B).

    out(X, 1) : in(X).

with `drlojekyll cp.dr -disable-dataflow-opt -dot-out -` yields the shape
on the left; the default (optimized) build yields the right:

    RECEIVE in[X]   CONST 1            RECEIVE in[X]   CONST 1
          |            |                     |            |
        TUPLE [X, 1 -> X, AutoVar_2]       TUPLE [X, 1 -> X, 1]
          |                                  |
        TUPLE [X, AutoVar_2]             MATERIALIZE out[X, 1]
          |
        TUPLE [X, AutoVar_2]
          |
        TUPLE [X, AutoVar_2]
          |
        UNION [X, AutoVar_2]
          |
        MATERIALIZE out[X, AutoVar_2]

Canonicalization propagates the constant `1` through the TUPLE chain
(each hop marks the output as a constant ref), MERGE canonicalization
replaces the single-operand UNION with a forwarding TUPLE, TUPLE
canonicalization collapses the forwarding chain, and unused views are
reclaimed. The optimized dump, in full (node IDs derive from pointers, so
they differ run to run):

```dot
digraph {
bgcolor="#f0f4f7";
node [shape=none margin=0 nojustify=false labeljust=l font=courier];
t4317911664 [ label=<...<TD>RELATION out</TD><TD port="p0">A</TD><TD port="p1">B</TD>...>];
t4317911664 -> v4317914768;
t4317908304 [label=<...<TD>I/O in</TD><TD port="p0">X</TD>...>];
v4317908512 -> t4317908304;
v4317907376 [label=<...<TD rowspan="2">EQ SET 1</TD><TD>CONST</TD><TD port="c1"><B>1</B></TD>...>];
v4317908512 [label=<...<TD rowspan="2">EQ SET 2</TD><TD>RECEIVE</TD><TD port="c2">X</TD>...>];
v4317914768 [label=<...<TD rowspan="2">TABLE 6<BR />EQ SET 3</TD><TD>MATERIALIZE out</TD><TD port="c0">X</TD><TD port="c1"><B>1</B></TD>...>];
v4317914768:c0 -> v4317912640:c3;
v4317914768:c1 -> v4317912640:c4;
v4317912640 [label=<...<TD rowspan="2">EQ SET 3</TD><TD rowspan="2">TUPLE</TD><TD port="c3">X</TD><TD port="c4"><B>1</B></TD></TR><TR><TD port="p0">X</TD><TD port="p1"><B>1</B></TD>...>];
v4317912640:p0 -> v4317908512:c2;
v4317912640:p1 -> v4317907376:c1;
}
```

Line by line:

- `t...` nodes are non-view entities: the `RELATION out` table (one cell
  per declared parameter) and the `I/O in` message. The edge
  `t(out) -> v(MATERIALIZE)` links the relation to its INSERT; the edge
  `v(RECEIVE) -> t(in)` links the receive to its message.
- Each `v...` node is a view. The first cell stacks annotations: `TABLE n`
  is the control-flow IR table ID assigned to the view's data
  (`VIEW::table_id`); `SET i DEPTH j` appears on inductive views
  (`InductionInfo` merge set and depth); `EQ SET n` is the equivalence-set
  ID (views sharing a data model). The next cell is `KindName()` (plus the
  declaration name for `RECEIVE`/`MATERIALIZE`/etc., the functor name for
  MAPs, the operator for COMPAREs). Then one cell per *output* column,
  port `c<id>`, labeled with the column's variable -- bold literals mark
  constant/constant-ref columns (`<B>1</B>`). A COMPARE prints
  `COMPARE eq|gt|lt|neq` and a `COPY` cell before its attached columns.
- Views with inputs get a second row: one cell per *input* column, port
  `p<index>`. Edges `v1:p<i> -> v2:c<j>` say "input `i` of `v1` reads
  the output column with ID `j` of `v2`": here the TUPLE reads `X` from
  the RECEIVE and `1` from the CONST stream. INSERTs have no output row;
  their `c<i>` ports are input *positions*, so the MATERIALIZE's edges
  `c0 -> c3`, `c1 -> c4` read the TUPLE's two outputs.
- On a JOIN, pivot output cells span the pivot's input columns and are
  color-filled; a UNION has plain (portless) edges to each merged view;
  condition and differential (deletion-carrying) edges print in purple.
