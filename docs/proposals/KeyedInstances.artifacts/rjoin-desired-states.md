# R-JOIN DESIRED OUTPUT STATES (DS-RJ-1..10) + the stage-(d) record

Banner. COMMITTED at the R-JOIN landing (2026-07-24; ledger §20(S); design =
rjoin-design.md). Designed and executed at tip `8cf377dd` (binaries ==
`2aa23b3f`). STAGE-(d) THREE-WAY CONVERGENCE (the R4/PIN-3/OD-13 precedent,
now FIVE slices deep): the author hand-prediction lane (dump-blind — derived
the exact POST bytes from walk-DFS + the key_of order law + eqset= partition
reads, WITHOUT implementing) == the BLIND worktree prototype (implemented
contract §4 in an isolated worktree, blind to the author's bytes) == the
PRISTINE implementation, BYTE-IDENTICAL on ALL 44 SURFACES (11 carriers × 4
modes; the orchestrator personally ran every cmp). THE M12 HEADLINE: the
mandatory ModelTableOrNull probe REFUTED the uniform table-less prediction —
demand_tc_witness carries TWO model-table-BACKED join views (tables 4 and 15;
the author lane independently derived the same pair from the OD-13 `eqset=`
tokens — the partition-visibility payoff §20(P) predicted, no worktree probe
needed for the derivation, only for the confirmation). join_1 / optimize_2 /
symrec / booleans / elim join views are all model-null (table-less lead-0).

## DS-RJ-1 — the census law (M15-conformant)

For every (case, mode): `kEagerJoin=N` == the BuildEagerJoinRegion dispatch
hit-count and `kEagerProduct=M` == the BuildEagerProductRegion hit-count,
PER-VISIT (one marker per (pred_view -> join_view) walk edge; join_1 = 4
markers over 2 join views) and EMISSION-SIDE (lldb hit-counts, measured on
all 44 rows by the stage-(d) oracle lane — never a per-view count, never the
`.ir` TABLEJOIN count, never a same-graph re-derivation). Census grows
24 -> 26 on EVERY `.deltarel` dump (zero-count kinds render).

## DS-RJ-2 — the eleven pinned carrier censuses (opt; measured == predicted)

    join_1                  kEagerJoin=4  kEagerProduct=0   (NEW golden, RAT-8)
    optimize_2              kEagerJoin=0  kEagerProduct=2   (NEW golden, RAT-8)
    demand_tc_witness       kEagerJoin=8                    (real blocks)
    symrec_tie_1            kEagerJoin=4                    (real blocks)
    booleans                kEagerJoin=4                    (real blocks)
    elim-cond-cycle-simple  kEagerJoin=2                    (real blocks)
    d5_recursive_negate     kEagerJoin=0  (census-only; the ZERO-MINT join
                            NEGATIVE guard — its differential join stays
                            kPivotAssemble=1 kFixpointFire=2, UNCHANGED)
    map_3 / merge_2 / negate_1 / negate_6   0/0 (census-line-only)

## DS-RJ-3 — cross-knob stability (measured, not asserted)

kEagerJoin/kEagerProduct counts are IDENTICAL across all 4 modes on every
carrier (opt==nocf and nodf==none hold as full-dump byte equalities on all
11; the join counts additionally equal across the df axis on THESE carriers —
measured, not a law; kEagerForward-style df-axis growth remains expected
elsewhere per DS-ADJ-1).

## DS-RJ-4 — block shape + placement (the order law)

Marker blocks render the bare kEagerForward production VERBATIM: header
`op.N kEagerJoin sign=· ctx=eager stratum=0` + `    args:` with ` table=`
ONLY when the model table is non-null. NO new token (no pivots=N — declined
unwitnessed; no E-71 lane was owed). Model-NULL markers sort in the
TABLE-LESS LEAD-0 band (with the R4 gate); demand_tc's FOUR table-backed
marker BLOCKS — two per table-backed join VIEW, per-VISIT: op.7/op.17
`table=%table:4`, then op.14/op.16 `table=%table:15` (golden emission
order) — sort into their tables' bands as sign-0 eager ops in op-id order
(the lower-sign-first band ordering of the R1 order law; tables 4/15 carry
NO ingest fold — the sign-+1 folds live in tables 19/23, where the R1
sign-0-before-fold layout is witnessed on the forwards). These are the
FIRST JOIN witnesses of the E-107 table-backed shape. [Fable review
RJDUMP-2/docs-F1/docs-F2 fixed pre-commit.]

## DS-RJ-5 — the sign glyph + hexdump pin

`sign=` on every marker is the middle dot `c2 b7` (hexdump-confirmed by the
step-1 mechanical lane at tip and unchanged by this slice).

## DS-RJ-6 — pre-bless red set (pre-registered; matched EXACTLY)

The pre-bless suite run reds are EXACTLY: `join_1 deltarel.opt
IRGOLD-MISSING` + `optimize_2 deltarel.opt IRGOLD-MISSING` + the NINE
committed `.deltarel.opt` IRGOLD-DIVERGE (demand_tc_witness, symrec_tie_1,
booleans, elim-cond-cycle-simple, d5_recursive_negate, map_3, merge_2,
negate_1, negate_6). NOTHING else red; every other pinned surface (tc
h/ir/df, symrec ir/df, negate_1 df, aggregate_1 df) stays OK. [MATCHED.]

## DS-RJ-7 — byte-identity (the slice is DUMP-ONLY)

Generated C++ (datalog.h AND datalog.cpp) is byte-identical to the frozen
`f60379c3` baselines on the FULL corpus × 4 modes + the nested witness × 4 +
data/ × 4, BOTH compiler pairs (debug 5f30847f… / release e69068d0…):
840 rows × 2 pairs, 0 diverged (evm_array_parse's identical-SIGABRT baseline
stands — exit codes match). next_id is UNTOUCHED by the marker mint
(ADJ-RJ-7); the deferral machinery is interior to the untouched builders.

## DS-RJ-8 — validator obligations

Both kinds pass IsEagerMarkerKind (8 kinds); the A.6(c) arms are
M10-STRENGTHENED on the pivot-count discriminant (kEagerJoin: IsJoin &&
NumPivotColumns()>0; kEagerProduct: IsJoin && ==0) because IsJoin alone is
ambiguous between the kinds; the table-match check (op.table_op_table ==
ModelTableOrNull) holds per-dispatch — including demand_tc's two non-null
rows. Zero dump aborts across all 44 surfaces; V-READY/key_of ride
IsEagerMarkerKind unchanged (no new branch).

## DS-RJ-9 — negative guards

d5_recursive_negate mints ZERO join markers (its recursive-differential join
is walk-cut; delta maintenance lives in Authority A — kPivotAssemble=1 in
its golden, the SHARED BuildJoin(for_delta=true) carrier). The differential
products (product_diff / product_self / product_mixed) mint zero
kEagerProduct (kProductArm-modeled); spot-checked at stage-(d), not blessed.

## DS-RJ-10 — residuals opened by this slice

(a) R-final owes the PER-JOIN emission op carrying the ContinueJoinOrder
drain-order key — the per-visit marker is only the REACHABILITY half of the
R-final substrate (ADJ-RJ-14, stated loudly). (b) The pivot-belt fold:
REDUNDANCY-HOLDS, owner-DECLINED this slice; a gated emission-shape
follow-up at R-final (the Join.cpp:513-516 "approximate scans" comment is
STALE wrt the shipped exact Index and lingers until then). (c) The
side_key_eqs delta fold is a SEPARATE third diff (Database.cpp comment :2788,
decl :2791, build :2862, use :2896 — the brief's ":2740-2744" cite was
stale, ADJ-RJ-16). (d) Dead BuildNestedLoopJoin (Join.cpp:179-294, behind
`true ||` :744 + assert(false) :775) — labeled, optional hygiene removal,
NOT taken this slice. (e) No committed golden yet witnesses a differential
PRODUCT's kProductArm blocks (d5 covers the join delta path only).

## The Fable-review record (stage (f), 2026-07-24; 18-agent workflow — 6
## dimension reviewers + adversarial verification of every finding)

TWELVE findings raised; TEN confirmed (one — docs-F1, the DS-RJ-4
four-blocks-not-two count — adjudicated by the ORCHESTRATOR personally at
the golden after its verifier lane died at the StructuredOutput cap; the
recover-from-journal path, never a respawn), ZERO live correctness — every
confirmed finding is doc/comment drift, ALL FIXED PRE-COMMIT and proven
DUMP-NEUTRAL (18/18 pinned surfaces byte-identical post-fix + post-fix
suite PASS + post-fix A/B subset clean):
  [MINT-1/mold-F2] DeltaRel.h eager_view field comment — the three marker
      enumerations gained the R-JOIN kinds.
  [mold-F1] DeltaRel.cpp EAGER_WEB reconstruct comment "six" -> "eight"
      (the :2506 sibling was already updated in-slice; this one was missed).
  [RJDUMP-3] Format.cpp census-guard comment "25th" -> "27th".
  [mold-F3] Build.cpp:743 "six batch-skeleton kinds" -> "eight"
      (PRE-EXISTING drift vs the Build.h:268 mirror; rode along).
  [RJDUMP-2/docs-F1/docs-F2] DS-RJ-4 reworded: FOUR table-backed marker
      blocks (per-visit, two per view), %table:4 pair first in golden
      order; the R1 before-the-fold clause re-attributed to tables 19/23
      (tables 4/15 carry no ingest fold).
  [docs-F3] the §20(S)/"COMMITTED" forward-references — DISCHARGED by the
      §20(S) entry + this record landing in the same commit (the PIN-3 [1]
      family).
  [FC-1, the review's headline] CLAUDE.md re-pointed: eight markers incl.
      kEagerJoin/kEagerProduct with the per-visit/deferral caveat, ELEVEN
      .deltarel goldens incl. join_1 + optimize_2, remaining unmodeled arm
      = E-42 only.
REFUTED (grounds in the review record): [RJDUMP-1] the DeltaRel.h enum
comment "demand_tc_witness carries two" counts table-backed join VIEWS
(correct; the four is the per-visit BLOCK count, stated correctly in
DS-RJ-1/DS-RJ-4); [FC-2] the DS-RJ-10d BuildNestedLoopJoin cite off-by-one
(decl :180 vs :179 — within the cited comment block, not minted).
