#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#include "datalog.h"

// Two-base-rule flip feeding a downstream stratum: the NetAdded-refinement
// discriminator (T2). p has two base rules keyed on the discriminator column
// Which of a single @differential message src; a downstream stratum
// q(X) : p(X), r(X) consumes p's net_additions frontier.
//   p(X) : src(1, X).
//   p(X) : src(2, X).
//   q(X) : p(X), r(X).
// The same-batch flip -src(1,5)/+src(2,5) leaves p(5)'s presence unchanged.
// With the refined NetAdded (kAdd && !kDel && !kInI) the already-present p(5)
// is NOT re-emitted into net_additions, so q(5) stays exactly once; the bare
// kAdd && !kDel would leak p(5) and re-derive/double-count q(5).
//
// Ground truth (positive-only equivalence oracle) final state:
//   after seed q = {5}
//   after flip q = {5}   (unchanged; p(5) still present via rule 2)
// (goldens/two_base_flip_downstream.oracle.stdout / .monotone.stdout).

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator, log, functors);

  auto dump = [&db](const char *label) {
    std::vector<uint64_t> rows;
    auto c = db.q_out_f();
    for (uint64_t x = 0; c.next(x);) {
      rows.push_back(x);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << label << ":\n";
    for (auto x : rows) {
      std::cout << "q(" << x << ")\n";
    }
  };

  // Seed: src(1,5) supports p(5) via rule 1; r(5) enables the downstream
  // join. q = {5}. (r seeded in its own batch, stable across the flip.)
  {
    hyde::rt::Vec<Tup_u64> add(allocator);
    hyde::rt::Vec<Tup_u64> rem(allocator);
    add.Add({5});
    db.r_1(std::move(add), std::move(rem));
  }
  {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    add.Add({1, 5});
    db.src_2(std::move(add), std::move(rem));
  }
  dump("after seed");

  // Flip: -src(1,5), +src(2,5) in ONE batch. p(5)'s presence never changes
  // (loses rule-1 support, gains rule-2 support same batch). q(5) must stay
  // exactly once (no spurious re-derivation).
  {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    add.Add({2, 5});
    rem.Add({1, 5});
    db.src_2(std::move(add), std::move(rem));
  }
  dump("after flip");
  return 0;
}
