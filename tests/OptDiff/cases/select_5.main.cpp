#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator, log, functors);

  auto dump = [&db]() {
    for (int32_t a = 1; a <= 5; ++a) {
      std::vector<int32_t> bs;
      auto c = db.reach_bf(a);
      for (int32_t b = 0; c.next(b);) {
        bs.push_back(b);
      }
      std::sort(bs.begin(), bs.end());
      std::cout << "reach " << a << ':';
      for (auto b : bs) {
        std::cout << ' ' << b;
      }
      std::cout << '\n';
    }
    std::cout << "--\n";
  };

  dump();

  {
    hyde::rt::Vec<base_input> vec(allocator);
    vec.Add({1, 2});
    vec.Add({2, 3});
    vec.Add({4, 5});
    db.base_2(std::move(vec));
  }
  dump();

  {
    hyde::rt::Vec<base_input> vec(allocator);
    vec.Add({3, 4});  // connects the two chains: 1 now reaches 5
    db.base_2(std::move(vec));
  }
  dump();
  return 0;
}
