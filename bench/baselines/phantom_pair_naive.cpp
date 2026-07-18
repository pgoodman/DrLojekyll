// Copyright 2026, Peter Goodman. All rights reserved.
//
// phantom_pair from-scratch baseline (the COST honesty line, design R3/R13):
// a persistent live-edge set mutated as the same phantom-pair stream arrives
// (edge ingestion charged identically to the engine), with the two-hop join
// h = e JOIN e recomputed from scratch per batch. No incrementality in h.
//
//   clang++ -std=c++23 -O2 -DNDEBUG bench/baselines/phantom_pair_naive.cpp \
//       -o phantom_pair_naive
//
// Consumes the identical knob set / edge stream as the engine driver
// (bench/workloads/flip_storm/phantom_gen.h) and emits the same TSV schema,
// including the final_h_count / final_h_hash sentinel the runner
// cross-checks against every engine mode (canonical FNV over ascending
// (X,Z) pairs), plus per-epoch delta_h_rows for work-per-changed-tuple
// normalization (design R3). h is the SET of (X,Z) with some Y s.t. e(X,Y)
// and e(Y,Z) both live -- SET semantics, matching the engine's relation.

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "../common/bench.h"
#include "../workloads/flip_storm/phantom_gen.h"

namespace {

// Recompute h = e JOIN e over the current live edge mirror as a sorted,
// deduplicated vector of (X,Z) pairs. out[a] gives a's live targets; for
// each live edge (X,Y) and each live (Y,Z), emit (X,Z). Dedup by sort+unique
// (SET semantics: h(X,Z) present iff at least one witnessing Y).
std::vector<std::pair<uint32_t, uint32_t>>
RecomputeH(const bench::PhantomGen &gen) {
  std::vector<std::pair<uint32_t, uint32_t>> h;
  for (const auto &[x, y] : gen.live) {
    for (uint32_t z : gen.out[y]) {
      h.push_back({x, z});
    }
  }
  std::sort(h.begin(), h.end());
  h.erase(std::unique(h.begin(), h.end()), h.end());
  return h;
}

// |symmetric difference| of two sorted pair sets (the |Delta h| per epoch).
uint64_t DeltaPairs(const std::vector<std::pair<uint32_t, uint32_t>> &a,
                    const std::vector<std::pair<uint32_t, uint32_t>> &b) {
  uint64_t d = 0u;
  size_t i = 0u, j = 0u;
  while (i < a.size() && j < b.size()) {
    if (a[i] == b[j]) {
      ++i;
      ++j;
    } else if (a[i] < b[j]) {
      ++d;
      ++i;
    } else {
      ++d;
      ++j;
    }
  }
  d += (a.size() - i) + (b.size() - j);
  return d;
}

}  // namespace

int main(int argc, char **argv) {
  bench::Knobs knobs(argc, argv);
  // Defaults match the engine driver's (dense enough to exercise phantoms).
  const uint64_t nodes = knobs.U64("nodes", 256u);
  const uint64_t m = knobs.U64("m", 32u);
  const uint64_t batches = knobs.U64("batches", 100u);
  const uint64_t warm_edges = knobs.U64("warm_edges", 2048u);
  const uint64_t seed = knobs.U64("seed", 1u);
  const uint64_t rep = knobs.U64("rep", 0u);
  const std::string mode = knobs.Str("mode", "native");
  const uint64_t block = knobs.U64("block", 1u);
  knobs.Finish();

  // Same canonical key layout as the engine driver, minus engine-only knobs
  // (alloc, canary) which do not affect the stream or the sentinel. The
  // runner cross-checks sentinels on the shared measurement-affecting knobs
  // (nodes, m, batches, warm_edges, seed).
  char knob_str[256];
  std::snprintf(knob_str, sizeof knob_str,
                "batches=%llu,block=%llu,m=%llu,nodes=%llu,seed=%llu,"
                "warm_edges=%llu",
                (unsigned long long) batches, (unsigned long long) block,
                (unsigned long long) m, (unsigned long long) nodes,
                (unsigned long long) seed, (unsigned long long) warm_edges);
  const bench::Tsv tsv{"phantom_pair_naive", knob_str, mode, rep};

  tsv.Row(-1, "clock_overhead_ns", bench::ClockOverheadNs());

  bench::PhantomGen gen(static_cast<uint32_t>(nodes), seed);

  // ---- warm phase (mirrors the engine driver's) ----
  {
    bench::Batch batch = gen.Warm(warm_edges);
    const uint64_t t0 = bench::NowNs();
    (void) RecomputeH(gen);  // charge one from-scratch join, as the engine
                             // materializes h at warm.
    tsv.Row(-1, "warm_wall_ns", bench::NowNs() - t0);
    tsv.Row(-1, "warm_edges_live", gen.live.size());
  }
  std::vector<std::pair<uint32_t, uint32_t>> prev = RecomputeH(gen);

  // ---- churn phase ----
  uint64_t block_wall = 0u;
  for (uint64_t b = 0u; b < batches; ++b) {
    bench::Batch batch = gen.Churn(m);
    // Mutation is already applied inside Churn (it maintains gen's mirror);
    // to charge ingestion identically we time the from-scratch recompute of
    // h over the post-batch mirror.
    const uint64_t t0 = bench::NowNs();
    std::vector<std::pair<uint32_t, uint32_t>> h = RecomputeH(gen);
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
    tsv.Row(epoch, "batch_adds", batch.adds.size());
    tsv.Row(epoch, "batch_removes", batch.removes.size());
    tsv.Row(epoch, "delta_h_rows", DeltaPairs(prev, h));
    prev = std::move(h);
  }

  // ---- sentinel (clock stopped) ----
  // RecomputeH returns pairs already sorted ascending and deduplicated, so
  // the canonical FNV feeds them directly (matching the engine driver's
  // sorted-cursor order).
  std::vector<std::pair<uint32_t, uint32_t>> h = RecomputeH(gen);
  bench::Fnv fnv;
  for (const auto &[x, z] : h) {
    fnv.Add(x);
    fnv.Add(z);
  }
  tsv.Row(-1, "final_h_count", h.size());
  tsv.Row(-1, "final_h_hash", fnv.h);
  tsv.Row(-1, "final_live_edges", gen.live.size());
  tsv.Row(-1, "peak_rss_bytes", bench::PeakRssBytes());
  tsv.Complete();
  return 0;
}
