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
    std::vector<uint64_t> nodes;
    auto nc = db.is_node_f();
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
      auto c = db.reachable_from_bf(f);
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
    for (auto t : nodes) {
      std::vector<uint64_t> froms;
      auto c = db.reaching_to_fb(t);
      for (uint64_t f = 0; c.next(f);) {
        froms.push_back(f);
      }
      std::sort(froms.begin(), froms.end());
      std::cout << "to " << t << ':';
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
  std::cout << "round 1\n";
  dump();

  {
    hyde::rt::Vec<add_edge_input> edges(allocator);
    edges.Add({4, 7});  // connect the components
    edges.Add({8, 1});  // close a cycle
    db.add_edge_2(std::move(edges));
  }
  std::cout << "round 2\n";
  dump();
  return 0;
}
