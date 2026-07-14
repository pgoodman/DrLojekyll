// Copyright 2026, Trail of Bits. All rights reserved.
//
// Knobbed, seeded phantom-pair edge-stream generator for the flip_storm
// family. Shared by the engine driver AND the hand-written baseline so both
// consume byte-identical batch sequences from the same knob string (the
// determinism story lives here and nowhere else, mirroring tc_random/gen.h).
//
// The relation is e(X,Y), a @differential edge message; the derived relation
// is the two-hop join h(X,Z) : e(X,Y), e(Y,Z). A WARM phase seeds
// warm_edges random e edges (adds only). The CHURN phase then emits, per
// batch, `m` phantom INSTANCES, each a mixed same-batch pair:
//
//     pick a live 2-path  X -> Y -> Z   (e(X,Y) and e(Y,Z) both live)
//     REMOVE e(Y,Z)   and   ADD a fresh e(W,Y)   in the same message
//
// The pairing of the added hop (W -> Y) with the removed hop (Y -> Z) would
// synthesize h(W,Z) -- a "phantom" instance in NEITHER the pre-batch nor the
// post-batch materialization (before: W->Y absent; after: Y->Z absent). The
// engine's signed claim gates (Table.h TryClaimAdd/TryClaimDel, F17) must
// drop that transient enqueue; the counts binary's ctr_stale_drops_{add,del}
// count the drops. Removing e(Y,Z) also genuinely retracts every real h(*,Z)
// hop through Y whose other leg survives, and adding e(W,Y) genuinely
// derives h(W, Z') for every live Y->Z'. Shared h instances across the m
// instances of a batch are what force canceling -/+ folds at scale.
//
// Batch validity (matching the suite stress drivers): within one batch no
// tuple is both added and removed, removes are of live edges only, and no
// edge is touched twice. Instances that cannot find a fresh W->Y target or a
// live 2-path are skipped; realized (adds,removes) sizes are what the driver
// reports.

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

// Minimal open-addressing u64 set (linear probe, 2x growth); 0 = empty,
// keys never 0 (node 0 self-loop is never generated). Identical discipline
// to tc_random/gen.h's U64Set (no std::unordered_set on any charged path).
struct U64Set {
  std::vector<uint64_t> slots;
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

struct PhantomGen {
  uint32_t nodes;
  Rng rng;

  // Live edge mirror: index-addressable `live` for uniform 2-path sampling,
  // out-adjacency for the second hop, exact membership `member`. Erasures
  // rebuild membership when they pile up (no tombstones), mirroring
  // tc_random/gen.h's discipline. Adjacency lists over-approximate between
  // rebuilds; IsLive confirms on `member` which is authoritative.
  std::vector<Edge> live;
  std::vector<std::vector<uint32_t>> out;  // out[a] = targets of live a->*
  U64Set member;

  PhantomGen(uint32_t nodes_, uint64_t seed)
      : nodes(nodes_),
        rng(seed * 0x9e3779b97f4a7c15ull + 1u),
        out(nodes_) {}

  bool IsLive(uint32_t a, uint32_t b) const {
    return member.Contains(PackEdge(a, b));
  }

  void AddLive(uint32_t a, uint32_t b) {
    live.push_back({a, b});
    out[a].push_back(b);
    member.Insert(PackEdge(a, b));
  }

  // Remove edge a->b from live/out (a->b must be live): swap-pop from both.
  // `member` has no tombstones; between threshold rebuilds it OVER-
  // approximates by the removed keys. IsLive is used only to reject invalid
  // adds, so a stale key can cause at most a conservative skipped-add, never
  // an invalid batch (callers never re-add a same-batch-removed edge). The
  // authoritative live set is `live`/`out`, always exact; membership is
  // reconciled on the threshold rebuild.
  uint64_t erased_since_rebuild{0u};
  void RemoveLive(uint32_t a, uint32_t b) {
    for (size_t i = 0u; i < live.size(); ++i) {
      if (live[i].first == a && live[i].second == b) {
        live[i] = live.back();
        live.pop_back();
        break;
      }
    }
    auto &v = out[a];
    for (size_t i = 0u; i < v.size(); ++i) {
      if (v[i] == b) {
        v[i] = v.back();
        v.pop_back();
        break;
      }
    }
    erased_since_rebuild += 1u;
    if (erased_since_rebuild * 4u > live.size() + 64u) {
      member = U64Set();
      for (const Edge &e : live) {
        member.Insert(PackEdge(e.first, e.second));
      }
      erased_since_rebuild = 0u;
    }
  }

  Edge SampleEdge(void) {
    for (;;) {
      const uint32_t a = static_cast<uint32_t>(rng.Below(nodes));
      const uint32_t b = static_cast<uint32_t>(rng.Below(nodes));
      if (a == 0u && b == 0u) {
        continue;
      }
      return {a, b};
    }
  }

  // Warm phase: adds-only, up to `count` fresh edges.
  Batch Warm(uint64_t count) {
    Batch batch;
    for (uint64_t i = 0u; i < count; ++i) {
      const Edge e = SampleEdge();
      if (IsLive(e.first, e.second)) {
        continue;
      }
      batch.adds.push_back(e);
      AddLive(e.first, e.second);
    }
    return batch;
  }

  // One churn batch: up to `m` phantom instances. Each instance picks a live
  // 2-path X->Y->Z (Y has >=1 live out-hop to Z), removes e(Y,Z), and adds a
  // fresh e(W,Y). W is a uniformly random node giving a fresh edge into Y.
  Batch Churn(uint64_t m) {
    Batch batch;
    U64Set touched;  // edges touched (added or removed) this batch
    if (live.empty()) {
      return batch;
    }
    for (uint64_t i = 0u; i < m; ++i) {
      // Pick a live 2-path: sample a live edge (X,Y), require Y to have a
      // live out-hop (Y,Z). Bounded retries to avoid degenerate spins.
      bool ok = false;
      uint32_t Y = 0u, Z = 0u, W = 0u;
      for (uint32_t tries = 0u; tries < 8u && !ok; ++tries) {
        const Edge xy = live[rng.Below(live.size())];
        Y = xy.second;
        if (out[Y].empty()) {
          continue;
        }
        Z = out[Y][rng.Below(out[Y].size())];
        // e(Y,Z) must still be live and untouched this batch.
        if (!IsLive(Y, Z) || touched.Contains(PackEdge(Y, Z))) {
          continue;
        }
        // Find a fresh W->Y edge (W uniform, not live, not touched, W!=Y so
        // the added hop is a genuine second-hop feeder into Y).
        for (uint32_t wtry = 0u; wtry < 8u; ++wtry) {
          const uint32_t w = static_cast<uint32_t>(rng.Below(nodes));
          if (w == Y) {
            continue;
          }
          if (w == 0u && Y == 0u) {
            continue;
          }
          if (IsLive(w, Y) || touched.Contains(PackEdge(w, Y))) {
            continue;
          }
          W = w;
          ok = true;
          break;
        }
      }
      if (!ok) {
        continue;
      }
      // Emit the mixed pair: remove e(Y,Z), add e(W,Y).
      touched.Insert(PackEdge(Y, Z));
      touched.Insert(PackEdge(W, Y));
      batch.removes.push_back({Y, Z});
      batch.adds.push_back({W, Y});
      RemoveLive(Y, Z);
      AddLive(W, Y);
    }
    return batch;
  }
};

}  // namespace bench
