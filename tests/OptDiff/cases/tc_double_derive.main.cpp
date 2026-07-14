#include <algorithm>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "datalog.h"

// A2 double-derivation discriminator (promoted from the checkpoint-c notes'
// "Double-derivation repro"; tc_mixed_batch driver pattern with seeded:/after:
// sorted dumps).
//   t(From,To) : edge(From,To).
//   t(From,To) : t(From,X), edge(X,To).     -- linear recursion
// The seed makes t(1,3) double-derived (base edge(1,3) AND recursive
// t(1,2),edge(2,3)); the batch removes BOTH supports. t(1,3) must fully
// retire (overdeleted once per support) while t(1,2) survives on its
// untouched support edge(1,2).
//
// Ground truth (positive-only equivalence oracle) final state:
//   seeded t = {(1,2),(1,3),(2,3)}, after t = {(1,2)}.
// (goldens/tc_double_derive.oracle.stdout / .monotone.stdout).

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  auto dump = [&db](const char *label) {
    std::vector<std::pair<uint64_t, uint64_t>> rows;
    auto c = t_out_ff(db);
    for (uint64_t f = 0, t = 0; c.next(f, t);) {
      rows.emplace_back(f, t);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << label << ":\n";
    for (auto [f, t] : rows) {
      std::cout << "t(" << f << ", " << t << ")\n";
    }
  };

  // Seed: edge = {(1,2),(2,3),(1,3)}. t(1,3) is double-derived.
  {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    add.Add({1, 2});
    add.Add({2, 3});
    add.Add({1, 3});
    edge_msg_2(db, log, functors, std::move(add), std::move(rem));
  }
  dump("seeded");

  // Batch: remove BOTH supports of t(1,3) -- edge(1,3) and edge(2,3).
  // Expected final t = {(1,2)}.
  {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    rem.Add({1, 3});
    rem.Add({2, 3});
    edge_msg_2(db, log, functors, std::move(add), std::move(rem));
  }
  dump("after");
  return 0;
}
