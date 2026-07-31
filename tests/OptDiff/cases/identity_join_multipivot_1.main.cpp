// Copyright 2026, Peter Goodman. All rights reserved.
//
// identity_join_multipivot_1 -- the NEGATIVE witness for the df.ident_join
// recognizer's single-pivot soundness fence (the opus pre-commit review's H1).
//
// The program's `result(A,B,C) : rel(A,B,C), sr(A,B)` compiles to a TWO-PIVOT
// join of `rel` (whose A ⊆ sr.col0 and B ⊆ sr.col1 hold INDEPENDENTLY, via xs/ys)
// against `sr` on the PAIR (A,B). Per-column containment does NOT imply the pair
// (A,B) is a row of sr, so the join genuinely FILTERS -- the recognizer must NOT
// treat it as an identity. With the data below the filter drops exactly the row
// (1,2,99): a recognizer that (wrongly) fired on the multi-pivot join would leak
// it (over-answer). The driver asserts the filtered answer, so a regression both
// diverges from the golden AND trips the assert.

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <tuple>
#include <vector>

#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  // sr = base = {(1,1),(2,2)}  => xs = {1,2}, ys = {1,2}.
  {
    hyde::rt::Vec<Tup_u64_u64> base(allocator);
    base.Add({1, 1});
    base.Add({2, 2});
    base_2_2(db, log, functors, std::move(base));
  }
  // pay = {(1,1,50),(2,2,60),(1,2,99)}. rel = pay filtered by A∈xs,B∈ys = all 3.
  // result = rel filtered by (A,B)∈sr = {(1,1,50),(2,2,60)} -- (1,2,99) DROPPED.
  {
    hyde::rt::Vec<Tup_u64_u64_u64> pay(allocator);
    pay.Add({1, 1, 50});
    pay.Add({2, 2, 60});
    pay.Add({1, 2, 99});
    pay_3_3(db, log, functors, std::move(pay));
  }

  std::vector<std::tuple<uint64_t, uint64_t, uint64_t>> rows;
  auto c = result_fff_fff(db);
  for (uint64_t a = 0, b = 0, d = 0; c.next(a, b, d);) {
    rows.emplace_back(a, b, d);
  }
  std::sort(rows.begin(), rows.end());

  // The (A,B) filter is load-bearing: (1,2,99) must NOT appear.
  const std::vector<std::tuple<uint64_t, uint64_t, uint64_t>> expected = {
      {1, 1, 50}, {2, 2, 60}};
  assert(rows == expected);

  std::cout << "result:";
  for (const auto &[a, b, d] : rows) {
    std::cout << " (" << a << ',' << b << ',' << d << ')';
  }
  std::cout << '\n';
  return 0;
}
