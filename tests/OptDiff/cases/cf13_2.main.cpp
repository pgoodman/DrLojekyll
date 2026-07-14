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
    auto c = q_f(db);
    for (int32_t a = 0; c.next(a);) {
      v.push_back(a);
    }
    std::sort(v.begin(), v.end());
    std::cout << "q:";
    for (auto a : v) {
      std::cout << ' ' << a;
    }
    std::cout << '\n';
  };

  auto send = [&](std::vector<int32_t> rows) {
    hyde::rt::Vec<in_input> vec(allocator);
    for (auto x : rows) {
      vec.Add({x});
    }
    in_1(db, log, functors, std::move(vec));
  };

  send({1, 2, 150});
  dump();

  send({50, 200});
  dump();
  return 0;
}
