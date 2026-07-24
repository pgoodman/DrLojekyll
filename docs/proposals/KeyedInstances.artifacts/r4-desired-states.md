======================================================================
COMMITTED AT THE R4 LANDING (2026-07-23, the Rel epoch; base tip
b1c95bac). THE DESIRED-IR-STATES CONTRACT for R4 NEGATE (option A
diff/re-source; design contract r4-design.md). STAGE-(d) THREE-WAY
CONVERGENCE RECORD (the PIN-3/OD-13 precedent): the AUTHOR lane's
hand-predicted post-R4 .deltarel files (label/edge/census re-derived
mechanically from the tip dumps under the Part III order law; the
negate_6 two-gate order derived from the walk-DFS successor
structure — NO SWAP predicted: normal/InI before @never/Present) ==
the BLIND worktree prototype's empirical dumps (own build, Part II
spec only) == the PRISTINE-TREE implementation's dumps —
BYTE-IDENTICAL on all THREE trio carriers (orchestrator-executed
cmp, E-77). The blind lane's oracles: census PRE->POST = EXACTLY
the predicted 9-case drop; the BENR per-VISIT lldb hit-count ==
POST census on all 11 opt cases (0/0/0/1/0/0/0/0/0/1/2); generated
C++ BYTE-IDENTICAL to the pristine tip on all 11 (zero emission
change); six existing .deltarel carriers byte-identical; zero
validator aborts (incl. the new R4 A.6(c) recount).
======================================================================

# R4 desired states — the negate-gate mint re-source

DS-R4-1 [STRUCT] THE MINT MOVE: the sole kNegateGate mint authority
  is MakeEagerNegateOp (DeltaRel.cpp, single ctor), invoked from the
  Build.cpp IsNegate walk dispatch (via LowerRelStep_Negate) and the
  EAGER_WEB re-invocation ONLY; the query.Negations() inventory loop
  is DELETED (with its stale E-108 comment block).

DS-R4-2 [STRUCT] EFFECT PRESERVATION: every minted gate carries the
  REAL kFlagRead{read_table=negated_table, pred=NegateGatePred(
  kEager,hint), ctx=kEager} — reconstructed identically at walk mint
  and re-invocation (M2' extended to an effect). The op populates
  ONLY gate_* + the effect (never eager_view/table_op_table): it
  stays OUT of IsEagerMarkerKind, REQUIRED (the marker recount loop
  reads fields that are null on the gate).

DS-R4-3 [BYTE] THE TRIO GOLDENS (first-evers, RAT-8): the three-way
  converged files. negate_1 (1→1): gate label 7→10 (tail-appended
  oi), ingest folds 8-9→7-8, forwards 10-11→9,11; the table-less
  lead-0 gate still renders FIRST — render sequence structurally
  identical, labels renumbered. negate_6 (2→2): gates 7,8→11,13,
  NO ORDER SWAP (walk-DFS reaches the normal/InI gate before the
  @never/Present gate — select.1's successor order); folds 9,10→
  7,8. d5_recursive_negate (1→0): the gate VANISHES (the ZERO-MINT
  NEGATIVE guard), labels 55-60→54-59, 5 gate WAR edges removed,
  deps 333→328, ops 61→60.

DS-R4-4 [STRUCT] THE 9-CASE CENSUS DROP (all four modes; PRE is
  df-axis dependent, POST==walk-reached): cond_both_polarities 2→0,
  cond_diff_flipflop 1→0, d5_recursive_negate 1→0, disassemble 2→1
  (the SOLE mixed case), merge_5 3opt/3nocf/4nodf/4none→0,
  negate_cobatch_diff 1→0, negate_lower_strata 1→0,
  negate_multiplicity 1→0, negation_flap 1→0. Corpus opt total
  30→18. NO other (case,mode) census changes.

DS-R4-5 [STRUCT] THE PER-VISIT/EMISSION-SIDE ORACLE (the corrected
  C1-F1+C2-F3 referee, normative for any future negate-gate count
  cross-check): POST kNegateGate census per (case,mode) == the
  BuildEagerNegateRegion DISPATCH count (per-VISIT — the walk
  multi-visits: demand_tc_witness kEagerForward=12 vs 11 TUPLE
  views; and emission-side — never re-derive CanReceiveDeletions
  from the same graph). Verified by the blind lane's lldb
  hit-counts, 11/11 agreement.

DS-R4-6 [BYTE] SIX EXISTING .deltarel CARRIERS BYTE-STABLE:
  kNegateGate=0 everywhere; deleting the empty loop + tail-append
  of zero gates shifts nothing; the ADJ-S2 op.0/op.1 ingest-fold
  pin holds (the deleted loop sat BEFORE the ingest loop).

DS-R4-7 [STRUCT] THE M10 RECOUNT (per-op, at the V-NEG-CTX site):
  abort iff op.gate_negate && op.gate_negate->CanReceiveDeletions()
  — the EXACT Build.cpp:970 cut criterion (NOT InductionGroupId,
  F22); never false-aborts (a minted op is walk-reached ⇒ the
  predicate is false; walk-cut negates mint nothing). The seed
  PlanNode{kGate} sub-objects and fixpoint arm DREffects are NOT
  kNegateGate DROps — the recount's domain is exactly the eager
  gates.

DS-R4-8 [BYTE] RENDER: NO change (owner ruling: @never stays
  IMPLICIT — reads: Present(%table:N) vs InI(%table:N)
  disambiguates; no token, no dedicated case, the generic fallback
  stands). The only .deltarel deltas corpus-wide are op existence
  (the 9-case drop) and label/tie-break renumbering.

DS-R4-9 [GATE] ZERO EMISSION CHANGE: BuildEagerNegateRegion
  byte-unchanged, the gate never lowers; A/B full corpus (4 modes +
  nested + data/) vs frozen b1c95bac baselines 0-diverged.

DS-R4-10 [STRUCT] LABELED LATENT GAP (owner ruling): a
  differential-INPUT @never negate (walk-cut with an @never hint —
  would carry a seed/fixpoint plan-spine gate while V-NEG-CTX's
  eager pin never sees a standalone op for it) has NO corpus
  witness and NO directed fence this slice; recorded here as the
  standing label.
======================================================================
