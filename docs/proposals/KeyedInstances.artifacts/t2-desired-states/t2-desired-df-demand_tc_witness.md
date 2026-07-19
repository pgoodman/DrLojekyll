# T2a desired `-df-out` — `demand_tc_witness.dr` (demand-ON)

DESIRED-STATE artifact. This is the EXACT text the future `-df-out` emitter
SHOULD print for `tests/OptDiff/cases/demand_tc_witness.dr` compiled WITH
`-demand` (its `.drflags` sidecar). Implementation later means: make the
compiler print §1 byte-for-byte; gate by diff (T3 `df opt` golden sidecar).

--------------------------------------------------------------------
## §0 Provenance

- Branch/tip: `keyed-instances` @ `b577735ef9202e26e6e21959e6a3553c445083b5`
  (frozen binary; `git rev-parse HEAD` confirmed this session).
- Spec (BINDING): `docs/proposals/KeyedInstances.artifacts/t2-dump-spec.md`
  §1 (T2a) — STATUS DRAFT pending owner ratification of decisions (a)-(d).
  Block-id decision (a) = `det_seq` rendered `^<kind>.<det_seq>`; ATTRIBUTES
  = `table=%table:<TableId()>`, `class=`, `stratum=<Stratum()>`, `producer=`
  debug-only; `=>` lines ordered `(user det_seq, port)`, args as `dst=src`,
  `; back-edge` comment when user det_seq <= def det_seq.
- Form refined: `docs/proposals/KeyedInstances.artifacts/ir-dump-formats.md`
  §1 (BB-with-arguments; one block per view, `=>` per user column-edge,
  MERGE = block-args φ join, JOIN declares ports, kind headers).
- Verified as-landed record (id rules, no-FinalizeViewIDs, stamp sites):
  `scratchpad/fleet-ckpt/consolidated.md` §1.A, §1.B, §3, §4.a.
- GROUND TRUTH graph (post-optimize, generated `-demand` from the b577735e
  binary this session):
  - `scratchpad/dump-inputs/demand_tc_witness.dot` — every view kind, column,
    variable name, STRATUM, EQ SET, TABLE id, and every column edge.
  - `scratchpad/dump-inputs/demand_tc_witness.ir` — table/col/index id spaces.
  - `tests/OptDiff/cases/demand_tc_witness.dr` — the source.
- Code re-read this session for the id rule:
  - `lib/DataFlow/Query.h:1176-1214` — `ForEachView` per-kind DefList order.
  - `lib/DataFlow/Columns.cpp:13-25` — `FinalizeColumnIDs` assigns col ids in
    ForEachView order, monotone (the load-bearing lever, see §2).
  - `lib/DataFlow/Insert.cpp:24-33` (MATERIALIZE = kQuery insert),
    `lib/DataFlow/Select.cpp:46-54` (RECEIVE = IO-stream select),
    `lib/DataFlow/Format.cpp:178-647` (DOT edge direction = use→def; ports).

--------------------------------------------------------------------
## §1 THE DESIRED `-df-out` TEXT

```
;; module: demand_tc_witness  (compiled with -demand)
;; path(F,T) : edge_2(F,T).
;; path(F,T) : path(F,M), edge_2(M,T).
;; #query reachable_from(bound From, free To) : path(From, To).
;;
;; block id = det_seq (ForEachView per-kind DefList order: select, tuple,
;;   kv_index, join, map, aggregate, merge, negate, compare, insert).
;; column names carry the FINALIZED col id (FinalizeColumnIDs); the demand
;; transform's fabricated columns show as _MissingVar (no source var).

recv ^select.0 () -> (M:u64@c1, T:u64@c2)         ; #message edge_2/2
  stratum=0 class=table-less
  => ^tuple.2 (F=M, T=T)
  => ^tuple.10 (M=M, T=T)

recv ^select.1 () -> (_MissingVar:u64@c3)          ; #message demand__reachable_from_bf/1 (fabricated; ABI-suppressed)
  stratum=1 class=table-less  producer=DEMAND-SEED
  => ^tuple.8 (_MissingVar=_MissingVar)
  => ^tuple.12 (_MissingVar=_MissingVar)

tuple ^tuple.2 (F:u64@c4, T:u64@c5)                ; callers: ^select.0
  table=%table:19 class=monotone stratum=2
  => ^join.14 .in0(F=F, T=T)

tuple ^tuple.3 (F:u64@c6, T:u64@c7)                ; callers: ^join.13
  class=table-less stratum=5
  => ^merge.17 (F=F, T=T)

tuple ^tuple.4 (From:u64@c8, To:u64@c9)            ; callers: ^merge.17
  table=%table:8 class=monotone stratum=5
  => ^join.15 .in0(From=From, To=To)
  => ^join.16 .in0(From=From, To=To)

tuple ^tuple.5 (From:u64@c10)                       ; callers: ^merge.17
  class=table-less stratum=5
  => ^merge.18 (_MissingVar=From)

tuple ^tuple.6 (_MissingVar:u64@c11)               ; callers: ^merge.18
  table=%table:12 class=monotone stratum=5
  => ^join.14 .in1(_MissingVar=_MissingVar)
  => ^join.15 .in1(_MissingVar=_MissingVar)

tuple ^tuple.7 (From:u64@c12, To:u64@c13)          ; callers: ^join.15
  table=%table:15 class=monotone stratum=5
  => ^join.13 .in0(From=From, To=To)

tuple ^tuple.8 (_MissingVar:u64@c14)               ; callers: ^select.1
  table=%table:23 class=monotone stratum=6
  => ^join.16 .in1(_MissingVar=_MissingVar)

tuple ^tuple.9 (From:u64@c15, To:u64@c16)          ; callers: ^join.16
  class=table-less stratum=8
  => ^insert.19 (From=From, To=To)

tuple ^tuple.10 (M:u64@c17, T:u64@c18)             ; callers: ^select.0
  table=%table:19 class=monotone stratum=4
  => ^join.13 .in1(M=M, T=T)

tuple ^tuple.11 (F:u64@c19, T:u64@c20)             ; callers: ^join.14
  class=table-less stratum=5
  => ^merge.17 (F=F, T=T)                          ; induction SET 0 back-edge (NOT flagged by the det_seq<=def rule; see §3 Q1)

tuple ^tuple.12 (_MissingVar:u64@c21)              ; callers: ^select.1
  class=table-less stratum=3
  => ^merge.18 (_MissingVar=_MissingVar)

join ^join.13 [pivot M:u64@c22] {                  ; d_path ⋈ path recursive-body SIP guard (interpretive; see §3 Q4)
    .in0 <- ^tuple.7  (pivot M<-To@c13, From@c12)   ; producer det_seq 7 (path image, %table:15)
    .in1 <- ^tuple.10 (pivot M<-M@c17, T@c18)       ; producer det_seq 10 (edge_2 image, %table:19)
  } -> (F:u64@c23, T:u64@c24)
  class=table-less stratum=5
  => ^tuple.3 (F=F, T=T)

join ^join.14 [pivot F:u64@c25] {
    .in0 <- ^tuple.2 (pivot F<-F@c4, T@c5)          ; producer det_seq 2 (edge_2 image, %table:19)
    .in1 <- ^tuple.6 (pivot F<-_MissingVar@c11)     ; producer det_seq 6 (demand relation d_path, %table:12)
  } -> (T:u64@c26)
  class=table-less stratum=5
  => ^tuple.11 (F=F, T=T)

join ^join.15 [pivot From:u64@c27] {               ; d_path ⋈ path SIP push-down guard (interpretive)
    .in0 <- ^tuple.4 (pivot From<-From@c8, To@c9)   ; producer det_seq 4 (path, %table:8)
    .in1 <- ^tuple.6 (pivot From<-_MissingVar@c11)  ; producer det_seq 6 (demand relation d_path, %table:12)
  } -> (To:u64@c28)
  class=table-less stratum=5
  => ^tuple.7 (From=From, To=To)

join ^join.16 [pivot From:u64@c29] {               ; d_seed ⋈ path query-projection guard (interpretive)
    .in0 <- ^tuple.4 (pivot From<-From@c8, To@c9)   ; producer det_seq 4 (path, %table:8)
    .in1 <- ^tuple.8 (pivot From<-_MissingVar@c14)  ; producer det_seq 8 (demand seed, %table:23)
  } -> (To:u64@c30)
  class=table-less stratum=7
  => ^tuple.9 (From=From, To=To)

merge ^merge.17 (F:u64@c31, T:u64@c32)             ; callers: ^tuple.3, ^tuple.11 ; relation path
  table=%table:8 class=monotone stratum=5
  => ^tuple.4 (From=F, To=T)
  => ^tuple.5 (From=F)

merge ^merge.18 (_MissingVar:u64@c33)              ; callers: ^tuple.5, ^tuple.12 ; demand relation d_path
  table=%table:12 class=monotone stratum=5
  => ^tuple.6 (_MissingVar=_MissingVar)

insert ^insert.19 (From=From@c15, To=To@c16) into %table:4   ; MATERIALIZE reachable_from ; terminal
  table=%table:4 class=monotone stratum=9
  ; reads ^tuple.9; no tail calls (query result sink)
```

--------------------------------------------------------------------
## §2 Derivation notes — every block mapped to its .dot evidence

### 2.1 The id rule (FIRM, derived this session — the key result)

`ForEachView` (lib/DataFlow/Query.h:1176-1214) iterates the per-kind DefLists
in this EXACT sequence: **selects → tuples → kv_indices → joins → maps →
aggregates → merges → negations → compares → inserts**. `det_seq` is stamped
dense 0..N-1 in this traversal (consolidated §1.A; last stamp
IdentifyInductions:144, run before FinalizeColumnIDs/Stratify which mint no
views). Block id = `^<kind>.<det_seq>`, blocks in ascending det_seq (spec §1.2,
decision (a)).

LEVER (lib/DataFlow/Columns.cpp:13-25): `FinalizeColumnIDs` assigns column ids
starting at 1, iterating **the same ForEachView traversal**, monotonically per
view. Therefore the finalized column-id order IS the det_seq order — a view
earlier in ForEachView gets strictly lower output column ids than every later
view. This makes the within-kind ordering FIRM, not illustrative: I read the
smallest output col id of each DOT node and it partitions cleanly into the kind
buckets and orders within them. The 20 live views:

| det_seq | block       | DOT node       | out cols  | evidence (DOT line) |
|---------|-------------|----------------|-----------|---------------------|
| 0  | ^select.0  | v4315097840   | c1,c2   | `RECEIVE ... c1 M, c2 T` (STRATUM 0 EQ SET 1) |
| 1  | ^select.1  | v4315107936   | c3      | `RECEIVE ... c3 _MissingVar` (STRATUM 1 EQ SET 2) |
| 2  | ^tuple.2   | v4315094736   | c4,c5   | `TABLE 19 ... TUPLE c4 F, c5 T` |
| 3  | ^tuple.3   | v4315093744   | c6,c7   | `TUPLE c6 F, c7 T` (STRATUM 5 EQ SET 4) |
| 4  | ^tuple.4   | v4315104480   | c8,c9   | `TABLE 8 ... TUPLE c8 From, c9 To` |
| 5  | ^tuple.5   | v34309505472  | c10     | `TUPLE c10 From` (STRATUM 5 EQ SET 6) |
| 6  | ^tuple.6   | v34309505920  | c11     | `TABLE 12 ... TUPLE c11 _MissingVar` |
| 7  | ^tuple.7   | v34309506816  | c12,c13 | `TABLE 15 ... TUPLE c12 From, c13 To` |
| 8  | ^tuple.8   | v34309507264  | c14     | `TABLE 23 ... TUPLE c14 _MissingVar` |
| 9  | ^tuple.9   | v34309505024  | c15,c16 | `TUPLE c15 From, c16 To` (STRATUM 8 EQ SET 10) |
| 10 | ^tuple.10  | v34309506368  | c17,c18 | `TABLE 19 ... TUPLE c17 M, c18 T` (STRATUM 4) |
| 11 | ^tuple.11  | v34309507712  | c19,c20 | `TUPLE c19 F, c20 T` (STRATUM 5 EQ SET 15) |
| 12 | ^tuple.12  | v34309508160  | c21     | `TUPLE c21 _MissingVar` (STRATUM 3 EQ SET 2) |
| 13 | ^join.13   | v4315098912   | c23,c24 (pivot c22) | `JOIN pivot c22 M, out c23 F, c24 T` |
| 14 | ^join.14   | v4315110336   | c26 (pivot c25)     | `JOIN pivot c25 F, out c26 T` |
| 15 | ^join.15   | v4315111424   | c28 (pivot c27)     | `JOIN pivot c27 From, out c28 To` |
| 16 | ^join.16   | v4315112736   | c30 (pivot c29)     | `JOIN pivot c29 From, out c30 To` (STRATUM 7) |
| 17 | ^merge.17  | v4315097328   | c31,c32 | `TABLE 8 ... UNION c31 F, c32 T ... path` |
| 18 | ^merge.18  | v4315109664   | c33     | `TABLE 12 ... UNION c33 _MissingVar` |
| 19 | ^insert.19 | v4315105888   | (none; ins c0,c1) | `TABLE 4 ... MATERIALIZE reachable_from c0 From, c1 To` |

FIRMNESS: block ids 0-19 are FIRM (the monotone col-id partition is a code
fact, not a guess). The only ILLUSTRATIVE hazard is the tie-break WITHIN a kind
when two views share no col-id ordering signal — but here every view has a
distinct, strictly increasing col-id range, so the whole numbering is FIRM.

### 2.2 Edge direction (DOT is use→def; `=>` is def→use)

Format.cpp:617-621 (INSERT), :643-646 (TUPLE ports), :339ff (JOIN) emit DOT
edges as `consumer:port -> producer:col` (a consumer's input port points at the
producing column). The spec's `=>` tail-call is the OPPOSITE (producer pushes
to user). So §1 REVERSES every DOT edge. The reversed producer→user map (with
`(user det_seq, port)` ordering per spec §1.3) was computed mechanically from
the 38 DOT edges; every `=>` line in §1 traces to exactly one DOT edge, e.g.:
- `^select.0 => ^tuple.2 (F=M,T=T)` ⇐ DOT `v4315094736:p0 -> v4315097840:c1`
  and `:p1 -> :c2` (tuple.2's inputs read select.0's c1/c2).
- `^merge.17 => ^tuple.4 (From=F,To=T)` ⇐ DOT `v4315104480:p0 -> v4315097328:c31`,
  `:p1 -> :c32`.
- `^tuple.11 => ^merge.17` ⇐ DOT `v4315097328 -> v34309507712` (the merge's
  un-ported def→use edge, reversed).

### 2.3 ATTRIBUTES (table/class/stratum)

- `table=%table:N`: present iff the DOT node carries a `TABLE N` cell. Six
  backed models: %table:4 (ds19), %table:8 (ds4 AND ds17 — shared model:
  tuple.4 is a view over the path union's table), %table:12 (ds6 AND ds18),
  %table:15 (ds7), %table:19 (ds2 AND ds10), %table:23 (ds8). All six ids
  cross-reference the .ir `create %table:N` blocks (lines 9,16,23,28,35,42).
- `class=`: ZERO purple (deletion) edges in the whole DOT (`grep color=purple`
  = 0) and every `.ir` update-count is `+recursive`/`+nonrecursive` with NO
  overdelete/rederive machinery — so every backed table is **monotone**;
  table-less views (no TABLE cell) render `class=table-less`. (This program has
  no negation/retraction; the demand transform introduces none here.)
- `stratum=N`: read verbatim from each DOT node's `STRATUM N` cell.

### 2.4 The demand machinery (what the fabricated graph shows)

- `^select.1` = the fabricated `demand__reachable_from_bf/1` receive (.ir:88
  `proc ^receive:demand__reachable_from_bf/1`; its ABI entry is registry-
  suppressed — no driver seam). Its single col c3 is `_MissingVar` (the demand
  transform mints a column with no source parse var).
- `^merge.18`/%table:12 = the demand relation `d_path` (single-col, the bound-
  column demand set); `^tuple.6` is its table view feeding the two push-down
  guard joins (join.14, join.15) on their pivot side.
- `^tuple.8`/%table:23 = the demand seed image; `^join.16` is the query-
  projection guard (d_seed ⋈ path) producing the reachable_from answer rows via
  tuple.9 → insert.19.
- `^join.13/14/15` are the SIP push-down guards (d_path ⋈ path / ⋈ edge_2). The
  precise "which join is which rule body" labels in §1 comments are
  INTERPRETIVE (see §3 Q4) — the STRUCTURE (producers, pivots, outputs) is
  DOT-firm; the English semantics are my reading of the demand recipe.

--------------------------------------------------------------------
## §3 Open questions / spec frictions (LOUD — for the critique round)

**Q1 — the `; back-edge` rule does NOT fire on the real inductive back-edge.**
Spec §1.3: emit `; back-edge` when `user det_seq <= def det_seq`. The genuine
inductive cycle here is `^merge.17(path) → ... → ^tuple.11 → ^merge.17` (DOT
marks SET 0 DEPTH 1 on ds13/14/17/18). But `^tuple.11(11) => ^merge.17(17)`
has user(17) > def(11), so the literal rule prints NOTHING, and NO other edge
in the graph satisfies user<=def either — so a strict spec implementation
emits ZERO back-edge comments on a program that manifestly has an induction.
The det_seq numbering happens to be near-topological across the cycle. RESULT:
the `<=` heuristic is unsound as a cycle marker. RECOMMEND: mark back-edges
from the actual induction membership (the `InductionGroupId`/SET-0 data the DOT
already prints as "SET 0 DEPTH 1"), NOT from a det_seq comparison. I annotated
tuple.11's `=>` with the TRUE back-edge note and did NOT emit any spec-rule
`; back-edge` comment, to surface the contradiction rather than hide it.

**Q2 — producer tags are debug-only AND absent from my ground truth.** Spec
§1.3 (E-52): print `producer=<tag>` when the field is non-empty (debug builds
only); T3 goldens blessed from the DEBUG preset make the tag golden-visible;
RELEASE prints no producer line. The DOT does NOT render producer tags, so I
have NO ground-truth values — I wrote `producer=DEMAND-SEED` on `^select.1`
speculatively and left it off every other demand-minted view. FRICTION: (a) I
cannot author the exact producer strings without reading Demand.cpp's tag-set
constants; (b) more importantly, a DEBUG-vs-RELEASE dump-text divergence means
the `df opt` golden is only stable if the suite always compiles debug — which
it does (consolidated confirms the debug compiler runs the suite), but this
couples the golden to the preset. RECOMMEND: either (i) drop producer from the
dump entirely (determinism > annotation), or (ii) make producer ALWAYS-emitted
(promote the field out of `#ifndef NDEBUG`) so debug and release agree. As
written, §1 is a DEBUG-preset golden; a release `-df-out` would differ on the
`producer=` line — a latent golden-stability trap the spec should resolve
before T3 blesses this surface.

**Q3 — MATERIALIZE/INSERT column syntax has no output columns.** An INSERT view
owns NO finalized output columns (Insert.cpp: terminal, produces none); its DOT
ports c0/c1 are input-column INDICES that reuse producer col ids (here c15/c16
from tuple.9, which collide numerically with select.0's c1/c2 — different id
space). I rendered `insert ^insert.19 (From=From@c15, To=To@c16) into
%table:4` reading tuple.9, with no `=>`. The spec §1.3/ir-dump §1 example shows
`insert ^insert.12 (F,T) into %tc` but does not pin whether the paren list is
the INPUT columns (my choice) or elided. FRICTION: pin the INSERT block's
column-list semantics (input cols vs none) and confirm the `into %table:N`
suffix. Also: should INSERT carry an ATTRIBUTES `table=` when the same id is in
the `into` clause? I emitted both (`into %table:4` on the header AND
`table=%table:4` on the attrs line) — possibly redundant; spec should pick one.

**Q4 — JOIN port identity/order is under-specified and possibly non-det.** The
spec/ir-dump form declares JOIN `.lhs/.rhs` ports, but (a) join.13 has FOUR
input ports from TWO producers (interleaved p0/p2=tuple.7, p1/p3=tuple.10) —
"lhs/rhs" is a 2-way idiom that doesn't obviously generalize; (b) the impl's
join input iteration order (`joined_views`) may be a DefList whose order is NOT
guaranteed det_seq-sorted. I chose to render one `.inK` port PER PRODUCER,
ordered by PRODUCER det_seq (deterministic, matches the `=>` det_seq rule), and
to name the pivot binding per port (`pivot X<-col@cN`). This DIVERGES from the
DOT's raw port order for join.14/15/16 (DOT puts the single-col demand side at
p0; det_seq order puts it last). FRICTION: the spec must (i) pin the JOIN port
ordering to a pointer-free key (I recommend producer det_seq) and (ii) define
the port-naming for >2 inputs and for the pivot-column binding. Until pinned,
the join blocks in §1 are STRUCTURE-firm (producers, cols, pivot, output all
DOT-verified) but PORT-ORDER-illustrative.

**Q5 — `_MissingVar` as a column name is a demand-transform artifact.** Four
fabricated columns (c3, c11, c14, c21) print `_MissingVar` (the DOT's name for
a column with no source parse variable). The spec §1.3 column rule is
`<var-or-cN>:<type>`; `_MissingVar` is neither a clean var nor a `cN` fallback
— it is the impl's placeholder string. FRICTION: decide whether the dump prints
`_MissingVar` verbatim (I did, matching the DOT) or falls back to `c<N>` for
nameless columns. If multiple nameless columns coexist in one view the
`_MissingVar` spelling is ambiguous; the `@cN` suffix I appended everywhere
disambiguates, but the spec doesn't mandate that suffix — I ADDED `@cN` to
every column (`name:type@cN`) beyond the spec's `name:type` to keep the dump
unambiguous and cross-referable to the .ir/.dot. Spec should ratify or reject
the `@cN` suffix.

**Q6 — shared-model views (%table:8, %table:12, %table:19 each on TWO views).**
tuple.4 & merge.17 both show `TABLE 8`; tuple.6 & merge.18 both `TABLE 12`;
tuple.2 & tuple.10 both `TABLE 19`. These are distinct views sharing one
DataTable model (the group_ids/CSE-guard invariant: a table's member-view list
holds each view by identity). I emitted `table=%table:N` on BOTH views of each
pair, which is faithful but means a reader cannot tell from the dump which view
"owns" the table vs merely reads it. FRICTION: the spec may want a distinction
(e.g. `table=%table:8` on the owning MERGE vs `reads=%table:8` on the view), or
may accept the current symmetric rendering. Recorded, not resolved.

**Q7 — stratum source vs Stratum() accessor.** I took `stratum=N` from the DOT
`STRATUM N` cells. The spec §1.3 says `stratum=<Stratum()>`. These should be
the same value (the DOT annotates from the same Stratify pass), but I did not
independently confirm `QueryView::Stratum()` returns the DOT's number vs some
offset. Low risk; flagging for completeness.

