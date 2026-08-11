# P7c charter — retire the probe-REDUNDANT partial-scan re-check (the AccessPlan payoff)

You are resuming the **keyed-instance greenfield rewrite** on branch `keyed-instances` (Dr.
Lojekyll, the `hyde` C++ Datalog compiler). **P1–P7 + P7b ARE LANDED** (OptDiff **SUITE: PASS
(226)**, ctest **5/5**; tip `f9a2cdd3`). The owner has picked **P7c** for this session.

**P7c in one line:** now that P7b NAMES every interior partial index scan `kPartialKeyHashSeek`
and the EmitScan V-PLAN-HONEST belt proves the emitted arm is the full-key-exact `keyed_chain`
(`Index::First/Next`), DROP the redundant `TUPLECMP` re-check that `BuildMaybeScanPartial` still
wraps around each such scan body. **This is the FIRST cut where the `AccessPlan` authority PAYS
OFF in emission** — unlike P7/P7b it MOVES codegen goldens (the seek carriers' generated
`if (key == scanned) { … }` gate disappears; `.h`/`.cpp`/`.ir` shrink). Answers are invariant
(the removed check was always-true), so every gate is STRUCTURAL + a codegen-shrink witness.

## Read first (resume authority, in order)
1. **`p7b-execution-grounding.md`** — the LANDED P7b record + the method exemplar (grounding loop
   + throwaway-worktree spike). P7c reuses that exact method.
2. **`session-25-prompt.md`** — the general post-P7b charter; P7c is the "P7c" bullet there.
3. **memory `regional-dataflow-core-epoch`** (P7b banner at head) + **`derivation-counters-state`**
   (the R-final "Fold C" candidate — P7c IS Fold C for the partial-scan path).
4. **`StackSafeNegation.md`** + the join-fold history — the precedent that retired the
   equivalent re-check on the TABLEJOIN path (the argument P7c extends to the partial scan).

## The gap (exact anchors, re-verify at tip)
`BuildMaybeScanPartial` (`lib/ControlFlow/Build/Build.h`, the SOLE LIVE TABLESCAN mint) builds,
for every partial index scan, a `TUPLECMP cmp` as `scan->body` (`Build.h:470-479`) and, in the
per-column loop (`:481-508`), adds an equality pair `cmp->lhs_vars=in_var (requested key value)`,
`cmp->rhs_vars=out_var (scanned row's column)` for each INDEXED (bound) column, then nests the
real body under `cmp->body` via `cb(cmp, true)` (`:517-522`). In codegen, `EmitScan`
(`lib/CodeGen/CPlusPlus/Database.cpp:2778`) emits the `keyed_chain` `First/Next` loop and the
`cmp` body emits an `if (in == out && …) { … }` gate.

**Why it is redundant (the load-bearing correctness fact — CONFIRMED at tip):** the runtime
`Index::First/Next` is FULL-KEY EXACT by contract (`include/drlojekyll/Runtime/Table.h:789-824`:
a slot matches only on `slot.used && slot.hash == hash && slot.key == key`, the generated Key's
memberwise VALUE `operator==` over every key column; `Next` walks the per-key `Add`-chain). The
contract note (`Table.h:799-803`) states outright that generated JOIN codegen — the monotone
TABLEJOIN body AND the differential sections — emits NO per-row re-check "because the probe is
the equality authority, not an approximation." **P7c extends that same argument to the
`BuildMaybeScanPartial` path**, which is the last place still emitting the re-check.

**The structural subtlety (why this is a fold, not a `git rm`):** the `cmp` is NOT only an
equality gate — it is also (a) the PARENT region the real body nests under (`cb(cmp, true)`,
`:517`), and (b) the anchor for `cmp->col_id_to_var[view_col.Id()] = out_var` (`:507`, the
recordization mapping later stages read, per the `NOTE(pag)` at `:503-506`). Retiring the belt
must PRESERVE the body nesting + the `out_var` column bindings while removing the equality pairs.
Likely shapes to weigh in design: (i) for `kPartialKeyHashSeek`, build the `cmp` with ZERO
comparison pairs (an empty/vacuous TUPLECMP — codegen already emits nothing for an empty compare,
so the gate vanishes but the parent + bindings survive) — the SMALLEST diff; (ii) replace the
`cmp` with a `LET`/pass-through region carrying `col_id_to_var`; (iii) drop the `in_cols`/`in_vars`
population that only fed the comparison. NOTE the full-scan arm (`index==nullptr`, zero bound
columns) already builds an EMPTY `cmp` (no indexed columns → no pairs), so it is already
gate-free — P7c changes ONLY the keyed (seek) case.

## The [confirm-then-ground] flow
The cut is chosen, but CONFIRM the design shape (i/ii/iii above) with the owner as the [OWNER
STOP] once the grounding loop has ranked them. Then run the grounding loop DOCS-ONLY, present the
design + exit gate, and STOP for the execution go/no-go. On green-light, execute as one coherent
commit, then update CLAUDE.md + memory + write the session-26 seed.

## Method — the grounding loop via WORKFLOWS (opus + sonnet), the loop that landed P2–P7b
Thin orchestrator; sequential single-phase workflows; watch the `(await parallel(...)).filter`
precedence. DOCS-ONLY until green-lit.
1. **As-is pseudocode** (sonnet) — re-verify every anchor at tip: the `Build.h` mint + `cmp`
   construction, `EmitScan`'s `keyed_chain` arm + how a TUPLECMP body lowers, the `Table.h`
   First/Next contract. Enumerate EVERY consumer of `cmp->col_id_to_var` / the scan's `in_cols` /
   `in_vars` so the retire preserves them.
2. **Design-goal diffs** (opus) — rank shapes (i/ii/iii); pick the minimal one that (a) removes
   the equality gate for `kPartialKeyHashSeek` ONLY, (b) preserves body nesting + `col_id_to_var`
   + `out_vars`, (c) leaves the full-scan and (dead) join-pivot paths untouched. Each diff carries
   a DISCRIMINATING STRUCTURAL gate.
3. **Adversarial refuter panel + throwaway-worktree spike** (opus) — the DECISIVE de-risk, because
   P7c MOVES codegen. The spike: apply the diffs, build, run the FULL suite, and measure (a) every
   `.stdout`/oracle/monotone/behavioral golden BYTE-IDENTICAL (answer-invariance — the removed
   check was always-true; a MOVED answer golden is a miscompile, STOP), (b) the exact set of
   `.h`/`.cpp`/`.ir` seek carriers whose codegen shrank, (c) that the P7b V-PLAN-HONEST belt still
   never fires. Refuters must attack: does any `kPartialKeyHashSeek` scan carry a bound column the
   index key does NOT cover (→ a real filter, not redundant)? can `keyed_chain` ever be false at
   codegen while `plan_kind==kPartialKeyHashSeek` (→ the re-check WAS load-bearing)? does dropping
   `cmp` orphan any `col_id_to_var` reader downstream? Grep the corpus for a scan whose
   `IndexedColumns() ⊊ InputVariables()`.
4. **IR desired states** (predict-then-verify) — pin the BEFORE/AFTER generated cursor on a named
   seek carrier (e.g. `negate_1`: the `if (@A == r.field) { … }` gate GONE from the `keyed_chain`
   loop body) + the `.ir` scan body. Decide precisely which goldens MOVE.

Model tiering (memory `subagent-model-tiering`): sonnet = anchors + baseline dumps; opus =
design, refuter panel, IR states.

## The discriminating exit gate (P7c MOVES codegen — this is the point)
- OptDiff **SUITE: PASS**; ctest **5/5**.
- EVERY `.stdout` / oracle / monotone / behavioral golden BYTE-IDENTICAL across all 4 modes (the
  retired check was always-true; answer-invariance is the correctness proof). A moved answer
  golden is a MISCOMPILE, never a bless.
- The seek carriers' generated code SHRINKS: the `kPartialKeyHashSeek` `keyed_chain` loop body no
  longer emits the `if (key == scanned) { … }` equality gate. Pin it with a NEW/updated `.h` (and
  optionally `.ir`) golden on a directed carrier (`negate_1` is the natural one — its P7b
  `plan=partial-key-hash-seek` scan is the witness; a recursive seek carrier like
  `tc_nonlinear_diff` or `transitive_closure_diff` additionally proves the fixpoint path).
- The P7b V-PLAN-HONEST belt is unchanged and still never fires.
- The full-scan (`kFullScanFilter`) and statically-dead join-pivot paths are BYTE-UNCHANGED.

## Gotchas (carried)
- clangd diagnostics in this repo are NOISE (no include paths) — trust the real build only.
- macOS bash 3.2 / zsh word-splitting: use `${=var}` when a var holds multiple CLI args.
- `.dr` files are ASCII-ONLY.
- Run builds/suite SILENT on success; the full OptDiff suite takes ~3–4 min — run it BACKGROUNDED
  and await the notification. Never run bench concurrently with the suite.
- Bless goldens ONLY via `runall.sh --bless <workroot> [filter]` after reviewing the delta (never
  through a symlink golden; never to make a red case green). P7c's answer goldens must NOT move —
  only the codegen (`.h`/`.cpp`/`.ir`) carriers you deliberately add/re-bless.
- The `Index::First/Next` full-key-exact contract (`Table.h:789-824`) is the whole correctness
  argument — if that contract is ever weakened (e.g. a hash-only match), P7c is unsound; the
  grounding loop must re-verify it at tip and the design should NOT silently depend on it without
  a comment pointing back to it.
- P7c is Fold C for the partial-scan path; the join path already retired its twin. Keep the two
  arguments explicitly linked in the commit + CLAUDE.md so the pattern is legible.
