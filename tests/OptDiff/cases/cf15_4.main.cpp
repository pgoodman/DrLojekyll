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

  auto dump = [&](const char *round) {
    std::cout << "-- " << round << '\n';

    // Probe the tuple finder over a grid; folding the union arm's constant
    // 1 into the bound B would flip some of these answers.
    std::cout << "entry_1:";
    for (int32_t a = 0; a <= 6; ++a) {
      for (int32_t b = 0; b <= 3; ++b) {
        if (entry_1_bb(db, a, b)) {
          std::cout << " (" << a << ',' << b << ')';
        }
      }
    }
    std::cout << '\n';

    std::vector<std::pair<int32_t, int32_t>> v;
    auto c = get_p_ff(db);
    for (int32_t a = 0, b = 0; c.next(a, b);) {
      v.emplace_back(a, b);
    }
    std::sort(v.begin(), v.end());
    std::cout << "get_p:";
    for (auto [a, b] : v) {
      std::cout << " (" << a << ',' << b << ')';
    }
    std::cout << '\n';
  };

  auto send_pairs = [&](std::vector<std::pair<int32_t, int32_t>> rows) {
    hyde::rt::Vec<Tup_i32_i32> v(allocator);
    for (auto [a, b] : rows) {
      v.Add({a, b});
    }
    add_entry_2(db, log, functors, std::move(v));
  };
  auto send_singles = [&](std::vector<int32_t> rows) {
    hyde::rt::Vec<Tup_i32> v(allocator);
    for (auto x : rows) {
      v.Add({x});
    }
    add_entry2_1(db, log, functors, std::move(v));
  };

  dump("round0");

  send_pairs({{1, 2}, {2, 1}, {3, 3}, {4, 4}});
  send_singles({5});
  dump("round1");

  send_pairs({{0, 3}, {6, 2}});
  send_singles({6, 1});
  dump("round2");
  return 0;
}
