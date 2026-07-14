// Copyright 2026, Trail of Bits. All rights reserved.

#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

#include "Allocator.h"
#include "BenchCounters.h"

namespace hyde::rt {

template <typename T>
class Vec;

template <typename T>
void NetBatch(Vec<T> &adds, Vec<T> &removes);

// A flat growable array with an explicit allocator. `T` must be trivially
// copyable and trivially destructible — generated row structs are plain
// aggregates of scalar columns, and the runtime enforces that.
template <typename T>
class Vec {
  static_assert(std::is_trivially_copyable_v<T>);
  static_assert(std::is_trivially_destructible_v<T>);

 public:
  explicit Vec(Allocator allocator_) : allocator(allocator_) {}

  Vec(Vec &&that) noexcept
      : allocator(that.allocator),
        items(that.items),
        count(that.count),
        capacity(that.capacity) {
    that.items = nullptr;
    that.count = 0u;
    that.capacity = 0u;
  }

  Vec &operator=(Vec &&that) noexcept {
    if (this != &that) {
      Release();
      allocator = that.allocator;
      items = that.items;
      count = that.count;
      capacity = that.capacity;
      that.items = nullptr;
      that.count = 0u;
      that.capacity = 0u;
    }
    return *this;
  }

  Vec(const Vec &) = delete;
  Vec &operator=(const Vec &) = delete;

  ~Vec(void) {
    Release();
  }

  void Add(T val) {
    if (count == capacity) {
      Grow();
    }
    items[count++] = val;
  }

  size_t Size(void) const noexcept {
    return count;
  }

  bool Empty(void) const noexcept {
    return !count;
  }

  const T &operator[](size_t i) const noexcept {
    assert(i < count);
    return items[i];
  }

  void Set(size_t i, T val) noexcept {
    assert(i < count);
    items[i] = val;
  }

  void Clear(void) noexcept {
    count = 0u;
  }

  void Swap(Vec &that) noexcept {
    std::swap(allocator, that.allocator);
    std::swap(items, that.items);
    std::swap(count, that.count);
    std::swap(capacity, that.capacity);
  }

  // Sorts and deduplicates. Rows are compared bytewise via a caller-supplied
  // ordering, or `operator<`/`operator==` when `T` provides them.
  //
  // The <= 1 early return is the exact identity boundary (a 0/1-element
  // range is already sorted and unique; std::unique's return makes the
  // count re-store idempotent), and it matters: deep inductive cascades
  // sort mostly-empty round queues (~0.3 elements per call measured), and
  // std::sort is not inlined at this call site, so each elided call drops
  // a full out-of-line sort frame. The counters stay ahead of the guard —
  // they are call-site volume, not sorts executed.
  void SortAndUnique(void) {
    HYDE_RT_BENCH_COUNT(sort_calls);
    HYDE_RT_BENCH_COUNT_N(sort_elems, count);
    if (count <= 1u) {
      return;
    }
    std::sort(items, items + count);
    count = static_cast<size_t>(
        std::unique(items, items + count) - items);
  }

  const T *begin(void) const noexcept {
    return items;
  }

  const T *end(void) const noexcept {
    return items + count;
  }

 private:
  template <typename U>
  friend void NetBatch(Vec<U> &adds, Vec<U> &removes);

  void Grow(void) {
    const size_t new_capacity = capacity ? capacity * 2u : 16u;
    T *new_items = allocator.AllocateArray<T>(new_capacity);
    if (count) {
      std::copy(items, items + count, new_items);
    }
    if (items) {
      allocator.FreeArray(items, capacity);
    }
    items = new_items;
    capacity = new_capacity;
  }

  void Release(void) {
    if (items) {
      allocator.FreeArray(items, capacity);
      items = nullptr;
      count = 0u;
      capacity = 0u;
    }
  }

  Allocator allocator;
  T *items{nullptr};
  size_t count{0u};
  size_t capacity{0u};
};

// Nets a differential message's explicit adds against its explicit removes
// within one batch. Each distinct row value's net count is (#occurrences in
// `adds`) − (#occurrences in `removes`); afterwards `adds` holds exactly one
// copy of each row with a positive net and `removes` exactly one copy of
// each row with a negative net. Rows appear in first-appearance order
// (`adds` scanned first, then `removes`).
template <typename T>
void NetBatch(Vec<T> &adds, Vec<T> &removes) {
  HYDE_RT_BENCH_COUNT(netbatch_calls);
  Vec<T> distinct(adds.allocator);
  Vec<int64_t> flags(adds.allocator);  // Bit 0: in `adds`; bit 1: in `removes`.

  const auto fold = [&](const T &row, int64_t flag) {
    for (size_t i = 0u; i < distinct.Size(); ++i) {
      HYDE_RT_BENCH_COUNT(netbatch_compares);
      if (distinct[i] == row) {
        flags.Set(i, flags[i] | flag);
        return;
      }
    }
    distinct.Add(row);
    flags.Add(flag);
  };

  for (const T &row : adds) {
    fold(row, 1);
  }
  for (const T &row : removes) {
    fold(row, 2);
  }

  // Per-batch explicit-op contract (MD §5.0/§5.5, Open Question 3 —
  // decided): SET semantics with annihilation. Each vector is deduplicated
  // (assert/retract are idempotent, so multiplicity within a batch is
  // meaningless), and a row in BOTH vectors — the adds∩removes
  // intersection — is dropped from both: the unordered pair is DEFINED as
  // annihilating, leaving the row's presence unchanged. This forecloses the
  // arithmetic-netting hazard where duplicated retractions outvote an add
  // within one batch ({+x, −x, −x} is a no-op, not a removal).
  adds.Clear();
  removes.Clear();
  for (size_t i = 0u; i < distinct.Size(); ++i) {
    if (flags[i] == 1) {
      adds.Add(distinct[i]);
    } else if (flags[i] == 2) {
      removes.Add(distinct[i]);
    }
    // flags[i] == 3: annihilated (present in both vectors).
  }
}

}  // namespace hyde::rt
