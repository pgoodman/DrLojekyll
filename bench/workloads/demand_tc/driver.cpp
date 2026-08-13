// S1b bench carrier: the REAL -demand transform vs the plain program on the
// recursive-TC kPushDown shape. Counters narrative only (COUNTS binary,
// -DDRLOJEKYLL_BENCH_COUNTERS; wall time deliberately not recorded).
//
// Dataset: NCHAIN disjoint chains of length LEN (node ids: chain c occupies
// [c*(LEN+1), c*(LEN+1)+LEN]). Full all-pairs closure = NCHAIN*LEN*(LEN+1)/2
// rows; one head's demanded slice = LEN rows.
//
// Regimes (argv): "selective" probes PROBES chain heads; "all" probes every
// node (demand-set == full closure => the machinery-overhead regime).
//
// Equality surface: per-probe (count, FNV) folded into one aggregate
// count+hash — must byte-agree between the plain and demand binaries.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "datalog.h"

static uint64_t fnv1a(uint64_t h, uint64_t v) {
  for (int i = 0; i < 8; ++i) {
    h ^= (v >> (i * 8)) & 0xffu;
    h *= 0x100000001b3ull;
  }
  return h;
}

static hyde::rt::BenchCounters Snap() { return hyde::rt::gBenchCounters; }

static void DumpDelta(const char *phase, const hyde::rt::BenchCounters &a,
                      const hyde::rt::BenchCounters &b) {
#define HYDE_RT_BENCH_FIELD(name) \
  if (b.name != a.name) \
    std::printf("%s\t%s\t%llu\n", phase, #name, \
                (unsigned long long) (b.name - a.name));
  HYDE_RT_BENCH_COUNTER_FIELDS(HYDE_RT_BENCH_FIELD)
#undef HYDE_RT_BENCH_FIELD
}

int main(int argc, char **argv) {
  if (argc < 5) {
    std::fprintf(stderr, "usage: %s <nchain> <len> <selective|all> <probes>\n",
                 argv[0]);
    return 2;
  }
  const uint64_t nchain = std::strtoull(argv[1], nullptr, 10);
  const uint64_t len = std::strtoull(argv[2], nullptr, 10);
  const bool selective = !std::strcmp(argv[3], "selective");
  const uint64_t nprobes = std::strtoull(argv[4], nullptr, 10);

  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  const auto c0 = Snap();

  {
    hyde::rt::Vec<edge_2_input> edges(allocator);
    for (uint64_t c = 0; c < nchain; ++c) {
      const uint64_t base = c * (len + 1);
      for (uint64_t i = 0; i < len; ++i) {
        edges.Add({base + i, base + i + 1});
      }
    }
    edge_2_2(db, log, functors, std::move(edges));
  }

  const auto c1 = Snap();
  DumpDelta("ingest", c0, c1);

  uint64_t rows = 0, hash = 0xcbf29ce484222325ull;
  auto probe = [&](uint64_t f) {
#ifdef DEMAND_BUILD
    auto c = reachable_from_bf(db, log, functors, f);
#else
    auto c = reachable_from_bf(db, f);
#endif
    std::vector<uint64_t> tos;
    for (uint64_t t = 0; c.next(t);) {
      tos.push_back(t);
    }
    // Keyed-cursor order is unspecified: fold order-independently by
    // (key, sorted values) — sort before hashing.
    std::sort(tos.begin(), tos.end());
    hash = fnv1a(hash, f);
    for (uint64_t t : tos) {
      hash = fnv1a(hash, t);
      ++rows;
    }
  };

  if (selective) {
    for (uint64_t p = 0; p < nprobes && p < nchain; ++p) {
      probe(p * (len + 1));  // chain heads
    }
  } else {
    for (uint64_t c = 0; c < nchain; ++c) {
      const uint64_t base = c * (len + 1);
      for (uint64_t i = 0; i <= len; ++i) {
        probe(base + i);  // every node
      }
    }
  }

  const auto c2 = Snap();
  DumpDelta("probe", c1, c2);
  std::printf("answer\trows\t%llu\n", (unsigned long long) rows);
  std::printf("answer\thash\t%016llx\n", (unsigned long long) hash);
  return 0;
}
