// Copyright 2026, Peter Goodman. All rights reserved.
//
// demand_tc engine driver: the S1b demand-pruning carrier (record:
// docs/proposals/RegionalDataFlowCore.artifacts/s1b-bench-carrier.md).
// The SAME driver serves the plain and the `-demand` compiles of
// demand_tc.dr — the probe call adapts at compile time to whichever query
// signature the generated header exposes (a forced query takes
// (db, log, functors, bound...); an unforced one takes (db, bound...)), so
// the runspec selects the transform purely via the harness `drflags=` knob.
//
// Dataset: nchain disjoint chains of length len (chain c occupies node ids
// [c*(len+1), c*(len+1)+len]). Full all-pairs closure = nchain*len*(len+1)/2
// rows; one head's demanded slice = len rows.
//
// Knobs (key=value args): nchain, len, probe (selective|all), probes
// (selective head-probe count), mode (echoed into TSV), rep.
//
// Timed regions: the ingest entry call (epoch 0) and the whole probe phase
// (epoch 1) — Vec construction and the sentinel fold sit outside the
// brackets. The COUNTS binary additionally emits ctr_* deltas per epoch.
// Sentinel: final_answer_count / final_answer_hash over the probed answers,
// order-independent (per-probe sorted values) — plain and demand binaries
// at one knob-point MUST agree (the harness sentinel check enforces it
// across modes AND drflags variants, since both label sets share one
// (workload, knobs) key).

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "datalog.h"

#include "../../common/bench.h"

namespace {

#ifdef DRLOJEKYLL_BENCH_COUNTERS
void EmitCounterDeltas(const bench::Tsv &tsv, int64_t epoch,
                       const hyde::rt::BenchCounters &before,
                       const hyde::rt::BenchCounters &after) {
#define HYDE_RT_BENCH_EMIT(name) \
  tsv.Row(epoch, "ctr_" #name, after.name - before.name);
  HYDE_RT_BENCH_COUNTER_FIELDS(HYDE_RT_BENCH_EMIT)
#undef HYDE_RT_BENCH_EMIT
}
#define SNAP_COUNTERS(var) const auto var = hyde::rt::gBenchCounters
#else
#define SNAP_COUNTERS(var) do {} while (0)
#endif

}  // namespace

int main(int argc, char **argv) {
  bench::Knobs knobs(argc, argv);
  const uint64_t nchain = knobs.U64("nchain", 4000u);
  const uint64_t len = knobs.U64("len", 10u);
  const std::string probe_kind = knobs.Str("probe", "selective");
  const uint64_t probes = knobs.U64("probes", 8u);
  const std::string mode = knobs.Str("mode", "opt");
  const uint64_t rep = knobs.U64("rep", 0u);
  knobs.Finish();

  const bool selective = probe_kind == "selective";
  if (!selective && probe_kind != "all") {
    std::fprintf(stderr, "probe must be selective|all\n");
    return 2;
  }

  bench::Tsv tsv;
  tsv.workload = "demand_tc";
  tsv.knobs = "len=" + std::to_string(len) + ",nchain=" +
              std::to_string(nchain) + ",probe=" + probe_kind +
              ",probes=" + std::to_string(probes);
  tsv.mode = mode;
  tsv.rep = rep;
  tsv.Row(-1, "clock_overhead_ns", bench::ClockOverheadNs());

  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  // ---- epoch 0: ingest (all edges, one batch) ---------------------------
  hyde::rt::Vec<edge_2_input> edges(allocator);
  for (uint64_t c = 0; c < nchain; ++c) {
    const uint64_t base = c * (len + 1u);
    for (uint64_t i = 0; i < len; ++i) {
      edges.Add({base + i, base + i + 1u});
    }
  }
  SNAP_COUNTERS(c0);
  const uint64_t t0 = bench::NowNs();
  edge_2_2(db, log, functors, std::move(edges));
  const uint64_t t1 = bench::NowNs();
  tsv.Row(0, "t_ingest_ns", t1 - t0);
#ifdef DRLOJEKYLL_BENCH_COUNTERS
  EmitCounterDeltas(tsv, 0, c0, hyde::rt::gBenchCounters);
#endif

  // ---- epoch 1: the probe phase ----------------------------------------
  uint64_t rows = 0u;
  bench::Fnv fnv;
  // Compile-time signature adaptation: a forced (demand) query takes
  // (db, log, functors, bound); a plain one takes (db, bound). The database
  // flows through a GENERIC lambda parameter so the requires-expression is
  // dependent (evaluated per-instantiation, never a hard error).
  const auto open_cursor = [&](auto &db_ref, uint64_t f) {
    if constexpr (requires { reachable_from_bf(db_ref, log, functors, f); }) {
      return reachable_from_bf(db_ref, log, functors, f);
    } else {
      return reachable_from_bf(db_ref, f);
    }
  };
  const auto probe = [&](uint64_t f) {
    auto cursor = open_cursor(db, f);
    std::vector<uint64_t> tos;
    for (uint64_t t = 0u; cursor.next(t);) {
      tos.push_back(t);
    }
    // Keyed-cursor order is unspecified: sort before hashing.
    std::sort(tos.begin(), tos.end());
    fnv.Add(f);
    for (uint64_t t : tos) {
      fnv.Add(t);
      ++rows;
    }
  };

  SNAP_COUNTERS(c1);
  const uint64_t t2 = bench::NowNs();
  if (selective) {
    for (uint64_t p = 0; p < probes && p < nchain; ++p) {
      probe(p * (len + 1u));  // chain heads
    }
  } else {
    for (uint64_t c = 0; c < nchain; ++c) {
      const uint64_t base = c * (len + 1u);
      for (uint64_t i = 0; i <= len; ++i) {
        probe(base + i);  // every node
      }
    }
  }
  const uint64_t t3 = bench::NowNs();
  tsv.Row(1, "t_probe_ns", t3 - t2);
#ifdef DRLOJEKYLL_BENCH_COUNTERS
  EmitCounterDeltas(tsv, 1, c1, hyde::rt::gBenchCounters);
#endif

  tsv.Row(-1, "peak_rss_bytes", bench::PeakRssBytes());
  tsv.Row(-1, "final_answer_count", rows);
  tsv.Row(-1, "final_answer_hash", fnv.h);
  tsv.Complete();
  return 0;
}
