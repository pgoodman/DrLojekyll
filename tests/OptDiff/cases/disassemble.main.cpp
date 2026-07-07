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

  // Dump function_instructions for every address we ever feed in.
  auto dump = [&db]() {
    for (uint64_t ea = 100; ea <= 132; ea += 4) {
      std::vector<uint64_t> insts;
      auto c = db.function_instructions_bf(ea);
      for (uint64_t i = 0; c.next(i);) {
        insts.push_back(i);
      }
      std::sort(insts.begin(), insts.end());
      std::cout << "func " << ea << ':';
      for (auto i : insts) {
        std::cout << ' ' << i;
      }
      std::cout << '\n';
    }
  };

  // Round 1: one function at 100 with a straight-line body 100->104->108,
  // which calls a function at 120 whose body is 120->124.
  {
    hyde::rt::Vec<raw_transfer_input> xfers(allocator);
    xfers.Add({100, 104, 0});  // FALL_THROUGH
    xfers.Add({104, 108, 0});
    xfers.Add({104, 120, 1});  // CALL
    xfers.Add({120, 124, 0});
    db.raw_transfer_3(std::move(xfers));

    hyde::rt::Vec<instruction_input> insts(allocator);
    insts.Add({100});
    insts.Add({104});
    insts.Add({108});
    insts.Add({120});
    insts.Add({124});
    db.instruction_1(std::move(insts));
  }
  std::cout << "round 1\n";
  dump();

  // Round 2: extend function 120's body, and add a new instruction at 128
  // that is called from 124, plus a fall-through from 128 to a fresh 132.
  {
    hyde::rt::Vec<raw_transfer_input> xfers(allocator);
    xfers.Add({124, 128, 1});  // CALL -> 128 becomes a function
    xfers.Add({128, 132, 0});  // FALL_THROUGH inside function 128
    db.raw_transfer_3(std::move(xfers));

    hyde::rt::Vec<instruction_input> insts(allocator);
    insts.Add({128});
    insts.Add({132});
    db.instruction_1(std::move(insts));
  }
  std::cout << "round 2\n";
  dump();
  return 0;
}
