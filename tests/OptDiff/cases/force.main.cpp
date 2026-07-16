#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#include "datalog.h"

// Deterministic id generator: a pure function of `Time` so the result does
// not depend on how many times the generated code invokes the functor.
uint32_t generate_next_id_bf(int64_t Time) {
  return static_cast<uint32_t>(Time * 10 + 3);
}

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  auto dump = [&db, &log, &functors](int64_t lo, int64_t hi) {
    for (int64_t t = lo; t <= hi; ++t) {
      std::vector<uint32_t> ids;
      auto c = get_next_id_bf(db, log, functors, t);
      for (uint32_t id = 0; c.next(id);) {
        ids.push_back(id);
      }
      std::sort(ids.begin(), ids.end());
      std::cout << "time " << t << ':';
      for (auto id : ids) {
        std::cout << ' ' << id;
      }
      std::cout << '\n';
    }
  };

  // Round 1: explicitly trigger a couple of times, then query a range that
  // includes both triggered and untriggered (query-forced) times.
  {
    hyde::rt::Vec<trigger_generate_next_id_input> v(allocator);
    v.Add({5});
    v.Add({6});
    trigger_generate_next_id_1(db, log, functors, std::move(v));
  }
  std::cout << "round 1\n";
  dump(4, 8);

  // Round 2: trigger another time, re-query the same range plus a new one.
  {
    hyde::rt::Vec<trigger_generate_next_id_input> v(allocator);
    v.Add({9});
    trigger_generate_next_id_1(db, log, functors, std::move(v));
  }
  std::cout << "round 2\n";
  dump(4, 10);
  return 0;
}
