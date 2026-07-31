# Comment and Code Architecture Drift

This audit compared two independent views:

- **Code-only view:** entry points, types, includes, calls, loops, writes, and
  tests. Comments and proposals were not used as authority.
- **Comment-only view:** source comments and architecture/proposal prose. Code
  behavior was treated as unknown.

The executable view is summarized in `architecture-code.md`.

## Comment-Only Architecture

```text
Query is the logical graph and demand-transform home.
Rel is the sole differential scheduling authority and sits between Query and
ControlFlow.
ControlFlow is emitted entirely from Rel and then optimized.
All historical discovery/emission paths have been deleted.
Pass flags reproduce four stable optimization modes.
Aggregates/KV are both a known unsupported gap and a landed StateCell feature.
Prov is shared by identity-join removal and a CostModel L1 pass.
The full suite, ASAN, config invariance, and fuzzing are standing gates.
```

## Drift Diff

```diff
 Query is the logical graph and demand-transform home.
 Rel models and validates differential scheduling.
-Rel is a standalone sole authority.
+Rel and ControlFlow are mutually dependent; some eager emission precedes Rel
+inventory and is later cross-checked. Rel is an in-build hybrid authority.

-The historical DataFlow -> ControlFlow path has become a clean
-DataFlow -> Rel -> ControlFlow pipeline.
+Rel is an active cutover. It owns substantial differential inventory and
+schedule, while eager ControlFlow builders still author region shape and feed
+later Rel enrollment. The intended end state is not encoded consistently.

-Rel.h describes the current DRFlowGraph.
+Rel.h's opening says it is an R1 shadow with two op families and no plan trees
+or emission. The file now contains roughly thirty op kinds, plan spines,
+rounds, linearization, and lowering support.

-demand_instance_enabled is unreachable and no CLI flag exists.
+Main parses -demand-instance and Program::Build assigns the field.

-disable-dataflow-opt aliases cse/canon/dfe/sink only.
+PassPolicy also disables df.ident_join; the mono .irgold depends on this split.

-Aggregates and KV indices are unsupported lowering gaps.
+StateCell lowering, generated reduction surfaces, runtime support, and tests
+exist. docs/Architecture.md is stale relative to code and RuntimeAndCodegen.md.

-Prov is a substrate for a present CostModel cardinality pass.
+No CostModel implementation exists. Prov currently serves containment and
+identity-join removal only.

-The full suite and fuzzing are normal automated gates.
+The full OptDiff suite is manual and absent from CTest/CI. The fuzz workflow is
+disabled by filename and uses stale option names.
```

## High-Value Cleanup Targets

1. `lib/Rel/Rel.h:3-19`: replace the R1 migration narrative with current object
   ownership, lifetime, and invariants.
2. `lib/ControlFlow/Build/Build.h:207-222`: update `dr_flow` ownership and remove
   the false no-CLI/unreachable demand-instance history.
3. `bin/drlojekyll/Main.cpp:398-400` and `PassPolicy`: establish one true alias
   definition including identity-join behavior.
4. `docs/Architecture.md:454-479`: update aggregate/KV and differential product
   support from current code and tests.
5. `lib/DataFlow/Prov.h:7-9`: remove the unimplemented CostModel as a present
   architectural justification. State the reusable containment contract.
6. `lib/Rel/Rel.cpp`, `Rel.h`, and `Build/Stratum.cpp`: remove stage, ledger,
   review-agent, and deleted-path narration. Keep current validators, ownership,
   lowering rules, and non-obvious scheduling invariants.
7. `lib/DataFlow/Optimize.cpp:805-825`: delete the dead commented `df.sink`
   implementation and its pass registration.
8. `lib/DataFlow/CMakeLists.txt:19-21`: remove the duplicate `Compare.cpp` entry.
9. `CLAUDE.md` Rel section: replace the accumulated landing narrative. It says
   both that eager wrappers were retired and that `LowerRelStep_*` wrappers
   currently lower the modeled arms, while code uses census-and-call dispatch
   instead.
10. `lib/DataFlow/Optimize.cpp:644` and `Query.h`: make `PassPolicy` required;
    no active caller uses the nullable/default policy path.
11. `lib/ControlFlow/Build/Build.h` and `Procedure.cpp`: remove the stale
    nullable `dr_flow` state or model construction as explicit pre/post-Rel
    contexts.

The source scan found 516 history/proposal-pattern lines across 80 source files.
This is a triage list, not an automatic deletion count. Terms such as
"differential", real algorithmic phase names, and current validator names can
be correct. Comments should survive only when they explain current state,
authority, termination, or a non-obvious invariant.

## Why This Matters for CostModel

The old continuation prompt tells the next session to anchor on comments that
are already false at current tip. That encourages a new layer to encode the
migration story instead of the executable contract. Before adding CostModel
comments, define the immutable input and output types in code and describe only
their current invariants.
