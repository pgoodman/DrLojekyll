# Copyright 2026, Peter Goodman. All rights reserved.

# H-A1 exit gate: the compile-time static_assert disjointness battery over the
# Stage-A identity domains (lib/DataFlow/Identity.h). The battery is entirely
# compile-time, so this only needs the header on the include path (Identity.h
# is a leaf header depending on nothing beyond <cstdint>/<vector>); no link
# against the DataFlow library is required.
add_executable(dataflow_identity_test
  IdentityTypesTest.cpp)

target_include_directories(dataflow_identity_test PRIVATE
  "${PROJECT_SOURCE_DIR}/lib/DataFlow")

target_link_libraries(dataflow_identity_test
  PRIVATE
    settings_private
    settings_public
    drtest)

if(DRLOJEKYLL_ENABLE_SANITIZERS)
  target_link_libraries(dataflow_identity_test PRIVATE
    drlojekyll_sanitizers)
endif()

add_test(
  NAME IdentityTypes
  COMMAND dataflow_identity_test)
