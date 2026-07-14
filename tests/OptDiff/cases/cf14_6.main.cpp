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
      auto c = outt_f(db);
      for (int32_t a = 0; c.next(a);) {
        vs.push_back(a);
      }
      std::sort(vs.begin(), vs.end());
      std::cout << "outt:";
      for (auto v : vs) {
        std::cout << ' ' << v;
      }
      std::cout << '\n';
    }
    {
      std::vector<int32_t> vs;
      auto c = outu_f(db);
      for (int32_t a = 0; c.next(a);) {
        vs.push_back(a);
      }
      std::sort(vs.begin(), vs.end());
      std::cout << "outu:";
      for (auto v : vs) {
        std::cout << ' ' << v;
      }
      std::cout << '\n';
    }
  };

  {
    hyde::rt::Vec<in_input> v(allocator);
    v.Add({1, 1, 1});
    v.Add({2, 2, 3});
    v.Add({4, 4, 4});
    v.Add({5, 6, 5});
    in_3(db, log, functors, std::move(v));
  }
  {
    hyde::rt::Vec<m1_input> v(allocator);
    v.Add({1});
    m1_1(db, log, functors, std::move(v));
  }
  {
    hyde::rt::Vec<m2_input> v(allocator);
    v.Add({2});
    m2_1(db, log, functors, std::move(v));
  }
  {
    hyde::rt::Vec<m3_input> v(allocator);
    v.Add({2});
    v.Add({3});
    m3_1(db, log, functors, std::move(v));
  }
  dump();

  {
    hyde::rt::Vec<in_input> v(allocator);
    v.Add({7, 7, 7});
    v.Add({8, 9, 9});
    in_3(db, log, functors, std::move(v));
  }
  {
    hyde::rt::Vec<m1_input> v(allocator);
    v.Add({4});
    m1_1(db, log, functors, std::move(v));
  }
  {
    hyde::rt::Vec<m3_input> v(allocator);
    v.Add({1});
    m3_1(db, log, functors, std::move(v));
  }
  dump();
  return 0;
}
