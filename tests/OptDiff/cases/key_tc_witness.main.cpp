// Driver for demand_tc_witness (compiled with `-demand` via the .drflags
// sidecar). The demanded query's entry point takes (db, log, functors,
// bound...) — the force.dr ABI shape: the call first INJECTS the bound key
// as demand (seeding the fabricated demand message through the synthesized
// injector, which runs the flow), then reads the answer.
//
// Fixed probe keys (never an all-free node enumeration — an all-free query
// over path would defeat the demand pruning, the tc.dr inertness shape).
// Cursor contract: drain fully before the next entry-point call; sort keyed
// drains before printing.

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  auto probe = [&](uint64_t f) {
    std::vector<uint64_t> tos;
    auto c = reachable_from_bf(db, log, functors, f);
    for (uint64_t t = 0; c.next(t);) {
      tos.push_back(t);
    }
    std::sort(tos.begin(), tos.end());
    std::cout << "from " << f << ':';
    for (auto t : tos) {
      std::cout << ' ' << t;
    }
    std::cout << '\n';
  };

  // Batch 1: a chain 1->2->3->4 and a detached edge 7->8.
  {
    hyde::rt::Vec<edge_2_input> edges(allocator);
    edges.Add({1, 2});
    edges.Add({2, 3});
    edges.Add({3, 4});
    edges.Add({7, 8});
    edge_2_2(db, log, functors, std::move(edges));
  }
  std::cout << "batch 1\n";
  probe(1);  // Demands key 1: the chain suffix.
  probe(3);  // A mid-chain key.
  probe(7);  // The detached component.
  probe(5);  // Never a source: empty.

  // Batch 2: close the cycle 4->1 and bridge the components 4->7. Old
  // demanded keys must see the UPDATED closure (their demand rows persist;
  // the new edges re-fire the guarded fixpoint); a first-time key demanded
  // only now (2) must work too.
  {
    hyde::rt::Vec<edge_2_input> edges(allocator);
    edges.Add({4, 1});
    edges.Add({4, 7});
    edge_2_2(db, log, functors, std::move(edges));
  }
  std::cout << "batch 2\n";
  probe(1);
  probe(2);  // First demanded AFTER the edges arrived.
  probe(3);
  probe(5);
  probe(7);
  return 0;
}
