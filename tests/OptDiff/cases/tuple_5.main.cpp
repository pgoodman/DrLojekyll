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
    std::vector<std::pair<int32_t, int32_t>> pairs;
    auto c = db.out_ff();
    for (int32_t a = 0, b = 0; c.next(a, b);) {
      pairs.emplace_back(a, b);
    }
    std::sort(pairs.begin(), pairs.end());
    std::cout << "out:";
    for (auto [a, b] : pairs) {
      std::cout << " (" << a << ',' << b << ')';
    }
    std::cout << '\n';
  };

  dump();

  {
    hyde::rt::Vec<input_input> rows(allocator);
    rows.Add({1, 2});
    db.input_2(std::move(rows));
  }
  dump();  // expect (1,2) and (2,1)

  {
    hyde::rt::Vec<input_input> rows(allocator);
    rows.Add({3, 3});
    rows.Add({4, 5});
    db.input_2(std::move(rows));
  }
  dump();  // expect (1,2) (2,1) (3,3) (4,5) (5,4)
  return 0;
}
