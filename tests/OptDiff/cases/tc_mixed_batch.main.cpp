#include <algorithm>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "datalog.h"

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

  // Seed state: edge = {(1,2), (1,4), (4,5)}.
  {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    add.Add({1, 2});
    add.Add({1, 4});
    add.Add({4, 5});
    edge_msg_2(db, log, functors, std::move(add), std::move(rem));
  }
  dump("seeded");

  // One mixed batch: remove edge(1,2) AND add edge(2,5). Expected final
  // t: {(1,4), (1,5), (2,5), (4,5)} -- t(1,2) retracts, t(2,5) appears,
  // t(1,5) survives on its untouched support (t(1,4), edge(4,5)).
  {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    add.Add({2, 5});
    rem.Add({1, 2});
    edge_msg_2(db, log, functors, std::move(add), std::move(rem));
  }
  dump("after mixed batch");
  return 0;
}
