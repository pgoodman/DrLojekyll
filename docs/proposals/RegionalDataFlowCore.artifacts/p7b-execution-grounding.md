# P7b execution grounding — interior/join plan-driven scans (the EmitScan honesty belt)

> The grounding-loop output for the P7b cut (branch `keyed-instances`), produced
> DOCS-ONLY per the session-24 charter [OWNER STOP] (owner picked P7b over the
> P6.3 spike and P8/P9). Companion to `session-24-whole-program-seed.md` (§2-P7b,
> §5-P7b) and the P7 exemplar `p7-execution-grounding.md`. Method: the 4-phase
> loop (anchors → design → adversarial refuter panel → IR states) via workflows +
> a throwaway-worktree empirical spike. Every anchor re-verified at tip
> **`c213733f`**. Grounded, adversarially critiqued, EMPIRICALLY validated
> (SUITE PASS with the belt LIVE across all 4 modes; a hardened-belt run proved
> no live scan is unplanned).

---

## §0 STATUS + the one-line gap

P1–P7 LANDED; the owner picked P7b for session 24. The gap: P7 threaded the
`AccessPlan` physical-structure authority onto the **`#query` path only**. The
INTERIOR/JOIN table-scan emitter `EmitScan` (`Database.cpp:2778`) still
re-derives its arm — `keyed_chain` (`Index::First/Next`, `:2825`), `keyed_probe`
(`.Find`, `:2831`), full-table-scan+filter (`:2836`) — from index-presence ×
arity with **NO stored plan** to referee it. P7b threads an `AccessPlan
plan_kind` onto `ProgramTableScanRegionImpl` and moves/duplicates the
V-PLAN-HONEST honesty belt to the `EmitScan` emission site with a `kUnplanned`
skip. This is "D4 Option-2" (`keyed-rewrite-p7p9-diffs.md §1.5`,
`keyed-rewrite-reconstruction-diffs.md` D4). **New codegen surface: NONE** —
`EmitScan` already chooses the P7b-shaped arms today; `plan_kind` is a
compile-time SHADOW of that choice + a referee that cross-checks the two agree.

**Character: a REFEREE + authority-completion cut, codegen byte-stable** (exactly
like P7's `#query` belt). It does not change one byte of emission; it names the
interior-scan access plan and asserts the emitted arm honours it.

---

## §1 WHAT P7b IS (no scope fork — the P7 Q1a/Q1b fork does not recur)

P7's load-bearing decision was a scope character (raw bound subset vs `@key`-gated).
P7b has no such fork: it planifies EVERY live interior TABLESCAN, and the plan is
mechanically forced by the mint (`index ? seek : filter`). The only real decisions
are (a) render the token on `.ir` or keep it internal, and (b) single-source vs
duplicate the plan-text — both settled in §2.

**The corpus reality (swept empirically at tip, all 4 modes):**
- 35/180 cases carry interior scans: **69 `scan-index` lines (all
  `kPartialKeyHashSeek`) + 22 `scan-table` lines (all `kFullScanFilter`)**.
- **ZERO all-key `scan-index` in any mode** → `keyed_probe` (the all-columns-bound
  `.Find` arm) NEVER arises on the live interior path → the two-way
  `index ? kPartialKeyHashSeek : kFullScanFilter` formula is COMPLETE.
- `BuildMaybeScanPartial` (`Build.h:448`) is the **SOLE LIVE** TABLESCAN mint.
  `BuildNestedLoopJoin` (`Join.cpp:254`) is **statically dead** — its one caller
  (`Join.cpp:775`) sits in the `else` of `else if (true || …)` (`Join.cpp:742`),
  compile-time unreachable in ALL modes (not merely `assert`-gated), so the
  acyclic pivot join emits a `TABLEJOIN`, never a `TABLESCAN` (`join_1` /
  `optimize_2` produce zero scan lines). Hence `kUnplanned` is a pure dead-default.

---

## §2 OPEN-QUESTION RESOLUTIONS (settled)

- **Q1 (referee or real select?):** BOTH — real classifications exist (91 live scan
  lines get a real plan) AND emission is byte-stable (`plan_kind` ∉
  Hash/Equals/MergeEqual; the `EmitScan` dispatch is untouched). P7b is a referee +
  render, not a new lowering decision.
- **Q2 (which callers keyed?):** `Stratum.cpp:1033` EmitJoinFire → seek;
  `:1215` LowerCrossoverArm (negation) → seek; `:1333` LowerProductArm (avail
  unconditionally empty) → full-scan. All through the one `BuildMaybeScanPartial`
  mint.
- **Q3 (belt × join pivots):** the `kUnplanned` skip is correct and **vacuous on
  the live path** — there is no live join-pivot TABLESCAN. Proven by the
  hardened-belt spike run (`assert plan != kUnplanned` → SUITE PASS). The shipped
  belt uses the SKIP form, so even a hypothetically-revived dead scan is skipped,
  never aborted — no new failure surface.
- **Q4 (render?):** **RENDER**, gated `plan != kUnplanned`, own trailing
  ` plan=<token>` token on the `.ir` scan line (P6.1/P6.2 gated-block precedent).
  Golden-INVISIBLE for the sole committed `.ir` golden (`symrec_tie_1`, zero scan
  lines). Carriers for the positive pin: **`negate_1`** (`partial-key-hash-seek`) +
  **`insert_4`** (`full-scan-filter`) — the two smallest carriers covering both
  live plan kinds. Deterministic under MergeEqual: `plan_kind` is a pure function
  of index-presence, and index-presence is part of the `Equals` key, so two
  Equals-equal scans carry identical `plan_kind` → the surviving token is honest
  and deterministic without folding `plan_kind` into `Equals`.
- **Q5 (plan-text source):** **promote** the `AccessPlanText` switch to an `inline`
  in `include/drlojekyll/Regional/RegionInstance.h` beside the enum (single source
  of truth; `lib/Regional/Format.cpp:138`'s file-static becomes a call to it) —
  kills the token-drift risk (R5). PROVEN FALLBACK: a local `ScanPlanText` switch
  duplicated in `lib/ControlFlow/Format.cpp` (what the spike used to keep Regional
  byte-untouched) — safe, but two copies can drift; the new `.ir` goldens catch a
  mismatch on first bless.

---

## §3 THE DIFFS (settled) + EXECUTION ORDER

One atomic commit. All anchors re-verified at tip `c213733f`.

1. **D1 — the field + accessor + include.** `ProgramTableScanRegionImpl`
   (`lib/ControlFlow/Program.h`, after `out_vars` ~`:1562`): add
   `AccessPlan plan_kind{AccessPlan::kUnplanned};`. Public wrapper
   `ProgramTableScanRegion` (`include/drlojekyll/ControlFlow/Program.h`): add
   `AccessPlan PlanKind(void) const noexcept;`, def in `lib/ControlFlow/Program.cpp`
   (`return impl->plan_kind;`). `#include <drlojekyll/Regional/RegionInstance.h>` in
   both ControlFlow `Program.h`s (the ControlFlow→Regional edge already exists —
   `Program::Build(const FrozenRegionalProgram&)`; spike confirmed no cycle).
   `plan_kind` NEVER in Hash (`Operation.cpp:1565`), Equals (`:1586`), MergeEqual
   (`:1637`) — the S5′ `mint_tag` precedent. *Gate:* any golden move ⇒ a
   Hash/Equals leak; caught corpus-wide.
2. **D2 — stamp the plan at the sole live mint.** `BuildMaybeScanPartial`
   (`Build.h`, after the `scan->index.Emplace` ~`:448-454`):
   `scan->plan_kind = index ? AccessPlan::kPartialKeyHashSeek
   : AccessPlan::kFullScanFilter;`. Exhaustive (all-bound early-returns at `:414`
   ⇒ strict-subset key ⇒ never `keyed_probe`). *Gate:* a swapped formula flips the
   `negate_1`/`insert_4` `.ir` token.
3. **D3 — the dead-mint comment (no code).** `BuildNestedLoopJoin`
   (`Join.cpp:254`): a one-line comment recording that it is statically dead
   (caller behind `else if (true || …)` at `:742`), and that a future re-enable
   must stamp `scan->out_cols.Empty() ? kFullKeyHashLookup : kPartialKeyHashSeek`
   to stay belt-correct (the SKIP belt would otherwise silently pass a revived
   unplanned keyed scan — Refuter 2 residual (2)).
4. **D4 — the EmitScan honesty belt.** `EmitScan` (`Database.cpp`, after
   `keyed_probe` is computed ~`:2824`, before `if (keyed_chain)` at `:2825`):
   fprintf+abort per-kind IMPLICATION belt (survives NDEBUG):
   `if (plan != kUnplanned) require (kFullScanFilter && !keyed_chain &&
   !keyed_probe) || (kPartialKeyHashSeek && keyed_chain) || (kFullKeyHashLookup
   && keyed_probe);`. Reads the booleans `EmitScan` already computes; does not
   drive dispatch. *Gate:* an F17/F18-class dispatch skew aborts the suite.
5. **D5 — render.** `operator<<(OutputStream&, ProgramTableScanRegion)`
   (`lib/ControlFlow/Format.cpp`, after the `} in <index>` / `from <table>`
   clause ~`:836`, before the newline): gated `if (plan != kUnplanned) os <<
   " plan=" << AccessPlanText(plan);`, spellings identical to Regional
   (`unplanned` / `full-scan-filter` / `full-key-hash-lookup` /
   `partial-key-hash-seek`). Promote `AccessPlanText` to the RegionInstance.h
   inline (Q5). *Gate:* `symrec_tie_1.ir` byte-identical (no scan lines); the two
   new goldens pin the token text + placement.
6. **D6 — goldens.** Add the two `ir opt` steps: append `ir opt` to
   `tests/OptDiff/cases/negate_1.irgold`; create `tests/OptDiff/cases/insert_4.irgold`
   with `ir opt`. Bless the two NEW `tests/OptDiff/goldens/{negate_1,insert_4}.ir.opt.golden`
   via `runall.sh --bless` after review. Verify NO other golden moves.

---

## §4 EMPIRICAL VALIDATION (the throwaway-worktree spike) — DONE

A throwaway worktree at tip `c213733f` had D1+D2+D4+D5 applied (D3 is a comment),
built, and ran the FULL OptDiff suite twice. **VERDICT: BUILDABLE + BYTE-STABLE +
BELT-CLEAN. No surprises.** (Worktree discarded; nothing committed/blessed.)

- **BUILD: OK** — clean debug build, no new warnings, **no include cycle** (the new
  ControlFlow→Regional `#include` compiles; `RegionInstance.h` pulls only std
  headers).
- **BYTE-STABILITY: PASS** — `SUITE: PASS (226 cases)`, `git status` shows ONLY the
  6 edited source files, **ZERO golden files modified** across all 4 modes.
- **BELT (live, skip form): never fired** — the stored plan is honest for all 91
  live scan lines × 4 modes.
- **BELT (hardened to `assert plan != kUnplanned`): PASS** — an incremental rebuild
  + full re-run stayed `SUITE: PASS`, zero aborts → **no live scan escapes as
  kUnplanned** → `BuildMaybeScanPartial` covers every live TABLESCAN,
  `BuildNestedLoopJoin` is dead.
- **RENDER (verbatim, real generated `.ir`):**
  - `negate_1`: `scan-index select {@A:30, @B:31} from %table:11[i32,i32] where {@A:27} in %index:28[i32,_] plan=partial-key-hash-seek`
  - `insert_4`: `scan-table select {@X:47} from %table:25[i32] plan=full-scan-filter`
- **Refuter panel (2 independent opus refuters): both refutations FAILED (claims
  hold).** (a) byte-stability + MergeEqual determinism confirmed (Hash/Equals/
  MergeEqual never read `plan_kind`; `next_id` is a monotonic counter, never
  layout-derived; the survivor keeps its own honest plan). (b) belt-completeness +
  dead-code confirmed (only 2 TABLESCAN mints; `Join.cpp:775` compile-time dead via
  `true ||`; `keyed_probe` impossible because `input_vars.size() == view_cols.size()
  < num_cols == fields.size()` for every minted scan).

**Non-blocking residuals to CARRY (execution-phase watch, not fixes):**
- **[R-drift]** If Q5's fallback (duplicate switch) is used, the two plan-text
  copies can drift; the new `.ir` goldens catch it on first bless. → Prefer the
  inline promotion.
- **[R-revive]** If the `true ||` at `Join.cpp:742` is ever removed,
  `BuildNestedLoopJoin` revives and mints a `kUnplanned` scan the SKIP belt passes
  silently. → D3's comment records the correct stamp so a re-enable is belt-correct.
- **[R-width]** `keyed_probe`-impossibility rests on the invariant
  `table.Columns().size() == view.Columns().size()` (the `NthColumn` loop). A future
  data-model change letting a scan target a narrower/wider table than its view could
  make `input_vars.size() == fields.size()` for a partial key → the belt would abort
  on a real program. Documented coupling; not reachable today.

---

## §5 DESIRED IR STATES (predict-then-verify — empirically confirmed by §4)

- **`negate_1.ir` (NEW golden):** the crossover scan line renders
  `scan-index … plan=partial-key-hash-seek`. Everything else byte-identical to a
  clean `-ir-out` of `negate_1` at tip.
- **`insert_4.ir` (NEW golden):** the product-arm scan line renders
  `scan-table … plan=full-scan-filter`.
- **`symrec_tie_1.ir.opt.golden` (existing):** BYTE-IDENTICAL (zero scan lines, so
  no `plan=` token appears).
- **EVERY** `.df` / `.rel` / `.region` / `.contract` / `.h` / `.cpp` / `.stdout` /
  `.oracle` / `.monotone` / `.behavioral` golden: BYTE-IDENTICAL corpus-wide (a
  moved one is a `plan_kind`-into-a-structural-fold leak, not a bless).

---

## §6 EXIT GATE (structural, for execution)

- OptDiff **`SUITE: PASS`**; ctest **5/5** (P7b touches no ctest expectation — the
  P4/P7 `RegionInstanceTest` selects on the `#query` path, unchanged by P7b; confirm
  at execution).
- `negate_1.ir` / `insert_4.ir` show the two `plan=` tokens on the correct arms.
- The V-PLAN-HONEST belt is LIVE at `EmitScan` and never fires (SUITE PASS is the
  proof); a deliberately-skewed dispatch would abort.
- EVERY non-`.ir` golden BYTE-IDENTICAL; the ONLY goldens added are the two new
  `.ir` files; NO golden modified.
- Codegen byte-stable for every program (P7b changes no emission).
