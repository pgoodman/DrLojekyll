// Copyright 2026, Peter Goodman. All rights reserved.
//
// deep_chain from-scratch baseline (the COST honesty line, design R3/R13): the
// reach set over the descending next-chain + base fact, recomputed from scratch
// per epoch by a trivial linear walk. No incrementality at all -- this is the
// honest from-scratch competitor at the deepest-chain shape, and its per-epoch
// cost is O(depth).
//
// Semantics match chain.dr: reach(k) holds iff there is a directed path in
// next from base(depth) down to k, i.e. reach = { k : 0 <= k <= depth } when
// base(depth) is present, and empty when it is retracted. The recompute walks
// the chain from the (single) live base fact following next-edges; here that
// walk is the descending run depth, depth-1, ..., 0.
//
//   clang++ -std=c++23 -O2 -DNDEBUG bench/baselines/deep_chain_naive.cpp \
//       -o deep_chain_naive
//
// Consumes the identical knob set as the engine driver and emits the same TSV
// schema, including the same final_reach_count / final_reach_hash sentinel.

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "../common/bench.h"

// Local bitset test helper (the closure.h Test is tc_random-scoped; deep_chain
// is independent of that header, so we inline the single bit-op we need).
namespace bench {
inline bool Closure_Test(const uint64_t *bits, uint32_t t) {
  return (bits[t >> 6u] >> (t & 63u)) & 1u;
}
}  // namespace bench

namespace {

// Adjacency for the next relation as flat out-edge lists (indexed by source
// node in [0, depth]). The chain has exactly one out-edge per node k>0:
// next(k, k-1). Kept as a general structure so the recompute is an honest
// graph walk rather than a closed-form shortcut.
struct Chain {
  std::vector<std::vector<uint32_t>> out;

  explicit Chain(uint32_t nodes) : out(nodes) {}

  void Add(uint32_t a, uint32_t b) { out[a].push_back(b); }
};

// From-scratch reach recompute: BFS/linear-walk from every live base fact over
// the next-chain, marking reach with a bitset visited. reach includes the base
// node itself (reach(X) : base(X)) and everything reachable downward.
// `reach_bits` is cleared and rewritten; `worklist` is caller-owned scratch.
uint64_t Recompute(const Chain &chain, const std::vector<uint32_t> &base_facts,
                   std::vector<uint64_t> &reach_bits,
                   std::vector<uint32_t> &worklist) {
  std::fill(reach_bits.begin(), reach_bits.end(), 0u);
  worklist.clear();
  for (uint32_t s : base_facts) {
    if (!bench::Closure_Test(reach_bits.data(), s)) {
      reach_bits[s >> 6u] |= 1ull << (s & 63u);
      worklist.push_back(s);
    }
  }
  for (size_t i = 0u; i < worklist.size(); ++i) {
    const uint32_t u = worklist[i];
    for (uint32_t v : chain.out[u]) {
      if (!((reach_bits[v >> 6u] >> (v & 63u)) & 1u)) {
        reach_bits[v >> 6u] |= 1ull << (v & 63u);
        worklist.push_back(v);
      }
    }
  }
  return worklist.size();
}

}  // namespace

int main(int argc, char **argv) {
  bench::Knobs knobs(argc, argv);
  const uint64_t depth = knobs.U64("depth", 100000u);
  const uint64_t cycles = knobs.U64("cycles", 20u);
  const uint64_t ingest_bs = knobs.U64("ingest_bs", 65536u);
  const uint64_t rep = knobs.U64("rep", 0u);
  const std::string mode = knobs.Str("mode", "native");
  const uint64_t block = knobs.U64("block", 1u);
  const uint64_t canary = knobs.U64("canary", 0u);
  knobs.Finish();

  // Same canonical knob key as the engine driver so the two rows join. alloc is
  // fixed at malloc for the baseline (no arena option), matching the engine's
  // alloc=malloc key form.
  char knob_str[256];
  std::snprintf(knob_str, sizeof knob_str,
                "alloc=malloc,block=%llu,canary=%llu,cycles=%llu,depth=%llu,"
                "ingest_bs=%llu",
                (unsigned long long) block, (unsigned long long) canary,
                (unsigned long long) cycles, (unsigned long long) depth,
                (unsigned long long) ingest_bs);
  const bench::Tsv tsv{"deep_chain_naive", knob_str, mode, rep};

  tsv.Row(-1, "clock_overhead_ns", bench::ClockOverheadNs());

  // Nodes are the keys 0..depth inclusive (base(depth) and reach down to 0).
  const uint32_t nodes = static_cast<uint32_t>(depth + 1u);
  const uint32_t words = (nodes + 63u) / 64u;
  Chain chain(nodes);
  std::vector<uint64_t> reach_bits(words, 0u);
  std::vector<uint32_t> worklist;
  worklist.reserve(nodes);

  // ---- seed phase: ingest the descending next-chain in ingest_bs batches ----
  // Ingestion is charged identically to the engine (per cost-note R13): the
  // baseline builds its adjacency incrementally as the batches arrive. No reach
  // is computed during ingest (base is not yet present), matching the engine's
  // seed phase which only accumulates chain edges.
  uint64_t seed_epochs = 0u;
  uint64_t seed_wall = 0u;
  for (uint64_t start = 0u; start < depth; start += ingest_bs) {
    const uint64_t stop = std::min(start + ingest_bs, depth);
    const uint64_t t0 = bench::NowNs();
    for (uint64_t k = start; k < stop; ++k) {
      chain.Add(static_cast<uint32_t>(k + 1u), static_cast<uint32_t>(k));
    }
    seed_wall += bench::NowNs() - t0;
    seed_epochs += 1u;
  }
  tsv.Row(-1, "seed_epochs", seed_epochs);
  tsv.Row(-1, "seed_wall_ns", seed_wall);
  tsv.Row(-1, "seed_edges", depth);

  // ---- initial base(depth) add: full from-scratch reach computation ----
  std::vector<uint32_t> base_facts;
  {
    base_facts.push_back(static_cast<uint32_t>(depth));
    const uint64_t t0 = bench::NowNs();
    Recompute(chain, base_facts, reach_bits, worklist);
    tsv.Row(-1, "init_add_wall_ns", bench::NowNs() - t0);
  }

  // ---- churn phase: per cycle, retract base(depth) then re-add it ----
  // Each epoch is a full from-scratch recompute (the honest naive competitor):
  // the retract recomputes reach over an empty base-fact set (empty reach); the
  // reseed recomputes over the restored base fact (full chain).
  uint64_t retract_block_wall = 0u;
  uint64_t reseed_block_wall = 0u;
  const std::vector<uint32_t> empty_base;
  for (uint64_t c = 0u; c < cycles; ++c) {
    const int64_t epoch = static_cast<int64_t>(c);

    // Retract: reach over an empty base fact set.
    {
      const uint64_t t0 = bench::NowNs();
      Recompute(chain, empty_base, reach_bits, worklist);
      const uint64_t dt = bench::NowNs() - t0;
      if (block <= 1u) {
        tsv.Row(epoch, "retract_wall_ns", dt);
      } else {
        retract_block_wall += dt;
        if ((c + 1u) % block == 0u) {
          tsv.Row(epoch, "retract_block_wall_ns", retract_block_wall);
          retract_block_wall = 0u;
        }
      }
    }

    // Reseed: reach over the restored base fact.
    {
      const uint64_t t0 = bench::NowNs();
      Recompute(chain, base_facts, reach_bits, worklist);
      const uint64_t dt = bench::NowNs() - t0;
      if (block <= 1u) {
        tsv.Row(epoch, "reseed_wall_ns", dt);
      } else {
        reseed_block_wall += dt;
        if ((c + 1u) % block == 0u) {
          tsv.Row(epoch, "reseed_block_wall_ns", reseed_block_wall);
          reseed_block_wall = 0u;
        }
      }
    }

    if (canary && ((c + 1u) % canary == 0u)) {
      tsv.Row(epoch, "canary_wall_ns", bench::CanaryWallNs());
    }
  }

  // ---- sentinel (clock stopped) ----
  // reach_bits currently holds the final (re-added) reach set. Drain ascending,
  // count, FNV-hash -- must match the engine's final_reach_count/hash.
  {
    std::vector<uint64_t> vals;
    for (uint32_t w = 0u; w < words; ++w) {
      uint64_t word = reach_bits[w];
      while (word) {
        const uint32_t bit = static_cast<uint32_t>(__builtin_ctzll(word));
        vals.push_back((static_cast<uint64_t>(w) << 6u) + bit);
        word &= word - 1u;
      }
    }
    std::sort(vals.begin(), vals.end());
    bench::Fnv fnv;
    for (uint64_t v : vals) {
      fnv.Add(v);
    }
    tsv.Row(-1, "final_reach_count", vals.size());
    tsv.Row(-1, "final_reach_hash", fnv.h);
  }

  tsv.Row(-1, "peak_rss_bytes", bench::PeakRssBytes());
  tsv.Complete();
  return 0;
}
