#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#include "datalog.h"

int32_t add_i32_bbf(int32_t L, int32_t R) {
  return static_cast<int32_t>(static_cast<uint32_t>(L) +
                              static_cast<uint32_t>(R));
}

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  auto dump = [&db]() {
    // 0..46 covers the full bounded model; 47 is past the bound and must
    // scan empty.
    for (int32_t n = 0; n <= 47; ++n) {
      std::vector<int32_t> vals;
      auto c = fib_bf(db, n);
      for (int32_t v = 0; c.next(v);) {
        vals.push_back(v);
      }
      std::sort(vals.begin(), vals.end());
      std::cout << "fib(" << n << "):";
      for (auto v : vals) {
        std::cout << ' ' << v;
      }
      std::cout << '\n';
    }
  };

  // No messages exist in this program; dump the query twice to check that
  // repeated scans are stable.
  dump();
  dump();
  return 0;
}
