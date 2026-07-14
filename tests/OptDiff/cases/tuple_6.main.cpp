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
    auto c = out_f(db);
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

  dump();  // correct datalog semantics: empty

  {
    hyde::rt::Vec<input_input> rows(allocator);
    rows.Add({1});
    rows.Add({2});
    input_1(db, log, functors, std::move(rows));
  }
  dump();  // correct datalog semantics: empty (1 and 2 satisfy no clause)

  {
    hyde::rt::Vec<input_input> rows(allocator);
    rows.Add({3});
    rows.Add({4});
    input_1(db, log, functors, std::move(rows));
  }
  dump();  // correct datalog semantics: out: 3
  return 0;
}
