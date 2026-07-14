#include <algorithm>
#include <cstdint>
#include <iostream>
#include <tuple>
#include <vector>

#include "datalog.h"

int32_t DatabaseFunctors::add_i32_bbf(int32_t l, int32_t r) {
  return l + r;
}

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  auto dump2 = [](const char *name, auto cursor) {
    std::vector<std::tuple<int32_t, int32_t>> vals;
    for (int32_t a = 0, b = 0; cursor.next(a, b);) {
      vals.emplace_back(a, b);
    }
    std::sort(vals.begin(), vals.end());
    std::cout << name << ':';
    for (auto [a, b] : vals) {
      std::cout << " (" << a << ',' << b << ')';
    }
    std::cout << '\n';
  };

  auto dump3 = [](const char *name, auto cursor) {
    std::vector<std::tuple<int32_t, int32_t, int32_t>> vals;
    for (int32_t a = 0, b = 0, c = 0; cursor.next(a, b, c);) {
      vals.emplace_back(a, b, c);
    }
    std::sort(vals.begin(), vals.end());
    std::cout << name << ':';
    for (auto [a, b, c] : vals) {
      std::cout << " (" << a << ',' << b << ',' << c << ')';
    }
    std::cout << '\n';
  };

  auto dump_all = [&](const char *round) {
    std::cout << "-- " << round << '\n';
    dump2("dup_bound", dup_bound_ff(db));
    dump3("dup_att", dup_att_fff(db));
    dump2("drop_att", drop_att_ff(db));
  };

  dump_all("round0");

  {
    hyde::rt::Vec<m_input> rows(allocator);
    rows.Add({2, 5});
    rows.Add({3, 7});
    m_2(db, log, functors, std::move(rows));
  }
  dump_all("round1");

  {
    hyde::rt::Vec<m_input> rows(allocator);
    rows.Add({10, 5});
    rows.Add({2, 9});
    m_2(db, log, functors, std::move(rows));
  }
  dump_all("round2");
  return 0;
}
