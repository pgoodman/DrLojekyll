#include <algorithm>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "datalog.h"

// Nonlinear TC diagonal self-loop (nonlin_tc_both_change driver pattern).
// The exactly-once + termination witness for the slice-4 nonlinear JoinFire.
//   p(From,To) : edge(From,To).
//   p(From,To) : p(From,Y), p(Y,To).
// A single self-loop edge(a,a) with a=7 makes p(7,7) tie BOTH same-stratum
// join positions to the same row (Y=7). The p(7,7) x p(7,7) self-join
// instance must fire EXACTLY ONCE per claim round (diagonal self-tie broken
// by the scanned occurrence's own Now bit); the recursion must terminate
// with p(7,7) present, then fully drain on retraction.
//
// Ground truth (positive-only equivalence oracle) final state:
//   after add    reachable = {(7,7)}
//   after remove reachable = {} (empty)
// (goldens/nonlin_diag_selfloop.oracle.stdout / .monotone.stdout).

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  auto dump = [&db](const char *label) {
    std::vector<std::pair<uint64_t, uint64_t>> rows;
    auto c = reachable_ff(db);
    for (uint64_t f = 0, t = 0; c.next(f, t);) {
      rows.emplace_back(f, t);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << label << ":\n";
    for (auto [f, t] : rows) {
      std::cout << "reachable(" << f << ", " << t << ")\n";
    }
  };

  auto edge_batch = [&](std::vector<std::pair<uint64_t, uint64_t>> adds,
                        std::vector<std::pair<uint64_t, uint64_t>> rems) {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    for (auto [f, t] : adds) add.Add({f, t});
    for (auto [f, t] : rems) rem.Add({f, t});
    add_edge_2(db, log, functors, std::move(add), std::move(rem));
  };

  // Batch 1: add the diagonal self-loop edge(7,7). p(7,7) present.
  edge_batch({{7, 7}}, {});
  dump("after add");

  // Batch 2: remove edge(7,7). The self-supporting diagonal drains to empty.
  edge_batch({}, {{7, 7}});
  dump("after remove");
  return 0;
}
