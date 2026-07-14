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
    std::vector<int32_t> j;
    {
      auto c = out_join_f(db);
      for (int32_t v = 0; c.next(v);) {
        j.push_back(v);
      }
    }
    std::sort(j.begin(), j.end());
    std::cout << "out_join:";
    for (auto v : j) {
      std::cout << ' ' << v;
    }
    std::vector<std::pair<int32_t, int32_t>> s;
    {
      auto c = out_seed_ff(db);
      for (int32_t a = 0, b = 0; c.next(a, b);) {
        s.emplace_back(a, b);
      }
    }
    std::sort(s.begin(), s.end());
    std::cout << "\nout_seed:";
    for (auto [a, b] : s) {
      std::cout << " (" << a << ',' << b << ')';
    }
    std::cout << '\n';
  };

  dump();
  {
    hyde::rt::Vec<input_input> vec(allocator);
    vec.Add({1});
    vec.Add({3});
    input_1(db, log, functors, std::move(vec));
  }
  dump();
  {
    hyde::rt::Vec<input_input> vec(allocator);
    vec.Add({4});
    input_1(db, log, functors, std::move(vec));
  }
  dump();
  return 0;
}
