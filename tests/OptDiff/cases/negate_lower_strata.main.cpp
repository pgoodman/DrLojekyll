#include <algorithm>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "datalog.h"

// Lower-stratum negated view standing fixture (checkpoint (d), analysis
// sec 4.5 R9). The negated view blk is STRICTLY BELOW the pred mid (mid is
// derived through the extra hop raw->step->mid). The oracle orders the
// negate rule's atoms by strata (Convention B, negated-first); the runtime
// uses global convention A. This fixture pins their OBSERVATIONAL
// EQUIVALENCE: identical published out on every batch, including the
// differential block flip.
//
// Oracle-verified per-batch truth:
//   after batch 1 (feed 1/2/3, block 2): out = {(1,10),(3,30)}
//   after batch 2 (block 1):             out = {(3,30)}
//   after batch 3 (unblock 2):           out = {(2,20),(3,30)}
//   after batch 4 (+block 3/-block 3):   out = {(2,20),(3,30)}  (net zero)

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator, log, functors);

  auto dump = [&db](const char *label) {
    std::vector<std::pair<uint64_t, uint64_t>> rows;
    auto c = db.out_ff();
    for (uint64_t k = 0, v = 0; c.next(k, v);) {
      rows.emplace_back(k, v);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << label << ": out:";
    for (auto &[k, v] : rows) {
      std::cout << " (" << k << ',' << v << ')';
    }
    std::cout << '\n';
  };

  auto block = [&](std::vector<uint64_t> adds, std::vector<uint64_t> rems) {
    hyde::rt::Vec<Tup_u64> add(allocator);
    hyde::rt::Vec<Tup_u64> rem(allocator);
    for (auto k : adds) add.Add({k});
    for (auto k : rems) rem.Add({k});
    db.block_1(std::move(add), std::move(rem));
  };

  // Batch 1: feed keys, block 2.
  {
    hyde::rt::Vec<Tup_u64_u64> f(allocator);
    f.Add({1, 10});
    f.Add({2, 20});
    f.Add({3, 30});
    db.feed_2(std::move(f));
  }
  block({2}, {});
  dump("after feed+block(2)");

  // Batch 2: block 1.
  block({1}, {});
  dump("after block(1)");

  // Batch 3: unblock 2.
  block({}, {2});
  dump("after unblock(2)");

  // Batch 4: same-batch flap +block 3 / -block 3, nets zero.
  block({3}, {3});
  dump("after flap block(3)");
  return 0;
}
