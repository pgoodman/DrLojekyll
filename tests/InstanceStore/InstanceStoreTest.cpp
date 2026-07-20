// Copyright 2026, Peter Goodman. All rights reserved.
//
// Unit tests for the InstanceStore (include/drlojekyll/Runtime/InstanceStore.h,
// the keyed-instances runtime primitive, KeyedInstances §A.3.1 R-1). Covers:
// mint/find/collision-growth; Touch/Touched/KeyAt; the double-buffer Seal
// (pointer swap + Reset + occupancy snapshot; frozen becomes prior current);
// RecycleCurrent idempotence (same-epoch flap); the H6 Arena regression
// (bounded allocation across reset/refill cycles); the pre-D3 death half
// (Recycle-then-partial-readd is a genuine drop under monotone=false); and the
// RAT-5 belt-fires NEGATIVE (a shrunk monotone Seal must abort with SIGABRT,
// exercised in a forked child). Intent-communicating asserts throughout
// (ASSERT_EQ/LT/GT, EQ on rows).

#include <DrTest.h>

#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <new>
#include <set>
#include <sys/wait.h>
#include <unistd.h>
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

}  // namespace

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

// -------------------------------------------------------------------------
// The H6 Arena regression (two observables).
// -------------------------------------------------------------------------

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

// The pre-D3 death half (monotone=false): RecycleCurrent empties current, a
// partial rebuild leaves a genuine (T,F) drop, and Seal (belt off) advances the
// occupancy snapshot — including occupancy DEATH (occupied->empty). Proves the
// store SUPPORTS retraction ahead of the D3.a R-DIFF wiring.
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

// -------------------------------------------------------------------------
// RAT-5: the belt-fires NEGATIVE. Under monotone=true (the R-MONO default) a
// Seal whose current is a STRICT SHRINK of frozen violates frozen ⊆ current;
// the HP-7 belt must fire — and the trip is an assert() abort (SIGABRT), which
// would kill the whole test process, so it runs in a forked child (the
// tests/DeltaRelValidators/InstanceOrderTest.cpp shape). The belt is
// assert()-based (debug-only); under NDEBUG it compiles out, so the whole arm
// is #ifndef NDEBUG-guarded — a release-built test binary skips it cleanly.
// -------------------------------------------------------------------------

#ifndef NDEBUG
namespace {

// Child-termination classes the arm asserts on. kForkFailed makes a fork()
// failure loud instead of masquerading as either outcome (RAT-5 pin).
enum class ChildOutcome { kCleanExit, kSigAbrt, kOtherAbnormal, kForkFailed };

// Run a deliberately-shrinking monotone Seal in a forked child and classify how
// the child terminated. The belt is fprintf + assert-abort, so the ONLY
// termination the death arm accepts is SIGABRT specifically — any other crash
// must turn the test red, not pass as a green death. stdout is flushed before
// fork so the child's abort-time flush cannot replay the parent's buffered
// DrTest progress lines into the ctest log.
static ChildOutcome RunShrinkSealInChild(void) {
  std::fflush(nullptr);
  const pid_t pid = fork();
  if (pid < 0) {
    return ChildOutcome::kForkFailed;
  }
  if (pid == 0) {
    Store st(MallocAllocator());  // monotone=true (default): belt armed.
    const uint32_t g = st.FindOrAddInstance(KeyX{1});
    // Epoch 1: frozen becomes {10,20}.
    st.TouchCurrent(g).TryAdd(Nbr{10});
    st.Current(g).TryAdd(Nbr{20});
    st.Seal();
    // Epoch 2: rebuild ONLY {10} — a (T,F) drop of {20} that the R-MONO belt
    // proves cannot exist. Seal MUST abort.
    st.TouchCurrent(g).TryAdd(Nbr{10});
    st.Seal();      // reached ONLY if the belt failed to fire.
    _exit(0);
  }
  int status = 0;
  (void) waitpid(pid, &status, 0);
  if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
    return ChildOutcome::kCleanExit;
  }
  if (WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT) {
    return ChildOutcome::kSigAbrt;
  }
  return ChildOutcome::kOtherAbnormal;
}

}  // namespace

TEST(InstanceStore, BeltFiresOnMonotoneDrop) {
  ASSERT_EQ(static_cast<int>(ChildOutcome::kSigAbrt),
            static_cast<int>(RunShrinkSealInChild()));
}
#endif  // NDEBUG
