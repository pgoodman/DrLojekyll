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

    std::vector<std::pair<int32_t, int32_t>> v;
    auto c = all_reach_ff(db);
    for (int32_t x = 0, y = 0; c.next(x, y);) {
      v.emplace_back(x, y);
    }
    std::sort(v.begin(), v.end());
    std::cout << "all_reach:";
    for (auto [x, y] : v) {
      std::cout << " (" << x << ',' << y << ')';
    }
    std::cout << '\n';

    std::cout << "is_reach:";
    for (int32_t x = 1; x <= 5; ++x) {
      for (int32_t y = 1; y <= 5; ++y) {
        std::cout << (is_reach_bb(db, x, y) ? '1' : '0');
      }
      std::cout << ' ';
    }
    std::cout << '\n';
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
    add_edge_2(db, log, functors, std::move(av), std::move(rv));
  };

  dump("round0");

  send({{1, 2}, {2, 3}, {3, 4}}, {});
  dump("round1");

  send({{5, 1}}, {{2, 3}});
  dump("round2");

  send({}, {{1, 2}});
  dump("round3");
  return 0;
}
