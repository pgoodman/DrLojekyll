#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#include "datalog.h"

int32_t DatabaseFunctors::add_i32_bbf(int32_t l, int32_t r) {
  return l + r;
}

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator, log, functors);

  auto dump = [&](const char *round) {
    std::cout << "-- " << round << '\n';

    std::cout << "getq:";
    for (int32_t a : {1, 5, 7}) {
      std::vector<int32_t> v;
      auto c = db.getq_bf(a);
      for (int32_t s = 0; c.next(s);) {
        v.push_back(s);
      }
      std::sort(v.begin(), v.end());
      std::cout << " [" << a << ':';
      for (auto s : v) {
        std::cout << ' ' << s;
      }
      std::cout << ']';
    }
    std::cout << '\n';

    std::cout << "lookup:";
    for (int32_t a : {1, 5, 7}) {
      for (int32_t s : {a + 1, a + 2}) {
        std::cout << ' ' << a << ',' << s << '='
                  << (db.lookup_bb(a, s) ? '1' : '0');
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

  dump("round0");

  send({1, 5});
  dump("round1");

  send({7});
  dump("round2");
  return 0;
}
