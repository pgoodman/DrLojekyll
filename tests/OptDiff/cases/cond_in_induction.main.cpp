#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <utility>
#include <vector>

#include "datalog.h"

// Drives the same node/edge data through three relation families that differ
// only in when the condition-setting seed message arrives: interleaved with
// the data (t1), before all data (t2), and after all data (t3). Every family
// must converge to the same closure {1 2 3 4 5 6}; nodes 7 and 8 stay out
// because 7 is never in t. Intermediate dumps after every message pin the
// partially-gated states of each ordering.

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  auto collect = [](auto cursor) {
    std::vector<int32_t> v;
    for (int32_t x = 0; cursor.next(x);) {
      v.push_back(x);
    }
    std::sort(v.begin(), v.end());
    return v;
  };

  auto print = [](const char *label, const std::vector<int32_t> &v) {
    std::cout << label << ':';
    for (auto x : v) {
      std::cout << ' ' << x;
    }
    std::cout << '\n';
  };

  auto nodes = [&](std::initializer_list<int32_t> xs) {
    hyde::rt::Vec<Tup_i32> v(allocator);
    for (auto x : xs) {
      v.Add({x});
    }
    return v;
  };

  auto edges = [&](std::initializer_list<std::pair<int32_t, int32_t>> es) {
    hyde::rt::Vec<Tup_i32_i32> v(allocator);
    for (auto [a, b] : es) {
      v.Add({a, b});
    }
    return v;
  };

  // Family 1: seed interleaved -- the gate opens after part of the graph has
  // arrived, then more data flows through the already-open gate.
  print("t1", collect(t1_out_f(db)));
  m1_1(db, log, functors, nodes({1}));
  print("t1", collect(t1_out_f(db)));
  step1_2(db, log, functors, edges({{1, 2}, {2, 3}}));
  print("t1", collect(t1_out_f(db)));
  seed1_1(db, log, functors, nodes({7}));
  print("t1", collect(t1_out_f(db)));
  step1_2(db, log, functors, edges({{3, 4}, {4, 5}, {6, 3}, {3, 1}, {7, 8}}));
  print("t1", collect(t1_out_f(db)));
  m1_1(db, log, functors, nodes({5}));
  print("t1", collect(t1_out_f(db)));
  step1_2(db, log, functors, edges({{5, 6}}));
  const auto f1 = collect(t1_out_f(db));
  print("t1 final", f1);

  // Family 2: seed first -- the recursive clause is live for every arrival.
  seed2_1(db, log, functors, nodes({7}));
  print("t2", collect(t2_out_f(db)));
  m2_1(db, log, functors, nodes({1}));
  print("t2", collect(t2_out_f(db)));
  step2_2(db, log, functors, edges({{1, 2}, {2, 3}}));
  print("t2", collect(t2_out_f(db)));
  step2_2(db, log, functors, edges({{3, 4}, {4, 5}, {6, 3}, {3, 1}, {7, 8}}));
  print("t2", collect(t2_out_f(db)));
  m2_1(db, log, functors, nodes({5}));
  print("t2", collect(t2_out_f(db)));
  step2_2(db, log, functors, edges({{5, 6}}));
  const auto f2 = collect(t2_out_f(db));
  print("t2 final", f2);

  // Family 3: seed last -- all data arrives with the gate closed (only the
  // base rows 1 and 5 are derivable), then the condition flip alone must
  // drive the full multi-hop closure through the recursive cycle.
  m3_1(db, log, functors, nodes({1}));
  print("t3", collect(t3_out_f(db)));
  step3_2(db, log, functors, edges({{1, 2}, {2, 3}}));
  print("t3", collect(t3_out_f(db)));
  step3_2(db, log, functors, edges({{3, 4}, {4, 5}, {6, 3}, {3, 1}, {7, 8}}));
  print("t3", collect(t3_out_f(db)));
  m3_1(db, log, functors, nodes({5}));
  print("t3", collect(t3_out_f(db)));
  step3_2(db, log, functors, edges({{5, 6}}));
  print("t3", collect(t3_out_f(db)));
  seed3_1(db, log, functors, nodes({7}));
  const auto f3 = collect(t3_out_f(db));
  print("t3 final", f3);

  std::cout << "orders agree: " << (f1 == f2 && f2 == f3 ? "yes" : "no")
            << '\n';
  return 0;
}
