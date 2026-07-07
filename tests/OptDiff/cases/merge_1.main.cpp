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
    dump1("dup", db.q_dup_f());
    dump1("self", db.q_self_f());
    dump1("unsat", db.q_unsat_f());
  };

  dump();
  {
    hyde::rt::Vec<m_input> rows(allocator);
    rows.Add({1});
    rows.Add({2});
    rows.Add({3});
    db.m_1(std::move(rows));
  }
  dump();
  {
    hyde::rt::Vec<m_input> rows(allocator);
    rows.Add({4});
    rows.Add({2});  // duplicate of an existing row
    db.m_1(std::move(rows));
  }
  dump();
  return 0;
}
