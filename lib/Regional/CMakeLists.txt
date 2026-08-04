# Copyright 2026, Peter Goodman. All rights reserved.

set(Regional_PUBLIC_H
    "${PROJECT_SOURCE_DIR}/include/drlojekyll/Regional/Regional.h"
)

set(Regional_SRCS
    "Planning.cpp"
    "Format.cpp"
)

set(Regional_PRIV_DEPS
)

set(Regional_DEPS
    DataFlow
    Display
    Lex
    Parse
    Util
)

dr_define_static_library(Regional
  CURDIR ${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES ${Regional_SRCS}
  PUBLIC_HEADERS ${Regional_PUBLIC_H}
  DEPENDENCIES ${Regional_DEPS}
  PRIVATE_DEPS ${Regional_PRIV_DEPS}
)

# The Stage-B freeze reads ONE private DataFlow seam: Planning.cpp includes
# the internal lib/DataFlow/Query.h for `QueryImpl::row_contracts` (the
# declared friend leak on the public Query wrapper). Expose the lib/DataFlow
# dir so the quoted include ("Query.h") resolves; no public surface is
# created (the lib/Rel private-include idiom).
target_include_directories(Regional PRIVATE
  "${PROJECT_SOURCE_DIR}/lib/DataFlow"
)
