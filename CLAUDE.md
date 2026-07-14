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
(`<name>.dr` + `<name>.main.cpp`, ~150 corner-case programs) has one
committed expected output in `tests/OptDiff/goldens/<name>.stdout`, and the
4 optimization modes are just execution variants — EVERY mode's stdout is
byte-compared against the same golden (cross-mode agreement is implied).
A case with a `<name>.batches` sidecar additionally runs the
derivation-counter oracle (`bin/Oracle`, built as `drlojekyll-oracle`) and
the monotone projection, each against its own golden
(`<name>.oracle.stdout` / `<name>.monotone.stdout`).

- One case: `tests/OptDiff/diffrun.sh <case.dr> <driver.cpp> <workdir>`
  (env: `DR=` compiler path, `TIMEOUT=` seconds).
- Full suite: `DR=build/debug/bin/drlojekyll tests/OptDiff/runall.sh
  <workroot> [jobs] [name-filter-regex]` — must end `SUITE: PASS`.
  Expected-diagnostic cases (aggregate_1, kvindex_1–4, evm_func_parse,
  nonascii_1, truncated_decl_1) are encoded in runall.sh.
- Blessing: goldens change ONLY via explicit
  `runall.sh --bless <workroot> [filter]` after reviewing a run's outputs —
  never automatically on failure, and never to make a red case green.

`tests/OptDiff/FINDINGS.md` is the ledger of bugs found this way, with
repros (F1–F19 and F21 fixed as of July 2026; F20 is an open record-only
latent-comparator note).

Manual compile of generated code (driver pattern in any `cases/*.main.cpp`):

```sh
build/debug/bin/drlojekyll foo.dr -cpp-out gen/          # emits datalog.h/.cpp
clang++ -std=c++23 -I include -I gen driver.cpp gen/datalog.cpp \
  lib/Runtime/Allocator.cpp -o case
```

Generated API (the hidden-friend surface, since the generated-surface
epoch): database name defaults to `datalog`, no namespace; `datalog.h` is
the whole header-only artifact (`datalog.cpp` is an anchor TU — compile
lines unchanged). `struct Database` is a sealed state struct constructed
with just the allocator; ALL driver-facing functions are hidden friends,
reachable only by unqualified ADL call with the database argument (never
qualify the calls; types may be qualified). Epoch 0 is explicit:
`init(db, log, functors)` once, immediately after construction, before
anything else (entry points and queries assert it). Messages are
`<name>_<arity>(db, log, functors, Vec<...>[, Vec<...>])`; queries are
`<name>_<bindings>(db, bound...)` (`b`/`f` per column) returning bool
(all-bound) or a cursor (`auto c = q_f(db); c.next(out...)`); queries
with a forcing function also take `(log, functors)` after `db`. Log and
functor types flow by deduction — a driver observes published deltas by
providing ITS OWN type with the message-hook signatures (no inheritance,
no virtual, no `override`); `DatabaseFunctors` members are declared in
the header and defined by the driver out-of-line. Always read the
generated `datalog.h` for exact signatures before writing a driver.

## Key internals

- Data-flow IR: `lib/DataFlow/Query.h` (node classes: SELECT/TUPLE/JOIN/
  MERGE/CMP/MAP/NEGATE/AGG/KVINDEX/INSERT). Per-node `Canonicalize` methods +
  the driver in `lib/DataFlow/Optimize.cpp` (Simplify → Canonicalize fixpoint
  → CSE; `OptimizationContext` flags in `lib/DataFlow/Optimize.h`). Every
  optimization pass carries a doc comment: algorithm, pseudocode, ASCII
  before/after diagram.
- Control-flow IR: regions in `lib/ControlFlow/Program.h` (SERIES/PARALLEL/
  INDUCTION/LET/TUPLECMP/UPDATECOUNT/CHECKMEMBER/COMMITSWEEP/CLAIM/...);
  built by `lib/ControlFlow/Build/`, optimized by
  `lib/ControlFlow/Optimize.cpp` (per-region `OptimizeImpl` overloads, same
  doc-comment convention). Differential maintenance is per-stratum
  OVERDELETE → REDERIVE → INSERT with split per-row derivation counters
  (see `docs/proposals/StackSafeNegation.md`).
- Core invariants (dataflow): no view is ever its own direct user (asserted
  in `RelabelGroupIDs`); a source-less forwarding cycle is unsatisfiable,
  collected by dead-flow elimination; `QueryImpl` owns no conditions —
  zero-arity predicates desugar in `BuildClause` into unit relations (1 bool
  column, `is_condition`, sole possible row `(true)`) and every inter-view
  dependency is a column edge; canonicalization never severs the last
  input-column edge to an incoming view (keep-last-edge rule); a JOIN pivot
  whose non-user side is a unit relation is never removed, and CSE never
  folds a unit SELECT into a non-unit one; a unit relation contains at most
  the row `(true)` — only the desugarer creates its INSERTs, and they insert
  only the token; zero-pivot JOINs appear only under `@product`; a table's
  member-view list holds each view at most once, by IDENTITY — never dedup
  it structurally (distinct-but-equal views sharing a model are intentional,
  the group_ids CSE guard).
- Core invariants (differential): every inductive back-edge fold is an
  `UPDATECOUNT` whose propagation body is dominated by its zero crossing
  (termination of generated fixpoints); differential rows carry split SIGNED
  counters (`C_nr`/`C_r`; presence = total > 0) that may dip below zero only
  mid-batch — the commit sweep asserts both ≥ 0 per class and publishes only
  `was != now`; generated code reads a differential table ONLY through the
  named membership predicates (`in-I`, `in-new`, the fixpoint-round forms,
  `recursively-supported`, `present`), placed by the seed/fixpoint delta
  schemas (seed: lower position `j < i` reads InNew, `j > i` reads InI;
  fixpoint rounds use the claim-relative matrix — StackSafeNegation.md
  §5.1); claim gates re-test at dequeue (`TryClaimDel`: C_nr ≤ 0,
  `TryClaimAdd`: total > 0 — F17); negate gates are CONTEXT-keyed, never
  sign-keyed (seed context: key absent in InI for BOTH signs; fixpoint
  refire: absent in InNew for both signs; `@never` gates on Present — F18);
  each non-@never negate has exactly ONE crossover arm-pair, folding into
  the negate's own table, emitted seed-before-drain; an ACYCLIC differential
  @product emits one signed frontier arm per side×sign (monotone sides have
  no `-` arm) — position-keyed sign-INDEPENDENT non-delta reads (`j < i`
  InNew, `j > i` InI), one fold into the product's own table, every arm
  seed-before-drain (the claim gates' phantom drop depends on that order);
  on-cycle differential products are rejected by exact self-reachability
  (`ViewSelfReachable`, NOT `InductionGroupId` — a fully interior join loses
  its group id, F22); explicit message batches net with SET semantics — each
  side deduplicated, adds∩removes annihilates, leaving presence exactly what
  the rest of the program proves (OQ3).
- Union sinking (`do_sink` in `QueryImpl::Optimize`) is commented out —
  `lib/DataFlow/Merge.cpp` sinking code is currently unreachable.

## Known feature gaps (clean diagnostics)

Aggregates and KV indices (mutable params) — design recorded in
`docs/proposals/AggregatingFunctors.md` (two-level group-by; a KV index is
the degenerate aggregate), gated on the delta-relational IR per that
ledger's sequencing; cross-products over differential (deletable) data
INSIDE RECURSIVE CYCLES (the acyclic case landed as Stage 5 of
`StackSafeNegation.plan.md`; the fence is `ViewSelfReachable` in
`Program::Build`'s pre-pass); impure functors (control-flow build); and
unstratified negation — a negated predicate recursively derived from the
negation's own result (rejected by the dataflow Stratify pass in all
modes). Corpus files exercising these:
`data/examples/average_weight.dr`, `pairwise_average_weight.dr` (KV
indices), `data/self_testing_examples/evm_func_parse.dr` (unstratified
negation). Every other file under `data/` — including
`conditions_to_bools.dr`, the acyclic differential @product example —
compiles in all 4 modes.

## Gotchas

- macOS ships bash 3.2: no `declare -A` in scripts. zsh does not word-split
  unquoted variables — use `${=var}` when a variable holds multiple CLI args.
- Debug builds round-trip the parser and re-assert; crashes usually surface
  as `Assertion failed` + SIGABRT (exit 134), stack overflow/null deref as
  exit 139. `lldb -b -s <script-file> -- <cmd>` gets backtraces reliably;
  `-o run -o bt` sometimes truncates.
- clangd diagnostics in this repo are noise (it lacks include paths); trust
  the real build only.
