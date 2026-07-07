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
    std::vector<int32_t> us;
    auto c = db.check_f();
    for (int32_t u = 0; c.next(u);) {
      us.push_back(u);
    }
    std::sort(us.begin(), us.end());
    std::cout << "check:";
    for (auto u : us) {
      std::cout << ' ' << u;
    }
    std::cout << '\n';
  };

  auto send_log_in = [&](std::vector<int32_t> add, std::vector<int32_t> rem) {
    hyde::rt::Vec<Tup_i32> av(allocator);
    hyde::rt::Vec<Tup_i32> rv(allocator);
    for (auto u : add) {
      av.Add({u});
    }
    for (auto u : rem) {
      rv.Add({u});
    }
    db.log_in_1(std::move(av), std::move(rv));
  };

  dump();

  send_log_in({1, 2}, {});
  dump();

  {
    hyde::rt::Vec<boot_input> vec(allocator);
    vec.Add({2});
    vec.Add({4});
    db.boot_1(std::move(vec));
  }
  dump();

  // Remove 1 (only differential support: should vanish) and 2 (still
  // supported by boot: should remain).
  send_log_in({3}, {1, 2});
  dump();
  return 0;
}
