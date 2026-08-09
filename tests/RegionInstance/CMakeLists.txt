# Copyright 2026, Peter Goodman. All rights reserved.

# P3 exit gate (RegionalDataFlowCore.artifacts/p3-grounding.md §8-F4a): the
# discriminating structural battery over the P3 request/derivation model
# (include/drlojekyll/Regional/RegionInstance.h). RegionInstance.h is a leaf
# header (depends only on the standard library), so this test needs only the
# public include path — no link against the Regional library.
add_executable(region_instance_test
  RegionInstanceTest.cpp)

target_include_directories(region_instance_test PRIVATE
  "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(region_instance_test
  PRIVATE
    settings_private
    settings_public
    drtest)

if(DRLOJEKYLL_ENABLE_SANITIZERS)
  target_link_libraries(region_instance_test PRIVATE
    drlojekyll_sanitizers)
endif()

add_test(
  NAME RegionInstance
  COMMAND region_instance_test)
