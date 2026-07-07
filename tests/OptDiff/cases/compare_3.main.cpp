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
    {
      std::vector<int32_t> v;
      auto c = db.q_lt_f();
      for (int32_t x = 0; c.next(x);) {
        v.push_back(x);
      }
      std::sort(v.begin(), v.end());
      std::cout << "q_lt:";
      for (auto x : v) {
        std::cout << ' ' << x;
      }
      std::cout << '\n';
    }
    {
      std::vector<std::pair<int32_t, int32_t>> v;
      auto c = db.q_gt_ff();
      for (int32_t x = 0, y = 0; c.next(x, y);) {
        v.emplace_back(x, y);
      }
      std::sort(v.begin(), v.end());
      std::cout << "q_gt:";
      for (auto [x, y] : v) {
        std::cout << " (" << x << ',' << y << ')';
      }
      std::cout << '\n';
    }
  };

  auto send = [&](auto member, std::vector<std::pair<int32_t, int32_t>> rows) {
    hyde::rt::Vec<Tup_i32_i32> v(allocator);
    for (auto [x, y] : rows) {
      v.Add({x, y});
    }
    (db.*member)(std::move(v));
  };

  send(&Database::a_2, {{1, 2}, {5, 3}, {20, 1}});
  send(&Database::b_2, {{2, 4}, {9, 9}, {11, 50}});
  dump();

  send(&Database::a_2, {{12, 12}, {0, 1}});
  send(&Database::b_2, {{30, 2}, {1, 2}});
  dump();
  return 0;
}
