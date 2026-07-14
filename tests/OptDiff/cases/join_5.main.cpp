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
    for (int32_t a = 0; qc.next(a);) {
      qs.push_back(a);
    }
    std::sort(qs.begin(), qs.end());
    std::cout << "q:";
    for (auto a : qs) {
      std::cout << ' ' << a;
    }
    std::cout << '\n';
  };

  {
    hyde::rt::Vec<m1_input> v1(allocator);
    v1.Add({1, 10});
    v1.Add({2, 20});
    m1_2(db, log, functors, std::move(v1));
    hyde::rt::Vec<m2_input> v2(allocator);
    v2.Add({1, 100});
    v2.Add({3, 300});
    m2_2(db, log, functors, std::move(v2));
  }
  dump();

  {
    hyde::rt::Vec<m2_input> v2(allocator);
    v2.Add({2, 200});
    m2_2(db, log, functors, std::move(v2));
    hyde::rt::Vec<m1_input> v1(allocator);
    v1.Add({3, 30});
    m1_2(db, log, functors, std::move(v1));
  }
  dump();
  return 0;
}
