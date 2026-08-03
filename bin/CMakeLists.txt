# Copyright 2026, Peter Goodman. All rights reserved.
# Copyright 2020, Trail of Bits, Inc. All rights reserved.

# `drlojekyll`
find_package(Filesystem REQUIRED)

add_executable(drlojekyll
    drlojekyll/Main.cpp
)

target_link_libraries(drlojekyll PRIVATE
    settings_private
    settings_public
    std::filesystem

    ControlFlow
    CodeGen
    DataFlow
    Display
    Lex
    Parse
    Util
    Version
)

if(DRLOJEKYLL_ENABLE_SANITIZERS)
    target_link_libraries(drlojekyll PRIVATE
        drlojekyll_sanitizers
    )
endif()

add_executable(${PROJECT_NAME}::drlojekyll ALIAS drlojekyll)

# `drlojekyll-oracle`: the reference counting oracle — an interpreter over
# the unoptimized dataflow IR that executes the derivation-counter
# semantics of docs/proposals/StackSafeNegation.md §5 and cross-checks an
# incremental counter evaluation against an independent from-scratch
# stratified evaluation after every batch.
add_executable(drlojekyll-oracle
    Oracle/Main.cpp
)

target_link_libraries(drlojekyll-oracle PRIVATE
    settings_private
    settings_public

    DataFlow
    Display
    Lex
    Parse
    Util
)

if(DRLOJEKYLL_ENABLE_SANITIZERS)
    target_link_libraries(drlojekyll-oracle PRIVATE
        drlojekyll_sanitizers
    )
endif()

# `drlojekyll-refinterp`: the I0 reference relational interpreter — a
# definitional evaluator over RAW PARSED CLAUSES (OG1-parsed: deliberately
# does NOT link DataFlow, so no bug in Query::Build can hide from the
# referee), emitting the Canonical Behavioral Format. See
# docs/proposals/RegionalDataFlowCore.artifacts/stage-i0-interpreter.md.
add_executable(drlojekyll-refinterp
    RefInterp/Main.cpp
)

target_link_libraries(drlojekyll-refinterp PRIVATE
    settings_private
    settings_public

    Display
    Lex
    Parse
    Util
)

if(DRLOJEKYLL_ENABLE_SANITIZERS)
    target_link_libraries(drlojekyll-refinterp PRIVATE
        drlojekyll_sanitizers
    )
endif()

# `drlojekyll-refharness`: the I0 behavioral-harness emitter — reads a
# parsed module and emits behavioral_main.cpp against the STABLE public
# query/message ABI naming (OG2-tool; couples to the frozen seam only).
add_executable(drlojekyll-refharness
    RefHarness/Main.cpp
)

target_link_libraries(drlojekyll-refharness PRIVATE
    settings_private
    settings_public

    Display
    Lex
    Parse
    Util
)

if(DRLOJEKYLL_ENABLE_SANITIZERS)
    target_link_libraries(drlojekyll-refharness PRIVATE
        drlojekyll_sanitizers
    )
endif()

if(DRLOJEKYLL_ENABLE_INSTALL)
    install(
        TARGETS drlojekyll
        EXPORT "${PROJECT_NAME}Targets"
    )
endif()
