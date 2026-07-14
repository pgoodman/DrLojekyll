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

  auto dump = [&db]() {
    std::vector<std::pair<int32_t, int32_t>> rows;
    auto c = q_ff(db);
    for (int32_t x = 0, y = 0; c.next(x, y);) {
      rows.emplace_back(x, y);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << "q:";
    for (auto &[x, y] : rows) {
      std::cout << " (" << x << ',' << y << ')';
    }
    std::cout << '\n';
  };

  dump();
  {
    hyde::rt::Vec<a_input> av(allocator);
    av.Add({1, 2});
    av.Add({3, 4});
    a_2(db, log, functors, std::move(av));
    hyde::rt::Vec<b_input> bv(allocator);
    bv.Add({2, 1});
    bv.Add({5, 6});
    b_2(db, log, functors, std::move(bv));
  }
  dump();
  {
    hyde::rt::Vec<b_input> bv(allocator);
    bv.Add({4, 3});
    bv.Add({7, 8});
    b_2(db, log, functors, std::move(bv));
  }
  dump();
  return 0;
}
