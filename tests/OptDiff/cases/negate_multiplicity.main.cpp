#include <algorithm>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "datalog.h"

// Negate multiplicity standing fixture (checkpoint (d), analysis sec 4.5 R9,
// Q5). The negate keys on A only; several pred rows (A,B*) share the key.
//   seen(A)   : block(A).
//   mid(A, B) : feed(A, B), !seen(A).
//   out(A, B) : mid(A, B).
// Gaining seen(A) must retract EVERY (A,B) row (one `-` crossover fold per
// pred ROW, not per key); losing it must re-derive them all. B is carried
// downstream so the per-row multiplicity is observable.
//
// Oracle-verified per-batch truth:
//   after batch 1 (feed key 1 x3, key 2): out = {(1,10),(1,11),(1,12),(2,20)}
//   after batch 2 (block 1):              out = {(2,20)}
//   after batch 3 (unblock 1):            out = {(1,10),(1,11),(1,12),(2,20)}

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  auto dump = [&db](const char *label) {
    std::vector<std::pair<uint64_t, uint64_t>> rows;
    auto c = out_ff(db);
    for (uint64_t a = 0, b = 0; c.next(a, b);) {
      rows.emplace_back(a, b);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << label << ": out:";
    for (auto &[a, b] : rows) {
      std::cout << " (" << a << ',' << b << ')';
    }
    std::cout << '\n';
  };

  auto block = [&](std::vector<uint64_t> adds, std::vector<uint64_t> rems) {
    hyde::rt::Vec<Tup_u64> add(allocator);
    hyde::rt::Vec<Tup_u64> rem(allocator);
    for (auto a : adds) add.Add({a});
    for (auto a : rems) rem.Add({a});
    block_1(db, log, functors, std::move(add), std::move(rem));
  };

  // Batch 1: key 1 has three rows; key 2 has one. No block.
  {
    hyde::rt::Vec<Tup_u64_u64> f(allocator);
    f.Add({1, 10});
    f.Add({1, 11});
    f.Add({1, 12});
    f.Add({2, 20});
    feed_2(db, log, functors, std::move(f));
  }
  dump("after feed");

  // Batch 2: block key 1 -> three `-` crossover folds; key 2 survives.
  block({1}, {});
  dump("after block(1)");

  // Batch 3: unblock key 1 -> three `+` crossover folds re-derive.
  block({}, {1});
  dump("after unblock(1)");
  return 0;
}
