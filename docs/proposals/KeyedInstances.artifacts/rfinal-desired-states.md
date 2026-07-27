# R-FINAL — DESIRED STATES + PER-SLICE STAGE-(d)/(e) RECORDS

> House banner. The per-slice desired-output-states record for the R-final
> program (companion to rfinal-design.md, whose §7 adjudication record amended
> the designs in place; rulings R1-R6 ratified per rfinal-ruling-brief.md).
> One section per landed slice, appended as slices land. Records are written
> by the landing session; predictions were PRE-REGISTERED before implementation.

## SLICE 1 — FOLD A (pivot-belt TUPLECMP retirement)

STAGE-(d) THREE-WAY CONVERGENCE (the SEVENTH slice under the precedent;
executed 2026-07-27 at tip e30632be):
  half 1  the DUMP-BLIND AUTHOR lane hand-predicted, from code + committed
          goldens only (compiler runs forbidden): the exact source diff
          (Option B — TABLEJOIN* return, cmp mint/fill deleted, hider
          re-homed to join->col_id_to_var under the foldA-F1 last-write-wins
          note, Table.h contract NOTE at :789, stale "approximate" comments
          retired), full unified diffs of the ONLY three changed pinned
          surfaces (demand_tc_witness h.opt −10 lines / ir.opt −10 lines,
          symrec_tie_1 ir.opt −4 lines; clean-dedent shape, VAR ids stable),
          the DONT-CHANGE set (all .deltarel/.df/stdout/oracle/monotone
          pins), and the pre-bless red vocabulary (EXACTLY three
          IRGOLD-DIVERGE lines, zero GOLDEN failures).
  half 2  the BLIND WORKTREE PROTOTYPE lane (hand-provisioned worktree
          verified at e30632be; author file unread) implemented from the
          design alone: 5 files touched; build green; 19-surface regen =
          EXACTLY the predicted 3 changed + 16 byte-identical; end-to-end
          stdout 12/12 (join_1/demand_tc_witness/symrec_tie_1 × 4 modes)
          byte-identical to committed goldens; corpus churn EXACTLY 58/175
          opt-mode .h (14 diagnostics emit no header; 0 asymmetric) — the
          design's pre-registered count matched.
  half 3  the PRISTINE implementation (the converged patch applied by the
          orchestrator, personally reviewed): the three changed surfaces
          regenerate BYTE-IDENTICAL to the prototype's outputs
          (orchestrator-executed cmp, E-77), and the 16 unchanged pinned
          surfaces (incl. the fresh barrier_neck_1 df pin — 19 total at this
          tip) byte-identical to the committed goldens.
  CONVERGENCE: author == prototype == pristine, byte-level, on every
  compared surface; the author's hand-predicted diff hunks byte-match the
  empirical diffs (spot-verified per hunk by the orchestrator).

PRE-REGISTERED PREDICTIONS (from rfinal-design.md §1, as amended
foldA-F1..F4) vs OUTCOME:
  [STRUCT] pre-bless reds EXACTLY {demand_tc_witness h.opt, ir.opt,
           symrec_tie_1 ir.opt} IRGOLD-DIVERGE, nothing else ......... see
           the gates record below.
  [STRUCT] 58/175 opt-mode generated-header churn ..................... MATCHED
           (prototype count, pristine identical by byte-convergence).
  [BYTE]   every .deltarel/.df pin + every .stdout/oracle/monotone golden
           byte-identical ........................................... MATCHED
           (16/16 pinned; suite stdout in the gates record).
  [BYTE]   VAR ids stable (the folded TUPLECMP mints no next_id) ..... MATCHED
           (clean-dedent diffs only; no renumbering anywhere).

## SLICE 2 — FOLD B (side_key_eqs retirement; codegen-only)

STAGE-(d) THREE-WAY (compact form — the design §2 carried the byte-level
after-shape): the ORCHESTRATOR-authored dump-blind predictions
(foldB/author-predictions.md: the 3-cut Database.cpp diff; cf16_2.h.opt =
EXACTLY 2 predicate lines rewritten in place, both post-fold lines given
verbatim; DONT-CHANGE = every other pinned surface; churn EXACTLY 28/175;
pre-bless red EXACTLY `cf16_2 irgold h.opt IRGOLD-DIVERGE`) == the BLIND
worktree prototype (Database.cpp only, 5+/22−; churn 28 EXACT after the
lane itself caught and corrected a stale-baseline confound — first count
85 = FoldA+FoldB compound, rebuilt the Fold-A-only reference; 19/19
worktree-pinned surfaces identical; e2e cf16_2 4/4 modes + oracle +
monotone identical) == the PRISTINE implementation (converged patch +
the three comment riders Fold B obligates: Table.h NOTE lifted to full
generality, both Program.h "sections still conjoin" clauses updated) —
cf16_2's header BYTE-IDENTICAL pristine==proto, and the author's two
predicted post-fold predicate lines match VERBATIM (orchestrator cmp,
E-77). WITNESS-FIRST: cf16_2's h.opt pin was seeded PRE-fold in its own
commit (RAT-8; red EXACTLY `IRGOLD-MISSING`; bless source byte-verified
against the orchestrator-reviewed dump) so the fold's churn landed as a
reviewed IRGOLD-DIVERGE re-bless.

GATES RECORD (binding copy = ledger §20(X)): pre-bless red EXACTLY the
one pre-registered IRGOLD-DIVERGE → bless (source byte-verified
same-as-converged; 3 sibling rewrites no-ops) → SUITE PASS(175) ×3;
data/ 144 rows behavior-identical vs frozen ea72d97a; ctest 5/5 debug +
5/5 ASAN; ASAN both surfaces PASS(175); config-invariance SINGLE-HASH
(cf16_2 h, 3-run debug + release); Q5 progsize@128 ABABAB noise (~0.7%)
+ tc_random engine medians 0.0-0.1% noise — tc_random's header
byte-identical: its joins are INDUCTIVE (round-shell emission), outside
the acyclic differential-section surface this fold touches; NEITHER fold
has a bench flagship carrier (honest ADJ-S8 note; a future acyclic-
differential-join workload would be the witness). Fable review record in
§20(X).

## SLICE 3 — THE PER-JOIN EMISSION OPS (kJoinEmit(27)/kProductEmit(28))

STAGE-(d) THREE-WAY (the full blind pair): the DUMP-BLIND AUTHOR
(emitop-d/author-predictions.md — the full source diff across 8 files;
per-pin census tails EXACT for all 11; the 7 block-gaining carriers' op.N
labels EXACT (join_1 op.20/21, optimize_2 op.18, demand_tc op.24-28,
symrec op.13/14, booleans op.15/16, elim op.12, d5 op.60 the DELTA
carrier); band placement via key_of lead-0; render shape exact with an
honest E-F residual ledger naming the only empirical-fill integers
(order=/seq= values, demand_tc's table= assignment)) == the BLIND
WORKTREE PROTOTYPE (full implementation, +465/−22 across 8 files, all
six adjudication amendments honored; 21-surface regen = EXACTLY the 11
.deltarel DIVERGE + all h/ir/df byte-identical; 175-case generated-.h
vs frozen 6107fd3d = 0 DIVERGED — the [BYTE-1] hard constraint; census
== blocks on every pin; cross-knob law on 3 carriers; DeltaRelValidators
ctest green; one self-caught build fix (AllSidesSameScc linkage) and
four design-truth adaptations, all adjudicated consistent: post-Fold-A
TABLEJOIN* signature, LowerProductEmit as the Site-5 recorder at the
inline TABLEPRODUCT mint, ctor-stored table= via ModelTableOrNull (the
marker precedent — Format has no impl), work_seq naming) == the PRISTINE
implementation (converged patch): 11/11 .deltarel dumps BYTE-IDENTICAL
pristine==proto (orchestrator cmp, E-77).

GATES RECORD (binding copy = ledger §20(Y)): E-62 re-grep CLEAN (LIVE
this diff); pre-bless reds EXACTLY the eleven .deltarel IRGOLD-DIVERGE →
bless (UNFILTERED — git-verified EXACTLY the 11 sanctioned goldens
changed, 276 other rewrites byte-identical no-ops; sources 11/11
byte-verified same-as-converged) → SUITE PASS(175) ×3; EMISSION
BYTE-IDENTITY 175/175 opt headers vs frozen 6107fd3d + data/ 144 rows
behavior-identical; ctest 5/5 debug + 5/5 ASAN; ASAN both surfaces
PASS(175); config-invariance SINGLE-HASH (join_1 + d5 deltarel, 3-run
debug + release); Q5 progsize@128 ABABAB ~1% noise (A1 cold discarded).
Fable review record in §20(Y).

## SLICE 1 GATES RECORD (as landed; the binding copy is ledger §20(W)): pre-bless
reds EXACTLY the 3 pre-registered IRGOLD-DIVERGE → bless (sources
byte-verified same-as-converged; 8 sibling rewrites no-ops) → SUITE
PASS(175) ×3 + post-fix; data/ 144 rows behavior-identical vs frozen
a7dde012; ctest 5/5 debug + 5/5 ASAN; ASAN both surfaces PASS(175) zero
reports; config-invariance SINGLE-HASH ×4; Q5 progsize@128 ABABAB ~0.6%
median noise (headers byte-identical — no joins) + tc_random engine A/B
noise (~0.1%; header byte-identical — delta-path joins only, confirming
the 58-case eager churn analysis); FABLE REVIEW 13 findings / 6 root
causes, zero live correctness, all fixed pre-commit (Table.h NOTE
re-scoped + operator== value-equality re-wording; the public-API
Program.h:977 reader; EmitJoin + Join.cpp comment retirements; the
rel-arch §5 discharge), fixes proven DUMP-NEUTRAL 19/19 + post-fix
SUITE PASS(175).
