# Copyright 2026, Peter Goodman. All rights reserved.

# A ctest unit over the DR-IR validators. It links the compiler-internal static
# libraries (DeltaRel + its transitive deps: DataFlow/ControlFlow/Parse/Lex/
# Display/Util) so it can construct a DRFlowGraph and call the always-on
# validators directly. DeltaRel and ControlFlow are mutually-referencing static
# libs resolved at the final executable link, so both are listed.
add_executable(deltarel_validators_test
  InstanceOrderTest.cpp)

# DeltaRel.h transitively includes ControlFlow's internal impl headers
# ("Program.h", "Build.h") by quoted include — expose those dirs (mirrors
# lib/DeltaRel/CMakeLists.txt).
target_include_directories(deltarel_validators_test PRIVATE
  "${PROJECT_SOURCE_DIR}/lib/DeltaRel"
  "${PROJECT_SOURCE_DIR}/lib/ControlFlow"
  "${PROJECT_SOURCE_DIR}/lib/ControlFlow/Build")

target_link_libraries(deltarel_validators_test
  PRIVATE
    settings_private
    settings_public
    drtest
    DeltaRel
    ControlFlow
    CodeGen
    DataFlow
    Display
    Lex
    Parse
    Util)

if(DRLOJEKYLL_ENABLE_SANITIZERS)
  target_link_libraries(deltarel_validators_test PRIVATE
    drlojekyll_sanitizers)
endif()

add_test(
  NAME DeltaRelValidators
  COMMAND deltarel_validators_test)
