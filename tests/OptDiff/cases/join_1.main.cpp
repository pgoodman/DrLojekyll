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
    std::vector<int32_t> qs;
    auto qc = q_f(db);
    for (int32_t b = 0; qc.next(b);) {
      qs.push_back(b);
    }
    std::sort(qs.begin(), qs.end());
    std::cout << "q:";
    for (auto b : qs) {
      std::cout << ' ' << b;
    }
    std::cout << '\n';

    std::vector<int32_t> ns;
    auto nc = never_f(db);
    for (int32_t b = 0; nc.next(b);) {
      ns.push_back(b);
    }
    std::sort(ns.begin(), ns.end());
    std::cout << "never:";
    for (auto b : ns) {
      std::cout << ' ' << b;
    }
    std::cout << '\n';
  };

  {
    hyde::rt::Vec<t1_input> v1(allocator);
    v1.Add({1, 10});
    v1.Add({1, 11});
    v1.Add({2, 20});
    v1.Add({3, 30});
    t1_2(db, log, functors, std::move(v1));
    hyde::rt::Vec<t2_input> v2(allocator);
    v2.Add({1});
    v2.Add({3});
    t2_1(db, log, functors, std::move(v2));
  }
  dump();

  {
    hyde::rt::Vec<t1_input> v1(allocator);
    v1.Add({1, 12});
    t1_2(db, log, functors, std::move(v1));
    hyde::rt::Vec<t2_input> v2(allocator);
    v2.Add({2});
    t2_1(db, log, functors, std::move(v2));
  }
  dump();
  return 0;
}
