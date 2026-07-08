# Dr. Lojekyll

Datalog compiler ("hyde" C++ namespace). Compiles Datalog to an incremental,
message-driven C++ database. Pipeline:

    parse (lib/Lex, lib/Parse)
      → data-flow IR        lib/DataFlow    Query::Build(module, log, optimize)
      → control-flow IR     lib/ControlFlow Program::Build(query, first_id, optimize)
      → C++ codegen         lib/CodeGen/CPlusPlus/Database.cpp
      → runs against        lib/Runtime + include/drlojekyll/Runtime (no deps)

`docs/RuntimeAndCodegen.md` describes the runtime and generated-code shape.
CLI driver: `bin/drlojekyll/Main.cpp`.

## Build

```sh
cmake --preset debug            # configure (Ninja, build/debug)
cmake --build --preset debug    # build; binary at build/debug/bin/drlojekyll
```

Presets: `debug`, `release`. C++23. No third-party deps beyond `vendor/`.
Coverage build (llvm-cov):

```sh
cmake -B build/coverage -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDRLOJEKYLL_ENABLE_TESTS=OFF \
  -DCMAKE_CXX_FLAGS='-fprofile-instr-generate -fcoverage-mapping' \
  -DCMAKE_EXE_LINKER_FLAGS='-fprofile-instr-generate'
# run with LLVM_PROFILE_FILE=..., then xcrun llvm-profdata merge / llvm-cov report
```

## Test

```sh
cd build/debug && ctest --output-on-failure   # MiniDisassembler, PointsTo, Runtime
```

End-to-end tests compile a `.dr` file at build time via `compile_datalog()`
(cmake/Compiler.cmake) and link a hand-written driver against the generated
code + runtime (see `tests/MiniDisassembler`, `tests/PointsTo`).
`tests/DrTest` is a dependency-free mini-GoogleTest (`TEST`, `ASSERT_*`).

### Golden-master optimization testing

The compiler has optimization toggles: `-disable-dataflow-opt` (skips
`QueryImpl::Optimize`: CSE, canonicalization rounds, dead-flow elimination)
and `-disable-controlflow-opt` (skips `ProgramImpl::Optimize`: region
flattening, no-op removal, procedure dedup).

The suite is golden-master-based: each case in `tests/OptDiff/cases/`
(`<name>.dr` + `<name>.main.cpp`, ~128 corner-case programs) has one
committed expected output in `tests/OptDiff/goldens/<name>.stdout`, and the
4 optimization modes are just execution variants — EVERY mode's stdout is
byte-compared against the same golden (cross-mode agreement is implied).

- One case: `tests/OptDiff/diffrun.sh <case.dr> <driver.cpp> <workdir>`
  (env: `DR=` compiler path, `TIMEOUT=` seconds).
- Full suite: `DR=build/debug/bin/drlojekyll tests/OptDiff/runall.sh
  <workroot> [jobs] [name-filter-regex]` — must end `SUITE: PASS`.
  Expected-diagnostic cases (aggregate_1, kvindex_1–4, evm_func_parse) are
  encoded in runall.sh.
- Blessing: goldens change ONLY via explicit
  `runall.sh --bless <workroot> [filter]` after reviewing a run's outputs —
  never automatically on failure, and never to make a red case green.

`tests/OptDiff/FINDINGS.md` is the ledger of bugs found this way, with
repros (F1–F15 all fixed as of July 2026).

Manual compile of generated code (driver pattern in any `cases/*.main.cpp`):

```sh
build/debug/bin/drlojekyll foo.dr -cpp-out gen/          # emits datalog.h/.cpp
clang++ -std=c++23 -I include -I gen driver.cpp gen/datalog.cpp \
  lib/Runtime/Allocator.cpp -o case
```

Generated API: database name defaults to `datalog`, no namespace; messages are
`db.<name>_<arity>(Vec<...>)`; queries are cursors `db.<name>_<bindings>()`
(`b`/`f` per column) — always read the generated `datalog.h` for exact
signatures before writing a driver.

## Key internals

- Data-flow IR: `lib/DataFlow/Query.h` (node classes: SELECT/TUPLE/JOIN/
  MERGE/CMP/MAP/NEGATE/AGG/KVINDEX/INSERT). Per-node `Canonicalize` methods +
  the driver in `lib/DataFlow/Optimize.cpp` (Simplify → Canonicalize fixpoint
  → CSE; `OptimizationContext` flags in `lib/DataFlow/Optimize.h`). Every
  optimization pass carries a doc comment: algorithm, pseudocode, ASCII
  before/after diagram.
- Control-flow IR: regions in `lib/ControlFlow/Program.h` (SERIES/PARALLEL/
  INDUCTION/LET/TUPLECMP/CHANGETUPLE/...); built by `lib/ControlFlow/Build/`,
  optimized by `lib/ControlFlow/Optimize.cpp` (per-region `OptimizeImpl`
  overloads, same doc-comment convention).
- Core invariants: no view is ever its own direct user (asserted in
  `RelabelGroupIDs`); every inductive back-edge append must be dominated by a
  state transition on the union's table (termination of generated fixpoints);
  a source-less forwarding cycle is unsatisfiable, collected by dead-flow
  elimination; `QueryImpl` owns no conditions — zero-arity predicates desugar
  in `BuildClause` into unit relations (1 bool column, `is_condition`, sole
  possible row `(true)`) and every inter-view dependency is a column edge;
  canonicalization never severs the last input-column edge to an incoming
  view (keep-last-edge rule); a JOIN pivot whose non-user side is a unit
  relation is never removed, and CSE never folds a unit SELECT into a
  non-unit one; a unit relation contains at most the row `(true)` — only the
  desugarer creates its INSERTs, and they insert only the token; zero-pivot
  JOINs appear only under `@product`.
- Union sinking (`do_sink` in `QueryImpl::Optimize`) is commented out —
  `lib/DataFlow/Merge.cpp` sinking code is currently unreachable.

## Known feature gaps (clean diagnostics)

Aggregates, KV indices (mutable params), cross-products over differential
(deletable) data, impure functors (control-flow build), and unstratified
negation — a negated predicate recursively derived from the negation's own
result (rejected by the dataflow Stratify pass in all modes). Corpus files
exercising these: `data/examples/average_weight.dr`,
`pairwise_average_weight.dr` (KV indices), `conditions_to_bools.dr` (an
explicit `@product` join of two deletable locals),
`data/self_testing_examples/evm_func_parse.dr` (unstratified negation).
Every other file under `data/` compiles in all 4 modes.

## Gotchas

- macOS ships bash 3.2: no `declare -A` in scripts. zsh does not word-split
  unquoted variables — use `${=var}` when a variable holds multiple CLI args.
- Debug builds round-trip the parser and re-assert; crashes usually surface
  as `Assertion failed` + SIGABRT (exit 134), stack overflow/null deref as
  exit 139. `lldb -b -s <script-file> -- <cmd>` gets backtraces reliably;
  `-o run -o bt` sometimes truncates.
- clangd diagnostics in this repo are noise (it lacks include paths); trust
  the real build only.
