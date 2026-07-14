#include <algorithm>
#include <cstdint>
#include <iostream>
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
    dump1("const_out", const_out_f(db));
    dump1("cond_out", cond_out_f(db));
  };

  // const_out must already contain 3; cond_out must stay empty until an
  // m(3) fact arrives (the MAP's condition on its dropped predecessor).
  dump_all("round0");

  {
    hyde::rt::Vec<m_input> rows(allocator);
    rows.Add({1});
    rows.Add({2});
    m_1(db, log, functors, std::move(rows));
  }
  dump_all("round1");

  {
    hyde::rt::Vec<m_input> rows(allocator);
    rows.Add({3});
    m_1(db, log, functors, std::move(rows));
  }
  dump_all("round2");
  return 0;
}
