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

  auto dump = [&]<typename D>(D &d) {
    {
      std::vector<int32_t> v;
      auto c = d.q_eq_f();
      for (int32_t x = 0; c.next(x);) {
        v.push_back(x);
      }
      std::sort(v.begin(), v.end());
      std::cout << "q_eq:";
      for (auto x : v) {
        std::cout << ' ' << x;
      }
      std::cout << '\n';
    }
    {
      // The optimizer proves q_lt unsatisfiable and drops its entry point;
      // an empty result is the expected output in every mode.
      std::cout << "q_lt:";
      if constexpr (requires { d.q_lt_f(); }) {
        std::vector<int32_t> v;
        auto c = d.q_lt_f();
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
  };

  auto send = [&](std::vector<std::pair<int32_t, int32_t>> add,
                  std::vector<std::pair<int32_t, int32_t>> rem) {
    hyde::rt::Vec<Tup_i32_i32> av(allocator);
    hyde::rt::Vec<Tup_i32_i32> rv(allocator);
    for (auto [x, y] : add) {
      av.Add({x, y});
    }
    for (auto [x, y] : rem) {
      rv.Add({x, y});
    }
    db.add_pair_2(std::move(av), std::move(rv));
  };

  send({{1, 1}, {1, 2}, {3, 3}, {4, 5}}, {});
  dump(db);

  send({{5, 5}, {2, 2}}, {{1, 1}, {4, 5}});
  dump(db);
  return 0;
}
