# Session 23 charter — P7: physical access planning (make a narrow @key SEEK)

You are resuming the **keyed-instance greenfield rewrite** on branch `keyed-instances` (Dr. Lojekyll,
the `hyde` C++ Datalog compiler). **P1 + P2 + P3 + P4 + P5 + P6.1 + P6.2 ARE LANDED** (OptDiff
**SUITE: PASS (226)**, ctest **5/5**); the P6 **compile-time first cut is COMPLETE**. The M3
full-materialization backend still EVALUATES; the frozen Regional layer is a compile-time OBSERVER, and
the ONLY authority it uses to steer codegen so far is P4's `AccessPlan`.

**The ranked next cut is P7 — physical access planning** (seed §3 ranks P7 #1 over the runtime cut
P6.3–P6.6; the clincher: P7 is closer to a prerequisite than an alternative — P6.3 fusion's payoff is a
physical arrangement P7 builds). P7 makes a bound `#query` over a narrow `@key` lower to an index **SEEK**
(`Index::First/Next`) instead of the full-scan-filter cursor P4 emits today — the FIRST codegen quality
win from the accumulated P5/@key + P6.2 routing model, and the close of the `keyed-instances` namesake arc.

## Read first (resume authority, in order)
1. **`p7-grounding-seed.md`** — THE START-HERE, current-code-grounded whole-program view (written
   POST-P6.2, every anchor read at tip): §0 status, §1 the as-is pseudocode (freeze SELECT →
   `SelectAccessPlan` stub → codegen READ → `EmitQueryFriends`/`EmitScan` arms → runtime
   `Table`/`Index`/`GetOrCreateIndex`), **§2 the P4-substrate-vs-s15-design RECONCILIATION table** (the
   load-bearing section: DONE-by-P4 / STALE-anchor / STILL-TODO — P7 is "finish what P4 stubbed," NOT
   "introduce AccessPlan"), **§3 the six P7 hunk-grain diffs** (D1 seek enumerator, D2 real dispatch, D3
   CodegenPlanCapabilities/D4-invariant, D4 thread plan_kind at every mint, D5 emit the seek, D6 move
   V-PLAN-HONEST), §4 IR desired-states on `key_partial_1`, §5 the seven open questions.
2. **`keyed-rewrite-p7p9-diffs.md` §1 (P7)** + **`keyed-rewrite-p7p9-critique.md`** (the s14–15 design +
   the 3 blocking corrections B-P7/B-P8/B-P9 + the "P8a dissolved" re-critique) — **PARTLY STALE**: they
   predate P4/P5/P6; trust the seed §2 reconciliation over their raw anchors.
3. **`p4-grounding.md`** (the AccessPlan substrate P7 extends) + memory `regional-dataflow-core-epoch`
   (P6.2 landed record at the head) + `greenfield-rewrite-motivation`.

## The named gap (what P7 closes, one sentence)
`SelectAccessPlan` (`RegionInstance.h:270`) is a near-stub returning only `kFullScanFilter` /
`kFullKeyHashLookup` on `has_free` alone; the real hash partial-key SEEK arm —
`GetOrCreateIndex(sorted(bound-subset)) → Index::First/Next` (the runtime already has both,
`Data.cpp:348` order-free, `Table.h:804`) — is neither SELECTED at freeze nor EMITTED at codegen, and
`plan_kind` is threaded at NEITHER `ProgramTableScanRegion` mint site (0/2). P7 closes exactly that gap:
a real dispatch on the bound subset (no cost model, fallback always-legal), the seek emitted by reusing
the existing `via_index` First/Next cursor (**new codegen surface: NONE**), `plan_kind` threaded at every
mint (B-P7), and V-PLAN-HONEST generalized to a per-kind implication belt at the emission site.

## Standing rulings (do not re-litigate)
- **GREENFIELD / structural gate.** P7 is codegen-HONEST and ANSWER-INVARIANT vs M3 (the
  full-materialization baseline answers a complete read correctly — an index seek yields exactly the
  bound-key rows the full scan filters to; `Index::First/Next` is FULL-KEY EXACT, `Table.h:791`). Every
  P7 gate is STRUCTURAL: a seek plan must NOT emit the whole-table rescan; codegen byte-stable for
  programs with no narrow key. NEVER answer-equality.
- **The authorities stay separate.** `AccessPlan` (P4, physical) is the only codegen-driving one; P7
  extends it. `DeclaredAccessPath` (P5, logical) is a HINT — the seed §5-Q1 is exactly how much P7
  bridges the deliberate P5↔AccessPlan firewall (`Regional.h:92`).
- **P8 (ordered TRIE) and P9 (path INFERENCE) are OUT of P7 scope** — the runtime is all-hash; do not
  add a trie, an ordered index, or an inference pass. The intra-relation prefix seek IS P7
  `GetOrCreateIndex(subset)` (the s15 "P8a dissolved" ruling).
- No cost model in P7: the seek is preferred whenever an index exists/can be minted; the full-scan
  fallback is always legal.

## The [OWNER STOP] — confirm the cut, then DOCS-ONLY grounding until execution is green-lit
The seed ranks P7 #1. First CONFIRM with the owner (ratify P7, or override to the runtime cut / a
compile-time-only P6.3 fusion-detection spike — seed §3 flip condition). Then run the grounding loop
below **DOCS-ONLY**; present the design + exit gate and STOP for the execution go/no-go. If green-lit,
execute as one coherent commit with the structural gate GREEN (OptDiff `SUITE: PASS`, ctest 5/5;
`key_partial_1` seek carrier + region golden re-blessed; codegen byte-stable for every non-seek program),
then update CLAUDE.md + the memory topic file + write the session-24 seed/prompt.

## Method — the grounding loop, via WORKFLOWS, opus + sonnet (the loop that landed P2–P6.2)
Run all four, each producing a persisted artifact (extend the seed or a new `p7-execution-grounding.md`),
via WORKFLOWS (keep the orchestrator thin; several sequential single-phase workflows beat one mega-one;
watch the `(await parallel(...)).filter(...)` precedence — an un-awaited `.filter` bit two prior sessions):
1. **Build the pseudocode of the algorithms + architecture.** Start from seed §1 (the grounded as-is) and
   re-verify every anchor at tip (sonnet: the `SelectAccessPlan`/`ComputeQueryAccessPlan`/`PlanFor` freeze
   path, the `BuildQueryEntryPointImpl`/`EmitQueryFriends`/`EmitScan` codegen reads, the two
   `ProgramTableScanRegion` mints, the `Table`/`Index`/`GetOrCreateIndex` runtime). Keep the seed current.
2. **Formulate design-goal diffs ON that pseudocode** at hunk grain (opus). Start from seed §3's six
   diffs; settle the seed §5 open questions (seek-key provenance P5-path-vs-raw-subset; the
   `kRetainedIndexScan` name overload → `kUnplanned`+`kPartialKeyHashSeek`; the recursive-owned fence;
   caps object vs implicit; where V-PLAN-HONEST lives + the D4-before-D6 sequencing; #query-path-only vs
   interior scans). Each diff carries a DISCRIMINATING STRUCTURAL exit gate.
3. **Critique the diffs adversarially** (opus refuter panel) — **VERIFY EMPIRICALLY by compiling
   throwaway carriers and dumping `-region-out`/`.h`/`.cpp`** (this caught P6.1's insert-arm inversion and
   P6.2's vacuous-cycle carrier — do not skip it). Scrutinize: does the seek actually change the emitted
   cursor to `First/Next` (not a withheld-index full scan)? is it answer-invariant vs M3 on a probe? does
   `plan_kind` ∉ Hash/Equals keep CSE bytes stable? does moving V-PLAN-HONEST before threading plan_kind
   abort the corpus (the B-P7 sequencing)? does a recursive-owned relation's seek interact with the
   fixpoint indexes? Rank survivors; record refuted diffs as certifications.
4. **Author the desired IR output states** (predict-then-verify, STRUCTURAL pins) — start from seed §4:
   the `key_partial_1` `-region-out` `plan=partial-key-hash-seek` flip; the generated cursor
   `First/Next` shape; exactly which goldens MOVE (region + the FIRST codegen golden since P4 — decide
   the seed §5-Q7 golden surface: extend `key_partial_1.irgold` with h/ir steps or add a sibling carrier)
   vs stay byte-identical; the `.stdout` answer-invariance pin. Sonnet pulls baseline dumps; opus authors
   the desired states.
Model tiering (memory `subagent-model-tiering`): **sonnet** = the mechanical anchor re-verification,
baseline carrier dumps, symbol/line grounding; **opus** = the design-goal diffs, the adversarial refuter
panel, the IR desired-states.

## Gotchas (carried)
- clangd diagnostics in this repo are NOISE (no include paths) — trust the real build only.
- macOS bash 3.2 / zsh word-splitting: use `${=var}` when a var holds multiple CLI args.
- `.dr` files are ASCII-ONLY (a non-ASCII em-dash is a lex error).
- Run builds/suite SILENT on success, surface only on failure; the full OptDiff suite takes ~3–4 min —
  run it BACKGROUNDED and await the completion notification. Never run bench concurrently with the suite.
- Manual codegen check (verify the seek cursor): `drlojekyll key_partial_1.dr -cpp-out gen/` then inspect
  `gen/datalog.h` for the `reachable_from_bf` factory — it must emit `<idx>.First/.Next`, not the
  `while (pos < …NumRows())` rescan.
- Copyright on NEW files: `// Copyright <year>, Peter Goodman. All rights reserved.` ONLY.
- Bless goldens ONLY via `runall.sh --bless <workroot>` after reviewing the delta (a symlink golden is
  never written through). P7 is the first cut since P4 to move a CODEGEN golden — review that delta
  especially (it is a real cursor-shape change on the seek carrier, not a pure suffix).
