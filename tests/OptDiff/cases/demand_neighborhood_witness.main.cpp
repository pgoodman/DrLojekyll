// Copyright 2026, Peter Goodman. All rights reserved.
//
// Driver for demand_neighborhood_witness (compiled with `-demand` via .drflags;
// the .eqgate sidecar additionally re-drives it under `-demand -demand-instance`
// and byte-compares the two stdouts). The demanded query entry
// neighborhood_bf(db, log, functors, Start) first INJECTS Start as demand
// (seeding the fabricated demand message through the synthesized injector, which
// runs the flow), then reads the answer.
//
// RAT-6 BIRTH-ONLY: every add_edge lands (in two epochs) BEFORE any demand
// probe -- there is NO edge-after-demand batch (that is the labeled feature
// gap; plumbing arrives with Rel). HP-5: the graph carries edges OUTSIDE each
// probed key's neighborhood, and each probe ASSERTS its drained answer is
// EXACTLY neighborhood(Start) -- an over-materialized nested arm aborts here
// (and also diverges from the golden). Cursor contract: drain fully before the
// next entry-point call; sort keyed drains before printing.

#include <algorithm>
#include <cassert>
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

  auto probe = [&](uint64_t start, std::vector<uint64_t> expected) {
    std::vector<uint64_t> nodes;
    auto c = neighborhood_bf(db, log, functors, start);
    for (uint64_t n = 0; c.next(n);) {
      nodes.push_back(n);
    }
    std::sort(nodes.begin(), nodes.end());
    // HP-5: the answer must be EXACTLY neighborhood(start) -- no over- or
    // under-materialization. A demand-scoping bug trips this before the golden.
    std::sort(expected.begin(), expected.end());
    assert(nodes == expected);
    std::cout << "nbhd " << start << ':';
    for (auto n : nodes) {
      std::cout << ' ' << n;
    }
    std::cout << '\n';
  };

  // Epoch 1: 1's first out-edges, the 9->9 self-loop, the detached 7->8.
  {
    hyde::rt::Vec<add_edge_input> edges(allocator);
    edges.Add({1, 2});
    edges.Add({1, 3});
    edges.Add({9, 9});
    edges.Add({7, 8});
    add_edge_2(db, log, functors, std::move(edges));
  }
  // Epoch 2: 1's third out-edge and 3's out-edges. BIRTH-ONLY: all edges are in
  // before any probe; the two epochs exercise multi-epoch seals.
  {
    hyde::rt::Vec<add_edge_input> edges(allocator);
    edges.Add({1, 4});
    edges.Add({3, 5});
    edges.Add({3, 6});
    add_edge_2(db, log, functors, std::move(edges));
  }

  probe(1, {2, 3, 4});  // multi-neighbor; a STRICT subset of all targets.
  probe(3, {5, 6});     // a neighbor-of-1 whose own edges are out-of-nbhd for 1.
  probe(9, {9});        // self-loop; over-materialization would add non-9 nodes.
  probe(5, {});         // 5 is a target but never a source -> empty answer.
  return 0;
}
