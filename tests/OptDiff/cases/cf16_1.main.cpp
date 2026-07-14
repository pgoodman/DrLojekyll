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
    std::vector<int32_t> v;
    auto c = all_users_f(db);
    for (int32_t id = 0; c.next(id);) {
      v.push_back(id);
    }
    std::sort(v.begin(), v.end());
    std::cout << "all_users:";
    for (auto id : v) {
      std::cout << ' ' << id;
    }
    std::cout << '\n';
  };

  auto send = [&](std::vector<int32_t> ids) {
    hyde::rt::Vec<Tup_i32> vec(allocator);
    for (auto id : ids) {
      vec.Add({id});
    }
    std::cout << "add rc=" << add_user_1(db, log, functors, std::move(vec)) << '\n';
  };

  dump();
  send({10, 20, 30});
  dump();
  send({20, 40});  // one duplicate, one new
  dump();
  send({});
  dump();
  return 0;
}
