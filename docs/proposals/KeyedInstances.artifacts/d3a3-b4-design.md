# D3.a.3 STAGE (b) — DESIGN LANE b4: THE O1 PER-PUB RELAXATION + ADV-10 + THE g6 WITNESS + CENSUS

Repo tip b65e7668 (docs-only atop D3.a.2 landing bfc068d1; **code bytes == the landing**).
Repo READ-ONLY. Every anchor is a CURRENT line number re-read at code this session; every
load-bearing claim carries a `file:line` + short quote or an OBSERVED scratch compile. The
§6 O1 ruling (adopt the `(pub_table, forcing_index)` keying) and the §2 g6 re-disposition
(`demand_multi_adorn_1` STAYS diagnostic; a NEW From-preserving success witness is needed)
are BINDING from `d3a3-substrate.md`; this lane specs the code + the witness that realize them.

**SCOPE:** the N-store landing (the O1 validator relaxation) + ADV-10 per-store coupling
confirmation + the g6 success witness family + the census arithmetic.

**SIBLING DEPENDENCIES (named up front):**
- **b1 (g3 fence-lift)** — lifts `Demand.cpp:457` + the two-phase locate/check/mint loop +
  the `forcing_index`/`first_annotation` snapshots-in-loop (ADV-2). Without b1 the two-adornment
  witness DOES NOT COMPILE and the O1 validator is never armed (single-adornment `inst_per_pub`
  counts stay 1). **My witness (Deliverable 3) is blocked on b1; the O1 change (Deliverable 1)
  compiles standalone but is only EXERCISED once b1 lands.**
- **b2/g1 (survivor policy + [F] guard, co-land, g1 FIRST)** — the kBody-survivor policy
  (View.cpp) + the always-on `[F]` handler-map fence (Build.cpp:393 mirror of :508-512). With
  N=2 fabricated demand messages, the release-compiled-out assert at Build.cpp:393-394 would let
  an unregistered message default-insert a nullptr callee → **release SIGSEGV**; g2's always-on
  fence is a HARD prerequisite for the witness to run in a release/ASAN build.
- **The O1 change depends on NOTHING** but is CO-COMMITTED in the one slice (it is a validator
  relaxation with no emission surface).

===============================================================================
## DELIVERABLE 1 — THE O1 DIFF (V-INST-SOLE per-pub relaxation, substrate §6 BINDING)

### 1.1 Where `forcing_index` lives at validate time — ALREADY ON THE OP (no threading)

The kSubgraphInstantiate op carries `forcing_index` as a first-class field. **VERIFIED at code:**

- **Declaration:** `lib/Rel/Rel.h:713` —
  `unsigned forcing_index{~0u};  // -> query.RecognizedSubgraphs()[i]` (in the DROp struct's
  `SUBGRAPH_INSTANTIATE / INSTANCE_DEATH / INSTANCE_SEAL data` block, :703-713).
- **Mint stamp:** `lib/Rel/Rel.cpp:1117` — `inst.forcing_index = rs.forcing_index;` (also
  `death.forcing_index = rs.forcing_index;` :1170, `seal.forcing_index = rs.forcing_index;` :1180).

So at V-INST-SOLE time `op.forcing_index` is directly available on every kSubgraphInstantiate op.
**NO threading, no new field, no mint-side change is needed.** This is the clean case: the field
the D3.a.1/D3.a.2 substrate already stamped for V-INST-ORDER/render is exactly the second key axis.

### 1.2 The map-key change — `std::map<std::pair<uintptr_t,unsigned>,unsigned>`

**File:** `lib/Rel/Rel.cpp`, function `ValidateDROps` (the V-INST-SOLE/V-INST-PAIR block opening
at :4286). Three edits, all inside this block.

**(a) The declaration (Rel.cpp:4289), BEFORE:**
```cpp
std::unordered_map<uintptr_t, unsigned> inst_per_pub;
```
**AFTER:**
```cpp
// D3.a.3 (g5/ADV-1, RULE-AT-CODE O1): per-pub uniqueness re-keyed on
// (pub_table, forcing_index). N adornments of one query name share the pub
// model table (Rel.cpp:992 resolves pub by q_decl.Id() = name+arity), so a
// per-pub-POINTER tally counts N and false-aborts under multi-adornment. The
// LEGAL deriver count per (pub, forcing) is exactly 1 — each live forcing mints
// one instantiate. std::map (not unordered_map): std::pair has operator< for
// free, no custom hash; the tally is compile-time and tiny.
std::map<std::pair<uintptr_t, unsigned>, unsigned> inst_per_pub;
```
Rationale for `std::map` over a hashed `unordered_map`: `std::pair<uintptr_t,unsigned>` has a
total order out of the box, so `std::map` needs no user hash functor; a composed single-`uintptr_t`
key (e.g. `ptr ^ (forcing_index*PRIME)`) is REJECTED — pointer bits and index bits cannot be
packed collision-free into 64 bits, and a false collision would silently mask a real double-mint.
`std::map` is the diff-minimal, collision-proof choice for a validator-local tally.

**(b) The increment (Rel.cpp:4405-4407), BEFORE:**
```cpp
if (op.table_op_table) {
  ++inst_per_pub[reinterpret_cast<uintptr_t>(op.table_op_table)];
}
```
**AFTER:**
```cpp
if (op.table_op_table) {
  ++inst_per_pub[{reinterpret_cast<uintptr_t>(op.table_op_table),
                  op.forcing_index}];
}
```

**(c) The abort predicate (Rel.cpp:4454-4458) — UNCHANGED logic, message sharpened:**
```cpp
for (const auto &kv : inst_per_pub) {
  if (kv.second != 1u) {
    ValidatorFail("V-INST-SOLE: a (published table, forcing) pair has more "
                  "than one SUBGRAPH_INSTANTIATE deriver");
  }
}
```
The `!= 1u` test is byte-for-byte the same predicate, now iterating pair keys. The message gains
`, forcing)` to name the sharpened key (a text-only diagnostic change — no corpus program reaches
this exit path, so no golden moves).

### 1.3 Why this stays a real bug-catcher (protection-preserving)

Each live forcing mints EXACTLY one instantiate (the mint loop pushes one kSubgraphInstantiate per
`RecognizedSubgraph`, Rel.cpp:1051/1109, with `inst.forcing_index = rs.forcing_index` :1117 and a
distinct `sid = flow.instances.size()` :1077). So `inst_per_pub[{pub, forcing}]` is 1 in every
legal flow; a mint bug that emitted TWO instantiates for ONE forcing still trips `!= 1u`. The
residual is backed by two INDEPENDENT authorities that O1 does not touch:
- the census recount `expect(kSubgraphInstantiate, exp_instance)` — **Rel.cpp:3964-3974**, which
  loops `RecognizedSubgraphs()`, resolves each live forcing, and `++exp_instance/++exp_seal` per
  forcing (`++exp_death` only for `TableIsDifferential(demand_table)`); this is the total-count
  authority that scales to N automatically (`3997`: *"The instance census expectations scale with
  the number of live recognized subgraphs"*).
- per-store `inst_per_store[sid] != 1u` (Rel.cpp:4460-4470, `n_inst != 1u` at :4467) — catches a
  double-mint into ONE store.
O1 keeps the per-(pub,forcing) sanity ON TOP of both.

### 1.4 [BYTE] pre-registration for the O1 change

- **Single-adornment corpus: [BYTE].** For every current program each pub has exactly one forcing
  → `inst_per_pub[{pub, 0}] == 1` → no abort, identical to the pre-change `inst_per_pub[pub] == 1`.
  A validator EMITS NOTHING; the mint already produces the same op count regardless of how the
  tally is keyed. No emitted byte moves.
- Gates: **SUITE** (178 existing cases byte-identical), all **20 pinned .irgold** [BYTE], the
  **4 nested witness dumps** (`demand_neighborhood_witness` / `demand_neighborhood_mono_witness` /
  `demand_diff_neighborhood_witness` / `demand_diff_input_1`) [BYTE], **eqgate** PASS unchanged,
  **config-invariance** single-hash unchanged. The 11 `.rel` census goldens [BYTE] (the census
  line does not surface `inst_per_pub`'s internal keying).
- **ctest RelValidators:** existing single-adornment ValidateDROps death tests are UNAFFECTED (they
  build single-forcing flows; `{pub,0}` count 1). A NEW forked death test is WARRANTED and is
  carried by **g2 design-1**: hand-build a flow with TWO instantiates for ONE forcing_index over
  one pub and assert the sharpened V-INST-SOLE aborts (teeth on the relaxed key — a masked-negative
  guard per the g8/L15 lesson: the relaxation must still bite the same-forcing double-mint). Also a
  positive test: two instantiates over one pub with DISTINCT forcing_index → PASSES (the N-store
  shape the old keying wrongly rejected).

===============================================================================
## DELIVERABLE 2 — ADV-10: PER-STORE FIVE-WAY COUPLING (confirmation, NO code change)

**Claim: the D3.a.2 five-way coupling + the band-(b) publish are per-store (namespaced by
`region.StoreId()`); N regions over N stores sharing one pub have NO cross-store TouchedFlag /
current / Present / demand-liveness aliasing.** CONFIRMED at code, no change needed.

### 2.1 Per-store namespacing of ALL store state (Database.cpp:2391-2564)

`EmitSubgraphInstance` derives ONE store name per region:
- **Rel/Database.cpp:2391-2392** — `const auto id = std::to_string(region.StoreId());`
  `const auto sname = "instance_" + id;`

EVERY store-state operation is emitted against `sname`, so N regions emit N disjoint
`instance_<id>` members with zero shared mutable state:
- V-INST-FRESH occupancy: `sname << ".WorkingOccupied(iid)"` (:2425), `sname << ".TouchCurrent(iid)"` (:2432)
- band-(a0) death: `sname << ".FindInstance(...)"` (:2510), `sname << ".RecycleCurrent(iid)"` (:2514)
- band-(a1) birth: `sname << ".FindOrAddInstance(...)"` (:2531), `sname << ".TouchedFlag(iid)"` (:2534)
- band-(a2) edge rebuild: `sname << ".FindInstance(...)"` (:2563)
- band-(b) publish: `sname << ".NumTouched()"` (:2718), `sname << ".Touched(t)"` (:2721),
  `sname << ".Current(iid)"` (:2722), `sname << ".Frozen(iid)"` (:2723),
  `sname << ".CurrentContains(iid, fr)"` (:2729)

`iid`, `cur`, `frz` are per-iteration LOCALS scoped to the region body. The demand-liveness gate
(mechanism 5) reads `table_member[region.DemandTable().Id()]` (:2602) — the store's OWN demand
table (each adornment fabricates a DISTINCT `demand__q_<adorn>` message → distinct demand table).
**No TouchedFlag / current / Present bit is shared across stores.**

### 2.2 The SHARED resources are read/folded safely (per the coupling comment's own invariants)

The only cross-store-shared handles are the input and the pub — both handled non-destructively:
- **Shared INPUT frontier, read-only:** band-(a2) iterates `VecName(input_front)` with a RANGE-FOR
  (`for (const auto &[...] : ...)`, Database.cpp:2560) — never a drain/clear. Two stores summarizing
  the same `edge_2` both iterate the same per-epoch-fresh frontier independently. The rescan mold
  scans `input_member.NumRows()` read-only (:2433-2434). This is exactly the coupling comment's
  mechanism-4 contract (:2367-2373, *"the mold materializes exactly the epoch-net LIVE input rows"*).
- **Shared PUB, reference-counted fold:** band-(b) folds each store's OWN current-vs-frozen delta
  into the shared `pub_member` via `SubDerivation` (:2737-2743, dropped = frozen\current) and
  `AddDerivation`/`TryAdd` (:2755-2764, born = current\frozen) — **NEVER a blind copy** (the RAT-4
  reference-counted-publish comment, Database.cpp:2712-2717: *"pub may be demanded by other
  subgraphs/rules (shared-model)"*). Monotone pub uses idempotent `TryAdd` (:2763) → set-union;
  differential pub accumulates `C_nr` counters. Two stores folding +1 into pub COMMUTE (the
  linearizer serializes the forward WAW by construction index, Rel.cpp:5299-5305, laneC #33).
  A row demanded by BOTH adornments' probes is `TryAdd`ed twice (monotone → idempotent) or gets
  `C_nr=2` (differential → losing one adornment leaves the row present, `was!=now` false → no
  spurious delta) — answer-identical to flat `-demand`.

### 2.3 The coupling comment is DESIGN-CORRECT for N stores as written

The Database.cpp:2343-2390 five-way block is written per-region ("*For a keyed-instance store...
each touched key rebuild...*") and every mechanism it names is per-store (2/3/4/5) or per-channel
at the message boundary (1, netting — each fabricated demand channel + the shared edge channel are
NetBatched independently, :2350-2353). Its C-REC precondition (:2385-2389, recursive-content input
fence) is orthogonal to the adornment axis. **No edit to the coupling block is needed.** Its OD-15
"one block every design touching the input rebuild band must quote" mandate is SATISFIED by
citation — the multi-adornment slice touches neither the band nor the gate, only the count of
regions. **DESIGN NOTE, not a code change.**

===============================================================================
## DELIVERABLE 3 — THE g6 NEW SUCCESS WITNESS

### 3.1 OBSERVED behavior grounding (scratch compiles, this session)

I recreated the §2.3 seed and compiled all three shapes with the debug compiler (results captured
to `scratch/results.txt`, `scratch/sigs.log`, `scratch/bf.rel`):

| program | `-demand` | `-demand -demand-instance` |
|---|---|---|
| `both` (bf **and** fb declared) | **exit 1**, `Multi-adornment demand is not yet supported ... under -demand` (Demand.cpp:457) | **exit 1**, SAME reject (DataFlow rejects before nested pre-pass) |
| `bf` alone (`q(bound A, free B)`) | **exit 0** | **exit 0** |
| `fb` alone (`q(free A, bound B)`) | **exit 0** | **exit 0** |

Both adornments compile IN ISOLATION and BOTH are From-preserving (each bound column traces to
`edge_2`'s own position via the direct copy `rel(A,B):edge_2(A,B)`); the ONLY blocker is the
per-name :457 reject b1 lifts. This is the intended post-lift success case.

### 3.2 The witness name + the `.dr`

**Name: `demand_multi_adorn_witness`** (mirrors the `demand_*_witness` family; distinct from the
retained-diagnostic `demand_multi_adorn_1`).

`tests/OptDiff/cases/demand_multi_adorn_witness.dr`:
```
; The multi-adornment success witness (D3.a.3): ONE query name `q` with TWO
; declared binding patterns (bf + fb), both From-preserving over a non-recursive
; copy of a directed edge relation. Post-g3-fence-lift this compiles to N=2
; disjoint keyed-instance stores sharing ONE pub (`q`). The bf probe enumerates
; out-neighbors-by-A, the fb probe enumerates in-neighbors-by-B; the shared pub
; is the reference-counted union of what either adornment demands.
#message edge_2(u64 A, u64 B).
#local rel(u64 A, u64 B).
rel(A, B) : edge_2(A, B).
#query q(bound u64 A, free u64 B).
#query q(free u64 A, bound u64 B).
q(A, B) : rel(A, B).
```

### 3.3 The `.drflags` + `.eqgate` sidecars (modeled on `demand_neighborhood_mono_witness`)

- `demand_multi_adorn_witness.drflags` → `-demand`  (the FLAT/MONO arm; the golden is blessed
  from this, byte-compared across all 4 optimization modes).
- `demand_multi_adorn_witness.eqgate` → `-demand -demand-instance`  (the NESTED arm; run_eqgate
  re-compiles with the same driver in all 4 modes and byte-compares each mode's stdout LIVE against
  `goldens/demand_multi_adorn_witness.stdout` — flat==nested==golden falls out transitively).

**DECISION — mono flagship only; NO diff arm (no second witness) in this slice.** Rationale:
1. The slice's headline (OD-15) is *N disjoint stores over one pub*; the minimal non-recursive mono
   shape is the HONEST FLOOR that exercises N=2 stores, the shared-pub reference-counted fold, and
   the O1 per-pub validator relaxation end-to-end — the exact three surfaces this lane lands.
2. A recursive body preserving BOTH declared positions is **not readily constructible**: any LINEAR
   recursion preserves only one bound position's demand (right-linear preserves From, left-linear
   preserves To), so the sibling adornment trips the left-linear reject (Demand.cpp:717-722) — this
   is precisely why `demand_multi_adorn_1` (right-linear TC) STAYS diagnostic (substrate §2.2). The
   non-recursive copy is the ONLY clean both-positions-preserving shape.
3. The diff × instance composition (`kInstanceDeath` under retractable demand) is ALREADY covered
   single-adornment by the d3a2 witnesses `demand_diff_neighborhood_witness` + `demand_diff_input_1`.
   A diff × MULTI-adornment cross would be NEW coverage but doubles the witness scope and adds an
   `@differential` tap + `-demand-retract` arm for marginal return. **RECORDED as a follow-up
   candidate (a `demand_multi_adorn_diff_witness` under `-demand -demand-retract`), explicitly
   DEFERRED — not required to discharge g6.** The eqgate family lifts 4→5, not 4→6.

### 3.4 The `.main.cpp` driver

Exact query surface (VERIFIED from the generated headers, `scratch/gbf/datalog.h`,
`scratch/gfb/datalog.h`):
- `q_bf_cursor q_bf(Database &db, Log &, Functors &, uint64_t A)` — cursor `bool next(uint64_t &B)`
  yielding every B with `q(A,B)` = **out-neighbors of A**. (datalog.h:154/170, cursor drains
  `db.q_4.RowAt(id).b` over `idx_57` keyed on the bound A.)
- `q_fb_cursor q_fb(Database &db, Log &, Functors &, uint64_t B)` — cursor `bool next(uint64_t &A)`
  yielding every A with `q(A,B)` = **in-neighbors of B**. (datalog.h:154/170 in the fb compile.)

Both are FORCING queries: the call injects demand for the bound key (`inject_53(...)`, datalog.h:19)
before returning the cursor. In the COMBINED compile both `q_bf` and `q_fb` friends are emitted,
both scanning the SHARED pub table `q` (the N=2-stores-one-pub shape).

`tests/OptDiff/cases/demand_multi_adorn_witness.main.cpp`:
```cpp
// Copyright 2026, Peter Goodman. All rights reserved.
//
// Driver for demand_multi_adorn_witness — the D3.a.3 multi-adornment success
// witness. ONE query name `q` with TWO declared binding patterns; the bf probe
// enumerates out-neighbors-by-A, the fb probe enumerates in-neighbors-by-B.
// Each probe asserts EXACTLY the expected set (an over-materialized nested arm
// both diverges and, if it over-derives across stores, aborts a belt — HP-5).
//
// The eqgate sidecar re-links this driver against the -demand-instance (nested,
// N=2 store) compile in all 4 optimization modes and byte-compares each mode's
// stdout LIVE against the flat golden.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <tuple>
#include <vector>

#include "datalog.h"

int main() {
  ::hyde::rt::StdErrorLog log;
  DatabaseFunctors functors;
  ::hyde::rt::Allocator alloc;
  Database db(alloc);
  init(db, log, functors);

  // Directed edges: a small in/out-neighborhood component {1,2,3,4} plus an
  // OUT-OF-NEIGHBORHOOD component {10,11,12} that must never surface in a probe
  // scoped to the first component (the HP-5 over-materialization catch).
  const std::vector<std::tuple<uint64_t, uint64_t>> edges = {
      {1, 2}, {1, 3}, {2, 4}, {10, 11}, {11, 12},
  };
  Vec<uint64_t, uint64_t> ev(alloc);
  for (auto &[a, b] : edges) {
    ev.Add(a, b);
  }
  edge_2(db, log, functors, ev);

  // CURSOR CONTRACT: drain each cursor FULLY before the next entry-point call;
  // sort every drain (enumeration order is unspecified).
  auto probe_bf = [&](uint64_t a) {          // out-neighbors of a
    std::vector<uint64_t> got;
    auto c = q_bf(db, log, functors, a);
    uint64_t b;
    while (c.next(b)) { got.push_back(b); }
    std::sort(got.begin(), got.end());
    return got;
  };
  auto probe_fb = [&](uint64_t b) {          // in-neighbors of b
    std::vector<uint64_t> got;
    auto c = q_fb(db, log, functors, b);
    uint64_t a;
    while (c.next(a)) { got.push_back(a); }
    std::sort(got.begin(), got.end());
    return got;
  };
  auto emit = [&](const char *tag, uint64_t k, const std::vector<uint64_t> &v) {
    std::printf("%s(%llu) =", tag, (unsigned long long) k);
    for (auto x : v) { std::printf(" %llu", (unsigned long long) x); }
    std::printf("\n");
  };

  // bf store (adornment 0): out-neighbors.
  emit("bf", 1, probe_bf(1));     // expect: 2 3
  emit("bf", 2, probe_bf(2));     // expect: 4
  emit("bf", 10, probe_bf(10));   // expect: 11   (other component)
  // fb store (adornment 1): in-neighbors.  Interleaved to exercise both stores
  // over the shared pub within one run.
  emit("fb", 4, probe_fb(4));     // expect: 2
  emit("fb", 2, probe_fb(2));     // expect: 1
  emit("fb", 11, probe_fb(11));   // expect: 10  (other component)
  emit("fb", 3, probe_fb(3));     // expect: 1
  return 0;
}
```
The blessed `goldens/demand_multi_adorn_witness.stdout` (deterministic, sorted) is:
```
bf(1) = 2 3
bf(2) = 4
bf(10) = 11
fb(4) = 2
fb(2) = 1
fb(11) = 10
fb(3) = 1
```
(Blessed from the FLAT `-demand` run after review; the eqgate proves the nested N=2-store run
produces byte-identical stdout.) The probes deliberately hit BOTH components under BOTH adornments,
proving no cross-store / cross-component over-materialization (HP-5) and that the shared pub is the
reference-counted union of both adornments' demanded rows.

### 3.5 `.batches` oracle — NOT warranted

The witness is MONOTONE (bare `-demand`, no `@differential`, no retraction) → no signed derivation
deltas to count. The `.batches` oracle (`bin/Oracle`) is the differential derivation-counter
referee; a monotone program has nothing for it to check. The equivalence authority IS the eqgate
(flat==nested==golden across 4 modes). **No `.batches`, no `.oracle.stdout`, no `.monotone.stdout`.**
(This matches the `demand_neighborhood_mono_witness` precedent exactly — eqgate-only, no oracle.)

### 3.6 OPTIONAL but RECOMMENDED — a `.rel` census pin (the 12th `.rel` golden)

The N=2 instance census (`kSubgraphInstantiate=2`) is validated at compile by the always-on
V-CENSUS recount (Rel.cpp:3964-3974) and the answer is locked by the eqgate — but NOTHING in the
corpus currently PINS a multi-store `.rel` census as a golden (the existing 11 `.rel` pins are all
single-store or non-instance). Since `kSubgraphInstantiate=2` is the HEADLINE deliverable of the
whole slice, I RECOMMEND adding `demand_multi_adorn_witness.rel` + its `.irgold` sidecar
(under `-demand -demand-instance`) as the 12th `.rel` census pin — the only multi-store census
witness in the corpus, a high-value defensive lock on the N-store shape. This DEVIATES from the
eqgate-only neighborhood-witness precedent; if the slice is kept minimal it can be dropped (the
V-CENSUS validator + eqgate already gate the shape). **Default: ADD the `.rel` pin** (census below
is the predicted golden content).

===============================================================================
## DELIVERABLE — THE CENSUS ARITHMETIC (grounded, not guessed — the D3.a.1 lesson)

### C.1 The measured single-adornment decomposition (`scratch/bf.rel`, nested)

For `bf` alone under `-demand -demand-instance` (VERIFIED census line, bf.rel:44):
```
kSeedFold=0 kCommitSweep=2 kIngestFold=2 kSubgraphInstantiate=1 kInstanceDeath=0
kInstanceSeal=1 kEagerForward=2   (all other kinds 0)
```
The op-by-op decomposition (bf.rel:6-30) partitions cleanly into SHARED (input/pub, counted once
under N) vs PER-ADORNMENT (demand-side, ×N):

| op | table / message | class |
|---|---|---|
| kIngestFold (op.4) | `edge_2/2` → table:11 | **SHARED** (input) |
| kIngestFold (op.5) | `demand__q_bf/1` → table:8 | **PER-ADORNMENT** (demand) |
| kEagerForward (op.6) | table:11 (edge/rel) | **SHARED** (input side) |
| kEagerForward (op.7) | table:8 (demand relation) | **PER-ADORNMENT** (demand) |
| kCommitSweep (op.3) | table:11 (`edge_2`) | **SHARED** (input) |
| kCommitSweep (op.2) | table:8 (demand) | **PER-ADORNMENT** (demand) |
| kSubgraphInstantiate (op.0) | i#0 | **PER-ADORNMENT** |
| kInstanceSeal (op.1) | i#0 | **PER-ADORNMENT** |

### C.2 The predicted combined N=2 census (both adornments, mono)

Applying `combined = shared·1 + per_adorn·2`:

| kind | value | derivation |
|---|---|---|
| `kSubgraphInstantiate` | **2** (=N) | one mint per live RecognizedSubgraph (Rel.cpp:1051/1109); recount `exp_instance=2` (:3969) |
| `kInstanceSeal` | **2** (=N) | 1:1 with instantiate (Rel.cpp:1176 always; recount :3970) |
| `kInstanceDeath` | **0** | monotone demand → `TableIsDifferential(demand_table)` false → gate Rel.cpp:1162 mints none; recount :3972-3973 counts 0 |
| `kIngestFold` | **3** | 1 shared `edge_2` (`MakeMonotoneIngestFold`) + 2 fabricated demand receives (`demand__q_bf`, `demand__q_fb`) |
| `kEagerForward` | **3** | 1 shared input-side (edge/rel) + 2 per-adornment demand-relation forwards |
| `kCommitSweep` | **3** | 1 shared `edge_2` + 2 per-adornment demand-relation sweeps |
| **`kSeedFold`** | **0** | **MEASURED 0 for the non-recursive copy shape** — the demand-side web mints ZERO seed folds here (the guard JOINs lower to eager forwards, not seed folds). This CORRECTS the substrate/laneC placeholder `shared + 2·per_adorn_demand_seeds`: for THIS minimal shape `per_adorn_demand_seeds = 0`, so `kSeedFold = 0`, not a positive integer. The D3.a.1 under-prediction lesson applied HONESTLY by MEASURING rather than guessing. |
| all others | **0** | kCrossover/kProductArm/kFixpointFire/kChainFold/kClaimDrain/kRetire/kRederive/kFrontierFilter/kNegateGate/kPivotAssemble/kGroupUpdate/kStateSeal/kEager{Insert,Compare,Generate,Union,Select,Join,Product}/kIngestLoop/kJoinEmit/kProductEmit — all 0 in the single-adornment measure; nothing new under N |

**The full predicted `demand_multi_adorn_witness.rel` census line (the .irgold golden content):**
```
census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0
kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=3
kNegateGate=0 kPivotAssemble=0 kIngestFold=3 kGroupUpdate=0 kStateSeal=0
kSubgraphInstantiate=2 kInstanceDeath=0 kInstanceSeal=2 kEagerForward=3
kEagerInsert=0 kEagerCompare=0 kEagerGenerate=0 kEagerUnion=0 kEagerSelect=0
kEagerJoin=0 kEagerProduct=0 kIngestLoop=0 kJoinEmit=0 kProductEmit=0
```
**CAVEAT (honesty flag):** this census is a PREDICTION — the combined program cannot compile until
b1 lifts Demand.cpp:457, so I could not MEASURE the combined `.rel` directly (house rules + the
fence). It is grounded in the MEASURED single-adornment decomposition (C.1) + the shared/per-adorn
factoring, which the whole D3.a substrate rests on. **Stage (c) MUST confirm this exact census
against the first compiled two-adornment nested flow before blessing the `.irgold`.** The three
N-scaled counts (`kIngestFold=3`, `kEagerForward=3`, `kCommitSweep=3`) and `kSeedFold=0` are the
lines to re-verify.

### C.3 Per-pub-option independence

The census is a pure function of (N live forcings, per-forcing demand differentiality) — INDEPENDENT
of how V-INST-SOLE keys its tally. O1 decides only whether the validator ABORTS on the N instantiate
ops, not how many are minted. (**O2 alone would perturb kSubgraphInstantiate/kInstanceSeal N→1** — a
census divergence + an OD-15 conflict; a further reason O1 is the faithful choice. Not adopted.)

===============================================================================
## DELIVERABLE 4 — THE GOLDEN / SUITE CHURN

- **Suite: 178 → 179** (+1: `demand_multi_adorn_witness`). `symrec_tie_1` etc. unchanged.
- **eqgate carriers: 4 → 5** (`demand_neighborhood_witness`, `demand_neighborhood_mono_witness`,
  `demand_diff_neighborhood_witness`, `demand_diff_input_1`, **+ `demand_multi_adorn_witness`**).
- **`.rel` census pins: 11 → 12** (IF Deliverable 3.6 is adopted — the recommended default).
- **Phase-A red set: EXACTLY +1**, all GOLDEN-MISSING:
  `demand_multi_adorn_witness` (missing `goldens/demand_multi_adorn_witness.stdout`, and, if 3.6
  adopted, missing `goldens/demand_multi_adorn_witness.rel`). No EXISTING golden goes red from this
  lane (the O1 change is [BYTE]).
- **NEW golden files (bless-created):**
  - `tests/OptDiff/goldens/demand_multi_adorn_witness.stdout` (the 7-line sorted probe output above)
  - `tests/OptDiff/goldens/demand_multi_adorn_witness.rel` (+ `cases/*.irgold` sidecar) — IF 3.6 adopted
- **NEW case files (hand-authored, not blessed):**
  - `tests/OptDiff/cases/demand_multi_adorn_witness.dr`
  - `tests/OptDiff/cases/demand_multi_adorn_witness.main.cpp`
  - `tests/OptDiff/cases/demand_multi_adorn_witness.drflags` (`-demand`)
  - `tests/OptDiff/cases/demand_multi_adorn_witness.eqgate` (`-demand -demand-instance`)
- **CLAUDE.md / runall.sh:** the eqgate-carrier list and the suite count (178→179) update; the new
  case is a GOLDEN case (NOT an expected-diagnostic case — it is NOT added to runall.sh's diagnostic
  list; `demand_multi_adorn_1` STAYS in that list, substrate §2.2). **Cross-lane note:** `demand_multi_adorn_1`'s
  golden/diagnostic disposition is owned by b1/g3 (its `fb` half's reject moves :457→:717-722, still
  diagnostic) — this lane does NOT touch it; the two cases coexist (one diagnostic, one golden).

===============================================================================
## DELIVERABLE 5 — PRE-REGISTERED GATES

| surface touched | gate | prediction |
|---|---|---|
| O1 map-key change (Rel.cpp:4289/4405-4407/4454-4458) | **SUITE** (178 existing) | **[BYTE]** — validator emits nothing; single-adornment `inst_per_pub[{pub,0}]==1` unchanged |
| O1 | **20 pinned .irgold** | **[BYTE]** |
| O1 | **4 nested witness dumps** | **[BYTE]** |
| O1 | **eqgate** (4 existing carriers) | **[BYTE]** PASS |
| O1 | **config-invariance** single-hash | **[BYTE]** |
| O1 | **ctest RelValidators** | existing PASS; **+1 NEW forked death test** (double-mint-one-forcing aborts) + **+1 positive** (two forcings one pub PASSES) — carried by g2 design-1 |
| O1 | **ctest** (all suites) | PASS |
| new witness (Deliverable 3) | **SUITE** | **179 SUITE: PASS**; new case green in all 4 modes vs its blessed golden |
| new witness | **Phase-A red** | **EXACTLY +1** GOLDEN-MISSING (`demand_multi_adorn_witness.stdout` [+ `.rel` if 3.6]) |
| new witness | **eqgate** | **[STRUCT→answer-identical]** flat==nested==golden (N=2 stores, ids re-anchored per store; the observable stdout is byte-identical) |
| new witness `.rel` (if 3.6) | **census .irgold** | **[STRUCT]** — the C.2 predicted census (`kSubgraphInstantiate=2` etc.); ids re-anchored, census counts pinned |
| new witness | **ASAN** (compile + run) | PASS — **REQUIRES g2's always-on [F] fence** (else N-message nullptr callee SIGSEGV) |
| whole slice | **census (V-CENSUS always-on)** | PASS — recount scales to N (Rel.cpp:3964-3974) |

**[BYTE] surfaces:** the entire pre-existing corpus + all 4 pre-existing eqgate carriers + 20
.irgold + 11 .rel goldens (the O1 change is byte-neutral).
**[STRUCT] surfaces:** ONLY the new witness's own outputs (its stdout golden is byte-fixed; its
`.rel` census, if pinned, has store-id-anchored ids with pinned counts).

### 5.1 Co-land / dependency summary (the merge contract with siblings)

- **b4's O1 change** merges independently (no shared lines with g1/g2/g3); it is co-committed but
  compiles and passes [BYTE] gates on the pre-b1 tree standalone.
- **b4's witness** is BLOCKED on **b1** (fence-lift, to compile the two-adornment `.dr`) and on
  **b2/g1** (the always-on [F] fence, to RUN it under ASAN/release without SIGSEGV; the kBody-survivor
  policy is dormant here — §3.2 — but must be correct). The witness's golden cannot be blessed until
  b1 lands; stage (c) blesses it as part of the one slice.
- **No line-level conflict** with any sibling: Deliverable 1 touches Rel.cpp:4289/4405/4455 (the
  validator block); g1 touches View.cpp:675-682 + the predicate; g3 touches Demand.cpp:444-462/668/
  723/770/983; g2 touches Build.cpp:393. Disjoint hunks.

===============================================================================
## APPENDIX — VERIFIED ANCHORS (this lane, re-read at code)

- `Rel.h:713` — `unsigned forcing_index{~0u};` on the DROp (available at validate time, no threading).
- `Rel.cpp:1117` — `inst.forcing_index = rs.forcing_index;` (mint stamp; also :1170 death, :1180 seal).
- `Rel.cpp:4289` — `std::unordered_map<uintptr_t, unsigned> inst_per_pub;` (the decl to change to a pair-keyed `std::map`).
- `Rel.cpp:4405-4407` — `++inst_per_pub[reinterpret_cast<uintptr_t>(op.table_op_table)];` (the increment to re-key).
- `Rel.cpp:4454-4458` — `if (kv.second != 1u) { ValidatorFail("V-INST-SOLE: a published table has more than one ...") }` (abort; predicate UNCHANGED).
- `Rel.cpp:3964-3974` — the N-scaling census recount (`++exp_instance/++exp_seal`, `++exp_death` iff differential demand) — the independent count authority.
- `Database.cpp:2391-2392` — `sname = "instance_" + std::to_string(region.StoreId());` (per-store namespace).
- `Database.cpp:2425/2432/2510/2514/2531/2534/2563` — per-store WorkingOccupied/TouchCurrent/FindInstance/RecycleCurrent/FindOrAddInstance/TouchedFlag on `sname`.
- `Database.cpp:2560` — `for (const auto &[...] : VecName(input_front))` (shared input frontier read non-destructively).
- `Database.cpp:2712-2717` — RAT-4 reference-counted publish comment (pub is shared-model).
- `Database.cpp:2718-2764` — band-(b): per-store `sname.NumTouched()/Touched/Current/Frozen` scan → `pub_member.SubDerivation`/`AddDerivation`/`TryAdd` into the SHARED pub.
- OBSERVED (scratch, exit codes): `both.dr` rejects :457 under both flags; `bf.dr`/`fb.dr` each exit 0 under both flags; bf nested `.rel` census `kSubgraphInstantiate=1 kInstanceSeal=1 kInstanceDeath=0 kIngestFold=2 kEagerForward=2 kCommitSweep=2 kSeedFold=0`.
- Generated query surface (VERIFIED): `q_bf(db,log,functors,A)`→`next(uint64_t&B)` (out-neighbors); `q_fb(db,log,functors,B)`→`next(uint64_t&A)` (in-neighbors); both scan the shared pub `q_4`.
