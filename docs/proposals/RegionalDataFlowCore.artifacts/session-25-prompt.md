# Session 25 charter — post-P7b: pick the next cut (P6.3 runtime-eval spike / P7c belt retire / P8-P9)

You are resuming the **keyed-instance greenfield rewrite** on branch `keyed-instances` (Dr.
Lojekyll, the `hyde` C++ Datalog compiler). **P1–P7 + P7b ARE LANDED** (OptDiff **SUITE: PASS
(226)**, ctest **5/5**). P7b extended the `AccessPlan` physical authority from the `#query` path
to the INTERIOR/JOIN table-scan path: `ProgramTableScanRegionImpl` now carries an
`AccessPlan plan_kind` (∉ Hash/Equals/MergeEqual), stamped at the sole live mint
`BuildMaybeScanPartial`, refereed by the **EmitScan V-PLAN-HONEST belt** (D4 "Option-2"), and
rendered as a gated ` plan=` token on the `.ir` `scan-*` line via the new single-source
`inline hyde::AccessPlanText`. Codegen is BYTE-UNCHANGED (the belt reads the arm codegen already
picks; `plan_kind` is a compile-time shadow). The M3 full-materialization backend still EVALUATES;
`lib/Regional` is a compile-time observer now steering/refereeing codegen at THREE points (P4
plan-select, P7 `#query` seek, P7b interior-scan belt).

## Read first (resume authority, in order)
1. **`p7b-execution-grounding.md`** — the LANDED P7b record: §1 what P7b is (no scope fork), §2
   settled open questions, §3 the 6 diffs, §4 the empirical spike (belt-live + hardened-belt
   runs), §5 IR states, §6 exit gate. The method exemplar.
2. **`session-24-whole-program-seed.md`** — the grounded whole-program pseudocode. STILL VALID
   except two P7b deltas: (a) §1.3 `ProgramTableScanRegionImpl` now has `plan_kind` + `PlanKind()`;
   (b) §1.4-B `EmitScan` now opens with the V-PLAN-HONEST per-kind implication belt (skip on
   `kUnplanned`), and the `.ir` scan render carries a gated ` plan=` token. Re-verify anchors at
   tip — the pipeline drifts each session.
3. **memory `regional-dataflow-core-epoch`** (P7b banner at the head) + **`greenfield-rewrite-motivation`**.
4. For the runtime layers: **`reconstruction-diffs.md §3-P6.3..P6.6`** + memory
   `demand-cost-model` / `mobius-differential-dataflow` / `free-termination-paper`. For the
   physical layers: **`keyed-rewrite-p7p9-diffs.md §2 (P8) / §3 (P9)`** (partly stale — trust the
   whole-program seed §2 for tip anchors).

## The [OWNER STOP] — pick the next cut, then DOCS-ONLY grounding until execution is green-lit
No pre-ranked next cut. CONFIRM with the owner first:

- **P6.3–P6.6 — runtime evaluation (the biggest, toward replacing M3).** The compile-time model
  (P3/P4/P5/P6.1/P6.2) is a pure OBSERVER; this makes the DORMANT derivation/route half do REAL
  work (fusion / cyclic `RuleActivationEdge` / joint rooted-reachability + semi-naive fixpoint /
  per-fact DRed). LARGE — the low-risk entry is a **compile-time P6.3 fusion-DETECTION spike**
  (detect fusable rule chains from `rules` + `recursive_components`, dump only, no runtime; the
  P6.1/P6.2 gated-block precedent means it should move no golden). First place the greenfield
  backend diverges from M3.
- **P7c — retire the probe-REDUNDANT TUPLECMP belt (small, on-theme, the AccessPlan payoff).**
  `BuildMaybeScanPartial` (`Build.h:461-467`) emits a TUPLECMP re-check belt after every partial
  index scan, even though `Index::First/Next` is full-key EXACT (the belt is dead — the R-final
  "Fold C" candidate). Now that P7b NAMES those scans `kPartialKeyHashSeek` and the EmitScan belt
  proves the arm honest, dropping the re-check is a real codegen QUALITY win with a clean witness
  (the seek carriers' `.ir`/`.h` shrink; answers invariant). This is the first cut where P7b's
  authority PAYS OFF in emission, not just referees it. UNLIKE P7b it MOVES codegen goldens.
- **P8 / P9 — the remaining physical layers (heaviest).** P8 = the cross-relation ordered TRIE
  (Free Join / COLT) — NEW runtime range/trie structure (none exists; all-hash today),
  `BindingStateId` node interning, ordered `.Range` subtree DFS. P9 = access-path INFERENCE from a
  DataFlow signal (a pre-Optimize walk). Both heavy; deprioritized behind P7.

Then run the grounding loop below **DOCS-ONLY**; present the design + exit gate and STOP for the
execution go/no-go. On green-light, execute as one coherent commit (SUITE PASS, ctest 5/5;
goldens re-blessed after review; codegen byte-stable for every non-affected program — for P7c,
byte-stable for every non-seek program), then update CLAUDE.md + memory + write the session-26 seed.

## Method — the grounding loop, run via WORKFLOWS with opus + sonnet (the loop that landed P2–P7b)
Run all four phases as **workflows** (thin orchestrator; sequential single-phase workflows; watch
the `(await parallel(...)).filter(...)` precedence). DOCS-ONLY until green-lit.
1. **Build the pseudocode of the as-is** — start from the whole-program seed §1 + the two P7b
   deltas above; **re-verify every anchor at tip** (sonnet).
2. **Formulate design-goal diffs** at hunk grain (opus); each with a DISCRIMINATING STRUCTURAL
   exit gate (what dump/byte differs, how a bug in that diff is caught).
3. **Critique adversarially** (opus refuter panel) — **VERIFY EMPIRICALLY**: compile throwaway
   carriers; for a measurable blast radius, a **THROWAWAY-WORKTREE spike** (apply diffs, build,
   run the full suite, measure the true golden move + answer-invariance, DISCARD — main tree
   pristine). This caught P7's stale unit expectation and confirmed P7b's zero-move blast radius.
4. **Author the desired OUTPUT STATES of the IRs** (predict-then-verify, STRUCTURAL pins); decide
   exactly which goldens MOVE.

Model tiering (memory `subagent-model-tiering`): **sonnet** = anchor re-verification, baseline
dumps; **opus** = design diffs, the refuter panel, IR desired-states. Keep the orchestrator thin.

## Gotchas (carried)
- clangd diagnostics in this repo are NOISE (no include paths) — trust the real build only.
- macOS bash 3.2 / zsh word-splitting: use `${=var}` when a var holds multiple CLI args.
- `.dr` files are ASCII-ONLY (a non-ASCII em-dash is a lex error).
- Run builds/suite SILENT on success; the full OptDiff suite takes ~3–4 min — run it BACKGROUNDED
  and await the completion notification. Never run bench concurrently with the suite.
- Bless goldens ONLY via `runall.sh --bless <workroot> [filter]` after reviewing the delta (a
  symlink golden is never written through). `negate_1.ir.opt.golden` / `insert_4.ir.opt.golden`
  are REAL files (P7b's additions); the twin-equivalence witnesses' region/ir goldens are symlinks.
- Copyright on NEW files: `// Copyright <year>, Peter Goodman. All rights reserved.` ONLY.
- `AccessPlanText` is now the SINGLE-SOURCE `inline` in `RegionInstance.h` — both the Regional
  `-region-out` dump and the ControlFlow `.ir` scan render call it; do not re-duplicate it.
- The AccessPlan enum is `{kUnplanned=0, kFullScanFilter, kFullKeyHashLookup, kPartialKeyHashSeek}`;
  `plan_kind` on `ProgramTableScanRegionImpl` is ∉ Hash/Equals/MergeEqual — keep it that way
  (byte-safety); the EmitScan belt SKIPS `kUnplanned` (the statically-dead join-pivot mint).
