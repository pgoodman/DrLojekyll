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
    std::cout << "reachable:";
    for (uint32_t f = 1; f <= 5; ++f) {
      for (uint32_t t = 1; t <= 5; ++t) {
        if (reachable_bb(db, f, t)) {
          std::cout << " (" << f << ',' << t << ')';
        }
      }
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
    add_edge_2(db, log, functors, std::move(av), std::move(rv));
  };

  dump();
  send({{1, 2}, {2, 3}}, {});
  dump();
  send({{3, 4}, {4, 2}}, {});  // creates a cycle 2->3->4->2
  dump();
  send({}, {{2, 3}});  // breaks the chain and the cycle
  dump();
  send({{2, 3}}, {{3, 4}});  // re-add one, remove another
  dump();
  return 0;
}
