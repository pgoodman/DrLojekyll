#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  auto dump = [&db]() {
    std::vector<int32_t> ys;
    auto c = is_on_f(db);
    for (int32_t y = 0; c.next(y);) {
      ys.push_back(y);
    }
    std::sort(ys.begin(), ys.end());
    std::cout << "is_on:";
    for (auto y : ys) {
      std::cout << ' ' << y;
    }
    std::cout << '\n';
  };

  dump();

  {
    hyde::rt::Vec<ping_input> vec(allocator);
    vec.Add({1});
    vec.Add({2});
    ping_1(db, log, functors, std::move(vec));
  }
  dump();  // condition not yet set: expect empty

  {
    hyde::rt::Vec<enable_input> vec(allocator);
    vec.Add({7u});
    enable_1(db, log, functors, std::move(vec));
  }
  dump();  // condition now set

  {
    hyde::rt::Vec<ping_input> vec(allocator);
    vec.Add({3});
    ping_1(db, log, functors, std::move(vec));
  }
  dump();
  return 0;
}
