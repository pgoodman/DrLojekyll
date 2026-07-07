#include <algorithm>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator, log, functors);

  auto dump = [&db]() {
    {
      std::vector<int32_t> v;
      auto c = db.q_f();
      for (int32_t x = 0; c.next(x);) {
        v.push_back(x);
      }
      std::sort(v.begin(), v.end());
      std::cout << "q:";
      for (auto x : v) {
        std::cout << ' ' << x;
      }
      std::cout << '\n';
    }
    {
      std::vector<int32_t> v;
      auto c = db.r_f();
      for (int32_t x = 0; c.next(x);) {
        v.push_back(x);
      }
      std::sort(v.begin(), v.end());
      std::cout << "r:";
      for (auto x : v) {
        std::cout << ' ' << x;
      }
      std::cout << '\n';
    }
    {
      std::vector<std::pair<int32_t, int32_t>> v;
      auto c = db.s_ff();
      for (int32_t x = 0, y = 0; c.next(x, y);) {
        v.emplace_back(x, y);
      }
      std::sort(v.begin(), v.end());
      std::cout << "s:";
      for (auto [x, y] : v) {
        std::cout << " (" << x << ',' << y << ')';
      }
      std::cout << '\n';
    }
  };

  auto send2 = [&](auto member, std::vector<std::pair<int32_t, int32_t>> rows) {
    hyde::rt::Vec<Tup_i32_i32> v(allocator);
    for (auto [x, y] : rows) {
      v.Add({x, y});
    }
    (db.*member)(std::move(v));
  };
  auto send1 = [&](auto member, std::vector<int32_t> rows) {
    hyde::rt::Vec<Tup_i32> v(allocator);
    for (auto x : rows) {
      v.Add({x});
    }
    (db.*member)(std::move(v));
  };

  send2(&Database::a_2, {{1, 2}, {3, 1}});
  send2(&Database::b_2, {{4, 8}});
  send1(&Database::c_1, {1, 4, 9});
  send2(&Database::in_2, {{1, 5}, {2, 3}, {6, 2}});
  send1(&Database::bad_1, {2});
  dump();

  // Second round: extend the union, add a negated fact that must retract an
  // existing s row, and add a c fact whose tx counterpart already exists.
  send2(&Database::a_2, {{9, 100}});
  send2(&Database::in_2, {{7, 8}});
  send1(&Database::bad_1, {1});
  send1(&Database::c_1, {3});
  dump();
  return 0;
}
