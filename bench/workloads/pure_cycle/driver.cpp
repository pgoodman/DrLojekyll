// Copyright 2026, Peter Goodman. All rights reserved.
//
// pure_cycle engine driver: the OQ7(a) pure-cycle overdelete-wave isolator
// (design R16). Times the generated incremental database (the hidden-friend
// surface, one Database instance per run) over a PLANTED-RING edge stream.
// Same generated program as tc_random (nonlinear TC over a @differential edge
// message), but the stream is a set of disjoint k-node rings; inside an intact
// ring every tc row's support is purely cyclic, so retracting one ring edge
// triggers the full overdelete wave over that ring's tc cone and then
// rederivation of the surviving path pairs. Compile (the diffrun 3-TU line at
// bench flags):
//
//   drlojekyll bench/workloads/pure_cycle/cycle.dr [-disable-*-opt] -cpp-out GEN/
//   clang++ -std=c++23 -O2 -DNDEBUG -I include -I GEN \
//       bench/workloads/pure_cycle/driver.cpp GEN/datalog.cpp \
//       lib/Runtime/Allocator.cpp -o pure_cycle_engine
//
// Knobs (key=value args): rings R, k (ring size), batches, rep, mode (echoed
// into TSV), alloc (malloc|arena), block (time K-epoch blocks), canary (emit a
// drift canary every N churn epochs; 0 = off), verify (1 = drain reachable_ff
// mid-run at each churn epoch and cross-check against the analytic formula;
// untimed, clock stopped).
//
// Ring layout: ring r occupies the disjoint node range [r*k, (r+1)*k). Its
// edges are r*k + i -> r*k + ((i+1) mod k) for i in [0, k). Seed: all ring
// edges, adds-only batches. Churn batch b: even b REMOVES edge index
// (b/2 mod k) of EVERY ring (one batch, R simultaneous removals -> R
// simultaneous overdelete waves); odd b RE-ADDS those same edges.
//
// Analytic sentinel (exact): all rings intact -> tc count = R*k*k (a k-node
// directed cycle makes every ordered pair, incl. self, reachable). After a
// remove batch each ring is a simple k-node path -> ordered reachable pairs
// (length >= 1) = k*(k-1)/2, so tc count = R*k*(k-1)/2.
//
// Timed region per churn epoch = exactly the entry call add_edge_2(...). Input
// Vec construction, the mid-run verify drain, canaries, and the final query
// drain + sentinel all sit OUTSIDE the timed brackets (design R5, R12; clock
// stopped before the sentinel).

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

#include "datalog.h"

#include "../../common/bench.h"

namespace {

// One directed edge of a ring, as (u32,u32) packed nodes.
struct Edge {
  uint32_t from;
  uint32_t to;
};

// Edge index i of ring r: r*k + i -> r*k + ((i+1) mod k).
inline Edge RingEdge(uint64_t r, uint64_t k, uint64_t i) {
  const uint64_t base = r * k;
  return {static_cast<uint32_t>(base + i),
          static_cast<uint32_t>(base + ((i + 1u) % k))};
}

hyde::rt::Vec<Tup_u64_u64> ToVec(const hyde::rt::Allocator &alloc,
                                 const std::vector<Edge> &edges) {
  hyde::rt::Vec<Tup_u64_u64> v(alloc);
  for (const auto &e : edges) {
    v.Add({e.from, e.to});
  }
  return v;
}

// Analytic tc count: broken rings contribute k*(k-1)/2, intact rings k*k.
uint64_t AnalyticCount(uint64_t rings, uint64_t k, bool all_intact) {
  return all_intact ? rings * k * k : rings * (k * (k - 1u) / 2u);
}

// Drain reachable_ff into a sorted pair list; caller hashes/counts. Clock must
// be stopped around every call (design R12).
std::vector<std::pair<uint64_t, uint64_t>> DrainReachable(Database &db) {
  std::vector<std::pair<uint64_t, uint64_t>> pairs;
  auto c = reachable_ff(db);
  for (uint64_t f = 0u, t = 0u; c.next(f, t);) {
    pairs.emplace_back(f, t);
  }
  std::sort(pairs.begin(), pairs.end());
  return pairs;
}

// COUNTS-binary support (compiled with -DDRLOJEKYLL_BENCH_COUNTERS): emit
// per-epoch counter deltas as ctr_* metric rows. This binary's wall rows are
// discarded downstream.
#ifdef DRLOJEKYLL_BENCH_COUNTERS
void EmitCounterDeltas(const bench::Tsv &tsv, int64_t epoch,
                       const hyde::rt::BenchCounters &before,
                       const hyde::rt::BenchCounters &after) {
#define HYDE_RT_BENCH_EMIT(name) \
  tsv.Row(epoch, "ctr_" #name, after.name - before.name);
  HYDE_RT_BENCH_COUNTER_FIELDS(HYDE_RT_BENCH_EMIT)
#undef HYDE_RT_BENCH_EMIT
}
#endif

}  // namespace

int main(int argc, char **argv) {
  bench::Knobs knobs(argc, argv);
  const uint64_t rings = knobs.U64("rings", 8u);
  const uint64_t k = knobs.U64("k", 32u);
  const uint64_t batches = knobs.U64("batches", 40u);
  const std::string alloc_kind = knobs.Str("alloc", "malloc");
  const uint64_t rep = knobs.U64("rep", 0u);
  const std::string mode = knobs.Str("mode", "opt");
  const uint64_t canary = knobs.U64("canary", 0u);
  const uint64_t block = knobs.U64("block", 1u);
  const uint64_t verify = knobs.U64("verify", 0u);
  knobs.Finish();

  if (k < 2u || rings < 1u) {
    std::fprintf(stderr, "pure_cycle requires k>=2 and rings>=1\n");
    return 2;
  }

  // Every measurement-affecting knob is part of the canonical (sorted) key;
  // mode and rep stay out (their own TSV columns).
  char knob_str[256];
  std::snprintf(knob_str, sizeof knob_str,
                "alloc=%s,batches=%llu,block=%llu,canary=%llu,k=%llu,"
                "rings=%llu,verify=%llu",
                alloc_kind.c_str(), (unsigned long long) batches,
                (unsigned long long) block, (unsigned long long) canary,
                (unsigned long long) k, (unsigned long long) rings,
                (unsigned long long) verify);
  const bench::Tsv tsv{"pure_cycle_engine", knob_str, mode, rep};

  tsv.Row(-1, "clock_overhead_ns", bench::ClockOverheadNs());

  // Database gets the counted allocator; driver-side Vecs use a separate
  // UNCOUNTED allocator so alloc_* rows describe database work only (matching
  // tc_random driver's discipline).
  bench::AllocStats stats;
  const hyde::rt::Allocator input_alloc = hyde::rt::MallocAllocator();
  hyde::rt::Arena arena(hyde::rt::MallocAllocator());
  hyde::rt::Allocator allocator =
      (alloc_kind == "arena")
          ? hyde::rt::ArenaAllocator(arena)
          : hyde::rt::Allocator{&stats, bench::CountingAlloc,
                                bench::CountingFree};

  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);

  {
    const uint64_t t0 = bench::NowNs();
    init(db, log, functors);
    tsv.Row(-1, "init_wall_ns", bench::NowNs() - t0);
  }

  // ---- seed phase: all ring edges, adds-only, one batch per edge index ----
  // Batch i (i in [0,k)) adds edge index i of every ring: R edges/batch,
  // R*k edges total, so the ring structure is built symmetrically.
  uint64_t seed_epochs = 0u;
  uint64_t seed_wall = 0u;
  for (uint64_t i = 0u; i < k; ++i) {
    std::vector<Edge> add_edges;
    add_edges.reserve(rings);
    for (uint64_t r = 0u; r < rings; ++r) {
      add_edges.push_back(RingEdge(r, k, i));
    }
    auto adds = ToVec(input_alloc, add_edges);
    hyde::rt::Vec<Tup_u64_u64> removes(input_alloc);
    const uint64_t t0 = bench::NowNs();
    add_edge_2(db, log, functors, std::move(adds), std::move(removes));
    seed_wall += bench::NowNs() - t0;
    seed_epochs += 1u;
  }
  tsv.Row(-1, "seed_epochs", seed_epochs);
  tsv.Row(-1, "seed_wall_ns", seed_wall);
  tsv.Row(-1, "seed_edges", rings * k);

  // Post-seed: all rings intact. Verify the analytic count (untimed).
  {
    auto pairs = DrainReachable(db);
    tsv.Row(-1, "seed_tc_count", pairs.size());
    tsv.Row(-1, "seed_tc_expect", AnalyticCount(rings, k, /*intact=*/true));
    if (verify && pairs.size() != AnalyticCount(rings, k, true)) {
      tsv.Row(-1, "SENTINEL_FAIL", 1u);
      tsv.Complete();
      return 1;
    }
  }

  // ---- churn phase ----
  // Track the current edge state so the final sentinel and the mid-run verify
  // both know the intact/broken parity. Even batch b removes edge index
  // (b/2 mod k) of every ring; odd b re-adds those same edges.
  uint64_t block_wall = 0u;
  for (uint64_t b = 0u; b < batches; ++b) {
    const bool removing = (b % 2u) == 0u;
    const uint64_t idx = (b / 2u) % k;

    std::vector<Edge> add_edges;
    std::vector<Edge> rem_edges;
    for (uint64_t r = 0u; r < rings; ++r) {
      const Edge e = RingEdge(r, k, idx);
      if (removing) {
        rem_edges.push_back(e);
      } else {
        add_edges.push_back(e);
      }
    }
    auto adds = ToVec(input_alloc, add_edges);
    auto removes = ToVec(input_alloc, rem_edges);
    const uint64_t adds_n = add_edges.size();
    const uint64_t removes_n = rem_edges.size();

#ifdef DRLOJEKYLL_BENCH_COUNTERS
    const hyde::rt::BenchCounters ctr_before = hyde::rt::gBenchCounters;
#endif
    const uint64_t t0 = bench::NowNs();
    add_edge_2(db, log, functors, std::move(adds), std::move(removes));
    const uint64_t dt = bench::NowNs() - t0;

    const int64_t epoch = static_cast<int64_t>(b);
#ifdef DRLOJEKYLL_BENCH_COUNTERS
    EmitCounterDeltas(tsv, epoch, ctr_before, hyde::rt::gBenchCounters);
#endif
    if (block <= 1u) {
      tsv.Row(epoch, "epoch_wall_ns", dt);
    } else {
      block_wall += dt;
      if ((b + 1u) % block == 0u) {
        tsv.Row(epoch, "block_wall_ns", block_wall);
        block_wall = 0u;
      }
    }
    tsv.Row(epoch, "batch_adds", adds_n);
    tsv.Row(epoch, "batch_removes", removes_n);

    // After an even (remove) batch all rings are broken; after an odd
    // (re-add) batch all rings are intact again.
    const bool all_intact = !removing;

    if (verify) {
      auto pairs = DrainReachable(db);  // clock stopped
      const uint64_t expect = AnalyticCount(rings, k, all_intact);
      tsv.Row(epoch, "verify_tc_count", pairs.size());
      tsv.Row(epoch, "verify_tc_expect", expect);
      if (pairs.size() != expect) {
        tsv.Row(epoch, "SENTINEL_FAIL", 1u);
        tsv.Complete();
        return 1;
      }
    }

    if (canary && ((b + 1u) % canary == 0u)) {
      tsv.Row(epoch, "canary_wall_ns", bench::CanaryWallNs());
    }
  }

  // ---- untimed tail: query drain + sentinel (clock stopped) ----
  {
    // Final parity: last batch was b = batches-1. If it was even (remove) the
    // rings are broken; odd (re-add) leaves them intact.
    const bool last_removed = ((batches - 1u) % 2u) == 0u;
    const bool all_intact = (batches == 0u) ? true : !last_removed;

    const uint64_t q0 = bench::NowNs();
    auto pairs = DrainReachable(db);
    const uint64_t q_wall = bench::NowNs() - q0;
    tsv.Row(-1, "final_query_wall_ns", q_wall);

    bench::Fnv fnv;
    for (const auto &[f, t] : pairs) {
      fnv.Add(f);
      fnv.Add(t);
    }
    tsv.Row(-1, "final_tc_count", pairs.size());
    tsv.Row(-1, "final_tc_hash", fnv.h);
    tsv.Row(-1, "final_tc_expect", AnalyticCount(rings, k, all_intact));
    tsv.Row(-1, "final_live_edges",
            all_intact ? rings * k : rings * (k - 1u));
    if (pairs.size() != AnalyticCount(rings, k, all_intact)) {
      tsv.Row(-1, "SENTINEL_FAIL", 1u);
      tsv.Complete();
      return 1;
    }
  }

  tsv.Row(-1, "peak_rss_bytes", bench::PeakRssBytes());
  if (alloc_kind != "arena") {
    tsv.Row(-1, "alloc_count", stats.alloc_count);
    tsv.Row(-1, "alloc_bytes", stats.alloc_bytes);
    tsv.Row(-1, "free_count", stats.free_count);
    tsv.Row(-1, "free_bytes", stats.free_bytes);
  }
  tsv.Complete();
  return 0;
}
