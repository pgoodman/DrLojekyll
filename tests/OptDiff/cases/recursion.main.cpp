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
    std::vector<int32_t> vals;
    auto c = output_f(db);
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

  dump();  // Expect just the constant fact: output: 1

  {
    hyde::rt::Vec<input_input> rows(allocator);
    rows.Add({5});
    rows.Add({9});
    input_1(db, log, functors, std::move(rows));
  }
  dump();  // Expect: output: 1 5 9
  return 0;
}
