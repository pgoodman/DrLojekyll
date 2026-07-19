# t2 desired-state: `-df-out` of symrec_tie_1.dr

DESIRED-OUTPUT-STATE artifact for the future `-df-out` DataFlow BB-with-
arguments dump. Hand-written against the spec BEFORE the emitter exists.
Implementation later means: make the compiler print EXACTLY the §1 text,
gated by strict byte-compare. This is a determinism witness — the two
structurally symmetric recursive arms tie on structural hash AND first-
column-id; only `det_seq` (ForEachView creation order = source clause
order) orders them, and §2 freezes which arm lands at the lower block id.

## §0 — Provenance

Inputs read this session (all at branch keyed-instances @ b577735e):
- SPEC (binding): `docs/proposals/KeyedInstances.artifacts/t2-dump-spec.md`
  — the T2a `-df-out` contract (§1: wiring §1.1, block-id decision (a)=
  det_seq §1.2, the form §1.3, predictions §1.4). STATUS in that file:
  DRAFT pending owner ratification of decisions (a)-(d). This artifact is
  written against decision (a)=det_seq (RECOMMENDED) as §1.2 directs.
- The BB form the spec refines: `docs/proposals/KeyedInstances.artifacts/
  ir-dump-formats.md` §1 (design idea §1.1, sketch §1.2, conventions +
  open questions Q1-Q3).
- Verified as-landed record: `.../fleet-ckpt/consolidated.md` — §1.A
  det_seq facts (Query.h:472, DeterministicOrder() Query.cpp:430-433,
  two stamp sites Induction.cpp:144 + Optimize.cpp:287, the three-level
  OrderViewsDeterministically total order Induction.cpp:112-126), §4.a
  (no FinalizeViewIDs pass; det_seq is THE substrate-sanctioned key;
  UniqueId REJECTED per E-57), §4.b (post-Program drain gets TableId()).
- GROUND TRUTH graph (generated this session from the frozen b577735e
  binary): `.../dump-inputs/symrec_tie_1.dot` (every view kind, columns,
  var names, STRATUM, EQ SET, TABLE ids, every column edge).
- ControlFlow IR: `.../dump-inputs/symrec_tie_1.ir` (table/col/index id
  spaces — %table:4=tc, %table:8=edge; the two fixpoint joins that pin
  which arm is which).
- Case source: `tests/OptDiff/cases/symrec_tie_1.dr`.
- ForEachView kind sequence: READ `lib/DataFlow/Query.h:1176-1214` (the
  non-const overload) — the per-kind DefList iteration order that IS the
  det_seq numbering order.
- DOT edge semantics: READ `lib/DataFlow/Format.cpp` (JOIN ports 311-390,
  MERGE fan-in 666-708, SELECT 178-195) — DOT edges are drawn use→def
  (`consumer:port -> producer:col`); the tail-call form below inverts them
  to def→use (producer pushes to consumer).

Binary hash source: frozen b577735e binary (per prompt); the .dot/.ir
ground truth were generated from it this session.

Spec version: t2-dump-spec.md as of b577735e (DRAFT; decisions (a)-(e)
unratified — see §3 frictions).

## §1 — THE DESIRED `-df-out` TEXT

Rendered per t2-dump-spec.md §1.2/§1.3: one block per live view, blocks
emitted in ASCENDING det_seq order, block id `^<kind>.<det_seq>`, columns
`<var-or-cN>:<type>` with N the FINALIZED column id (the same id the DOT
ports print), `=>` tail-call lines ordered by (user det_seq, port) with
`dst=src` column maps, `; back-edge` comment when user det_seq <= def
det_seq, and one ATTRIBUTES line per block carrying `table=%table:<N>`
(cross-referencing the .ir), `class=`, `stratum=`. No `producer=` line
appears anywhere in THIS program — it has no pass-minted views (no demand,
no aggregate), so the field is empty at every block (spec §1.3: printed
only when non-empty). All columns are `u64` (edge/tc/out are all u64,u64).

The det_seq numbering derives from ForEachView (Query.h:1176-1214) kind
order — SELECT, TUPLE, KVINDEX, JOIN, MAP, AGG, MERGE, NEGATE, COMPARE,
INSERT — applied in §2. FIRM ids: the SELECT (0), the two JOINs (8, 9 —
the load-bearing determinism claim, §2), the MERGE (10), the INSERT (11).
ILLUSTRATIVE-PREDICTED ids: the seven TUPLEs (1..7) — intra-kind creation
order is not derivable from the .dot/.ir alone (§2, §3-F1).

```
;; module: symrec_tie_1
;;   tc(A,B) : edge(A,B).                 base
;;   tc(A,B) : tc(A,X), edge(X,B).        arm2 (tc-first / left-recursive)
;;   tc(A,B) : edge(A,X), tc(X,B).        arm3 (edge-first / right-recursive)
;;   out(A,B) : tc(A,B).
;; det_seq order: SELECT · TUPLE* · JOIN* · MERGE · INSERT

recv ^select.0 () -> (A:u64, X:u64)          ; #message edge/2
  ATTRIBUTES class=table-less stratum=0
  => ^tuple.1 (A=A, X=X)
  => ^tuple.2 (A=A, X=X)
  => ^tuple.3 (A=A, X=X)

tuple ^tuple.1 (A:u64, X:u64)                 ; callers: ^select.0
  ATTRIBUTES class=table-less stratum=1
  => ^merge.10 (A=A, B=X)                     ; base-case row into tc

tuple ^tuple.2 (A:u64, X:u64)                 ; callers: ^select.0
  ATTRIBUTES table=%table:8 class=monotone stratum=2
  => ^join.9 .rhs(A=A, X=X)                   ; edge side of arm3 (edge-first)

tuple ^tuple.3 (A:u64, X:u64)                 ; callers: ^select.0
  ATTRIBUTES table=%table:8 class=monotone stratum=3
  => ^join.8 .rhs(A=A, X=X)                   ; edge side of arm2 (tc-first)

tuple ^tuple.4 (A:u64, B:u64)                 ; callers: ^join.8
  ATTRIBUTES class=table-less stratum=4
  => ^merge.10 (A=A, B=B)                     ; arm2 result into tc

tuple ^tuple.5 (A:u64, B:u64)                 ; callers: ^join.9
  ATTRIBUTES class=table-less stratum=4
  => ^merge.10 (A=A, B=B)                     ; arm3 result into tc

tuple ^tuple.6 (A:u64, B:u64)                 ; callers: ^merge.10
  ATTRIBUTES table=%table:4 class=differential stratum=4
  => ^join.8 .lhs(A=A, X=B)                   ; tc side of arm2, keyed on B
  => ^join.9 .lhs(A=A, X=A)                   ; tc side of arm3, keyed on A

tuple ^tuple.7 (A:u64, B:u64)                 ; callers: ^merge.10
  ATTRIBUTES class=table-less stratum=5
  => ^insert.11 (c0=A, c1=B)

join ^join.8 [pivot X:u64] {                  ; arm2: tc(A,X),edge(X,B)
    pivot X <- ^tuple.6.B, ^tuple.3.A          ; tc.B == edge.A
    .lhs  <- ^tuple.6  out A = tuple.6.A        ; tc keyed on B (=X)
    .rhs  <- ^tuple.3  out B = tuple.3.X        ; edge keyed on A (=X)
  } -> (A:u64, B:u64)
  ATTRIBUTES class=table-less stratum=4
  => ^tuple.4 (A=A, B=B)

join ^join.9 [pivot X:u64] {                  ; arm3: edge(A,X),tc(X,B)
    pivot X <- ^tuple.2.X, ^tuple.6.A          ; edge.X == tc.A
    .lhs  <- ^tuple.6  out B = tuple.6.B        ; tc keyed on A (=X)
    .rhs  <- ^tuple.2  out A = tuple.2.A        ; edge keyed on X (=X)
  } -> (A:u64, B:u64)
  ATTRIBUTES class=table-less stratum=4
  => ^tuple.5 (A=A, B=B)

merge ^merge.10 (A:u64, B:u64)                ; callers: ^tuple.1, ^tuple.4, ^tuple.5
  ATTRIBUTES table=%table:4 class=differential stratum=4
  => ^tuple.6 (A=A, B=B)                       ; back-edge (tc read-back into joins)
  => ^tuple.7 (A=A, B=B)                       ; back-edge (into out path)

insert ^insert.11 (A:u64, B:u64) into out     ; terminal (query out)
  ATTRIBUTES table=%table:4 class=differential stratum=6
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

symrec_tie_1's live-view census (from the .dot): 1 SELECT, 7 TUPLE,
0 KVINDEX, 2 JOIN, 0 MAP, 0 AGG, 1 MERGE, 0 NEGATE, 0 COMPARE, 1 INSERT
= 12 views. So the det_seq blocks fall out as:

    det_seq  kind    block id     .dot node
    0        SELECT  ^select.0    v4387028560  (RECEIVE edge)
    1..7     TUPLE   ^tuple.1..7  (7 TUPLE nodes; intra-kind order §2.3)
    8        JOIN    ^join.8      v4387025776  (EQ SET 9, arm2 tc-first)
    9        JOIN    ^join.9      v4387030656  (EQ SET 10, arm3 edge-first)
    10       MERGE   ^merge.10    v4387021296  (UNION tc, TABLE 4)
    11       INSERT  ^insert.11   v4387036480  (MATERIALIZE out)

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

Arm identity confirmed against the .ir fixpoint joins:
- .ir:72-81 (first join): pivot X, tc via `%index:55[_,u64] where %col:6`
  (tc keyed on col6=B) ⋈ edge via `%index:56[u64,_] where %col:9` (edge
  keyed on col9=A), result `+recursive {@A:59,@X:60}` — this is
  tc.B==edge.A, i.e. arm2 (tc-first). Matches ^join.8's DOT wiring
  (v4387025776 p0→tuple6.c8=B, p1→tuple3.c11=A).
- .ir:87-96 (second join): pivot X, tc via `%index:47[u64,_] where
  %col:5` (tc keyed on col5=A) ⋈ edge via `%index:48[_,u64] where
  %col:10` (edge keyed on col10=X), result `+recursive {@A:51,@B:52}` —
  edge.X==tc.A, i.e. arm3 (edge-first). Matches ^join.9's DOT wiring.

### 2.3 Every block → its .dot evidence

DOT edges are drawn use→def (`consumer:port -> producer:col`,
Format.cpp: JOIN 377/387, MERGE 706, INSERT 620, TUPLE via the generic
port linker); the §1 `=>` tail-calls INVERT them to def→use.

- **^select.0** — `v4387028560 [ ... RECEIVE ... port="c1">A ... port="c2">X ]`
  (.dot:8), edge `v4387028560 -> t4387021104` (.dot:7, the I/O edge
  relation). Cols c1=A, c2=X (edge's two params; the second is named X in
  the graph). class=table-less (no TABLE annotation), STRATUM 0. Pushes to
  the three edge-guard TUPLEs (.dot:35-42, all three read c1/c2).
- **^tuple.1** (illustrative id) — the base-case guard. One of the three
  SELECT-fed TUPLEs whose downstream is the MERGE directly: TUPLE EQ SET 1
  STRATUM 1 `v46523434304` (.dot:40), feeds MERGE via .dot:44
  (`v4387021296 -> v46523434304`, MERGE fan-in). This is `tc(A,B):edge(A,B)`
  — edge's X column becomes tc's B. class=table-less, STRATUM 1.
- **^tuple.2** (illustrative id) — TUPLE EQ SET 1 STRATUM 2 `v46523433856`,
  TABLE 8 (.dot:37). Reads SELECT (.dot:38-39). Feeds ^join.9's edge side
  (.dot:15-18: v4387030656 p0→c14=X, p2→c13=A). class=monotone (edge is a
  plain non-differential #message), table=%table:8, STRATUM 2.
- **^tuple.3** (illustrative id) — TUPLE EQ SET 1 STRATUM 3 `v46523433408`,
  TABLE 8 (.dot:34). Reads SELECT (.dot:35-36). Feeds ^join.8's edge side
  (.dot:10-13: v4387025776 p1→c11=A, p3→c12=X). class=monotone,
  table=%table:8, STRATUM 3.
- **^tuple.4** (illustrative id) — TUPLE EQ SET 2 STRATUM 4 `v4387023856`
  (.dot:22). Reads ^join.8 outputs (.dot:23-24: p0→join EQ9 c18=A, p1→c19=B).
  Feeds MERGE (.dot:45). arm2 result row. class=table-less, STRATUM 4.
- **^tuple.5** (illustrative id) — TUPLE EQ SET 3 STRATUM 4 `v4387024304`
  (.dot:25). Reads ^join.9 outputs (.dot:26-27: p0→join EQ10 c21=A, p1→c22=B).
  Feeds MERGE (.dot:46). arm3 result row. class=table-less, STRATUM 4.
- **^tuple.6** (illustrative id) — TUPLE EQ SET 11 STRATUM 4 `v4387024752`,
  TABLE 4 (.dot:28). The tc read-back shared by BOTH joins. Reads MERGE
  (.dot:29-30: p0→merge c23=A, p1→c24=B). Feeds ^join.8.lhs (.dot:10,12:
  c8=B,c7=A) AND ^join.9.lhs (.dot:16,18: c7=A,c8=B). table=%table:4,
  class=differential, STRATUM 4.
- **^tuple.7** (illustrative id) — TUPLE EQ SET 11 STRATUM 5 `v46523432960`
  (.dot:31). Reads MERGE (.dot:32-33). Feeds INSERT (.dot:20-21). The
  out-materialize path. class=table-less, STRATUM 5.
- **^join.8** — `v4387025776` JOIN, EQ SET 9, SET 0 DEPTH 1, STRATUM 4
  (.dot:9). pivot X=c17, out c18=A/c19=B. Ports p0=B,p1=A (pivot inputs),
  p2=A,p3=X (merged outputs) → .dot:10-13. class=table-less, STRATUM 4.
- **^join.9** — `v4387030656` JOIN, EQ SET 10, SET 0 DEPTH 1, STRATUM 4
  (.dot:14). pivot X=c20, out c21=A/c22=B. Ports p0=X,p1=A, p2=A,p3=B →
  .dot:15-18. class=table-less, STRATUM 4.
- **^merge.10** — `v4387021296` UNION "tc", TABLE 4, SET 0 DEPTH 1, STRATUM
  4, EQ SET 11 (.dot:43). MergedViews (fan-in) = ^tuple.1, ^tuple.4,
  ^tuple.5 (.dot:44-46). Pushes to ^tuple.6 (tc read-back) and ^tuple.7
  (out path). table=%table:4, class=differential, STRATUM 4.
- **^insert.11** — `v4387036480` MATERIALIZE "out", TABLE 4, STRATUM 6, EQ
  SET 11 (.dot:19). Terminal. Input ports c0/c1 are INPUT-POSITION indices
  (Format.cpp:598-601, NOT finalized col ids) reading ^tuple.7 (.dot:20-21:
  c0→c9=A, c1→c10=B). table=%table:4, class=differential, STRATUM 6.

### 2.4 Firm vs illustrative

- FIRM (derived from code + .dot + .ir, not guessable-away):
  - The kind sequence and the resulting det_seq BANDS (SELECT=0,
    TUPLE=1..7, JOIN=8..9, MERGE=10, INSERT=11) — from Query.h:1176-1214
    + the census.
  - The two JOIN ids (^join.8=arm2/tc-first, ^join.9=arm3/edge-first) and
    the REASON (§2.2) — the whole point of the witness.
  - The MERGE id (10) and INSERT id (11) — singletons in their kinds.
  - The SELECT id (0) — singleton.
  - Every column name/type, every edge (def→use), every TABLE id, class,
    and stratum — read directly from the .dot / .ir.
- ILLUSTRATIVE-PREDICTED (house precedent: witness-deltarel-target.md op
  ids):
  - The seven TUPLE ids 1..7 (intra-TUPLE-kind creation order). The .dot/
    .ir expose no creation-order signal for TUPLEs (EQ SET is an
    equivalence-class id, Format.cpp:56, NOT a mint counter; STRATUM is a
    post-hoc topological layer). The §1 assignment groups them by role
    (edge-guards first, then join-results, then tc-readbacks) as a
    plausible creation order, but the ACTUAL 1..7 permutation must be read
    from the built graph when the emitter lands. Their det_seq BAND (1..7,
    strictly between SELECT and JOIN) is firm; the permutation inside it is
    not. A regression-gate `.irgold` would freeze whatever the emitter
    actually prints — this artifact's TUPLE ids are the desired SHAPE, and
    the bless step pins the exact permutation.

## §3 — Open questions / spec frictions (LOUD — feed the critique round)

F1. **INTRA-KIND det_seq is UNSPECIFIED by both spec and .dot/.ir.** The
    spec (§1.2) pins block id = det_seq = ForEachView order, but does not
    address that the seven TUPLEs' relative order is a DefList creation
    order I cannot reconstruct from the ground-truth dumps. This is the
    single biggest uncertainty in §1. It is NOT the determinism witness
    (the JOIN pair is), so it does not weaken the artifact's PURPOSE — but
    the .irgold sidecar WILL freeze these ids, so the emitter's actual
    permutation must be captured at bless. RECOMMENDATION: the critique
    round should either (a) accept that ILLUSTRATIVE TUPLE ids are pinned
    at bless (house precedent), or (b) have the implementer dump the built
    graph once and amend §1 with the real permutation before blessing.

F2. **The `class=` vocabulary is under-specified.** Spec §1.3 names
    `class=` values {differential, monotone, table-less} but the mapping
    from a QueryView to that label is not pinned to an accessor. I inferred
    it: a view WITH a TABLE annotation is differential iff its relation can
    receive deletions (tc, recursive → differential; edge, plain #message
    → monotone), and a view with NO TABLE is table-less. Two edge cases the
    spec should nail: (i) is a table-BACKED monotone view `class=monotone`
    or `class=monotone table=%table:8` (I chose to print BOTH the table and
    the class — see ^tuple.2/3)? (ii) ^tuple.6 and ^merge.10 are
    table-less-vs-differential ambiguous — I labeled the TABLE-4-backed
    ones differential and printed table=%table:4. The implementer must map
    `class=` to a concrete QueryView predicate (CanReceiveDeletions() /
    CanProduceDeletions() / a table-presence test).

F3. **INSERT input ports: index vs finalized col id.** The DOT INSERT
    ports are input-POSITION indices c0/c1 (Format.cpp:598-601), NOT
    finalized column ids — unlike every other view whose DOT ports are
    col.Id(). Spec §1.3 says columns render as `<var-or-cN>:<type>` with N
    the finalized id, but the INSERT has no output columns (it is a sink).
    I rendered the INSERT's terminal line with the incoming var NAMES
    (A,B) and used `c0=A, c1=B` on the caller's `=>` line into it. The spec
    should state explicitly how an INSERT (no output cols) renders its
    header and how a caller addresses its input positions (by index? by the
    incoming view's col id?). This is a real emitter decision.

F4. **JOIN port rendering convention is only sketched.** ir-dump-formats
    §1.2 shows `.lhs <- ^merge.4 (F, X=T)` but a JOIN has THREE column
    roles (pivot inputs, per-side; and merged non-pivot outputs) and the
    sketch's single-line-per-side form loses the pivot-vs-output
    distinction. I expanded it (§1) to a `pivot X <- A.col, B.col` line
    plus per-side output lines, because symrec_tie_1's whole point is that
    the two arms differ ONLY in which side supplies the pivot. The spec
    should ratify a JOIN block grammar rich enough to make the arm
    asymmetry visible — a flattened `.lhs/.rhs (colmap)` form would render
    the two symmetric arms nearly identically and blunt the witness.

F5. **SELECT column naming: the second edge column is `X`, not `B`.** The
    graph names edge's params A and X (not the declared A,B) because the
    var propagated from the recursive bodies (edge(X,B), edge(A,X)) renamed
    it. The spec's `<var-or-cN>` rule takes whatever the built graph
    carries; I used `X` to match the .dot. Flagging so the critique round
    knows the header vars are GRAPH vars, not DECLARATION param names — a
    reader expecting `edge(A,B)` will see `(A:u64, X:u64)`.

F6. **`producer=` never appears here — cannot validate that surface.** This
    program has no pass-minted views, so the load-bearing D1-review
    `producer=` line (ir-dump-formats §1 conventions; spec §1.3 debug-only,
    E-52) is untestable from symrec_tie_1. The demand_tc_witness desired-
    state artifact must be the one that exercises `producer=DEMAND-GUARD`.
    Noted so the critique round does not treat this artifact as covering
    that surface.

F7. **`table=%table:4` on the query-`out` INSERT is surprising.** The .dot
    shows the MATERIALIZE-out INSERT annotated TABLE 4 (tc's table), and
    the .ir declares only %table:4 and %table:8 — there is no distinct
    table for the `out` query. So `out` materializes THROUGH tc's table 4.
    I rendered `table=%table:4` on ^insert.11 to match, but the critique
    round should confirm this is the intended TableId() semantics (out
    sharing tc's backing store) and not an artifact of the frozen binary's
    annotation. If TableId() for the out-INSERT should instead be "none",
    §1's insert ATTRIBUTES line changes.

F8. **`; back-edge` marker = det_seq rule, NOT fixpoint semantics — they
    DIVERGE here.** Spec §1.3 defines the `; back-edge` comment mechanically
    as "user det_seq <= def det_seq". Applying that literally, the ONLY
    back-edges in symrec_tie_1 are ^merge.10's two `=>` lines (to ^tuple.6
    id 6 and ^tuple.7 id 7, both < 10) — which I marked. But the TRUE
    fixpoint back-edges of the recursion (the join results ^tuple.4/^tuple.5
    feeding MERGE, and MERGE feeding the tc read-back ^tuple.6) do not all
    coincide with the det_seq rule: ^tuple.4 (4)→^merge.10 (10) is
    det_seq-FORWARD yet is the semantic cycle edge. So the mechanical marker
    UNDER-reports the cycle. This is inherent to emitting in det_seq (non-
    topological) order (spec §1.3 accepts this: "ids, not layout, carry
    identity"). Flagging so the critique round does not read `; back-edge`
    as a cycle map — it is a print-order artifact. If a cycle-accurate
    annotation is wanted, that is a distinct feature (a `; cycle-edge`
    comment keyed on actual induction back-edges), not the det_seq marker.

