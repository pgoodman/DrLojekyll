// Copyright 2026, Trail of Bits. All rights reserved.
//
// tc_random from-scratch baseline (the COST honesty line, design R3/R13):
// a persistent adjacency structure mutated as edges arrive (edge ingestion
// charged identically to the engine), with the FULL transitive closure
// recomputed from scratch per batch — per-source BFS over flat adjacency
// with a bitset visited. No incrementality in the closure at all.
//
//   clang++ -std=c++23 -O2 -DNDEBUG bench/baselines/tc_random_naive.cpp \
//       -o tc_random_naive
//
// Consumes the identical knob set / edge stream as the engine driver
// (bench/workloads/tc_random/gen.h) and emits the same TSV schema,
// including per-epoch delta_tc_rows (the |Δtc| the engine cannot cheaply
// report; joined on epoch downstream for work-per-changed-tuple
// normalization, design R3).

#include <cstdint>
#include <string>
#include <vector>

#include "../common/bench.h"
#include "../workloads/tc_random/closure.h"
#include "../workloads/tc_random/gen.h"

int main(int argc, char **argv) {
  bench::Knobs knobs(argc, argv);
  const uint64_t nodes = knobs.U64("nodes", 1000u);
  const uint64_t ef = knobs.U64("ef", 200u);
  const uint64_t batches = knobs.U64("batches", 200u);
  const uint64_t bs = knobs.U64("bs", 64u);
  const uint64_t rr = knobs.U64("rr", 30u);
  const std::string graph = knobs.Str("graph", "uniform");
  const uint64_t seed = knobs.U64("seed", 1u);
  const uint64_t rep = knobs.U64("rep", 0u);
  const std::string mode = knobs.Str("mode", "native");
  const uint64_t block = knobs.U64("block", 1u);
  knobs.Finish();

  char knob_str[256];
  std::snprintf(knob_str, sizeof knob_str,
                "alloc=malloc,batches=%llu,bs=%llu,ef=%llu,graph=%s,"
                "nodes=%llu,rr=%llu,seed=%llu",
                (unsigned long long) batches, (unsigned long long) bs,
                (unsigned long long) ef, graph.c_str(),
                (unsigned long long) nodes, (unsigned long long) rr,
                (unsigned long long) seed);
  const bench::Tsv tsv{"tc_random_naive", knob_str, mode, rep};

  tsv.Row(-1, "clock_overhead_ns", bench::ClockOverheadNs());

  const uint32_t n32 = static_cast<uint32_t>(nodes);
  bench::GraphGen gen(n32, static_cast<uint32_t>(rr), graph == "powerlaw",
                      seed);
  bench::Adjacency adj(n32);
  bench::Closure closure(n32);
  std::vector<uint64_t> prev_bits(closure.bits.size(), 0u);
  std::vector<uint32_t> worklist;
  std::vector<uint64_t> visited(closure.words, 0u);

  // ---- seed phase (mirrors the engine driver's) ----
  const uint64_t target_edges = nodes * ef / 100u;
  uint64_t seed_epochs = 0u;
  uint64_t seed_wall = 0u;
  while (gen.live.size() < target_edges) {
    bench::Batch batch = gen.Seed(bs, target_edges);
    if (batch.adds.empty()) {
      break;
    }
    const uint64_t t0 = bench::NowNs();
    for (const auto &[a, b] : batch.adds) {
      adj.Add(a, b);
    }
    bench::RecomputeAll(adj, closure, worklist, visited);
    seed_wall += bench::NowNs() - t0;
    seed_epochs += 1u;
  }
  tsv.Row(-1, "seed_epochs", seed_epochs);
  tsv.Row(-1, "seed_wall_ns", seed_wall);
  tsv.Row(-1, "seed_edges", gen.live.size());
  prev_bits = closure.bits;

  // ---- churn phase ----
  uint64_t block_wall = 0u;
  for (uint64_t b = 0u; b < batches; ++b) {
    bench::Batch batch = gen.Churn(bs);

    const uint64_t t0 = bench::NowNs();
    for (const auto &[x, y] : batch.adds) {
      adj.Add(x, y);
    }
    for (const auto &[x, y] : batch.removes) {
      adj.Remove(x, y);
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
    tsv.Row(epoch, "batch_adds", batch.adds.size());
    tsv.Row(epoch, "batch_removes", batch.removes.size());
    tsv.Row(epoch, "delta_tc_rows", closure.DeltaPairs(prev_bits));
    prev_bits = closure.bits;
  }

  // ---- sentinel (clock stopped) ----
  tsv.Row(-1, "final_tc_count", closure.CountPairs());
  tsv.Row(-1, "final_tc_hash", closure.Hash());
  tsv.Row(-1, "final_live_edges", gen.live.size());
  tsv.Row(-1, "peak_rss_bytes", bench::PeakRssBytes());
  tsv.Complete();
  return 0;
}
