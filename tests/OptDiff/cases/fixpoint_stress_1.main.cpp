// fixpoint_stress_1 driver -- directed fixpoint-family witness
// (DeltaRelationalIR.md sec 9 risk #2). See fixpoint_stress_1.dr for the full
// round-arithmetic trace proving the same-round double-claim of tc(1,5) via
// tc(1,3) . tc(3,5) (both first claimed in round 1), and the .batches sidecar
// for the remove-then-rederive / phantom-pair / add-side-stale-drop batches.
//
//   tc(X,Z) : tc(X,Y), tc(Y,Z).      nonlinear same-SCC recursion
//   tc(X,Y) : edge(X,Y).             base
//   edge(X,Y) : add_edge(X,Y).       add_edge @differential
//   reach_from(bound From, free To) : tc(From, To).
//   reach_to(free From, bound To)   : tc(From, To).
//
// Per-batch expected tc (hand-computed AND oracle/monotone-verified; the .dr
// header gives the round arithmetic):
//   batch 1  chain 1->2->3->4->5:   all i<j in 1..5  (10 pairs)
//   batch 2  +5->1 (full SCC):      all 25 ordered pairs incl. self-loops
//   batch 3  -3->4 (break cycle):   {12,13,23, 41,42,43,45, 51,52,53}
//   batch 4  -5->1 +2->5:           {12,13,15,23,25,45}
//
// CURSOR CONTRACT: any entry-point call invalidates open cursors and keyed-
// cursor order is unspecified, so each drain is fully collected then sorted
// before printing (CLAUDE.md "Generated API").

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  // Node ids used are 1..5; loop 0..6 to also pin the empty rows on both
  // endpoints (a spurious row at 0 or 6 would surface immediately).
  constexpr uint64_t kLo = 0;
  constexpr uint64_t kHi = 6;

  auto dump = [&db](const char *label) {
    std::cout << label << '\n';
    for (uint64_t n = kLo; n <= kHi; ++n) {
      std::vector<uint64_t> tos;
      auto c = reach_from_bf(db, n);
      for (uint64_t t = 0; c.next(t);) {
        tos.push_back(t);
      }
      std::sort(tos.begin(), tos.end());
      if (!tos.empty()) {
        std::cout << "  from " << n << ':';
        for (auto t : tos) {
          std::cout << ' ' << t;
        }
        std::cout << '\n';
      }
    }
    for (uint64_t n = kLo; n <= kHi; ++n) {
      std::vector<uint64_t> froms;
      auto c = reach_to_fb(db, n);
      for (uint64_t f = 0; c.next(f);) {
        froms.push_back(f);
      }
      std::sort(froms.begin(), froms.end());
      if (!froms.empty()) {
        std::cout << "  to " << n << ':';
        for (auto f : froms) {
          std::cout << ' ' << f;
        }
        std::cout << '\n';
      }
    }
  };

  auto edges = [&](std::vector<std::pair<uint64_t, uint64_t>> adds,
                   std::vector<std::pair<uint64_t, uint64_t>> rems) {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    for (auto [u, v] : adds) add.Add({u, v});
    for (auto [u, v] : rems) rem.Add({u, v});
    add_edge_2(db, log, functors, std::move(add), std::move(rem));
  };

  // Batch 1: seed the chain 1->2->3->4->5 (same-round double-claim of tc(1,5)).
  edges({{1, 2}, {2, 3}, {3, 4}, {4, 5}}, {});
  dump("batch 1");

  // Batch 2: close the cycle with 5->1 (full SCC; add-side stale drops).
  edges({{5, 1}}, {});
  dump("batch 2");

  // Batch 3: remove the chord 3->4 (remove-then-rederive; C_r>0 REDERIVE).
  edges({}, {{3, 4}});
  dump("batch 3");

  // Batch 4: remove 5->1 and add 2->5 in one batch (phantom pairs; mixed).
  edges({{2, 5}}, {{5, 1}});
  dump("batch 4");

  return 0;
}
