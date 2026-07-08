// Copyright 2026, Trail of Bits. All rights reserved.

#pragma once

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "Hash.h"
#include "Vec.h"

namespace hyde::rt {

// Sentinel row id meaning "no row".
inline constexpr uint32_t kNoRow = ~0u;

// Derivation class of a counter fold on a differential relation. A row's
// presence is maintained as two signed derivation counters: `C_nr` counts
// rule instances whose deriving view lies outside the SCC of the row's
// table (plus one for explicit message support), and `C_r` counts instances
// arriving over an inductive back-edge of that SCC. The split is what makes
// rederivation after overdeletion a single counter read: a row with any
// surviving nonrecursive support never enters the overdeletion set, and a
// purely cyclically supported row survives iff `C_r > 0` once the
// overdelete fixpoint quiesces.
enum class DerivClass : uint8_t {
  kNonRecursive,
  kRecursive,
};

// Per-row flags of a differential relation. Storage is persistent (one byte
// per row) but the meaning of every bit except `kInI`/`kExplicit` is
// batch-transient scratch, reset by `Commit`.
enum RowFlags : uint8_t {
  kInI = 1u << 0,       // Present at batch start (the frozen "I").
  kDel = 1u << 1,       // Claimed by the overdeletion set D this batch.
  kAdd = 1u << 2,       // Claimed by the addition set A this batch.
  kDelNow = 1u << 3,    // Claimed by the CURRENT delete frontier round.
  kAddNow = 1u << 4,    // Claimed by the CURRENT insert frontier round.
  kExplicit = 1u << 5,  // Counted +1 in C_nr for explicit (message) support.
  kTouched = 1u << 6,   // Id already recorded in `touched` this batch.
};

// Append-only row storage shared by both table flavors: a row log (row ids
// are stable offsets) plus an open-addressing hash set for row-by-value
// lookup.
//
// `Row` is a generated aggregate of scalar columns providing
// `uint64_t Hash() const` and `operator==`.
template <typename Row>
class RowStore {
 public:
  explicit RowStore(Allocator allocator_)
      : allocator(allocator_),
        rows(allocator_),
        hashes(allocator_) {}

  ~RowStore(void) {
    if (slots) {
      allocator.FreeArray(slots, slot_capacity);
    }
  }

  RowStore(const RowStore &) = delete;
  RowStore &operator=(const RowStore &) = delete;

  // Number of rows ever added.
  uint32_t NumRows(void) const noexcept {
    return static_cast<uint32_t>(rows.Size());
  }

  const Row &RowAt(uint32_t id) const noexcept {
    return rows[id];
  }

  // Row id of a tuple by value, or `kNoRow`.
  uint32_t Find(const Row &row) const noexcept {
    return FindWithHash(row, row.Hash());
  }

 protected:
  uint32_t FindWithHash(const Row &row, uint64_t hash) const noexcept {
    if (!slot_capacity) {
      return kNoRow;
    }
    for (size_t i = hash & (slot_capacity - 1u);;
         i = (i + 1u) & (slot_capacity - 1u)) {
      const uint32_t id = slots[i];
      if (id == kNoRow) {
        return kNoRow;
      }
      if (hashes[id] == hash && rows[id] == row) {
        return id;
      }
    }
  }

  // Finds `row`, storing it as a fresh log entry if it was never seen.
  // Returns the row id and whether a fresh entry was created.
  std::pair<uint32_t, bool> FindOrAdd(const Row &row) {
    const uint64_t hash = row.Hash();
    if (const uint32_t id = FindWithHash(row, hash); id != kNoRow) {
      return {id, false};
    }
    const uint32_t id = NumRows();
    if (id == kNoRow) {
      std::fprintf(stderr, "hyde::rt: row-id space exhausted\n");
      std::abort();
    }
    rows.Add(row);
    hashes.Add(hash);
    InsertSlot(id, hash);
    return {id, true};
  }

 private:
  void InsertSlot(uint32_t id, uint64_t hash) {
    // Grow at 7/8 load. `Rehash` links every stored row, including the one
    // being inserted, so there is nothing left to do after it runs.
    const size_t num_rows = rows.Size();
    if ((num_rows + (num_rows >> 3u)) >= slot_capacity) {
      Rehash();
      return;
    }
    for (size_t i = hash & (slot_capacity - 1u);;
         i = (i + 1u) & (slot_capacity - 1u)) {
      if (slots[i] == kNoRow) {
        slots[i] = id;
        return;
      }
    }
  }

  void Rehash(void) {
    const size_t new_capacity = slot_capacity ? slot_capacity * 2u : 64u;
    auto new_slots = allocator.AllocateArray<uint32_t>(new_capacity);
    for (size_t i = 0u; i < new_capacity; ++i) {
      new_slots[i] = kNoRow;
    }
    if (slots) {
      allocator.FreeArray(slots, slot_capacity);
    }
    slots = new_slots;
    slot_capacity = new_capacity;
    for (uint32_t id = 0u; id < NumRows(); ++id) {
      const uint64_t hash = hashes[id];
      for (size_t i = hash & (slot_capacity - 1u);;
           i = (i + 1u) & (slot_capacity - 1u)) {
        if (slots[i] == kNoRow) {
          slots[i] = id;
          break;
        }
      }
    }
  }

  Allocator allocator;
  Vec<Row> rows;
  Vec<uint64_t> hashes;
  uint32_t *slots{nullptr};
  size_t slot_capacity{0u};
};

// Monotone relation: presence is "row exists in the log". Insert-only; a
// stored row is present forever, so there is no per-row state of any kind.
template <typename Row>
class Table : public RowStore<Row> {
 public:
  struct Insert {
    bool added;   // The tuple is new; append its id to every index.
    uint32_t id;  // Row id, valid regardless of `added`.
  };

  using RowStore<Row>::RowStore;

  // Dedup by value; the only mutator.
  Insert TryAdd(const Row &row) {
    const auto [id, added] = this->FindOrAdd(row);
    return {added, id};
  }

  // Always true for stored rows.
  bool Present(uint32_t id) const noexcept {
    assert(id < this->NumRows());
    (void) id;
    return true;
  }
};

// Differential relation: split signed derivation counters (`C_nr`, `C_r`)
// plus per-batch scratch flags. One received message batch is one epoch:
// counter folds (`AddDerivation`/`SubDerivation`, `Add`/`SubExplicit`)
// accumulate signed deltas whose zero crossings drive the per-stratum
// OVERDELETE -> REDERIVE -> INSERT fixpoints, and `Commit` seals the epoch,
// publishing net 0/1 presence changes against the batch-start snapshot
// (`kInI`) and resetting the scratch flags.
//
// Counters are SIGNED and may transiently dip below zero mid-batch: a
// canceling "phantom pair" (an instance whose earlier-position atom is
// added and later-position atom is deleted in the same batch) can apply its
// minus before its plus. Non-negativity is asserted per class at `Commit`,
// never per fold. Crossing predicates read only the fold's own
// before/after counter snapshot plus the batch-frozen `kInI` bit — never
// `kDel`/`kAdd`, which mutate concurrently within a phase.
template <typename Row>
class DiffTable : public RowStore<Row> {
 public:
  struct Delta {
    bool crossed;    // This fold is a zero-crossing event -> enqueue.
    bool added_row;  // Fresh log entry -> append id to every index.
    uint32_t id;
  };

  explicit DiffTable(Allocator allocator_)
      : RowStore<Row>(allocator_),
        counts(allocator_),
        flags(allocator_),
        touched(allocator_) {}

  // ++count(c); crossed <=> before_total <= 0 && after_total > 0.
  Delta AddDerivation(const Row &row, DerivClass c) {
    return Fold(row, c, +1);
  }

  // --count(c); crossed <=> kInI && after_C_nr <= 0. Every decrement of an
  // in-I row whose post-state C_nr is non-positive fires (not only a sign
  // crossing); duplicate enqueues are absorbed by `TryClaimDel`.
  Delta SubDerivation(const Row &row, DerivClass c) {
    return Fold(row, c, -1);
  }

  // kExplicit 0->1: ++C_nr, same crossing rule as AddDerivation; else no-op.
  Delta AddExplicit(const Row &row) {
    const auto [id, added_row] = this->FindOrAdd(row);
    GrowTo(id);
    if (flags[id] & kExplicit) {
      return {false, added_row, id};
    }
    Touch(id);
    flags.Set(id, static_cast<uint8_t>(flags[id] | kExplicit));
    Delta d = FoldAt(id, DerivClass::kNonRecursive, +1);
    d.added_row = added_row;
    return d;
  }

  // kExplicit 1->0: --C_nr, same crossing rule as SubDerivation; else no-op.
  Delta SubExplicit(const Row &row) {
    const uint32_t id = this->Find(row);
    if (id == kNoRow || !(flags[id] & kExplicit)) {
      return {false, false, id};
    }
    Touch(id);
    flags.Set(id, static_cast<uint8_t>(flags[id] & ~kExplicit));
    return FoldAt(id, DerivClass::kNonRecursive, -1);
  }

  // Named membership predicates (single flag-byte or counter-word reads).
  // These are the ONLY forms in which generated joins may read a
  // differential table; each CHECKMEMBER region names one explicitly.

  // Batch-start state (the frozen "I").
  bool InI(uint32_t id) const noexcept {
    return 0 != (flags[id] & kInI);
  }

  // Final-so-far: (kInI && !kDel) || kAdd.
  bool InNew(uint32_t id) const noexcept {
    const uint8_t f = flags[id];
    return ((f & kInI) && !(f & kDel)) || (f & kAdd);
  }

  // Fixpoint-round reads; "frontier" = the claim round now draining.
  bool SurvivesSoFar(uint32_t id) const noexcept {
    const uint8_t f = flags[id];
    return (f & kInI) && !(f & kDel);
  }

  bool AliveAtClaim(uint32_t id) const noexcept {
    const uint8_t f = flags[id];
    return (f & kInI) && (!(f & kDel) || (f & kDelNow));
  }

  bool InNewWithFrontier(uint32_t id) const noexcept {
    return InNew(id);
  }

  bool InNewSansFrontier(uint32_t id) const noexcept {
    const uint8_t f = flags[id];
    return ((f & kInI) && !(f & kDel)) || ((f & kAdd) && !(f & kAddNow));
  }

  // Post-commit presence: counts > 0.
  bool Present(uint32_t id) const noexcept {
    return Total(counts[id]) > 0;
  }

  // C_r > 0: the REDERIVE survival test after OVERDELETE quiesces.
  bool RecursivelySupported(uint32_t id) const noexcept {
    return CountR(counts[id]) > 0;
  }

  // Claim `id` into the overdeletion set D and the current delete frontier
  // round. Returns false if already claimed this batch (dequeue dedup).
  bool TryClaimDel(uint32_t id) {
    const uint8_t f = flags[id];
    if (f & kDel) {
      return false;
    }
    assert(CountNr(counts[id]) <= 0);
    Touch(id);
    flags.Set(id, static_cast<uint8_t>(flags[id] | kDel | kDelNow));
    return true;
  }

  // Claim `id` into the addition set A and the current insert frontier round.
  bool TryClaimAdd(uint32_t id) {
    const uint8_t f = flags[id];
    if (f & kAdd) {
      return false;
    }
    Touch(id);
    flags.Set(id, static_cast<uint8_t>(flags[id] | kAdd | kAddNow));
    return true;
  }

  // Clear the kDelNow bits of a drained delete frontier round.
  void RetireDelFrontier(const Vec<uint32_t> &round) {
    for (uint32_t id : round) {
      flags.Set(id, static_cast<uint8_t>(flags[id] & ~kDelNow));
    }
  }

  // Clear the kAddNow bits of a drained insert frontier round.
  void RetireAddFrontier(const Vec<uint32_t> &round) {
    for (uint32_t id : round) {
      flags.Set(id, static_cast<uint8_t>(flags[id] & ~kAddNow));
    }
  }

  // End-of-batch: for each touched row, report 0/1 presence crossings vs
  // the batch-start snapshot to `sink(row, added)`, set kInI := (counts >
  // 0), and clear every batch-scratch flag; counters are asserted
  // non-negative per class. Then forget the touched set.
  template <typename Sink>
  void Commit(Sink &&sink) {
    for (uint32_t id : touched) {
      const uint64_t w = counts[id];
      const int32_t nr = CountNr(w);
      const int32_t r = CountR(w);
      assert(0 <= nr);
      assert(0 <= r);
      uint8_t f = flags[id];
      const bool was = 0 != (f & kInI);
      const bool now = (static_cast<int64_t>(nr) + static_cast<int64_t>(r)) > 0;
      if (was != now) {
        sink(this->RowAt(id), now);
      }
      f &= static_cast<uint8_t>(~(kDel | kAdd | kDelNow | kAddNow | kTouched));
      if (now) {
        f |= kInI;
      } else {
        f &= static_cast<uint8_t>(~kInI);
      }
      flags.Set(id, f);
    }
    touched.Clear();
  }

  // Debug-build coherence validation, called by the generated commit sweep
  // after `Commit`: counters non-negative per class, no batch-scratch flag
  // survives, and the sealed `kInI` snapshot agrees with count-based
  // presence for every row.
  void DebugValidateCounts(void) const {
#ifndef NDEBUG
    assert(touched.Empty());
    for (uint32_t id = 0u; id < this->NumRows(); ++id) {
      const uint64_t w = counts[id];
      assert(0 <= CountNr(w));
      assert(0 <= CountR(w));
      const uint8_t f = flags[id];
      assert(0 == (f & (kDel | kAdd | kDelNow | kAddNow | kTouched)));
      assert((0 != (f & kInI)) == (Total(w) > 0));
      if (f & kExplicit) {
        assert(1 <= CountNr(w));
      }
    }
#endif
  }

 private:
  // counts[id]: two SIGNED int32 counters packed in one 64-bit word (C_nr
  // low, C_r high), so one read-modify-write yields a consistent
  // (before, after) snapshot of both.
  static int32_t CountNr(uint64_t w) noexcept {
    return static_cast<int32_t>(static_cast<uint32_t>(w));
  }

  static int32_t CountR(uint64_t w) noexcept {
    return static_cast<int32_t>(static_cast<uint32_t>(w >> 32u));
  }

  static uint64_t Pack(int32_t nr, int32_t r) noexcept {
    return (static_cast<uint64_t>(static_cast<uint32_t>(r)) << 32u) |
           static_cast<uint64_t>(static_cast<uint32_t>(nr));
  }

  static int64_t Total(uint64_t w) noexcept {
    return static_cast<int64_t>(CountNr(w)) + static_cast<int64_t>(CountR(w));
  }

  // Extend the per-row side arrays to cover `id` (a fresh log entry).
  void GrowTo(uint32_t id) {
    while (counts.Size() <= id) {
      counts.Add(0u);
      flags.Add(0u);
    }
  }

  // Record `id` in the touched set exactly once per batch.
  void Touch(uint32_t id) {
    if (!(flags[id] & kTouched)) {
      flags.Set(id, static_cast<uint8_t>(flags[id] | kTouched));
      touched.Add(id);
    }
  }

  Delta Fold(const Row &row, DerivClass c, int32_t delta) {
    const auto [id, added_row] = this->FindOrAdd(row);
    GrowTo(id);
    Delta d = FoldAt(id, c, delta);
    d.added_row = added_row;
    return d;
  }

  // The counter read-modify-write: one signed +/-1 on one class, with the
  // crossing decision taken from this fold's own before/after snapshot.
  Delta FoldAt(uint32_t id, DerivClass c, int32_t delta) {
    Touch(id);
    const uint64_t before = counts[id];
    int32_t nr = CountNr(before);
    int32_t r = CountR(before);
    int32_t &target = (c == DerivClass::kNonRecursive) ? nr : r;
    target += delta;
    if (target == INT32_MAX || target == INT32_MIN) {
      std::fprintf(
          stderr,
          "hyde::rt::DiffTable: derivation-counter magnitude overflow\n");
      std::abort();
    }
    const uint64_t after = Pack(nr, r);
    counts.Set(id, after);
    bool crossed;
    if (0 < delta) {
      crossed = Total(before) <= 0 && Total(after) > 0;
    } else {
      crossed = (0 != (flags[id] & kInI)) && nr <= 0;
    }
    return {crossed, false, id};
  }

  Vec<uint64_t> counts;   // 8 B/row, persistent (two packed int32).
  Vec<uint8_t> flags;     // 1 B/row storage; batch-transient meaning.
  Vec<uint32_t> touched;  // Ids whose counts/flags changed this batch.
};

// A secondary index over a table: maps a key (a generated aggregate of the
// index's key columns, with `Hash()` and `operator==`) to the chain of row
// ids that share it. Chains are threaded through a per-row `next` array, so
// the index performs no per-key allocation.
//
// The index is append-only: generated code links each fresh row id (a
// `TryAdd` whose `added` is true, or a counter fold whose `added_row` is
// true) exactly once, in row-id order, and readers filter liveness through
// the owning table's membership predicates.
template <typename Key>
class Index {
 public:
  explicit Index(Allocator allocator_)
      : allocator(allocator_),
        next(allocator_) {}

  ~Index(void) {
    if (slots) {
      allocator.FreeArray(slots, slot_capacity);
    }
  }

  Index(const Index &) = delete;
  Index &operator=(const Index &) = delete;

  // Links `id` under `key`. Row ids must be added in increasing order.
  void Add(const Key &key, uint32_t id) {
    while (next.Size() <= id) {
      next.Add(kNoRow);
    }
    Slot &slot = FindOrCreateSlot(key);
    next.Set(id, slot.head);
    slot.head = id;
  }

  // First row id for `key`, or `kNoRow`. Iterate with `Next`.
  uint32_t First(const Key &key) const noexcept {
    if (!slot_capacity) {
      return kNoRow;
    }
    const uint64_t hash = key.Hash();
    for (size_t i = hash & (slot_capacity - 1u);; i = (i + 1u) & (slot_capacity - 1u)) {
      const Slot &slot = slots[i];
      if (slot.head == kNoRow && !slot.used) {
        return kNoRow;
      }
      if (slot.used && slot.hash == hash && slot.key == key) {
        return slot.head;
      }
    }
  }

  uint32_t Next(uint32_t id) const noexcept {
    return next[id];
  }

 private:
  struct Slot {
    Key key;
    uint64_t hash;
    uint32_t head;
    bool used;
  };

  Slot &FindOrCreateSlot(const Key &key) {
    if ((num_keys + (num_keys >> 3u)) >= slot_capacity) {
      Rehash();
    }
    const uint64_t hash = key.Hash();
    for (size_t i = hash & (slot_capacity - 1u);; i = (i + 1u) & (slot_capacity - 1u)) {
      Slot &slot = slots[i];
      if (!slot.used) {
        slot.key = key;
        slot.hash = hash;
        slot.head = kNoRow;
        slot.used = true;
        ++num_keys;
        return slot;
      }
      if (slot.hash == hash && slot.key == key) {
        return slot;
      }
    }
  }

  void Rehash(void) {
    const size_t new_capacity = slot_capacity ? slot_capacity * 2u : 64u;
    auto new_slots = allocator.AllocateArray<Slot>(new_capacity);
    for (size_t i = 0u; i < new_capacity; ++i) {
      new_slots[i].used = false;
      new_slots[i].head = kNoRow;
    }
    const auto old_slots = slots;
    const auto old_capacity = slot_capacity;
    slots = new_slots;
    slot_capacity = new_capacity;
    if (old_slots) {
      for (size_t i = 0u; i < old_capacity; ++i) {
        if (old_slots[i].used) {
          for (size_t j = old_slots[i].hash & (slot_capacity - 1u);;
               j = (j + 1u) & (slot_capacity - 1u)) {
            if (!slots[j].used) {
              slots[j] = old_slots[i];
              break;
            }
          }
        }
      }
      allocator.FreeArray(old_slots, old_capacity);
    }
  }

  Allocator allocator;
  Vec<uint32_t> next;
  Slot *slots{nullptr};
  size_t slot_capacity{0u};
  size_t num_keys{0u};
};

}  // namespace hyde::rt
