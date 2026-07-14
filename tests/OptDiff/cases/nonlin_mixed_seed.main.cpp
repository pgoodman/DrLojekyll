#include <algorithm>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "datalog.h"

// Mixed nonlinear join driver (see nonlin_mixed_seed.dr): one JOIN with two
// same-SCC p positions and a lower differential gate side sharing pivot Y.
// The batches (mirroring nonlin_mixed_seed.batches, single-message epochs)
// exercise the dual-section seed over gate's frontier in BOTH signs plus the
// k=2 claim-round fire, including the gate-removal cascade and the
// mixed-parity rederivation after re-adding the gate.
//
// Ground truth (positive-only equivalence oracle) per batch:
//   b1: p = {(1,2),(2,3),(3,4)}
//   b2: p = b1 + {(1,3),(2,4),(1,4)}
//   b3: p = {(1,2),(2,3),(3,4),(2,4)}
//   b4: p = b2
//   b5: p = {(1,2),(2,3),(1,3)}

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  auto dump = [&db](const char *label) {
    std::vector<std::pair<uint64_t, uint64_t>> rows;
    auto c = out_ff(db);
    for (uint64_t f = 0, t = 0; c.next(f, t);) {
      rows.emplace_back(f, t);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << label << ":\n";
    for (auto [f, t] : rows) {
      std::cout << "out(" << f << ", " << t << ")\n";
    }
  };

  auto edge_batch = [&](std::vector<std::pair<uint64_t, uint64_t>> adds,
                        std::vector<std::pair<uint64_t, uint64_t>> rems) {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    for (auto [f, t] : adds) add.Add({f, t});
    for (auto [f, t] : rems) rem.Add({f, t});
    edge_msg_2(db, log, functors, std::move(add), std::move(rem));
  };

  auto gate_batch = [&](std::vector<uint64_t> adds,
                        std::vector<uint64_t> rems) {
    hyde::rt::Vec<Tup_u64> add(allocator);
    hyde::rt::Vec<Tup_u64> rem(allocator);
    for (auto n : adds) add.Add({n});
    for (auto n : rems) rem.Add({n});
    gate_msg_1(db, log, functors, std::move(add), std::move(rem));
  };

  edge_batch({{1, 2}, {2, 3}, {3, 4}}, {});
  dump("b1 edges");

  gate_batch({2, 3}, {});
  dump("b2 gates 2,3");

  gate_batch({}, {2});
  dump("b3 -gate 2");

  gate_batch({2}, {});
  dump("b4 +gate 2");

  edge_batch({}, {{3, 4}});
  dump("b5 -edge 3-4");
  return 0;
}
