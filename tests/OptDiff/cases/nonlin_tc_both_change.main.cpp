#include <algorithm>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "datalog.h"

// FLAG-F / FLAG-H standing fixture: nonlinear transitive closure
//   p(From,To) : edge(From,To).
//   p(From,To) : p(From,Y), p(Y,To).
// whose recursive rule has TWO same-stratum body positions, so both of them
// change in the same batch (both-deleted batch 2, both-added batch 3,
// one-added/one-deleted batch 4). This is the discriminator for FLAG-F
// (InNewWithFrontier must be the plain InNew alias, reading R-i) and FLAG-H
// (k emissions per delta position with the static SurvivesSoFar/AliveAtClaim
// resp. InNewWithFrontier/InNewSansFrontier same-round-exactly-once split).
// See docs/proposals/StackSafeNegation.checkpoint-c-notes.md open-flags.
//
// Ground truth (positive-only equivalence oracle) final state:
//   reachable = {(4,5),(4,6),(4,7),(5,6),(5,7),(6,7)}
// (goldens/nonlin_tc_both_change.oracle.stdout / .monotone.stdout).
//
// GOLDEN NOTE: this case is red until checkpoint (c) wires OVERDELETE for
// recursive strata; the compiled-driver golden (goldens/*.stdout) is
// intentionally ABSENT and must be authored at (c) slice-2 bring-up. The
// oracle/monotone goldens are the authoritative end-state truth in the
// meantime.

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator, log, functors);

  auto dump = [&db](const char *label) {
    std::vector<std::pair<uint64_t, uint64_t>> rows;
    auto c = db.reachable_ff();
    for (uint64_t f = 0, t = 0; c.next(f, t);) {
      rows.emplace_back(f, t);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << label << ":\n";
    for (auto [f, t] : rows) {
      std::cout << "reachable(" << f << ", " << t << ")\n";
    }
  };

  auto edge_batch = [&](std::vector<std::pair<uint64_t, uint64_t>> adds,
                        std::vector<std::pair<uint64_t, uint64_t>> rems) {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    for (auto [f, t] : adds) add.Add({f, t});
    for (auto [f, t] : rems) rem.Add({f, t});
    db.add_edge_2(std::move(add), std::move(rem));
  };

  // Batch 1: seed a 4-node chain 1->2->3->4. p(1,4) is double-derived.
  edge_batch({{1, 2}, {2, 3}, {3, 4}}, {});
  dump("after batch 1");

  // Batch 2 (both-deleted crossing): remove edge(2,3) AND edge(3,4).
  edge_batch({}, {{2, 3}, {3, 4}});
  dump("after batch 2");

  // Batch 3 (both-added crossing): add edge(4,5) AND edge(5,6); p(4,6) is
  // derivable only by combining the two rows claimed in the same round.
  edge_batch({{4, 5}, {5, 6}}, {});
  dump("after batch 3");

  // Batch 4 (one-added-one-deleted crossing): remove edge(1,2), add edge(6,7).
  edge_batch({{6, 7}}, {{1, 2}});
  dump("after batch 4");
  return 0;
}
