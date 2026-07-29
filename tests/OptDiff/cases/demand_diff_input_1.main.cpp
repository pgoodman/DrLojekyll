// Copyright 2026, Peter Goodman. All rights reserved.
//
// Driver for demand_diff_input_1 (compiled with `-demand -demand-retract` via
// .drflags; the .eqgate sidecar re-drives it under `-demand-instance` and
// byte-compares the two stdouts). The diff-input x DIFF-demand COMPOSITION
// witness (P-STORE true, P-DEATH true): the `pt` input is @differential AND the
// demand relation is differential (`-demand-retract`), so both axes are on.
//
// It is the ONLY witness of the demand-death x edge-retract interleavings
// (E-F1/E-F2/E-F3, d3a2-substrate sec 4), which need a differential DEMAND to
// have demand death:
//   rd1/d1  (E-F1)  = the R-3 dead-key gate-close in the a2' REMOVAL arm: after
//                     demand for 1 dies, retracting (1,10) is SILENT (demand
//                     DiffTable::Present(1)==false closes the gate).
//   p1b     (E-F2)  = the ADV-3 conjunct on the a1 BIRTH source under diff-demand
//                     rebirth: re-demanding 1 rebuilds from the SHRUNKEN input
//                     (the (1,10) retracted at d1 is ABSENT).
//   d2/p1c  (E-F3)  = edge-retract for a dead-but-iid-bound key closes the gate;
//                     p1c rebirths from the fully-shrunken input -> {}.
// `retract_edges_silent` carries the hard-abort teeth (survive NDEBUG).
//
// Death is observed ONLY through the published @differential `getpt_out` tap (a
// probe re-injects demand = rebirth, so a probe cannot see death). HP-5: each
// probe ASSERTS its drained answer is EXACTLY getpt(Key). Cursor contract:
// drain fully before the next entry-point call; sort keyed drains before print.

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "datalog.h"

// Published-delta observer (the product_diff PrintLog idiom; sorted per-epoch
// flush for 4-mode byte-identity).
struct PrintLog {
  std::vector<std::string> rows;

  void getpt_out_2(uint64_t K, uint64_t V, bool added) {
    rows.push_back(std::string(added ? "+(" : "-(") + std::to_string(K) +
                   "," + std::to_string(V) + ")");
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

  auto probe = [&](uint64_t key, std::vector<uint64_t> expected,
                   const char *label) {
    std::vector<uint64_t> vals;
    auto c = getpt_bf(db, log, functors, key);
    for (uint64_t v = 0; c.next(v);) {
      vals.push_back(v);
    }
    std::sort(vals.begin(), vals.end());
    std::sort(expected.begin(), expected.end());
    assert(vals == expected);  // HP-5.
    log.flush(label);
    std::cout << "getpt " << key << ':';
    for (auto v : vals) {
      std::cout << ' ' << v;
    }
    std::cout << '\n';
  };
  // pt adds-only, publishing (two-Vec differential, empty `rem`).
  auto send = [&](std::vector<std::pair<uint64_t, uint64_t>> es,
                  const char *label) {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    for (auto &[k, v] : es) {
      add.Add({k, v});
    }
    pt_2(db, log, functors, std::move(add), std::move(rem));
    log.flush(label);
  };
  // demand retract (the -demand-retract surface).
  auto retract_demand = [&](uint64_t key, const char *label) {
    getpt_bf_retract(db, log, functors, key);
    log.flush(label);
  };
  // pt rem-only, SILENT: an edge retract for a key whose demand is dead must
  // close the a2' gate and publish nothing. Hard ABORT (teeth survive NDEBUG).
  auto retract_edges_silent =
      [&](std::vector<std::pair<uint64_t, uint64_t>> es, const char *label) {
        hyde::rt::Vec<Tup_u64_u64> add(allocator);
        hyde::rt::Vec<Tup_u64_u64> rem(allocator);
        for (auto &[k, v] : es) {
          rem.Add({k, v});
        }
        pt_2(db, log, functors, std::move(add), std::move(rem));
        if (!log.rows.empty()) {
          std::cerr << "dead-key edge published (" << label << ")\n";
          std::abort();
        }
        log.flush(label);
      };

  send({{1, 10}, {1, 20}, {3, 30}}, "e1");  // pt adds (two-Vec differential).
  probe(1, {10, 20}, "p1");                 // stand demand for key 1.
  probe(3, {30}, "p3");
  // ---- E-F1: demand-death THEN edge-retract same key (gate CLOSES) ----
  retract_demand(1, "rd1");             // getpt_bf_retract(db,log,functors,1).
  retract_edges_silent({{1, 10}}, "d1");  // 1 is dead: demand.Present(1)==false
                                          //   -> a2' gate closes -> SILENT.
  // ---- E-F2: edge-retract then RE-DEMAND (a1 birth from shrunken input) ----
  probe(1, {20}, "p1b");                // re-demand 1: a1 rebuilds; (1,10) was
                                        //   retracted at d1 -> must be ABSENT.
  // ---- E-F2b: LIVE-demand edge retract (DiffTable Present==true, gate OPEN) --
  send({{1, 30}}, "alive");             // a2 add under standing demand.
  {                                     // a2' with demand LIVE: DiffTable
    hyde::rt::Vec<Tup_u64_u64> add(allocator);   //   Present(1)==true -> the net
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);   //   retraction of (1,30) MUST
    rem.Add({1, 30});                            //   publish (teeth: abort).
    pt_2(db, log, functors, std::move(add), std::move(rem));
    if (log.rows.empty()) {
      std::cerr << "live-key edge retract published nothing (dlive)\n";
      std::abort();
    }
    log.flush("dlive");
  }
  probe(1, {20}, "plive");              // (1,30) gone, (1,20) survives.
  // ---- E-F3: edge-retract for a key whose demand is dead (still bound iid) --
  retract_demand(1, "rd1b");
  retract_edges_silent({{1, 20}}, "d2");  // dead key: gate closes, nothing drops.
  probe(1, {}, "p1c");                  // rebirth from fully-shrunken input: {}.
  return 0;
}
