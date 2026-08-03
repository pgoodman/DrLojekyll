// Copyright 2026, Peter Goodman. All rights reserved.
//
// Driver for demand_diff_pub_1 -- THE FLAGSHIP (test-matrix C8): the R-DIFF
// differential-PUBLISHED demand answer witness. Compiled with
// `-demand -demand-retract` (.drflags); the .eqgate sidecar re-drives it under
// -demand-instance and byte-compares the two stdouts (flat == nested == golden).
//
// The distinguishing move (vs demand_diff_input_1, which stresses demand-DEATH x
// edge-retract): here the demand STANDS across the retraction. Epoch 1 lands
// edges and probes (standing demand for the probed keys); epoch 2 REMOVES an
// in-answer edge while that demand is still live, and the published answer must
// SHRINK through the @differential `nbhd_out` tap (DiffTable Present(key)==true
// keeps the a2' gate OPEN, so the net retraction publishes a -delta). This is
// the sole pre-cutover oracle for the DRInstance::differential pub arm
// (P-STORE: TableIsDifferential(pub) true).
//
// Death is NOT exercised (that is demand_diff_input_1's job): the demand never
// dies here, so every published -delta is an EDGE retraction observed live at
// the add_edge_2(rem) call. Teeth: retract_live_edge hard-ABORTS (survives
// NDEBUG) if a live-demand edge retraction publishes NOTHING -- an unreachable
// or dead diff pub arm dies loudly here. HP-5: each probe ASSERTS its drained
// answer is EXACTLY the surviving single-hop for the key. Cursor contract:
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

// Published-delta observer (the nbhd_out PrintLog idiom). Sorted per-epoch
// flush: published-delta ORDER within an epoch is unspecified and mode-varying,
// so the driver sorts each epoch's tokens before printing -- 4-mode
// byte-identity by construction.
struct PrintLog {
  std::vector<std::string> rows;

  void nbhd_out_2(uint64_t From, uint64_t To, bool added) {
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

  // Inject demand for `key`, drain the answer, assert it is EXACTLY expected
  // (HP-5), flush the injection epoch's deltas, print the answer.
  auto probe = [&](uint64_t key, std::vector<uint64_t> expected,
                   const char *label) {
    std::vector<uint64_t> vals;
    auto c = neighbors_bf(db, log, functors, key);
    for (uint64_t v = 0; c.next(v);) {
      vals.push_back(v);
    }
    std::sort(vals.begin(), vals.end());
    std::sort(expected.begin(), expected.end());
    assert(vals == expected);  // HP-5.
    log.flush(label);
    std::cout << "nbhd " << key << ':';
    for (auto v : vals) {
      std::cout << ' ' << v;
    }
    std::cout << '\n';
  };
  // add_edge adds (two-Vec differential, empty `rem`). No demand yet -> pruned.
  auto send = [&](std::vector<std::pair<uint64_t, uint64_t>> es,
                  const char *label) {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    for (auto &[f, t] : es) {
      add.Add({f, t});
    }
    add_edge_2(db, log, functors, std::move(add), std::move(rem));
    log.flush(label);
  };
  // add_edge rem-only, under STANDING demand: DiffTable Present(key)==true keeps
  // the a2' gate OPEN, so the net retraction MUST publish a -delta through the
  // diff pub tap. Hard ABORT (teeth survive NDEBUG) if it publishes nothing --
  // an unreachable/dead diff pub arm is exactly the finding this case guards.
  auto retract_live_edge =
      [&](std::vector<std::pair<uint64_t, uint64_t>> es, const char *label) {
        hyde::rt::Vec<Tup_u64_u64> add(allocator);
        hyde::rt::Vec<Tup_u64_u64> rem(allocator);
        for (auto &[f, t] : es) {
          rem.Add({f, t});
        }
        add_edge_2(db, log, functors, std::move(add), std::move(rem));
        if (log.rows.empty()) {
          std::cerr << "live-demand edge retract published nothing (" << label
                    << ")\n";
          std::abort();
        }
        log.flush(label);
      };

  // ---- Epoch 1: land edges (pruned, no demand), then stand demand + probe. ----
  send({{1, 10}, {1, 20}, {3, 30}}, "e1");  // demand-pruned: publishes nothing.
  probe(1, {10, 20}, "p1");                 // stand demand for 1; +(1,10)+(1,20).
  probe(3, {30}, "p3");                      // stand demand for 3; +(3,30).

  // ---- Epoch 2: retract an in-answer edge while demand STANDS (answer shrinks
  //      through the diff pub tap). ----
  retract_live_edge({{1, 10}}, "d1");        // demand.Present(1)==true -> -(1,10).
  probe(1, {20}, "p1b");                      // re-demand idempotent: no deltas.
  retract_live_edge({{3, 30}}, "d2");        // -(3,30); 3's answer empties.
  probe(3, {}, "p3b");                        // shrunk to {}.
  probe(1, {20}, "p1c");                      // 1 unaffected by 3's retraction.
  return 0;
}
