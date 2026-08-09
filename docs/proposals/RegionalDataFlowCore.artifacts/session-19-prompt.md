# Session 19 prompt — keyed-instance rewrite: P4 (honest complete-path specialization / FullScanFilter) grounding + execution-readiness

Continue in the Dr. Lojekyll repo on branch `keyed-instances`. **Re-check the branch tip and worktree
first.** The tip should be `63573a67` ("P3: RequestEdge / FactDerivation acyclic slice (compile-time
model)"). **P1 + P2 + P3 ARE LANDED** — compile-clean, OptDiff `SUITE: PASS (222 cases)`, ctest 5/5
(the new `RegionInstance` gate). The working tree should be clean.

## Context you must load (the greenfield rewrite state)

- **P3 shipped the request/derivation MODEL.** `include/drlojekyll/Regional/RegionInstance.h` (new
  public leaf) holds all regional typed-id domains + `RegionInstanceRelations` (request edges,
  per-fact derivations, routed results, the edge/derivation ops), stored by value on
  `FrozenRegionalProgram` (`Instances()`). `Planning.cpp`'s `BuildRequestPorts` splits the query loop:
  a bound `#query` → a `RootLease` **request port** (`-region-out` renders `-> request-port P<k>`); an
  all-free `#query` → a `PermanentRoot`; both mint a `RequestEdge`. It is a COMPILE-TIME model layered
  over the retained full-materialization backend — **codegen is UNCHANGED (M3)**. Design + the
  adversarial critique survivors + IR desired-states: `p3-grounding.md`.
- **The P3 model's evaluation half is DORMANT.** `AddDerivation`/`RouteResults` are defined but never
  called for a real program (no rule sweep — `RegionTemplate.rules` RESERVED-EMPTY); `BindingStateId`
  interns to EXACTLY the empty state; a bound `#query` still evaluates via the retained
  `BuildQueryEntryPointImpl` full-materialization cursor. **P4 is the first phase where the model
  DRIVES evaluation** (complete-path, `FullScanFilter`).
- The rewrite is **greenfield delete-then-rebuild**. Every post-P1 gate is STRUCTURAL, never
  answer-equality (the full-materialization baseline already answers correctly with `@key` inert).
  Motivation: memory `greenfield-rewrite-motivation`.
- **P4 does NOT narrow keys.** The conservative row-contract layer keys recursive-SCC views on
  AllFields (sound, but useless for specialization). Narrowing an UNDECLARED key is P9 + the
  Minimize/DeterminedBy provability widening (O-R3.5); a DECLARED narrow key is `@key` (P5). P4 binds
  ALL of a query's bound columns (complete-path) over a full scan.

## Read in this order

1. `RegionalDataFlowCore.artifacts/INDEX.md` (read-order + supersession matrix).
2. **`session-19-whole-program-seed.md`** — START HERE. The POST-P3 whole-program view: §0 status, §1
   the CURRENT pipeline pseudocode grounded in real post-P3 code (§1.3 is the freeze with
   `BuildRequestPorts`; §1.3a is the P3 model as the P4 launch point; §1.4 the retained backend a bound
   query still uses), §2 the four-authority target, §3 the path forward P4–P9 as diffs (P4 is the next
   actionable), §4 what this session must do. **All anchors are byte-current at tip `63573a67`** — §0
   lists the drift the P4 diff carries.
3. `p3-grounding.md` — the P3 record (the model P4 extends; note `AddDerivation` projects a row through
   `RelationSchema.member_key_positions`; `BindingStateId.vals` is empty-only until P4; the §8 critique
   survivors and §9 IR states are the method template).
4. `next-session-prompt.md` — the SEMANTIC authority (four authorities, two edges, ordered access
   paths, the "Avoid these false starts" checklist, the "Target semantic representation" typed records,
   Phase 4).
5. `keyed-rewrite-reconstruction-diffs.md` §3-P4 (lines 417-487, the P4 operational diff) + §5.5 (the
   B3 discriminating-gate correction) and §3-P5/P6 for the sequel. **Re-verify every anchor — they
   predate P1/P2/P3 and have MOVED** (session-19 seed §0 shows: `ProgramTableScanRegion` Program.h:1028
   not 1096-1140; `EmitScan` Database.cpp:2778 not 3334-3396; `IsTableScan` dispatch Database.cpp:1846
   not 1943).
6. `keyed-rewrite-reconstruction-critique.md` (B3/B4/B5, D1) + `keyed-rewrite-p7p9-critique.md` for the
   standing blocking realizations.

## The decision this session opens on

**[OWNER STOP] Is P4 execution green-lit, or stay in grounding?**

- **If grounding (default):** run the grounding loop below on the POST-P3 codebase — deepen toward P4
  execution-readiness. Do NOT edit production code toward P4 without an explicit owner go-ahead.
  Confirm via git you touched only docs.
- **If green-lit:** execute P4 against `reconstruction-diffs §3-P4` + §5.5 — introduce `AccessPlan`
  (its own domain, `kFullScanFilter` the only arm) + `AccessRequirement`, extend the P3 model so
  `EvaluateEpoch` INVOKES `AddDerivation`/`RouteResults` over a `FullScanFilter` for the complete-path
  slice (D1: carry the terminal `BindingStateId` values), and realize the physical read by REUSING
  `ProgramTableScanRegion` (`index=nullopt` → EmitScan's honest scan+filter mold). Land it with the B3
  STRUCTURAL exit gate green (region-cursor `s<id>` vs query-cursor `pos`; 2nd-requester adds a
  RoutedResult with ZERO new fact table; the emitted filter constant equals the threaded bound value;
  OptDiff SUITE: PASS; ctest ≥5/5; codegen-golden movement reviewed and blessed only as an explicit P4
  consequence). Keep it one coherent commit.

## This session's task (the grounding loop — do this whether or not P4 lands)

Run the **build-pseudocode → design-goal diffs → critique → IR-desired-states** loop, keeping
`session-19-whole-program-seed.md` current as the whole-program backbone.

1. **Build out / refresh the whole-program pseudocode** at implementer altitude, weighted toward the
   NEXT step (P4): re-express the key algorithms as pseudocode grounded in the REAL post-P3 code — the
   `ProgramTableScanRegion`/`EmitScan` FullScanFilter mold, `LowerAccessRequirement`, the P4 extension
   of `EvaluateEpoch` (where `AddDerivation`/`RouteResults` first get invoked), and the terminal
   `BindingStateId` (D1). Run the P4 analog of the P1/P2/P3 symbol grep — enumerate every current site
   P4 must reuse or touch (the scan-region handle/impl + its mint sites, `EmitScan`, the
   `IsTableScan` dispatch, the `BuildQueryEntryPointImpl` keyed-scan seam, where the model drives
   evaluation). Ground every anchor; settle the **reuse-existing-seam vs mint-fresh-scan** question the
   seed §3-P4 flags.
2. **Formulate design-goal diffs** on the deepened pseudocode at hunk grain, with DISCRIMINATING
   STRUCTURAL exit gates (region-cursor `s<id>` vs query-cursor `pos`; routed-result-vs-derivation;
   terminal-BindingStateId-value in the emitted filter constant; compile-abort). Answer-equality is a
   LOST CHECK. Keep the four authorities separate (logical fact / residual / logical path / physical
   structure `AccessPlan`) and the two edges distinct.
3. **Critique the diffs adversarially.** Run a refuter panel against the real POST-P3 code + the
   retained `RegionalDataFlowCore.md` invariants + the "Avoid these false starts" checklist. Special
   scrutiny: is `FullScanFilter` always a correct realization (answer identity)? Does the B3 gate
   DISCRIMINATE a real P4 from the post-P1 baseline cursor (which already emits a NumRows scan + key
   filter)? Does D1 land the terminal `BindingStateId` values without dragging P5's partial-binding DAG
   in? Does P4 keep `AccessPlan` a distinct authority (never a logical path or a fact id)? VERIFY
   findings against the codebase — do not merely assert. Rank survivors; record refuted diffs as
   certifications.
4. **Author/extend the desired IR output states.** Predict-then-verify the target IR dumps with
   STRUCTURAL pins only: does `-region-out` change (a plan annotation on the request port)? the `.rel`
   `kAccessKeyedRelation` op + `plan=FullScanFilter`; the generated `datalog.h` keyed-scan mold (the
   region-cursor `s<id>` shape vs the query-cursor `pos`); any `RegionInstance` unit-test extension.
   Sonnet pulls the current carrier dumps as the baseline (read-only compiles into a scratch dir); opus
   authors the desired states.

## How to do the work (workflows + model tiering)

Use the **Workflow tool** for the fan-out/critique structure — the owner has opted into multi-agent
orchestration for this grounding. Several sequential single-phase workflows, staying in the loop
between them, beat one mega-workflow. Suggested shape (adapt freely):

- **Ground + inventory (fan-out, mostly sonnet):** one reader runs the P4 layer-site enumeration
  (scan-region handle/impl + mint sites / `EmitScan` / `IsTableScan` dispatch / the
  `BuildQueryEntryPointImpl` seam); one reader per P4 sub-area (the `AccessPlan`/`AccessRequirement`
  types; the `EvaluateEpoch`-invokes-`AddDerivation` extension; `LowerAccessRequirement` +
  `ProgramTableScanRegion` reuse; the D1 terminal-`BindingStateId` interner). Each returns first-cut
  operational pseudocode + a drift table (byte-current at `63573a67` — re-verify before trusting).
  Sonnet for mechanical census/extraction; opus for genuine judgment readers.
- **Diff + amend (opus, effort high):** author the P4 diffs + the compile-clean inventory into the
  authority docs in place (a `p4-grounding.md`, sibling of `p3-grounding.md`). Judgment work — opus.
- **Critique (adversarial panel, opus, effort high):** per-area refuters + a cross-phase completeness/
  invariants critic, each prompted to BREAK the diffs against real code + the retained invariants +
  the false-starts checklist; verify, don't assert; rank.
- **IR desired-states (opus author, sonnet carrier-dump extraction):** sonnet pulls current
  `-region-out`/`.rel`/`datalog.h` dumps for a bound-query carrier (`booleans`, `force`,
  `average_weight`) as the diff baseline; opus authors the desired FullScanFilter states.

Keep the orchestrator thin: subagents return distilled pseudocode/findings, not raw file dumps.
Front-load cheap artifacts (dump the current carrier IRs once into scratch; embed paths in agent
prompts). Land results as updates to the existing docs + a new `p4-grounding.md`; keep
`session-19-whole-program-seed.md` current as the whole-program backbone.

## Guardrails

- Re-verify every cited anchor before trusting it. Treat the `session-19-whole-program-seed.md` §1 line
  numbers as current-at-`63573a67` and re-grep any older doc's anchor (the P4 diff's anchors moved at
  P1/P2/P3 — Program.h and Database.cpp especially).
- Do NOT edit production code toward P4 without an explicit owner go-ahead. If green-lit, use a
  symbol/shape-driven acceptance check (the layer-site inventory + the B3 discriminating gate), not a
  review read; keep the region goldens + the `RegionInstance` ctest as the anti-stub belt.
- P4 MAY move codegen goldens (it changes how a bound query lowers). Any golden movement must be an
  explicit, reviewed consequence of a green-lit P4 cut — never blessed to make a red case green, never
  automatically on failure. If you compile carriers for baseline dumps, that is read-only; the suite
  must stay `SUITE: PASS` in grounding.
- Prefer silent-on-success builds/tests (capture to file, surface only on failure); tokens matter.
- Update memory (`regional-dataflow-core-epoch.md`, `MEMORY.md`) at session close with what landed and
  the next ranked step.
