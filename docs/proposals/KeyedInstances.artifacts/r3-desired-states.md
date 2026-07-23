======================================================================
COMMITTED AT THE R3 LANDING (2026-07-22; landing record §20(L)). The
ADJUDICATED R3 desired-output-states contract (DS-R3-1..9), authored
against REAL tip dumps on the r2-desired-states mold, with the house
BLIND op-inventory re-derivation lane (forbidden from every prior R3
doc) and the E-71 grammar-conformance lane, then xhigh-adjudicated
number-by-number (d-adjudication): ONE systematic discrepancy resolved
AT PROBE — the kEagerUnion markers on the witnessed carriers are
TABLE-BACKED (ModelTableOrNull reads the ControlFlow DataModel
equivalence-set, NOT the .df per-view class; merge_2 5 unions =
%table:4 x3 + %table:8 x2) — AM-1..AM-9 applied in place (E-107).
EVERY [BYTE]/[STRUCT] prediction below MATCHED at the landing: censuses
(merge_2 20 ops 10F/5I/5U; booleans 11 ops 6F/2I/2fold/1S@%table:4;
elim 9 ops 6F/2I/1S@%table:5 with the induction-owned merge minting
ZERO unions — the negative guard), the three existing carriers'
census-tail-only churn, DS-ADJ-1 cross-knob (opt==nocf, nodf==none:
merge_2 20/20/53/53, booleans 11/11/18/18, elim 9/9/24/24), the
DS-ADJ-4 hexdump pins (sign=· c2 b7; `    args: table=%table:4` =
20 20 20 20 61 72 67 73 3a 20 74 61 62 6c 65 3d 25 74 61 62 6c 65 3a
34 0a), the six-red pre-bless set, and the git-status postcondition
(exactly 3 modified + 3 new .deltarel goldens + 3 sidecars).
======================================================================

# R3 desired-states — the eager-web plain-MERGE-union + SELECT-rebind marker ops in the `.deltarel` dump

R3 recap (design §0/§A): model the monotone eager web's **plain-MERGE-union**
dispatch arm (`Build.cpp:1189-1197`, the `else`/non-inductive leg →
`BuildEagerUnionRegion`, `Union.cpp`) and the **SELECT-rebind** arm
(`Build.cpp:1233-1248`, the unit-condition INSERT→RELATION→SELECT rebind) as two
new effect-free DR-IR op kinds — `kEagerUnion` (22) / `kEagerSelect` (23) — minted
at the walk, enrolled by the EAGER_WEB block STRICTLY AFTER the ingest folds
(ADJ-S2), lowered IN PLACE by `LowerRelStep_Union`/`LowerRelStep_Select` wrappers
calling the untouched region builders (`BuildEagerUnionRegion` unchanged;
`BuildEagerSelectRegion` = the extracted-verbatim inline rebind, §A.2(A)). NO new
`DROp`/`EmittedEagerOp` field (M2'): a union carries no operator/functor; a
select's unit-condition-ness is view-derivable, and R3 renders NOTHING extra for
it (§F.4). **Zero emission change.** The ONLY bytes that move anywhere in the
corpus are the `.deltarel` dumps: census-line-only on the three existing carriers,
plus the new union/select op blocks on the three new carriers.

Census **22 → 24** (`kEagerUnion=<n> kEagerSelect=<n>` appended at the tail, enum
order, day one — F-CENSUSABORT forces `kAllKinds` to 24).

---

## (0) REAL BASELINE CAPTURE (tip 694b4fff, all 4 modes, this session)

Regenerated LIVE into `scratchpad/r3/ds-base/` exactly as `run_irgold` compiles
(opt = no opt flags; the sidecar drives only the opt compile; nocf/nodf/none by hand
for the referee reads):

```
DR=build/debug/bin/drlojekyll
$DR cases/<case>.dr [flags]                       -deltarel-out .. -df-out ..
  flags per mode: (none)=opt | -disable-controlflow-opt=nocf
                  -disable-dataflow-opt=nodf | both=none
```

**Existing-carrier baseline census lines (VERBATIM from the committed goldens,
verified this session):**
- `demand_tc_witness`: `… kEagerForward=12 kEagerInsert=2 kEagerCompare=0 kEagerGenerate=0`
- `symrec_tie_1`:      `… kEagerForward=7 kEagerInsert=1 kEagerCompare=0 kEagerGenerate=0`
- `map_3`:             `… kEagerForward=3 kEagerInsert=3 kEagerCompare=1 kEagerGenerate=3`

**New-carrier baseline op TOTALS (current PRE-R3 opt `.deltarel`, this session):**
merge_2 = 15 ops (10F/5I); booleans = 10 ops (6F/2I/2Ingest); elim-cond-cycle-simple
= 8 ops (6F/2I); select_2 = 6 ops (4F/2I). NONE has a committed `.deltarel` golden
yet (RAT-8 first-ever seeds).

---

## (1) [BYTE] — ZERO EMISSION CHANGE corpus-wide (the hard gate)

**Prediction (design G.1 / §A.3): `.df`, `.ir`, `.h`, `.cpp`, `stdout` byte-identical
corpus-wide, all 173 cases, all 4 optimization modes, vs frozen baselines
debug c0a8a819 / release 958ddf8b.** Mechanism (mold M3, mechanical): the mint +
`RecordEagerDispatch` + EAGER_WEB enrollment allocate ZERO `impl->next_id` and
Emplace NO region; `ModelTableOrNull` is a `.find()` read; `MakeEagerUnionOp`/
`MakeEagerSelectOp` are POD-field stores; the wrappers CALL the untouched
`BuildEagerUnionRegion` / the extracted-verbatim `BuildEagerSelectRegion` with
identical args at the identical walk moment ⇒ every `impl->next_id++` fires in the
identical order ⇒ ControlFlow IR byte-identical, `.h`/`.cpp` pure functions of that
IR, `.df` upstream of the whole CF build. `Union.cpp`, `InTryInsert`,
`BuildEagerInsertionRegions`, `BuildUpdateCount` stay byte-identical. **[BYTE]**

**The committed non-`.deltarel` IR goldens stay byte-identical [BYTE]:**
`symrec_tie_1.{ir,df}.opt.golden`, `demand_tc_witness.{ir,h,df}.opt.golden`,
`map_3.stdout`, and every new-carrier `.stdout` / existing golden. The 676-row
knob-off A/B (×2), the post-baseline-4 set (incl. `demand_neighborhood_witness`
nested eqgate ×4 modes), and the `data/` A/B all stay **0-diverged**. **If ANY
`.h`/`.ir`/`.df` golden moves at bless → R3's zero-emission-change claim is ALREADY
violated → STOP** (§7 tripwire). This is the SOLE miscompile referee — it nets that
the recorded dispatch stream (hence emission) did not move.

---

## (2) [BYTE-text/STRUCT-append] — the THREE existing carriers: census-line-ONLY churn 22→24

tc, symrec, AND map_3 contain NO eagerly-reached plain-union / unit-condition-select
view ⇒ NO new op BLOCKS ⇒ **census-line-only churn** (E-101 logic — census renders
zero-count kinds). GROUNDED THIS SESSION (`ds-base/<c>/opt/df`):
- `demand_tc_witness`: merge.17/merge.18 are TABLE-BACKED monotone (`table=%table:8`
  / `%table:12`, `class=monotone set=0 depth=1`) ⇒ **inductive** ⇒ route to
  `BuildEagerInductiveRegion`, mint ZERO kEagerUnion (§B.5 mint guard
  `!InductionGroupId()`). Zero `select … relation …` unit-cond selects. ⇒ `0/0`.
- `symrec_tie_1`: merge.10 TABLE-BACKED monotone (`%table:4`) ⇒ inductive ⇒ 0 union.
  Zero unit-cond selects. ⇒ `0/0`.
- `map_3`: ZERO merges, zero unit-cond selects. ⇒ `0/0`.

Every op block, `rounds:`, `deps:` **byte-identical [BYTE]**. The SOLE changed line
is the census tail append ` kEagerUnion=0 kEagerSelect=0` (enum/kAllKinds tail order
Forward,Insert,Compare,Generate,**Union,Select**). **Full desired census lines (the
one changed line each) — [BYTE], grounded on the current blessed lines + the
mechanical append:**

```
# demand_tc_witness.deltarel.opt.golden
census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=2 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0 kEagerForward=12 kEagerInsert=2 kEagerCompare=0 kEagerGenerate=0 kEagerUnion=0 kEagerSelect=0

# symrec_tie_1.deltarel.opt.golden
census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=1 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0 kEagerForward=7 kEagerInsert=1 kEagerCompare=0 kEagerGenerate=0 kEagerUnion=0 kEagerSelect=0

# map_3.deltarel.opt.golden
census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=0 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0 kEagerForward=3 kEagerInsert=3 kEagerCompare=1 kEagerGenerate=3 kEagerUnion=0 kEagerSelect=0
```

These are the ONLY changed bytes in the three files.

---

## (3) [STRUCT→BYTE] — the THREE new carriers' predicted `.deltarel.opt` dumps

Method: take the CURRENT (observable, PRE-R3) opt `.deltarel` (every forward/insert/
ingest block below is VERBATIM from `ds-base/<c>/opt/deltarel` at tip 694b4fff) and
extend it with the predicted union/select marker blocks at their `key_of` positions.
Marker LINE TEXT is **[BYTE-text]** (fully determined: `sign=·` = `\xC2\xB7`,
`ctx=eager`, `stratum=0`, table-less→bare `args:`, table-backed→` table=%table:N`).
The op.`<idx>` NUMBERS and the intra-band INTERLEAVE are **[STRUCT]** — enrollment
tail-appends the new markers into the walk/DFS stream, so every op ctor index
RENUMBERS; each new marker sorts into the `key_of` band its OWN `ModelTableOrNull`
result assigns it to — a table-BACKED marker joins its `%table:N` block (interleaving
with that block's existing fwd/ins pair by ctor), and only a table-LESS marker would
land in the leading table-less band (E-107/AM corrected: merge_2's union markers are
all table-backed, so none land in the leading band — see §3.1). Below, renumbered
indices are shown as `op.?`; the render-order BAND STRUCTURE (which `key_of` block
each marker falls in) IS predicted.

### 3.1 merge_2 (PRIMARY kEagerUnion) — predicted opt dump

Grounding (`ds-base/merge_2/opt/df`): merge.10 (`class=table-less`, callers
tuple.7/tuple.8/tuple.9 = 3) + merge.11 (`class=table-less`, callers
tuple.3/tuple.4 = 2), both non-inductive ⇒ **5 kEagerUnion markers, ALL
table-BACKED** — `ModelTableOrNull(merge.10)=%table:4` (×3 visits),
`ModelTableOrNull(merge.11)=%table:8` (×2 visits); the render `table=` reads
the ControlFlow equivalence-set DataModel, NOT the `.df` per-view `TableId`
(they diverge here — merge.10/11 are `.df class=table-less` but
model-table-backed). Confirmed by direct `ModelTableOrNull` probe. Census opt
total 15→**20 ops**. (E-107/AM: corrected at stage-(d) — ModelTableOrNull probe.)

```
deltarel

# --- LEADING table-less band (op_table_id sentinel 0) — ONLY the 5 existing
#   kEagerForward (VERBATIM below). NO kEagerUnion in this band: merge_2's
#   unions are model-table-BACKED, so they sort into the %table:4/%table:8
#   blocks below instead [STRUCT band placement, BYTE-text] ---
op.0  kEagerForward sign=· ctx=eager stratum=0
    args:
op.4  kEagerForward sign=· ctx=eager stratum=0
    args:
op.8  kEagerForward sign=· ctx=eager stratum=0
    args:
op.12 kEagerForward sign=· ctx=eager stratum=0
    args:
op.16 kEagerForward sign=· ctx=eager stratum=0
    args:

# --- %table:4 block — 3 kEagerUnion markers (merge.10, ×3 visits), each
#   preceding its fwd/ins pair by ctor [STRUCT placement, BYTE-text] ---
op.5  kEagerUnion sign=· ctx=eager stratum=0
    args: table=%table:4
op.6  kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:4
op.7  kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:4
op.13 kEagerUnion sign=· ctx=eager stratum=0
    args: table=%table:4
op.14 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:4
op.15 kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:4
op.17 kEagerUnion sign=· ctx=eager stratum=0
    args: table=%table:4
op.18 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:4
op.19 kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:4

# --- %table:8 block — 2 kEagerUnion markers (merge.11, ×2 visits), each
#   preceding its fwd/ins pair by ctor [STRUCT placement, BYTE-text] ---
op.1  kEagerUnion sign=· ctx=eager stratum=0
    args: table=%table:8
op.2  kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:8
op.3  kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:8
op.9  kEagerUnion sign=· ctx=eager stratum=0
    args: table=%table:8
op.10 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:8
op.11 kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:8

rounds:

deps:

census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=0 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0 kEagerForward=10 kEagerInsert=5 kEagerCompare=0 kEagerGenerate=0 kEagerUnion=5 kEagerSelect=0
```

NOTE: census `kEagerForward=10 kEagerInsert=5 …` is VERBATIM-observable at tip; the
tail ` kEagerUnion=5 kEagerSelect=0` is the predicted append (`kEagerUnion=5`
[STRUCT], caller-edge-cross-checked; `kEagerSelect=0` [BYTE] — zero selects
reached). The op.`<idx>` numbers and band placement shown are [STRUCT]
bless-pinned per lane 1's high-confidence reconstruction (d-adjudication.md §2).

### 3.2 booleans (PRIMARY kEagerSelect) — predicted opt dump

Grounding (`ds-base/booleans/opt/df`): opt has ZERO merges (fused into join.10/
join.11) ⇒ `kEagerUnion=0` **[BYTE]**. The ONE unit-condition select is `select.3
(c4:bool)` reading `relation pred_…/0` → tuple.9 (`table=%table:4`); its
pred is the INSERT into the condition relation `%table:4` (insert.12), so the
SELECT↔pred-INSERT union rule unifies its model → **kEagerSelect renders
`table=%table:4`** (design §D.3; %table:4 is observably present — the existing op.6
kEagerInsert / op.7 kEagerForward carry it). `kEagerSelect=1` (single walk path;
[STRUCT], design-traced). Census opt total 10→**11 ops**.

```
deltarel

# --- %table:4 block — the existing Ins/Fwd pair (VERBATIM [BYTE-text]) + the NEW
#     kEagerSelect marker (table=%table:4), interleaved by ctor [STRUCT placement] ---
op.? kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:4
op.? kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:4
op.? kEagerSelect sign=· ctx=eager stratum=0                 # NEW [BYTE-text]
    args: table=%table:4                                     # [STRUCT: table id bless-pinned per DS-ADJ-7]

# --- remaining blocks VERBATIM (structure UNCHANGED) [BYTE-text] ---
op.? kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:7
op.? kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:11
op.? kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:11
op.? kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:14
op.? kIngestFold sign=+ ctx=eager stratum=0
    effects: {kCounter(%table:14, +, NonRecursive)}
    spine: —
    args: table=%table:14 message=log_in/1
op.? kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:17
op.? kIngestFold sign=+ ctx=eager stratum=0
    effects: {kCounter(%table:17, +, NonRecursive)}
    spine: —
    args: table=%table:17 message=add_user/1
op.? kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:20

rounds:

deps:

census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=2 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0 kEagerForward=6 kEagerInsert=2 kEagerCompare=0 kEagerGenerate=0 kEagerUnion=0 kEagerSelect=1
```

NOTE: the ingest-fold ops KEEP their ctor 0/1 (ADJ-S2 tail-append preserves op.0/op.1
for the two `kIngestFold` — the select/forward markers enroll strictly after). The
`sink=relation` / `message=…/1` / `effects:{…}` / `spine: —` lines are VERBATIM.

### 3.3 elim-cond-cycle-simple (NEGATIVE GUARD) — predicted opt dump

Grounding (`ds-base/elim-cond-cycle-simple/opt/df`): the one merge (merge.11) is
INDUCTION-OWNED (`table=%table:12 class=monotone set=0 depth=1`) → routes to
`BuildEagerInductiveRegion`, mints **ZERO kEagerUnion** — the correctness witness for
the mint guard `IsMerge() && !InductionGroupId()`. `kEagerUnion=0` **[BYTE, the guard
witness]**. The unit-condition `select.3 (c4:bool, relation pred_…/0)` → tuple.9;
its condition INSERT is into `%table:5` (insert.12), so kEagerSelect renders
`table=%table:5`. `kEagerSelect=1` [STRUCT]. Census opt total 8→**9 ops**.

```
deltarel

# --- LEADING table-less band (2 existing kEagerForward, VERBATIM) [BYTE-text] ---
op.? kEagerForward sign=· ctx=eager stratum=0
    args:
op.? kEagerForward sign=· ctx=eager stratum=0
    args:

# --- %table:5 block — existing Ins/Fwd pair (VERBATIM) + NEW kEagerSelect
#     (table=%table:5), interleaved by ctor [STRUCT placement] ---
op.? kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:5
op.? kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:5
op.? kEagerSelect sign=· ctx=eager stratum=0                 # NEW [BYTE-text]
    args: table=%table:5                                     # [STRUCT: table id bless-pinned]

# --- remaining blocks VERBATIM [BYTE-text] ---
op.? kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:8
op.? kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:12
op.? kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:12
op.? kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:15

rounds:

deps:

census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=0 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0 kEagerForward=6 kEagerInsert=2 kEagerCompare=0 kEagerGenerate=0 kEagerUnion=0 kEagerSelect=1
```

---
## (4) DS-ADJ-1 MODE-STABILITY TABLE (regenerated 4 `.df`/`.deltarel` per carrier this session)

The census is fixed pre-`ProgramImpl::Optimize` (F-ORDER) and reads the `.df` view
inventory (upstream of controlflow-opt) ⇒ **opt==nocf and nodf==none, always** on the
eager census. Fwd/Ins/Ingest columns are VERBATIM-OBSERVED this session (`ds-base/<c>/
<mode>/deltarel`); U/S columns are the predicted union/select counts.

| carrier | mode | flags | Fwd | Ins | Ingest | **U** | **S** | **total** | note |
|---|---|---|---|---|---|---|---|---|---|
| merge_2 | opt  | (none) | 10 | 5 | 0 | **5** | 0 | **20** | U cross-checked 3+2=5, **all table-BACKED (3×%table:4, 2×%table:8)** — model-table, not `.df` class |
| merge_2 | nocf | -dcf   | 10 | 5 | 0 | **5** | 0 | **20** | == opt [BYTE] |
| merge_2 | nodf | -ddf   | 36 | 5 | 0 | **12**| 0 | **53** | U=12 = 3×t4 + 2×t8 + **7 table-less** (CSE-off intermediate unions); nodf not golden-pinned |
| merge_2 | none | both   | 36 | 5 | 0 | **12**| 0 | **53** | == nodf [BYTE] |
| booleans | opt  | (none) | 6 | 2 | 2 | 0 | **1** | **11** | U=0 [BYTE] (0 merges opt); S=1 select.3→%table:4 |
| booleans | nocf | -dcf   | 6 | 2 | 2 | 0 | **1** | **11** | == opt [BYTE] |
| booleans | nodf | -ddf   | 12| 2 | 1 | **2**| **1** | **18** | U=2 caller-cross-checked merge.18(1)+merge.19(1); S=1 |
| booleans | none | both   | 12| 2 | 1 | **2**| **1** | **18** | == nodf [BYTE] |
| elim-cond-cycle-simple | opt  | (none) | 6 | 2 | 0 | **0** | **1** | **9** | U=0 [BYTE, GUARD]: merge.11 INDUCTIVE(%table:12)→0; S=1→%table:5 |
| elim-cond-cycle-simple | nocf | -dcf   | 6 | 2 | 0 | 0 | 1 | 9 | == opt [BYTE] |
| elim-cond-cycle-simple | nodf | -ddf   | 17| 2 | 0 | **2**| **1** | **22** | U=2 caller-cross-checked: merge.22(1)+merge.24(1); merge.23 INDUCTIVE(%table:15)→0 |
| elim-cond-cycle-simple | none | both   | 17| 2 | 0 | 2 | 1 | 22 | == nodf [BYTE] |
| select_2 (OPTIONAL) | opt  | (none) | 4 | 2 | 0 | **2** | 0 | **8** | U cross-checked merge.5(2 callers)=2, **both table-BACKED %table:4** (model-table) |
| select_2 (OPTIONAL) | nodf | -ddf   | 12| 2 | 0 | **4**| 0 | **18** | U=4 = 2×t4 + 2×table-less |

**Controlflow-axis stability [BYTE]:** opt==nocf and nodf==none on EVERY row — verified
directly for Fwd/Ins/Ingest this session, and inherited by U/S (census fixed
pre-controlflow-opt; `.df` upstream). The `.deltarel.opt.golden` sidecar pins
`deltarel opt` ONLY; the suite emits no other mode's `.deltarel`, so the golden is
df-axis-untouched.

**Dataflow-axis growth (EXPECTED, not knob coupling) [STRUCT-explanatory]:**
`-disable-dataflow-opt` skips CSE, leaving the Query graph un-fused, so the eager walk
descends a LARGER view set — merge_2 U 5→12, Fwd 10→36; booleans U 0→2 (2 merges
un-fused from the opt JOINs); elim U 0→2. A df-axis delta is EXPECTED per DS-ADJ-1,
never accidental coupling. The nodf/none union counts are MANUAL referee reads
(`-deltarel-out` by hand; the sidecar drives only opt); the distinct-caller-edge sum
is a stated FLOOR (merge_2 nodf: 8; select_2 nodf: 3) below the per-visit design count
because walk re-descent multiplicity inflates them — bless-pinned.

---

## (5) HEXDUMP PINS (DS-ADJ-4 — the first-time byte pins)

Captured this session from live marker lines:

- **`sign=·` glyph** = the multibyte signless glyph `\xC2\xB7` (`SignGlyph(0)`,
  `table_op_sign==0`). On EVERY new kEagerUnion/kEagerSelect header line, the bytes
  after `sign=` are **`c2 b7`**, then a space (`20`). Verified on an existing marker
  header (`op.7 kEagerForward sign=·…`):
  ```
  … 72 64 20 73 69 67 6e 3d  c2 b7 20 63 74 78 3d 65   |rd sign=.. ctx=e|
  ```
  R3's `op.? kEagerUnion sign=·…` / `op.? kEagerSelect sign=·…` lines carry the
  IDENTICAL `73 69 67 6e 3d c2 b7 20` (`sign=` `\xC2\xB7` ` `) sequence.
- **booleans kEagerSelect `args: table=%table:4` line** — predicted bytes (VERBATIM
  from the existing `table=%table:4` line in booleans opt; the select marker's args
  line is byte-identical):
  ```
  20 20 20 20 61 72 67 73  3a 20 74 61 62 6c 65 3d   |    args: table=|
  25 74 61 62 6c 65 3a 34  0a                        |%table:4.|
  ```
- **elim kEagerSelect `args: table=%table:5` line** — predicted bytes (VERBATIM from
  the existing `table=%table:5` line):
  ```
  20 20 20 20 61 72 67 73  3a 20 74 61 62 6c 65 3d   |    args: table=|
  25 74 61 62 6c 65 3a 35  0a                        |%table:5.|
  ```
- **merge_2 kEagerUnion (table-BACKED) args line** — `    args: table=%table:4\n`
  = `20 20 20 20 61 72 67 73 3a 20 74 61 62 6c 65 3d 25 74 61 62 6c 65 3a 34 0a`
  (and the `%table:8` variant, final byte `38`). The bare-`args:` union line is
  NOT witnessed at opt (nodf/none only, unpinned).

---

## (6) DS-ADJ-4 FIRST-PIN LIST — what R3 pins for the first time

R3's new goldens PIN these tokens/bytes for the FIRST time in corpus history:
1. **`kEagerUnion` header token** — the `DROpKindName(kEagerUnion)="kEagerUnion"`
   string (Format.cpp site 9) and the census token `kEagerUnion=<n>` (kAllKinds slot
   22). First pinned by merge_2 (table-BACKED arm) + the three census-only carriers'
   ` kEagerUnion=0`.
2. **`kEagerSelect` header token** — `DROpKindName(kEagerSelect)="kEagerSelect"` and
   census token `kEagerSelect=<n>`. First pinned by booleans/elim (table-backed arm)
   + ` kEagerSelect=0`.
3. **The kEagerSelect `table=%table:N` arm** — a SELECT marker carrying a merged-model
   table (SELECT↔pred-INSERT union). First witnessed by booleans (`%table:4`) / elim
   (`%table:5`). This is the ONLY table-BACKED new-marker arm R3 pins.
4. **The kEagerUnion table-BACKED arm** (`table=%table:N`) — first witnessed by
   merge_2's 5 markers (%table:4 ×3, %table:8 ×2) and select_2 (%table:4 ×2). This
   is the ONLY kEagerUnion arm R3 witnesses at opt.
5. **The 24-kind census line** (`kAllKinds` → 24; census banner comment → 24, abort
   comment → "25th" per A3).
6. **NOT pinned (labeled residual):** the table-LESS **kEagerUnion** arm (bare
   `args:`) stays opt-UNWITNESSED — it appears only in nodf/none (merge_2 7×, elim
   1×, select_2 2×), which are NOT golden-pinned. (E-106 is CORRECTED: acyclic
   merges are `.df class=table-less` but ControlFlow-model-table-BACKED; the render
   uses the model table, so the witnessed union arm is table-backed.) Also the OPEN
   `cond=`/`rel=` select token is DECLINED (§F.4 — select renders nothing extra), so
   no such token is pinned.

---

## (7) PRE-BLESS DIVERGENCE SET + git-status POSTCONDITION

**PRE-bless — EXACTLY six reds (literal `run_irgold` format, `irgold` keyword +
DOTTED `surface.mode`):**
```
merge_2                irgold deltarel.opt IRGOLD-MISSING     (new golden absent first run)
booleans               irgold deltarel.opt IRGOLD-MISSING     (idem)
elim-cond-cycle-simple irgold deltarel.opt IRGOLD-MISSING     (idem; if adopted)
demand_tc_witness      irgold deltarel.opt IRGOLD-DIVERGE     (census +2 cols)
symrec_tie_1           irgold deltarel.opt IRGOLD-DIVERGE     (census +2 cols)
map_3                  irgold deltarel.opt IRGOLD-DIVERGE     (census +2 cols)
```
NOTE the R2→R3 shift: R2 had 1 MISSING (map_3) + 2 DIVERGE (tc/symrec). R3 has 3
MISSING (new carriers) + **3 DIVERGE** (tc/symrec/map_3 — all three now hold a
`.deltarel` golden). Everything else GREEN across all three PRE-bless runs (×3 — a new
ordering-sensitive Kahn input on every compile: the two new `flow.ops` members).

**POST-bless — desired verdict [BYTE]:** `SUITE: PASS (173 cases)` ×3 (the carriers are
EXISTING `.dr` cases; `.irgold`/`.deltarel` sidecars add ZERO caselist entries —
runall.sh counts `cases/*.dr` = 173; the R2 precedent stayed PASS(173) after map_3).

**git-status desired state [BYTE] — EXACTLY these paths changed (merge_2+booleans+elim
carrier set):**
```
tests/OptDiff/goldens/demand_tc_witness.deltarel.opt.golden      (census line +2 cols)
tests/OptDiff/goldens/symrec_tie_1.deltarel.opt.golden           (census line +2 cols)
tests/OptDiff/goldens/map_3.deltarel.opt.golden                  (census line +2 cols)
tests/OptDiff/goldens/merge_2.deltarel.opt.golden                (NEW file)
tests/OptDiff/goldens/booleans.deltarel.opt.golden               (NEW file)
tests/OptDiff/goldens/elim-cond-cycle-simple.deltarel.opt.golden (NEW file)
tests/OptDiff/cases/merge_2.irgold                               (NEW sidecar, "deltarel opt")
tests/OptDiff/cases/booleans.irgold                              (NEW sidecar, "deltarel opt")
tests/OptDiff/cases/elim-cond-cycle-simple.irgold                (NEW sidecar, "deltarel opt")
```
= 3 re-blessed census-only goldens + 3 new goldens + 3 new sidecars (plus the source
files: DeltaRel.{h,cpp}, Format.cpp, Build.cpp, Union.cpp only if the extract touches
it — but Union.cpp stays byte-unchanged). **If ANY `.h`/`.ir`/`.df` golden or any
`.stdout` appears in `git status`, R3's zero-emission-change claim is ALREADY violated
→ STOP.** (If select_2 is also adopted, add its golden + sidecar — owner call §F.7.)

---

## (8) CONFIG-INVARIANCE + debug==release EXPECTATIONS

- **config-invariance (3-run)** [BYTE-expectation]: the `.deltarel` opt dump for ALL
  carriers (tc, symrec, map_3, merge_2, booleans, elim[, select_2]) is byte-identical
  across three independent `-deltarel-out` renders (the census + op stream is a pure
  function of the Query/CF build, no map-iteration/pointer order — `key_of` sorts by
  `table->id`/ctor, the (F) law). Expected: `cmp -s` clean ×3.
- **debug==release** [BYTE-expectation]: render each carrier's `.deltarel` under both
  presets; expected byte-identical single hash. R3 adds no NDEBUG-gated field/branch
  (the always-on validators V-PRED-XCHECK / A.6(c) survive NDEBUG; the marker ctors
  take the same `QueryView`/`TABLE*` handles debug and release).
- **ASAN both surfaces ×2** [expectation]: zero reports, SUITE PASS both. R3 adds no
  new field/lifetime (M2' — no payload).
- **E-62 re-grep clean** [expectation]: no NEW `pinned_order`/`body_ops`/`output_ops`
  reader; `key_of`/EAGER_WEB/A.6(c) are EDITED (wider branches), none a NEW reader.

---

## (9) DS-R3 NUMBERED PINS — the binding output-state contracts

- **DS-R3-1** [BYTE]. Zero emission change corpus-wide: `.df`/`.ir`/`.h`/`.cpp`/
  `stdout` byte-identical, all 173 cases × 4 modes, vs frozen debug c0a8a819 /
  release 958ddf8b. The committed non-deltarel IR goldens (symrec `{ir,df}`, tc
  `{ir,h,df}`) + `map_3.stdout` + all new-carrier `.stdout` stay byte-unchanged. The
  676-row knob-off A/B (×2), post-baseline-4 (incl. nested eqgate ×4), and `data/` A/B
  all 0-diverged. ANY `.h`/`.ir`/`.df`/`.stdout` movement = STOP.
- **DS-R3-2** [BYTE]. The THREE existing carriers (tc, symrec, map_3) get
  census-line-ONLY churn: the SOLE changed line is the tail append
  ` kEagerUnion=0 kEagerSelect=0` (enum/kAllKinds tail order …Compare,Generate,Union,
  Select). Every op block / `rounds:` / `deps:` byte-identical. Full desired census
  lines pinned in §(2). Grounded: all three have only inductive (table-backed
  monotone) merges + zero unit-cond selects (`.df` this session).
- **DS-R3-3** [STRUCT-count, BYTE-text]. New-carrier opt census counts:
  merge_2 = 5 kEagerUnion / 0 kEagerSelect (total 15→20); booleans = 0 U / 1 S (total
  10→11); elim = 0 U / 1 S (total 8→9). Fwd/Ins/Ingest are VERBATIM-observed; U/S are
  predicted (merge_2 U caller-edge-cross-checked 3+2=5; booleans/elim U=0 induction/
  no-merge-observed [BYTE]; S=1 design-traced [STRUCT]).
- **DS-R3-4** [BYTE-text]. New-marker per-op TEXT is fully determined: header
  `op.<idx> <Kind> sign=· ctx=eager stratum=0` (`sign=·`=`\xC2\xB7`, `ctx=eager`,
  `stratum=0` default arm); kEagerUnion → `    args: table=%table:N` (merge_2
  %table:4 / %table:8 — the merged-model table, table-BACKED at opt; a table-LESS
  union renders bare `    args:` only off-opt); kEagerSelect → `    args:
  table=%table:N` (booleans %table:4 / elim %table:5). NO operator/functor/
  positivity/cond/rel token (M2' + §F.4 — select renders nothing extra).
- **DS-R3-5** [STRUCT]. New-carrier render ORDER + op.`<idx>` numbers: enrollment
  tail-appends the new markers into the walk/DFS stream ⇒ every ctor index RENUMBERS;
  the merge_2 union markers sort into their **%table:4 / %table:8 blocks** by ctor
  (each union precedes its fwd/ins pair); the leading table-less band holds ONLY the
  5 pre-merge forwards. The booleans/elim select markers sort into their `%table:N`
  block by ctor. The BAND STRUCTURE (which key_of block each marker falls in) is
  predicted (§3); the intra-band interleave is bless-pinned (A7, DS-ADJ-7). ADJ-S2
  preserves ingest-fold ops 0/1 (booleans keeps op.0/op.1 = kIngestFold).
- **DS-R3-6** [BYTE]. DS-ADJ-1 controlflow-axis stability: opt==nocf and nodf==none on
  EVERY carrier's eager census (verified this session for Fwd/Ins/Ingest; inherited by
  U/S). The df-axis growth (merge_2 U 5→12, Fwd 10→36; booleans/elim U 0→2) is
  EXPECTED, not knob coupling; the opt golden is df-axis-untouched.
- **DS-R3-7** [BYTE]. Suite verdict: PRE-bless exactly six reds (3 new carriers
  MISSING; tc/symrec/map_3 DIVERGE); POST-bless `SUITE: PASS (173 cases)` ×3.
- **DS-R3-8** [BYTE]. git-status: EXACTLY 3 re-blessed census-only goldens + 3 new
  `.deltarel.opt.golden` + 3 new `.irgold` sidecars (`deltarel opt`) + the source diff
  (DeltaRel.{h,cpp}, Format.cpp, Build.cpp). NO `.h`/`.ir`/`.df`/`.stdout` golden moves
  (else STOP). (+select_2 golden+sidecar if adopted, §F.7.)
- **DS-R3-9** [BYTE/STRUCT]. First-pin (DS-ADJ-4 hexdump): R3's goldens PIN the
  `kEagerUnion` AND `kEagerSelect` `table=%table:N` arms for the FIRST time (merge_2
  unions %table:4/%table:8; booleans/elim selects %table:4/%table:5), plus the
  header + census tokens; the `sign=·` `\xC2\xB7` byte carries. The table-LESS
  **kEagerUnion** arm (bare `args:`) stays opt-UNWITNESSED (nodf/none only,
  unpinned); merge_2 witnesses the table-BACKED arm. E-106 CORRECTED: `.df`
  class-table-less ≠ model-table-null. The OPEN select `cond=`/`rel=` token is
  DECLINED — not pinned (labeled residual). elim's `kEagerUnion=0` is the
  INDUCTION-SKIP correctness witness (merge.11 InductionGroupId →
  BuildEagerInductiveRegion → zero union markers).
