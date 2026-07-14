#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  auto dump1 = [](const char *name, auto cursor) {
    std::vector<int32_t> vals;
    for (int32_t v = 0; cursor.next(v);) {
      vals.push_back(v);
    }
    std::sort(vals.begin(), vals.end());
    std::cout << name << ':';
    for (auto v : vals) {
      std::cout << ' ' << v;
    }
    std::cout << '\n';
  };
  auto dump = [&]() {
    dump1("chain", q_chain_f(db));
    dump1("gated", q_gated_f(db));
  };

  dump();
  {
    hyde::rt::Vec<in_input> rows(allocator);
    rows.Add({3});
    rows.Add({0});
    rows.Add({-2});
    in_1(db, log, functors, std::move(rows));
  }
  dump();  // chain: -2 3; gated still empty (condition not enabled)
  {
    hyde::rt::Vec<log_in_input> rows(allocator);
    rows.Add({100});
    log_in_1(db, log, functors, std::move(rows));
  }
  dump();  // gated now: -2 3
  {
    hyde::rt::Vec<in_input> rows(allocator);
    rows.Add({5});
    rows.Add({0});  // duplicate, filtered
    in_1(db, log, functors, std::move(rows));
  }
  dump();  // both gain 5
  return 0;
}
