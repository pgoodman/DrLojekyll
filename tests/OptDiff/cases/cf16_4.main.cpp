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
    std::vector<int32_t> v;
    auto c = observed_f(db);
    for (int32_t a = 0; c.next(a);) {
      v.push_back(a);
    }
    std::sort(v.begin(), v.end());
    std::cout << "observed:";
    for (auto a : v) {
      std::cout << ' ' << a;
    }
    std::cout << '\n';
  };

  auto send = [&](std::vector<int32_t> vals) {
    hyde::rt::Vec<Tup_i32> vec(allocator);
    for (auto a : vals) {
      vec.Add({a});
    }
    std::cout << "in rc=" << in_val_1(db, log, functors, std::move(vec)) << '\n';
  };

  dump();
  send({7, 8});
  dump();
  send({8, 9});  // one duplicate, one new
  dump();
  return 0;
}
