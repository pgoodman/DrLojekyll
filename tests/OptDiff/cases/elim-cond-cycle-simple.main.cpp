#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator, log, functors);

  auto dump = [&db]() {
    std::vector<int32_t> vals;
    auto c = db.output_f();
    for (int32_t v = 0; c.next(v);) {
      vals.push_back(v);
    }
    std::sort(vals.begin(), vals.end());
    std::cout << "output:";
    for (auto v : vals) {
      std::cout << ' ' << v;
    }
    std::cout << '\n';
  };

  dump();  // Before any cond_func message.

  // Round 1: make cond_res true.
  {
    hyde::rt::Vec<cond_func_input> rows(allocator);
    rows.Add({10});
    db.cond_func_1(std::move(rows));
  }
  dump();

  // Round 2: another cond_func fact (condition stays true).
  {
    hyde::rt::Vec<cond_func_input> rows(allocator);
    rows.Add({20});
    db.cond_func_1(std::move(rows));
  }
  dump();
  return 0;
}
