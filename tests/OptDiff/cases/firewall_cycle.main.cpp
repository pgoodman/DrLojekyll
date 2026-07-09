#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#include "datalog.h"

// REDERIVE / firewall standing fixture (checkpoint-(c) companion; adjacent
// to FLAG-E kRecursivelySupported):
//   p(X) : base(X).
//   p(X) : p(Y), edge(Y,X).
// A self-supporting cycle 1<->2 (base(1) only) must drain to empty when
// base(1) is retracted (REDERIVE's C_r>0 post-quiescence check finds C_r=0
// for the whole cycle), while a firewalled cycle 3<->4 (base(3) AND base(4))
// must keep {3,4} when only base(3) is retracted (base(4)'s C_nr>0 firewall
// refuses the crossing at p(4)). One oracle run ground-truths both arms.
// See docs/proposals/StackSafeNegation.checkpoint-c-notes.md open-flags.
//
// Ground truth (positive-only equivalence oracle) final state:
//   p_out = {3, 4}.
// (goldens/firewall_cycle.oracle.stdout / .monotone.stdout).
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
    auto c = db.p_out_f();
    for (uint64_t x = 0; c.next(x);) {
      rows.push_back(x);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << label << ":\n";
    for (auto x : rows) {
      std::cout << "p(" << x << ")\n";
    }
  };

  auto edge_batch = [&](std::vector<std::pair<uint64_t, uint64_t>> adds) {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    for (auto [f, t] : adds) add.Add({f, t});
    db.edge_2(std::move(add), std::move(rem));
  };

  auto base_batch = [&](std::vector<uint64_t> adds, std::vector<uint64_t> rems) {
    hyde::rt::Vec<Tup_u64> add(allocator);
    hyde::rt::Vec<Tup_u64> rem(allocator);
    for (auto x : adds) add.Add({x});
    for (auto x : rems) rem.Add({x});
    db.base_1(std::move(add), std::move(rem));
  };

  // Batch 1: edges for both cycles (1<->2 and 3<->4), never retracted.
  edge_batch({{1, 2}, {2, 1}, {3, 4}, {4, 3}});
  dump("after batch 1");

  // Batch 2: base(1) [drain arm], base(3)+base(4) [firewall arm].
  base_batch({1, 3, 4}, {});
  dump("after batch 2");

  // Batch 3: retract base(1) AND base(3). Drain arm empties, firewall arm
  // keeps {3,4}. Final p_out = {3,4}.
  base_batch({}, {1, 3});
  dump("after batch 3");
  return 0;
}
