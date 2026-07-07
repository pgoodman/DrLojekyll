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
    {
      std::vector<int32_t> vals;
      auto c = db.out_all_f();
      for (int32_t v = 0; c.next(v);) {
        vals.push_back(v);
      }
      std::sort(vals.begin(), vals.end());
      std::cout << "out_all:";
      for (auto v : vals) {
        std::cout << ' ' << v;
      }
      std::cout << '\n';
    }
    {
      std::vector<int32_t> vals;
      auto c = db.out_dead_f();
      for (int32_t v = 0; c.next(v);) {
        vals.push_back(v);
      }
      std::sort(vals.begin(), vals.end());
      std::cout << "out_dead:";
      for (auto v : vals) {
        std::cout << ' ' << v;
      }
      std::cout << '\n';
    }
  };

  dump();

  {
    hyde::rt::Vec<feed_input> rows(allocator);
    rows.Add({5});
    rows.Add({9});
    db.feed_1(std::move(rows));
  }
  dump();

  {
    hyde::rt::Vec<feed_input> rows(allocator);
    rows.Add({1});
    rows.Add({2});
    db.feed_1(std::move(rows));
  }
  dump();
  return 0;
}
