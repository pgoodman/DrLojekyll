// Copyright 2026, Peter Goodman. All rights reserved.
//
// flip_storm / phantom_pair engine driver: times the generated incremental
// database (the hidden-friend surface, one Database per run) over the shared
// seeded phantom-pair edge stream, and OBSERVES the published net-change
// stream of the @differential message h_msg. Unlike tc_random (query-only,
// empty sinks), this driver provides its OWN log type whose h_msg_2 hook is
// called once per net presence change of an h row at each batch's commit
// sweep, so publish cost has a number (design R18) and the live h set is
// reconstructable purely from the publication stream.
//
// Compile (the diffrun 3-TU line at bench flags):
//
//   drlojekyll bench/workloads/flip_storm/phantom_pair.dr [-disable-*-opt] \
//       -cpp-out GEN/
//   clang++ -std=c++23 -O2 -DNDEBUG -I include -I GEN \
//       bench/workloads/flip_storm/driver.cpp GEN/datalog.cpp \
//       lib/Runtime/Allocator.cpp -o phantom_pair_engine
//
// Knobs (key=value): nodes, m (phantom instances per churn batch), batches,
// warm_edges (pre-seeded random e edges), seed, rep, mode, alloc
// (malloc|arena), block (time K-epoch blocks), canary (constant-work canary
// every N epochs; 0 = off).
//
// Timed region per churn epoch = exactly the entry call e_msg_2(...). Input
// Vec construction, the counting log flush, the sentinel query drain, and
// all sentinel computation sit OUTSIDE the timed brackets (design R5/R12;
// clock stopped before the sentinel).

#include <algorithm>
#include <cstdint>
#include <vector>

#include "datalog.h"

#include "../../common/bench.h"
#include "phantom_gen.h"

namespace {

hyde::rt::Vec<Tup_u64_u64> ToVec(const hyde::rt::Allocator &alloc,
                                 const std::vector<bench::Edge> &edges) {
  hyde::rt::Vec<Tup_u64_u64> v(alloc);
  for (const auto &[a, b] : edges) {
    v.Add({a, b});
  }
  return v;
}

// The observation log: its member signatures MATCH the generated
// DatabaseLog's hooks, so the entry points deduce THIS type and route
// published h_msg deltas here with no inheritance and no virtual dispatch
// (CLAUDE.md Generated API). Every net presence change of an h row is one
// call: added=true is a publication add, added=false a publication del. We
// count adds/dels per epoch AND maintain the driver-side live h SET implied
// by the stream (apply the delta), for the sentinel cross-check.
struct CountingLog {
  uint64_t adds{0u};
  uint64_t dels{0u};

  void h_msg_2(uint64_t X, uint64_t Z, bool added) {
    if (added) {
      adds += 1u;
      events.push_back({static_cast<uint32_t>(X), static_cast<uint32_t>(Z)});
    } else {
      dels += 1u;
      events.push_back({static_cast<uint32_t>(X) | kDelFlag,
                        static_cast<uint32_t>(Z)});
    }
  }

  // A @differential publication stream emits one call per NET presence
  // change, so for any (X,Z) the calls strictly alternate +,-,+,-,...: the
  // parity of the count of events for a key gives its final presence. We
  // record events in order and fold them at the end (design R12: sentinel
  // work is off-clock). kDelFlag on X marks a del event; X values are < 2^32
  // so the top bit is free.
  static constexpr uint32_t kDelFlag = 0x80000000u;
  std::vector<std::pair<uint32_t, uint32_t>> events;
};

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
  // Defaults chosen so the DEFAULT point robustly exercises the phantom
  // machinery: at nodes=256 with warm_edges=2048 (mean degree 8) the graph
  // is dense enough that shared 2-paths abound, so the mixed churn batches
  // force real phantom crossings -- the counts binary reports several hundred
  // ctr_stale_drops_del at this point (the F17 del-side claim gate firing).
  const uint64_t nodes = knobs.U64("nodes", 256u);
  const uint64_t m = knobs.U64("m", 32u);
  const uint64_t batches = knobs.U64("batches", 100u);
  const uint64_t warm_edges = knobs.U64("warm_edges", 2048u);
  const std::string alloc_kind = knobs.Str("alloc", "malloc");
  const uint64_t seed = knobs.U64("seed", 1u);
  const uint64_t rep = knobs.U64("rep", 0u);
  const std::string mode = knobs.Str("mode", "opt");
  const uint64_t block = knobs.U64("block", 1u);
  const uint64_t canary = knobs.U64("canary", 0u);
  knobs.Finish();

  char knob_str[256];
  std::snprintf(knob_str, sizeof knob_str,
                "alloc=%s,batches=%llu,block=%llu,canary=%llu,m=%llu,"
                "nodes=%llu,seed=%llu,warm_edges=%llu",
                alloc_kind.c_str(), (unsigned long long) batches,
                (unsigned long long) block, (unsigned long long) canary,
                (unsigned long long) m, (unsigned long long) nodes,
                (unsigned long long) seed, (unsigned long long) warm_edges);
  const bench::Tsv tsv{"phantom_pair_engine", knob_str, mode, rep};

  tsv.Row(-1, "clock_overhead_ns", bench::ClockOverheadNs());

  bench::AllocStats stats;
  const hyde::rt::Allocator input_alloc = hyde::rt::MallocAllocator();
  hyde::rt::Arena arena(hyde::rt::MallocAllocator());
  hyde::rt::Allocator allocator =
      (alloc_kind == "arena")
          ? hyde::rt::ArenaAllocator(arena)
          : hyde::rt::Allocator{&stats, bench::CountingAlloc,
                                bench::CountingFree};

  DatabaseFunctors functors;
  CountingLog log;
  Database db(allocator);

  {
    const uint64_t t0 = bench::NowNs();
    init(db, log, functors);
    tsv.Row(-1, "init_wall_ns", bench::NowNs() - t0);
  }

  bench::PhantomGen gen(static_cast<uint32_t>(nodes), seed);

  // ---- warm phase: adds-only, one big batch up to warm_edges ----
  {
    bench::Batch batch = gen.Warm(warm_edges);
    auto adds = ToVec(input_alloc, batch.adds);
    hyde::rt::Vec<Tup_u64_u64> removes(input_alloc);
    const uint64_t warm_before_adds = log.adds;
    const uint64_t t0 = bench::NowNs();
    e_msg_2(db, log, functors, std::move(adds), std::move(removes));
    tsv.Row(-1, "warm_wall_ns", bench::NowNs() - t0);
    tsv.Row(-1, "warm_edges_live", gen.live.size());
    tsv.Row(-1, "warm_publish_adds", log.adds - warm_before_adds);
  }

  // ---- churn phase: m phantom instances per batch ----
  uint64_t block_wall = 0u;
  for (uint64_t b = 0u; b < batches; ++b) {
    bench::Batch batch = gen.Churn(m);
    auto adds = ToVec(input_alloc, batch.adds);
    auto removes = ToVec(input_alloc, batch.removes);
    const uint64_t adds_n = batch.adds.size();
    const uint64_t removes_n = batch.removes.size();
    const uint64_t pub_adds_before = log.adds;
    const uint64_t pub_dels_before = log.dels;

#ifdef DRLOJEKYLL_BENCH_COUNTERS
    const hyde::rt::BenchCounters ctr_before = hyde::rt::gBenchCounters;
#endif
    const uint64_t t0 = bench::NowNs();
    e_msg_2(db, log, functors, std::move(adds), std::move(removes));
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
    // The publish-cost signal (design R18/R4): net-change publications of h
    // per epoch, adds and dels separately. Clock already stopped.
    tsv.Row(epoch, "publish_adds", log.adds - pub_adds_before);
    tsv.Row(epoch, "publish_dels", log.dels - pub_dels_before);

    if (canary && ((b + 1u) % canary == 0u)) {
      tsv.Row(epoch, "canary_wall_ns", bench::CanaryWallNs());
    }
  }

  tsv.Row(-1, "total_publish_adds", log.adds);
  tsv.Row(-1, "total_publish_dels", log.dels);

  // ---- untimed tail: two independent sentinels (clock stopped) ----
  //
  // Sentinel A: the live h set implied by the PUBLICATION STREAM. Fold the
  // ordered add/del events per (X,Z): a @differential publication emits one
  // call per net presence change, so events for a key alternate; the final
  // presence is (adds - dels) for that key > 0. We compute presence with a
  // small map keyed by packed (X,Z).
  {
    // Net-count per key: +1 on add event, -1 on del event; present iff > 0.
    // Use an ordered vector + sort to avoid std::unordered_map on the
    // sentinel path (kept simple; this is off-clock).
    std::vector<std::pair<uint64_t, int64_t>> tally;
    tally.reserve(log.events.size());
    for (const auto &[xf, z] : log.events) {
      const bool is_del = (xf & CountingLog::kDelFlag) != 0u;
      const uint32_t x = xf & ~CountingLog::kDelFlag;
      tally.push_back({bench::PackEdge(x, z), is_del ? -1 : 1});
    }
    std::sort(tally.begin(), tally.end());
    std::vector<std::pair<uint64_t, uint64_t>> pub_pairs;
    for (size_t i = 0u; i < tally.size();) {
      const uint64_t key = tally[i].first;
      int64_t net = 0;
      while (i < tally.size() && tally[i].first == key) {
        net += tally[i].second;
        ++i;
      }
      if (net > 0) {
        pub_pairs.push_back({key >> 32u, key & 0xffffffffu});
      }
    }
    std::sort(pub_pairs.begin(), pub_pairs.end());
    bench::Fnv fnv;
    for (const auto &[x, z] : pub_pairs) {
      fnv.Add(x);
      fnv.Add(z);
    }
    tsv.Row(-1, "final_h_pub_count", pub_pairs.size());
    tsv.Row(-1, "final_h_pub_hash", fnv.h);
  }

  // Sentinel B: drain the h_out query (the materialized h relation). This is
  // the canonical sentinel cross-checked across the 4 modes AND the baseline
  // (which recomputes h = e JOIN e from the same stream). Sentinel A above
  // must agree with B for any single mode (publication stream == materialized
  // relation).
  {
    const uint64_t q0 = bench::NowNs();
    std::vector<std::pair<uint64_t, uint64_t>> pairs;
    auto c = h_out_ff(db);
    for (uint64_t x = 0u, z = 0u; c.next(x, z);) {
      pairs.emplace_back(x, z);
    }
    const uint64_t q_wall = bench::NowNs() - q0;
    tsv.Row(-1, "final_query_wall_ns", q_wall);
    std::sort(pairs.begin(), pairs.end());
    bench::Fnv fnv;
    for (const auto &[x, z] : pairs) {
      fnv.Add(x);
      fnv.Add(z);
    }
    tsv.Row(-1, "final_h_count", pairs.size());
    tsv.Row(-1, "final_h_hash", fnv.h);
    tsv.Row(-1, "final_live_edges", gen.live.size());
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
