# Session-30 charter — EXECUTE S1a: resurrect flat `-demand`, green on the flag-off corpus

You are resuming the keyed-instance greenfield rewrite on branch `keyed-instances` (Dr. Lojekyll, the `hyde`
C++ Datalog compiler). Session 29 GROUNDED the demand reopen and the owner RATIFIED the plan; session 30
EXECUTES S1a. Tip `05af6595` + the four session-29 docs (grounding, this prompt, the s1a manifest, and the
un-wired restored sources already in the tree).

## What session 29 settled (do NOT re-litigate)
- **The namesake win is REAL** — an in-tree spike (compiler UNMODIFIED) measured demand pruning `r`'s
  materialization in the selective regime (join-work `idx_hops` 40000→11, `finds` 3.5–7×, answer
  byte-identical, scaling with the derived/input ratio) and regressing +40% non-selective (→ cost/mode-gate).
- **Owner decisions (AskUserQuestion):** Path R **flat-only** (skip InstanceStore = S2+); **recursive-TC**
  first slice; **proceed** to build S1a. Rationale + full evidence: `session-29-grounding.md`.
- **Scope is a 2–3 session arc.** S1a = resurrect the flat transform + re-integrate with the four always-on
  abort-validator surfaces so the compiler builds and the flag-OFF corpus stays byte-identical. S1b = the
  recursive-TC witness + bench carrier + the `kPushDown` lowering exercised end-to-end. S1c = ABI suppression
  + Regional census + Tier-1 naming + bless.

## S1a is fully spec'd — execute the playbook
**`session-29-s1a-restoration-manifest.md` is the hunk-by-hunk work-list** (CMake, the Query.h/Parse.h types +
accessors, the CSE-migration surface, the `Query::Build` integration, the Main.cpp `-demand` flag, the Oracle
`suppress_demand=true` fix, the ONE real validator fix = Planning.cpp `CollectMessages`, the 2 ABI-suppression
sites, and the dependency-first restoration ORDER with two build CHECKPOINTS). It already recorded the
NO-CHANGE surfaces (RowContract, origin_decls, Rel eager-web) and the stale-anchor corrections. Re-verify tip
line numbers before each edit (drift).

Already done for you: `lib/DataFlow/Demand.cpp` (1464) + `lib/Parse/Demand.cpp` (263) are restored from git
`48cd0a4f` into the tree (untracked, un-wired). If a checkout wiped them: `git show 48cd0a4f:lib/DataFlow/
Demand.cpp > lib/DataFlow/Demand.cpp` and the Parse twin.

## The S1a exit gate (all four)
1. **Build GREEN** (`cmake --build --preset debug`), including `bin/Oracle` (the `suppress_demand=true` fix).
2. **OptDiff SUITE PASS byte-identical** — demand is OFF by default, so `ApplyDemandTransform` returns at its
   head (the flag-off containment gate: `!demand_mode && no @key pragma`). The 181-case corpus must be
   byte-for-byte unchanged (the orthogonality proof, exactly as the old `-demand` was). Run BACKGROUNDED
   (~3–4 min); await.
3. **ctest 5/5.**
4. **A demand program COMPILES** through `Query::Build` under `-demand` (CHECKPOINT 2): `-df-out` renders the
   guard JOINs + the fabricated demand relation. (End-to-end answer-correctness of a demand program is S1b —
   S1a only needs the flag-OFF corpus green + a demand program reaching a clean `-df` dump.)

## Discipline (carried, load-bearing)
- **WIP-commit at CHECKPOINT 1** (DataFlow static lib compiles standalone) before the ControlFlow/CodeGen
  edits — the memory records an incident where a checkout wiped an uncommitted prototype. Commit only WIP
  checkpoints on the branch; the owner has NOT asked for a final commit — reach a clean gate first, then ask.
- **Hand-pick hunks, never mechanically reapply a whole pre-cut diff** — top trap: `Program::Build`'s
  `bool demand_instance` 5th param + the whole InstanceStore family (keep tip's 4-arg). Leave the vestigial
  dead `DRInstance`/`demand_table` scaffolding in `lib/Rel/Rel.h:855-890` untouched (S2+ reserved).
- Run builds/suite SILENT on success (surface output only on failure). clangd is NOISE — trust the real build.
- Predict-then-verify: the flag-off SUITE is byte-identical BY CONSTRUCTION — a single golden diff is a
  containment-gate leak, investigate before blessing anything (and S1a should bless NOTHING — no goldens move).

## After S1a green
Update CLAUDE.md (the `[REMOVED at the P1 cut]` demand sections → LIVE for flat `-demand`; note InstanceStore
still removed), update memory `regional-dataflow-core-epoch`, and write the S1b seed: the recursive-TC witness
(`.dr` = the `demand_tc_witness` shape from `48cd0a4f`, `.drflags` = `-demand`, `.probes` naming the demanded
keys, driver demands-then-probes, `.batches` + oracle/monotone/behavioral goldens per the precedented pattern)
+ the bench carrier (a large selective TC) proving the measured pruning + the `kPushDown` fixpoint-interior
guard lowering exercised end-to-end. Then the S1b exit gate (result-equality ×4 modes + codegen goldens MOVE +
bench pruning + SUITE PASS).

## Read first (resume authority, in order)
1. `session-29-grounding.md` — the evidence, the fork, the scope, the result-equality mechanism, §7 the
   ratified decisions + go/no-go.
2. `session-29-s1a-restoration-manifest.md` — THE work-list. Start at §8 (the order), execute §1–§7.
3. memory `regional-dataflow-core-epoch` (s29 head) + `demand-cost-model` + `demand-subgraph-unification`.
4. `recursive-demand-seed.md` (§1 two-lowering frame, §4 why flat recursive demand hosts the fixpoint, §7 the
   cost model) + `demand_tc_witness.*`@`48cd0a4f` (the witness precedent for S1b).
