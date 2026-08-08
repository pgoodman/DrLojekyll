# Session 15 prompt — keyed-instance rewrite: absorb the P7–P9 blocking corrections, deepen the physical residues, re-critique, IR desired-states

Continue in the Dr. Lojekyll repo on branch `keyed-instances`. **Re-check the branch tip and worktree
first** — the s11 Phase-0 parser witnesses (`lib/Parse/Parser.cpp`, `lib/Parse/Query.cpp`,
`tests/OptDiff/rejects/reject_key_redecl_1.dr` + `reject_key_on_query_1.dr`) may still be uncommitted, and
the session-14 grounding docs were added (docs only — no production code was touched; suite was
`SUITE: PASS (251 cases)` by construction; nothing blessed). **Re-derive every cited anchor against the
current tip before trusting it — anchors drift.**

## Where things stand (session 14 close)

Session 14 was a GROUNDING round (design/critique, executed with workflows + opus/sonnet tiering). It (1)
RE-VERIFIED the B1–B5 P1 compile gate whole-tree and found it **NOT compile-clean** — 7 more un-enumerated
consumers; (2) deepened the P7–P9 physical layer into hunk-grained diffs; (3) critiqued them (3 blocking + 5
high + 18 certifications); (4) authored their desired IR states. Read in this order:

1. `RegionalDataFlowCore.artifacts/INDEX.md` (read-order + supersession matrix; the session-14 block).
2. `next-session-prompt.md` — the SEMANTIC authority: `@key` is a relation-local, ORDERED access-path
   declaration, NOT a query adornment / demand opt-in. Its "Avoid these false starts" list is a checklist.
3. **`session-15-whole-program-seed.md`** — START HERE: the CONSOLIDATED whole-program pseudocode. Unlike
   the s13/s14 seeds, its **§1 now includes the PHYSICAL layer** (ControlFlow lowering → CodeGen `EmitScan`
   three-arm dispatch → `DataIndex`/`GetOrCreateIndex` → runtime `Table`/`Index`), which is exactly the layer
   P7–P9 build on. §2 = the four-authority target. §3 = the path forward P0–P9 with the s14 corrections
   folded into the actual diffs. §4 = what this session must do.
4. `keyed-rewrite-p7p9-diffs.md` — the P7–P9 operational diffs; **§7 is the s14 critique amendments** to
   absorb into §1–§3 this session.
5. `keyed-rewrite-p7p9-critique.md` — the s14 refuter panel (15 survivors: 3 blocking, 5 high, 5 medium, 2
   low; + 18 certifications). The certifications are load-bearing — they name mechanisms already proven
   against real code (don't re-litigate them).
6. `keyed-rewrite-reconstruction-diffs.md` §6/§6.1 — the P1 gate re-verification (the 7 new consumers + the
   B2–B5 consistency certification) and the symbol-driven `git grep -l` acceptance-gate recommendation.
7. `keyed-rewrite-ir-desired-states.md` §9–§12 — the P7–P9 desired IR states to refine.

**Key facts to carry:**
- **The P7 headline:** `AccessPlan` is its own Rel domain; `kFullScanFilter`≡`index=nullopt` and
  `kFullKeyHashLookup`≡`index=Some` BOTH lower to the same `ProgramTableScanRegion` (EmitScan's three-arm
  dispatch is keyed on `region.Index()`), so at P7 the plan→region map STOPS being injective — the D4
  Option-1→Option-2 transition. `V-PLAN-HONEST` moves to the `EmitScan` emission site.
- **The 3 P7–P9 BLOCKING corrections (folded as p7p9-diffs §7; absorb them into §1–§3 this session):**
  (B-P7) thread `plan_kind` at EVERY `ProgramTableScanRegion` mint or use a `kUnplanned` sentinel the belt
  skips — else it aborts the pre-existing join scans; (B-P8) intern trie nodes on `BindingStateId` =
  (schema, sorted values) — value-partitioned AND convergent, else `[A,B]`/`[B,A]` don't converge; (B-P9)
  re-source the inferred-path ORDER from a DataFlow signal (`out_to_in` / decl order), NEVER the file-static
  ControlFlow `SortedPredecessors` (a layer + authority violation).
- **The P9 pre-vs-post-Optimize hazard (blocking prereq):** `ApplyDemandTransform` runs BEFORE `Optimize`
  (Build.cpp:2601 vs :2622), and `Optimize` DELETES the forwarding TUPLEs / single-source MERGEs the P9 lift
  walk descends. Decide pre-Optimize inference vs a post-Optimize-invariant rewrite before deepening §3.2.
- **The physical layer is ALL HASH — no trie/sorted structure exists** (grep-confirmed). Whole-key
  `First`/`Next` REUSES `Index` verbatim; the `.Range` partial-prefix seek is the ONE genuinely-new codegen
  surface. `GetOrCreateIndex`'s `SortAndUnique` (Data.cpp:350) is the order-free hook a trie must change.
- **Discriminating exit gates only:** answer-equality is a LOST CHECK for the whole physical layer — the P1
  full-materialization baseline AND the honest `FullScanFilter` both answer correctly, so a no-op P7/P8/P9
  passes any answer test. Every pin must be structural / cursor-shape / census-token / node-count /
  provenance-survival.

**Phase 1 remains OWNER-GATED and destructive.** Do NOT begin it without an explicit go-ahead. **P0-item-4**
(the parser order-significance flip at Parser.cpp:974-995 + 1477-1488) is the ONE production change
independent of the P1 cut — flag it as a landable-ahead option; do not act on it unprompted.

## This session's task

Continue the **build-pseudocode → diff → critique → IR-desired-states** loop, now weighted toward absorbing
the P7–P9 blocking corrections and deepening the two least-grounded physical residues.

1. **Build out / refresh the whole-program pseudocode of the architecture and algorithms.** Keep
   `session-15-whole-program-seed.md` current: re-verify its §1 anchors (esp. the newly-added physical layer)
   at tip; deepen §3 P7–P9 by absorbing the 3 blocking corrections (B-P7 `plan_kind`/`kUnplanned` seam, B-P8
   `BindingStateId` trie-node interning, B-P9 DataFlow-sourced order) into the operational diffs IN PLACE, and
   grow the two thin residues into full operational algorithms grounded in real codegen: (a) the `.Range`
   partial-prefix codegen contract (an `EmitAccessPlan` branch + a runtime seek+subtree-enumerate contract
   over the `DiffTable`, grounded in `EmitScan` Database.cpp:3334-3426 and `Index` Table.h:748-888); (b) the
   `InducedOrdering`↔P6-joint-fixpoint structural belt (tie the chosen order to the P6.2 SymbolicField
   classes). Show how each re-provides capability and how the four authorities stay separate.

2. **Formulate the design-goal diffs** on the deepened pseudocode at hunk grain with DISCRIMINATING exit
   gates (answer-equality is a lost check): the honest non-scan emission with `V-PLAN-HONEST` at the emission
   site (goal 5); the lazy shared trie physically realizing the P5 partial-binding DAG (goals 2+6); inference
   as an additive LOGICAL-path source that never overrides a declared path (goal 1). Keep `RequestEdge` vs
   `RuleActivationEdge` distinct (goal 3) and co-recursive key flow sound (goals 4+7).

3. **Critique the diffs adversarially.** Re-run a refuter panel against the real code and the retained
   `RegionalDataFlowCore.md` invariants (member identity, exact request ownership, caller-qualified results,
   effect/epoch invariants, drain-before-retire) and the "Avoid these false starts" checklist. VERIFY findings
   against the codebase — do not merely assert them. Rank survivors; record refuted diffs (and any refuter
   claims that don't survive verification) as certifications. Pay special attention to whether the absorbed
   B-P7/B-P8/B-P9 fixes introduce NEW seams (e.g. does `kUnplanned` interact with region CSE/`MergeEqual`;
   does `BindingStateId` interning survive compaction; does the DataFlow-sourced P9 order actually exist on
   the frozen graph).

4. **Author/extend the desired IR output states** for P7–P9 in `keyed-rewrite-ir-desired-states.md`: the
   `.rel` `AccessPlan` render, the generated `.Range()`/`.First()` C++ vs the honest FullScanFilter scan, the
   trie `-region-out` block (reuse the P5 binding-schema surface), the `kInferred` `-contract-out` lines
   (reconcile with CLAUDE.md's declared-key format; the split render MOVES DataFlow→Regional). Predict the
   target bytes/shape, mark what each phase changes, and add STRUCTURAL pins (a keyed non-scan read must be
   distinguishable from the honest scan by cursor-shape / row-visit-count / node-count — not answer-equality).

## How to do the work (workflows + model tiering)

Use the **Workflow tool** for the fan-out/critique structure — the owner has opted into multi-agent
orchestration for this grounding. Several sequential single-phase workflows, staying in the loop between them,
beats one mega-workflow. Suggested shape (adapt freely):

- **Deepen + anchor re-verify (fan-out, sonnet):** one reader per residue (the `.Range`/EmitAccessPlan codegen
  contract; the `InducedOrdering`↔P6 belt) + one reader per blocking correction (the `plan_kind` mint-site
  census; the `BindingStateId` trie-node + compaction interaction; the DataFlow-visible order signal on the
  FROZEN graph — settle pre-vs-post-Optimize). Each re-verifies anchors at tip, extracts the current code it
  builds on, and returns first-cut operational pseudocode. Sonnet is right for the mechanical census/extraction.
- **Diff + amend (opus, effort high):** absorb the blocking corrections into `keyed-rewrite-p7p9-diffs.md`
  §1–§3 in place; author the two deepened residue diffs. Judgment work — opus.
- **Critique (adversarial panel, opus, effort high):** per-phase refuters + a cross-phase completeness critic,
  each prompted to BREAK the diffs against real code + retained invariants; verify, don't assert; rank.
- **IR desired-states (opus author, sonnet carrier-dump extraction):** sonnet pulls the current carrier dumps
  as the diff baseline (read-only compiles into a scratch dir); opus authors the desired states.

Keep the orchestrator thin: subagents return distilled pseudocode/findings, not raw file dumps. Front-load
cheap artifacts (dump the current carrier IRs once into scratch; embed paths in agent prompts). Land results as
updates to the existing docs; keep `session-15-whole-program-seed.md` current as the whole-program backbone.

## Guardrails

- Re-derive every cited anchor against the current tip before trusting it; anchors drift (the seed §1 physical
  layer + the diffs' file:line are starting points, not gospel — re-verify).
- Do NOT edit production code toward Phase 1 without an explicit owner go-ahead. The destructive demand
  deletion is owner-gated. **P0-item-4** (the parser order-significant-paths flip at Parser.cpp:974-995 +
  1477-1488) COULD land ahead of P1 as the P5 prerequisite — flag it as a landable option, don't act on it
  unprompted.
- Do not bless any golden. If you compile carriers for baseline dumps, that is read-only; the suite must stay
  `SUITE: PASS`. Confirm via git that you touched only docs (production untouched ⇒ the 251 PASS baseline holds
  by construction).
- Prefer silent-on-success builds/tests (capture to file, surface only on failure); tokens matter.
- Update memory (`regional-dataflow-core-epoch.md`, `MEMORY.md`) at session close with what landed and the
  next ranked step.
