// Copyright 2026, Peter Goodman. All rights reserved.
//
// Driver for demand_beside_mutual_1 (compiled with `-demand` via .drflags).
// Two observation surfaces exercised side by side:
//
//   (1) the PUBLISHED mutual ra<->rb SCC -- monotone `#message ra_out/rb_out`
//       taps observed through the DatabaseLog hooks (the product_diff PrintLog
//       idiom); these are NOT touched by the demand transform, so their
//       per-epoch published deltas are byte-identical to a flag-off build and
//       to every optimization mode.
//   (2) the DEMANDED `pick_of(bound V, free W)` query -- pick_of_bf(db, log,
//       functors, V) INJECTS V as demand then reads the answer cursor.
//
// ra(K,V):base; ra(K,W):rb(K,V),step(V,W); rb(K,W):ra(K,V),step(V,W) is a
// step-parity split: ra = even-step reachable from the base value, rb = odd.
// Cursor contract: drain fully before the next entry-point call; sort every
// keyed drain / published-delta epoch before printing (4-mode byte identity).

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "datalog.h"

// Published-delta observer for the two monotone SCC taps. Sorted flush:
// published-delta ORDER within an epoch is unspecified and mode-varying, so the
// driver sorts each epoch's tokens before printing.
struct PrintLog {
  std::vector<std::string> rows;

  void push(const char *rel, char sign, uint64_t k, uint64_t v) {
    rows.push_back(std::string(rel) + sign + '(' + std::to_string(k) + ',' +
                   std::to_string(v) + ')');
  }
  void ra_out_2(uint64_t K, uint64_t V, bool added) {
    push("ra", added ? '+' : '-', K, V);
  }
  void rb_out_2(uint64_t K, uint64_t V, bool added) {
    push("rb", added ? '+' : '-', K, V);
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
  log.flush("init");

  auto base = [&](uint64_t k, uint64_t v, const char *label) {
    hyde::rt::Vec<base_input> b(allocator);
    b.Add({k, v});
    base_2(db, log, functors, std::move(b));
    log.flush(label);
  };
  auto steps = [&](std::vector<std::pair<uint64_t, uint64_t>> es,
                   const char *label) {
    hyde::rt::Vec<step_input> s(allocator);
    for (auto &[v, w] : es) {
      s.Add({v, w});
    }
    step_2(db, log, functors, std::move(s));
    log.flush(label);
  };
  auto probe = [&](uint64_t v) {
    std::vector<uint64_t> ws;
    auto c = pick_of_bf(db, log, functors, v);
    for (uint64_t w = 0; c.next(w);) {
      ws.push_back(w);
    }
    std::sort(ws.begin(), ws.end());
    std::cout << "pick_of " << v << ':';
    for (auto w : ws) {
      std::cout << ' ' << w;
    }
    std::cout << '\n';
  };

  // Epoch 1: seed key 1 at value 10 (ra(1,10) only -- no step yet, no rb).
  base(1, 10, "e1 base(1,10)");
  // Epoch 2: the branching step graph arrives; the K=1 SCC fixpoint completes.
  steps({{10, 20}, {20, 30}, {30, 40}, {20, 25}}, "e2 step");
  // Epoch 3: a second key 2 at the SAME start value; full K=2 closure fires
  // against the already-present step graph.
  base(2, 10, "e3 base(2,10)");

  // Demanded probes over pick=step (a separate, non-recursive relation). Each
  // injects demand for its key; the beside SCC publishes nothing here.
  probe(10);  // {20}
  probe(20);  // {25, 30} -- the branch
  probe(30);  // {40}
  probe(40);  // {} -- no outgoing step
  probe(99);  // {} -- never a source
  log.flush("after probes");  // expected empty: demand touches only pick.
  return 0;
}
