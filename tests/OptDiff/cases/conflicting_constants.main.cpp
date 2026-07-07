#include <algorithm>
#include <cstdint>
#include <iostream>
#include <tuple>
#include <vector>

#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator, log, functors);

  auto dump = [&db]() {
    std::vector<std::tuple<int32_t, int32_t, int32_t>> rows;
    auto c = db.foo_fff();
    for (int32_t a = 0, b = 0, cc = 0; c.next(a, b, cc);) {
      rows.emplace_back(a, b, cc);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << "foo:";
    for (auto &[a, b, cc] : rows) {
      std::cout << " (" << a << ',' << b << ',' << cc << ')';
    }
    std::cout << '\n';
  };

  // No messages exist; the facts are baked in at init time.
  dump();
  dump();
  return 0;
}
