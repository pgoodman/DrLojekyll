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
    std::vector<std::pair<int32_t, int32_t>> o;
    {
      auto c = out_ff(db);
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
    in_2(db, log, functors, std::move(vec));
  }
  dump();
  {
    hyde::rt::Vec<in_input> vec(allocator);
    vec.Add({5, 5});
    in_2(db, log, functors, std::move(vec));
  }
  dump();
  return 0;
}
