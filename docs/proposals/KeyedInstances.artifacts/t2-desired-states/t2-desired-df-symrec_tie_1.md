# t2 desired-state: `-df-out` of symrec_tie_1.dr

REVISED 2026-07-18 against ratified t2-dump-spec v3 (post-E-61..E-66).
Supersedes the DRAFT-PENDING-REVISION version; folds the committed
critique (critique-df-symrec_tie_1.md, findings F-A..F-F) and the v3
ratified rulings (decisions (a)/(a2)/(a3) + the E-61 kind order and the
PINNED class semantics). Amendments applied are enumerated in §4.

DESIRED-OUTPUT-STATE artifact for the future `-df-out` DataFlow BB-with-
arguments dump. Hand-written against the spec BEFORE the emitter exists.
Implementation later means: make the compiler print EXACTLY the §1 text,
gated by strict byte-compare. This is a determinism witness — the two
structurally symmetric recursive arms tie on structural hash AND first-
column-id; only `det_seq` (ForEachView creation order = source clause
order) orders them, and §2 freezes which arm lands at the lower block id.

## §0 — Provenance

Inputs re-read this REVISION session (branch keyed-instances @ tip
63c8443c; ground truth regenerated from the frozen 63c8443c baseline
binary, NOT the older b577735e dumps):
- SPEC (binding): `docs/proposals/KeyedInstances.artifacts/t2-dump-spec.md`
  — v3, RATIFIED 2026-07-18. The T2a `-df-out` contract (§1: wiring §1.1,
  block-id decision (a)=det_seq §1.2, the form §1.3, predictions §1.4).
  This revision applies v3 END TO END; where the older ir-dump-formats
  sketch or a v1/v2 critique conflicts, v3 rules.
- COMMITTED CRITIQUE (this artifact's):
  `.../t2-desired-states/critique-df-symrec_tie_1.md` (F-A..F-F).
- GROUND TRUTH graph (generated this session from the frozen 63c8443c
  baseline): `.../scratchpad/ground/symrec_tie_1.dot` (every view kind,
  columns, var names, STRATUM, EQ SET, TABLE ids, every column edge) and
  `.../scratchpad/ground/symrec_tie_1.ir` (table/col/index id spaces —
  %table:4=tc, %table:8=edge; the two fixpoint joins that pin which arm
  is which). Pointer node-ids differ from the b577735e dumps the critique
  cites; the STRUCTURE is byte-for-byte the same (determinism witness).
- Case source: `tests/OptDiff/cases/symrec_tie_1.dr`.
- ForEachView kind sequence: `lib/DataFlow/Query.h:1176-1214` (the
  non-const overload) — the per-kind DefList iteration order that IS the
  det_seq numbering order. Re-read this session; E-61 kind order confirmed
  (negations BEFORE compares — irrelevant here, no negate/compare).
- JOIN accessor order (v3 §1.3 grammar): `lib/DataFlow/Query.cpp:822-844`
  (NthInputPivotSet / NthInputMergedColumn, key-lookup on `columns[n]`);
  joined_views / `.in<K>` order built at `Build.cpp:1246-1293`.
- CLASS accessor: `lib/DataFlow/Query.cpp:347-354`
  (CanReceiveDeletions / CanProduceDeletions) — the v3-pinned class map.
- DOT edge semantics: `lib/DataFlow/Format.cpp` (JOIN ports 311-392,
  MERGE fan-in, SELECT) — DOT edges are drawn use→def
  (`consumer:port -> producer:col`); the tail-call form below inverts them
  to def→use (producer pushes to consumer).

Spec version: t2-dump-spec.md v3 (RATIFIED 2026-07-18). All eight owner
decisions (a),(a2),(a3),(b),(b2),(c),(d),(e) ratified.

AMENDED 2026-07-19 (round-2 adjudication + v3.2 grammar unification,
ledger §10, errata E-70/E-71): §1 re-rendered under the session-pinned
emitter rulings (p5)-(p9) with the RATIFIED (p2)/(p3) pins
back-applied (E-70 — this artifact predated their cross-artifact
application): identity `=>` entries now BARE tokens (p2: `(A=A, B=B)`
-> `(A, B)`); producer-side `.in<K>` maps render pure producer tokens
in JOIN-PORT order (p3-order: tuple.2's edge is now `.in0(X, A)`,
tuple.6's arm2 edge `.in0(B, A)`); ALL hand-written prose annotation
comments STRIPPED from the byte-block (p8 — only `; cycle`,
`; callers:`, and select provenance are emitter-derivable; the
stripped annotations survive in §2's derivation prose); provenance
re-spelled per p7 (`; recv #message edge/2`); JOIN bodies to the p9
form (2-space body indent, single-space separators, `}` at column 0);
comments re-padded to the p6 column. GRAPH FACTS UNCHANGED (ids,
strata, classes, tables, arm assignments, cycle set — incl. the §2
det_seq tie-break determinism claim). Pre-amendment renderings in
derivation prose are HISTORICAL.

## §1 — THE DESIRED `-df-out` TEXT

Rendered per t2-dump-spec.md v3 §1.2/§1.3:
- header line is exactly `dataflow` (v3 §1.3 — no clause reconstruction,
  no module-name dependency);
- one block per LIVE view, blocks emitted in ASCENDING det_seq order,
  block id `^<kind>.<det_seq>`;
- block PARAMS = the view's OWN finalized columns as
  `<var-or-cN>:<type>` (var when finalized-present, else `c<id>`; NEVER
  `_MissingVar`, NO `@cN` suffix). All columns here carry graph var names,
  so no cN fallback fires (§2.5);
- `=>` tail-call lines, one per user column-edge, ordered (user det_seq,
  port), `dst=src` maps where dst is the USER's column token and src is
  THIS producer's column token;
- `; cycle` on a `=>` line iff the def (this producer) is REACHABLE from
  the user — v3 decision (a2-i), reachability-exact; v1's `; back-edge`
  det_seq-comparison rule is WITHDRAWN as unsound (critique F-C);
- `; callers: ...` ONLY on MERGE blocks, ascending det_seq (v3 §1.3);
- ATTRIBUTES line carries `table=%table:<id>` (on every table-backed view),
  `class=<differential|monotone|table-less>` (the v3-PINNED map, §2.4),
  and `stratum=<Stratum()>`;
- NO `producer=` anywhere — v3 decision (a3) drops it from the default
  dump entirely (it is `#ifndef NDEBUG`-only, a config-variant field);
  this program has no pass-minted views regardless.

All columns are `u64` (edge/tc/out are all u64,u64).

The det_seq numbering derives from ForEachView (Query.h:1176-1214) kind
order — selects, tuples, kv_indices, joins, maps, aggregates, merges,
NEGATIONS, COMPARES, inserts (E-61 order) — applied in §2. FIRM ids: the
SELECT (0), the two JOINs (8, 9 — the load-bearing determinism claim, §2),
the MERGE (10), the INSERT (11). ILLUSTRATIVE-PREDICTED ids AND their
paired stratum values: the seven TUPLEs (1..7) — the intra-kind creation
order (and thus which DOT-node's stratum each carries) is not derivable
from the .dot/.ir alone; pinned at bless (§2.4, §3-F1, critique F-A).

CLASS NOTE (v3-PINNED, the biggest change from v1): symrec_tie_1 is a
PURELY MONOTONE insert-only recursion — nothing produces deletions (the
ground-truth .dot has ZERO purple/deletion edges; the .ir is all
`+nonrecursive`/`+recursive`, no `-`). Per v3 §1.3 "RECURSION DOES NOT
IMPLY DIFFERENTIAL", tc's table-backed views are `class=monotone`, NOT
`differential`. So NO block here is `class=differential` (v1 mislabeled
tc-readback/merge/insert — critique F-D, corrected below).

```
dataflow

select ^select.0 (A:u64, X:u64)                    ; recv #message edge/2
  ATTRIBUTES class=table-less stratum=0
  => ^tuple.1 (A, X)
  => ^tuple.2 (A, X)
  => ^tuple.3 (A, X)

tuple ^tuple.1 (A:u64, X:u64)
  ATTRIBUTES class=table-less stratum=1
  => ^merge.10 (A, B=X)

tuple ^tuple.2 (A:u64, X:u64)
  ATTRIBUTES table=%table:8 class=monotone stratum=2
  => ^join.9 .in0(X, A)

tuple ^tuple.3 (A:u64, X:u64)
  ATTRIBUTES table=%table:8 class=monotone stratum=3
  => ^join.8 .in1(A, X)

tuple ^tuple.4 (A:u64, B:u64)
  ATTRIBUTES class=table-less stratum=4
  => ^merge.10 (A, B)                              ; cycle

tuple ^tuple.5 (A:u64, B:u64)
  ATTRIBUTES class=table-less stratum=4
  => ^merge.10 (A, B)                              ; cycle

tuple ^tuple.6 (A:u64, B:u64)
  ATTRIBUTES table=%table:4 class=monotone stratum=4
  => ^join.8 .in0(B, A)                            ; cycle
  => ^join.9 .in1(A, B)                            ; cycle

tuple ^tuple.7 (A:u64, B:u64)
  ATTRIBUTES class=table-less stratum=5
  => ^insert.11 (A, B)

join ^join.8 (X:u64, A:u64, B:u64) {
  pivot X:u64 <- .in0.B, .in1.A
  out A:u64 <- .in0.A
  out B:u64 <- .in1.X
}
  ATTRIBUTES class=table-less stratum=4 set=0 depth=1
  => ^tuple.4 (A, B)                               ; cycle

join ^join.9 (X:u64, A:u64, B:u64) {
  pivot X:u64 <- .in0.X, .in1.A
  out A:u64 <- .in0.A
  out B:u64 <- .in1.B
}
  ATTRIBUTES class=table-less stratum=4 set=0 depth=1
  => ^tuple.5 (A, B)                               ; cycle

merge ^merge.10 (A:u64, B:u64)                     ; callers: ^tuple.1, ^tuple.4, ^tuple.5
  ATTRIBUTES table=%table:4 class=monotone stratum=4 set=0 depth=1
  => ^tuple.6 (A, B)                               ; cycle
  => ^tuple.7 (A, B)

insert ^insert.11 (A:u64, B:u64) into %table:4
  ATTRIBUTES class=monotone stratum=6
```

## §2 — Derivation notes

### 2.1 The det_seq numbering rule (the block-id source)

Block id = `det_seq` (decision (a), t2-dump-spec §1.2; UniqueId REJECTED
per E-57). det_seq is the dense 0..N-1 position in `ForEachView` order at
the last stamp (Query.h:472; DeterministicOrder() Query.cpp:430-433;
stamped at IdentifyInductions head Induction.cpp:144 which runs after
LinkViews, then re-run/kept through CSE Optimize.cpp:287). ForEachView
(READ Query.h:1176-1214, non-const overload) walks the per-kind DefLists
in this FIXED sequence, skipping dead views:

    selects · tuples · kv_indices · joins · maps · aggregates ·
    merges · negations · compares · inserts

symrec_tie_1's live-view census (from the fresh 63c8443c .dot): 1 SELECT,
7 TUPLE, 0 KVINDEX, 2 JOIN, 0 MAP, 0 AGG, 1 MERGE, 0 NEGATE, 0 COMPARE,
1 INSERT = 12 views. So the det_seq blocks fall out as (node ids are the
63c8443c pointers — they are NOT part of the dump and change run to run;
only the STRUCTURE is stable):

    det_seq  kind    block id     .dot node
    0        SELECT  ^select.0    v31189043072  (RECEIVE edge)
    1..7     TUPLE   ^tuple.1..7  (7 TUPLE nodes; intra-kind order §2.3)
    8        JOIN    ^join.8      v4317406784   (EQ SET 9, arm2 tc-first)
    9        JOIN    ^join.9      v4317409744   (EQ SET 10, arm3 edge-first)
    10       MERGE   ^merge.10    v4317405776   (UNION tc, TABLE 4)
    11       INSERT  ^insert.11   v31184749568  (MATERIALIZE out)

Within a kind, det_seq = DefList insertion order = view CREATION order.
DefLists are append-only insertion-ordered vectors (consolidated §1.A);
CSE marks dead but never reorders. Clauses build in source order
(Build.cpp:2528-2536 → BuildClause per clause), so creation order tracks
source-clause order for the per-clause views.

### 2.2 THE DETERMINISM CLAIM — which arm gets the lower block id, and why

**^join.8 = arm2 (`tc(A,B):tc(A,X),edge(X,B)`, tc-first, source line 18).
^join.9 = arm3 (`tc(A,B):edge(A,X),tc(X,B)`, edge-first, source line 19).**
The tc-first arm lands at the LOWER join block id.

Why this is the witness: the two JOINs are STRUCTURALLY SYMMETRIC — same
kind (JOIN), same arity, same u64 columns, same base relation (edge) and
same recursive relation (tc), same head vars (A,B). So they TIE on the
first two levels of OrderViewsDeterministically (Induction.cpp:112-126):
  (1) structural hash `Sort()==Hash()` — identical (same KindName, same
      deletion flags, same col count, same folded structure);
  (2) first output column's id — at IdentifyInductions time (BEFORE
      FinalizeColumnIDs) this is the source-lexical VarId-derived id of
      the head's first var `A`, identical for both arms.
Only level (3), `det_seq` (= ForEachView creation order = source clause
order), separates them — and arm2's JOIN is created first (source line 18
< line 19), so it holds the smaller det_seq. This ordering is EXACTLY what
the future `.irgold` sidecar freezes; a regression that reordered the two
joins (e.g. a pointer-keyed iteration creeping back in) would flip
^join.8 / ^join.9 and their downstream ^tuple.4 / ^tuple.5 assignments,
and the byte-compare would catch it. (Note the FINALIZED ids c18 vs c21
differ between the arms, but finalization runs AFTER the det_seq stamp, so
it cannot influence the ordering — it only labels columns for display.)

Arm identity re-confirmed against the fresh 63c8443c .ir fixpoint joins:
- .ir:72-81 (first join): pivot X, tc via `%index:55[_,u64] where %col:6`
  (tc keyed on col6=B) ⋈ edge via `%index:56[u64,_] where %col:9` (edge
  keyed on col9=A), result `+recursive {@A:59,@X:60}` — this is
  tc.B==edge.A, i.e. arm2 (tc-first). Matches ^join.8's DOT wiring
  (v4317406784 p0→tuple6.c8=B, p1→tuple3.c11=A).
- .ir:87-96 (second join): pivot X, tc via `%index:47[u64,_] where
  %col:5` (tc keyed on col5=A) ⋈ edge via `%index:48[_,u64] where
  %col:10` (edge keyed on col10=X), result `+recursive {@A:51,@B:52}` —
  edge.X==tc.A, i.e. arm3 (edge-first). Matches ^join.9's DOT wiring.

The whole `.ir` is `+nonrecursive`/`+recursive` update-counts — NO signed
retraction, NO `-` op anywhere — which is what pins `class=monotone`
throughout (§2.4); and the fixpoint is a genuine cycle, which is what
makes the reachability-exact `; cycle` markers fire on the six cycle-node
edges (§2.5).

### 2.2b `.in<K>` assignment — the arm asymmetry, pinned to code

The v3 JOIN grammar labels inputs `.in<K>` by `joined_views` position
(Build.cpp:1246-1249 pushes `joined_views` from `pivot_groups[best_pivot]`
in `next_views` order), and renders pivot/out entries in
OUTPUT-COLUMN-POSITION order (NthInputPivotSet/NthInputMergedColumn,
Query.cpp:822-844, key-lookup on `columns[n]`). The DOT pivot-input ports
p0,p1 enumerate NthInputPivotSet in that same joined_views order, so the
DOT gives `.in<K>` directly:
- ^join.8 (arm2): pivot ports p0→tc.c8, p1→edge.c11 ⇒ `.in0`=tc,
  `.in1`=edge. Merged out A(c18)←tc.c7=`.in0.A`; out B(c19)←edge.c12=
  `.in1.X`.
- ^join.9 (arm3): pivot ports p0→edge.c14, p1→tc.c7 ⇒ `.in0`=edge,
  `.in1`=tc. Merged out A(c21)←edge.c13=`.in0.A`; out B(c22)←tc.c8=
  `.in1.B`.
THIS is the witness's visible asymmetry: in arm2 the tc side is `.in0`
and supplies the pivot from its B column; in arm3 the tc side is `.in1`
and supplies the pivot from its A column. The `.in<K>` order is
joined_views order, itself det_seq-adjacent (pointer-free), so it is
stable — a pointer-ordered regression would swap `.in0`/`.in1` and the
byte-compare catches it. (v3 drops the invented `.lhs/.rhs` labels that
critique F-E flagged; `.in<K>` is graph-carried, lhs/rhs was not.)

### 2.3 Every block → its .dot evidence (fresh 63c8443c node ids)

DOT edges are drawn use→def (`consumer:port -> producer:col`,
Format.cpp: JOIN 373-390, MERGE fan-in, INSERT, TUPLE via the generic
port linker); the §1 `=>` tail-calls INVERT them to def→use. The DOT
carries NO purple/deletion edge anywhere (Format.cpp:376 draws
`CanProduceDeletions` edges purple) — the graph-level confirmation that
NOTHING is differential (§2.4).

- **^select.0** — `v31189043072 [ ... RECEIVE ... port="c1">A ... port="c2">X ]`
  (.dot:8), edge `v31189043072 -> t31184732544` (.dot:7, the I/O edge
  relation). Cols c1=A, c2=X (edge's two params; the second is named X in
  the graph — see §2.5). class=table-less (no TABLE annotation), STRATUM 0.
  Pushes to the three edge-guard TUPLEs (.dot:34-42, all three read c1/c2).
- **^tuple.1** (illustrative id/stratum) — the base-case guard. STRATUM 1
  EQ SET 1 `v31184782656`, NO TABLE (.dot:40), feeds MERGE (.dot:44
  `v4317405776 -> v31184782656`). This is `tc(A,B):edge(A,B)` — edge's X
  column becomes tc's B. class=table-less.
- **^tuple.2** (illustrative id/stratum) — STRATUM 2 EQ SET 1
  `v31184782208`, TABLE 8 (.dot:37). Reads SELECT (.dot:38-39). Feeds
  ^join.9's edge side = `.in0` (.dot:15,17: v4317409744 p0→c14=X,
  p2→c13=A). edge is a plain non-deletable #message ⇒ class=monotone,
  table=%table:8.
- **^tuple.3** (illustrative id/stratum) — STRATUM 3 EQ SET 1
  `v31184781760`, TABLE 8 (.dot:34). Reads SELECT (.dot:35-36). Feeds
  ^join.8's edge side = `.in1` (.dot:11,13: v4317406784 p1→c11=A,
  p3→c12=X). class=monotone, table=%table:8.
- **^tuple.4** (illustrative id/stratum) — STRATUM 4 EQ SET 2
  `v4317413200` (.dot:22). Reads ^join.8 merged outputs (.dot:23-24:
  p0→join c18=A, p1→c19=B). Feeds MERGE (.dot:45). arm2 result row.
  class=table-less.
- **^tuple.5** (illustrative id/stratum) — STRATUM 4 EQ SET 3
  `v4317413808` (.dot:25). Reads ^join.9 merged outputs (.dot:26-27:
  p0→c21=A, p1→c22=B). Feeds MERGE (.dot:46). arm3 result row.
  class=table-less.
- **^tuple.6** (illustrative id/stratum) — STRATUM 4 EQ SET 11
  `v4317414256`, TABLE 4 (.dot:28). The tc read-back shared by BOTH joins.
  Reads MERGE (.dot:29-30: p0→c23=A, p1→c24=B). Feeds ^join.8 `.in0`
  (.dot:10,12: c8=B pivot, c7=A out) AND ^join.9 `.in1` (.dot:16,18:
  c7=A pivot, c8=B out). table=%table:4; class=MONOTONE (tc is
  insert-only — v3-pinned, NOT differential; §2.4).
- **^tuple.7** (illustrative id/stratum) — STRATUM 5 EQ SET 11
  `v31184781312` (.dot:31). Reads MERGE (.dot:32-33). Feeds INSERT
  (.dot:20-21). The out-materialize path. class=table-less.
- **^join.8** — `v4317406784` JOIN, EQ SET 9, SET 0 DEPTH 1, STRATUM 4
  (.dot:9). Output columns [c17=X (pivot), c18=A, c19=B]. Pivot inputs
  p0=B(tc),p1=A(edge); merged inputs p2=A(tc),p3=X(edge) → .dot:10-13.
  `.in0`=tc, `.in1`=edge (§2.2b). class=table-less, STRATUM 4.
- **^join.9** — `v4317409744` JOIN, EQ SET 10, SET 0 DEPTH 1, STRATUM 4
  (.dot:14). Output columns [c20=X (pivot), c21=A, c22=B]. Pivot inputs
  p0=X(edge),p1=A(tc); merged inputs p2=A(edge),p3=B(tc) → .dot:15-18.
  `.in0`=edge, `.in1`=tc (§2.2b). class=table-less, STRATUM 4.
- **^merge.10** — `v4317405776` UNION "tc", TABLE 4, SET 0 DEPTH 1,
  STRATUM 4, EQ SET 11 (.dot:43). MergedViews (fan-in) = ^tuple.1,
  ^tuple.4, ^tuple.5 (.dot:44-46, in that vector order). Pushes to
  ^tuple.6 (tc read-back) and ^tuple.7 (out path). table=%table:4;
  class=MONOTONE (tc insert-only). The `; callers:` line lists the three
  fan-in producers ascending det_seq (^tuple.1, ^tuple.4, ^tuple.5).
- **^insert.11** — `v31184749568` MATERIALIZE "out", TABLE 4, STRATUM 6,
  EQ SET 11 (.dot:19). Terminal. Reads ^tuple.7 via input-position ports
  c0/c1 (.dot:20-21: c0→c9=A, c1→c10=B). v3 INSERT header form: the input
  tokens are the PRODUCER's (tuple.7's) columns in input-position order —
  `(A:u64, B:u64)` — and `into %table:4` (TableId in the HEADER; NO
  ATTRIBUTES table= line, v3 §1.3). class=monotone stratum=6. (out
  materializes THROUGH tc's %table:4 — critique F-F CONFIRMED: the .ir
  declares only %table:4 and %table:8, no separate `out` table.)

### 2.4 Class semantics (v3-PINNED) + firm vs illustrative

CLASS = the v3-PINNED map (spec §1.3, critique F-D), keyed to a QueryView
predicate (Query.cpp:347-354):
- `class=table-less` — no backing table (no TABLE annotation in the .dot).
- `class=monotone` — table present, NOT deletion-capable
  (`!CanReceiveDeletions()`).
- `class=differential` — table present AND `CanReceiveDeletions()`.
RECURSION DOES NOT IMPLY DIFFERENTIAL. symrec_tie_1 is insert-only: the
.dot has zero purple (`CanProduceDeletions`) edges and the .ir is entirely
`+`-signed, so NOTHING is deletion-capable. Hence the tc-backed views
(^tuple.6, ^merge.10, ^insert.11) are `class=monotone`, NOT differential
(v1's error, critique F-D). No `class=differential` block exists in this
program. This is the single largest v3 correction to the dump body.

- FIRM (derived from code + .dot + .ir, not guessable-away):
  - The kind sequence and the resulting det_seq BANDS (SELECT=0,
    TUPLE=1..7, JOIN=8..9, MERGE=10, INSERT=11) — from Query.h:1176-1214
    + the census.
  - The two JOIN ids (^join.8=arm2/tc-first, ^join.9=arm3/edge-first),
    their `.in<K>` asymmetry, and the REASON (§2.2/2.2b) — the witness.
  - The MERGE id (10) and INSERT id (11) — singletons in their kinds.
  - The SELECT id (0) — singleton.
  - Every column name/type, every edge (def→use), every TABLE id, and the
    class values (from the code-pinned predicate above; monotone
    throughout the table-backed views) — read from the .dot/.ir + code.
  - The `; cycle` marker set (§2.5) — reachability-exact, deterministic.
- ILLUSTRATIVE-PREDICTED (house precedent: witness-deltarel-target.md op
  ids):
  - The seven TUPLE ids 1..7 (intra-TUPLE-kind creation order) AND THE
    STRATUM VALUE PAIRED WITH EACH (critique F-A): the .dot/.ir expose no
    creation-order signal for TUPLEs (EQ SET is an equivalence-class id,
    Format.cpp:56, NOT a mint counter; STRATUM is a post-hoc topological
    layer, firm PER DOT-NODE but bound to a GUESSED det_seq here). The §1
    assignment groups them by role (base guard, edge sides, join results,
    tc read-back, out path) as a plausible creation order, but the ACTUAL
    1..7 permutation — and therefore which node's stratum each ^tuple.N
    carries — must be read from the built graph when the emitter lands.
    The per-node strata that EXIST in this graph are {1,2,3,4,4,4,5}; the
    §1 rows pair them with the guessed ids and will very likely need a
    permutation fix at bless. det_seq is ForEachView/creation order, NOT
    stratum — the two need not ascend together. Their det_seq BAND (1..7,
    strictly between SELECT and JOIN) is firm; the permutation inside it,
    and thus each (id,stratum) pairing, is not. A regression-gate `.irgold`
    freezes whatever the emitter actually prints — the TUPLE ids AND their
    strata here are the desired SHAPE, and the bless step pins the exact
    permutation.

### 2.5 The `; cycle` marker (v3 decision a2-i, reachability-exact)

v3 §1.3 / decision (a2-i): mark a `=>` edge `; cycle` IFF the def (the
producer of the edge) is REACHABLE FROM the user (the consumer). I.e. the
edge closes a cycle: from the consumer, following the graph forward, you
can return to the producer. This is exact and det_seq-INDEPENDENT — it
replaces v1's unsound `; back-edge` (which compared det_seq ids and
therefore both over-fired and under-fired; critique F-C, spec §1.3).

The recursion cycle here is the 6-node loop
`^merge.10 → ^tuple.6 → {^join.8, ^join.9} → {^tuple.4, ^tuple.5} → ^merge.10`.
Walking every `=>` edge, the def-reachable-from-user test yields EXACTLY
the seven intra-cycle edges (and nothing else):

    edge (producer => consumer)        def reachable from consumer?   mark
    ^select.0 => ^tuple.1/2/3          no (select is a source)        —
    ^tuple.1  => ^merge.10             no (tuple.1 fed only by select) —
    ^tuple.2  => ^join.9               no                             —
    ^tuple.3  => ^join.8               no                             —
    ^tuple.4  => ^merge.10             yes (merge→tuple.6→join.8→t4)   cycle
    ^tuple.5  => ^merge.10             yes (merge→tuple.6→join.9→t5)   cycle
    ^tuple.6  => ^join.8               yes (join.8→t4→merge→tuple.6)   cycle
    ^tuple.6  => ^join.9               yes (join.9→t5→merge→tuple.6)   cycle
    ^tuple.7  => ^insert.11            no (insert is terminal)         —
    ^join.8   => ^tuple.4              yes (t4→merge→tuple.6→join.8)   cycle
    ^join.9   => ^tuple.5              yes (t5→merge→tuple.6→join.9)   cycle
    ^merge.10 => ^tuple.6              yes (tuple.6→join.8→t4→merge)   cycle
    ^merge.10 => ^tuple.7             no (tuple.7→insert, terminal)    —

So `; cycle` fires on the seven edges among the six cycle nodes, and
nowhere else. This is the cycle-accurate annotation F8 asked for and the
consistency F-C demanded — the marker set is now the exact induction
sub-graph, uniform under one rule. (One memoized reachability pass in the
emitter, spec §1.3.)

## §3 — Spec frictions — v3-RESOLUTION STATUS

The v1 frictions F1-F8 were raised against the DRAFT spec. v3 has since
ratified rulings for all of them; this section records the resolution and
what residual uncertainty (if any) survives.

F1. **INTRA-KIND TUPLE det_seq — RESIDUAL, unchanged.** The seven TUPLEs'
    relative det_seq (DefList creation order) is still not reconstructable
    from the .dot/.ir alone; v3 did not add a creation-order signal to
    either dump. RESOLUTION: house precedent — the TUPLE ids (and, per
    critique F-A, their paired stratum values) stay ILLUSTRATIVE and are
    pinned at bless from the real emitter output. §2.4 now flags the
    (id,stratum) pairing as jointly unpinned. This is the ONE surviving
    uncertainty in the dump body; it does not touch the JOIN-pair witness.

F2. **`class=` — RESOLVED by v3 §1.3 (PINNED) + critique F-D.** The map is
    now code-pinned: table-less = no table; monotone = table &&
    !CanReceiveDeletions(); differential = table && CanReceiveDeletions()
    (Query.cpp:347-354). RECURSION DOES NOT IMPLY DIFFERENTIAL. Applied:
    all table-backed views here are `class=monotone` (§2.4). Edge case (i)
    (print BOTH `table=` and `class=` on a monotone table-backed view) is
    resolved YES by v3 §1.3 (`table=` printed on every table-backed view
    incl. shared-model pairs) — see ^tuple.2/3/6, ^merge.10. Edge case (ii)
    is resolved: they are MONOTONE, not differential.

F3. **INSERT header — RESOLVED by v3 §1.3.** The INSERT header is
    `insert ^insert.<id> (<producer col tokens>) into %table:<TableId>` —
    input tokens are the PRODUCER's (tuple.7's) columns in input-position
    order (`A:u64, B:u64`), and the TABLE lives in the HEADER, not an
    ATTRIBUTES table= line (redundant). Terminal, no `=>`. Applied to
    ^insert.11. (The DOT's c0/c1 input-position ports map to tuple.7's
    c9=A/c10=B; the dump renders the producer's tokens, not the c0/c1
    indices.)

F4. **JOIN grammar — RESOLVED by v3 §1.3 (PINNED) + critique F-B.** v3
    pins the grammar: inputs `.in<K>` by joined_views position; render in
    output-column-position order `pivot <col> <- .in<J>.<col>, .in<K>.<col>`
    then `out <col> <- .in<J>.<col>`; header arity = ALL output columns
    incl. projected pivots; NO lhs/rhs labels. Applied in §1 (§2.2b derives
    the `.in<K>` assignment). The arm asymmetry is now carried by which
    joined_view is `.in0` vs `.in1` (tc=.in0 in arm2, edge=.in0 in arm3) —
    graph-carried, not invented.

F5. **SELECT column naming (`X` not `B`) — unchanged, benign.** The graph
    names edge's second param `X` (propagated from the recursive bodies).
    v3's `<var-or-cN>` rule takes the finalized graph var, so `(A:u64,
    X:u64)` is correct; no cN fallback fires. Recorded so a reader expecting
    `edge(A,B)` is not surprised.

F6. **`producer=` — RESOLVED by v3 decision (a3).** producer= is DROPPED
    from the default dump entirely (it is `#ifndef NDEBUG`-only; a
    config-variant golden field). So there is nothing for symrec_tie_1 to
    exercise, and its absence is now CORRECT-by-spec, not a coverage gap.
    (The demand mint-site D1-review need is served by fabricated
    `demand__*` names + structure, config-invariant — spec §1.3.)

F7. **out shares tc's %table:4 — CONFIRMED (critique F-F).** The .ir
    declares only %table:4 and %table:8, no separate `out` table; out(A,B):
    tc(A,B) is pure forwarding, so the optimizer shares tc's store. Real
    TableId() semantics, not a binary artifact. The INSERT header renders
    `into %table:4`. Downgraded from "surprising, confirm" to CONFIRMED.

F8. **cycle marking — SUPERSEDED by v3 (a2-i) + critique F-C.** v1's
    `; back-edge` det_seq rule is WITHDRAWN as unsound (it both over- and
    under-fired; critique F-C showed join.8→tuple.4 / join.9→tuple.5 were
    left unmarked under the v1 rule's own logic). v3's reachability-exact
    `; cycle` (§2.5) replaces it: mark iff def is reachable from user. This
    yields the exact 7-edge induction sub-graph, uniformly — the
    cycle-accurate annotation F8 wanted. RESOLVED.

## §4 — Amendments applied in this revision (against v3 + the critique)

Every change from the DRAFT-PENDING-REVISION version:

1. **Header line** — the block prose header (`;; module: …`, clause
   reconstruction) REPLACED by the single line `dataflow` (v3 §1.3).
2. **CLASS corrected to monotone throughout (BIGGEST change; critique F-D,
   v3 §1.3 PINNED).** symrec_tie_1 is insert-only (zero purple/deletion
   edges in the .dot, all-`+` .ir), so tc's table-backed views are
   `class=monotone`, NOT `differential`. Fixed on ^tuple.6, ^merge.10,
   ^insert.11. No `class=differential` block remains. Class map pinned to
   CanReceiveDeletions() (Query.cpp:347-354) in §2.4.
3. **Cycle marker** — v1's `; back-edge` (det_seq comparison) REPLACED by
   v3's reachability-exact `; cycle` (decision a2-i; critique F-C). Derived
   the exact 7-edge cycle set in §2.5; markers applied on the intra-cycle
   `=>` lines only.
4. **JOIN grammar** — the invented `.lhs/.rhs` form REPLACED by v3's pinned
   `.in<K>` grammar (critique F-B/F-E): inputs `.in<K>` by joined_views
   position, rendered `pivot`/`out` in output-column-position order, header
   arity = ALL output columns incl. the projected pivot `X` (so both join
   headers are `(X:u64, A:u64, B:u64)`). `.in<K>` assignment derived from
   code + DOT in §2.2b (arm2: .in0=tc/.in1=edge; arm3: .in0=edge/.in1=tc).
5. **Producer `=>` maps into joins normalized** — a producer's `=>` line
   into a join now carries the identity `dst=src` map over the columns the
   join consumes (`.in0(A=A, B=B)` etc.); the pivot/out ROLE assignment is
   shown ONLY in the join block (removes v1's role-token leakage
   `X=B` onto the producer edge).
6. **INSERT header** — v3 form `insert ^insert.11 (A:u64, B:u64) into
   %table:4`; the redundant ATTRIBUTES `table=` line dropped (table in
   header); class=monotone (was differential). (critique F3 resolved.)
7. **SELECT leading word** — `recv` → `select` to match the E-61 kind name
   that names the block id `^select`; the `; #message edge/2` comment kept
   (graph-derived signal).
8. **producer= confirmed absent by spec** — v1 said "empty because no
   pass-minted views"; v3 (a3) drops producer= from the default dump
   outright, so its absence is correct-by-spec (critique/F6).
9. **Node ids + citations refreshed** — the .dot/.ir were regenerated from
   the frozen 63c8443c baseline this session; all node-id and line
   references updated (pointer ids are NOT part of the dump — the structure
   is byte-identical to the b577735e dumps the critique cited).
10. **Illustrative caveat extended to TUPLE strata** (critique F-A): §2.4
    now flags the (id,stratum) PAIRING as jointly unpinned, not just the
    ids — det_seq (creation order) and stratum (topological layer) need not
    ascend together.

## §5 — Residual uncertainties (LOUD)

- **[RESIDUAL] The seven TUPLE det_seq ids 1..7 AND their paired stratum
  values are ILLUSTRATIVE** (F1/critique F-A). The .dot/.ir carry no TUPLE
  creation-order signal; the emitter's actual permutation — and thus which
  DOT-node's stratum each ^tuple.N carries — must be captured at bless.
  The det_seq BAND (1..7 between SELECT=0 and JOIN=8) is firm; the
  permutation inside it is not. This is a bless-time pin, NOT a
  nondeterminism source (the emitter produces a stable permutation).
- **[VERIFY-AT-EMITTER] Projected-but-unconsumed pivot output column.**
  Both join headers include the pivot output `X` (columns list has c17/c20)
  though downstream tuple.4/tuple.5 consume only A,B. v3 §1.3 says "header
  arity = ALL output columns including projected pivots"; I read that as
  "render every column in the JOIN's `columns` DefList" and included X. If
  the emitter instead prunes a genuinely-dead pivot output column before
  the dump, the two join headers drop to `(A:u64, B:u64)` and the pivot
  appears only on the `pivot X:u64 <-` line. Confirm the JOIN column-list
  contents at emitter time.
- **[VERIFY-AT-BLESS] table-id population** (spec §1.4 v3 rider A1): the
  `%table:4`/`%table:8` ids depend on the post-Program drain point
  populating TableId(); eyeball a real `%table:<id>` in the first real
  emitter output before blessing (a mis-drained emitter prints empty ids
  deterministically and would still byte-compare-pass a hand-blessed
  golden baked from empties).
- **[VERIFY-AT-BLESS] ATTRIBUTES field order for set=/depth=.** §1.3 lists
  `table= class= stratum=` then `set= depth=` for induction members
  (^join.8, ^join.9, ^merge.10, added this revision per critique2 D1); the
  spec does not pin field order as a hard grammar rule, so this artifact
  places set=/depth= last (after stratum=) to match the spec's own listing
  order. Confirm the emitter settles on this order at first bless.

The load-bearing witness — arm2(tc-first)=^join.8 (lower id),
arm3(edge-first)=^join.9, separated ONLY by det_seq, with the visible
`.in<K>` asymmetry — is FIRM and independently re-verified from the DOT
wiring and the .ir fixpoint-join index keys (§2.2/2.2b).

