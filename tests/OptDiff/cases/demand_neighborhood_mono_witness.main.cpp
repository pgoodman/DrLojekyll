// Copyright 2026, Peter Goodman. All rights reserved.
//
// Driver for demand_neighborhood_witness (compiled with `-demand` via .drflags;
// the .eqgate sidecar additionally re-drives it under `-demand -demand-instance`
// and byte-compares the two stdouts). The demanded query entry
// neighborhood_bf(db, log, functors, Start) first INJECTS Start as demand
// (seeding the fabricated demand message through the synthesized injector, which
// runs the flow), then reads the answer.
//
// R-a2 BIRTH-AND-REBUILD: the birth phase lands all edges (in two epochs)
// BEFORE its four probes; the REBUILD phase then adds edges AFTER their key's
// demand is standing and re-probes — band-(a2) [R-REBUILD-a2] rebuilds the
// standing instance via a full edge-frontier rescan (edge-after-demand is no
// longer a gap under -demand-instance). HP-5: the graph carries edges OUTSIDE
// each probed key's neighborhood, and each probe ASSERTS its drained answer is
// EXACTLY neighborhood(Start) -- an over-materialized nested arm aborts here
// (and also diverges from the golden). Cursor contract: drain fully before the
// next entry-point call; sort keyed drains before printing.
//
// LOAD-BEARING STRUCTURE (ADJ-R3(c), do NOT coalesce): the REBUILD phase is
// ONE edge per epoch with an IMMEDIATE single probe of that key. This shape is
// what discriminates band-(a2): a1-only drops the post-demand edge STRUCTURALLY
// (if-crossed demand idempotence — a re-asserted demand never re-seeds vec29,
// so a1 is idle and the edge is dropped), whereas a multi-key rebuild batch can
// phantom-mask the gap (a stale pub/idx read) and make the eqgate pass PRE-a2,
// defeating the witness. Keep one-edge-per-epoch / immediate-single-probe.

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

  // --- R-a2 REBUILD: edge-after-demand for STANDING keys (un-retires RAT-6). ---
  // Each edge lands in its own epoch AFTER its key's demand is standing; the
  // probe re-injects the (if-crossed) demand and reads the rebuilt answer.
  { hyde::rt::Vec<add_edge_input> e(allocator); e.Add({1, 11});
    add_edge_2(db, log, functors, std::move(e)); }
  probe(1, {2, 3, 4, 11});   // rebuild a multi-neighbor standing key (HP-5: 11 is
                             //   out-of-nbhd for 3/5/9 — a mis-key would leak it).
  { hyde::rt::Vec<add_edge_input> e(allocator); e.Add({3, 7});
    add_edge_2(db, log, functors, std::move(e)); }
  probe(3, {5, 6, 7});       // rebuild another standing key.
  { hyde::rt::Vec<add_edge_input> e(allocator); e.Add({5, 12});
    add_edge_2(db, log, functors, std::move(e)); }
  probe(5, {12});            // rebuild a previously-EMPTY standing instance (5).
  { hyde::rt::Vec<add_edge_input> e(allocator); e.Add({13, 14});
    add_edge_2(db, log, functors, std::move(e)); }
  probe(9, {9});             // (13,14): edge for an UNDEMANDED key. This probe only
                             //   witnesses NO LEAK INTO key 9 (holds whether or not 13
                             //   was mis-birthed — a stray birth is answer-invisible;
                             //   ADJ-R4). The FindInstance-vs-FindOrAdd skip itself is
                             //   pinned by the A.3.2 unit arm, NOT this probe.
  return 0;
}
