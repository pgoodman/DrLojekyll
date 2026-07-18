// Copyright 2026, Peter Goodman. All rights reserved.
//
// pure_cycle from-scratch baseline (the COST honesty line, design R3/R13):
// the SAME planted-ring stream as the pure_cycle engine driver, replayed
// against a persistent adjacency structure with the FULL transitive closure
// recomputed from scratch per batch (per-source BFS over flat adjacency with a
// bitset visited -- reused from tc_random/closure.h). No incrementality in the
// closure at all; this is the wave the engine does incrementally, done the
// dumb way, so the final-state hash cross-checks the engine in all 4 modes.
//
//   clang++ -std=c++23 -O2 -DNDEBUG bench/baselines/pure_cycle_naive.cpp \
//       -o pure_cycle_naive
//
// Ring layout and churn schedule are identical to the engine driver:
// ring r occupies [r*k, (r+1)*k); edge index i of ring r is
// r*k + i -> r*k + ((i+1) mod k). Even batch b removes edge index (b/2 mod k)
// of every ring; odd b re-adds them.

#include <cstdint>
#include <string>
#include <vector>

#include "../common/bench.h"
#include "../workloads/tc_random/closure.h"

namespace {

struct Edge {
  uint32_t from;
  uint32_t to;
};

inline Edge RingEdge(uint64_t r, uint64_t k, uint64_t i) {
  const uint64_t base = r * k;
  return {static_cast<uint32_t>(base + i),
          static_cast<uint32_t>(base + ((i + 1u) % k))};
}

uint64_t AnalyticCount(uint64_t rings, uint64_t k, bool all_intact) {
  return all_intact ? rings * k * k : rings * (k * (k - 1u) / 2u);
}

}  // namespace

int main(int argc, char **argv) {
  bench::Knobs knobs(argc, argv);
  const uint64_t rings = knobs.U64("rings", 8u);
  const uint64_t k = knobs.U64("k", 32u);
  const uint64_t batches = knobs.U64("batches", 40u);
  const uint64_t rep = knobs.U64("rep", 0u);
  const std::string mode = knobs.Str("mode", "native");
  const uint64_t block = knobs.U64("block", 1u);
  const uint64_t verify = knobs.U64("verify", 0u);
  knobs.Finish();

  if (k < 2u || rings < 1u) {
    std::fprintf(stderr, "pure_cycle requires k>=2 and rings>=1\n");
    return 2;
  }

  // Canonical key must match the engine driver's exactly (minus alloc/canary,
  // which are engine-only measurement knobs, but the cross-check joins on the
  // SUBSET of shared knobs -- keep batches,k,rings,verify,block identical).
  char knob_str[256];
  std::snprintf(knob_str, sizeof knob_str,
                "batches=%llu,block=%llu,k=%llu,rings=%llu,verify=%llu",
                (unsigned long long) batches, (unsigned long long) block,
                (unsigned long long) k, (unsigned long long) rings,
                (unsigned long long) verify);
  const bench::Tsv tsv{"pure_cycle_naive", knob_str, mode, rep};

  tsv.Row(-1, "clock_overhead_ns", bench::ClockOverheadNs());

  const uint32_t n32 = static_cast<uint32_t>(rings * k);
  bench::Adjacency adj(n32);
  bench::Closure closure(n32);
  std::vector<uint64_t> prev_bits(closure.bits.size(), 0u);
  std::vector<uint32_t> worklist;
  std::vector<uint64_t> visited(closure.words, 0u);

  // ---- seed phase: all ring edges, one batch per edge index ----
  uint64_t seed_epochs = 0u;
  uint64_t seed_wall = 0u;
  for (uint64_t i = 0u; i < k; ++i) {
    const uint64_t t0 = bench::NowNs();
    for (uint64_t r = 0u; r < rings; ++r) {
      const Edge e = RingEdge(r, k, i);
      adj.Add(e.from, e.to);
    }
    bench::RecomputeAll(adj, closure, worklist, visited);
    seed_wall += bench::NowNs() - t0;
    seed_epochs += 1u;
  }
  tsv.Row(-1, "seed_epochs", seed_epochs);
  tsv.Row(-1, "seed_wall_ns", seed_wall);
  tsv.Row(-1, "seed_edges", rings * k);
  {
    const uint64_t got = closure.CountPairs();
    tsv.Row(-1, "seed_tc_count", got);
    tsv.Row(-1, "seed_tc_expect", AnalyticCount(rings, k, /*intact=*/true));
    if (verify && got != AnalyticCount(rings, k, true)) {
      tsv.Row(-1, "SENTINEL_FAIL", 1u);
      tsv.Complete();
      return 1;
    }
  }
  prev_bits = closure.bits;

  // ---- churn phase ----
  uint64_t block_wall = 0u;
  for (uint64_t b = 0u; b < batches; ++b) {
    const bool removing = (b % 2u) == 0u;
    const uint64_t idx = (b / 2u) % k;

    const uint64_t t0 = bench::NowNs();
    for (uint64_t r = 0u; r < rings; ++r) {
      const Edge e = RingEdge(r, k, idx);
      if (removing) {
        adj.Remove(e.from, e.to);
      } else {
        adj.Add(e.from, e.to);
      }
    }
    bench::RecomputeAll(adj, closure, worklist, visited);
    const uint64_t dt = bench::NowNs() - t0;

    const int64_t epoch = static_cast<int64_t>(b);
    if (block <= 1u) {
      tsv.Row(epoch, "epoch_wall_ns", dt);
    } else {
      block_wall += dt;
      if ((b + 1u) % block == 0u) {
        tsv.Row(epoch, "block_wall_ns", block_wall);
        block_wall = 0u;
      }
    }
    const uint64_t adds_n = removing ? 0u : rings;
    const uint64_t removes_n = removing ? rings : 0u;
    tsv.Row(epoch, "batch_adds", adds_n);
    tsv.Row(epoch, "batch_removes", removes_n);
    tsv.Row(epoch, "delta_tc_rows", closure.DeltaPairs(prev_bits));
    prev_bits = closure.bits;

    const bool all_intact = !removing;
    if (verify) {
      const uint64_t got = closure.CountPairs();
      const uint64_t expect = AnalyticCount(rings, k, all_intact);
      tsv.Row(epoch, "verify_tc_count", got);
      tsv.Row(epoch, "verify_tc_expect", expect);
      if (got != expect) {
        tsv.Row(epoch, "SENTINEL_FAIL", 1u);
        tsv.Complete();
        return 1;
      }
    }
  }

  // ---- sentinel (clock stopped) ----
  const bool last_removed = ((batches - 1u) % 2u) == 0u;
  const bool all_intact = (batches == 0u) ? true : !last_removed;
  tsv.Row(-1, "final_tc_count", closure.CountPairs());
  tsv.Row(-1, "final_tc_hash", closure.Hash());
  tsv.Row(-1, "final_tc_expect", AnalyticCount(rings, k, all_intact));
  tsv.Row(-1, "final_live_edges", all_intact ? rings * k : rings * (k - 1u));
  tsv.Row(-1, "peak_rss_bytes", bench::PeakRssBytes());
  tsv.Complete();
  return 0;
}
