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

  auto dump = [&db]() {
    for (uint64_t n = 1; n <= 9; ++n) {
      std::vector<uint64_t> froms;
      auto c = db.reaching_to_fb(n);
      for (uint64_t f = 0; c.next(f);) {
        froms.push_back(f);
      }
      std::sort(froms.begin(), froms.end());
      std::cout << "to " << n << ':';
      for (auto f : froms) {
        std::cout << ' ' << f;
      }
      std::cout << '\n';
    }
  };

  {
    hyde::rt::Vec<add_edge_input> edges(allocator);
    edges.Add({1, 2});
    edges.Add({2, 3});
    edges.Add({3, 4});
    edges.Add({7, 8});
    db.add_edge_2(std::move(edges));
  }
  {
    hyde::rt::Vec<unlock_reaching_to_input> unlocks(allocator);
    unlocks.Add({4});
    db.unlock_reaching_to_1(std::move(unlocks));
  }
  std::cout << "round 1\n";
  dump();

  {
    hyde::rt::Vec<add_edge_input> edges(allocator);
    edges.Add({4, 7});  // connect the components
    edges.Add({8, 1});  // close a cycle
    db.add_edge_2(std::move(edges));
  }
  {
    hyde::rt::Vec<unlock_reaching_to_input> unlocks(allocator);
    unlocks.Add({8});
    unlocks.Add({3});
    db.unlock_reaching_to_1(std::move(unlocks));
  }
  std::cout << "round 2\n";
  dump();
  return 0;
}
