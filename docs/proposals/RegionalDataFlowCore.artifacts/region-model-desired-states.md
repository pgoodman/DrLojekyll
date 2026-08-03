# Region-model desired IR output states — the DIFF-R1..R6 ADDENDUM (session 3)

Authored 2026-08-03, branch `keyed-instances`. This is the **predict-then-verify
target** artifact for the DIFF-R1..R6 formalization (`region-model-diffs.md`). It
is an ADDENDUM, not a replacement: it extends the session-1 desired-state docs

- `regional-dump-stage-b-desired-states.md` (the G1 `-region-out` grammar — G1
  is RATIFIED, D2.2),
- `rel-stage-c-desired-states.md` (the `.rel` lifecycle census + region sections),
- `header-stage-c-desired-states.md` (the `datalog.h` collapse),

by rendering the DIFF-R1 REQUEST-EDGE node and its edge_kind (DIFF-R4) on
**freshly collected dumps** (minutes-old, quoted verbatim below; NEVER from
memory) and by APPLYING the owner adjudications those session-1 docs pre-dated
(ADJ-2 region-internal `demand__`, ADJ-3 all-free = no request port, the
`rel[Bound...](Free...)` notation, the `cluster_region_<id>` / one-box
key-invariant DOT directive). Where this addendum's rendering DIFFERS from a
session-1 doc, the difference is a **ratified-adjudication reconciliation** and is
flagged as such (§5). No production code, no golden change — design only.

## 0. Fresh-dump provenance (the before-state; quoted, never remembered)

Three programs compiled minutes ago at the `keyed-instances` tip:

- `demand_tc_witness` — `-demand` (from its `.drflags`); right-linear TC, one
  bound query `reachable_from(bound From, free To) : path(From, To)`. The flat
  guard-web demand carrier; `kSubgraphInstantiate=0` (no instance store).
- `tc_nonlinear_diff` — plain (no demand); non-linear TC over a `@differential`
  `add_edge`; all-free query `reachable(free From, free To)`. The no-demand
  recursive control.
- `minidis` — plain; source of the `cluster_stratum_21` DOT shape reference.

The four load-bearing before-lines, quoted from the fresh dumps:

    ; demand_tc_witness.rel op.1 (the fabricated demand ingest — REPLACED by §1)
    op.1 kIngestFold sign=+ ctx=eager stratum=0
        effects: {kCounter(%table:23, +, NonRecursive)}
        spine: —
        args: table=%table:23 message=demand__reachable_from_bf/1

    ; demand_tc_witness.rel census (29 kinds — the SLOT §1 rewrites)
    census: … kIngestFold=2 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0 kEagerForward=12 kEagerInsert=2 … kEagerJoin=8 kEagerProduct=0 kIngestLoop=0 kJoinEmit=5 kProductEmit=0

    ; demand_tc_witness.dot — the interior the region cluster will wrap
    subgraph cluster_stratum_5 { label="stratum 5"; style="rounded,dashed"; v4329909280; … v42521987584; }

    ; minidis.dot — the stratum-cluster shape reference
    subgraph cluster_stratum_21 { label="stratum 21"; style="rounded,dashed"; v4318477936; v4318480272; v42396260864; … }

**Layering caveat (normative, read before every prediction below).** DIFF-R1 mints
a DataFlow-IR `QueryRequestImpl` NODE. Its FLAT (`-demand`, no `-demand-instance`)
lowering is asserted **byte-identical** to today (R1-a; the node lowers back to
the guard JOIN it replaced), and the DIFF-R1-lifecycle-2 amendment offers option
(A) *gate the node mint on `demand_instance_enabled`* — under which the flat
`.rel` shows **no** request-edge op at all. Therefore the request-edge OP
rendering in §1 is the **eventual region-model target** (DIFF-R1 node + the
Stage-C lifecycle-op cutover `stage-c-diff.md` H-G + DIFF-R4 edge_kind typing),
NOT the DIFF-R1-node-alone flat dump. Each prediction is tagged with its
realizing DIFF and the owner decision that invalidates it. `[pinned]` = exact
predicted bytes; `[shape]` = ids/counts drift in a greenfield cutover;
`[INVENTED]` = a token/id not derivable from an existing dump convention (a
determinism hazard, catalogued in §5).

---

## 1. demand_tc_witness `.rel` — the REQUEST-EDGE op family

**Realizes:** DIFF-R1 (the node) + `stage-c-diff.md` H-G (the lifecycle-op
vocabulary the node lowers onto) + DIFF-R4 (the `kind=` typing).
**Invalidated by:** the §6-vs-§11 routing rule (D1.5/D1.6, OPEN — STOP for a
recursive demanded body; demand_tc_witness's `path` IS a TC) and the
DIFF-R1-lifecycle-2 option A/B choice. The recursive `path` is a Variant-A/B
fork (`rel-stage-c-desired-states.md` §5 Adjudication B).

### 1.a The node label + edge_kind

The bound query `reachable_from(bound From, free To)` mints ONE request node.
Under the owner notation `rel[Bound...](Free...)` (record §337) — bracket = the
InstanceStore key (the bound columns), parens = the answer schema (the free
columns):

    node label:  reachable_from[From](To)          [pinned to the notation]
    edge_kind:   LAZY                               [pinned — DIFF-R4 rule]

`edge_kind=LAZY` because the request's answer is consumed by a **monotone**
consumer — the query observation / the recursive JOIN pivot, never a
NEGATE/AGG (DIFF-R4 inference rule, `region-model-diffs.md:1018`). This is the
already-landed regime (the e5 soundness-by-irrevocability case). `FORCE_COMPLETE`
is **unreachable on this carrier** (no negation/aggregate in the demanded body;
NEGATE/AGG still fenced at `Demand.cpp:648-651`) — the token is declared, inert
until DIFF-R4 lands a negate/agg carrier.

The bracket here is a **single column** (`From`), so no physical bracket ORDER
question arises. A multi-column bracket's rendered ORDER (`rel[A,C]` vs
`rel[C,A]`) is the logical-key-vs-physical-arrangement split (record §435) and is
**DIFF-R5 — RE-AUTHORING IN FLIGHT** (`region-model-diffs.md:1329`); this
addendum renders only the single-column bracket and takes no order position.

### 1.b What replaces today's guard-join lines

The fresh `op.1 kIngestFold … message=demand__reachable_from_bf/1` (the fabricated
demand-seed ingest) and the **query-projection guard JOIN** it feeds (Step 8,
`Demand.cpp:1105`; one of the fresh dump's 8 `kEagerJoin` ops) are subsumed into
ONE lifecycle op. The **body** guards (the push-down `d_path ⋈ path` JOIN,
Step 7) are DIFF-R1 region interior and **stay** (minimal-cut thesis, R1-b) —
DIFF-R1 promotes ONLY the query-projection guard, never the body guards.

Desired replacement op **[shape]**:

    op.<r> kRequestEdgeAdd sign=+ ctx=eager stratum=0
        reads: —
        effects: {kVecDrain(%table:23, kNetAddition)}
        args: edge=%table:23 node=reachable_from[From](To) kind=LAZY owner=RootLease call-site=cs0 routes-to=%table:4

- `edge=%table:23` REUSES the fabricated demand relation's physical table id
  (the `.rel` edge-rel convention, `rel-stage-c-desired-states.md` §3.1: "REUSES
  the current demand table's physical table id"). `%table:23` is where the fresh
  `op.1` folds. **[shape — physical id]**
- `owner=RootLease call-site=cs0` are the exact-ownership columns the
  demand-row-presence model lacked (F4 fix); `RootLease` because the observation
  root owns the one-level request. **[INVENTED — `RootLease`/`cs0`]**
  **DETERMINISM CRITIQUE 2026-08-03 (D2, applied):** the ORDINAL in `cs0`/`e0`
  must key on `forcing_index` (mint-order, Optimize-immune), NOT on a post-Optimize
  `RequestNodes()` traversal — the panel record flags that enumeration as
  un-belted and live (`region-model-diffs.md:205` lifecycle-3, `:241` necessity-5),
  so numbering by it would leak enumeration/hash order into the dump. The single
  request here is `forcing_index==0 ⇒ cs0/e0`; a multi-request program numbers in
  ascending `forcing_index`.
- `routes-to=%table:4` is the pub (`reachable_from`'s MATERIALIZE table in the
  fresh `.df`). **[shape]**
- Sign `+` / `ctx=eager` / `stratum=0` mirror the retired `op.1`'s attributes so
  the Kahn linearization slots the op where the ingest fold sat (op-order
  preservation, `rel-stage-c-desired-states.md` §6). `op.<r>` is the linearizer's
  assigned index. **[shape — op index]**

### 1.c The census delta — every new kind and its count

The 29-kind census becomes the 36-kind census (`29 − 3 + 10`): DELETE
`kSubgraphInstantiate`/`kInstanceDeath`/`kInstanceSeal` (all already `=0` here),
INSERT the ten `RelRegionLifecycle` kinds in the **same slot**, in the pinned
field order of `rel-stage-c-desired-states.md` §1
(`request_edge_add, request_edge_remove, input_delta, local_fixpoint,
child_result_add, child_result_remove, routed_result_add, routed_result_remove,
retire_inactive, seal_epoch`).

**PREDICTION P-rel-target** (the eventual region-model target — DIFF-R1 node
surfaces as a lifecycle op; the `demand__` web retires under H-C/Variant-A)
**[shape on the eager tail, pinned on the lifecycle block]**:

    census: … kIngestFold=1 kGroupUpdate=0 kStateSeal=0 kRequestEdgeAdd=1 kRequestEdgeRemove=0 kInputDelta=0 kLocalFixpoint=0 kChildResultAdd=0 kChildResultRemove=0 kRoutedResultAdd=0 kRoutedResultRemove=0 kRetireInactive=0 kSealEpoch=0 kEagerForward=12 kEagerInsert=2 kEagerCompare=0 kEagerGenerate=0 kEagerUnion=0 kEagerSelect=0 kEagerJoin=<8−g> kEagerProduct=0 kIngestLoop=0 kJoinEmit=<5−g'> kProductEmit=0

Named new-kind counts on THIS carrier:

| new census kind | count | why |
| --- | --- | --- |
| `kRequestEdgeAdd` | **1** | the one bound query → one request edge |
| `kRequestEdgeRemove` | **0** | demand is MONOTONE here (bare `-demand`, no `-demand-retract`) → add-only-by-irrevocability, the lifecycle-4 carve-out (`region-model-diffs.md:209`); this is the request-edge analog of today's `kInstanceDeath=0` on a mono witness |
| `kInputDelta` … `kSealEpoch` (8 kinds) | **0** | flat = the zero-replication degenerate (record §205): no child region, the key stays a row column, the request merely filters. No instance is born, sealed, or routed |

Body-count deltas: `kIngestFold` 2 → **1** (the `demand__` ingest retires; only
`edge_2/2`'s fold survives); `kEagerJoin` 8 → `8 − g` and `kJoinEmit` 5 → `5 − g'`
where `g`/`g'` are the retired query-projection guard's join/join-emit
contribution. `g` is **not derivable from the fresh dump alone** (the dump does
not tag which of its 8 joins is the query-projection guard) — **[shape,
determinism hazard §5]**.

**PREDICTION P-rel-min** (the DIFF-R1-lifecycle-2 **option A** slice — gate the
node on `demand_instance_enabled`; flat stays byte-identical) **[pinned]**: the
whole op body, `vec`, `branch`/`join`, `rounds:`, `deps:` are byte-identical; the
census line changes ONLY by the 3-out/10-in field-set swap with EVERY lifecycle
field `=0` and `kIngestFold=2` intact:

    census: … kIngestFold=2 kGroupUpdate=0 kStateSeal=0 kRequestEdgeAdd=0 kRequestEdgeRemove=0 kInputDelta=0 kLocalFixpoint=0 kChildResultAdd=0 kChildResultRemove=0 kRoutedResultAdd=0 kRoutedResultRemove=0 kRetireInactive=0 kSealEpoch=0 kEagerForward=12 kEagerInsert=2 kEagerCompare=0 kEagerGenerate=0 kEagerUnion=0 kEagerSelect=0 kEagerJoin=8 kEagerProduct=0 kIngestLoop=0 kJoinEmit=5 kProductEmit=0

P-rel-min is the **high-confidence** prediction (it follows from R1-a's
byte-identity thesis + the pinned 36-kind vocabulary); P-rel-target is the
end-state the request-edge op family renders once H-G lands and Variant-A resolves
the recursive `path`. The owner must pick option A vs B before P-rel-target's
`kRequestEdgeAdd=1` becomes real; until then the byte-identical P-rel-min holds.

---

## 2. `-region-out` G1 blocks (D2.2 grammar, ratified)

**Realizes:** DIFF-R1's `-region-out` surface (R1-f) rendered in G1 (the RATIFIED
grammar, D2.2), extending `regional-dump-stage-b-desired-states.md` §2 with the
Stage-C `request-edges{…}` sub-block (session-1 §2's own extension contract).
**Invalidated by:** the D2.6 reader-handle schema (the `owner=`/`call-site=`
columns' exact spelling) and the option A/B choice (whether a flat program renders
a request-edge at all).

Two ratified adjudications the session-1 renderings pre-dated are APPLIED here:

- **ADJ-2 (D2.8, ratified):** the fabricated `demand__…` input renders
  **region-internal**, NOT as a program-root `input-abi`. Session-1 §1.2 rendered
  it as a program-root input-abi flagged `[fabricated, driver-suppressed]`; this
  addendum moves it inside the region (dropping the program-root input-port count
  by one).
- **ADJ-3 (D2.8, ratified):** an all-free query renders as an **output/permanent
  root with NO request port** (request-port count = "has demand"). Session-1
  §1.1 rendered `join_1`'s all-free queries WITH `fields=()` request ports; this
  addendum drops them.

### 2.a demand_tc_witness (`-demand`, one request edge)

    region-program
    program-root {
      input-abi   edge_2/2(From:u64, To:u64)                  -> R0 via P1
      query-abi   reachable_from(From:bound u64, To:free u64) -> R0 via P0
      output-abi  <none>
    }
    region R0  owner=program-root  parents=()  children=()  key-invariant=(From) {
      request-port P0  query=reachable_from  fields=(From)
      input-port   P1  message=edge_2/2      fields=(From, To)
      region-internal demand__reachable_from_bf/1(<dcol>:u64)  [fabricated, driver-suppressed]   ; ADJ-2
      row-contract E0  rel=path            member-key=(From, To)  support=monotone
      row-contract E1  rel=reachable_from  member-key=(From, To)  support=monotone
      request-edges {
        edge e0  rel=reachable_from  key=(From)  answer=(To)  kind=LAZY  owner=RootLease  call-site=cs0  routes-to=E1
      }
    }
    census: regions=1 child-calls=0 program-roots=1 request-ports=1 input-ports=1 result-ports=0 row-contracts=2 request-edges=1

Deltas from `regional-dump-stage-b-desired-states.md` §2.2 (all flagged):

- `input-ports` **2 → 1**: the `demand__…` message moves region-internal (ADJ-2),
  so it is no longer a program-root input-port. **[reconciliation — ADJ-2]**
- `key-invariant=(From)` appears on the region header — the one-box replication
  schema (record §194: "a demand/region cluster renders ONE box annotated
  `key-invariant: <K>`"). **[INVENTED token — `key-invariant=`]**
- `request-edges { edge e0 … }` sub-block APPENDED after the row-contracts
  (session-1 §2 Stage-C extension contract: appended so skeleton lines stay
  byte-identical). `kind=LAZY` carries the DIFF-R4 typing. **[INVENTED tokens —
  `edge e0`/`call-site=cs0`/`RootLease`; `kind=` from DIFF-R4]**
  **DETERMINISM CRITIQUE 2026-08-03 (D1, applied — was: "the first order-free
  multiset a permcheck referee compares"):** two corrections. (a) The EMITTED
  byte order must be a pure graph function regardless of any permcheck allowance —
  a stated emission SORT KEY, else a multi-edge program leaks enumeration order.
  Emission order = ascending `forcing_index`, tie-broken by `(rel, key-cols,
  answer-cols)`; the single-edge witnesses here do not exhibit the hazard but the
  convention needs the key. (b) permcheck order-freedom is NOT yet established:
  `region-model-diffs.md:152` says the block "MAY EARN permcheck order-free
  diffability at Stage C" (conditional/future) and `permcheck.py` today
  order-compares ONLY published-delta tokens. Until permcheck is taught this
  block, it is a byte-identical (sorted) block, not an order-free one.
- `census:` gains `request-edges=1` (the eighth census field). **[INVENTED census
  field]**

`<dcol>` (the fabricated column name, `c3` today) is the only metavariable
(shape-exact, may drift) — inherited from session-1's determinism contract.

### 2.b tc_nonlinear_diff (no demand — the ADJ-3 control)

    region-program
    program-root {
      input-abi   add_edge/2(From:u64, To:u64)  [differential]  -> R0 via P0
      query-abi   reachable(From:free u64, To:free u64)  -> permanent-root   ; ADJ-3: all-free, NO request port
      output-abi  <none>
    }
    region R0  owner=program-root  parents=()  children=()  key-invariant=() {
      input-port     P0  message=add_edge/2  fields=(From, To)
      permanent-root reachable(From, To)                                     ; ADJ-3
      row-contract   E0  rel=tc    member-key=(From, To)  support=differential
      row-contract   E1  rel=edge  member-key=(From, To)  support=differential
    }
    census: regions=1 child-calls=0 program-roots=1 request-ports=0 input-ports=1 result-ports=0 row-contracts=2 request-edges=0

Grounding (from the fresh dumps + `.dr`):

- `add_edge/2 @differential` is the ONE received message → `input-ports=1`.
  `support=differential` on both contracts because `add_edge` is `@differential`
  (deletions flow; `tc` is on a cycle → member-key = AllFields = `(From,To)`,
  D1.2). **[grounded]**
- `reachable(free, free)` → `permanent-root`, `request-ports=0`, `request-edges=0`
  (ADJ-3). This is the whole point of the control: a no-demand recursive program
  has ZERO request ports and ZERO request edges — the empty-multiset baseline for
  the permcheck referee. **[reconciliation — ADJ-3; INVENTED token
  `permanent-root`]**
- `key-invariant=()` (empty) — no demand ⇒ no keyed region ⇒ zero-replication
  (record §205). The region exists (Stage B: one ProgramRoot + one R0) but
  parameterizes on nothing. **[INVENTED — empty key-invariant rendering]**
- `tc` and `edge` are the two `#local`s (decl order `tc`, then `edge`) →
  `row-contracts=2` (row-contracts count RELATION DECLS in decl order —
  `regional-dump-stage-b-desired-states.md:82`, not member-role views).
  `reachable` is a table-less query projection of `tc` (no stored contract),
  matching session-1's `join_1` treatment of `q`/`never`.
  **DETERMINISM CRITIQUE 2026-08-03 (D3, applied — was: "[grounded — contract
  dump `role=member` ×2 for the stored bodies]"):** that citation MISREAD the
  fresh dump. `tc_nonlinear_diff.contract` census shows `role{... member=5 ...}`,
  not ×2 — the per-VIEW member-role count (5) is not the per-RELATION row-contract
  count (2). row-contracts=2 is right by the relation-decl definition; the "×2"
  grounding conflated the two counts and is struck. **[grounded — 2 `#local`
  relation decls; the fresh contract census member=5 is a per-view count, not the
  row-contract count]**

---

## 3. DOT twins (advisory; owner directive 2026-08-03)

**Realizes:** the `cluster_region_<id>` DOT twin directive (record §133; owner
directive "request edges as inter-cluster edges when Stage C lands") + the
one-box key-invariant rendering (record §194) + the interior-SCC nesting (record
§257: "interior SCC boxes nest INSIDE region clusters"). **Invalidated by:**
nothing goldened — DOT stays **ADVISORY, never byte-goldened** (record §142); the
G1 text dump remains the referee. The `vNNNN` node ids are pointer-derived and
DRIFT per run (already true in the fresh `-dot-out`); they are NOT a determinism
target.

### 3.a demand_tc_witness — region cluster wraps the stratum cluster

The fresh `demand_tc_witness.dot` has `subgraph cluster_stratum_5 { … 11 nodes … }`.
Under the region model that stratum cluster becomes the INNER box of the
region cluster, and the request edge enters from the program-root observation:

    digraph {
    bgcolor="#f0f4f7";
    node [shape=none margin=0 nojustify=false labeljust=l font=courier];
    compound=true;                                   ; enables lhead/ltail inter-cluster edges

    subgraph cluster_region_0 {
      label="region R0  |  key-invariant: From";     ; the ONE box, annotated (record §194)
      style="rounded,bold";
      color="#3a6ea5";
      subgraph cluster_stratum_5 {                   ; the interior SCC, INNER box (record §257)
        label="stratum 5";
        style="rounded,dashed";
        v4329909280; v4329920384; v4329921408; v4329916128; v4329916800;
        v42521987072; v42521988096; v42521989120; v42521986560; v4329915296; v42521987584;
      }
      ; … the region's acyclic connective tissue (path UNION, TABLE 8/12) …
    }

    ; program-root observation node (OUTSIDE any region cluster)
    q_reachable_from [label=<<TABLE border="1" bgcolor="white"><TR><TD>QUERY reachable_from</TD><TD port="p0">From</TD><TD port="p1">To</TD></TR></TABLE>>];

    ; THE REQUEST EDGE — inter-cluster, program-root -> region R0, labeled (DIFF-R4 kind)
    q_reachable_from -> v4329915296 [
        label="reachable_from[From](To)  kind=LAZY",
        lhead=cluster_region_0,
        style=dashed, color="#3a6ea5", fontname=courier];
    }

Notes: `compound=true` + `lhead=cluster_region_0` makes the arrow terminate on
the region BOX boundary (a port on the cluster boundary, the owner's Stage-B port
directive). The interior `vNNNN` ids are copied verbatim from the fresh dump but
DRIFT run-to-run (pointer-derived) — advisory only. The edge label is exactly the
G1 `request-edges` line's `rel[Bound](Free) kind=…` projection, so the DOT and G1
tell the same story. **[shape — node ids drift; INVENTED — `cluster_region_0`,
the `key-invariant:` label, the request-edge label composition]**

### 3.b tc_nonlinear_diff — a region box with NO request edge

    subgraph cluster_region_0 {
      label="region R0  |  key-invariant: (none)";   ; no demand ⇒ empty key
      style="rounded,bold"; color="#3a6ea5";
      subgraph cluster_stratum_<s> { label="stratum <s>"; style="rounded,dashed"; … }
    }
    ; reachable renders as a permanent root (ADJ-3) with a PLAIN (non-request) edge
    reachable_root -> v<...> [style=solid];           ; NOT dashed, NOT labeled kind= — it is not a request

The contrast is the point: a region box with `key-invariant: (none)` and ZERO
dashed inter-cluster request edges is the visual of a no-demand program; the
all-free `reachable` is a permanent root reaching in with an ordinary (solid,
unlabeled) edge. This makes ADJ-3 legible at a glance. **[advisory]**

---

## 4. Generated-header section — STUB (authoring PAUSED on D2.6)

**Realizes:** nothing yet. **Blocked by:** D2.6 (the reader-handle schema —
DEFERRED, `owner-adjudication-record.md:60`). Per the charter, **Stage-C header
authoring is PAUSED for the D2.6 re-brief** (record §66, §126). This section is
deliberately a stub; the full desired header lives in
`header-stage-c-desired-states.md` (748 lines) and is likewise D2.6-gated on its
load-bearing construct.

**What D2.6 gates (CANNOT be authored now):** the request-edge **row type** and
the cursor's lease member. D2.6 decides whether a concurrent same-key request is
INDIVIDUALLY retractable — then the row is owner-bearing `RowReq{owner, key}` — or
a refcount suffices (anonymous handle count). Until that answer:

- the `RootRequestLease` / request-edge row struct schema
  (`header-stage-c-desired-states.md` §1.2) — **PAUSED**;
- the cursor's move-only lease member + the forcing-inject → lease-acquisition
  retype (`header-stage-c-desired-states.md` §1.7) — **PAUSED**;
- the acquire-proc signature (`header-stage-c-desired-states.md` §1.4) — **PAUSED**.

**What does NOT depend on D2.6 (authorable now, but only NAMED here — the exact
bytes live in `header-stage-c-desired-states.md`):**

- **`join_1` header byte-identity** (`header-stage-c-desired-states.md` §3): no
  demand ⇒ no request edge ⇒ no lease ⇒ the hidden-friend surface is
  byte-unchanged. This is the header analog of §1's no-demand control and is
  D2.6-independent — it can be pinned the moment Stage C authoring resumes.
- **The nested `InstanceStore` key/row struct DELETION**
  (`header-stage-c-desired-states.md` §1.3, §2.1): D8-driven (results store once
  in the ordinary pub), NOT D2.6-driven. The deletion is authorable; only the
  request-edge row type that REPLACES the store threading is D2.6-gated.
- **The demand-message hidden-friend + `*_detail`/`inject` proc DELETION**
  (`header-stage-c-desired-states.md` §1.4, §1.6; H-F/H-C): the `demand__…`
  message ABI entry and its detail procs retire with the fabricated message
  (grep gate: no `demand__` token). D2.6-independent.

The determinism contract for the header (four orderings, no census line,
`permcheck.py` never applies to `.h`) is stated in
`header-stage-c-desired-states.md` §0 and is unaffected by D2.6.

---

## 5. Determinism-hazard ledger (every invented id/token) + realization map

Each row is a token/id NOT derivable from an existing fresh-dump convention — a
determinism hazard the critique pass must adjudicate. "Realizing DIFF" and
"invalidating owner decision" per block.

| # | invented token/id | where | realizing DIFF | invalidated by |
| --- | --- | --- | --- | --- |
| H1 | `reachable_from[From](To)` — the **parens=Free-only** convention (vs the key-in-output `[From](From,To)` convention) | §1.a, §2.a, §3.a | DIFF-R1 (node label) | the owner's OPEN convention question: key-in-output vs sequestered (record §425, §429) — the two spellings are answer-compatible but render differently |
| H2 | `kRequestEdgeAdd=1` **with `kRequestEdgeRemove=0`** on a mono carrier | §1.c | DIFF-R1 lifecycle-4 carve-out | **DETERMINISM CRITIQUE 2026-08-03 (D4): SETTLED, not open.** `region-model-diffs.md:209` (lifecycle-4 AMENDMENT, NORMATIVE) already carves a monotone-demand `kRequestEdgeAdd` out of V-EDGE-BALANCE (add-only-by-irrevocability) ⇒ `kRequestEdgeRemove=0` is the RATIFIED prediction and this doc AGREES with the diffs panel record. The tension is ONLY with the un-ratified `rel-stage-c-desired-states.md` §4.2/§5.1 (H-G.3 mono-arm-collapse), which must YIELD to the panel record — the reconciliation burden is on rel-stage-c, not on this prediction. Downgraded from "highest-value hazard to resolve." |
| H3 | `LAZY` / `FORCE_COMPLETE` dump-token spelling (uppercase, vs code `kLazy`/`kForceComplete`) | §1.a, §2.a, §3.a | DIFF-R4 | task-pinned to uppercase; the code enum is `EdgeKind::kLazy` — the dump-render spelling is a choice (mirrors the `sign=+`/`ctx=eager` render-vs-enum split). **DETERMINISM CRITIQUE 2026-08-03 (D5): consistent with precedent.** `region-model-diffs.md`'s own DIFF-R4 inference-rule pseudocode (`:1018-1031`, and `:1042`ff) already renders the surface token uppercase `LAZY`/`FORCE_COMPLETE` while the enum is `kLazy`/`kForceComplete`. Cross-doc render token AGREES; low-risk. |
| H4 | `owner=RootLease`, `call-site=cs0`, `edge e0`, `routed rr#`, dense request-edge ids | §1.b, §2.a | DIFF-R1 (F4 ownership) | **D2.6** — the owner-bearing `RowReq{owner,key}` vs refcount choice fixes whether `owner=` is a real column or elided |
| H5 | `kEagerJoin=8−g` / `kJoinEmit=5−g'` — the retired query-projection guard's contribution `g` | §1.c | DIFF-R1 (R1-b minimal cut) | not derivable from the fresh dump (no join is tagged as the query-projection guard); needs a `-df`/`Demand.cpp` cross-check. A **body-count hazard** independent of the census field-set. |
| H6 | `kIngestFold` 2→1 (the `demand__` ingest retires) | §1.b, §1.c | H-C (fabricated-message retirement) | the option A/B choice: under DIFF-R1 option A the demand message STAYS and `kIngestFold=2` (P-rel-min) |
| H7 | `request-edges=1` — the 8th G1 census field | §2.a | session-1 §2 Stage-C census extension | D2.2 (whether the request-edge multiset is a census-counted field vs a permcheck-only block) |
| H8 | `key-invariant=(From)` / `key-invariant=()` G1 region-header token | §2.a, §2.b, §3 | record §194 (one-box replication schema) | the region-model surface (Stage-B/D dump grammar not yet frozen for the key-invariant annotation) |
| H9 | `region-internal <msg>` (ADJ-2) and `permanent-root <query>` (ADJ-3) G1 tokens | §2.a, §2.b | D2.8 (ADJ-2/ADJ-3 ratified) | the token SPELLING is a choice; the placement (region-internal / permanent-root) is ratified |
| H10 | `cluster_region_0`, the `key-invariant:` DOT label, the inter-cluster request-edge label composition | §3 | record §133/§194 DOT directive | nothing goldened (DOT advisory); `vNNNN` node ids drift (pointer-derived) — never a determinism target |
| H11 | `%table:23` (edge-rel) / `%table:4` (routes-to) physical id reuse | §1.b | `rel-stage-c` §3.1 (edge-rel reuses demand-table id) | greenfield table-id assignment; a `[shape]` reuse, not `[pinned]` |

### Sections written (summary)

1. **demand_tc_witness `.rel` request-edge op family** — node label
   `reachable_from[From](To)`, `edge_kind=LAZY` (DIFF-R4; `FORCE_COMPLETE`
   unreachable on this carrier), the `kRequestEdgeAdd` op replacing the
   `demand__` ingest + query-projection guard, and TWO census predictions
   (P-rel-target with named lifecycle counts; P-rel-min, the byte-identical
   DIFF-R1-option-A fallback).
2. **G1 `-region-out` blocks** for demand_tc_witness (one request edge, ADJ-2
   region-internal `demand__`, the appended `request-edges{…}` order-free
   multiset) AND tc_nonlinear_diff (no demand, ADJ-3 all-free = permanent-root,
   zero request ports/edges — the empty-multiset control).
3. **DOT twins** — `cluster_region_0` wrapping the fresh `cluster_stratum_5`
   inner box, one box annotated `key-invariant: From`, the request edge as a
   dashed labeled inter-cluster edge; the no-demand contrast (empty key, no
   dashed edge). Advisory, never goldened.
4. **Header STUB** — PAUSED on D2.6 (the reader-handle schema); names only the
   D2.6-independent surfaces (join_1 byte-identity, InstanceStore-struct/
   demand-message deletions) and cites `header-stage-c-desired-states.md`.

### Predicted census delta (headline)

demand_tc_witness census: **29 → 36 kinds** (delete
`kSubgraphInstantiate`/`kInstanceDeath`/`kInstanceSeal`, all `=0`; insert the ten
`RelRegionLifecycle` kinds in the vacated slot). Named nonzero new kind on the
carrier: **`kRequestEdgeAdd=1`** (mono ⇒ `kRequestEdgeRemove=0`, the H2 hazard);
the other nine lifecycle kinds `=0` (flat = zero-replication, no child region).
Body deltas in the eventual target: `kIngestFold` 2→1, `kEagerJoin`/`kJoinEmit`
drop by the retired guard's contribution `g`/`g'` (H5/H6). The G1 census gains a
`request-edges=1` field (H7). The safe fallback (P-rel-min / option A) keeps the
body byte-identical with every lifecycle field `=0`.

---

## 6. DETERMINISM CRITIQUE (2026-08-03) — findings + dispositions

Lens: is every predicted line a PURE, ORDER-STABLE function of the graph (ids from
a deterministic scheme, multisets under a stated sort key, no hash/pointer-order
leak, permcheck-compatible where order-freedom is claimed); are the `.rel` /
`-region-out` / DOT renderings of the same object byte-consistent; and does any
prediction contradict a normative amendment in `region-model-diffs.md`'s panel
records. Findings applied inline above are tagged **D1–D5**.

**D1 — request-edges emission lacks a stated sort key; permcheck order-freedom
overclaimed. [MAJOR — APPLIED, §2.a]** The doc called the `request-edges{…}`
block "the first order-free multiset a permcheck referee compares." Two defects:
(a) the EMITTED byte order must be a pure graph function irrespective of any
permcheck allowance — otherwise a multi-edge program leaks enumeration/hash order
into `.rel`/`-region-out`/DOT; the single-edge witnesses hide it. Fix: pinned the
emission sort key to ascending `forcing_index`, tie-break `(rel, key-cols,
answer-cols)`. (b) permcheck order-freedom is NOT established —
`region-model-diffs.md:152` says the block "MAY EARN" it "at Stage C" and
`permcheck.py` today order-compares only published-delta tokens. Fix: downgraded to
a byte-identical (sorted) block until permcheck is taught it.

**D2 — `edge e0`/`call-site=cs0` dense-id numbering key unstated. [MAJOR —
APPLIED, §1.b]** The ordinal that makes `e0`/`cs0` be 0-vs-N is a determinism
surface. The panel record flags `RequestNodes()` as an un-belted live post-Optimize
enumeration (`region-model-diffs.md:205` lifecycle-3, `:241` necessity-5) and names
`forcing_index` the stable Optimize-immune key. Fix: pinned the numbering to
ascending `forcing_index`, not `RequestNodes()` traversal order. (Additive to H4,
which covers only the token spelling / owner-column existence, not the ordinal.)

**D3 — §2.b grounding parenthetical misreads the fresh contract dump. [MINOR —
APPLIED, §2.b]** The claim "[grounded — contract dump `role=member` ×2]" for
tc_nonlinear_diff contradicts the freshly-quoted census `role{… member=5 …}`. The
conclusion `row-contracts=2` is correct by the session-1 relation-decl definition
(`regional-dump-stage-b-desired-states.md:82` — row-contracts count RELATION DECLS,
`tc`+`edge`), but the "×2" citation conflated the per-VIEW member count (5) with
the per-RELATION row-contract count (2). Fix: re-grounded on relation-decl order;
struck the "×2" claim. A "quote-never-remember" §0-discipline slip.

**D4 — H2 framed "highest-value hazard, needs owner adjudication" is already
settled by the panel record. [MINOR — APPLIED, §5 H2 row]** `kRequestEdgeRemove=0`
on a mono carrier does NOT contradict `region-model-diffs.md`; it AGREES with the
normative lifecycle-4 AMENDMENT (`:209`, mono add-only-by-irrevocability carve-out
of V-EDGE-BALANCE). The contradiction is only with the un-ratified
`rel-stage-c-desired-states.md` §4.2/§5.1, which must yield. Fix: downgraded H2,
recorded the panel record as the tie-breaker.

**D5 — H3 uppercase LAZY/FORCE_COMPLETE token consistent with precedent. [INFO —
APPLIED, §5 H3 row]** `region-model-diffs.md`'s DIFF-R4 inference rule (`:1018-1031`,
`:1042`ff) already renders the surface token uppercase against the `kLazy` enum.
The desired-states choice agrees; low risk. Fix: annotated H3.

**Cross-surface consistency — VERIFIED CLEAN (no edit).** `.rel`
`node=reachable_from[From](To) kind=LAZY`, `-region-out`
`rel=reachable_from key=(From) answer=(To) kind=LAZY`, and the DOT edge label
`reachable_from[From](To)  kind=LAZY` render the SAME object with the same id,
bracket/key order (`From`), answer (`To`), and edge_kind token (`LAZY`). The one
per-surface variance — `routes-to=%table:4` (`.rel` physical id) vs `routes-to=E1`
(G1 contract id) — is the correct dual id-scheme (physical op surface vs logical
contract surface), the same routing target, and is confirmed by the fresh
`.df`/`.rel` (`%table:4` = reachable_from's INSERT/pub, `%table:23` = the reused
`demand__reachable_from_bf/1` table). No token/id/order divergence found.

**No-contradiction check vs `region-model-diffs.md` panel records — PASS.** Every
prediction that touches a normative amendment (the mono carve-out `:209`; the
lifecycle-2 option A/B fork rendered as P-rel-min/P-rel-target; the LAZY-for-
monotone-consumer rule `:1018`; the DIFF-R5 single-column-bracket-only scope
`:1329`) is consistent with or explicitly deferred to the panel record. The
remaining self-declared hazards H1/H4–H11 are honestly tagged `[INVENTED]`/`[shape]`
and gated on the correct owner decisions; they are not determinism defects in the
rendered single-edge witnesses, only under-specified conventions for the multi-edge
/ D2.6 / DIFF-R5 futures.
