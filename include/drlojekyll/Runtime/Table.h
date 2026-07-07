// Copyright 2026, Trail of Bits. All rights reserved.

#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "Hash.h"
#include "Vec.h"

namespace hyde::rt {

// Datalog tuple state. A row, once added, is never removed from storage; its
// state tracks whether the tuple is logically in the relation.
enum class TupleState : uint8_t {
  kAbsent,
  kPresent,
  kUnknown,
};

// Sentinel row id meaning "no row".
inline constexpr uint32_t kNoRow = ~0u;

// Storage for one relation: an append-only row log (row ids are stable
// offsets) plus an open-addressing hash set for row-by-value lookup.
//
// `Row` is a generated aggregate of scalar columns providing
// `uint64_t Hash() const` and `operator==`.
//
// Secondary indexes are separate `Index` objects owned by the generated
// database; generated code updates them explicitly whenever an insert
// reports `added`.
template <typename Row>
class Table {
 public:
  struct Insert {
    bool changed;    // The tuple just became present.
    bool added_row;  // A new row was stored; add its id to every index.
    uint32_t id;     // Row id, valid regardless of `changed`.
  };

  explicit Table(Allocator allocator_)
      : allocator(allocator_),
        rows(allocator_),
        hashes(allocator_),
        states(allocator_) {}

  ~Table(void) {
    if (slots) {
      allocator.FreeArray(slots, slot_capacity);
    }
  }

  Table(const Table &) = delete;
  Table &operator=(const Table &) = delete;

  // Number of rows ever added (present, unknown, and retracted alike).
  uint32_t NumRows(void) const noexcept {
    return static_cast<uint32_t>(rows.Size());
  }

  const Row &RowAt(uint32_t id) const noexcept {
    return rows[id];
  }

  TupleState StateAt(uint32_t id) const noexcept {
    return static_cast<TupleState>(states[id]);
  }

  // State of a tuple by value; absent if never added.
  TupleState State(const Row &row) const noexcept {
    const uint32_t id = Find(row);
    return id == kNoRow ? TupleState::kAbsent : StateAt(id);
  }

  // Row id of a tuple by value, or `kNoRow`.
  uint32_t Find(const Row &row) const noexcept {
    if (!slot_capacity) {
      return kNoRow;
    }
    const uint64_t hash = row.Hash();
    for (size_t i = hash & (slot_capacity - 1u);; i = (i + 1u) & (slot_capacity - 1u)) {
      const uint32_t id = slots[i];
      if (id == kNoRow) {
        return kNoRow;
      }
      if (hashes[id] == hash && rows[id] == row) {
        return id;
      }
    }
  }

  // Makes `row` present if it is absent (or was never added). `changed` is
  // true when the tuple newly became present; generated code then appends
  // the row id to every index on this table.
  Insert TryChangeAbsentToPresent(const Row &row) {
    return TryPromote(row, /* allow_unknown= */ false);
  }

  // Makes `row` present if it is absent or unknown.
  Insert TryChangeAbsentOrUnknownToPresent(const Row &row) {
    return TryPromote(row, /* allow_unknown= */ true);
  }

  // Downgrades a present tuple to unknown (differential update start).
  bool TryChangePresentToUnknown(const Row &row) noexcept {
    return TryTransition(row, TupleState::kPresent, TupleState::kUnknown);
  }

  // Retracts an unknown tuple.
  bool TryChangeUnknownToAbsent(const Row &row) noexcept {
    return TryTransition(row, TupleState::kUnknown, TupleState::kAbsent);
  }

  // Retracts a present tuple.
  bool TryChangePresentToAbsent(const Row &row) noexcept {
    return TryTransition(row, TupleState::kPresent, TupleState::kAbsent);
  }

 private:
  Insert TryPromote(const Row &row, bool allow_unknown) {
    const uint64_t hash = row.Hash();
    if (uint32_t id = FindWithHash(row, hash); id != kNoRow) {
      const auto state = StateAt(id);
      if (state == TupleState::kAbsent ||
          (allow_unknown && state == TupleState::kUnknown)) {
        states.Set(id, static_cast<uint8_t>(TupleState::kPresent));
        return {true, false, id};
      }
      return {false, false, id};
    }
    const uint32_t id = NumRows();
    if (id == kNoRow) {
      std::fprintf(stderr, "hyde::rt::Table: row-id space exhausted\n");
      std::abort();
    }
    rows.Add(row);
    hashes.Add(hash);
    states.Add(static_cast<uint8_t>(TupleState::kPresent));
    InsertSlot(id, hash);
    return {true, true, id};
  }

  bool TryTransition(const Row &row, TupleState from, TupleState to) noexcept {
    const uint32_t id = Find(row);
    if (id != kNoRow && StateAt(id) == from) {
      states.Set(id, static_cast<uint8_t>(to));
      return true;
    }
    return false;
  }

  uint32_t FindWithHash(const Row &row, uint64_t hash) const noexcept {
    if (!slot_capacity) {
      return kNoRow;
    }
    for (size_t i = hash & (slot_capacity - 1u);; i = (i + 1u) & (slot_capacity - 1u)) {
      const uint32_t id = slots[i];
      if (id == kNoRow) {
        return kNoRow;
      }
      if (hashes[id] == hash && rows[id] == row) {
        return id;
      }
    }
  }

  void InsertSlot(uint32_t id, uint64_t hash) {
    // Grow at 7/8 load. `Rehash` links every stored row, including the one
    // being inserted, so there is nothing left to do after it runs.
    const size_t num_rows = rows.Size();
    if ((num_rows + (num_rows >> 3u)) >= slot_capacity) {
      Rehash();
      return;
    }
    for (size_t i = hash & (slot_capacity - 1u);; i = (i + 1u) & (slot_capacity - 1u)) {
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
      for (size_t i = hash & (slot_capacity - 1u);; i = (i + 1u) & (slot_capacity - 1u)) {
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
  Vec<uint8_t> states;
  uint32_t *slots{nullptr};
  size_t slot_capacity{0u};
};

// A secondary index over a table: maps a key (a generated aggregate of the
// index's key columns, with `Hash()` and `operator==`) to the chain of row
// ids that share it. Chains are threaded through a per-row `next` array, so
// the index performs no per-key allocation.
//
// Generated code appends to the index on every insert whose `changed` is
// true, in row-id order:
//
//     if (auto [changed, id] = table.TryChangeAbsentToPresent(row); changed) {
//       index.Add({row.key_col}, id);
//     }
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
