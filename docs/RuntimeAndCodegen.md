# Runtime & C++ Code Generation

The C++ backend compiles a Datalog program into a concrete, template-free
implementation: a header with named row structs and a `Database` class, and a
source file with the procedure and query-cursor definitions. Storage is
provided by a small runtime (`include/drlojekyll/Runtime`) with explicit
allocators. There are no third-party dependencies.

## The runtime

Four headers plus one source file:

- **`Allocator.h`** — a Zig-style allocator: a two-pointer-wide value
  (`ctx` + `alloc`/`free` function pointers) passed to every container at
  construction. `MallocAllocator()` wraps libc; `Arena` is a bump allocator
  whose `ArenaAllocator(arena)` view makes frees no-ops and releases
  everything on `Reset()`.
- **`Hash.h`** — value hashing: `HashRow(a, b, c)` combines per-field
  `DrHash` values (splitmix64 mixing). Foreign types participate via an
  ADL-visible `uint64_t DrHash(const T &)`.
- **`Vec.h`** — `Vec<T>`: a flat growable array of trivially copyable rows
  with `Add`, `Clear`, `Swap`, `SortAndUnique`, and range-`for` iteration.
  Move-only.
- **`Table.h`** — `Table<Row>`: an append-only row log (row ids are stable
  `uint32_t` offsets) plus an open-addressing hash set for row-by-value
  lookup. Each row carries a 2-bit `TupleState` (`kAbsent`/`kPresent`/
  `kUnknown`); rows are never physically removed, so retraction and
  re-derivation are state flips. `Index<Key>` is a secondary index mapping a
  key struct to a chain of row ids threaded through a per-row `next` array —
  no per-key allocation. Generated code updates indexes explicitly when an
  insertion reports `added_row`.

Cursors iterate by row id and re-read through the container on each step, so
tables and indexes may be mutated while a scan over them is live (an
invariant the generated fixpoint code relies on).

## Generated code

For `#database points_to` the compiler emits `points_to.h` + `points_to.cpp`:

- **Row structs** per table, with fields named after the Datalog variables
  (`struct Row13 { uint32_t var; uint32_t heap; ... }`), a `Hash()` method,
  and defaulted equality. Key structs per non-covering index.
- **Tuple shapes** (`Tup_u64_u64`, ...) for dataflow vectors, plus a
  `using <message>_input = Tup...;` alias per message.
- **Enums** from `#enum` declarations, with enumerator values from their
  `#constant` definitions.
- **`DatabaseFunctors`** — a plain struct declaring one member function per
  `#functor`; the user defines them in their own translation unit (link-time
  binding). Filters return `bool`, `@range(?)` returns `std::optional`,
  `@range(*)`/`@range(+)` return `std::vector`.
- **`DatabaseLog`** — a plain struct with one no-op method per published
  message.
- **`class Database`** — constructed with
  `Database(hyde::rt::Allocator, DatabaseLog &, DatabaseFunctors &)`.
  - *Messages*: one entry point per `#message`, taking a
    `hyde::rt::Vec<msg_input>` by value.
  - *Queries*: one manual cursor per `#query` binding pattern:

    ```cpp
    auto c = db.function_instructions_bf(func_ea);
    for (uint64_t inst_ea; c.next(inst_ea);) { ... }
    ```

    Cursor state is visible struct members (bound params + a `uint32_t`
    position); `next` returns `false` when exhausted. All-bound queries are
    plain `bool` existence checks.

The emitted code contains no templates and no hidden heap use; every
container is constructed from the database's allocator. Covering indexes
(keys spanning every column) are not materialized — the table's own hash
lookup serves them via `Find`.

## Building generated code

`compile_datalog(...)` (cmake/Compiler.cmake) runs the compiler at build time
and, given `LIBRARY_NAME foo`, defines a static library compiling the
generated `.cpp`, publicly linked against `DrLojekyll::Runtime` with the
output directory on the include path. See `tests/PointsTo` and
`tests/MiniDisassembler` for end-to-end examples.

## User-code injection points

`#inline(<stage>)` code blocks are pasted into the generated header at these
stages: `c++:database:{prologue,epilogue}`,
`c++:database:{prologue,epilogue}:namespace`,
`c++:database:enums:{prologue,epilogue}`,
`c++:database:functors:{prologue,epilogue}`,
`c++:database:functors:definition:{prologue,epilogue}`, and the same
`log` variants.
