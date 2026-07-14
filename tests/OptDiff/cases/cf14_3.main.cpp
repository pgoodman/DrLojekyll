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
    {
      std::vector<int32_t> vs;
      auto c = reach_f(db);
      for (int32_t a = 0; c.next(a);) {
        vs.push_back(a);
      }
      std::sort(vs.begin(), vs.end());
      std::cout << "reach:";
      for (auto v : vs) {
        std::cout << ' ' << v;
      }
      std::cout << '\n';
    }
    {
      std::vector<int32_t> vs;
      auto c = live_f(db);
      for (int32_t a = 0; c.next(a);) {
        vs.push_back(a);
      }
      std::sort(vs.begin(), vs.end());
      std::cout << "live:";
      for (auto v : vs) {
        std::cout << ' ' << v;
      }
      std::cout << '\n';
    }
  };

  {
    hyde::rt::Vec<Tup_i32> add(allocator);
    hyde::rt::Vec<Tup_i32> rem(allocator);
    add.Add({1});
    add.Add({2});
    add.Add({5});
    input_1(db, log, functors, std::move(add), std::move(rem));
  }
  {
    hyde::rt::Vec<edge_input> v(allocator);
    v.Add({1, 2});
    v.Add({2, 3});
    v.Add({5, 6});
    edge_2(db, log, functors, std::move(v));
  }
  dump();

  {
    hyde::rt::Vec<Tup_i32> add(allocator);
    hyde::rt::Vec<Tup_i32> rem(allocator);
    add.Add({7});
    rem.Add({1});
    input_1(db, log, functors, std::move(add), std::move(rem));
  }
  dump();
  return 0;
}
