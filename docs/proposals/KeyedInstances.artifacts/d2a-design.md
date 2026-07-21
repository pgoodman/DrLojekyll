======================================================================
COMMITTED AT THE §20 EPOCH-CLOSE CHECKPOINT (2026-07-21). This is the
adjudicated BINDING implementation contract for its diff, verbatim from
the session scratchpad (d2a/d2a-design.md); its ADJUDICATION LEDGER records every
folded amendment, and the KeyedInstances.md §19(K)-(O) landing records
carry the owner rulings (RAT-1..RAT-10) that resolved its open items.
======================================================================

# D2.a DESIGN — the InstanceStore runtime primitive + RowStore::Reset()

Repo tip **932ad4a6** (keyed-instances; verified `git rev-parse HEAD`).
Design-only; NO code exists yet. D1.a + D1.b landed upstream of this tip.

D2.a is **INERT**: nothing references `InstanceStore` until D2.b, and
`RowStore::Reset()` is uncalled by generated code until D2.b. The whole diff
is header-and-test only. No `.cpp` in `lib/` changes; no generated-code byte
changes; the golden suite is untouched (P-D2a.1).

## Deliverables (three files)
1. `include/drlojekyll/Runtime/InstanceStore.h` — NEW (R-1; **Peter Goodman
   copyright line ONLY** — this is a NEW file, never the ToB idiom).
2. `include/drlojekyll/Runtime/Table.h` — EDIT: add `RowStore<Row>::Reset()`
   (protected) + `Table<Row>::Reset()` (public, chains base + `sealed = 0`)
   (R-2, A.3.2; the H6 discharge).
3. `tests/InstanceStore/` — NEW dir: `CMakeLists.txt` +
   `InstanceStoreTest.cpp` (the DrTest unit) → **5th ctest target**
   (ctest 4→5, P-D2a.2).

## Binding precedence (top wins)
`d1-pinned.md` (HP-7 frozen⊆current seal assert under R-MONO; HP-16 ON-build
bench-counters gate; §3 D2.a entry; N-1 carried note) **>**
`d1-design-consolidated.md` §A.3 (A.3.1 R-1, A.3.2 R-2, A.3.5 band-b/D3
note). Ledger: KeyedInstances.md §19(K)/(L) + the N-1 carried note (record
the working_count drop in the header comment, revisit at R-DIFF).

Cross-refs to current code are `file:line` at tip 932ad4a6.

======================================================================
## 1. CURRENT-STATE — what D2.a is a transpose / borrow of
======================================================================

### 1.1 The DONOR: `StateCellStore<Key, Algebra>` (StateCell.h:271-640)

The InstanceStore is the StateCellStore mold with the *value cell* replaced
by a *relation double-buffer*. The donor shapes reused verbatim:

- **ctor** (StateCell.h:278-289): `explicit StateCellStore(Allocator)`, one
  Allocator stored + every member Vec constructed from it. Copy deleted
  (:300-301).
- **`FindGroup` / `FindGroupWithHash`** (:309-326, :499-515): open-addressing
  probe over `slots[]`, `slot_capacity` a power of two, `kNoGroup` sentinel,
  `hashes[gid]==hash && keys[gid]==key` confirm. Counters `finds`,
  `probe_steps`.
- **`FindOrAddGroup`** (:330-374): probe; on miss mint the dense id
  `gid = NumGroups()`, exhaustion check vs `kNoGroup`, then `keys.Add` /
  `hashes.Add` / per-column side-Vec `.Add` / `InsertSlot(gid, hash)`.
- **`InsertSlot` / `Rehash`** (:517-555): 7/8-load grow (`(n + n>>3) >=
  slot_capacity`), doubling from 64, `allocator.AllocateArray<uint32_t>` +
  `FreeArray`, re-link every id. Counters `rehash_events`, `rehash_rows`.
- **`Touch`** (:560-567, private): per-group append-once via `touched_flag`,
  appends to `touched`. Counters `touch_calls`, `touch_appends`.
- **`Touched()`** (:486-489): `touched.SortAndUnique(); return touched;`
  returns a **const** ref (the paired dedup structure must never be mutated
  externally — desync of `touched`/`touched_flag`).
- **`KeyAt`** (:491-493): `return keys[gid];`.
- **`Seal()`** (:441-451): `HYDE_RT_BENCH_COUNT_N(commit_visits,
  touched.Size())`, per-touched write the batch-start snapshot +
  `sealed_occupied` bit + clear `touched_flag`, then `touched.Clear()`.
- **the owned-heap-object pool** (:596-624): `MakeVec<VecT>` =
  `allocator.Allocate(sizeof, alignof)` + placement-`new VecT(allocator)`,
  pushed onto a `Vec<VecT*>` pool; `FreeMembership` destroys `p->~VecT()` +
  `allocator.Free`. **This is the exact mold for `MakeTable` / the
  frozen+current table pools** (Table is not trivially copyable, so —
  identical to why `Vec<Working>` can't hold membership Vecs, StateCell.h:194
  — the tables live behind pointers in `Vec<Table<RowT>*>`).
- **`DebugValidate`** (:642-697): `#ifndef NDEBUG`, `assert(touched.Empty())`
  + per-id coherence (`touched_flag==0`, occupancy agreement).
- **the bench-counter seam** (StateCell.h passim): every hot method carries
  `HYDE_RT_BENCH_COUNT(<name>)` from the enumerated X-macro
  (BenchCounters.h:23-52); off-build the macro is `((void)0)` (:74-77) so the
  golden build is byte-neutral.

`working_count` (StateCell.h:346, :406, :630) is the member the transpose
**DROPS** (A.3.1): occupancy comes from `current->NumRows() > 0` instead
(see §2.5, N-1).

### 1.2 The BORROW: `RowStore` internals + the `CompactRowsInPlace` tail

- **RowStore privates** (Table.h:212-216): `Allocator allocator; Vec<Row>
  rows; Vec<uint64_t> hashes; uint32_t *slots{nullptr}; size_t
  slot_capacity{0u};`. These are `private` (Table.h:169) → `Reset` that
  touches them MUST be a `RowStore` member (store-F-5 / A.3.2). `Table<Row>`
  inherits (Table.h:226).
- **The slot-reset loop `Reset` borrows** is the tail of `CompactRowsInPlace`
  (Table.h:135-137):
  ```
  for (size_t i = 0u; i < slot_capacity; ++i) {
    slots[i] = kNoRow;
  }
  ```
  In-place, no allocation — the doc-comment (Table.h:113-115) states the
  invariant: "no allocation happens anywhere here (capacities only shrink)".
- **`Vec::Truncate(n)`** (Vec.h:96-99): `assert(n<=count); count=n;` — capacity
  **retained** ("compaction must not allocate"). `Vec::Clear()` (:90-92):
  `count=0`. Both allocation-free — the Arena-safety keystone (§3).
- **`Table<Row>::sealed`** (Table.h:283): a `private uint32_t` of `Table`, NOT
  of `RowStore`. `Seal()` (:278-280) sets `sealed = NumRows()`; `InI` (:250)
  reads it. The watermark is vestigial for InstanceStore's tables (§2.3), but
  `Reset` must zero it — and it can only be zeroed at the **Table layer**
  (A.3.2), because `RowStore::Reset` cannot see it.

### 1.3 Vec / Allocator surface used

- `Allocator` (Allocator.h:13-35): value type (two ptrs), `Allocate(size,
  align)`, `Free(ptr,size,align)`, `AllocateArray<T>`, `FreeArray<T>`. Under
  `Arena` (`ArenaAllocator`, :71) **`Free` is a no-op** (:42) — freed en
  masse at arena reset.
- `Vec<T>` (Vec.h:27): requires `is_trivially_copyable_v<T>` &&
  `is_trivially_destructible_v<T>` (:28-29). `Add/Size/Empty/operator[]/Set/
  Clear/Truncate/Swap/SortAndUnique/begin/end`. **Consequence:**
  `Vec<Table<RowT>>` is ILL-FORMED (Table has Vecs) → the table pair lives as
  `Vec<Table<RowT>*>`.
- `Table<RowT>` (Table.h:225): `TryAdd(row) -> {added,id}` (:236),
  `Present(id)` (:242), `RowAt(id)` (inherited, :75), `Find(row)` (:80),
  `NumRows()` (:71). `RowT` is a generated aggregate with `Hash()` +
  `operator==` (:52-53). **Index-free** (A.3.1): the store uses `Find`/`TryAdd`
  by whole row — flat's `idx_38` on the guarded copy DISAPPEARS.

======================================================================
## 2. `include/drlojekyll/Runtime/InstanceStore.h` — the transpose
======================================================================

The mental model (A.3.1 + A.3.5): a demand-forcing's recognized nested
relation, keyed by the α bound columns (`Key`), materializes one **standing
nested table per instance key**. To publish batch deltas (band (b)) the store
double-buffers each instance's content:

- `current[iid]` — this epoch's rebuilt content (band-(a) `TryAdd` target;
  monotone within the epoch).
- `frozen[iid]` — last epoch's sealed content (the batch-start "old", the
  `Table::kInI` analogue at the RELATION granularity).

`Seal` makes this-epoch `current` next-epoch `frozen` by a **pointer swap**
and `Reset`s the retired buffer for reuse (no copy, no per-epoch allocation).
Band (b) (codegen, D2.b) publishes born `{(F,T)}` (`current∖frozen`) and
dropped `{(T,F)}` (`frozen∖current`) by the two-scan partition (A.3.5).

### 2.1 File skeleton

```cpp
// Copyright 2026, Peter Goodman. All rights reserved.
//
// The InstanceStore: standing per-instance nested-relation state for ONE
// recognized demand forcing (KeyedInstances §A.3.1, R-1). It is the
// keyed-instances transpose of StateCellStore (StateCell.h): where the cell
// store holds a two-WORD value per dense group id, the instance store holds a
// two-BUFFER relation per dense instance id (iid) — a `current` table (this
// epoch's rebuilt content) and a `frozen` table (last epoch's sealed content,
// the batch-start snapshot / the Table::kInI analogue lifted to relation
// granularity). Seal swaps the two pointers and Resets the retired buffer, so
// no per-epoch copy or allocation occurs.
//
// DENSE iid NAMESPACE, MONOTONE-FOREVER (mirror StateCell.h:10-24): an iid,
// once minted for a key, is retained for the program's life. iids are a
// SEPARATE id space from any pub-table row id; a pub-table compaction never
// touches this store.
//
// NESTED TABLES ARE INDEX-FREE (A.3.1): membership is Table::Find / TryAdd by
// WHOLE row; flat's per-copy hash index (idx_38) disappears — a storage win.
//
// working_count DROPPED (A.3.1; N-1 carried): StateCellStore tracks a signed
// per-group member count; here occupancy is `current->NumRows() > 0`. This is
// exact under R-MONO (band-(a) plus is a monotone TryAdd, no mid-epoch dip).
// N-1 (d1-pinned §CARRIED): when R-DIFF lands (D3.a) a mid-batch retraction
// could make NumRows()-as-occupancy a lie — revisit re-introducing a signed
// count then; recorded here so it is not lost.
//
// BENCH-COUNTER SEAM (HP-16): every counter name below is an ENUMERATED
// HYDE_RT_BENCH_COUNTER_FIELDS name (BenchCounters.h:23-52) — no new field is
// introduced (a new name would require a pre-registered X-macro edit).
// Off-build the macros expand to ((void)0) (BenchCounters.h:74-77): the
// golden build is byte-neutral.

#pragma once

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <utility>

#include "Allocator.h"
#include "BenchCounters.h"
#include "Table.h"     // Table<RowT> + RowStore::Reset (R-2).
#include "Vec.h"

namespace hyde::rt {

// Sentinel dense instance id meaning "no instance" (mirror kNoGroup,
// StateCell.h:74 / kNoRow, Table.h:18).
inline constexpr uint32_t kNoInstance = ~0u;
```

### 2.2 Class shape + members

Template shape per A.3.1 — **two** type params, matching the donor's
`<Key, Algebra>` arity but with `RowT` (the nested relation's row type) in the
Algebra slot, because the "algebra" here is fixed: a monotone `Table<RowT>`.

```cpp
template <typename Key, typename RowT>
class InstanceStore {
 public:
  using Table = ::hyde::rt::Table<RowT>;

  // `monotone` gates the HP-7 seal belt (§2.6). D2 (R-MONO) constructs with
  // the default true; the D3.a R-DIFF lowering (and the death-half unit test)
  // constructs with false. Nothing else reads it.
  explicit InstanceStore(Allocator allocator_, bool monotone_ = true)
      : allocator(allocator_),
        monotone(monotone_),
        keys(allocator_),
        hashes(allocator_),
        frozen(allocator_),
        current(allocator_),
        sealed_occupied(allocator_),
        touched(allocator_),
        touched_flag(allocator_) {}

  ~InstanceStore(void) {
    if (slots) {
      allocator.FreeArray(slots, slot_capacity);
    }
    // Each iid owns exactly two tables; frozen[] and current[] hold both
    // (swapped or not). Destroy + free every one. Under Arena, Free is a
    // no-op and the tables leak into the arena (reset frees them en masse).
    for (uint32_t iid = 0u, n = NumInstances(); iid < n; ++iid) {
      DestroyTable(frozen[iid]);
      DestroyTable(current[iid]);
    }
  }

  InstanceStore(const InstanceStore &) = delete;
  InstanceStore &operator=(const InstanceStore &) = delete;

  // ... methods, §2.3-§2.7 ...

 private:
  Allocator allocator;
  bool monotone;                 // HP-7 belt gate (R-MONO only).
  Vec<Key> keys;                 // Dense iid -> key (KeyAt + debug).
  Vec<uint64_t> hashes;          // Parallel to keys; cached key hashes.
  Vec<Table *> frozen;           // iid -> last epoch's sealed content (old()).
  Vec<Table *> current;          // iid -> this epoch's rebuilt content.
  Vec<uint8_t> sealed_occupied;  // iid -> batch-start occupancy bit.
  Vec<uint32_t> touched;         // iids touched this epoch.
  Vec<uint8_t> touched_flag;     // per-iid append-once bit (kTouched mirror).
  uint32_t *slots{nullptr};      // Open-addressing key -> iid (RowStore mold).
  size_t slot_capacity{0u};
};
```

Note: NO `working_count` Vec, NO `Index`, NO membership pool. The two table
pools ARE `frozen`/`current` (each an owning `Vec<Table*>`).

### 2.3 Find / mint (`FindOrAddInstance = FindOrAddGroup` mold + MakeTable pair)

`FindInstance` is `FindGroup` verbatim (StateCell.h:309-326) with
`kNoInstance`. The mint path allocates the **table PAIR**:

```cpp
  uint32_t NumInstances(void) const noexcept {
    return static_cast<uint32_t>(keys.Size());
  }

  // Dense iid for `key`, or kNoInstance (no allocation). Mirror FindGroup.
  uint32_t FindInstance(const Key &key) const noexcept {
    return FindInstanceWithHash(key, key.Hash());
  }

  // Dense iid for `key`, minting a fresh (empty) instance on first touch.
  // Mirror FindOrAddGroup (StateCell.h:330-374).
  uint32_t FindOrAddInstance(const Key &key) {
    const uint64_t hash = key.Hash();
    if (const uint32_t iid = FindInstanceWithHash(key, hash);
        iid != kNoInstance) {
      return iid;
    }
    const uint32_t iid = NumInstances();
    if (iid == kNoInstance) {
      std::fprintf(stderr, "hyde::rt: instance-id space exhausted\n");
      std::abort();
    }
    keys.Add(key);
    hashes.Add(hash);
    frozen.Add(MakeTable());   // empty batch-start snapshot (never occupied).
    current.Add(MakeTable());  // empty working buffer (V-INST-FRESH holds).
    sealed_occupied.Add(0u);   // never occupied at batch start.
    touched_flag.Add(0u);
    InsertSlot(iid, hash);
    return iid;
  }
```

`MakeTable` is the `MakeVec` mold (StateCell.h:605-611) specialized to `Table`:

```cpp
  Table *MakeTable(void) {
    void *mem = allocator.Allocate(sizeof(Table), alignof(Table));
    return new (mem) Table(allocator);
  }

  void DestroyTable(Table *p) noexcept {
    p->~Table();
    allocator.Free(p, sizeof(Table), alignof(Table));
  }
```

`FindInstanceWithHash`, `InsertSlot`, `Rehash` are the StateCell privates
(:499-555) with `kNoInstance`, `NumInstances()`, and `finds` / `probe_steps`
/ `rehash_events` / `rehash_rows` counters unchanged.

### 2.4 Touch / access / KeyAt (band-(a) entry + band-(b) scan sources)

The band-(a) rebuild (codegen, D2.b) drives its own rescan→TryAdd loop into
the current table, so — unlike StateCell where `Fold` folds `Touch` in — the
store exposes `Touch` and the buffer accessors:

```cpp
  // Record `iid` in the touched set exactly once this epoch, and return its
  // current buffer for the band-(a) rebuild's TryAdd loop. THE band-(a) entry
  // point (codegen calls this once per touched iid, then emits the
  // V-INST-FRESH guard + rescan). Mirror of StateCell.h Touch (:560-567).
  Table &TouchCurrent(uint32_t iid) {
    Touch(iid);
    return *current[iid];
  }

  // Buffers for band-(b)'s two-scan publish (A.3.5): scan current (Find in
  // frozen) for born {(F,T)}; scan frozen (Find in current) for dropped
  // {(T,F)}. Const frozen: the old snapshot is read-only until Seal.
  Table &Current(uint32_t iid) noexcept { return *current[iid]; }
  const Table &Frozen(uint32_t iid) const noexcept { return *frozen[iid]; }

  // The SORT-UNIQUE touched set for band-(b) iteration (mirror StateCell
  // Touched(), :486-489). Const result: the paired dedup structure
  // (touched / touched_flag) must never be mutated externally.
  const Vec<uint32_t> &Touched(void) {
    touched.SortAndUnique();
    return touched;
  }

  bool TouchedFlag(uint32_t iid) const noexcept {
    return 0u != touched_flag[iid];
  }

  const Key &KeyAt(uint32_t iid) const noexcept { return keys[iid]; }
```

`Touch` (private) is StateCell.h:560-567 verbatim (`touch_calls`,
`touch_appends`).

### 2.5 Occupancy (working_count DROPPED — A.3.1, N-1)

```cpp
  // Does the instance have content THIS epoch? (Occupancy = non-empty current
  // buffer.) StateCellStore tracks a signed working_count; the transpose reads
  // NumRows() instead. EXACT under R-MONO: band-(a) plus is a monotone TryAdd,
  // so current only grows within an epoch — no mid-epoch dip below zero.
  // N-1 (carried): under R-DIFF a mid-batch retraction breaks this equivalence
  // — revisit a signed count at D3.a.
  bool WorkingOccupied(uint32_t iid) const noexcept {
    return current[iid]->NumRows() > 0u;
  }

  // Was the instance occupied at batch start? (old() is meaningful only when
  // true.) Mirror StateCell SealedOccupied (:416-418).
  bool SealedOccupied(uint32_t iid) const noexcept {
    return 0u != sealed_occupied[iid];
  }
```

### 2.6 `Seal()` — pointer swap + Reset + occupancy snapshot + HP-7 belt

The double-buffer seal. For each touched iid: (1) HP-7 belt asserts
`frozen ⊆ current` pre-swap under R-MONO; (2) swap the pointers so this
epoch's `current` becomes next epoch's `frozen` (the new "old" baseline);
(3) `Reset` the retired buffer (now at `current[iid]`) so band-(a) begins the
next epoch empty (V-INST-FRESH holds); (4) snapshot `sealed_occupied` from the
new baseline; (5) clear `touched_flag`. Then `touched.Clear()`.

```cpp
  // End-of-epoch, at the commit-sweep tail (A3.4; codegen wiring is D2.b).
  // Mirror of StateCell Seal (:441-451) / Table Seal (:278-280), lifted to a
  // double-buffer swap.
  void Seal(void) {
    HYDE_RT_BENCH_COUNT_N(commit_visits, touched.Size());
    for (uint32_t iid : touched) {
#ifndef NDEBUG
      // HP-7 (d1-pinned §2): under R-MONO, monotone input => instance content
      // monotone-growing => frozen ⊆ current, so the (T,F) drop set is
      // PROVABLY EMPTY. A (T,F) that "cannot exist" trips this LOUD assert
      // instead of silently attempting a retract on a monotone pub table. The
      // check is the exact dual of the codegen V-INST-FRESH guard (A3.3).
      // Gated by `monotone`: R-DIFF (D3.a) and the death-half unit legitimately
      // shrink current, so the belt is R-MONO-only.
      if (monotone) {
        const Table &f = *frozen[iid];
        const Table &c = *current[iid];
        for (uint32_t r = 0u, fn = f.NumRows(); r < fn; ++r) {
          assert(c.Find(f.RowAt(r)) != kNoRow &&
                 "InstanceStore R-MONO belt: frozen row absent from current "
                 "(a (T,F) drop under R-MONO)");
        }
      }
#endif
      // Pointer swap: this epoch's current -> next epoch's frozen. Vec exposes
      // ONLY a const operator[] (Vec.h:80-83), so std::swap(frozen[iid],
      // current[iid]) is ILL-FORMED (fails -Werror, G1) — the swap MUST be
      // spelled via the .Set mutator (Vec.h:85-88) with read-before-overwrite.
      Table *f = frozen[iid];
      frozen.Set(iid, current[iid]);
      current.Set(iid, f);
      current[iid]->Reset();                 // retire old frozen -> empty.
      sealed_occupied.Set(iid,
                          (frozen[iid]->NumRows() > 0u) ? 1u : 0u);
      touched_flag.Set(iid, 0u);
    }
    touched.Clear();
  }
```

[ADJ:crit-runtime-1 / crit-pins-2] The `.Set` triple above is the PRIMARY (and
only compiling) spelling. The earlier draft's readable-shorthand
`std::swap(frozen[iid], current[iid])` was REMOVED: `Vec::operator[]` returns
`const T&` (Vec.h:80-83), so `std::swap` on two element lvalues is ill-formed
(`no matching function for call to 'swap'` — could not bind `int *const`), and
a verbatim transcription of that block fails G1 under `-Werror`. The swap of
two `Vec<Table*>` slots moves two raw pointers (trivial); the local `f`
preserves the frozen pointer across the `frozen.Set` overwrite.

### 2.7 `RecycleCurrent` (the death-arm belt) + DebugValidate + helpers

```cpp
  // Empty the current buffer for one instance — the R-DIFF death arm's belt
  // (A3.3 matched pair with the minus-arm) and the same-epoch demand-flap
  // rebuild. UNCONDITIONAL + IDEMPOTENT: Reset on an already-empty table is a
  // no-op (Truncate(0) on empty + slots already kNoRow), and Touch is
  // append-once, so RecycleCurrent twice in one epoch == once. Touches the iid
  // so Seal visits it (advances sealed_occupied to reflect the emptied state).
  void RecycleCurrent(uint32_t iid) {
    Touch(iid);
    current[iid]->Reset();
  }

  // §A.3.1 coherence validation (mirror StateCell DebugValidate, :642-697).
  void DebugValidate(void) const {
#ifndef NDEBUG
    // SEAL COHERENCE: after Seal, touched is empty and no flag survives.
    assert(touched.Empty());
    for (uint32_t iid = 0u, n = NumInstances(); iid < n; ++iid) {
      assert(0u == touched_flag[iid]);
      // OCCUPANCY COHERENCE: the sealed occupancy bit agrees with the (now
      // stable) frozen baseline. (Post-Seal, frozen holds the sealed content.)
      assert((0u != sealed_occupied[iid]) == (frozen[iid]->NumRows() > 0u));
      // NON-ALIASING: the two buffers of an iid are distinct objects.
      assert(frozen[iid] != current[iid]);
    }
#endif
  }
```

`FindInstanceWithHash`, `InsertSlot`, `Rehash`, `Touch` complete the private
section (StateCell molds, §2.3/§2.4). `namespace hyde::rt` closes.

**Counter audit (HP-16 — every name enumerated in
HYDE_RT_BENCH_COUNTER_FIELDS, BenchCounters.h:24-52):**

| store site | counter | enumerated? |
|---|---|---|
| FindInstance / FindInstanceWithHash | `finds`, `probe_steps` | ✓ :25-27 |
| Rehash | `rehash_events`, `rehash_rows` | ✓ :28-29 |
| Touch | `touch_calls`, `touch_appends` | ✓ :37-38 |
| Seal | `commit_visits` | ✓ :41 |

No `folds_plus`/`member_checks`/etc. minted BY the store. **Zero new counter
names → no BenchCounters.h edit → HP-16 satisfied by construction.**

[ADJ:crit-runtime-2] Attribution note (diagnostic resolution, NOT a
correctness/HP-16 concern): the counter store is a single process-global
`gBenchCounters` (BenchCounters.h:62). The store's own key lookups
(`FindInstanceWithHash`) and the nested `Table`'s membership probes (band-(a)
`TryAdd` → `Table::FindWithHash`) both `HYDE_RT_BENCH_COUNT` into the SAME
`finds`/`probe_steps` fields — they are NOT separable in the aggregate. An
earlier draft phrased band-(a) TryAdd as hitting "the Table's own, not the
store's" counters; there is no such separation. This overstates nothing about
HP-16 (the names are enumerated either way) — it only means the seam cannot be
read as a per-layer probe breakdown.

======================================================================
## 3. `RowStore<Row>::Reset()` + `Table<Row>::Reset()` (R-2, A.3.2 — the H6 discharge)
======================================================================

The reset splits across the two layers because `sealed` (Table.h:283) is a
`Table` private and `rows`/`hashes`/`slots`/`slot_capacity` (Table.h:212-216)
are `RowStore` privates (store-F-5). The meaty part is `RowStore::Reset` (the
log + slots); the Table layer adds `sealed = 0`.

### 3.1 `RowStore<Row>::Reset()` — a new `protected` member

Placed in `RowStore`'s `protected:` block (Table.h:84, beside
`CompactRowsInPlace`, whose slot-reset tail it borrows):

```cpp
  // Reset the log to empty, retaining all backing storage. The slot loop is
  // CompactRowsInPlace's tail (Table.h:135-137); the two Truncate(0)s drop the
  // row/hash logs (Vec::Truncate retains capacity, Vec.h:96-99). Calls NO
  // allocator entry point — Arena-safe: no Free (a no-op under Arena anyway,
  // Allocator.h:22/:42) and no Allocate, so repeated reset/refill cycles never
  // grow the footprint. DISCHARGES H6. A reused RowStore is byte-fresh: an
  // append after Reset re-mints ids from 0.
  void Reset(void) noexcept {
    rows.Truncate(0u);
    hashes.Truncate(0u);
    for (size_t i = 0u; i < slot_capacity; ++i) {
      slots[i] = kNoRow;
    }
  }
```

`protected`, not `public`: the only legitimate caller is a derived table
(`Table::Reset`), never external code that could desync a `Table`'s `sealed`
watermark or a `DiffTable`'s counters/flags from a half-reset log.

### 3.2 `Table<Row>::Reset()` — a new `public` member

Added to `Table`'s public section (near `Seal()`, Table.h:277-280):

```cpp
  // Reset to empty for reuse (the InstanceStore double-buffer, A.3.2). Chains
  // the RowStore log/slot reset, then dissolves the batch-start watermark. NO
  // allocation (RowStore::Reset is allocation-free) — Arena-safe.
  void Reset(void) noexcept {
    this->RowStore<Row>::Reset();
    sealed = 0u;
  }
```

Resolution: `InstanceStore` holds `Table<RowT>*`, so `table->Reset()` binds
`Table::Reset` (the derived, watermark-aware form). `DiffTable` deliberately
gets **no** `Reset` in D2.a — its counters/flags/`touched`/`num_live` would
also need clearing, and no D2 caller needs it (the InstanceStore's tables are
monotone `Table`s). Adding a `DiffTable::Reset` is deferred to whenever
R-DIFF instance content is stored differentially (out of D2 scope).

### 3.3 Arena-safety argument (the H6 discharge, spelled out)

H6 = "a store that Resets-and-refills across epochs must not leak/grow under
the bump Arena (where `Free` is a no-op)". The chain:

1. `Vec::Truncate(n)` sets `count = n`, **keeps `capacity`** (Vec.h:96-99,
   doc-comment: "compaction must not allocate"). No `Free`, no `Allocate`.
2. The slot loop overwrites the **existing** `slots[]` array in place
   (`slot_capacity` unchanged). No `AllocateArray`/`FreeArray`.
3. `sealed = 0` is a scalar store.

So `Reset` performs **zero** allocator calls. After a Reset, a refill of the
same row set re-`Add`s into the retained `rows`/`hashes` capacity and re-slots
into the retained `slots[]` — allocation happens only if the refill exceeds
the previous high-water capacity, which a steady-state instance never does.
Across N Seal/RecycleCurrent cycles the footprint is bounded by the peak
single-epoch content, NOT N × content. Under `MallocAllocator` the same holds
(Truncate frees nothing; only genuine growth mallocs). This is what the H6
regression test (§4.4) asserts.

======================================================================
## 4. THE DRTEST UNIT — `tests/InstanceStore/` (5th ctest target, ctest 4→5)
======================================================================

### 4.0 Target placement decision

**NEW dir `tests/InstanceStore/` → `instance_store_test` → the 5th ctest
target** (currently 4: DeltaRelValidators, MiniDisassembler, PointsTo,
Runtime — tests/CMakeLists.txt:14-17). This honors P-D2a.2's ctest 4→5 and
mirrors DeltaRelValidators landing as its own epoch-scoped dir.

Rationale over folding into `runtime_test` (where the DONOR `StateCellTest.cpp`
lives, tests/Runtime/CMakeLists.txt:4-6): (a) P-D2a.2 is pinned at ctest 4→5;
(b) keyed-instances is its own epoch and its runtime unit is reviewable as one
directory; (c) `runtime_test` stays byte-stable (no re-link of the donor
suite). *Alternative for the owner:* fold `InstanceStoreTest.cpp` into
`runtime_test` — then ctest stays 4 and P-D2a.2's count line amends to 4→4.
Rejected here per the pinned prediction. ASAN reaches the target through
`settings_public`'s global sanitizer interface (CMakeLists.txt:112-120) — no
per-target stanza needed (same as `runtime_test`).

[ADJ:crit-pins-3] Provenance precision: this target mirrors DeltaRelValidators
ONLY in the directory pattern (its own epoch-scoped `tests/<dir>/`); the CMake
CONTENT mirrors `tests/Runtime/CMakeLists.txt` (settings_private +
settings_public + drtest + Runtime), §4.1. It deliberately OMITS the explicit
`if(DRLOJEKYLL_ENABLE_SANITIZERS) ... drlojekyll_sanitizers` stanza that
DeltaRelValidators carries (its link surface pulls compiler-internal libs);
the omission is SAFE — `add_sanitizer_settings(settings_public ...)`
(CMakeLists.txt:111-120) attaches the ASAN/UBSAN flags globally through
settings_public, and `runtime_test` (identically Runtime-only) ran green under
ASAN at D1.b with settings_public alone. So instance_store_test is
ASAN-instrumented with no per-target stanza.

### 4.1 `tests/InstanceStore/CMakeLists.txt` (mirror Runtime/CMakeLists.txt)

```cmake
# Copyright 2026, Peter Goodman. All rights reserved.

add_executable(instance_store_test
  InstanceStoreTest.cpp)

target_link_libraries(instance_store_test
  PRIVATE
    settings_private
    settings_public
    drtest
    Runtime)   # Allocator.cpp: MallocAllocator / Arena / ArenaAllocator.

add_test(
  NAME InstanceStore
  COMMAND instance_store_test)
```

Plus one line in `tests/CMakeLists.txt` (after :14 `add_subdirectory(
DeltaRelValidators)`): `add_subdirectory(InstanceStore)`.

### 4.2 Test-file preamble (`InstanceStoreTest.cpp`)

```cpp
// Copyright 2026, Peter Goodman. All rights reserved.
//
// Unit tests for the InstanceStore (include/drlojekyll/Runtime/InstanceStore.h,
// the keyed-instances runtime primitive, KeyedInstances §A.3.1 R-1). Covers:
// mint/find/collision-growth; Touch/Touched/KeyAt; the double-buffer Seal
// (pointer swap + Reset + occupancy snapshot; frozen becomes prior current);
// RecycleCurrent idempotence (same-epoch flap); the H6 Arena regression
// (bounded allocation across reset/refill cycles); and the pre-D3 death half
// (Recycle-then-partial-readd is a genuine drop under monotone=false).
// Intent-communicating asserts throughout (ASSERT_EQ/LT/GT, EQ on rows).

#include <DrTest.h>

#include <cstddef>
#include <cstdint>
#include <new>
#include <set>
#include <vector>

#include <drlojekyll/Runtime/Allocator.h>
#include <drlojekyll/Runtime/Hash.h>
#include <drlojekyll/Runtime/InstanceStore.h>
#include <drlojekyll/Runtime/Vec.h>

namespace {

using hyde::rt::Allocator;
using hyde::rt::Arena;
using hyde::rt::ArenaAllocator;
using hyde::rt::InstanceStore;
using hyde::rt::kNoInstance;
using hyde::rt::MallocAllocator;

// The instance key (the α bound columns) — e.g. the demanded source node.
struct KeyX {
  int32_t x{0};
  uint64_t Hash(void) const noexcept { return hyde::rt::HashRow(x); }
  bool operator==(const KeyX &) const noexcept = default;
};

// A nested-relation row (e.g. a reached neighbor). A generated aggregate:
// Hash() + operator== (Table.h:52-53).
struct Nbr {
  int32_t v{0};
  uint64_t Hash(void) const noexcept { return hyde::rt::HashRow(v); }
  bool operator==(const Nbr &) const noexcept = default;
};

using Store = InstanceStore<KeyX, Nbr>;

// Count the live rows of an instance buffer by draining its row log (the
// band-(b) scan source). Order-independent membership set.
static std::set<int32_t> Drain(const hyde::rt::Table<Nbr> &t) {
  std::set<int32_t> s;
  for (uint32_t r = 0u, n = t.NumRows(); r < n; ++r) {
    s.insert(t.RowAt(r).v);
  }
  return s;
}
```

### 4.3 Core-mechanism tests

```cpp
// Mint on first touch; find-existing returns the same iid; collision growth
// keeps ids dense and every key findable.
TEST(InstanceStore, FindOrAddMintFindGrow) {
  Store st(MallocAllocator());
  ASSERT_EQ(st.NumInstances(), 0u);
  ASSERT_EQ(st.FindInstance(KeyX{7}), kNoInstance);

  const uint32_t a = st.FindOrAddInstance(KeyX{7});
  ASSERT_EQ(a, 0u);
  ASSERT_EQ(st.NumInstances(), 1u);
  ASSERT_EQ(st.FindInstance(KeyX{7}), a);
  ASSERT_EQ(st.FindOrAddInstance(KeyX{7}), a);   // idempotent find-existing.
  ASSERT_EQ(st.NumInstances(), 1u);              // no new mint.
  ASSERT_EQ(st.KeyAt(a), (KeyX{7}));

  // Force several Rehash growths (7/8 load from 64).
  constexpr int32_t kN = 300;
  for (int32_t i = 1; i < kN; ++i) {
    const uint32_t iid = st.FindOrAddInstance(KeyX{i + 1000});
    ASSERT_EQ(iid, static_cast<uint32_t>(i));   // dense, monotone.
  }
  ASSERT_EQ(st.NumInstances(), static_cast<uint32_t>(kN));
  for (int32_t i = 1; i < kN; ++i) {
    ASSERT_EQ(st.FindInstance(KeyX{i + 1000}), static_cast<uint32_t>(i));
  }
  ASSERT_EQ(st.FindInstance(KeyX{999999}), kNoInstance);
}

// Touch records each iid at most once; Touched() is sorted-unique.
TEST(InstanceStore, TouchAppendOnceTouchedSortedUnique) {
  Store st(MallocAllocator());
  const uint32_t a = st.FindOrAddInstance(KeyX{1});
  const uint32_t b = st.FindOrAddInstance(KeyX{2});
  const uint32_t c = st.FindOrAddInstance(KeyX{3});

  st.TouchCurrent(c);
  st.TouchCurrent(a);
  st.TouchCurrent(a);                 // second touch: no second append.

  ASSERT_TRUE(st.TouchedFlag(a));
  ASSERT_FALSE(st.TouchedFlag(b));
  ASSERT_TRUE(st.TouchedFlag(c));

  const auto &touched = st.Touched();
  ASSERT_EQ(touched.Size(), 2u);
  ASSERT_LT(touched[0], touched[1]);  // sorted.
  ASSERT_EQ(touched[0], a);
  ASSERT_EQ(touched[1], c);
}

// The double-buffer Seal: this epoch's current becomes next epoch's frozen;
// current is reset empty; occupancy snapshot advances.
TEST(InstanceStore, SealSwapsBuffersAndSnapshotsOccupancy) {
  Store st(MallocAllocator());
  const uint32_t g = st.FindOrAddInstance(KeyX{42});

  // Fresh instance: both buffers empty, neither occupancy set.
  ASSERT_EQ(st.Frozen(g).NumRows(), 0u);
  ASSERT_EQ(st.Current(g).NumRows(), 0u);
  ASSERT_FALSE(st.WorkingOccupied(g));
  ASSERT_FALSE(st.SealedOccupied(g));

  // Epoch 1: rebuild current = {10,20}.
  st.TouchCurrent(g).TryAdd(Nbr{10});
  st.Current(g).TryAdd(Nbr{20});
  ASSERT_TRUE(st.WorkingOccupied(g));
  ASSERT_EQ(st.Current(g).NumRows(), 2u);
  ASSERT_EQ(st.Frozen(g).NumRows(), 0u);   // old still empty pre-seal.

  st.Seal();
  ASSERT_EQ(Drain(st.Frozen(g)), (std::set<int32_t>{10, 20}));  // prior cur.
  ASSERT_EQ(st.Current(g).NumRows(), 0u);  // reset — V-INST-FRESH holds.
  ASSERT_TRUE(st.SealedOccupied(g));
  ASSERT_FALSE(st.WorkingOccupied(g));
  st.DebugValidate();                       // touched empty, coherent.

  // Epoch 2: monotone rebuild current = {10,20,30}; frozen ⊆ current holds.
  st.TouchCurrent(g).TryAdd(Nbr{10});
  st.Current(g).TryAdd(Nbr{20});
  st.Current(g).TryAdd(Nbr{30});
  st.Seal();
  ASSERT_EQ(Drain(st.Frozen(g)), (std::set<int32_t>{10, 20, 30}));
  ASSERT_TRUE(st.SealedOccupied(g));
  st.DebugValidate();
}

// RecycleCurrent is unconditional + idempotent: twice == once (same-epoch
// demand flap); Touch stays append-once; a subsequent rebuild + monotone Seal
// is coherent.
TEST(InstanceStore, RecycleCurrentIdempotentSameEpochFlap) {
  Store st(MallocAllocator());
  const uint32_t g = st.FindOrAddInstance(KeyX{5});
  st.TouchCurrent(g).TryAdd(Nbr{10});
  st.Current(g).TryAdd(Nbr{20});
  st.Seal();                                // frozen = {10,20}.

  // Same-epoch flap: partial rebuild, then recycle TWICE.
  st.TouchCurrent(g).TryAdd(Nbr{10});
  ASSERT_EQ(st.Current(g).NumRows(), 1u);
  st.RecycleCurrent(g);
  ASSERT_EQ(st.Current(g).NumRows(), 0u);
  st.RecycleCurrent(g);                     // idempotent.
  ASSERT_EQ(st.Current(g).NumRows(), 0u);
  ASSERT_EQ(st.Touched().Size(), 1u);       // append-once across all touches.

  // Rebuild the full (monotone) content and seal — belt holds.
  st.Current(g).TryAdd(Nbr{10});
  st.Current(g).TryAdd(Nbr{20});
  st.Seal();
  ASSERT_EQ(Drain(st.Frozen(g)), (std::set<int32_t>{10, 20}));
  st.DebugValidate();
}
```

### 4.4 The H6 Arena regression (two observables)

Growth is made **observable** two ways: (a) a counting allocator with real
`Free`, asserting alloc-call count is FLAT after warm-up (the quantitative
bound — the direct H6 statement); (b) a real `Arena` run whose `Free` is a
no-op, exercised under ASAN so a use-after-free across a Reset-reused buffer is
caught. Both run the same reset/refill lifecycle.

```cpp
// A malloc-backed allocator that tallies allocation CALLS (real Free, so ASAN
// leak-checking stays valid). alloc_calls is the growth observable.
struct Tally { uint64_t alloc_calls{0u}; };
static void *TallyAlloc(void *ctx, size_t size, size_t align) {
  static_cast<Tally *>(ctx)->alloc_calls += 1u;
  return ::operator new(size, std::align_val_t(align));
}
static void TallyFree(void *, void *ptr, size_t, size_t align) {
  ::operator delete(ptr, std::align_val_t(align));
}
static Allocator CountingAllocator(Tally &t) {
  return Allocator{&t, &TallyAlloc, &TallyFree};
}

// H6 QUANTITATIVE: after a warm-up epoch mints the tables + grows the row/hash
// logs + slot arrays to their steady-state capacity, every further identical
// reset/refill epoch allocates NOTHING. Reset retains capacity (Vec::Truncate,
// in-place slot loop); the refill re-Adds within the retained capacity.
TEST(InstanceStore, H6SteadyStateAllocationBounded) {
  Tally tally;
  {
    Store st(CountingAllocator(tally));
    const uint32_t g = st.FindOrAddInstance(KeyX{1});

    auto rebuild_and_seal = [&] {
      auto &cur = st.TouchCurrent(g);
      for (int32_t v = 0; v < 64; ++v) {
        cur.TryAdd(Nbr{v});
      }
      st.Seal();
    };

    rebuild_and_seal();          // WARM-UP: allocates the two tables' storage.
    rebuild_and_seal();          // second epoch may still top-up a capacity.
    const uint64_t warm = tally.alloc_calls;

    for (int i = 0; i < 50; ++i) {
      rebuild_and_seal();        // steady state: reset reuses all storage.
    }
    // No unbounded growth: 50 identical epochs allocated nothing.
    ASSERT_EQ(tally.alloc_calls, warm);
  }
}

// H6 LIFETIME: the whole lifecycle under a real bump Arena (Free is a no-op).
// Correctness holds across Reset-reused buffers; ASAN validates no
// use-after-free / bounds error. The Arena frees en masse at scope exit.
TEST(InstanceStore, H6ArenaLifecycleReuseIsSafe) {
  Arena arena(MallocAllocator());
  {
    // Belt OFF: content varies DOWN across epochs (a non-monotone drop the
    // R-MONO belt would reject); this test targets lifetime/reuse safety, not
    // the belt (which the monotone tests above exercise).
    Store st(ArenaAllocator(arena), /*monotone=*/false);
    const uint32_t g = st.FindOrAddInstance(KeyX{9});
    for (int epoch = 0; epoch < 100; ++epoch) {
      st.RecycleCurrent(g);                        // empty before each rebuild.
      auto &cur = st.Current(g);
      const int32_t hi = 8 + (epoch % 4);          // content varies a little.
      for (int32_t v = 0; v < hi; ++v) {
        cur.TryAdd(Nbr{v});
      }
      st.Seal();
      // Prior current is now frozen; content is exactly what we added.
      std::set<int32_t> want;
      for (int32_t v = 0; v < hi; ++v) {
        want.insert(v);
      }
      ASSERT_EQ(Drain(st.Frozen(g)), want);
      st.DebugValidate();
    }
  }
  // Arena destructor frees every chunk here (ASAN: no leak).
}
```

### 4.5 The pre-D3 death half (`monotone=false`)

The ONLY pre-D3 execution of the death arm: RecycleCurrent empties current,
a partial rebuild leaves a genuine `(T,F)` drop, and Seal (belt off) advances
the occupancy snapshot — including occupancy DEATH (occupied→empty). This
exercises that the store SUPPORTS retraction ahead of the D3.a R-DIFF wiring;
the belt-fires-on-a-monotone-drop negative is deferred to the D3.a landing
gate (HP-8) because a tripped `assert` aborts the process (untestable in-proc).

```cpp
TEST(InstanceStore, DeathHalfRecycleThenPartialReaddDropsRows) {
  Store st(MallocAllocator(), /*monotone=*/false);   // belt OFF (R-DIFF mold).
  const uint32_t g = st.FindOrAddInstance(KeyX{3});

  // Epoch 1: current = {10,20,30}; seal -> frozen = {10,20,30}, occupied.
  auto &c1 = st.TouchCurrent(g);
  c1.TryAdd(Nbr{10});
  c1.TryAdd(Nbr{20});
  c1.TryAdd(Nbr{30});
  st.Seal();
  ASSERT_EQ(Drain(st.Frozen(g)), (std::set<int32_t>{10, 20, 30}));
  ASSERT_TRUE(st.SealedOccupied(g));

  // Epoch 2: recycle, re-add ONLY {10} — a genuine drop of {20,30}.
  st.RecycleCurrent(g);
  st.Current(g).TryAdd(Nbr{10});
  ASSERT_TRUE(st.WorkingOccupied(g));
  // Band-(b) would publish dropped = frozen∖current = {20,30}, born = {}.
  const std::set<int32_t> frozen2 = Drain(st.Frozen(g));  // {10,20,30}.
  const std::set<int32_t> cur2 = Drain(st.Current(g));    // {10}.
  ASSERT_LT(cur2.size(), frozen2.size());                 // a real (T,F) drop.
  st.Seal();                                              // belt off: no abort.
  ASSERT_EQ(Drain(st.Frozen(g)), (std::set<int32_t>{10}));
  ASSERT_TRUE(st.SealedOccupied(g));

  // Epoch 3: recycle to EMPTY, no re-add — occupancy DEATH (occupied->empty).
  st.RecycleCurrent(g);
  ASSERT_FALSE(st.WorkingOccupied(g));
  st.Seal();
  ASSERT_EQ(st.Frozen(g).NumRows(), 0u);
  ASSERT_FALSE(st.SealedOccupied(g));                     // sealed occ. died.
  st.DebugValidate();
}
```

======================================================================
## 5. HP-16 GATE — the one-shot ON-build bench-counters check
======================================================================

Purpose (HP-16, d1-pinned §2): prove `InstanceStore.h` uses **only**
enumerated `HYDE_RT_BENCH_COUNTER_FIELDS` names (BenchCounters.h:24-52) — a
non-enumerated name expands under the define to `gBenchCounters.<bad> += 1u`
and fails to compile (:66-71). Because `InstanceStore.h` is INERT at D2.a
(no generated TU includes it yet), the load-bearing exercise is the UNIT TEST
TU, which does include it. The gate also confirms SUITE PASS holds so the
define breaks nothing.

```sh
# (1) Scratch build with counters ON for the whole tree (compiler + ctest
#     targets, incl. instance_store_test which #includes InstanceStore.h).
cmake -B build/benchcount -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DDRLOJEKYLL_ENABLE_TESTS=ON \
  -DCMAKE_CXX_FLAGS='-DDRLOJEKYLL_BENCH_COUNTERS'
cmake --build build/benchcount            # (a) instance_store_test COMPILES
                                          #     => every counter name enumerated

# (2) ctest under the define: InstanceStore + the other four targets green.
( cd build/benchcount && ctest --output-on-failure )

# (3) Corpus + suite under the define (CXX flows the define into diffrun's
#     clang++ line, :74). SUITE: PASS. (At D2.a this exercises the other
#     runtime headers under counters-on; InstanceStore.h joins the generated
#     TUs at D2.b — the pathway is pre-checked here.)
DR=build/benchcount/bin/drlojekyll \
  CXX='clang++ -DDRLOJEKYLL_BENCH_COUNTERS' \
  tests/OptDiff/runall.sh /tmp/d2a-benchcount   # must end 'SUITE: PASS'
```

If any InstanceStore.h counter name were non-enumerated, step (1) fails at
`instance_store_test` compile — the intended loud failure. A genuinely-new
counter would require a pre-registered BenchCounters.h X-macro edit (none is
introduced; §2.7 audit table).

[ADJ:crit-pins-1] **`-DDRLOJEKYLL_ENABLE_TESTS=ON` is MANDATORY on the
benchcount configure line** (added above). It defaults OFF
(cmake/options.cmake:5) and `add_subdirectory(tests)` is guarded by it
(CMakeLists.txt:164-167) — the `debug`/`release` presets set it ON
(CMakePresets.json:17,:28) but this hand-rolled `-B build/benchcount` does not
inherit a preset. WITHOUT the flag, `tests/` is skipped, `instance_store_test`
is never built, and — since InstanceStore.h is INERT at D2.a (the unit TU is
its SOLE includer under the ON define) — step (1) would compile-check NO
InstanceStore.h counter name and step (2) `ctest` would find zero InstanceStore
tests. **The HP-16 gate would pass VACUOUSLY.** The flag is the whole point of
the gate having teeth at D2.a.

======================================================================
## 6. PREDICTIONS + GATES CHECKLIST
======================================================================

### P-D2a.1 — OFF-build byte-neutral, suite untouched
- **Suite 169 green** unchanged: `DR=build/debug/bin/drlojekyll
  tests/OptDiff/runall.sh <work>` ends `SUITE: PASS`, all 169 cases
  byte-identical to tip. InstanceStore.h is included by NO generated TU and
  no `lib/` TU at D2.a; `RowStore::Reset`/`Table::Reset` are uncalled.
- **Zero generated-code byte change:** the two new `Reset` members are
  inert (uncalled); adding them to Table.h changes no emitted C++ (the 676-row
  A/B byte-compare and permcheck lanes stay identical).
- **Off-build bench seam byte-neutral:** without `-DDRLOJEKYLL_BENCH_COUNTERS`
  every `HYDE_RT_BENCH_COUNT` in InstanceStore.h expands to `((void)0)`
  (BenchCounters.h:74-77) — object-file byte-compare of any TU including the
  header is unchanged vs a counter-free variant (verified pattern, the
  StateCell.h precedent).

### P-D2a.2 — the unit passes debug + release + asan; ctest 4→5
- `instance_store_test` compiles and PASSES under **debug** (asserts +
  DebugValidate + the HP-7 belt active), **release** (`-DNDEBUG`: belt +
  DebugValidate compile out; the double-buffer + H6 bound still hold), and
  **asan** (`build/asan`: the Arena-lifetime test is the target — no
  use-after-free across Reset-reused buffers, no leak at Arena teardown).
- **ctest 4→5:** the new `InstanceStore` target joins DeltaRelValidators,
  MiniDisassembler, PointsTo, Runtime (`ctest -N` lists 5).

Commands:
```sh
# debug
cmake --preset debug && cmake --build --preset debug
( cd build/debug && ctest --output-on-failure -R InstanceStore )
( cd build/debug && ctest -N | tail -1 )   # 'Total Tests: 5'
# release
cmake --preset release && cmake --build --preset release
( cd build/release && ctest --output-on-failure -R InstanceStore )
# asan (per the standing gate — Runtime IS touched, surface 2 mandatory)
# [ADJ:crit-pins-1] -DDRLOJEKYLL_ENABLE_TESTS=ON is MANDATORY: it defaults OFF
# (cmake/options.cmake:5) and this hand-rolled `-B build/asan` inherits no
# preset — without it tests/ is skipped, instance_store_test is never built,
# and `ctest -R InstanceStore` finds zero tests (surface-1 runs vacuously).
cmake -B build/asan -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DDRLOJEKYLL_ENABLE_TESTS=ON \
  -DDRLOJEKYLL_ENABLE_SANITIZERS=ON
cmake --build build/asan
( cd build/asan && ctest --output-on-failure -R InstanceStore )
```

### Gates checklist
- **G1 (build):** all three presets build clean (`-Werror` via
  `settings_private`); the two `Reset` members and InstanceStore.h compile
  with no warning.
- **G2 (suite):** `SUITE: PASS` at 169 unchanged (P-D2a.1); the four
  optimization modes stay orthogonal.
- **G4 (ctest):** 4→5, all green (P-D2a.2). New target registered in
  `tests/CMakeLists.txt` + `tests/InstanceStore/CMakeLists.txt`.
- **G7 (data/ clean):** no stray artifacts; scratch build dirs
  (`build/benchcount`, `build/asan`, `/tmp/d2a-*`) are out-of-tree.
- **HP-16 gate:** §5 — ON-build compiles + SUITE PASS.
- **ASAN (both surfaces, standing gate):** surface 1 = the compiler/ctest
  tree under `build/asan` (incl. `instance_store_test`); **surface 2
  MANDATORY** because Runtime is touched — run the OptDiff suite with
  `DR=build/asan/bin/drlojekyll` and drivers/Runtime compiled with ASAN
  (`CXX='clang++ -fsanitize=address -fno-omit-frame-pointer'`), confirming no
  new report. Never time Q5/bench against the ASAN build.
- **No E-62 (DeltaRel dump):** D2.a touches NO DeltaRel/Format/`.deltarel`
  surface — the census/dump golden lane is not engaged (that begins at D1.b's
  spelling rows, already landed, and D2.b). No `.deltarel` bless at D2.a.

### Residuals / carried notes for the implementer
- **N-1 (d1-pinned §CARRIED):** the dropped `working_count` — record in the
  InstanceStore.h header comment (done, §2.1) that `NumRows()`-as-occupancy is
  exact only under R-MONO; revisit a signed count when R-DIFF lands (D3.a),
  where a mid-batch dip below zero could make it a lie.
- **HP-7 belt staging:** the `monotone` ctor gate ships true (R-MONO); the
  D3.a R-DIFF lowering constructs `monotone=false`. The belt-fires-on-drop
  negative is deferred to the D3.a landing gate (HP-8) — an in-proc `assert`
  abort is untestable in DrTest; the death-half test (§4.5) instead proves the
  store SUPPORTS a drop with the belt off.
- **Ledger:** KeyedInstances.md §19(K)/(L) records this landing; the N-1 note
  reaches the InstanceStore.h header comment per the carried directive.

======================================================================
## 7. ADJUDICATION LEDGER (XHIGH adjudicator, tip 932ad4a6)
======================================================================

Every critic finding (2 reports, 5 findings) verified against the real headers
at tip 932ad4a6 and ruled. crit-runtime-1 and crit-pins-2 are the SAME defect
reported by both critics.

| id | sev | verdict | disposition |
|---|---|---|---|
| crit-runtime-1 | LOW | **ACCEPT** | §2.6: removed the non-compiling `std::swap(frozen[iid], current[iid])` shorthand; the `.Set` triple is now the sole/primary spelling. [ADJ:crit-runtime-1] |
| crit-pins-2 | LOW | **ACCEPT (dup of crit-runtime-1)** | Same Seal defect. Folded by the same §2.6 edit; tagged jointly [ADJ:crit-runtime-1 / crit-pins-2]. |
| crit-runtime-2 | LOW | **ACCEPT** | §2.7: corrected the counter-attribution wording — `gBenchCounters` is process-global (BenchCounters.h:62); store lookups and nested-Table probes share `finds`/`probe_steps`, not separable. No HP-16 impact. [ADJ:crit-runtime-2] |
| crit-pins-1 | MED | **ACCEPT** | §5 + §P-D2a.2: added `-DDRLOJEKYLL_ENABLE_TESTS=ON` to the hand-rolled `build/benchcount` and `build/asan` configure lines. Without it both gates run VACUOUSLY (tests/ guarded OFF by default, CMakeLists.txt:164 / options.cmake:5). [ADJ:crit-pins-1] |
| crit-pins-3 | LOW | **ACCEPT (clarify)** | §4.0: made the provenance precise — the dir pattern mirrors DeltaRelValidators, the CMake content mirrors Runtime; the omitted `drlojekyll_sanitizers` stanza is safe (settings_public carries ASAN globally, CMakeLists.txt:111-120). [ADJ:crit-pins-3] |

### Verified-CORRECT (spot audit, no change)
- **HP-16 counter audit (§2.7 table)**: all 7 store counter names —
  `finds`/`probe_steps`/`rehash_events`/`rehash_rows`/`touch_calls`/
  `touch_appends`/`commit_visits` — confirmed present in
  `HYDE_RT_BENCH_COUNTER_FIELDS` (BenchCounters.h:22-49). Zero new names. HP-16
  satisfied by construction.
- **Vec allocation-free reset chain (§3.3, H6)**: `Vec::Truncate` retains
  capacity (Vec.h:96-99); `Set` is the mutator, `operator[]` const-only
  (Vec.h:80-88). Reset makes zero allocator calls — Arena-safe. CONFIRMED.
- **RowStore/Table layering (§3.1/§3.2)**: `sealed` is a Table private, log
  members are RowStore private (Table.h) — the protected-Reset-on-RowStore +
  public-chain-on-Table split is forced and correct. CONFIRMED.
- **ASAN reachability of instance_store_test**: settings_public attaches the
  sanitizer interface globally (CMakeLists.txt:111-120); a Runtime-only target
  is instrumented with no per-target stanza (runtime_test precedent, D1.b).
  CONFIRMED.

### Owner decisions surfaced (NOT adjudicator calls — carried to the landing)
- **OWN-1 — test-target placement (designer flag, §4.0).** NEW
  `tests/InstanceStore/` (5th ctest target, ctest 4→5) honors the pinned
  prediction P-D2a.2 "ctest 4→5"; the alternative is folding
  `InstanceStoreTest.cpp` into `runtime_test` (where the DONOR StateCellTest.cpp
  lives, ctest stays 4, P-D2a.2 amends to 4→4). Design chose the new dir per the
  pin; owner may override. This is a pinned-prediction question, owner's call.
- **OWN-2 — HP-7 belt gating via a `monotone` ctor bool (designer flag,
  §2.2/§2.6).** Extends the store's public ctor surface one bool beyond a
  literal reading of A.3.1 (used only by the belt until D3.a). The alternative
  (codegen-only R-MONO assert) is heavier and can't drive the death-half unit.
  Reasonable; flagged for the owner since it grows the public API.
- **OWN-3 — belt-fires-on-monotone-drop negative deferred to D3.a/HP-8
  (designer flag, §4.5 / HP-7 staging).** An in-proc `assert` abort is
  untestable in DrTest; the death-half test proves the store SUPPORTS a drop
  with the belt off instead. Consistent with HP-17 staging; a fork/EXPECT_DEATH
  harness (DeltaRelValidators fork/waitpid precedent, RAT-3) would be needed to
  test the fire at D2.a. Owner: accept the defer, or require the harness now.
- **OWN-4 — N-1 discharge is a header comment only (§2.1/§6).** The
  `working_count` re-introduction is genuinely deferred to R-DIFF/D3.a per the
  carried directive. VERIFY the ledger KeyedInstances.md §19(K)/(L) is updated
  to point at the InstanceStore.h header note when D2.a lands.

======================================================================
## 8. FINAL VERDICT — GO-WITH-AMENDMENTS
======================================================================

**GO-WITH-AMENDMENTS.** The design is a faithful, well-grounded StateCellStore
transpose; the architecture (double-buffer + pointer-swap Seal, RowStore/Table
two-layer Reset, index-free nested tables, the H6 Arena discharge) is sound and
verified against tip 932ad4a6. All 5 critic findings are ACCEPTED and folded in
place (§2.6, §2.7, §5, §P-D2a.2, §4.0). One finding is MED-severity and
consequential: **crit-pins-1** — the missing `-DDRLOJEKYLL_ENABLE_TESTS=ON`
would have made the HP-16 bench-counters gate AND the ASAN surface-1 run pass
vacuously (no defect visible, no teeth). That single fix is what separates a
real gate from a green-but-empty one; it is now folded and MUST be present on
the implemented build lines. crit-runtime-1/pins-2 (the non-compiling
`std::swap`) would fail G1 `-Werror` only if transcribed verbatim — now removed
from the doc so the implementer copies the compiling form. The remaining three
are LOW/wording. No NEEDS-REWORK trigger: no correctness hole in the primitive,
no HP violation, no golden/E-62 exposure (D2.a is inert). The four owner
decisions (OWN-1..OWN-4) are pre-existing designer flags carried to the landing,
not adjudicator amendments. Clear to implement once the amendments are honored
and the owner rules OWN-1..OWN-4.
