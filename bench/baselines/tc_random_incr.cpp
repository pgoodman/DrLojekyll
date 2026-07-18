// Copyright 2026, Peter Goodman. All rights reserved.
//
// tc_random hand-incremental baseline: what a performance-minded engineer
// maintaining a transitive closure by hand would actually write (design
// R4 — the competent incremental opponent; DRed-by-hand was rejected as
// reimplementing the measured engine).
//
// Adds (exact single-edge closure update, Italiano-style): for new edge
// (a,b), new pairs are {(s,t) : (s==a || old(s,a)) && (t==b || old(b,t))};
// applied edge-at-a-time with row(b) snapshotted first.
//
// Deletes: strategy `bfs` (default) = affected-source recompute — only
// rows whose OLD closure reaches a deleted edge's tail are recomputed by
// BFS over the updated adjacency. Strategy `full` = any delete batch
// triggers a full from-scratch recompute (the labeled upper-bound control,
// design R4: its delete cost IS the from-scratch cost).
//
//   clang++ -std=c++23 -O2 -DNDEBUG bench/baselines/tc_random_incr.cpp \
//       -o tc_random_incr
//
// Same knobs / stream / TSV schema as the engine driver and _naive.

#include <cstdint>
#include <string>
#include <vector>

#include "../common/bench.h"
#include "../workloads/tc_random/closure.h"
#include "../workloads/tc_random/gen.h"

namespace {

// Exact closure update for one inserted edge (a, b).
void AddEdgeIncremental(bench::Closure &closure, uint32_t a, uint32_t b,
                        std::vector<uint64_t> &scratch_r) {
  const uint32_t words = closure.words;
  // Snapshot R = row(b) | {b} before touching any row (b may reach a).
  const uint64_t *row_b = closure.Row(b);
  scratch_r.assign(row_b, row_b + words);
  bench::Closure::Set(scratch_r.data(), b);
  for (uint32_t s = 0u; s < closure.nodes; ++s) {
    uint64_t *row_s = closure.Row(s);
    if (s == a || bench::Closure::Test(row_s, a)) {
      for (uint32_t w = 0u; w < words; ++w) {
        row_s[w] |= scratch_r[w];
      }
    }
  }
}

}  // namespace

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
  const std::string del = knobs.Str("del", "bfs");  // bfs | full
  const uint64_t pairs = knobs.U64("pairs", 0u);
  knobs.Finish();

  char knob_str[256];
  std::snprintf(knob_str, sizeof knob_str,
                "alloc=malloc,batches=%llu,bs=%llu,del=%s,ef=%llu,graph=%s,"
                "nodes=%llu,pairs=%llu,rr=%llu,seed=%llu",
                (unsigned long long) batches, (unsigned long long) bs,
                del.c_str(), (unsigned long long) ef, graph.c_str(),
                (unsigned long long) nodes, (unsigned long long) pairs,
                (unsigned long long) rr, (unsigned long long) seed);
  const bench::Tsv tsv{"tc_random_incr", knob_str, mode, rep};

  tsv.Row(-1, "clock_overhead_ns", bench::ClockOverheadNs());

  const uint32_t n32 = static_cast<uint32_t>(nodes);
  bench::GraphGen gen(n32, static_cast<uint32_t>(rr), graph == "powerlaw",
                      seed);
  bench::Adjacency adj(n32);
  bench::Closure closure(n32);
  std::vector<uint64_t> prev_bits(closure.bits.size(), 0u);
  std::vector<uint32_t> worklist;
  std::vector<uint64_t> visited(closure.words, 0u);
  std::vector<uint64_t> scratch_r(closure.words, 0u);
  std::vector<uint32_t> affected;

  // ---- seed phase: incremental adds (this baseline's own discipline) ----
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
      AddEdgeIncremental(closure, a, b, scratch_r);
    }
    seed_wall += bench::NowNs() - t0;
    seed_epochs += 1u;
  }
  tsv.Row(-1, "seed_epochs", seed_epochs);
  tsv.Row(-1, "seed_wall_ns", seed_wall);
  tsv.Row(-1, "seed_edges", gen.live.size());
  prev_bits = closure.bits;

  // ---- churn phase (pairs=0) OR matched-pair phase (pairs>0) ----
  if (pairs == 0u) {
    uint64_t block_wall = 0u;
    for (uint64_t b = 0u; b < batches; ++b) {
      bench::Batch batch = gen.Churn(bs);

      const uint64_t t0 = bench::NowNs();

      // Deletes first: collect the affected-source superset against the OLD
      // closure, update adjacency, then recompute exactly those rows.
      if (!batch.removes.empty()) {
        if (del == "full") {
          for (const auto &[x, y] : batch.removes) {
            adj.Remove(x, y);
          }
          bench::RecomputeAll(adj, closure, worklist, visited);
        } else {
          affected.clear();
          for (uint32_t s = 0u; s < n32; ++s) {
            const uint64_t *row = closure.Row(s);
            for (const auto &[x, y] : batch.removes) {
              if (s == x || bench::Closure::Test(row, x)) {
                affected.push_back(s);
                break;
              }
            }
          }
          for (const auto &[x, y] : batch.removes) {
            adj.Remove(x, y);
          }
          for (uint32_t s : affected) {
            bench::RecomputeRow(adj, s, closure.Row(s), closure.words,
                                worklist, visited);
          }
        }
      }
      for (const auto &[x, y] : batch.adds) {
        adj.Add(x, y);
        AddEdgeIncremental(closure, x, y, scratch_r);
      }

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
  } else {
    // ---- matched-pair phase (Q2, design R15) ----
    // Frozen surrounding graph = post-seed closure (prev_bits). Each pair i
    // samples a fresh non-live edge e_i: add it via the exact single-edge
    // update (pair_add_wall_ns), record |Δtc| of the add against the frozen
    // closure (pair_delta_tc), then delete it via this baseline's delete
    // strategy (pair_del_wall_ns). Set-semantics-equivalent: the incremental
    // delete recomputes the affected rows from the current adjacency, which
    // (adjacency restored) reproduces the frozen closure exactly.
    tsv.Row(-1, "state_tc_count_pre_pairs", closure.CountPairs());
    tsv.Row(-1, "state_hash_pre_pairs", closure.Hash());

    for (uint64_t i = 0u; i < pairs; ++i) {
      const bench::Edge e = gen.SampleFreshEdge();
      const int64_t epoch = static_cast<int64_t>(i);

      // add leg (exact single-edge incremental update).
      const uint64_t ta = bench::NowNs();
      adj.Add(e.first, e.second);
      AddEdgeIncremental(closure, e.first, e.second, scratch_r);
      tsv.Row(epoch, "pair_add_wall_ns", bench::NowNs() - ta);
      tsv.Row(epoch, "pair_delta_tc", closure.DeltaPairs(prev_bits));
      gen.MarkLive(e);

      // del leg (this baseline's delete strategy for one edge).
      const uint64_t td = bench::NowNs();
      if (del == "full") {
        adj.Remove(e.first, e.second);
        bench::RecomputeAll(adj, closure, worklist, visited);
      } else {
        affected.clear();
        for (uint32_t s = 0u; s < n32; ++s) {
          const uint64_t *row = closure.Row(s);
          if (s == e.first || bench::Closure::Test(row, e.first)) {
            affected.push_back(s);
          }
        }
        adj.Remove(e.first, e.second);
        for (uint32_t s : affected) {
          bench::RecomputeRow(adj, s, closure.Row(s), closure.words, worklist,
                              visited);
        }
      }
      tsv.Row(epoch, "pair_del_wall_ns", bench::NowNs() - td);
      gen.EraseLive(e);
    }
  }

  // ---- sentinel (clock stopped) ----
  tsv.Row(-1, "final_tc_count", closure.CountPairs());
  tsv.Row(-1, "final_tc_hash", closure.Hash());
  tsv.Row(-1, "final_live_edges", gen.live.size());
  tsv.Row(-1, "peak_rss_bytes", bench::PeakRssBytes());
  tsv.Complete();
  return 0;
}
