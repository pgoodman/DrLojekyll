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
    std::vector<int32_t> qs;
    auto qc = db.q_f();
    for (int32_t a = 0; qc.next(a);) {
      qs.push_back(a);
    }
    std::sort(qs.begin(), qs.end());
    std::cout << "q:";
    for (auto a : qs) {
      std::cout << ' ' << a;
    }
    std::cout << '\n';

    std::vector<int32_t> ss;
    auto sc = db.qs_f();
    for (int32_t a = 0; sc.next(a);) {
      ss.push_back(a);
    }
    std::sort(ss.begin(), ss.end());
    std::cout << "qs:";
    for (auto a : ss) {
      std::cout << ' ' << a;
    }
    std::cout << '\n';
  };

  {
    hyde::rt::Vec<m1_input> v1(allocator);
    v1.Add({1, 10});
    v1.Add({2, 20});
    db.m1_2(std::move(v1));
    hyde::rt::Vec<m2_input> v2(allocator);
    v2.Add({1});
    db.m2_1(std::move(v2));
    hyde::rt::Vec<m3_input> v3(allocator);
    v3.Add({5, 50});
    db.m3_2(std::move(v3));
  }
  dump();

  {
    hyde::rt::Vec<m2_input> v2(allocator);
    v2.Add({2});
    v2.Add({7});
    db.m2_1(std::move(v2));
    hyde::rt::Vec<m3_input> v3(allocator);
    v3.Add({6, 60});
    db.m3_2(std::move(v3));
  }
  dump();
  return 0;
}
