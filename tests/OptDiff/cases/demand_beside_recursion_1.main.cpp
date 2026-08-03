// Copyright 2026, Peter Goodman. All rights reserved.
//
// Driver for demand_beside_recursion_1 (compiled with `-demand` via the
// .drflags sidecar). Test-matrix C4: a recursive transitive-closure SCC
// (`path`, published by `path_out`) sits BESIDE the demanded subgraph
// (`pair` consumed by the bound query `pair_of(bound X, free Y)`). The SIP
// walk from the bound query stops at `seed_2` and never reaches
// `path`/`edge_2`, so the planner must NOT annex the recursion into demand.
//
// THE DISCRIMINATOR: `path_out`'s published deltas are observed EAGERLY right
// after each edge batch, BEFORE any `pair_of` probe. `path` has an eager
// consumer (`path_out`), so it must materialize the FULL closure the instant
// its `edge_2` inputs arrive -- an annexed-into-demand `path` would under-
// publish here (nothing demands it yet). Symmetrically, each `pair_of` probe
// asserts the `path_out` delta buffer stays EMPTY: demanding the pair query
// must not re-fire the beside recursion.
//
// Cursor contract: drain fully before the next entry-point call; sort keyed
// drains before printing. Published-delta order within an epoch is
// unspecified, so path_out deltas are sorted before printing (4-mode
// byte-identity by construction). All path node ids are two digits so the
// string sort coincides with the numeric tuple order.

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "datalog.h"

// Published-delta observer for the beside recursion's `path_out` message.
struct PrintLog {
  std::vector<std::string> rows;

  void path_out_2(uint64_t From, uint64_t To, bool added) {
    rows.push_back(std::string(added ? "+(" : "-(") + std::to_string(From) +
                   "," + std::to_string(To) + ")");
  }

  void flush(const char *label) {
    std::sort(rows.begin(), rows.end());
    std::cout << label << ':';
    for (const auto &r : rows) {
      std::cout << ' ' << r;
    }
    std::cout << '\n';
    rows.clear();
  }
};

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  PrintLog log;
  Database db(allocator);
  init(db, log, functors);

  auto send_edges = [&](std::vector<std::pair<uint64_t, uint64_t>> es,
                        const char *label) {
    hyde::rt::Vec<edge_2_input> edges(allocator);
    for (auto &[f, t] : es) {
      edges.Add({f, t});
    }
    edge_2_2(db, log, functors, std::move(edges));
    log.flush(label);  // the FULL closure delta of the beside recursion.
  };
  auto send_seeds = [&](std::vector<std::pair<uint64_t, uint64_t>> ss,
                        const char *label) {
    hyde::rt::Vec<seed_2_input> seeds(allocator);
    for (auto &[x, y] : ss) {
      seeds.Add({x, y});
    }
    seed_2_2(db, log, functors, std::move(seeds));
    // Seeds feed only the demanded region; they must NOT touch path_out.
    assert(log.rows.empty());
    log.flush(label);
  };
  auto probe = [&](uint64_t x) {
    std::vector<uint64_t> ys;
    auto c = pair_of_bf(db, log, functors, x);
    for (uint64_t y = 0; c.next(y);) {
      ys.push_back(y);
    }
    std::sort(ys.begin(), ys.end());
    // C4: demanding the pair query must not re-fire the beside recursion.
    assert(log.rows.empty());
    std::cout << "pair_of " << x << ':';
    for (auto y : ys) {
      std::cout << ' ' << y;
    }
    std::cout << '\n';
  };

  // ---- Batch 1: a chain 10->20->30 and a detached edge 40->50; four seeds. --
  send_edges({{10, 20}, {20, 30}, {40, 50}}, "edges b1");
  send_seeds({{1, 100}, {1, 101}, {2, 200}, {3, 300}}, "seeds b1");
  std::cout << "probes b1\n";
  probe(1);  // two answers.
  probe(2);  // one answer.
  probe(3);  // one answer.
  probe(9);  // never a seed key -> empty answer.

  // ---- Batch 2: extend the chain (30->60) and close a cycle (50->40), plus
  //      two more seeds (one extends key 3, one is a brand-new key 5). --------
  send_edges({{30, 60}, {50, 40}}, "edges b2");
  send_seeds({{3, 301}, {5, 500}}, "seeds b2");
  std::cout << "probes b2\n";
  probe(1);  // unchanged.
  probe(3);  // now two answers.
  probe(5);  // first demanded after its seed arrived.
  probe(9);  // still empty.
  return 0;
}
