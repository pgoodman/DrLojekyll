# Copyright 2026, Peter Goodman. All rights reserved.
# Copyright 2020, Trail of Bits, Inc. All rights reserved.

set(ControlFlow_PUBLIC_H
    "${PROJECT_SOURCE_DIR}/include/drlojekyll/ControlFlow/Format.h"
    "${PROJECT_SOURCE_DIR}/include/drlojekyll/ControlFlow/Program.h"
)

set(ControlFlow_SRCS
    "Program.h"
    
    "Build/Build.h"
    "Build/Build.cpp"
    "Build/Compare.cpp"
    "Build/Generate.cpp"
    "Build/Induction.cpp"
    "Build/Induction.h"
    "Build/Init.cpp"
    "Build/Insert.cpp"
    "Build/Join.cpp"
    "Build/Negate.cpp"
    "Build/Procedure.cpp"
    "Build/Product.cpp"
    "Build/Stratum.cpp"
    "Build/Tuple.cpp"
    "Build/Union.cpp"
    
    "Analyze.cpp"
    "Data.cpp"
    "Format.cpp"
    "Induction.cpp"
    "Operation.cpp"
    "Optimize.cpp"
    "Parallel.cpp"
    "Procedure.cpp"
    "Program.cpp"
    "Region.cpp"
    "Series.cpp"
    "Visitor.cpp"
)

set(ControlFlow_PRIV_DEPS
)

set(ControlFlow_DEPS
    DataFlow
    DR
    Display
    Lex
    Parse
    Util
)

dr_define_static_library(ControlFlow
  CURDIR ${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES ${ControlFlow_SRCS}
  PUBLIC_HEADERS ${ControlFlow_PUBLIC_H}
  DEPENDENCIES ${ControlFlow_DEPS}
  PRIVATE_DEPS ${ControlFlow_PRIV_DEPS}
)

# DR.{h,cpp} moved to lib/DR (its own static-library target, linked above via
# ControlFlow_DEPS). Build/{Stratum,Procedure}.cpp include "DR.h" — expose the
# lib/DR dir so the quoted include resolves. The DR-IR stays compiler-internal
# (no include/drlojekyll/ surface).
target_include_directories(ControlFlow PRIVATE
  "${PROJECT_SOURCE_DIR}/lib/DR"
)
