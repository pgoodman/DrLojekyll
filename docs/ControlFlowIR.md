# Control-Flow IR ("Program" IR)

The control-flow IR is the imperative middle layer of the compiler. It is
built from the data-flow `Query` by `Program::Build(query, first_id, optimize)`
(`lib/ControlFlow/Build/Build.cpp`) and consumed by the C++ code generator
(`lib/CodeGen/CPlusPlus/Database.cpp`). Where the data-flow IR is a graph of
relational operators, the control-flow IR is a concrete program: a set of
procedures made of nested *regions* that implement **incremental maintenance**
of the dataflow — both the *addition* of tuples arriving in messages and the
*removal* (retraction) of tuples. Differential correctness rests on split
per-row *derivation counters* rather than a speculative tuple state: a row's
presence is the sign of its counters, a retraction is a signed counter fold,
and a tuple provable by another route survives the removal of one derivation
because its counter stays positive. The design rationale for this model is
`docs/proposals/StackSafeNegation.md`; this document describes the compiler as
it is.

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
  `VariableRole` (`kConstant`, `kInitGuard`, `kJoinPivot`, `kScanOutput`,
  `kFunctorOutput`, `kMessageOutput`, ...). The init guard is a global;
  everything else is defined by a region.
- **Procedures** (`ProgramProcedure`) — every procedure returns `true` or
  `false`, takes vector and variable parameters, and owns locally defined
  vectors.
- **Queries** (`ProgramQuery`) — glue records tying a `#query` declaration to
  its backing `table`, optional `index`, and optional `forcing_function`
  procedure. Code generators implement query cursors from these without the IR
  prescribing an implementation; a cursor filters rows through the table's
  `Present(id)` membership predicate.

### Procedure kinds (`ProcedureKind`)

There are exactly five kinds; there is no top-down tuple-finder procedure —
re-derivation is a counter read, not a recursive check.

| Kind | Dump prefix | Purpose |
|---|---|---|
| `kInitializer` | `^init:` | Runs once; starts flows rooted at constant tuples by calling the entry function with empty vectors. |
| `kEntryDataFlowFunc` | `^entry:` | Receives message add/remove vectors, does the initial hops (folding explicit message support into tables, parking zero-crossings in the delete/add queues), then calls the primary function. |
| `kPrimaryDataFlowFunc` | `^flow:` | Takes the queues collected by the entry function and executes the per-stratum differential phases (seeds, joins, crossovers, claim drains, D/R/I fixpoints) and the end-of-batch commit sweep. |
| `kMessageHandler` | `^receive:` | One per received `#message`; nets a batch's explicit adds against removes and calls the entry function. |
| `kQueryMessageInjector` | `^inject:` | Forcing function for a query: internally sends a message given the query's `bound` parameters. |

### Vectors (`DataVector`, `VectorKind`)

Vectors are transient, in-procedure tuple buffers (compiled to
`hyde::rt::Vec`). `VectorKind` distinguishes their purpose (printed name in
parentheses):

- Fixpoint plumbing: `kInductionInputs` (`$induction_in`), `kInductionSwaps`
  (`$induction_swap`), `kInductionOutputs` (`$induction_out`), `kJoinPivots`
  (`$pivots`) and the inductive join/product variants (`$induction_pivots`,
  `$induction_product`, ...), `kProductInput` (`$product`).
- `kTableScan` (`$scan`), `kParameter` (`$param`), and `kEmpty` (`$empty`, a
  guaranteed-empty vector the initializer passes).
- `kMessageOutputs` (`$publish`) buffers non-deletable `@differential`
  message publishes.
- The differential worklists a stratum's phases drain: `kDeleteQueue`
  (`$delete_queue`) and `kAddQueue` (`$add_queue`) hold the rows whose folds
  crossed zero this batch; `kOverdeleteSet` (`$overdelete`) and `kAdditionSet`
  (`$addition`) accumulate the batch's claimed overdeletion / addition rows;
  `kNetRemovals` (`$net_removals`) and `kNetAdditions` (`$net_additions`) are
  the consolidated net-presence-change frontiers that higher strata's seeds
  range over.
- A differential message's netted explicit ops: `kNetRemovals` / `kNetAdditions`
  are reused as the receive procedure's netted remove / add vectors.
- A recursive SCC's per-round claimed frontier (`Δ`): `kClaimedDeleteFrontier`
  (`$claimed_del`) and `kClaimedAddFrontier` (`$claimed_add`) hold just the
  rows claimed in the *current* claim-round of a D/R/I fixpoint — the loop's
  break condition and the round's delta source.

Vectors may be sharded across workers (`IsSharded()`), and appends can carry a
worker-id computed by a `ProgramWorkerIdRegion`.

### The row-state / counter model

Tables never physically remove rows. A row's presence is carried by *split
signed derivation counters*, not a discrete state. Two flavors of table share
one append-only row log (`docs/RuntimeAndCodegen.md`):

- **Monotone tables** (`Table<Row>`, `CanReceiveDeletions()` false) are
  insert-only: a stored row is present forever. Their only batch state is a
  *sealed row-id watermark* — row ids are append-ordered, so "present at batch
  start" (`InI`) is one id comparison. This lets a monotone table answer the
  same frozen-vs-current membership reads a differential table does when it
  sits at a read position of a delta join. `Seal()` advances the watermark in
  the table's commit sweep; a negated monotone table is auto-enrolled in
  sealing via its net-additions frontier.
- **Differential tables** (`DiffTable<Row>`, `CanReceiveDeletions()` true)
  keep two signed counters per row — `C_nr` (`kNonRecursive`: rule instances
  deriving the row from outside its recursive SCC, plus explicit message
  support) and `C_r` (`kRecursive`: instances arriving over an inductive
  back-edge) — plus a batch-transient flags byte (`RowFlags`): `kInI`
  (present at batch start, the frozen "I"), `kDel`/`kAdd` (claimed into the
  batch's overdeletion / addition set), `kDelNow`/`kAddNow` (claimed into the
  *current* fixpoint round), `kExplicit` (message support counted in `C_nr`),
  and `kTouched` (row changed this batch).

One received message batch is one epoch. Correctness lives in signed *counter
folds* and their **zero crossings**:

- Every rule-instance firing folds one `±1` on one counter (`UPDATECOUNT`),
  multiset-discipline: one fold per firing. A `+1` **crosses** when the row's
  total goes from `≤0` to `>0`; a `−1` crosses when the row was in-I and its
  post-fold `C_nr ≤ 0`. A crossing enqueues the row in the table's add or
  delete queue.
- Counters are signed and may dip below zero mid-batch (a phantom pair may
  fold its `−` before its `+`); non-negativity is asserted only at commit, per
  class.
- A crossing is only a *candidate*: the row is later **claimed** (`CLAIM`)
  into the batch's overdeletion / addition set, and the claim re-tests the
  crossing condition at claim time (a stale queue entry canceled by a later
  same-batch fold is dropped) and dedups, so each row is claimed once.
- **Membership predicates** (`MembershipPredicate`) are the named reads a
  `CHECKMEMBER` gate performs: `kInI` (frozen batch start), `kInNew`
  (batch-final-so-far), the fixpoint-round reads `kSurvivesSoFar` /
  `kAliveAtClaim` / `kInNewWithFrontier` / `kInNewSansFrontier`, `kPresent`
  (`C_nr + C_r > 0`), `kRecursivelySupported` (`C_r > 0`, the REDERIVE
  survival test), and the net-change frontiers `kNetDeleted` / `kNetAdded`.
  Every read of a differential table by a generated join names exactly one, so
  the frozen-vs-current read discipline is auditable in `-ir-out`.

The end-of-batch **commit sweep** (`COMMITSWEEP`) publishes each touched row's
net 0→1 / 1→0 presence change against the batch-start snapshot, seals the new
`kInI` snapshot, and clears the batch-scratch flags.

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
- `ProgramInductionRegion` (INDUCTION) — fixpoint loop; see below. Serves two
  roles: the monotone-recursion fixpoint (`Build/Induction.cpp`) and, as a
  bare vehicle, a recursive SCC's differential claim-round loop (its only
  maintained vectors are the per-round `Δ` frontiers).

**Vector operations**

- `ProgramVectorLoopRegion` (VECTORLOOP) — iterates a vector, binding
  `TupleVariables()` per element; printed `vector-loop {...} over $vec`.
- `ProgramVectorAppendRegion` (VECTORAPPEND) — appends a tuple, optionally
  tagged with a worker id (`of-worker`).
- `ProgramVectorClearRegion` (VECTORCLEAR), `ProgramVectorUniqueRegion`
  (VECTORUNIQUE — sort and deduplicate), `ProgramVectorSwapRegion`
  (VECTORSWAP — exchange two vectors' contents).

**Table folds and gates**

- `ProgramUpdateCountRegion` (UPDATECOUNT) — one signed `±1` derivation-counter
  fold of a tuple into `Table()`. `IsAdd()` picks the sign, `DerivationClass()`
  picks the counter (`kNonRecursive` = `C_nr`, `kRecursive` = `C_r`),
  `IsExplicit()` marks the `kExplicit` message-support fold. `Body()` executes
  only on a zero crossing (`if-crossed`). On a monotone table the fold
  degenerates to insert-if-new. Printed
  `update-count[-explicit] ±class {...} in %table:N[...]`.
- `ProgramChangeRecordRegion` (CHANGERECORD) — like UPDATECOUNT but also
  defines fresh `RecordVariables()` bound to the table-resident record, so the
  crossing body operates on the canonical stored values. Printed
  `{...} = update-count-record ±class {...} in %table:N[...]`.
- `ProgramCheckMemberRegion` (CHECKMEMBER) — a two-way membership gate reading
  one named `Predicate()` (a `MembershipPredicate`) of the tuple in `Table()`:
  `IfPresent()` runs when it holds, `IfAbsent()` when it does not. On a
  monotone table the gate is row existence. Printed
  `check-member <predicate> {...} in %table:N[...]` with `if-member` /
  `if-absent` bodies.
- `ProgramCheckRecordRegion` (CHECKRECORD) — record-defining variant of
  CHECKMEMBER; `RecordVariables()` are visible to both bodies. Printed
  `{...} = check-member-record <predicate> {...} in %table:N[...]`.
- `ProgramClaimRegion` (CLAIM) — claims a row of a differential `Table()` into
  the batch's overdeletion set (`IsDelete()`) or addition set, and into the
  current frontier round. `Body()` (`if-claimed`) runs only on the row's first
  claim this batch; a repeated claim is absorbed, and a stale claim (canceled
  by a later same-batch fold) is dropped. Printed `claim-del` / `claim-add`.
- `ProgramRetireRegion` (RETIRE) — clears one row's current-round frontier bit
  (`kDelNow`/`kAddNow`) so the next fixpoint round reads a clean frontier. No
  body. Printed `retire-del` / `retire-add`.
- `ProgramCommitSweepRegion` (COMMITSWEEP) — the end-of-batch sweep of one
  `Table()`: on a differential table it publishes each touched row's net 0/1
  presence change to `Message()` (when the table backs a `@differential`
  transmit), seals the new snapshot, and clears the scratch flags; on a
  monotone boundary table it advances the sealed watermark and carries no
  message. Printed `commit-sweep %table:N[...] [publishing name/arity]`.
- `ProgramNetBatchRegion` (NETBATCH) — nets a differential message's explicit
  adds against removes within one batch (set semantics with annihilation), in
  place on `AddVector()` / `RemoveVector()`. No body. Printed `net-batch`.

**Table joins, products, and scans**

- `ProgramTableJoinRegion` (TABLEJOIN) — equi-join: loops over a
  `PivotVector()` and, per pivot, scans each joined table's index
  (`IndexedColumns(n)` / `SelectedColumns(n)` / `Index(n)`), binding
  `OutputPivotVariables()` and per-table `OutputVariables(n)`. Besides the
  plain `Body()`, a delta join carries two *sections*: `AddedBody()` fires
  once per newly-holding combination (every side read at `kInNew`, at least
  one side a net addition) and `RemovedBody()` once per combination that
  stopped holding (every side at `kInI`, at least one a net deletion). Printed
  `join-tables` with a `vector-loop`, one `select ... using %index ... where
  ...` per table, and `added: all-in-new, some-net-added` /
  `removed: all-in-i, some-net-deleted` section headers.
- `ProgramTableProductRegion` (TABLEPRODUCT) — cross product: for each input
  vector, pairs its tuples against full contents of the other tables. Printed
  `cross-product`.
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
- `ProgramTestAndSetRegion` (TESTANDSET) — run-once guard on a global
  accumulator: `(A += 1) == 1`. Its sole producer is the entry procedure's
  init guard (`VariableRole::kInitGuard`), which makes the
  constant-initialization flows run exactly once. The body executes only on
  the first increment. Printed `if (@A += 1) == 1` (or `@A += 1` when the
  body was optimized away).
- `ProgramWorkerIdRegion` (WORKERID) — hashes `HashedVariables()` into a
  16-bit `WorkerId()` used to shard vector appends. Printed
  `hash {...} into @N`.

**Control transfer**

- `ProgramCallRegion` (CALL) — calls another procedure with variable and
  vector arguments; dispatches on the boolean result to `BodyIfTrue()` /
  `BodyIfFalse()`. The message handlers, entry, primary, and query forcing
  functions are wired together with CALLs.
- `ProgramReturnRegion` (RETURN) — `return-true` / `return-false`.
- `ProgramPublishRegion` (PUBLISH) — emits a `#message` with
  `VariableArguments()`; `IsRemoval()` marks the retraction variant of a
  `@differential` message. Deletion-capable `@differential` messages instead
  publish through their backing table's COMMITSWEEP; PUBLISH carries the
  non-deletable `$publish` buffered outputs.

## How building works (`lib/ControlFlow/Build/`)

`Program::Build` proceeds in phases:

0. **Feature-gap diagnostics** (`Program::Build`): a pre-pass (`Build.cpp`)
   walks `query.Aggregates()`, `KVIndices()`, impure `Maps()`, and pivot-less
   differential `Joins()`, reporting each to the `ErrorLog` and returning
   `std::nullopt` before any lowering. The per-node dispatch asserts still
   guard these kinds as internal backstops, but this pre-pass dominates them so
   the gaps surface as clean user diagnostics (F14).
1. **Data model** (`BuildDataModel`, `FillDataModel`): views are unioned into
   storage equivalence classes; tables are created ahead-of-time for
   everything that must be persisted — join inputs, negated views, views
   that can receive deletions, inductive merges. Doing this up front prevents
   two successors from independently folding into the same table in ways that
   would make each other unsatisfiable. `CanReceiveDeletions()` picks each
   table's flavor: a monotone `Table<Row>` or a differential `DiffTable<Row>`.
2. **Bottom-up ingest walk**: `BuildEntryProcedure` starts a *work-list walk*
   from every message RECEIVE downward through the dataflow.
   `BuildEagerRegion` dispatches per node kind to the per-node build files —
   `Join.cpp` (`BuildEagerJoinRegion`), `Union.cpp`, `Induction.cpp`,
   `Negate.cpp`, `Product.cpp`, `Tuple.cpp`, `Compare.cpp`, `Generate.cpp`,
   `Insert.cpp`, `Select.cpp`. The walk **cuts** at any `CanReceiveDeletions()`
   successor: the eager path folds explicit message support and monotone
   derivations, parking every zero crossing in a table's delete/add queue, and
   hands the differential subgraph to the stratum phases. Nodes that need
   cross-branch coordination (joins, products, inductions) enqueue `WorkItem`s
   (`ContinueJoinWorkItem`, `ContinueInductionWorkItem`, ...) whose `order`
   field encodes priority (`Build.h` documents the ordering);
   `CompleteProcedure` drains the work list.

   *Unit gates*: a joined or negated view backed by a unit (condition) table
   never multiplies cardinality — its only possible row is `(true)` — so it
   is not lowered as a scan source. Join lowering (`Build/Join.cpp`) excludes
   unit sides from the emitted TABLEJOIN's scan arms and gates on a CHECKMEMBER
   of the unit table instead; negation lowering (`Build/Negate.cpp`) gates on a
   position-keyed CHECKMEMBER of the negated table — a `!` negate reads it
   batch-frozen (`kInI`), a `@never` negate reads `kPresent` and never
   retracts (so it gets no crossover and keeps its present row).
3. **Per-message I/O procedures** (`BuildIOProcedure`) and the initializer
   (`BuildInitProcedure`), plus one query entry point per `#query`.
4. **Per-stratum differential phases** (`BuildStratumPhases`, `Build/Stratum.cpp`):
   the retraction/re-derivation machinery. See the stratum-phases section
   below; in brief, for each stratum in ascending order the builder emits the
   **seeds** (frontier-vector loops walking delta chains, and dual-section
   TABLEJOINs), the **negation crossovers** (`Build/Negate.cpp`'s forward gate
   plus `Stratum.cpp`'s D1′ dual), the **claim drains** of the stratum's
   differential tables (delete/add queues → overdeletion/addition sets), the
   recursive-SCC **OVERDELETE → REDERIVE → INSERT** claim-round fixpoints, and
   the **net-frontier filters** higher strata's seeds range over.
5. **Extraction, publish, and cleanup**: `ExtractPrimaryProcedure` splits the
   entry procedure in two — the entry function keeps the ingest walk up to the
   queue appends, and the primary dataflow function (`^flow:`) receives the
   queues and runs the stratum phases. `BuildProcedure` closes `^flow:` with
   the **publish split**: non-deletable `@differential` outputs flush from
   their `$publish` (`kMessageOutputs`) vectors through PUBLISH regions, then
   one COMMITSWEEP per differential table (and per monotone boundary table)
   seals the epoch — a table backing a deletion-capable `@differential`
   transmit publishes its net presence changes through its sweep. Procedures
   without a trailing RETURN get one appended; `ProgramImpl::Optimize` runs
   (unless disabled).

## INDUCTION regions in depth (`Build/Induction.cpp`)

Recursion is lowered two ways, and the split is load-bearing: read the code to
tell which path a given cycle takes.

- **Monotone recursion** (`Build/Induction.cpp`) — cycles of `QueryMerge`s
  none of whose members can receive deletions. Merges whose cycles intersect
  form an *induction set* (`InductionSet` in `Build.h`) compiled into one
  `ProgramInductionRegion` so mutually recursive unions reach fixpoint
  together; described below.
- **Differential recursion** (`Build/Stratum.cpp`) — a recursive SCC of
  deletion-capable tables. Its OVERDELETE / REDERIVE / INSERT is emitted by the
  stratum phases (see the next section), not by `Induction.cpp`. Even a
  linear recursion whose views carry an `InductionGroupId` but for which no
  induction region was built (transitive closure, whose recursion terminates at
  a JOIN) is stratum-phase-owned; `TableIsInductionOwned` draws the line. Each
  claim-round loop is a bare `ProgramInductionRegion` whose only vectors are
  its per-round `Δ` frontiers.

A monotone `ProgramInductionRegion` has three parts:

- **`Initializer()`** — the code that first appends tuples to the *induction
  vectors* (`Vectors()`, kind `kInductionInputs`). The builder finds the
  common ancestor of all initial appends and re-roots it as the init region.
- **`FixpointLoop()`** — always entered at least once and repeated while any
  induction vector is non-empty (`fixpoint-loop testing $vec, ...` in dumps).
  Each iteration, per inductive view: clear the swap vector, sort/unique the
  input vector, `vector-swap` input↔swap, then loop over the swap vector and
  run the view's inductive successors, which append newly derived tuples back
  into the input vectors. Inductive joins get pivot vectors
  (`kInductiveJoinPivots`) with the same clear/unique/swap discipline.
- **`Output()`** — after the fixpoint drains: sort/unique the output vectors
  (`kInductionOutputs`, filled during init and every iteration for views with
  non-inductive successors), loop over them pushing tuples to the rest of the
  dataflow, and clear all induction vectors.

**Termination invariant.** Every append to an induction *input* vector along
the inductive back edge must be dominated by a zero-crossing counter fold
(UPDATECOUNT / CHANGERECORD) on the union's own table. Only a tuple whose fold
crossed — newly present in the table — can grow the fixpoint; a path that comes
around the cycle without folding the table (a pure forwarding cycle, e.g. the
dataflow of `p(A) : p(A).`) re-delivers rows that already fed an iteration, and
appending them again would spin the loop forever. `PathTransitionsTable` in
`Induction.cpp` enforces this by walking up from the append point: a dominating
UPDATECOUNT or CHANGERECORD on the table keeps the append; hitting the
induction's own cycle VECTORLOOP first drops it. Because the crossing body runs
only on the `≤0 → >0` transition, each tuple can feed the loop at most once per
derivation epoch, so generated fixpoints terminate.

## Stratum phases in depth (`Build/Stratum.cpp`)

The differential machinery. The ingest walk leaves every zero-crossing parked
in some table's delete/add queue; `BuildStratumPhases` drains those queues and
propagates the changes stratum by stratum. A *scheduling fixpoint* lifts each
emission until every state it reads is phase-final (a data model's views can
straddle strata, so spec strata alone do not guarantee it), then, for each
stratum in ascending order, emits into the `^flow:` procedure:

- **Seeds.** For each *branch chain* — a delta path from a source table's
  member view to the first table-backed head or first JOIN — a `vector-loop`
  over the source's net-removal / net-addition frontier walks the chain's
  per-node plumbing (variable mapping, a TUPLECMP for a CMP, a generator for a
  MAP, a position-keyed CHECKMEMBER gate for a NEGATE) and ends in an
  UPDATECOUNT head fold or a pivot append into a join's delta pivot vector.
- **Dual-section joins.** One TABLEJOIN per join view over a sort-uniqued
  pivot vector, its `added:` and `removed:` sections folding into the join's
  persisted table.
- **Negation crossovers** (D1′, `Build/Negate.cpp` comments + `EmitCrossover`).
  The dual of the forward negate gate: when the negated key crosses, the SAME
  negate row is re-folded, sign-dualized — a negated-view gain retracts the
  negate output (the `−` arm over the negated table's net-additions frontier),
  a negated-view loss re-derives it (the `+` arm over net-removals, only when
  the negated table is differential). One arm-pair per non-`@never` negate,
  emitted in the seed block before the claim drains.
- **Claim drains.** `EmitClaimDrain` sort-uniques a table's delete/add queue
  and CLAIMs each crossed row into the batch overdeletion (`$overdelete`) /
  addition (`$addition`) set — a counter crossing is a candidate, the claim
  gate confirms it.
- **Net-frontier filters.** `EmitFrontierFilter` copies each claimed row that
  is a genuine net presence change (`kNetDeleted` / `kNetAdded`) into the
  consolidated `$net_removals` / `$net_additions` frontier the next strata's
  seeds range over. A row overdeleted and re-added this batch passes neither.

A **recursive-SCC** table's OVERDELETE and INSERT are claim-round *fixpoints*
(`build_claim_round_loop`), each a bare INDUCTION whose per-round `Δ` frontier
(`$claimed_del` / `$claimed_add`) is cleared, filled by a claim drain, fired
(`EmitJoinFire` re-fires each recursive JOIN delta over `Δ`; projection edges
re-seed over `Δ`), and retired each round; the loop breaks when a round claims
nothing. Between them, **REDERIVE** (`EmitRederive`) re-enters every overdeleted
row still recursively supported (`kRecursivelySupported`, `C_r > 0`) via the
add queue. Both signed net frontiers are consolidated only after INSERT
quiesces, so a re-added row never leaks into `$net_removals`.

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
- `OptimizeImpl(CALL / GENERATOR / UPDATECOUNT / CHANGERECORD / CLAIM /
  CHECKMEMBER / CHECKRECORD)` and a generic arm — prune no-op bodies. A
  CHECKMEMBER / CHECKRECORD whose both bodies are gone becomes a dead pure read
  and is replaced with an empty LET; an UPDATECOUNT / CHANGERECORD / CLAIM is
  never removed (the fold or claim mutates the table), only its crossing /
  claimed body is dropped.
- **Procedure deduplication** — used (or raw-used) procedures other than
  initializer/entry/primary/message-handler/query-injector kinds are bucketed
  by structural `Hash`, compared with `Equals`, and duplicates are replaced
  via `ReplaceAllUsesWith`; raw references from `ProgramQuery` forcing
  functions are rewritten to the representative.

`-disable-controlflow-opt` skips all of this; the built IR is then noticeably
deeper (long `seq`/`par` chains, un-merged parallel arms).

## Known gaps

`Program::Build`'s pre-pass (`Build.cpp`) reports the dataflow shapes the
control-flow build does not yet lower as clean `ErrorLog` diagnostics and
returns `std::nullopt` (F14); the per-node `assert(false && "TODO ...")` arms
remain only as internal backstops the pre-pass dominates.

- **Aggregates** (`QueryView::IsAggregate`) and **KV indices** (`IsKVIndex`,
  `mutable`-attributed parameters) — design recorded in
  `docs/proposals/AggregatingFunctors.md`, gated on the delta-relational IR.
- **Differential cross-products** — a pivot-less JOIN over deletable data;
  Stage 5 of `docs/proposals/StackSafeNegation.plan.md`.
- **Impure functors** — only pure functors are lowered.

Unstratified negation — a negated predicate recursively derived from the
negation's own result — is rejected earlier, by the dataflow Stratify pass, in
all modes. Corpus files exercising these: `data/examples/average_weight.dr`,
`pairwise_average_weight.dr` (KV indices), `conditions_to_bools.dr`
(differential cross-product), `data/self_testing_examples/evm_func_parse.dr`
(unstratified negation).

## Reading a program dump

`drlojekyll tests/OptDiff/cases/transitive_closure_diff2.dr -ir-out tc.ir`
(`tc(F,T) : edge(F,T).` / `tc(F,T) : edge(F,X), tc(X,T).`, over an
`@differential add_edge` message) prints the whole differential shape. The
entry procedure folds explicit message support and parks crossings:

```
proc ^entry:21($param:23<u64,u64>, $param:24<u64,u64>)   ; add vec, remove vec
  seq
    par
      vector-loop {@From:26, @To:27} over $param:23<u64,u64>
        update-count-explicit +nonrecursive {@From:26, @To:27} in %table:4[u64,u64]
          if-crossed                                     ; edge became present...
            vector-append {@From:26, @To:27} into $add_queue:28<u64,u64>
      vector-loop {@From:30, @To:31} over $param:24<u64,u64>
        update-count-explicit -nonrecursive {@From:30, @To:31} in %table:4[u64,u64]
          if-crossed
            vector-append {@From:30, @To:31} into $delete_queue:32<u64,u64>
    call ^flow:205($add_queue:28<u64,u64>, $delete_queue:32<u64,u64>)
```

The primary function drains the queues, seeds the recursion, runs the
claim-round OVERDELETE fixpoint, and commits (excerpted):

```
  seq
    @22 += 1                                             ; init-guard TESTANDSET
    vector-loop {@40, @41} over $add_queue:28<u64,u64>
      claim-add {@40, @41} in %table:4[u64,u64]          ; crossing -> addition set
        if-claimed
          vector-append {@40, @41} into $addition:38<u64,u64>
    vector-loop {@48, @49} over $addition:38<u64,u64>
      check-member net-added {@48, @49} in %table:4[u64,u64]   ; net frontier
        if-member
          vector-append {@48, @49} into $net_additions:46<u64,u64>
    ...                                                  ; seed joins feed $pivots
    induction                                            ; OVERDELETE claim-round loop
      fixpoint-loop testing $claimed_del:83<u64,u64>, ...
        seq
          vector-clear $claimed_del:83<u64,u64>
          vector-loop {@88, @89} over $delete_queue:77<u64,u64>
            claim-del {@88, @89} in %table:8[u64,u64]
              if-claimed
                seq
                  vector-append {@88, @89} into $overdelete:86<u64,u64>
                  vector-append {@88, @89} into $claimed_del:83<u64,u64>   ; this round's Δ
          ...                                            ; fire recursive rules delta over Δ
          vector-loop {@114, @115} over $claimed_del:83<u64,u64>
            retire-del {@114, @115} in %table:8[u64,u64]
      output
        vector-loop {@124, @125} over $overdelete:86<u64,u64>
          check-member recursively-supported {@124, @125} in %table:8[u64,u64]  ; REDERIVE
            if-member
              vector-append {@124, @125} into $add_queue:81<u64,u64>
    ...                                                  ; INSERT loop, frontier filters
    commit-sweep %table:8[u64,u64] publishing found_path/2
    return-true
```

Notation: `@N` are variables (`@Name:N` when a Datalog name is known),
`$name:N<types>` are vectors, `%table:N[types]` / `%index:N[...]` / `%col:N`
are storage, `^name:N` are procedures, `;` starts a comment, and nesting depth
is region containment. `_` positions in an index type mark non-key columns.
Note the counter discipline: an `update-count` fires its `if-crossed` body only
on a zero crossing (parking the row in a queue), a `claim-del`/`claim-add`
confirms the crossing and records the row into both the accumulated
overdeletion/addition set and this round's `Δ` frontier, and a `check-member
recursively-supported` re-enters an overdeleted row iff `C_r > 0`. The
back-edge `vector-append` onto a queue sits under an `if-crossed`, which is what
bounds the fixpoint.
