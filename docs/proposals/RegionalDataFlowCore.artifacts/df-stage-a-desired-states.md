# Stage A — desired IR state: the `.df` dump and the new `-contract-out` surface

Authored 2026-08-02 against tip f0c913e0 (branch keyed-instances), grounded in
the collected phase4 dumps. House idiom: DESIRED = DIFF from CURRENT. Current
lines are quoted verbatim from the real dumps
(`/private/tmp/.../scratchpad/phase4/{agg_distinct_1,demand_tc_witness,join_1}.{df,orient.md}`);
never invented.

Charge surface: the `.df` dump AFTER Stage A, plus the NEW contract dump
surface. Stage artifact: `stage-a-diff.md` (H-A2/H-A3/H-A7/H-A8). Witnesses:
`agg_distinct_1` (primary — the identity/collapse story), `demand_tc_witness`,
`join_1`.

**The load-bearing desired-state claim, stated once:** Stage A changes the
`.df` dump for **zero** of these three witnesses (and, per the exit gate, zero
of the 180-case corpus). The observable Stage-A surface is an ENTIRELY NEW,
opt-in `-contract-out` dump. This document pins both: the `.df` non-change
(with its determinism contract restated so a lander can prove byte-identity)
and the `-contract-out` dump concretely, per view, for each witness. §7 is the
ADJUDICATION-INPUT alternatives section (the in-`.df` variant) — surfaced, not
resolved, per charge.

---

## 1. The tension this document must carry (not resolve)

The session charter expected Stage A to put "explicit projection nodes,
member-key annotations" INTO the `.df` dump. `stage-a-diff.md` chose the
OPPOSITE realization:

- **H-A2** folds the `MemberProjection`/`DistinctProjection` distinction into an
  immutable `QueryTupleImpl::ProjectionRole {kMember,kDistinct}` that is part of
  the node's `Hash`/`Equals` structural identity — NOT a new subclass, NOT a
  rendered `.df` tag.
- **H-A3/H-A4** compute `RowContract`s as a recomputable pure function of the
  final graph, materialized once in the `Query::Build` tail, never present
  during Optimize.
- **H-A8** renders those contracts through a SEPARATE opt-in `-contract-out`
  sink; the default `-df-out` dump stays byte-identical.

This document authors the desired state PER THE STAGE DOC'S CHOICE (byte-
identical `.df` + a concrete `-contract-out` dump), and in §7 sketches what the
in-`.df` variant would look like for `agg_distinct_1`, with the trade-off stated.
> **AMENDED 2026-08-02 (D2.3 RATIFIED).** The separate-sink choice is now
> owner-ratified (stage-a-diff.md H-A8); §7's in-`.df` sketch is retained as the
> record of the road not taken, no longer an open adjudication.

---

## 2. Global determinism contract (both surfaces)

### 2.1 The `.df` dump (unchanged)

- **Order** is a pure function of the graph: views render in `det_seq` order,
  which is the `^<kind>.<id>` id order visible in every quoted dump below
  (`^select.0`, `^select.1`, `^tuple.2`, …). Stage A adds no node, mints no id,
  reorders nothing → the id stream, and therefore the order, is byte-identical
  to tip.
- **Content** per line is unchanged: `<kind> ^<kind>.<id> (cols) ; <comment>`,
  the `ATTRIBUTES` line, and the `=> ^succ (proj) ; <comment>` edges. Stage A
  renders neither `projection_role` nor any `RowContract` field here.
- **Bijection tripwire** (Format.cpp:770, the det_seq↔live-view witness) is
  unchanged and still passes.
- **Permutation referee:** N/A. The `.df` dump has no published-delta tokens;
  every line is byte-exact today and stays byte-exact. `permcheck.py` does not
  apply to `.df` and Stage A does not change that.

### 2.2 The `-contract-out` dump (new)

- **Order** is the SAME `det_seq` (`^id`) order as the `.df` dump — one contract
  block per live view, in the identical order the `.df` renders those views.
  This is deliberate: a reviewer reads the two dumps side by side and each
  `^tuple.N` block lines up. Order is a pure function of the frozen post-Optimize
  graph.
- **Bijection tripwire:** the SAME always-on det_seq↔live-view check as `QueryDF`
  (`|contract blocks| == |live views|`, exactly one per live view, none for a
  dead view — this is V-CONTRACT-CENSUS, H-A7). A reintroduced traversal that
  visits a view twice or skips one aborts at dump time.
- **Content** is a pure, recomputable function of the final graph (member keys
  from the transfer rules of proposal §4.4; equalities from `BuildEquivalenceSets`;
  constants from `TrackConstAfterInit`). No satellite state, nothing for CSE to
  preserve.
- **Permutation referee:** N/A, and MORE STRONGLY than for `.df`. The contract
  dump has NO order-free field whatsoever — every member key is rendered in
  visible-field order. `permcheck.py` does NOT apply; the referee is pure
  byte-compare against a fresh `.contract.opt.golden`.
  > **AMENDED 2026-08-02 (flat-key RowContract, stage-a-diff.md H-A3 / D3.4
  > candidate 3).** The struck clause "every candidate-key antichain is rendered
  > in a canonical field-id order" is GONE: the Stage-A `RowContract` is flat-key
  > `{visible_fields, member_key}` — there is no candidate-key antichain to render
  > (the antichain + `Minimize` defer to Stage B). The determinism argument is
  > strictly simpler, not weaker: a single member key in visible-field order.
- **Mode scope:** OPT MODE ONLY. Contracts are a post-Optimize graph property;
  the four optimization modes can produce different graphs (different CSE
  merges, different canonical columns), so a `.contract.opt.golden` pins opt
  mode exactly as `.df.opt.golden`/`.irgold` already do. A `.contract` sidecar
  recognizer in runall.sh/diffrun.sh emits `-contract-out` and byte-compares
  opt-mode only.

### 2.3 The contract-dump grammar (design choice, stated concretely)

> **AMENDED 2026-08-02 (flat-key RowContract, stage-a-diff.md H-A1/H-A3 / D3.4
> candidate 3 + candidate 1/A-nec-1).** The grammar below is FLAT-KEY. The
> `support=<algebra>` token and the `candidates={ … }` line are STRUCK from every
> block: Stage A's `RowContract` is `{visible_fields, member_key}` only —
> `derivation_support` is struck (candidate 1 / A-nec-1: only the domain TYPES
> survive, in the H-A1 static_assert battery; Stage A transfers neither shape nor
> counts) and the pre-minimization antichain defers to Stage B (candidate 3).
> `role=`, `key=`, and the aggregate-only `input_key=` are the entire per-view
> surface. E-A3 is CLOSED by this strip.

Header token `contracts` (line 1), mirroring `.df`'s `dataflow` and `.rel`'s
`rel`. One block per live view:

```text
<kind> ^<kind>.<id> (visible_fields...)
  role=<member|distinct|n/a> key=(member_key...)
  [input_key=(...)]        # aggregate views only (H-A5)
```

- `role=` is `member`/`distinct` only for `QueryTupleImpl` (the H-A2
  discriminant); `n/a` for every other kind (a role is a projection property).
- `key=` is the PROVEN `SemanticMemberKey` (a flat `SmallVec<FieldId>`) in
  visible-field order. For every view on a MULTI-VIEW STRATUM (a recursive SCC,
  H-A3 Phase 1 cycle rule) it is the conservative `AllFields(columns)`.
- `input_key=` appears ONLY on `aggregate` blocks: the explicit input member key
  (H-A5), read from the summarized-input view's contract.

Trailing census line (pure function; the V-CONTRACT-CENSUS witness rendered):

```text
census: views=<V> contracts=<V> role{distinct=<d> member=<m> na=<n>}
        agg_input_key_ok=<a> collapse_error=0
```

`collapse_error` is the count of V-NO-COLLAPSE hard failures; it is `0` on every
current corpus program (E-A2) and a nonzero value here means the compile already
returned `std::nullopt` (the dump would not be produced). It is pinned in the
golden as a standing `0` witness.

---

## 3. Witness 1 — `agg_distinct_1` (PRIMARY: the identity/collapse story)

### 3.1 The `.df` dump: DESIRED = byte-identical to CURRENT

The current 68-line dump (`agg_distinct_1.df`, quoted at the head of this
witness) is the desired dump UNCHANGED. Restating the two lines a reader might
expect Stage A to touch, to pin the non-change:

CURRENT (and DESIRED — no diff):

```text
tuple ^tuple.2 (X:i32, W2:i32)
  ATTRIBUTES table=%table:25 eqset=3 class=monotone stratum=2
  => ^aggregate.9 (X, W2)
  => ^aggregate.10 (X, W2)
```

```text
aggregate ^aggregate.9 (X:i32, N:i32)              ; cinv
  ATTRIBUTES table=%table:5 eqset=6 class=differential stratum=5
  => ^tuple.5 (X, N)
```

No `role=`, no `member_key=`, no `input_key=` is added to these `ATTRIBUTES`
lines. The `; cinv` / `; crec` functor comments on the aggregate lines are
unchanged. **Contract:** Stage A is upstream of nothing that renders here; the
`projection_role` stamp (H-A2) rides in `Hash`/`Equals` only, and the
`RowContract` is a side-table drained by a different sink.

### 3.2 Why this is the identity/collapse story — five lint sites, THREE nodes

This is the charge's centerpiece. The source (`agg_distinct_1.dr`) has FIVE
projected-away columns that the deleted `LintAggregateProjection` warned on
(the five stderr lines at `.dr:31:55 / 32:55 / 35:54 / 35:61 / 38:63`). Those
five SOURCE sites collapse onto exactly THREE distinct projection TUPLE nodes in
the optimized `.df`, because CSE already folded the two structurally identical
`ew(_B, X, W2)` over-bodies:

| # | src loc | over-body clause | `.df` projection node | member key | dropped col(s) |
|---|---------|------------------|-----------------------|------------|----------------|
| 1 | :31:55 `_B` | `cnt_inv over (X,W2){ew(_B,X,W2)}` | `^tuple.2 (X,W2)` | `(X,W2)` | `B` |
| 2 | :32:55 `_B` | `cnt_rec over (X,W2){ew(_B,X,W2)}` | `^tuple.2 (X,W2)` **(CSE-shared)** | `(X,W2)` | `B` |
| 3 | :35:54 `_`  | `cnt_ux over (K,X2){ew(_,X2,_),K=1}` | `^tuple.3 (c7,X2)` | `(c7,X2)` | `B` |
| 4 | :35:61 `_`  | `cnt_ux over (K,X2){ew(_,X2,_),K=1}` | `^tuple.3 (c7,X2)` **(same node)** | `(c7,X2)` | `W` |
| 5 | :38:63 `_`  | `cnt_rows over (X,B2){ew(B2,X,_)}` | `^tuple.4 (X,B2)` | `(X,B2)` | `W` |

The `.df` proves the collapse structurally: `^tuple.2` has TWO successor edges,
`=> ^aggregate.9 (X, W2)` and `=> ^aggregate.10 (X, W2)` — one over-body head
feeding both `cnt_inv` (`cinv`/`@invertible`) and `cnt_rec` (`crec`/`@recompute`).
This is H-A2's refinement-safety property firing in the SAFE direction: two
`kDistinct`-over-`kDistinct` projections with identical columns are structurally
equal, so CSE merges them. Stage A does not un-merge them (both sides are
`kDistinct`; the role is equal, so the fold is legal). **If the role stamp had
DIFFERED between these two, H-A2 would refuse the merge and the dump would grow
a line — the E-A1 divergence signal. It does not here: `.df` byte-identical.**

### 3.3 The `-contract-out` dump: DESIRED (new file, fresh golden)

Concrete desired `agg_distinct_1.contract.opt.golden`. Ids are shape-exact
(`^tuple.2` etc. track the `.df` `^id`s; the const-fold names `c7`/`c15`/`c23`
track the `.df`). Blocks in `det_seq` (`^id`) order, matching the `.df`:

> **AMENDED 2026-08-02 (flat-key RowContract).** `support=` and the
> `candidates={ … }` line struck from every block (D3.4 candidate 1/A-nec-1 +
> candidate 3). `agg_distinct_1` has NO multi-view stratum (every view is its own
> stratum — verified against the `.df`), so the H-A3 Phase 1 cycle rule fires on
> zero views and no `key=` value changes; only the struck lines differ. The
> census `contracts=17` is unchanged (still one contract per live view — the
> flat-key strip removes fields WITHIN a contract, never a contract).

```text
contracts

select ^select.0 (B:i32, X:i32, W:i32)
  role=n/a key=(B,X,W)
select ^select.1 (c4:i32)
  role=n/a key=(c4)
tuple ^tuple.2 (X:i32, W2:i32)
  role=distinct key=(X,W2)
tuple ^tuple.3 (c7:i32, X2:i32)
  role=distinct key=(c7,X2)
tuple ^tuple.4 (X:i32, B2:i32)
  role=distinct key=(X,B2)
tuple ^tuple.5 (X:i32, N:i32)
  role=distinct key=(X,N)
tuple ^tuple.6 (X:i32, N:i32)
  role=distinct key=(X,N)
tuple ^tuple.7 (c15:i32, N:i32)
  role=distinct key=(c15,N)
tuple ^tuple.8 (X:i32, N:i32)
  role=distinct key=(X,N)
aggregate ^aggregate.9 (X:i32, N:i32)
  role=n/a key=(X)
  input_key=(X,W2)
aggregate ^aggregate.10 (X:i32, N:i32)
  role=n/a key=(X)
  input_key=(X,W2)
aggregate ^aggregate.11 (c23:i32, N:i32)
  role=n/a key=(c23)
  input_key=(c7,X2)
aggregate ^aggregate.12 (X:i32, N:i32)
  role=n/a key=(X)
  input_key=(X,B2)
insert ^insert.13 (X:i32, N:i32)
  role=n/a key=(X,N)
insert ^insert.14 (X:i32, N:i32)
  role=n/a key=(X,N)
insert ^insert.15 (c15:i32, N:i32)
  role=n/a key=(c15,N)
insert ^insert.16 (X:i32, N:i32)
  role=n/a key=(X,N)
census: views=17 contracts=17 role{distinct=7 member=0 na=10}
        agg_input_key_ok=4 collapse_error=0
```

**Per-construct contract:**

- `select.0` / `select.1` — `QuerySelectImpl` (declared relation / stream read).
  Transfer rule §4.4 "Declared relation": no declared key on `edge/3` or on the
  unit constant, so `key = AllFields`. `role=n/a` (a SELECT is not a projection).
- `tuple.2/3/4` — the over-body head TUPLEs, stamped `kDistinct` by H-A2 (a
  clause-head projection into the aggregate's summarized input). `key = visible
  output tuple` = the `over()` column list. The dropped columns (`B`; `B` and
  `W`; `W`) are OUTSIDE the member key by construction — this is the
  §4.4 "Distinct projection" arm, which requires NO proof. `tuple.2` is the
  CSE-shared node (sites 1 & 2).
- `tuple.5/6/7/8` — the aggregate OUTPUT clause heads (`cnt_inv`/`cnt_rec`/
  `cnt_ux`/`cnt_rows`), also `kDistinct`. `key = (X,N)` / `(c15,N)` — the visible
  output tuple. `N` is in the key here because these are set members of the
  output relation, not the group.
- `aggregate.9/10/11/12` — §4.4 "Aggregate": OUTPUT `key` = the group key
  (`(X)` / `(c23)` — the `N` summary column is functionally determined by the
  group, so it is NOT in the output member key). `input_key` = the summarized
  input view's member key (H-A5): `(X,W2)` for 9 & 10 (both read the shared
  `tuple.2`), `(c7,X2)` for 11, `(X,B2)` for 12. This is the typed re-description
  of "distinct over()-projection tuples" — provable, not empirical.
- `insert.13-16` — §4.4 INSERT: passthrough of the input head's key.

### 3.4 The five lint sites — V-NO-COLLAPSE / V-AGG-INPUT-KEY verdicts

Per charge, the explicit verdict for each of the five former-lint sites:

| # | node | V-NO-COLLAPSE | why | V-AGG-INPUT-KEY (on the fed aggregate) |
|---|------|---------------|-----|----------------------------------------|
| 1 | `^tuple.2` | **PASS** (validated-safe) | `kDistinct`; dropped `B` is outside member key `(X,W2)` by construction — no proof required | `aggregate.9` input_key `(X,W2)` realized → **PASS** |
| 2 | `^tuple.2` (shared) | **PASS** | same node/verdict as #1 — the CSE-collapse means ONE contract answers two source sites | `aggregate.10` input_key `(X,W2)` realized → **PASS** |
| 3 | `^tuple.3` | **PASS** | `kDistinct`; dropped `B` outside `(c7,X2)` | `aggregate.11` input_key `(c7,X2)` realized → **PASS** |
| 4 | `^tuple.3` (same node) | **PASS** | `kDistinct`; dropped `W` outside `(c7,X2)` — two drops on ONE projection | (same aggregate.11) → **PASS** |
| 5 | `^tuple.4` | **PASS** | `kDistinct`; dropped `W` outside `(X,B2)` | `aggregate.12` input_key `(X,B2)` realized → **PASS** |

**All five VALIDATED-SAFE; zero hard errors.** `collapse_error=0` in the census.
`role{member=0}` — there is NO `kMember` projection anywhere in this program, so
V-NO-COLLAPSE (the hard-error arm) fires on zero sites. This is the concrete
witness for E-A2: on the current corpus V-NO-COLLAPSE is unreachable as a
user-facing reject; the deleted lint's replacement here is TYPED-AWAY ambiguity
(every drop is a definitional `kDistinct` collapse), plus internal belts.
**stdout unchanged; stderr loses its 5 advisory lines.**

---

## 4. Witness 2 — `demand_tc_witness`

### 4.1 The `.df` dump: DESIRED = byte-identical to CURRENT

The current 99-line dump is the desired dump UNCHANGED. The demand layer is
carried across Stage A entirely unaltered (Stage C, not A, deletes it). The
demand-fabricated select renders exactly as today:

CURRENT (and DESIRED — no diff):

```text
select ^select.1 (c3:u64)                          ; recv #message demand__reachable_from_bf/1
  ATTRIBUTES eqset=2 class=table-less stratum=1
  => ^tuple.8 (c14=c3)
  => ^tuple.12 (c21=c3)
```

**Contract:** Stage A adds no contract-specific rendering to the demand seed and
does not re-slot the demand transform relative to Stratify. The `demand__` name,
the `; recv #message` comment, and both projection edges are byte-identical.

### 4.2 The `-contract-out` dump: DESIRED (new, fresh golden)

The key demonstration here: the demand-fabricated `select.1` receives an
ORDINARY declared-relation contract — Stage A does not special-case the demand
layer. Merge and join nodes on the recursive SCC take the conservative
`AllFields(columns)` member key from the H-A3 Phase 1 cycle rule; the acyclic
`join.16` takes the §4.4 union-of-pivots directly (no minimization — deferred to
Stage B). Blocks in `^id` order (recursive-cycle keys marked `[cycle] AllFields`;
see the AMENDED note below):

> **AMENDED 2026-08-02 (flat-key RowContract + H-A3 Phase 1 cycle rule).**
> Two consistent amendments applied to this block:
> - **flat-key:** `support=` and `candidates={ … }` struck from every block
>   (D3.4 candidate 1/A-nec-1 + candidate 3). The `[join-min]` annotation is GONE
>   with the antichain — `Minimize` defers to Stage B.
> - **AllFields cycle rule (BROKEN-2 resolution):** every view on the MULTI-VIEW
>   STRATUM 5 — the recursive SCC `{tuple.3, tuple.4, tuple.5, tuple.6, tuple.7,
>   tuple.11, join.13, join.14, join.15, merge.17, merge.18}` (11 views, verified
>   against the `.df` `stratum=5` histogram) — takes `key = AllFields(columns)`
>   directly from Phase 1 (a pure function of SCC structure, no transfer, no
>   fixpoint). Marked `# [cycle] AllFields` below. The ONE value change from the
>   pre-amendment block is `join.13`: its former join-minimized `key=(M,T)`
>   becomes the conservative `key=(M,F,T)` (all three visible columns). Every
>   other cyclic view's AllFields key equals its former key by coincidence of
>   shape, so only its rationale/comment changes. Roles are build-stamps,
>   unaffected → the census `role{distinct=5 member=6 na=9}` is unchanged, and
>   `contracts=20` still counts one contract per live view.
>   > **AMENDED 2026-08-03 (panel close-out: necessity-3 / determinism-2).** The
>   > "Roles are build-stamps, unaffected → `role{distinct=5 member=6 na=9}` is
>   > unchanged" clause is SUPERSEDED: the pre-close-out roles used a SEMANTIC
>   > "set projection" classifier, not the ratified STRUCTURAL rule. See the
>   > restamp note directly below — the corrected tally is
>   > `role{distinct=1 member=10 na=9}`. `contracts=20` is genuinely unchanged.

> **AMENDED 2026-08-03 (panel close-out: necessity-3 / determinism-2).** The
> per-block roles below are RESTAMPED to the RATIFIED STRUCTURAL rule
> (stage-a-diff.md H-A2 / BROKEN-3): `kDistinct` is stamped ONLY at the two
> clause-head mint sites (source-level clause-head projection + over-body head);
> EVERYTHING else — including every `Demand.cpp` mint and every non-clause-head
> read facade — DEFAULTS to `kMember`. The pre-close-out roles used a SEMANTIC
> "set projection ⇒ distinct" classifier, which the ratified rule does not use.
> Fresh provenance audit this pass (real dump compiled via
> `drlojekyll demand_tc_witness.dr -demand -df-out`; `Demand.cpp` mint sites and
> the d4s3-recipe node map read directly):
>
> | block | .df site (stratum) | provenance | ratified role |
> | --- | --- | --- | --- |
> | tuple.2 (F,T) | acyclic (2); `select.0`(edge_2) → tuple.2 → `join.14 .in1` | edge_2 BODY-atom read feeding the base-guard JOIN (recipe JOIN-20 `.in1`) — a JOIN input, NOT a projection into a relation; the base clause-head role sits on the restore `tuple.11` (already `kMember`) | **kMember** (was distinct) |
> | tuple.5 (From) | cycle (5); `merge.17` → tuple.5 → `merge.18` | `Demand.cpp` propagation projection (`:987`; drops T) into the d_path plumbing | **kMember** (was distinct) |
> | tuple.8 (c14) | acyclic (6); `select.1`(demand seed) → tuple.8 → `join.16 .in0` | `Demand.cpp` raw-seed projection (`:1093`, §3.7 query-guard) | **kMember** (was distinct) |
> | tuple.12 (c21) | acyclic (3); `select.1` → tuple.12 → `merge.18` | `Demand.cpp` d_path root/seed projection | **kMember** (was distinct) |
> | tuple.9 (From,To) | acyclic (8); `join.16` → tuple.9 → `insert.19` into %table:4 | the QUERY clause head projecting into the `reachable_from` OUTPUT relation (a kReadAtTuple site — the original head survives, no restore minted) | **kDistinct** (unchanged) |
>
> Net: FOUR blocks restamp distinct→member (tuple.2/5/8/12); `tuple.9` is the
> SOLE surviving `kDistinct` — the only projection into an output relation set.
> This REFINES the panel's predicted `role{distinct=2 …}`: the panel kept
> tuple.2 distinct, but the re-audit ("re-audit tuple.2 specifically") shows
> tuple.2 feeds a JOIN, not a relation — it is not a clause head. Corrected
> census (verified, not trusted): **`role{distinct=1 member=10 na=9}`**
> (1 + 10 + 9 = 20 = |live views|; `contracts=20` unchanged — restamping moves
> roles WITHIN contracts, never removes one). The dump block, census line, and
> the per-construct bullets below are updated accordingly.

```text
contracts

select ^select.0 (M:u64, T:u64)
  role=n/a key=(M,T)
select ^select.1 (c3:u64)
  role=n/a key=(c3)
tuple ^tuple.2 (F:u64, T:u64)
  role=member key=(F,T)                        # restamped 2026-08-03 (panel close-out); edge_2 body-read into join.14, not a clause head
tuple ^tuple.3 (F:u64, T:u64)
  role=member key=(F,T)                        # [cycle] AllFields
tuple ^tuple.4 (From:u64, To:u64)
  role=member key=(From,To)                    # [cycle] AllFields
tuple ^tuple.5 (From:u64)
  role=member key=(From)                        # restamped 2026-08-03 (panel close-out); Demand.cpp prop projection; [cycle] AllFields subsumes the dropped T
tuple ^tuple.6 (c11:u64)
  role=member key=(c11)                        # [cycle] AllFields
tuple ^tuple.7 (From:u64, To:u64)
  role=member key=(From,To)                    # [cycle] AllFields
tuple ^tuple.8 (c14:u64)
  role=member key=(c14)                        # restamped 2026-08-03 (panel close-out); Demand.cpp raw-seed projection
tuple ^tuple.9 (From:u64, To:u64)
  role=distinct key=(From,To)
tuple ^tuple.10 (M:u64, T:u64)
  role=member key=(M,T)
tuple ^tuple.11 (F:u64, T:u64)
  role=member key=(F,T)                        # [cycle] AllFields
tuple ^tuple.12 (c21:u64)
  role=member key=(c21)                        # restamped 2026-08-03 (panel close-out); Demand.cpp d_path seed projection
join ^join.13 (M:u64, F:u64, T:u64)
  role=n/a key=(M,F,T)                         # [cycle] AllFields (was (M,T))
join ^join.14 (F:u64, T:u64)
  role=n/a key=(F,T)                           # [cycle] AllFields
join ^join.15 (From:u64, To:u64)
  role=n/a key=(From,To)                       # [cycle] AllFields
join ^join.16 (From:u64, To:u64)
  role=n/a key=(From,To)                       # acyclic (stratum 7); pivot From
merge ^merge.17 (F:u64, T:u64)
  role=n/a key=(F,T)                           # [cycle] AllFields
merge ^merge.18 (c33:u64)
  role=n/a key=(c33)                           # [cycle] AllFields
insert ^insert.19 (From:u64, To:u64)
  role=n/a key=(From,To)
census: views=20 contracts=20 role{distinct=1 member=10 na=9}
        agg_input_key_ok=0 collapse_error=0
        ; role tally AMENDED 2026-08-03 (panel close-out: necessity-3/determinism-2):
        ; was role{distinct=5 member=6 na=9}; tuple.2/5/8/12 restamped distinct->member
```

**Per-construct contract (the load-bearing arms):**

- `select.1` — the demand seed gets a plain §4.4 "Declared relation" contract:
  `key=(c3)` (a leaf; no `support=` token under flat-key). Stage A neither reads
  nor annotates `GuardAnnotation`/`RecognizedSubgraph`; the contract is blind to
  demand.
- `tuple.9` — the QUERY clause head projecting `path`'s `(From,To)` into the
  `reachable_from` OUTPUT relation (`insert.19` into `%table:4`); a genuine
  source-level clause-head projection into a relation set → `kDistinct`. It is
  the SOLE surviving `kDistinct` in this program.
- `tuple.2` — RESTAMPED `kMember` (AMENDED 2026-08-03, panel close-out:
  necessity-3 / determinism-2). Despite feeding the base rule's guard, it is
  the edge_2 BODY-atom read (`select.0` → tuple.2 → `join.14 .in1`) — a JOIN
  input, NOT a projection into a relation, so NOT a clause head. The base
  rule's clause-head role sits on the restore `tuple.11` (already `kMember`).
  Acyclic (stratum 2); Phase-2 preserves its input key `(F,T)` with no dropped
  column. (The pre-close-out "tuple.2/9 — source clause heads" reading
  mis-classified tuple.2.)
- `tuple.5/8/12` — `Demand.cpp` mints (propagation / raw-seed / d_path-seed
  projections into the demand-guard / recursive plumbing), so by the ratified
  STRUCTURAL rule (H-A2 / BROKEN-3: every `Demand.cpp` mint DEFAULTS `kMember`)
  they are `kMember`, NOT `kDistinct`. The pre-close-out "set projection ⇒
  distinct" reading was a SEMANTIC classifier the ratified rule does not use.
  On `tuple.5` the facade DOES drop a column (`merge.17 (F,T)` → `(From)`,
  dropping `T`), which would seem to violate the "demand facades are
  key-preserving" claim — but tuple.5 is on the MULTI-VIEW stratum-5 SCC, so
  H-A3 Phase 1's cycle rule assigns `key = AllFields(columns) = (From)` DIRECTLY
  (no Phase-2 transfer, so no dropped-column `DeterminedBy` proof runs on a
  cyclic view); V-NO-COLLAPSE never fires and the drop is safe by the cycle
  rule. "Key-preserving" is therefore an ACYCLIC-facade (Phase-2) claim only;
  on a cycle the conservative AllFields key subsumes any drop. AMENDED
  2026-08-03 (panel close-out: necessity-3 / determinism-2).
> **AMENDED 2026-08-02 (flat-key + cycle rule).** The three bullets below are
> rewritten to the flat-key / AllFields-cycle-rule forms: `tuple.3/4/6/7/11`,
> `join.13/14/15`, and `merge.17/18` are all on the stratum-5 SCC and take
> `AllFields` from H-A3 Phase 1 (not join-min inheritance, not a minimized
> antichain, not a merge-support fold); `tuple.10` and `join.16` are the acyclic
> members. No `candidates`/`support` token survives.

- `tuple.10` — an ACYCLIC (stratum 4) internal forwarding facade
  (optimizer-minted, key-preserving by the keep-last-edge rule), `kMember`;
  Phase 2 acyclic transfer preserves its single input's key `(M,T)`, and
  V-NO-COLLAPSE would fire if it dropped an un-determined column (it does not →
  `collapse_error=0`).
- `tuple.3/4/6/7/11` — internal forwarding facades stamped `kMember`, but ON the
  stratum-5 SCC, so H-A3 Phase 1 assigns each `key = AllFields(columns)`
  directly (no transfer, no drop to check — the conservative cyclic key). The
  role stamp still records that they are member projections; the cycle rule
  simply supplies the most conservative member key. This is where a reviewer
  sees the cycle rule do its work: recursive-view keys are AllFields, not
  minimized.
- `join.13/14/15` — §4.4 "Join" ON the SCC → Phase 1 `key = AllFields(columns)`
  (`(M,F,T)` / `(F,T)` / `(From,To)`); no minimization (deferred to Stage B),
  no candidate antichain. `join.16` is the ACYCLIC (stratum 7) join: Phase 2
  takes the conservative union of mapped contributor keys directly, which yields
  `(From,To)` because the pivot column mapping already collapses `c14`/`From` to
  the single pivot output — again no minimization.
- `merge.17/18` — §4.4 "Merge/union" ON the SCC → Phase 1 `key = AllFields`
  (`(F,T)` / `(c33)`). The colliding arms (`tuple.3`+`tuple.11` into 17;
  `tuple.5`+`tuple.12` into 18) are still the phantom-pair source of §4b Step 8
  at RUNTIME, but Stage A renders NO support token for them (E-A3 CLOSED: the
  `derivation_support` field is struck — candidate 1/A-nec-1); the real support
  algebra is Rel's.

No aggregate → `agg_input_key_ok=0` (vacuous), V-AGG-INPUT-KEY has nothing to
check. No `kMember` unprovable drop → `collapse_error=0`.

---

## 5. Witness 3 — `join_1`

### 5.1 The `.df` dump: DESIRED = byte-identical to CURRENT

The current 81-line dump is the desired dump UNCHANGED. The two `#query`
adornment joins (`q(free B)` and the provably-unsat `never(free B)`) render
exactly as today. Restating the join a reader might expect a member-key
annotation on:

CURRENT (and DESIRED — no diff):

```text
join ^join.10 (c14:i32, B:i32) {
  pivot c14:i32 <- .in0.c8, .in1.c10
  out B:i32 <- .in0.B
}
  ATTRIBUTES eqset=11 class=table-less stratum=8
  => ^tuple.4 (B)
```

**Contract:** the pivot/out block and the `ATTRIBUTES` line are untouched; the
member key lands in `-contract-out`, not here.

### 5.2 The `-contract-out` dump: DESIRED (new, fresh golden)

`join_1` demonstrates the JOIN transfer rule (key = union of mapped contributor
keys, no minimization — deferred to Stage B) and the COMPARE transfer rule
(preserve input key; the compared `eq` columns become value/presence
requirements, NOT key members). Blocks in `^id` order:

> **AMENDED 2026-08-02 (flat-key RowContract).** `support=` and `candidates={ … }`
> struck from every block. `join_1` has NO multi-view stratum (every view is its
> own stratum — verified against the `.df`), so the H-A3 Phase 1 cycle rule fires
> on zero views; the acyclic JOIN arm's union of mapped contributor keys already
> collapses each pivot pair to the single pivot output, so `key=(c14,B)` /
> `(c16,B)` are unchanged (no minimization needed). `contracts=20` unchanged.

```text
contracts

select ^select.0 (A:i32, B:i32)
  role=n/a key=(A,B)
select ^select.1 (A:i32)
  role=n/a key=(A)
select ^select.2 (c4:i32)
  role=n/a key=(c4)
select ^select.3 (c5:i32)
  role=n/a key=(c5)
tuple ^tuple.4 (B:i32)
  role=distinct key=(B)
tuple ^tuple.5 (B:i32)
  role=distinct key=(B)
tuple ^tuple.6 (c8:i32, B:i32)
  role=member key=(c8,B)
tuple ^tuple.7 (c10:i32)
  role=member key=(c10)
tuple ^tuple.8 (c11:i32, B:i32)
  role=member key=(c11,B)
tuple ^tuple.9 (c13:i32)
  role=member key=(c13)
join ^join.10 (c14:i32, B:i32)
  role=n/a key=(c14,B)                          # pivot c14 (acyclic union)
join ^join.11 (c16:i32, B:i32)
  role=n/a key=(c16,B)                          # pivot c16 (acyclic union)
compare ^compare.12 (c18:i32, B:i32)
  role=n/a key=(c18,B)                          # eq; compared cols = value req
compare ^compare.13 (c20:i32, B:i32)
  role=n/a key=(c20,B)
compare ^compare.14 (c22:i32)
  role=n/a key=(c22)
compare ^compare.15 (c23:i32)
  role=n/a key=(c23)
compare ^compare.16 (c24:i32, B:i32)
  role=n/a key=(c24,B)
compare ^compare.17 (c26:i32, B:i32)
  role=n/a key=(c26,B)
insert ^insert.18 (B:i32)
  role=n/a key=(B)
insert ^insert.19 (B:i32)
  role=n/a key=(B)
census: views=20 contracts=20 role{distinct=2 member=4 na=14}
        agg_input_key_ok=0 collapse_error=0
```

**Per-construct contract:**

- `select.0-3` — declared/const relations; `key=AllFields`. `select.2/3` are the
  const inputs (`c4`/`c5`) folded in by `ConvertConstantInputsToTuples`.
- `tuple.4/5` — the `q` and `never` query clause heads, `kDistinct`, `key=(B)`.
- `tuple.6-9` — internal `p`/`r` intermediate facades, `kMember`, key-preserving.
- `join.10/11` — §4.4 "Join" (acyclic): `key` = the union of mapped contributor
  keys, which already collapses each pivot pair to its single pivot output
  (`(c14,B)` / `(c16,B)`) — NO minimization step (deferred to Stage B) and NO
  candidate antichain (struck under flat-key). `join.11` feeds the unsat `never`
  chain; its contract is well-formed even though the join is provably empty (the
  unsat is a value-requirement conflict, not a member-key failure).
  > **AMENDED 2026-08-02 (flat-key + Minimize-deferred).**
- `compare.12-17` — §4.4 "Filter/compare": `key` = input key preserved; the
  compared `eq` columns become value requirements, never key members. `role=n/a`.
- `insert.18/19` — passthrough into `%table:6`/`%table:9`.

No aggregate, no `kMember` unprovable drop → `agg_input_key_ok=0`,
`collapse_error=0`.

---

## 6. Consolidated exit-gate restatement (desired-state acceptance)

| Surface | Desired vs current | Referee |
|---------|--------------------|---------|
| `agg_distinct_1.df` / `demand_tc_witness.df` / `join_1.df` | BYTE-IDENTICAL | byte-compare |
| every `.irgold`-pinned surface × mode (24 today: 5 `.df.opt` + 12 `.rel.opt` + 3 `.rel.{nocf,nodf,none}` + 2 `.h.opt` + 2 `.ir.opt`; AMENDED 2026-08-03, panel close-out: testability-oracle-5) | BYTE-IDENTICAL | `runall.sh` run_irgold byte-compare |
| `agg_distinct_1` stdout (+ oracle/monotone/batches) | BYTE-IDENTICAL | `runall.sh` + `bin/Oracle` |
| `agg_distinct_1` stderr | ZERO warnings (loses the 5 advisory lines) | explicit expected-diagnostics assertion = 0 warnings (T-oracle-4) |
| NEW `*.contract.opt.golden` (×3 here + witness set) | FRESH (§3.3/§4.2/§5.2) | byte-compare + review bless; permcheck N/A |

> **AMENDED 2026-08-02 (T-oracle-4, stage-a-diff.md H-A6/H-A9).** The
> `agg_distinct_1` stderr row is upgraded from "not suite-compared; witness note"
> to an EXPLICIT expected-diagnostics assertion of ZERO warnings after the
> lint→contract swap — "stdout unchanged" is not coverage; the zero-warnings
> count is pinned. (The `member_collapse_1` MODE-SPLIT negative witness lives in
> stage-a-diff.md's exit gate, not in this desired-IR-state doc, which pins only
> the three positive witnesses' `.df`/contract surfaces.)

Hard requirement (E-A1): if the H-A2 role-in-identity refinement forces ANY of
the three `.df` dumps to change, that is a merge folding a member-preserving
projection into a set-collapsing one — CORRECT-BY-DESIGN but it blocks on I0
adjudication, never blessed on vibes. The desired state asserts the change is
ZERO (verified structurally: no witness has a `kMember`/`kDistinct` pair that is
otherwise structurally identical over identical i/o).

---

## 7. ALTERNATIVES — the in-`.df` variant (adjudication RESOLVED: separate sink ratified)

> **AMENDED 2026-08-02 (D2.3 RATIFIED, stage-a-diff.md H-A8).** The owner has
> RATIFIED the separate `-contract-out` sink: `.df` stays byte-untouched, and
> contracts live in their own `*.contract.opt.golden` keyed by the same view ids.
> This section is RETAINED as the historical adjudication input (the in-`.df`
> variant it sketches was considered and NOT taken); the axis in §7.2 is now
> DECIDED in favor of the separate sink, not open.

The session charter expected the contract to live IN the `.df` dump. Here is
what that would look like for `agg_distinct_1`, so the adjudicator could weigh it
against the stage doc's separate-sink choice — the choice now ratified (D2.3).

### 7.1 In-`.df` desired shape (the road not taken)

The projection role and member key ride the `ATTRIBUTES` line; aggregates gain
an `input_key=` token. CURRENT → in-`.df`-VARIANT for the shared over-body head
and its aggregate:

CURRENT:

```text
tuple ^tuple.2 (X:i32, W2:i32)
  ATTRIBUTES table=%table:25 eqset=3 class=monotone stratum=2
  => ^aggregate.9 (X, W2)
  => ^aggregate.10 (X, W2)
```

IN-`.df` VARIANT:

```text
tuple ^tuple.2 (X:i32, W2:i32)
  ATTRIBUTES table=%table:25 eqset=3 class=monotone stratum=2 role=distinct member_key=(X,W2)
  => ^aggregate.9 (X, W2)
  => ^aggregate.10 (X, W2)
```

CURRENT:

```text
aggregate ^aggregate.9 (X:i32, N:i32)              ; cinv
  ATTRIBUTES table=%table:5 eqset=6 class=differential stratum=5
  => ^tuple.5 (X, N)
```

IN-`.df` VARIANT:

```text
aggregate ^aggregate.9 (X:i32, N:i32)              ; cinv
  ATTRIBUTES table=%table:5 eqset=6 class=differential stratum=5 member_key=(X) input_key=(X,W2)
  => ^tuple.5 (X, N)
```

The identity/collapse story would then be visible in ONE dump: a reviewer sees
`^tuple.2 role=distinct member_key=(X,W2)` with its two aggregate successors
right there, no second file.

### 7.2 The trade-off, stated (for the adjudicator)

**In favor of the in-`.df` variant (one-surface observability):**
- The contract travels WITH the graph the reviewer already reads; no `-contract-out`
  flag, no second golden family, no side-by-side file alignment.
- The five-sites → three-nodes collapse and each node's role/key are legible in
  the same place as the successor edges that PROVE the CSE merge.

**Against (the stage doc's reasons for the separate sink):**
- **Dump-golden churn.** Every `ATTRIBUTES` line across the 180-case corpus
  changes shape → mass re-bless of all 5 `.df.opt.golden` (and the risk of a
  ripple into any tooling that parses `ATTRIBUTES`). The separate sink touches
  ZERO existing golden.
- **permcheck scope.** The `.df` dump is byte-compared today with no permutation
  token; adding a per-view contract token keeps it byte-compared, but it enlarges
  the golden surface that a future permutation-bearing change must reason about,
  and couples contract determinism to `.df` determinism. The separate sink keeps
  the contract's "no order-free field, pure byte-compare" determinism (§2.2)
  isolated.
- **F1-lesson orthogonality.** Rendering the contract on the persisted `.df`
  ATTRIBUTES line invites the reading that the contract is a satellite annotation
  the graph CARRIES (which CSE/canon would then have to preserve). It is not — it
  is recomputed post-Optimize (H-A3/H-A4). The separate, post-Optimize-only sink
  makes the "recomputed, not carried" property structurally obvious; the in-`.df`
  rendering blurs it.
- **Mode coupling.** `.df` is dumped in all four modes; contracts are opt-mode
  only (§2.2). An in-`.df` contract token would either render in all four modes
  (three of them semantically dubious for a post-Optimize property) or force a
  per-mode conditional in the `.df` emitter.

**AXIS RESOLVED (AMENDED 2026-08-02, D2.3):** one-surface reviewer ergonomics
(in-`.df`) versus zero-golden-churn + isolated contract determinism + explicit
recomputed-not-carried semantics (separate `-contract-out`). The stage doc chose
the latter; the charter leaned the former; the OWNER RATIFIED the latter (D2.3).
Both desired shapes remain fully sketched above for `agg_distinct_1` as the
record of what was weighed; the separate `-contract-out` sink is the decided
realization.

---

## 8. Structured summary

- **Surface:** the `.df` dump after Stage A (DESIRED = byte-identical to tip for
  all three witnesses and, per exit gate, the whole corpus) + the NEW opt-in
  `-contract-out` dump (concretely sketched per witness, §3.3/§4.2/§5.2).
- **Primary-witness contract (`agg_distinct_1`):** five source lint sites
  (`.dr:31/32/35×2/38`) collapse onto THREE `.df` projection nodes — `^tuple.2`
  is CSE-shared between `cnt_inv`/`cnt_rec` (sites 1&2), `^tuple.3` carries two
  drops (sites 3&4), `^tuple.4` carries site 5. All three are `kDistinct`; all
  five verdicts are **V-NO-COLLAPSE PASS (validated-safe)** and each fed
  aggregate is **V-AGG-INPUT-KEY PASS** (`input_key` = the over() list). Census:
  `role{distinct=7 member=0 na=10} agg_input_key_ok=4 collapse_error=0`. stdout
  byte-identical; stderr loses 5 advisories.
- **Determinism contract:** `.df` — det_seq (`^id`) order, pure graph function,
  byte-compare, no permcheck (unchanged from tip). `-contract-out` — SAME `^id`
  order, one block per live view under the V-CONTRACT-CENSUS bijection tripwire,
  every field byte-exact (no order-free field → permcheck N/A), OPT-MODE ONLY
  (`.contract.opt.golden`). The census line is a pure function reporting
  `views/contracts/role{}/agg_input_key_ok/collapse_error`.
  > **AMENDED 2026-08-02 (flat-key).** The struck clause "`support=` is
  > SHAPE-ONLY (E-A3)" is GONE: the `support=` token and the `candidates=`
  > antichain are struck from the dump (D3.4 candidate 1/A-nec-1 + candidate 3);
  > the per-view surface is `role=` / `key=` / aggregate-only `input_key=`. E-A3
  > is CLOSED. On a recursive SCC (multi-view stratum) `key=AllFields` from the
  > H-A3 Phase 1 cycle rule.
- **Adjudication inputs flagged:**
  - **(§7) in-`.df` vs separate-sink — RESOLVED (AMENDED 2026-08-02, D2.3):** the
    owner RATIFIED the separate `-contract-out` sink; the in-`.df` shape sketched
    in §7 is the retained record of the road not taken. No longer open.
  - **(E-A1) `.df` dump divergence** — desired state asserts ZERO `.df` change;
    any actual change from role-in-identity blocks on I0, never blessed on vibes.
  - **(E-A2) V-NO-COLLAPSE reachability** — `role{member=…}` is 0 (agg_distinct_1)
    / >0 but all key-preserving (demand_tc, join_1); NO witness exhibits a
    `kMember` unprovable drop, so V-NO-COLLAPSE fires on zero corpus sites. The
    deleted lint's user-facing replacement is typed-away ambiguity + internal
    belts; whether a user-facing reject surface is still owed is the open item.
  - **(E-A3) `support=` — CLOSED (AMENDED 2026-08-02):** struck entirely under
    flat-key (Stage A transfers neither support shape nor counts); only the H-A1
    domain TYPES survive, in the static_assert battery. The real support algebra
    is Rel's.

> **RECONCILED 2026-08-03 (post-implementation, predict-then-verify close).**
> Stage A landed; the produced `-contract-out` dumps match this doc EXACTLY on
> `key=`, `input_key=`, the census bijection, and every cyclic AllFields key —
> the ONLY divergence was the `role=` tally, and the PRODUCED dumps won the
> adjudication (blessed as the goldens): the build-time kDistinct clause-head
> stamp does not survive `ConnectInsertsToSelects`/canonicalization PROXYING
> (surviving projections are kMember proxies; the sole surviving kDistinct is
> a clause head that RENAMES columns and feeds a JOIN — demand_tc's tuple.2,
> not tuple.9). Blessed censuses: agg_distinct_1 `role{distinct=0 member=7
> na=10}`, demand_tc_witness `role{distinct=1 member=10 na=9}` (tally as
> predicted, the distinct SURVIVOR flipped to tuple.2), join_1
> `role{distinct=0 member=6 na=14}`. The per-block `role=` lines below that
> disagree are superseded by the blessed goldens. Open owner item (recorded in
> the adjudication record): whether Stage B should INHERIT roles through proxy
> mints so set-boundary provenance survives onto the final graph.
