#include <algorithm>
#include <array>
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
      std::vector<std::pair<int32_t, int32_t>> v;
      auto c = db.q_c_ff();
      for (int32_t x = 0, y = 0; c.next(x, y);) {
        v.emplace_back(x, y);
      }
      std::sort(v.begin(), v.end());
      std::cout << "q_c:";
      for (auto [x, y] : v) {
        std::cout << " (" << x << ',' << y << ')';
      }
      std::cout << '\n';
    }
    {
      std::vector<int32_t> v;
      auto c = db.q_u_f();
      for (int32_t x = 0; c.next(x);) {
        v.push_back(x);
      }
      std::sort(v.begin(), v.end());
      std::cout << "q_u:";
      for (auto x : v) {
        std::cout << ' ' << x;
      }
      std::cout << '\n';
    }
  };

  auto send2 = [&](std::vector<std::pair<int32_t, int32_t>> rows) {
    hyde::rt::Vec<Tup_i32_i32> v(allocator);
    for (auto [x, y] : rows) {
      v.Add({x, y});
    }
    db.in_2(std::move(v));
  };
  auto send3 = [&](std::vector<std::array<int32_t, 3>> rows) {
    hyde::rt::Vec<Tup_i32_i32_i32> v(allocator);
    for (auto [x, y, z] : rows) {
      v.Add({x, y, z});
    }
    db.in3_3(std::move(v));
  };

  send2({{1, 2}, {2, 1}, {1, 5}, {3, 4}});
  send3({{1, 2, 9}, {5, 3, 8}, {2, 10, 7}});
  dump();

  send2({{0, 9}, {7, 7}, {1, 2}});
  send3({{1, 2, 100}, {6, 8, 0}});
  dump();
  return 0;
}
