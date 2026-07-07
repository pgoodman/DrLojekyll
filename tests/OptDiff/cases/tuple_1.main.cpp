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
    auto c = db.out_f();
    for (int32_t v = 0; c.next(v);) {
      vals.push_back(v);
    }
    std::sort(vals.begin(), vals.end());
    std::cout << "out:";
    for (auto v : vals) {
      std::cout << ' ' << v;
    }
    std::cout << '\n';
  };

  dump();

  {
    hyde::rt::Vec<input_input> rows(allocator);
    rows.Add({3});
    rows.Add({7});
    db.input_1(std::move(rows));
  }
  dump();

  {
    hyde::rt::Vec<input_input> rows(allocator);
    rows.Add({7});
    rows.Add({-2});
    db.input_1(std::move(rows));
  }
  dump();
  return 0;
}
