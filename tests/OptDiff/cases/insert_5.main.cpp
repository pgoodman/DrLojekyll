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
  auto dump2 = [](const char *name, auto cursor) {
    std::vector<std::pair<int32_t, int32_t>> rows;
    for (int32_t a = 0, b = 0; cursor.next(a, b);) {
      rows.emplace_back(a, b);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << name << ':';
    for (auto [a, b] : rows) {
      std::cout << " (" << a << ',' << b << ')';
    }
    std::cout << '\n';
  };
  auto dump = [&]() {
    dump2("const", db.q_const_ff());
    dump1("join", db.q_join_f());
  };

  dump();
  {
    hyde::rt::Vec<m_input> rows(allocator);
    rows.Add({1});
    rows.Add({2});
    db.m_1(std::move(rows));
  }
  dump();
  {
    hyde::rt::Vec<m_input> rows(allocator);
    rows.Add({3});
    rows.Add({1});  // duplicate of an existing row
    db.m_1(std::move(rows));
  }
  dump();
  return 0;
}
