#include <algorithm>
#include <iostream>
#include <vector>

#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator, log, functors);

  auto dump = [&]<typename D>(D &d, const char *round) {
    std::cout << "-- " << round << '\n';

    std::cout << "result:";
    if constexpr (requires { d.result_f(); }) {
      std::vector<int> v;
      auto c = d.result_f();
      for (int x = 0; c.next(x);) {
        v.push_back(x);
      }
      std::sort(v.begin(), v.end());
      for (auto x : v) {
        std::cout << ' ' << x;
      }
    }
    std::cout << '\n';

    std::vector<int> v;
    auto c = d.all_likes_f();
    for (int x = 0; c.next(x);) {
      v.push_back(x);
    }
    std::sort(v.begin(), v.end());
    std::cout << "all_likes:";
    for (auto x : v) {
      std::cout << ' ' << x;
    }
    std::cout << '\n';
  };

  auto send = [&](std::vector<int> rows) {
    hyde::rt::Vec<Tup_int> v(allocator);
    for (auto x : rows) {
      v.Add({x});
    }
    db.like_1(std::move(v));
  };

  dump(db, "round0");

  send({3, 4});  // neither RED nor BLUE
  dump(db, "round1");

  send({2, 1});  // BLUE and RED via the message path
  dump(db, "round2");
  return 0;
}
