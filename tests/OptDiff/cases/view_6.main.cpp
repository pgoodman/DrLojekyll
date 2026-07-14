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
  auto dump2 = [](const char *name, auto cursor) {
    std::vector<std::pair<int32_t, int32_t>> rows;
    for (int32_t a = 0, b = 0; cursor.next(a, b);) {
      rows.emplace_back(a, b);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << name << ':';
    for (auto &[a, b] : rows) {
      std::cout << " (" << a << ',' << b << ')';
    }
    std::cout << '\n';
  };
  auto dump = [&]() {
    dump2("eq", q_eq_ff(db));
    dump1("ceq", q_ceq_f(db));
    dump1("lt", q_lt_f(db));
    dump2("pairs", pairs_ff(db));
  };

  dump();
  {
    hyde::rt::Vec<e_input> ev(allocator);
    ev.Add({2, 2, 9});
    ev.Add({1, 3, 0});
    ev.Add({4, 3, 5});
    ev.Add({5, 6, 6});
    e_3(db, log, functors, std::move(ev));
    hyde::rt::Vec<add_node_input> nv(allocator);
    nv.Add({1});
    nv.Add({2});
    add_node_1(db, log, functors, std::move(nv));
  }
  dump();
  {
    hyde::rt::Vec<e_input> ev(allocator);
    ev.Add({3, 3, 3});
    e_3(db, log, functors, std::move(ev));
    hyde::rt::Vec<add_node_input> nv(allocator);
    nv.Add({3});
    add_node_1(db, log, functors, std::move(nv));
  }
  dump();
  return 0;
}
