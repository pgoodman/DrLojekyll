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

  auto dump_all = [&](const char *round) {
    std::cout << "-- " << round << '\n';
    dump1("out_t", db.out_t_f());
    dump1("out_u", db.out_u_f());
    dump1("out_v", db.out_v_f());
  };

  dump_all("round0");

  {
    hyde::rt::Vec<m_input> rows(allocator);
    rows.Add({2});
    rows.Add({4});
    db.m_1(std::move(rows));
  }
  dump_all("round1");

  {
    hyde::rt::Vec<n_input> rows(allocator);
    rows.Add({30});
    rows.Add({4});
    db.n_1(std::move(rows));
  }
  {
    hyde::rt::Vec<m_input> rows(allocator);
    rows.Add({5});
    db.m_1(std::move(rows));
  }
  dump_all("round2");
  return 0;
}
