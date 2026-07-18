// Copyright 2026, Peter Goodman. All rights reserved.
//
// Shared transitive-closure machinery for the tc_random hand-written
// baselines: flat adjacency, per-source bitset closure rows, BFS with a
// bitset visited — the competent single-threaded substrate (design R13:
// no node-per-element containers anywhere a baseline is charged for).
// Closure semantics match tc.dr: tc(a,b) iff a path of length >= 1.

#pragma once

#include <cstdint>
#include <vector>

#include "../../common/bench.h"
#include "gen.h"

namespace bench {

// N x N closure as per-source bitset rows (words = ceil(N/64)).
struct Closure {
  uint32_t nodes;
  uint32_t words;
  std::vector<uint64_t> bits;  // nodes * words

  explicit Closure(uint32_t nodes_)
      : nodes(nodes_),
        words((nodes_ + 63u) / 64u),
        bits(static_cast<size_t>(nodes_) * words, 0u) {}

  uint64_t *Row(uint32_t s) {
    return bits.data() + static_cast<size_t>(s) * words;
  }

  const uint64_t *Row(uint32_t s) const {
    return bits.data() + static_cast<size_t>(s) * words;
  }

  static bool Test(const uint64_t *row, uint32_t t) {
    return (row[t >> 6u] >> (t & 63u)) & 1u;
  }

  static void Set(uint64_t *row, uint32_t t) {
    row[t >> 6u] |= 1ull << (t & 63u);
  }

  uint64_t CountPairs(void) const {
    uint64_t n = 0u;
    for (uint64_t w : bits) {
      n += static_cast<uint64_t>(__builtin_popcountll(w));
    }
    return n;
  }

  // Rows differing from `old_bits` (same geometry), in pair count.
  uint64_t DeltaPairs(const std::vector<uint64_t> &old_bits) const {
    uint64_t n = 0u;
    for (size_t i = 0u; i < bits.size(); ++i) {
      n += static_cast<uint64_t>(__builtin_popcountll(bits[i] ^ old_bits[i]));
    }
    return n;
  }

  // Canonical hash: ascending (from, to) pairs — must match the engine
  // driver's sorted-cursor FNV exactly.
  uint64_t Hash(void) const {
    Fnv fnv;
    for (uint32_t s = 0u; s < nodes; ++s) {
      const uint64_t *row = Row(s);
      for (uint32_t w = 0u; w < words; ++w) {
        uint64_t word = row[w];
        while (word) {
          const uint32_t bit = static_cast<uint32_t>(__builtin_ctzll(word));
          fnv.Add(s);
          fnv.Add((static_cast<uint32_t>(w) << 6u) + bit);
          word &= word - 1u;
        }
      }
    }
    return fnv.h;
  }
};

// Mutable adjacency: out-edge lists as flat vectors (removal = swap-pop).
struct Adjacency {
  std::vector<std::vector<uint32_t>> out;

  explicit Adjacency(uint32_t nodes) : out(nodes) {}

  void Add(uint32_t a, uint32_t b) {
    out[a].push_back(b);
  }

  void Remove(uint32_t a, uint32_t b) {
    auto &v = out[a];
    for (size_t i = 0u; i < v.size(); ++i) {
      if (v[i] == b) {
        v[i] = v.back();
        v.pop_back();
        return;
      }
    }
  }
};

// BFS from `s` over `adj`, writing reach-in->=1-step into closure row `row`
// (cleared first). `worklist` and `visited` are caller-owned scratch to
// avoid per-source allocation; `visited` must hold `words` zeroed words on
// entry and is re-zeroed (touched words only) before return.
inline void RecomputeRow(const Adjacency &adj, uint32_t s, uint64_t *row,
                         uint32_t words, std::vector<uint32_t> &worklist,
                         std::vector<uint64_t> &visited) {
  for (uint32_t w = 0u; w < words; ++w) {
    row[w] = 0u;
  }
  worklist.clear();
  for (uint32_t t : adj.out[s]) {
    if (!Closure::Test(visited.data(), t)) {
      Closure::Set(visited.data(), t);
      worklist.push_back(t);
    }
  }
  for (size_t i = 0u; i < worklist.size(); ++i) {
    const uint32_t u = worklist[i];
    for (uint32_t t : adj.out[u]) {
      if (!Closure::Test(visited.data(), t)) {
        Closure::Set(visited.data(), t);
        worklist.push_back(t);
      }
    }
  }
  for (uint32_t u : worklist) {
    Closure::Set(row, u);
  }
  for (uint32_t u : worklist) {  // re-zero touched bits only
    visited[u >> 6u] = 0u;
  }
}

// Full from-scratch closure recompute.
inline void RecomputeAll(const Adjacency &adj, Closure &closure,
                         std::vector<uint32_t> &worklist,
                         std::vector<uint64_t> &visited) {
  for (uint32_t s = 0u; s < closure.nodes; ++s) {
    RecomputeRow(adj, s, closure.Row(s), closure.words, worklist, visited);
  }
}

}  // namespace bench
