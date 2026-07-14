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
    auto c = pairs_ff(db);
    for (uint64_t a = 0, b = 0; c.next(a, b);) {
      rows.emplace_back(a, b);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << "pairs:";
    for (auto &[a, b] : rows) {
      std::cout << " (" << a << ',' << b << ')';
    }
    std::cout << '\n';

    std::vector<uint64_t> selfs;
    auto sc = self_pairs_f(db);
    for (uint64_t a = 0; sc.next(a);) {
      selfs.push_back(a);
    }
    std::sort(selfs.begin(), selfs.end());
    std::cout << "self:";
    for (auto a : selfs) {
      std::cout << ' ' << a;
    }
    std::cout << '\n';
  };

  {
    hyde::rt::Vec<add_node_input> nodes(allocator);
    nodes.Add({1});
    nodes.Add({2});
    add_node_1(db, log, functors, std::move(nodes));
  }
  dump();

  {
    hyde::rt::Vec<add_node_input> nodes(allocator);
    nodes.Add({3});
    add_node_1(db, log, functors, std::move(nodes));
  }
  dump();
  return 0;
}
