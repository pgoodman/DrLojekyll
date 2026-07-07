#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator, log, functors);

  auto dump = [&db]() {
    std::vector<int32_t> vals;
    auto c = db.out_f();
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
    db.add_user_1(std::move(rows));
  }
  dump();  // users exist, but no log_in: condition false

  {
    hyde::rt::Vec<log_in_input> rows(allocator);
    rows.Add({10});
    db.log_in_1(std::move(rows));
  }
  dump();  // condition true: all users flow through

  {
    hyde::rt::Vec<add_user_input> rows(allocator);
    rows.Add({30});
    db.add_user_1(std::move(rows));
  }
  dump();  // late user also gated in
  return 0;
}
