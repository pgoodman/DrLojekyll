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
  Database db(allocator, log, functors);

  auto dump = [&db]() {
    std::vector<std::array<int32_t, 3>> foos;
    auto fc = db.foo_fff();
    int32_t a = 0, b = 0, c = 0;
    while (fc.next(a, b, c)) {
      foos.push_back({a, b, c});
    }
    std::sort(foos.begin(), foos.end());
    std::cout << "foo:";
    for (auto &r : foos) {
      std::cout << " (" << r[0] << ',' << r[1] << ',' << r[2] << ')';
    }
    std::cout << '\n';

    std::vector<std::array<int32_t, 3>> bars;
    auto bc = db.bar_all_fff();
    while (bc.next(a, b, c)) {
      bars.push_back({a, b, c});
    }
    std::sort(bars.begin(), bars.end());
    std::cout << "bar:";
    for (auto &r : bars) {
      std::cout << " (" << r[0] << ',' << r[1] << ',' << r[2] << ')';
    }
    std::cout << '\n';
  };

  dump();

  {
    hyde::rt::Vec<mb_input> v(allocator);
    v.Add({1});
    v.Add({4});
    db.mb_1(std::move(v));
  }
  dump();

  {
    hyde::rt::Vec<mb_input> v(allocator);
    v.Add({5});
    db.mb_1(std::move(v));
  }
  dump();
  return 0;
}
