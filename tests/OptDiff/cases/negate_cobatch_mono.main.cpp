#include <algorithm>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "datalog.h"

// Monotone co-batch negate standing fixture (checkpoint (d), analysis
// sec 4.5 R9, F-A case 1). negate_1 shape, MONOTONE negated view (seen):
//   copy(A, B) : feed(A, B).
//   seen(A)    : unsee(A).
//   out(A, B)  : copy(A, B), !seen(A).
//
// NOTE ON EPOCHS: the compiled runtime exposes one entry point per message,
// so each db.feed_2 / db.unsee_1 call is its OWN epoch — there is no single-
// epoch two-message API. The oracle's `batch 1` (feed(1,10)+unsee(1), one
// epoch) nets that instance to zero via F-A convention A. The runtime
// instead publishes out(1,10) at the feed epoch, then the `-` crossover
// retracts it at the unsee epoch (seen(1) gains a key with a live pred row).
// Both settle to the SAME observable end state after the pair (out empty),
// which is what the oracle final-state golden pins. We dump AFTER EACH
// message to record the true per-epoch behavior and make the standalone
// `-` crossover retract deterministic and visible.
//
// Per-epoch runtime truth (oracle final after the pair = {}):
//   after feed(1,10):  out = {(1,10)}   forward publish, seen(1) absent
//   after unsee(1):    out = {}         `-` crossover retract
//   after feed(2,20):  out = {(2,20)}   forward publish, seen(2) absent
//   after unsee(2):    out = {}         `-` crossover retract

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator, log, functors);

  auto dump = [&db](const char *label) {
    std::vector<std::pair<int32_t, int32_t>> rows;
    auto c = db.out_ff();
    for (int32_t a = 0, b = 0; c.next(a, b);) {
      rows.emplace_back(a, b);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << label << ": out:";
    for (auto &[a, b] : rows) {
      std::cout << " (" << a << ',' << b << ')';
    }
    std::cout << '\n';
  };

  auto feed = [&](std::vector<std::pair<int32_t, int32_t>> rows) {
    hyde::rt::Vec<feed_input> v(allocator);
    for (auto [a, b] : rows) v.Add({a, b});
    db.feed_2(std::move(v));
  };
  auto unsee = [&](std::vector<int32_t> rows) {
    hyde::rt::Vec<unsee_input> v(allocator);
    for (auto a : rows) v.Add({a});
    db.unsee_1(std::move(v));
  };

  dump("initial");

  // Co-batch pair (two epochs in the runtime; one in the oracle): the pred
  // row and the negated key. Settles to out empty.
  feed({{1, 10}});
  dump("after feed(1,10)");
  unsee({1});
  dump("after unsee(1)");

  // Ordinary forward publish, no matching unsee.
  feed({{2, 20}});
  dump("after feed(2,20)");

  // Standalone `-` crossover retract.
  unsee({2});
  dump("after unsee(2)");
  return 0;
}
