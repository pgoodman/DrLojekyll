#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "datalog.h"

// Stage-5 product_self driver: differential SELF-product (both sides of
// `pairs` are distinct views over ONE table). Pins: emitter side
// non-distinctness (both arm positions loop the same frontier vectors and
// scan the same table) and the SAME-EPOCH mixed batch +3/-2 -- the
// position-keyed reads must net exactly {-(2,2), +(3,3), and the mixed
// pairs (2,3)/(3,2) never appearing} with the (3,2)/(2,3) phantom halves
// dropped by the claim gates.
struct PrintLog {
  std::vector<std::string> rows;

  void pairs_2(uint32_t A, uint32_t B, bool added) {
    rows.push_back(std::string(added ? "+(" : "-(") + std::to_string(A) +
                   "," + std::to_string(B) + ")");
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

  auto n = [&](std::vector<uint32_t> add, std::vector<uint32_t> rem,
               const char *label) {
    hyde::rt::Vec<Tup_u32> adds(allocator);
    for (auto x : add) {
      adds.Add({x});
    }
    hyde::rt::Vec<Tup_u32> rems(allocator);
    for (auto x : rem) {
      rems.Add({x});
    }
    n_in_1(db, log, functors, std::move(adds), std::move(rems));
    log.flush(label);
  };

  log.flush("init");

  n({1}, {}, "b1 +1");        // (1,1)
  n({2}, {}, "b2 +2");        // (1,2) (2,1) (2,2)
  n({}, {1}, "b3 -1");        // every pair touching 1 dies
  n({3}, {2}, "b4 +3 -2");    // the same-epoch mixed batch over {2}
  return 0;
}
