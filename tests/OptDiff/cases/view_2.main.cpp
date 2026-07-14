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
    dump2("dup", q_dup_ff(db));
    dump2("const", q_const_ff(db));
    dump1("unused", q_unused_f(db));
  };

  dump();
  {
    hyde::rt::Vec<in_input> rows(allocator);
    rows.Add({0});
    rows.Add({2});
    rows.Add({5});
    in_1(db, log, functors, std::move(rows));
  }
  dump();
  {
    hyde::rt::Vec<in_input> rows(allocator);
    rows.Add({7});
    rows.Add({-1});
    rows.Add({2});  // duplicate
    in_1(db, log, functors, std::move(rows));
  }
  dump();
  return 0;
}
