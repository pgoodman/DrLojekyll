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

  // Scan both queries; the clause testing `c` twice and the clause testing
  // it once must always hold identical rows.
  auto dump = [&db]() {
    std::vector<int32_t> qd;
    {
      auto c = q_double_f(db);
      for (int32_t x = 0; c.next(x);) {
        qd.push_back(x);
      }
    }
    std::sort(qd.begin(), qd.end());

    std::vector<int32_t> ps;
    {
      auto c = p_single_f(db);
      for (int32_t x = 0; c.next(x);) {
        ps.push_back(x);
      }
    }
    std::sort(ps.begin(), ps.end());

    std::cout << "q_double:";
    for (auto x : qd) {
      std::cout << ' ' << x;
    }
    std::cout << "\np_single:";
    for (auto x : ps) {
      std::cout << ' ' << x;
    }
    std::cout << "\nagree: " << (qd == ps ? "yes" : "NO") << '\n';
  };

  auto feed_r = [&](std::vector<int32_t> xs) {
    hyde::rt::Vec<Tup_i32> v(allocator);
    for (auto x : xs) {
      v.Add({x});
    }
    feed_r_1(db, log, functors, std::move(v));
  };

  auto set_c = [&](int32_t v) {
    hyde::rt::Vec<Tup_i32> vec(allocator);
    vec.Add({v});
    set_c_1(db, log, functors, std::move(vec));
  };

  dump();  // Nothing anywhere.

  // Never-set-c prefix: rows in r, but both queries stay empty.
  feed_r({1, 2, 3});
  dump();

  // Flip the condition on: existing r rows flow through both clauses.
  set_c(7);
  dump();

  // Insert while the condition already holds.
  feed_r({4, 5});
  dump();

  // Redundant extra support for c: nothing may change.
  set_c(8);
  dump();

  feed_r({6});
  dump();
  return 0;
}
