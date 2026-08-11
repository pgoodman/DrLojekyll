# Session 26 charter — post-P7c: pick the next cut (P6.3 runtime-eval spike / P8–P9)

You are resuming the **keyed-instance greenfield rewrite** on branch `keyed-instances` (Dr.
Lojekyll, the `hyde` C++ Datalog compiler). **P1–P7 + P7b + P7c ARE LANDED** (OptDiff **SUITE:
PASS (226)**, ctest **5/5**). P7c retired the probe-REDUNDANT partial-scan re-check — the FIRST cut
where the `AccessPlan` authority PAYS OFF in emission (it MOVED codegen). The `AccessPlan`-authority
arc through emission is now **COMPLETE for the partial-scan path** (P7 #query seek, P7b interior/join
plan naming + honesty belt, P7c the re-check retire). No pre-ranked next cut — the [OWNER STOP] below.

## Read first (resume authority, in order)
1. **`p7c-execution-grounding.md`** — the LANDED P7c record: §1 the cut (shape (i)), §2 why
   redundant (the `Table.h:789-824` full-key-exact contract = the TABLEJOIN-body argument), §3 the
   diff, §4 the empirical spike + 3-refuter panel, §5 IR states, §6 exit gate. The method exemplar.
2. **`p7b-execution-grounding.md`** — the LANDED P7b record (the plan-naming + V-PLAN-HONEST belt
   P7c builds on).
3. **`session-26-whole-program-seed.md`** — the grounded whole-program pseudocode, REFRESHED at
   the P7c tip (`99b91335`): §0 status (P1–P7c landed), §1 the as-is pipeline pseudocode with
   P7b/P7c folded in as LANDED (§1.3 the `BuildMaybeScanPartial` mint + the vacuous seek `cmp`;
   §1.4-B `EmitScan` plan-driven + V-PLAN-HONEST belt), §2 the path forward as diffs (P6.3–P6.6,
   P8, P9, the small cuts), §3 the four authorities, §4 the next-cut table, §5 open questions.
   START HERE for the whole-program view. Re-verify anchors at tip — the pipeline drifts each
   session. (The older `session-24-whole-program-seed.md` is the pre-P7b snapshot — superseded.)
4. **memory `regional-dataflow-core-epoch`** (P7c banner at the head) + **`greenfield-rewrite-motivation`**.
5. For the runtime layers: **`reconstruction-diffs.md §3-P6.3..P6.6`** + memory `demand-cost-model`
   / `mobius-differential-dataflow` / `free-termination-paper`. For the physical layers:
   **`keyed-rewrite-p7p9-diffs.md §2 (P8) / §3 (P9)`** (partly stale — trust the whole-program seed
   §2 for tip anchors).

## The [OWNER STOP] — pick the next cut, then DOCS-ONLY grounding until execution is green-lit
No pre-ranked next cut. CONFIRM with the owner first:

- **P6.3–P6.6 — runtime evaluation (the biggest, toward replacing M3).** The compile-time model
  (P3/P4/P5/P6.1/P6.2) is a pure OBSERVER; this makes the DORMANT derivation/route half do REAL
  work (fusion / cyclic `RuleActivationEdge` / joint rooted-reachability + semi-naive fixpoint /
  per-fact DRed). LARGE — the low-risk entry is a **compile-time P6.3 fusion-DETECTION spike**
  (detect fusable rule chains from `rules` + `recursive_components`, dump only, no runtime; the
  P6.1/P6.2 gated-block precedent means it should move no golden). First place the greenfield
  backend diverges from M3.
- **P8 / P9 — the remaining physical layers (heaviest).** P8 = the cross-relation ordered TRIE
  (Free Join / COLT) — NEW runtime range/trie structure (none exists; all-hash today),
  `BindingStateId` node interning, ordered `.Range` subtree DFS. P9 = access-path INFERENCE from a
  DataFlow signal (a pre-Optimize walk). Both heavy.
- **Smaller on-theme candidates.** The header-token E-71 mini-diff (`deltarel`→`rel` in the dump
  header); any residual TUPLECMP-belt hygiene the P7c spike surfaced (none known — P7c was total for
  the seek path). Note: with P7c landed there is **no known remaining redundant re-check** on the
  scan path — do not invent one.

Then run the grounding loop below **DOCS-ONLY**; present the design + exit gate and STOP for the
execution go/no-go. On green-light, execute as one coherent commit (SUITE PASS, ctest 5/5; goldens
re-blessed after review; codegen byte-stable for every non-affected program), then update CLAUDE.md
+ memory + write the session-27 seed.

## Method — the grounding loop, run via WORKFLOWS with opus + sonnet (the loop that landed P2–P7c)
Thin orchestrator; sequential single-phase workflows; watch the `(await parallel(...)).filter(...)`
precedence. DOCS-ONLY until green-lit.
1. **As-is pseudocode** — start from the whole-program seed §1 + the P7b/P7c deltas above;
   **re-verify every anchor at tip** (sonnet).
2. **Design-goal diffs** at hunk grain (opus); each with a DISCRIMINATING STRUCTURAL exit gate.
3. **Critique adversarially** (opus refuter panel) — **VERIFY EMPIRICALLY**: for a codegen-moving
   cut, an in-tree-then-revert OR throwaway-worktree spike (apply, build, run the FULL suite,
   measure the true golden move + answer-invariance). P7c's spike found exactly one moved golden
   (`negate_1.ir.opt`) with every answer golden byte-identical — the clean template.
4. **IR desired states** (predict-then-verify, STRUCTURAL pins); decide exactly which goldens MOVE.

Model tiering (memory `subagent-model-tiering`): **sonnet** = anchor re-verification, baseline
dumps; **opus** = design diffs, refuter panel, IR desired-states. Keep the orchestrator thin.

## Gotchas (carried)
- clangd diagnostics in this repo are NOISE (no include paths) — trust the real build only.
- macOS bash 3.2 / zsh word-splitting: use `${=var}` when a var holds multiple CLI args.
- `.dr` files are ASCII-ONLY.
- Run builds/suite SILENT on success; the full OptDiff suite takes ~3–4 min — run it BACKGROUNDED
  and await the notification. Never run bench concurrently with the suite.
- `runall.sh --bless <workroot> [filter]` takes NO jobs arg (it PROMOTES from an existing workroot;
  run the suite into that workroot FIRST, with any new `.irgold` step already added). Bless ONLY
  after reviewing the delta; never through a symlink golden; never to make a red case green.
- The `Index::First/Next` full-key-exact contract (`Table.h:789-824`) is the load-bearing fact P7c
  depends on — if it is ever weakened (a hash-only match), P7c is unsound; re-verify it at tip.
