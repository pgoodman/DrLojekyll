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

  // entry_1 is a bound/bound query, so probe a grid of pairs.
  auto dump = [&db, &allocator]() {
    std::cout << "entry_1:";
    for (int32_t a = -4; a <= 6; ++a) {
      for (int32_t b = -4; b <= 6; ++b) {
        if (entry_1_bb(db, a, b)) {
          std::cout << " (" << a << ',' << b << ')';
        }
      }
    }
    std::cout << '\n';
    (void) allocator;
  };

  // Round 1: base entries, a self-entry, and a differential remove-add.
  {
    hyde::rt::Vec<add_entry_input> v(allocator);
    v.Add({1, 2, 3});   // entry_3(1,2,3) -> entry_2(1,2) -> entry_1(1,2)
    v.Add({2, 5, 1});   // 5 < 1 fails: no entry_2
    v.Add({3, 4, 5});   // -> entry_1(3,4)
    v.Add({5, 2, 6});   // 5 < 2 fails at entry_1
    add_entry_3(db, log, functors, std::move(v));
  }
  {
    hyde::rt::Vec<add_entry2_input> v(allocator);
    v.Add({4});         // entry_1(4,4)
    add_entry2_1(db, log, functors, std::move(v));
  }
  {
    hyde::rt::Vec<Tup_i32> added(allocator);
    hyde::rt::Vec<Tup_i32> removed(allocator);
    added.Add({0});     // entry_3(0,1,2) -> entry_2(0,1) -> entry_1(0,1)
    added.Add({-3});    // -> entry_1(-3,1)
    remove_1(db, log, functors, std::move(added), std::move(removed));
  }
  std::cout << "round 1\n";
  dump();

  // Round 2: retract remove(0), extend with new entries.
  {
    hyde::rt::Vec<Tup_i32> added(allocator);
    hyde::rt::Vec<Tup_i32> removed(allocator);
    removed.Add({0});   // retract entry_1(0,1)
    remove_1(db, log, functors, std::move(added), std::move(removed));
  }
  {
    hyde::rt::Vec<add_entry_input> v(allocator);
    v.Add({-2, -1, 0}); // -> entry_1(-2,-1)
    v.Add({1, 2, 3});   // duplicate of round 1
    add_entry_3(db, log, functors, std::move(v));
  }
  std::cout << "round 2\n";
  dump();
  return 0;
}
