// Copyright 2026, Trail of Bits. All rights reserved.
//
// Unit tests for the runtime: allocators, vectors, tables, and indexes.

#include <DrTest.h>

#include <cstdint>
#include <set>
#include <utility>

#include <drlojekyll/Runtime/Allocator.h>
#include <drlojekyll/Runtime/Hash.h>
#include <drlojekyll/Runtime/Table.h>
#include <drlojekyll/Runtime/Vec.h>

namespace {

// A two-column row, as codegen would emit it.
struct PairRow {
  uint32_t x;
  uint32_t y;

  uint64_t Hash(void) const noexcept {
    return hyde::rt::HashRow(x, y);
  }

  bool operator==(const PairRow &) const noexcept = default;
  auto operator<=>(const PairRow &) const noexcept = default;
};

// A one-column index key, as codegen would emit it.
struct KeyX {
  uint32_t x;

  uint64_t Hash(void) const noexcept {
    return hyde::rt::HashRow(x);
  }

  bool operator==(const KeyX &) const noexcept = default;
};

}  // namespace

TEST(Allocator, MallocRoundTrip) {
  auto a = hyde::rt::MallocAllocator();
  auto p = a.AllocateArray<uint64_t>(100);
  ASSERT_NE(p, nullptr);
  p[0] = 1u;
  p[99] = 2u;
  ASSERT_EQ(p[0] + p[99], 3u);
  a.FreeArray(p, 100);
}

TEST(Allocator, ArenaBumpsAndReuses) {
  hyde::rt::Arena arena(hyde::rt::MallocAllocator());
  auto a = hyde::rt::ArenaAllocator(arena);

  // Cross a chunk boundary.
  for (int i = 0; i < 100; ++i) {
    auto p = a.AllocateArray<uint64_t>(1024);
    ASSERT_NE(p, nullptr);
    p[0] = static_cast<uint64_t>(i);
    p[1023] = static_cast<uint64_t>(i);
  }

  arena.Reset();
  auto p = a.AllocateArray<uint32_t>(8);
  ASSERT_NE(p, nullptr);
}

TEST(Vec, AddIterateSortUnique) {
  hyde::rt::Vec<uint32_t> v(hyde::rt::MallocAllocator());
  ASSERT_TRUE(v.Empty());

  v.Add(3u);
  v.Add(1u);
  v.Add(3u);
  v.Add(2u);
  ASSERT_EQ(v.Size(), 4u);

  v.SortAndUnique();
  ASSERT_EQ(v.Size(), 3u);
  ASSERT_EQ(v[0], 1u);
  ASSERT_EQ(v[1], 2u);
  ASSERT_EQ(v[2], 3u);

  uint32_t sum = 0u;
  for (uint32_t x : v) {
    sum += x;
  }
  ASSERT_EQ(sum, 6u);

  hyde::rt::Vec<uint32_t> w(hyde::rt::MallocAllocator());
  w.Swap(v);
  ASSERT_TRUE(v.Empty());
  ASSERT_EQ(w.Size(), 3u);

  auto moved = std::move(w);
  ASSERT_EQ(moved.Size(), 3u);

  moved.Clear();
  ASSERT_TRUE(moved.Empty());
}

TEST(Vec, GrowsPastInlineCapacity) {
  hyde::rt::Vec<PairRow> v(hyde::rt::MallocAllocator());
  for (uint32_t i = 0u; i < 10000u; ++i) {
    v.Add({i, i * 2u});
  }
  ASSERT_EQ(v.Size(), 10000u);
  ASSERT_EQ(v[9999].y, 19998u);
}

TEST(Table, InsertFindStates) {
  hyde::rt::Table<PairRow> t(hyde::rt::MallocAllocator());

  ASSERT_EQ(static_cast<int>(t.State({1u, 2u})),
            static_cast<int>(hyde::rt::TupleState::kAbsent));

  auto r1 = t.TryChangeAbsentToPresent({1u, 2u});
  ASSERT_TRUE(r1.changed);
  auto r2 = t.TryChangeAbsentToPresent({1u, 2u});
  ASSERT_FALSE(r2.changed);
  ASSERT_EQ(r1.id, r2.id);

  ASSERT_EQ(static_cast<int>(t.State({1u, 2u})),
            static_cast<int>(hyde::rt::TupleState::kPresent));
  ASSERT_EQ(t.NumRows(), 1u);

  // Differential cycle: present -> unknown -> present.
  ASSERT_TRUE(t.TryChangePresentToUnknown({1u, 2u}));
  ASSERT_FALSE(t.TryChangeAbsentToPresent({1u, 2u}).changed);
  ASSERT_TRUE(t.TryChangeAbsentOrUnknownToPresent({1u, 2u}).changed);

  // Retraction: present -> absent -> present again.
  ASSERT_TRUE(t.TryChangePresentToAbsent({1u, 2u}));
  ASSERT_EQ(static_cast<int>(t.State({1u, 2u})),
            static_cast<int>(hyde::rt::TupleState::kAbsent));
  ASSERT_TRUE(t.TryChangeAbsentToPresent({1u, 2u}).changed);

  // Row storage is stable: still one row.
  ASSERT_EQ(t.NumRows(), 1u);
}

TEST(Table, ManyRowsRehashCorrectly) {
  hyde::rt::Table<PairRow> t(hyde::rt::MallocAllocator());
  constexpr uint32_t kN = 50000u;

  for (uint32_t i = 0u; i < kN; ++i) {
    ASSERT_TRUE(t.TryChangeAbsentToPresent({i, i ^ 0xdeadbeefu}).changed);
  }
  ASSERT_EQ(t.NumRows(), kN);

  // Every row is findable after many rehashes, and no duplicate inserts.
  for (uint32_t i = 0u; i < kN; ++i) {
    ASSERT_FALSE(t.TryChangeAbsentToPresent({i, i ^ 0xdeadbeefu}).changed);
  }

  // Scan sees every row exactly once.
  std::set<uint32_t> seen;
  for (uint32_t id = 0u; id < t.NumRows(); ++id) {
    ASSERT_EQ(static_cast<int>(t.StateAt(id)),
              static_cast<int>(hyde::rt::TupleState::kPresent));
    seen.insert(t.RowAt(id).x);
  }
  ASSERT_EQ(seen.size(), kN);
}

TEST(Index, KeyedChains) {
  hyde::rt::Table<PairRow> t(hyde::rt::MallocAllocator());
  hyde::rt::Index<KeyX> by_x(hyde::rt::MallocAllocator());

  // Three rows under x=7, one under x=8, none under x=9.
  for (PairRow row : {PairRow{7u, 1u}, PairRow{7u, 2u}, PairRow{8u, 1u},
                      PairRow{7u, 3u}}) {
    if (auto ins = t.TryChangeAbsentToPresent(row); ins.added_row) {
      by_x.Add({row.x}, ins.id);
    }
  }

  std::set<uint32_t> ys;
  for (uint32_t id = by_x.First({7u}); id != hyde::rt::kNoRow;
       id = by_x.Next(id)) {
    ys.insert(t.RowAt(id).y);
  }
  ASSERT_EQ(ys.size(), 3u);
  ASSERT_TRUE(ys.contains(1u));
  ASSERT_TRUE(ys.contains(2u));
  ASSERT_TRUE(ys.contains(3u));

  ASSERT_NE(by_x.First({8u}), hyde::rt::kNoRow);
  ASSERT_EQ(by_x.First({9u}), hyde::rt::kNoRow);
}

TEST(Index, ManyKeysRehash) {
  hyde::rt::Table<PairRow> t(hyde::rt::MallocAllocator());
  hyde::rt::Index<KeyX> by_x(hyde::rt::MallocAllocator());
  constexpr uint32_t kN = 20000u;

  for (uint32_t i = 0u; i < kN; ++i) {
    // Two rows per key.
    for (uint32_t j = 0u; j < 2u; ++j) {
      if (auto ins = t.TryChangeAbsentToPresent({i, j}); ins.added_row) {
        by_x.Add({i}, ins.id);
      }
    }
  }

  for (uint32_t i = 0u; i < kN; ++i) {
    unsigned n = 0u;
    for (uint32_t id = by_x.First({i}); id != hyde::rt::kNoRow;
         id = by_x.Next(id)) {
      ++n;
    }
    ASSERT_EQ(n, 2u);
  }
}

TEST(Hash, DistinctRowsDistinctHashes) {
  // Not a strong property test; a sanity check that the mixer isn't degenerate.
  std::set<uint64_t> hashes;
  for (uint32_t i = 0u; i < 1000u; ++i) {
    hashes.insert(PairRow{i, 0u}.Hash());
    hashes.insert(PairRow{0u, i}.Hash());
  }
  ASSERT_EQ(hashes.size(), 1999u);  // {0,0} counted once.
}
