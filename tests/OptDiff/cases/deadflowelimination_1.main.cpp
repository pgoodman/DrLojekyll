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
    std::vector<int32_t> d, g;
    {
      auto c = db.out_d_f();
      for (int32_t v = 0; c.next(v);) {
        d.push_back(v);
      }
    }
    {
      auto c = db.out_ghost_f();
      for (int32_t v = 0; c.next(v);) {
        g.push_back(v);
      }
    }
    std::sort(d.begin(), d.end());
    std::sort(g.begin(), g.end());
    std::cout << "out_d:";
    for (auto v : d) {
      std::cout << ' ' << v;
    }
    std::cout << "\nout_ghost:";
    for (auto v : g) {
      std::cout << ' ' << v;
    }
    std::cout << '\n';
  };

  dump();
  {
    hyde::rt::Vec<in_input> vec(allocator);
    vec.Add({1});
    vec.Add({2});
    db.in_1(std::move(vec));
  }
  dump();
  {
    hyde::rt::Vec<in_input> vec(allocator);
    vec.Add({3});
    db.in_1(std::move(vec));
  }
  dump();
  return 0;
}
