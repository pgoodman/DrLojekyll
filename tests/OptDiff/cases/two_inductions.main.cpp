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

  // output is a bound query; probe a fixed range of values.
  auto dump = [&db]() {
    std::cout << "output:";
    for (int32_t a = 0; a <= 10; ++a) {
      if (output_b(db, a)) {
        std::cout << ' ' << a;
      }
    }
    std::cout << '\n';
  };

  // Round 1: seed input and one step in each induction.
  {
    hyde::rt::Vec<input_input> v(allocator);
    v.Add({1});
    input_1(db, log, functors, std::move(v));
  }
  {
    hyde::rt::Vec<blah1_input> v(allocator);
    v.Add({1, 2});      // loop1: 1 -> 2
    blah1_2(db, log, functors, std::move(v));
  }
  {
    hyde::rt::Vec<blah2_input> v(allocator);
    v.Add({2, 3});      // loop2: 2 -> 3
    v.Add({7, 8});      // unreachable edge
    blah2_2(db, log, functors, std::move(v));
  }
  std::cout << "round 1\n";
  dump();  // loop1 = {1,2}; loop2 = {1,2,3}

  // Round 2: extend both inductions, including a cycle in loop2.
  {
    hyde::rt::Vec<blah1_input> v(allocator);
    v.Add({2, 4});      // loop1: 2 -> 4
    blah1_2(db, log, functors, std::move(v));
  }
  {
    hyde::rt::Vec<blah2_input> v(allocator);
    v.Add({4, 5});      // loop2: 4 -> 5
    v.Add({5, 1});      // cycle back
    v.Add({3, 7});      // now 7 (and 8) become reachable
    blah2_2(db, log, functors, std::move(v));
  }
  {
    hyde::rt::Vec<input_input> v(allocator);
    v.Add({10});        // fresh seed
    input_1(db, log, functors, std::move(v));
  }
  std::cout << "round 2\n";
  dump();  // loop2 = {1,2,3,4,5,7,8,10}
  return 0;
}
