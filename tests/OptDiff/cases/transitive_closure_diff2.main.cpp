#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  auto dump = [&db]() {
    for (uint64_t n = 1; n <= 9; ++n) {
      std::vector<uint64_t> tos;
      auto c = reachable_from_bf(db, n);
      for (uint64_t t = 0; c.next(t);) {
        tos.push_back(t);
      }
      std::sort(tos.begin(), tos.end());
      std::cout << "from " << n << ':';
      for (auto t : tos) {
        std::cout << ' ' << t;
      }
      std::cout << '\n';
    }
    for (uint64_t n = 1; n <= 9; ++n) {
      std::vector<uint64_t> froms;
      auto c = reaching_to_fb(db, n);
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
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    add.Add({1, 2});
    add.Add({2, 3});
    add.Add({3, 4});
    add.Add({7, 8});
    add_edge_2(db, log, functors, std::move(add), std::move(rem));
  }
  std::cout << "round 1\n";
  dump();

  {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    add.Add({4, 7});  // connect the components
    add.Add({8, 1});  // close a cycle
    rem.Add({2, 3});  // differential removal splits the chain
    add_edge_2(db, log, functors, std::move(add), std::move(rem));
  }
  std::cout << "round 2\n";
  dump();
  return 0;
}
