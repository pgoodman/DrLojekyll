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
    std::vector<int32_t> o;
    {
      auto c = db.out_f();
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
    db.input_1(std::move(vec));
  }
  dump();
  {
    hyde::rt::Vec<input_input> vec(allocator);
    vec.Add({30});
    db.input_1(std::move(vec));
  }
  dump();
  return 0;
}
