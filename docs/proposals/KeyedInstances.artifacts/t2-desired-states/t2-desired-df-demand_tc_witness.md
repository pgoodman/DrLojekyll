# T2a desired `-df-out` — `demand_tc_witness.dr` (demand-ON)

DESIRED-STATE artifact. This is the EXACT text the future `-df-out` emitter
SHOULD print for `tests/OptDiff/cases/demand_tc_witness.dr` compiled WITH
`-demand` (its `.drflags` sidecar). Implementation later means: make the
compiler print §1 byte-for-byte; gate by diff (T3 `df opt` golden sidecar).

STATUS: **REVISED 2026-07-18 against ratified t2-dump-spec v3 (post-E-61..E-66).**
No longer DRAFT-PENDING-REVISION. This pass folds the committed critique
(`critique-df-demand_tc_witness.md`) AND the v3 ratified rulings, which
supersede several critique recommendations. Amendments applied (see §4):

- **F0 back-edge rule WITHDRAWN** (per task + spec a2). The v1 `; back-edge`
  (`user det_seq <= def det_seq`) heuristic is gone; the critique's
  induction-membership counter-proposal is ALSO gone. v3 (decision a2-i)
  mandates the reachability-exact `; cycle` marker: an edge `def => user` is
  marked `; cycle` iff `def` is REACHABLE from `user`. Recomputed from actual
  graph reachability — 13 edges fire (§3 Q1), the intra-SCC edge set.
- **F1 (join header arity)**: v3 pins header arity = ALL output columns
  INCLUDING projected pivots. Joins now declare pivot + merged outputs.
- **F1b (join port order)**: v3 JOIN grammar — `.in<K>` by joined_views
  position; block body renders `pivot`/`out <- .in<J>.<col>` in OUTPUT-COLUMN
  position (accessor order). Re-derived from the accessors + Demand.cpp.
- **F2 (producer=)**: v3 (a3) DROPS `producer=` from the default dump
  entirely (config-invariance). The speculative `producer=DEMAND-SEED` line is
  gone. The fabricated `demand__*` names ARE config-invariant and stay visible.
- **F3 (INSERT redundancy)**: v3 INSERT header is
  `insert ^insert.<id> (<producer col tokens>) into %table:<id>`, terminal,
  NO ATTRIBUTES `table=` line, no `=>`.
- **F4 (@cN suffix)**: REJECTED. `cN` is a fallback-ONLY token (used in place
  of an absent name), never an always-appended suffix. Every `@cN` removed.
- **F4/Q5 (`_MissingVar`)**: v3 column rule is `<var-or-cN>` — NEVER
  `_MissingVar`. Every nameless column now renders its finalized `c<id>`.
- **F7 (prose header)**: REJECTED. The multi-line `;; module:`/clause header
  is gone; the header is the single line `dataflow`.
- **E-61 kind order**: negations BEFORE compares (moot here — no neg/cmp).
- **callers**: v3 restricts `; callers:` to MERGE blocks only.

--------------------------------------------------------------------
## §0 Provenance

- Branch/tip: `keyed-instances` @ `6328d01121eb8a447cef204a0b78ec2e47334e27`
  (the owner-ratification commit; docs-only past `63c8443c`, the frozen
  binary — the built graph is byte-identical, ratification touched no code).
- Frozen binary used to ground the graph:
  `scratchpad/baseline-bin/drlojekyll.debug.63c8443c`, invoked
  `-demand <case> -dot-out ... -ir-out ...`.
- Spec (BINDING, RATIFIED): `t2-dump-spec.md` v3 §1 (T2a). Read end to end.
- GROUND TRUTH graph (post-optimize, `-demand`, this session):
  - `scratchpad/ground/demand_tc_witness.dot` — every view kind, column,
    variable name, STRATUM, EQ SET, TABLE id, SET/DEPTH, and all 38 edges.
  - `scratchpad/ground/demand_tc_witness.ir` — table/col/index id spaces.
  - `tests/OptDiff/cases/demand_tc_witness.dr` — the source.
- Code re-read this session:
  - `lib/DataFlow/Query.h:1176-1214` — `ForEachView` per-kind DefList order
    (selects, tuples, kv_indices, joins, maps, aggregates, merges,
    NEGATIONS, compares, inserts — E-61 order confirmed in-code).
  - `lib/DataFlow/Query.cpp:822-844` — `NthInputPivotSet`/`NthInputMergedColumn`
    (key-lookup accessor order; the JOIN grammar's OUTPUT-COLUMN position).
  - `lib/DataFlow/Format.cpp:328-390` — DOT JOIN port emission == accessor
    order (pivot sets by out-col index + UseList, then merged by out-col
    index); DOT edge direction = consumer:port -> producer:col.
  - `lib/DataFlow/Demand.cpp:154-168` — demand-injected joins have
    `joined_views = {demand_side, read}`, input-use order matching; so on
    joins 14/15/16 the demand relation is `.in0`, the read is `.in1`.

--------------------------------------------------------------------
AMENDED 2026-07-19 (round-2 adjudication + v3.2 grammar unification,
ledger §10, errata E-70/E-71): §1 re-rendered under the session-pinned
emitter rulings (p5)-(p9) with the RATIFIED (p2)/(p3) pins
back-applied (E-70 — this artifact's §2.3 "all edges dst=src" rule
predated them): `ATTRIBUTES` keyword restored on every attributes
line (p5); identity `=>` entries now BARE (p2); producer-side
`.in<K>` maps render pure producer tokens in join-port order (p3:
`.in0(F=c11)` -> `.in0(c11)`, `.in0(M=To, F=From)` -> `.in0(To,
From)`); `; terminal` stripped (p8); JOIN bodies to the p9 form;
comments re-padded to the p6 column; insert.19 GAINS its ATTRIBUTES
line `class=monotone stratum=9` (derived from this artifact's own
§2.1 table row det_seq=19 — the committed block omitted it, an E-70
axis). GRAPH FACTS UNCHANGED (20 blocks, 24 `=>` edges, 13 cycle
marks, demand__ numbering). Pre-amendment renderings in derivation
prose (incl. §2.3's dst=src rule statement) are HISTORICAL.

## §1 THE DESIRED `-df-out` TEXT

```
dataflow

select ^select.0 (M:u64, T:u64)                    ; recv #message edge_2/2
  ATTRIBUTES class=table-less stratum=0
  => ^tuple.2 (F=M, T)
  => ^tuple.10 (M, T)

select ^select.1 (c3:u64)                          ; recv #message demand__reachable_from_bf/1
  ATTRIBUTES class=table-less stratum=1
  => ^tuple.8 (c14=c3)
  => ^tuple.12 (c21=c3)

tuple ^tuple.2 (F:u64, T:u64)
  ATTRIBUTES table=%table:19 class=monotone stratum=2
  => ^join.14 .in1(F, T)

tuple ^tuple.3 (F:u64, T:u64)
  ATTRIBUTES class=table-less stratum=5
  => ^merge.17 (F, T)                              ; cycle

tuple ^tuple.4 (From:u64, To:u64)
  ATTRIBUTES table=%table:8 class=monotone stratum=5
  => ^join.15 .in1(From, To)                       ; cycle
  => ^join.16 .in1(From, To)

tuple ^tuple.5 (From:u64)
  ATTRIBUTES class=table-less stratum=5
  => ^merge.18 (c33=From)                          ; cycle

tuple ^tuple.6 (c11:u64)
  ATTRIBUTES table=%table:12 class=monotone stratum=5
  => ^join.14 .in0(c11)                            ; cycle
  => ^join.15 .in0(c11)                            ; cycle

tuple ^tuple.7 (From:u64, To:u64)
  ATTRIBUTES table=%table:15 class=monotone stratum=5
  => ^join.13 .in0(To, From)                       ; cycle

tuple ^tuple.8 (c14:u64)
  ATTRIBUTES table=%table:23 class=monotone stratum=6
  => ^join.16 .in0(c14)

tuple ^tuple.9 (From:u64, To:u64)
  ATTRIBUTES class=table-less stratum=8
  => ^insert.19 (From, To)

tuple ^tuple.10 (M:u64, T:u64)
  ATTRIBUTES table=%table:19 class=monotone stratum=4
  => ^join.13 .in1(M, T)

tuple ^tuple.11 (F:u64, T:u64)
  ATTRIBUTES class=table-less stratum=5
  => ^merge.17 (F, T)                              ; cycle

tuple ^tuple.12 (c21:u64)
  ATTRIBUTES class=table-less stratum=3
  => ^merge.18 (c33=c21)

join ^join.13 (M:u64, F:u64, T:u64) {
  pivot M:u64 <- .in0.To, .in1.M
  out F:u64 <- .in0.From
  out T:u64 <- .in1.T
}
  ATTRIBUTES class=table-less stratum=5 set=0 depth=1
  => ^tuple.3 (F, T)                               ; cycle

join ^join.14 (F:u64, T:u64) {
  pivot F:u64 <- .in0.c11, .in1.F
  out T:u64 <- .in1.T
}
  ATTRIBUTES class=table-less stratum=5 set=0 depth=1
  => ^tuple.11 (F, T)                              ; cycle

join ^join.15 (From:u64, To:u64) {
  pivot From:u64 <- .in0.c11, .in1.From
  out To:u64 <- .in1.To
}
  ATTRIBUTES class=table-less stratum=5
  => ^tuple.7 (From, To)                           ; cycle

join ^join.16 (From:u64, To:u64) {
  pivot From:u64 <- .in0.c14, .in1.From
  out To:u64 <- .in1.To
}
  ATTRIBUTES class=table-less stratum=7
  => ^tuple.9 (From, To)

merge ^merge.17 (F:u64, T:u64)                     ; callers: ^tuple.3, ^tuple.11
  ATTRIBUTES table=%table:8 class=monotone stratum=5 set=0 depth=1
  => ^tuple.4 (From=F, To=T)                       ; cycle
  => ^tuple.5 (From=F)                             ; cycle

merge ^merge.18 (c33:u64)                          ; callers: ^tuple.5, ^tuple.12
  ATTRIBUTES table=%table:12 class=monotone stratum=5 set=0 depth=1
  => ^tuple.6 (c11=c33)                            ; cycle

insert ^insert.19 (From:u64, To:u64) into %table:4
  ATTRIBUTES class=monotone stratum=9
```

--------------------------------------------------------------------
## §2 Derivation notes — every block mapped to its ground evidence

### 2.1 The id rule (FIRM — the block numbering)

`ForEachView` (Query.h:1176-1214) iterates the per-kind DefLists in EXACTLY:
**selects → tuples → kv_indices → joins → maps → aggregates → merges →
negations → compares → inserts** (E-61 in-code confirmed: negations are
pushed BEFORE compares — Query.h:1199-1203; moot here, no neg/cmp views).
`det_seq` (Query.h:472) is stamped dense 0..N-1 in this traversal at the last
stamp (IdentifyInductions, Build.cpp:2597), and the window to the drain is
view-neutral (spec §1.2), so det_seq is a bijection onto {0..19}. Block id =
`^<kind>.<det_seq>`, blocks ascending.

LEVER (Columns.cpp `FinalizeColumnIDs`): column ids are assigned in the SAME
ForEachView traversal, monotone per view — so the smallest output col-id of
each view is strictly increasing in det_seq. The 20 live views, keyed to the
ground DOT by their finalized cols:

| det_seq | block       | out cols            | DOT cell (kind / STRATUM / TABLE / SET) |
|---------|-------------|---------------------|-----------------------------------------|
| 0  | ^select.0  | M:c1, T:c2          | RECEIVE  STRATUM 0  EQ SET 1 |
| 1  | ^select.1  | c3 (_MissingVar)    | RECEIVE  STRATUM 1  EQ SET 2 |
| 2  | ^tuple.2   | F:c4, T:c5          | TUPLE  TABLE 19  STRATUM 2 |
| 3  | ^tuple.3   | F:c6, T:c7          | TUPLE  STRATUM 5  EQ SET 4 |
| 4  | ^tuple.4   | From:c8, To:c9      | TUPLE  TABLE 8  STRATUM 5 |
| 5  | ^tuple.5   | From:c10            | TUPLE  STRATUM 5  EQ SET 6 |
| 6  | ^tuple.6   | c11 (_MissingVar)   | TUPLE  TABLE 12  STRATUM 5 |
| 7  | ^tuple.7   | From:c12, To:c13    | TUPLE  TABLE 15  STRATUM 5 |
| 8  | ^tuple.8   | c14 (_MissingVar)   | TUPLE  TABLE 23  STRATUM 6 |
| 9  | ^tuple.9   | From:c15, To:c16    | TUPLE  STRATUM 8  EQ SET 10 |
| 10 | ^tuple.10  | M:c17, T:c18        | TUPLE  TABLE 19  STRATUM 4 |
| 11 | ^tuple.11  | F:c19, T:c20        | TUPLE  STRATUM 5  EQ SET 15 |
| 12 | ^tuple.12  | c21 (_MissingVar)   | TUPLE  STRATUM 3  EQ SET 2 |
| 13 | ^join.13   | pivot M:c22, F:c23, T:c24 | JOIN  SET 0 DEPTH 1  STRATUM 5 |
| 14 | ^join.14   | pivot F:c25, T:c26  | JOIN  SET 0 DEPTH 1  STRATUM 5 |
| 15 | ^join.15   | pivot From:c27, To:c28 | JOIN  STRATUM 5 |
| 16 | ^join.16   | pivot From:c29, To:c30 | JOIN  STRATUM 7 |
| 17 | ^merge.17  | F:c31, T:c32        | UNION  TABLE 8  SET 0 DEPTH 1  STRATUM 5 (path) |
| 18 | ^merge.18  | c33 (_MissingVar)   | UNION  TABLE 12  SET 0 DEPTH 1  STRATUM 5 |
| 19 | ^insert.19 | (none; ins From/To) | MATERIALIZE reachable_from  TABLE 4  STRATUM 9 |

FIRMNESS: block ids 0-19 are FIRM (the monotone col-id partition is a code
fact). Each view's out-col range is strictly increasing, so the whole
numbering is pinned.

### 2.2 Column tokens — the v3 `<var-or-cN>` rule (NO `_MissingVar`, NO `@cN`)

The DOT prints `_MissingVar` for a column with no source parse variable
(Format.cpp `col.Variable()` placeholder). v3 §1.3 mandates: token =
`<var-or-cN>:<type>`, var when present, ELSE `c<id>` (the finalized
FinalizeColumnIDs id) — NEVER `_MissingVar`, and NEVER an `@cN` suffix on a
named column. So the six nameless columns render their finalized id:
select.1 c3, tuple.6 c11, tuple.8 c14, tuple.12 c21, merge.18 c33. (The join
pivot OUTPUT columns c22/c25/c27/c29 all carry real names — M/F/From/From —
in the DOT, so only the demand-relation *input* columns are nameless, and
they surface as `.in0.c11` / `.in0.c14` in the join bodies.) Every type is
`u64`.

### 2.3 Edge direction and `=>` maps (DOT is use→def; `=>` is def→use)

Format.cpp emits DOT edges `consumer:port -> producer:col`. §1 REVERSES every
DOT edge to a producer→user `=>` tail call, ordered (user det_seq, port), arg
map `dst=src` (dst = the USER's own column token, src = the PRODUCER's column
token). All 38 DOT edges reversed mechanically; each `=>` traces to exactly
one DOT edge. Spot checks:
- `^select.0 => ^tuple.2 (F=M, T=T)` ⇐ DOT `v4364685424:p0 -> v4364688112:c1`,
  `:p1 -> :c2` (tuple.2 F/T read select.0 M/T).
- `^merge.17 => ^tuple.4 (From=F, To=T)` ⇐ DOT `v4364693680:p0 ->
  v4364684432:c31`, `:p1 -> :c32`.
- `^tuple.11 => ^merge.17` ⇐ DOT `v4364684432 -> v54639299200` (un-ported
  UNION input edge, reversed).

### 2.4 JOIN grammar — v3 accessor-order rendering (F1/F1b applied)

v3 §1.3: inputs labeled `.in<K>` by joined_views position; the block header
declares ALL output columns (pivots + merged); the body renders, in
OUTPUT-COLUMN position (NthInputPivotSet / NthInputMergedColumn accessor
order): `pivot <tok> <- .in<J>.<col>, .in<K>.<col>` per pivot set, then
`out <tok> <- .in<J>.<col>` per merged column. `.in<K>` binds to a producer
via that producer's `=> ^join.N .in<K>(...)` line.

joined_views order (the `.in<K>` binding) per join, from the DOT pivot-port
order (== joined_views order at construction) cross-checked with
Demand.cpp:154-168 (`{demand_side, read}`):
- join.13: `.in0`=tuple.7 (path image, %table:15), `.in1`=tuple.10 (edge_2
  image, %table:19). pivot M ← .in0.To(c13), .in1.M(c17); out F ← .in0.From(c12);
  out T ← .in1.T(c18). (recursive body `path(F,M) ⋈ edge_2(M,T)`.)
- join.14: `.in0`=tuple.6 (demand relation d_path, %table:12), `.in1`=tuple.2
  (edge_2 image, %table:19). pivot F ← .in0.c11, .in1.F(c4); out T ← .in1.T(c5).
- join.15: `.in0`=tuple.6 (d_path), `.in1`=tuple.4 (path, %table:8). pivot From
  ← .in0.c11, .in1.From(c8); out To ← .in1.To(c9).
- join.16: `.in0`=tuple.8 (demand seed, %table:23), `.in1`=tuple.4 (path).
  pivot From ← .in0.c14, .in1.From(c8); out To ← .in1.To(c9).

Header arity now INCLUDES the pivot (F1 fix): join.13 → `(M, F, T)`;
join.14/15/16 → `(pivot, out)` two-col. A producer feeding a pivot emits that
pivot in its `=> .in<K>(...)` map (e.g. tuple.6 `=> ^join.14 .in0(F=c11)`
because the pivot output is named F and tuple.6 contributes its c11 to it);
a producer feeding both pivot and a merged output lists both (tuple.7
`=> ^join.13 .in0(M=To, F=From)`).

### 2.5 ATTRIBUTES (table / class / stratum / set-depth)

- `table=%table:N`: present iff the DOT node carries a `TABLE N` cell. Six
  backed models, all cross-referenced to the `.ir` `create %table:N` blocks
  (lines 9,16,23,28,35,42): %table:4 (insert.19 reachable_from), %table:8
  (tuple.4 AND merge.17 — shared model, the path union + its view), %table:12
  (tuple.6 AND merge.18 — d_path), %table:15 (tuple.7), %table:19 (tuple.2 AND
  tuple.10 — edge_2 image), %table:23 (tuple.8 — demand seed). Shared-model
  pairs print `table=` on BOTH views, faithful to the model (spec §1.3, and
  the group_ids/CSE-guard invariant — distinct views by identity on one
  DataTable).
- `class=`: ZERO purple (deletion) edges in the whole DOT (`grep purple` = 0)
  and ZERO overdelete/rederive/`-recursive`/`-nonrecursive` in the `.ir`
  (grep = 0) — every backed table is deletion-INcapable, hence
  **class=monotone**; table-less views (no TABLE cell) render
  **class=table-less**. Per spec §1.3 CLASS SEMANTICS: RECURSION DOES NOT
  IMPLY DIFFERENTIAL — this is a fully monotone insert-only recursive tc, so
  path/d_path are `monotone` despite being SET-0 induction members.
- `stratum=N`: from each DOT `STRATUM N` cell (== `QueryView::Stratum()`;
  the DOT annotates from the same Stratify pass — critique F6 confirmed no
  offset). Values: select.0=0, select.1=1, tuple.2=2, tuple.12=3, tuple.10=4,
  tuple.3/4/5/6/7/11=5, tuple.8=6, tuple.9=8, join.13/14/15=5, join.16=7,
  merge.17/18=5, insert.19=9.
- `set=0 depth=1`: the DOT `SET 0 DEPTH 1` cell (merge_set_id + InductionDepth)
  — carried by exactly the four induction members join.13, join.14, merge.17,
  merge.18. No other view carries a SET/DEPTH cell.

### 2.6 INSERT (F3 applied)

v3 §1.3: `insert ^insert.<id> (<producer col tokens>) into %table:<id>` —
terminal, NO `=>`, NO ATTRIBUTES `table=` line. insert.19 reads tuple.9's
From(c15)/To(c16) in input-position order → `(From:u64, To:u64)`. The
`into %table:4` carries the table id; the redundant `table=%table:4` attrs
line (and the speculative `class=/stratum=` on it) are DROPPED. The DOT's
`c0/c1` ports are input-position INDICES (Format.cpp loop index, not col.Id())
pointing at tuple.9's finalized c15/c16 — an INSERT owns no finalized output
columns.

### 2.7 The demand machinery (what the fabricated graph shows)

- `^select.1` = the fabricated `demand__reachable_from_bf/1` receive
  (`.ir` `proc ^receive:demand__reachable_from_bf/1`; ABI-suppressed — no
  driver seam). Its single nameless col renders `c3`. The `demand__` name is
  config-invariant (a3) and stays visible in §1.
- `^merge.18`/%table:12 = the demand relation `d_path`; `^tuple.6` is its
  table view feeding the push-down guard joins join.14/join.15 on their pivot
  (`.in0`) side.
- `^tuple.8`/%table:23 = the demand seed image; `^join.16` is the
  query-projection guard (`d_seed ⋈ path`) producing the answer rows via
  tuple.9 → insert.19.
- `^join.13` = the recursive body join `path ⋈ edge_2`; `^join.14/15` = the
  SIP push-down guards (`d_path ⋈ edge_2` / `d_path ⋈ path`). STRUCTURE
  (producers, pivots, outputs, tables) is ground-firm; the English
  rule-body labels are interpretive and kept OUT of the dump body (§1) to
  keep the golden emitter-faithful.

--------------------------------------------------------------------
## §3 Resolved frictions (was: open questions) + residual uncertainties

**Q1 — the cycle marker (F0 fully resolved via v3 reachability-exact rule).**
The v1 `; back-edge` (`user det_seq <= def det_seq`) heuristic is WITHDRAWN
(task directive; spec a2). It was unsound by OVER-firing (the critique F0
showed 7 fires; a literal impl would tag non-cycle edges). v3 (a2-i) mandates:
mark `; cycle` iff the edge's def is REACHABLE from its user (the edge closes a
cycle). Recomputed over the actual `=>` graph (script
`scratchpad/reach.py`): the SCC is
{tuple.3, tuple.4, tuple.5, tuple.6, tuple.7, tuple.11, join.13, join.14,
join.15, merge.17, merge.18}, and the 13 intra-SCC edges fire:

    tuple.3  => merge.17   (3 reachable from 17)
    tuple.4  => join.15    (4 reachable from 15)
    tuple.5  => merge.18   (5 reachable from 18)
    tuple.6  => join.14    (6 reachable from 14)
    tuple.6  => join.15    (6 reachable from 15)
    tuple.7  => join.13    (7 reachable from 13)
    tuple.11 => merge.17   (11 reachable from 17)
    join.13  => tuple.3    (13 reachable from 3)
    join.14  => tuple.11   (14 reachable from 11)
    join.15  => tuple.7    (15 reachable from 7)
    merge.17 => tuple.4    (17 reachable from 4)
    merge.17 => tuple.5    (17 reachable from 5)
    merge.18 => tuple.6    (18 reachable from 6)

Note `tuple.4 => join.16` does NOT fire (join.16 → tuple.9 → insert.19 is a
sink; tuple.4 is not reachable from join.16), nor does `tuple.8 => join.16`
(demand-seed feed, outside the SCC), nor `join.16 => tuple.9`. This matches
intuition: the demand-seed answer-projection arm (tuple.8, join.16, tuple.9,
insert.19) and the edge_2/demand-seed ingress (select.0/1, tuple.2, tuple.8)
sit OUTSIDE the path/d_path fixpoint SCC. The marker is emitter-computable as
one memoized reachability pass (spec a2-i).

**Q3 — INSERT (F3): RESOLVED.** Header carries producer col tokens + `into
%table:4`, terminal, no attrs line, no `=>`. See §2.6.

**Q4 — JOIN ports (F1/F1b): RESOLVED.** Rendered by v3 accessor order
(`.in<K>` = joined_views position; pivot/out in output-column position). See
§2.4. The v1 producer-det_seq regrouping is gone.

**Q5 — `_MissingVar` (F4): RESOLVED.** Nameless columns render `c<id>`, never
`_MissingVar`, never an `@cN` suffix. See §2.2.

**Q6 — shared-model views: RESOLVED as symmetric.** `table=%table:N` on both
views of each shared pair, faithful (spec §1.3). No owner/reader distinction.

### Residual uncertainties (LOUD — flagged, not silently guessed)

1. **`=>`-to-join port-map surface form: RESOLVED by v3.1 (p1)-(p3).** (p1)
   pins the UNIFORM block header (`<kind> ^<kind>.<det_seq> (<own finalized
   column tokens>)` for every kind including join — no `-> (...)` arrow, no
   `[pivot ...]` header tag) — the header-form question this residual
   originally flagged. (p2) pins that `=>` maps render the BARE token for an
   identity mapping, `dst=src` only when names differ, applying to
   producer-side join edges too. (p3) pins that a producer block DOES emit
   its `=> ^join.<id> .in<K> (...)` line (tail-call completeness), in the
   producer's OWN tokens (bare where identity per p2), with the join block
   itself owning the role mapping; the `.in<K>` assignment is joined_views
   UseList position, carried as PREDICTED until the first bless code-reads
   it. §1's rendering already conforms (all producer-side join maps here are
   non-identity renames, so no p2 bare-token instance appears in this
   particular corpus case — flagged for the NEXT witness to confirm the bare
   form renders correctly).

2. **INSERT loses class/stratum.** Following v3's "no ATTRIBUTES table= line"
   + the sketch's terminal one-liner literally, insert.19 has NO attributes
   line, so its `class=monotone stratum=9` do not appear. If the owner wants
   stratum on every block, the INSERT rule needs a carve-out (attrs line minus
   the redundant `table=`). Rendered minimal here per the literal spec; FLAG
   for owner taste.

3. **Attribute-field ORDER on the attrs line** (`table= class= stratum=
   set= depth=`) follows the spec §1.3 bullet's listing order. Not
   independently pinned as canonical; low risk (deterministic either way once
   the emitter fixes an order), flagged for completeness.

4. **Select trailing `; #message` comment.** Rendered per the ir-dump §1
   SELECT convention (relation/stream + receive kind). If the emitter omits
   the message-name comment, select.0/select.1 diverge on that trailing token.
   Low risk (derivable from the SELECT's stream), flagged.

These four are rendering-surface pins, NOT graph-fact uncertainties: every
block, column, edge, table, class, stratum, set/depth, and cycle marker in §1
is ground-verified against the frozen binary's DOT/IR this session.

--------------------------------------------------------------------
## §4 Amendment ledger (what changed vs the DRAFT)

| Finding | Ruling | Action in this revision |
|---------|--------|-------------------------|
| F0 | WITHDRAWN; v3 a2-i reachability-exact `; cycle` | 13 intra-SCC edges marked `; cycle`; the old prose back-edge note removed |
| F1 | v3: header arity = all outputs incl. pivots | join headers now `(pivot, ...outs)` |
| F1b | v3 JOIN grammar (`.in<K>`, output-col order) | join bodies re-rendered by accessor order; producer-det_seq regroup dropped |
| F2 | v3 a3: no `producer=` in default dump | `producer=DEMAND-SEED` removed |
| F3 | v3: terminal INSERT, no attrs `table=` | insert.19 is a single terminal line |
| F4 (@cN) | REJECTED (fallback-only) | every `@cN` suffix removed |
| F4/Q5 (_MissingVar) | v3: `<var-or-cN>`, never `_MissingVar` | 6 nameless cols render `c<id>` |
| F5 | benign (symmetric shared-model) | unchanged (both views print `table=`) |
| F6 | benign (stratum correct, no offset) | unchanged |
| F7 | REJECTED (no prose header) | header is the single line `dataflow`; clause/meta comments removed |
| E-61 | kind order negations-before-compares | confirmed in-code; moot (no neg/cmp) |
| callers | v3: MERGE blocks only | `; callers:` kept only on merge.17/18 |
