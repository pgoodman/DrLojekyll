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
      // Proven unsatisfiable: must stay empty in every mode. The optimizer
      // may drop the entry point entirely.
      std::cout << "q_unsat_eq:";
      if constexpr (requires { q_unsat_eq_f(d); }) {
        std::vector<int> v;
        auto c = q_unsat_eq_f(d);
        for (int x = 0; c.next(x);) {
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
      std::vector<std::pair<int, int>> v;
      auto c = q_ne_true_ff(d);
      for (int x = 0, y = 0; c.next(x, y);) {
        v.emplace_back(x, y);
      }
      std::sort(v.begin(), v.end());
      std::cout << "q_ne_true:";
      for (auto [x, y] : v) {
        std::cout << " (" << x << ',' << y << ')';
      }
      std::cout << '\n';
    }
    {
      std::cout << "q_same_ne:";
      if constexpr (requires { q_same_ne_f(d); }) {
        std::vector<int> v;
        auto c = q_same_ne_f(d);
        for (int x = 0; c.next(x);) {
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

  auto send = [&](std::vector<std::pair<int, int>> rows) {
    hyde::rt::Vec<Tup_int_int> v(allocator);
    for (auto [x, y] : rows) {
      v.Add({x, y});
    }
    tin_2(db, log, functors, std::move(v));
  };

  send({{1, 2}, {2, 1}, {1, 1}, {2, 2}});
  dump(db);

  send({{1, 2}, {3, 1}, {1, 3}, {0, 0}});
  dump(db);
  return 0;
}
