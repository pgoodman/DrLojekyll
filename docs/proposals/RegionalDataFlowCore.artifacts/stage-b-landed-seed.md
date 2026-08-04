# Stage-B-landed seed — the architecture WITH the regional layer + the path forward as diffs (session-4 close, 2026-08-03)

Purpose: ground the NEXT session in a whole-program view at the session-4
close (Stage B LANDED on top of tip 8a4520d9; session-4 deltas uncommitted at
authoring). SEED discipline: single pass, orchestrator-authored — the next
session MUST fleet-re-verify every anchor before building.

ONE-AUTHORITY RULE: the deep pseudocode is `regional-arch-pseudocode.md`
(§1–§7, Part R, **Part B**); the normative Stage-B hunks are `stage-b-diff.md`
**AMENDMENTS (session 4)**; the realized dump bytes are
`regional-dump-stage-b-desired-states.md` **§9 + §9.7**; the Stage-C hunk doc
is `stage-c-diff.md` (pre-dates the DIFF-R1 panel — re-brief needed). This
seed does not duplicate them.

---

## Part 1 — the LANDED pipeline at the Stage-B grain (pseudocode)

```
CompileModule(module):                                # bin/drlojekyll/Main.cpp
  gPassPolicy.bisect_counter = 0
  query  := Query::Build(module, log, policy, demand, demand_retract)
    # ... build + Optimize + demand transform ...
    # tail: FinalizeDepths/ColumnIDs; TrackDifferentialUpdates; TrackConstAfterInit;
    #   BuildEquivalenceSets; Stratify; row_contracts := InferConservativeRowContracts;
    #   ValidateRowContracts; return Query(impl)      # UNTOUCHED by Stage B (ESC-4 (iii))
  frozen := FrozenRegionalProgram::Build(query, log)  # NEW (lib/Regional/Planning.cpp)
    # THE THIRD SLOT (Main.cpp, post-null-check gap). Degenerate planner:
    #   ONE ProgramRoot + ONE observation-root region R0, zero child calls.
    #   Derives (pure, decl-ordered, public API + ONE friend leak for
    #   query.impl->row_contracts):
    #     request-ports  := one per DemandForcing (ascending forcing order)
    #     input-ports    := received real messages (decl order; IsDemandMessage skipped)
    #     result-ports   := published messages
    #     region-internal:= the fabricated demand__ messages (ADJ-2)
    #     permanent-roots:= queries with no forcing (ADJ-3; all-free OR unforced-bound)
    #     row-contracts  := R-STORE-NARROWED: one per distinct non-demand__
    #                       relation-INSERT declaration, first-insert-wins,
    #                       member-key positional from row_contracts, support
    #                       from CanReceiveDeletions  (§9.7 — merge-materialized
    #                       interiors like demand_tc's `path` are UNNAMEABLE:
    #                       QueryMergeImpl carries no decl link; the necessity
    #                       witness for logical-origin provenance)
    #     census         := DeriveRegionalCensus(query)   # PURE PUBLIC-surface twin
    #   FreezeAndValidate: V-FROZEN-NO-OPEN-PORT + V-OWNERSHIP-ACYCLIC
    #   (fprintf+abort scaffolds; Stage C/D extend)
  if gRegionStream:    dump G1 (FrozenRegionalDump)   # -region-out; 4 .region.<mode> pins
  if gRegionDOTStream: dump DOT twin (advisory)       # -region-dot-out; never goldened
  SetRelDumpStream(gRelStream)                        # unchanged
  program := Program::Build(frozen, log, first_id, policy, demand_instance)
    # H4: signature takes frozen; FIRST statement: query = frozen.Query()
    #   (the THIN seam — everything downstream byte-unchanged);
    #   context.frozen_census = &frozen.Census()
    # ... pre-pass fences -> BuildDataModel -> DR-IR -> eager web -> Optimize ...
    # ValidateDROps tail: V-REGION-CENSUS (ALWAYS-ON, corpus-wide): stored
    #   census == DeriveRegionalCensus(query) re-derived fresh — the
    #   positive-presence referee (a stubbed planner ABORTS)
  emit -ir-out / -dot-out / -df-out / -contract-out / codegen   # unchanged
```

Library shape: `lib/Regional` (Planning.cpp, Format.cpp) is an ACYCLIC peer —
`bin/drlojekyll → {ControlFlow, Rel} → Regional → DataFlow`. Public header
`include/drlojekyll/Regional/Regional.h`; `class Query` gained
`friend class FrozenRegionalProgram` (the row_contracts read); ControlFlow/Rel
link Regional. Referee stack: 190×4 bespoke + 63 oracle + 63 monotone + 59
behavioral + 6 eqgate + `.irgold` pins now incl. **16 `.region` goldens**
(4 witnesses × 4 modes: demand_tc_witness, join_1, merge_2,
demand_multi_adorn_witness; all four byte-identical cross-mode at tip but
pinned per-mode).

## Part 2 — the path forward as DIFFS on Part 1

- **DIFF-NEXT-1 (Stage C, the cutover — needs the re-brief).** Replace the
  demand transform's implicit guard-JOIN encoding with the first-class
  request-edge node + lifecycle ops; `frozen` stops being a skeleton and
  carries request edges; the `-region-out` block gains the `request-edges{…}`
  sub-block + 8th census field (H7/H2/H1 desired-states items UNBLOCK here).
  PRECONDITIONS: D2.6 reader-handle schema (STOP), the §6-vs-§11 routing rule
  (STOP), and the DIFF-R1 panel's 15 normative amendments folded into
  stage-c-diff.md (esp. lifecycle-1 mint-then-desugar vs full pipeline
  treatment; oracle-1 positive-presence; necessity-3 additive cut).
- **DIFF-NEXT-2 (DIFF-R3 declared regions, owner-recommended post-B slot).**
  The bracket surface (`rel[K](F)`) turning region inference into checking +
  the adornment-placement fuzzing harness; G1 grammar is generator-friendly
  by design. Independent of D2.6.
- **DIFF-NEXT-3 (the R-STORE lift — TWO TIERS, refined 2026-08-03 post-landing).**
  - **Tier 1, the demand-scoped naming lift (rider-sized, answer-inert):**
    record the demanded relation's `ParsedDeclaration` on the
    `RecognizedSubgraph` at `ApplyDemandTransform` Step 2 (a mint-time
    SNAPSHOT — parse identity is Optimize-stable, nothing to migrate; the
    DIFF-R1 necessity-4 argument); at freeze time resolve the live interior
    model via a Regional-layer walk over the public `GuardAnnotations()`
    (the ResolveLiveRecognition pattern); emit the interior row-contract +
    census arm; re-bless 8 goldens (demand_tc + multi_adorn × 4 modes,
    predicted first: `rel=path` returns, `row-contracts` 1→2). NOT a step
    toward Tier 2 (different mechanism family) — it is the INFERENCE-side
    record DIFF-R3's checking fork (R3-P2) reconciles a declared bracket
    against. DECISION RULE: if DIFF-R3 ranks next, fold Tier 1 in as its
    first hunk; otherwise do it as a standalone rider.
  - **Tier 2, the general mechanism (its own mini-slice under the standing
    method):** origin DECL-SETS on models — stamped at the
    `ConnectInsertsToSelects` erasure site, UNION-ONLY on CSE folds/proxy
    replacements (monotone: a missed union under-names, never miscompiles),
    never in Hash/Equals; ONE slice with the D2.9 proxy-role-inheritance
    question (same erasure-site family). Covers undemanded interiors
    (`tc_nonlinear_diff`'s `tc`); consumers = Stage-C demand-area member
    naming, hoist-vs-nest DeterminedBy, cost attribution, D5 trie order.
    The first sanctioned post-F1 maintained satellite — the panel must
    litigate that explicitly.
- **DIFF-NEXT-4 (record-only wart).** Implicitly-declared zero-arity exports
  are minted without a name spelling → `rel=` renders empty in `-region-out`
  only (e.g. `booleans`, unpinned). Fix = parser mint carries the head-atom
  spelling; harmless until a condition-bearing witness is pinned.

## Part 3 — what the next session verifies before building

1. Fleet re-verify Part B + AMENDMENTS anchors against the then-tip
   (session-4 landed code shifts lines).
2. The owner's next-slice RANKING (Stage-C re-brief vs DIFF-R3) — STOP if
   unranked and load-bearing.
3. Baselines: SUITE: PASS (190) incl. the 16 `.region` pins + V-REGION-CENSUS
   live; ctest 7/7.
