// Driver for config_agg_1 (config-dependent aggregate). sum_above sums only
// readings whose Val >= the per-sensor Threshold. C-5 reduction free
// functions gain the leading config arg (Threshold). Per-Sensor query drains
// are SORTED before printing (cursor contract, CLAUDE.md).
//
// FUNCTOR SEMANTICS (must match the oracle's by-name interpretation):
//   sum_above(cfg=Threshold): running sum of Val over members with
//     Threshold <= Val (identity 0; combine +v iff cfg<=v; uncombine -v iff
//     cfg<=v — invertible because cfg is constant per group).
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#include "datalog.h"

int32_t sum_above_identity() { return 0; }
int32_t sum_above_combine(int32_t cfg, int32_t w, int32_t v) {
  return (cfg <= v) ? (w + v) : w;
}
int32_t sum_above_uncombine(int32_t cfg, int32_t w, int32_t v) {
  return (cfg <= v) ? (w - v) : w;
}

static void Dump(Database &db) {
  for (int32_t s = 0; s <= 6; ++s) {
    std::vector<int32_t> totals;
    auto c = total_above_bf(db, s);
    for (int32_t t; c.next(t);) {
      totals.push_back(t);
    }
    std::sort(totals.begin(), totals.end());
    for (auto t : totals) {
      std::cout << s << " " << t << "\n";
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

  // Set per-sensor thresholds: sensor 1 -> 15, sensor 2 -> 50.
  {
    hyde::rt::Vec<set_threshold_input> vec(allocator);
    vec.Add({1, 15});
    vec.Add({2, 50});
    set_threshold_2(db, log, functors, std::move(vec));
  }
  // Readings. Sensor 1: 10 (<15, excluded), 20 (>=15), 30 (>=15) -> 50.
  //           Sensor 2: 40 (<50, excluded), 60 (>=50), 100 (>=50) -> 160.
  {
    hyde::rt::Vec<add_reading_input> vec(allocator);
    vec.Add({1, 10});
    vec.Add({1, 20});
    vec.Add({1, 30});
    vec.Add({2, 40});
    vec.Add({2, 60});
    vec.Add({2, 100});
    add_reading_2(db, log, functors, std::move(vec));
  }
  Dump(db);

  // Batch 2 (ADD-ONLY): an above-threshold reading arrives. Sensor 1's sum
  // CHANGES 50 -> 75 (25 >= 15), exercising the emit_touched change arm
  // (-old, +new) AND re-exercising the config gate (25 passes the threshold).
  // Add-only avoids any removal-message convention; the change arm is still
  // driven because the group's summary value moves.
  {
    hyde::rt::Vec<add_reading_input> vec(allocator);
    vec.Add({1, 25});
    add_reading_2(db, log, functors, std::move(vec));
  }
  Dump(db);
  return 0;
}
