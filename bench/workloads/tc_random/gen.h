// Copyright 2026, Trail of Bits. All rights reserved.
//
// Knobbed, seeded random edge-stream generator for the tc_random family.
// Shared by the engine driver AND both hand-written baselines so all three
// consume byte-identical batch sequences from the same knob string (the
// determinism story lives here and nowhere else — a deviation from the
// PerfRoadmap §6.3 seed's per-family gen.py, recorded in the landing
// record: a C++ header shared by all consumers needs no file plumbing and
// no second PRNG contract).
//
// Stream shape: a SEED phase of adds-only batches growing the live edge set
// to ef% of nodes (edge factor ×100, integer knob), then a CHURN phase of
// `batches` batches of `bs` operations, each op a removal of a uniformly
// random live edge with probability rr% (else an add). No tuple is both
// added and removed within one batch (matching the suite stress driver;
// same-tuple annihilation is OQ3-verified machinery, priced by the
// phantom_pair workload, not here) — but both vectors are always SENT, so
// NetBatch runs its O(|adds|+|removes|)^2 scan every churn batch.
//
// Graph models: `uniform` picks endpoints U[0,N); `powerlaw` picks the
// TARGET by preferential attachment (endpoint of a random live edge) with
// probability 1/2, else uniform — concentrating fan-in on hubs, which is
// what builds long newest-first index chains (hazard 3) and hub cascades.

#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#include "../../common/bench.h"

namespace bench {

using Edge = std::pair<uint32_t, uint32_t>;

inline uint64_t PackEdge(uint32_t a, uint32_t b) {
  return (static_cast<uint64_t>(a) << 32u) | b;
}

// Minimal open-addressing u64 set (linear probe, 2x growth). The generator
// and baselines need exact-membership at bench scales; std::unordered_set
// is deliberately avoided in anything a baseline is charged for (COST
// honesty, design R13) and the generator keeps the same discipline.
struct U64Set {
  std::vector<uint64_t> slots;  // 0 = empty; keys are never 0 (a<<32|b with
                                // a==b==0 excluded by the generator: node 0
                                // self-loop is simply never generated).
  size_t count{0u};

  U64Set(void) : slots(64u, 0u) {}

  static uint64_t Mix(uint64_t x) {
    x ^= x >> 30u;
    x *= 0xbf58476d1ce4e5b9ull;
    x ^= x >> 27u;
    x *= 0x94d049bb133111ebull;
    x ^= x >> 31u;
    return x;
  }

  bool Contains(uint64_t key) const {
    const size_t mask = slots.size() - 1u;
    for (size_t i = Mix(key) & mask;; i = (i + 1u) & mask) {
      if (!slots[i]) {
        return false;
      }
      if (slots[i] == key) {
        return true;
      }
    }
  }

  // Insert; returns false if already present.
  bool Insert(uint64_t key) {
    if ((count + (count >> 2u)) >= slots.size()) {
      Grow();
    }
    const size_t mask = slots.size() - 1u;
    for (size_t i = Mix(key) & mask;; i = (i + 1u) & mask) {
      if (slots[i] == key) {
        return false;
      }
      if (!slots[i]) {
        slots[i] = key;
        count += 1u;
        return true;
      }
    }
  }

  // Erase by tombstone-free backward-shift is overkill here: erasure only
  // happens on live-edge removal, and the generator instead REBUILDS its
  // membership set from the live vector when tombstones would be needed.
  // Kept simple: mark-by-rebuild (see GraphGen::EraseLive).

  void Grow(void) {
    std::vector<uint64_t> old = std::move(slots);
    slots.assign(old.size() * 2u, 0u);
    count = 0u;
    for (uint64_t k : old) {
      if (k) {
        Insert(k);
      }
    }
  }
};

struct Batch {
  std::vector<Edge> adds;
  std::vector<Edge> removes;
};

struct GraphGen {
  uint32_t nodes;
  uint32_t rr_pct;    // removal probability per churn op, percent
  bool powerlaw;
  Rng rng;

  std::vector<Edge> live;  // index-addressable for uniform removal pick
  U64Set member;           // exact membership of `live`
  uint64_t erased_since_rebuild{0u};

  GraphGen(uint32_t nodes_, uint32_t rr_pct_, bool powerlaw_, uint64_t seed)
      : nodes(nodes_),
        rr_pct(rr_pct_),
        powerlaw(powerlaw_),
        rng(seed * 0x9e3779b97f4a7c15ull + 1u) {}

  Edge SampleEdge(void) {
    for (;;) {
      const uint32_t a = static_cast<uint32_t>(rng.Below(nodes));
      uint32_t b;
      if (powerlaw && !live.empty() && (rng.Next() & 1u)) {
        b = live[rng.Below(live.size())].second;  // preferential attachment
      } else {
        b = static_cast<uint32_t>(rng.Below(nodes));
      }
      if (a == 0u && b == 0u) {
        continue;  // key 0 reserved by U64Set
      }
      return {a, b};
    }
  }

  void EraseLiveAt(size_t idx) {
    live[idx] = live.back();
    live.pop_back();
    erased_since_rebuild += 1u;
    // U64Set has no tombstones: rebuild membership when erasures pile up.
    if (erased_since_rebuild * 4u > live.size() + 64u) {
      member = U64Set();
      for (const Edge &e : live) {
        member.Insert(PackEdge(e.first, e.second));
      }
      erased_since_rebuild = 0u;
    }
  }

  bool IsLive(uint64_t key) {
    if (!member.Contains(key)) {
      return false;
    }
    if (!erased_since_rebuild) {
      return true;
    }
    // Between rebuilds the set over-approximates; confirm on the vector.
    for (const Edge &e : live) {
      if (PackEdge(e.first, e.second) == key) {
        return true;
      }
    }
    return false;
  }

  // One churn batch of `ops` operations. Ops that cannot apply (removal
  // from an empty graph, add of an existing or batch-touched edge, removal
  // of a batch-touched edge) are skipped, matching the suite stress
  // driver's discipline; batch sizes are therefore <= ops, and the
  // realized sizes are what the driver reports.
  Batch Churn(uint64_t ops) {
    Batch batch;
    U64Set touched;
    for (uint64_t i = 0u; i < ops; ++i) {
      const bool removing = !live.empty() && rng.Below(100u) < rr_pct;
      if (removing) {
        const size_t idx = rng.Below(live.size());
        const Edge e = live[idx];
        const uint64_t key = PackEdge(e.first, e.second);
        if (touched.Contains(key)) {
          continue;
        }
        touched.Insert(key);
        batch.removes.push_back(e);
        EraseLiveAt(idx);
      } else {
        const Edge e = SampleEdge();
        const uint64_t key = PackEdge(e.first, e.second);
        if (touched.Contains(key) || IsLive(key)) {
          continue;
        }
        touched.Insert(key);
        batch.adds.push_back(e);
        live.push_back(e);
        member.Insert(key);
      }
    }
    return batch;
  }

  // Matched-pair (Q2, design R15) support: sample one edge that is NOT
  // currently live, without mutating any state. Deterministic from the same
  // seeded Rng as the seed/churn stream, so the engine and both baselines
  // draw the byte-identical pair sequence. The caller applies the add and
  // the matching remove (set semantics restore the pre-pair state), then
  // MarkLive / EraseLive keep this generator's `live`/`member` in lockstep
  // so the next pair samples against the same frozen surrounding graph.
  Edge SampleFreshEdge(void) {
    for (;;) {
      const Edge e = SampleEdge();
      if (!IsLive(PackEdge(e.first, e.second))) {
        return e;
      }
    }
  }

  // Mirror an applied add into the generator's live set (a pair's add leg).
  void MarkLive(const Edge &e) {
    live.push_back(e);
    member.Insert(PackEdge(e.first, e.second));
  }

  // Mirror an applied remove (a pair's del leg): find the edge and erase it,
  // restoring the exact pre-pair live set. The edge was just MarkLive'd, so
  // it is present; a linear scan is fine at matched-pair cadence (one pair
  // at a time, not a batch).
  void EraseLive(const Edge &e) {
    for (size_t i = 0u; i < live.size(); ++i) {
      if (live[i] == e) {
        EraseLiveAt(i);
        return;
      }
    }
  }

  // One seed-phase batch: adds only, up to `ops` fresh edges, stopping at
  // the target live-edge count.
  Batch Seed(uint64_t ops, uint64_t target_edges) {
    Batch batch;
    for (uint64_t i = 0u; i < ops && live.size() < target_edges; ++i) {
      const Edge e = SampleEdge();
      const uint64_t key = PackEdge(e.first, e.second);
      if (IsLive(key)) {
        continue;
      }
      batch.adds.push_back(e);
      live.push_back(e);
      member.Insert(key);
    }
    return batch;
  }
};

}  // namespace bench
