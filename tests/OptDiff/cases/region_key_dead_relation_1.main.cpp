// Driver for region_key_dead_relation_1: the bracketed-but-dead #local must
// not perturb the live flow. All-free cursor drain, sorted before printing
// (the keyed-cursor contract).
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

  {
    hyde::rt::Vec<edge_2_input> edges(allocator);
    edges.Add({1, 2});
    edges.Add({2, 3});
    edges.Add({1, 3});
    edge_2_2(db, log, functors, std::move(edges));
  }

  std::vector<std::pair<uint64_t, uint64_t>> rows;
  auto c = q_all_ff(db);
  for (uint64_t f = 0, t = 0; c.next(f, t);) {
    rows.emplace_back(f, t);
  }
  std::sort(rows.begin(), rows.end());
  for (auto [f, t] : rows) {
    std::cout << f << ' ' << t << '\n';
  }
  return 0;
}
