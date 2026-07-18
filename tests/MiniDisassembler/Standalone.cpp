// Copyright 2026, Peter Goodman. All rights reserved.
// Copyright 2021, Trail of Bits. All rights reserved.

#include <DrTest.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#include "mini_disassembler.h"  // Auto-generated.

using namespace mini_disassembler;

namespace {

void Dump(Database &db) {
  std::cout << "Dump:\n";
  for (uint64_t func_ea = 0; func_ea < 50; func_ea++) {
    auto cursor = function_instructions_bf(db, func_ea);
    for (uint64_t inst_ea = 0; cursor.next(inst_ea);) {
      std::cout << "  FuncEA=" << func_ea << " InstEA=" << inst_ea << "\n";
    }
  }
  std::cout << "\n";
}

size_t NumFunctionInstructions(Database &db, uint64_t func_ea) {
  std::vector<uint64_t> eas;
  auto cursor = function_instructions_bf(db, func_ea);
  for (uint64_t inst_ea = 0; cursor.next(inst_ea);) {
    eas.push_back(inst_ea);
  }
  std::sort(eas.begin(), eas.end());
  const auto it = std::unique(eas.begin(), eas.end());
  eas.erase(it, eas.end());
  return eas.size();
}

}  // namespace

TEST(MiniDisassembler, DifferentialUpdatesWork) {
  const auto allocator = hyde::rt::MallocAllocator();

  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  // Start with a few instructions, with no control-flow between them.
  hyde::rt::Vec<instruction_input> instructions(allocator);
  instructions.Add({10});
  instructions.Add({11});
  instructions.Add({12});
  instructions.Add({13});
  instructions.Add({14});
  instructions.Add({15});
  instruction_1(db, log, functors, std::move(instructions));

  Dump(db);
  ASSERT_EQ(NumFunctionInstructions(db, 9), 0u);
  ASSERT_EQ(NumFunctionInstructions(db, 10), 1u);
  ASSERT_EQ(NumFunctionInstructions(db, 11), 1u);
  ASSERT_EQ(NumFunctionInstructions(db, 12), 1u);
  ASSERT_EQ(NumFunctionInstructions(db, 13), 1u);
  ASSERT_EQ(NumFunctionInstructions(db, 14), 1u);
  ASSERT_EQ(NumFunctionInstructions(db, 15), 1u);

  // Now we add the fall-through edges, and 10 is the only instruction with
  // no predecessor, so its the function head.
  hyde::rt::Vec<raw_transfer_input> transfers(allocator);
  transfers.Add({10, 11, EdgeType::FALL_THROUGH});
  transfers.Add({11, 12, EdgeType::FALL_THROUGH});
  transfers.Add({12, 13, EdgeType::FALL_THROUGH});
  transfers.Add({13, 14, EdgeType::FALL_THROUGH});
  transfers.Add({14, 15, EdgeType::FALL_THROUGH});
  raw_transfer_3(db, log, functors, std::move(transfers));

  Dump(db);
  ASSERT_EQ(NumFunctionInstructions(db, 9), 0u);
  ASSERT_EQ(NumFunctionInstructions(db, 10), 6u);
  ASSERT_EQ(NumFunctionInstructions(db, 11), 0u);
  ASSERT_EQ(NumFunctionInstructions(db, 12), 0u);
  ASSERT_EQ(NumFunctionInstructions(db, 13), 0u);
  ASSERT_EQ(NumFunctionInstructions(db, 14), 0u);
  ASSERT_EQ(NumFunctionInstructions(db, 15), 0u);

  // Now add the instruction 9. It will show up as a function head, because
  // it has no predecessors. The rest will stay the same because there is
  // no changes to control-flow.
  hyde::rt::Vec<instruction_input> instructions2(allocator);
  instructions2.Add({9});
  instruction_1(db, log, functors, std::move(instructions2));

  Dump(db);
  ASSERT_EQ(NumFunctionInstructions(db, 9), 1u);
  ASSERT_EQ(NumFunctionInstructions(db, 10), 6u);
  ASSERT_EQ(NumFunctionInstructions(db, 11), 0u);
  ASSERT_EQ(NumFunctionInstructions(db, 12), 0u);
  ASSERT_EQ(NumFunctionInstructions(db, 13), 0u);
  ASSERT_EQ(NumFunctionInstructions(db, 14), 0u);
  ASSERT_EQ(NumFunctionInstructions(db, 15), 0u);

  // Now add a fall-through between 9 and 10. 10 now has a predecessor, so
  // it's not a function head anymore, so all of the function instructions
  // transfer over to function 9.
  hyde::rt::Vec<raw_transfer_input> transfers2(allocator);
  transfers2.Add({9, 10, EdgeType::FALL_THROUGH});
  raw_transfer_3(db, log, functors, std::move(transfers2));

  Dump(db);
  ASSERT_EQ(NumFunctionInstructions(db, 9), 7u);
  ASSERT_EQ(NumFunctionInstructions(db, 10), 0u);
  ASSERT_EQ(NumFunctionInstructions(db, 11), 0u);
  ASSERT_EQ(NumFunctionInstructions(db, 12), 0u);
  ASSERT_EQ(NumFunctionInstructions(db, 13), 0u);
  ASSERT_EQ(NumFunctionInstructions(db, 14), 0u);
  ASSERT_EQ(NumFunctionInstructions(db, 15), 0u);

  // Now add a function call between 10 and 14. That makes 14 look like
  // a function head, and so now that 14 is a function head, it's no longer
  // part of function 9.
  hyde::rt::Vec<raw_transfer_input> transfers3(allocator);
  transfers3.Add({10, 14, EdgeType::CALL});
  raw_transfer_3(db, log, functors, std::move(transfers3));

  Dump(db);
  ASSERT_EQ(NumFunctionInstructions(db, 9), 5u);
  ASSERT_EQ(NumFunctionInstructions(db, 10), 0u);
  ASSERT_EQ(NumFunctionInstructions(db, 11), 0u);
  ASSERT_EQ(NumFunctionInstructions(db, 12), 0u);
  ASSERT_EQ(NumFunctionInstructions(db, 13), 0u);
  ASSERT_EQ(NumFunctionInstructions(db, 14), 2u);
  ASSERT_EQ(NumFunctionInstructions(db, 15), 0u);
}
