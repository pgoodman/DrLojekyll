# KeyedInstances §7 re-verification — CONSOLIDATED RECORD (fleet7, eleventh fleet run)

XHIGH consolidator. Tip 35b89aab (branch keyed-instances). Errata continue at E-67.
Inputs: §7 (KeyedInstances.md:903-1047) + §5/§6 context; 4 seed-unread lanes
(substrate, linearizer, mainwiring, harness) + 4 seed-read adversarial verifiers.
Every adjudication below re-checked AT CODE by this consolidator with file:line
evidence (the four disputed anchors were re-Read, not taken on report authority).

STATUS: COMPLETE.

---

## 0. OVERALL VERDICT

**§7's core HOLDS.** SEED-HOLDS-WITH-ERRATA. Every load-bearing mechanism claim in
A.1 (substrate + pipeline tail), A.2 (linearizer key + pointer-free property +
E-62 tripwire), A.4 (harness) is CODE-CONFIRMED at 35b89aab. All four verifiers
independently reached SEED-HOLDS-WITH-ERRATA. The E-62 tripwire (the one
load-bearing runtime check for T2b) was re-grepped by FOUR agents + this
consolidator: **CLEAN** — zero emission-path readers of pinned_order/body_ops/
output_ops.

**The T2a path is UNBLOCKED.** Every ASSUMPTION the (B) T2a emitter pseudocode
makes about the code (det_seq stamp is total+dense over live views after
IdentifyInductions; ForEachView traversal order = stamp order; TableId() populated
during Program::Build; the -dot-out drain-site clone point) is confirmed. The
design choices in (B) are out of scope (not adjudicated — per task).

**NO STOP-the-line finding.** Every erratum is cosmetic or a stale line-anchor;
none changes a byte contract, none misdirects a T2a/T2b/T3/P1 byte-diff gate.

---

## 1. ADJUDICATION OF ERRATA CANDIDATES + DISAGREEMENTS

### The four disputed anchors — re-checked AT CODE by this consolidator

Re-Read DeltaRel.cpp:3394-3402, :3808-3852, :3990-4003; DeltaRel.h:583-596;
Build.cpp (both). Ground truth:

- **Substrate-fill loop**: `for (unsigned oi : flow.pinned_order)` at
  **DeltaRel.cpp:3813**, body to **:3824**; the body_ops push is :3817, output_ops
  push :3821. (Confirmed by direct Read.)
- **Validators block**: banner comment :3826-3828; first ValidatorFail (V-LINEAR)
  at :3834 (size guard at :3833); last (V-OLD-EQUIV(order)) at :3995; the
  validator region runs **:3830-4000**; `LinearizeAndValidateDRFlow` closes at
  **:4001**. (Confirmed by direct Read + grep.)
- **op_table_id chain**: DeltaRel.cpp:3394-3402 — five ternary-guarded candidates
  (`table_op_table, product_table, agg_table, negate_table, ingest_table`) with
  `fire_table` as the final `:` else. SIX table fields total, fire_table the 6th.
  Form `t ? uintptr_t(t->id) + 1u : 0u` at :3401. (Confirmed by direct Read.)
- **const_to_vc**: exists ONLY in `lib/DataFlow/Build.cpp` (decl :83, clear :53,
  iterate :501). `lib/ControlFlow/Build/Build.cpp:501` is an unrelated
  `const DataTable table(model->table);` line. Ambiguity is REAL. (Confirmed by
  grep + sed.)

### Candidate ledger (as reported) → adjudication

| Reporter | Candidate | This consolidator's ruling |
|---|---|---|
| verify-substrate | E-67a: A.2 linearizer consumer line ranges drifted/transposed | **ACCEPT as E-67** (STALE-ANCHOR). All 4 verifiers independently flagged the same drift; code confirms substrate loop :3813-3824, validators :3830-4000. Seed's `:3817-3981`=validators lands :3817 on the body_ops push. |
| verify-linearizer | E-67a (same anchor drift, incl. Kahn :~3739→:3748) | **MERGED into E-67** — same defect, broader (also names the Kahn loop tail). |
| verify-mainwiring | E-67a: Build.cpp:501 filename under-qualified (DataFlow vs ControlFlow) | **ACCEPT as E-68** (SEED-DEFECT cosmetic). Distinct defect (E-63 anchor, not A.2). Both files have a :501; only DataFlow holds const_to_vc. |
| verify-harness | E-67a (anchor drift — same as E-67) + E-67b: "six fields else fire_table" double-counts | Anchor drift MERGED into E-67. The wording candidate **ACCEPT as E-69** (SEED-DEFECT cosmetic/NUANCE). |
| verify-linearizer | E-67b: lane-linearizer.md §3 mis-cites DeltaRel.h:584-586 for the "never read" label | **RECORD-ONLY (not a ledger erratum).** The label lives at :592 (above output_ops); :590 is body_ops decl (comment: "OWN pinned order / band template"); the "populated, never read" text is at :584-586 for body_ops AND :592 for output_ops — see ruling below. Working-artifact nit, not a §7 defect. Logged as REJECTED-as-ledger-erratum (D4). |
| verify-mainwiring | lane §5.2 "DeltaRel.h:7 states internal" → actually :8, text "lib/DR" | **RECORD-ONLY** lane nit; not a §7 defect. |

### D4 disagreement resolved (verify-linearizer vs the DeltaRel.h reality)

verify-linearizer claimed lane-linearizer §3's cite of DeltaRel.h:584-586 for the
"never read" comment is wrong and the label is "only" at :592. **AT CODE**
(DeltaRel.h:583-596): the "R2+ SUBSTRATE (dead-but-alive): populated ... never
read" comment appears TWICE — at **:584-586** (attached to `body_ops`, decl :590)
AND at **:592** (attached to `output_ops`, decl :595). So lane-linearizer §3's
:584-586 cite is CORRECT for body_ops, and verify-linearizer's "only :592" claim is
itself slightly off (it missed the :584-586 instance). Net: NO ledger defect either
way — both fields carry the "never read" label, the emission-neutral verdict is
unaffected. verify-linearizer's proposed E-67b (lane mis-cite) is **REJECTED** as a
non-defect. Recorded here per house precedent (rejected candidates are recorded).

---

## 2. FINAL ERRATA (E-67 .. E-69)

### E-67 — STALE-ANCHOR (cosmetic-to-mildly-load-bearing). §7 A.2:935-941 (copied verbatim from §5(A), inherited from §6 E-62; never re-floated after the T2b.0 landing 97d02111 shifted surrounding code).
- **Wrong fact (seed):** `pinned_order = Kahn ... (:~3702-3739)`; `consumers =
  validators (:~3817-3981) + the NEVER-READ body_ops/output_ops substrate loop
  (:~3804-3815)`.
- **Corrected fact (35b89aab):** Kahn ready-set select loop **:3731-3748**
  (min-key argmin, not stable_sort); body_ops/output_ops substrate-fill loop
  **:3813-3824** (pushes at :3817 / :3821); validators **:3830-4000** (V-LINEAR
  size guard :3833 / fail :3834 … V-OLD-EQUIV(order) :3995; fn closes :4001).
  The seed's `:3817` (labelled "validators") is actually the body_ops push INSIDE
  the substrate loop — the two spans are transposed at the boundary.
- **What future diff it misdirects:** a T2b implementer eyeballing ":3817-3981 =
  validators" to harvest ValidatorFail token literals lands IN the substrate loop,
  not the validators. NOT load-bearing for the E-62 tripwire itself (that is a
  NAME re-grep, not a line lookup) nor for any byte contract.
- Re-float: Kahn :3731-3748; substrate loop :3813-3824; validators :3830-4000.

### E-68 — SEED-DEFECT (cosmetic; filename under-qualification). §7 A.1:920-921 ("E-63's const_to_vc dormant gap") + §5 (KeyedInstances.md:~675) "Build.cpp:501".
- **Wrong fact:** bare `Build.cpp:501` is ambiguous — TWO files carry a line 501.
- **Corrected fact:** the const_to_vc gap is **lib/DataFlow/Build.cpp:501**
  (`for (const auto &[col, vc] : context.const_to_vc)` in `AllConstantsView`,
  decl :83). `lib/ControlFlow/Build/Build.cpp:501` is an unrelated
  `const DataTable table(model->table);`.
- **What future diff it misdirects:** an implementer chasing the E-63 latent to the
  ControlFlow Build.cpp:501 lands on the wrong line. NOT load-bearing (E-63 is
  dormant — needs an all-constants clause head with ≥2 distinct constant columns,
  nil on the corpus). One-word fix: prefix `lib/DataFlow/`.

### E-69 — SEED-DEFECT (cosmetic/NUANCE; count wording). §7 A.2:927-928 comment "t = first non-null of six DROp table fields else fire_table" (same phrasing at §5(A) and E-64).
- **Wrong fact:** "six ... fields else fire_table" double-counts — reads as 6 + a
  7th (fire_table). The chain is FIVE ternary-guarded candidates
  (table_op_table, product_table, agg_table, negate_table, ingest_table) whose
  final `:` else IS fire_table — the 6th and last field.
- **Corrected fact:** "first non-null of five DROp table fields else fire_table
  (six total)". The +1-shift null-sentinel logic (`t ? uintptr_t(t->id)+1u : 0u`,
  :3401) is UNAFFECTED and confirmed correct.
- **What future diff it misdirects:** none functionally; a T2b implementer counting
  fields for the -deltarel-out %table:<id> mapping could momentarily mis-count.
  Purely cosmetic.

### REJECTED candidates (recorded per house precedent)
- **REJECT (verify-linearizer's proposed lane mis-cite E-67b):** the "populated,
  never read" comment IS at DeltaRel.h:584-586 (body_ops) as lane-linearizer cited,
  AND repeated at :592 (output_ops). Not a defect; not a ledger erratum. See §1 D4.
- **NON-ISSUE (verify-mainwiring lane nit):** lane §5.2 "DeltaRel.h:7" → :8 "lib/DR"
  is a working-artifact anchor drift, not a §7 defect. Not laddered.
- **NON-ISSUE (verify-mainwiring §1.3 wording nit):** "main returns EXIT_FAILURE" →
  `code` initialized to EXIT_FAILURE; mechanism (clean fail-before-compile) correct.

---

## 3. PER-CLAIM VERDICT TABLE (§7 core blocks)

### A.1 — Compile pipeline + determinism substrate
| Claim | Verdict | Evidence |
|---|---|---|
| §5(A) main(argv)/Query::Build tail/substrate blocks stand verbatim | CLEAN | Main.cpp:46-56/74-77/106-109/125-129; Build.cpp:2564-2627 (all 4 verifiers + lane-substrate §1/§2) |
| E-61 kind order (…merges, NEGATIONS, COMPARES, inserts) | CLEAN | Query.h:1196/1199/1202/1205 (const :1248/1253/1258/1263) |
| det_seq stamp = EXACTLY TWO sites (CSE :285-287, IdentifyInductions :142-144), IdentifyInductions LAST | CLEAN | Optimize.cpp:285-287; Induction.cpp:142-144; every post-stamp pass view-neutral (lane-substrate §2) |
| E-63 const_to_vc dormant gap "recorded in place" | CLEAN (mechanism) / ERRATUM E-68 (filename) | lib/DataFlow/Build.cpp:501 |
| E-63 const_to_vc referenced as "Build.cpp:501" (bare) | ERRATUM E-68 | ambiguous basename |

### A.2 — DeltaRel linearizer (post-T2b.0)
| Claim | Verdict | Evidence |
|---|---|---|
| key_of = {lead,stratum,band,op_table_id,sign,ctor} | CLEAN | Key struct :3431-3438; key_of :3439-3463; key_less :3464-3471 |
| op_table_id = t ? uintptr_t(t->id)+1u : 0u, first non-null of six table fields | CLEAN (form) / ERRATUM E-69 ("six ... else fire_table" count wording) | :3394-3402 |
| null sentinel 0 disjoint by construction (survives -first-id wrap to id 0) | CLEAN | :3385-3393 rationale + +1 shift :3401 |
| pinned_order = Kahn over non-loop-carried edges, key_less ready tie-break | CLEAN (mechanism) / ERRATUM E-67 (anchor :~3702-3739 → :3731-3748) | loop-carried skip :3713-3716; argmin loop :3731-3748 |
| consumers = validators + never-read body_ops/output_ops substrate loop; EMISSION READS NEITHER | CLEAN (mechanism) / ERRATUM E-67 (span anchors transposed) | substrate loop :3813-3824; validators :3830-4000; E-62 tripwire grep CLEAN (×5) |
| whole key pointer-free → pinned_order fit for golden bytes (T2b precondition) | CLEAN | uintptr_t holds table id+1, not a pointer; all other fields unsigned/int/ctor-index |
| E-62 tripwire: re-grep body_ops/output_ops readers before trusting | CLEAN (satisfied) | zero readers repo-wide (grep run by 4 verifiers + consolidator) |

### A.4 — Harness
| Claim | Verdict | Evidence |
|---|---|---|
| 169 cases; 158 stdout + 52 oracle + 52 monotone goldens | CLEAN | ls globs (lane-harness §4; all verifiers) |
| --one owns all golden policy | CLEAN | diffrun.sh oracle/irgold-agnostic; run_oracle + dispatch + --bless in runall.sh |
| :231 diagnostic names (11); :284 summary grep | CLEAN | runall.sh:231 (11-name pattern), :284 FAIL\|DIVERGE\|EXPECT-ERROR\|MISSING |

### (B) T2a emitter pseudocode — ASSUMPTIONS ABOUT THE CODE (design choices out of scope)
| Assumption | Verdict | Evidence |
|---|---|---|
| ForEachView traversal order == det_seq stamp order (PASS 2 shares the traversal) | CLEAN | stamp IS the ForEachView walk (Optimize.cpp:286 / Induction.cpp:143); const overload = per-kind DefList order |
| det_seq total + dense over LIVE views after IdentifyInductions (bijection witness: s<N, no dup) | CLEAN | dead views skipped (Query.h:1209-1213), dense over live; totality assert Induction.cpp:124-125; sentinel ~0u Query.h:472 |
| det_seq stamp is the LAST stamp before dump (no later pass re-stamps or adds/removes views) | CLEAN | IdentifyInductions is last stamp; all post-stamp passes view-neutral (lane-substrate §2) |
| TableId() populated during Program::Build → drain must be post-Build, beside -dot-out :106-109 | CLEAN | SetTableId at Data.cpp:252 (in Program::Build); DOT drain post-Build Main.cpp:103-109 |
| A second DataFlow text format can coexist with the DOT operator<< (named free fn / tag struct) | CLEAN | operator<<(OutputStream&,Query) taken by DOT (Format.h:11); named-fn precedent = GenerateDatabaseCode |
| CanReceiveDeletions-derived class, Stratum()/InductionGroupId()/InductionDepth() readable off the Query handle | CLEAN | DOT dump already reads these (Format.cpp:41ff) post-Build |

UNVERIFIED (none load-bearing for the T2a UNBLOCK): the exact BYTE layout the T2a
pseudocode will emit is the A.3 artifact contract (already twice-critiqued at
fa2bc7c5), adjudicated at first bless, not re-derived here — this is by design
(§7's own "amend the wrong one LOUDLY" gate), not a gap.

---

## 4. STOP-THE-LINE FINDINGS

NONE. No candidate rose above cosmetic/stale-anchor. The one load-bearing runtime
invariant (E-62 emission-neutrality of pinned_order/body_ops/output_ops) is CLEAN.
T2a is unblocked; the E-67/E-68/E-69 anchor/wording fixes ride along at the next
§7 touch (no code change, doc-only).
