#include <algorithm>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "datalog.h"

// LINEAR recursive -> downstream-differential seam standing fixture
// (checkpoint-(c), the A4 ready_after seam):
//   tc(From,To) : edge(From,To).
//   tc(From,To) : tc(From,X), edge(X,To).   -- LINEAR recursive stratum
//   marked(X)   : tc(W,X), watch(W).        -- downstream differential
//   flagged(X)  : marked(X), watch(X).
// The linear analogue of recursive_to_downstream (whose tc(From,X),tc(X,To)
// is nonlinear and behind the linearity gate). tc must populate its
// outDel/outAdd net frontiers so the higher differential stratum
// (marked/flagged) consumes them. A broken seam corrupts flagged's
// membership without tripping any within-tc counter assert, so this needs
// an END-STATE oracle comparison.
//
// Topology: main chain 10->1->2->3->4 plus diamond 1->5->3, so tc(10,3)/
// tc(10,4) have two edge-disjoint supports through node 1.
//
// Ground truth (positive-only equivalence oracle) FINAL state:
//   flagged_out = {4}.
// (goldens/linear_rec_downstream.oracle.stdout / .monotone.stdout).

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator, log, functors);

  auto dump = [&db](const char *label) {
    std::vector<uint64_t> rows;
    auto c = db.flagged_out_f();
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
    db.add_edge_2(std::move(add), std::move(rem));
  };

  auto watch_batch = [&](std::vector<uint64_t> adds) {
    hyde::rt::Vec<Tup_u64> add(allocator);
    hyde::rt::Vec<Tup_u64> rem(allocator);
    for (auto x : adds) add.Add({x});
    db.watch_1(std::move(add), std::move(rem));
  };

  // Batch 1: build main chain 10->1->2->3->4 and diamond 1->5->3.
  edge_batch({{10, 1}, {1, 2}, {2, 3}, {3, 4}, {1, 5}, {5, 3}}, {});
  dump("after batch 1");

  // Batch 2: watch(10), watch(4). marked={1,2,3,4,5}, flagged={4}.
  watch_batch({10, 4});
  dump("after batch 2");

  // Batch 3: retract edge(2,3). tc(10,3)/tc(10,4) survive via 1->5->3;
  // flagged must NOT flap -- stays {4}.
  edge_batch({}, {{2, 3}});
  dump("after batch 3");

  // Batch 4: retract edge(5,3) too. Both supports gone; flagged EMPTY.
  edge_batch({}, {{5, 3}});
  dump("after batch 4");

  // Batch 5: re-add edge(2,3). flagged must return to {4}.
  edge_batch({{2, 3}}, {});
  dump("after batch 5");
  return 0;
}
