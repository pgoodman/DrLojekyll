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
    dump2("eq", db.q_eq_ff());
    dump1("ceq", db.q_ceq_f());
    dump1("lt", db.q_lt_f());
    dump2("pairs", db.pairs_ff());
  };

  dump();
  {
    hyde::rt::Vec<e_input> ev(allocator);
    ev.Add({2, 2, 9});
    ev.Add({1, 3, 0});
    ev.Add({4, 3, 5});
    ev.Add({5, 6, 6});
    db.e_3(std::move(ev));
    hyde::rt::Vec<add_node_input> nv(allocator);
    nv.Add({1});
    nv.Add({2});
    db.add_node_1(std::move(nv));
  }
  dump();
  {
    hyde::rt::Vec<e_input> ev(allocator);
    ev.Add({3, 3, 3});
    db.e_3(std::move(ev));
    hyde::rt::Vec<add_node_input> nv(allocator);
    nv.Add({3});
    db.add_node_1(std::move(nv));
  }
  dump();
  return 0;
}
