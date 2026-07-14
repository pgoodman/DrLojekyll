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

  auto dump = [&db]() {
    std::vector<std::pair<int32_t, int32_t>> outer;
    auto oc = q_outer_ff(db);
    for (int32_t x = 0, y = 0; oc.next(x, y);) {
      outer.emplace_back(x, y);
    }
    std::sort(outer.begin(), outer.end());
    std::cout << "outer:";
    for (auto [x, y] : outer) {
      std::cout << " (" << x << ',' << y << ')';
    }
    std::cout << '\n';

    std::vector<int32_t> proj;
    auto pc = q_proj_f(db);
    for (int32_t x = 0; pc.next(x);) {
      proj.push_back(x);
    }
    std::sort(proj.begin(), proj.end());
    std::cout << "proj:";
    for (auto x : proj) {
      std::cout << ' ' << x;
    }
    std::cout << '\n';
  };

  dump();
  {
    hyde::rt::Vec<m1_input> rows(allocator);
    rows.Add({1, 10});
    rows.Add({2, 20});
    m1_2(db, log, functors, std::move(rows));
  }
  {
    hyde::rt::Vec<m2_input> rows(allocator);
    rows.Add({3, 30});
    m2_2(db, log, functors, std::move(rows));
  }
  dump();
  {
    hyde::rt::Vec<m3_input> rows(allocator);
    rows.Add({4, 40});
    rows.Add({1, 10});  // same tuple as an m1 row, via the other operand
    m3_2(db, log, functors, std::move(rows));
  }
  {
    hyde::rt::Vec<m2_input> rows(allocator);
    rows.Add({20, 2});  // proj sees this reversed: proj(2, 20)
    m2_2(db, log, functors, std::move(rows));
  }
  dump();
  return 0;
}
