# Runtime & C++ Code Generation

The C++ backend compiles a Datalog program into a header-only implementation:
`datalog.h` carries the whole artifact — named row structs, a sealed
`Database` state struct, its hidden-friend entry points, and the templated
flow-procedure definitions — while `datalog.cpp` is an anchor translation unit
(banner plus `#include "datalog.h"`). The entry points are function templates
that deduce the driver's log and functors types statically, so the emitted
code is heavily templated. Storage is provided by a small runtime
(`include/drlojekyll/Runtime`) with explicit allocators. There are no
third-party dependencies.

## The runtime

Four core headers plus one source file (two further headers are
conditional: `BenchCounters.h`, the default-off bench counter seam; and
`StateCell.h`, included only by generated code that has an aggregate or KV
index — see below):

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
  Move-only. The free function `NetBatch(adds, removes)` nets a differential
  message's explicit adds against its removes within one batch with **set
  semantics and annihilation**: each vector is deduplicated (assert/retract are
  idempotent) and any row appearing in both is dropped from both (the
  unordered add∕remove pair is defined to annihilate), so `{+x, −x, −x}` is a
  no-op rather than a removal.
- **`Table.h`** — an append-only `RowStore<Row>` (row ids are stable
  `uint32_t` offsets, plus an open-addressing hash set for row-by-value lookup)
  shared by two flavors:
  - `Table<Row>` — **monotone**, insert-only: a stored row is present forever.
    Its only batch state is a sealed row-id watermark, so `InI(id)` ("present
    at batch start") is one id comparison against `sealed`; `Seal()` advances
    it. This lets a monotone table answer the same frozen-vs-current membership
    reads a differential table does when it sits at a read position of a delta
    join.
  - `DiffTable<Row>` — **differential**: two packed signed derivation counters
    per row (`C_nr` low, `C_r` high, in one 64-bit word) plus a per-row flags
    byte (`RowFlags`: `kInI`, `kDel`/`kAdd`, `kDelNow`/`kAddNow`, `kExplicit`,
    `kTouched`) and a `touched` vector of the ids changed this batch. Counter
    folds (`AddDerivation`/`SubDerivation`, `AddExplicit`/`SubExplicit`) return
    a `Delta` whose `crossed` flag reports a zero crossing. The named
    **membership predicates** (`Present`, `InI`, `InNew`, `SurvivesSoFar`,
    `AliveAtClaim`, `RecursivelySupported`, `NetDeleted`, `NetAdded`, ...) are
    single flag-byte or counter reads. `TryClaimDel`/`TryClaimAdd` are the
    dedup-and-stale-gate CASes into the batch overdeletion/addition set;
    `RetireDel`/`RetireAdd` clear the current-round frontier bit. `Commit(sink)`
    ends the epoch: for each touched row it publishes the net 0/1 presence
    change against `kInI` to `sink`, re-seals `kInI`, and clears the scratch
    flags; `DebugValidateCounts()` then asserts per-class non-negativity and
    snapshot coherence.

  `Index<Key>` is a secondary index mapping a key struct to a chain of row ids
  threaded through a per-row `next` array — no per-key allocation. It is
  append-only: generated code links each fresh row id (a `TryAdd`/fold whose
  `added` is true) once, in row-id order, and readers filter liveness through
  the owning table's membership predicates.

- **`StateCell.h`** (conditional, aggregate/KV only) — a `StateCellStore`
  holds the standing per-group state for one aggregate or KV-index view (the
  R3 delta-relational-IR family): a dense group-id space, a two-word
  sealed/working cell per group, and an occupancy count. It is a peer of the
  view's own `DiffTable`. `Fold(gid, sign, summary)` accumulates the
  summarized input's net-frontier rows (`@invertible` folds/unfolds in O(1);
  `@recompute` rescans the live multiset), and the emit step applies an
  occupancy-generalized one-net-pair guard (birth `+new`, death `−old`,
  change `−old,+new`) into the agg table. The reduction bodies themselves are
  DRIVER-SUPPLIED (see `DatabaseFunctors` below).

Cursors iterate by row id and re-read through the container on each step, so
tables and indexes may be mutated while a scan over them is live (an
invariant the generated fixpoint code relies on). A query cursor filters its
results through the backing table's `Present(id)` predicate.

## Generated code

For `#database points_to` the compiler emits `points_to.h` + `points_to.cpp`.
`points_to.h` is the whole artifact; `points_to.cpp` is an anchor TU (banner +
`#include "points_to.h"`). Its contents:

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
  `@range(*)`/`@range(+)` return `std::vector`. For an aggregate (`over(){}`)
  or KV-index (`mutable(...)`) reduction functor `f`, the driver also supplies
  FREE FUNCTIONS (forward-declared in the header, defined out-of-line, named
  after the functor): `@invertible` needs `f_identity()`, `f_combine(w, v)`,
  `f_uncombine(w, v)`; `@recompute` needs `f_reduce(const S *values, const
  int32_t *counts, size_t n)` (a from-scratch rescan over the live multiset).
- **`DatabaseLog`** — a plain struct with one no-op method per published
  message, emitted as a concrete default type. Nothing is virtual: the entry
  points deduce the log's static type, so any driver type providing the same
  member signatures observes the published delta stream with no virtual
  dispatch (the commit sweep calls `log.<message>_<arity>(cols..., added)` on
  the deduced type). No inheritance or overriding is needed.
- **`struct Database`** — a sealed state struct holding the tables, indices,
  and globals as PRIVATE members plus a `bool initialized_`. Constructed with
  `explicit Database(hyde::rt::Allocator)`: it allocates empty tables and
  cannot fail. It does NOT run epoch 0. All driver-facing functions are
  **hidden friends**, reachable only by unqualified (ADL) call with the
  database as an argument; a qualified call such as `points_to::init(db, ...)`
  does not compile (types may be qualified, calls never).
  - *Epoch 0*: `init(db, log, functors)` derives the empty-EDB least model and
    publishes its `t=0` deltas. Call it exactly once, after construction and
    before any message; the message entry points and queries assert it has run.
  - *Messages*: one entry point per `#message`, `<message>_<arity>(db, log,
    functors, Vec...)`, taking a `hyde::rt::Vec<msg_input>` by value — TWO
    Vecs (adds then removes) for a `@differential` message.
  - *Queries*: one entry point per `#query` binding pattern,
    `<query>_<pattern>(db, bound...)` (`b`/`f` per column). All-bound queries
    return a plain `bool` existence check; otherwise the call returns a cursor:

    ```cpp
    auto c = function_instructions_bf(db, func_ea);
    for (uint64_t inst_ea; c.next(inst_ea);) { ... }
    ```

    A query with a forcing function additionally takes `(log, functors)` after
    `db`. The cursor is a nested struct holding a mutable `Database &`, bound
    values, and a `uint32_t` position; `next` returns `false` when exhausted
    and filters results through the backing table's `Present(id)` predicate.

The flow procedures are namespace-scope detail functions whose parameter lists
name exactly the state they read and write (the allocator, the deduced
log/functors types when the closure publishes or calls functors, the tables
and indices, and globals); their forward declarations precede the `Database`
struct, and each message handler has a `_detail` twin that the entry-point
wrapper delegates to. Every container is constructed from the database's
allocator. Covering indexes (keys spanning every column) are not materialized —
the table's own hash lookup serves them via `Find`.

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
