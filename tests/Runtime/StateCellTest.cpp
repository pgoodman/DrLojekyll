// Copyright 2026, Peter Goodman. All rights reserved.
//
// Unit tests for the StateCell store (include/drlojekyll/Runtime/StateCell.h,
// the R3a delta-relational-IR runtime primitive, spec
// v3-spec-statecell.md §1). Covers: fold/old/emit/seal round-trips; the
// two-word cell (old() frozen while working moves); the one-net-pair
// contract shape; OQ3 annihilation under an @invertible algebra; group-id
// density + the non-aliasing invariant; MIN-via-@recompute rescan; and the
// declared-algebra property checks (§3, unfold ∘ fold == id on sampled
// values). Positive AND negative space per the intent-communicating assert
// style (ASSERT_LT/LE/GT/GE, EQ on structs).

#include <DrTest.h>

#include <cstddef>
#include <cstdint>
#include <new>
#include <set>
#include <vector>

#include <drlojekyll/Runtime/Allocator.h>
#include <drlojekyll/Runtime/Hash.h>
#include <drlojekyll/Runtime/StateCell.h>
#include <drlojekyll/Runtime/Vec.h>

namespace {

using hyde::rt::Invertible;
using hyde::rt::kNoGroup;
using hyde::rt::Recompute;
using hyde::rt::StateCellStore;
using hyde::rt::Vec;

// A one-column summary scalar (a SUM/COUNT/MIN value), as codegen would emit
// a single summary column.
struct I32 {
  int32_t v{0};
  bool operator==(const I32 &) const noexcept = default;
  auto operator<=>(const I32 &) const noexcept = default;
};

// A group key: the outer group-by column (e.g. the destination node X in
// average_incoming_weight). No summary value in the key — that is the
// non-aliasing property (spec §1.1).
struct KeyX {
  int32_t x{0};
  uint64_t Hash(void) const noexcept {
    return hyde::rt::HashRow(x);
  }
  bool operator==(const KeyX &) const noexcept = default;
};

// --- SUM: @invertible, Working == Summary == I32 --------------------------
struct SumReduce {
  using Working = I32;
  using Summary = I32;
  static void Identity(Working &w) {
    w = I32{0};
  }
  static void Combine(Working &w, const Summary &v) {
    w.v += v.v;
  }
  static void Uncombine(Working &w, const Summary &v) {
    w.v -= v.v;
  }
  static Summary Finalize(const Working &w) {
    return w;
  }
};
using SumCell = StateCellStore<KeyX, Invertible<SumReduce>>;

// --- COUNT: @invertible, folds a +1/-1 regardless of the value ------------
struct CountReduce {
  using Working = I32;
  using Summary = I32;  // The value column is present but ignored (a count).
  static void Identity(Working &w) {
    w = I32{0};
  }
  static void Combine(Working &w, const Summary &) {
    w.v += 1;
  }
  static void Uncombine(Working &w, const Summary &) {
    w.v -= 1;
  }
  static Summary Finalize(const Working &w) {
    return w;
  }
};
using CountCell = StateCellStore<KeyX, Invertible<CountReduce>>;

// --- AVG: @invertible with a Working {sum,count} and a dividing Finalize.
// The two-level reduction average_weight.dr's div_i32(Sum,Count) computes,
// folded into one cell to exercise a non-identity Finalize.
struct AvgWorking {
  int32_t sum{0};
  int32_t count{0};
};
struct AvgReduce {
  using Working = AvgWorking;
  using Summary = I32;
  static void Identity(Working &w) {
    w = AvgWorking{0, 0};
  }
  static void Combine(Working &w, const Summary &v) {
    w.sum += v.v;
    w.count += 1;
  }
  static void Uncombine(Working &w, const Summary &v) {
    w.sum -= v.v;
    w.count -= 1;
  }
  static Summary Finalize(const Working &w) {
    return I32{w.count ? w.sum / w.count : 0};
  }
};
using AvgCell = StateCellStore<KeyX, Invertible<AvgReduce>>;

// --- MIN: @recompute (spec §6/G-5). No O(1) unfold; emit rescans the group.
struct MinReduce {
  using Summary = I32;
  static Summary ReduceLive(const Vec<Summary> &values,
                            const Vec<int32_t> &counts) {
    bool seen = false;
    int32_t best = 0;
    for (size_t i = 0u, n = values.Size(); i < n; ++i) {
      if (counts[i] <= 0) {
        continue;  // Retracted to identity; not a live member.
      }
      if (!seen || values[i].v < best) {
        best = values[i].v;
        seen = true;
      }
    }
    return I32{seen ? best : 0};  // Empty group -> identity.
  }
};
using MinCell = StateCellStore<KeyX, Recompute<MinReduce>>;

}  // namespace

// --------------------------------------------------------------------------
// Fold / emit / old / seal round-trip: the batch lifecycle of one group.
// --------------------------------------------------------------------------
TEST(StateCell, FoldEmitOldSealRoundTrip) {
  SumCell sc(hyde::rt::MallocAllocator());

  // Empty store: no group yet.
  ASSERT_EQ(sc.FindGroup({7}), kNoGroup);
  ASSERT_EQ(sc.NumGroups(), 0u);

  // First touch allocates dense group id 0; identity working, identity old().
  const uint32_t g = sc.FindOrAddGroup({7});
  ASSERT_EQ(g, 0u);
  ASSERT_EQ(sc.NumGroups(), 1u);
  ASSERT_EQ(sc.FindGroup({7}), g);
  ASSERT_EQ(sc.Emit(g), (I32{0}));
  ASSERT_EQ(sc.Old(g), (I32{0}));

  // Fold in +5, +3: working moves to 8, but old() still reads the sealed
  // batch-start value (0) — the two-word cell (spec §1.2 / C-0b).
  sc.Fold(g, +1, I32{5});
  sc.Fold(g, +1, I32{3});
  ASSERT_EQ(sc.Emit(g), (I32{8}));
  ASSERT_EQ(sc.Old(g), (I32{0}));

  // Seal advances the batch-start snapshot to the working value.
  sc.Seal();
  ASSERT_EQ(sc.Old(g), (I32{8}));
  ASSERT_EQ(sc.Emit(g), (I32{8}));  // Working is untouched by Seal.
  sc.DebugValidate();

  // Next epoch: fold -3, working 5, old() frozen at 8 until the next Seal.
  sc.Fold(g, -1, I32{3});
  ASSERT_EQ(sc.Emit(g), (I32{5}));
  ASSERT_EQ(sc.Old(g), (I32{8}));
  ASSERT_GT(sc.Old(g).v, sc.Emit(g).v);  // Retraction: old > new this epoch.
  sc.Seal();
  ASSERT_EQ(sc.Old(g), (I32{5}));
  sc.DebugValidate();
}

// --------------------------------------------------------------------------
// The two-word cell under an interleaved retraction/addition: old() must
// report the pre-batch value while working moves through negatives.
// --------------------------------------------------------------------------
TEST(StateCell, TwoWordCellFrozenOldWhileWorkingMoves) {
  SumCell sc(hyde::rt::MallocAllocator());
  const uint32_t g = sc.FindOrAddGroup({1});
  sc.Fold(g, +1, I32{10});
  sc.Seal();
  ASSERT_EQ(sc.Old(g), (I32{10}));

  // -10 then +4: working dips to 0 then rises to 4; old() stays 10 all epoch.
  sc.Fold(g, -1, I32{10});
  ASSERT_EQ(sc.Emit(g), (I32{0}));
  ASSERT_EQ(sc.Old(g), (I32{10}));
  sc.Fold(g, +1, I32{4});
  ASSERT_EQ(sc.Emit(g), (I32{4}));
  ASSERT_EQ(sc.Old(g), (I32{10}));

  sc.Seal();
  ASSERT_EQ(sc.Old(g), (I32{4}));
}

// --------------------------------------------------------------------------
// One-net-pair contract shape (spec §2.2 BAND (b) / §2.5 V-ONE-NET-PAIR): k
// folds on one group in a batch produce exactly one sealed transition. The
// emit_touched guard fires (new != old) at most once per touched group; a
// SORT-UNIQUE touched set has one entry regardless of the fold count.
// --------------------------------------------------------------------------
TEST(StateCell, OneNetPairPerGroupRegardlessOfFoldCount) {
  SumCell sc(hyde::rt::MallocAllocator());
  const uint32_t g = sc.FindOrAddGroup({42});
  sc.Fold(g, +1, I32{1});
  sc.Seal();  // old(g) = 1.

  // Eight folds this batch; the touched set still names g exactly once.
  for (int i = 0; i < 4; ++i) {
    sc.Fold(g, +1, I32{2});
    sc.Fold(g, -1, I32{1});
  }
  const Vec<uint32_t> &touched = sc.Touched();
  ASSERT_EQ(touched.Size(), 1u);
  ASSERT_EQ(touched[0], g);

  // emit_touched: exactly one (old, new) pair for the group, new != old.
  const I32 old_v = sc.Old(g);
  const I32 new_v = sc.Emit(g);
  ASSERT_EQ(old_v, (I32{1}));
  ASSERT_EQ(new_v, (I32{5}));  // 1 + 4*(2-1) = 5.
  ASSERT_NE(old_v, new_v);     // Exactly one net pair emitted.
  sc.Seal();
  ASSERT_EQ(sc.Old(g), (I32{5}));
  sc.DebugValidate();
}

// --------------------------------------------------------------------------
// OQ3-style annihilation (spec §6 annihilation sub-case / V-AGG-NET): fold
// +x then -x under an @invertible algebra leaves working == old, so
// emit_touched emits NO pair (new == old). Netting composes.
// --------------------------------------------------------------------------
TEST(StateCell, AnnihilationEmitsNoNetPair) {
  SumCell sc(hyde::rt::MallocAllocator());
  const uint32_t g = sc.FindOrAddGroup({3});
  sc.Fold(g, +1, I32{16});
  sc.Seal();
  ASSERT_EQ(sc.Old(g), (I32{16}));

  // Retract 3, re-add 3 in the same batch: working returns to old exactly.
  sc.Fold(g, -1, I32{3});
  ASSERT_EQ(sc.Emit(g), (I32{13}));  // Transient dip mid-batch.
  sc.Fold(g, +1, I32{3});

  // The group WAS touched, but new == old, so no net pair is emitted.
  const Vec<uint32_t> &touched = sc.Touched();
  ASSERT_EQ(touched.Size(), 1u);
  ASSERT_EQ(sc.Emit(g), sc.Old(g));  // new == old: emit nothing.

  sc.Seal();
  ASSERT_EQ(sc.Old(g), (I32{16}));  // Unchanged across the annihilated batch.
  sc.DebugValidate();
}

// --------------------------------------------------------------------------
// Group-id density + non-aliasing invariant (spec §1.1): keys map to a dense
// 0..N id space, allocated in first-touch order, stable across the store's
// life. Re-touching an existing key returns the SAME id (no aliasing, no
// renumbering — group ids never move even as the DiffTable compacts).
// --------------------------------------------------------------------------
TEST(StateCell, GroupIdsDenseAndStable) {
  SumCell sc(hyde::rt::MallocAllocator());

  // First touch of three distinct keys yields 0, 1, 2 in order.
  const uint32_t g10 = sc.FindOrAddGroup({10});
  const uint32_t g20 = sc.FindOrAddGroup({20});
  const uint32_t g30 = sc.FindOrAddGroup({30});
  ASSERT_EQ(g10, 0u);
  ASSERT_EQ(g20, 1u);
  ASSERT_EQ(g30, 2u);
  ASSERT_EQ(sc.NumGroups(), 3u);

  // Re-touch: same id, no new group (density + stability).
  ASSERT_EQ(sc.FindOrAddGroup({20}), g20);
  ASSERT_EQ(sc.NumGroups(), 3u);

  // KeyAt is the inverse of FindOrAddGroup on the dense id.
  ASSERT_EQ(sc.KeyAt(g10), (KeyX{10}));
  ASSERT_EQ(sc.KeyAt(g30), (KeyX{30}));

  // Every allocated id is in range and distinct (density invariant).
  std::set<uint32_t> ids{g10, g20, g30};
  ASSERT_EQ(ids.size(), 3u);
  for (uint32_t id : ids) {
    ASSERT_LT(id, sc.NumGroups());
  }
  ASSERT_EQ(sc.FindGroup({99}), kNoGroup);  // Negative space: absent key.
}

// Rehash stress: many keys must all round-trip through the open-addressing
// slot array after growth, each keeping its dense id.
TEST(StateCell, ManyGroupsRehashKeepsDenseIds) {
  SumCell sc(hyde::rt::MallocAllocator());
  constexpr int32_t kN = 20000;

  for (int32_t i = 0; i < kN; ++i) {
    const uint32_t g = sc.FindOrAddGroup({i});
    ASSERT_EQ(g, static_cast<uint32_t>(i));  // Dense, first-touch order.
    sc.Fold(g, +1, I32{i});
  }
  ASSERT_EQ(sc.NumGroups(), static_cast<uint32_t>(kN));

  // Every key still finds its id after all the rehashes, and no duplicates.
  for (int32_t i = 0; i < kN; ++i) {
    ASSERT_EQ(sc.FindGroup({i}), static_cast<uint32_t>(i));
    ASSERT_EQ(sc.Emit(static_cast<uint32_t>(i)), (I32{i}));
  }
  ASSERT_EQ(sc.NumGroups(), static_cast<uint32_t>(kN));
}

// --------------------------------------------------------------------------
// Touched set is sort-unique (spec §2.5 V-TOUCH-SORTED / G-8): folds across
// several groups in scrambled order produce an ascending, duplicate-free
// touched vector, pinning golden determinism.
// --------------------------------------------------------------------------
TEST(StateCell, TouchedSetSortUnique) {
  SumCell sc(hyde::rt::MallocAllocator());
  const uint32_t g0 = sc.FindOrAddGroup({100});
  const uint32_t g1 = sc.FindOrAddGroup({200});
  const uint32_t g2 = sc.FindOrAddGroup({300});
  sc.Seal();  // Clear the first-touch marks.

  // Fold in a scrambled, repeated order.
  sc.Fold(g2, +1, I32{1});
  sc.Fold(g0, +1, I32{1});
  sc.Fold(g2, +1, I32{1});
  sc.Fold(g1, +1, I32{1});
  sc.Fold(g0, +1, I32{1});

  const Vec<uint32_t> &touched = sc.Touched();
  ASSERT_EQ(touched.Size(), 3u);
  ASSERT_EQ(touched[0], g0);
  ASSERT_LT(touched[0], touched[1]);  // Ascending.
  ASSERT_LT(touched[1], touched[2]);
  ASSERT_EQ(touched[2], g2);
}

// --------------------------------------------------------------------------
// MIN via @recompute (spec §6 HAND-TRACE 1): the rescan emit re-runs the
// functor over surviving members. Retracting the current min must surface
// the next-smallest — something no O(1) unfold could know.
// --------------------------------------------------------------------------
TEST(StateCell, MinRecomputeRescanOnRetraction) {
  MinCell sc(hyde::rt::MallocAllocator());
  const uint32_t g = sc.FindOrAddGroup({1});

  // Members {5, 3, 8}; min = 3.
  sc.Fold(g, +1, I32{5});
  sc.Fold(g, +1, I32{3});
  sc.Fold(g, +1, I32{8});
  ASSERT_EQ(sc.Emit(g), (I32{3}));
  sc.Seal();
  ASSERT_EQ(sc.Old(g), (I32{3}));

  // Retract the 3: the rescan must find the new min 5 (the whole point of
  // @recompute — the retracted min's successor is not recoverable by unfold).
  sc.Fold(g, -1, I32{3});
  const I32 new_min = sc.Emit(g);
  const I32 old_min = sc.Old(g);
  ASSERT_EQ(old_min, (I32{3}));
  ASSERT_EQ(new_min, (I32{5}));
  ASSERT_GT(new_min.v, old_min.v);  // Retraction raised the min.
  ASSERT_NE(new_min, old_min);      // One net pair.
  sc.Seal();
  ASSERT_EQ(sc.Old(g), (I32{5}));

  // Add a smaller value: the rescan lowers the min again.
  sc.Fold(g, +1, I32{2});
  ASSERT_EQ(sc.Emit(g), (I32{2}));
  ASSERT_LT(sc.Emit(g).v, sc.Old(g).v);
  sc.Seal();
  ASSERT_EQ(sc.Old(g), (I32{2}));
}

// @recompute membership honors multiplicity: adding 3 twice then retracting
// once keeps 3 live (count 1 > 0); retracting again drops it.
TEST(StateCell, RecomputeMembershipMultiplicity) {
  MinCell sc(hyde::rt::MallocAllocator());
  const uint32_t g = sc.FindOrAddGroup({1});
  sc.Fold(g, +1, I32{3});
  sc.Fold(g, +1, I32{3});  // 3 now has multiplicity 2.
  sc.Fold(g, +1, I32{9});
  ASSERT_EQ(sc.Emit(g), (I32{3}));

  sc.Fold(g, -1, I32{3});  // 3 still live (count 1).
  ASSERT_EQ(sc.Emit(g), (I32{3}));
  sc.Fold(g, -1, I32{3});  // 3 gone (count 0); min is now 9.
  ASSERT_EQ(sc.Emit(g), (I32{9}));
  ASSERT_GT(sc.Emit(g).v, 3);

  // Fully empty the group: emit falls back to the algebra identity.
  sc.Fold(g, -1, I32{9});
  ASSERT_EQ(sc.Emit(g), (I32{0}));
}

// --------------------------------------------------------------------------
// SUM @invertible (spec §6 HAND-TRACE 2): retract 3, add 10 in one batch —
// O(1) unfold then fold — yields exactly one net pair despite two folds.
// --------------------------------------------------------------------------
TEST(StateCell, SumInvertibleRetractAndAddOneNetPair) {
  SumCell sc(hyde::rt::MallocAllocator());
  const uint32_t g = sc.FindOrAddGroup({1});
  sc.Fold(g, +1, I32{5});
  sc.Fold(g, +1, I32{3});
  sc.Fold(g, +1, I32{8});
  ASSERT_EQ(sc.Emit(g), (I32{16}));
  sc.Seal();

  // Retract 3 (16 -> 13), add 10 (13 -> 23): two folds, one net pair.
  sc.Fold(g, -1, I32{3});
  sc.Fold(g, +1, I32{10});
  const Vec<uint32_t> &touched = sc.Touched();
  ASSERT_EQ(touched.Size(), 1u);
  ASSERT_EQ(sc.Old(g), (I32{16}));
  ASSERT_EQ(sc.Emit(g), (I32{23}));
  ASSERT_NE(sc.Old(g), sc.Emit(g));
  sc.Seal();
  ASSERT_EQ(sc.Old(g), (I32{23}));
  sc.DebugValidate();
}

// --------------------------------------------------------------------------
// AVG with a non-identity Finalize (average_weight.dr's Sum/Count folded into
// one cell): the sealed snapshot is the FINALIZED scalar, and old() reports
// it across a batch that changes both sum and count.
// --------------------------------------------------------------------------
TEST(StateCell, AvgFinalizeAndSealSnapshot) {
  AvgCell sc(hyde::rt::MallocAllocator());
  const uint32_t g = sc.FindOrAddGroup({1});

  // {10, 20, 30}: sum 60, count 3, avg 20.
  sc.Fold(g, +1, I32{10});
  sc.Fold(g, +1, I32{20});
  sc.Fold(g, +1, I32{30});
  ASSERT_EQ(sc.Emit(g), (I32{20}));
  sc.Seal();
  ASSERT_EQ(sc.Old(g), (I32{20}));  // Sealed = the finalized average.

  // Retract 10, add 50: sum 100, count 3, avg 33 (integer division).
  sc.Fold(g, -1, I32{10});
  sc.Fold(g, +1, I32{50});
  ASSERT_EQ(sc.Emit(g), (I32{33}));
  ASSERT_EQ(sc.Old(g), (I32{20}));
  ASSERT_GT(sc.Emit(g).v, sc.Old(g).v);
  sc.Seal();
  ASSERT_EQ(sc.Old(g), (I32{33}));
  sc.DebugValidate();
}

// --------------------------------------------------------------------------
// COUNT @invertible: folds a +1/-1 per input regardless of value; the sealed
// snapshot tracks the running group size.
// --------------------------------------------------------------------------
TEST(StateCell, CountInvertibleTracksGroupSize) {
  CountCell sc(hyde::rt::MallocAllocator());
  const uint32_t g = sc.FindOrAddGroup({1});
  sc.Fold(g, +1, I32{999});  // Value ignored; count = 1.
  sc.Fold(g, +1, I32{7});    // count = 2.
  sc.Fold(g, +1, I32{7});    // count = 3 (multiset, not a set).
  ASSERT_EQ(sc.Emit(g), (I32{3}));
  sc.Seal();

  sc.Fold(g, -1, I32{7});  // count = 2.
  ASSERT_EQ(sc.Emit(g), (I32{2}));
  ASSERT_LT(sc.Emit(g).v, sc.Old(g).v);
  sc.Seal();
  ASSERT_EQ(sc.Old(g), (I32{2}));
  sc.DebugValidate();
}

// --------------------------------------------------------------------------
// Independent groups do not alias: a fold on one group leaves every other
// group's working AND sealed value untouched (columnar-by-dense-id, spec
// §1.4). This is the store-level face of the non-aliasing invariant.
// --------------------------------------------------------------------------
TEST(StateCell, IndependentGroupsDoNotAlias) {
  SumCell sc(hyde::rt::MallocAllocator());
  const uint32_t ga = sc.FindOrAddGroup({1});
  const uint32_t gb = sc.FindOrAddGroup({2});
  sc.Fold(ga, +1, I32{100});
  sc.Fold(gb, +1, I32{5});
  sc.Seal();

  // Move only group a this epoch.
  sc.Fold(ga, +1, I32{1});
  ASSERT_EQ(sc.Emit(ga), (I32{101}));
  ASSERT_EQ(sc.Emit(gb), (I32{5}));   // b's working untouched.
  ASSERT_EQ(sc.Old(gb), (I32{5}));    // b's sealed untouched.

  // Only group a is touched; the emit sweep would visit a alone.
  const Vec<uint32_t> &touched = sc.Touched();
  ASSERT_EQ(touched.Size(), 1u);
  ASSERT_EQ(touched[0], ga);
}

// --------------------------------------------------------------------------
// Declared-algebra property check host (spec §1.5 / AggregatingFunctors §3):
// DebugValidate runs the unfold ∘ fold == id law on sampled @invertible
// groups. A large populated store must pass; this exercises the sampling
// walk itself.
// --------------------------------------------------------------------------
TEST(StateCell, DebugValidateAlgebraLawsOnManyGroups) {
  AvgCell sc(hyde::rt::MallocAllocator());
  for (int32_t i = 0; i < 500; ++i) {
    const uint32_t g = sc.FindOrAddGroup({i});
    sc.Fold(g, +1, I32{i});
    sc.Fold(g, +1, I32{i * 2});
  }
  sc.Seal();
  sc.DebugValidate();  // Asserts internally; a clean return is the pass.
  ASSERT_EQ(sc.NumGroups(), 500u);
}

// --------------------------------------------------------------------------
// (Review finding 1) @recompute membership handle SURVIVAL across rehash: a
// @recompute Working is a POD handle {values*, counts*} into store-owned Vecs
// (the membership pool). The open-addressing slot array rehashes as groups
// accumulate (InsertSlot grows past the 7/8 load), but a rehash only rewires
// `slots` — the `working` column (and thus every group's membership handle) is
// indexed by the STABLE dense gid and is never moved. So each group's rescan
// (Emit re-runs the functor over its own surviving members) must remain correct
// after many rehashes: the handle still points at THAT group's Vecs, never at a
// neighbor's. Regression guard for a rehash that mistakenly touched `working`.
// --------------------------------------------------------------------------
TEST(StateCell, RecomputeHandleSurvivesRehash) {
  MinCell sc(hyde::rt::MallocAllocator());
  constexpr int32_t kN = 5000;  // Well past the first several rehashes.

  // Each group i gets a distinct three-member multiset {i+100, i+50, i+200};
  // its MIN is i+50. The distinct per-group members let a mis-wired handle
  // (pointing at a neighbor's Vecs) surface as a wrong rescan result.
  for (int32_t i = 0; i < kN; ++i) {
    const uint32_t g = sc.FindOrAddGroup({i});
    ASSERT_EQ(g, static_cast<uint32_t>(i));  // Dense, first-touch order.
    sc.Fold(g, +1, I32{i + 100});
    sc.Fold(g, +1, I32{i + 50});
    sc.Fold(g, +1, I32{i + 200});
  }
  ASSERT_EQ(sc.NumGroups(), static_cast<uint32_t>(kN));

  // After all the rehashes: every group's handle still rescans ITS members.
  for (int32_t i = 0; i < kN; ++i) {
    const uint32_t g = sc.FindGroup({i});
    ASSERT_EQ(g, static_cast<uint32_t>(i));
    ASSERT_EQ(sc.Emit(g), (I32{i + 50}));  // MIN of the group's own members.
  }

  // Retract each group's min through the surviving handle: the rescan must
  // surface that group's next-smallest (i+100), never a neighbor's.
  for (int32_t i = 0; i < kN; ++i) {
    const uint32_t g = static_cast<uint32_t>(i);
    sc.Fold(g, -1, I32{i + 50});
    ASSERT_EQ(sc.Emit(g), (I32{i + 100}));
  }
}

// --------------------------------------------------------------------------
// (Review finding 1) TEARDOWN with a non-trivial allocator: the store is the
// SOLE allocation authority (StateCell.h §1: `slots`, the key/hash/working/
// sealed/touched columns, and every @recompute membership Vec flow through the
// injected `allocator`). A counting allocator proves the destructor frees
// everything it allocated — in particular FreeMembership + the slots array + the
// column Vecs — so no store-owned block leaks. Exercised on @recompute (the pool
// path) so the membership Vecs' allocate/free pairs are in the ledger too.
// --------------------------------------------------------------------------
namespace {
struct CountingAlloc {
  size_t live_bytes{0};
  size_t live_blocks{0};
  size_t total_allocs{0};
  size_t total_frees{0};

  static void *Alloc(void *ctx, size_t size, size_t align) {
    auto *self = static_cast<CountingAlloc *>(ctx);
    self->live_bytes += size;
    self->live_blocks += 1;
    self->total_allocs += 1;
    return ::operator new(size, std::align_val_t{align});
  }
  static void Free(void *ctx, void *ptr, size_t size, size_t align) {
    auto *self = static_cast<CountingAlloc *>(ctx);
    self->live_bytes -= size;
    self->live_blocks -= 1;
    self->total_frees += 1;
    ::operator delete(ptr, std::align_val_t{align});
  }
  hyde::rt::Allocator View(void) {
    return hyde::rt::Allocator{this, &Alloc, &Free};
  }
};
}  // namespace

// --------------------------------------------------------------------------
// OCCUPANCY (spec §C-1, the batch-1 abort fix). A never-touched group is
// EMPTY: WorkingOccupied/SealedOccupied are both false, and its identity-
// initialized sealed value is NOT a real batch-start row. The occupancy-
// generalized emit_touched guard (which the generated code and the oracle
// apply, reading these accessors) is:
//   birth  (empty->occupied):  +new only;
//   death  (occupied->empty):  -old only;
//   change (occupied, new!=old): -old, +new;
//   no-op  (else):              nothing.
// These traces exercise the three occupancy transitions on the store.
// --------------------------------------------------------------------------

// A tiny model of the generated/oracle emit_touched guard, returning how many
// (-old, +new) counter events a group's transition would emit. Proves the
// store's occupancy accessors express the C-1 guard with no phantom row.
namespace {
template <typename Cell>
struct EmitCounts {
  int retract_old{0};  // -old counter events (0 or 1)
  int assert_new{0};   // +new counter events (0 or 1)
};
template <typename Cell>
EmitCounts<Cell> EmitTouchedGuard(Cell &sc, uint32_t gid) {
  const bool old_occ = sc.SealedOccupied(gid);
  const bool new_occ = sc.WorkingOccupied(gid);
  EmitCounts<Cell> out;
  if (!old_occ && !new_occ) {
    return out;  // never present; nothing (also the identity-init first touch).
  }
  if (old_occ && !new_occ) {
    out.retract_old = 1;  // death: -old only.
    return out;
  }
  if (!old_occ && new_occ) {
    out.assert_new = 1;  // birth: +new only.
    return out;
  }
  if (sc.Emit(gid) != sc.Old(gid)) {  // both occupied.
    out.retract_old = 1;  // change: the one net pair.
    out.assert_new = 1;
  }
  return out;  // else no-op (value unchanged / zero-sum-nonempty).
}
}  // namespace

// BIRTH: the first batch of every aggregate case is all births. A group's
// first fold moves empty->occupied; the guard emits ONLY +new and NEVER a
// phantom -old against the identity-initialized sealed value. (Without C-1 the
// commit sweep's >=0-per-class assert aborts on batch 1 — the CRITICAL fix.)
TEST(StateCell, OccupancyBirthEmitsPlusNewOnly) {
  SumCell sc(hyde::rt::MallocAllocator());
  const uint32_t g = sc.FindOrAddGroup({7});

  // Never-touched: empty in both words. Its sealed value is identity but NOT a
  // real row — occupancy, not value, decides.
  ASSERT_FALSE(sc.SealedOccupied(g));
  ASSERT_FALSE(sc.WorkingOccupied(g));

  // First fold: empty -> occupied (birth).
  sc.Fold(g, +1, I32{5});
  ASSERT_FALSE(sc.SealedOccupied(g));  // old still empty this epoch.
  ASSERT_TRUE(sc.WorkingOccupied(g));

  const EmitCounts<SumCell> e = EmitTouchedGuard(sc, g);
  ASSERT_EQ(e.retract_old, 0);  // NO phantom retraction of the identity row.
  ASSERT_EQ(e.assert_new, 1);   // +new only.

  sc.Seal();
  ASSERT_TRUE(sc.SealedOccupied(g));  // now occupied at next batch start.
  ASSERT_EQ(sc.Old(g), (I32{5}));
  sc.DebugValidate();
}

// DEATH: a group emptied by retracting its last member moves occupied->empty;
// the guard emits ONLY -old and NEVER a phantom +new. Value-based suppression
// cannot see this (SUM=0 is ambiguous) — occupancy makes it exact.
TEST(StateCell, OccupancyDeathEmitsMinusOldOnly) {
  SumCell sc(hyde::rt::MallocAllocator());
  const uint32_t g = sc.FindOrAddGroup({1});
  sc.Fold(g, +1, I32{9});
  sc.Seal();  // sealed-occupied, old()=9.
  ASSERT_TRUE(sc.SealedOccupied(g));

  // Retract the sole member: working value returns to identity (0), but the
  // group is now EMPTY, not a zero-sum group.
  sc.Fold(g, -1, I32{9});
  ASSERT_EQ(sc.Emit(g), (I32{0}));  // value ambiguous with a zero-sum group...
  ASSERT_FALSE(sc.WorkingOccupied(g));  // ...but occupancy is unambiguous.
  ASSERT_TRUE(sc.SealedOccupied(g));

  const EmitCounts<SumCell> e = EmitTouchedGuard(sc, g);
  ASSERT_EQ(e.retract_old, 1);  // -old only.
  ASSERT_EQ(e.assert_new, 0);   // NO phantom +new for the emptied group.

  sc.Seal();
  ASSERT_FALSE(sc.SealedOccupied(g));  // dead at next batch start.
  sc.DebugValidate();
}

// ZERO-SUM-NONEMPTY: a group whose members sum to zero (e.g. {+3, -3}) is
// OCCUPIED (two live members) even though its SUM value is the identity. The
// guard must NOT treat value==identity as death; a same-value re-seal emits
// nothing (no-op), and only a real value change emits the one net pair.
TEST(StateCell, OccupancyZeroSumNonEmptyIsOccupied) {
  SumCell sc(hyde::rt::MallocAllocator());
  const uint32_t g = sc.FindOrAddGroup({2});

  // Members {+3, -3}: SUM = 0 but the group has two members (occupied).
  sc.Fold(g, +1, I32{3});
  sc.Fold(g, +1, I32{-3});
  ASSERT_EQ(sc.Emit(g), (I32{0}));      // value is the identity...
  ASSERT_TRUE(sc.WorkingOccupied(g));   // ...but the group is NOT empty.

  // Birth into a zero-SUM value: occupancy says +new (a real (g,0) row exists).
  EmitCounts<SumCell> e = EmitTouchedGuard(sc, g);
  ASSERT_EQ(e.retract_old, 0);
  ASSERT_EQ(e.assert_new, 1);  // the zero-valued row is asserted, not skipped.
  sc.Seal();
  ASSERT_TRUE(sc.SealedOccupied(g));
  ASSERT_EQ(sc.Old(g), (I32{0}));

  // Next epoch: add another zero-sum pair {+8,-8}. Still occupied, value still
  // 0 == old -> NO net pair (a genuine no-op, not a death).
  sc.Fold(g, +1, I32{8});
  sc.Fold(g, +1, I32{-8});
  ASSERT_TRUE(sc.WorkingOccupied(g));
  ASSERT_EQ(sc.Emit(g), sc.Old(g));
  e = EmitTouchedGuard(sc, g);
  ASSERT_EQ(e.retract_old, 0);
  ASSERT_EQ(e.assert_new, 0);  // value unchanged AND still occupied: nothing.
  sc.Seal();
  sc.DebugValidate();
}

// Full lifecycle across occupancy: birth -> change -> death on one group, each
// transition emitting the C-1-correct event count. This is the batch-sequence
// shape an aggregate case drives; every step keeps the commit-sweep >=0-per-
// class invariant (no phantom either side).
TEST(StateCell, OccupancyBirthChangeDeathLifecycle) {
  SumCell sc(hyde::rt::MallocAllocator());
  const uint32_t g = sc.FindOrAddGroup({1});

  // Batch 1 — BIRTH: +new only.
  sc.Fold(g, +1, I32{10});
  EmitCounts<SumCell> e = EmitTouchedGuard(sc, g);
  ASSERT_EQ(e.retract_old + e.assert_new, 1);
  ASSERT_EQ(e.assert_new, 1);
  sc.Seal();

  // Batch 2 — CHANGE: -old, +new (the one net pair).
  sc.Fold(g, +1, I32{5});
  e = EmitTouchedGuard(sc, g);
  ASSERT_EQ(e.retract_old, 1);
  ASSERT_EQ(e.assert_new, 1);
  sc.Seal();
  ASSERT_EQ(sc.Old(g), (I32{15}));

  // Batch 3 — DEATH: -old only (retract both members).
  sc.Fold(g, -1, I32{10});
  sc.Fold(g, -1, I32{5});
  ASSERT_FALSE(sc.WorkingOccupied(g));
  e = EmitTouchedGuard(sc, g);
  ASSERT_EQ(e.retract_old, 1);
  ASSERT_EQ(e.assert_new, 0);
  sc.Seal();
  ASSERT_FALSE(sc.SealedOccupied(g));
  sc.DebugValidate();
}

// @recompute occupancy: a group emptied by retracting every member (all counts
// back to 0) is EMPTY, so Emit-over-all-dead-membership is never consulted for
// a live row (the guard's death arm fires first). Guards the "Recompute::Emit
// over an all-dead membership is undefined" corner the critique names.
TEST(StateCell, OccupancyRecomputeAllDeadMembershipIsDeath) {
  MinCell sc(hyde::rt::MallocAllocator());
  const uint32_t g = sc.FindOrAddGroup({1});
  sc.Fold(g, +1, I32{5});
  sc.Fold(g, +1, I32{3});
  sc.Seal();
  ASSERT_TRUE(sc.SealedOccupied(g));
  ASSERT_EQ(sc.Old(g), (I32{3}));

  // Retract both members: every count is 0 (all-dead membership).
  sc.Fold(g, -1, I32{5});
  sc.Fold(g, -1, I32{3});
  ASSERT_FALSE(sc.WorkingOccupied(g));  // death, regardless of the rescan value.

  const EmitCounts<MinCell> e = EmitTouchedGuard(sc, g);
  ASSERT_EQ(e.retract_old, 1);  // -old only; the rescan-over-empty is not a row.
  ASSERT_EQ(e.assert_new, 0);
  sc.Seal();
  ASSERT_FALSE(sc.SealedOccupied(g));
  sc.DebugValidate();
}

TEST(StateCell, TeardownFreesEverythingNonTrivialAllocator) {
  CountingAlloc ca;
  {
    MinCell sc(ca.View());  // @recompute: exercises the membership pool.
    for (int32_t i = 0; i < 300; ++i) {
      const uint32_t g = sc.FindOrAddGroup({i});
      sc.Fold(g, +1, I32{i});
      sc.Fold(g, +1, I32{i + 1});
      sc.Fold(g, +1, I32{i + 2});
    }
    sc.Seal();
    ASSERT_EQ(sc.NumGroups(), 300u);
    // The store is holding live allocations right now (columns + pool + slots).
    ASSERT_GT(ca.live_blocks, 0u);
    ASSERT_GT(ca.total_allocs, 0u);
  }
  // After destruction: every block the store allocated is freed. No leak, and
  // FreeMembership (the @recompute pool teardown) ran for every group.
  ASSERT_EQ(ca.live_blocks, 0u);
  ASSERT_EQ(ca.live_bytes, 0u);
  ASSERT_EQ(ca.total_allocs, ca.total_frees);
  ASSERT_GT(ca.total_frees, 0u);
}
