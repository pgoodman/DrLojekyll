# Architecture

Dr. Lojekyll is a Datalog compiler (C++ namespace `hyde`). It compiles a
Datalog dialect into an *incremental, message-driven C++ database*: the
generated code is a `Database` class whose inputs are messages (vectors of
tuples to add or remove), whose derived relations are updated bottom-up and
differentially on each message, and whose outputs are queries (cursors over
materialized tables) and published messages. Evaluation is bottom-up with
differential updates: a received tuple flows forward through joins, maps,
unions, and negations, flipping per-row states (`absent`/`present`/`unknown`)
in persistent tables rather than recomputing relations from scratch.

The language itself (declarations such as `#message`, `#query`, `#local`,
`#functor`, clause syntax, pragmas) is documented in `docs/Language.md`, with
the concrete grammar in `docs/Grammar.md`.

## Pipeline

```
 .dr source files
       |
       v
  DisplayManager / Lexer            lib/Display, lib/Lex
       |   (tokens)
       v
  Parser  ------------------------- lib/Parse
       |   hyde::ParsedModule       (AST; round-trips through the formatter)
       v
  Query::Build  ------------------- lib/DataFlow
       |   hyde::Query              (data-flow graph of relational operators)
       |     +- QueryImpl::Optimize   (CSE, canonicalization, dead-flow elim)
       v
  Program::Build  ----------------- lib/ControlFlow/Build
       |   hyde::Program            (region tree: procedures, loops, joins)
       |     +- ProgramImpl::Optimize (flattening, no-op removal, proc dedup)
       v
  cxx::GenerateDatabaseCode  ------ lib/CodeGen/CPlusPlus/Database.cpp
       |   <db>.h + <db>.cpp        (header-only C++; .cpp is an anchor TU)
       v
  user driver + runtime  ---------- include/drlojekyll/Runtime, lib/Runtime
```

Every stage after parsing consumes the previous stage's public handle type and
returns `std::nullopt` on error, reporting diagnostics through a shared
`hyde::ErrorLog`. The CLI driver (`bin/drlojekyll/Main.cpp`) is a thin wiring
of these calls plus output-file plumbing.

## The def-use substrate (`include/drlojekyll/Util`)

Every IR in the compiler — the parse tree, the data-flow graph, and the
control-flow region tree — is built on one def-use library,
`include/drlojekyll/Util/DefUse.h`, plus the pImpl wrapper in
`include/drlojekyll/Util/Node.h`. Understanding these two headers first makes
every other header legible.

- **`Def<T>` / `Use<T>` / `User`.** A `Def<T>` is mixed (CRTP-style) into an
  IR node class `T` and owns the list of `Use<T>` records pointing back at it.
  A `User` is anything that holds uses; whenever a use is created, moved, or
  destroyed, the user's virtual `Update(timestamp)` is invoked (timestamps
  come from the global counter `User::gNextTimestamp`), which IR nodes use to
  invalidate cached hashes/depths and mark themselves non-canonical.
- **`Def<T>::ReplaceAllUsesWith(T *)`** is the workhorse of every
  optimization pass: it migrates all uses to another def and notifies the
  affected users. `ReplaceUsesWithIf` does the same for a filtered subset.
- **`UseList<T>` / `UseRef<T>`.** A `UseList` is an owned, ordered list of
  uses (e.g. a JOIN's input columns); it supports `Sort`, `Unique`,
  `RemoveIf`, and `Swap`. A `UseRef` is a single-slot use (e.g. "the body of
  this region"). `WeakUseList`/`WeakUseRef` variants observe a def without
  keeping it alive; a deleted def nulls its weak uses.
- **`DefList<T>`** owns the nodes themselves (`Create`, `CreateDerived`,
  `RemoveUnused`, `RemoveIf`). Each IR's impl object holds one `DefList` per
  node kind, so "delete everything unreachable" is `RemoveUnused()` per list.
- **`Node<PublicT, PrivateT>`** (`Util/Node.h`) is a two-word value type
  wrapping a raw impl pointer. All public API classes — `QueryJoin`,
  `ProgramSeriesRegion`, `ParsedClause`, ... — derive from it and are passed
  by value; the mutable graph lives in the private `*Impl` classes.
  `DefinedNodeRange<T>` / `UsedNodeRange<T>` (in `DefUse.h`) adapt the private
  def/use lists into iterable ranges of these public values, which is why
  public headers expose e.g. `DefinedNodeRange<QueryJoin> Joins() const`.

## Stage 1: displays and lexing (`lib/Display`, `lib/Lex`)

`hyde::DisplayManager` (`include/drlojekyll/Display/DisplayManager.h`) owns
every input "display" (file, buffer, or stream) and maps `DisplayPosition`
values back to file/line/column for diagnostics. `hyde::OutputStream`
(`include/drlojekyll/Display/Format.h`) is the formatting wrapper used by all
`Format.h` pretty-printers (tokens, parsed modules, data-flow, control-flow).

`hyde::Lexer` (`include/drlojekyll/Lex/Lexer.h`) reads a display via
`ReadFromDisplay(const DisplayReader &)` and yields `hyde::Token` values one
at a time through `TryGetNextToken(const StringPool &, Token *)`. Tokens carry
a `Lexeme` kind plus display positions; identifiers are interned in a
`StringPool`. Lexical errors are ordinary tokens with error lexemes, so the
parser owns all diagnostics.

## Stage 2: parsing (`lib/Parse`)

Entry point: `hyde::Parser` (`include/drlojekyll/Parse/Parser.h`).

```cpp
Parser parser(display_manager, error_log);
std::optional<ParsedModule> m = parser.ParsePath(path, display_config);
// also: ParseBuffer(std::string_view, ...), ParseStream(std::istream &, ...)
```

The result is a `hyde::ParsedModule` (`include/drlojekyll/Parse/Parse.h`): a
shared handle to the root of the AST. From it hang `ParsedDeclaration`s
(queries, messages, locals, exports, functors), `ParsedClause`s with their
`ParsedPredicate`/`ParsedComparison`/`ParsedAssignment`/`ParsedAggregate`
bodies, `#import`s, `#inline` blocks, and foreign/enum type definitions.
`#import` resolution uses the search paths added with
`Parser::AddModuleSearchPath`; `ParsedModuleIterator` walks a module and its
transitive imports. Errors accumulate in `hyde::ErrorLog`
(`include/drlojekyll/Parse/ErrorLog.h`), which renders positioned diagnostics.

The parser establishes name resolution and semantic checks (arity/type
agreement across redeclarations, binding-mode legality, variable usage), so
later stages assume a well-formed module. The AST pretty-printer
(`Parse/Format.h`) emits parseable Datalog; debug builds of the driver
round-trip every module through print→parse→print and assert a fixpoint.

## Stage 3: data-flow IR (`lib/DataFlow`)

Entry point (`include/drlojekyll/DataFlow/Query.h`):

```cpp
static std::optional<Query> Query::Build(const ParsedModule &module,
                                         const ErrorLog &log,
                                         bool optimize = true);
```

`hyde::Query` is a graph of relational-algebra *views*, each a node class
deriving from `Node<..., ...Impl>`: `QuerySelect` (relation/stream sources),
`QueryTuple` (projection/forwarding), `QueryJoin`, `QueryMerge` (union),
`QueryCompare`, `QueryMap` (functor application), `QueryNegate`,
`QueryAggregate`, `QueryKVIndex`, and `QueryInsert` (sinks into relations or
published messages). Views produce `QueryColumn`s; zero-arity predicates
(conditions) desugar into ordinary *unit relations* -- 1-column `bool`
relations whose only possible row is `(true)`, flagged
`QueryRelation::IsCondition` -- set by INSERTs and tested by joins and
negations. `QueryIO` nodes represent message endpoints. The private node
classes live in `lib/DataFlow/Query.h`.

`Query::Build` (`lib/DataFlow/Build.cpp`) translates each `ParsedClause` into
a sub-graph (`BuildClause`), merges multi-clause predicates with `QueryMerge`,
then runs a fixed post-pass sequence: simplification and canonicalization,
linking inserts to selects, and — when `optimize` is true —
`QueryImpl::Optimize` (`lib/DataFlow/Optimize.cpp`), which interleaves CSE
over structurally equal views, a canonicalization fixpoint driven by
`OptimizationContext` flags, and dead-flow elimination.
Whether optimized or not, `Build` finishes by identifying inductive cycles
(`IdentifyInductions`; exposed as `QueryView::InductionGroupId`/
`InductionDepth`), finalizing view depths and column IDs, tracking which flows
need differential (removal) support, running forwards/backwards taint
analysis, and computing view equivalence sets that later decide which views
share a persistent table.

Invariants on the emitted graph: every view is canonical; no view is its own
direct user (asserted in `RelabelGroupIDs`); inserts are proxied by tuples;
inductive unions are labeled with group and depth. Each optimization pass in
`lib/DataFlow` carries a doc comment with its algorithm, pseudocode, and an
ASCII before/after diagram. See `docs/DataFlowIR.md` for the per-node
deep dive.

## Stage 4: control-flow IR (`lib/ControlFlow`)

Entry point (`include/drlojekyll/ControlFlow/Program.h`):

```cpp
static std::optional<Program> Program::Build(const Query &query,
                                             unsigned first_id = 0,
                                             bool optimize = true);
```

`hyde::Program` lowers the data-flow graph to an imperative *region tree*.
Data definitions come first: `BuildDataModel`/`FillDataModel`
(`lib/ControlFlow/Build/Build.cpp`) assign a persistent `DataTable` (with
`DataColumn`s and `DataIndex`es) to each equivalence set of views that needs
backing storage, plus `DataVariable` constants/globals and per-procedure
`DataVector`s for in-flight tuples.

Code is a set of `ProgramProcedure`s (kinds in `ProcedureKind`: initializer,
entry data-flow function, primary data-flow function, message handlers,
top-down checkers, bottom-up removers, forcing functions), each a tree of
`ProgramRegion` subclasses: structuring regions (`ProgramSeriesRegion`,
`ProgramParallelRegion`, `ProgramInductionRegion`, `ProgramLetBindingRegion`,
`ProgramModeSwitchRegion`) and operations (`ProgramTableJoinRegion`,
`ProgramTableScanRegion`, `ProgramTableProductRegion`,
`ProgramChangeTupleRegion`/`ProgramCheckTupleRegion` (table state
transitions), vector append/loop/swap/unique/clear, `ProgramTupleCompareRegion`,
`ProgramGenerateRegion` (functor calls), `ProgramCallRegion`,
`ProgramPublishRegion`, `ProgramReturnRegion`, ...). `ProgramVisitor`
dispatches over the concrete kinds. Inductive cycles in the data-flow graph
become `ProgramInductionRegion` fixpoint loops over swap-vectors.

`Program::Build` builds message-driven entry procedures, the initialization
procedure (constant-fed flows), query entry points (`ProgramQuery` records:
table, optional index, optional tuple-checker/forcing procedures), then
iterates `BuildTopDownCheckers` / `BuildBottomUpRemovalProvers` to a fixpoint
so that differential removal is supported wherever the data-flow stage
demanded it. When `optimize` is true, `ProgramImpl::Optimize`
(`lib/ControlFlow/Optimize.cpp`) runs per-region `OptimizeImpl` overloads
deepest-first (parallel/series flattening, mode-switch and no-op removal, LET
propagation) and deduplicates identical procedures.

Invariants: every procedure ends with a return region; every variable knows
its defining region (`MapVariables`); every inductive back-edge append is
dominated by a state transition on the union's table, which is what makes the
generated fixpoints terminate. See `docs/ControlFlowIR.md` for region
semantics in depth.

## Stage 5: C++ code generation (`lib/CodeGen`)

Entry point (`include/drlojekyll/CodeGen/CodeGen.h`):

```cpp
void hyde::cxx::GenerateDatabaseCode(const Program &program,
                                     OutputStream &os_h, OutputStream &os_cc,
                                     std::string_view header_name);
```

Implemented in `lib/CodeGen/CPlusPlus/Database.cpp`, this walks the program's
tables, procedures, and queries and emits two files: `<db>.h` carries the whole
artifact — row/key structs, tuple aliases, enums, `DatabaseFunctors`,
`DatabaseLog`, the sealed `Database` state struct with its hidden-friend entry
points, and the templated flow-procedure definitions — while `<db>.cpp` is an
anchor TU (banner plus `#include "<db>.h"`). The output is header-only and
heavily templated (the entry points deduce the driver's log/functors types);
every region kind maps to a small, readable statement pattern. `docs/RuntimeAndCodegen.md`
describes the generated-code shape and the runtime containers
(`Allocator.h`, `Hash.h`, `Vec.h`, `Table.h` under
`include/drlojekyll/Runtime`; the only compiled runtime source is
`lib/Runtime/Allocator.cpp`).

## The CLI driver (`bin/drlojekyll/Main.cpp`)

`main` builds a `DisplayManager`, `ErrorLog`, and `Parser`; parses one module
(or an amalgamation `#import`ing every input path); and calls the pipeline.
Output flags, each independent:

| Flag | Output |
| --- | --- |
| `-cpp-out <DIR>` | Generated `<db>.h` and `<db>.cpp` in `DIR` (name from `#database`, default `datalog`) |
| `-ir-out <PATH>` | Textual control-flow IR (`ControlFlow/Format.h`) |
| `-dot-out <PATH>` | Data-flow graph as GraphViz DOT (emitted after the control-flow build so views carry their table IDs) |
| `-dr-out <PATH>` | Single-file Datalog amalgamation of all inputs and imports |
| `-first-id <N>` | Starting integer for control-flow IR IDs (for combining multiple generated headers) |
| `-M <PATH>` | Module search path for `#import` |
| `-disable-dataflow-opt` | Skip `QueryImpl::Optimize` |
| `-disable-controlflow-opt` | Skip `ProgramImpl::Optimize` |

## End to end: from `.dr` to a running database

`path.dr`:

```datalog
#message edge(i32 From, i32 To).
#local path(i32 From, i32 To).
#query reachable(bound i32 From, free i32 To).

path(A, B) : edge(A, B).
path(A, C) : path(A, B), edge(B, C).
reachable(A, B) : path(A, B).
```

`drlojekyll path.dr -cpp-out gen/` emits `gen/datalog.h` and
`gen/datalog.cpp`. The header declares (among other things):

```cpp
using edge_input = Tup_i32_i32;
struct Database {
 public:
  explicit Database(::hyde::rt::Allocator);   // allocates empty tables; no epoch 0
  // Driver-facing functions are HIDDEN FRIENDS, reached by unqualified (ADL)
  // call with the database argument -- e.g. init(db, log, functors),
  // edge_2(db, log, functors, vec), reachable_bf(db, From). A qualified call
  // does not compile. Tables/indices/globals + initialized_ are private.
};
```

A driver (same shape as `tests/OptDiff/cases/*.main.cpp`):

```cpp
#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;   // one member fn per #functor; none here
  DatabaseLog log;             // one no-op method per published message
  Database db(allocator);      // construction cannot fail; runs no epoch 0
  init(db, log, functors);     // epoch 0: empty-EDB least model + t=0 deltas

  hyde::rt::Vec<edge_input> edges(allocator);
  edges.Add({1, 2});
  edges.Add({2, 3});
  edge_2(db, log, functors, std::move(edges));  // drives the incremental update

  auto c = reachable_bf(db, 1);      // b=bound, f=free per query column
  for (int32_t to; c.next(to);) { /* 2, 3 */ }
}
```

Build: compile `gen/datalog.cpp` and the driver with `-I include -I gen`,
link `lib/Runtime/Allocator.cpp` (or the `DrLojekyll::Runtime` CMake target).
In-tree, `compile_datalog()` from `cmake/Compiler.cmake` runs the compiler at
build time and wraps the result in a static library; `tests/PointsTo` and
`tests/MiniDisassembler` are complete examples. Always read the generated
header for exact signatures — names encode arity and binding patterns.

## Where to look

| Task | Location |
| --- | --- |
| Language syntax, tokens | `docs/Grammar.md`, `lib/Lex`, `lib/Parse/Parser.cpp` |
| AST shape, semantic checks | `include/drlojekyll/Parse/Parse.h`, `lib/Parse` |
| Add/inspect a data-flow node or pass | `include/drlojekyll/DataFlow/Query.h`, `lib/DataFlow` (`Optimize.cpp` driver, per-node `Canonicalize`) |
| Region semantics, lowering from data-flow | `include/drlojekyll/ControlFlow/Program.h`, `lib/ControlFlow/Build`, `lib/ControlFlow/Optimize.cpp` |
| Generated C++ shape | `lib/CodeGen/CPlusPlus/Database.cpp`, `docs/RuntimeAndCodegen.md` |
| Runtime containers | `include/drlojekyll/Runtime`, `lib/Runtime/Allocator.cpp` |
| Def-use plumbing shared by all IRs | `include/drlojekyll/Util/DefUse.h`, `Util/Node.h` |
| CLI flags and driver flow | `bin/drlojekyll/Main.cpp` |
| End-to-end and differential-optimization tests | `tests/`, `tests/OptDiff/diffrun.sh`, `data/examples` |
| Diagnostics and source positions | `include/drlojekyll/Parse/ErrorLog.h`, `lib/Display` |

## Related documents

- `docs/Language.md` — the Datalog dialect: declarations, pragmas, semantics.
- `docs/Grammar.md` — the concrete grammar.
- `docs/DataFlowIR.md` — data-flow node kinds, canonicalization, optimization.
- `docs/ControlFlowIR.md` — region kinds and the build/optimize passes.
- `docs/RuntimeAndCodegen.md` — runtime containers and generated-code shape.
- `docs/Tutorial/` — a worked introduction to writing Datalog programs.
