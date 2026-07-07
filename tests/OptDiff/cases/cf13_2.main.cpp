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
    std::vector<int32_t> v;
    auto c = db.q_f();
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
    db.in_1(std::move(vec));
  };

  send({1, 2, 150});
  dump();

  send({50, 200});
  dump();
  return 0;
}
