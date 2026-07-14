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
    std::vector<int32_t> xs;
    auto c = proof_f(db);
    for (int32_t x = 0; c.next(x);) {
      xs.push_back(x);
    }
    std::sort(xs.begin(), xs.end());
    std::cout << "proof:";
    for (auto x : xs) {
      std::cout << ' ' << x;
    }
    std::cout << '\n';
  };

  std::cout << "round 0\n";
  dump();  // No `something` yet, so `indirect(1)` is unproven.

  // Round 1: any `something` message proves `indirect(1)`.
  {
    hyde::rt::Vec<something_input> v(allocator);
    v.Add({42});
    something_1(db, log, functors, std::move(v));
  }
  std::cout << "round 1\n";
  dump();

  // Round 2: more `something` facts; proof set must stay exactly {1}.
  {
    hyde::rt::Vec<something_input> v(allocator);
    v.Add({-3});
    v.Add({42});
    something_1(db, log, functors, std::move(v));
  }
  std::cout << "round 2\n";
  dump();
  return 0;
}
