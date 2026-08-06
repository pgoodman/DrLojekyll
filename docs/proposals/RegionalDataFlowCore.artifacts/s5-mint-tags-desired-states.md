# S5-PRIME desired states — predicted `.df.opt` golden byte-diffs (slice 1)

**Stage (d), 2026-08-05. Tip edde6c82.** Predicted post-implementation bytes
for the goldened DataFlow surface, computed from FRESH `-df-out` dumps
(verified byte-identical to the six committed goldens today) + the
authoritative tag table (`s5p-tags.md`) + the emitter/sweep analysis
(`s5p-diff-draft.md`). This is the predict-then-verify target: after the 253
`Create`→`Mint` rewrites land, each real dump must equal these predicted
bytes before any bless.

**Inputs consumed:** `s5p-diff-draft.md` (token shape = trailing `tag=` on the
ATTRIBUTES line, no padding interaction; A5), `s5p-tags.md` (103 distinct
tags over 253 sites). **Dependence on inputs is flagged inline** where a
panel adjustment to either would move a predicted byte.

---

## 0. SANITY — the six current dumps == the six goldens (today, pre-change)

```
aggregate_1        IDENTICAL   (6 ATTRIBUTES lines)
barrier_neck_1     IDENTICAL   (26)
demand_tc_witness  IDENTICAL   (20)
negate_1           IDENTICAL   (7)
symrec_tie_1       IDENTICAL   (12)
key_tc_witness     IDENTICAL   (== demand_tc_witness.df byte-for-byte; symlink)
```
`cmp <fresh dump> tests/OptDiff/goldens/<case>.df.opt.golden` → no output for
all six. `key_tc_witness.df` and `demand_tc_witness.df` are byte-equal, which
is exactly why the golden is a symlink and bless_copy skips it.

Golden inventory (verified): exactly **6** `.df.opt.golden`, **no** other
`.df` mode exists (`.df.nodf/.nocf/.none` absent — the `.df` family is
opt-only). `key_tc_witness.df.opt.golden` is the **sole** symlink among them.

---

## 1. THE UNIVERSAL SHAPE RULE (certain)

The `.df` emitter (`lib/DataFlow/Format.cpp`) renders exactly ONE node block
per surviving VIEW; columns get no row of their own (R1: a column mint carries
its parent's tag but is never rendered). The tag is appended to the
**ATTRIBUTES** line only, as the **final** token, by `attrs_line` inserting
`" tag=" + mint_tag` immediately before `return r;` — AFTER the existing
`stratum=` / `set=…depth=…` tail (draft §2b).

Therefore, on every case:

* **Every ATTRIBUTES line — and only ATTRIBUTES lines — changes.** Header rows
  (`select ^select.N (…) ; comment`), `=>` edge lines, join pivot/out blocks,
  and `; cycle`/`; callers`/`; negates`/`; recv` comments are **byte-untouched**.
* Every view in all six goldens is minted through one of the 253 swept
  DataFlow `Create` sites (census: 0 CreateDerived, all view creation lives in
  these 17 files), so **no view is untagged** — the I5 "untagged renders
  nothing" guard never fires in this corpus. Changed-row count == ATTRIBUTES
  count exactly.

**Per-line transform (the only edit shape):**
```
-  ATTRIBUTES <existing tokens>
+  ATTRIBUTES <existing tokens> tag=<mint_tag>
```
Byte delta on a changed line = `len(" tag=") + len(mint_tag)` = `5 +
len(mint_tag)`. No other byte on the line moves.

**Predicted changed-row totals (byte-certain; tag STRINGS attributed below):**

| case | ATTRIBUTES lines = changed rows |
|---|---|
| aggregate_1 | 6 |
| barrier_neck_1 | 26 |
| demand_tc_witness | 20 |
| negate_1 | 7 |
| symrec_tie_1 | 12 |
| **5 real goldens total** | **71** |
| key_tc_witness | 0 (symlink; bless skip) |

---

## 2. PADDING / ALIGNMENT FINDING (certain)

`attrs_line` builds `r` by pure `+=` space-concatenation of `table=` /
`eqset=` / `class=` / `stratum=` / `set= depth=` — **no column-alignment, no
padding, no per-dump MAX**. The one alignment law in `Format.cpp` (pin p6,
`with_comment` padding header CONTENT to byte 51 before `; comment`) lives on
the HEADER row, which the tag does not touch. The Regional emitter's
E-K5-PAD per-dump-MAX member-key padding is a DIFFERENT file (`lib/Regional`)
and is out of slice 1.

⟹ The trailing `tag=` token is **byte-clean append**: no existing token's
column, spacing, or the header's byte-52 padding is recomputed. There is no
alignment interaction to compute. (Confirmed by reading `attrs_line`,
Format.cpp ~1261-1312.)

---

## 3. ATTRIBUTION METHOD + CONFIDENCE

The rendered tag is the survivor's OWN create-site tag (I2: CSE /
`ReplaceAllUsesWith` never migrate `mint_tag`; the deleted loser's tag
vanishes). Which Create-site OBJECT survives canonicalization/CSE is a
runtime fact of `QueryImpl::Optimize`; for structurally-pinned nodes it is
determinable from the pipeline, for canonicalized interior TUPLE/MERGE
survivors it is not fully determinable statically. Confidence labels:

* **HIGH** — the code path is unambiguous (message SELECT by receive count;
  demand-fabricated SELECT; the `!p` NEGATE; the pivot/product JOIN; the AGG;
  the `#query` re-materialization INSERT; a condition/bool INSERT).
* **MED** — one dominant candidate, one or two structural alternates.
* **LOW** — a canonicalized interior proxy/guard/forward/restore TUPLE or a
  relation MERGE whose surviving object identity depends on canonicalization
  order; a candidate SET is given. **These rows are provisional and are
  resolved at Step-1 predict-then-verify** (dump the real instrumented `.df`,
  read the tag the emitter prints, reconcile). The SHAPE (`+ tag=…`) and the
  changed-row COUNT are certain regardless; only the LOW tag STRINGS carry
  attribution risk.

**Recurring resolved rules (from reading Build/Connect/Demand/View):**
- Message SELECT, **1 receive** → survives as `build/predicate-select`
  (Connect `continue`s on `1u==num_receives`).
- Message SELECT, **>1 receive** → `connect/receive-select` (Connect unifies).
- Demand-seed message SELECT (post-Connect mint) → `demand/seed-receive`.
- `#query` relation → a fresh `connect/query-insert` INSERT survives
  (Connect.cpp:304); its input proxy is a `connect/insert-proxy` /
  `connect/insert-union` (canon-dependent).
- Single-transmit `#message` head → `build/message-insert` survives; >1 →
  `connect/transmit-insert`.
- Materialized `#local`/`#export` relation → its `build/relation-insert`
  INSERTs are `PrepareToDelete`'d in `CreateProxyOfInserts` (Connect.cpp:54);
  the relation is materialized by the `connect/insert-union` MERGE — **there is
  no surviving INSERT node** for such a relation. (This is why demand_tc /
  negate / symrec / aggregate each show exactly ONE `insert into %table`, the
  `#query` one, and barrier shows 4 `#query` + 1 condition.)
- Zero-arity/condition head (bool token) → `build/condition-insert`.
- Relation MERGE carrying `; callers:` = the relation union
  (`connect/insert-union`) OR, for a demand relation, `demand/relation-union`.

---

## 4. FULL PREDICTED DIFF — negate_1 (7/7 rows)

Source: `copy(A,B):feed(A,B). seen(A):unsee(A). #query out(free,free):copy,!seen`.

| # | dumped node (header) | ATTRIBUTES today | PRIMARY tag | conf | candidates / note |
|---|---|---|---|---|---|
| 1 | `select ^select.0 … recv #message feed/2` | `eqset=1 class=table-less stratum=0` | `build/predicate-select` | HIGH | feed read once (1 receive) |
| 2 | `select ^select.1 … recv #message unsee/1` | `eqset=2 class=table-less stratum=1` | `build/predicate-select` | HIGH | unsee read once |
| 3 | `tuple ^tuple.2 (A)` table:8 mono (negated) | `table=%table:8 eqset=2 class=monotone stratum=2` | `connect/select-proxy` | LOW | {`connect/select-proxy`,`connect/insert-proxy`,`link/negate-source-proxy`,`link/negated-view-proxy`} — the `seen` materialization the NEGATE reads |
| 4 | `tuple ^tuple.3 (A,B)` table:4 diff → insert.6 | `table=%table:4 eqset=4 class=differential stratum=5` | `connect/insert-proxy` | LOW | {`connect/insert-proxy`,`merge-canon/forward`,`connect/select-proxy`} — `out` #query input-proxy feeding query-insert |
| 5 | `tuple ^tuple.4 (A,B)` table:11 mono → negate.5 | `table=%table:11 eqset=1 class=monotone stratum=3` | `connect/insert-proxy` | LOW | {`connect/insert-proxy`,`merge-canon/forward`,`connect/select-proxy`} — `copy` content into the negate body |
| 6 | `negate ^negate.5 … negates ^tuple.2` | `table=%table:4 eqset=4 class=differential stratum=4` | `build/negate-predicate` | HIGH | the `!seen(A)` NEGATION |
| 7 | `insert ^insert.6 … into %table:4` (`out` #query) | `eqset=4 class=differential stratum=6` | `connect/query-insert` | HIGH | `out` is `#query` |

**Predicted unified diff (PRIMARY tags):**
```
@@ negate_1.df.opt.golden @@
-  ATTRIBUTES eqset=1 class=table-less stratum=0
+  ATTRIBUTES eqset=1 class=table-less stratum=0 tag=build/predicate-select
-  ATTRIBUTES eqset=2 class=table-less stratum=1
+  ATTRIBUTES eqset=2 class=table-less stratum=1 tag=build/predicate-select
-  ATTRIBUTES table=%table:8 eqset=2 class=monotone stratum=2
+  ATTRIBUTES table=%table:8 eqset=2 class=monotone stratum=2 tag=connect/select-proxy
-  ATTRIBUTES table=%table:4 eqset=4 class=differential stratum=5
+  ATTRIBUTES table=%table:4 eqset=4 class=differential stratum=5 tag=connect/insert-proxy
-  ATTRIBUTES table=%table:11 eqset=1 class=monotone stratum=3
+  ATTRIBUTES table=%table:11 eqset=1 class=monotone stratum=3 tag=connect/insert-proxy
-  ATTRIBUTES table=%table:4 eqset=4 class=differential stratum=4
+  ATTRIBUTES table=%table:4 eqset=4 class=differential stratum=4 tag=build/negate-predicate
-  ATTRIBUTES eqset=4 class=differential stratum=6
+  ATTRIBUTES eqset=4 class=differential stratum=6 tag=connect/query-insert
```
7 changed rows; 4 HIGH, 3 LOW (rows 3-5, interior proxy survivors).

---

## 5. FULL PREDICTED DIFF — demand_tc_witness (20/20 rows)

Source: right-linear TC, `-demand`. Views: 2 SELECT, 11 TUPLE, 4 JOIN, 2
MERGE, 1 INSERT. Line numbers are the fresh dump's.

| # | node (line) | ATTRIBUTES today | PRIMARY tag | conf | candidates / note |
|---|---|---|---|---|---|
| 1 | `select.0` edge_2 recv (L4) | `eqset=1 class=table-less stratum=0` | `connect/receive-select` | HIGH | edge_2 read in BOTH clause bodies → 2 receives → Connect unifies |
| 2 | `select.1` demand__reachable_from_bf recv (L9) | `eqset=2 class=table-less stratum=1` | `demand/seed-receive` | HIGH | fabricated demand-seed message SELECT (post-Connect) |
| 3 | `tuple.2 (F,T)` table:19 mono → join.14.in1 (L14) | `table=%table:19 eqset=1 class=monotone stratum=2` | `demand/guard-restore` | LOW | {`demand/guard-restore`,`connect/receive-select`→proxy,`view/guard`,`connect/insert-proxy`} — guarded edge_2 read into the recursive guard-join |
| 4 | `tuple.3 (F,T)` table-less → merge.17 ;cycle (L18) | `eqset=4 class=table-less stratum=5` | `join/dedup-restore` | LOW | {`join/dedup-restore`,`build/pivot-join`out-restore,`connect/insert-proxy`} — join.13 result into `path` union |
| 5 | `tuple.4 (From,To)` table:8 mono → join.15/16 ;cycle (L22) | `table=%table:8 eqset=18 class=monotone stratum=5` | `merge-canon/forward` | LOW | {`merge-canon/forward`,`connect/insert-proxy`,`demand/guard-restore`} — `path`-union output read back into recursive joins |
| 6 | `tuple.5 (From)` table-less → merge.18 ;cycle (L27) | `eqset=6 class=table-less stratum=5` | `demand/prop-projection` | LOW | {`demand/prop-projection`,`demand/prop-member`,`join/unused-col-guard`} — recursive demand propagation into d_path union |
| 7 | `tuple.6 (c11)` table:12 mono → join.14/15.in0 ;cycle (L31) | `table=%table:12 eqset=19 class=monotone stratum=5` | `demand/relation-reader` | LOW | {`demand/relation-reader`,`merge-canon/forward`,`connect/insert-proxy`} — d_path relation read into the guard joins |
| 8 | `tuple.7 (From,To)` table:15 mono → join.13.in0 ;cycle (L36) | `table=%table:15 eqset=8 class=monotone stratum=5` | `demand/guard-restore` | LOW | {`demand/guard-restore`,`connect/insert-proxy`,`view/guard`} |
| 9 | `tuple.8 (c14)` table:23 mono → join.16.in0 (L40) | `table=%table:23 eqset=2 class=monotone stratum=6` | `demand/guard-restore` | LOW | {`demand/guard-restore`,`demand/relation-reader`,`connect/select-proxy`} — from select.1 demand seed |
| 10 | `tuple.9 (From,To)` table-less → insert.19 (L44) | `eqset=10 class=table-less stratum=8` | `connect/insert-proxy` | LOW | {`connect/insert-proxy`,`join/dedup-restore`,`demand/guard-restore`} — feeds the `reachable_from` query-insert |
| 11 | `tuple.10 (M,T)` table:19 mono → join.13.in1 (L48) | `table=%table:19 eqset=1 class=monotone stratum=4` | `demand/guard-restore` | LOW | {`demand/guard-restore`,`connect/insert-proxy`,`view/guard`} — 2nd edge_2 guarded read |
| 12 | `tuple.11 (F,T)` table-less → merge.17 ;cycle (L52) | `eqset=15 class=table-less stratum=5` | `join/dedup-restore` | LOW | {`join/dedup-restore`,`connect/insert-proxy`} — join.14 result into `path` union |
| 13 | `tuple.12 (c21)` table-less → merge.18 (L56) | `eqset=2 class=table-less stratum=3` | `demand/prop-projection` | LOW | {`demand/prop-projection`,`demand/relation-reader`,`connect/select-proxy`} — seed key into d_path union |
| 14 | `join.13 (M,F,T) pivot M` (L64) | `eqset=14 class=table-less stratum=5 set=0 depth=1` | `build/pivot-join` | HIGH | recursive body `path(F,M),edge_2(M,T)` |
| 15 | `join.14 (F,T) pivot F` (L71) | `eqset=15 class=table-less stratum=5 set=0 depth=1` | `demand/guard-join` | MED | {`demand/guard-join`,`build/pivot-join`} — demand pushdown `d_path ⋈ path` |
| 16 | `join.15 (From,To) pivot From` (L78) | `eqset=8 class=table-less stratum=5` | `demand/guard-join` | MED | {`demand/guard-join`,`build/pivot-join`} |
| 17 | `join.16 (From,To) pivot From` (L85) | `eqset=10 class=table-less stratum=7` | `demand/guard-join` | MED | {`demand/guard-join`,`build/pivot-join`} — base-body demand guard |
| 18 | `merge.17 (F,T)` callers tuple.3,tuple.11 (L89) | `table=%table:8 eqset=18 class=monotone stratum=5 set=0 depth=1` | `connect/insert-union` | MED | `path` relation union (2 clause bodies) |
| 19 | `merge.18 (c33)` callers tuple.5,tuple.12 (L94) | `table=%table:12 eqset=19 class=monotone stratum=5 set=0 depth=1` | `demand/relation-union` | MED | the d_path (single-col) demand relation union |
| 20 | `insert.19 … into %table:4` (`reachable_from` #query) (L98) | `eqset=10 class=monotone stratum=9` | `connect/query-insert` | HIGH | `reachable_from` is `#query` |

**Predicted unified diff (PRIMARY tags; append-only, one token per line):**
```
@@ demand_tc_witness.df.opt.golden @@  (== key_tc_witness by symlink)
   L4   … stratum=0                              +tag=connect/receive-select
   L9   … stratum=1                              +tag=demand/seed-receive
   L14  … class=monotone stratum=2               +tag=demand/guard-restore     (LOW)
   L18  … class=table-less stratum=5             +tag=join/dedup-restore       (LOW)
   L22  … class=monotone stratum=5               +tag=merge-canon/forward      (LOW)
   L27  … class=table-less stratum=5             +tag=demand/prop-projection   (LOW)
   L31  … class=monotone stratum=5               +tag=demand/relation-reader   (LOW)
   L36  … class=monotone stratum=5               +tag=demand/guard-restore     (LOW)
   L40  … class=monotone stratum=6               +tag=demand/guard-restore     (LOW)
   L44  … class=table-less stratum=8             +tag=connect/insert-proxy     (LOW)
   L48  … class=monotone stratum=4               +tag=demand/guard-restore     (LOW)
   L52  … class=table-less stratum=5             +tag=join/dedup-restore       (LOW)
   L56  … class=table-less stratum=3             +tag=demand/prop-projection   (LOW)
   L64  … set=0 depth=1                          +tag=build/pivot-join
   L71  … set=0 depth=1                          +tag=demand/guard-join        (MED)
   L78  … stratum=5                              +tag=demand/guard-join        (MED)
   L85  … stratum=7                              +tag=demand/guard-join        (MED)
   L89  … set=0 depth=1                          +tag=connect/insert-union     (MED)
   L94  … set=0 depth=1                          +tag=demand/relation-union    (MED)
   L98  … stratum=9                              +tag=connect/query-insert
```
20 changed rows; 3 HIGH + 4 MED + 13 LOW. (The join `set=0 depth=1` rows put
`tag=` after `depth=1`; the non-induction rows put it after `stratum=`.)

Note the two-token append on L64/L71/L89/L94 (they carry `set= depth=`): e.g.
```
-  ATTRIBUTES eqset=14 class=table-less stratum=5 set=0 depth=1
+  ATTRIBUTES eqset=14 class=table-less stratum=5 set=0 depth=1 tag=build/pivot-join
```

---

## 6. key_tc_witness — symlink expectation (certain)

`key_tc_witness.df` == `demand_tc_witness.df` byte-for-byte TODAY, and remains
so after the sweep (the pragma-activated compile IS the `-demand` compile; the
same object graph → the same surviving tags → the same bytes). The golden is a
symlink → `demand_tc_witness.df.opt.golden`. **Do NOT dump-compare or bless it
separately.** At `runall.sh --bless`, `bless_copy` sees a byte-identical
symlink target and emits a **SKIP** line (never a write-through; any
BLESS-REFUSED here is a hard stop). It contributes **0** to the update count.

---

## 7. REPRESENTATIVE DIFFS + TOTALS — the other three real goldens

### 7a. aggregate_1 — 6 changed rows (all shown)
`#message pair`, `#local grouped`, `#query get_grouped`, `count_i32` aggregate.

| # | node | tag (PRIMARY) | conf | note |
|---|---|---|---|---|
| 1 | `select.0 (c1:i32)` table-less (no recv) | `build/literal-constant` | MED | {`build/literal-constant`,`build/true-constant`} — a constant SELECT |
| 2 | `select.1 (A,Y)` recv pair/2 | `build/predicate-select` | HIGH | pair read once |
| 3 | `tuple.2 (A,c5,Y)` table:15 mono → aggregate.4 | `connect/select-proxy` | LOW | {`connect/select-proxy`,`build/all-constants`,`view/guard`} — AGG input |
| 4 | `tuple.3 (A,c8,N)` table:10 diff → insert.5 | `connect/insert-proxy` | LOW | {`connect/insert-proxy`,`build/clause-head`,`merge-canon/forward`} |
| 5 | `aggregate.4 … count_i32` | `build/aggregate` | HIGH | the `over(){}` AGG |
| 6 | `insert.5 … into %table:10` (get_grouped #query) | `connect/query-insert` | HIGH | `get_grouped` is `#query` |

Predicted (append-only): rows gain `tag=build/literal-constant`,
`tag=build/predicate-select`, `tag=connect/select-proxy`,
`tag=connect/insert-proxy`, `tag=build/aggregate`, `tag=connect/query-insert`.
**Total changed rows: 6.**

### 7b. symrec_tie_1 — 12 changed rows (all shown)
`#message edge`, `#local tc`, `#query out`. Non-linear TC (self-join).

| # | node | tag (PRIMARY) | conf | note |
|---|---|---|---|---|
| 1 | `select.0` edge/2 recv (feeds 3 tuples) | `connect/receive-select` | HIGH | edge read in ≥2 bodies → unified |
| 2 | `tuple.1 (A,B)` → merge.10 ;cycle | `join/dedup-restore` | LOW | {`join/dedup-restore`,`connect/insert-proxy`} — join.8 result into tc union |
| 3 | `tuple.2 (A,B)` → merge.10 ;cycle | `join/dedup-restore` | LOW | join.9 result into tc union |
| 4 | `tuple.3 (A,B)` table:4 mono → join.8/9 ;cycle | `merge-canon/forward` | LOW | {`merge-canon/forward`,`connect/insert-proxy`} — tc read back into joins |
| 5 | `tuple.4 (A,B)` → insert.11 | `connect/insert-proxy` | LOW | {`connect/insert-proxy`,`connect/query-insert`input} — into `out` query-insert |
| 6 | `tuple.5 (A,X)` table:8 mono → join.8.in1 | `view/guard` | LOW | {`view/guard`,`connect/receive-select`proxy,`connect/insert-proxy`} — guarded edge read |
| 7 | `tuple.6 (A,X)` table:8 mono → join.9.in0 | `view/guard` | LOW | guarded edge read |
| 8 | `tuple.7 (A,X)` → merge.10 (A,B=X) | `build/clause-head` | LOW | {`build/clause-head`,`connect/insert-proxy`} — base edge into tc union |
| 9 | `join.8 (X,A,B) pivot X` | `build/pivot-join` | HIGH | self-join arm |
| 10 | `join.9 (X,A,B) pivot X` | `build/pivot-join` | HIGH | self-join arm |
| 11 | `merge.10` callers tuple.1,2,7 | `connect/insert-union` | MED | `tc` relation union (3 operands) |
| 12 | `insert.11 … into %table:4` (`out` #query) | `connect/query-insert` | HIGH | `out` is `#query` |

**Total changed rows: 12.** (4 HIGH, 1 MED, 7 LOW.)

### 7c. barrier_neck_1 — 26 changed rows (representative 14 shown)
5 `#message`, 6 `#local`, 4 `#query` (t_all/tb_all/tp_all/s_all), plus a
`@barrier`-desugared condition (the `pred_18446…/0` unit relation, `c*:bool`).

| node | tag (PRIMARY) | conf | note |
|---|---|---|---|
| `select.0` mp/2 recv | `build/predicate-select` | HIGH | 1 receive |
| `select.1` mq/1 recv | `build/predicate-select` | HIGH | 1 receive |
| `select.2` mr/1 recv | `build/predicate-select` | HIGH | 1 receive |
| `select.3 (c5:bool)` (no recv) | `build/true-constant` | MED | {`build/true-constant`,`build/literal-constant`} — condition token |
| `select.4 (c6:bool)` `relation pred_…/0` | `build/condition-select` | HIGH | unit-relation read (SELECT survives) |
| `join.16/17/18/19/20` pivot | `build/pivot-join` | HIGH | body joins |
| `insert.21 (c13:bool) into %table:4` | `build/condition-insert` | MED | {`build/condition-insert`} — condition unit relation |
| `insert.22 (A) into %table:11` (t_all #query) | `connect/query-insert` | HIGH | |
| `insert.23 (A) into %table:14` (tb_all #query) | `connect/query-insert` | HIGH | |
| `insert.24 (A) into %table:17` (tp_all #query) | `connect/query-insert` | HIGH | |
| `insert.25 (A) into %table:20` (s_all #query) | `connect/query-insert` | HIGH | |
| `tuple.5…15` (interior guards/proxies/restores) | LOW candidate sets | LOW | {`view/guard`,`connect/select-proxy`,`connect/insert-proxy`,`join/dedup-restore`,`build/clause-head`,`build/condition-*`} per structural role |

**Total changed rows: 26** (5 SELECT + 11 TUPLE + 5 JOIN + 5 INSERT). The 5
INSERTs = 4 `connect/query-insert` + 1 `build/condition-insert` (HIGH/MED);
the 5 SELECTs and 5 JOINs are HIGH/MED; the 11 interior TUPLEs are LOW.

---

## 8. GOLDENED-ZERO — every other family moves 0 bytes (I4)

`mint_tag` is never in Hash/Equals, never a lowering input, never copied by any
field-copy helper (draft §3 I3/I4). Only the `.df` emitter reads it. So EVERY
non-`.df` golden family is byte-invariant. Full inventory + the command that
proves zero movement:

| family | files | why zero | proof |
|---|---|---|---|
| `.stdout` | 172 | runtime behavior unchanged (tag never gates a transform) | full suite byte-compare |
| `.oracle.stdout` | 66 | independent evaluator, no `.df` | suite |
| `.monotone.stdout` | 66 | " | suite |
| `.behavioral.stdout` | 62 | frozen ABI, no `.df` | suite |
| `.rel.{opt,none,nodf,nocf}` | 18 | Rel emitter UNTOUCHED (slice 1) | suite |
| `.region.{opt,none,nodf,nocf}` | 24 | Regional emitter UNTOUCHED | suite |
| `.contract.opt` | 5 | contract emitter UNTOUCHED | suite |
| `.ir.opt` | 3 | ControlFlow emitter UNTOUCHED | suite |
| `.h.opt` | 3 | codegen UNTOUCHED | suite |
| **`.df.opt`** | **6** | **THE ONLY family that moves** — 5 real + 1 symlink | Step-1 predict-then-verify |

**Verification commands (repo root):**
```sh
# (a) full suite — every non-.df family byte-compared, must end SUITE: PASS
DR=build/debug/bin/drlojekyll tests/OptDiff/runall.sh /tmp/s5p-work
# (b) ctest — IdentityTypes / DataFlowValidators / RelValidators / …
cd build/debug && ctest --output-on-failure
```
Any moved byte in an (a) non-`.df` family is an I4 violation (tag leaked into a
transform/emission decision) — a finding, not a re-bless.

**Bless expectation (after reviewing Step-1 diffs):**
```sh
tests/OptDiff/runall.sh --bless /tmp/s5p-work
# EXPECT: exactly 5 content updates → "BLESS: 5 golden(s) updated"
#         + 1 SKIP line for key_tc_witness.df.opt.golden (symlink, byte-identical
#           to demand_tc_witness's target) — never a write-through.
# Since F32, byte-identical non-symlink re-blesses also print SKIP, so the
# count "5" is exactly the real content deltas (the 5 real .df.opt goldens).
```

---

## 9. DETERMINISM (the stability contract)

- **Literal-derived, per-site constant.** Every `mint_tag` is a compile-time
  `const char*` string literal at a fixed source site. Same binary + same
  input → identical `QueryImpl::Optimize` object graph → identical surviving
  views → identical rendered tags → **byte-identical `.df` dump**. Two
  back-to-back `-df-out` runs diff empty (I1).
- **Churns ONLY on deliberate rename.** The value changes exclusively when a
  human edits the literal (renames the concept). It does NOT change on
  unrelated compiler edits, recompiles, or reorderings — which is the entire
  reason the tag (unlike `file:line` or `std::source_location`) is
  golden-safe and lives in a pinned dump. A tag delta in a future diff is a
  reviewed, intentional concept rename → a one-line bless.
- **Path/basename independence is moot.** The tag encodes no file path and no
  line number (R2: `<pass>` names the phase, not the file). There is nothing
  path-derived to normalize; relocating a file or renumbering lines leaves
  every tag byte unchanged.

---

## 10. PANEL-DEPENDENCE FLAGS (where a finding would move a predicted byte)

1. **Render placement (draft OQ1).** These predictions assume `tag=` on the
   ATTRIBUTES line (append-only, no padding). If the panel moves it to the
   HEADER row (through `with_comment`), EVERY predicted diff here is void —
   the token would interact with the byte-52 padding law and displace the
   `;`-prose provenance comment. **All §4-§7 bytes depend on the ATTRIBUTES
   choice.** (Recommend ATTRIBUTES, per draft.)
2. **Tag STRINGS (s5p-tags.md).** The HIGH/MED rows are pinned by structure but
   still print the literal from the tag table; any rename in a panel revision
   (e.g. `demand/guard-join` → `demand/pushdown-join`) changes those exact
   bytes 1:1. The LOW rows additionally depend on the canonicalization
   survivor — resolved at predict-then-verify, not by the table alone.
3. **Column-tag non-render (R1).** Predictions assume columns render no row
   (only VIEW `mint_tag` reaches the emitter). If a panel finding renders
   column tags, the changed-row count explodes (253 sites, not 71 rows) — a
   different golden shape entirely. (Confirmed against the emitter: columns
   have no node block; R1 holds.)
4. **Untagged-view assumption.** Predictions assume every view in the 6
   goldens is minted via a swept site (→ every ATTRIBUTES line gains a token).
   Verified: 0 CreateDerived, all DataFlow view creation is in the 17 swept
   files. If any view reaches the dump via an unswept path, its ATTRIBUTES
   line would gain NO token (I5) and the count for that case drops by one.
```
