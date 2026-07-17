// Driver for config_agg_2 (config-dependent @recompute aggregate). max_above is
// the MAX of readings whose Val >= the per-sensor Threshold. The C-5 @recompute
// reduction body max_above_reduce takes the LEADING config arg (Threshold) and
// rescans the live multiset — the arm where config enters at reduce time. A
// group with no above-threshold live member returns the sentinel meaning
// "no max" (INT32_MIN); the engine emits no row for such a group because its
// occupancy is driven by fold count, not the gate — see the INVARIANT below.
//
// INVARIANT: every (Sensor, Threshold) group keeps >=1 above-threshold live
// member in EVERY batch state — do not edit a batch to violate this, or the
// engine (occupancy by fold count) publishes INT32_MIN while the oracle
// suppresses the row, and the cross-check FIRES (the recorded occupancy-vs-gate
// gap; d2-config-agg-2-target.md §2.3 NOTE / §6 prediction 8).
//
// FUNCTOR SEMANTICS (must match the oracle's by-name interpretation):
//   max_above(cfg=Threshold): max of Val over live members with Threshold<=Val.
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#include "datalog.h"

int32_t max_above_reduce(int32_t cfg, const int32_t *values,
                         const int32_t *counts, ::std::size_t n) {
  bool any = false;
  int32_t best = 0;
  for (::std::size_t i = 0; i < n; ++i) {
    if (counts[i] <= 0) { continue; }        // retracted-to-absent member
    if (values[i] < cfg) { continue; }       // below the per-group threshold
    if (!any || values[i] > best) { best = values[i]; any = true; }
  }
  // No above-threshold live member: return the "empty group" sentinel. The
  // corpus keeps every group non-empty at the gate (see the INVARIANT above),
  // so this branch never publishes; a general gated-empty @recompute is a
  // recorded follow-on (occupancy-vs-gate reconciliation).
  return any ? best : INT32_MIN;
}

static void Dump(Database &db) {
  for (int32_t s = 0; s <= 6; ++s) {
    std::vector<int32_t> maxes;
    auto c = max_of_bf(db, s);
    for (int32_t m; c.next(m);) {
      maxes.push_back(m);
    }
    std::sort(maxes.begin(), maxes.end());
    for (auto m : maxes) {
      std::cout << s << " " << m << "\n";
    }
  }
  std::cout << "--\n";
}

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  // Per-sensor thresholds: sensor 1 -> 15, sensor 2 -> 50.
  {
    hyde::rt::Vec<set_threshold_input> vec(allocator);
    vec.Add({1, 15});
    vec.Add({2, 50});
    set_threshold_2(db, log, functors, std::move(vec));
  }
  // Readings (add-side of the @differential message).
  //   sensor 1: {10 (<15 excluded), 20, 30} -> max 30
  //   sensor 2: {40 (<50 excluded), 60, 100} -> max 100
  {
    // add_reading is @differential -> its entry point takes two Vecs (added,
    // removed) of the message row type (no `_input` alias is emitted for a
    // differential message; the 2xi32 row type is Tup_i32_i32).
    hyde::rt::Vec<Tup_i32_i32> added(allocator);
    hyde::rt::Vec<Tup_i32_i32> removed(allocator);
    added.Add({1, 10});
    added.Add({1, 20});
    added.Add({1, 30});
    added.Add({2, 40});
    added.Add({2, 60});
    added.Add({2, 100});
    add_reading_2(db, log, functors, std::move(added), std::move(removed));
  }
  Dump(db);

  // Batch 2: RETRACT sensor 2's reading 100. Live above-threshold set becomes
  // {60} -> max 60. The @recompute rescan drops the old max (100) and finds the
  // runner-up (60) — the descending step an @invertible max cannot do. Exercises
  // the emit_touched CHANGE arm (-old 100, +new 60) AND config at ReduceLive.
  {
    hyde::rt::Vec<Tup_i32_i32> added(allocator);
    hyde::rt::Vec<Tup_i32_i32> removed(allocator);
    removed.Add({2, 100});
    add_reading_2(db, log, functors, std::move(added), std::move(removed));
  }
  Dump(db);

  // Batch 3: ADD sensor 1 reading 25 (>=15). Live {20,30,25} -> max still 30.
  // The group is TOUCHED (a fold happened) but new==old -> emit NOTHING (the
  // no-op arm), a distinct emit_touched path from the change arm.
  {
    hyde::rt::Vec<Tup_i32_i32> added(allocator);
    hyde::rt::Vec<Tup_i32_i32> removed(allocator);
    added.Add({1, 25});
    add_reading_2(db, log, functors, std::move(added), std::move(removed));
  }
  Dump(db);
  return 0;
}
