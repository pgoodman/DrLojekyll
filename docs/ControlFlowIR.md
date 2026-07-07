# Control-Flow IR ("Program" IR)

The control-flow IR is the imperative middle layer of the compiler. It is
built from the data-flow `Query` by `Program::Build(query, first_id, optimize)`
(`lib/ControlFlow/Build/Build.cpp`) and consumed by the C++ code generator
(`lib/CodeGen/CPlusPlus/Database.cpp`). Where the data-flow IR is a graph of
relational operators, the control-flow IR is a concrete program: a set of
procedures made of nested *regions* that implement **incremental maintenance**
of the dataflow — both the *addition* of tuples arriving in messages and the
*removal* (retraction) of tuples, with re-derivation checks so that a tuple
provable by another route survives the removal of one of its derivations.

The public API is `include/drlojekyll/ControlFlow/Program.h` (treat it as
ground truth for names); the printer is `lib/ControlFlow/Format.cpp`; the
`-ir-out <PATH>` flag of the CLI dumps the IR, and `-disable-controlflow-opt`
skips `ProgramImpl::Optimize` so the raw built form can be inspected.

## Top-level structure

A `Program` owns:

- **Tables** (`DataTable`) — persistent storage, each with `DataColumn`s and
  `DataIndex`es. One table may back several `QueryView`s (`DataTable::Views()`):
  the builder unions views into *data models* (equivalence classes of views
  that can share storage). `DataIndex` maps key columns to value columns.
- **Constants and global variables** (`DataVariable`) — every variable has a
  `VariableRole` (`kConstant`, `kConditionRefCount`, `kInitGuard`,
  `kJoinPivot`, `kScanOutput`, `kFunctorOutput`, `kMessageOutput`, ...).
  Condition ref-counts and init guards are globals; everything else is defined
  by a region.
- **Procedures** (`ProgramProcedure`) — every procedure returns `true` or
  `false`, takes vector and variable parameters, and owns locally defined
  vectors.
- **Queries** (`ProgramQuery`) — glue records tying a `#query` declaration to
  its backing `table`, optional `index`, optional `tuple_checker` procedure,
  and optional `forcing_function` procedure. Code generators implement query
  cursors from these without the IR prescribing an implementation.

### Procedure kinds (`ProcedureKind`)

| Kind | Dump prefix | Purpose |
|---|---|---|
| `kInitializer` | `^init:` | Runs once; starts flows rooted at constant tuples by calling the entry function with empty vectors. |
| `kEntryDataFlowFunc` | `^entry:` | Receives message vectors, does the initial hops (e.g. persisting inputs, appending to induction vectors), then calls the primary function. |
| `kPrimaryDataFlowFunc` | `^flow:` | Takes the induction vectors collected by the entry function and executes the rest of the dataflow (fixpoints, joins, publishing). |
| `kMessageHandler` | `^receive:` | One per received `#message`; wraps a vector of tuples and calls the entry function. |
| `kTupleFinder` | `^find:` | Top-down checker: given a tuple, returns `true` if it is (re-)provable; converts unprovable *unknown* states into *absent*. |
| `kConditionTester` | `^test:` | Tests condition variables (`@cond` ref counts); returns `true` iff all are satisfied. |
| `kQueryMessageInjector` | `^inject:` | Forcing function for a query: internally sends a message given the query's `bound` parameters. |

### Vectors (`DataVector`, `VectorKind`)

Vectors are transient, in-procedure tuple buffers (compiled to
`hyde::rt::Vec`). `VectorKind` distinguishes their purpose: `kParameter`,
`kInductionInputs` / `kInductionSwaps` / `kInductionOutputs` (fixpoint
plumbing, printed as `$induction_in` / `$induction_swap` / `$induction_out`),
`kJoinPivots` and the inductive join/product variants, `kProductInput`,
`kTableScan`, `kMessageOutputs` (buffers `@differential` message publishes,
printed `$publish`), and `kEmpty` (a guaranteed-empty vector passed by the
initializer). Vectors may be sharded across workers (`IsSharded()`), and
appends can carry a worker-id computed by a `ProgramWorkerIdRegion`.

### The tuple state model

Tables never physically remove rows. Each row carries a `TupleState`:

- `kPresent` — the tuple is derivable and visible.
- `kAbsent` — the tuple is not in the relation (or was disproven).
- `kUnknown` — the tuple is *speculatively deleted*: one of its derivations
  was retracted, and it must be re-proven by alternate means before it can be
  used. (`kAbsentOrUnknown` is a from-state wildcard used in transitions.)

Differential correctness is encoded entirely in guarded state transitions:

- Bottom-up **addition** does `from absent|unknown to present`; the guarded
  body (successor propagation) executes only if the transition happens, so a
  tuple already present is not re-propagated.
- Bottom-up **removal** does `from present to unknown`, then propagates the
  possible removal to successors.
- Top-down **checking** (`kTupleFinder` procedures) inspects a tuple in
  `unknown` state and either re-proves it (`from unknown to present`) or
  finalizes the deletion (`from unknown to absent`).

This matches the runtime's `Table<Row>` 2-bit state field described in
`docs/RuntimeAndCodegen.md`: retraction and re-derivation are state flips.

## Region catalog

Every region is a subclass wrapped by `ProgramRegion`; most have an optional
`Body()` executed under the region's binding/condition. Names below are the
real classes from `Program.h` (internal builder names in parentheses, matching
the macro aliases used in `lib/ControlFlow/`).

**Structure**

- `ProgramSeriesRegion` (SERIES) — executes child regions strictly in order.
  Printed `seq`.
- `ProgramParallelRegion` (PARALLEL) — children have no mutual ordering
  constraints and may run concurrently (codegen may serialize them). Printed
  `par`.
- `ProgramLetBindingRegion` (LET) — assigns `UsedVariables()` to freshly
  `DefinedVariables()` scoped over its body; the IR's variable-renaming node.
- `ProgramModeSwitchRegion` (MODESWITCH) — marks that its body executes in a
  new `Mode`: `kBottomUpAddition` or `kBottomUpRemoval`. Procedures implicitly
  begin in addition mode; the innermost switch wins. Printed
  `mode-switch-to-add` / `mode-switch-to-remove`.
- `ProgramInductionRegion` (INDUCTION) — fixpoint loop; see below.

**Vector operations**

- `ProgramVectorLoopRegion` (VECTORLOOP) — iterates a vector, binding
  `TupleVariables()` per element; printed `vector-loop {...} over $vec`.
- `ProgramVectorAppendRegion` (VECTORAPPEND) — appends a tuple, optionally
  tagged with a worker id (`of-worker`).
- `ProgramVectorClearRegion` (VECTORCLEAR), `ProgramVectorUniqueRegion`
  (VECTORUNIQUE — sort and deduplicate), `ProgramVectorSwapRegion`
  (VECTORSWAP — exchange two vectors' contents).

**Table operations**

- `ProgramChangeTupleRegion` (CHANGETUPLE) — the guarded state transition: if
  the tuple's state matches `FromState()`, set it to `ToState()` and run
  `BodyIfSucceeded()`, else `BodyIfFailed()`. Printed
  `change-tuple {...} in %table:N[...] from S to S'` with `if-transitioned` /
  `if-failed` bodies.
- `ProgramChangeRecordRegion` (CHANGERECORD) — like CHANGETUPLE but also
  defines `RecordVariables()` naming the persisted record; both bodies see
  them.
- `ProgramCheckTupleRegion` (CHECKTUPLE) — reads a tuple's state and
  dispatches to up to three bodies: `IfPresent()`, `IfAbsent()`,
  `IfUnknown()`. This is the IR's existence test against a table.
- `ProgramCheckRecordRegion` (CHECKRECORD) — record-defining variant of
  CHECKTUPLE.
- `ProgramTableJoinRegion` (TABLEJOIN) — equi-join: loops over a
  `PivotVector()` and, per pivot, scans each joined table's index
  (`IndexedColumns(n)` / `SelectedColumns(n)` / `Index(n)`), binding
  `OutputPivotVariables()` and per-table `OutputVariables(n)` in the body.
  Printed `join-tables` with a `vector-loop` and one `select ... using
  %index ... where ...` per table.
- `ProgramTableProductRegion` (TABLEPRODUCT) — cross product: for each input
  vector, pairs its tuples against full contents of the other tables.
- `ProgramTableScanRegion` (TABLESCAN) — scans a table, optionally through an
  index constrained by `InputVariables()`, binding one output variable per
  table column in the body. Scans tolerate slightly-stale indexes, so the
  builder re-checks scan outputs with a TUPLECMP.

**Value tests and computation**

- `ProgramTupleCompareRegion` (TUPLECMP) — compares two variable tuples with
  a `ComparisonOperator`; `BodyIfTrue()` / `BodyIfFalse()`. Printed
  `if-compare {...} <op> {...}`.
- `ProgramGenerateRegion` (GENERATOR / GENERATE) — applies a `#functor`
  (`ParsedFunctor`) to `InputVariables()`, binding `OutputVariables()`;
  `BodyIfResults()` runs per generated tuple (or on filter success),
  `BodyIfEmpty()` runs when zero tuples are generated (functor negation).
- `ProgramTestAndSetRegion` (TESTANDSET) — atomic `(A += D) == C` /
  `(A -= D) == C` on a global accumulator; used for condition (`@cond`)
  reference counts and init guards. The body executes only if the comparison
  holds. Printed `@A += @D` (with `if (...)` when the result is tested).
- `ProgramWorkerIdRegion` (WORKERID) — hashes `HashedVariables()` into a
  16-bit `WorkerId()` used to shard vector appends. Printed
  `hash {...} into @N`.

**Control transfer**

- `ProgramCallRegion` (CALL) — calls another procedure with variable and
  vector arguments; dispatches on the boolean result to `BodyIfTrue()` /
  `BodyIfFalse()`. Condition tests on views compile to CALLs of
  `kConditionTester` procedures; re-proving compiles to CALLs of tuple
  finders.
- `ProgramReturnRegion` (RETURN) — `return-true` / `return-false`.
- `ProgramPublishRegion` (PUBLISH) — emits a `#message` with
  `VariableArguments()`; `IsRemoval()` marks the retraction variant of a
  `@differential` message.

## How building works (`lib/ControlFlow/Build/`)

`Program::Build` proceeds in phases:

1. **Data model** (`BuildDataModel`, `FillDataModel`): views are unioned into
   storage equivalence classes; tables are created ahead-of-time for
   everything that must be persisted — join inputs, negated views, condition
   participants, views that can receive deletions, inductive merges. Doing
   this up front prevents two successors from independently inventing state
   transitions on the same table that would make each other unsatisfiable.
2. **Bottom-up insertion paths**: `BuildEntryProcedure` starts a *work-list
   walk* from every message RECEIVE downward through the dataflow.
   `BuildEagerRegion` dispatches per node kind to the per-node build files —
   `Join.cpp` (`BuildEagerJoinRegion`), `Union.cpp`, `Induction.cpp`,
   `Negate.cpp`, `Product.cpp`, `Tuple.cpp`, `Compare.cpp`, `Generate.cpp`,
   `Insert.cpp`, `Select.cpp`. Nodes that need cross-branch coordination
   (joins, products, inductions) do not emit code immediately; they enqueue
   `WorkItem`s (`ContinueJoinWorkItem`, `ContinueInductionWorkItem`, ...)
   whose `order` field encodes priority: all joins of a frontier complete
   before the inductions they feed, and induction finalization at one depth
   precedes induction continuation at the next (`Build.h` documents the
   ordering). `CompleteProcedure` drains the work list.
3. **Per-message I/O procedures** (`BuildIOProcedure`) and the initializer
   (`BuildInitProcedure`), plus one `BuildQueryEntryPoint` per `#query`
   insert.
4. **Top-down checkers and bottom-up removers** are built to a fixpoint
   (`BuildTopDownCheckers` / `BuildBottomUpRemovalProvers` loop), since each
   can demand more of the other:
   - *Top-down checking* answers "is this tuple really present?" A checker
     procedure (`kTupleFinder`) for a view checks the view's own table state
     (via `BuildTopDownCheckerStateCheck`) and, on `unknown`, recursively
     calls the checkers of predecessors — join checker, union checker,
     negation checker, etc. (`BuildTopDownJoinChecker`,
     `BuildTopDownUnionChecker`, `BuildTopDownInductionChecker`, ...). If a
     checker holds only a subset of a view's columns, `BuildMaybeScanPartial`
     inserts a TABLESCAN to recover the rest. Success re-marks the tuple
     `present`; failure marks it `absent`.
   - *Removal propagation* (the differential path) mirrors insertion:
     `BuildEagerRemovalRegion` dispatches to `CreateBottomUp*Remover`
     functions. A removal first transitions the source tuple
     `present → unknown` (`InTryMarkUnknown`), then walks successors under a
     `MODESWITCH(kBottomUpRemoval)`, marking downstream tuples `unknown` and,
     where needed, scheduling top-down re-proving. `@differential` messages
     publish removals discovered this way.
5. **Extraction and cleanup**: `ExtractPrimaryProcedure` splits the entry
   procedure in two — the entry function keeps only the code reachable from
   message vectors up to the induction-vector appends, and the primary
   dataflow function (`^flow:`) receives those vectors and runs everything
   else. Procedures that don't end in a RETURN get `return-false` appended;
   `ProgramImpl::Optimize` runs (twice, around extraction) unless disabled.

## INDUCTION regions in depth (`Build/Induction.cpp`)

Recursive rules create cycles of `QueryMerge`s in the dataflow. Merges whose
cycles intersect form an *induction set* (`InductionSet` in `Build.h`); the
whole set is compiled into one `ProgramInductionRegion` so mutually recursive
unions reach fixpoint together. The region has three parts:

- **`Initializer()`** — the code that first appends tuples to the *induction
  vectors* (`Vectors()`, kind `kInductionInputs`). The builder finds the
  common ancestor of all initial appends and re-roots it as the init region.
- **`FixpointLoop()`** — always entered at least once and repeated while any
  induction vector is non-empty (`fixpoint-loop testing $vec, ...` in dumps).
  Each iteration, per inductive view: clear the swap vector, sort/unique the
  input vector, `vector-swap` input↔swap, then loop over the swap vector and
  run the view's inductive successors, which append newly derived tuples back
  into the input vectors. Inductive joins get pivot vectors
  (`kInductiveJoinPivots`) with the same clear/unique/swap discipline. When a
  merge can produce deletions, the loop body first does a CHECKTUPLE on the
  union's table and routes `unknown` entries through a
  `MODESWITCH(kBottomUpRemoval)` removal pass that runs before the addition
  pass, so the same vector carries both additions and retractions.
- **`Output()`** — after the fixpoint drains: sort/unique the output vectors
  (`kInductionOutputs`, filled during init and every iteration for views with
  non-inductive successors), loop over them pushing tuples to the rest of the
  dataflow (calling the top-down checker per tuple when deletions are
  possible), and clear all induction vectors.

**Termination invariant.** Every append to an induction *input* vector along
the inductive back edge must be dominated by a successful state transition
(CHANGETUPLE/CHANGERECORD) on the union's own table. Only a tuple newly
transitioned into the table can grow the fixpoint; a path that comes around
the cycle without transitioning the table (a pure forwarding cycle, e.g. the
dataflow of `p(A) : p(A).`) re-delivers rows that already fed an iteration,
and appending them again would spin the loop forever. `PathTransitionsTable`
in `Induction.cpp` enforces this by walking up from the append point: a
dominating CHANGETUPLE/CHANGERECORD on the table keeps the append; hitting
the induction's own cycle VECTORLOOP first drops it. Because the table
transition is `absent|unknown → present`, each tuple can feed the loop at
most once per derivation epoch, so generated fixpoints terminate.

## The optimizer (`lib/ControlFlow/Optimize.cpp`)

`ProgramImpl::Optimize` sweeps every region kind with a region-local
`OptimizeImpl` overload, deepest regions first so no-op removal "bubbles up",
repeating until a full pass changes nothing, then deduplicates procedures.
Each pass carries a doc comment with pseudocode and before/after diagrams:

- `OptimizeImpl(PARALLEL)` — flattening (single child elevated, nested
  PARALLEL spliced into parent), no-op child removal, and CSE across
  children: structurally identical children are deduplicated, children with
  equal root operations but different bodies are merged so the shared root
  executes once.
- `OptimizeImpl(SERIES)` — flattening, no-op removal, and unreachable-code
  truncation after a child through which every path returns.
- `OptimizeImpl(INDUCTION)` — drops no-op init/output regions and hoists the
  init region into a parent SERIES, exposing it to sibling optimization.
- `OptimizeImpl(LET)` — down-propagates variable assignments to eliminate the
  binding.
- `OptimizeImpl(TUPLECMP)` — simplifies comparisons (constant/duplicate
  operand elimination) so structurally-alike siblings merge in PARALLELs.
- `OptimizeImpl(MODESWITCH)` — deletes switches that re-assert the enclosing
  mode (procedures implicitly start in addition mode; the innermost switch
  wins).
- `OptimizeImpl(CALL / GENERATOR / CHECKTUPLE / CHECKRECORD / CHANGETUPLE /
  CHANGERECORD)` and a generic arm — prune no-op bodies; a body-less test
  becomes removable, feeding further bubbling.
- **Procedure deduplication** — used procedures other than
  initializer/entry/primary/message-handler/query-injector kinds are bucketed
  by structural `Hash`, compared with `Equals`, and duplicates are replaced
  via `ReplaceAllUsesWith`; raw references from `ProgramQuery` tuple checkers
  and forcing functions are rewritten to the representative.

`-disable-controlflow-opt` skips all of this; the built IR is then noticeably
deeper (long `seq`/`par` chains, un-merged parallel arms, duplicate checker
procedures).

## Known gaps

`Build.cpp` contains `assert(false && "TODO ...")` for dataflow shapes the
control-flow build does not yet lower; hitting them aborts a debug build:

- **Aggregates** (`QueryView::IsAggregate`) — no eager region, checker, or
  remover.
- **KV indices** (`IsKVIndex`, i.e. `mutable`-attributed parameters) — same.
- **Cross-products in removal paths** — `CreateBottomUpJoinRemover` only
  handles joins with pivots; a pivot-less join (product) in a differential
  path asserts (`"TODO: Cross-products!"`).
- **Impure functors in removal paths** — only pure functors have generate
  removers.

Corpus files exercising these: `data/examples/average_weight.dr`,
`pairwise_average_weight.dr`, `conditions_to_bools.dr`.

## Reading a program dump

`drlojekyll tc.dr -ir-out tc.ir` for transitive closure
(`reach(F,T) : edge(F,T).` / `reach(F,T) : reach(F,X), edge(X,T).`) prints,
excerpted:

```
proc ^flow:52($induction_in:20<u32,u32>, $induction_pivots:23<u32>)
  vector-define $induction_swap:21<u32,u32>
  ...
  seq
    @14 += @1                       ; TESTANDSET on the init-guard global
    ; set 0 depth 1                 ; region comment: induction set/depth
    induction
      empty-init                    ; Initializer() was optimized away
      fixpoint-loop testing $induction_in:20<u32,u32>, $induction_pivots:23<u32>
        par                         ; one arm per inductive view
          seq                       ; arm 1: the UNION's cycle
            vector-clear $induction_swap:21<u32,u32>
            vector-unique $induction_in:20<u32,u32>
            vector-swap $induction_in:20<u32,u32>, $induction_swap:21<u32,u32>
            vector-loop {@From:28, @To:29} over $induction_swap:21<u32,u32>
              par
                vector-append {@From:28, @To:29} into $induction_out:22<u32,u32>
                hash {@To:29} into @36          ; WORKERID over the pivot
                  vector-append {@To:29} into $induction_pivots:23<u32> of-worker @36
          seq                       ; arm 2: the inductive JOIN's cycle
            ...
            join-tables             ; TABLEJOIN driven by the pivot vector
              vector-loop {@X:39} over $induction_pivots_swap:24<u32>
              select {%col:6 as @From:44, %col:7 as @X:42} from %table:5[u32,u32]
                  using %index:40[_,u32] where %col:7 = @X:39
              select {%col:10 as @X:43, %col:11 as @To:45} from %table:9[u32,u32]
                  using %index:41[u32,_] where %col:10 = @X:39
                if-compare {@X:39, @X:39} = {@X:42, @X:43}   ; re-check scan results
                  if-true
                    change-tuple {@From:44, @To:45} in %table:5[u32,u32]
                        from absent to present   ; the dominating state transition
                      if-transitioned            ; only NEW tuples...
                        hash {@From:44, @To:45} into @37
                          vector-append {@From:44, @To:45}    ; ...feed the back edge
                              into $induction_in:20<u32,u32> of-worker @37
      output
        seq
          par
            vector-clear $induction_in:20<u32,u32>
            ...
    return-true
```

Notation: `@N` are variables (`@Name:N` when a Datalog name is known), `$name:N<types>`
are vectors, `%table:N[types]` / `%index:N[...]` / `%col:N` are storage, `^name:N`
are procedures, `;` starts a comment, and nesting depth is region containment.
The `_` positions in an index type (`%index:40[_,u32]`) mark non-key columns.
Note the invariant in the join arm: the `vector-append` on the induction input
vector sits under `if-transitioned` of a `change-tuple ... from absent to
present` on the union's table — the state transition dominates the back-edge
append, which is what bounds the fixpoint.
