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

  auto dump = [&db]() {
    std::vector<std::pair<int32_t, int32_t>> rows;
    auto c = db.q_ff();
    for (int32_t a = 0, b = 0; c.next(a, b);) {
      rows.emplace_back(a, b);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << "q:";
    for (auto &[a, b] : rows) {
      std::cout << " (" << a << ',' << b << ')';
    }
    std::cout << '\n';
  };

  auto send = [&](std::vector<std::pair<int32_t, int32_t>> rows) {
    hyde::rt::Vec<edge_input> vec(allocator);
    for (auto [a, b] : rows) {
      vec.Add({a, b});
    }
    db.edge_2(std::move(vec));
  };

  send({{1, 2}, {2, 3}});
  dump();

  send({{3, 4}, {5, 1}});
  dump();
  return 0;
}
