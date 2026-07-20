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

// Keyed (α bound columns) -> a dense instance id (iid); a standing double-
// buffered nested `Table<RowT>` per iid (spec §A.3.1). The two type params
// match the donor's <Key, Algebra> arity, but the "algebra" slot is fixed: a
// monotone Table<RowT>.
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

  // Number of dense instance ids ever allocated (append-only namespace).
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

 private:
  uint32_t FindInstanceWithHash(const Key &key, uint64_t hash) const noexcept {
    HYDE_RT_BENCH_COUNT(finds);
    if (!slot_capacity) {
      return kNoInstance;
    }
    for (size_t i = hash & (slot_capacity - 1u);;
         i = (i + 1u) & (slot_capacity - 1u)) {
      HYDE_RT_BENCH_COUNT(probe_steps);
      const uint32_t iid = slots[i];
      if (iid == kNoInstance) {
        return kNoInstance;
      }
      if (hashes[iid] == hash && keys[iid] == key) {
        return iid;
      }
    }
  }

  void InsertSlot(uint32_t iid, uint64_t hash) {
    const size_t num_instances = keys.Size();
    if ((num_instances + (num_instances >> 3u)) >= slot_capacity) {
      Rehash();
      return;
    }
    for (size_t i = hash & (slot_capacity - 1u);;
         i = (i + 1u) & (slot_capacity - 1u)) {
      if (slots[i] == kNoInstance) {
        slots[i] = iid;
        return;
      }
    }
  }

  void Rehash(void) {
    HYDE_RT_BENCH_COUNT(rehash_events);
    HYDE_RT_BENCH_COUNT_N(rehash_rows, NumInstances());
    const size_t new_capacity = slot_capacity ? slot_capacity * 2u : 64u;
    auto new_slots = allocator.AllocateArray<uint32_t>(new_capacity);
    for (size_t i = 0u; i < new_capacity; ++i) {
      new_slots[i] = kNoInstance;
    }
    if (slots) {
      allocator.FreeArray(slots, slot_capacity);
    }
    slots = new_slots;
    slot_capacity = new_capacity;
    for (uint32_t iid = 0u, n = NumInstances(); iid < n; ++iid) {
      const uint64_t hash = hashes[iid];
      for (size_t i = hash & (slot_capacity - 1u);;
           i = (i + 1u) & (slot_capacity - 1u)) {
        if (slots[i] == kNoInstance) {
          slots[i] = iid;
          break;
        }
      }
    }
  }

  // Record `iid` in the touched set exactly once per epoch. Mirror of
  // StateCell Touch (:560-567): a per-instance dedup byte, not a scan.
  void Touch(uint32_t iid) {
    HYDE_RT_BENCH_COUNT(touch_calls);
    if (!touched_flag[iid]) {
      HYDE_RT_BENCH_COUNT(touch_appends);
      touched_flag.Set(iid, 1u);
      touched.Add(iid);
    }
  }

  // The MakeVec mold (StateCell.h:605-611) specialized to Table: Table is not
  // trivially copyable (it owns Vecs), so the per-iid buffers live behind
  // pointers in Vec<Table*> and are heap-minted through the store's allocator.
  Table *MakeTable(void) {
    void *mem = allocator.Allocate(sizeof(Table), alignof(Table));
    return new (mem) Table(allocator);
  }

  void DestroyTable(Table *p) noexcept {
    p->~Table();
    allocator.Free(p, sizeof(Table), alignof(Table));
  }

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

}  // namespace hyde::rt
