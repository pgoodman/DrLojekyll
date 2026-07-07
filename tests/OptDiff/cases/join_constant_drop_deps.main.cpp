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
    std::vector<int32_t> ys;
    auto c = db.bar_f();
    for (int32_t y = 0; c.next(y);) {
      ys.push_back(y);
    }
    std::sort(ys.begin(), ys.end());
    std::cout << "bar:";
    for (auto y : ys) {
      std::cout << ' ' << y;
    }
    std::cout << '\n';
  };

  std::cout << "round 0\n";
  dump();  // Only the fact foo(1, 2) exists; no add_x yet, so bar is empty.

  // Round 1: add_x values that do not match foo's X (=1) and one that does.
  {
    hyde::rt::Vec<add_x_input> v(allocator);
    v.Add({5});
    v.Add({7});
    db.add_x_1(std::move(v));
  }
  std::cout << "round 1\n";
  dump();

  // Round 2: now send the matching X = 1 (plus a duplicate of round 1).
  {
    hyde::rt::Vec<add_x_input> v(allocator);
    v.Add({1});
    v.Add({5});
    db.add_x_1(std::move(v));
  }
  std::cout << "round 2\n";
  dump();
  return 0;
}
