======================================================================
COMMITTED AT THE OD-13 LANDING (2026-07-23, the Rel epoch, pre-R4;
base tip 3e02db13). THE DESIRED-IR-STATES CONTRACT for the OD-13
model-set observability diff — every touched surface's post-diff
bytes PRE-REGISTERED against real tip dumps. STAGE-(d) THREE-WAY
CONVERGENCE RECORD (the PIN-3 precedent extended): the AUTHOR lane's
hand-simulated goldens (BuildEquivalenceSets union rules simulated
from the .df graph + Build.cpp:2297-2513, DOT-BLIND by fiat) == the
BLIND worktree prototype's empirical dumps (own build, own binary,
design-shape-only prompt) == the MECHANICAL comparator's DOT-derived
insertions (per-view "EQ SET" cells read from the INDEPENDENT
-dot-out surface at tip, no code change) — BYTE-IDENTICAL on all
FOUR carriers (orchestrator-executed three-way cmp, E-77; then the
pristine-tree implementation reproduced the converged bytes exactly:
a FOUR-way identity). Churn matched the pre-registration EXACTLY:
41 ATTRIBUTES lines in place (tc 19 / symrec 11 / negate_1 6 /
aggregate_1 5), zero non-ATTRIBUTES changes, all four INSERT
ATTRIBUTES lines byte-identical, line counts unchanged.
======================================================================

# OD-13 desired states — the .df partition token (eqset=)

Tip goldens = the four `.df.opt.golden` pins at base 3e02db13. The
post-diff goldens are the three-way-converged files (blessed from the
reviewed suite run per RAT-8's re-bless arm).

DS-OD13-1 [BYTE] TOKEN SHAPES (owner rulings Q-A eqset= / Q-B RAW /
  Q-C render-on-INSERT; adjudicated position after table=, before
  class=; bare integer):
    table-backed:      "  ATTRIBUTES table=%table:8 eqset=2 class=..."
    table-less:        "  ATTRIBUTES eqset=1 class=table-less ..."
    INSERT (Q-C):      "  ATTRIBUTES eqset=10 class=monotone stratum=9"
  omit_table drops ONLY table= (redundant with the `into %table:N`
  header); eqset= is unconditional on EVERY block. [Q-C superseded
  the stage-(d)-era INSERT-omit after the Fable review refuted its
  recoverability premise on demand_tc_witness — see od13-design.md
  banner.]

DS-OD13-2 [BYTE] PER-GOLDEN CHURN (as landed, post-Q-C): exactly 45
  ATTRIBUTES lines mutate in place vs the pre-slice goldens —
  demand_tc_witness 20, symrec_tie_1 12, negate_1 7, aggregate_1 6
  (every ATTRIBUTES line, INSERT included); zero header/edge/body/
  block-count changes; line counts unchanged (98/60/27/23). [The
  stage-(d) three-way converged on the pre-Q-C 41-line shape; the
  Q-C delta is exactly the 4 INSERT lines, values verified by the
  table-consistency law + the DOT cross-surface check.]

DS-OD13-3 [BYTE] THE VALUE MAPS (view -> eqset, RAW EquivalenceSetId;
  (t<N>) marks the block's table= where present; INSERT values per
  Q-C, verified against the table-consistency law + DOT):
  negate_1:    select.0=1  select.1=2  tuple.2=2(t8)  tuple.3=4(t4)
               tuple.4=1(t11)  negate.5=4(t4)  insert.6=4(into t4)
  aggregate_1: select.0=1  select.1=2  tuple.2=3(t15)  tuple.3=4(t10)
               aggregate.4=5(t5)  insert.5=4(into t10)
  symrec_tie_1: select.0=1  tuple.1=2  tuple.2=3  tuple.3=11(t4)
               tuple.4=11  tuple.5=1(t8)  tuple.6=1(t8)  tuple.7=1
               join.8=9  join.9=10  merge.10=11(t4)
               insert.11=11(into t4)
  demand_tc_witness: select.0=1  select.1=2  tuple.2=1(t19)
               tuple.3=4  tuple.4=18(t8)  tuple.5=6  tuple.6=19(t12)
               tuple.7=8(t15)  tuple.8=2(t23)  tuple.9=10
               tuple.10=1(t19)  tuple.11=15  tuple.12=2  join.13=14
               join.14=15  join.15=8  join.16=10  merge.17=18(t8)
               merge.18=19(t12)  insert.19=10(into t4 — the SOLE
               table-stamped block of set 10: the Q-C witness)

DS-OD13-4 [STRUCT] CROSS-SURFACE IDENTITY (normative, t2-dump-spec):
  every rendered eqset= value EQUALS the DOT "EQ SET" cell of the
  same view (verified by the comparator lane's derivation being the
  DOT surface itself). A dense renumber, if ever adopted, must land
  on BOTH surfaces together.

DS-OD13-5 [STRUCT] TABLE-CONSISTENCY LAW (the one-union-site fact
  observable): all blocks sharing table=%table:N carry ONE eqset
  value (verified on every carrier by all three lanes).

DS-OD13-6 [BYTE] THE E-106/E-107 SHAPE IS NOW VISIBLE IN TEXT:
  negate_1 select.0 (class=table-less) renders eqset=1 == tuple.4's
  (%table:11) set; select.1 renders eqset=2 == tuple.2's (%table:8)
  set — the model membership that previously needed a worktree
  fprintf probe (the M12 ritual) is now one grep away.

DS-OD13-7 [GATE] DETERMINISM: 3-run byte-identical -df-out per
  carrier + debug==release single hash, opt AND none modes. Framing
  per the adjudication (MED-3): eqset= is as stable as ForEachView
  order — the gate is the guard, not a universal-determinism proof
  (F24 remains the standing record of that failure mode on
  conflicting_constants nodf, out of scope here).

DS-OD13-8 [STRUCT] DOT FLOOR: unchanged, ZERO-golden, nothing
  minted — Format.cpp:65 already renders EQ SET unconditionally on
  every view node; whole-file DOT stays non-byte-reproducible
  (UniqueId pointer node names), which is WHY no DOT golden exists.

DS-OD13-9 [GATE] DUMP-ONLY: the A/B corpus (all 4 modes, frozen
  debug 0e017e89 / release 5090a5d2 baselines) is .df-blind and must
  show ZERO divergence; generated headers byte-identical (Q5).

DS-OD13-10 [STRUCT] THE DF-EQSET BELT: the ~0u sentinel abort is
  UNREACHABLE at drain (BuildEquivalenceSets runs unconditionally at
  lib/DataFlow/Build.cpp:2640, pre-Stratify, before every dump) —
  it exists as the DF-CLASS-idiom belt and never fires across the
  corpus.

======================================================================
FABLE-REVIEW RECORD (at the landing; 11-agent workflow, high
effort; the od13-design.md banner carries the ruling summary):
5 verified findings, ZERO live code-correctness.
  [1] HEADLINE — the OMIT-on-INSERT recoverability premise REFUTED
      on demand_tc_witness (insert.19's %table:4 has zero
      table-stamped rendered siblings; set 10 renders elsewhere
      only table-less on tuple.9/join.16). ESCALATED to the owner
      -> RULING Q-C: eqset= renders on INSERT (table= stays
      omitted). Second ritual bless cycle executed: pre-bless reds
      EXACTLY the four .df IRGOLD-DIVERGE, one changed line per
      carrier (the INSERT ATTRIBUTES line), values
      insert.19=10 / insert.11=11 / insert.6=4 / insert.5=4 —
      each equal to its target table's set (consistency law) and
      to the DOT cell (cross-surface law). DS-OD13-1/2/3 amended
      in place above.
  [2] the §20(P) + Fable-record cross-references were dangling in
      the working tree — discharged by the §20(P) ledger entry and
      THIS record landing in the SAME commit (the PIN-3 [1]
      family).
  [3] this record's absence from the pointed-to file — fixed by
      THIS record.
  [4] the t2-dump-spec "same order" cross-surface claim overstated
      — tightened to the PAIR order (table= before eqset= == TABLE
      before EQ SET; full field sequences differ by surface).
  [5] the kDFInsert call-site comment was stale — refreshed to the
      Q-C semantics.
======================================================================
