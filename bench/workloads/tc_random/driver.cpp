// Copyright 2026, Trail of Bits. All rights reserved.
//
// tc_random engine driver: times the generated incremental database (the
// hidden-friend surface, one Database instance per run) over the shared
// seeded edge stream. Compile (the diffrun 3-TU line at bench flags):
//
//   drlojekyll bench/workloads/tc_random/tc.dr [-disable-*-opt] -cpp-out GEN/
//   clang++ -std=c++23 -O2 -DNDEBUG -I include -I GEN \
//       bench/workloads/tc_random/driver.cpp GEN/datalog.cpp \
//       lib/Runtime/Allocator.cpp -o tc_random_engine
//
// Knobs (key=value args): nodes, ef (edge factor x100), batches, bs,
// rr (remove %, churn phase), graph (uniform|powerlaw), alloc
// (malloc|arena), seed, rep, mode (echoed into TSV), canary (emit a
// machine-drift canary row every N churn epochs; 0 = off), block (time
// K-epoch blocks instead of single epochs; for sub-clock-resolution
// points), shadow (1 = per-epoch driver-side NetBatch shadow timing).
//
// Timed region per churn epoch = exactly the entry call add_edge_2(...).
// Input Vec construction, the NetBatch shadow, canaries, and the final
// query drain + sentinel all sit OUTSIDE the timed brackets (design R5,
// R12; clock stopped before the sentinel).

#include <algorithm>
#include <cstdint>
#include <vector>

#include "datalog.h"

#include "../../common/bench.h"
#include "gen.h"

namespace {

hyde::rt::Vec<Tup_u64_u64> ToVec(const hyde::rt::Allocator &alloc,
                                 const std::vector<bench::Edge> &edges) {
  hyde::rt::Vec<Tup_u64_u64> v(alloc);
  for (const auto &[a, b] : edges) {
    v.Add({a, b});
  }
  return v;
}

// COUNTS-binary support (compiled with -DDRLOJEKYLL_BENCH_COUNTERS): emit
// per-epoch counter deltas as ctr_* metric rows. This binary's wall rows
// are discarded downstream — counts and wall are separate narratives.
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
  const uint64_t nodes = knobs.U64("nodes", 1000u);
  const uint64_t ef = knobs.U64("ef", 200u);  // edges = nodes * ef / 100
  const uint64_t batches = knobs.U64("batches", 200u);
  const uint64_t bs = knobs.U64("bs", 64u);
  const uint64_t rr = knobs.U64("rr", 30u);
  const std::string graph = knobs.Str("graph", "uniform");
  const std::string alloc_kind = knobs.Str("alloc", "malloc");
  const uint64_t seed = knobs.U64("seed", 1u);
  const uint64_t rep = knobs.U64("rep", 0u);
  const std::string mode = knobs.Str("mode", "opt");
  // tc_random is the churn/drift workload: the canary defaults ON (R10) so
  // an unattended flagship run can always separate machine drift from
  // database-state drift.
  const uint64_t canary = knobs.U64("canary", 64u);
  const uint64_t block = knobs.U64("block", 1u);
  const uint64_t shadow = knobs.U64("shadow", 1u);
  const uint64_t warmup = knobs.U64("warmup", 3u);
  knobs.Finish();

  // Every measurement-affecting knob is part of the canonical key (sorted);
  // only mode and rep stay out (they are their own TSV columns). A
  // shadow=0 and shadow=1 run must never collide under one key.
  char knob_str[256];
  std::snprintf(knob_str, sizeof knob_str,
                "alloc=%s,batches=%llu,block=%llu,bs=%llu,canary=%llu,"
                "ef=%llu,graph=%s,nodes=%llu,rr=%llu,seed=%llu,shadow=%llu",
                alloc_kind.c_str(), (unsigned long long) batches,
                (unsigned long long) block, (unsigned long long) bs,
                (unsigned long long) canary, (unsigned long long) ef,
                graph.c_str(), (unsigned long long) nodes,
                (unsigned long long) rr, (unsigned long long) seed,
                (unsigned long long) shadow);
  const bench::Tsv tsv{"tc_random_engine", knob_str, mode, rep};

  tsv.Row(-1, "clock_overhead_ns", bench::ClockOverheadNs());

  // Allocator: counting wrapper over malloc (default), or an arena whose
  // counts describe chunk traffic (BASELINE.md caveat; RSS rows are only
  // comparable under alloc=malloc — design R9).
  //
  // The DATABASE gets the counted allocator; driver-side Vecs (message
  // marshalling, shadow copies) use a separate UNCOUNTED allocator so
  // alloc_* rows describe database work only. One documented consequence:
  // the input Vecs travel into the engine by move, so the engine's
  // NetBatch scratch (allocated from the input vec's allocator, Vec.h) is
  // attributed to marshalling, not the database — recorded in BASELINE.md.
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

  // Epoch 0, separately timeable on the new surface. Caveated in
  // BASELINE.md: for this program init derives an empty least model, so
  // this wall is first-touch/pagefault dominated (design R11).
  {
    const uint64_t t0 = bench::NowNs();
    init(db, log, functors);
    tsv.Row(-1, "init_wall_ns", bench::NowNs() - t0);
  }

  bench::GraphGen gen(static_cast<uint32_t>(nodes),
                      static_cast<uint32_t>(rr), graph == "powerlaw", seed);

  // ---- seed phase: adds-only batches up to ef% of nodes ----
  const uint64_t target_edges = nodes * ef / 100u;
  uint64_t seed_epochs = 0u;
  uint64_t seed_wall = 0u;
  while (gen.live.size() < target_edges) {
    bench::Batch batch = gen.Seed(bs, target_edges);
    if (batch.adds.empty()) {
      break;  // graph saturated below target (tiny node counts)
    }
    auto adds = ToVec(input_alloc, batch.adds);
    hyde::rt::Vec<Tup_u64_u64> removes(input_alloc);
    const uint64_t t0 = bench::NowNs();
    add_edge_2(db, log, functors, std::move(adds), std::move(removes));
    seed_wall += bench::NowNs() - t0;
    seed_epochs += 1u;
  }
  tsv.Row(-1, "seed_epochs", seed_epochs);
  tsv.Row(-1, "seed_wall_ns", seed_wall);
  tsv.Row(-1, "seed_edges", gen.live.size());
  // Downstream drops churn epochs < warmup from aggregate stats (the rows
  // stay in the series for drift analysis — methodology R2/R10).
  tsv.Row(-1, "warmup_epochs", warmup);

  // ---- churn phase ----
  uint64_t block_wall = 0u;
  for (uint64_t b = 0u; b < batches; ++b) {
    bench::Batch batch = gen.Churn(bs);
    auto adds = ToVec(input_alloc, batch.adds);
    auto removes = ToVec(input_alloc, batch.removes);
    const uint64_t adds_n = batch.adds.size();
    const uint64_t removes_n = batch.removes.size();

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

    // Driver-side NetBatch shadow (design R5): same code, same data, timed
    // outside the epoch bracket. The engine's own in-entry NetBatch cost at
    // this batch is (to first order) this number.
    if (shadow) {
      auto s_adds = ToVec(input_alloc, batch.adds);
      auto s_removes = ToVec(input_alloc, batch.removes);
      const uint64_t s0 = bench::NowNs();
      hyde::rt::NetBatch(s_adds, s_removes);
      tsv.Row(epoch, "netshadow_wall_ns", bench::NowNs() - s0);
    }

    if (canary && ((b + 1u) % canary == 0u)) {
      tsv.Row(epoch, "canary_wall_ns", bench::CanaryWallNs());
    }
  }

  // ---- untimed tail: query drain + sentinel (clock stopped) ----
  {
    const uint64_t q0 = bench::NowNs();
    std::vector<std::pair<uint64_t, uint64_t>> pairs;
    auto c = reachable_ff(db);
    for (uint64_t f = 0u, t = 0u; c.next(f, t);) {
      pairs.emplace_back(f, t);
    }
    const uint64_t q_wall = bench::NowNs() - q0;
    tsv.Row(-1, "final_query_wall_ns", q_wall);
    std::sort(pairs.begin(), pairs.end());
    bench::Fnv fnv;
    for (const auto &[f, t] : pairs) {
      fnv.Add(f);
      fnv.Add(t);
    }
    tsv.Row(-1, "final_tc_count", pairs.size());
    tsv.Row(-1, "final_tc_hash", fnv.h);
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
