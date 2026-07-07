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
    std::vector<std::pair<int32_t, int32_t>> o;
    {
      auto c = db.out_ff();
      for (int32_t a = 0, b = 0; c.next(a, b);) {
        o.emplace_back(a, b);
      }
    }
    std::sort(o.begin(), o.end());
    std::cout << "out:";
    for (auto [a, b] : o) {
      std::cout << " (" << a << ',' << b << ')';
    }
    std::cout << '\n';
  };

  dump();
  {
    hyde::rt::Vec<in_input> vec(allocator);
    vec.Add({1, 2});
    vec.Add({3, 4});
    db.in_2(std::move(vec));
  }
  dump();
  {
    hyde::rt::Vec<in_input> vec(allocator);
    vec.Add({5, 5});
    db.in_2(std::move(vec));
  }
  dump();
  return 0;
}
