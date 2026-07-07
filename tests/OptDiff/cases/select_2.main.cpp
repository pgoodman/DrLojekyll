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
    std::vector<int32_t> xs;
    auto c = db.get_u_f();
    for (int32_t x = 0; c.next(x);) {
      xs.push_back(x);
    }
    std::sort(xs.begin(), xs.end());
    std::cout << "get_u:";
    for (auto x : xs) {
      std::cout << ' ' << x;
    }
    std::cout << '\n';
  };

  dump();

  {
    hyde::rt::Vec<a_input> vec(allocator);
    vec.Add({1});
    vec.Add({2});
    db.a_1(std::move(vec));
  }
  dump();

  {
    hyde::rt::Vec<b_input> vec(allocator);
    vec.Add({2});
    vec.Add({3});
    db.b_1(std::move(vec));
  }
  dump();
  return 0;
}
