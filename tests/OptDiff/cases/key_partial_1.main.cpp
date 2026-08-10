// Copyright 2026, Peter Goodman. All rights reserved.
//
// P5 positive @key carrier driver. `path(From, To) @key(From)` is materialized
// from `edge_2`; the bound+free `#query reachable_from(bound From, free To)`
// reads it. @key drives the P5 declared-access-path model + the -region-out
// declared-key render (pinned via the .irgold sidecar); the ANSWER is
// unchanged (the full-materialization backend answers a complete read), so this
// .stdout is answer-invariant across all 4 optimization modes.

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

  // Drain the keyed cursor for one bound key, sorted (cursor enumeration order
  // is unspecified — the driver sorts, per the cursor contract).
  auto probe = [&db](uint64_t from) {
    auto cursor = reachable_from_bf(db, from);
    std::vector<uint64_t> tos;
    uint64_t to;
    while (cursor.next(to)) {
      tos.push_back(to);
    }
    std::sort(tos.begin(), tos.end());
    std::cout << "reachable_from(" << from << "):";
    for (uint64_t t : tos) {
      std::cout << ' ' << t;
    }
    std::cout << '\n';
  };

  probe(1);  // empty — no edges yet.

  {
    hyde::rt::Vec<edge_2_input> edges(allocator);
    edges.Add({1, 2});
    edges.Add({1, 3});
    edges.Add({2, 4});
    edges.Add({7, 9});  // out-of-key noise.
    edge_2_2(db, log, functors, std::move(edges));
  }

  probe(1);  // 2, 3
  probe(2);  // 4
  probe(7);  // 9
  probe(5);  // empty — no such key.
  return 0;
}
