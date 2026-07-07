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
    for (uint64_t from : {1u, 2u, 3u, 4u}) {
      std::vector<uint64_t> tos;
      auto c = db.has_edge_bf(from);
      for (uint64_t to = 0; c.next(to);) {
        tos.push_back(to);
      }
      std::sort(tos.begin(), tos.end());
      std::cout << "has_edge(" << from << "):";
      for (auto t : tos) {
        std::cout << ' ' << t;
      }
      std::cout << '\n';
    }
    std::cout << "--\n";
  };

  dump();
  {
    hyde::rt::Vec<add_edge_input> rows(allocator);
    rows.Add({1, 10, 5});
    rows.Add({1, 11, 3});
    rows.Add({2, 20, 7});
    db.add_edge_3(std::move(rows));
  }
  dump();
  {
    hyde::rt::Vec<add_edge_input> rows(allocator);
    rows.Add({1, 10, 2});  // same key pair, different weight
    rows.Add({2, 21, 4});
    rows.Add({3, 30, 1});
    db.add_edge_3(std::move(rows));
  }
  dump();
  return 0;
}
