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
    dump2("dead", db.q_dead_ff());
    dump2("live", db.q_live_ff());
  };

  dump();
  {
    hyde::rt::Vec<m_input> rows(allocator);
    rows.Add({1, 2});
    rows.Add({3, 4});
    db.m_2(std::move(rows));
  }
  dump();
  {
    hyde::rt::Vec<m_input> rows(allocator);
    rows.Add({5, 6});
    rows.Add({1, 2});  // duplicate of an existing row
    db.m_2(std::move(rows));
  }
  dump();
  return 0;
}
