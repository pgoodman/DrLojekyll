# Session 22 charter — P6.2 (typed edge-local routing + SymbolicFieldId promotion)

You are resuming the **keyed-instance greenfield rewrite** on branch `keyed-instances` (Dr. Lojekyll,
the `hyde` C++ Datalog compiler). Tip **`734712c0`** ("P6.1: query-independent recursive components").
**P1 + P2 + P3 + P4 + P5 + P6.1 ARE LANDED** (OptDiff **SUITE: PASS (224)**, ctest **5/5**). The next
actionable step is **P6.2** — the SECOND and LAST compile-time slice of P6 (populate
`RegionTemplate.rules` / `inherited_symbolic_fields` via a per-rule field-routing projection +
`SymbolicFieldId` promotion). This is a GROUNDING-then-(owner-gated)-EXECUTE session.

## Read first (resume authority, in order)
1. **`docs/proposals/RegionalDataFlowCore.artifacts/session-22-whole-program-seed.md`** — THE
   START-HERE whole-program view: §0 POST-P6.1 status (what's grounded/reserved for P6.2), §1 the
   POST-P6.1 pipeline pseudocode, §2 the four-authority + SymbolicFieldId target, §3 the P6.2 diffs +
   the HEADLINE open decision + open design questions, §4 the tasks. **Keep it current as you work.**
2. **`p6-grounding.md`** — the P6.1 landed record (§6 = the shipped design + the 3-refuter panel
   verdict; the method template) + **§4 the P6.2 deferred sketch**.
3. **`keyed-rewrite-reconstruction-diffs.md §3-P6.1/P6.2`** (lines 540-577) — the
   `RuleRoutingProjection` / `PromoteSharedSymbolicField` formulation + the co-recursion exit gate
   (note: its `ComputeRecursiveComponents-after-rules` ordering is DEAD — P6.1 landed first).
4. **`next-session-prompt.md`** Phase 6 (step 2 = "typed edge-local field projections; promote a
   schema-level symbolic identity only when every producer proves the same mapping") + "Avoid these
   false starts".
5. Memory **`regional-dataflow-core-epoch`** (P6.1 landed record + P6.2-next) and **`greenfield-rewrite-motivation`**.

## Standing rulings (do not re-litigate)
- **GREENFIELD**: compiler not in use → every post-P1 gate is STRUCTURAL, never answer-equality (the
  M3 full-materialization backend answers correctly; the regional model is a compile-time OBSERVER).
- **P6 is a COMPILE-TIME first cut** (owner, session 21): P6.1 + P6.2 populate the recursive-analysis
  + routing halves as compile-time analysis; **codegen UNCHANGED**; the M3 backend still evaluates.
  Real RUNTIME evaluation (P6.3–P6.6) is a LATER, SEPARATELY-gated cut — do NOT start it here.
- Keep the **four (now five) authorities separate**: RegionalFactId, BindingState, DeclaredAccessPath
  (P5), AccessPlan (P4, the only codegen-driving one), recursive_components (P6.1). SymbolicFieldId
  (P6.2) is region-global field identity for ROUTING; the trie spine is P8 (do NOT build it here).
- P6.2 mints **NO RuleActivationEdge** and touches **NONE** of the dormant runtime half
  (`activation_edges`/`AddDerivation`/`RouteResults`) — that is P6.4+.

## The P6.2 HEADLINE open decision to SETTLE with the owner (do NOT presume)
Source the per-rule field-routing projection from **PARSED CLAUSES** (identity-preserving — P6.1
EMPIRICALLY proved the DataFlow graph loses per-relation identity via CSE model-sharing) vs the
**DataFlow JOIN/MAP views** (Optimize-faithful but identity-lossy). The seed recommends clause-based,
built ONLY among the frozen `R.relation_schemas`' decls (a folded interior rule is out of the model by
construction, mirroring P6.1's `rel_ids` filter) — but the grounding loop must CONFIRM this covers the
corpus and does not mis-handle multi-clause relations. Also settle: does P6.2 change codegen (predict
NO, pin it P6.1-style)? Where does the render live + census impact (P6.1 chose NO census count)?

## Method — WORKFLOWS, opus + sonnet (the loop that landed P2–P6.1)
Run the **build-pseudocode → design-goal diffs → adversarial critique → IR-desired-states** loop via
WORKFLOWS. Keep the orchestrator thin; prefer several sequential single-phase workflows over one
mega-workflow. Model tiering (memory `subagent-model-tiering`):
- **sonnet** — mechanical: the P6.2 symbol/anchor grep (RESERVED-EMPTY `rules`/`inherited_symbolic_fields`
  + the `RuleRoutingProjection` stub + `SymbolicFieldId`; the `ParsedClause`/`ParsedPredicate`/
  `ParsedVariable` walk API; the P5 `(RelationId, ordinal)` schema_table; `RelationSchema` shape),
  baseline carrier dumps, anchor re-verification.
- **opus** — judgment: authoring the design-goal diffs at hunk grain, the adversarial refuter panel,
  the IR desired-states.

Concretely, do all four, each producing a persisted artifact (extend `p6-grounding.md` or a new
`p6.2-grounding.md`):
1. **Ground** the whole program at POST-P6.1 (keep the seed current). Enumerate every P6.2 touch/reuse
   site; ground every anchor at tip; SETTLE the headline decision with the owner.
2. **Formulate design-goal diffs** at hunk grain with DISCRIMINATING STRUCTURAL exit gates: an
   all-producers-agree case PROMOTES a shared `SymbolicFieldId`; a co-occurrence-only case does NOT
   (both arms of `PromoteSharedSymbolicField`, F16); the promotion reaches a FIXPOINT (F28);
   `recursive_components` byte-identical under add/remove `#query`. Answer-equality is a LOST CHECK.
3. **Critique adversarially** (opus refuter panel) — **VERIFY EMPIRICALLY by compiling throwaway
   carriers in the scratchpad and dumping `-df`/`-region-out`/`-dot-out`** (this is exactly what caught
   the P6.1 BLOCKING insert-arm inversion; do not skip it). Scrutinize: clause-source soundness under
   CSE/folded interiors; the "every producer agrees" test for multi-clause relations; RuleId
   determinism (HP-9 — no pointer/UniqueId golden key); whether P6.2 leaks into codegen or the dormant
   runtime half; whether negated/aggregate/@product body atoms participate. Rank survivors; record
   refuted diffs as certifications.
4. **Author the desired IR output states** (predict-then-verify, STRUCTURAL pins): the `-region-out`
   `rule`/`routing` render (placement + deterministic order), census impact, the `.rel`/`.h`/`.stdout`
   byte-stability prediction (pin it), and a NEW `@key`-bearing co-recursion carrier (there is none —
   likely build on `key_partial_1`'s `#local … @key(…)` + distinct-`#query` shape). Sonnet pulls
   baseline dumps; opus authors the desired states.

## The [OWNER STOP]
This is **DOCS-ONLY until the owner green-lights P6.2 execution.** Present the grounding + the settled
headline decision + the exit gate, then STOP for the go/no-go. If green-lit (the session-21
"ground-then-execute" precedent), execute as ONE coherent commit with the structural gate GREEN:
OptDiff `SUITE: PASS`, ctest 5/5, codegen byte-stable, the new carrier + region goldens blessed via
`runall.sh --bless` after review, then update CLAUDE.md + the memory topic file + write the session-23
seed/prompt.

## Gotchas (carried)
- clangd diagnostics in this repo are NOISE (no include paths) — trust the real build only.
- macOS bash 3.2 / zsh word-splitting: use `${=var}` when a var holds multiple CLI args (a
  multi-flag mode string passed unquoted becomes ONE arg — bit the P6.1 quick-test).
- `.dr` files are ASCII-only (a non-ASCII em-dash in a comment is a lex error — bit the P6.1 carrier).
- Run builds/suite SILENT on success, surface only on failure (memory `silent-tests-save-tokens`);
  never run the bench harness concurrently with the OptDiff suite.
- The full OptDiff suite takes ~3–4 min — run it BACKGROUNDED (`run_in_background`) and await the
  completion notification; do not block a foreground 2-min timeout on it.
- Copyright on NEW files: `// Copyright <year>, Peter Goodman. All rights reserved.` ONLY (never the
  ToB idiom).
