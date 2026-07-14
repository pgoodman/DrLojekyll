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
    dump1("same", q_same_f(db));
    dump1("tag", q_tag_f(db));
  };

  // Round 1: no denials yet.
  {
    hyde::rt::Vec<a_input> rows(allocator);
    rows.Add({1});
    rows.Add({2});
    a_1(db, log, functors, std::move(rows));
  }
  {
    hyde::rt::Vec<b_input> rows(allocator);
    rows.Add({2});
    rows.Add({3});
    b_1(db, log, functors, std::move(rows));
  }
  dump();  // same: 1 2 3 / tag: 1 2 3

  // Round 2: deny 2 through da. `same` loses 2 entirely; `tagd` keeps 2
  // because only the a-side clause negates deny_a.
  {
    hyde::rt::Vec<Tup_i32> adds(allocator);
    hyde::rt::Vec<Tup_i32> removes(allocator);
    adds.Add({2});
    da_1(db, log, functors, std::move(adds), std::move(removes));
  }
  dump();  // same: 1 3 / tag: 1 2 3

  // Round 3: retract the da denial and deny 3 through db instead.
  {
    hyde::rt::Vec<Tup_i32> adds(allocator);
    hyde::rt::Vec<Tup_i32> removes(allocator);
    removes.Add({2});
    da_1(db, log, functors, std::move(adds), std::move(removes));
  }
  {
    hyde::rt::Vec<Tup_i32> adds(allocator);
    hyde::rt::Vec<Tup_i32> removes(allocator);
    adds.Add({3});
    db_1(db, log, functors, std::move(adds), std::move(removes));
  }
  dump();  // same: 1 2 3 / tag: 1 2
  return 0;
}
