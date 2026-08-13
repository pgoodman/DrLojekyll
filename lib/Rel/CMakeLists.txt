# Copyright 2026, Peter Goodman. All rights reserved.

set(REL_SRCS
    "Rel.h"

    "Rel.cpp"
    "Format.cpp"
)

set(REL_PRIV_DEPS
)

set(REL_DEPS
    DataFlow
    Display
    Lex
    Parse
    Regional
    Util
)

dr_define_static_library(Rel
  CURDIR ${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES ${REL_SRCS}
  DEPENDENCIES ${REL_DEPS}
  PRIVATE_DEPS ${REL_PRIV_DEPS}
)

# The DR-IR is a private compiler-internal IR of the Program::Build pass: it
# has no include/drlojekyll/ public surface, but it references ControlFlow's
# internal impl headers (Program.h for TABLE/VECTOR/ProgramImpl, Build/Build.h
# for Context and the build helpers). ControlFlow, in turn, links Rel and
# includes Rel.h from Build/{Stratum,Procedure}.cpp — the two targets are
# mutually referencing static libraries, resolved at the final executable
# link. Expose ControlFlow's internal dirs to this target so the quoted
# includes ("Program.h", "Build.h") resolve; no public surface is created.
target_include_directories(Rel PRIVATE
  "${PROJECT_SOURCE_DIR}/lib/ControlFlow"
  "${PROJECT_SOURCE_DIR}/lib/ControlFlow/Build"
  # s34 Phase-C: DRTable refers to StateResourceId (lib/DataFlow/Materialization.h),
  # the first Rel object to carry a materialization-map identity beside its TABLE*.
  "${PROJECT_SOURCE_DIR}/lib/DataFlow"
)
