#include <algorithm>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "datalog.h"

// BUILDFRONTIERS producer->consumer seam standing fixture (checkpoint-(c)):
//   tc(From,To) : edge(From,To).
//   tc(From,To) : tc(From,X), tc(X,To).     -- recursive stratum
//   marked(X)   : tc(W,X), watch(W).        -- downstream differential
//   flagged(X)  : marked(X), watch(X).
// The recursive stratum tc must populate its outDel/outAdd net frontiers so
// the higher differential stratum (marked/flagged) consumes them. A broken
// seam corrupts flagged's membership without tripping any within-tc counter
// assert, so this needs an END-STATE oracle comparison.
// See docs/proposals/StackSafeNegation.checkpoint-c-notes.md open-flags.
//
// Ground truth (positive-only equivalence oracle) final state:
//   flagged_out = {4}.
// (goldens/recursive_to_downstream.oracle.stdout / .monotone.stdout).
//
// GOLDEN NOTE: red until checkpoint (c) populates recursive-stratum
// frontiers; compiled-driver golden ABSENT, author at (c) slice-2 bring-up.

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  auto dump = [&db](const char *label) {
    std::vector<uint64_t> rows;
    auto c = flagged_out_f(db);
    for (uint64_t x = 0; c.next(x);) {
      rows.push_back(x);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << label << ":\n";
    for (auto x : rows) {
      std::cout << "flagged(" << x << ")\n";
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

  auto watch_batch = [&](std::vector<uint64_t> adds) {
    hyde::rt::Vec<Tup_u64> add(allocator);
    hyde::rt::Vec<Tup_u64> rem(allocator);
    for (auto x : adds) add.Add({x});
    watch_1(db, log, functors, std::move(add), std::move(rem));
  };

  // Batch 1: chain 1->2->3->4 plus side edge 10->1.
  edge_batch({{1, 2}, {2, 3}, {3, 4}, {10, 1}}, {});
  dump("after batch 1");

  // Batch 2: watch(10), watch(4). marked={1,2,3,4}, flagged={4}.
  watch_batch({10, 4});
  dump("after batch 2");

  // Batch 3: retract edge(2,3). Chain splits; flagged must become EMPTY.
  edge_batch({}, {{2, 3}});
  dump("after batch 3");

  // Batch 4: re-add edge(2,3). flagged must return to {4}.
  edge_batch({{2, 3}}, {});
  dump("after batch 4");
  return 0;
}
