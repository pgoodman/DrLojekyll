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
    for (int32_t n = 0; n <= 12; ++n) {
      std::vector<int32_t> vals;
      auto c = fibonnaci_bf(db, n);
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

  dump();  // Before any init message.

  // Round 1: request the fibonacci loop up to 8.
  {
    hyde::rt::Vec<init_fibonacci_input> rows(allocator);
    rows.Add({8});
    init_fibonacci_1(db, log, functors, std::move(rows));
  }
  dump();

  // Round 2: N=1 is the only value for which the loop's `I < N` guard lets
  // `fibonnaci(N, _)` be derived (via the base tuple I=1), plus two others.
  {
    hyde::rt::Vec<init_fibonacci_input> rows(allocator);
    rows.Add({1});
    rows.Add({5});
    rows.Add({12});
    init_fibonacci_1(db, log, functors, std::move(rows));
  }
  dump();
  return 0;
}
