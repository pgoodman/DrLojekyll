# R-JOIN DESIGN CONTRACT — the FIFTH step-kind migration (JOIN + PRODUCT)

Banner. COMMITTED at the R-JOIN landing (2026-07-24; ledger §20(S)). Designed at
tip `8cf377dd` on `keyed-instances` (docs-only atop `2aa23b3f`; compiler binaries
unchanged since `2aa23b3f`). This contract is the stage-(b)/(c) close for R-JOIN:
the merged best design across three competing designs (A = pivots+product mold;
B = pivots+product complete; C = pivots-only fold), every contested point
re-adjudicated AT THE CODE by the XHIGH adjudicator, with all load-bearing
empirics re-measured live (lldb hit-counts + golden census greps). **Design B is
the substantially-adopted base; A and C are corrected/superseded on the
empirical crux below.**

OWNER RULINGS (2026-07-24, at the ritual head, all four recommendations
RATIFIED verbatim): (i) SCOPE = JOIN + PRODUCT together, ONE slice
(kEagerJoin + kEagerProduct; product carrier = the existing optimize_2 via a
new sidecar); (ii) REFERENT = the PER-VISIT dispatch edge (the §2 normative
sentence is the pinned meaning; the once-per-join deferred TABLEJOIN emission
stays hand-coded, owed to R-final); (iii) EFFECT-FREE (both kinds join
IsEagerMarkerKind; M14 does not transfer — no pre-existing eager DR op);
(iv) the pivot-equality-belt fold is DECLINED from this slice (the NOT-RULED
epoch-brief §5 candidate, RULED: redundancy premise HOLDS — Index::First is
exact full-key, Table.h:801 — but the belt is also the body-anchor and the
provenance-hider, so folding is an EMISSION-shape change; it lands as its own
gated follow-up, naturally at R-final; this REVISES the epoch-brief §5
"belongs WITH the JOIN migration" pre-registration on principled grounds).

STAGE-(d)/(e) RECORD + the Fable-review record: see rjoin-desired-states.md
(DS-RJ-1..10, the three-way convergence, the M12 demand_tc table-backed
finding) and the §20(S) ledger entry.

THE CRUX, MEASURED LIVE (lldb `BuildEagerJoinRegion` / `BuildEagerProductRegion`
hit-counts at this tip; all reproduced by the adjudicator, not taken on trust):

| case | flags | Join dispatch | Product dispatch |
|---|---|---|---|
| demand_tc_witness | -demand | **8** | 0 |
| symrec_tie_1 | — | **4** | 0 |
| booleans | — | **4** | 0 |
| elim-cond-cycle-simple | — | **2** | 0 |
| d5_recursive_negate | — | **0** | 0 |
| join_1 | — | **4** (over 2 join views) | 0 |
| optimize_2 | — | 0 | **2** |
| view_6 | — | 0 | **2** |
| product_ind | — | 0 | **2** |
| product_conds | — | **4** | **0** |
| product_diff / product_self / product_mixed | — | 0 | 0 (kProductArm modeled) |

And the golden fact that refutes the stage-(a) digest: `d5_recursive_negate.deltarel.opt.golden`
census line reads `... kFixpointFire=2 ... kPivotAssemble=1 ...` — Path #2 (the
differential SCC join through `BuildJoin(for_delta=true)`, Stratum.cpp:1532) IS
witnessed by a committed golden. The digest/L2 "all 9 goldens kPivotAssemble=0
kFixpointFire=0" is FALSE.

---

## ADJUDICATIONS (ADJ-RJ-1 .. ADJ-RJ-16)

### ADJ-RJ-1 — Design A's "demand_tc_witness = ZERO-MINT" is REFUTED (A-FATAL-1 / A1 / CM-1 all CONFIRMED)
Measured `BuildEagerJoinRegion` on `demand_tc_witness -demand` = **8** (opt AND
none, stable). Design A's central carrier ruling — demand_tc as the 0-dispatch
negative guard, "inductive joins never enter BuildEagerJoinRegion because
Induction.cpp:726 pre-creates the work item" — is empirically false: the walk-side
`BuildEagerJoinRegion` (sole caller Build.cpp:1249) fires for EVERY walk-reached
join edge, inductive or not; the Induction.cpp:726 site only pre-creates the
`ContinueJoinWorkItem` so the walk's `if(!join_action)` at Join.cpp:757 finds it
present and merely pushes to `inserts` — the walk still dispatches. demand_tc is
the WIDEST real-block carrier (8), not a zero-mint case. **Design A is not
implementable as written**; its three-way convergence would FAIL (author predicts
census-only, blind worktree produces 8 real blocks). SUPERSEDED by Design B.

### ADJ-RJ-2 — the real zero-mint JOIN negative guard is d5_recursive_negate (A-MAJOR-3 / A3 CONFIRMED)
Measured d5 `BuildEagerJoinRegion` = **0**, yet d5 HAS a join emitted through the
differential Path #2 (`BuildJoin(for_delta=true)`, Stratum.cpp:1532;
`kPivotAssemble=1 kFixpointFire=2` in its committed golden). d5 is therefore the
genuine kEagerJoin=0 negative guard — a walk-cut recursive-differential join that
mints ZERO eager markers while its delta maintenance lives in Authority A. Design B
names it correctly; A never identifies it. ADOPTED: d5 is the R-JOIN zero-mint
negative guard (mirrors R4's d5 zero-mint negate guard — same case, second role).

### ADJ-RJ-3 — four existing carriers gain REAL kEagerJoin blocks, not census-line-only (A-MAJOR-2 / A2 / CM-3 CONFIRMED)
Measured: demand_tc=8, symrec=4, booleans=4, elim=2 — all >0, so all four gain
real new-content `op.N kEagerJoin` blocks on re-bless. Design A buckets three of
these (booleans, elim, and — via ADJ-RJ-1 — demand_tc) as "CENSUS-LINE churn
ONLY"; that understates the blast radius by three carriers. ADOPTED (Design B's
account): the real-block carrier set is **{demand_tc(8), symrec(4), booleans(4),
elim(2)}** — the WIDEST existing-carrier blast radius of the Rel epoch (R2/R3
touched priors census-line-only; R4 touched zero priors). This is a review-surface
provisioning fact, not a correctness defect, but it must be stated loudly.

### ADJ-RJ-4 — the stage-(a) digest "all 9 goldens kPivotAssemble=0" is REFUTED (AC-MINOR-4 / AC4 / CM-2 CONFIRMED)
Verified at the golden: `d5_recursive_negate.deltarel.opt.golden` →
`kFixpointFire=2 kPivotAssemble=1`. Only Design B (§0.A Correction 1) caught this.
CONSEQUENCE for Design C: C's entire ADOPT-branch risk dossier ("Path #2 has NO
carrier / a delta-BuildJoin perturbation slips past the suite / side_key_eqs has
zero carrier coverage") rests on this false premise and is REFUTED — d5 is a
committed golden exercising exactly the shared `BuildJoin(for_delta=true)` factory
(Stratum.cpp:1532). The stage-(a) consolidated digest §6 and L2 §0/§4 carry this
erratum; the CORRECT account is Design B's.

### ADJ-RJ-5 — the marker referent is the PER-VISIT DISPATCH edge (referent ruling, all three concur; A/B/C agree)
A kEagerJoin op is minted once per `(pred_view → join_view)` walk edge dispatched
at Build.cpp:1249 (join_1 = 4 markers over 2 join views — per-visit ≠ per-view ≠
per-emission). It records that the eager walk REACHED this join at this predecessor
edge and lowered it IN PLACE by calling the untouched deferral machinery
(`BuildEagerJoinRegion`). It does NOT model the once-per-join deferred `TABLEJOIN`
emission (pivot vector, work-item drain order via `ContinueJoinOrder`, the
pivot-equality belt, CHECKMEMBER gating, successor continuation) — that stays
hand-coded and is owed to R-final. Precedent: kEagerForward is ALSO per-visit
(12 markers over 11 TUPLE views on demand_tc, M15) — so per-visit is settled house
practice; the only NEW wrinkle is that here the underlying emission is DEFERRED
(Run) rather than in place, which the digest §3(a)/§5 flagged as the "id-stream
identity does not transfer mechanically" caveat. It DOES transfer for the MARKER
(ADJ-RJ-7): the marker ctor allocates zero `next_id`, so it rides before
`InTryInsert`/`VectorFor` and the deferred `TABLEJOIN`'s `next_id++` fires in the
identical drain order regardless.

### ADJ-RJ-6 — effect-FREE, not effect-bearing (Design B's effect analysis ADOPTED; A/B/C concur, M14 does NOT transfer)
Both kEagerJoin and kEagerProduct are effect-free markers on the R1-R3 branch,
NOT effect-bearing like R4's kNegateGate. Grounds, verified at the code: (1) a
walk-reached join produces ZERO DR ops today (join_1 `.deltarel` shows only
Compare/Forward/Insert; the join views are invisible), so there is NO pre-existing
eager DR effect to RE-SOURCE — M14 was a relocation of a pre-existing
seed-schedule `kFlagRead` from the deleted `BuildDRInventory` NEGATE_GATE loop; JOIN
has no such antecedent (`kPivotAssemble` is the DISJOINT differential Path #2, not
an eager op). (2) Effect-bearing-ness is a property of the DEP-EDGE the op
contributes to the DR flow, NOT of the runtime reads its lowered region performs —
kEagerForward's region also reads/writes tables (TABLESCAN/INSERT) yet is
effect-free; the join's index scans are identically emission-internal, not DR-flow
`kFlagRead` dep edges. ADOPTED: both kinds join `IsEagerMarkerKind`, lead-0
off-lattice, no `effects`/`reads`/`spine` sublines. The digest §3(c) open question
("does the marker carry pred-table kFlagReads?") is RULED: NO.

### ADJ-RJ-7 — id-stream identity holds for the MARKER despite deferral (next_id safety, verified)
Every R1-R4 ctor (DeltaRel.cpp:1279-1389) sets value fields only and allocates no
`impl->next_id`. `MakeEagerJoinOp`/`MakeEagerProductOp` follow verbatim. At the
walk moment the marker mint + `RecordEagerDispatch` ride BEFORE
`BuildEagerJoinRegion`'s `InTryInsert(pred_view)` and first-visit `VectorFor`, and
enroll at op-TAIL after the ingest folds (keeping ingest fold op.0/op.1). The
deferred `TABLEJOIN` (`impl->next_id++`, Join.cpp:313) fires later in work-item
drain order, entirely unperturbed by a next_id-free marker. Byte-identity A/B on
generated C++ is therefore preserved in all 4 modes (the SHARED-VERIFIED /
CM-7 verdict). VERIFIED against the mold at DeltaRel.cpp:1279-1360.

### ADJ-RJ-8 — SLICE SCOPE: JOIN + PRODUCT together, one slice (SCOPE-B-OVER-C CONFIRMED; Design B ADOPTED over Design C's pivots-only)
The R1/R2/R3 precedent is two-kind sibling PAIRS per slice (forward+insert /
compare+generate / union+select). `BuildEagerProductRegion`'s deferral structure
is a byte-identical MIRROR of the join's (Product.cpp:9-42 == Join.cpp:9-42;
same M14/M2'/M7 analysis transfers verbatim), so pairing them is the cheapest
correct grouping. Design C's pivots-only split double-churns the census (24→25 on
all 9 committed goldens, re-bless; then a later R-PRODUCT 25→26, re-bless all 9
again) for no isolation benefit. C's counter — "product has no committed
`.deltarel` golden, so a product carrier is NEW work landing in-slice" — is
CONFIRMED-true but CHEAP to satisfy (ADJ-RJ-13: reuse `optimize_2`, an EXISTING
corpus case with Product-dispatch=2, via an `.irgold`/`.deltarel.opt` sidecar — no
new `.dr`/`.main.cpp`). ADOPTED: one slice, two kinds. (Ritual-head ruling; if the
owner prefers pivots-only, see HEAD-RULING (i).)

### ADJ-RJ-9 — Design C's product carrier `product_conds` is REFUTED (C1 CONFIRMED)
Measured `product_conds`: `BuildEagerProductRegion`=0, `BuildEagerJoinRegion`=**4**
— its zero-arity conditions desugar into UNIT PIVOT-JOINS, so it reaches joins, not
products. A product carrier seeded on it mints zero kEagerProduct markers. The
valid acyclic-product positives are `optimize_2` / `view_6` / `product_ind`
(Product-dispatch=2 each, measured). ADOPTED: `optimize_2` is the R-JOIN product
carrier.

### ADJ-RJ-10 — the pivot-belt fold: REDUNDANCY-HOLDS, but DECLINE from R-JOIN (all three DECLINE; G1 confirms redundancy)
NO critic refuted the redundancy premise. Re-verified at the code: `Index::First`
(Table.h:801) does exact full-key open addressing (`slot.hash==hash && slot.key==key`),
`Index::Next` (Table.h:807) walks the single per-exact-key chain, and the
no-index `assert(false)` path is dead (`Data.cpp:352-355` null-index return is
commented out). So every scanned pivot column equals `@pivot` by construction and
the monotone-form `TUPLECMP` belt (Join.cpp:317-321, filled :513-519) is
tautologically redundant at this tip. **fold_premise_status = REDUNDANCY-HOLDS.**
BUT the belt is ALSO (a) the structural body-anchor `join->body` (BuildJoin returns
it as `parent`, child regions attach under it) and (b) the provenance-hider
(Join.cpp:532-534 remaps the most-represented view's pivot output to a scan var).
Folding it RELOCATES both roles → an EMISSION-shape change (generated C++ changes),
categorically unlike R-JOIN's byte-identity markers, and it touches the SHARED
`BuildJoin` (Stratum.cpp:1532 for_delta=true delta path). DECLINE from R-JOIN; land
it as its own gated emission-shape follow-up, naturally at R-final. (This REVISES
the epoch-brief §5 pre-registration on principled grounds; see HEAD-RULING (iv).)

### ADJ-RJ-11 — Design C's "delta-BuildJoin has zero carrier coverage" is REFUTED (C2 CONFIRMED)
C's blast-radius argument for the fold — "Path #2 has NO carrier, a delta-BuildJoin
perturbation slips past the golden suite" — is false: `d5_recursive_negate`
exercises `BuildJoin(for_delta=true)` at Stratum.cpp:1532 (kPivotAssemble=1) and is
a committed golden, so a shared-factory regression IS caught. C reaches the correct
DECLINE verdict via a false premise; the shared-`BuildJoin` blast radius is LOWER
than C's MEDIUM-HIGH framing. (Does not change the DECLINE recommendation — the
fold is declined on emission-shape grounds regardless, ADJ-RJ-10.)

### ADJ-RJ-12 — Design C's Part J carrier table uses the WRONG predictor (C-MAJOR-1 / C-MAJOR-2 / CM-5 / C3 CONFIRMED)
C's Part J predicts marker-block counts from the per-EMISSION `.ir` join-tables
count (demand_tc=5, symrec=2, booleans=2, elim=1, d5=1) — the exact per-visit ≠
per-emission M15 trap C's own Part I warns against. The correct per-VISIT dispatch
counts are 8/4/4/2/**0**. C's predicted census values would RED at the M15 oracle
and three-way convergence for every one of its goldens, and it mis-roles d5 as
"NEW blocks" when d5 is kEagerJoin=0 (census-line-only). ADOPTED: the census
predictor is the BuildEagerJoinRegion dispatch hit-count, never `.ir` join-tables
(ADJ-RJ-15).

### ADJ-RJ-13 — Design A's new-case `product_mono.dr` is mis-ranked vs `optimize_2` (CM-6 CONFIRMED)
A recommends a purpose-built `product_mono.dr` (+ `.main.cpp` + all-4-modes-clean
verification) as the first kEagerProduct golden. An EXISTING case dispatching
Product=2 (`optimize_2`) + an `.irgold`/`.deltarel.opt` sidecar is strictly cheaper
and equally isolating — a `.deltarel` golden is a compiler dump, needing no driver.
ADOPTED: `optimize_2.deltarel.opt` (new sidecar on the existing case). `view_6` /
`product_ind` are equivalent fallbacks.

### ADJ-RJ-14 — forward-compat ledger: the per-visit marker is HALF the R-final substrate (CM-4 CONFIRMED; Design B §9.2 ADOPTED)
The `TABLEJOIN` emits once-per-join at work-item DRAIN in `ContinueJoinWorkItem::Run`
(Join.cpp:558-704, ordered by `ContinueJoinOrder` = depth|induction-class,
Join.cpp:9-42), NOT in walk/inventory order. So the kEagerJoin per-visit census is
NOT the TABLEJOIN count, and R-final's direction flip will need a SEPARATE
per-JOIN emission op carrying the drain-order key — the per-visit marker is only the
REACHABILITY half of the R-final substrate. Design A (Part 2/10) and C (Part B)
over-claim the marker as "THE prerequisite/substrate", hiding half the ledger. This
contract RECORDS the full ledger: R-JOIN delivers the reachability marker; R-final
owes the emission op + the belt-fold-as-access-plan-attribute.

### ADJ-RJ-15 — the M15 count-oracle statement (per-VISIT + emission-side; all designs' M12/M15 headline risks folded in)
The census `kEagerJoin=N` / `kEagerProduct=M` for a given (case, mode) EQUALS the
`BuildEagerJoinRegion` / `BuildEagerProductRegion` dispatch hit-count at that
(case, mode) — measured per-VISIT, on the emission side (at the mint), NEVER the
`.ir` join-tables count (per-emission TABLEJOIN), NEVER the number of join views,
NEVER a same-graph re-derivation. By construction each dispatch mints exactly one
marker and enrolls it; census == dispatch is definitional. A.6(c) is M10-STRENGTHENED
on the pivot-count discriminant (kEagerJoin arm: `v.IsJoin() && NumPivotColumns()>0`;
kEagerProduct arm: `v.IsJoin() && NumPivotColumns()==0`) because `IsJoin()` alone is
ambiguous between the two kinds — the kEagerUnion `!InductionGroupId` strengthen
precedent (DeltaRel.cpp:3543-3556).

### ADJ-RJ-16 — the C-CREDIT-DB / side_key_eqs erratum + the M12 model-table residual (C-CREDIT-DB / M12-RESIDUAL CONFIRMED, non-blocking)
(a) Design C's Database.cpp erratum is CORRECT and credited: the brief's
`side_key_eqs` cite "Database.cpp:2740-2744" is stale (actual comment :2788, decl
:2791, build :2862, use :2896); the delta `side_key_eqs` fold is a SEPARATE third
diff with no Path #2 carrier and must NOT ride R-JOIN. (b) M12 residual: all three
predict `ModelTableOrNull(join_view)==null → table-less lead-0`, correctly flagged
for the stage-(d) fprintf probe; `.df class=table-less` is NOT authoritative for
model-null (E-107). This is the ONE golden-byte prediction that must be probed live
before bless (a shared-model join would render `table=` and sort off lead-0, AND
would trip the A.6(c) `op.table_op_table != ModelTableOrNull` check at
DeltaRel.cpp:3573 — doubly load-bearing). Appropriately hedged by all; not a defect.

MINOR / prose: B1 (CONFIRMED) — Design B occasionally phrases dispatch counts as
"N joins"; tighten to "N dispatches over K join views" in the final contract. No
substantive impact.

---

## DESIGN BODY

### §1 Scope
ONE slice, TWO effect-free marker kinds: `kEagerJoin` (pivot-join,
NumPivotColumns>0) + `kEagerProduct` (zero-pivot @product). Both builders
(`Join.cpp` 785 L, `Product.cpp` 386 L) are UNTOUCHED; thin `LowerRelStep_Join` /
`LowerRelStep_Product` wrappers forward the exact builder signatures and call the
untouched region builders in place. EXCLUDED from this slice: the pivot-equality
belt fold (ADJ-RJ-10, R-final-owed), the `side_key_eqs` delta fold (ADJ-RJ-16,
separate diff), the dead `BuildNestedLoopJoin` removal (Join.cpp:179-294, optional
hygiene — may ride as a separate commit but is NOT required), and the R-final
per-join emission op (ADJ-RJ-14).

### §2 Referent + normative meaning sentence
Per-VISIT dispatch edge (ADJ-RJ-5). NORMATIVE: *"A `kEagerJoin` op marks that the
eager walk reached a pivot-join at one `(pred_view → join_view)` dispatch edge
(Build.cpp:1249) and lowered it in place via the untouched deferral machinery; it
is a per-visit reachability record, not the once-per-join deferred `TABLEJOIN`
emission (which remains hand-coded, owed to R-final). `kEagerProduct` is the
zero-pivot analog at Build.cpp:1251."*

### §3 Effect classification
EFFECT-FREE (ADJ-RJ-6). Both kinds join `IsEagerMarkerKind`; lead-0 off-lattice;
no `effects`/`reads`/`spine` sublines; `table=` omitted when null (join views are
table-less → null → lead-0, pending the M12 probe).

### §4 The M6' hand-edit site list (VERIFIED COMPLETE — grep-exhaustive: only
DeltaRel.cpp + Format.cpp carry `case DROpKind::` switches; CM-7 confirmed)

1. **DeltaRel.h** — `enum class DROpKind`: add `kEagerJoin`, `kEagerProduct`;
   declare `MakeEagerJoinOp(QueryView, TABLE*)` + `MakeEagerProductOp(QueryView, TABLE*)`.
2. **DeltaRel.cpp**:
   - `MakeEagerJoinOp` / `MakeEagerProductOp` ctors — next_id-free, effect-free,
     verbatim clones of `MakeEagerUnionOp` (DeltaRel.cpp:1342): set `ctx=kEager`,
     `eager_view=join_view`, `table_op_table=table` (= `ModelTableOrNull`, null for
     joins). NO effects push.
   - `IsEagerMarkerKind` (:1306) — add both kinds (predicate becomes 8 kinds).
   - EAGER_WEB re-invocation switch (:2450) — add two cases calling the new ctors
     with `(*rec.view, rec.table)` (RecordEagerDispatch's default eager_view/table
     path carries them; NO RecordEagerDispatch edit).
   - A.6(c) recount switch (:3522) — add two M10-STRENGTHENED arms (ADJ-RJ-15):
     kEagerJoin → `v.IsJoin() && QueryJoin::From(v).NumPivotColumns()>0`;
     kEagerProduct → `v.IsJoin() && NumPivotColumns()==0`.
   - key_of (:4492), op_table_id (:4426), DROpStratum default (:4179) — **NO edit**
     (key_of routes via `IsEagerMarkerKind`; op_table_id null→sentinel-0;
     DROpStratum default→0). VERIFIED.
3. **Format.cpp**:
   - `DROpKindName` (:96) — add `"kEagerJoin"` / `"kEagerProduct"`.
   - render switch (:871ff) — add two DEDICATED cases, byte-identical to the
     kEagerForward shape (:871-879): `sign=· ctx=eager stratum=0`, `args:` with
     `table=` omitted when null. NO new token → NO E-71 (ADJ §5 stage-(d)).
   - `kAllKinds[]` census array (:1027) — add both (census 24→26).
4. **Build.cpp**:
   - dispatch mint at :1249 (kEagerJoin) and :1251 (kEagerProduct) — replace the
     bare `BuildEagerJoinRegion(...)` / `BuildEagerProductRegion(...)` calls with
     `const DROp op = MakeEagerJoinOp(view, ModelTableOrNull(impl, view));
      LowerRelStep_Join(impl, context, op, pred_view, join, parent, last_table);`
     (and the product analog).
   - `LowerRelStep_Join` / `LowerRelStep_Product` wrappers — clone `LowerRelStep_Union`:
     `RecordEagerDispatch(op)` then call the untouched builder with the identical
     forwarded signature (`pred_view`, `join`/product view, `parent`, `last_table`
     are all params). Declared alongside the other wrappers.

No other file changes. `next_id` untouched → byte-identity generated C++.

### §5 Count-oracle statement (M15-conformant)
Per ADJ-RJ-15: `kEagerJoin=N` at (case,mode) == `BuildEagerJoinRegion` dispatch
hit-count at (case,mode); `kEagerProduct=M` == `BuildEagerProductRegion` hit-count.
Per-VISIT, emission-side, no same-graph re-derivation. The bless-time structural
control is the lldb hit-count read (ADJ-S10 precedent), cross-checked by the
three-way convergence.

### §6 Carrier / golden plan (exact case names + pre-registerable predictions)

NEW RAT-8 `.deltarel.opt` goldens (2):
- **join_1** — acyclic 2-pivot-join positive; PREDICT `kEagerJoin=4` (2 join views
  × 2 pred edges each), both table-less lead-0, args `table=` OMITTED (M12-probe).
- **optimize_2** — acyclic @product positive; PREDICT `kEagerProduct=2`, table-less
  lead-0, `table=` omitted (M12-probe). Existing corpus case; sidecar only.

RE-BLESS existing committed `.deltarel.opt` goldens (9), census 24→26 on all:
- REAL-BLOCK carriers (4): **demand_tc_witness** `kEagerJoin=8`, **symrec_tie_1**
  `kEagerJoin=4`, **booleans** `kEagerJoin=4`, **elim-cond-cycle-simple**
  `kEagerJoin=2`. (Widest existing-carrier blast radius of the Rel epoch — ADJ-RJ-3.)
- CENSUS-LINE-ONLY carriers (5): **d5_recursive_negate** (kEagerJoin=0 — the
  zero-mint join NEGATIVE guard, ADJ-RJ-2; its differential join stays in
  kPivotAssemble=1), **map_3**, **merge_2**, **negate_1**, **negate_6** (+2 columns,
  count 0).

NEGATIVE guards (dispatch=0, mint zero markers): **d5_recursive_negate** (kEagerJoin=0,
differential join), and for the product kind **product_diff / product_self /
product_mixed** (kEagerProduct=0, kProductArm-modeled differential products). These
need not all be committed goldens; d5 already is (dual-role with R4). Recommend one
product differential guard as an `.irgold`-less spot-check at stage-(d) if not
blessed.

M9 existing-carrier coverage is DESIGN INPUT (measured, not asserted): the 4
real-block carriers span acyclic-inductive-monotone (symrec), demand-recursive
(demand_tc), unit/condition-desugared joins (booleans), and cond-cycle (elim).

### §7 Gates plan
- **Byte-identity A/B** vs frozen baseline, all 4 modes, full corpus (generated C++
  unchanged — the marker is dump-only; next_id untouched per ADJ-RJ-7).
- **Census extended DAY ONE** (kAllKinds 24→26; DELTAREL-DUMP abort guards a 27th).
- **irgold re-bless ritual** for the 2 new + 9 re-blessed goldens: pre-bless reds
  EXACTLY the expected IRGOLD-DIVERGE set (2 missing + 9 census/block churn),
  sources byte-verified same-as-reviewed, then `runall.sh --bless`.
- **RAT-8** for join_1 + optimize_2 (first-ever join/product `.deltarel` goldens).
- **M15 count-oracle**: lldb hit-count == census on all carriers, blind-lane, all modes.
- **eqgate green** (demand_tc_witness / demand_neighborhood_witness — dump-only,
  answers unchanged); check the OD-10 pin on demand_tc_witness.irgold is lifted
  before re-blessing (B's headline risk).
- **ctest 5/5 debug + 5/5 ASAN**; ASAN both surfaces PASS(173) zero reports.
- **config-invariance SINGLE-HASH** (trio × opt+none); E-62 re-grep CLEAN.
- **bench A/B** NOT required (no emission change) — but run progsize@128 ABABAB as
  the standing 0.0%-median witness.
- **M12 fprintf probe** (mandatory before pinning golden bytes): `ModelTableOrNull(impl, join_view)`
  on join_1's 2 join views + optimize_2's product view == null (predicts table-less
  lead-0; a non-null result changes the golden bytes AND trips A.6(c):3573).

### §8 Residuals
- **R-final owes** the per-JOIN emission op carrying the `ContinueJoinOrder`
  drain-order key (ADJ-RJ-14) — the per-visit marker is only the reachability half.
- **The pivot-belt fold** (ADJ-RJ-10) — REDUNDANCY-HOLDS but deferred to a gated
  emission-shape follow-up (R-final's eager-vs-frontier lowering choice, where
  probe-exactness becomes a modeled access-plan attribute).
- **The `side_key_eqs` delta fold** (ADJ-RJ-16) — separate third diff, no Path #2
  carrier, off R-JOIN.
- **Dead `BuildNestedLoopJoin`** (Join.cpp:179-294, guarded by `assert(false)` at
  :775 under the `true ||` at :744) — optional removal, may ride as a separate
  hygiene commit.
- **Path #2 product coverage** — no committed product `.deltarel` golden exercises
  a differential product's kProductArm in the modeled-block sense beyond census;
  d5 covers the join delta path. Consider a differential-product golden at R-final.

---

## HEAD-RULING RECOMMENDATION SET (verbatim-quotable)

### (i) SLICE SCOPE
**Recommendation:** JOIN + PRODUCT together, ONE slice, two effect-free kinds
(`kEagerJoin` + `kEagerProduct`) — the R1/R2/R3 two-kind-sibling-pair precedent.
Product carrier = the EXISTING `optimize_2` case (`.deltarel.opt` sidecar; Product
dispatch=2 measured), so pairing adds no new `.dr`/`.main.cpp`.
**Strongest argument AGAINST:** PRODUCT has no committed `.deltarel` golden today
(CONFIRMED), so the product kind lands unwitnessed-by-prior — a pivots-only slice
would let the join kind land on 4 measured existing carriers first and defer the
product carrier question. **If the owner overrules → pivots-only:** mint only the
:1249 branch; census grows 24→25 on all 9 committed goldens (re-bless once), then a
later R-PRODUCT grows 25→26 (re-bless all 9 AGAIN) — deadweight double-churn; the
:1251 product branch stays a bare unmodeled dispatch and the "two unmodeled arms
remain (JOIN done, PRODUCT + E-42)" CLAUDE.md line must be re-split.

### (ii) MARKER REFERENT + normative meaning sentence
**Recommendation:** the PER-VISIT dispatch edge at Build.cpp:1249/:1251 (per-visit
≠ per-view ≠ per-emission; join_1 = 4 markers over 2 join views). NORMATIVE
SENTENCE (for the docs): *"A `kEagerJoin` op marks that the eager walk reached a
pivot-join at one `(pred_view → join_view)` dispatch edge and lowered it in place
via the untouched deferral machinery; it is a per-visit reachability record, NOT
the once-per-join deferred `TABLEJOIN` emission — which stays hand-coded and is
owed to R-final. `kEagerProduct` is the zero-pivot analog."*
**Strongest argument AGAINST:** the underlying `TABLEJOIN` is emitted ONCE per join
at work-item drain (Join.cpp:558-704, `ContinueJoinOrder`), so a reader may expect
the census to equal the join count; a per-JOIN referent would align census with
emission. **If the owner overrules → per-join referent:** the marker can no longer
mint at the dispatch site (the join isn't known-once until Run), so it must move
INTO `ContinueJoinWorkItem::Run` / the induction setup, breaking the "mint in place
at the walk site" mold shared by R1-R4, and the M3 id-stream-identity argument
(next_id-free mint before InTryInsert) no longer applies mechanically — the marker
would then need the drain-order key and effectively BECOMES the R-final emission
op, collapsing R-JOIN and R-final into one much larger slice.

### (iii) EFFECT-FREE vs EFFECT-BEARING
**Recommendation:** EFFECT-FREE (both kinds join `IsEagerMarkerKind`, lead-0
off-lattice, no dep edges). M14 (the kNegateGate effect) does NOT transfer: it was
a RE-SOURCE of a pre-existing seed-schedule `kFlagRead`; JOIN/PRODUCT have no
pre-existing eager DR op to preserve (`kPivotAssemble` is the disjoint differential
Path #2). Effect-bearing-ness is a property of the DEP-EDGE the op contributes, not
the runtime reads its lowered region performs (kEagerForward reads/writes tables
yet is effect-free).
**Strongest argument AGAINST:** the join's real data lives in PREDECESSOR tables
(%table:12/16/19/23 for join_1) that the marker's lowered region genuinely reads
via index scans — one could argue the marker should carry those pred-table
`kFlagRead`s (like kNegateGate carries the negated table's), making the DR flow's
dep graph reflect the read. **If the owner overrules → effect-bearing:** each
kEagerJoin gains N pred-table `kFlagRead`s (one per joined side), moving it OFF
lead-0 onto the dependence lattice — this changes the V-READY/Kahn banding and the
key_of branch (it would need its own gate-style branch like kNegateGate, OUT of
`IsEagerMarkerKind`), changes the render (effects: sublines appear), and forces an
E-71 grammar lane for the reads token. It also re-opens whether the dep edges are
byte-stable under tail-append. Net: a materially larger, R4-shaped slice for zero
lowering benefit (the reads are already emitted correctly by the untouched builder).

### (iv) THE PIVOT-BELT FOLD
**fold_premise_status: REDUNDANCY-HOLDS.** NO critic refuted the redundancy
premise; it is re-verified at the code (Table.h:801 exact full-key `slot.key==key`;
`Data.cpp:352-355` null-index return commented out; the belt TUPLECMP is
tautologically true after exact-key scans). There is NO reachable non-tautological
TUPLECMP. Therefore (iv) is DECLINE on EMISSION-SHAPE grounds — not on a refuted
premise.
**Recommendation:** DECLINE the fold from R-JOIN. Keep R-JOIN a pure byte-identity
modeling slice; land the belt fold as its own gated emission-shape follow-up
(structural gates + irgold re-bless + bench A/B), naturally at R-final where
eager-vs-frontier lowering becomes a modeled access-plan attribute.
**Strongest argument AGAINST DECLINE:** the belt is dead weight shipping in every
monotone join today; folding it now (while the code is open) saves a future bless
cycle and removes a confusing stale comment (Join.cpp:513-516 claims the belt
"permits approximate scans" — false for the shipped exact Index).

**CRISP OWNER-QUESTION (both branches' costs):**
> The pivot-equality belt `TUPLECMP` (Join.cpp:317-321, filled :513-519) is
> tautologically redundant at this tip (exact full-key `Index`, Table.h:801) —
> VERIFIED, redundancy HOLDS. But it is ALSO the structural body-anchor
> (`join->body`, returned as `parent`) and the provenance-hider (Join.cpp:532-534).
> Folding it changes generated C++ (an EMISSION-shape change), unlike R-JOIN's
> byte-identity markers, and touches the SHARED `BuildJoin` (Stratum.cpp:1532
> for_delta=true).
>
> • **DECLINE (recommended):** R-JOIN stays byte-identity (trivial A/B in all 4
>   modes). The redundant TUPLECMP keeps shipping — no regression, it ships today.
>   The fold lands later at R-final as a gated access-plan lowering choice. Cost:
>   one extra bless cycle later; a known-dead comment lingers.
> • **FOLD-NOW:** the belt fold rides R-JOIN as a SEPARATED second commit (the
>   marker commit lands first/green). Cost: R-JOIN forfeits its trivial
>   byte-identity A/B safety net; needs structural gates + irgold re-bless +
>   bench A/B + a shared-`BuildJoin` blast-radius review (delta path IS carrier-
>   covered by d5, kPivotAssemble=1 — so the review is bounded) IN THE SAME SLICE;
>   zero modeling benefit (the belt is invisible in the `.deltarel`).

---

## STAGE-(d) CHARTER — what stage-(d) must produce

### Desired-state surfaces (which dumps on which carriers)
- **`.deltarel.opt` (NEW, RAT-8):** `join_1` (predict `kEagerJoin=4`, two
  table-less lead-0 blocks per join view, `args:` with `table=` OMITTED);
  `optimize_2` (predict `kEagerProduct=2`, table-less lead-0, `table=` OMITTED).
- **`.deltarel.opt` (RE-BLESS, real blocks + census 24→26):** `demand_tc_witness`
  (`kEagerJoin=8`), `symrec_tie_1` (`=4`), `booleans` (`=4`),
  `elim-cond-cycle-simple` (`=2`).
- **`.deltarel.opt` (RE-BLESS, census-line-only 24→26, count 0):**
  `d5_recursive_negate` (kEagerJoin=0 — the zero-mint join negative guard; its
  `kPivotAssemble=1` line UNCHANGED), `map_3`, `merge_2`, `negate_1`, `negate_6`.
- Confirm `opt==nocf` and `nodf==none` for all 11 (dump-only, mode-invariant).

### E-71 grammar lane — NOT NEEDED
The render reuses the bare kEagerForward production verbatim (sign/ctx/stratum +
`args:` with `table=` omitted-when-null); NO new op-block token. The only new
render text is the `DROpKindName` census tokens `"kEagerJoin"`/`"kEagerProduct"`,
which inherit the kEagerUnion/kEagerSelect naming precedent mechanically (a census
token, not an op-grammar production). FLAG it in the t2b-grammar note, but no E-71
adjudication is triggered. (If — and only if — the M12 probe forces a discriminating
token, e.g. a `pivots=N` arg, THAT would trigger an E-71 lane; it is DECLINED
unwitnessed.)

### Blind-prototype lane charter
1. **Author hand-prediction (dump-blind):** derive the 6 carrier census lines
   (join_1=4, optimize_2=2, demand_tc=8, symrec=4, booleans=4, elim=2; d5=0 +
   4 census-only) from the walk DFS + measured dispatch counts, and the block
   PLACEMENT (all table-less lead-0, sort by op_table_id sentinel-0 then ctor
   order). MUST converge byte-identical with the blind worktree prototype AND the
   pristine implementation × 3, on ALL modes.
2. **M12 fprintf probe (MANDATORY, before pinning golden bytes):** instrument
   `ModelTableOrNull(impl, join_view)` on join_1's 2 join views + optimize_2's
   product view; confirm == null (predicts table-less lead-0 + `table=` omission).
   A non-null result changes the predicted bytes AND trips the A.6(c) `op.table_op_table
   != ModelTableOrNull` check at DeltaRel.cpp:3573 — abort-loud, re-derive.
3. **M15 count-oracle blind lane:** lldb `BuildEagerJoinRegion`/`BuildEagerProductRegion`
   hit-counts on all carriers == census, all 4 modes (11/11 or the exact carrier set).
4. **Byte-identity generated-C++ A/B:** full corpus, 4 modes, vs frozen baseline —
   ZERO divergence (the slice is dump-only; next_id untouched).
5. **B1 prose fix:** the final contract states dispatch counts as "N dispatches over
   K join views", never "N joins".
