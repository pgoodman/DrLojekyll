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
    dump1("dup", q_dup_f(db));
    dump1("self", q_self_f(db));
    dump1("unsat", q_unsat_f(db));
  };

  dump();
  {
    hyde::rt::Vec<m_input> rows(allocator);
    rows.Add({1});
    rows.Add({2});
    rows.Add({3});
    m_1(db, log, functors, std::move(rows));
  }
  dump();
  {
    hyde::rt::Vec<m_input> rows(allocator);
    rows.Add({4});
    rows.Add({2});  // duplicate of an existing row
    m_1(db, log, functors, std::move(rows));
  }
  dump();
  return 0;
}
