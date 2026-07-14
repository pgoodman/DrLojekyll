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
  Database db(allocator);
  init(db, log, functors);

  auto dump = [&db]() {
    std::vector<std::pair<int32_t, int32_t>> rows;
    auto c = q_ff(db);
    for (int32_t x = 0, y = 0; c.next(x, y);) {
      rows.emplace_back(x, y);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << "q:";
    for (auto [x, y] : rows) {
      std::cout << " (" << x << ',' << y << ')';
    }
    std::cout << '\n';
  };

  dump();

  {
    hyde::rt::Vec<m_input> vec(allocator);
    vec.Add({2});
    vec.Add({3});
    m_1(db, log, functors, std::move(vec));
  }
  dump();

  {
    hyde::rt::Vec<m_input> vec(allocator);
    vec.Add({1});
    m_1(db, log, functors, std::move(vec));
  }
  dump();
  return 0;
}
