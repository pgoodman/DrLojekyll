#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator, log, functors);

  auto dump = [&db]() {
    std::cout << "logged_in:";
    for (int32_t id = 1; id <= 4; ++id) {
      if (db.user_is_logged_in_b(id)) {
        std::cout << ' ' << id;
      }
    }
    std::cout << '\n';
  };

  auto add_users = [&](std::vector<int32_t> ids) {
    hyde::rt::Vec<Tup_i32> vec(allocator);
    for (auto id : ids) {
      vec.Add({id});
    }
    db.add_user_1(std::move(vec));
  };

  auto log_in = [&](std::vector<int32_t> add, std::vector<int32_t> rem) {
    hyde::rt::Vec<Tup_i32> av(allocator);
    hyde::rt::Vec<Tup_i32> rv(allocator);
    for (auto id : add) {
      av.Add({id});
    }
    for (auto id : rem) {
      rv.Add({id});
    }
    db.log_in_1(std::move(av), std::move(rv));
  };

  dump();
  add_users({1, 2, 3});
  dump();  // no logins yet: condition false
  log_in({2}, {});
  dump();  // condition true: all users report logged in state via condition
  log_in({}, {2});
  dump();  // condition false again
  log_in({4}, {});  // login by a non-user does not set the condition
  dump();
  add_users({4});  // now user 4 exists and its login sets the condition
  dump();
  return 0;
}
