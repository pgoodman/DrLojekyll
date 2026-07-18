# Copyright 2020, Trail of Bits, Inc. All rights reserved.

set(DR_SRCS
    "DR.h"

    "DR.cpp"
)

set(DR_PRIV_DEPS
)

set(DR_DEPS
    DataFlow
    Display
    Lex
    Parse
    Util
)

dr_define_static_library(DR
  CURDIR ${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES ${DR_SRCS}
  DEPENDENCIES ${DR_DEPS}
  PRIVATE_DEPS ${DR_PRIV_DEPS}
)

# The DR-IR is a private compiler-internal IR of the Program::Build pass: it
# has no include/drlojekyll/ public surface, but it references ControlFlow's
# internal impl headers (Program.h for TABLE/VECTOR/ProgramImpl, Build/Build.h
# for Context and the build helpers). ControlFlow, in turn, links DR and
# includes DR.h from Build/{Stratum,Procedure}.cpp — the two targets are
# mutually referencing static libraries, resolved at the final executable
# link. Expose ControlFlow's internal dirs to this target so the quoted
# includes ("Program.h", "Build.h") resolve; no public surface is created.
target_include_directories(DR PRIVATE
  "${PROJECT_SOURCE_DIR}/lib/ControlFlow"
  "${PROJECT_SOURCE_DIR}/lib/ControlFlow/Build"
)
