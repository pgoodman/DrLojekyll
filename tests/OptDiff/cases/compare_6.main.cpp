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

  auto dump = [&]<typename D>(D &d) {
    {
      std::cout << "q_ne:";
      if constexpr (requires { q_ne_ff(d); }) {
        std::vector<std::pair<int32_t, int32_t>> v;
        auto c = q_ne_ff(d);
        for (int32_t x = 0, y = 0; c.next(x, y);) {
          v.emplace_back(x, y);
        }
        std::sort(v.begin(), v.end());
        for (auto [x, y] : v) {
          std::cout << " (" << x << ',' << y << ')';
        }
      }
      std::cout << '\n';
    }
    {
      std::cout << "q_lt:";
      if constexpr (requires { q_lt_f(d); }) {
        std::vector<int32_t> v;
        auto c = q_lt_f(d);
        for (int32_t x = 0; c.next(x);) {
          v.push_back(x);
        }
        std::sort(v.begin(), v.end());
        for (auto x : v) {
          std::cout << ' ' << x;
        }
      }
      std::cout << '\n';
    }
    {
      std::cout << "q_gt:";
      if constexpr (requires { q_gt_ff(d); }) {
        std::vector<std::pair<int32_t, int32_t>> v;
        auto c = q_gt_ff(d);
        for (int32_t x = 0, y = 0; c.next(x, y);) {
          v.emplace_back(x, y);
        }
        std::sort(v.begin(), v.end());
        for (auto [x, y] : v) {
          std::cout << " (" << x << ',' << y << ')';
        }
      }
      std::cout << '\n';
    }
  };

  auto send = [&](auto member, std::vector<std::pair<int32_t, int32_t>> rows) {
    hyde::rt::Vec<Tup_i32_i32> v(allocator);
    for (auto [x, y] : rows) {
      v.Add({x, y});
    }
    member(db, log, functors, std::move(v));
  };

  send([](auto &&...args) { return in_2(std::forward<decltype(args)>(args)...); }, {{1, 2}, {2, 1}, {20, 3}});
  send([](auto &&...args) { return b_2(std::forward<decltype(args)>(args)...); }, {{15, 4}, {1, 2}});
  dump(db);

  send([](auto &&...args) { return in_2(std::forward<decltype(args)>(args)...); }, {{11, 30}, {1, 5}});
  dump(db);
  return 0;
}
