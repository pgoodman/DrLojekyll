#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  auto dump = [&db]() {
    std::vector<int32_t> outs;
    auto c = out_f(db);
    for (int32_t x = 0; c.next(x);) {
      outs.push_back(x);
    }
    std::sort(outs.begin(), outs.end());
    std::cout << "out:";
    for (auto x : outs) {
      std::cout << ' ' << x;
    }
    std::cout << '\n';
  };

  dump();

  {
    hyde::rt::Vec<ping_input> rows(allocator);
    rows.Add({4});
    rows.Add({8});
    ping_1(db, log, functors, std::move(rows));
  }
  dump();

  {
    hyde::rt::Vec<ping_input> rows(allocator);
    rows.Add({15});
    ping_1(db, log, functors, std::move(rows));
  }
  dump();
  return 0;
}
