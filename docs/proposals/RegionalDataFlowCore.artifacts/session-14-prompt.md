# Session 14 prompt — keyed-instance rewrite: verify the P1 gate, deepen P7–P9, re-critique, IR desired-states

Continue in the Dr. Lojekyll repo on branch `keyed-instances`. **Re-check the branch tip and
worktree first** — the s11 Phase-0 parser witnesses (`lib/Parse/Parser.cpp`,
`lib/Parse/Query.cpp`, `tests/OptDiff/rejects/reject_key_redecl_1.dr` +
`reject_key_on_query_1.dr`) may still be uncommitted, and the session-13 grounding docs were
added (docs only — no production code was touched; suite was `SUITE: PASS (251 cases)`; nothing
blessed). Re-derive every cited anchor against the current tip before trusting it — anchors drift.

## Where things stand (session 13 close)

Session 13 was a GROUNDING round (design/critique, executed with workflows + opus/sonnet
tiering). Read in this order:

1. `RegionalDataFlowCore.artifacts/INDEX.md` (read-order + supersession matrix).
2. `next-session-prompt.md` — the SEMANTIC authority: `@key` is a relation-local, ORDERED
   access-path declaration, NOT a query adornment / demand opt-in. A 10-phase greenfield
   rewrite (P1 DELETES the demand machinery). Its "Avoid these false starts" list is a checklist.
3. **`session-14-whole-program-seed.md`** — START HERE: the CONSOLIDATED whole-program
   pseudocode (§1 current pipeline, §2 target four-authority model, §3 the path forward as
   P1–P9 diffs with the s13 corrections folded), plus §4 "what session 14 must do".
4. `keyed-rewrite-reconstruction-diffs.md` — the actionable authority for P1 + P2–P6:
   §0.1 re-verified anchor drift table, §1 compile-clean P1 inventory, §2 D1–D4 resolutions,
   §3 deepened P2–P6 operational diffs, §4 design-goal diffs, §5 the s13 re-critique amendments.
5. `keyed-rewrite-reconstruction-critique.md` — the s13 re-critique (23 survivors: 5 blocking
   B1–B5, 6 high, 8 medium, 4 low; + 20 certifications). The certifications are load-bearing —
   they tell you which mechanisms are already proven against real code (don't re-litigate them).
6. `keyed-rewrite-ir-desired-states.md` — predict-then-verify IR states; §6/§7 are the two
   no-baseline carriers (P6 co-recursive, P5 convergence), each grounded by its VERIFIED CURRENT
   REJECT; §8 has the P4-emission revision.

**Key facts to carry:**
- **D1–D4 are RESOLVED** (reconstruction-diffs §2): D1 pull the minimal terminal
  BindingStateSCHEMA interner (not the DAG) into P4; D2 P6.1 is the sole `recursive_components`
  populator (P2 leaves it empty); D3 per-fact DRed + rooted worklist; D4 label==emission by
  construction via `ProgramTableScanRegion` reuse (codegen is label-blind).
- **Five BLOCKING findings gate the owner's P1 green-light** (all folded, reconstruction-diffs
  §5): B1 the atomic P1 cut STILL won't compile (injector + census consumers of
  `QueryDemandForcing`/`DemandForcings()` + `IsCutSuccessorDR`); B2 P3 must root permanent-root
  programs; B3 P4's exit gate is non-discriminating vs the P1 baseline; B4 DRed is a per-FACT
  rebuild not a per-BindingState `C_nr`/`C_r` mirror; B5 cross-component transitive retraction
  needs a joint fixpoint. H6 makes P0-item-4 (the parser order-free→order-significant flip at
  Parser.cpp:974-995 + 1477-1488) a HARD prerequisite of P5.
- **P7–P9 (physical access planning / lazy tries / path inference) are the LEAST-grounded
  phases** — they were not deepened at s13.

**Phase 1 remains OWNER-GATED and destructive.** Do NOT begin it without an explicit go-ahead.
This session is grounding, not implementation.

## This session's task

Continue the deepen → diff → critique → IR-desired-states loop, now weighted toward (a) proving
the P1 green-light gate is real and (b) the P7–P9 physical layer.

1. **Build out / refresh the whole-program pseudocode of the architecture and algorithms.**
   Keep `session-14-whole-program-seed.md` current: re-verify its §1 anchors at tip and deepen
   §3 P7–P9 (the thinnest phases) into OPERATIONAL algorithms at implementer grain, grounded in
   the real codegen they build on (the `EmitScan` index/probe/scan arms at
   `Database.cpp:3374-3418`, the `DataIndex`/`ProgramTableScanRegion` machinery, the trie/COLT
   territory). Show how each re-provides capability and how the four authorities stay separate.

2. **Verify the P1 green-light gate (B1–B5) is compile-clean and sound at hunk grain.** Re-grep
   the WHOLE tree (lib/ include/ bin/ tests/) for every P1-deleted symbol and diff the hit set
   against reconstruction-diffs §1 + §5.1 — a fresh refuter should try to find one more
   un-enumerated consumer. Confirm B2/B3/B4/B5 are internally consistent with the seed §2/§3
   algebra and the retained invariants. This is what the owner needs before green-lighting P1.

3. **Formulate the design-goal diffs** on the deepened P7–P9 pseudocode at hunk grain with
   DISCRIMINATING exit gates (answer-equality is a lost check — see the s13 critique): honest
   non-scan emission with V-PLAN-HONEST at the emission site (goal 5, D4 Option-1/2); the lazy
   shared trie physically realizing the P5 partial-binding DAG (goals 2+6); inference as an
   additive LOGICAL-path source that never overrides a declared path (goal 1). Keep RequestEdge
   vs RuleActivationEdge distinct (goal 3) and co-recursive key flow sound (goals 4+7).

4. **Critique the diffs adversarially.** Re-run a refuter panel against the real code and the
   retained `RegionalDataFlowCore.md` invariants (member identity, exact request ownership,
   caller-qualified results, effect/epoch invariants, drain-before-retire) and the "Avoid these
   false starts" checklist. VERIFY findings against the codebase — do not merely assert them.
   Rank survivors; record refuted diffs (and any refuter claims that don't survive verification)
   as certifications.

5. **Author the desired IR output states** for P7–P9, extending
   `keyed-rewrite-ir-desired-states.md`: the `.rel` AccessPlan render, the generated
   `.Find()`/`.Range()` C++ vs the honest FullScanFilter scan, the trie `-region-out` block, the
   kInferred `-contract-out` lines (reconcile with CLAUDE.md's declared-key format). Predict
   the target bytes/shape, mark what each phase changes, and add STRUCTURAL pins (the `.rel`
   census multiset remains the strongest anti-silent-pass belt; a keyed non-scan read must be
   distinguishable from the honest scan — a row-visit-count / loop-shape pin, not answer-equality).

## How to do the work (workflows + model tiering)

Use the **Workflow tool** for the fan-out/critique structure — the owner has opted into
multi-agent orchestration for this grounding. Several sequential single-phase workflows, staying
in the loop between them, beats one mega-workflow. Suggested shape (adapt freely):

- **Deepen + anchor re-verify (fan-out, sonnet):** one reader per subsystem (P7 access planning
  / EmitScan; P8 trie; P9 inference) + one B1–B5 compile-gate verifier that re-greps every
  deleted symbol. Each re-verifies anchors at tip, extracts the current code it builds on, and
  returns first-cut operational pseudocode. Sonnet is right for the mechanical census/extraction.
- **Diff + amend (opus, effort high):** author the deepened P7–P9 diffs + fold the B1–B5
  verification results into reconstruction-diffs §5. Judgment work — opus.
- **Critique (adversarial panel, opus, effort high):** per-phase refuters + a cross-phase
  completeness critic, each prompted to BREAK the diffs against real code + retained invariants;
  verify, don't assert; rank.
- **IR desired-states (opus author, sonnet carrier-dump extraction):** sonnet pulls the current
  carrier dumps as the diff baseline (read-only compiles into a scratch dir); opus authors the
  desired states.

Keep the orchestrator thin: subagents return distilled pseudocode/findings, not raw file dumps.
Front-load cheap artifacts (dump the current carrier IRs once into scratch; embed paths in agent
prompts). Land results as updates to the existing docs; keep `session-14-whole-program-seed.md`
current as the whole-program backbone.

## Guardrails

- Re-derive every cited anchor against the current tip before trusting it; anchors drift (the
  s13 tables in `reconstruction-diffs.md` §0.1 and `current-pseudocode.md` §1.0 are starting
  points, not gospel — re-verify).
- Do NOT edit production code toward Phase 1 without an explicit owner go-ahead. The destructive
  demand deletion is owner-gated. P0-item-4 (the parser order-significant-paths flip at
  Parser.cpp:974-995 + 1477-1488) COULD land ahead of P1 as the P5 prerequisite — flag it as a
  landable option, don't act on it unprompted.
- Do not bless any golden. If you compile carriers for baseline dumps, that is read-only; the
  suite must stay `SUITE: PASS`. Confirm via git that you touched only docs (production untouched
  ⇒ the 251 PASS baseline holds by construction).
- Prefer silent-on-success builds/tests (capture to file, surface only on failure); tokens matter.
- Update memory (`regional-dataflow-core-epoch.md`, `MEMORY.md`) at session close with what
  landed and the next ranked step.
