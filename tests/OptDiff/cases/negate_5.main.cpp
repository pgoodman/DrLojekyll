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

  auto dump = [&db]() {
    std::vector<int32_t> vals;
    auto c = db.out_f();
    for (int32_t v = 0; c.next(v);) {
      vals.push_back(v);
    }
    std::sort(vals.begin(), vals.end());
    std::cout << "out:";
    for (auto v : vals) {
      std::cout << ' ' << v;
    }
    std::cout << '\n';
  };

  dump();

  {
    hyde::rt::Vec<feed_input> rows(allocator);
    rows.Add({1, 10});
    rows.Add({1, 11});
    rows.Add({2, 20});
    db.feed_2(std::move(rows));
  }
  dump();

  {
    hyde::rt::Vec<unsee_input> rows(allocator);
    rows.Add({1});
    db.unsee_1(std::move(rows));
  }
  dump();

  {
    hyde::rt::Vec<feed_input> rows(allocator);
    rows.Add({3, 30});
    db.feed_2(std::move(rows));
  }
  dump();
  return 0;
}
