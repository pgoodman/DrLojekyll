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
  Database db(allocator);
  init(db, log, functors);

  auto dump = [&db]() {
    {
      std::vector<int32_t> v;
      auto c = q_f(db);
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
      auto c = r_f(db);
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
      auto c = s_ff(db);
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
    member(db, log, functors, std::move(v));
  };
  auto send1 = [&](auto member, std::vector<int32_t> rows) {
    hyde::rt::Vec<Tup_i32> v(allocator);
    for (auto x : rows) {
      v.Add({x});
    }
    member(db, log, functors, std::move(v));
  };

  send2([](auto &&...args) { return a_2(std::forward<decltype(args)>(args)...); }, {{1, 2}, {3, 1}});
  send2([](auto &&...args) { return b_2(std::forward<decltype(args)>(args)...); }, {{4, 8}});
  send1([](auto &&...args) { return c_1(std::forward<decltype(args)>(args)...); }, {1, 4, 9});
  send2([](auto &&...args) { return in_2(std::forward<decltype(args)>(args)...); }, {{1, 5}, {2, 3}, {6, 2}});
  send1([](auto &&...args) { return bad_1(std::forward<decltype(args)>(args)...); }, {2});
  dump();

  // Second round: extend the union, add a negated fact that must retract an
  // existing s row, and add a c fact whose tx counterpart already exists.
  send2([](auto &&...args) { return a_2(std::forward<decltype(args)>(args)...); }, {{9, 100}});
  send2([](auto &&...args) { return in_2(std::forward<decltype(args)>(args)...); }, {{7, 8}});
  send1([](auto &&...args) { return bad_1(std::forward<decltype(args)>(args)...); }, {1});
  send1([](auto &&...args) { return c_1(std::forward<decltype(args)>(args)...); }, {3});
  dump();
  return 0;
}
