#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "datalog.h"

// Co-recursion carrier (session-21 P6.1). The mutually recursive `ping`/`pong`
// together compute the transitive closure of `add_edge`; `reachable_pair`
// publishes `ping` (== TC). add_edge is monotone (one vec), reachable_pair is a
// monotone publish (observed via the log hook). The behavioral golden is
// secondary here; the DISCRIMINATING P6.1 gate is the `.region` golden's
// `recursive-component  C0  members=(ping, pong)` block (one component spanning
// the ping<->pong cycle). Answers are recursion-detection-invariant.
struct PrintLog {
  std::vector<std::string> rows;

  void reachable_pair_2(uint64_t From, uint64_t To, bool added) {
    rows.push_back(std::string(added ? "+(" : "-(") + std::to_string(From) +
                   "," + std::to_string(To) + ")");
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
    hyde::rt::Vec<add_edge_input> v(allocator);
    for (auto [f, t] : es) {
      v.Add({f, t});
    }
    add_edge_2(db, log, functors, std::move(v));
    log.flush(label);
  };

  log.flush("init");

  // Batch 1: a 1->2->3 chain. TC(ping) = {(1,2),(2,3),(1,3)}.
  edges({{1, 2}, {2, 3}}, "batch 1");

  // Batch 2: extend to 3->4. New pairs reaching 4: (1,4),(2,4),(3,4).
  edges({{3, 4}}, "batch 2");
  return 0;
}
