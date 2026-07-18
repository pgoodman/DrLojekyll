# Copyright 2026, Peter Goodman. All rights reserved.

add_executable(runtime_test
  RuntimeTest.cpp
  StateCellTest.cpp)

target_link_libraries(runtime_test
  PRIVATE
    settings_private
    settings_public
    drtest
    Runtime)

add_test(
  NAME Runtime
  COMMAND runtime_test)
