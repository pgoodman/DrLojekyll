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

  auto dump = [&db]() {
    std::vector<int32_t> vals;
    auto c = q_f(db);
    for (int32_t v = 0; c.next(v);) {
      vals.push_back(v);
    }
    std::sort(vals.begin(), vals.end());
    std::cout << "q:";
    for (auto v : vals) {
      std::cout << ' ' << v;
    }
    std::cout << '\n';
  };

  // Round 1: condition `enabled` is still false (me(2) does not match
  // the me(1) trigger), so only the unconditional mb operand shows.
  {
    hyde::rt::Vec<ma_input> rows(allocator);
    rows.Add({1});
    rows.Add({2});
    ma_1(db, log, functors, std::move(rows));
  }
  {
    hyde::rt::Vec<mb_input> rows(allocator);
    rows.Add({3});
    mb_1(db, log, functors, std::move(rows));
  }
  {
    hyde::rt::Vec<me_input> rows(allocator);
    rows.Add({2});
    me_1(db, log, functors, std::move(rows));
  }
  dump();

  // Round 2: me(1) sets the condition; previously inserted ma rows must
  // now appear.
  {
    hyde::rt::Vec<me_input> rows(allocator);
    rows.Add({1});
    me_1(db, log, functors, std::move(rows));
  }
  dump();

  // Round 3: new ma rows flow through while the condition holds.
  {
    hyde::rt::Vec<ma_input> rows(allocator);
    rows.Add({4});
    ma_1(db, log, functors, std::move(rows));
  }
  dump();
  return 0;
}
