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

  auto dump = [&]() {
    for (uint64_t bucket : {6u, 7u, 8u}) {
      std::vector<uint32_t> cs;
      auto c = db.get_bucket_bf(bucket);
      for (uint32_t n = 0; c.next(n);) {
        cs.push_back(n);
      }
      std::sort(cs.begin(), cs.end());
      std::cout << "bucket(" << bucket << "):";
      for (auto n : cs) {
        std::cout << ' ' << n;
      }
      std::cout << '\n';
    }
    std::cout << "--\n";
  };

  dump();
  {
    hyde::rt::Vec<seen_input> rows(allocator);
    rows.Add({100, 2});
    rows.Add({101, 3});
    db.seen_2(std::move(rows));
  }
  dump();
  {
    hyde::rt::Vec<seen_input> rows(allocator);
    rows.Add({102, 5});
    db.seen_2(std::move(rows));
  }
  dump();
  return 0;
}
