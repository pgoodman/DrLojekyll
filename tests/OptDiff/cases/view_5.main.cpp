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
    dump1("self", db.out_self_f());
    dump1("two", db.out_two_f());
  };

  dump();
  {
    hyde::rt::Vec<input_input> rows(allocator);
    rows.Add({1});
    rows.Add({2});
    db.input_1(std::move(rows));
    hyde::rt::Vec<m1_input> r1(allocator);
    r1.Add({0});
    r1.Add({3});
    db.m1_1(std::move(r1));
    hyde::rt::Vec<m2_input> r2(allocator);
    r2.Add({3});
    r2.Add({4});
    db.m2_1(std::move(r2));
  }
  dump();
  {
    hyde::rt::Vec<input_input> rows(allocator);
    rows.Add({2});  // duplicate
    rows.Add({5});
    db.input_1(std::move(rows));
    hyde::rt::Vec<m2_input> r2(allocator);
    r2.Add({0});
    r2.Add({7});
    db.m2_1(std::move(r2));
  }
  dump();
  return 0;
}
