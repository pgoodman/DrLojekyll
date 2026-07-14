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

  auto dump = [&]<typename D>(D &d, const char *round) {
    std::cout << "-- " << round << '\n';

    // Proven empty in optimized builds; the entry point may be dropped.
    std::cout << "p:";
    if constexpr (requires { p_f(d); }) {
      std::vector<int32_t> v;
      auto c = p_f(d);
      for (int32_t x = 0; c.next(x);) {
        v.push_back(x);
      }
      std::sort(v.begin(), v.end());
      for (auto x : v) {
        std::cout << ' ' << x;
      }
    }
    std::cout << '\n';

    std::vector<std::pair<int32_t, int32_t>> v;
    auto c = all_bar_ff(d);
    for (int32_t a = 0, b = 0; c.next(a, b);) {
      v.emplace_back(a, b);
    }
    std::sort(v.begin(), v.end());
    std::cout << "all_bar:";
    for (auto [a, b] : v) {
      std::cout << " (" << a << ',' << b << ')';
    }
    std::cout << '\n';
  };

  auto send = [&](std::vector<int32_t> rows) {
    hyde::rt::Vec<Tup_i32> v(allocator);
    for (auto x : rows) {
      v.Add({x});
    }
    m_1(db, log, functors, std::move(v));
  };

  dump(db, "round0");

  send({5, 1});
  dump(db, "round1");

  send({7, 2});
  dump(db, "round2");
  return 0;
}
