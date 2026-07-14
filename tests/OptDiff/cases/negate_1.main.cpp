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

  auto dump = [&db]() {
    std::vector<std::pair<int32_t, int32_t>> rows;
    auto c = out_ff(db);
    for (int32_t a = 0, b = 0; c.next(a, b);) {
      rows.emplace_back(a, b);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << "out:";
    for (auto &[a, b] : rows) {
      std::cout << " (" << a << ',' << b << ')';
    }
    std::cout << '\n';
  };

  dump();

  {
    hyde::rt::Vec<feed_input> rows(allocator);
    rows.Add({1, 10});
    rows.Add({2, 20});
    rows.Add({3, 30});
    feed_2(db, log, functors, std::move(rows));
  }
  dump();

  {
    hyde::rt::Vec<unsee_input> rows(allocator);
    rows.Add({2});
    unsee_1(db, log, functors, std::move(rows));
  }
  dump();

  {
    hyde::rt::Vec<feed_input> rows(allocator);
    rows.Add({2, 25});
    rows.Add({4, 40});
    feed_2(db, log, functors, std::move(rows));
  }
  dump();
  return 0;
}
