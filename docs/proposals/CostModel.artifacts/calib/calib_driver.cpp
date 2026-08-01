// Copyright 2026, Peter Goodman. All rights reserved.
//
// Parametric calibration driver for the monotone non-recursive identity-join
// demand witness (demand_neighborhood_mono_witness.dr:
// `neighborhood(bound Start, free Node) : edge(Start, Node)`). It is compiled
// TWICE against the same source -- once against the recognizer-ON codegen
// (default `-demand`, the query-projection guard join.7 dropped) and once
// against the recognizer-OFF codegen (`-opt-disable=df.ident_join`, join.7
// retained). Both are built `-DDRLOJEKYLL_BENCH_COUNTERS`. The driver snapshots
// `hyde::rt::gBenchCounters` around a K-probe demand epoch ONLY (ingest is
// excluded), then prints every counter's delta as a machine-readable line so a
// runner can diff ON vs OFF. This makes the `measured-calibration-1.md` table
// reproducible from tracked source (cost-model-findings.md #7) and is the
// MEASUREMENT half of the identity-join vertical slice: it reports observed
// counters, it does not itself statically predict them.
//
// Input model (argv): N F K DUP [REPEAT]
//   N      distinct source keys loaded: key s in [0, N) has F out-edges.
//   F      fanout: source s points to targets kTargetBase + s*F + j,
//          j in [0, F). The target space is disjoint from the source-key space
//          [0, N), so a probe answer is EXACTLY {kTargetBase + s*F + j} and any
//          leak is caught.
//   K      distinct demand probes: keys s in [0, K). A probe of s >= N is a
//          standing-but-empty demand (no out-edges) and must drain to nothing.
//   DUP    each edge is ingested DUP times (>=1) to exercise set/distinct-row
//          semantics: the live neighborhood is F distinct rows regardless of DUP.
//   REPEAT (optional, default 1) how many times EACH key is probed inside the
//          measured region. REPEAT>1 exercises demand idempotence: the standing
//          instance from the first probe means later probes of the same key
//          re-read but re-materialize nothing, so the probe-epoch idx_adds law
//          stays F*min(N,K) -- it does NOT scale with REPEAT.
//
// The probe loop asserts each drained answer is exactly the expected set (the
// HP-5 discipline), so an over/under-materialized arm aborts here before any
// counter is trusted.

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include <drlojekyll/Runtime/BenchCounters.h>

#include "datalog.h"

namespace {

// Disjoint from the source-key space so a probe answer cannot alias a key.
constexpr uint64_t kTargetBase = 1'000'000'000ull;

}  // namespace

int main(int argc, char **argv) {
  if (argc != 5 && argc != 6) {
    std::fprintf(stderr, "usage: %s N F K DUP [REPEAT]\n", argv[0]);
    return 2;
  }
  const uint64_t N = std::strtoull(argv[1], nullptr, 10);
  const uint64_t F = std::strtoull(argv[2], nullptr, 10);
  const uint64_t K = std::strtoull(argv[3], nullptr, 10);
  const uint64_t DUP = std::strtoull(argv[4], nullptr, 10);
  const uint64_t REPEAT = (argc == 6) ? std::strtoull(argv[5], nullptr, 10) : 1u;
  assert(DUP >= 1u);
  assert(REPEAT >= 1u);

  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  // Ingest: N source keys, each with F distinct out-edges, each edge added DUP
  // times. This happens BEFORE the measured region so ingest cost is excluded.
  {
    hyde::rt::Vec<add_edge_input> edges(allocator);
    for (uint64_t s = 0; s < N; ++s) {
      for (uint64_t j = 0; j < F; ++j) {
        const uint64_t to = kTargetBase + s * F + j;
        for (uint64_t d = 0; d < DUP; ++d) {
          edges.Add({s, to});
        }
      }
    }
    add_edge_2(db, log, functors, std::move(edges));
  }

  // Snapshot the counters around the K-probe demand epoch ONLY.
  const hyde::rt::BenchCounters before = hyde::rt::gBenchCounters;

  uint64_t total_drained = 0;
  for (uint64_t r = 0; r < REPEAT; ++r) {
    for (uint64_t s = 0; s < K; ++s) {
      std::vector<uint64_t> got;
      auto c = neighborhood_bf(db, log, functors, s);
      for (uint64_t n = 0; c.next(n);) {
        got.push_back(n);
      }
      std::sort(got.begin(), got.end());

      std::vector<uint64_t> expected;
      if (s < N) {
        for (uint64_t j = 0; j < F; ++j) {
          expected.push_back(kTargetBase + s * F + j);
        }
      }
      // HP-5: exactly neighborhood(s), no over/under-materialization.
      assert(got == expected);
      total_drained += got.size();
    }
  }

  const hyde::rt::BenchCounters after = hyde::rt::gBenchCounters;

  // Machine-readable output: a PARAMS line, then one COUNTER line per field
  // with its delta across the probe epoch. `CONFIG` is supplied by the build so
  // the runner can label ON vs OFF without guessing.
  std::printf("PARAMS N=%llu F=%llu K=%llu DUP=%llu REPEAT=%llu drained=%llu "
              "config=%s\n",
              (unsigned long long) N, (unsigned long long) F,
              (unsigned long long) K, (unsigned long long) DUP,
              (unsigned long long) REPEAT, (unsigned long long) total_drained,
#ifdef CALIB_CONFIG
              CALIB_CONFIG
#else
              "unknown"
#endif
  );
#define HYDE_RT_PRINT_DELTA(field) \
  std::printf("COUNTER %s %llu\n", #field, \
              (unsigned long long) (after.field - before.field));
  HYDE_RT_BENCH_COUNTER_FIELDS(HYDE_RT_PRINT_DELTA)
#undef HYDE_RT_PRINT_DELTA
  return 0;
}
