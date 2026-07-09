#include <algorithm>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "datalog.h"

// FLAG-H (cross-round axis) standing fixture: linear recursion
//   q(X) : base(X).
//   q(Y) : q(X), link(X,Y).
// over a diamond of links so the SAME row is reachable at two different
// round depths from one base retraction. This stresses the Delta_D claim
// discipline (fire over the per-round CLAIMED frontier, never the raw
// drained queue): a diamond re-enqueue of an already-claimed row must not
// re-fire. Complements nonlin_tc_both_change's within-round asymmetry axis.
// See docs/proposals/StackSafeNegation.checkpoint-c-notes.md open-flags.
//
// Ground truth (positive-only equivalence oracle) final state:
//   q_out = {} (retracting the sole base fact drains the whole cascade).
// (goldens/diamond_reenqueue.oracle.stdout / .monotone.stdout).
//
// GOLDEN NOTE: red until checkpoint (c); compiled-driver golden ABSENT,
// author at (c) slice-2 bring-up.

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator, log, functors);

  auto dump = [&db](const char *label) {
    std::vector<uint64_t> rows;
    auto c = db.q_out_f();
    for (uint64_t x = 0; c.next(x);) {
      rows.push_back(x);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << label << ":\n";
    for (auto x : rows) {
      std::cout << "q(" << x << ")\n";
    }
  };

  auto link_batch = [&](std::vector<std::pair<uint64_t, uint64_t>> adds) {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    for (auto [f, t] : adds) add.Add({f, t});
    db.link_2(std::move(add), std::move(rem));
  };

  auto base_batch = [&](std::vector<uint64_t> adds, std::vector<uint64_t> rems) {
    hyde::rt::Vec<Tup_u64> add(allocator);
    hyde::rt::Vec<Tup_u64> rem(allocator);
    for (auto x : adds) add.Add({x});
    for (auto x : rems) rem.Add({x});
    db.base_1(std::move(add), std::move(rem));
  };

  // Batch 1: seed the diamond links (never retracted).
  link_batch({{1, 2}, {1, 3}, {2, 4}, {3, 4}, {4, 5}, {2, 5}});
  dump("after batch 1");

  // Batch 2: seed base(1); q_out reaches {1,2,3,4,5}.
  base_batch({1}, {});
  dump("after batch 2");

  // Batch 3: retract base(1); q_out must become EMPTY.
  base_batch({}, {1});
  dump("after batch 3");
  return 0;
}
