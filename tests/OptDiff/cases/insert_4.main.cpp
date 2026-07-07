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

  auto dump1 = [](const char *name, auto cursor) {
    std::vector<int32_t> vals;
    for (int32_t v = 0; cursor.next(v);) {
      vals.push_back(v);
    }
    std::sort(vals.begin(), vals.end());
    std::cout << name << ':';
    for (auto v : vals) {
      std::cout << ' ' << v;
    }
    std::cout << '\n';
  };
  auto dump = [&]() {
    dump1("on", db.on_vals_f());
    dump1("off", db.off_vals_f());
  };

  dump();
  {
    hyde::rt::Vec<m_input> rows(allocator);
    rows.Add({10});
    rows.Add({20});
    db.m_1(std::move(rows));
  }
  dump();  // is_on still false: on empty, off has 10 20
  {
    hyde::rt::Vec<log_in_input> rows(allocator);
    rows.Add({7});
    db.log_in_1(std::move(rows));
  }
  dump();  // condition flipped: on has 10 20, off empty
  {
    hyde::rt::Vec<m_input> rows(allocator);
    rows.Add({30});
    db.m_1(std::move(rows));
  }
  dump();
  return 0;
}
