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
  Database db(allocator, log, functors);

  auto dump1 = [](const char *name, auto cursor) {
    std::vector<int32_t> vals;
    for (int32_t v = 0; cursor.next(v);) {
      vals.push_back(v);
    }
    std::sort(vals.begin(), vals.end());
    std::cout << name << ':';
    for (auto v : vals) {
      std::cout << ' ' << v;
    }
    std::cout << '\n';
  };
  auto dump = [&]() {
    dump1("out1", db.out1_f());
    dump1("out2", db.out2_f());
  };

  dump();
  {
    hyde::rt::Vec<input_input> rows(allocator);
    rows.Add({1});
    rows.Add({2});
    db.input_1(std::move(rows));
  }
  {
    hyde::rt::Vec<m2_input> rows(allocator);
    rows.Add({2});
    rows.Add({3});
    db.m2_1(std::move(rows));
  }
  dump();
  {
    hyde::rt::Vec<input_input> rows(allocator);
    rows.Add({4});
    db.input_1(std::move(rows));
  }
  {
    hyde::rt::Vec<m2_input> rows(allocator);
    rows.Add({1});  // already reachable via `input`
    db.m2_1(std::move(rows));
  }
  dump();
  return 0;
}
