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
  Database db(allocator, log, functors);

  auto dump = [&db]() {
    std::vector<std::pair<uint32_t, uint32_t>> rows;
    auto c = db.symmetric_ff();
    for (uint32_t f = 0, t = 0; c.next(f, t);) {
      rows.emplace_back(f, t);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << "symmetric:";
    for (auto [f, t] : rows) {
      std::cout << " (" << f << ',' << t << ')';
    }
    std::cout << '\n';
  };

  auto send = [&](std::vector<std::pair<uint32_t, uint32_t>> add,
                  std::vector<std::pair<uint32_t, uint32_t>> rem) {
    hyde::rt::Vec<Tup_u32_u32> av(allocator);
    hyde::rt::Vec<Tup_u32_u32> rv(allocator);
    for (auto [f, t] : add) {
      av.Add({f, t});
    }
    for (auto [f, t] : rem) {
      rv.Add({f, t});
    }
    db.add_edge_2(std::move(av), std::move(rv));
  };

  dump();
  send({{1, 2}, {2, 1}, {3, 4}}, {});
  dump();
  send({{4, 3}, {5, 5}}, {});  // completes (3,4)/(4,3); self-loop
  dump();
  send({}, {{2, 1}});  // breaks symmetry of 1<->2
  dump();
  send({{2, 1}}, {{5, 5}});  // restore one, remove self-loop
  dump();
  send({}, {{3, 4}, {4, 3}});  // remove both directions
  dump();
  return 0;
}
