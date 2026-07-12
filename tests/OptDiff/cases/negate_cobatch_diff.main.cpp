#include <algorithm>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "datalog.h"

// Differential co-batch negate standing fixture (checkpoint (d), analysis
// sec 4.5 R9). ONE @differential message sig(Tag, A, B) feeds BOTH the pred
// relation (Tag 0 -> pr) and the negated relation (Tag 1 -> blk), so a
// single runtime epoch (one db.sig_3 call with both adds and removes) can
// carry every combination of pred-change x negated-change. Unlike the
// monotone two-message shape, this co-batch IS expressible in the runtime,
// so all four F-A mixed cases fire in one genuine epoch:
//   case 1 (key 10): +pr, +blk -> out net 0 (phantom pair, absent)
//   case 2 (key 20): +pr, -blk -> out(20,200) born (+1)
//   case 3 (key 30): -pr, +blk -> out(30,300) dies (-1)
//   case 4 (key 40): -pr, -blk -> out net 0 (churn, absent)
//
// Oracle-verified truth:
//   after batch 1 (setup): out = {(30,300)}
//   after batch 2 (co-batch): out = {(20,200)}

using sig_row = std::tuple<uint64_t, uint64_t, uint64_t>;

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator, log, functors);

  auto dump = [&db](const char *label) {
    std::vector<std::pair<uint64_t, uint64_t>> rows;
    auto c = db.out_ff();
    for (uint64_t a = 0, b = 0; c.next(a, b);) {
      rows.emplace_back(a, b);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << label << ": out:";
    for (auto &[a, b] : rows) {
      std::cout << " (" << a << ',' << b << ')';
    }
    std::cout << '\n';
  };

  auto sig = [&](std::vector<sig_row> adds, std::vector<sig_row> rems) {
    hyde::rt::Vec<Tup_u64_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64_u64> rem(allocator);
    for (auto [t, a, b] : adds) add.Add({t, a, b});
    for (auto [t, a, b] : rems) rem.Add({t, a, b});
    db.sig_3(std::move(add), std::move(rem));
  };

  dump("initial");

  // Batch 1 (setup): pre-states each case flips FROM.
  //   blk(20) present; pr(30,300) live (out(30,300)); pr(40,400)+blk(40).
  sig({{1, 20, 0}, {0, 30, 300}, {0, 40, 400}, {1, 40, 0}}, {});
  dump("after setup");

  // Batch 2 (co-batch): all four F-A cases in ONE epoch.
  sig({{0, 10, 100}, {1, 10, 0}, {0, 20, 200}, {1, 30, 0}},
      {{1, 20, 0}, {0, 30, 300}, {0, 40, 400}, {1, 40, 0}});
  dump("after co-batch (cases 1-4)");
  return 0;
}
