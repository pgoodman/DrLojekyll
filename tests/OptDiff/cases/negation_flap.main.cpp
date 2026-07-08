#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator, log, functors);

  auto dump = [&db](const char *label) {
    std::vector<int32_t> vals;
    auto c = db.visible_f();
    for (int32_t v = 0; c.next(v);) {
      vals.push_back(v);
    }
    std::sort(vals.begin(), vals.end());
    std::cout << label << ": visible:";
    for (auto v : vals) {
      std::cout << ' ' << v;
    }
    std::cout << '\n';
  };

  auto send_blocker = [&](std::vector<int32_t> add, std::vector<int32_t> rem) {
    hyde::rt::Vec<Tup_i32> added(allocator);
    hyde::rt::Vec<Tup_i32> removed(allocator);
    for (auto v : add) {
      added.Add({v});
    }
    for (auto v : rem) {
      removed.Add({v});
    }
    db.blocker_1(std::move(added), std::move(removed));
  };

  {
    hyde::rt::Vec<Tup_i32> rows(allocator);
    rows.Add({1});
    rows.Add({2});
    rows.Add({3});
    db.item_1(std::move(rows));
  }
  send_blocker({2}, {});
  dump("initial (items 1,2,3; blocker 2)");

  // Flap A: one batch adds AND removes blocker(3), never previously
  // present. Pinned baseline: net no-op, 3 stays visible.
  send_blocker({3}, {3});
  dump("after add+remove blocker(3)");

  // Flap B: one batch adds AND removes blocker(2), which IS present.
  // Pinned baseline: the removal wins the dequeue race and blocker(2)
  // ends absent, so 2 becomes visible.
  send_blocker({2}, {2});
  dump("after add+remove blocker(2)");
  return 0;
}
