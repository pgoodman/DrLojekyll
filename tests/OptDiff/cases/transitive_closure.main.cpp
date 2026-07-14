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
    std::vector<uint64_t> nodes;
    auto nc = is_node_f(db);
    for (uint64_t n = 0; nc.next(n);) {
      nodes.push_back(n);
    }
    std::sort(nodes.begin(), nodes.end());
    std::cout << "nodes:";
    for (auto n : nodes) {
      std::cout << ' ' << n;
    }
    std::cout << '\n';
    for (auto f : nodes) {
      std::vector<uint64_t> tos;
      auto c = reachable_from_bf(db, f);
      for (uint64_t t = 0; c.next(t);) {
        tos.push_back(t);
      }
      std::sort(tos.begin(), tos.end());
      std::cout << "from " << f << ':';
      for (auto t : tos) {
        std::cout << ' ' << t;
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
    add_edge_2(db, log, functors, std::move(edges));
  }
  dump();

  {
    hyde::rt::Vec<add_edge_input> edges(allocator);
    edges.Add({4, 1});  // close a cycle
    edges.Add({4, 7});  // connect the components
    add_edge_2(db, log, functors, std::move(edges));
  }
  dump();
  return 0;
}
