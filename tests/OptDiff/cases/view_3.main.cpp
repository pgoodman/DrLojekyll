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
    dump1("proof", db.proof_f());
    dump1("shared", db.q_shared_f());
    dump2("fact", db.q_fact_ff());
  };

  dump();  // nothing proven yet except the constant fact
  {
    hyde::rt::Vec<something_input> rows(allocator);
    rows.Add({42});
    db.something_1(std::move(rows));
  }
  dump();  // proof: 1
  {
    hyde::rt::Vec<a_input> rows(allocator);
    rows.Add({9});
    db.a_1(std::move(rows));
  }
  dump();  // shared: 1 via first setter
  {
    hyde::rt::Vec<b_input> rows(allocator);
    rows.Add({10});
    db.b_1(std::move(rows));
  }
  dump();  // shared still: 1 (second setter also true)
  return 0;
}
