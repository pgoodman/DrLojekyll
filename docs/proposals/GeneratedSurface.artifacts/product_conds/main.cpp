#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "datalog.h"

// Stage-5 product_conds driver: the observation surface is the published
// delta stream of the @differential @product message `enabled_features`
// (the commit sweep's was!=now events), captured by a log type providing
// the published-message hook (deduced statically -- no inheritance, no
// virtual) and flushed SORTED after each message epoch (publish order
// within one sweep is not part of the contract; the per-epoch delta SET
// is).
struct PrintLog {
  std::vector<std::string> rows;

  void enabled_features_2(bool FooEnabled, bool BarEnabled, bool added) {
    std::string s(added ? "+(" : "-(");
    s += FooEnabled ? "true," : "false,";
    s += BarEnabled ? "true)" : "false)";
    rows.push_back(std::move(s));
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

  {
    PrintLog log;
    Database db(allocator);
    init(db, log, functors);
    log.flush("init");

    auto enable = [&](std::vector<uint32_t> xs, const char *label) {
      hyde::rt::Vec<Tup_u32> v(allocator);
      for (auto x : xs) {
        v.Add({x});
      }
      enable_feature_1(db, log, functors, std::move(v));
      log.flush(label);
    };

    // The two single flips (position-keyed reads, both directions), then
    // the idempotent re-add (all four arms loop empty frontiers).
    enable({1}, "b1 +enable(1)");
    enable({2}, "b2 +enable(2)");
    enable({1}, "b3 +enable(1) again");
  }

  // Fresh instance: BOTH conditions flip in ONE epoch (the T1
  // discriminator). Must publish exactly {-(false,false), +(true,true)} --
  // any transient (true,false)/(false,true) is a position-keying bug.
  {
    PrintLog log;
    Database db(allocator);
    init(db, log, functors);
    log.flush("db2 init");

    hyde::rt::Vec<Tup_u32> v(allocator);
    v.Add({1});
    v.Add({2});
    enable_feature_1(db, log, functors, std::move(v));
    log.flush("db2 b1 +enable(1)+enable(2)");
  }
  return 0;
}
