# Copyright 2026, Peter Goodman. All rights reserved.
# Copyright 2021, Trail of Bits, Inc. All rights reserved.

compile_datalog(
  DATABASE_NAME mini_disassembler
  LIBRARY_NAME mini_disassembler
  CXX_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}"
  DOT_OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/database.dot"
  IR_OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/database.ir"
  SOURCES database.dr
)

add_executable(mini_disassembler_standalone_test
  Standalone.cpp)

target_link_libraries(mini_disassembler_standalone_test
  PRIVATE
    drtest
    mini_disassembler)

add_test(
  NAME MiniDisassembler
  COMMAND mini_disassembler_standalone_test)
