# Stage I0 — the reference relational interpreter, as diffs

Design author pass, 2026-08-02, tip f0c913e0 (branch keyed-instances).
Diff base: `regional-arch-pseudocode.md` (FLEET-VERIFIED 2026-08-02); every
hunk names the pseudocode section it modifies. Normative target:
`RegionalDataFlowCore.md` §12 (testing), §11 (validators), §15 (acceptance).
Review input: `fable-review-2026-08-01.md` Concern 1 (the inserted step),
Concerns 2/4 (routing/realization, which bound I0's coverage ceiling).

Ranking context: **A → I0 → B → C → D.** I0 is the referee that must exist
and be corpus-validated against the CURRENT compiler *before* Stage B/C touch
anything. I0 is **strictly additive** — it deletes no production code, no
golden, no test. (The deletion-must-name-its-replacement house rule is
therefore vacuous for this stage; every deletion in the sequence lands in
Stage C, which I0 exists to referee.)

---

## 0. The load-bearing discovery that reshapes the stage

`bin/Oracle/Main.cpp` (`drlojekyll-oracle`, 2679 lines) is **already** a
definitional evaluator with everything I0's §6-sketch asks for except three
things:

| I0 requirement (§6 sketch)                    | Oracle today                                            | Gap I0 must close |
| ---                                            | ---                                                     | --- |
| naive/semi-naive set eval                      | ✅ from-scratch semi-naive stratified set eval           | — |
| set semantics + OQ3 batch netting              | ✅ `NetBatch`, per-batch adds∩removes annihilate         | — |
| aggregates @invertible/@recompute + config     | ✅ definitional per-group recompute, both algebras       | — |
| KV mutable merges                              | ✅ degenerate aggregate, same path                       | — |
| negation incl. @never                          | ✅ positive-atom + `NegatedView` atom (set level)        | @never *ordering* semantics unrefereed (set eval only) |
| @product, @barrier, recursion, diff inputs     | ✅ (56/56 `.batches` cases pass an oracle golden)         | — |
| **final membership**                           | ✅ `DumpRelations`                                        | — |
| **sorted PUBLISHED DELTAS per epoch**          | ❌ only final membership                                  | **I0 must add per-epoch delta emission** |
| **compare vs the COMPILER's runtime output**   | ❌ self-cross-checks incr-vs-from-scratch; byte-golden    | **I0 must compare against a compiler-side behavioral capture** |
| **demand answer**                              | ⚠️ evaluates the PLAIN program (answer-neutral referee)  | enough for -demand; NOT per-owner routing (Concern 4) |

Corpus arithmetic (measured): **180** goldens; **56** carry `.batches`; all
**56** already pass both `<case>.oracle.stdout` and `<case>.monotone.stdout`.
The `.batches` corpus was curated *inside* the Oracle's definitional envelope
(float/foreign columns, multi-summary aggregates, unimplemented-by-name MAP
functors are Oracle rejects — no `.batches` case hits them). **124** cases
have no `.batches` and are therefore outside any interpreter referee, today or
under I0.

Consequence for the design: I0 is **not** a from-scratch rewrite of an
evaluator. It is (1) a definitional core that reaches feature-parity with the
Oracle's from-scratch envelope, (2) a new *canonical behavioral format* (CBF)
that adds per-epoch deltas, (3) a compiler-side behavioral capture (the
tagged binary), and (4) the referee wiring. The one real independence
question — does I0 reuse the Oracle's `Query::Build`-based front end, or
evaluate raw parsed clauses — is **OG1** below.

---

## 1. Diff hunks

### H1 — expand the §6 I0 sketch (anchor: pseudocode §6 `I0` block)

```diff
 ### I0 — the reference relational interpreter (INSERTED; review C1)

-    + tests/RefInterp (or bin/RefInterp): a small definitional
-      evaluator (naive/semi-naive over parsed clauses + set semantics
-      + batch netting) — NO Rel, NO codegen
-    + oracle harness: per corpus case with .batches, compare final
-      membership + sorted published deltas vs the CURRENT compiler,
-      all 4 modes
-    EXIT GATE: interpreter agrees with the tip compiler on the full
-      corpus BEFORE any Stage B/C change; disagreement = a finding.
+    + bin/RefInterp/Main.cpp (drlojekyll-refinterp): a definitional
+      evaluator emitting the CANONICAL BEHAVIORAL FORMAT (CBF, H2) —
+      per-epoch sorted published deltas + final sorted membership +
+      per-#query sorted answers. NO Rel, NO codegen. Definitional core
+      is feature-parity with the drlojekyll-oracle from-scratch envelope
+      (§0 table); OG1 decides parsed-clause vs Oracle-core reuse.
+    + bin/RefHarness/Main.cpp (drlojekyll-refharness): reads a parsed
+      module, EMITS behavioral_main.cpp against the stable query/message
+      ABI (the TAGGED-BINARY emitter, H4). OG2 decides emitter-tool vs
+      compiler codegen tag.
+    + goldens/<case>.behavioral.stdout: CBF captured from the TIP
+      compiler's behavioral binary, blessed and FROZEN — the referee
+      snapshot through the Stage C cutover (H5).
+    + run_refinterp step in runall.sh (peer of run_oracle/run_eqgate,
+      H6): builds the behavioral binary in all 4 modes, cross-mode
+      byte-checks its CBF, runs I0 once, byte-compares I0 CBF against
+      the frozen behavioral golden. NOT a 5th golden mode.
+    + bootstrap check (H7): I0's final-membership sub-block byte-equals
+      the drlojekyll-oracle from-scratch dump for all 56 .batches cases
+      (independence-of-referee cross-tie; transitional).
+    + directed R-DIFF witness demand_diff_pub_witness (H8, OG3): the
+      first differential-PUBLISHED demand-instance case — closes the
+      corpus gap that leaves DRInstance::differential==false today.
+    EXIT GATE: see §2. I0 agrees with the tip compiler on all 56
+      .batches cases (minus the adjudicated negation_flap carve-out,
+      H9) before any Stage B/C change lands; every disagreement is a
+      FINDINGS.md entry, never a tolerance fudge (§3).
```

### H2 — the Canonical Behavioral Format (CBF) (anchor: NEW; consumed by §4b Step 6 publish + §4c cursor surfaces)

One deterministic text transcript emitted **identically** by I0 (H3) and by
the behavioral binary (H4). This is the sole comparison surface; it is
independent of any bespoke `.main.cpp` print format.

```
    CBF grammar (line-oriented, LF, no trailing space):
      transcript := header { epoch } final { query_answer }
      header     := "REFINTERP <case> netting=oq3" NL
      epoch      := "EPOCH " <n> NL { delta }          # n from 0, per .batches block
      delta      := ("+" | "-") <pubmsg> { " " <val> } NL   # SORTED within epoch
      final      := "FINAL" NL { member }
      member     := <relname> { " " <val> } NL          # SORTED, every PUBLISHED relation
      query_answer := "QUERY " <name> "_" <bindings> NL { answer_row }
      answer_row := <val> { " " <val> } NL              # SORTED; free cols only, bound echoed
```

Determinism rules (each is a referee-hazard the CBF closes):
- **within-epoch delta order is unspecified at runtime** (publish walks a Vec)
  → CBF **sorts** deltas per epoch. The per-epoch permutation referee is
  `permcheck.py` (published-delta tokens compare order-free per epoch); CBF's
  sort makes the byte path the common case and `permcheck.py` the fallback for
  a case whose driver legitimately reorders. (Reuses the existing
  delta-relational-IR golden policy — CLAUDE.md `permcheck.py` line.)
- **keyed-cursor enumeration order is unspecified** (CLAUDE.md cursor
  contract) → CBF **sorts** final membership and query answers.
- `<val>` rendering reuses `PrintValue` (Oracle Main.cpp:206) so I0 and any
  Oracle-bootstrap comparison share one lexeme spelling.
- `<pubmsg>`/`<relname>` = declared public names only. A **fabricated
  `demand__` message is never a `<pubmsg>`** (it is `IsDemandMessage`-
  suppressed, §4c:1511-1513) — so the CBF of a demand case is byte-identical
  to the plain program's CBF, which is exactly the answer-neutrality the
  referee needs.

### H3 — the I0 evaluator (anchor: NEW `bin/RefInterp/Main.cpp`; NO change to §1 pipeline)

```diff
+ drlojekyll-refinterp <case.dr> <case.batches>  ->  CBF on stdout
+   parse module (Lex/Parse only; NO Query::Build under OG1-parsed,
+     or unoptimized Query::Build under OG1-oraclecore)
+   stratify definitionally (topological over predicate dependency;
+     reuse the Stratify authority ONLY under OG1-oraclecore)
+   for each .batches block (one epoch):
+     net the block (OQ3: dedup each side, adds∩removes annihilate)   # matches NetBatch
+     accumulate explicit facts into the base relations
+     old = snapshot(published relations)                            # membership BEFORE
+     recompute the whole materialization from-scratch (semi-naive,
+       set semantics; aggregates/KV by definitional per-group
+       recompute; negation on the completed lower strata; @product =
+       cross join; @barrier = semantically transparent join staging)
+     new = snapshot(published relations)
+     emit "EPOCH n" + sorted (new \ old) as '+', (old \ new) as '-'  # the DELTA the compiler publishes
+   emit "FINAL" + sorted final membership of every published relation
+   for each #query redeclaration/adornment:
+     emit "QUERY <name>_<bindings>" + sorted answer rows            # demand-neutral: full answer
```

The evaluator is **demand-flag-blind**: it never reads `.drflags`. A demanded
query's answer under I0 is the full definitional answer of the plain program;
the compiler's behavioral binary (compiled WITH `.drflags`) must produce the
same CBF — this IS the flat-eqgate property, refereed structurally rather than
by a bespoke driver assertion.

@product / @barrier / recursion / differential inputs need **no special I0
code**: `@product` is an ordinary cross join in a set eval, `@barrier` (and its
`:-` sugar) only stages join order and is answer-transparent, recursion is the
semi-naive fixpoint, and differential inputs are just epochs with `-` ops. This
matches the Oracle's from-scratch path exactly (that is why the 56/56 envelope
carries over).

### H4 — the tagged-binary emitter (anchor: NEW `bin/RefHarness`; reads §4c ABI)

```diff
+ drlojekyll-refharness <case.dr> -o behavioral_main.cpp
+   parse module; for the STABLE public ABI (CLAUDE.md generated-API):
+     message  <name>_<arity>(db, log, functors, add_vec[, del_vec])
+     query    <name>_<bindings>(db, bound...) -> bool | cursor
+   emit a driver that:
+     - constructs Database + DatabaseFunctors + a CBF-recording LogHook
+       (LogHook captures (pubmsg, sign, values); prints "EPOCH n" + sorted
+        deltas after each batch's entry-point call)
+     - reads the SAME .batches grammar (msgname resolved by name+arity),
+       one block = one entry-point call
+     - after all batches: drains every published relation's query/scan and
+       every #query cursor (forcing where required), prints FINAL + QUERY
+       blocks sorted
+   -> the emitted behavioral_main.cpp + the case's generated datalog.cpp +
+      Runtime = the BEHAVIORAL BINARY (the "tagged binary").
```

The harness targets the **ABI naming convention only** — the seam
`RegionalDataFlowCore.md` §3.1 calls "sealed declared query and message ABIs",
which Stage C explicitly preserves. It does **not** read `QueryView`, guard
annotations, `RecognizedSubgraph`, or any churning internal. That is the point:
the referee couples to the frozen seam, not the code under cutover.

### H5 — the behavioral golden family (anchor: NEW `tests/OptDiff/goldens/<case>.behavioral.stdout`)

```diff
+ goldens/<case>.behavioral.stdout   # CBF captured from the TIP behavioral
+                                    # binary (canonical mode 'opt'), blessed
+                                    # ONCE at I0 landing, then FROZEN.
```

This is the review's "tagged binary" capture (Concern 1, amendment 2). It is
blessed from the **pre-cutover** compiler and held fixed through Stage
B (which must reproduce it byte-for-byte — pure refactor) and Stage C (whose
new binary must still match it, giving the cutover an external behavioral
referee alongside I0). Blessing rides the existing `runall.sh --bless` path
(new copy line beside the `.oracle`/`.monotone` copies at runall.sh:102-111).

### H6 — the run_refinterp harness step (anchor: pseudocode §4b context; concretely runall.sh, peer of run_oracle:217 / run_eqgate)

```diff
   run_oracle()   { ... existing derivation-counter oracle + monotone ... }
+  run_refinterp() {                      # any case with a .batches sidecar
+    batches="$HERE/cases/$NAME.batches"; [ -f "$batches" ] || return 0
+    # 1. emit + build the behavioral binary in ALL 4 modes (reuses the
+    #    per-mode $DRC generated code already built by the mode loop; the
+    #    .drflags sidecar is ALREADY appended to $mflags, so a demand case's
+    #    behavioral binary is the demand-lowered code).
+    drlojekyll-refharness "$DRC" -o "$W/behavioral_main.cpp"
+    for MODE in opt nodf nocf none; do
+      build behavioral binary from gen[$MODE] + behavioral_main.cpp
+      run it over "$batches"  >  "$W/behavioral.$MODE"
+    done
+    # 2. behavioral 4-mode cross-agreement (NEW invariant; cheap):
+    byte-compare behavioral.{nodf,nocf,none} against behavioral.opt
+       -> mismatch = "$NAME behavioral MODE-SPLIT" (a real finding: the ABI
+          surface must be mode-invariant by construction)
+    # 3. freeze referee: behavioral.opt must equal the blessed golden.
+    byte-compare behavioral.opt against goldens/$NAME.behavioral.stdout
+       (permcheck.py fallback per-epoch for a legitimately-reordered driver)
+    # 4. THE I0 GATE: run I0 once (mode-independent) and compare.
+    drlojekyll-refinterp "$DRC" "$batches"  >  "$W/refinterp.cbf"
+    byte-compare refinterp.cbf against goldens/$NAME.behavioral.stdout
+       -> mismatch = "$NAME REFINTERP-DISAGREE" (§3 adjudication)
+    # 5. bootstrap tie (H7).
+  }
```

Why this is **not a 5th golden mode**: the four existing modes are compiler opt
toggles over the bespoke `.main.cpp` drivers; those goldens are untouched
(exit gate §2). I0 is a *referee over a separate behavioral capture*, run
**once** per case, mode-independent. The behavioral binary runs in 4 modes only
to prove ABI mode-invariance (step 2), then a single canonical mode (`opt`)
carries the I0 comparison — transitively covering all modes via the existing
cross-mode byte-agreement of the ABI.

### H7 — the independence bootstrap (anchor: NEW; ties I0 to the Oracle from-scratch authority)

```diff
+  # transitional cross-tie, retire when I0 is trusted:
+  I0.FINAL-block  ==  drlojekyll-oracle <case> <batches> from-scratch dump
+     for all 56 .batches cases (modulo CBF vs DumpRelations formatting —
+     a mechanical normalizer, NOT a semantic transform).
```

Purpose: the Oracle's from-scratch path is *already blessed truth* for final
membership on all 56 cases. Requiring I0's FINAL block to match it bootstraps
I0's correctness from an independent, already-trusted evaluator, so I0 does not
need its own separately-blessed golden (which would merely duplicate the
behavioral golden). Under **OG1-parsed**, this bootstrap is the primary
evidence that a fresh parsed-clause evaluator reached the Oracle envelope;
under **OG1-oraclecore** it is near-tautological (same engine) and downgrades
to a formatting regression check.

### H8 — the R-DIFF witness (anchor: pseudocode §3 BuildSubgraphInstanceOps `if (diff)` arm; NEW corpus case — OG3)

```diff
+ cases/demand_diff_pub_witness.dr        # bound #query over a @differential-
+                                         # derived relation; edges added then
+                                         # REMOVED after demand stands, so an
+                                         # already-published ANSWER row retracts
+ cases/demand_diff_pub_witness.batches   # add-then-remove epochs
+ cases/demand_diff_pub_witness.main.cpp  # asserts the answer set SHRINKS
+ cases/demand_diff_pub_witness.drflags   # -demand -demand-retract
+ cases/demand_diff_pub_witness.eqgate    # -demand -demand-retract -demand-instance
+ goldens/demand_diff_pub_witness.{stdout,oracle.stdout,monotone.stdout,behavioral.stdout}
```

This is the **only** hunk that exercises new *compiler* behavior rather than
pure referee machinery: it must drive `DRInstance::differential == true` and
the band-b signed publish (`del_queue`/`add_queue`) of §4 — code-complete but
UNEXERCISED corpus-wide today (pseudocode §3:289-291). See escalation E1 and
OG3 for whether this lands in I0 or is deferred with an escalation.

### H9 — the OQ3 netting carve-out (anchor: pseudocode §5 OQ3 line; NEW adjudication record)

```diff
+  # negation_flap flap B: same-batch explicit add+remove of one fact.
+  # I0 (and the Oracle) net it to ZERO deterministically (OQ3 set
+  # semantics). The TIP compiler is ORDER-DEPENDENT here (Oracle
+  # Main.cpp header, known divergence). So for negation_flap ALONE the
+  # I0==behavioral gate CANNOT be clean.
+  carve_out: negation_flap  ->  adjudicated, NOT blessed green (OG4)
```

I0 does not paper over this; it **surfaces** it. `RegionalDataFlowCore.md`
§7/acceptance-14 mandate same-epoch permutation invariance, so under the target
architecture the order-dependence is a **defect Stage C is required to close**,
and I0 is the referee that proves the closure. Pre-Stage-C, negation_flap is a
recorded carve-out (OG4 decides: latent-bug-fix-now vs carve-until-Stage-C).

---

## 2. Exit gate (golden-master terms)

**Stays byte-identical (I0 is additive):**
- All **180** existing `goldens/<case>.stdout` — every bespoke driver, all 4
  modes. I0 touches no `.main.cpp`, no compiler code, no opt toggle.
- All **56** `.oracle.stdout` and **56** `.monotone.stdout`. I0 does not modify
  `drlojekyll-oracle`; it consumes it (H7).
- All `.rel`/`.df`/`.ir` structural goldens. No IR shape changes.

**Legitimately new (added, not changed):**
- **56** `goldens/<case>.behavioral.stdout` (H5), blessed from the tip binary.
- **1** new directed corpus case `demand_diff_pub_witness` + its goldens (H8),
  IF OG3 lands it in I0.
- Two new binaries: `drlojekyll-refinterp`, `drlojekyll-refharness`.

**Referees, per surface:**
- I0 CBF vs frozen behavioral golden — **byte-compare** (H6 step 4), with
  **`permcheck.py`** per-epoch delta permutation as the sanctioned fallback.
- Behavioral binary 4-mode agreement — **byte-compare** (H6 step 2).
- I0 FINAL vs Oracle from-scratch — **byte-compare after mechanical
  normalize** (H7).
- `demand_diff_pub_witness` flat==nested — the existing **`.eqgate`** referee
  (run_eqgate, live 4-mode).

**THE GATE (the charge's one-line requirement):** for all 56 `.batches` cases,

    I0_CBF(case) == behavioral_golden(case) == live_tip_behavioral_binary(case, every mode)

with the single adjudicated carve-out `negation_flap` (H9), **before any Stage
B or Stage C change lands.** Any inequality is a `FINDINGS.md` entry (§3).

**Missing-oracle honesty (the holes, counted):**
1. **124** cases with no `.batches` → outside any interpreter referee. Covered
   only by bespoke byte-goldens (+ structural `.rel`/`.df`/`.ir` goldens where
   present). Unchanged by I0; stated so the referee's reach is not overclaimed.
2. **Envelope holes** (Oracle rejects, inherited by I0): float/foreign-typed
   columns; multi-summary/multi-value aggregates; not-by-name MAP functors.
   No current `.batches` case hits them (corpus curated inside the envelope);
   any FUTURE behavior needing them cannot be refereed by I0 and needs a
   directed byte-golden + hand argument. Count: **3** feature classes.
3. **Per-owner routing** (Concern 4): I0 referees the *aggregate* demand answer
   (answer-neutrality), NOT per-owner attach/detach/late-subscriber. The §12.3
   multi-owner witnesses have **no pre-cutover referee** — escalation E3.
   Count: **1** witness class (multiple-owners / detachment / late-subscriber).
4. **@never ordering**: refereed only at final-membership/set level, not the
   DS-R4-10 differential-ordering fence. `negate_never_diff_1` is a diagnostic
   anyway. Count: **1** ordering property.
5. **OQ3 same-batch annihilation**: `negation_flap` (H9) — **1** adjudicated
   divergence where the gate cannot be clean pre-Stage-C.
6. **R-DIFF differential-pub demand-instance**: **0** corpus cases today
   (escalation E1 / H8 / OG3).

Total referee holes carried into Stage C decision-making: **6 classes**,
enumerated so the necessity audit and the owner can see exactly what I0 does
and does not adjudicate.

---

## 3. Adjudication protocol (finding, never fudge)

When `run_refinterp` reports `REFINTERP-DISAGREE` (I0 ≠ behavioral) or
`behavioral MODE-SPLIT`:

1. **Never** adjust a tolerance, never bless the disagreement to green, never
   edit a golden to match (the CLAUDE.md bless rule: goldens change only via
   explicit `--bless` after review, never to make red green).
2. Isolate the **minimal failing epoch**: the first `EPOCH n` block or FINAL
   line that differs. Hand-derive the definitional answer for that epoch from
   the source clauses (set semantics, OQ3 netting).
3. Attribute:
   - I0 wrong → **interpreter bug**. Fix `drlojekyll-refinterp`; re-run the H7
     Oracle bootstrap (a real I0 bug usually also breaks the Oracle tie).
   - behavioral binary wrong → **compiler bug** (an F-series defect, the exact
     class the eqgate family caught at HP-5). File in `tests/OptDiff/FINDINGS.md`
     with the minimal `.batches` repro; fix the compiler; re-bless the affected
     bespoke byte-goldens through `--bless` after review.
   - Both plausibly "right" but differ → a **semantics gap** (e.g.
     `negation_flap`, H9). Record as an owner decision (OG4); it is NOT silently
     absorbed.
4. `behavioral MODE-SPLIT` is always a finding: the public ABI is required to
   be mode-invariant, so a split is either a codegen determinism bug or a
   harness bug — never accepted.

---

## 4. Mechanisms carried forward / introduced (necessity-audit input)

**Introduced by I0 (new):**
- `drlojekyll-refinterp` — definitional set evaluator, emits CBF (H3).
- `drlojekyll-refharness` — reads module, emits `behavioral_main.cpp` against
  the stable ABI (H4). [OG2 may relocate this into a compiler codegen tag.]
- Canonical Behavioral Format (CBF) — one deterministic transcript surface,
  shared by I0 and the behavioral binary (H2).
- Behavioral binary (the "tagged-binary" capture) — per-case generated code +
  emitted harness, run in 4 modes (H4/H6).
- `goldens/<case>.behavioral.stdout` — frozen tip-compiler CBF, the Stage-C
  external referee (H5). New golden family (56 files).
- `run_refinterp` — runall.sh per-case step, peer of run_oracle/run_eqgate (H6).
- Behavioral 4-mode cross-agreement check — new mode-invariance invariant (H6).
- I0-vs-behavioral referee — the exit gate (H6 step 4).
- Oracle-bootstrap tie — I0 FINAL == Oracle from-scratch (H7; transitional).
- OQ3 netting carve-out record — `negation_flap` adjudication (H9).
- `demand_diff_pub_witness` — first differential-pub demand-instance corpus
  case (H8; OG3-gated).

**Carried forward / reused (no new mechanism):**
- `.batches` sidecar grammar — shared with the Oracle, unchanged.
- `.drflags` sidecar — the behavioral binary is compiled with it (demand cases
  get demand-lowered code); I0 ignores it (answer-neutral).
- `.eqgate` sidecar + `run_eqgate` — the flat==nested transitivity referee,
  reused by `demand_diff_pub_witness`.
- `drlojekyll-oracle` — reused as the independent bootstrap oracle-for-I0 (H7).
- The documented stable query/message ABI naming (CLAUDE.md generated-surface)
  — the frozen seam the harness targets.
- The 4 opt modes — the behavioral binary is built in each (mode-invariance
  proof only; not a 5th I0 mode).
- `permcheck.py` per-epoch permutation referee + byte-compare — reused as the
  CBF delta referee (H2/H6).
- `runall.sh --bless` — extended with one copy line for `.behavioral` (H5).

**No mechanism deleted.** (Deletions are Stage C; the deletion-names-its-
replacement rule is vacuous here.)

---

## 5. Owner-gated decisions (variants laid out, not silently chosen)

### OG1 — I0 over parsed clauses vs I0 = Oracle-core extension
- **OG1-parsed (recommended):** `drlojekyll-refinterp` evaluates **raw parsed
  clauses**, independent of `Query::Build`. *Pro:* maximal referee value —
  independent of the dataflow builder, so a Stage B refactor of `Query::Build`'s
  tail (pseudocode §6 Stage B moves the tail through the regional rep) cannot
  hide a bug from I0. *Con:* re-implements stratification + aggregate
  multiplicity + functor-by-name + netting that the Oracle already has; the H7
  bootstrap against the Oracle is the mitigating cross-tie.
- **OG1-oraclecore:** `refinterp` reuses the Oracle's unoptimized-`Query::Build`
  engine, adding only the CBF delta emission. *Pro:* cheapest; reuses proven,
  byte-pinned semantics. *Con:* shares `Query::Build` with the compiler → a
  builder bug is invisible to the referee; H7 degrades to a formatting check.
- **Charge signal:** the §6 sketch and the charge both say "over parsed
  clauses" → OG1-parsed, with the Oracle as bootstrap. Owner ratifies.

### OG2 — behavioral harness via emitter tool vs compiler codegen tag
- **OG2-tool (recommended):** `drlojekyll-refharness` reads the module and
  emits `behavioral_main.cpp` against the documented ABI naming. *Pro:* leaves
  the compiler's codegen surface FROZEN — critical, because Stage C rewrites
  codegen and we do not want the referee entangled in the code under cutover;
  couples only to the stable ABI seam. *Con:* duplicates ABI-naming knowledge
  outside codegen (but that seam is sealed/stable across the cutover).
- **OG2-codegen:** a compiler emit `-behavioral-out <dir>` emits the harness
  beside `datalog.h`. *Pro:* exact typed calls straight from codegen. *Con:*
  a new codegen surface Stage C must also maintain; entangles the referee with
  the churning layer.

### OG3 — R-DIFF witness now vs deferred (see escalation E1)
- **OG3-now (recommended):** author `demand_diff_pub_witness` (H8) as part of
  I0 so a pre-cutover behavioral golden exists for the differential-published
  demand-instance arm. *Pro:* Stage C's tagged-binary goldens then have a real
  pre-cutover referee for `RoutedResultRelation`'s differential realization
  (Concern 4). *Con:* requires driving `DRInstance::differential==true`, which
  may expose that the arm is currently unreachable (then it becomes a real
  finding — which is the point).
- **OG3-defer:** leave it; Stage C authors the first such witness with I0 as the
  **sole** referee (no old-binary cross-check for that one case). *Con:*
  violates the review's amendment that the referee precede the cutover for
  exactly this arm.

### OG4 — negation_flap disposition (H9)
- **OG4-bug:** treat the compiler's order-dependence as a latent defect
  (F20-adjacent); fix netting to deterministic OQ3 now; I0==behavioral becomes
  clean corpus-wide. *Pro:* no carve-out. *Con:* a compiler change during the
  referee-building stage.
- **OG4-carve (recommended pre-Stage-C):** record `negation_flap` as the one
  adjudicated carve-out; Stage C's mandated permutation invariance
  (acceptance-14) closes it, with I0 as the closure referee. *Pro:* keeps I0
  purely additive. *Con:* the gate carries one documented exception into
  Stage C.

---

## 6. Escalations

- **E1 (R-DIFF pub gap, from the charge (e)):** `DRInstance::differential ==
  false` corpus-wide (pseudocode §3:289-291), so the `if (diff)` pub arm and
  the band-b signed publish are code-complete but UNEXERCISED. No pre-cutover
  behavioral golden exists for a differential-PUBLISHED demand-instance answer.
  **Proposal:** land `demand_diff_pub_witness` (H8) under OG3-now — a bound
  query over a `@differential`-derived relation whose already-published answer
  rows retract when an input edge is removed while demand stands. **Alternative
  (escalate):** if driving `differential==true` proves impossible under the
  current lowering (the pub is always provisioned monotone), then the R-DIFF
  arm is effectively dead code and Concern 4's differential `RoutedResult`
  realization has NO pre-cutover referee — owner must accept that Stage C lands
  its first differential-pub witness with I0 as sole (un-cross-checked) referee,
  and the arm's correctness rests entirely on I0's definitional derivation.

- **E2 (Stage B independence, from OG1):** if OG1-oraclecore is chosen, I0
  shares `Query::Build` with the compiler, and Stage B's relocation of the
  `Query::Build` tail through the regional representation (pseudocode §6 Stage
  B) is a refactor whose bugs I0 cannot see. The Stage B exit gate ("180
  goldens byte-identical") then rests on the bespoke drivers alone for the
  builder-tail refactor. Recommend OG1-parsed to keep I0 an independent Stage B
  referee; escalate if the cost is declined.

- **E3 (per-owner routing has no pre-cutover referee):** I0 referees the
  aggregate demand answer, not per-owner routing (no `RequestEdgeRelation`
  exists pre-Stage-C — there is nothing to referee against). The §12.3
  multiple-owners / detachment / late-subscriber witnesses are therefore
  Stage-C-authored AND I0-extended (I0 must grow a request-edge model to
  referee them), not pre-cutover-blessed. This bounds Concern 4's referee: the
  first realization of `RoutedResultRelation` is checked for *answer* by I0 and
  for *shape* by the D3.a.3 refcounted-union-pub precedent, but not for
  per-owner routing by any pre-cutover oracle. Owner should acknowledge this is
  a Stage C deliverable, not an I0 one.
```
