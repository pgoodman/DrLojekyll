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
    {
      std::vector<int32_t> vs;
      auto c = db.q1_f();
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
      auto c = db.q2_f();
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
    db.input_1(std::move(v));
  }
  {
    hyde::rt::Vec<e1_input> v(allocator);
    v.Add({1, 2});
    v.Add({2, 3});
    db.e1_2(std::move(v));
  }
  {
    hyde::rt::Vec<e2_input> v(allocator);
    v.Add({5, 6});
    db.e2_2(std::move(v));
  }
  dump();

  {
    hyde::rt::Vec<input_input> v(allocator);
    v.Add({2});
    db.input_1(std::move(v));
  }
  {
    hyde::rt::Vec<e1_input> v(allocator);
    v.Add({3, 4});
    db.e1_2(std::move(v));
  }
  {
    hyde::rt::Vec<e2_input> v(allocator);
    v.Add({6, 7});
    v.Add({1, 9});
    db.e2_2(std::move(v));
  }
  dump();
  return 0;
}
