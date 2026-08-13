// Copyright 2026, Peter Goodman. All rights reserved.
//
// Driver for demand_diff_neighborhood_witness (compiled with `-demand` via
// .drflags; the .eqgate sidecar additionally re-drives it under
// `-demand -demand-instance` and byte-compares the two stdouts). The e5
// DIVERGENCE carrier: the `add_edge` input is @differential (deletable) while
// the demand relation stays MONOTONE, so P-STORE is TRUE while P-DEATH is FALSE
// (d3a2-substrate H-16). DEATH IS EDGE-DRIVEN: demand is monotone (no
// `neighborhood_bf_retract` -- verified absent), so a neighborhood SHRINKS by
// retracting its EDGES through the two-Vec differential message
// `add_edge_2(db, log, functors, adds, removes)`. Death is observed ONLY through
// the published @differential `nbhd_out` tap (a probe re-injects demand
// idempotently = rebirth, so a probe can never see death).
//
// HP-5: the graph carries edges OUTSIDE each probed key's neighborhood, and
// each probe ASSERTS its drained answer is EXACTLY neighborhood(Start) -- an
// over-materialized nested arm both aborts here AND diverges from the golden.
// Cursor contract: drain fully before the next entry-point call; sort keyed
// drains before printing.
//
// Discriminator roll-call (which step forces which gap):
//   d1  (E-D)      forces the a2' input net-removals trigger (OB1/H-11).
//   p1b/p1c        force the a2' rescan's `input.Present(s)` conjunct (ADV-3).
//   m1/p1c (E-E)   force the FORBIDDEN ungated-late-Recycle fence + shared
//                  TouchedFlag one-rescan (R-A2 sec 3 / OB5/OB6).
//   m2/p3b (O-6)   witness the NetBatch same-row annihilation (OB7).
//   d2  (O-5)      witnesses death-only-inside-the-demanded-neighborhood
//                  (G-STALE subsumption/H-18) -- hard-abort teeth.
//   m3a/m3b/p5     force the ADV-3 conjunct on the a1 BIRTH source: a CROSS-batch
//                  add-then-retract of (5,12) BEFORE 5 is demanded leaves a
//                  physically-present-but-DEAD row the a1 birth-rescan must skip
//                  (same-batch +/- would NetBatch-annihilate pre-fold and never
//                  create the dead row -- b4C-1/A4.1).

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "datalog.h"

// Published-delta observer (the product_diff PrintLog idiom). Sorted flush:
// published-delta ORDER within an epoch is unspecified and mode-varying
// (permcheck's order-free-per-epoch policy), so the driver sorts each epoch's
// tokens before printing -- 4-mode byte-identity by construction.
struct PrintLog {
  std::vector<std::string> rows;

  void nbhd_out_2(uint64_t S, uint64_t N, bool added) {
    rows.push_back(std::string(added ? "+(" : "-(") + std::to_string(S) +
                   "," + std::to_string(N) + ")");
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

  auto probe = [&](uint64_t start, std::vector<uint64_t> expected,
                   const char *label) {
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
    log.flush(label);  // deltas of the injection epoch.
    std::cout << "nbhd " << start << ':';
    for (auto n : nodes) {
      std::cout << ' ' << n;
    }
    std::cout << '\n';
  };
  // adds-only, publishing (the two-Vec differential call, empty `rem`).
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
  // rem-only, publishing (edge retract for a live-demanded key -> the a2'
  // input net-removals trigger).
  auto retract_edges = [&](std::vector<std::pair<uint64_t, uint64_t>> es,
                           const char *label) {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    for (auto &[f, t] : es) {
      rem.Add({f, t});
    }
    add_edge_2(db, log, functors, std::move(add), std::move(rem));
    log.flush(label);
  };
  // both non-empty, publishing (same-batch +/-; NetBatch annihilates same-row
  // pairs, different rows both survive as ONE net rescan).
  auto send_pm = [&](std::vector<std::pair<uint64_t, uint64_t>> adds,
                     std::vector<std::pair<uint64_t, uint64_t>> rems,
                     const char *label) {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    for (auto &[f, t] : adds) {
      add.Add({f, t});
    }
    for (auto &[f, t] : rems) {
      rem.Add({f, t});
    }
    add_edge_2(db, log, functors, std::move(add), std::move(rem));
    log.flush(label);
  };
  // A4.2 twin (add-only silent): the a1-Present cross-batch step m3a + the O-5
  // undemanded-key discriminator. Hard ABORT (not assert -- teeth survive
  // NDEBUG) if the epoch published anything.
  auto send_silent = [&](std::vector<std::pair<uint64_t, uint64_t>> es,
                         const char *label) {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    for (auto &[f, t] : es) {
      add.Add({f, t});
    }
    add_edge_2(db, log, functors, std::move(add), std::move(rem));
    if (!log.rows.empty()) {
      std::cerr << "undemanded-key edge published (" << label << ")\n";
      std::abort();
    }
    log.flush(label);
  };
  // A4.2 twin (rem-only silent): O-5 undemanded-key retract + the a1-Present
  // cross-batch step m3b. Same hard-abort teeth.
  auto retract_edges_silent =
      [&](std::vector<std::pair<uint64_t, uint64_t>> es, const char *label) {
        hyde::rt::Vec<Tup_u64_u64> add(allocator);
        hyde::rt::Vec<Tup_u64_u64> rem(allocator);
        for (auto &[f, t] : es) {
          rem.Add({f, t});
        }
        add_edge_2(db, log, functors, std::move(add), std::move(rem));
        if (!log.rows.empty()) {
          std::cerr << "dead/undemanded-key edge published (" << label << ")\n";
          std::abort();
        }
        log.flush(label);
      };

  // ---- BIRTH ----
  send({{1, 2}, {1, 3}, {9, 9}, {7, 8}}, "e1");
  send({{1, 4}, {3, 5}, {3, 6}}, "e2");
  probe(1, {2, 3, 4}, "p1");  // stands demand for 1 (mono, idempotent).
  probe(3, {5, 6}, "p3");
  probe(9, {9}, "p9");
  // ---- E-D: edge-retract shrinking a LIVE-demanded neighborhood (a2') ----
  retract_edges({{1, 2}}, "d1");   // -(1,2) published; 1 still demanded.
  probe(1, {3, 4}, "p1b");         // shrunk; HP-5 asserts EXACTLY {3,4}.
  // ---- E-E: same-batch +/- for one demanded key (the FORBIDDEN-fence catcher)
  send_pm({{1, 11}}, {{1, 3}}, "m1");  // +11 -3 net; one rescan reads NET input.
  probe(1, {4, 11}, "p1c");        // an ungated-late Recycle drops ALL of 1 here.
  // ---- O-6: same-batch +/- of the EXACT row -> no-op (NetBatch annihilates) --
  send_pm({{3, 5}}, {{3, 5}}, "m2");
  probe(3, {5, 6}, "p3b");         // unchanged.
  // ---- O-5: edge-retract of an UNDEMANDED key -> SILENT ----
  retract_edges_silent({{7, 8}}, "d2");  // 7 never demanded; nothing published.
  // ---- a1-Present-conjunct: CROSS-batch add-then-retract BEFORE demand ----
  // (same-batch +/- would NetBatch-annihilate pre-fold and never create the
  //  dead-but-PHYSICALLY-PRESENT row the a1 conjunct exists to skip -- b4C-1.)
  send_silent({{5, 12}}, "m3a");          // 5 undemanded: nothing published.
  retract_edges_silent({{5, 12}}, "m3b"); // (5,12) counters -> 0; row stays at
                                          //   RowAt(s) (no compaction, XC-7).
  probe(5, {}, "p5");                     // a1 birth-rescan meets the DEAD
                                          //   (5,12); Present(s) MUST skip it.
  return 0;
}
