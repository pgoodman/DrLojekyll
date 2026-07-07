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

  auto dump = [&]<typename D>(D &d, const char *round) {
    std::cout << "-- " << round << '\n';
    std::cout << "foo:";
    if constexpr (requires { d.foo_f(); }) {
      std::vector<int32_t> v;
      auto c = d.foo_f();
      for (int32_t x = 0; c.next(x);) {
        v.push_back(x);
      }
      std::sort(v.begin(), v.end());
      for (auto x : v) {
        std::cout << ' ' << x;
      }
    }
    std::cout << '\n';
  };

  auto send = [&](std::vector<int32_t> rows) {
    hyde::rt::Vec<Tup_i32> v(allocator);
    for (auto x : rows) {
      v.Add({x});
    }
    db.m_1(std::move(v));
  };

  dump(db, "round0");

  send({2, 3});
  dump(db, "round1");

  send({1, 4});
  dump(db, "round2");
  return 0;
}
