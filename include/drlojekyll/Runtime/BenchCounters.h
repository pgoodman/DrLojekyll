// Copyright 2026, Trail of Bits. All rights reserved.

#pragma once

// Bench-only operation counters (PerfRoadmap.md §2/§6.3; bench/BASELINE.md
// methodology). DEFAULT-OFF: without -DDRLOJEKYLL_BENCH_COUNTERS this
// header contributes no declarations and the count macros expand to
// nothing — the golden-compared build is a verified no-op (suite PASS +
// object-file byte-compare, see the bench README). With the define, a
// single process-global counter struct tallies runtime operations; the
// COUNTS binary that enables it is never the binary whose wall time is
// recorded (counts and wall are separate narratives — the increments in
// FindWithHash's probe loop and NetBatch's inner scan deliberately trade
// timing fidelity for attribution).
//
// Drivers snapshot `gBenchCounters` (trivially copyable) around an epoch
// and emit field deltas; the X-macro keeps the dump generic.

#ifdef DRLOJEKYLL_BENCH_COUNTERS

#include <cstdint>

#define HYDE_RT_BENCH_COUNTER_FIELDS(X) \
  X(folds_plus)            /* FoldAt, delta > 0 */ \
  X(folds_minus)           /* FoldAt, delta < 0 */ \
  X(finds)                 /* RowStore::FindWithHash calls */ \
  X(probe_steps)           /* open-addressing slots inspected (hot!) */ \
  X(rehash_events)         /* RowStore::Rehash calls */ \
  X(rehash_rows)           /* rows re-linked across all Rehash calls */ \
  X(claim_calls_del)       /* TryClaimDel calls */ \
  X(claim_calls_add)       /* TryClaimAdd calls */ \
  X(claims_del)            /* ... that claimed */ \
  X(claims_add) \
  X(stale_drops_del)       /* ... rejected by the C_nr<=0 re-test */ \
  X(stale_drops_add)       /* ... rejected by the Total>0 re-test */ \
  X(retires)               /* RetireDel + RetireAdd */ \
  X(touch_calls)           /* Touch calls (the OQ6-pivot numerator) */ \
  X(touch_appends)         /* first-touch appends (the denominator) */ \
  X(member_checks)         /* named membership predicate reads */ \
  X(present_checks)        /* Present() reads (cursor liveness filters) */ \
  X(commit_visits)         /* touched rows walked by Commit */ \
  X(commit_publishes)      /* was!=now sink calls */ \
  X(idx_adds)              /* Index::Add */ \
  X(idx_first)             /* Index::First probes */ \
  X(idx_hops)              /* Index::Next chain hops (hot!) */ \
  X(idx_rehash_events)     /* Index::Rehash calls */ \
  X(sort_calls)            /* Vec::SortAndUnique calls */ \
  X(sort_elems)            /* pre-sort element counts (call site) */ \
  X(netbatch_calls)        /* NetBatch invocations */ \
  X(netbatch_compares)     /* NetBatch distinct-scan row compares (hot!) */

namespace hyde::rt {

struct BenchCounters {
#define HYDE_RT_BENCH_FIELD(name) uint64_t name{0u};
  HYDE_RT_BENCH_COUNTER_FIELDS(HYDE_RT_BENCH_FIELD)
#undef HYDE_RT_BENCH_FIELD
};

inline BenchCounters gBenchCounters{};

}  // namespace hyde::rt

#define HYDE_RT_BENCH_COUNT(field) \
  ((void) (::hyde::rt::gBenchCounters.field += 1u))
#define HYDE_RT_BENCH_COUNT_N(field, n) \
  ((void) (::hyde::rt::gBenchCounters.field += \
           static_cast<uint64_t>(n)))

#else  // !DRLOJEKYLL_BENCH_COUNTERS

#define HYDE_RT_BENCH_COUNT(field) ((void) 0)
#define HYDE_RT_BENCH_COUNT_N(field, n) ((void) 0)

#endif  // DRLOJEKYLL_BENCH_COUNTERS
