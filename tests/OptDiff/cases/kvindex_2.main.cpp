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
    for (uint64_t from : {1u, 2u}) {
      for (uint64_t to : {10u, 20u}) {
        std::vector<uint32_t> ws;
        auto c = db.get_weight_bbf(from, to);
        for (uint32_t w = 0; c.next(w);) {
          ws.push_back(w);
        }
        std::sort(ws.begin(), ws.end());
        std::cout << "w(" << from << ',' << to << "):";
        for (auto w : ws) {
          std::cout << ' ' << w;
        }
        std::cout << '\n';
      }
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
    rows.Add({1, 10, 3});  // merges with the existing (1,10) weight
    rows.Add({1, 20, 2});
    db.add_edge_3(std::move(rows));
  }
  dump();
  return 0;
}
