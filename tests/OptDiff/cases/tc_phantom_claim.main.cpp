#include <algorithm>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "datalog.h"

// Phantom-drift kill fixture (linear tc; tc_double_derive program + driver
// conventions). The standing regression for the slice-4 signed claim gates
// (include/drlojekyll/Runtime/Table.h).
//
// Batch 2 is MD 5.1.1's batch: -edge(1,2), +edge(2,5). The phantom pair on
// join instance (X=2,From=1,To=5) [t(1,2) deleted x edge(2,5) added] fires
// a +1 seed section (run BEFORE OVERDELETE) that crosses 0->1 and enqueues;
// the compensating fixpoint -1 never crosses (kInI=0). Without the claim
// gate (TryClaimAdd: Total>0) that dead entry is claimed and drifts a +1
// into the witness count of t(1,5). Batch 3 removes t(1,5)'s only real
// support edge(4,5): if the drift were real, t(1,5) survives immortally.
// Truth: t(1,5) dies.
//
// Ground truth (positive-only equivalence oracle) final state:
//   seeded t = {(1,2),(1,4),(4,5),(1,5)}
//   after batch2 t = {(1,4),(2,5)}
//   after batch3 t = {(1,4),(2,5)}
// (goldens/tc_phantom_claim.oracle.stdout / .monotone.stdout).

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator, log, functors);

  auto dump = [&db](const char *label) {
    std::vector<std::pair<uint64_t, uint64_t>> rows;
    auto c = db.t_out_ff();
    for (uint64_t f = 0, t = 0; c.next(f, t);) {
      rows.emplace_back(f, t);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << label << ":\n";
    for (auto [f, t] : rows) {
      std::cout << "t(" << f << ", " << t << ")\n";
    }
  };

  // Seed: edge = {(1,2),(1,4),(4,5)}. t = {(1,2),(1,4),(4,5),(1,5)}.
  {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    add.Add({1, 2});
    add.Add({1, 4});
    add.Add({4, 5});
    db.edge_msg_2(std::move(add), std::move(rem));
  }
  dump("seeded");

  // Batch 2: -edge(1,2), +edge(2,5). The phantom join instance (2,1,5).
  {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    rem.Add({1, 2});
    add.Add({2, 5});
    db.edge_msg_2(std::move(add), std::move(rem));
  }
  dump("after batch2");

  // Batch 3: -edge(4,5), removing t(1,5)'s only real support. t(1,5) dies.
  {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    rem.Add({4, 5});
    db.edge_msg_2(std::move(add), std::move(rem));
  }
  dump("after batch3");
  return 0;
}
