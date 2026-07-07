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
    std::cout << "chk:";
    for (int32_t x = 0; x <= 5; ++x) {
      std::cout << ' ' << (db.chk_b(x) ? '1' : '0');
    }
    std::cout << '\n';

    std::vector<int32_t> v;
    auto c = db.all_f();
    for (int32_t x = 0; c.next(x);) {
      v.push_back(x);
    }
    std::sort(v.begin(), v.end());
    std::cout << "all:";
    for (auto x : v) {
      std::cout << ' ' << x;
    }
    std::cout << '\n';
  };

  auto send_a = [&](std::vector<int32_t> add, std::vector<int32_t> rem) {
    hyde::rt::Vec<Tup_i32> av(allocator);
    hyde::rt::Vec<Tup_i32> rv(allocator);
    for (auto x : add) {
      av.Add({x});
    }
    for (auto x : rem) {
      rv.Add({x});
    }
    db.a_1(std::move(av), std::move(rv));
  };

  auto send_b = [&](std::vector<int32_t> add, std::vector<int32_t> rem) {
    hyde::rt::Vec<Tup_i32> av(allocator);
    hyde::rt::Vec<Tup_i32> rv(allocator);
    for (auto x : add) {
      av.Add({x});
    }
    for (auto x : rem) {
      rv.Add({x});
    }
    db.b_1(std::move(av), std::move(rv));
  };

  send_a({1, 2}, {});
  send_b({2, 3}, {});
  dump();

  send_a({}, {2});  // t(2) still supported via b
  dump();

  send_b({4}, {2, 3});  // t = {1, 4}
  dump();
  return 0;
}
