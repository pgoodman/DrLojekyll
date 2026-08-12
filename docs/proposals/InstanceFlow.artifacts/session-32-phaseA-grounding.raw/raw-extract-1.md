Now I have the full tail. This confirms the exact slot for the new InstanceFlow pass (after `row_contracts`/`ValidateRowContracts`, before the DEBUG belt or `return Query(...)`).

## IF2 — The dump + CLI-flag pattern for `-instanceflow-out`

### 1. The mechanism, verbatim, mirrored from `-contract-out` (chosen over `-df-out`/`-origin-out` as the template since InstanceFlow, like row-contracts, is a pure post-Optimize `Query`-derived side-table stored on `QueryImpl`, drained pre-freeze)

**Global stream pointer** (`bin/drlojekyll/Main.cpp:55-64`, add alongside the existing block):
```cpp
static OutputStream *gDOTStream = nullptr;
static OutputStream *gDFStream = nullptr;
static OutputStream *gContractStream = nullptr;
static OutputStream *gOriginStream = nullptr;  // K5 advisory origin-provenance.
static OutputStream *gInstanceFlowStream = nullptr;  // NEW: -instanceflow-out.
static OutputStream *gRelStream = nullptr;
...
```

**Local `unique_ptr<FileStream>` in the argv-parsing function** (`bin/drlojekyll/Main.cpp:327-336`, add alongside `contract_out`/`origin_out`):
```cpp
std::unique_ptr<hyde::FileStream> contract_out;
std::unique_ptr<hyde::FileStream> origin_out;  // K5 advisory origin dump.
std::unique_ptr<hyde::FileStream> instanceflow_out;  // NEW.
```

**Flag-parse arm** — copy the `-contract-out` arm verbatim (`bin/drlojekyll/Main.cpp:415-430`), renamed:
```cpp
// InstanceFlow flat-grove text dump (the `-instanceflow-out` surface).
} else if (!strcmp(argv[i], "--instanceflow-out") ||
           !strcmp(argv[i], "-instanceflow-out")) {
  ++i;
  if (i >= argc) {
    error_log.Append() << "Command-line argument '" << argv[i - 1]
                       << "' must be followed by a file path for "
                       << "InstanceFlow output";
  } else {
    instanceflow_out.reset(new hyde::FileStream(display_manager, argv[i]));
    if (!instanceflow_out->fs.is_open()) {
      error_log.Append() << "Unable to open '" << argv[i]
                         << "' for InstanceFlow output";
    }
    hyde::gInstanceFlowStream = &(instanceflow_out->os);
  }
```
Insert it adjacent to the `-contract-out`/`-origin-out` arms (both are simple `Query`-only dumps; order among the `else if` chain is irrelevant — `strcmp` dispatch — so append after the `-origin-out` arm at `bin/drlojekyll/Main.cpp:448`).

**Help line** (`bin/drlojekyll/Main.cpp:247-248`, insert after `-contract-out`):
```cpp
<< "  -contract-out <PATH>      Emit the Stage-A row contracts in text form to PATH." << std::endl
<< "  -instanceflow-out <PATH>  Emit the InstanceFlow flat grove in text form to PATH." << std::endl
<< "  -origin-out <PATH>        Emit the K5 Tier-2 origin provenance (advisory) to PATH." << std::endl
```

**Drain call site** (`bin/drlojekyll/Main.cpp` inside `CompileModule`) — must run **after `Query::Build`, before `FrozenRegionalProgram::Build`**, i.e. immediately after the `-origin-out` drain at line 83-86 (the grove is stored on `QueryImpl`, finalized at the `Query::Build` tail — same lifetime window as `row_contracts`; draining before freeze matches the doc's stated pass slot "after `InferConservativeRowContracts`, before return" and keeps the precedent that `-contract-out` itself is actually drained later, post-`Program::Build`, at line 166-169 purely for grouping with `-df-out`— for InstanceFlow prefer the earlier `-origin-out`-style placement since nothing downstream needs it yet and earlier drain fails faster on a bad path):
```cpp
auto query_opt =
    Query::Build(module, error_log, gPassPolicy, gDemand);
if (!query_opt) {
  return EXIT_FAILURE;
}

// K5 (advisory belt (ii)): the `-origin-out` ...
if (gOriginStream) {
  (*gOriginStream) << hyde::QueryOrigins{*query_opt};
  gOriginStream->Flush();
}

// NEW: the InstanceFlow flat-grove text dump (the `-instanceflow-out`
// surface). The grove is a pure post-Optimize function of the final Query
// graph, stored on QueryImpl (impl->instance_flow, the row_contracts
// precedent) by the Query::Build tail pass — final by the time query_opt
// is returned, so the drain sits here, before Stage-B freeze.
if (gInstanceFlowStream) {
  (*gInstanceFlowStream) << hyde::QueryInstanceFlow{*query_opt};
  gInstanceFlowStream->Flush();
}

auto frozen_opt = FrozenRegionalProgram::Build(*query_opt, error_log);
```
(`hyde::QueryInstanceFlow` is the new tag struct, mirroring `QueryOrigins`/`QueryContracts`.)

### 2. Where the pass result must be stored

Follow the `row_contracts` precedent exactly (`lib/DataFlow/Query.h:1234-1240`, member of `QueryImpl`):
```cpp
// InstanceFlow Phase A/B.1-B.2: the maximally-shared flat grove, materialized
// ONCE at the Query::Build tail (post-row_contracts, pre-return). A PURE,
// RECOMPUTABLE function of the final graph — not present during Optimize.
// Read by the V-IF-* validators and the `-instanceflow-out` dump.
InstanceFlowGrove instance_flow;
```
Set at `lib/DataFlow/Build.cpp:2654`-adjacent tail, right after the existing:
```cpp
impl->row_contracts = InferConservativeRowContracts(impl.get());
if (!ValidateRowContracts(impl.get(), log)) {  // H-A7 validators.
  return std::nullopt;
}

// NEW slot:
impl->instance_flow = BuildInstanceFlowGrove(impl.get());
if (!ValidateInstanceFlow(impl.get(), log)) {  // V-IF-ORIGIN/CONTEXT/COVERAGE/EMISSION.
  return std::nullopt;
}
```
before the `#ifndef NDEBUG` K5 conservation belt (`Build.cpp:2659`) and before `return Query(std::move(impl));` (`Build.cpp:2682`).

**Access from the emitter**: `QueryInstanceFlow` needs the same `friend` grant `QueryContracts` has (`include/drlojekyll/DataFlow/Query.h:1173-1176`, private section of `class Query`):
```cpp
private:
  friend OutputStream &operator<<(OutputStream &os, QueryContracts qc);
  friend OutputStream &operator<<(OutputStream &os, QueryInstanceFlow qif);  // NEW.
  friend class ::hyde::FrozenRegionalProgram;
  friend OutputStream &operator<<(OutputStream &os, Query query);
```

### 3. Tag-struct + operator<< idiom to copy

New header (either append to `include/drlojekyll/DataFlow/Format.h` beside `QueryOrigins`, or a new `include/drlojekyll/InstanceFlow/Format.h` if InstanceFlow gets its own library — matches the eventual `lib/Rel` split precedent):
```cpp
// The InstanceFlow flat-grove text dump (the `-instanceflow-out` surface).
// A tag struct keeps this operator<< disjoint from DOT/`.df`/contract/origin.
struct QueryInstanceFlow {
  Query query;
};

OutputStream &operator<<(OutputStream &os, QueryInstanceFlow qif);
```
Emitter body mirrors `QueryContracts`' operator<< (`lib/DataFlow/Format.cpp:1546` on): pull the stored grove off `qif.query.impl->instance_flow`, reuse the SAME kind-tagged `for_each_view` det_seq-order traversal lambda (lines 1553-1573: `kCSelect..kCInsert`, `query.Selects()`...`query.Inserts()`, `is_dead`-skipped), buffer through a local `std::ostringstream`/`OutputStream bos` (line 1599-1600 `take()` idiom) for per-block assembly, and gate an always-on census belt in the SAME fprintf+abort shape as V-CONTRACT-CENSUS (lines 1575-1597) — this becomes V-IF-COVERAGE (or a dedicated check invoked from the operator<< as a redundant belt, or purely delegated to `ValidateInstanceFlow` at build time; `-contract-out` does both: build-time `ValidateRowContracts` AND a redundant dump-time census).

**Files to touch, in order:**
1. `include/drlojekyll/DataFlow/Format.h` — add `QueryInstanceFlow` tag struct + operator<< decl (or new `InstanceFlow/Format.h`).
2. `include/drlojekyll/DataFlow/Query.h:1173-1180` — add the friend grant, add `InstanceFlowGrove instance_flow;` member near `row_contracts` (`:1240`).
3. `lib/DataFlow/Build.cpp:2654-2657` — add `impl->instance_flow = BuildInstanceFlowGrove(...)` + `ValidateInstanceFlow(...)` call, tail slot.
4. `lib/DataFlow/Format.cpp` (or new `lib/InstanceFlow/Format.cpp`) — add the `operator<<(OutputStream&, QueryInstanceFlow)` emitter, det_seq-ordered `for_each_view` reused verbatim from `QueryContracts`.
5. `bin/drlojekyll/Main.cpp` — the four edits above: global stream decl (~:59), local `unique_ptr` decl (~:330), flag-parse arm (~after :448), help line (~after :247), drain call site (after the `-origin-out` drain, ~:86).