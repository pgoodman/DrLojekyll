// Copyright 2026, Peter Goodman. All rights reserved.
//
// disasm_synth engine driver (design R19: the realistic-program point).
// Times the generated recursive disassembler (the hidden-friend surface,
// one Database per run) as it INGESTS a synthetic instruction/transfer
// stream. Compile (the diffrun 3-TU line at bench flags):
//
//   drlojekyll bench/workloads/disasm_synth/disasm.dr [-disable-*-opt] \
//       -cpp-out GEN/
//   clang++ -std=c++23 -O2 -DNDEBUG -I include -I GEN \
//       bench/workloads/disasm_synth/driver.cpp GEN/mini_disassembler.cpp \
//       lib/Runtime/Allocator.cpp -o disasm_synth_engine
//
// NOTE the generated header/anchor are named after the #database:
// mini_disassembler.h / mini_disassembler.cpp (not datalog.*). Types live
// in namespace mini_disassembler; the driver-facing calls are unqualified
// ADL hidden friends (init, instruction_1, raw_transfer_3, function_b,
// function_instructions_bf) reached with the database as first argument.
//
// SCOPE: the message surface is MONOTONE (plain #message, add-only). There
// is no @differential relation, hence NO retraction and NO edit_batches
// knob -- this family measures INGEST/GROWTH only (engine epoch series +
// counters + compile metrics). There is deliberately NO hand-written COST
// baseline: a competent hand-incremental disassembler is out of scope this
// epoch (recorded as a caveat in BASELINE.md). The sentinel is CROSS-MODE
// agreement: a canonical dump (function count, instruction count, Fnv hash
// over sorted (funcEA, instEA)) identical across all 4 dr optimization
// modes at a small knob-point.
//
// Knobs (key=value): blocks (default 2000), block_len (mean, default 8),
// call_density (percent, default 20), orphan_pct (percent, default 5),
// base_ea (first EA, default 4096), bs (instructions/transfers per ingest
// batch, default 512), seed, rep, mode (echoed into TSV), alloc
// (malloc|arena), block (time K-epoch blocks for sub-clock points),
// canary (drift canary every N epochs; 0 = off).
//
// Timed region per ingest epoch = exactly ONE message entry call
// (instruction_1 or raw_transfer_3). Input Vec construction, canaries, the
// final query drain, and the sentinel all sit OUTSIDE the timed brackets
// (design R12; clock stopped before the sentinel).

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <utility>
#include <vector>

#include "mini_disassembler.h"

#include "../../common/bench.h"
#include "gen.h"

using namespace mini_disassembler;

namespace {

// COUNTS-binary support (compiled with -DDRLOJEKYLL_BENCH_COUNTERS): emit
// per-epoch counter deltas as ctr_* metric rows. This binary's wall rows
// are discarded downstream -- counts and wall are separate narratives.
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
  const uint64_t blocks = knobs.U64("blocks", 2000u);
  const uint64_t block_len = knobs.U64("block_len", 8u);
  const uint64_t call_density = knobs.U64("call_density", 20u);
  const uint64_t orphan_pct = knobs.U64("orphan_pct", 5u);
  const uint64_t base_ea = knobs.U64("base_ea", 4096u);
  const uint64_t bs = knobs.U64("bs", 512u);
  const std::string alloc_kind = knobs.Str("alloc", "malloc");
  const uint64_t seed = knobs.U64("seed", 1u);
  const uint64_t rep = knobs.U64("rep", 0u);
  const std::string mode = knobs.Str("mode", "opt");
  const uint64_t canary = knobs.U64("canary", 0u);
  const uint64_t block = knobs.U64("block", 1u);
  const uint64_t warmup = knobs.U64("warmup", 3u);
  knobs.Finish();

  // Every measurement-affecting knob is part of the canonical sorted key;
  // only mode and rep stay out (their own TSV columns).
  char knob_str[320];
  std::snprintf(knob_str, sizeof knob_str,
                "alloc=%s,base_ea=%llu,block=%llu,block_len=%llu,blocks=%llu,"
                "bs=%llu,call_density=%llu,canary=%llu,orphan_pct=%llu,"
                "seed=%llu",
                alloc_kind.c_str(), (unsigned long long) base_ea,
                (unsigned long long) block, (unsigned long long) block_len,
                (unsigned long long) blocks, (unsigned long long) bs,
                (unsigned long long) call_density,
                (unsigned long long) canary,
                (unsigned long long) orphan_pct, (unsigned long long) seed);
  const bench::Tsv tsv{"disasm_synth_engine", knob_str, mode, rep};

  tsv.Row(-1, "clock_overhead_ns", bench::ClockOverheadNs());

  // Build the whole synthetic program up front (untimed). Deterministic in
  // the knob string alone.
  const bench::DisasmStream stream =
      bench::GenDisasm(static_cast<uint32_t>(blocks),
                       static_cast<uint32_t>(block_len),
                       static_cast<uint32_t>(call_density),
                       static_cast<uint32_t>(orphan_pct), base_ea, seed);
  tsv.Row(-1, "gen_instructions", stream.instructions.size());
  tsv.Row(-1, "gen_transfers", stream.transfers.size());
  tsv.Row(-1, "gen_blocks", stream.block_heads.size());

  // Allocator: counting wrapper over malloc (default; RSS/alloc rows are
  // only comparable here -- design R9), or an arena whose counts describe
  // chunk traffic (BASELINE.md caveat). The database gets the counted
  // allocator; driver-side input Vecs use a separate UNCOUNTED allocator so
  // alloc_* rows describe database work only (same split as tc_random).
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

  // Epoch 0 (design R11: init wall is first-touch dominated; reported,
  // excluded from aggregates).
  {
    const uint64_t t0 = bench::NowNs();
    init(db, log, functors);
    tsv.Row(-1, "init_wall_ns", bench::NowNs() - t0);
  }

  tsv.Row(-1, "warmup_epochs", warmup);

  // ---- ingest phase ---------------------------------------------------
  // Two message streams, each batched at `bs`: first all instructions
  // (instruction_1), then all transfers (raw_transfer_3). Epoch index is a
  // single monotone counter across both phases so the TSV series is
  // continuous; the phase is recorded as a per-epoch metric so downstream
  // can split them. Timed region = exactly the one entry call.
  int64_t epoch = 0;
  uint64_t block_wall = 0u;
  uint64_t ins_epochs = 0u;
  uint64_t xfer_epochs = 0u;

  auto flush_block = [&](uint64_t dt, int64_t ep) {
    if (block <= 1u) {
      tsv.Row(ep, "epoch_wall_ns", dt);
    } else {
      block_wall += dt;
      if (((ep + 1) % static_cast<int64_t>(block)) == 0) {
        tsv.Row(ep, "block_wall_ns", block_wall);
        block_wall = 0u;
      }
    }
  };

  // Phase A: instructions.
  for (size_t i = 0u; i < stream.instructions.size(); i += bs) {
    const size_t end =
        std::min<size_t>(i + bs, stream.instructions.size());
    hyde::rt::Vec<Tup_u64> vec(input_alloc);
    for (size_t j = i; j < end; ++j) {
      vec.Add({stream.instructions[j]});
    }
    const uint64_t n = end - i;

#ifdef DRLOJEKYLL_BENCH_COUNTERS
    const hyde::rt::BenchCounters ctr_before = hyde::rt::gBenchCounters;
#endif
    const uint64_t t0 = bench::NowNs();
    instruction_1(db, log, functors, std::move(vec));
    const uint64_t dt = bench::NowNs() - t0;
#ifdef DRLOJEKYLL_BENCH_COUNTERS
    EmitCounterDeltas(tsv, epoch, ctr_before, hyde::rt::gBenchCounters);
#endif
    flush_block(dt, epoch);
    tsv.Row(epoch, "batch_size", n);
    tsv.Row(epoch, "phase", 0u);  // 0 = instructions
    if (canary && ((epoch + 1) % static_cast<int64_t>(canary) == 0)) {
      tsv.Row(epoch, "canary_wall_ns", bench::CanaryWallNs());
    }
    epoch += 1;
    ins_epochs += 1u;
  }

  // Phase B: transfers (fall-through + call, in generation order).
  for (size_t i = 0u; i < stream.transfers.size(); i += bs) {
    const size_t end = std::min<size_t>(i + bs, stream.transfers.size());
    hyde::rt::Vec<Tup_u64_u64_EdgeType> vec(input_alloc);
    for (size_t j = i; j < end; ++j) {
      const bench::Transfer &t = stream.transfers[j];
      vec.Add({t.from, t.to,
               t.kind ? EdgeType::CALL : EdgeType::FALL_THROUGH});
    }
    const uint64_t n = end - i;

#ifdef DRLOJEKYLL_BENCH_COUNTERS
    const hyde::rt::BenchCounters ctr_before = hyde::rt::gBenchCounters;
#endif
    const uint64_t t0 = bench::NowNs();
    raw_transfer_3(db, log, functors, std::move(vec));
    const uint64_t dt = bench::NowNs() - t0;
#ifdef DRLOJEKYLL_BENCH_COUNTERS
    EmitCounterDeltas(tsv, epoch, ctr_before, hyde::rt::gBenchCounters);
#endif
    flush_block(dt, epoch);
    tsv.Row(epoch, "batch_size", n);
    tsv.Row(epoch, "phase", 1u);  // 1 = transfers
    if (canary && ((epoch + 1) % static_cast<int64_t>(canary) == 0)) {
      tsv.Row(epoch, "canary_wall_ns", bench::CanaryWallNs());
    }
    epoch += 1;
    xfer_epochs += 1u;
  }

  tsv.Row(-1, "ins_epochs", ins_epochs);
  tsv.Row(-1, "xfer_epochs", xfer_epochs);

  // ---- untimed tail: query drain + canonical sentinel (clock stopped) --
  //
  // `function` is queryable only bound (function_b(db, ea)); we cannot
  // enumerate function heads directly. But we know every instruction EA
  // exactly from the stream, so we probe function_b at each one to find the
  // heads (function count), and drain function_instructions_bf(db, head)
  // per head to accumulate the instruction membership. The canonical dump
  // is the sorted set of (funcEA, instEA) pairs; its Fnv hash + the two
  // counts are the cross-MODE sentinel.
  {
    const uint64_t q0 = bench::NowNs();
    std::vector<std::pair<uint64_t, uint64_t>> pairs;
    uint64_t func_count = 0u;
    for (const uint64_t ea : stream.instructions) {
      if (!function_b(db, ea)) {
        continue;
      }
      func_count += 1u;
      auto c = function_instructions_bf(db, ea);
      for (uint64_t inst = 0u; c.next(inst);) {
        pairs.emplace_back(ea, inst);
      }
    }
    const uint64_t q_wall = bench::NowNs() - q0;
    tsv.Row(-1, "final_query_wall_ns", q_wall);

    // Canonicalize: sort + unique (a function_instructions cursor can, in
    // principle, surface a row more than once across index chains; the
    // ctest driver dedups too, so the sentinel is over the SET).
    std::sort(pairs.begin(), pairs.end());
    pairs.erase(std::unique(pairs.begin(), pairs.end()), pairs.end());

    bench::Fnv fnv;
    for (const auto &[f, i] : pairs) {
      fnv.Add(f);
      fnv.Add(i);
    }
    tsv.Row(-1, "final_function_count", func_count);
    tsv.Row(-1, "final_instruction_count", pairs.size());
    tsv.Row(-1, "final_disasm_hash", fnv.h);
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
