#include <algorithm>
#include <array>
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
    std::vector<std::array<int32_t, 3>> rows;
    auto qc = q_fff(db);
    int32_t a = 0, b = 0, c = 0;
    while (qc.next(a, b, c)) {
      rows.push_back({a, b, c});
    }
    std::sort(rows.begin(), rows.end());
    std::cout << "q:";
    for (auto &r : rows) {
      std::cout << " (" << r[0] << ',' << r[1] << ',' << r[2] << ')';
    }
    std::cout << '\n';
  };

  {
    hyde::rt::Vec<m1_input> v1(allocator);
    v1.Add({1});
    v1.Add({2});
    m1_1(db, log, functors, std::move(v1));
    hyde::rt::Vec<m2_input> v2(allocator);
    v2.Add({1, 10});
    v2.Add({1, 11});
    v2.Add({3, 30});
    m2_2(db, log, functors, std::move(v2));
  }
  dump();

  {
    hyde::rt::Vec<m1_input> v1(allocator);
    v1.Add({3});
    m1_1(db, log, functors, std::move(v1));
    hyde::rt::Vec<m2_input> v2(allocator);
    v2.Add({2, 20});
    m2_2(db, log, functors, std::move(v2));
  }
  dump();
  return 0;
}
