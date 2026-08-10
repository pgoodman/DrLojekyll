#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "datalog.h"

// P6.2 exit-gate driver. `p`/`q` are mutually recursive (@key(K)); the bound
// #query `lookup(bound K, free V)` reads the full-materialization backend's `q`,
// so the answer is a COMPLETE read, invariant across all four optimization modes.
// The DISCRIMINATING P6.2 gate is the `.region` golden's rule/shared-field block
// (verified against the synthesis prediction); this driver pins answer-invariance.
//
// Data: base_a={(1,10)}, base_b={(2,20)}, edge={(10,11),(20,21)}. So
//   p ⊇ a ∪ b = {(1,10),(2,20)}; q=p; p also gets q⋈edge = {(1,11),(2,21)};
//   fixpoint p=q={(1,10),(1,11),(2,20),(2,21)}. lookup(1)={10,11}; lookup(2)={20,21}.
struct PrintLog {
  std::vector<std::string> rows;

  void out_pair_2(uint64_t K, uint64_t V, bool added) {
    rows.push_back(std::string(added ? "+(" : "-(") + std::to_string(K) + "," +
                   std::to_string(V) + ")");
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
  log.flush("init");

  hyde::rt::Vec<edge_input> ev(allocator);
  ev.Add({10, 11});
  ev.Add({20, 21});
  edge_2(db, log, functors, std::move(ev));

  hyde::rt::Vec<base_a_input> av(allocator);
  av.Add({1, 10});
  base_a_2(db, log, functors, std::move(av));

  hyde::rt::Vec<base_b_input> bv(allocator);
  bv.Add({2, 20});
  base_b_2(db, log, functors, std::move(bv));
  log.flush("after edges");

  // Probe the bound query for K=1 and K=2; sort the (unspecified-order) drain.
  for (uint64_t k : {uint64_t(1), uint64_t(2)}) {
    std::vector<uint64_t> vs;
    auto c = lookup_bf(db, k);
    uint64_t v;
    while (c.next(v)) {
      vs.push_back(v);
    }
    std::sort(vs.begin(), vs.end());
    std::cout << "lookup(" << k << "):";
    for (uint64_t x : vs) {
      std::cout << ' ' << x;
    }
    std::cout << '\n';
  }
  return 0;
}
