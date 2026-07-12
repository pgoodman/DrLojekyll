#include <algorithm>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "datalog.h"

// Downstream-differential negate standing fixture (checkpoint (d), analysis
// sec 4.5 R9). A MONOTONE-negated negate (out) whose output feeds a further
// differential rule one stratum down:
//   copy(A, B)     : feed(A, B).
//   seen(A)        : unsee(A).
//   out(A, B)      : copy(A, B), !seen(A).      monotone-negated negate
//   flag_out(A, B) : out(A, B), active(A).      downstream differential join
// This discriminates the R3 seed-before-drain ordering: a phantom `+`
// claimed on out (crossover fold after the claim drains) would leak a
// spurious NetAdded frontier entry that flag_out's differential stratum
// would DOUBLE-COUNT. flag_out must track out x active exactly.
//
// Oracle-verified per-batch truth:
//   after batch 1 (feed 1/2, active 1/2):  flag_out = {(1,10),(2,20)}
//   after batch 2 (unsee 1):               flag_out = {(2,20)}
//   after batch 3 (-active 2, +active 3):  flag_out = {}
//   after batch 4 (feed 3):                flag_out = {(3,30)}

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator, log, functors);

  auto dump = [&db](const char *label) {
    std::vector<std::pair<uint64_t, uint64_t>> rows;
    auto c = db.flag_out_ff();
    for (uint64_t a = 0, b = 0; c.next(a, b);) {
      rows.emplace_back(a, b);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << label << ": flag_out:";
    for (auto &[a, b] : rows) {
      std::cout << " (" << a << ',' << b << ')';
    }
    std::cout << '\n';
  };

  auto feed = [&](std::vector<std::pair<uint64_t, uint64_t>> rows) {
    hyde::rt::Vec<Tup_u64_u64> v(allocator);
    for (auto [a, b] : rows) v.Add({a, b});
    db.feed_2(std::move(v));
  };
  auto unsee = [&](std::vector<uint64_t> rows) {
    hyde::rt::Vec<Tup_u64> v(allocator);
    for (auto a : rows) v.Add({a});
    db.unsee_1(std::move(v));
  };
  auto active = [&](std::vector<uint64_t> adds, std::vector<uint64_t> rems) {
    hyde::rt::Vec<Tup_u64> add(allocator);
    hyde::rt::Vec<Tup_u64> rem(allocator);
    for (auto a : adds) add.Add({a});
    for (auto a : rems) rem.Add({a});
    db.active_1(std::move(add), std::move(rem));
  };

  // Batch 1: feed keys 1,2; active 1,2.
  feed({{1, 10}, {2, 20}});
  active({1, 2}, {});
  dump("after feed+active(1,2)");

  // Batch 2: unsee 1 -> `-` crossover retracts out(1,10) and flag_out(1,10).
  unsee({1});
  dump("after unsee(1)");

  // Batch 3: active flip -active 2 / +active 3 in one epoch.
  active({3}, {2});
  dump("after active flip");

  // Batch 4: feed key 3; active 3 present -> flag_out(3,30).
  feed({{3, 30}});
  dump("after feed(3,30)");
  return 0;
}
