// Copyright 2026, Peter Goodman. All rights reserved.
//
// Driver for demand_multi_adorn_witness — the D3.a.3 multi-adornment success
// witness. ONE query name `q` with TWO declared binding patterns; the bf probe
// enumerates out-neighbors-by-A, the fb probe enumerates in-neighbors-by-B.
// Each probe asserts EXACTLY the expected set — an over-materialized nested arm
// both diverges from the golden and trips this assert (HP-5).
//
// The .eqgate sidecar re-links this driver against the -demand-instance (nested,
// N=2 store) compile in all 4 optimization modes and byte-compares each mode's
// stdout LIVE against the flat golden: flat == nested == golden falls out
// transitively. The graph carries an OUT-OF-NEIGHBORHOOD component {10,11,12}
// that must never surface in a probe scoped to the first component.
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <tuple>
#include <vector>

#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  // Directed edges: a small in/out-neighborhood component {1,2,3,4} plus an
  // OUT-OF-NEIGHBORHOOD component {10,11,12} (the HP-5 over-materialization
  // catch — a mis-keyed store would leak these into a first-component probe).
  const std::vector<std::tuple<uint64_t, uint64_t>> edges = {
      {1, 2}, {1, 3}, {2, 4}, {10, 11}, {11, 12},
  };
  hyde::rt::Vec<edge_2_input> ev(allocator);
  for (auto &[a, b] : edges) {
    ev.Add({a, b});
  }
  edge_2_2(db, log, functors, std::move(ev));

  // CURSOR CONTRACT: drain each cursor FULLY before the next entry-point call;
  // sort every drain (keyed-cursor enumeration order is unspecified).
  auto probe_bf = [&](uint64_t a, std::vector<uint64_t> expect) {  // out-neighbors
    std::vector<uint64_t> got;
    auto c = q_bf(db, log, functors, a);
    uint64_t b;
    while (c.next(b)) {
      got.push_back(b);
    }
    std::sort(got.begin(), got.end());
    std::sort(expect.begin(), expect.end());
    assert(got == expect);  // HP-5: EXACTLY out-neighbors(a).
    return got;
  };
  auto probe_fb = [&](uint64_t b, std::vector<uint64_t> expect) {  // in-neighbors
    std::vector<uint64_t> got;
    auto c = q_fb(db, log, functors, b);
    uint64_t a;
    while (c.next(a)) {
      got.push_back(a);
    }
    std::sort(got.begin(), got.end());
    std::sort(expect.begin(), expect.end());
    assert(got == expect);  // HP-5: EXACTLY in-neighbors(b).
    return got;
  };
  auto emit = [&](const char *tag, uint64_t k, const std::vector<uint64_t> &v) {
    std::printf("%s(%llu) =", tag, (unsigned long long) k);
    for (auto x : v) {
      std::printf(" %llu", (unsigned long long) x);
    }
    std::printf("\n");
  };

  // bf store (adornment 0): out-neighbors.
  emit("bf", 1, probe_bf(1, {2, 3}));
  emit("bf", 2, probe_bf(2, {4}));
  emit("bf", 10, probe_bf(10, {11}));  // the OTHER component
  // fb store (adornment 1): in-neighbors. Interleaved to exercise both stores
  // over the shared pub within one run.
  emit("fb", 4, probe_fb(4, {2}));
  emit("fb", 2, probe_fb(2, {1}));
  emit("fb", 11, probe_fb(11, {10}));  // the OTHER component
  emit("fb", 3, probe_fb(3, {1}));
  return 0;
}
