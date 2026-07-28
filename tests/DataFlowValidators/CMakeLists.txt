# Copyright 2026, Peter Goodman. All rights reserved.

# A ctest unit over the DataFlow-layer always-on diagnostics (OWN-3, the
# guard-annotation fold check). It links the compiler-internal DataFlow static
# library (+ its transitive deps Display/Lex/Parse/Util) so it can hand-build
# GuardAnnotation records and call the pure check directly.
add_executable(dataflow_validators_test
  GuardAnnotationFoldTest.cpp)

# The pure-check declarations live on the PRIVATE lib/DataFlow/Query.h --
# expose that dir (mirrors tests/RelValidators exposing lib/Rel).
target_include_directories(dataflow_validators_test PRIVATE
  "${PROJECT_SOURCE_DIR}/lib/DataFlow")

target_link_libraries(dataflow_validators_test
  PRIVATE
    settings_private
    settings_public
    drtest
    DataFlow
    Display
    Lex
    Parse
    Util)

if(DRLOJEKYLL_ENABLE_SANITIZERS)
  target_link_libraries(dataflow_validators_test PRIVATE
    drlojekyll_sanitizers)
endif()

add_test(
  NAME DataFlowValidators
  COMMAND dataflow_validators_test)
