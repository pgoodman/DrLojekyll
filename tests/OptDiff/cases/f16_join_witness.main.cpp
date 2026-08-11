// Copyright 2026, Peter Goodman. All rights reserved.
//
// F16 join-multiplicity witness (session-27, the P6.3 A1 close). The
// DISCRIMINATING gate is this case's `.region` golden: its recursive component
// `members=(r)` must classify as `joint-fixpoint  C0` (NOT
// `fused-fixpoint  binding-prefix=(A, B)`), because the recursive producer of
// `r` join-binds each head ordinal from two distinct in-cycle sources. The
// stdout below is the secondary answer-invariance net — the M3 backend
// materializes `r` identically in every mode regardless of the fusion label.
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "datalog.h"

struct PrintLog {
  std::vector<std::string> rows;

  void out_pair_2(uint64_t A, uint64_t B, bool added) {
    rows.push_back(std::string(added ? "+(" : "-(") + std::to_string(A) + "," +
                   std::to_string(B) + ")");
  }

  void flush(const char *label) {
    std::sort(rows.begin(), rows.end());
    std::cout << label << ":";
    for (const auto &r : rows) {
      std::cout << ' ' << r;
    }
    std::cout << '\n';
    rows.clear();
  }
};

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  PrintLog log;
  Database db(allocator);
  init(db, log, functors);

  auto edges = [&](std::vector<std::pair<uint64_t, uint64_t>> es,
                   const char *label) {
    hyde::rt::Vec<edge_2_input> v(allocator);
    for (auto [f, t] : es) {
      v.Add({f, t});
    }
    edge_2_2(db, log, functors, std::move(v));
    log.flush(label);
  };

  log.flush("init");

  // A directed 3-cycle 1->2->3->1: every ordered pair on the cycle satisfies
  // the triangle-with-reverse-edge recursion, so `r` closes to all 9 pairs
  // over {1,2,3}. (Answers are fusion-classification-invariant; the gate is
  // the `.region` joint-fixpoint block.)
  edges({{1, 2}, {2, 3}, {3, 1}}, "batch 1");

  // Add a 4th node on the cycle: 3->4, 4->1 (replacing nothing; 3->1 stays).
  edges({{3, 4}, {4, 1}}, "batch 2");
  return 0;
}
