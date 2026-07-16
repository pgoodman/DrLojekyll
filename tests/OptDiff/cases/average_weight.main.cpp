// Driver for average_weight (R3 aggregate flagship). ADL hidden-friend surface;
// C-5 driver-supplied reduction free functions (@invertible sum/count →
// identity/combine/uncombine; @recompute KV merge → reduce); free MAP
// functions (div_i32, new_weight_i32) defined out-of-line. The per-X query
// drains are SORTED before printing (cursor contract, CLAUDE.md).
//
// FUNCTOR SEMANTICS (must match the oracle's by-name interpretation, spec §4):
//   sum_i32   : running sum (identity 0, combine +, uncombine -)
//   count_i32 : member count (identity 0, combine +1, uncombine -1)
//   new_weight_i32 (KV merge, @recompute) : the surviving/last value
//   div_i32   : integer division (RHS != 0)
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#include "datalog.h"

int32_t sum_i32_identity() { return 0; }
int32_t sum_i32_combine(int32_t w, int32_t v) { return w + v; }
int32_t sum_i32_uncombine(int32_t w, int32_t v) { return w - v; }

int32_t count_i32_identity() { return 0; }
int32_t count_i32_combine(int32_t w, int32_t) { return w + 1; }
int32_t count_i32_uncombine(int32_t w, int32_t) { return w - 1; }

// @recompute KV merge over the live multiset: the surviving value (a KV index
// holds one value per key; the last live value wins).
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

int32_t div_i32_bbf(int32_t LHS, int32_t RHS) {
  return RHS != 0 ? LHS / RHS : 0;
}
int32_t new_weight_i32_bbf(int32_t /*OldWeight*/,
                           int32_t NewWeight) {
  return NewWeight;  // last-writer merge (matches the @recompute rescan)
}

static void Dump(Database &db) {
  for (int32_t x = 0; x <= 6; ++x) {
    std::vector<int32_t> avgs;
    auto c = average_incoming_weight_bf(db, x);
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
    vec.Add({5, 4, 100});
    add_edge_3(db, log, functors, std::move(vec));
  }
  Dump(db);

  // KV value-churn: re-key edge (1,2) to weight 30. The surviving sum into
  // node 2 becomes 30 + 20 = 50 over 2 members -> avg 25.
  {
    hyde::rt::Vec<add_edge_input> vec(allocator);
    vec.Add({1, 2, 30});
    add_edge_3(db, log, functors, std::move(vec));
  }
  Dump(db);
  return 0;
}
