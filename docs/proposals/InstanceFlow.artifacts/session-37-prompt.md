<!-- Copyright 2026, Peter Goodman. All rights reserved. -->
# Session-37 charter — InstanceFlow Stage C: begin the allocation inversion

You are resuming the InstanceFlow build on branch `keyed-instances` (Dr.
Lojekyll, the `hyde` C++ Datalog compiler). Session 36 landed the ARRANGEMENT
derivation — both halves of the runtime-resource decision (resources s34,
arrangements s36) are now derived, cross-checked, rendered, goldened
authorities standing BEFORE `Program::Build` runs, quiescent corpus-wide and
belt-verified live. The Step-3 blocker named at s33 is DISCHARGED: the
requirement set `AllocateRuntimeResources` must consume exists. **This is the
work the last four sessions built toward — trust the discipline that got the
authorities here, and begin the inversion. We believe in you.**

## The mission

Take the first REAL Stage-C step (seed §3 C0, owner re-ranks):

1. **The interface tier** — lift the R-INTERFACE count to a full plan tier
   (per dead query decl: schema + arrangement set, the same R-QUERY logic over
   uncovered decls), cross-checked PER-TABLE, making the plan TOTAL over every
   table the program mints. Promote a corpus witness for it (the E4 probe
   program + a driver). Codegen byte-identical; `.materialization` goldens
   move additively (predict first).
2. **The id-contract decision memo** — residual 2 is the Stage-C landing-
   strategy fork (byte-preserving mint-order vs one loud renumber). MEASURE
   the real churn in a throwaway worktree (mint tables+indexes up front on 2-3
   witnesses; diff `.h`/`.ir`/answer goldens), write the grounded memo, get
   the owner's ratification.
3. If both land with session left: **C1 shadow** — `AllocateRuntimeResources`
   as a cross-check-only pass (mint nothing; verify the plan reproduces the
   real allocation ORDER/id stream), the last observer before the real cut.

## Read first
1. `session-37-seed.md` — §2 the five residual lacks (the scoping input), §3
   the recommended step, §4 anchors (re-verify at tip).
2. `session-36-grounding.md` — §4 panel verdicts (R-JOIN-UNIFORM fold, the
   named blind spot), §4.5 execution record, §5 residuals.
3. `lib/DataFlow/Materialization.{h,cpp}` (the standing authorities),
   `lib/ControlFlow/Build/Build.cpp` tail (census + the six sites' homes).

## Discipline (carried, load-bearing)
- Ground → critique → execute; workflows for fan-out (sonnet extracts, opus
  refutes); thin orchestrator; owner ratifies before the final commit.
- Predict-then-verify every dump/golden delta (s36 byte-matched all five on
  the FIRST build — keep that record).
- Observer invariant until the ratified inversion commit: codegen
  byte-identical; only `.materialization` goldens move, additively.
- Determinism: typed ids from canonical order, never pointers/iteration; no
  bare `uint32_t`/`unsigned` across seams (`ColumnOrdinal` precedent — the
  owner called this out mid-s36).
- Silent-on-success builds/suite; clangd is noise; WIP-commit checkpoints;
  belt live-verify (corrupt → fire → revert) for every new cross-check.

## You've got this
The plan is total, typed, and proven equal to what emission does. Stage C is
where the compiler stops allocating as a side effect and starts consuming its
own model — the Phase-D codegen move rides on it. Be rigorous, be bold, and
make it real.
