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
    std::vector<int32_t> o;
    {
      auto c = out_f(db);
      for (int32_t v = 0; c.next(v);) {
        o.push_back(v);
      }
    }
    std::sort(o.begin(), o.end());
    std::cout << "out:";
    for (auto v : o) {
      std::cout << ' ' << v;
    }
    std::cout << '\n';
  };

  dump();
  {
    hyde::rt::Vec<input_input> vec(allocator);
    vec.Add({10});
    vec.Add({20});
    input_1(db, log, functors, std::move(vec));
  }
  dump();
  {
    hyde::rt::Vec<input_input> vec(allocator);
    vec.Add({30});
    input_1(db, log, functors, std::move(vec));
  }
  dump();
  return 0;
}
