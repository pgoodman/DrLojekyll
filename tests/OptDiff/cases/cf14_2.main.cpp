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
      auto c = q1_f(db);
      for (int32_t a = 0; c.next(a);) {
        vs.push_back(a);
      }
      std::sort(vs.begin(), vs.end());
      std::cout << "q1:";
      for (auto v : vs) {
        std::cout << ' ' << v;
      }
      std::cout << '\n';
    }
    {
      std::vector<int32_t> vs;
      auto c = q2_f(db);
      for (int32_t a = 0; c.next(a);) {
        vs.push_back(a);
      }
      std::sort(vs.begin(), vs.end());
      std::cout << "q2:";
      for (auto v : vs) {
        std::cout << ' ' << v;
      }
      std::cout << '\n';
    }
  };

  {
    hyde::rt::Vec<input_input> v(allocator);
    v.Add({1});
    v.Add({5});
    input_1(db, log, functors, std::move(v));
  }
  {
    hyde::rt::Vec<e1_input> v(allocator);
    v.Add({1, 2});
    v.Add({2, 3});
    e1_2(db, log, functors, std::move(v));
  }
  {
    hyde::rt::Vec<e2_input> v(allocator);
    v.Add({5, 6});
    e2_2(db, log, functors, std::move(v));
  }
  dump();

  {
    hyde::rt::Vec<input_input> v(allocator);
    v.Add({2});
    input_1(db, log, functors, std::move(v));
  }
  {
    hyde::rt::Vec<e1_input> v(allocator);
    v.Add({3, 4});
    e1_2(db, log, functors, std::move(v));
  }
  {
    hyde::rt::Vec<e2_input> v(allocator);
    v.Add({6, 7});
    v.Add({1, 9});
    e2_2(db, log, functors, std::move(v));
  }
  dump();
  return 0;
}
