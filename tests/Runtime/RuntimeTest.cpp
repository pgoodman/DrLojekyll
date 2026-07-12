// Copyright 2026, Trail of Bits. All rights reserved.
//
// Unit tests for the runtime: allocators, vectors, tables, and indexes.

#include <DrTest.h>

#include <cstdint>
#include <initializer_list>
#include <set>
#include <utility>
#include <vector>

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

TEST(Vec, NetBatchNetsAddsAgainstRemoves) {
  hyde::rt::Vec<PairRow> adds(hyde::rt::MallocAllocator());
  hyde::rt::Vec<PairRow> removes(hyde::rt::MallocAllocator());

  // Set semantics with annihilation (MD sec 5.0/5.5, OQ3): each side is
  // deduplicated and the adds-intersect-removes set is dropped from both,
  // so duplicate ops are meaningless and a row on both sides is a no-op.
  //   {1,1}: adds x2              -> add (dedup'd)
  //   {2,2}: adds x1, removes x1  -> annihilates
  //   {3,3}: adds x1, removes x2  -> annihilates (removes can't outvote)
  //   {4,4}: adds x1              -> add
  //   {5,5}: removes x2           -> remove (dedup'd)
  adds.Add({1u, 1u});
  adds.Add({2u, 2u});
  adds.Add({1u, 1u});
  adds.Add({3u, 3u});
  adds.Add({4u, 4u});
  removes.Add({2u, 2u});
  removes.Add({3u, 3u});
  removes.Add({3u, 3u});
  removes.Add({5u, 5u});
  removes.Add({5u, 5u});

  hyde::rt::NetBatch(adds, removes);

  // One copy per add-only row, in first-appearance order.
  ASSERT_EQ(adds.Size(), 2u);
  ASSERT_TRUE(adds[0] == (PairRow{1u, 1u}));
  ASSERT_TRUE(adds[1] == (PairRow{4u, 4u}));

  // One copy per remove-only row; both-sides rows vanish from both.
  ASSERT_EQ(removes.Size(), 1u);
  ASSERT_TRUE(removes[0] == (PairRow{5u, 5u}));
}

TEST(Vec, NetBatchOnEmptyVectorsIsANoOp) {
  hyde::rt::Vec<PairRow> adds(hyde::rt::MallocAllocator());
  hyde::rt::Vec<PairRow> removes(hyde::rt::MallocAllocator());
  hyde::rt::NetBatch(adds, removes);
  ASSERT_TRUE(adds.Empty());
  ASSERT_TRUE(removes.Empty());
}

TEST(Table, MonotoneTryAddDedup) {
  hyde::rt::Table<PairRow> t(hyde::rt::MallocAllocator());

  ASSERT_EQ(t.Find({1u, 2u}), hyde::rt::kNoRow);

  auto r1 = t.TryAdd({1u, 2u});
  ASSERT_TRUE(r1.added);
  auto r2 = t.TryAdd({1u, 2u});
  ASSERT_FALSE(r2.added);
  ASSERT_EQ(r1.id, r2.id);

  ASSERT_EQ(t.Find({1u, 2u}), r1.id);
  ASSERT_TRUE(t.Present(r1.id));
  ASSERT_EQ(t.NumRows(), 1u);
}

TEST(Table, ManyRowsRehashCorrectly) {
  hyde::rt::Table<PairRow> t(hyde::rt::MallocAllocator());
  constexpr uint32_t kN = 50000u;

  for (uint32_t i = 0u; i < kN; ++i) {
    ASSERT_TRUE(t.TryAdd({i, i ^ 0xdeadbeefu}).added);
  }
  ASSERT_EQ(t.NumRows(), kN);

  // Every row is findable after many rehashes, and no duplicate inserts.
  for (uint32_t i = 0u; i < kN; ++i) {
    ASSERT_FALSE(t.TryAdd({i, i ^ 0xdeadbeefu}).added);
  }

  // Scan sees every row exactly once.
  std::set<uint32_t> seen;
  for (uint32_t id = 0u; id < t.NumRows(); ++id) {
    seen.insert(t.RowAt(id).x);
  }
  ASSERT_EQ(seen.size(), kN);
}

namespace {

using DiffPairs = hyde::rt::DiffTable<PairRow>;
using hyde::rt::DerivClass;

// A commit sink that records the published (row, added) crossings.
struct RecordingSink {
  std::vector<std::pair<PairRow, bool>> published;

  void operator()(const PairRow &row, bool added) {
    published.emplace_back(row, added);
  }
};

}  // namespace

TEST(DiffTable, AddCrossingAndCommit) {
  DiffPairs t(hyde::rt::MallocAllocator());

  // First derivation up-crosses; the second does not.
  auto d1 = t.AddDerivation({1u, 2u}, DerivClass::kNonRecursive);
  ASSERT_TRUE(d1.crossed);
  ASSERT_TRUE(d1.added_row);
  auto d2 = t.AddDerivation({1u, 2u}, DerivClass::kRecursive);
  ASSERT_FALSE(d2.crossed);
  ASSERT_FALSE(d2.added_row);
  ASSERT_EQ(d1.id, d2.id);

  // Mid-batch: count-based presence holds; InI is still the empty snapshot.
  ASSERT_TRUE(t.Present(d1.id));
  ASSERT_TRUE(t.RecursivelySupported(d1.id));
  ASSERT_FALSE(t.InI(d1.id));

  RecordingSink sink;
  t.Commit(sink);
  ASSERT_EQ(sink.published.size(), 1u);
  ASSERT_TRUE(sink.published[0].first == (PairRow{1u, 2u}));
  ASSERT_TRUE(sink.published[0].second);

  // Sealed snapshot: InI and InNew now agree with presence.
  ASSERT_TRUE(t.InI(d1.id));
  ASSERT_TRUE(t.InNew(d1.id));
  t.DebugValidateCounts();
}

TEST(DiffTable, SubCrossingFiresOnEveryInIDecrementWithNonPositiveNr) {
  DiffPairs t(hyde::rt::MallocAllocator());

  // Row with C_nr = 0, C_r = 1, committed present.
  auto d = t.AddDerivation({3u, 4u}, DerivClass::kRecursive);
  RecordingSink sink;
  t.Commit(sink);
  ASSERT_TRUE(t.InI(d.id));

  // A recursive decrement fires the down-crossing because the post-state
  // C_nr is non-positive, regardless of which counter moved.
  auto s = t.SubDerivation({3u, 4u}, DerivClass::kRecursive);
  ASSERT_TRUE(s.crossed);
  ASSERT_FALSE(t.Present(s.id));
}

TEST(DiffTable, NrFirewallSuppressesCrossing) {
  DiffPairs t(hyde::rt::MallocAllocator());

  // Row with C_nr = 2, committed.
  t.AddDerivation({5u, 6u}, DerivClass::kNonRecursive);
  auto d = t.AddDerivation({5u, 6u}, DerivClass::kNonRecursive);
  RecordingSink sink;
  t.Commit(sink);

  // First decrement leaves C_nr = 1 > 0: no crossing, cascade stops dead.
  ASSERT_FALSE(t.SubDerivation({5u, 6u}, DerivClass::kNonRecursive).crossed);

  // Second decrement reaches C_nr = 0: crossing.
  ASSERT_TRUE(t.SubDerivation({5u, 6u}, DerivClass::kNonRecursive).crossed);

  RecordingSink sink2;
  t.Commit(sink2);
  ASSERT_EQ(sink2.published.size(), 1u);
  ASSERT_FALSE(sink2.published[0].second);
  ASSERT_FALSE(t.InI(d.id));
}

TEST(DiffTable, PhantomPairOnNeverPresentRowIsSilent) {
  DiffPairs t(hyde::rt::MallocAllocator());

  // Phantom minus on a never-present head: the counter dips to -1, but the
  // crossing predicate requires kInI, so nothing propagates.
  auto s = t.SubDerivation({7u, 8u}, DerivClass::kNonRecursive);
  ASSERT_FALSE(s.crossed);
  ASSERT_TRUE(s.added_row);  // Fresh log entry even for a transient negative.
  ASSERT_FALSE(t.Present(s.id));

  // The paired phantom plus restores -1 -> 0; the up-crossing predicate
  // requires after_total > 0, so it too is silent.
  auto a = t.AddDerivation({7u, 8u}, DerivClass::kNonRecursive);
  ASSERT_FALSE(a.crossed);

  RecordingSink sink;
  t.Commit(sink);
  ASSERT_TRUE(sink.published.empty());
  t.DebugValidateCounts();
}

TEST(DiffTable, TransientNegativeDipOnPresentRowNetsToNothing) {
  DiffPairs t(hyde::rt::MallocAllocator());

  // Present row with C_nr = 1.
  t.AddDerivation({9u, 1u}, DerivClass::kNonRecursive);
  RecordingSink sink0;
  t.Commit(sink0);

  // Two decrements (1 -> 0 -> -1): both fire the down-crossing (duplicate
  // enqueues are absorbed by TryClaimDel), then two increments restore 1.
  auto s1 = t.SubDerivation({9u, 1u}, DerivClass::kNonRecursive);
  ASSERT_TRUE(s1.crossed);
  auto s2 = t.SubDerivation({9u, 1u}, DerivClass::kNonRecursive);
  ASSERT_TRUE(s2.crossed);

  auto a1 = t.AddDerivation({9u, 1u}, DerivClass::kNonRecursive);
  ASSERT_FALSE(a1.crossed);  // -1 -> 0: not an up-crossing.
  auto a2 = t.AddDerivation({9u, 1u}, DerivClass::kNonRecursive);
  ASSERT_TRUE(a2.crossed);  // 0 -> 1: a genuine up-crossing.

  // Batch-start presence equals final presence: nothing is published.
  RecordingSink sink;
  t.Commit(sink);
  ASSERT_TRUE(sink.published.empty());
  ASSERT_TRUE(t.Present(s1.id));
  t.DebugValidateCounts();
}

TEST(DiffTable, ClaimDedupAndFrontierRetirement) {
  DiffPairs t(hyde::rt::MallocAllocator());

  auto d = t.AddDerivation({2u, 2u}, DerivClass::kNonRecursive);
  RecordingSink sink0;
  t.Commit(sink0);

  t.SubDerivation({2u, 2u}, DerivClass::kNonRecursive);

  // Claim into D exactly once.
  ASSERT_TRUE(t.TryClaimDel(d.id));
  ASSERT_FALSE(t.TryClaimDel(d.id));

  // While in the current frontier round, the row is alive-at-claim but no
  // longer survives-so-far, and it is a net deletion.
  ASSERT_FALSE(t.SurvivesSoFar(d.id));
  ASSERT_TRUE(t.AliveAtClaim(d.id));
  ASSERT_FALSE(t.InNew(d.id));
  ASSERT_TRUE(t.NetDeleted(d.id));
  ASSERT_FALSE(t.NetAdded(d.id));

  // Retiring the row clears only the frontier bit.
  t.RetireDel(d.id);
  ASSERT_FALSE(t.AliveAtClaim(d.id));

  RecordingSink sink;
  t.Commit(sink);
  ASSERT_EQ(sink.published.size(), 1u);
  ASSERT_FALSE(sink.published[0].second);

  // Commit reset the claim flags: the row is claimable again next batch.
  t.AddDerivation({2u, 2u}, DerivClass::kNonRecursive);
  t.SubDerivation({2u, 2u}, DerivClass::kNonRecursive);
  ASSERT_TRUE(t.TryClaimDel(d.id));
  RecordingSink sink2;
  t.Commit(sink2);
  ASSERT_TRUE(sink2.published.empty());
}

TEST(DiffTable, AddClaimAndInsertFrontierPredicates) {
  DiffPairs t(hyde::rt::MallocAllocator());

  auto d = t.AddDerivation({4u, 4u}, DerivClass::kRecursive);
  ASSERT_TRUE(d.crossed);

  ASSERT_TRUE(t.TryClaimAdd(d.id));
  ASSERT_FALSE(t.TryClaimAdd(d.id));

  // Claimed into A and into the current insert frontier round.
  ASSERT_TRUE(t.InNew(d.id));
  ASSERT_TRUE(t.InNewWithFrontier(d.id));
  ASSERT_FALSE(t.InNewSansFrontier(d.id));

  t.RetireAdd(d.id);
  ASSERT_TRUE(t.InNewSansFrontier(d.id));

  RecordingSink sink;
  t.Commit(sink);
  ASSERT_EQ(sink.published.size(), 1u);
  ASSERT_TRUE(sink.published[0].second);
}

TEST(DiffTable, DeleteThenRederivePublishesNothing) {
  DiffPairs t(hyde::rt::MallocAllocator());

  // Committed row supported once nonrecursively.
  auto d = t.AddDerivation({6u, 6u}, DerivClass::kNonRecursive);
  RecordingSink sink0;
  t.Commit(sink0);

  // Overdelete it, claim it, then rederive it via a recursive support and
  // claim the re-add: net no change, so Commit publishes nothing and the
  // row ends the batch with both claim flags reset.
  ASSERT_TRUE(t.SubDerivation({6u, 6u}, DerivClass::kNonRecursive).crossed);
  ASSERT_TRUE(t.TryClaimDel(d.id));
  ASSERT_TRUE(t.AddDerivation({6u, 6u}, DerivClass::kRecursive).crossed);
  ASSERT_TRUE(t.TryClaimAdd(d.id));

  // Claimed into both D and A: neither a net deletion nor a net addition.
  ASSERT_FALSE(t.NetDeleted(d.id));
  ASSERT_FALSE(t.NetAdded(d.id));

  RecordingSink sink;
  t.Commit(sink);
  ASSERT_TRUE(sink.published.empty());
  ASSERT_TRUE(t.Present(d.id));
  ASSERT_TRUE(t.InI(d.id));
  t.DebugValidateCounts();
}

TEST(DiffTable, ExplicitSupportIsOneSetSemanticsBit) {
  DiffPairs t(hyde::rt::MallocAllocator());

  // First explicit add up-crosses; a duplicate explicit add is a no-op.
  auto a1 = t.AddExplicit({8u, 8u});
  ASSERT_TRUE(a1.crossed);
  auto a2 = t.AddExplicit({8u, 8u});
  ASSERT_FALSE(a2.crossed);
  ASSERT_TRUE(t.Present(a1.id));

  RecordingSink sink0;
  t.Commit(sink0);
  ASSERT_EQ(sink0.published.size(), 1u);

  // Explicit removal drops the one explicit support; a duplicate removal
  // (and one on a never-added row) is a no-op.
  auto s1 = t.SubExplicit({8u, 8u});
  ASSERT_TRUE(s1.crossed);
  auto s2 = t.SubExplicit({8u, 8u});
  ASSERT_FALSE(s2.crossed);
  ASSERT_FALSE(t.SubExplicit({1u, 9u}).crossed);

  RecordingSink sink;
  t.Commit(sink);
  ASSERT_EQ(sink.published.size(), 1u);
  ASSERT_FALSE(sink.published[0].second);
  t.DebugValidateCounts();
}

TEST(Index, KeyedChains) {
  hyde::rt::Table<PairRow> t(hyde::rt::MallocAllocator());
  hyde::rt::Index<KeyX> by_x(hyde::rt::MallocAllocator());

  // Three rows under x=7, one under x=8, none under x=9.
  for (PairRow row : {PairRow{7u, 1u}, PairRow{7u, 2u}, PairRow{8u, 1u},
                      PairRow{7u, 3u}}) {
    if (auto ins = t.TryAdd(row); ins.added) {
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
      if (auto ins = t.TryAdd({i, j}); ins.added) {
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
