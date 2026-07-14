#include <algorithm>
#include <cstdint>
#include <iostream>

#include "datalog.h"

// Chain depth. The retraction of base(kChainDepth) makes every reach(k)
// unknown, and the recheck's first checker call (on reach(0), the deepest
// row) recurses through all kChainDepth unresolved predecessors, costing
// one native stack frame per link. In today's debug build the probe crashes
// with a stack overflow (exit 139) at 14000 and passes at 13000; 6000 is
// roughly half the largest passing depth, leaving margin across
// optimization modes. Stage 3 acceptance gate for the derivation-counter
// swap (docs/proposals/StackSafeNegation.md): kChainDepth = 100000 running
// in constant stack.
static constexpr uint64_t kChainDepth = 6000;

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  auto dump = [&db](const char *label) {
    uint64_t count = 0;
    uint64_t min = ~0ull;
    uint64_t max = 0;
    auto c = reach_out_f(db);
    for (uint64_t x = 0; c.next(x);) {
      ++count;
      min = std::min(min, x);
      max = std::max(max, x);
    }
    std::cout << label << ": count=" << count;
    if (count) {
      std::cout << " min=" << min << " max=" << max;
    }
    std::cout << '\n';
  };

  // Descending chain: next(k+1, k) for k in [0, kChainDepth), so reach
  // derives downward from base(kChainDepth) to reach(0).
  {
    hyde::rt::Vec<Tup_u64_u64> steps(allocator);
    for (uint64_t i = 0; i < kChainDepth; ++i) {
      steps.Add({i + 1, i});
    }
    next_2(db, log, functors, std::move(steps));
  }
  {
    hyde::rt::Vec<Tup_u64> add(allocator);
    hyde::rt::Vec<Tup_u64> rem(allocator);
    add.Add({kChainDepth});
    base_1(db, log, functors, std::move(add), std::move(rem));
  }
  dump("after seed");

  // Retract the sole base fact: the entire chain must cascade away.
  {
    hyde::rt::Vec<Tup_u64> add(allocator);
    hyde::rt::Vec<Tup_u64> rem(allocator);
    rem.Add({kChainDepth});
    base_1(db, log, functors, std::move(add), std::move(rem));
  }
  dump("after retract");
  return 0;
}
