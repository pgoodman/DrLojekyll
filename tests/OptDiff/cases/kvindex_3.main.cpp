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
    for (uint64_t from : {1u, 2u, 3u}) {
      std::vector<uint32_t> cs;
      auto c = db.get_count_bf(from);
      for (uint32_t n = 0; c.next(n);) {
        cs.push_back(n);
      }
      std::sort(cs.begin(), cs.end());
      std::cout << "count(" << from << "):";
      for (auto n : cs) {
        std::cout << ' ' << n;
      }
      std::cout << '\n';
    }
    std::cout << "--\n";
  };

  dump();
  {
    hyde::rt::Vec<add_edge_input> rows(allocator);
    rows.Add({1, 10, 5});
    rows.Add({2, 20, 7});
    db.add_edge_3(std::move(rows));
  }
  dump();
  {
    hyde::rt::Vec<add_edge_input> rows(allocator);
    rows.Add({1, 11, 3});
    rows.Add({3, 30, 1});
    db.add_edge_3(std::move(rows));
  }
  dump();
  return 0;
}
