#include <cstdint>
#include <iostream>

#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator, log, functors);

  auto dump = [&db]() {
    std::cout << "logged_in:";
    for (int32_t id = 1; id <= 6; ++id) {
      if (db.user_is_logged_in_b(id)) {
        std::cout << ' ' << id;
      }
    }
    std::cout << '\n';
  };

  dump();  // Nothing yet.

  // Round 1: add users, then log one in.
  {
    hyde::rt::Vec<add_user_input> users(allocator);
    users.Add({1});
    users.Add({2});
    users.Add({3});
    db.add_user_1(std::move(users));
  }
  dump();
  {
    hyde::rt::Vec<Tup_i32> added(allocator);
    hyde::rt::Vec<Tup_i32> removed(allocator);
    added.Add({2});
    db.log_in_1(std::move(added), std::move(removed));
  }
  dump();  // is_logged_in holds; every user reports logged in.

  // Round 2: extend the data, then differentially retract the login.
  {
    hyde::rt::Vec<add_user_input> users(allocator);
    users.Add({4});
    db.add_user_1(std::move(users));
  }
  {
    hyde::rt::Vec<Tup_i32> added(allocator);
    hyde::rt::Vec<Tup_i32> removed(allocator);
    added.Add({5});  // id 5 is not a user
    db.log_in_1(std::move(added), std::move(removed));
  }
  dump();
  {
    hyde::rt::Vec<Tup_i32> added(allocator);
    hyde::rt::Vec<Tup_i32> removed(allocator);
    removed.Add({2});  // retract the only real login
    db.log_in_1(std::move(added), std::move(removed));
  }
  dump();  // is_logged_in should no longer hold.
  return 0;
}
