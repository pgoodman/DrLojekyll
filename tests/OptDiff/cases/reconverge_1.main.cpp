#include <algorithm>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "datalog.h"

// Driver for reconverge_1.dr: a differential source `src(X, Y)` fans out
// through reconvergent table-less CMP plumbing into `mid`, which fans out
// again into `sink`, exercising the DR-IR branch discovery's memoized
// worklist on reconvergent plumbing (v3-spec section 1.4 / B-13).
//
// Ground truth: `mid(X, Y)` holds for every src pair with X != Y (the three
// clause bodies X<Y / X>Y / X!=Y together admit exactly X != Y); `sink(X)`
// holds for every X that appears as the first column of such a pair. So
// `sink_q = { X : exists Y, src(X, Y) held and X != Y }`.
//
// KEYED-DRAIN NOTE (cursor contract): the query cursor enumeration order is
// unspecified, so we SORT the drained rows before printing.

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  auto dump = [&db](const char *label) {
    std::vector<uint64_t> rows;
    auto c = sink_q_f(db);
    for (uint64_t x = 0; c.next(x);) {
      rows.push_back(x);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << label << ":\n";
    for (auto x : rows) {
      std::cout << "sink(" << x << ")\n";
    }
  };

  auto src_batch = [&](std::vector<std::pair<uint64_t, uint64_t>> adds,
                       std::vector<std::pair<uint64_t, uint64_t>> rems) {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    for (auto [x, y] : adds) add.Add({x, y});
    for (auto [x, y] : rems) rem.Add({x, y});
    src_2(db, log, functors, std::move(add), std::move(rem));
  };

  // Batch 1: seed pairs. (5,5) has X==Y so contributes NOTHING to mid/sink.
  src_batch({{1, 2}, {2, 1}, {3, 9}, {5, 5}, {7, 4}}, {});
  dump("after batch 1");  // sink = {1, 2, 3, 7}

  // Batch 2: add a fresh pair and a second pair sharing first column 3.
  src_batch({{4, 8}, {3, 6}}, {});
  dump("after batch 2");  // sink = {1, 2, 3, 4, 7}

  // Batch 3: retract (3,9); 3 still supported by (3,6), so sink keeps 3.
  src_batch({}, {{3, 9}});
  dump("after batch 3");  // sink = {1, 2, 3, 4, 7}

  // Batch 4: retract (3,6) too; now 3 has no surviving X!=Y pair, so it drops.
  src_batch({}, {{3, 6}});
  dump("after batch 4");  // sink = {1, 2, 4, 7}
  return 0;
}
