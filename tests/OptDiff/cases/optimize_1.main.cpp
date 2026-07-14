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
    std::vector<std::pair<uint64_t, uint64_t>> rows;
    auto c = out_ff(db);
    for (uint64_t a = 0, b = 0; c.next(a, b);) {
      rows.emplace_back(a, b);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << "out:";
    for (auto &[a, b] : rows) {
      std::cout << " (" << a << ',' << b << ')';
    }
    std::cout << '\n';
  };

  {
    hyde::rt::Vec<add_edge_input> edges(allocator);
    edges.Add({1, 2});
    edges.Add({2, 3});
    add_edge_2(db, log, functors, std::move(edges));
  }
  dump();

  {
    hyde::rt::Vec<add_edge_input> edges(allocator);
    edges.Add({3, 4});
    edges.Add({5, 1});  // extends the chain from a new source
    add_edge_2(db, log, functors, std::move(edges));
  }
  dump();
  return 0;
}
