// Copyright 2026, Peter Goodman. All rights reserved.
//
// Shared driver kit for the bench harness (PerfRoadmap.md §2/§6.3). Pure
// driver-side code: nothing here touches lib/Runtime or the generated
// artifacts. Every bench binary (engine drivers and hand-written baselines)
// emits the same flat TSV schema on stdout:
//
//   workload <TAB> knobs <TAB> mode <TAB> rep <TAB> epoch <TAB> metric <TAB> value
//
// knobs is the canonical sorted "k=v,k=v" string; epoch is an integer for
// per-epoch series rows and "-" for run-scoped rows; value is always an
// integer (ns, bytes, or a count — no floats in the file). A run that ends
// cleanly emits a final run-scoped `run_complete = 1` row; the runner folds
// only fragments carrying that marker into results.tsv, so a mid-run abort
// (counter saturation, row-id exhaustion, sentinel mismatch) is detectable
// rather than a silently short series.

#pragma once

#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/resource.h>

namespace bench {

// ---------------------------------------------------------------- timing --

inline uint64_t NowNs(void) {
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
}

// Measured cost of one empty timed region (two clock reads), averaged over
// enough iterations to be stable. Emitted as a run-scoped metric so drift
// slopes are only trusted where the per-point signal clears this floor by
// a stated factor (BASELINE.md methodology).
inline uint64_t ClockOverheadNs(void) {
  constexpr uint64_t kIters = 1u << 20u;
  const uint64_t begin = NowNs();
  uint64_t sink = 0u;
  for (uint64_t i = 0u; i < kIters; ++i) {
    sink ^= NowNs();
  }
  const uint64_t end = NowNs();
  // Keep `sink` observable so the loop is not elided.
  if (sink == 1u) {
    std::fputs("", stderr);
  }
  return (end - begin) / kIters;
}

// Peak resident set size. On macOS ru_maxrss is in BYTES (on Linux it is
// KiB); this harness is calibrated for the macOS dev machine and records
// the platform in the run header.
inline uint64_t PeakRssBytes(void) {
  struct rusage ru;
  getrusage(RUSAGE_SELF, &ru);
#ifdef __APPLE__
  return static_cast<uint64_t>(ru.ru_maxrss);
#else
  return static_cast<uint64_t>(ru.ru_maxrss) * 1024u;
#endif
}

// A fixed database-independent spin of work, timed. Interleaved as a canary
// series in long drift runs: machine-level drift (thermal throttle, core
// migration) shows up in the canary; database-state drift (log bloat) does
// not. Overlay the two before attributing a workload slope to state growth.
inline uint64_t CanaryWallNs(void) {
  uint64_t x = 0x9e3779b97f4a7c15ull;
  const uint64_t begin = NowNs();
  for (uint32_t i = 0u; i < (1u << 22u); ++i) {
    x ^= x << 13u;
    x ^= x >> 7u;
    x ^= x << 17u;
  }
  const uint64_t end = NowNs();
  if (x == 1u) {
    std::fputs("", stderr);
  }
  return end - begin;
}

// ------------------------------------------------------------------ PRNG --

// splitmix64: deterministic across platforms, seedable per knob.
struct Rng {
  uint64_t state;

  explicit Rng(uint64_t seed) : state(seed) {}

  uint64_t Next(void) {
    state += 0x9e3779b97f4a7c15ull;
    uint64_t z = state;
    z = (z ^ (z >> 30u)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27u)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31u);
  }

  // Uniform in [0, n). n must be nonzero.
  uint64_t Below(uint64_t n) {
    return Next() % n;
  }
};

// ------------------------------------------------------------------- FNV --

// Canonical live-set hash: FNV-1a over a sequence of little-endian u64
// values. Every binary at the same knob-point (engine in any mode, both
// baselines) must emit the same `final_*_hash` for the same live set; the
// runner cross-checks. Feed pairs in ascending (from, to) order.
struct Fnv {
  uint64_t h{0xcbf29ce484222325ull};

  void Add(uint64_t v) {
    for (uint32_t i = 0u; i < 8u; ++i) {
      h ^= (v >> (i * 8u)) & 0xffu;
      h *= 0x100000001b3ull;
    }
  }
};

// ------------------------------------------------------------------ knobs --

// Every CLI argument is `key=value`. Missing keys take defaults; unknown
// keys are a hard error (a typo'd knob must never silently run defaults).
struct Knobs {
  int argc;
  char **argv;
  // Bitmask of consumed argv entries so Finish() can reject strays.
  uint64_t used{0u};

  Knobs(int argc_, char **argv_) : argc(argc_), argv(argv_) {}

  const char *Raw(const char *key) {
    const size_t len = std::strlen(key);
    for (int i = 1; i < argc; ++i) {
      if (!std::strncmp(argv[i], key, len) && argv[i][len] == '=') {
        used |= 1ull << i;
        return argv[i] + len + 1u;
      }
    }
    return nullptr;
  }

  uint64_t U64(const char *key, uint64_t def) {
    const char *v = Raw(key);
    if (!v) {
      return def;
    }
    char *end = nullptr;
    const uint64_t parsed = std::strtoull(v, &end, 10);
    // Reject trailing garbage: an unsplit shell variable gluing several
    // knobs into one argv entry must fail loudly, not run mostly-defaults.
    if (end == v || *end != '\0') {
      std::fprintf(stderr, "malformed knob value: %s=%s\n", key, v);
      std::exit(2);
    }
    return parsed;
  }

  std::string Str(const char *key, const char *def) {
    const char *v = Raw(key);
    return v ? std::string(v) : std::string(def);
  }

  // Call after all knobs are read.
  void Finish(void) {
    for (int i = 1; i < argc; ++i) {
      if (!(used & (1ull << i))) {
        std::fprintf(stderr, "unknown knob: %s\n", argv[i]);
        std::exit(2);
      }
    }
  }
};

// ------------------------------------------------------------------- TSV --

struct Tsv {
  std::string workload;
  std::string knobs;  // canonical sorted k=v,k=v
  std::string mode;   // opt|nodf|nocf|none|debug — semantic key, never a hash
  uint64_t rep;

  void Row(int64_t epoch, const char *metric, uint64_t value) const {
    if (epoch < 0) {
      std::printf("%s\t%s\t%s\t%" PRIu64 "\t-\t%s\t%" PRIu64 "\n",
                  workload.c_str(), knobs.c_str(), mode.c_str(), rep, metric,
                  value);
    } else {
      std::printf("%s\t%s\t%s\t%" PRIu64 "\t%" PRId64 "\t%s\t%" PRIu64 "\n",
                  workload.c_str(), knobs.c_str(), mode.c_str(), rep, epoch,
                  metric, value);
    }
  }

  // The clean-completion marker. Emit as the LAST row of a run; the runner
  // discards fragments without it.
  void Complete(void) const {
    Row(-1, "run_complete", 1u);
    std::fflush(stdout);
  }
};

// ------------------------------------------------- counting allocation seam --

// Wraps a backing allocator (normally malloc) in count/byte accounting.
// Pure driver code: the runtime's Allocator is a by-value {ctx, fns} pair,
// so this needs no runtime change (PerfRoadmap §6.3). Under the arena knob
// the counts describe CHUNK traffic, not container ops — and arena RSS is
// cumulative reservation including grown-Vec leak-by-design, NOT live bytes
// (Allocator.cpp: ArenaFree is a no-op); BASELINE.md carries the caveat.
struct AllocStats {
  uint64_t alloc_count{0u};
  uint64_t alloc_bytes{0u};
  uint64_t free_count{0u};
  uint64_t free_bytes{0u};
};

inline void *CountingAlloc(void *ctx, size_t size, size_t align) {
  auto stats = static_cast<AllocStats *>(ctx);
  stats->alloc_count += 1u;
  stats->alloc_bytes += size;
  if (align <= alignof(std::max_align_t)) {
    return std::malloc(size);
  }
  return std::aligned_alloc(align, (size + align - 1u) / align * align);
}

inline void CountingFree(void *ctx, void *ptr, size_t size, size_t) {
  auto stats = static_cast<AllocStats *>(ctx);
  stats->free_count += 1u;
  stats->free_bytes += size;
  std::free(ptr);
}

}  // namespace bench
