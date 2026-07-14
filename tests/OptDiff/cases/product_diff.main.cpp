#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "datalog.h"

// Stage-5 product_diff driver: both product sides fed by @differential
// messages. Observation surface: the published delta stream of `pair`
// (sorted per epoch, see product_conds). The runtime has one entry point
// per message, so each oracle batch splits into an a_in epoch then a b_in
// epoch (the oracle referees the combined batch; the per-epoch splits
// settle to the same end state).
struct PrintLog {
  std::vector<std::string> rows;

  void pair_2(uint32_t X, uint32_t Y, bool added) {
    rows.push_back(std::string(added ? "+(" : "-(") + std::to_string(X) +
                   "," + std::to_string(Y) + ")");
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

  auto vec = [&](std::vector<uint32_t> xs) {
    hyde::rt::Vec<Tup_u32> v(allocator);
    for (auto x : xs) {
      v.Add({x});
    }
    return v;
  };
  auto a = [&](std::vector<uint32_t> add, std::vector<uint32_t> rem,
               const char *label) {
    a_in_1(db, log, functors, vec(add), vec(rem));
    log.flush(label);
  };
  auto b = [&](std::vector<uint32_t> add, std::vector<uint32_t> rem,
               const char *label) {
    b_in_1(db, log, functors, vec(add), vec(rem));
    log.flush(label);
  };

  log.flush("init");

  // b1: first row on each side.
  a({1}, {}, "b1a +a1");
  b({10}, {}, "b1b +b10");

  // b2: second a row (fires against the existing b side).
  a({2}, {}, "b2 +a2");

  // b3: retract-one-side (every pair with b10 dies).
  b({}, {10}, "b3 -b10");

  // b4: same-SIDE mixed epoch: +a3/-a2 in one batch (both signed arms of
  // side 1 fire; b side is empty here so no pairs move -- pins the empty-
  // other-side degenerate).
  a({3}, {2}, "b4 +a3 -a2");

  // b5: repopulate b (pairs reappear for the surviving a rows).
  a({4}, {}, "b5a +a4");
  b({20}, {}, "b5b +b20");

  // b6: ingest annihilation (+a5/-a5 nets to nothing at NetBatch; no
  // publish, no frontier).
  a({5}, {5}, "b6 +a5 -a5");

  // b7: full drain to empty.
  a({}, {1, 3, 4}, "b7a -a1 -a3 -a4");
  b({}, {20}, "b7b -b20");
  return 0;
}
