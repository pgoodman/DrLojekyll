# Copyright 2026, Peter Goodman. All rights reserved.
# Copyright 2021, Trail of Bits, Inc. All rights reserved.

include(CTest)
enable_testing()

include("${PROJECT_SOURCE_DIR}/cmake/Compiler.cmake")

# Dependency-free unit-test harness; a drop-in for the sliver of GoogleTest the
# tests use (`TEST` + `ASSERT_*` + a `main`).
add_library(drtest STATIC DrTest/DrTest.cpp)
target_include_directories(drtest PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/DrTest")

add_subdirectory(DeltaRelValidators)
add_subdirectory(InstanceStore)
add_subdirectory(MiniDisassembler)
add_subdirectory(PointsTo)
add_subdirectory(Runtime)
