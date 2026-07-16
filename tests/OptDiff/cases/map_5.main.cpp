#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#include "datalog.h"

int32_t add_i32_bbf(int32_t l, int32_t r) {
  return l + r;
}

bool is_even_b(int32_t x) {
  return (x % 2) == 0;
}

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

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

  auto dump_all = [&](const char *round) {
    std::cout << "-- " << round << '\n';
    dump1("evens", evens_f(db));
    dump1("odds", odds_f(db));
    dump1("merged", merged_f(db));
  };

  dump_all("round0");

  {
    hyde::rt::Vec<m_input> rows(allocator);
    rows.Add({2});
    rows.Add({3});
    rows.Add({4});
    m_1(db, log, functors, std::move(rows));
  }
  dump_all("round1");

  {
    hyde::rt::Vec<n_input> rows(allocator);
    rows.Add({9});
    rows.Add({100});
    n_1(db, log, functors, std::move(rows));
  }
  dump_all("round2");
  return 0;
}
