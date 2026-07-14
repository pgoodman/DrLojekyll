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

  auto dump = [&db, &dump1](const char *label) {
    std::cout << "-- " << label << '\n';
    dump1("pos_neg", pos_neg_f(db));
    dump1("both_pos", both_pos_f(db));
    dump1("mixed", mixed_f(db));
  };

  auto set_c1 = [&db, &allocator, &log, &functors](bool on) {
    hyde::rt::Vec<Tup_i32> added(allocator);
    hyde::rt::Vec<Tup_i32> removed(allocator);
    (on ? added : removed).Add({100});
    set_c1_1(db, log, functors, std::move(added), std::move(removed));
  };
  auto set_c2 = [&db, &allocator, &log, &functors](bool on) {
    hyde::rt::Vec<Tup_i32> added(allocator);
    hyde::rt::Vec<Tup_i32> removed(allocator);
    (on ? added : removed).Add({200});
    set_c2_1(db, log, functors, std::move(added), std::move(removed));
  };

  dump("empty, c1=0 c2=0");

  {
    hyde::rt::Vec<add_r_input> rows(allocator);
    rows.Add({1});
    rows.Add({2});
    add_r_1(db, log, functors, std::move(rows));
  }
  {
    hyde::rt::Vec<add_s_input> rows(allocator);
    rows.Add({10});
    rows.Add({11});
    add_s_1(db, log, functors, std::move(rows));
  }
  dump("data, c1=0 c2=0");  // mixed = s only (via !c1).

  set_c1(true);
  dump("c1=1 c2=0");  // pos_neg = r; both_pos empty (AND, not OR); mixed = r.

  {
    hyde::rt::Vec<add_r_input> rows(allocator);
    rows.Add({3});
    add_r_1(db, log, functors, std::move(rows));
  }
  dump("more r, c1=1 c2=0");  // new row flows through the open gates.

  set_c2(true);
  dump("c1=1 c2=1");  // pos_neg empty (!c2 must still be tested); both_pos = r.

  set_c1(false);
  dump("c1=0 c2=1");  // everything gated on c1 retracts; mixed = s.

  set_c2(false);
  dump("c1=0 c2=0");  // back to the initial gate state.

  set_c1(true);
  {
    hyde::rt::Vec<add_s_input> rows(allocator);
    rows.Add({12});  // arrives while !c1 is false: must NOT reach mixed.
    add_s_1(db, log, functors, std::move(rows));
  }
  dump("reopen c1, c1=1 c2=0");

  set_c1(false);
  dump("final, c1=0 c2=0");  // s row 12 appears once !c1 holds again.
  return 0;
}
