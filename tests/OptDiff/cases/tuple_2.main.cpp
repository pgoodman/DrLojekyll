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
    auto c = out_f(db);
    for (int32_t v = 0; c.next(v);) {
      vals.push_back(v);
    }
    std::sort(vals.begin(), vals.end());
    std::cout << "out:";
    for (auto v : vals) {
      std::cout << ' ' << v;
    }
    std::cout << '\n';
  };

  dump();  // no users, not logged in

  {
    hyde::rt::Vec<add_user_input> rows(allocator);
    rows.Add({10});
    rows.Add({20});
    add_user_1(db, log, functors, std::move(rows));
  }
  dump();  // users exist, but no log_in: condition false

  {
    hyde::rt::Vec<log_in_input> rows(allocator);
    rows.Add({10});
    log_in_1(db, log, functors, std::move(rows));
  }
  dump();  // condition true: all users flow through

  {
    hyde::rt::Vec<add_user_input> rows(allocator);
    rows.Add({30});
    add_user_1(db, log, functors, std::move(rows));
  }
  dump();  // late user also gated in
  return 0;
}
