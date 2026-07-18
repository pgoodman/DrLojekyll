// Copyright 2026, Peter Goodman. All rights reserved.

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  auto dump = [&db]() {
    std::vector<std::pair<uint64_t, uint64_t>> pairs;
    auto c = out_ff(db);
    for (uint64_t a = 0, b = 0; c.next(a, b);) {
      pairs.emplace_back(a, b);
    }
    std::sort(pairs.begin(), pairs.end());
    std::cout << "tc:";
    for (auto [a, b] : pairs) {
      std::cout << ' ' << a << "->" << b;
    }
    std::cout << '\n';
  };

  {
    hyde::rt::Vec<edge_input> edges(allocator);
    edges.Add({1, 2});
    edges.Add({2, 3});
    edges.Add({5, 6});
    edge_2(db, log, functors, std::move(edges));
  }
  std::cout << "round 1\n";
  dump();

  {
    hyde::rt::Vec<edge_input> edges(allocator);
    edges.Add({3, 5});  // bridge the components
    edge_2(db, log, functors, std::move(edges));
  }
  std::cout << "round 2\n";
  dump();

  return 0;
}
