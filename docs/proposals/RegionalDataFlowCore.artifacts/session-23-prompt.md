# Session 23 charter — POST-P6.2: the P6 compile-time first cut is COMPLETE; choose the next cut

You are resuming the **keyed-instance greenfield rewrite** on branch `keyed-instances` (Dr. Lojekyll,
the `hyde` C++ Datalog compiler). **P1 + P2 + P3 + P4 + P5 + P6.1 + P6.2 ARE LANDED** (OptDiff
**SUITE: PASS (226)**, ctest **5/5**). P6.2 was the SECOND and LAST **compile-time** slice of P6, so
**the entire P6 compile-time first cut is now COMPLETE** — the frozen regional program owns all five
compile-time authorities (RegionalFactId, BindingState, DeclaredAccessPath, AccessPlan,
recursive_components) PLUS the routing half (`rules`/`inherited_symbolic_fields`/`SymbolicFieldId`).
The M3 full-materialization backend still EVALUATES; the regional model is still a compile-time OBSERVER.

## Read first (resume authority, in order)
1. **`session-23-whole-program-seed.md`** — THE START-HERE whole-program view: §0 POST-P6.2 status,
   §1 the POST-P6.2 pipeline pseudocode, §2 the authorities + what's now populated vs still dormant,
   §3 the TWO candidate next cuts as diffs + the HEADLINE ranking decision, §4 the tasks. Keep it current.
2. **`p6.2-grounding.md`** — the P6.2 landed record (§1.1 headline evidence, §2 survivor design, §3 the
   5 folded fixes, §4 IR desired-states, §6 exit gate) + the method template.
3. Memory **`regional-dataflow-core-epoch`** (P6.2 landed record at the head) and
   **`greenfield-rewrite-motivation`**.
4. For the RUNTIME candidate: **`keyed-rewrite-reconstruction-diffs.md §3-P6.3..P6.6`** (lines ~579+)
   + **`next-session-prompt.md` Phase 6 steps 3-6** + memory `mobius-differential-dataflow` /
   `free-termination-paper`. For the PHYSICAL candidate: **`keyed-rewrite-p7p9-diffs.md`** +
   **`keyed-rewrite-p7p9-critique.md`** (both deeply critiqued in sessions 14-15).

## The HEADLINE decision to SETTLE with the owner (do NOT presume)
The P6 compile-time first cut is done. The next cut is a genuine FORK — settle the ranking at the
[OWNER STOP]:
- **(A) The RUNTIME cut — P6.3–P6.6.** Begin real recursive EVALUATION in the regional model: P6.3
  fusion (FusedFixpoint vs JointFixpoint from the promoted `SymbolicFieldId` prefix), P6.4
  `DeriveActivationEdge` (the cross-state `RuleActivationEdge`, MAY cycle — the first dormant-half
  activation), P6.5 EvaluateEpoch (two-pass within-epoch DRed on `RegionalFactId` + joint
  rooted-reachability semi-naive worklist), P6.6 RetireUnreachableSCCs (liveness RE-DERIVED, never
  refcounted). This is the LARGE cut toward REPLACING the M3 backend; it TOUCHES codegen/runtime and
  every gate becomes answer-equality-BEARING (the greenfield structural-only posture ends here — the
  regional model starts to EVALUATE). MUST be heavily sub-sliced.
- **(B) The PHYSICAL cut — P7 (then P8/P9).** Keep the compile-time-observer posture one more layer:
  P7 makes `AccessPlan` its own domain with REAL hash/trie paths (where a narrow `@key` first PAYS) —
  `kRetainedIndexScan`/`kFullKeyHashLookup`/trie become COST decisions; the intra-relation prefix seek
  is `GetOrCreateIndex(subset)`. Structurally testable (a trie plan must NOT execute the old whole-table
  rescan) — the greenfield structural-gate discipline still applies. P8 = the cross-relation ordered
  trie / Free Join / COLT (REBUILDS the P5 schema spine region-global over `SymbolicFieldSet`); P9 =
  access-path inference.

The seed §3 lays out both as diffs. Recommend a ranking WITH rationale (the runtime cut is the
"honest end state" but the largest/riskiest; the physical cut keeps the tractable structural-gate
cadence and cashes the P5/@key work into a real perf win) — but the RANKING is the owner's.

## Standing rulings (do not re-litigate)
- **GREENFIELD**: compiler not in use. Through P6.2 every gate is STRUCTURAL, never answer-equality.
  NOTE: the RUNTIME cut (A) is where this CHANGES — real evaluation makes answers observable and
  answer-equality gates legitimate (cross-checked against the M3 backend). Flag this at the fork.
- **The five+one authorities stay separate**: RegionalFactId, BindingState, DeclaredAccessPath (P5),
  AccessPlan (P4, the only codegen-driving one so far), recursive_components (P6.1), SymbolicFieldId
  (P6.2, routing). P7 makes AccessPlan dual-homed; P8 rebuilds the P5 spine region-global — do NOT
  migrate the P5 `schema_table`, REBUILD it.
- Clause-source is the SETTLED routing authority (P6.2 headline — do not revert to DataFlow-view
  recognition; "do not let DataFlow mutation + post-optimization recognition remain the authority").

## Method — the grounding loop as WORKFLOWS (the loop that landed P2–P6.2)
Run **build-pseudocode → design-goal diffs → adversarial EMPIRICAL critique → IR-desired-states** via
WORKFLOWS. Keep the orchestrator thin; several sequential single-phase workflows beat one mega-workflow.
Model tiering (memory `subagent-model-tiering`): sonnet for the mechanical symbol/anchor grep +
baseline carrier dumps + anchor re-verification; opus for design-goal diffs, the adversarial refuter
panel (**VERIFY EMPIRICALLY — compile throwaway carriers, dump the relevant IR**; this caught P6.1's
insert-arm inversion AND P6.2's vacuous-cycle carrier), and IR desired-states. Watch the workflow
`parallel()`/`await` precedence (a `.filter` on an un-awaited parallel bit session 22 — write
`(await parallel(...)).filter(Boolean)`).

## The [OWNER STOP]
This is **DOCS-ONLY until the owner ranks the fork AND green-lights execution.** Present the POST-P6.2
grounding + the (A)-vs-(B) recommendation + the chosen cut's exit gate, then STOP for the go/no-go. If
green-lit, execute as coherent commits with the gate GREEN (OptDiff `SUITE: PASS`, ctest 5/5; for (B)
codegen-honest structural gates; for (A) codegen/runtime changes + answer-equality cross-checks vs M3),
then update CLAUDE.md + the memory topic file + write the session-24 seed/prompt.

## Gotchas (carried)
- clangd diagnostics in this repo are NOISE (no include paths) — trust the real build only.
- macOS bash 3.2 / zsh word-splitting: use `${=var}` when a var holds multiple CLI args.
- `.dr` files are ASCII-ONLY (a non-ASCII em-dash is a lex error).
- Run builds/suite SILENT on success, surface only on failure; the full OptDiff suite takes ~3–4 min —
  run it BACKGROUNDED and await the completion notification. Never run bench concurrently with the suite.
- Copyright on NEW files: `// Copyright <year>, Peter Goodman. All rights reserved.` ONLY.
- Bless goldens ONLY via `runall.sh --bless <workroot>` after reviewing the pure-suffix/expected delta;
  a symlink golden is never written through (bless_copy refuses).
