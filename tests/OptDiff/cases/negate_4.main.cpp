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
    for (auto &[a, b] : rows) {
      std::cout << " (" << a << ',' << b << ')';
    }
    std::cout << '\n';
  };

  auto dump = [&db, &dump2]() {
    dump2("qres", db.qres_ff());
    dump2("qres2", db.qres2_ff());
    dump2("qres3", db.qres3_ff());
  };

  dump();

  {
    hyde::rt::Vec<feed_input> rows(allocator);
    rows.Add({1, 10});
    rows.Add({2, 20});
    db.feed_2(std::move(rows));
  }
  {
    hyde::rt::Vec<other_input> rows(allocator);
    rows.Add({3, 30});
    db.other_2(std::move(rows));
  }
  dump();

  {
    hyde::rt::Vec<unsee_input> rows(allocator);
    rows.Add({1});
    db.unsee_1(std::move(rows));
  }
  {
    hyde::rt::Vec<other_input> rows(allocator);
    rows.Add({1, 99});
    db.other_2(std::move(rows));
  }
  dump();
  return 0;
}
