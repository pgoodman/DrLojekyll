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

  auto dump2 = [](const char *name, auto cursor) {
    std::vector<std::pair<int32_t, int32_t>> rows;
    for (int32_t a = 0, b = 0; cursor.next(a, b);) {
      rows.emplace_back(a, b);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << name << ':';
    for (auto [a, b] : rows) {
      std::cout << " (" << a << ',' << b << ')';
    }
    std::cout << '\n';
  };
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
    dump2("swap", db.q_swap_ff());
    dump2("kconst", db.q_kconst_ff());
    dump2("fact", db.q_fact_ff());
    dump1("mix", db.q_mix_f());
  };

  dump();  // Just the fact clause: fact: (1,2)
  {
    hyde::rt::Vec<t1_input> rows(allocator);
    rows.Add({1, 10});
    rows.Add({2, 20});
    db.t1_2(std::move(rows));
  }
  {
    hyde::rt::Vec<t2_input> rows(allocator);
    rows.Add({3, 30});
    db.t2_2(std::move(rows));
  }
  dump();
  {
    hyde::rt::Vec<t1_input> rows(allocator);
    rows.Add({4, 40});
    db.t1_2(std::move(rows));
  }
  {
    hyde::rt::Vec<t2_input> rows(allocator);
    rows.Add({1, 10});  // same tuple through the other union operand
    db.t2_2(std::move(rows));
  }
  dump();
  return 0;
}
