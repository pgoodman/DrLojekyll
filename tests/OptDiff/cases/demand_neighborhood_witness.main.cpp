// Copyright 2026, Peter Goodman. All rights reserved.
//
// Driver for demand_neighborhood_witness (compiled with `-demand
// -demand-retract` via .drflags; the .eqgate sidecar additionally re-drives it
// under `-demand-instance` and byte-compares the two stdouts). The demanded
// query entry neighborhood_bf(db, log, functors, Start) first INJECTS Start as
// demand (seeding the fabricated demand message through the synthesized
// injector, which runs the flow), then reads the answer.
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
// D3.a.1 RETRACT (OD-15 SET-demand): death is LOG-observed, never probed — a
// probe of a dead key re-injects demand (= rebirth), so the only demand-blind
// observable is the published @differential nbhd_out delta stream. The e7
// step (an edge for a DEAD key) is the dead-key-edge suppression
// discriminator: send_expect_silent hard-ABORTS (not assert — the teeth
// survive NDEBUG) if the nested band-(a2) rebuilds a dead instance (the
// demand-liveness gate, R-3). p1c is the rebirth-freshness discriminator
// (same iid rebinds; frozen is empty post-death, so ALL rows re-publish,
// INCLUDING the while-dead (1,15)). r1b pins the (T,F) full retract against
// the REBUILT frozen set (death -> rebirth -> death cycling).
//
// LOAD-BEARING STRUCTURE (ADJ-R3(c), do NOT coalesce): the REBUILD phase is
// ONE edge per epoch with an IMMEDIATE single probe of that key. This shape is
// what discriminates band-(a2): a1-only drops the post-demand edge STRUCTURALLY
// (if-crossed demand idempotence — a re-asserted demand never re-seeds the
// demand frontier, so a1 is idle and the edge is dropped), whereas a multi-key
// rebuild batch can phantom-mask the gap (a stale pub/idx read) and make the
// eqgate pass PRE-a2, defeating the witness. Keep one-edge-per-epoch /
// immediate-single-probe.

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
// (permcheck's order-free-per-epoch policy), so the driver sorts each
// epoch's tokens before printing — 4-mode byte-identity by construction.
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
  auto send = [&](std::vector<std::pair<uint64_t, uint64_t>> es,
                  const char *label) {
    hyde::rt::Vec<add_edge_input> edges(allocator);
    for (auto &[f, t] : es) {
      edges.Add({f, t});
    }
    add_edge_2(db, log, functors, std::move(edges));
    log.flush(label);
  };
  // A4.3 NORMATIVE: the dead-key-edge discriminator. Hard ABORT (not assert)
  // BEFORE flushing if the epoch published anything — the teeth survive an
  // NDEBUG driver build; a nested arm missing the band-(a2) demand-liveness
  // gate (R-3) re-materializes the dead neighborhood and dies HERE (and also
  // diverges from the golden + the eqgate — triply loud).
  auto send_expect_silent = [&](std::vector<std::pair<uint64_t, uint64_t>> es,
                                const char *label) {
    hyde::rt::Vec<add_edge_input> edges(allocator);
    for (auto &[f, t] : es) {
      edges.Add({f, t});
    }
    add_edge_2(db, log, functors, std::move(edges));
    if (!log.rows.empty()) {
      std::cerr << "dead-key edge published\n";
      std::abort();
    }
    log.flush(label);
  };
  auto retract = [&](uint64_t start, const char *label) {
    neighborhood_bf_retract(db, log, functors, start);  // b1's surface.
    log.flush(label);
  };

  // ---- BIRTH (unchanged batches; labels + flushes added) ----
  // Epoch 1: 1's first out-edges, the 9->9 self-loop, the detached 7->8.
  // Epoch 2: 1's third out-edge and 3's out-edges. BIRTH-ONLY: all edges are
  // in before any probe; the two epochs exercise multi-epoch seals.
  send({{1, 2}, {1, 3}, {9, 9}, {7, 8}}, "e1");
  send({{1, 4}, {3, 5}, {3, 6}}, "e2");
  probe(1, {2, 3, 4}, "p1");  // multi-neighbor; a STRICT subset of all targets.
  probe(3, {5, 6}, "p3");     // a neighbor-of-1 whose edges are out-of-nbhd for 1.
  probe(9, {9}, "p9");        // self-loop; over-materialization adds non-9 nodes.
  probe(5, {}, "p5");         // a target but never a source -> empty answer.

  // ---- R-a2 REBUILD: edge-after-demand for STANDING keys (un-retires RAT-6).
  //      One edge per epoch, immediate single probe (ADJ-R3(c)). ----
  send({{1, 11}}, "e3");
  probe(1, {2, 3, 4, 11}, "p1b");  // rebuild a multi-neighbor standing key.
  send({{3, 7}}, "e4");
  probe(3, {5, 6, 7}, "p3b");      // rebuild another standing key.
  send({{5, 12}}, "e5");
  probe(5, {12}, "p5b");           // rebuild a previously-EMPTY standing key.
  send({{13, 14}}, "e6");
  probe(9, {9}, "p9b");            // (13,14): undemanded key -> no leak into 9.

  // ---- D3.a.1 RETRACT: SET-demand death (OD-15) ----
  retract(1, "r1");            // full (T,F) retract: -(1,2..11) published.
  probe(3, {5, 6, 7}, "p3c");  // standing keys untouched by 1's death.
  probe(9, {9}, "p9c");
  // Edge for the DEAD key in a LATER epoch: must be a NO-OP (flat: no demand
  // row joins; nested: band-(a2) must gate on demand-liveness — FindInstance
  // still returns 1's iid, no tombstone, so liveness is the ONLY correct
  // gate). send_expect_silent makes a leak a hard abort, not just a golden
  // diverge.
  send_expect_silent({{1, 15}}, "e7");
  probe(5, {12}, "p5c");       // no leak of 15 anywhere.
  retract(7, "r7");            // never-demanded key: retract is a no-op (R-4
                               //   total+idempotent surface: remove of an
                               //   absent explicit row is structurally
                               //   nothing; was==now publishes nothing).

  // ---- REBIRTH: re-demand the SAME key (band-(a1); FindOrAdd rebinds the
  //      SAME iid — no tombstone; frozen is empty post-death so ALL rows are
  //      born, INCLUDING the while-dead (1,15) — rebirth is a fresh rescan,
  //      never a stale-store replay). ----
  probe(1, {2, 3, 4, 11, 15}, "p1c");

  // ---- SECOND DEATH: the rebuilt frozen set retracts in full (death ->
  //      rebirth -> death cycling; pins (T,F) against the REBUILT frozen). ----
  retract(1, "r1b");
  probe(9, {9}, "p9d");        // store sane after the second death.
  return 0;
}
