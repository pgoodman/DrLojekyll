# Session 24 charter — post-P7: pick the next cut (runtime P6.3–P6.6 vs physical P7b/P8/P9)

You are resuming the **keyed-instance greenfield rewrite** on branch `keyed-instances` (Dr. Lojekyll,
the `hyde` C++ Datalog compiler). **P1 + P2 + P3 + P4 + P5 + P6.1 + P6.2 + P7 ARE LANDED** (OptDiff
**SUITE: PASS**, ctest **5/5**). **P7 closed the `keyed-instances` namesake arc**: a bound+free
`#query` now lowers to a partial-key hash SEEK (`Index::First/Next`) — the FIRST codegen quality win
from the `AccessPlan` authority. The M3 full-materialization backend still EVALUATES; the frozen
Regional layer is a compile-time observer that now steers codegen via P4's plan-select + P7's seek.

## Read first (resume authority, in order)
1. **`p7-execution-grounding.md`** — the LANDED P7 record (the CURRENT-CODE grounding): §1 the
   Q1a-vs-Q1b scope fork (owner ratified **Q1a broad** — the seek fires on ANY partial binding, NOT
   gated on `@key`; the P5↔physical firewall stays UP), §2 the seven open-Q resolutions, §3 the
   five landed diffs (D1 enum split, D2 dispatch, D5 no-op, D6 belt, D8 test) + execution order, §4
   the empirical-spike results, §5 IR states, §6 the exit gate.
2. **memory `regional-dataflow-core-epoch`** (P7 banner at the head) + **`greenfield-rewrite-motivation`**.
3. **`p7-grounding-seed.md`** (§1 the as-is pseudocode is still accurate for the freeze/codegen path;
   §2 the P4-substrate reconciliation; §3 the P7 diffs now LANDED — read as history).
4. For the physical layers: **`keyed-rewrite-p7p9-diffs.md` §2 (P8) / §3 (P9)** +
   **`keyed-rewrite-p7p9-critique.md`** (the s14–15 design; PARTLY STALE — predate P4/P5/P6/P7; trust
   `p7-execution-grounding.md` §2-Q6 and the P7 banner for what is #query-path-only vs interior).
   For the runtime layers: **`reconstruction-diffs.md` §3-P6.3..P6.6** + the memory
   `demand-cost-model` / `mobius-differential-dataflow` / `free-termination-paper` theory tier.

## The [OWNER STOP] — pick the next cut, then DOCS-ONLY grounding until execution is green-lit
There is NO pre-ranked next cut this session — P7 completed the physical-seek arc, and the three
candidate directions are genuinely different in character. CONFIRM the cut with the owner first:

- **P7b — interior/join plan-driven scans (the natural P7 continuation).** P7 is #query-path-ONLY;
  the interior/join `EmitScan` path (`Database.cpp:2778`) still re-derives its arm from
  index-presence × arity with NO stored plan. P7b threads `plan_kind` onto `ProgramTableScanRegion`
  (the deferred B-P7 / D4), moves V-PLAN-HONEST to the EmitScan emission site with the `kUnplanned`
  skip, and drives interior scans by plan. SMALL, on-theme, compile-time+codegen, structural gates —
  the cleanest continuation of the AccessPlan authority. (See `p7-execution-grounding.md` §2-Q5/Q6.)
- **P6.3–P6.6 — runtime evaluation (the biggest, toward replacing M3).** Fusion / cyclic
  `RuleActivationEdge` / joint rooted-reachability + semi-naive fixpoint / per-fact DRed on
  `RegionalFactId`. The compile-time model (P3/P4/P5/P6.1/P6.2) is the substrate; this makes the
  DORMANT derivation/route half do REAL work. LARGE — must be sub-sliced; likely touches
  codegen/runtime; the first place the greenfield backend diverges from M3. A compile-time-only
  P6.3 fusion-DETECTION spike is the low-risk entry (detect fusion opportunities, no runtime).
- **P8 / P9 — the remaining physical layers.** P8 = the cross-relation ordered TRIE (Free Join /
  COLT) — needs a NEW runtime range/trie structure (none exists; all-hash today), `BindingStateId`
  node interning, ordered `.Range` subtree DFS. P9 = access-path INFERENCE from a DataFlow signal.
  Both are heavier and were deprioritized behind P7; P8 in particular is a real runtime-structure
  build, not a select-and-read.

Then run the grounding loop below **DOCS-ONLY**; present the design + exit gate and STOP for the
execution go/no-go. If green-lit, execute as one coherent commit with the structural gate GREEN
(OptDiff `SUITE: PASS`, ctest 5/5; carriers + goldens re-blessed after review; codegen byte-stable
for every non-affected program), then update CLAUDE.md + the memory topic + write the session-25 seed.

## Method — the grounding loop, via WORKFLOWS, opus + sonnet (the loop that landed P2–P7)
1. **Pseudocode the as-is** (sonnet re-verify every anchor at tip — the pipeline drifts each session).
2. **Formulate design-goal diffs at hunk grain** (opus), settle the open questions, each with a
   DISCRIMINATING STRUCTURAL exit gate.
3. **Critique adversarially** (opus refuter panel) — **VERIFY EMPIRICALLY**: compile throwaway
   carriers and dump `-region-out`/`.rel`/`.h`/`.cpp`; for a change with a measurable blast radius,
   a THROWAWAY-WORKTREE spike (apply the candidate diffs, build `-Werror`, run the full suite,
   measure the true golden move + answer-invariance, DISCARD) is the decisive de-risk — it caught
   P7's stale unit expectation and confirmed the Q1a blast radius. Keep the main tree pristine.
4. **Author the desired IR states** (predict-then-verify, STRUCTURAL pins).
Model tiering (memory `subagent-model-tiering`): sonnet = anchor re-verification + baseline dumps;
opus = the design diffs, the refuter panel, the IR states.

## Gotchas (carried)
- clangd diagnostics in this repo are NOISE (no include paths) — trust the real build only.
- macOS bash 3.2 / zsh word-splitting: use `${=var}` when a var holds multiple CLI args
  (`for c in $CASES` iterates ONCE — this bit twice this session; use `${=CASES}`).
- `.dr` files are ASCII-ONLY (a non-ASCII em-dash is a lex error).
- Run builds/suite SILENT on success; the full OptDiff suite takes ~3–4 min — run it BACKGROUNDED
  (`run_in_background`) and await the completion notification (a 2-min foreground bash times out).
  Never run bench concurrently with the suite.
- Bless goldens ONLY via `runall.sh --bless <workroot>` after reviewing the delta (a symlink golden
  is never written through). `key_partial_1.region.*` / `key_corecursion_1.region.*` are REAL files
  (not symlinks); `key_tc_witness` / `key_neighborhood_witness` / `key_multi_adorn_witness` region
  goldens ARE symlinks — never bless through them.
- Copyright on NEW files: `// Copyright <year>, Peter Goodman. All rights reserved.` ONLY.
- The AccessPlan enum is `{kUnplanned=0, kFullScanFilter, kFullKeyHashLookup, kPartialKeyHashSeek}`;
  `plan` is ∉ Hash/Equals and `RequestPortRecord` is never hashed — keep it that way (byte-safety).
