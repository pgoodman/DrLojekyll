#include <algorithm>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "datalog.h"

// D5 recursive-negate standing fixture (checkpoint (d), analysis sec 4.5 R9).
//   reach(X) : src(X).
//   reach(Y) : reach(X), edge(X, Y), !dead(Y).   -- negate on a recursive SCC
//   reach_out(X) : reach(X).
// `dead` is @differential so both crossover arms live; the negate sits inside
// the recursive stratum, exercising the R2 fixpoint-context negate gate
// (kInNew both signs on refire) and the recursive crossover class.
//
// Oracle-verified per-batch truth (analysis sec 4.5 R9):
//   after batch 1: reach = {1,2,3,4}
//   after batch 2: reach = {1,2}          (+dead 3 kills 3 and, transitively, 4)
//   after batch 3: reach = {1,2,3,4}      (-dead 3 restores both)
//   after batch 4: reach = {1,2,3,4}      (+dead 3 / -dead 3 nets to zero)
// Final-state oracle golden: reach_out = {1,2,3,4}.

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  auto dump = [&db](const char *label) {
    std::vector<uint64_t> rows;
    auto c = reach_out_f(db);
    for (uint64_t x = 0; c.next(x);) {
      rows.push_back(x);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << label << ": reach:";
    for (auto x : rows) {
      std::cout << ' ' << x;
    }
    std::cout << '\n';
  };

  auto dead_batch = [&](std::vector<uint64_t> adds, std::vector<uint64_t> rems) {
    hyde::rt::Vec<Tup_u64> add(allocator);
    hyde::rt::Vec<Tup_u64> rem(allocator);
    for (auto x : adds) add.Add({x});
    for (auto x : rems) rem.Add({x});
    dead_1(db, log, functors, std::move(add), std::move(rem));
  };

  // Batch 1: seed src=1 and the chain 1->2->3->4.
  {
    hyde::rt::Vec<Tup_u64> s(allocator);
    s.Add({1});
    src_1(db, log, functors, std::move(s));
  }
  {
    hyde::rt::Vec<Tup_u64_u64> e(allocator);
    e.Add({1, 2});
    e.Add({2, 3});
    e.Add({3, 4});
    edge_2(db, log, functors, std::move(e));
  }
  dump("after batch 1");

  // Batch 2: +dead 3. Kills reach(3) and, transitively, reach(4).
  dead_batch({3}, {});
  dump("after batch 2");

  // Batch 3: -dead 3. Restores reach(3) and reach(4).
  dead_batch({}, {3});
  dump("after batch 3");

  // Batch 4: same-batch flap +dead 3 / -dead 3, nets to zero.
  dead_batch({3}, {3});
  dump("after batch 4");
  return 0;
}
