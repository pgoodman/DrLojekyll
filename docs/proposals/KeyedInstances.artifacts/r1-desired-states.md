======================================================================
COMMITTED AT THE R1 LANDING (2026-07-22). The desired-output-states
contract: xhigh writer + E-71 grammar lane + fresh adversarial critic
+ xhigh adjudicator; DS-ADJ-1..6 applied IN PLACE (DS-ADJ-1: census
mode-stability holds ONLY on the controlflow axis — the df axis
legitimately grows the walk). DS-ADJ-7 (post-bless, from the Fable
review): the eager table= RENDER AUTHORITY is the union-find MERGED
model, never the .df per-view attribute — §2.2's per-block table
attributions are SUPERSEDED by the as-blessed goldens; the A.6(c)
table-match validator enforces the merged-model authority always-on.
======================================================================

# R1 desired-states — the eager-web TUPLE-forward + terminal-INSERT ops in the .deltarel dump

Ritual stage (d), xhigh. Derived from `r1-design.md` (GO-WITH-AMENDMENTS,
ADJ-S1..S14 applied in place) reconciled against the code at tip **6d695aec**
and against LIVE tip-baseline dumps compiled below. This document pins the
POST-R1 desired output states for the two eager-web carriers
(`demand_tc_witness`, `symrec_tie_1`) and the corpus-wide unchanged-surface
claim, with every line marked **[BYTE]** (committed byte-exact) or **[STRUCT]**
(shape-committed; value bless-pinned — each justified).

R1 recap (design §0): model the monotone eager web's TUPLE-forward dispatch
(`Build.cpp:1150`, `BuildEagerTupleRegion`) and terminal-INSERT dispatch
(`Build.cpp:1154`, `BuildEagerInsertRegion`) as two new effect-free DR-IR op
kinds — `kEagerForward` (enum 18) / `kEagerInsert` (enum 19) — minted at the
walk (`RecordEagerDispatch`) and enrolled into `flow.ops` by a new EAGER_WEB
block appended STRICTLY AFTER the INGEST_FOLD loop (ADJ-S2). PURE MODELING +
LOWERING-IN-PLACE: **zero emission change** (region builders CALLED unchanged at
the unchanged walk moment; op ctor/record/enroll mint no `impl->next_id`). The
ONLY output byte that moves anywhere in the corpus is the `.deltarel` dump.

---

## THE CENTRAL HONEST FACT (why the eager-op blocks are irreducibly [STRUCT])

The `.deltarel` census counts **raw walk dispatches**, computed inside
`BuildStratumPhases` during `Program::Build` (F-ORDER), which runs BEFORE
`ProgramImpl::Optimize` (controlflow-opt). So:

1. The eager-op count is a property of the **live walk execution** —
   `BuildEagerTupleRegion` / `BuildEagerInsertRegion` CALL counts — not of any
   dumpable tip artifact. R1 is not implemented, so no tip dump exposes it. The
   `.ir` (post-controlflow-opt) UNDER-counts because region dedup/CSE collapses
   duplicate folds AFTER the census is fixed; the `.df` gives the view/edge
   substrate but not the per-dispatch multiplicity.
2. The walk **re-descends SCC members per incoming edge / per
   init-vs-cycle context**. Proven live below: `demand_tc_witness`'s single
   INSERT view `insert.19` folds into `%table:4` at **two** distinct post-opt
   sites (`tc/ir.opt.out:187` inside the induction body, `:206` at the
   top-level flow) — so `kEagerInsert` for that one view is dispatched ≥2×, and
   a naive one-op-per-view count is provably WRONG.

Consequently the exact `<F>`/`<I>` and the per-table eager-block multiplicities
are **[STRUCT]**, pinned at bless by the ADJ-S10 structural count read (§5). What
IS derivable and committed **[BYTE/STRUCT-shape]**: the render-ORDER law (which
table blocks, in which order, eager-before-ingest-fold within a shared table —
§3), the per-op header/args GRAMMAR, the sink=relation invariant, and every
unchanged surface (ingest folds, `rounds:`, `deps:`, census columns 0–17).
This matches the design: D.3 calls the counts "structural upper-ish, NOT
verified," OQ1 pins them at bless, and ADJ-S10 makes the bless-time structural
read the SOLE correctness check on the two new census columns.

---

## (1) TIP BASELINES (frozen 6d695aec, compiled LIVE at opt with all sinks)

Reproduce exactly as `runall.sh run_irgold` does (opt mode = no opt flags +
`.drflags`; all four sinks in ONE compile — ADJ-S7 bless-provenance):

```
DR=build/debug/bin/drlojekyll
# symrec_tie_1: no .drflags
$DR cases/symrec_tie_1.dr        -df-out df -deltarel-out deltarel -ir-out ir -cpp-out cpp
# demand_tc_witness: .drflags = "-demand"
$DR cases/demand_tc_witness.dr -demand -df-out df -deltarel-out deltarel -ir-out ir -cpp-out cpp
```

### 1a. `symrec_tie_1` — tip `.deltarel` (VERBATIM, 12 lines; NO committed golden today) [DS-ADJ-6, was "15"]

```
deltarel

op.0 kIngestFold sign=+ ctx=eager stratum=0
    effects: {kCounter(%table:8, +, NonRecursive)}
    spine: —
    args: table=%table:8 message=edge/2

rounds:

deps:

census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=1 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0
```

One ingest fold (message `edge/2` → `%table:8`). `symrec_tie_1` carries NO
`.deltarel` golden at tip (its `.irgold` sidecar pins only `ir opt` / `df opt`);
ADJ-S9 ADDS `deltarel opt` and blesses this file's POST-R1 successor (§3a, §5).

### 1b. `demand_tc_witness` — tip `.deltarel` (VERBATIM = the committed golden, 16 lines)

Byte-identical to `goldens/demand_tc_witness.deltarel.opt.golden` (verified
`cmp` clean against the live compile):

```
deltarel

op.0 kIngestFold sign=+ ctx=eager stratum=0
    effects: {kCounter(%table:19, +, NonRecursive)}
    spine: —
    args: table=%table:19 message=edge_2/2
op.1 kIngestFold sign=+ ctx=eager stratum=0
    effects: {kCounter(%table:23, +, NonRecursive)}
    spine: —
    args: table=%table:23 message=demand__reachable_from_bf/1

rounds:

deps:

census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=2 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0
```

Two ingest folds: `op.0` message `edge_2/2` → `%table:19`; `op.1` fabricated
demand seed `demand__reachable_from_bf/1` → `%table:23`. `rounds:`/`deps:` empty
(the recursion is emitted by the hand-coded eager+induction path, NOT the DR
fixpoint machinery — F-ORDER). This is the ONLY committed `.deltarel` golden R1
re-blesses.

---

## (2) EAGER-OP INVENTORY — derivation per carrier

### 2.0 The dispatch grain (one op per activation), and the cut rules

From the code (re-read at tip): the walk begins at each message receive
(`ExtendEagerProcedure`, `Procedure.cpp:14`) which calls
`BuildEagerInsertionRegions(receive, receive.Successors())`. That function
(`Build.cpp:841`) does `InTryInsert(view)` (folds `view` into its own table if
table-bearing — NOT an R1 op, it is the InsertionRegions fold), then for each
successor **that is not a cut** creates a LET and calls `BuildEagerRegion`
(`Build.cpp:1085`). `BuildEagerRegion` dispatches on view kind:

- `IsTuple()` → `BuildEagerTupleRegion` = **one `kEagerForward`**; its body
  (`Tuple.cpp:14`) is `BuildEagerInsertionRegions(tuple, tuple.Successors())` —
  it recurses into the tuple's successors.
- `IsInsert()` → `BuildEagerInsertRegion` = **one `kEagerInsert`**; its body
  (`Insert.cpp:26`) folds into the insert's table, then a sink discriminant:
  stream → publish-now/publish-vec/commit-published; relation →
  `BuildEagerInsertionRegions(insert, insert.Successors())`.
- JOIN / MERGE(inductive→`BuildEagerInductiveRegion`) / MAP / CMP / SELECT /
  AGG-KV(cut,return) → **no R1 op** (their own later Rk; JOIN/MERGE-inductive
  defer to the work list which re-drives `BuildEagerRegion` when built).

**Cut successors** (`Build.cpp:968`, skipped — never descended):
`CanReceiveDeletions()`, `IsAggregate()`, `IsKVIndex()`, or (under
`-demand-instance` only) a recognized-subgraph guard. **Both carriers are FULLY
MONOTONE** (`.df` shows every class `monotone`/`table-less`, zero deletions) and
neither runs under `-demand-instance` (plain `-demand` / no flag), so **NO
successor is ever cut** — every `=>` edge in the `.df` is a live dispatch.

**Target table** of each op = `ModelTableOrNull(view)` (ADJ-S14, `.find()`-guarded)
= the view's own model table from its `.df` `ATTRIBUTES table=` (null when
table-less). This drives `op_table_id` and hence render order (§3).

The one source of count MULTIPLICITY beyond a flat edge count: an inductive
MERGE / cycle JOIN re-descends its downstream TUPLE/INSERT successors **per
incoming edge and per init-vs-cycle context** (proven live in §2.2). So the
edge-count below is a DERIVED LOWER structure, not the final integer.

### 2.1 `symrec_tie_1` — views, tables, and dispatch edges

From `symrec/df.opt.out`. Receive: `edge/2` at `select.0` (table-less → E-42
VECTORLOOP shim) → successors {`tuple.5`,`tuple.6`,`tuple.7`}.

| view | kind | model table | `op_table_id` | dispatched-from (incoming `=>` edges) |
|---|---|---|---|---|
| `tuple.5` | TUPLE | `%table:8` | 9 | `select.0` |
| `tuple.6` | TUPLE | `%table:8` | 9 | `select.0` |
| `tuple.7` | TUPLE | table-less | 0 | `select.0` |
| `tuple.1` | TUPLE | table-less | 0 | `join.8` |
| `tuple.2` | TUPLE | table-less | 0 | `join.9` |
| `tuple.3` | TUPLE | `%table:4` | 5 | `merge.10` (inductive) |
| `tuple.4` | TUPLE | table-less | 0 | `merge.10` (inductive) |
| `insert.11` | INSERT | `%table:4` | 5 | `tuple.4` — into `%table:4`, **relation**, no succs |

Non-op views on the paths: `join.8`,`join.9` (JOIN, deferred — feed
`tuple.1`/`tuple.2`), `merge.10` (inductive MERGE → `BuildEagerInductiveRegion`;
callers `tuple.7`,`tuple.1`,`tuple.2`; successors `tuple.3`,`tuple.4`).

**Derived TUPLE-forward views (7 distinct):** `tuple.1,2,3,4,5,6,7`.
**Derived INSERT view (1 distinct):** `insert.11`.
Edge-lower-structure: `<F>` ≥ 7, `<I>` ≥ 1. The design's D.3 estimate (~7F,
2–3I) is the induction-re-entry UPPER-ish; `merge.10` has 3 callers, so its
successors `tuple.3`/`tuple.4` (and `tuple.4`→`insert.11`) may be re-descended,
lifting `<F>`/`<I>` above the distinct-view floor. **Exact = bless (OQ1).**

Confirming multiplicity live: `symrec/ir.opt.out` folds into `%table:4` at THREE
sites — `:51` (`+nonrecursive`) and `:78`/`:93` (`+recursive`, the two cycle-join
arms) — evidence of SCC re-descent in the `%table:4` activity, so `<F>`/`<I>` for
this carrier exceed the distinct-view floor. See [DS-ADJ-2] on the attribution.

> **[DS-ADJ-2] (adjudicated LOW, UPHELD — crit N-2). Do NOT pin the 3 folds to
> `tuple.3`; attribute them to the `%table:4` PRODUCER SET collectively.** VERIFIED
> at tip: in `symrec_tie_1`, `%table:4` is the model of THREE views — `tuple.3`
> (`table=%table:4`), `merge.10` (`table=%table:4`, callers tuple.1/2/7), and
> `insert.11` (`into %table:4`). So "3 folds into `%table:4`" is aggregate
> SCC-re-descent evidence over that producer set and does NOT establish that
> `tuple.3`'s dispatch specifically fires 3×. (The `+recursive` sites `:78`/`:93`
> ride the cycle-join arms `tuple.3`→join.8/join.9; `:51` `+nonrecursive` is the
> seed path — but which producer's InsertionRegions fold lands at each site is not
> separable from the dump.) CONTRAST the `tc` `<I>≥2` argument in §2.2, which IS
> clean: `%table:4` is `insert.19`'s SOLE modeler in tc (verified — no tuple/merge
> touches `%table:4` there), so its two `update-count` sites (ir:187/:206) both
> pin to `insert.19`. This is [STRUCT]/bless-pinned either way (the ADJ-S10
> structural read is the guard), so the unsound derivation cannot fail first
> compile — the fix is wording only.

### 2.2 `demand_tc_witness` — views, tables, and dispatch edges

From `tc/df.opt.out`. Two receives: `edge_2/2` at `select.0` (table-less shim) →
{`tuple.2`,`tuple.10`}; `demand__reachable_from_bf/1` at `select.1` (table-less
shim) → {`tuple.8`,`tuple.12`}.

| view | kind | model table | `op_table_id` | dispatched-from |
|---|---|---|---|---|
| `tuple.2` | TUPLE | `%table:19` | 20 | `select.0` |
| `tuple.10` | TUPLE | `%table:19` | 20 | `select.0` |
| `tuple.8` | TUPLE | `%table:23` | 24 | `select.1` |
| `tuple.12` | TUPLE | table-less | 0 | `select.1` |
| `tuple.3` | TUPLE | table-less | 0 | `join.13` (cycle) |
| `tuple.11` | TUPLE | table-less | 0 | `join.14` (cycle) |
| `tuple.7` | TUPLE | `%table:15` | 16 | `join.15` |
| `tuple.9` | TUPLE | table-less | 0 | `join.16` |
| `tuple.4` | TUPLE | `%table:8` | 9 | `merge.17` (inductive) |
| `tuple.5` | TUPLE | table-less | 0 | `merge.17` (inductive) |
| `tuple.6` | TUPLE | `%table:12` | 13 | `merge.18` (inductive) |
| `insert.19` | INSERT | `%table:4` | 5 | `tuple.9` — into `%table:4`, **relation**, no succs |

Non-op views: `join.13`,`join.14` (cycle JOINs, deferred), `join.15`,`join.16`
(non-cycle JOINs), `merge.17`(→`%table:8`, inductive, callers `tuple.3`/`tuple.11`,
succs `tuple.4`/`tuple.5`), `merge.18`(→`%table:12`, inductive, callers
`tuple.5`/`tuple.12`, succ `tuple.6`).

**Derived TUPLE-forward views (11 distinct):** `tuple.2,3,4,5,6,7,8,9,10,11,12`.
**Derived INSERT view (1 distinct):** `insert.19`.
Edge-lower-structure: `<F>` ≥ 11, `<I>` ≥ **2** (not 1 — proven below). Design
D.3 estimate: ~11F, 3–4I.

**Multiplicity proven live (the crucial one):** `insert.19`'s fold into
`%table:4` appears TWICE post-opt — `tc/ir.opt.out:187` (inside the induction
body, the `join.16`→`tuple.9`→`insert.19` descent nested under the fixpoint) and
`:206` (at the top-level flow, the same join re-driven outside the induction).
So `kEagerInsert` fires ≥2× for the ONE insert view. This is exactly the
per-context re-descent that makes `<I> > distinct-INSERT-views` and forces the
count to bless. (Design D.3's "3–4 inserts" is this effect, upper-ish.)

---

## (3) POST-R1 `.deltarel` BYTE-DRAFT — both carriers

### 3.0 The per-op GRAMMAR (both carriers) — [BYTE] shape

Fixed by design §C (E-71 adjudicated, ADJ-S1/S3 applied). Every eager op block:

- **`kEagerForward`**, table-bearing view:
  ```
  op.<oi> kEagerForward sign=· ctx=eager stratum=0
      args: table=%table:N
  ```
- **`kEagerForward`**, table-LESS view (ADJ-S3 OMIT-when-null — the `table=`
  token is absent; the `args:` line has no trailing content):
  ```
  op.<oi> kEagerForward sign=· ctx=eager stratum=0
      args:
  ```
- **`kEagerInsert`** (always table-bearing; both carriers `sink=relation`,
  no `message=` arm — see 3.0a):
  ```
  op.<oi> kEagerInsert sign=· ctx=eager stratum=0 sink=relation
      args: table=%table:N
  ```

Byte facts, all [BYTE]: `sign=·` is the multibyte signless glyph `\xC2\xB7`
(`table_op_sign==0`); `ctx=eager`; `stratum=0` (the `DROpStratum` default arm,
no edit); NO `effects:`/`spine:`/`reads:` sublines (effect-free markers, A.2);
`%table:N` is `table->id` = the id the `.ir` prints. `<oi>` is the construction
(enrollment) index = **walk-DFS order**, and because the EAGER_WEB block enrolls
STRICTLY AFTER the two ingest folds (ADJ-S2), every eager op has `oi ≥ 2`
(`demand_tc_witness`) / `oi ≥ 1` (`symrec_tie_1`). Labels are therefore
NON-MONOTONIC in render order (already true of every differential dump).

> **[DS-ADJ-5] (adjudicated LOW, UPHELD — crit N-3 + grammar F4). The no-subline
> shape is guaranteed ONLY by the C.2 DEDICATED render cases; the golden diff
> (E.1#2) is its SOLE tripwire.** The header-plus-`args:`-only shape above is NOT
> enforced by the enum, F-SWITCHTOTAL, or F-CENSUSABORT. VERIFIED at tip: the op
> render switch (Format.cpp:654) carries a `default:` arm (Format.cpp:823). If an
> implementer omits the dedicated `case kEagerForward:`/`case kEagerInsert:`, the
> eager op falls SILENTLY to that default (renders, no abort), which emits
> `emit_effects` → `    effects: {}` and `emit_spine` → `    spine: —` (both
> unconditional — confirmed at Format.cpp:536/476) and emits NO `args:` line at
> all. That is a compiling under/over-render that would be blessed into the golden.
> The ONLY guard is the E.1 criterion-#2 direct-diff referee ("every added line is
> an eager-op block or the census tail — nothing else"), which catches both the
> stray `effects:`/`spine:` lines and the missing `args:`. Treat the two deltarel
> golden diffs (both carriers) as the sole tripwire for the render switch.

**3.0a The `sink=relation` invariant — [BYTE].** Both carriers PUBLISH ZERO
messages (verified: `grep -c publish` = 0 in both `.ir`s). Every terminal INSERT
is `insert.IsRelation()` → `ClassifyEagerSink` returns `kRelation`,
`MessageOfInsertOrNull` returns `nullopt`. So EVERY `kEagerInsert` renders
`sink=relation` with NO `message=` arm, and NO new `PUBLISH` id enters the
`.ir`/`.h` (the byte-identity invariant D.1 rests on). The four other sink
spellings (`publish-now`/`publish-vec`/`commit-published`) and the stream
`message=<name>/<arity>` arm are CORPUS-UNWITNESSED at R1 (ADJ-S5, LOUD) — they
ship structurally-predicted only.

### 3.1 The RENDER-ORDER LAW (ADJ-S1, CORRECTED) — [BYTE] structure

`pinned_order` sorts lead-0 ops by **`(op_table_id, sign, ctor=oi)`**
(`key_less`, `DeltaRel.cpp:4275`), where `op_table_id` maps **null → sentinel 0**,
a real table → **`id+1`**. Eager ops are `sign 0`; ingest folds are `sign +1`.
Consequences pinned for the draft:

1. The **table-less block leads** (`op_table_id 0`): all table-less
   `kEagerForward`s render FIRST.
2. Each real table's block follows in ascending `id+1`. Within a shared table,
   **sign-0 eager ops precede that table's sign-+1 ingest fold** — so the two
   `kIngestFold`s (`op.0`/`op.1`) render NOT first but each AFTER its table's
   eager forwards.
3. Within a `(table, sign)` group, ties break by `ctor=oi` (walk order).

The order is fully deterministic and pointer-free (V-ORDER-CONSISTENT holds
trivially: edgeless ops emit in exact key order). Legend for the schematics:
`op.⟨w⟩` = a walk-order label ≥ the ingest-fold count; `[STRUCT ×m]` = the block
repeats per raw-walk dispatch (multiplicity `m` bless-pinned).

### 3.2 `demand_tc_witness.deltarel.opt.golden` — POST-R1 byte-draft

Ingest folds: `op.0`→`%table:19` (id+1=20), `op.1`→`%table:23` (24). Eager target
`op_table_id`s: table-less 0; `insert.19`→`%table:4`=5; `tuple.4`→`%table:8`=9;
`tuple.6`→`%table:12`=13; `tuple.7`→`%table:15`=16; `tuple.2`,`tuple.10`→
`%table:19`=20; `tuple.8`→`%table:23`=24.

```
deltarel                                                                    [BYTE]
                                                                            [BYTE]
# --- op_table_id 0 : table-less kEagerForward block (LEADS) ---           [STRUCT]
op.⟨w⟩ kEagerForward sign=· ctx=eager stratum=0                            [BYTE-shape]
    args:                                                                  [BYTE-shape]
#   … one such block per table-less forward dispatch, walk-order oi.
#   Distinct table-less forward views: tuple.12, tuple.3, tuple.11,
#   tuple.9, tuple.5 — each [STRUCT ×m] by induction/cycle re-descent.

# --- op_table_id 5 : %table:4  (insert.19, relation; NO ingest fold) ---  [STRUCT]
op.⟨w⟩ kEagerInsert sign=· ctx=eager stratum=0 sink=relation               [BYTE-shape]
    args: table=%table:4                                                   [BYTE-shape]
#   [STRUCT ×m], m ≥ 2 (proven: two folds into %table:4, ir:187 + ir:206).

# --- op_table_id 9 : %table:8  (tuple.4; NO ingest fold) ---              [STRUCT]
op.⟨w⟩ kEagerForward sign=· ctx=eager stratum=0                            [BYTE-shape]
    args: table=%table:8                                                   [BYTE-shape]

# --- op_table_id 13 : %table:12 (tuple.6) ---                             [STRUCT]
op.⟨w⟩ kEagerForward sign=· ctx=eager stratum=0                            [BYTE-shape]
    args: table=%table:12                                                  [BYTE-shape]

# --- op_table_id 16 : %table:15 (tuple.7) ---                             [STRUCT]
op.⟨w⟩ kEagerForward sign=· ctx=eager stratum=0                            [BYTE-shape]
    args: table=%table:15                                                  [BYTE-shape]

# --- op_table_id 20 : %table:19 (tuple.2, tuple.10 forwards, THEN op.0) --[STRUCT]
op.⟨w⟩ kEagerForward sign=· ctx=eager stratum=0                            [BYTE-shape]
    args: table=%table:19                                                  [BYTE-shape]
#   … (tuple.2 and tuple.10 forward blocks, sign 0) …
op.0 kIngestFold sign=+ ctx=eager stratum=0                                [BYTE]
    effects: {kCounter(%table:19, +, NonRecursive)}                        [BYTE]
    spine: —                                                               [BYTE]
    args: table=%table:19 message=edge_2/2                                 [BYTE]

# --- op_table_id 24 : %table:23 (tuple.8 forward, THEN op.1) ---          [STRUCT]
op.⟨w⟩ kEagerForward sign=· ctx=eager stratum=0                            [BYTE-shape]
    args: table=%table:23                                                  [BYTE-shape]
op.1 kIngestFold sign=+ ctx=eager stratum=0                                [BYTE]
    effects: {kCounter(%table:23, +, NonRecursive)}                        [BYTE]
    spine: —                                                               [BYTE]
    args: table=%table:23 message=demand__reachable_from_bf/1              [BYTE]

rounds:                                                                     [BYTE]
                                                                            [BYTE]
deps:                                                                       [BYTE]
                                                                            [BYTE]
census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=2 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0 kEagerForward=<F> kEagerInsert=<I>
#   census columns 0..17 + kIngestFold=2 :                                 [BYTE]
#   tail append " kEagerForward=<F> kEagerInsert=<I>" :  <F>,<I> values     [STRUCT]
#   (grammar/position [BYTE]; count [STRUCT], bless-pinned; <F> ≥ 11, <I> ≥ 2).
```

**Precise [BYTE] vs [STRUCT] accounting for 3.2:**
- [BYTE]: line 1 `deltarel`, every blank line, the TWO ingest-fold blocks
  (`op.0`/`op.1` — labels, effects, spine, args ALL byte-identical to tip),
  `rounds:`, `deps:`, census columns 0–17, `kIngestFold=2`, and the census tail
  TOKEN spellings `kEagerForward=`/`kEagerInsert=`.
- [BYTE-shape] (grammar committed, appears N× where N is [STRUCT]): every eager
  op header + args line — the per-op TEXT is fully determined by the view's
  table id and kind (all listed above), only the COUNT and label `oi` vary.
- [STRUCT]: the NUMBER of eager op blocks per table (the `×m` multiplicity), the
  `oi` label values (walk order), and the census `<F>`/`<I>` integers. WHY: raw
  walk dispatch count (per-caller/per-context re-descent, §2.2) is not derivable
  from any tip artifact; pinned at bless (§5).

### 3.3 `symrec_tie_1.deltarel.opt.golden` — POST-R1 byte-draft (NEW golden)

Ingest fold: `op.0`→`%table:8` (id+1=9). Eager target `op_table_id`s: table-less
0 (`tuple.7`,`tuple.1`,`tuple.2`,`tuple.4`); `tuple.3`+`insert.11`→`%table:4`=5;
`tuple.5`,`tuple.6`→`%table:8`=9.

```
deltarel                                                                    [BYTE]
                                                                            [BYTE]
# --- op_table_id 0 : table-less kEagerForward block (LEADS) ---           [STRUCT]
op.⟨w⟩ kEagerForward sign=· ctx=eager stratum=0                            [BYTE-shape]
    args:                                                                  [BYTE-shape]
#   distinct table-less forwards: tuple.7, tuple.1, tuple.2, tuple.4
#   — each [STRUCT ×m] (tuple.3/tuple.1/tuple.2 in the SCC re-descend).

# --- op_table_id 5 : %table:4  (tuple.3 forward + insert.11 insert) ---   [STRUCT]
op.⟨w⟩ kEagerForward sign=· ctx=eager stratum=0                            [BYTE-shape]
    args: table=%table:4                                                   [BYTE-shape]
op.⟨w⟩ kEagerInsert sign=· ctx=eager stratum=0 sink=relation               [BYTE-shape]
    args: table=%table:4                                                   [BYTE-shape]
#   (tuple.3 sign-0 and insert.11 sign-0 share table:4; order within the
#    group is ctor=oi = walk order — [STRUCT]. Both before any table:8 fold.)

# --- op_table_id 9 : %table:8  (tuple.5, tuple.6 forwards, THEN op.0) --- [STRUCT]
op.⟨w⟩ kEagerForward sign=· ctx=eager stratum=0                            [BYTE-shape]
    args: table=%table:8                                                   [BYTE-shape]
#   … (tuple.5, tuple.6 forward blocks, sign 0) …
op.0 kIngestFold sign=+ ctx=eager stratum=0                                [BYTE]
    effects: {kCounter(%table:8, +, NonRecursive)}                         [BYTE]
    spine: —                                                               [BYTE]
    args: table=%table:8 message=edge/2                                    [BYTE]

rounds:                                                                     [BYTE]
                                                                            [BYTE]
deps:                                                                       [BYTE]
                                                                            [BYTE]
census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=1 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0 kEagerForward=<F> kEagerInsert=<I>
#   columns 0..17 + kIngestFold=1 [BYTE]; tail <F> ≥ 7, <I> ≥ 1 [STRUCT].
```

Note `op.0` here is the LAST block (its table:8 has the highest `op_table_id` and
the ingest fold's sign +1 sorts after table:8's eager forwards) — the sharpest
illustration of ADJ-S1: `op.0` is not first.

> **[DS-ADJ-3] (adjudicated LOW, UPHELD — grammar F1). Single space after the
> kind name.** The §3.3 `kEagerInsert` schematic carried a column-alignment
> double-space (`kEagerInsert··sign=`); FIXED in place to one space. VERIFIED
> against the emitter: Format.cpp:652 emits `" " << DROpKindName(op.kind)` then the
> C.2 case opens `os << " sign="` → exactly ONE space between the kind and `sign=`.
> §3.2 line 315 already used the correct single space; the golden must inherit the
> single-space form, never the schematic's alignment padding.

---

## (4) UNCHANGED-SURFACES CLAIM — [BYTE], corpus-wide + both carriers

**Prediction (D.1): `.ir`, `.h`, `.cpp`, `.df` byte-identical to frozen 6d695aec,
for both carriers and every one of the 173 corpus cases, all 4 optimization
modes; suite ends `SUITE: PASS`.** The mechanism is mechanical, not argued: R1's
lowering CALLS the region builders (`BuildEagerTupleRegion`/`BuildEagerInsertRegion`,
`Tuple.cpp`/`Insert.cpp` byte-unchanged) with the identical
`(impl, pred_view, view, context, parent, last_table)` at the identical walk
moment — every `impl->next_id++` inside them and inside the descent
(`InTryInsert`/`TableDeltaVector`/`AppendViewTupleToVector`) fires in the same
order, so the ControlFlow IR is byte-identical; `.h`/`.cpp` are pure functions of
that IR; `.df` is UPSTREAM of the whole CF build (dataflow, untouched). The op
construction (`MakeEagerForwardOp`/`MakeEagerInsertOp`), the `RecordEagerDispatch`
`push_back`, and the EAGER_WEB `flow.ops.push_back` mint NO `impl->next_id` and
Emplace NO region — they are invisible to emission. The only artifact that
observes them is the `.deltarel` dump (post-hoc, `gDeltaRelDumpStream`-guarded,
off the timed/emitted path). The 5 pinned non-`.deltarel` IR goldens
(`symrec_tie_1.{ir,df}.opt.golden`, `demand_tc_witness.{ir,h,df}.opt.golden`)
stay byte-identical — they are the live referees that emission did not move; if
any of them moves at bless (§5 post-condition), R1's zero-emission-change claim
is already violated.

---

## (5) BLESS PLAN — restated concretely for BOTH goldens

Two `.deltarel` goldens churn (ADJ-S9 widened D.2's "one golden" to two):
`demand_tc_witness.deltarel.opt.golden` (existing, re-bless wholesale) and
`symrec_tie_1.deltarel.opt.golden` (NEW file). All other pinned surfaces
byte-identical.

**Step 0 — sidecar edit lands in the SAME diff (ADJ-S9).** Append the line
`deltarel opt` to `tests/OptDiff/cases/symrec_tie_1.irgold` (it currently holds
`ir opt` / `df opt`). Its `run_irgold` all-sinks compile ALREADY passes
`-deltarel-out` (`runall.sh:271-278`); this line makes the produced
`deltarel.opt.out` byte-compared against the new golden instead of discarded.
Also bump the `kAllKinds` census comment `18 → 20` (ADJ-S11) and the
`Format.cpp:886` comment — code hygiene in the same diff.

**Step 1 — one full-suite run, ONE workroot (ADJ-S7 provenance + same-workroot).**
```
DR=build/debug/bin/drlojekyll tests/OptDiff/runall.sh $WORKROOT
```
Pre-registered PRE-bless state (ADJ-S6): the ONLY reds across the run are
`demand_tc_witness irgold deltarel.opt IRGOLD-DIVERGE` and `symrec_tie_1 irgold
deltarel.opt IRGOLD-MISSING` (new golden absent) / `-DIVERGE`. Everything else
green; run ×3 to catch nondeterministic flakes (`flow.ops` is a new
ordering-sensitive Kahn input on every compile).

**Step 2 — the direct-diff referee on the ACTUAL bless-source files (ADJ-S7).**
Referee the file `--bless` will copy, NOT a separate compile:
```
# demand_tc_witness (existing golden)
diff tests/OptDiff/goldens/demand_tc_witness.deltarel.opt.golden \
     $WORKROOT/demand_tc_witness/demand_tc_witness.irgold/deltarel.opt.out
# symrec_tie_1 (new golden — expect "no such file" on the golden side first run)
cat  $WORKROOT/symrec_tie_1/symrec_tie_1.irgold/deltarel.opt.out
```
Acceptance (E.1 three-point, applied to BOTH):
1. **Existing lines untouched.** Every non-eager line of the OLD
   `demand_tc_witness` golden appears byte-identical in NEW — both
   `kIngestFold` blocks (`op.0`/`op.1` payloads verbatim), `rounds:`, `deps:`
   empty; the SOLE census delta is the tail append
   ` kEagerForward=<F> kEagerInsert=<I>`, NO existing column changes value or
   position. (For `symrec_tie_1` there is no old golden; the `op.0 kIngestFold`
   block must match its tip `.deltarel` from §1a verbatim.)
2. **Every added line is an eager-op block or the census tail.** The `>` diff
   lines are exclusively `op.N kEagerForward …` / `op.N kEagerInsert …` headers,
   their `    args:` lines, and the census tail — nothing else. Op blocks are
   atomic and blank-separated, so an eager block rendered BEFORE `op.0` (per §3.1)
   is still a purely additive insertion (ADJ-S1: the render interleaving does not
   break the additive-diff class).
3. **`deps:`/`rounds:` empty, census-sum green.** No `-deltarel` abort ⇒
   `<F>+<I>+2 == flow.ops.size()` (tc) / `<F>+<I>+1 == flow.ops.size()` (symrec)
   holds by construction (F-CENSUSABORT).

**Step 3 — the ADJ-S10 STRUCTURAL COUNT READ (the SOLE correctness check on the
new columns).** Before `--bless`, read `<F>`/`<I>` from each produced dump and
cross-check against §2's per-dispatch reasoning: `<F>` counts the TUPLE-forward
dispatches (≥ 11 tc / ≥ 7 symrec; the distinct forward views listed in §2.1/§2.2,
times the induction/cycle re-descent multiplicity), `<I>` the terminal-INSERT
dispatches (≥ 2 tc — the proven `insert.19` double-descent — / ≥ 1 symrec). This
is a bless-gate DISCIPLINE, not code: the full-corpus byte-identity A/B (§4) nets
that EMISSION did not move, but the census counts are BORN at R1 with no
prior-correct baseline, so a structural read is the only guard against an
off-by-N enrollment (a TUPLE reached from 2 edges miscounted, a cut-successor
leaking in). Also verify (ADJ-S6 knob-independence): compile `demand_tc_witness`
under `-disable-dataflow-opt` / `-disable-controlflow-opt` with `-deltarel-out`
and confirm the eager census is stable ACROSS THE CONTROLFLOW-OPT AXIS ONLY
(opt≡nocf, nodf≡none) — see [DS-ADJ-1].

> **[DS-ADJ-1] (adjudicated MEDIUM, UPHELD — crit N-1). Scope the census-
> stability check to the controlflow-opt axis; the dataflow-opt axis LEGITIMATELY
> resizes the eager web.** The unamended "stable across the 4 modes / a
> mode-varying count signals accidental knob coupling" is EMPIRICALLY FALSE on the
> dataflow-opt axis and would false-alarm the bless. VERIFIED live at tip 6d695aec
> (distinct forward-view count, the `kEagerForward` dispatch surface, from the
> `.df`):
>
> | carrier | opt | nocf | nodf | none |
> |---|---|---|---|---|
> | `demand_tc_witness` | 11 | 11 | **17** | **17** |
> | `symrec_tie_1` | 7 | 7 | **12** | **12** |
>
> `-disable-dataflow-opt` skips `QueryImpl::Optimize` (CSE), leaving the Query
> graph un-merged, so the syntax-directed eager walk descends a LARGER TUPLE view
> set — the eager census legitimately GROWS opt→nodf. It is stable only across the
> controlflow-opt axis (opt≡nocf, nodf≡none), because the census is fixed
> pre-`ProgramImpl::Optimize` (F-ORDER) and the `.df` is upstream of controlflow-
> opt. The design E.2 justification ("same monotone views") is self-refuting on the
> df axis. CORRECTED bless discipline: expect `opt == nocf` and `nodf == none` on
> the eager census; a df-axis delta (opt≠nodf) is EXPECTED, not coupling. The
> opt-only golden is UNTHREATENED regardless (the sidecar is `deltarel opt`; the
> suite emits no other mode's `.deltarel`). Also amend design E.2 second bullet if
> that check is ever run from the design.

**Step 4 — bless from the SAME `$WORKROOT`.**
```
tests/OptDiff/runall.sh --bless $WORKROOT demand_tc_witness
tests/OptDiff/runall.sh --bless $WORKROOT symrec_tie_1
```
`--bless <name>` copies ALL sidecar surfaces (h/ir/df/deltarel); it re-copies the
exact `deltarel.opt.out` refereed in Step 2 (same workroot; a stale/empty
workroot trips the `[ ! -f "$isrc" ]` FATAL at `runall.sh:111-113`).

**Step 5 — POST-condition tripwire (ADJ-S7).** `git status` must show EXACTLY
the two `.deltarel.opt.golden` files (plus the `symrec_tie_1.irgold` sidecar)
changed. If `demand_tc_witness.{h,ir,df}.opt.golden` or
`symrec_tie_1.{ir,df}.opt.golden` appear, R1's zero-emission-change claim is
ALREADY violated — emission moved, STOP. Then re-run the suite ×3: the ONLY
expected outcome is `SUITE: PASS (173)` with the two deltarel goldens now green.

**Standing gates unchanged (design §E):** debug==release `cmp` on both
carriers' `-deltarel-out` (ADJ-S6 G5-analog on the new dump path); ASAN both
surfaces zero reports; ctest 5/5 (DeltaRelValidators unaffected); E-62 re-grep
clean (no new `pinned_order` reader); the Q5 bound is a MEASURED
`bench/runbench.sh progsize 128` ABABAB verdict (ADJ-S8), not the pre-registered
sub-1%. PLUS [DS-ADJ-4]: at bless, `hexdump -C` (or `cat -v`) the first eager
op's `sign=` header bytes and confirm the `·` renders as `\xC2\xB7` — R1's two
goldens are the FIRST committed `.deltarel` goldens to PIN that byte.

> **[DS-ADJ-4] (adjudicated LOW, UPHELD-as-strengthening — grammar F2).** The
> `sign=·` token IS emitter-sourced ([BYTE]-legitimate: `SignGlyph(0)` →
> `"\xC2\xB7"`, Format.cpp:275, `table_op_sign==0` by default). VERIFIED that no
> committed `.deltarel` golden currently contains the byte: the sole existing one
> (`demand_tc_witness.deltarel.opt.golden`) holds `sign=+` only — `grep -c` for
> `\xC2\xB7` returns **0** across all committed deltarel goldens. So R1's bless is
> the FIRST to pin `\xC2\xB7` in the suite and has no prior golden to cross-check
> against — hence the deliberate hexdump at bless (above). NOTE the design's
> "corpus-proven multibyte-clean" phrasing (design C.1 line 582) is the
> overstatement; the desired-states §3.0 does NOT repeat it (it says only "the
> multibyte signless glyph `\xC2\xB7`"), so no [BYTE] mark is downgraded — this is
> a bless-ritual add, not a correction to the drafts.

---

## Appendix — [STRUCT] tally (minimized; each justified)

Only THREE things are [STRUCT] in the entire R1 desired state, all reducing to
the same irreducible cause:

1. **Census `<F>`/`<I>` integers** (both carriers). WHY: the count is a live-walk
   dispatch tally computed pre-controlflow-opt (F-ORDER); no tip artifact exposes
   it, and R1 is unimplemented. Floor derived (§2): tc `<F>`≥11/`<I>`≥2, symrec
   `<F>`≥7/`<I>`≥1. Pinned by the ADJ-S10 structural read at bless.
2. **Eager-op block MULTIPLICITY per table** (the `×m`). WHY: inductive-MERGE /
   cycle-JOIN successors re-descend per incoming edge and per init-vs-cycle
   context (proven live: `insert.19` folds twice, `tuple.3` folds thrice). Same
   cause as (1).
3. **The `oi` LABEL values** on eager blocks. WHY: `oi` = walk-DFS enrollment
   order, deterministic but only observable by running the walk; the labels are
   non-monotonic in render order by construction.

Everything else is [BYTE] or [BYTE-shape]: the render-ORDER law and per-table
block sequence (§3.1, fully derived from `op_table_id`), every op header/args
TEXT (§3.0, determined by view kind + table id), the `sink=relation`/no-`message=`
invariant (§3.0a, zero publishes verified), both ingest-fold blocks, `rounds:`,
`deps:`, census columns 0–17 + `kIngestFold`, and all corpus non-`.deltarel`
surfaces (§4).

======================================================================
[DS-ADJ-7 — POST-BLESS AMENDMENT (Fable-review R1 finding [2],
orchestrator-adjudicated at the blessed goldens + ModelTableOrNull)]
THE RENDER AUTHORITY FOR AN EAGER OP'S table= IS THE UNION-FIND MERGED
MODEL (impl->view_to_model ->FindAs<DataModel>()->table), NOT the .df
per-view table attribute this document's §2.2 derivation used. Views
that unify their model with a downstream INSERT render that model's
table even where the .df dump shows them table-less (tc as-blessed:
kEagerForward at %table:4 ×2 + %table:23 ×2, 3 table-less — vs §2.2's
0/1/5 prediction; symrec as-blessed: 3 @ %table:8, 2 @ %table:4, 2
table-less — vs §2.2's 2/1/4). The [STRUCT] envelope held (counts
12/2 and 7/1 within floors; order law exact); the per-block table
ATTRIBUTIONS in §2.2 are hereby SUPERSEDED by the as-blessed goldens,
and any future Rk bless review must read table= against the MERGED
model, never the .df attribute. The A.6(c) table-match validator
(op.table_op_table == ModelTableOrNull) enforces exactly this
authority corpus-wide, always-on.
