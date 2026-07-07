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

  dump();  // No users at all.

  // Round 1: add users, then log one in.
  {
    hyde::rt::Vec<add_user_input> users(allocator);
    users.Add({1});
    users.Add({2});
    users.Add({3});
    db.add_user_1(std::move(users));
  }
  dump();  // Users exist but no one has logged in.
  {
    hyde::rt::Vec<log_in_input> logins(allocator);
    logins.Add({2});
    db.log_in_1(std::move(logins));
  }
  dump();  // is_logged_in holds, so every user is "logged in".

  // Round 2: another user, plus a login by a non-user id.
  {
    hyde::rt::Vec<add_user_input> users(allocator);
    users.Add({4});
    db.add_user_1(std::move(users));
  }
  {
    hyde::rt::Vec<log_in_input> logins(allocator);
    logins.Add({5});  // id 5 is not a user
    db.log_in_1(std::move(logins));
  }
  dump();
  return 0;
}
