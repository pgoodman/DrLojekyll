# P7c execution grounding — retire the probe-REDUNDANT partial-scan re-check (AccessPlan Fold C)

> STATUS: **LANDED** (session 25). OptDiff **SUITE: PASS (226)**, ctest **5/5**. This is the
> FIRST cut where the `AccessPlan` authority PAYS OFF in emission — unlike P7/P7b it MOVES codegen
> (the seek carriers' generated key re-check gate disappears). Method: the grounding loop
> (confirm-then-ground) + an in-tree empirical spike + a 3-refuter opus adversarial panel, the
> loop that landed P2–P7b (`p7b-execution-grounding.md` is the method exemplar).

## §0 STATUS + the one-line gap

P7b NAMES every interior partial index scan `kPartialKeyHashSeek` and the EmitScan V-PLAN-HONEST
belt proves the emitted arm is the full-key-exact `keyed_chain` (`Index::First/Next`). P7c DROPS
the redundant `TUPLECMP` re-check that `BuildMaybeScanPartial` still wrapped around each such scan
body. It is **Fold C for the partial-scan path** — the TABLEJOIN body already retired its twin
re-check (`Table.h:799-803`), and P7c extends that exact argument to the last place still emitting
the re-check.

## §1 WHAT P7c IS (the cut, shape (i) — owner-ratified)

`BuildMaybeScanPartial` (`lib/ControlFlow/Build/Build.h`, the SOLE LIVE TABLESCAN mint) built, per
partial index scan, a `TUPLECMP cmp` as `scan->body` and, in the per-column loop, added an equality
pair `cmp->lhs_vars=in_var (requested key), cmp->rhs_vars=out_var (scanned column)` for each INDEXED
(bound) column, nesting the real body under `cmp->body`. In codegen that `cmp` lowered to an
`if (in == out && …) { … }` gate wrapping the seek body.

**The cut (shape (i), the minimal diff):** for the seek, add NO comparison pairs. The per-column
loop's indexed branch is reached ONLY when an index exists (`in_col_indices` non-empty), which is
ALWAYS the `kPartialKeyHashSeek` arm (the all-columns-bound case early-returns before the mint).
So the two `AddUse` lines were exclusively on the seek path — deleting them (and the now-dead
`in_var`/`j` machinery) leaves the `cmp` a vacuous (trivially-equal) TUPLECMP. Preserved unchanged:
`scan->in_vars` (the seek probe key, read at `Database.cpp:2818`), `scan->in_cols`, `scan->out_vars`,
`cmp->col_id_to_var` (recordization, still populated for EVERY column), and the `cmp` as the body's
parent region.

Shapes (ii) LET pass-through and (iii) drop `in_cols`/`in_vars` were ranked and rejected: (ii) is
larger IR churn with no codegen benefit; (iii) breaks the seek (in_vars IS the probe key).

## §2 WHY IT IS REDUNDANT (the load-bearing correctness fact — CONFIRMED at tip)

- The index key set == the bound-column set by construction: `index = GetOrCreateIndex(in_col_indices)`
  and `in_col_indices` is exactly the bound columns; `KeyColumns() == SortAndUnique(in_col_indices)`.
  So every bound column IS an index key column — never a bound-but-unindexed filter. (Refuter 1.)
- `Index::First/Next` is FULL-KEY EXACT by contract (`include/drlojekyll/Runtime/Table.h:789-824`:
  a slot matches only on `slot.used && slot.hash == hash && slot.key == key`, the generated Key's
  memberwise VALUE `operator==` over every key column, IEEE `==` for float columns — identical to
  the deleted TUPLECMP's `==`). Every row the seek yields already has key columns == the requested
  key, so the deleted `in_var == out_var` pairs were trivially true.
- The P7b V-PLAN-HONEST belt (`Database.cpp:2832`, UNCHANGED) asserts `kPartialKeyHashSeek ⇒
  keyed_chain` at the emission site (fprintf+abort, survives NDEBUG). `keyed_chain` cannot be false
  on this path (index always registered in `index_member`; `input_vars.size() == KeyColumns().size()`
  always), and even a hypothetical divergence aborts LOUDLY — never a silent miscompile. (Refuter 2.)
- The full-scan arm (`index == nullptr`, zero bound columns) ALREADY built an empty `cmp`; the 22
  corpus `scan-table` carriers already exercise the empty-cmp shape through all 4 modes + codegen.
  P7c merely makes the seek arm match. (Refuter 3.)

This is the SAME argument that licensed the TABLEJOIN body (and the differential sections) to omit
their per-row re-check — keep the two explicitly linked (`Table.h:799-803`).

## §3 THE DIFF (settled)

- `lib/ControlFlow/Build/Build.h` — in `BuildMaybeScanPartial`: delete the two `cmp->lhs_vars/
  rhs_vars.AddUse` pairs and the `in_var`/`j` machinery that only fed them; keep `scan->in_cols`,
  `scan->in_vars`, `scan->out_vars`, `cmp->col_id_to_var`, and the `cmp` parent. Update the two
  belt comments to point at the `Table.h:789-824` full-key-exact contract and link the TABLEJOIN
  precedent (Fold C).
- `tests/OptDiff/cases/negate_1.irgold` — add `h opt` to pin the codegen shrink (`negate_1` had no
  `.h` golden; the point of P7c is the emission payoff).
- `tests/OptDiff/goldens/negate_1.ir.opt.golden` — MOVED (the `if-compare`/`if-true` gate removed;
  5 ins / 7 del, net −2 lines + de-indent). Every region id + other line byte-identical.
- `tests/OptDiff/goldens/negate_1.h.opt.golden` — NEW (pins the gate-free `idx_28.First/.Next` loop).

## §4 EMPIRICAL VALIDATION (the in-tree spike + refuter panel) — DONE

- **Suite (change applied, pre-bless):** exactly ONE divergence, `negate_1 irgold ir.opt
  IRGOLD-DIVERGE` — the predicted seek-carrier shrink. EVERY answer golden (`.stdout` / oracle /
  monotone / behavioral) BYTE-IDENTICAL across all 4 modes. No other `.ir` / `.region` / `.h` /
  `.df` / `.rel` golden moved. No crash/abort (debug re-asserts pass → the vacuous TUPLECMP trips
  no always-on validator).
- **`.ir.opt` diff:** the `if-compare {@A:30} = {@A:27}` / `if-true` gate vanishes (opt's
  `OptimizeImpl(TUPLECMP)` replaces the vacuous kEqual cmp with its body, `Optimize.cpp:595`); the
  `check-member` de-indents to nest directly under `scan-index`; all else byte-identical.
- **Codegen:** `negate_1`'s `idx_28.First/.Next` loop binds `r29.a/r29.b` and enters the body with
  NO `if (v27 == r29.a)` re-check.
- **Full-scan carrier `insert_4.ir.opt`:** BYTE-IDENTICAL (full-scan path untouched).
- **Adversarial panel (3 opus refuters, all `refuted=false` high confidence, "none found"):**
  Refuter 1 (unindexed bound column — impossible by construction), Refuter 2 (`keyed_chain` false
  under `kPartialKeyHashSeek` — impossible + belt catches it), Refuter 3 (orphaned `col_id_to_var`
  / vacuous-cmp downstream breakage — none; empty-cmp is pre-existing, `col_id_to_var` preserved,
  independently re-ran build + ctest 5/5 + `key_partial_1`/`negate_1` diffrun all-4-modes clean).

## §5 DESIRED IR STATES (predict-then-verify — empirically confirmed by §4)

BEFORE (`negate_1.ir.opt`):
```
scan-index select {@A:30, @B:31} from %table:11 where {@A:27} in %index:28 plan=partial-key-hash-seek
  if-compare {@A:30} = {@A:27}
    if-true
      check-member in-new {@A:30, @B:31} in %table:11
        ...
```
AFTER:
```
scan-index select {@A:30, @B:31} from %table:11 where {@A:27} in %index:28 plan=partial-key-hash-seek
  check-member in-new {@A:30, @B:31} in %table:11
    ...
```
Codegen BEFORE: `for (…First/…Next) { r = RowAt; if (v27 == r.a) { bind; body } }`.
Codegen AFTER: `for (…First/…Next) { r = RowAt; bind; body }`.

Mode note: opt/nodf (controlflow-opt ON) elide the vacuous cmp entirely; nocf/none (OFF) keep an
empty `if-compare` that EmitCompare renders as no gate (`Database.cpp:2914`, `terms.empty()`). Both
produce identical gate-free codegen. `.ir` goldens are per-mode via `.irgold` (no cross-mode byte
compare), so the opt-elided vs no-opt-present shape is fine — the norm for full-scan carriers.

## §6 EXIT GATE — MET

- OptDiff **SUITE: PASS (226)**; ctest **5/5**.
- Every `.stdout` / oracle / monotone / behavioral golden BYTE-IDENTICAL across all 4 modes
  (answer-invariance = the correctness proof; the retired check was always-true).
- The seek carriers' generated code SHRINKS: no `if (key == scanned)` gate in the `keyed_chain`
  loop body. Pinned by `negate_1.h.opt` (NEW) + `negate_1.ir.opt` (MOVED).
- The P7b V-PLAN-HONEST belt is unchanged and still never fires.
- Full-scan (`kFullScanFilter`) and statically-dead join-pivot paths BYTE-UNCHANGED.

## §7 NEXT (owner re-ranks)

P6.3–P6.6 runtime evaluation (compile-time P6.3 fusion-detection spike = low-risk entry), P8/P9
(ordered trie / path inference). The `AccessPlan`-authority arc through emission is now complete for
the partial-scan path.
