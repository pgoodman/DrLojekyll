# T2 desired-state — `-df-out` of `tests/OptDiff/cases/transitive_closure.dr`

Hand-written EXACT desired text for the future `-df-out` DataFlow BB-with-args
dump of the non-linear transitive-closure case. Implementation later means
"make the compiler print exactly the §1 block, gated by strict byte-diff".

STATUS: REVISED 2026-07-18 against ratified t2-dump-spec v3 (post-E-61..E-66).
Was DRAFT-PENDING-REVISION, verdicted UNSOUND by
`critique-df-transitive_closure.md` (tc-critic F1/F2 CRITICAL: block params were
rendered with the caller's names, not the view's OWN finalized columns). This
revision applies EVERY enumerated finding and re-verifies against the frozen
tip-binary dumps regenerated this session.

AMENDMENTS APPLIED IN THIS REVISION (v3 rules win all conflicts):
- F1/F2/F3 (CRITICAL/MED): block params = the view's OWN finalized columns;
  caller renames appear ONLY in `=>` `dst=src` maps (dst=user token,
  src=producer token). `^tuple.2` header is now `(AutoVar_2:u64, Node:u64)`,
  `^tuple.3` is `(From:u64, X:u64)` — the merge.11 `=>` lines carry the renames.
- F4 + spec §1.3 JOIN GRAMMAR (E-61): the join block now renders `.in<K>`
  (by joined_views position) / `pivot` / `out` in output-column-position
  accessor order; NO `lhs`/`rhs` role labels; header arity = ALL output
  columns including the projected pivot `(X, From, To)`.
- Cycle marker (decision a2-i): `; back-edge` (the withdrawn det_seq-comparison
  rule) is GONE. `; cycle` now marks exactly the edges whose DEF (the producer
  emitting the `=>`) is REACHABLE FROM the USER (target) — i.e. every edge
  internal to the tc SCC {merge.11, tuple.2, tuple.3, join.10, tuple.1}, and
  nothing else. Six edges, computed in §2.3.
- Kind order (E-61): ForEachView pushes NEGATIONS before COMPARES; the kind
  sequence in §2.1 is corrected to
  selects, tuples, kv_indices, joins, maps, aggregates, merges, negations,
  compares, inserts. (This case has neither negations nor compares, so det_seq
  is unaffected — the sequence text is corrected regardless.)
- Header line is exactly `dataflow` (tc-critic F8): the invented
  `;; module: ...` clause block is REMOVED — clause text is not reliably
  recoverable post-optimize and is not emitter output.
- `; callers:` ONLY on MERGE blocks (spec §1.3, ascending det_seq): removed
  from every non-merge block (was F6, an over-extension).
- No `producer=` anywhere (decision a3): already absent; confirmed correct
  (this case is compiled without `-demand`).
- INSERT header is terminal `insert ^insert.<id> (<producer col tokens>) into
  %table:<TableId>`, NO ATTRIBUTES table= line; class=/stratum= stay.
- Column token `<var-or-cN>:<type>`, no `@cN` suffix; cN only as a fallback
  (none needed here — every finalized column has a variable name).

--------------------------------------------------------------------
## §0 Provenance

INPUTS READ THIS SESSION (revision pass, all at branch keyed-instances tip
63c8443c — the frozen witness binary):
- SPEC (binding): `docs/proposals/KeyedInstances.artifacts/t2-dump-spec.md`
  §1 (T2a `-df-out`) — v3, RATIFIED 2026-07-18. Read END TO END. Where an
  earlier draft/critique conflicts, v3 rules.
- The committed critique this revision discharges:
  `docs/proposals/KeyedInstances.artifacts/t2-desired-states/critique-df-transitive_closure.md`.
- GROUND TRUTH of the post-optimize graph, regenerated this session from the
  FROZEN tip binary (`baseline-bin/drlojekyll.debug.63c8443c`) into the session
  scratchpad `ground/`:
  - GraphViz: `ground/tc.dot`
  - ControlFlow IR: `ground/tc.ir`
  - source: `tests/OptDiff/cases/transitive_closure.dr`
- id-rule code re-read this session:
  - `lib/DataFlow/Query.h:1176-1214` — `QueryImpl::ForEachView` (the per-kind
    DefList iteration order — the det_seq / FinalizeColumnIDs traversal; E-61:
    negations pushed BEFORE compares).
  - `lib/DataFlow/Columns.cpp:13-25` — `FinalizeColumnIDs` (col ids assigned
    in ForEachView order, `next_col_id` from 1, sequential per view/column).
  - `lib/DataFlow/Query.cpp:810-850` — `QueryJoin::NthInputPivotSet` /
    `NthInputMergedColumn` (the output-column-position accessor order the
    JOIN grammar renders in).
  - `lib/DataFlow/Insert.cpp:24-34` — `KindName`: MATERIALIZE = query INSERT.

DECISIONS this desired-state COMMITS TO (per v3): (a) block id = det_seq,
rendered `^<kind>.<det_seq>`, blocks ascending det_seq; (a2-i) reachability-exact
`; cycle`; (a3) no `producer=`; (b) drain post-Program (so `table=%table:N` is
populated); §1.3 column naming `<var-or-cN>:<type>` with N = finalized column id;
JOIN grammar `.in<K>`/pivot/out; INSERT terminal header.

--------------------------------------------------------------------
AMENDED 2026-07-19 (round-2 adjudication + v3.2 grammar unification,
ledger §10, errata E-70/E-71): §1 re-rendered under the session-pinned
emitter rulings (p5)-(p9) and the round-2 consolidator's C-TC-1 —
line 1 of the block: `recv ... () -> (...)` header re-rendered to the
(p1) form (`select`, no arrow, provenance in the trailing comment, p7
spelling `; recv #message add_edge/2`); JOIN-body/INSERT lhs column
tokens now TYPED (p9/spec col-token definition); every trailing
comment re-padded to the p6 column (`;` at byte 52). GRAPH FACTS
(det_seq ids, strata, classes, tables, edge structure, cycle set)
UNCHANGED. Pre-amendment renderings quoted in derivation prose below
are HISTORICAL.

## §1 THE DESIRED `-df-out` TEXT (complete — nothing elided)

```
dataflow

select ^select.0 (From:u64, To:u64)                ; recv #message add_edge/2
  ATTRIBUTES class=table-less stratum=0
  => ^tuple.9 (From, To)

tuple ^tuple.1 (From:u64, To:u64)
  ATTRIBUTES class=table-less stratum=2
  => ^merge.11 (From, To)                          ; cycle

tuple ^tuple.2 (AutoVar_2:u64, Node:u64)
  ATTRIBUTES table=%table:4 class=monotone stratum=2
  => ^join.10 .in1(AutoVar_2, Node)                ; cycle

tuple ^tuple.3 (From:u64, X:u64)
  ATTRIBUTES table=%table:4 class=monotone stratum=2
  => ^join.10 .in0(X, From)                        ; cycle

tuple ^tuple.4 (Node:u64)
  ATTRIBUTES class=table-less stratum=3
  => ^merge.12 (Node)

tuple ^tuple.5 (Node:u64)
  ATTRIBUTES class=table-less stratum=4
  => ^merge.12 (Node)

tuple ^tuple.6 (From:u64, To:u64)
  ATTRIBUTES class=table-less stratum=5
  => ^insert.13 (From, To)

tuple ^tuple.7 (From:u64, To:u64)
  ATTRIBUTES class=table-less stratum=6
  => ^insert.14 (From, To)

tuple ^tuple.8 (Node:u64)
  ATTRIBUTES class=table-less stratum=8
  => ^insert.15 (Node)

tuple ^tuple.9 (From:u64, To:u64)
  ATTRIBUTES class=table-less stratum=1
  => ^merge.11 (From, To)

join ^join.10 (X:u64, From:u64, To:u64) {
  pivot X:u64 <- .in0.X, .in1.AutoVar_2
  out From:u64 <- .in0.From
  out To:u64 <- .in1.Node
}
  ATTRIBUTES class=table-less stratum=2
  => ^tuple.1 (From, To)                           ; cycle

merge ^merge.11 (From:u64, To:u64)                 ; callers: ^tuple.1, ^tuple.9
  ATTRIBUTES table=%table:4 class=monotone stratum=2 set=0 depth=1
  => ^tuple.2 (AutoVar_2=From, Node=To)            ; cycle
  => ^tuple.3 (From, X=To)                         ; cycle
  => ^tuple.4 (Node=From)
  => ^tuple.5 (Node=To)
  => ^tuple.6 (From, To)
  => ^tuple.7 (From, To)

merge ^merge.12 (Node:u64)                         ; callers: ^tuple.4, ^tuple.5
  ATTRIBUTES class=table-less stratum=7
  => ^tuple.8 (Node)

insert ^insert.13 (From:u64, To:u64) into %table:4
  ATTRIBUTES class=monotone stratum=9

insert ^insert.14 (From:u64, To:u64) into %table:4
  ATTRIBUTES class=monotone stratum=10

insert ^insert.15 (Node:u64) into %table:8
  ATTRIBUTES class=monotone stratum=11
```

--------------------------------------------------------------------
## §2 Derivation notes

### 2.1 The det_seq (block-id) ordering rule — how it was applied

Block id = `det_seq` = a view's position in `ForEachView` order at the last
stamp (spec §1.2, decision (a)). `ForEachView` (Query.h:1176-1214) iterates the
per-kind DefLists in THIS FIXED KIND SEQUENCE (E-61 — negations BEFORE compares,
the code pushes negations at :1196-1204 / :1248-1258):

    selects → tuples → kv_indices → joins → maps → aggregates
           → merges → negations → compares → inserts

skipping `is_dead` views. Within a kind, order = DefList insertion (creation)
order. det_seq is the flat 0-based counter across that whole traversal. This
case has NO kv_indices/maps/aggregates/negations/compares, so the negation/
compare transposition does not move any id here — the sequence text is
corrected regardless.

FIRMNESS PROOF (independent of guessing creation order):
`FinalizeColumnIDs` (Columns.cpp:13-25) assigns `col->id` from `next_col_id=1`,
sequentially per view per column, IN THE SAME `ForEachView` traversal. So the
finalized column ids VISIBLE IN THE .dot ports ARE a direct readout of det_seq
order (both view order AND intra-view column order). The observed col ids
c1..c23 rise monotonically exactly along the kind sequence above — so det_seq
0..12 is FIRM from the .dot, not predicted. Only the three INSERTs (no output
columns → consume no col id) are relatively ordered by prediction.

Kinds present here: SELECT×1, TUPLE×9, JOIN×1, MERGE×2, INSERT×3 = 16 views.
(No kv_indices / maps / aggregates / negations / compares.)

det_seq assignment (col-id evidence in brackets):
  0  ^select.0   RECV add_edge          [out cols c1=From, c2=To]        FIRM
  1  ^tuple.1    (join result feeder)   [c3=From, c4=To]                 FIRM
  2  ^tuple.2    (tc read, TABLE 4)     [c5=AutoVar_2, c6=Node]          FIRM
  3  ^tuple.3    (tc read, TABLE 4)     [c7=From, c8=X]                  FIRM
  4  ^tuple.4    (is_node From arm)     [c9=Node]                        FIRM
  5  ^tuple.5    (is_node To arm)       [c10=Node]                       FIRM
  6  ^tuple.6    (reachable_from arm)   [c11=From, c12=To]               FIRM
  7  ^tuple.7    (reaching_to arm)      [c13=From, c14=To]               FIRM
  8  ^tuple.8    (is_node union feeder) [c15=Node]                       FIRM
  9  ^tuple.9    (add_edge arm of tc)   [c16=From, c17=To]               FIRM
  10 ^join.10    (tc ⋈ tc)              [c18=X pivot, c19=From, c20=To]  FIRM
  11 ^merge.11   UNION tc (TABLE 4)     [c21=From, c22=To]               FIRM
  12 ^merge.12   UNION is_node          [c23=Node]                       FIRM
  13 ^insert.13  MATERIALIZE reachable_from   [no out cols]  ILLUSTRATIVE-PREDICTED
  14 ^insert.14  MATERIALIZE reaching_to      [no out cols]  ILLUSTRATIVE-PREDICTED
  15 ^insert.15  MATERIALIZE is_node          [no out cols]  ILLUSTRATIVE-PREDICTED

INSERT relative order (13<14<15) is predicted from (i) source clause order
(reachable_from, reaching_to, is_node — .dr:6,7,12-15) and (ii) the strata
STRATUM 9 < 10 < 11 on those MATERIALIZE nodes. Firm that all three inserts
come AFTER both merges (kind sequence); the internal order is the only genuinely
uncertain id assignment in the whole dump.

### 2.2 Block-by-block mapping to .dot evidence

NOTE (revision): the DOT was regenerated this session from the frozen tip
binary into `ground/tc.dot`; the raw pointer aliases and absolute line numbers
below are from the ORIGINAL draft's dump and differ from the regenerated file,
but the load-bearing facts — the finalized col ids c1..c23, strata, and TABLE
annotations — are BYTE-IDENTICAL to `ground/tc.dot` (re-verified: select.0
c1=From/c2=To; tuple.2 c5=AutoVar_2/c6=Node; tuple.3 c7=From/c8=X; join.10
c18=X(pivot)/c19=From/c20=To with ports p0=X p1=AutoVar_2 p2=From p3=Node;
merge.11 c21=From/c22=To TABLE 4 SET 0 DEPTH 1; is_node MATERIALIZE TABLE 8).
Aliases anchor the evidence quote only (names are ignored per the brief).
All columns u64 (.ir table %table:4[u64,u64] %col:5=From %col:6=To/X/Node;
%table:8[u64] %col:9=Node — `ground/tc.ir:9-18`).

^select.0  RECV — DOT line 12:
  `v4391463104 [... STRATUM 0 ... RECEIVE c1=From c2=To]`; line 11
  `v4391463104 -> t4391462912` is the message I/O association (add_edge I/O
  table t4391462912, DOT:10). KindName RECEIVE = a SELECT over the message
  stream (Insert.cpp cousin; a receive SELECT is table-less). stratum=0.
  User edge: DOT:48-49 `v30555638400:p0->v4391463104:c1`, `:p1->:c2` — i.e.
  ^tuple.9 reads RECV.(From,To). Forward: `=> ^tuple.9 (From, To)`.

^tuple.1  join-result feeder — DOT:26
  `v4391459360 [STRATUM 2 EQ SET 2 TUPLE c3=From c4=To]`. Reads JOIN:
  DOT:27-28 `v4391459360:p0->v4391460368:c19`, `:p1->:c20`. So its caller
  (producer) is ^join.10 (From c19, To c20). It is a member of UNION tc:
  DOT:51 `v4391459808 -> v4391459360`. Forward: `=> ^merge.11 (From, To)`.
  No TABLE annotation → class=table-less. stratum=2.

^tuple.2  tc read (the .in1 side of the join) — DOT:29
  `v4391456096 [TABLE 4 STRATUM 2 EQ SET 12 TUPLE c5=AutoVar_2 c6=Node]`.
  Its OWN finalized columns are (AutoVar_2, Node) — so its BLOCK HEADER is
  `(AutoVar_2:u64, Node:u64)` (F1 fix; NOT From/To). Reads UNION tc:
  DOT:30-31 `:p0->v4391459808:c21`, `:p1->:c22` — its input ports From/To read
  merge.11.From/To, and it RENAMES them to its outputs AutoVar_2/Node. That
  rename is recorded on the PRODUCER (merge.11) `=>` line as `dst=src`:
  `=> ^tuple.2 (AutoVar_2=From, Node=To)`. Feeds the JOIN input labeled
  `.in1`: DOT:15,17 `v4391460368:p1->v4391456096:c5` (AutoVar_2→join p1),
  `:p3->:c6` (Node→join p3). TABLE 4 → class=monotone, table=%table:4.
  stratum=2.

^tuple.3  tc read (the .in0 side of the join) — DOT:32
  `v30555635712 [TABLE 4 STRATUM 2 EQ SET 12 TUPLE c7=From c8=X]`. Its OWN
  finalized columns are (From, X) — BLOCK HEADER `(From:u64, X:u64)` (F1 fix).
  Reads UNION tc: DOT:33-34 — input ports From/To read merge.11.From/To,
  renaming To→X (From stays From). Recorded on merge.11's `=>`:
  `=> ^tuple.3 (From, X=To)` (From identity = bare token; X=To the rename).
  Feeds JOIN input `.in0`: DOT:14,16 `:p0->v30555635712:c8` (X→join p0),
  `:p2->:c7` (From→join p2). TABLE 4 → monotone. stratum=2.

^tuple.4  is_node "From" arm — DOT:35
  `v30555637056 [STRATUM 3 EQ SET 5 TUPLE c9=Node]`. Single input port p0
  reads UNION tc From: DOT:36 `:p0->v4391459808:c21`. So it RENAMES tc.From
  → Node (the `is_node(Node):tc(Node,_)` clause, .dr:14). Member of UNION
  is_node: DOT:54 `v30543022080->v30555637056`. table-less. stratum=3.
  Incoming rename recorded at the caller edge `(Node=From)`.

^tuple.5  is_node "To" arm — DOT:37
  `v30555637504 [STRATUM 4 EQ SET 6 TUPLE c10=Node]`. Port p0 reads UNION tc
  To: DOT:38 `:p0->v4391459808:c22`. Renames tc.To → Node (`is_node(Node):
  tc(_,Node)`, .dr:15). Member of UNION is_node: DOT:55. table-less.
  stratum=4.

^tuple.6  reachable_from arm — DOT:39
  `v30555636608 [STRATUM 5 EQ SET 12 TUPLE c11=From c12=To]`. Reads UNION tc:
  DOT:40-41. Feeds INSERT reachable_from: DOT:19-20
  `v30543020544:c0->v30555636608:c11`, `:c1->:c12`. table-less. stratum=5.

^tuple.7  reaching_to arm — DOT:42
  `v30555636160 [STRATUM 6 EQ SET 12 TUPLE c13=From c14=To]`. Reads UNION tc:
  DOT:43-44. Feeds INSERT reaching_to: DOT:22-23. table-less. stratum=6.

^tuple.8  is_node union feeder — DOT:45
  `v30555637952 [STRATUM 8 EQ SET 9 TUPLE c15=Node]`. Port p0 reads UNION
  is_node: DOT:46 `:p0->v30543022080:c23`. Feeds INSERT is_node: DOT:25
  `v30543022592:c0->v30555637952:c15`. table-less. stratum=8.

^tuple.9  add_edge arm of tc — DOT:47
  `v30555638400 [STRATUM 1 EQ SET 1 TUPLE c16=From c17=To]`. Reads RECV:
  DOT:48-49. Member of UNION tc: DOT:52 `v4391459808->v30555638400`.
  table-less. stratum=1. (The `tc(From,To):add_edge(From,To)` clause,
  .dr:10.)

^join.10  the tc ⋈ tc self-join — DOT:13
  `v4391460368 [STRATUM 2 EQ SET 11 pivot c18=X, out c19=From c20=To;
  ports p0=X p1=AutoVar_2 p2=From p3=Node]`. Port producers (DOT:14-17):
  p0(X)←tuple.3.c8, p1(AutoVar_2)←tuple.2.c5, p2(From)←tuple.3.c7,
  p3(Node)←tuple.2.c6.
  v3 JOIN GRAMMAR (spec §1.3, E-61) — NO lhs/rhs labels; inputs are `.in<K>`
  by joined_views position; the block renders in OUTPUT-COLUMN-POSITION
  accessor order (Query.cpp:822-844): pivot columns first (num_pivots), then
  merged out columns. HEADER ARITY = ALL output columns including the projected
  pivot → `(X:u64, From:u64, To:u64)`.
    - pivot X: NthInputPivotSet(0) ties the two input columns matched on X.
      In use-list order these are `.in0.X` (tuple.3.c8=X, DOT p0) and
      `.in1.AutoVar_2` (tuple.2.c5=AutoVar_2, DOT p1). → `pivot X <- .in0.X,
      .in1.AutoVar_2`.
    - out From: NthInputMergedColumn ← tuple.3.c7=From (DOT p2) → `out From
      <- .in0.From`.
    - out To: ← tuple.2.c6=Node (DOT p3) → `out To <- .in1.Node`.
  `.in0`=tuple.3, `.in1`=tuple.2 by joined_views position (the pivot-set
  use-list order p0-before-p1) — PREDICTED, not col-id-witnessed; see §3 R2.
  Feeds ^tuple.1: DOT:27-28 (join.From→tuple.1.From,
  join.To→tuple.1.To). table-less (no TABLE annotation). stratum=2.

^merge.11  UNION tc — DOT:50
  `v4391459808 [TABLE 4 SET 0 DEPTH 1 STRATUM 2 EQ SET 12 UNION "tc"
  c21=From c22=To]`. Members (DOT:51-52): ^tuple.1, ^tuple.9 (the two tc
  clause bodies). Users (who READ tc): ^tuple.2, ^tuple.3, ^tuple.4,
  ^tuple.5, ^tuple.6, ^tuple.7 (DOT:30,33,36,38,40,43 all target c21/c22).
  TABLE 4 → monotone, table=%table:4; the `SET 0 DEPTH 1` from the DOT
  becomes `set=0 depth=1` (the induction-set watermark). stratum=2.

^merge.12  UNION is_node — DOT:53
  `v30543022080 [STRATUM 7 EQ SET 9 UNION "is_node" c23=Node]`. Members
  (DOT:54-55): ^tuple.4, ^tuple.5. User: ^tuple.8 (DOT:46). No TABLE →
  table-less. stratum=7.

^insert.13/14/15  the three MATERIALIZE sinks — DOT:18,21,24
  `v30543020544 [TABLE 4 STRATUM 9 EQ SET 12 MATERIALIZE reachable_from
  c0=From c1=To]`; `v30543021568 [TABLE 4 STRATUM 10 ... reaching_to]`;
  `v30543022592 [TABLE 8 STRATUM 11 EQ SET 9 ... is_node Node]`. Relation
  tables point AT the inserts (DOT:4-9). INSERTs are TERMINAL (no output
  columns, no `=>`). v3 INSERT HEADER (spec §1.3): `insert ^insert.<id>
  (<producer col tokens>) into %table:<TableId>` — the tokens are the
  PRODUCER's columns in input-position order (tuple.6=(From,To),
  tuple.7=(From,To), tuple.8=(Node)); NO ATTRIBUTES `table=` line (redundant
  with the header). NOTE (faithful): reachable_from and reaching_to BOTH
  MATERIALIZE into %table:4 (they share tc's backing table, DOT TABLE 4 on
  both) — so insert.13 and insert.14 are distinct INSERT views into the SAME
  table id. is_node → %table:8. class=monotone on all three; strata 9/10/11
  stay on the ATTRIBUTES line. Real table ids ≥ 4 here (constant-var ids 0/1/2
  precede any table Create — `ground/tc.ir:1-9`).

### 2.3 `; cycle` marks (v3 decision a2-i: reachability-exact)

The withdrawn v1 rule (`; back-edge` when USER det_seq <= DEF det_seq) is GONE
— it over-fired (flagged merge.11=>tuple.6/7, which EXIT the cycle) and is not
what v3 emits. v3 marks `; cycle` iff the edge's DEF (the producer emitting the
`=>` line) is REACHABLE FROM the USER (target), following dataflow producer→user
edges — i.e. the edge closes a cycle. That is exactly every edge INTERNAL to a
strongly-connected component.

The one SCC here is the tc fixpoint {merge.11, tuple.2, tuple.3, join.10,
tuple.1} (DOT SET 0 DEPTH 1). Its internal edges — all 6 marked:
  ^tuple.1  => ^merge.11 : is tuple.1 reachable from merge.11?
                           merge.11→tuple.2→join.10→tuple.1 → YES → cycle
  ^tuple.2  => ^join.10  : join.10→tuple.1→merge.11→tuple.2       → YES → cycle
  ^tuple.3  => ^join.10  : join.10→tuple.1→merge.11→tuple.3       → YES → cycle
  ^join.10  => ^tuple.1  : tuple.1→merge.11→tuple.2→join.10       → YES → cycle
  ^merge.11 => ^tuple.2  : tuple.2→join.10→tuple.1→merge.11       → YES → cycle
  ^merge.11 => ^tuple.3  : tuple.3→join.10→tuple.1→merge.11       → YES → cycle
Every OTHER edge is NOT marked (its def is not reachable from its user — the
target only flows OUT to sinks / other strata):
  select.0=>tuple.9, tuple.9=>merge.11, merge.11=>tuple.4/5/6/7,
  tuple.4=>merge.12, tuple.5=>merge.12, merge.12=>tuple.8,
  tuple.6=>insert.13, tuple.7=>insert.14, tuple.8=>insert.15.
Note this is the crisp improvement over the old rule: the cycle mark now lands
on EXACTLY the SCC and nothing else — the query-sink exits (merge.11=>tuple.6/7)
and the is_node sub-DAG are correctly UNmarked.

### 2.4 `=>` edge ordering (spec §1.3: by (user det_seq, port))

Within each block the `=>` lines are sorted by (user det_seq, then user port).
merge.11's six users sort tuple.2,3,4,5,6,7 — already ascending det_seq, one
line each (a two-column feed is one line with the column list). This is why
merge.11 lists its edges 2→3→4→5→6→7.

### 2.5 Firmness summary
  FIRM (col-id-proven, re-verified against `ground/tc.dot` this revision):
    all det_seq 0..12; every column name/id INCLUDING tuple.2=(AutoVar_2,Node)
    and tuple.3=(From,X) (F1 fix); every edge src/dst and its `dst=src` map
    (read straight from .dot ports); every TABLE annotation; every stratum;
    every `; cycle` mark (reachability over firm edges); the JOIN pivot/out
    structure.
  ILLUSTRATIVE-PREDICTED / RESIDUAL: insert.13/14/15 relative ids (ordered by
    clause + stratum, no col-id witness — §3 R1); the JOIN `.in0`/`.in1`
    producer-identity assignment (joined_views UseList position — not read out
    by any col-id/DOT witness — §3 R2, PREDICTED per spec pin (p3)); the
    recv-block leading keyword (`recv` vs the `select` kind — §3 R3); the exact
    punctuation of headers/ATTRIBUTES and `set=0 depth=1` (spec-shape).

--------------------------------------------------------------------
## §3 Open questions / spec frictions (LOUD — feed the critique round)

RESOLVED BY v3 (the drafts' frictions the ratified spec closed — kept as a
paper trail, no longer open):
- (old F3/F4, tc-critic F1/F2/F3 CRITICAL) block-param rename rule — v3 §1.3
  RESOLVES to "block params = the view's OWN finalized columns; renames only in
  `=>` dst=src". APPLIED: tuple.2=(AutoVar_2,Node), tuple.3=(From,X), the
  renames on merge.11's `=>` lines.
- (old F3) JOIN lhs/rhs labels — v3 §1.3 REMOVES role labels; the grammar is
  `.in<K>`/pivot/out in output-column-position accessor order, header arity =
  all output columns. APPLIED.
- (old F2, a3) `producer=` — v3 DROPS it from the default dump (config-
  invariance). This case carries none anyway (no `-demand`). APPLIED (absent).
- (old F6) `; callers:` on non-merge blocks — v3 restricts `; callers:` to
  MERGE blocks only, ascending det_seq. APPLIED (removed from all tuples/recv/
  join/insert; kept on merge.11 and merge.12).
- (old F8) `;; module:` clause header — v3 §1.3 makes the header exactly the
  single line `dataflow`. APPLIED (clause block removed).
- (old F5) class=monotone despite recursion — v3 §1.3 PINS class off
  deletion-capability, "RECURSION DOES NOT IMPLY DIFFERENTIAL". APPLIED and
  re-verified against `ground/tc.ir` (only +recursive/+nonrecursive, no
  overdelete/rederive → monotone).
- (old F9) back-edge over-report — the whole `; back-edge` rule is WITHDRAWN;
  v3's reachability-exact `; cycle` marks exactly the SCC (§2.3). APPLIED.
- (old F7) `set=0 depth=1` — spec §1.3 ATTRIBUTES list includes, for induction
  members, `set=<merge_set_id> depth=<InductionDepth>`. KEPT on merge.11.

GENUINE RESIDUALS (LOUD — surviving after v3):

R1. INSERT det_seq ids 13/14/15 remain UNWITNESSED. INSERTs own no output
    columns, so FinalizeColumnIDs assigns no col id — the col-id monotonicity
    proof that firms det_seq 0..12 does not reach them. Predicted 13<14<15 from
    (i) source clause order and (ii) strata 9<10<11. FIRM that all three come
    AFTER both merges (kind order: inserts last). The internal order is the only
    genuinely uncertain id assignment in the dump. FIRST BLESS must be reviewed
    against a real det_seq print; if the inserts' DefList order differs, only the
    three insert ids AND the `=> ^insert.N` targets on tuple.6/7/8 move together.

R2. PRODUCER-SIDE `=>` INTO A JOIN. Presence and token form are now SANCTIONED
    by spec pin (p3) (v3.1 session-pinned emitter rulings): a producer block
    DOES emit its `=> ^join.<id> .in<K> (...)` line (tail-call completeness),
    rendering the producer's OWN tokens — bare where identity, per pin (p2)
    (§1 now renders `.in1(AutoVar_2, Node)` / `.in0(X, From)`, not the earlier
    `dst=src` self-maps). What remains PREDICTED, per pin (p3)'s own text
    ("artifacts carry it as PREDICTED until the first bless code-reads it"),
    is the `.in<K>` ASSIGNMENT ITSELF — which input tuple is `.in0` vs `.in1`
    is joined_views UseList position, a separate use-list not read out by any
    col-id/DOT witness available to this artifact. I assigned `.in0`=tuple.3,
    `.in1`=tuple.2 from the pivot-set use-list order (p0-before-p1) as the best
    available heuristic; FIRST BLESS must read joined_views order in code (or
    diff against a real emitter run) — if the assignment swaps, the two
    producer-side `=>` lines' `.in<K>` labels and the join block's own
    `.in0`/`.in1` labels move together (a cmp -s break otherwise).

R3. RECV LEADING KEYWORD. Block id is `^select.0` (kind = select; a RECEIVE is
    a SELECT over the message stream). The leading keyword rendered is `recv`
    (descriptive) while the id keeps the `select` kind. v3 does not pin the
    recv-block leading token; the emitter may use `select` uniformly. If it does,
    the line becomes `select ^select.0 () -> (From:u64, To:u64)`. Cosmetic but
    byte-affecting — pin before bless.

R4. TABLE-ID EYEBALL (spec §1.4 A1 rider). This golden asserts %table:4 (shared
    by tc / reachable_from / reaching_to) and %table:8 (is_node), read from
    `ground/tc.ir:9,17`. The bijection witness guards view NUMBERING only, not
    table-id population — before bless, EYEBALL a real `%table:<id>` in the live
    emitter output (a mis-drained emitter prints empty ids deterministically).

R5. HEADER/ATTRIBUTES PUNCTUATION is spec-shape, not graph-derived: the exact
    spacing/ordering of the ATTRIBUTES line, the `-> (…)` recv arrow, the
    `{ … }` join-block layout, and `set=0 depth=1` spelling are all illustrative
    of the intended form; the load-bearing content (ids, tokens, edges, marks)
    is the contract.
