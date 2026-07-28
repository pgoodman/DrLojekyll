# Copyright 2026, Peter Goodman. All rights reserved.

# A ctest unit over the DR-IR validators. It links the compiler-internal static
# libraries (Rel + its transitive deps: DataFlow/ControlFlow/Parse/Lex/
# Display/Util) so it can construct a DRFlowGraph and call the always-on
# validators directly. Rel and ControlFlow are mutually-referencing static
# libs resolved at the final executable link, so both are listed.
add_executable(rel_validators_test
  DeathFrontierTest.cpp
  InstanceOrderTest.cpp)

# Rel.h transitively includes ControlFlow's internal impl headers
# ("Program.h", "Build.h") by quoted include — expose those dirs (mirrors
# lib/Rel/CMakeLists.txt).
target_include_directories(rel_validators_test PRIVATE
  "${PROJECT_SOURCE_DIR}/lib/Rel"
  "${PROJECT_SOURCE_DIR}/lib/ControlFlow"
  "${PROJECT_SOURCE_DIR}/lib/ControlFlow/Build")

target_link_libraries(rel_validators_test
  PRIVATE
    settings_private
    settings_public
    drtest
    Rel
    ControlFlow
    CodeGen
    DataFlow
    Display
    Lex
    Parse
    Util)

if(DRLOJEKYLL_ENABLE_SANITIZERS)
  target_link_libraries(rel_validators_test PRIVATE
    drlojekyll_sanitizers)
endif()

add_test(
  NAME RelValidators
  COMMAND rel_validators_test)
