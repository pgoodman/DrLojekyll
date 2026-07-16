// Driver for pairwise_average_weight (R3 pure-KV flagship). ADL hidden-friend
// surface; C-5 @recompute KV merge reduce; DatabaseFunctors MAP members
// (add_i32, div_i32, new_weight_i32) out-of-line. Per-X query drains are SORTED
// before printing (cursor contract, CLAUDE.md).
//
// FUNCTOR SEMANTICS (must match the oracle, spec §4):
//   new_weight_i32 (KV merge, @recompute) : surviving/last value
//   add_i32 : LHS + RHS ; div_i32 : integer division (RHS != 0)
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#include "datalog.h"

int32_t new_weight_i32_reduce(const int32_t *values, const int32_t *counts,
                              std::size_t n) {
  int32_t res = 0;
  for (std::size_t i = 0; i < n; ++i) {
    if (counts[i] > 0) {
      res = values[i];
    }
  }
  return res;
}

int32_t DatabaseFunctors::add_i32_bbf(int32_t LHS, int32_t RHS) {
  return LHS + RHS;
}
int32_t DatabaseFunctors::div_i32_bbf(int32_t LHS, int32_t RHS) {
  return RHS != 0 ? LHS / RHS : 0;
}
int32_t DatabaseFunctors::new_weight_i32_bbf(int32_t /*OldWeight*/,
                                             int32_t NewWeight) {
  return NewWeight;
}

static void Dump(Database &db) {
  for (int32_t x = 0; x <= 6; ++x) {
    std::vector<int32_t> avgs;
    auto c = pairwise_average_weight_bf(db, x);
    for (int32_t a; c.next(a);) {
      avgs.push_back(a);
    }
    std::sort(avgs.begin(), avgs.end());
    for (auto a : avgs) {
      std::cout << x << " " << a << "\n";
    }
  }
  std::cout << "--\n";
}

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  {
    hyde::rt::Vec<add_edge_input> vec(allocator);
    vec.Add({1, 2, 10});
    vec.Add({3, 2, 20});
    vec.Add({5, 2, 60});
    add_edge_3(db, log, functors, std::move(vec));
  }
  Dump(db);

  // KV value-churn: re-key edge (1,2) to weight 40; the pairwise averages of
  // node 2's incoming edges shift as the (1,2) member's value changes.
  {
    hyde::rt::Vec<add_edge_input> vec(allocator);
    vec.Add({1, 2, 40});
    add_edge_3(db, log, functors, std::move(vec));
  }
  Dump(db);
  return 0;
}
