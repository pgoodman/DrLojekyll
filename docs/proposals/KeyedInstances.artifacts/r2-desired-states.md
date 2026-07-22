======================================================================
ADJUDICATED R2 DESIRED-STATES CONTRACT (Rel epoch, SECOND step-kind
migration; committed with the R2 landing). Ritual stage (d), xhigh
author. Derived from `r2-design.md` (ADJ-R2-0..8 BINDING) and the R1
mold `r1-desired-states.md` (DS-ADJ-1..7 inherited), RE-VERIFIED
against LIVE tip dumps regenerated from build/debug/bin/drlojekyll at
tip 3fc9d79f atop R1 code 8fa156bc. Every line marked **[BYTE]**
(committed byte-exact) or **[STRUCT]** (shape-committed; value/order
bless-pinned — each justified).
CRITIQUE RECORD (stage-d, 2 lanes): (i) a BLIND independent
op-inventory re-derivation (author-doc-unread; walked the .df successor
edges under the dispatch rules) REPRODUCED every load-bearing number —
counts, ctor order, table-less determination, zero new markers on
tc/symrec; (ii) the E-71 grammar-conformance lane: PASS — both
productions derivable in the pinned t2b-grammar structure, exact
§2.0/§2.4/§2.5 amendments written, no existing production invalidated
(the census production quantifies over enum members, count-agnostic).
ORCHESTRATOR ADJUDICATIONS AT THE CODE: (1) DS-R2-4's Candidate A
UPGRADED to [BYTE]-derived going-in — personally verified
all_cols_match (Build.cpp:2313-2328: size + positional col-Id equality)
fails for all three tuples (widths 2/3/1 vs map widths 4/4/3) and
compare.9's successor is a MAP (no rule applies), so no union path
exists IN map_3; bless still confirms per DS-ADJ-7. (2) The blind
lane's BROAD claim ("no union rule ever admits a MAP/COMPARE") is
REFUTED in general: the TUPLE-perfect-pass rule (Build.cpp:2469-2479)
CAN union a full-width passthrough TUPLE with a MAP/COMPARE
predecessor, putting the marker in a table-bearing model — such a
program is corpus-UNWITNESSED (a future-carrier note; the A.6(c)
merged-model match enforces whichever holds, always-on). (3) The
cmp=/functor= token [BYTE] marks are CONTRACT-pinned (E-71 pre-code
adjudication binds the implementation), not code-derived at tip — the
d1 critic's [STRUCT] relabel is declined on that ground. (4) The
grammar lane's -Wswitch finding folded into r2-design.md §B5 (totality
= by-construction + loud-abort tail; -Wswitch is warning-only, no
-Werror in presets), plus the std::string(NameAsString()) wrap note.
======================================================================

# R2 desired-states — the eager-web CMP-filter + MAP-call marker ops in the .deltarel dump

R2 recap (design §0 / §A): model the monotone eager web's **CMP-filter** dispatch
arm (`Build.cpp:1201-1202`, `BuildEagerCompareRegions`) and **MAP functor-call**
arm (`Build.cpp:1191-1197`, `BuildEagerGenerateRegion`) as two new effect-free
DR-IR op kinds — `kEagerCompare` (20) / `kEagerGenerate` (21) — minted at the walk
(`RecordEagerDispatch`), enrolled by the EAGER_WEB block via a 4-way re-invocation
STRICTLY AFTER the ingest folds (ADJ-S2), lowered IN PLACE by
`LowerRelStep_Compare`/`LowerRelStep_Generate` wrappers calling the UNTOUCHED
region builders. NO new `DROp`/`EmittedEagerOp` field: `cmp=`/`functor=` re-derive
from the stored `eager_view` at Format time. **Zero emission change.** The ONLY
bytes that move anywhere in the corpus are the `.deltarel` dumps: census-line-only
on the two existing carriers, plus the new `map_3.deltarel.opt.golden` (the sole
carrier with non-zero CMP/MAP op blocks).

---

## THE CENTRAL HONEST FACT — R2 is LESS [STRUCT] than R1

R1's census `<F>`/`<I>` were irreducibly [STRUCT] because inductive-MERGE /
cycle-JOIN re-descent made the raw-walk dispatch count exceed the distinct-view
floor (proven live: `insert.19` folded twice). **R2's sole new carrier, `map_3`,
is fully acyclic + monotone + single-predecessor** (`.df` regenerated below:
every view `class=table-less`/`monotone`, no MERGE, no JOIN, no cycle). So the
walk visits each CMP/MAP/TUPLE/INSERT view EXACTLY ONCE — the per-visit marker
count EQUALS the distinct-view count, and I have PROVEN this coincidence against
the live tip dump: the CURRENT (PRE-R2) `map_3.deltarel` already shows
`kEagerForward=3 kEagerInsert=3` (opt), matching the `.df` view inventory
`tuples=3 inserts=3` exactly (§4). The two NEW counts derive identically from
`maps=3 compares=1` (opt). Hence map_3's census counts are **[BYTE]**
(derived-and-cross-checked), not [STRUCT].

The ONE genuine [STRUCT] residue is map_3's **render ORDER + which markers carry
`table=`** (DS-ADJ-7): the render authority is the union-find MERGED model, not
the `.df` per-view `table=` attribute, so a going-in `.df`-read ordering is
bless-superseded. §2.3 states BOTH candidate layouts and my code-grounded lean.
The two existing carriers (`tc`, `symrec`) have ZERO CMP/MAP views, so they get
census-line-only churn — every op block, `rounds:`, `deps:` byte-identical.

---

## (1) REAL BASELINE CAPTURE (tip 3fc9d79f/8fa156bc, opt mode, all sinks in one compile)

Regenerated LIVE into `.../scratchpad/r2/ds-base/` exactly as `run_irgold` compiles
(opt = no opt flags + `.drflags`; all sinks one compile — ADJ-S7):

```
DR=build/debug/bin/drlojekyll
$DR cases/symrec_tie_1.dr             -df-out .. -deltarel-out .. -ir-out .. -cpp-out ..
$DR cases/demand_tc_witness.dr -demand -df-out .. -deltarel-out .. -ir-out .. -cpp-out ..
$DR cases/map_3.dr                     -df-out .. -deltarel-out .. -ir-out .. -cpp-out ..
```

**Byte-verification against committed goldens (VERIFIED this session):**
- `ds-base/tc/deltarel` `cmp`-clean vs `goldens/demand_tc_witness.deltarel.opt.golden` → **TC MATCH** [BYTE]
- `ds-base/symrec/deltarel` `cmp`-clean vs `goldens/symrec_tie_1.deltarel.opt.golden` → **SYMREC MATCH** [BYTE]
- `goldens/map_3.deltarel.opt.golden` → **does not exist** (only `map_3.stdout` committed). RAT-8 first-ever seed.

### 1a. `map_3` — CURRENT tip `.deltarel` (VERBATIM, PRE-R2, 20 lines)

The pre-R2 dump shows ONLY the R1 forward/insert markers; the 3 MAP + 1 CMP views
are as-yet UNMODELED (the walk mints no marker for them). `kIngestFold=0` — map_3's
`m/2` receive is a **table-less monotone receive** taking the E-42 VECTORLOOP-shim
else-branch (`Procedure.cpp`), so no ingest-fold prefix; the eager markers start at
ctor 0.

```
deltarel

op.0 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:6
op.1 kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:6
op.2 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:10
op.3 kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:10
op.4 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:15
op.5 kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:15

rounds:

deps:

census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=0 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0 kEagerForward=3 kEagerInsert=3
```

### 1b. `map_3` opt `.df` view inventory (VERBATIM-derived; drives every R2 prediction)

Regenerated `ds-base/map3/df`. The eager walk begins at the `m/2` receive
`select.2` → successors `{map.6, map.7, compare.9}`:

| view | kind | `.df` class | succ | merged-model table (§2.3) |
|---|---|---|---|---|
| `select.2` | SELECT | table-less | => map.6, map.7, compare.9 | — (receive, table-less shim) |
| `map.6` | MAP `add_i32` | table-less | => tuple.3 | **null** (no union rule) |
| `map.7` | MAP `add_i32` | table-less | => tuple.4 | **null** |
| `compare.9` | CMP `eq` | table-less | => map.8 | **null** |
| `map.8` | MAP `add_i32` | table-less | => tuple.5 | **null** |
| `tuple.3` | TUPLE | table-less | => insert.10 | `%table:6` (via insert union) |
| `tuple.4` | TUPLE | table-less | => insert.11 | `%table:10` |
| `tuple.5` | TUPLE | table-less | => insert.12 | `%table:15` |
| `insert.10` | INSERT | monotone | into `%table:6` | `%table:6` |
| `insert.11` | INSERT | monotone | into `%table:10` | `%table:10` |
| `insert.12` | INSERT | monotone | into `%table:15` | `%table:15` |

MAP annotations `; add_i32` on `.df:27/31/35`; CMP annotation `; eq` on `.df:39`.
Functor decl `#functor add_i32(bound i32 L, bound i32 R, free i32 Sum)` (`map_3.dr:8`)
→ NAME `add_i32`, ARITY 3.

---

## (2) DESIRED `.deltarel` STATES POST-R2, per carrier

### 2.1 `demand_tc_witness.deltarel.opt.golden` — census-line-ONLY diff

Zero CMP/MAP views (`.df` golden: `maps=0 compares=0`) ⇒ NO new op blocks ⇒ every
op block, `rounds:`, `deps:` **byte-unchanged [BYTE]**. Ops `op.0`..`op.15` keep
IDENTICAL headers/args/labels (walk records the same dispatches in the same order;
EAGER_WEB enrollment count/order unchanged — ADJ-R2-4). The SOLE changed line is
the census tail append ` kEagerCompare=0 kEagerGenerate=0` (enum/kAllKinds tail
order: Forward, Insert, **Compare, Generate** — verified `Format.cpp:955` +
`DeltaRel.h` enum). **Full desired census line (the one changed line) [BYTE]:**

```
census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=2 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0 kEagerForward=12 kEagerInsert=2 kEagerCompare=0 kEagerGenerate=0
```

(Old tail was `... kEagerForward=12 kEagerInsert=2`, verified against the committed
golden this session.) These are the ONLY changed bytes in the file.

### 2.2 `symrec_tie_1.deltarel.opt.golden` — census-line-ONLY diff

Zero CMP/MAP views (`.df` golden: `maps=0 compares=0`) ⇒ census-line-only churn.
Ops `op.0`..`op.8` byte-unchanged [BYTE]; `rounds:`/`deps:` empty [BYTE]. **Full
desired census line (the one changed line) [BYTE]:**

```
census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=1 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0 kEagerForward=7 kEagerInsert=1 kEagerCompare=0 kEagerGenerate=0
```

(Old tail `... kEagerForward=7 kEagerInsert=1`, verified.) NOTE (per contract C.5):
unlike R1 where symrec was IRGOLD-MISSING, its `.deltarel` golden now EXISTS, so
R2's pre-bless red is a DIVERGE, not a MISSING.

### 2.3 `map_3.deltarel.opt.golden` (NEW) — the desired dump

**Census counts (opt), derived from §1b and cross-checked against the live PRE-R2
Fwd/Ins counts [BYTE]:** `kEagerGenerate=3` (maps=3), `kEagerCompare=1`
(compares=1), `kEagerForward=3` (tuples=3, unchanged from PRE-R2 live),
`kEagerInsert=3` (inserts=3, unchanged), `kIngestFold=0`, all other kinds 0. **Opt
total = 10 ops.** Full desired census line **[BYTE]**:

```
census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=0 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0 kEagerForward=3 kEagerInsert=3 kEagerCompare=1 kEagerGenerate=3
```

**Per-op header/token TEXT — fully determined [BYTE]:**
- Header (all 10): `op.<oi> <Kind> sign=· ctx=eager stratum=0` — `sign=·` is the
  multibyte signless glyph `\xC2\xB7` (`SignGlyph(0)`, `table_op_sign==0`); `ctx=eager`
  (`op.ctx==kEager`); `stratum=0` (`DROpStratum` default arm).
- `kEagerGenerate` (×3): args line ` functor=add_i32/3` — from
  `QueryMap::From(eager_view).Functor().NameAsString()` + `Arity()` (the 3-param
  DECL arity, not `QueryMap::Arity()`); NO positivity token (ADJ-R2-2); no `table=`
  (§2.3-lean). [BYTE]
- `kEagerCompare` (×1): header token `cmp=eq` — `compare.9` is the `Z=5` equality,
  VIEW operator `kEqual` → `ComparisonOperatorName(kEqual)="eq"` (`.df:39` annotates
  `; eq`); bare `args:` line (no `table=`). [BYTE]
- `kEagerForward` (×3): args line ` table=%table:6` / ` table=%table:10` /
  ` table=%table:15` (the three relation tables, via insert-union merged model —
  UNCHANGED from PRE-R2 live). [BYTE]
- `kEagerInsert` (×3): header token `sink=relation` (map_3 publishes zero messages —
  the three `#query` outputs are relation drains, not stream publishes; PRE-R2 live
  confirms `sink=relation` on all three); args ` table=%table:6/10/15`. [BYTE]

**The op ORDER + which markers carry `table=` — [STRUCT], DS-ADJ-7 pinned at bless.**
`pinned_order` sorts lead-0 eager ops by `(op_table_id, sign=0, ctor=oi)` where
`op_table_id` = null→sentinel 0, real→`id+1` (`key_of`, extended per ADJ-R2-4/M5).
`ctor` = walk-DFS enrollment index. The DFS from `select.2` mints in this order
(verified: matches the PRE-R2 forward/insert ctor stream, now interleaved with the
new Gen/Cmp mints):

| ctor | view | kind |
|---|---|---|
| 0 | map.6 | kEagerGenerate |
| 1 | tuple.3 | kEagerForward |
| 2 | insert.10 | kEagerInsert |
| 3 | map.7 | kEagerGenerate |
| 4 | tuple.4 | kEagerForward |
| 5 | insert.11 | kEagerInsert |
| 6 | compare.9 | kEagerCompare |
| 7 | map.8 | kEagerGenerate |
| 8 | tuple.5 | kEagerForward |
| 9 | insert.12 | kEagerInsert |

The render ORDER depends on whether the four Gen/Cmp markers are table-less. TWO
candidate layouts:

**CANDIDATE A — table-less-leading (my code-grounded lean, = contract C.3
going-in).** ALL four Gen/Cmp markers are table-less (`op_table_id` sentinel 0),
so they LEAD by ctor; the forward/insert pairs follow in table-id order. Evidence
(`BuildEquivalenceSets`, `Build.cpp:2405-2494`): the ONLY union rules are
INSERT↔pred-TUPLE (`:2416-2420`), SELECT↔pred-INSERT (`:2429-2436`), TUPLE↔pred on
perfect passthrough `all_cols_match` (`:2469-2479`), NEGATE↔succ-TUPLE
(`:2483-2492`). **MAP and COMPARE have NO union rule and are never a union source.**
The only way a map/compare model could acquire a table is a downstream TUPLE
perfectly passing it through — but `tuple.3/4/5` forward strict SUBSETS of their
map preds (`map.6`=(X,X,Y,Z) 4 cols → `tuple.3`=(Y,Z); etc.), so `all_cols_match`
FAILS and no union fires. Hence `ModelTableOrNull(map.6/7/8)=null`,
`ModelTableOrNull(compare.9)=null`. Desired dump (line-by-line) **[STRUCT-order,
BYTE-text]**:

```
deltarel

op.0 kEagerGenerate sign=· ctx=eager stratum=0
    args: functor=add_i32/3
op.3 kEagerGenerate sign=· ctx=eager stratum=0
    args: functor=add_i32/3
op.6 kEagerCompare sign=· ctx=eager stratum=0 cmp=eq
    args:
op.7 kEagerGenerate sign=· ctx=eager stratum=0
    args: functor=add_i32/3
op.1 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:6
op.2 kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:6
op.4 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:10
op.5 kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:10
op.8 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:15
op.9 kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:15

rounds:

deps:

census: <the 22-column opt line from §2.3 above>
```

Under A: the four table-less Gen/Cmp lead in ctor order (0,3,6,7); the three
forward/insert pairs render in table-id order (6→10→15), each pair `Fwd` before
`Ins` (ctor 1<2, 4<5, 8<9). This is the going-in golden.

**CANDIDATE B — merged-model-interleaved (the DS-ADJ-7 contingency).** IF (contrary
to the union analysis) any of `map.6/7/8`/`compare.9` unifies its DataModel with a
downstream table via some path I have not enumerated, that marker renders a
`table=%table:N` token and re-sorts into that table's block by ctor. Concretely: if
`map.6` unified with `%table:6`, then `op.0 kEagerGenerate` would render
` table=%table:6` and sort INTO the `%table:6` block AHEAD of `tuple.3` (ctor 0 <
1), giving `%table:6` block = Gen(0)/Fwd(1)/Ins(2); the leading table-less block
would shrink. The census COUNTS (3G/1C/3F/3I) are ROBUST either way (they don't
read `table=`); only the ORDER and the four `table=`/`cmp=`/`functor=` line
placements move. Per DS-ADJ-7 the as-blessed golden is authoritative and the A.6(c)
merged-model match validator (`op.table_op_table == ModelTableOrNull`, extended
4-way per ADJ-R2-7) enforces whichever holds corpus-wide, always-on.

**My assessment:** Candidate A is near-certain (the union code has no rule that can
give a MAP/COMPARE a table, and the subset-forward tuples defeat the only inbound
passthrough path). Candidate B is the formal bless contingency the contract's C.3
caveat / §E open-question-2 flags at <90%. Bless resolves it; the census counts and
per-op text are unaffected.

---

## (3) BYTE-UNCHANGED CLAIMS [BYTE] — every other surface

**Prediction (contract C.1): `.df`, `.ir`, `.h`, `.cpp`, `stdout` byte-identical
corpus-wide, all 173 cases, all 4 optimization modes, vs frozen a4b807dc.**
Mechanism (mold M3, mechanical): the mint + `RecordEagerDispatch` + EAGER_WEB
inventory path allocates ZERO `impl->next_id` and Emplaces NO region; the wrappers
CALL the untouched `BuildEagerCompareRegions`/`BuildEagerGenerateRegion` with
identical args at the identical walk moment, so the ControlFlow IR is byte-identical,
`.h`/`.cpp` are pure functions of that IR, `.df` is upstream of the whole CF build.
`Compare.cpp`, `Generate.cpp`, `InTryInsert`, `CreateCompareRegion`,
`CreateGeneratorCall` stay byte-identical. [BYTE]

**The 5 committed non-`.deltarel` IR goldens stay byte-identical [BYTE]** (verified
present this session, `Jul 21 22:38`):
1. `symrec_tie_1.ir.opt.golden`
2. `symrec_tie_1.df.opt.golden`
3. `demand_tc_witness.ir.opt.golden`
4. `demand_tc_witness.h.opt.golden`
5. `demand_tc_witness.df.opt.golden`

If ANY of these five moves at bless, R2's zero-emission-change claim is ALREADY
violated — STOP (§5 post-condition tripwire). Additionally `map_3.stdout`
(committed, driver output) stays byte-unchanged [BYTE] — its driver output is
emission-derived, and R2 moves no emission. The 676-row knob-off A/B, the
post-baseline-4 set (incl. `demand_neighborhood_witness` nested eqgate ×4 modes),
and the `data/` A/B all stay 0-diverged [BYTE].

---

## (4) DS-ADJ-1 MODE TABLE for `map_3` (regenerated four .df dumps; counts RE-DERIVED)

The census is fixed pre-`ProgramImpl::Optimize` (F-ORDER) and reads the `.df` view
inventory (upstream of controlflow-opt). REGENERATED all four `.df` this session
(`ds-base/modes/df.{opt,nocf,nodf,none}`) and re-derived from the view kinds:

| mode | flags | maps | compares | tuples | inserts | ⇒ Gen | Cmp | Fwd | Ins | **total** |
|---|---|---|---|---|---|---|---|---|---|---|
| opt  | (none)                | 3 | 1 | 3 | 3 | 3 | 1 | 3 | 3 | **10** |
| nocf | `-disable-controlflow-opt` | 3 | 1 | 3 | 3 | 3 | 1 | 3 | 3 | **10** |
| nodf | `-disable-dataflow-opt`    | 3 | 3 | 9 | 3 | 3 | 3 | 9 | 3 | **18** |
| none | both                       | 3 | 3 | 9 | 3 | 3 | 3 | 9 | 3 | **18** |

Each `Gen/Cmp/Fwd/Ins` = the corresponding `maps/compares/tuples/inserts` view
count, because map_3 is acyclic + single-predecessor (per-visit marker count ==
distinct-view count). **Cross-check [BYTE]:** the CURRENT PRE-R2 `map_3.deltarel`
census, regenerated in all four modes this session, already shows exactly
`kEagerForward=3 kEagerInsert=3` (opt/nocf) and `kEagerForward=9 kEagerInsert=3`
(nodf/none) — matching the `tuples`/`inserts` columns. The two NEW columns
(`kEagerGenerate`, `kEagerCompare`) derive identically from `maps`/`compares`.

**Controlflow-axis stability [BYTE]:** `opt == nocf` and `nodf == none` on the
eager census, always (census fixed pre-controlflow-opt; `.df` upstream of it). The
opt-mode `.deltarel.opt.golden` is UNTHREATENED by the df axis — the sidecar pins
`deltarel opt` only, and the suite emits no other mode's `.deltarel`.

**Dataflow-axis growth (EXPECTED, not coupling) [STRUCT-explanatory]:**
`-disable-dataflow-opt` skips `QueryImpl::Optimize` (CSE), leaving the Query graph
un-merged, so the syntax-directed eager walk descends a LARGER view set — compares
grow **1→3** and forwards **3→9** (maps stay 3, inserts stay 3). A df-axis delta
(opt≠nodf) is EXPECTED per DS-ADJ-1, never accidental knob coupling. The three
nocf/nodf/none census reads are MANUAL referee compiles (`-deltarel-out` by hand —
the sidecar drives only the opt compile per the runall.sh mode gate; [CRIT-FOLD
c3-LOW-3]); all four pre-run clean this session.

---

## (5) THE SUITE-VERDICT DESIRED STATE

**PRE-bless — EXACTLY three reds (literal `run_irgold` format, `irgold` keyword +
DOTTED `surface.mode`; a referee greps these exact lines):**

```
demand_tc_witness irgold deltarel.opt IRGOLD-DIVERGE
symrec_tie_1 irgold deltarel.opt IRGOLD-DIVERGE
map_3 irgold deltarel.opt IRGOLD-MISSING
```

- `demand_tc_witness` / `symrec_tie_1`: **DIVERGE** (census line grows by 2 columns;
  golden EXISTS). NOTE the R1→R2 shift: symrec was IRGOLD-MISSING at R1, is DIVERGE
  at R2 (its `.deltarel` golden now exists).
- `map_3`: **MISSING** (new golden absent first run; its `.irgold` sidecar seeds
  `deltarel opt`).
Everything else GREEN across all three PRE-bless runs (×3, a new ordering-sensitive
Kahn input on every compile — the two new `flow.ops` members).

**POST-bless — desired verdict [BYTE]:** `SUITE: PASS (173)` ×3, all three
`.deltarel` goldens green.

**git-status desired state [BYTE] — EXACTLY 4 paths changed:**
```
tests/OptDiff/goldens/demand_tc_witness.deltarel.opt.golden   (census line +2 cols)
tests/OptDiff/goldens/symrec_tie_1.deltarel.opt.golden        (census line +2 cols)
tests/OptDiff/goldens/map_3.deltarel.opt.golden               (NEW file)
tests/OptDiff/cases/map_3.irgold                              (NEW sidecar, "deltarel opt")
```
= 3 goldens (2 re-blessed + 1 new) + 1 new sidecar. If any `.h`/`.ir`/`.df` golden
appears in `git status`, R2's zero-emission-change claim is ALREADY violated — STOP.
(The `map_3.irgold` line-count = 1: `deltarel opt`; owner MAY add `h opt` to also
lock codegen, ADJ-R2-6 — an owner call at the ritual head, not required.)

---

## (6) DS-ADJ NUMBERED PINS — the binding output-state contracts

- **DS-R2-1** [BYTE]. `tc` + `symrec` `.deltarel.opt.golden`: the SOLE changed line
  is the census tail append ` kEagerCompare=0 kEagerGenerate=0` (enum/kAllKinds
  tail order Forward,Insert,Compare,Generate). Every op block, `rounds:`, `deps:`
  byte-identical. Full desired census lines pinned in §2.1/§2.2.
- **DS-R2-2** [BYTE]. `map_3` opt census counts = 3 kEagerGenerate / 1 kEagerCompare
  / 3 kEagerForward / 3 kEagerInsert / 0 kIngestFold, **total 10 ops** — derived
  from the `.df` view inventory (maps=3,compares=1,tuples=3,inserts=3) AND
  cross-checked against the live PRE-R2 Fwd=3/Ins=3.
- **DS-R2-3** [BYTE]. `map_3` per-op TEXT is fully determined: `sign=·`
  (`\xC2\xB7`), `ctx=eager`, `stratum=0`; `cmp=eq` (compare.9 VIEW operator kEqual);
  `functor=add_i32/3` (decl name + 3-param decl arity, NOT QueryMap::Arity);
  `sink=relation` on every kEagerInsert (zero publishes); `table=%table:6/10/15` on
  the three forward/insert pairs.
- **DS-R2-4** [BYTE-derived going-in; bless confirms per DS-ADJ-7 — UPGRADED at
  the stage-(d) adjudication]. `map_3` render ORDER = Candidate A (all four
  Gen/Cmp markers table-less → they LEAD, ctor 0/3/6/7, then the table:6/10/15
  pairs). Derivation (orchestrator-verified at the code): the ONLY inbound union
  path for a MAP/COMPARE model is the TUPLE-perfect-pass rule
  (`Build.cpp:2469-2479`), and `all_cols_match` (`:2313-2328`, size + positional
  col-Id equality) FAILS for tuple.3/4/5 (widths 2/3/1 vs map widths 4/4/3);
  compare.9's successor is a MAP (no rule); monotone ⇒ no own table. Candidate B
  (merged-model interleave) is retained as the FORMAL contingency only — and
  NOTE, loud: a full-width passthrough TUPLE after a MAP/COMPARE *would* union
  (the blind lane's "never" is over-broad); that shape is corpus-UNWITNESSED.
  The A.6(c) 4-way merged-model match enforces whichever holds, always-on.
- **DS-R2-5** [BYTE]. Zero emission change corpus-wide: `.df`/`.ir`/`.h`/`.cpp`/
  `stdout` byte-identical, all 173 cases × 4 modes, vs frozen a4b807dc. The 5
  committed non-deltarel IR goldens (symrec `{ir,df}`, tc `{ir,h,df}`) and
  `map_3.stdout` stay byte-unchanged; any movement = STOP.
- **DS-R2-6** [BYTE]. DS-ADJ-1 controlflow-axis stability: `map_3` eager census
  `opt==nocf` (10) and `nodf==none` (18). The df-axis growth (Cmp 1→3, Fwd 3→9) is
  EXPECTED, not knob coupling; the opt golden is df-axis-untouched.
- **DS-R2-7** [BYTE]. Suite verdict: PRE-bless exactly three reds
  (`demand_tc_witness`/`symrec_tie_1` DIVERGE, `map_3` MISSING); POST-bless
  `SUITE: PASS (173)` ×3.
- **DS-R2-8** [BYTE]. git-status: EXACTLY 3 goldens (2 re-blessed + 1 new
  `map_3.deltarel.opt.golden`) + 1 new sidecar (`map_3.irgold` = `deltarel opt`)
  changed. Nothing else.
- **DS-R2-9** [BYTE]. R2's two goldens (map_3 first) PIN the `cmp=eq` and
  `functor=add_i32/3` bytes for the first time, and (with tc/symrec) the `sign=·`
  `\xC2\xB7` byte — DS-ADJ-4 hexdump at bless. `neq`/`lt`/`gt` and any positivity
  spelling remain corpus-UNWITNESSED at R2 (ADJ-S5-analog residual; only `eq` is
  pinned).
