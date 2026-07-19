# T2 desired-state — `-df-out` of `tests/OptDiff/cases/transitive_closure.dr`

Hand-written EXACT desired text for the future `-df-out` DataFlow BB-with-args
dump of the non-linear transitive-closure case. Implementation later means
"make the compiler print exactly the §1 block, gated by strict byte-diff".

--------------------------------------------------------------------
## §0 Provenance

INPUTS READ THIS SESSION (all at branch keyed-instances tip b577735e):
- SPEC (binding): `docs/proposals/KeyedInstances.artifacts/t2-dump-spec.md`
  §1 (T2a `-df-out`), read in full. STATUS in that file: DRAFT pending owner
  ratification of decisions (a)-(d); this desired-state is written against it.
- FORM it refines: `docs/proposals/KeyedInstances.artifacts/ir-dump-formats.md`
  §1 (the BB-with-arguments sketch + conventions), read in full.
- VERIFIED as-landed record:
  `scratchpad/fleet-ckpt/consolidated.md` §1.A (det_seq facts), §1.B
  (surfaces), §3 (reconciliation), §4.a/§4.b (id-space + dump-position facts).
- GROUND TRUTH of the post-optimize graph (generated this session from the
  FROZEN b577735e binary):
  - GraphViz: `scratchpad/dump-inputs/transitive_closure.dot`
  - ControlFlow IR: `scratchpad/dump-inputs/transitive_closure.ir`
  - source: `tests/OptDiff/cases/transitive_closure.dr`
- id-rule code re-read this session:
  - `lib/DataFlow/Query.h:1176-1214` — `QueryImpl::ForEachView` (the per-kind
    DefList iteration order — the det_seq / FinalizeColumnIDs traversal).
  - `lib/DataFlow/Columns.cpp:13-25` — `FinalizeColumnIDs` (col ids assigned
    in ForEachView order, `next_col_id` from 1, sequential per view/column).
  - `lib/DataFlow/Insert.cpp:24-34` — `KindName`: MATERIALIZE = query INSERT.
  - `lib/DataFlow/Query.h:472,534` — `det_seq{~0u}`, `producer` field.

BINARY-HASH SOURCE: the .dot/.ir were dumped from the compiler built at
b577735e (the frozen witness binary; the fleet's re-derivation baseline).

SPEC VERSION: t2-dump-spec.md as of b577735e (DRAFT). Decisions this
desired-state COMMITS TO (per spec recommendations): (a) block id = det_seq,
rendered `^<kind>.<det_seq>`, blocks in ascending det_seq order; (b) drain
post-Program (so `table=%table:N` is populated); §1.3 column naming
`<var-or-cN>:<type>` with N = finalized column id.

--------------------------------------------------------------------
## §1 THE DESIRED `-df-out` TEXT (complete — nothing elided)

```
;; module: transitive_closure
;;   #message add_edge(u64 From, u64 To).
;;   #query   reachable_from(bound u64 From, free u64 To).
;;   #query   reaching_to(free u64 From, bound u64 To).
;;   #query   is_node(free u64 Node).
;;   tc(From, To) : tc(From, X), tc(X, To).
;;   tc(From, To) : add_edge(From, To).
;;   reachable_from(From, To) : tc(From, To).
;;   reaching_to(From, To) : tc(From, To).
;;   is_node(Node) : tc(Node, _).
;;   is_node(Node) : tc(_, Node).

recv ^select.0 () -> (From:u64, To:u64)          ; #message add_edge/2
  ATTRIBUTES class=table-less stratum=0
  => ^tuple.9 (From, To)

tuple ^tuple.1 (From:u64, To:u64)                ; callers: ^join.10
  ATTRIBUTES class=table-less stratum=2
  => ^merge.11 (From, To)

tuple ^tuple.2 (From:u64, To:u64)                ; callers: ^merge.11
  ATTRIBUTES table=%table:4 class=monotone stratum=2
  => ^join.10 .rhs(AutoVar_2=From, Node=To)

tuple ^tuple.3 (From:u64, To:u64)                ; callers: ^merge.11
  ATTRIBUTES table=%table:4 class=monotone stratum=2
  => ^join.10 .lhs(From, X=To)

tuple ^tuple.4 (Node:u64)                        ; callers: ^merge.11 (Node=From)
  ATTRIBUTES class=table-less stratum=3
  => ^merge.12 (Node)

tuple ^tuple.5 (Node:u64)                        ; callers: ^merge.11 (Node=To)
  ATTRIBUTES class=table-less stratum=4
  => ^merge.12 (Node)

tuple ^tuple.6 (From:u64, To:u64)                ; callers: ^merge.11
  ATTRIBUTES class=table-less stratum=5
  => ^insert.13 (From, To)

tuple ^tuple.7 (From:u64, To:u64)                ; callers: ^merge.11
  ATTRIBUTES class=table-less stratum=6
  => ^insert.14 (From, To)

tuple ^tuple.8 (Node:u64)                        ; callers: ^merge.12
  ATTRIBUTES class=table-less stratum=8
  => ^insert.15 (Node)

tuple ^tuple.9 (From:u64, To:u64)                ; callers: ^select.0
  ATTRIBUTES class=table-less stratum=1
  => ^merge.11 (From, To)

join ^join.10 [pivot X:u64] {
  .lhs <- ^tuple.3  (From, X)                     ; tc(From, X)
  .rhs <- ^tuple.2  (X=AutoVar_2, To=Node)        ; tc(X, To)
} -> (From:u64, To:u64)
  ATTRIBUTES class=table-less stratum=2
  => ^tuple.1 (From, To)                          ; back-edge

merge ^merge.11 (From:u64, To:u64)               ; callers: ^tuple.1, ^tuple.9
  ATTRIBUTES table=%table:4 class=monotone stratum=2 set=0 depth=1
  => ^tuple.2 (From, To)                          ; back-edge
  => ^tuple.3 (From, To)                          ; back-edge
  => ^tuple.4 (Node=From)                         ; back-edge
  => ^tuple.5 (Node=To)                           ; back-edge
  => ^tuple.6 (From, To)                          ; back-edge
  => ^tuple.7 (From, To)                          ; back-edge

merge ^merge.12 (Node:u64)                        ; callers: ^tuple.4, ^tuple.5
  ATTRIBUTES class=table-less stratum=7
  => ^tuple.8 (Node)                              ; back-edge

insert ^insert.13 (From:u64, To:u64) into reachable_from
  ATTRIBUTES table=%table:4 class=monotone stratum=9
  ; terminal (MATERIALIZE #query reachable_from)

insert ^insert.14 (From:u64, To:u64) into reaching_to
  ATTRIBUTES table=%table:4 class=monotone stratum=10
  ; terminal (MATERIALIZE #query reaching_to)

insert ^insert.15 (Node:u64) into is_node
  ATTRIBUTES table=%table:8 class=monotone stratum=11
  ; terminal (MATERIALIZE #query is_node)
```

--------------------------------------------------------------------
## §2 Derivation notes

### 2.1 The det_seq (block-id) ordering rule — how it was applied

Block id = `det_seq` = a view's position in `ForEachView` order at the last
stamp (spec §1.2, decision (a)). `ForEachView` (Query.h:1176-1214) iterates the
per-kind DefLists in THIS FIXED KIND SEQUENCE:

    selects → tuples → kv_indices → joins → maps → aggregates
           → merges → negations → compares → inserts

skipping `is_dead` views. Within a kind, order = DefList insertion (creation)
order. det_seq is the flat 0-based counter across that whole traversal.

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

Node aliases below use the .dot raw pointer names (IGNORE the names per the
brief; they anchor the evidence quote only). All columns u64 (.ir table
%table:4[u64,u64] %col:5=From %col:6=To/X/Node; %table:8[u64] %col:9=Node).

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

^tuple.2  tc read (rhs of join) — DOT:29
  `v4391456096 [TABLE 4 STRATUM 2 EQ SET 12 TUPLE c5=AutoVar_2 c6=Node]`.
  Reads UNION tc: DOT:30-31 `:p0->v4391459808:c21`, `:p1->:c22`. Caller
  ^merge.11. Feeds the JOIN rhs: DOT:15,17 `v4391460368:p1->v4391456096:c5`
  (AutoVar_2→join p1), `:p3->:c6` (Node→join p3). TABLE 4 → class=monotone,
  table=%table:4. stratum=2. Forward edge is a back-edge (user det_seq 10 >
  def 2 is forward — NOT a back-edge; see 2.3).

^tuple.3  tc read (lhs of join) — DOT:32
  `v30555635712 [TABLE 4 STRATUM 2 EQ SET 12 TUPLE c7=From c8=X]`. Reads
  UNION tc: DOT:33-34. Feeds JOIN lhs: DOT:14,16 `:p0->v30555635712:c8`
  (X→join p0), `:p2->:c7` (From→join p2). TABLE 4 → monotone. stratum=2.

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
  p3(Node)←tuple.2.c6. So lhs=tuple.3 contributes {pivot X (p0), payload
  From (p2)}, rhs=tuple.2 contributes {pivot AutoVar_2 (p1), payload
  Node→To (p3)}. Pivot X. Output From←p2, To←p3. lhs/rhs bound to DOT port
  order (p0/p2 producer = lhs, p1/p3 producer = rhs) — see §3 friction F3.
  Feeds ^tuple.1: DOT:27-28. table-less (no TABLE annotation). stratum=2.

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
  tables point AT the inserts (DOT:4-9: t30555588288->v30543020544, etc.).
  INSERTs are terminal (no output columns, no `=>`). MATERIALIZE = a query-
  result INSERT (Insert.cpp:24-26). classes monotone; table=%table:4 (rf,
  rt) / %table:8 (is_node) from the DOT TABLE annotations. strata 9/10/11.

### 2.3 back-edge marks (spec §1.3: mark when USER det_seq <= DEF det_seq)

Because det_seq is NOT topological (spec §1.3 note), many forward-in-dataflow
edges are "back" by id. Computed per edge:
  ^join.10 => ^tuple.1     : user 1  <= def 10 → back-edge
  ^merge.11 => ^tuple.2    : user 2  <= def 11 → back-edge
  ^merge.11 => ^tuple.3    : user 3  <= def 11 → back-edge
  ^merge.11 => ^tuple.4    : user 4  <= def 11 → back-edge
  ^merge.11 => ^tuple.5    : user 5  <= def 11 → back-edge
  ^merge.11 => ^tuple.6    : user 6  <= def 11 → back-edge
  ^merge.11 => ^tuple.7    : user 7  <= def 11 → back-edge
  ^merge.12 => ^tuple.8    : user 8  <= def 12 → back-edge
All other `=>` edges have user det_seq > def det_seq (forward, unmarked):
  select.0=>tuple.9, tuple.1=>merge.11, tuple.2=>join.10, tuple.3=>join.10,
  tuple.4=>merge.12, tuple.5=>merge.12, tuple.6=>insert.13,
  tuple.7=>insert.14, tuple.8=>insert.15, tuple.9=>merge.11.
NOTE the true SEMANTIC cycle is exactly merge.11 ↔ {tuple.2,tuple.3} → join.10
→ tuple.1 → merge.11 (the tc fixpoint, DOT SET 0 DEPTH 1). The id-based
back-edge mark over-reports (it flags merge.11=>tuple.6/tuple.7, which exit the
cycle into the query sinks) — this is the documented det_seq-not-topological
behavior, not an error.

### 2.4 `=>` edge ordering (spec §1.3: by (user det_seq, port))

Within each block the `=>` lines are sorted by (user det_seq, then user port).
merge.11's six users sort tuple.2,3,4,5,6,7 — already ascending det_seq, one
port each (single-column feeds get one line; two-column feeds one line with the
column list). This is why merge.11 lists its edges 2→3→4→5→6→7.

### 2.5 Firmness summary
  FIRM (col-id-proven): all det_seq 0..12; every column name/id; every edge
    src/dst and its column map (read straight from .dot ports); every TABLE
    annotation (6 nodes, re-grepped); every stratum (from .dot STRATUM);
    every back-edge mark (arithmetic on firm det_seq).
  ILLUSTRATIVE-PREDICTED: insert.13/14/15 relative ids (ordered by clause +
    stratum, but no col-id witness); the exact spelling of headers/attribute
    punctuation (spec-shape, not graph-derived); lhs/rhs labels on the join
    (see §3 F3); `set=0 depth=1` punctuation.

--------------------------------------------------------------------
## §3 Open questions / spec frictions (LOUD — feed the critique round)

F1. INSERT det_seq ids are UNWITNESSED. INSERTs carry no output columns, so
    FinalizeColumnIDs gives them no col id — the one place the .dot cannot
    confirm det_seq. I predicted 13<14<15 from clause order + strata (9<10<11),
    but the ONLY way to freeze this is to read the inserts DefList directly (or
    dump det_seq itself). RECOMMENDATION for the emitter: when `-df-out` lands,
    the FIRST bless of this golden must be reviewed against a det_seq print, not
    trusted from this artifact. If the DefList order differs, only the three
    insert.13/14/15 ids (and the `=> ^insert.N` targets in tuple.6/7/8) move.

F2. `producer` line is SPEC-CONDITIONAL and this graph has NONE. Spec §1.3 says
    producer prints only when the field is non-empty (debug builds; e.g.
    DEMAND-* tags). transitive_closure is compiled WITHOUT -demand, so no view
    carries a producer tag — I emitted no producer line anywhere. FRICTION: the
    spec says T3 goldens are blessed from the DEBUG preset and producer "IS
    golden-visible" — but for THIS case debug and release agree (empty). A
    reviewer must not expect a producer line here; it only appears on demand-ON
    / pass-minted cases (demand_tc_witness is where that surface actually shows).

F3. JOIN lhs/rhs labels are a NAMING CHOICE not carried by the graph. The .dot
    exposes only ordered ports p0..p3 and their producers; "lhs"/"rhs" is the
    ir-dump-formats §1 sketch vocabulary. I bound lhs = the p0/p2 producer
    (^tuple.3, the From+pivot side) and rhs = the p1/p3 producer (^tuple.2, the
    To side). This is defensible (DOT port order) but the emitter could equally
    key lhs/rhs off input-view index or joined_views order and flip them. The
    STRUCTURE (pivot X; From←p2; To←p3; the two producers) is firm; the LABELS
    are not. Spec should pin: are the join input ports rendered by (a) their
    p-index (p0..p3, fully faithful, ugly), or (b) lhs/rhs role labels (pretty,
    but needs a defined rule for which producer is lhs)? I chose (b); flag for
    ratification.

F4. TUPLE block-param convention under RENAME. tuple.4/tuple.5 receive tc.From /
    tc.To respectively but OUTPUT a column named Node (finalized c9/c10). I
    render block params = OUTPUT columns (`(Node:u64)`) and show the rename ONLY
    at the caller edge (`=> ^tuple.4 (Node=From)`) + a `callers: ^merge.11
    (Node=From)` hint. The ir-dump-formats §1 sketch never showed a renaming
    tuple, so this is an unratified refinement. Alternative the spec could pick:
    print `(From:u64) -> (Node:u64)` on the tuple itself (input->output). I
    rejected that because it double-encodes the rename (also on the edge) and
    breaks the "params = the columns the block RECEIVES-as-its-identity /
    finalized-col-id" rule. NEEDS a one-line spec ruling.

F5. `class=` for an INSERT-ONLY (monotone) program. The whole case is monotone:
    add_edge is a #message with no deletion, the .ir shows only +recursive /
    +nonrecursive update-counts and no overdelete/rederive, and the .dot colors
    NO node purple (nothing CanReceiveDeletions). So every table-backed view is
    class=monotone, NOT differential — even though tc is recursive (it uses the
    induction/fixpoint machinery). FRICTION: "differential" (spec §1.3's three
    classes) keys off deletion-capability, NOT recursion. A reviewer expecting
    the recursive tc UNION to read "differential" would be wrong. The spec's
    three-class vocabulary (differential/monotone/table-less) should note that
    recursion alone does NOT imply differential. (If the emitter instead keys
    `class` off "has an induction/fixpoint", every label on %table:4 flips to
    differential and this golden is wrong — that ambiguity must be resolved
    before first bless.)

F6. The `callers:` hint line is an ADDITION beyond the spec §1.3 letter. The
    ir-dump-formats §1 sketch shows `; callers: ^select.1, ^tuple.9` on MERGE
    (fan-in). I extended it to every non-source block (so a reader can navigate
    up as well as down without a second pass). Cheap, but not spec-mandated —
    ratify or drop. If dropped, remove every `; callers:` comment (no id/edge
    impact).

F7. `set=0 depth=1` on the tc UNION. The .dot prints `SET 0 DEPTH 1` (the
    induction-set watermark / InductiveSet id + depth). I surfaced it as
    `set=0 depth=1` on the ATTRIBUTES line since it is load-bearing for
    fixpoint review (which UNION owns the induction). Spec §1.3's ATTRIBUTES
    list (table/class/stratum/producer) does not mention it. Add to the spec or
    drop; if kept, pin the spelling.

F8. Module header comment block (`;; module: ... ;; <clauses>`) is my
    invention (ir-dump-formats §1.2 sketch opened with a single `;; module:`
    comment). Provenance-in-comments is spec Q2 ("only where parse links
    survive") — post-optimize these clause strings are NOT reliably
    reconstructable from the Query graph, so if the emitter cannot cheaply
    recover them it should print only `;; module: transitive_closure` (or the
    module name alone). I included the full clause list for reviewer legibility;
    it is the MOST likely part of §1 to not survive contact with the emitter.
    Treat the `;;` header as illustrative, the blocks as the contract.

F9. Emission ORDER vs BACK-EDGE noise. det_seq order (spec decision) means the
    dump is not topological and 8 of 18 edges carry `; back-edge`, several of
    which (merge.11=>tuple.6/tuple.7) are not part of any real cycle — they just
    happen to target lower ids. This is spec-intended (§1.3: "ids, not layout,
    carry identity") but is genuinely noisy for a human reviewer of a recursive
    program. Non-blocking, but worth an owner eyeball: is the `; back-edge`
    mark's stated purpose (flag true fixpoint edges) served when it over-reports
    on a non-topological ordering? Consider marking only edges INTO a view in
    the same induction SET, or renaming the comment to `; id<=` to avoid
    implying "fixpoint back-edge".
