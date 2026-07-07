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
      auto c = db.q1_f();
      for (int32_t b = 0; c.next(b);) {
        v.push_back(b);
      }
      std::sort(v.begin(), v.end());
      std::cout << "q1:";
      for (auto b : v) {
        std::cout << ' ' << b;
      }
      std::cout << '\n';
    }
    {
      std::vector<int32_t> v;
      auto c = db.q2_f();
      for (int32_t b = 0; c.next(b);) {
        v.push_back(b);
      }
      std::sort(v.begin(), v.end());
      std::cout << "q2:";
      for (auto b : v) {
        std::cout << ' ' << b;
      }
      std::cout << '\n';
    }
  };

  auto send = [&](std::vector<std::pair<int32_t, int32_t>> rows) {
    hyde::rt::Vec<in_input> vec(allocator);
    for (auto [a, b] : rows) {
      vec.Add({a, b});
    }
    db.in_2(std::move(vec));
  };

  send({{1, 5}, {1, 3}, {2, 3}});
  dump();

  send({{1, 7}, {1, 50}, {3, 2}});
  dump();
  return 0;
}
