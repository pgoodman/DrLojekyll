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

  auto dump = [&db, &dump1]() {
    dump1("out7", out7_f(db));
    dump1("outdup", outdup_f(db));
    dump1("outflag", outflag_f(db));
  };

  dump();

  {
    hyde::rt::Vec<feed_input> rows(allocator);
    rows.Add({1});
    rows.Add({2});
    rows.Add({3});
    feed_1(db, log, functors, std::move(rows));
  }
  {
    hyde::rt::Vec<pin_input> rows(allocator);
    rows.Add({3, 7});
    rows.Add({2, 2});
    pin_2(db, log, functors, std::move(rows));
  }
  dump();

  {
    hyde::rt::Vec<pin_input> rows(allocator);
    rows.Add({1, 7});
    pin_2(db, log, functors, std::move(rows));
  }
  {
    hyde::rt::Vec<feed_input> rows(allocator);
    rows.Add({4});
    feed_1(db, log, functors, std::move(rows));
  }
  dump();
  return 0;
}
