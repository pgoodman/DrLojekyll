# KeyedInstances §5 re-verification — XHIGH CONSOLIDATED RECORD (tenth run)

> **[ORCHESTRATOR ADJUDICATION NOTE — 2026-07-18, before commit.]**
> E-61's attribution below ("seed §5(A) AND t2-dump-spec.md §1.2") is
> HALF right: the ledger §5 never spells the ForEachView kind list
> (grep `compares` over KeyedInstances.md = no hit in §5), and all
> four t2-desired-*.md artifacts already carry the CORRECT
> negations→compares order. The transposition was SPEC-ONLY (v2
> §1.2:56-57), fixed in spec v3. Everything else stands as
> adjudicated; ledger §6 is the binding errata record.

Errata continue from **E-61** (ledger closed at E-60). Every ruling below was
re-read from code THIS session; `RULED:` lines carry the grep/sed evidence inline.
Where lane/verifier/seed/critic disagreed, the code is the tiebreaker.

Bottom line: §5(A)+(B) are substantially CODE-ACCURATE. The (F) emission-neutrality
and T2b.0 emission-neutrality arguments SURVIVE. **One load-bearing seed+spec defect**
(the ForEachView compares↔negations transposition, E-61) would mis-order a T2a golden
if transcribed from prose. The owner-decision recommendations STAND with two sharpenings.

---

## 1. §5 SEED CLAIM VERDICT TABLE

### (A) Block — pipeline + substrate + DeltaRel + harness

| Claim (seed §5A) | Verdict | Evidence (re-read this session) |
|---|---|---|
| Main.cpp stream globals gCxxOutDir:52, gDOT/gDR/gIR :54-56; flag arms :268/:282/:299/:315 | CONFIRMED | Main.cpp:52,54-56; arms exact |
| Drain order: Query::Build(:60-61)→Program::Build(:66-67)→gIR(:74-77)→cpp(:79-101)→gDOT(:106-109) LAST; gDR in ProcessModule :125-129 (parsed module) | CONFIRMED | Main.cpp:60-129; -dot-out deferred per :103-105 comment (table-id annotation) |
| Build.cpp tail: ConnectInsertsToSelects 2564, ApplyDemandTransform 2576, Optimize gate `if(optimize)` 2584 wraps only 2585, LinkViews 2595, IdentifyInductions 2597 (OUTSIDE gate), FinalizeDepths 2602, FinalizeColumnIDs 2603, TrackDiff(final) 2604, TrackConstAfterInit 2608, BuildEquivalenceSets 2626, Stratify 2627 | CONFIRMED | Build.cpp:2564-2627 exact |
| 2598-2627 view-neutral → det_seq stays dense 0..N-1 over live views through Program::Build | CONFIRMED (with the const_to_vc caveat, see E-63) | no Create/kill/Sort/is_dead= in FinalizeDepths/ColumnIDs/Diff/ConstAfterInit/BuildEquivalenceSets/Stratify; Program::Build takes const Query&, writes only table_id |
| det_seq Query.h:472; stamped at TWO code sites (CSE Optimize.cpp:285-287; IdentifyInductions Induction.cpp:142-144); re-entrant re-stamp; DeterministicOrder() asserts stamped; ForEachView skips dead (density mechanism) | CONFIRMED | Query.h:472; Optimize.cpp:285-287; Induction.cpp:142-144,453-456; Query.cpp:430-433 |
| OrderViewsDeterministically Sort()==Hash()→first-col-id→det_seq TOTAL; HashInit = KindName+2 del-flags+col-count (pointer-free) | CONFIRMED | Induction.cpp:112-126; View.cpp:120-122, 417-427 |
| ForEachView kind order (as WRITTEN in seed): `…merges, compares, negations, inserts` | **WRONG → E-61** | Query.h:1196-1204 & 1248-1258: code is `…merges, NEGATIONS, COMPARES, inserts`. Seed transposes. |
| OrderedViewMap (ControlFlow Program.h:48/:55) under DeterministicOrder; 5 INDUCTION maps :1832-1853 | CONFIRMED (path = lib/ControlFlow/Program.h) | Program.h:48-55, 1832-1853 |
| RESIDUAL: "nothing pointer-ordered reaches emission" | **CORRECTED → E-63** | Build.cpp:501 const_to_vc iterates unordered_map into ALL-CONSTS tuple column layout, no sort-first. Corpus-scoped true; unconditionally false. |
| DeltaRel: BuildStratumPhases:2049; BuildDRInventory 2081→DeriveDRStrata 2088→ValidateDRInventory 2095→ValidateDROps 2096→LinearizeAndValidateDRFlow 2097 | CONFIRMED | Stratum.cpp:2049,2081,2088,2095,2096,2097 |
| key_of={lead,stratum,band,op_table_id,sign,ctor}; op_table_id = reinterpret_cast<uintptr_t>(TABLE*) :3387-3394 (the C-1 latent) | CONFIRMED | DeltaRel.cpp:3424-3456; 3387-3394 (return cast at 3394) |
| pinned_order consumers "VALIDATORS ONLY (:3804-3981)"; emission unaffected | **CORRECTED → E-62** | :3804-3815 is the body_ops/output_ops SUBSTRATE-fill loop (NOT a validator); validators proper start ~:3817. body_ops/output_ops are "populated, never read" (DeltaRel.h:592) → emission-neutral CONCLUSION HOLDS. |
| pinned_order = "stable-sort by key" | CORRECTED (cosmetic, rolls into E-62) | It is a Kahn topo-sort whose ready-set tie-break is key_less (DeltaRel.cpp:3702-3739), not a flat std::stable_sort. Seed's own linearizer block says "checked linearization" correctly. |
| context.dr_flow = move (2166), sole surviving handle; emission reads mint order NOT pinned | CONFIRMED | Stratum.cpp:2166 move; :2167 binds `flow=*context.dr_flow`; Lower* walk construction-order vectors |
| Inventory: 15 DROpKind, 10 Pred, 10 EffKind, 14 VecRole, no id fields (identity=index) | CONFIRMED | DeltaRel.h DROpKind 113-138, Pred 94-105, EffKind 73-84, VecRole 52-67 (verify-detsub counted) |
| Harness: diffrun.sh pure 4-mode; 11 always-diagnostic names (:231); kvindex_1 mode-split; run_oracle .batches-guarded; grep FAIL\|DIVERGE\|EXPECT-ERROR\|MISSING (:284); --bless sole write path; 169 cases, 158 stdout + 52 oracle + 52 monotone | CONFIRMED | runall.sh:231 (name list),284; diffrun.sh; counts verified |

### (B) Block — the five diffs' load-bearing premises

| Premise | Verdict | Evidence |
|---|---|---|
| T2a: `-df-out` BB dump drained beside -dot-out (post-Program::Build so TableId populated); density-asserted; zero churn | CONFIRMED sound (drain placement load-bearing; density assert needs hardening, see §3) | Main.cpp:106-109 drain slot; -df-out flag does NOT yet exist (new arm needed) |
| T2b.0: harden op_table_id pointer→table id; emission-neutral (validator-only consumers) | CONFIRMED sound; **null path is a required amendment (E-64)** | grep: pinned_order/body_ops/output_ops/key_less zero emission readers; t->id in scope (Program.h:129); op_table_id null-capable |
| T2b: `-deltarel-out` at post-stash validate-exit reads *context.dr_flow | CONFIRMED (drain at/after :2167, not the :2166 move statement) | Stratum.cpp:2166 move, :2167 readable ref |
| T3: `.irgold` sidecar + goldens, runall.sh --one only, verdict tokens hit the grep, suite stays 169 | CONFIRMED sound; layout-vs-"matching run_oracle" mismatch (E-65) | runall.sh helper slot after :227, invoke beside :248; bless mirror after :92 |
| P1: PassPolicy at two Build sites, legacy flags exact aliases, byte-identity | CONFIRMED exact (2 sites :61/:67, 2 flag arms :335/:340; no third consumer) | widen prediction to all-4-modes (§4) |

---

## 2. FINAL ERRATA (numbered from E-61, deduped across all reports)

Only REAL code-vs-artifact discrepancies get a number. Stale-anchor / cosmetic
slips roll up into E-66 unless load-bearing. Classification: SEED / LANE / SPEC /
CRITIC defect. MISDIRECTS = would have sent an implementer wrong.

**E-61 — SEED + SPEC defect (LOAD-BEARING; MISDIRECTS T2a).**
The ForEachView / per-kind DefList order is written `…merges, compares, negations,
inserts` in BOTH seed §5(A) substrate block AND t2-dump-spec.md §1.2. The code order
is `…merges, NEGATIONS, COMPARES, inserts`.
RULED: `sed -n '1176,1214p' lib/DataFlow/Query.h` → negations pushed before compares
(non-const overload); const overload Query.h:1248-1258 concurs (negations@1248,
compares@1258); Link.cpp:457-460 reset_all concurs. The lane report (lane-pipeline §4)
has it RIGHT; the seed/spec prose is transposed. det_seq NUMBERING is unaffected
(stamped in true code order), so the density assert will NOT catch a hand-transcribed
golden with the blocks swapped. FIX: swap to `…merges, negations, compares, inserts`
in seed §5(A) and spec §1.2; check the four DRAFT-PENDING-REVISION t2-desired-*.md
artifacts for the same swap during their revision pass. Dedup: verify-pipeline EC-1,
verify-detsub ERRATA-1, verify-deltarel (implicit K1) — all the same finding.

**E-62 — SEED + SPEC defect (precision; NOT misdirecting the conclusion).**
"pinned_order consumers are VALIDATORS ONLY (:3804-3981)" over-narrows by one loop.
RULED: `sed -n '3804,3815p' lib/DeltaRel/DeltaRel.cpp` → :3804-3815 is the
body_ops/output_ops SUBSTRATE-population loop (pushes op ids into DRRound bodies),
NOT a validator; validators proper begin ~:3817. The load-bearing conclusion HOLDS:
body_ops/output_ops are "R2+ SUBSTRATE (dead-but-alive): populated, never read"
(DeltaRel.h:592) with ZERO readers repo-wide, so the pointer tie-break does not reach
emission. FIX: restate as "consumers are the validators (:3817-3981) plus the never-read
body_ops/output_ops substrate loop (:3804-3815) — none emission-reaching." Also fold the
"stable-sort by key" wording (§5A line 79) → "Kahn linearization tie-broken by key_less."
Dedup: verify-pipeline EC-2, verify-detsub ERRATA-2, verify-deltarel EC-2.
**T2b.0-critical rider:** if any future R2+ lowering ever READS body_ops/output_ops, the
table_id tie-break becomes emitted bytes and T2b.0's emission-neutrality is VOIDED — the
T2b.0 implementer must re-grep body_ops/output_ops readers at implementation time.

**E-63 — SEED defect (scope over-statement; latent, corpus-safe).**
§5(A) RESIDUAL "nothing pointer-ordered reaches emission" is unconditionally false.
RULED: `sed -n '499,507p' lib/DataFlow/Build.cpp` → the ALL-CONSTS tuple builder
iterates `context.const_to_vc` (an `unordered_map<QueryColumnImpl*,VarColumn*>`,
decl Build.cpp:83) assigning `col_index++` column positions and `input_columns.AddUse(col)`
in native pointer-bucket order, with NO pointer-free sort first. Fires only on an
all-constants clause body with ≥2 distinct constant columns — nil on the current corpus.
Accurate statement: "nothing pointer-ordered reaches emission ON THE CURRENT CORPUS; one
dormant gap (Build.cpp:501) remains." Does NOT threaten (F) byte-stability today.
Recommend a `col->id` sort before the loop (mirrors the (F) fixes) as future hardening —
NOT part of the T2 path. Dedup: lane-detsub §7 UNPROTECTED, verify-detsub ERRATA-2b.

**E-64 — SEED defect (LOAD-BEARING for T2b.0; MISDIRECTS — crashes).**
The T2b.0 diff prose ("harden op_table_id … to the table's deterministic id") elides the
NULL case. RULED: `sed -n '3386,3395p' lib/DeltaRel/DeltaRel.cpp` → `t` is the first
non-null of six DROp table fields, else `op.fire_table`, which can itself be null; today
`reinterpret_cast<uintptr_t>(nullptr)==0` harmlessly. Op kinds reaching null include
kNegateGate(eager) (sets gate_table not negate_table), kSeedFold, kChainFold,
kPivotAssemble. A bare `return t->id;` null-derefs (exit 139) on nearly every corpus case.
FIX (mandatory): `return t ? t->id : 0u;`. Invariant that keeps null→0 collision-free:
real table ids ≥3 (Program.cpp constant-var ids 0/1/2 precede any table Create), so a null
op can never tie a real table on the table_id field. Dedup: verify-deltarel EC-5,
critic-t2b0 Amendment A, critic-predictions mechanism 2 / A2.

**E-65 — SPEC defect (LOAD-BEARING for T3; MISDIRECTS layout).**
t2-dump-spec.md §3.2 pins the helper write/bless path `$WORKROOT/$NAME/irgold/<surface>.<mode>.out`
and calls it "(nested layout, matching run_oracle)". RULED: run_oracle writes
`$WORKROOT/$NAME/$NAME.oracle` and `$NAME.monotone` — a `$NAME.<kind>` SIBLING dir
(runall.sh:190,210), NOT a bare `<kind>/` subdir. The spec's `irgold/…` is a THIRD layout
matching neither diffrun's `$NAME.$mode` nor oracle's `$NAME.oracle`. An implementer copying
run_oracle verbatim would emit `$WORKROOT/$NAME/$NAME.irgold`, and the --bless mirror must
read whichever is actually chosen. The SEPARATE-dir choice is correct (must not clobber
diffrun's `$NAME.opt` stdout dir), only the "matching run_oracle" justification is wrong.
FIX: either rename to `$NAME.irgold/` for true symmetry, OR drop "matching run_oracle" and
pin the leaf shape explicitly as a deliberate divergence; add `[ -d "$d/…" ]` guard in the
bless arm. Dedup: verify-harness EC-H2, critic-irgold D-A.

**E-66 — cosmetic roll-up (stale/imprecise anchors; NONE load-bearing, NONE misdirect).**
- Spec §3.1 cites flags_of "runall.sh:122-140"; body is 122-134 (line 140 is the call site
  inside expect_diagnostic). RULED: `sed -n '122,134p' runall.sh`. (verify-harness EC-H1,
  critic-irgold.)
- lane-deltarel §5a cited DataTable::Id() at "include Program.h:225"; the DEFINITION is
  lib/ControlFlow/Program.cpp:877 (`return impl->id;`), %table: prints at Format.cpp:94.
  RULED: `grep -n 'DataTable::Id' lib/ControlFlow/Program.cpp` → 877; the id== %table: id
  claim is correct, only the citation was wrong. (verify-deltarel EC-3.)
- Seed §5 "UniqueId … Node.h:31" resolves against include/drlojekyll/Util/Node.h:31, and
  "OrderedViewMap … Program.h:48/:55" against lib/ControlFlow/Program.h (not the include
  header). Both correct once the right file is opened. (verify-pipeline EC-3.)
- Seed injection-sites loop ":404-440" undershoots the tail (ends :450) — cosmetic.

### REJECTED erratum candidates (adjudicated FALSE)

- **verify-pipeline EC-4 (H1): "diagnostic names at :246, not :231" — REFUTED.**
  RULED: `sed -n '228,249p' runall.sh` → :230 is `case $NAME in`, :231 is the 11-name
  pattern, :246 is the `*)` diffrun.sh default arm. Seed's ":231" is CORRECT; the
  verifier mis-adjudicated. verify-detsub/verify-harness/verify-deltarel all confirm ~:230/:231.
  No erratum.
- **verify-deltarel EC-4 "V-ORDER-CONSISTENT emits token V-OLD-EQUIV(order)" — this is REAL
  but is a code comment/token drift, NOT a seed defect** (the seed does not print validator
  tokens). RULED: `sed -n '3978,3985p' DeltaRel.cpp` → `ValidatorFail("V-OLD-EQUIV(order): …")`
  under a comment block titled V-ORDER-CONSISTENT. Folded into the T2b spec amendment (§3):
  any dumped/censused validator token MUST be harvested from the emitted string literal, not
  the comment title or CLAUDE.md. Not a numbered seed erratum; it is a spec-amendment input.

---

## 3. CRITIQUE ADJUDICATION (per critic report: CONFIRMED / REFUTED + spec amendments)

### critic-density (T2a density assert) — CONFIRMED, sound-with-amendments
- **S4 duplicate-with-gap blind spot: CONFIRMED.** `max == count-1` is necessary but
  not sufficient for a permutation of {0..N-1}. RULED (reasoning, not new code): a
  re-stamp bug producing {0,1,2,2,4} has max=4=count-1 yet is not a bijection. The
  bitset (range `s<N` + no-dup) is the minimal airtight always-on witness.
- **S5 NDEBUG hole: CONFIRMED.** DeterministicOrder()'s stamped-ness `assert` compiles
  out under NDEBUG (Query.cpp:431); the density guard is the ONLY always-on authority in
  release, so it must range-check the raw stamp, not lean on the accessor assert.
- **N==0 underflow: CONFIRMED.** literal `max==count-1` underflows to `~0u` on the empty
  graph; the bitset form is a correct no-op at N==0.
- SPEC AMENDMENT (§1.2): replace the `max == count-1` check with the two-pass seen-bitset
  (pass 1 count N + size; pass 2 `s < N` range-check subsuming the ~0u sentinel, plus
  no-duplicate) — a bijection-onto-{0..N-1} witness, release-safe, N==0-safe. Reject the
  sum/XOR fingerprint (cancellation-prone, verify-density's {0,1,1,3,5} counterexample).

### critic-t2b0 (op_table_id hardening) — core claim CONFIRMED; two amendments
- **Emission-neutrality SURVIVES: CONFIRMED.** RULED: `grep -rn 'pinned_order\|body_ops\|
  output_ops'` outside DeltaRel = one Stratum.cpp:1073 COMMENT only; Lower* walk
  construction-order inventory vectors (Stratum.cpp:1071-1073 disclaims the band key).
- **No validator flips under the id key: CONFIRMED (by reasoning).** Every pos-comparing
  validator (V-RETIRE-AFTER band4-vs-band6, V-LOOP cross-band, V-READY stratum-keyed)
  pairs ops differing in a key field ABOVE table_id, or reads the stratum map — table_id
  is level 4, below lead/stratum/band. V-BAND-HAZARD/V-LINEAR/V-OLD-EQUIV are forward
  by construction under any total key.
- **Null-table break: CONFIRMED (this is E-64).** kNegateGate(eager)/kSeedFold/kChainFold/
  kPivotAssemble reach `t==nullptr`; a bare `t->id` SIGSEGVs corpus-wide.
- SPEC AMENDMENTS (§2.0): (A, MANDATORY) `return t ? t->id : 0u;`. (B) replace the
  justification "any consistent total order is a valid linearization" with the precise
  reason — *the tie-break feeds pinned_order position and same-band edge orientation, but
  emission reads neither and every pos-comparing validator pairs ops differing above the
  table_id level* — the current wording wrongly implies inertness. (C) document the
  invariant "real table id ≥3, null→0 never collides."

### critic-irgold (T3 sidecar) — spine CONFIRMED; four defects, two real
- **D-A layout "matching run_oracle": CONFIRMED FALSE (this is E-65).** run_oracle uses
  `$NAME.oracle` sibling dirs (runall.sh:190,210), not an `irgold/` subdir.
- **D-B retirement over-claim: CONFIRMED.** A single per-run golden byte-compare and the
  N-run allocation sweep measure ORTHOGONAL classes: the golden pins the canonical VALUE
  (catches deterministic regressions the sweep is value-blind to); the sweep proves
  ABSENCE of run-to-run flakiness (which a one-sample-per-run compare cannot — residual
  1/N flakiness becomes an unattributable intermittent red). NOT substitutable.
- **D-C diagnostic/mode-split guard: CONFIRMED.** run_irgold fires beside :248 for every
  arm; a diagnostic-set or kvindex_1(nodf/none) case that carried `.irgold` would
  compile-exit-1 → spurious IRGOLD-FAIL. Guard: `.irgold` only on all-4-modes-clean cases.
- **D-D new flags unlanded: CONFIRMED.** RULED: `grep -n 'df-out\|deltarel-out' Main.cpp`
  → absent; only ir/dr/dot/cpp arms exist. "One compile all surfaces" proven for {h,ir}
  only; T2a/T2b must add peer independent `else if` arms (df in CompileModule; deltarel a
  lib/DeltaRel static global set from Main's loop, drained at the Stratum stash).
- SPEC AMENDMENTS (§3): fix D-A path/justification + add `[ -d ]` bless guard; rewrite
  §3.4 retirement (retire sweep as ROUTINE per-commit step, RETAIN as substrate-change
  acceptance gate); add the diagnostic-guard policy; pin flags_of call form unquoted (SC2046);
  cross-ref the T2b.0 precondition at §3.3's `average_weight (deltarel opt)` carrier line.

### critic-predictions (pre-registered predictions) — all five verdicts CONFIRMED
- **T2a** predictions SOUND (trivially — new off-path surface); real risk is bless-time
  format correctness (A1: eyeball a real `%table:<id>` before blessing — the density
  assert does NOT guard table-id population, only view numbering). CONFIRMED.
- **T2b.0** zero-emission CONFIRMED at source; validators-green PLAUSIBLE/runtime-gated;
  null hazard = E-64. Stale comment DeltaRel.cpp:~3971-3973 ("the band key IS the emission
  driver's walk order") is aspirational — emission walks construction order; correct it
  in-scope while touching the key (A3).
- **T2b** SOUND, contingent on the null-sink being a PRE-GUARD no-op (A5: `if(stream){format}`,
  never format-then-discard) and a §2.3 config-invariance audit (A4: no rendered field may
  read an `#ifndef NDEBUG` member — the trap the (a3) producer ruling already fixed for .df).
- **T3** all four §3.4 predictions CONFIRMED against harness; residual risks are review
  discipline (irgold stricter than permcheck'd stdout — never re-bless to green, A7) and
  sidecar+golden commit atomicity (A6).
- **P1** SOUND under a Main.cpp-only PassPolicy; widen the prediction from "default config"
  to "all four suite modes byte-identical" (A8) since the golden gate exercises all four.
- SPEC AMENDMENTS: adopt critic-predictions' A1–A8 as pre-registered landing gates. All
  confirmed against code this session; none refuted.

### Net spec amendments the t2-dump-spec.md revision needs (actionable)
1. §1.2 — swap kind order to `…merges, negations, compares, inserts` (E-61).
2. §1.2 — replace `max==count-1` density check with the seen-bitset bijection witness
   (range `s<N` + no-dup, two-pass, N==0-safe, release-safe).
3. §2.0 — `op_table_id` hardening: `t ? t->id : 0u` (E-64); precise verdict-neutral
   rationale (E-62/critic-t2b0 B); document real-id≥3 invariant; correct the stale
   emission-order comment (A3).
4. §2.0 — restate pinned_order consumers as validators + never-read substrate loop, and
   add the re-grep-body_ops/output_ops-readers tripwire to the T2b.0 gate (E-62).
5. §2.3 — validator tokens dumped/censused MUST be harvested from the emitted
   `ValidatorFail(...)` string literal (V-OLD-EQUIV(order), NOT the comment title
   V-ORDER-CONSISTENT); run the config-invariance audit on §2.3 fields before any deltarel
   bless (A4).
6. §2.2 — drain the deltarel dump at/after Stratum.cpp:2167 (readable `flow` ref), never
   the moved-from local `dr_flow`; null-sink must be a pre-guard no-op (A5).
7. §3.2/§3.4 — fix the `irgold/` layout justification (E-65); rewrite the sweep-retirement
   claim (retire routine / retain acceptance-gate, critic-irgold D-B); add the diagnostic-set
   guard; pin flags_of unquoted call form; sidecar+golden atomic-commit note (A6);
   irgold-not-re-blessed-to-green discipline note (A7).
8. §3.1 — cite flags_of as runall.sh:122-134 (E-66).
9. §4 — widen P1 prediction to all-four-modes; pin PassPolicy default→(df=true,cf=true) at
   both Build sites (A8).

---

## 4. DECISION-FEEDING DELTA (owner decisions a, a2, a3, b, b2, c, d, e)

**All recommendations STAND. Two sharpenings, no reversals.**

- **(a) / (a2) / (a3) [.df dump surface, config-invariance, producer= NDEBUG-gate]:**
  STAND. The (a3) producer ruling (producer= is `#ifndef NDEBUG`-only, kept out of the
  default .df output) is CONFIRMED as the right config-invariance discipline. SHARPEN:
  extend the SAME audit to the §2.3 deltarel dump fields before any deltarel bless (A4) —
  the ruling that fixed .df has not yet been run on the deltarel surface. No change to (a)/(a2).
- **(b) [T2b.0 → T2b landing order]:** STANDS and is REINFORCED. T2b.0 MUST land before any
  deltarel/`pinned_order`-walking golden is blessed, else the golden bakes pointer order —
  the F-anti-pattern. Confirmed by critic-irgold corollary + critic-predictions T2b mech 2.
- **(b2) [same ordering, spec §5]:** STANDS. Add E-64 (null-guard) as a hard precondition
  of the T2b.0 half — the diff crashes without it.
- **(c) [T3 .irgold harness]:** STANDS with the E-65 layout correction and the critic-irgold
  D-B/D-C/D-D amendments folded in. The sweep is RETAINED as a substrate-change acceptance
  gate (do NOT let the .irgold goldens retire it wholesale — that was the over-claim).
- **(d) [P1 PassPolicy]:** STANDS. Prefer the Main.cpp-only PassPolicy path (byte-identity
  trivial); widen the pre-registered prediction to all-four-modes (A8).
- **(e) [implementation order: artifacts → T2a → T2b.0 → T2b → T3 → P1]:** STANDS. Add:
  the four DRAFT-PENDING-REVISION t2-desired-*.md artifacts MUST be swept for the E-61
  compares↔negations transposition during their revision pass (they were written from the
  same prose the seed carries).

---

## 5. LOUD FINDINGS (would have misdirected the T2a/T2b.0/T2b/T3/P1 implementer)

1. **[T2a, HIGH] E-61 compares↔negations transposition.** An implementer hand-blessing a
   `.df` golden from the seed/spec prose emits compare-blocks before negation-blocks. The
   density assert (§1.2, even hardened) is ORDER-INDEPENDENT and will NOT catch it. ANY
   `.df` golden must be diffed against a code-derived ForEachView traversal, never the prose.

2. **[T2b.0, HIGH] E-64 null-table deref.** The seed's one-line T2b.0 diff says "harden to
   the table's deterministic id." A literal `return t->id;` null-derefs (exit 139) on nearly
   every corpus case (eager negate / seed fold / chain fold / pivot assemble reach null).
   MUST be `t ? t->id : 0u`. This is the single most likely way T2b.0's "full-suite
   byte-identity" prediction fails at implementation.

3. **[T2b.0, MED] Emission-neutrality depends on body_ops/output_ops staying DEAD (E-62).**
   The pointer tie-break reaches emission through NOTHING today only because
   body_ops/output_ops have zero readers. The T2b.0 implementer must RE-GREP those readers
   at implementation time (not trust this snapshot) — a future R2+ lowering that reads them
   voids the neutrality argument.

4. **[T2b, MED] Validator-token spelling drift.** The final monotonicity validator emits
   `V-OLD-EQUIV(order)` (DeltaRel.cpp:3982) but its comment/CLAUDE.md call it
   V-ORDER-CONSISTENT. Any T2b census/dump or T3 golden capturing a validator token must use
   the EMITTED string literal, harvested from `ValidatorFail(...)`, not the comment title.

5. **[T2b, MED] Config-invariance of the deltarel dump is UN-AUDITED (A4).** The (a3) ruling
   fixed the .df producer= NDEBUG trap; the equivalent audit has NOT been run on the §2.3
   deltarel fields. A `deltarel opt` golden blessed under debug could fail under the release
   preset if any rendered field reads an `#ifndef NDEBUG` member. Audit before blessing.

6. **[T2b, MED] Drain the moved-from graph.** Read `*context.dr_flow` via the `flow` ref
   bound at Stratum.cpp:2167 — the local `dr_flow` is moved-from at :2166 (dumping it = UB /
   empty graph). Drain in the 2167–2199 window (before the no-phase-work early return at
   :2200-2203) or unconditionally, so no-phase programs still emit.

7. **[T2b/T2b.0, MED] Null-sink MUST be a pre-guard no-op (A5).** The deltarel hook fires
   for all 169 cases × 4 modes every Program::Build; `auto s=format(); if(strm) strm<<s;`
   would tax every suite compile and Q5. Enforce `if(stream){format...}` at code review —
   the bench cannot reliably see sub-noise off-path work.

8. **[T3, HIGH] E-65 layout is NOT "matching run_oracle."** An implementer copying run_oracle
   verbatim writes `$WORKROOT/$NAME/$NAME.irgold`, not the spec's `$WORKROOT/$NAME/irgold/`.
   The bless mirror must read whichever is actually chosen; pick one and pin it.

9. **[T3, MED] Suite PASS/FAIL is grep-over-verdicts, NOT exit codes (lane-harness L19).**
   xargs's exit status is discarded; a new IRGOLD verdict token MUST contain
   FAIL/DIVERGE/EXPECT-ERROR/MISSING or a red arm SILENTLY passes the suite. The proposed
   IRGOLD-FAIL/-MISSING/-DIVERGE all satisfy this — confirmed against the runall.sh:284 grep.

10. **[T3, MED] Diagnostic/kvindex_1 arms exit-1 inline and SKIP run_irgold on failure
    (lane-harness L15).** A `.irgold` attached to a diagnostic-set or mode-split case never
    gates on a failing compile, and worse a compiling-but-diagnostic surface yields spurious
    IRGOLD-FAIL. Policy: `.irgold` only on all-4-modes-clean cases (demand_tc_witness,
    symrec_tie_1 qualify).

11. **[T3, MED] The sweep is NOT subsumed (critic-irgold D-B).** A single per-run golden
    compare pins the canonical VALUE but cannot prove absence of run-to-run flakiness. Retain
    the N-run allocation sweep as the substrate-change acceptance gate; retire it only as the
    routine per-commit step.

12. **[latent, LOW] E-63 Build.cpp:501 const_to_vc.** One live unprotected pointer-order
    iteration into ALL-CONSTS tuple column layout survives; nil on the current corpus but the
    seed's "nothing pointer-ordered reaches emission" is unconditionally false. Not on the T2
    path; recommend a `col->id` sort as future (F)-style hardening.

13. **[P1, LOW] Widen byte-identity to all-4-modes (A8).** The §4 prediction covers only the
    default (opt) config; the golden gate exercises all four (df,cf) bool pairs. Pin
    PassPolicy's default→(df=true,cf=true) at both Build sites (:61/:67) and map the four
    modes to today's four flag combinations, or a mode silently churns.

### Verifier self-errors caught (house adjudication)
- verify-pipeline mis-adjudicated the diagnostic-names line as :246 (H1 "CORRECTED").
  RULED FALSE: seed's :231 is correct (:246 is the diffrun.sh default arm). Do not carry
  that "correction" into the artifacts.
- verify-deltarel/verify-detsub flagged several lines NOT RE-VERIFIED (Procedure.cpp:314
  LowerCommitSweeps, Stratum.cpp:1602 LowerGroupUpdate, enum cardinalities). These are
  non-load-bearing for the emission-neutrality conclusion; the deltarel lane and
  verify-detsub independently confirmed the cardinalities. No open exposure.
