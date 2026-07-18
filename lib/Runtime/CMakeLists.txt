# Copyright 2026, Peter Goodman. All rights reserved.
# Copyright 2021, Trail of Bits, Inc. All rights reserved.

set(Runtime_PUBLIC_H
  "${PROJECT_SOURCE_DIR}/include/drlojekyll/Runtime/Allocator.h"
  "${PROJECT_SOURCE_DIR}/include/drlojekyll/Runtime/Hash.h"
  "${PROJECT_SOURCE_DIR}/include/drlojekyll/Runtime/Table.h"
  "${PROJECT_SOURCE_DIR}/include/drlojekyll/Runtime/Vec.h"
)

set(Runtime_SRCS
  "Allocator.cpp"
)

dr_define_static_library(Runtime
  CURDIR ${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES ${Runtime_SRCS}
  PUBLIC_HEADERS ${Runtime_PUBLIC_H}
)
