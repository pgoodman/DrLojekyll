#include <cstdint>
#include <iostream>

#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  auto dump = [&db]() {
    std::cout << "logged_in:";
    for (int32_t id = 1; id <= 6; ++id) {
      if (user_is_logged_in_b(db, id)) {
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
    add_user_1(db, log, functors, std::move(users));
  }
  dump();  // Users exist but no one has logged in.
  {
    hyde::rt::Vec<log_in_input> logins(allocator);
    logins.Add({2});
    log_in_1(db, log, functors, std::move(logins));
  }
  dump();  // is_logged_in holds, so every user is "logged in".

  // Round 2: another user, plus a login by a non-user id.
  {
    hyde::rt::Vec<add_user_input> users(allocator);
    users.Add({4});
    add_user_1(db, log, functors, std::move(users));
  }
  {
    hyde::rt::Vec<log_in_input> logins(allocator);
    logins.Add({5});  // id 5 is not a user
    log_in_1(db, log, functors, std::move(logins));
  }
  dump();
  return 0;
}
