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

  auto send = [&](std::vector<int32_t> add, std::vector<int32_t> rem) {
    hyde::rt::Vec<Tup_i32> av(allocator);
    hyde::rt::Vec<Tup_i32> rv(allocator);
    for (auto x : add) {
      av.Add({x});
    }
    for (auto x : rem) {
      rv.Add({x});
    }
    add_1(db, log, functors, std::move(av), std::move(rv));
  };

  send({1, 2, 3}, {});
  dump();

  send({4}, {2});
  dump();

  send({}, {1, 4});
  dump();
  return 0;
}
