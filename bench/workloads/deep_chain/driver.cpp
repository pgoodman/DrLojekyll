// Copyright 2026, Trail of Bits. All rights reserved.
//
// deep_chain engine driver: times the generated incremental database (the
// hidden-friend surface, one Database instance per run) over the deepest-chain
// shape (PerfRoadmap Sec 2; OQ7 pure-cascade extreme). The chain is a
// descending next-edge relation next(k+1, k) for k in [0, depth), plus the
// single @differential base fact base(depth). reach(k) has exactly one
// derivation of depth depth-k; retracting base(depth) cascades the whole chain
// away (full overdelete), re-adding it rederives the whole chain. This is the
// Stage-3 constant-stack acceptance gate at scale: depth=100000 at -O2 -DNDEBUG
// must run WITHOUT stack overflow in all 4 optimization modes.
//
// Compile (the diffrun 3-TU line at bench flags):
//
//   drlojekyll bench/workloads/deep_chain/chain.dr [-disable-*-opt] -cpp-out GEN/
//   clang++ -std=c++23 -O2 -DNDEBUG -I include -I GEN \
//       bench/workloads/deep_chain/driver.cpp GEN/datalog.cpp \
//       lib/Runtime/Allocator.cpp -o deep_chain_engine
//
// Knobs (key=value args): depth (chain length), cycles (retract/reseed churn
// count), ingest_bs (chain-edge batch size for the monotone next_2 message),
// block, rep, mode (echoed into TSV), alloc (malloc|arena), canary.
//
// Timed regions: the ingest of the next-chain (summed as seed_wall_ns); the
// base(depth) add (init_add epoch); and per cycle the base(depth) retract
// (retract_wall_ns, epoch=cycle) and re-add (reseed_wall_ns, epoch=cycle).
// Input Vec construction, canaries, and the final query drain + sentinel all
// sit OUTSIDE the timed brackets (design R5, R12; clock stopped before the
// sentinel).

#include <algorithm>
#include <cstdint>
#include <vector>

#include "datalog.h"

#include "../../common/bench.h"

namespace {

// COUNTS-binary support (compiled with -DDRLOJEKYLL_BENCH_COUNTERS): emit
// per-epoch counter deltas as ctr_* metric rows. This binary's wall rows are
// discarded downstream -- counts and wall are separate narratives.
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
  const uint64_t depth = knobs.U64("depth", 100000u);
  const uint64_t cycles = knobs.U64("cycles", 20u);
  const uint64_t ingest_bs = knobs.U64("ingest_bs", 65536u);
  const std::string alloc_kind = knobs.Str("alloc", "malloc");
  const uint64_t rep = knobs.U64("rep", 0u);
  const std::string mode = knobs.Str("mode", "opt");
  const uint64_t block = knobs.U64("block", 1u);
  const uint64_t canary = knobs.U64("canary", 0u);
  knobs.Finish();

  // Every measurement-affecting knob is part of the canonical (sorted) key;
  // only mode and rep stay out (they are their own TSV columns).
  char knob_str[256];
  std::snprintf(knob_str, sizeof knob_str,
                "alloc=%s,block=%llu,canary=%llu,cycles=%llu,depth=%llu,"
                "ingest_bs=%llu",
                alloc_kind.c_str(), (unsigned long long) block,
                (unsigned long long) canary, (unsigned long long) cycles,
                (unsigned long long) depth, (unsigned long long) ingest_bs);
  const bench::Tsv tsv{"deep_chain_engine", knob_str, mode, rep};

  tsv.Row(-1, "clock_overhead_ns", bench::ClockOverheadNs());

  // Allocator: counting wrapper over malloc (default), or an arena whose
  // counts describe chunk traffic (BASELINE.md caveat; RSS rows are only
  // comparable under alloc=malloc -- design R9). The DATABASE gets the counted
  // allocator; driver-side Vecs use a separate UNCOUNTED allocator so alloc_*
  // rows describe database work only.
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

  // Epoch 0, separately timeable on the new surface. Caveated in BASELINE.md:
  // for this program init derives an empty least model, so this wall is
  // first-touch/pagefault dominated (design R11).
  {
    const uint64_t t0 = bench::NowNs();
    init(db, log, functors);
    tsv.Row(-1, "init_wall_ns", bench::NowNs() - t0);
  }

  // ---- seed phase: ingest the descending next-chain in ingest_bs batches ----
  // next(k+1, k) for k in [0, depth): a descending chain, so reach derives
  // downward from base(depth) to reach(0). The next_2 message is monotone
  // (no @differential), one add-only Vec per call.
  uint64_t seed_epochs = 0u;
  uint64_t seed_wall = 0u;
  for (uint64_t start = 0u; start < depth; start += ingest_bs) {
    const uint64_t stop = std::min(start + ingest_bs, depth);
    hyde::rt::Vec<Tup_u64_u64> steps(input_alloc);
    for (uint64_t k = start; k < stop; ++k) {
      steps.Add({k + 1u, k});
    }
    const uint64_t t0 = bench::NowNs();
    next_2(db, log, functors, std::move(steps));
    seed_wall += bench::NowNs() - t0;
    seed_epochs += 1u;
  }
  tsv.Row(-1, "seed_epochs", seed_epochs);
  tsv.Row(-1, "seed_wall_ns", seed_wall);
  tsv.Row(-1, "seed_edges", depth);

  // ---- initial base(depth) add: the whole chain derives ----
  {
    hyde::rt::Vec<Tup_u64> add(input_alloc);
    hyde::rt::Vec<Tup_u64> rem(input_alloc);
    add.Add({depth});
    const uint64_t t0 = bench::NowNs();
    base_1(db, log, functors, std::move(add), std::move(rem));
    tsv.Row(-1, "init_add_wall_ns", bench::NowNs() - t0);
  }

  // ---- churn phase: per cycle, retract base(depth) then re-add it ----
  // The retract is the FULL-CHAIN overdelete cascade (every reach(k) marked
  // unknown, rechecked in ascending key order -> the deep recursion shape);
  // the re-add is the full rederive. Epoch = cycle index; metric distinguishes
  // retract from reseed so downstream can separate the two phases.
  uint64_t retract_block_wall = 0u;
  uint64_t reseed_block_wall = 0u;
  for (uint64_t c = 0u; c < cycles; ++c) {
    const int64_t epoch = static_cast<int64_t>(c);

    // Retract base(depth): the overdelete cascade.
    {
      hyde::rt::Vec<Tup_u64> add(input_alloc);
      hyde::rt::Vec<Tup_u64> rem(input_alloc);
      rem.Add({depth});
#ifdef DRLOJEKYLL_BENCH_COUNTERS
      const hyde::rt::BenchCounters ctr_before = hyde::rt::gBenchCounters;
#endif
      const uint64_t t0 = bench::NowNs();
      base_1(db, log, functors, std::move(add), std::move(rem));
      const uint64_t dt = bench::NowNs() - t0;
#ifdef DRLOJEKYLL_BENCH_COUNTERS
      EmitCounterDeltas(tsv, epoch, ctr_before, hyde::rt::gBenchCounters);
#endif
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

    // Re-add base(depth): the full rederive.
    {
      hyde::rt::Vec<Tup_u64> add(input_alloc);
      hyde::rt::Vec<Tup_u64> rem(input_alloc);
      add.Add({depth});
#ifdef DRLOJEKYLL_BENCH_COUNTERS
      const hyde::rt::BenchCounters ctr_before = hyde::rt::gBenchCounters;
#endif
      const uint64_t t0 = bench::NowNs();
      base_1(db, log, functors, std::move(add), std::move(rem));
      const uint64_t dt = bench::NowNs() - t0;
#ifdef DRLOJEKYLL_BENCH_COUNTERS
      EmitCounterDeltas(tsv, epoch, ctr_before, hyde::rt::gBenchCounters);
#endif
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

  // ---- untimed tail: query drain + sentinel (clock stopped) ----
  // After the final cycle the chain is re-added, so reach holds every k in
  // [0, depth]: exactly depth+1 rows. Drain ascending, count, and FNV-hash.
  {
    const uint64_t q0 = bench::NowNs();
    std::vector<uint64_t> vals;
    auto cur = reach_out_f(db);
    for (uint64_t x = 0u; cur.next(x);) {
      vals.push_back(x);
    }
    const uint64_t q_wall = bench::NowNs() - q0;
    tsv.Row(-1, "final_query_wall_ns", q_wall);
    std::sort(vals.begin(), vals.end());
    bench::Fnv fnv;
    for (uint64_t v : vals) {
      fnv.Add(v);
    }
    tsv.Row(-1, "final_reach_count", vals.size());
    tsv.Row(-1, "final_reach_hash", fnv.h);
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
