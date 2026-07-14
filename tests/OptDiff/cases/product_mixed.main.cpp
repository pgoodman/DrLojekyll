#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "datalog.h"

// Stage-5 product_mixed driver: monotone side (a_in, add-only) x
// differential side (b_in). Pins: the monotone side's + arm runs off the
// eager walk's boundary net-additions frontier (and its table Seals at
// commit); the differential side's arms scan the monotone side (InI = the
// sealed watermark, InNew = log presence); the monotone side has NO - arm.
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
  auto a = [&](std::vector<uint32_t> add, const char *label) {
    a_in_1(db, log, functors, vec(add));
    log.flush(label);
  };
  auto b = [&](std::vector<uint32_t> add, std::vector<uint32_t> rem,
               const char *label) {
    b_in_1(db, log, functors, vec(add), vec(rem));
    log.flush(label);
  };

  log.flush("init");

  // b1: one row each side (monotone epoch first, then differential).
  a({1}, "b1a +a1");
  b({10}, {}, "b1b +b10");

  // b2: monotone growth fires against the existing differential side.
  a({2}, "b2 +a2");

  // b3: differential retraction scans the (sealed) monotone side.
  b({}, {10}, "b3 -b10");

  // b4: ingest annihilation on the differential side (+b20/-b20).
  b({20}, {20}, "b4 +b20 -b20");

  // b5: differential re-add (pairs reappear for every monotone row).
  b({30}, {}, "b5 +b30");
  return 0;
}
