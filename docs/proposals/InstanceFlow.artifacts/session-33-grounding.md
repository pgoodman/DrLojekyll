<!-- Copyright 2026, Peter Goodman. All rights reserved. -->
# Session-33 grounding — adjudicated (CP2 render + Phase-D line-of-sight)

Branch `keyed-instances`, tip `1db62f80`. Method: 2 sonnet scouts (current-tip
anchors + predict-then-verify dumps) + a 3-refuter opus panel, each reading §6/§16
VERBATIM. **All three claims in the orchestrator's reconciliation memo were
REFUTED** — the findings below are FOLDED into the CP2 design and the two owner
STOPs. Raw transcripts: `subagents/workflows/wf_5a9d6636-81b/journal.jsonl`.

## The panel verdicts (folded)

### CLAIM 1 (observer + no-churn) — REFUTED, med-high
- **SURVIVES:** codegen stays byte-identical AND the existing OptDiff corpus stays
  byte-identical (nothing downstream reads `QueryImpl::instance_flow`; the grove is
  built strictly downstream at `Build.cpp:2664`). CP2 is the FIRST `.instanceflow`
  bless (none exist today).
- **R1 (FOLD):** a `kBoundQueryRead` use has no consumer/producer/columns, so the
  landed sort comparator (`InstanceFlow.cpp:233`) does not order it. Naïve insertion
  renumbers every existing `u#`. FIX: append reads as a CONTIGUOUS SUFFIX after all
  interior/terminal uses, ordered by `(read_collection.v, bound-param-ordinal)` —
  never by decl-enumeration/hash. Add a V-IF belt: read ids form `[M_interior, M)`.
- **R2 (bounded):** §7.2 items 3/4 (join-pivot / agg-group) are ALSO "roots" even
  without a bound param, and are the DEFERRED `CandidateSeed seeds` catalog. FOLD:
  define node `role=root` STRICTLY as "satisfies an emission/read OUTPUT obligation"
  (INSERT writer or bound-read target); candidate CONTEXTS are the separate `seeds`
  catalog, NEVER node roles — so a future seed-surfacing observer slice does not
  churn `role=`.
- **R4 (accept as scope):** both CP2 witnesses vacuously exercise the bound-read arm
  (writer0 == the INSERT node == role=root either way). A discriminating witness
  (bound `#query` over a relation with ≥2 non-mergeable INSERT sites) is desirable
  but NOT in the corpus; recorded as a follow-on, not a CP2 blocker.

### CLAIM 2 (§6/§16 fidelity) — REFUTED, high — THE HARD BREAK
- **F1:** `roots=(u#a,u#b,…)` PLURAL contradicts §6 `root_use: OriginUseId`
  (singular, bare type = not an `ordered list`) and §16 `root=u#41` (singular).
- **F2 (ADOPTED reconciliation):** keep `Family::root_use` as
  `std::optional<OriginUseId>`, `nullopt` in the flat slice; render `root=none` on
  the family line (the §16 `root=` slot, honestly empty). Phase-D subdivision fills
  the singular field with a real id → zero churn to the flat token. The covers block
  already enumerates the obligations, so `roots=` was redundant too.
- **F4/F5/F7 (SURVIVE, kept):** 2-value `{root,interior}` is a faithful strict
  subset of §16's role set (`role=input` is a Phase-D PORT concept; the flat grove
  reifies no inter-family ports — that, not "one family", is why input is deferred —
  F3). Node `role=root` == the INSERT-writer set (already marked by `-> lc#`), so it
  is §16-parity convenience, not new structure. Covering a bound read by `writer0`
  is consistent with §8.4 (non-authoritative occurrences MAY read the authoritative
  collection; the read carries no authority → V-EMISSION-UNIQUE untouched) + §7.2.
- **F6 (decided by orchestrator):** the landed covers line spells `role=copied|…`
  (an `InputColumnRole`); node lines add `role=root|interior` (an `OccurrenceRole`).
  Different line-kinds (`node …` vs `covers …`); §16's node line uses `role=root`
  and its covers line uses no `role=`. DECISION: keep node `role=` (§16-canonical);
  keep covers `role=` (landed); the two never collide within a line. Documented in
  the emitter — not renamed (no goldens exist to churn, but renaming buys nothing).

### CLAIM 3 (Phase-D line-of-sight, no hidden multi-session prereq) — REFUTED, high
- Phase C is **XL, multi-KLOC, cross-library**. `TABLE*` is baked into Rel's object
  model (144 occurrences; `Rel.h:24` hard-includes ControlFlow `Program.h`); table +
  index allocation is INTERLEAVED into ControlFlow region construction
  (`GetOrCreateIndex` inline at `Join.cpp:272/408`, `Build.h:443`), and
  `BuildDRInventory` runs LAST over already-allocated tables — Rel is a post-hoc
  SHADOW census, not the first authority.
- **TRUE first blocker:** re-sequence runtime-resource allocation OUT of
  `lib/ControlFlow/Build/*` (~8272 LOC) so it happens AFTER a Rel authority decides
  (Phase B item 5). Phase D's `SortedSectionProbe`/`SortedBatchMerge` (§11.4
  `AccessDecision`) can only DRIVE emission once Rel owns that decision = the Phase C
  exit gate. Phase D is sequenced strictly after it (§20 L1889).
- **No non-shadow step toward the codegen move is reachable this session.** Honest
  scope: CP2 (observer finish) + a written grounded plan. Any next code slice
  (observer MaterializationPlan cross-check, M-size; or candidate-JoinPivot seeds) is
  a LABELED shadow — byte-identical, NOT codegen-moving — and must be sold as one.
- **Strategic:** P7/P7b/P7c ALREADY delivered the join-pivot-flavored codegen win
  (partial-key hash seek + retired TUPLECMP) OUTSIDE InstanceFlow's chain, via
  `lib/Regional/RegionInstance.h` — so there is no urgency to rush Phase C, and the
  owner should weigh whether Phase D is even the cheapest path to that class of win.
- Phase B splits: items 1-3 (observer plan, abstract IDs, layouts=current) are
  one-session shadow-reachable; items 4-5 (Rel consumes IDs; allocation moves) ARE
  the megaproject → belong with Phase C. Draw the line inside Phase B at item 4.

## The adopted CP2 design (post-fold)
1. Node line gains ` role=<root|interior>` after the kind tag. root ⟺ terminal-INSERT
   writer OR bound-`#query`-read target (writer0). Candidate contexts stay in `seeds`.
2. Family line gains ` root=none` (flat families have no single root_use; keep the
   field `std::optional`, nullopt). NO plural `roots=`.
3. `kBoundQueryRead` coverage arm: one use per (collection whose decl `IsQuery()` with
   ≥1 bound param, per unique bound adornment). Appended as a deterministic CONTIGUOUS
   SUFFIX ordered by `(read_collection.v, bound-param-ordinal)`. Covered by writer0.
   Render: `covers u#K query <name> bound=(<cols>) reads lc#k occ=if#f.l`.
4. V-IF-COVERAGE arm for `kBoundQueryRead` (occ resolves; ids are the suffix `[Mi,M)`).
   V-IF-EMISSION unchanged (reads carry no authority).
5. Goldens: `instanceflow opt` `.irgold` steps for join_1, merge_2, transitive_closure
   (the bound-read witness), barrier_neck_1. Wire `-instanceflow-out` into run_irgold.
6. Gate: OptDiff SUITE PASS byte-identical (observer proof) + 4 new goldens + ctest 5/5.
