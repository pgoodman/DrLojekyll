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

  auto dump = [&db, &dump1]() {
    dump1("qx", db.qx_f());
    dump1("qy", db.qy_f());
  };

  dump();

  {
    hyde::rt::Vec<feed_input> rows(allocator);
    rows.Add({1});
    rows.Add({2});
    db.feed_1(std::move(rows));
  }
  dump();

  {
    hyde::rt::Vec<unsee_input> rows(allocator);
    rows.Add({2});
    db.unsee_1(std::move(rows));
  }
  dump();

  {
    hyde::rt::Vec<feed_input> rows(allocator);
    rows.Add({3});
    db.feed_1(std::move(rows));
  }
  dump();
  return 0;
}
