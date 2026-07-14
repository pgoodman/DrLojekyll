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
    std::vector<std::pair<uint64_t, uint64_t>> rows;
    auto c = out_ff(db);
    for (uint64_t a = 0, b = 0; c.next(a, b);) {
      rows.emplace_back(a, b);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << "out:";
    for (auto &[a, b] : rows) {
      std::cout << " (" << a << ',' << b << ')';
    }
    std::cout << '\n';

    std::vector<std::pair<int32_t, int32_t>> e1;
    auto c2 = entry1_ff(db);
    for (int32_t a = 0, b = 0; c2.next(a, b);) {
      e1.emplace_back(a, b);
    }
    std::sort(e1.begin(), e1.end());
    std::cout << "entry1:";
    for (auto &[a, b] : e1) {
      std::cout << " (" << a << ',' << b << ')';
    }
    std::cout << '\n';
  };

  {
    hyde::rt::Vec<add_input> rows(allocator);
    rows.Add({1, 2});
    rows.Add({3, 3});
    add_2(db, log, functors, std::move(rows));
  }
  {
    hyde::rt::Vec<add_entry_input> rows(allocator);
    rows.Add({1, 2, 3});   // passes both B < C and A < B
    rows.Add({5, 4, 9});   // passes B < C, fails A < B
    rows.Add({1, 5, 2});   // fails B < C
    add_entry_3(db, log, functors, std::move(rows));
  }
  dump();

  {
    hyde::rt::Vec<add_entry2_input> rows(allocator);
    rows.Add({7});
    add_entry2_1(db, log, functors, std::move(rows));
  }
  {
    hyde::rt::Vec<add_input> rows(allocator);
    rows.Add({10, 20});
    add_2(db, log, functors, std::move(rows));
  }
  dump();
  return 0;
}
