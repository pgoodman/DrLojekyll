# P7 execution grounding — physical access planning (the @key SEEK)

> The grounding-loop output for the P7 cut (branch `keyed-instances`), produced
> DOCS-ONLY per the session-23 charter [OWNER STOP]. Companion to
> `p7-grounding-seed.md` (the as-is whole-program view). Method: 4-phase loop
> (anchors → design → empirical refuter panel → IR states) via workflows +
> a throwaway-worktree empirical spike. Every file:line re-verified at tip
> (commit 0217338f). Grounded, adversarially critiqued, EMPIRICALLY validated.

---

## §0 STATUS + the one-line gap

P1–P6.2 LANDED; P7 is ranked-#1. The gap: `SelectAccessPlan`
(`RegionInstance.h:270-275`) is a near-stub dispatching on `has_free` alone —
the real partial-key hash SEEK arm (`GetOrCreateIndex(bound-subset) →
Index::First/Next`) is neither SELECTED at freeze nor EMITTED at codegen. P7
closes exactly that. **New codegen surface: NONE** — the `via_index` First/Next
arm (`Database.cpp:1668-1673/1724-1730`) and the `GetOrCreateIndex` provisioner
(`Data.cpp:348`) already exist; the entire emission flip is driven by the
selector return value alone (`withhold_index = plan==kFullScanFilter` at
`Build.cpp:446` already goes false for a seek plan).

---

## §1 THE DESIGN FORK the owner must ratify — Q1a vs Q1b (scope character)

The single load-bearing decision. The dispatch fires on
`has_free && !available_bindings.empty()` = **any partial-bound `#query`**, not
just `@key` ones. So the seek-key provenance question (seed §5-Q1) decides the
whole character of P7:

| | **Q1a — raw bound subset** (design agent's pick) | **Q1b — gate on routed `@key`** (charter framing) |
|---|---|---|
| Seek fires on | ANY bound+free query | only queries whose bound subset routes (P6.2) to a `declared-key` |
| Real seeks gained | **~25** (whole `transitive_closure*`/`fibonacci*`/agg families) | **1–2** (`key_partial_1`, maybe `key_corecursion_1`) |
| Firewall (`Regional.h:92`) | stays UP (selector never reads `declared_access_paths`) — bridged by ZERO | deliberately BRIDGED (physical choice tied to logical hint) |
| Goldens that MOVE | 8 region (`key_partial_1`+`key_corecursion_1` ×4) + 1 new `.h` | 4 region (`key_partial_1` ×4) + 1 new `.h` |
| Matches seed §4 "only key_partial_1 moves" | NO (§4 undercounts; but move set still tiny) | YES exactly |
| Matches charter "narrow **@key** SEEK" framing | NO (@key becomes irrelevant to the mechanism) | YES (the namesake arc) |
| Recursive-owned seek (Q3) | forced NOW; UNFENCED; exercised by real recursive carriers | mostly moot (only key_partial_1 acyclic, unless key_corecursion_1 in) |

**Why Q1a is technically the stronger design** (and golden-safe): the 25
compiling partial-bound carriers have **0 codegen goldens** — their cursors flip
full-scan→seek invisibly to the golden set; their **25 stdout** (4-mode) +
**7×(oracle+monotone+behavioral)** goldens act as a FREE answer-invariance net
that HARD-catches any miscompile (esp. the recursive `transitive_closure_diff{,2}`
and `fixpoint_stress_1`). So Q1a buys ~25 real seeks + real recursive-seek
coverage for the SAME tiny 8+1 golden move — while Q1b buys 1–2 seeks.

**Why this is nonetheless an OWNER call, not a subagent's:** Q1a OVERRIDES the
charter's explicit "@key SEEK" framing and seed §4's prediction. It changes what
P7 *is* (broad "seek on any partial binding" vs narrow "@key specialization").
That scope character is the owner's to set. The empirical spike (§3) proves Q1a
is buildable + answer-correct so the choice is informed either way; Q1b is always
a safe strict-subset fallback.

---

## §2 OPEN-QUESTION RESOLUTIONS (settled)

- **Q1 (provenance):** the fork above — OWNER RATIFIES. Design recommends **Q1a**
  (raw bound subset; firewall stays up; `SortAndUnique` gives `[A,B]≡[B,A]` free).
  Fallback **Q1b** if the owner wants P7 to stay the narrow @key cut.
- **Q2 (`kRetainedIndexScan`):** **Option A — split** into `kUnplanned` (=0
  sentinel, never returned) + `kPartialKeyHashSeek` (the real seek). Removes the
  overloaded-value trap. Renumber is byte-safe (R3, §3).
- **Q3 (fence):** **NO fence** (`HasOrCanMintHashIndexOn` ≡ true). A bound+free
  query reads the SETTLED relation after the fixpoint quiesces; the seek is an
  acyclic complete read via a secondary index (identical to P4's kFullScanFilter
  argument), and `GetOrCreateIndex` SHARES an existing index by `column_spec` or
  mints a maintained one — the fixpoint's own indexes are untouched. Empirically
  gated by the recursive carriers' answer goldens (§3). Fallback: gate on
  `!recursive_owned` if the spike shows any problem.
- **Q4 (caps):** **IMPLICIT** — a `static_assert` + comment enumerating the three
  emittable arms; no threaded object, `SelectAccessPlan` signature unchanged.
- **Q5 (V-PLAN-HONEST):** stays at the query-build site (`Build.cpp:458`),
  generalized to a 3-way per-kind implication. **NO EmitScan belt in P7** — since
  P7 is #query-path-only (Q6), the D4→D6 sequencing hazard EVAPORATES and
  `plan_kind`-on-`ProgramTableScanRegion` is deferred to P7b.
- **Q6 (new op?):** **NO** — purely Regional-select + codegen-read; #query path
  ONLY. No Rel op, no runtime structure, no `ProgramTableScanRegion` change. The
  interior/join `EmitScan` path stays index-presence-derived (a clean P7b
  follow-on).
- **Q7 (golden surface):** **extend `key_partial_1.irgold` with `h opt`** — the
  FIRST codegen golden since P4 (the `Index<>` member + First/Next cursor). One
  mode suffices (region goldens pin the mode-invariant `plan=` flip).

---

## §3 THE DIFFS (settled) + EXECUTION ORDER

One atomic commit. D4 (thread plan_kind on ProgramTableScanRegion) is DROPPED to
P7b (Q5/Q6). D5 is a NO-OP (documented — the flip is selector-driven).

1. **D1 — enum split + render token + record default.**
   `RegionInstance.h:242-250` enum → `{kUnplanned=0, kFullScanFilter=1,
   kFullKeyHashLookup=2, kPartialKeyHashSeek=3}` + `static_assert`.
   `Regional.h:153` default → `kUnplanned`. `Format.cpp:139-141` `AccessPlanText`
   → add `partial-key-hash-seek`/`unplanned` (make the switch exhaustive to dodge
   `-Werror=switch`). Update stale comments (`Regional.h:150-152`, `Build.h:217`).
   *Gate:* no-partial-bound programs byte-identical; a mis-wired token renders
   `retained-index-scan` and fails the region diff.
2. **D2 — real `SelectAccessPlan`.** `RegionInstance.h:270-275`: all-bound→
   `.Find`; bound+free with non-empty bindings→`kPartialKeyHashSeek`; else
   fallback `kFullScanFilter`. `available_bindings` (computed, discarded today)
   becomes load-bearing. *Gate:* never returns `kUnplanned`; flip observed at D5/D7.
3. **D5 — NO code diff** (`Build.cpp:446-453` unchanged; `withhold_index` already
   false for a seek plan → `GetOrCreateIndex(col_indices)` provisions the bound
   subset → `via_index=true` → existing First/Next cursor). Add a one-line comment
   so nobody "helpfully" adds a redundant switch.
4. **D6 — 3-way V-PLAN-HONEST belt** at `Build.cpp:458` (+ mirror the positive arm
   into `BuildEmptyQueryEntryPointImpl` after `:502`): keep
   `kFullScanFilter ⇒ ¬index`; ADD `kPartialKeyHashSeek ⇒ index`. fprintf+abort,
   survives NDEBUG. *Gate:* a seek plan that failed to provision aborts corpus-wide.
5. **D8 — update the stale P4 unit expectation.** `RegionInstanceTest.cpp:262`:
   `SelectAccessPlan(bound_free)` now yields `kPartialKeyHashSeek`, not
   `kFullScanFilter`. Flip the assertion (spike-confirmed). Restores ctest 5/5.
6. **D7 — goldens:** extend `key_partial_1.irgold` with `h opt`; re-bless
   `key_partial_1.region.*` (+ `key_corecursion_1.region.*` under Q1a) + the new
   `key_partial_1.h.opt`. Verify NO other golden moves.

---

## §4 EMPIRICAL VALIDATION (the throwaway-worktree spike) — DONE

A throwaway worktree at tip `0217338f` had D1+D2+D6 applied, built under `-Werror`,
and ran the FULL OptDiff suite + ctest. **VERDICT: Q1a is buildable, answer-correct,
and golden-safe. No surprises.** (Worktree discarded; nothing committed/blessed.)

- **BUILD: OK** under `-Werror`. `available_bindings` is populated with bound-param
  indices (`Planning.cpp:252-254`) so bound+free correctly reaches the seek arm.
- **R1 (recursive answer-invariance): PASS — the decisive bet holds.** Suite reported
  `SUITE: FAIL` with **exactly 8 lines**, all `IRGOLD-DIVERGE` on
  `key_partial_1.region.{opt,nodf,nocf,none}` + `key_corecursion_1.region.*` (the
  `plan=` token only). **ZERO** stdout/oracle/monotone/behavioral diffs anywhere.
  The recursive carriers (`transitive_closure_diff{,2}`, `fixpoint_stress_1`,
  `key_corecursion_1`) all ran and are byte-identical — reading a recursive relation
  via an index seek over the settled table is answer-invariant.
- **R2 (blast radius): MATCHES exactly** — the complete diffing set is the 8 region
  `plan=` goldens; everything else byte-identical across all 4 modes.
- **R3 (enum renumber byte-safety): PASS** (analytic + build) — all `AccessPlan` uses
  by-name; `RequestPortRecord` never hashed; renumber touches nothing.
- **Check 1 (cursor flip): PASS** — `reachable_from_bf` factory
  `return {db, From, db.idx_19.First({From})};`; step `pos = db.idx_19.Next(id);`;
  member `::hyde::rt::Index<Key19> idx_19;` (maintained `idx_19.Add({v12}, ins0.id)`).
  No `NumRows()` rescan, no `row.f != From` re-check remain.
- **R6 (index sharing, no dup member): PASS.** `transitive_closure_diff` → the query
  seek `reachable_from_bf` reuses `db.idx_80` — the SAME index the recursive join
  loops read/maintain (SHARED by `column_spec`, zero new index). `key_corecursion_1`
  → the `{K}` query seek mints ONE fresh `idx_74` (withheld under P4), no spec
  collision with the co-recursion join indexes `idx_48`/`idx_49`.

**NEW execution item the spike surfaced (not in the seed):**
- **ctest RegionInstance 4/5** — `tests/RegionInstance/RegionInstanceTest.cpp:262`
  `ASSERT_TRUE(SelectAccessPlan(bound_free) == AccessPlan::kFullScanFilter)` is a
  STALE P4 unit expectation; P7 makes a bound+free requirement select
  `kPartialKeyHashSeek`. **Execution must update line 262 → `kPartialKeyHashSeek`.**
  NOT a defect — an expectation flip. (All other ctests pass.)

---

## §5 DESIRED IR STATES (predict-then-verify) — key_partial_1

**`-region-out` line 9:** `plan=full-scan-filter` → `plan=partial-key-hash-seek`
(declared-key/rule/shared-field/census lines unchanged).

**Generated `reachable_from_bf` cursor** — BEFORE (full scan) → AFTER (seek):
```
- while (pos < db.reachable_from_4.NumRows()) { id=pos++; row=RowAt(id);
-   if (row.f != From) continue; To=row.t; return true; }
+ while (pos != ::hyde::rt::kNoRow) { id=pos; pos=db.<idx>.Next(id);
+   row=RowAt(id); To=row.t; return true; }        // no re-check (exact index)
  factory: return {db, From, 0};  ->  return {db, From, db.<idx>.First(Row{From})};
+ NEW: ::hyde::rt::Index<...> <idx>; static member on Database.
```
`.stdout` (all 4 modes) BYTE-IDENTICAL (answer-invariant — the M3 backend answers
a complete read; `Index::First/Next` is FULL-KEY EXACT, `Table.h:791`).

---

## §6 EXIT GATE (structural, for execution)

- OptDiff **`SUITE: PASS`**; ctest **5/5** (requires the D8 `RegionInstanceTest.cpp:262`
  expectation flip — spike showed 4/5 without it, that one assertion only).
- `key_partial_1` (+ `key_corecursion_1` under Q1a) `-region-out` shows
  `plan=partial-key-hash-seek`; its `.h` emits `<idx>.First/.Next` + an `Index<>`
  member and NOT the `NumRows()` rescan.
- EVERY `.stdout`/oracle/monotone/behavioral golden BYTE-IDENTICAL (a moved one is
  a miscompile, not a bless).
- Goldens re-blessed: exactly the region `plan=` flips + the new `key_partial_1.h.opt`.
- Codegen byte-stable for every all-bound / all-free query (and, under Q1b, every
  non-@key program).
