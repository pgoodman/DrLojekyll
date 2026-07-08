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
  Database db(allocator, log, functors);

  auto dump = [&db](const char *label) {
    std::vector<std::pair<uint64_t, uint64_t>> rows;
    auto c = db.h_out_ff();
    for (uint64_t x = 0, z = 0; c.next(x, z);) {
      rows.emplace_back(x, z);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << label << ":\n";
    for (auto [x, z] : rows) {
      std::cout << "h(" << x << ", " << z << ")\n";
    }
  };

  // Seed state: e = {(1,3), (3,9), (3,4), (9,5), (1,7), (7,9)}.
  // h = {(1,9) [via (1,3)(3,9) AND (1,7)(7,9)], (1,4), (3,5), (7,5)}.
  {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    add.Add({1, 3});
    add.Add({3, 9});
    add.Add({3, 4});
    add.Add({9, 5});
    add.Add({1, 7});
    add.Add({7, 9});
    db.e_msg_2(std::move(add), std::move(rem));
  }
  dump("seeded");

  // One mixed batch: remove e(3,9) AND add e(2,3). The pairing of the
  // added e(2,3) with the removed e(3,9) is the phantom instance h(2,9),
  // in neither the old nor the new materialization. Expected final h:
  // {(1,4), (1,9), (2,4), (7,5)} -- h(3,5) loses its only derivation,
  // h(1,9) survives on (1,7)(7,9), h(2,4) is genuinely new, no h(2,9).
  {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    add.Add({2, 3});
    rem.Add({3, 9});
    db.e_msg_2(std::move(add), std::move(rem));
  }
  dump("after mixed batch");
  return 0;
}
