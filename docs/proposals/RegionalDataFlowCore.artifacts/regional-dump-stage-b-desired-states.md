# Stage B regional dump (`-region-out`) — desired states, all three grammars

Design session, 2026-08-02, branch `keyed-instances`, tip `f0c913e0`. This
is the GRAMMAR-DECISION STOP CONDITION artifact for the new `-region-out`
regional dump surface introduced by Stage B (stage-b-diff.md H6/H7). It
renders each witness's Stage-B regional program CONCRETELY in all three
candidate grammars (G1 indented block, G2 BB-with-args/tail-call, G3
S-expression) so the owner can adjudicate on samples. **It does not pick.**
Design only — no production code, no golden change, no edit to any file but
this one.

Witnesses (charge): `join_1` (rendered FULLY in all three grammars —
smallest), `demand_tc_witness` and `demand_multi_adorn_witness`
(representative EXCERPTS in all three).

---

## 0. Framing: the diff is a net-new surface

There is no current `-region-out` dump to diff against — the surface does not
exist at tip. So the house "quote the current lines being replaced, then the
desired lines" idiom is adapted: for each witness I quote the REAL current
lines (`.df`/`.rel`/`.dr`, collected in `phase4/`) that the Stage-B skeleton
DERIVES FROM, then give the desired new dump. Nothing is *replaced*;
everything is *added*, and every added line is a pure function of quoted real
state. The closest existing house precedents the new grammar could imitate:

- `.df` header token `dataflow` + BB-with-args node form (`select ^select.0
  (A:i32, B:i32) ... => ^compare.12 (...)`) — precedent for **G2**.
- `.rel` header token `rel` + flat op list + trailing `census:` line —
  precedent for the census-line placement (all grammars reuse it).
- `.ir` region indentation (`create %table:6[i32]` / nested `%index` /
  `init proc ^init:3()` / `seq`) — precedent for **G1**.
- No DrL dump uses parentheses/S-expressions — **G3** has NO house precedent.

### 0.1 The load-bearing Stage-B determinism CONSTRAINT (all grammars)

The regional dump drains **AFTER `Query::Build`, BEFORE `Program::Build`**
(stage-b-diff.md H6 — the third timing slot). Therefore **no `TableId()`
exists yet** — `%table:N` / `%index:N` / `%col:N` are minted inside
`Program::Build` and are UNAVAILABLE to this dump. Every witness's `.df`
already proves the point: `.df` node ids are `^select.0`, `^tuple.6`,
`%table:12` — but the `%table:12` there is a *DataFlow-attributed* storage id
that only some views carry and that is NOT stable pre-Program. The regional
skeleton must therefore key its contracts on **declared logical identity**
(relation name/arity, declared field names) and on **dense structural ids
minted by the dump itself** (`R0`, `P0`, `E0`), NEVER on `%table:N` and NEVER
on a `QueryView *`/`UniqueId` (the HP-9 rule from regional-arch-pseudocode.md
§2: order re-derived from a DefList view walk, never from opaque handles).
This is a real, cross-grammar constraint and is the first adjudication input
(§5, ADJ-1).

---

## 1. The Stage-B skeleton content (grammar-independent), per witness

At Stage B every program is ONE `ProgramRoot` + ONE observation-root region
`RegionId(0)`, zero child calls (stage-b-diff.md H2). The dump prints the
SKELETON only — root ABIs, the region tree, ports, and row-contracts — and
does NOT re-print the body graph (`.df`/`.rel` already own that text). The
content below is derived line-by-line from the collected real dumps.

### 1.1 `join_1` (no demand)

CURRENT (grounded source, `join_1.dr` + `join_1.rel` census):

    #message t1(i32 A, i32 B).      #message t2(i32 A).
    #local p(i32 A, i32 B).         #local r(i32 A).
    #query q(free i32 B).           #query never(free i32 B).
    ; join_1.rel census: kSubgraphInstantiate=0 ... (no demand machinery)

DESIRED skeleton (abstract, before grammar):

- program-root ABIs (declaration order): input `t1/2(A,B)`, input `t2/1(A)`;
  query `q(free B)`, query `never(free B)`; output `<none>`.
- region `R0` owner=program-root, parents=(), children=().
  - request-port per query: `P0` for `q` **fields=()** (all-free ⇒ empty
    request port ⇒ a permanent-observation-like full scan, NO demand),
    `P1` for `never` **fields=()**.
  - input-port per received message: `P2` = `t1/2(A,B)`, `P3` = `t2/1(A)`.
  - result-port per published message: **none**.
  - row-contracts (relation-decl order): `E0` = `p(A,B)` key=(A,B)
    support=monotone; `E1` = `r(A)` key=(A) support=monotone.
    (The two `#local`s are the region's persistent members; `q`/`never` are
    table-less query projections — no stored contract. Grounded by
    `join_1.rel`: only `kEagerInsert=2` — exactly two stored sinks.)

### 1.2 `demand_tc_witness` (`-demand`, flat)

CURRENT (grounded, `demand_tc_witness.df` :1-10 + `.rel` op.1):

    select ^select.0 (M:u64, T:u64)   ; recv #message edge_2/2
    select ^select.1 (c3:u64)         ; recv #message demand__reachable_from_bf/1
    op.1 kIngestFold ... message=demand__reachable_from_bf/1
    ; #query reachable_from(bound u64 From, free u64 To) : path(From, To).

DESIRED skeleton:

- program-root ABIs: input `edge_2/2(From,To)`; input
  `demand__reachable_from_bf/1(<dcol>)` **[demand-fabricated,
  driver-suppressed]**; query `reachable_from(bound From, free To)`; output
  `<none>`.
- region `R0`: request-port `P0` for `reachable_from` **fields=(From)** (the
  BOUND column — this is where demand shows: `P0` couples to the fabricated
  `demand__…` message); input-port `P1`=`edge_2/2(From,To)`; input-port
  `P2`=`demand__reachable_from_bf/1(<dcol>)`; result-port none;
  row-contracts `E0`=`path(From,To)` key=(From,To) monotone,
  `E1`=`reachable_from(From,To)` key=(From,To) monotone.

The `[demand-fabricated, driver-suppressed]` flag is the Stage-B honesty
concession (ADJ-2): under `-demand` the fabricated message is a real received
`ParsedMessageImpl` in the module (hence it appears), but it is
`IsDemandMessage`-suppressed from the driver ABI. At **Stage C these two
`demand__` lines VANISH** and `P0`'s `fields=(From)` becomes a
`request-edge` construct. `<dcol>` is a metavariable — the fabricated column
name (`c3` in the current `.df`) is compiler-minted and MAY drift; render
shape-exact.

### 1.3 `demand_multi_adorn_witness` (`-demand`, flat; the two-request flagship)

CURRENT (grounded, `demand_multi_adorn_witness.df` :7-13):

    select ^select.1 (c3:u64)   ; recv #message demand__q_bf/1
    select ^select.2 (c4:u64)   ; recv #message demand__q_fb/1
    ; #query q(bound u64 A, free u64 B).   #query q(free u64 A, bound u64 B).

DESIRED skeleton:

- program-root ABIs: query `q_bf(bound A, free B)`, query `q_fb(free A, bound
  B)` (ONE name, TWO adornments — the D3.a.3 shape); input `edge_2/2(A,B)`;
  input `demand__q_bf/1(<dcol>)` **[fabricated,suppressed]**; input
  `demand__q_fb/1(<dcol>)` **[fabricated,suppressed]**; output `<none>`.
- region `R0`: request-port `P0` for `q_bf` **fields=(A)**; request-port `P1`
  for `q_fb` **fields=(B)**; input-ports `P2`=`edge_2/2(A,B)`,
  `P3`=`demand__q_bf/1(<dcol>)`, `P4`=`demand__q_fb/1(<dcol>)`; row-contracts
  `E0`=`rel(A,B)` key=(A,B) monotone, `E1`=`q(A,B)` key=(A,B) monotone (the
  ONE shared pub — grounded by the nested `.rel`: `i#0`/`i#1` share
  `pub=%table:4`). TWO request ports over ONE region is the Stage-B
  foreshadow of Stage C's two `RequestEdgeId`s / two `ChildInstanceId`s.

---

## 2. Grammar G1 — indented region/port/contract block (`.ir`-like)

### 2.1 `join_1` — FULL

    region-program                                    ; header token
    program-root {
      input-abi   t1/2(A:i32, B:i32)          -> R0 via P2
      input-abi   t2/1(A:i32)                 -> R0 via P3
      query-abi   q(B:free i32)               -> R0 via P0
      query-abi   never(B:free i32)           -> R0 via P1
      output-abi  <none>
    }
    region R0  owner=program-root  parents=()  children=() {
      request-port P0  query=q       fields=()
      request-port P1  query=never   fields=()
      input-port   P2  message=t1/2  fields=(A, B)
      input-port   P3  message=t2/1  fields=(A)
      row-contract E0  rel=p  member-key=(A, B)  support=monotone
      row-contract E1  rel=r  member-key=(A)     support=monotone
    }
    census: regions=1 child-calls=0 program-roots=1 request-ports=2 input-ports=2 result-ports=0 row-contracts=2

### 2.2 `demand_tc_witness` — EXCERPT

    region-program
    program-root {
      input-abi   edge_2/2(From:u64, To:u64)                  -> R0 via P1
      input-abi   demand__reachable_from_bf/1(<dcol>:u64)     -> R0 via P2   [fabricated, driver-suppressed]
      query-abi   reachable_from(From:bound u64, To:free u64) -> R0 via P0
      output-abi  <none>
    }
    region R0  owner=program-root  parents=()  children=() {
      request-port P0  query=reachable_from  fields=(From)
      input-port   P1  message=edge_2/2      fields=(From, To)
      input-port   P2  message=demand__reachable_from_bf/1  fields=(<dcol>)
      row-contract E0  rel=path            member-key=(From, To)  support=monotone
      row-contract E1  rel=reachable_from  member-key=(From, To)  support=monotone
    }
    census: regions=1 child-calls=0 program-roots=1 request-ports=1 input-ports=2 result-ports=0 row-contracts=2

### 2.3 `demand_multi_adorn_witness` — EXCERPT (the two-request head)

    program-root {
      input-abi   edge_2/2(A:u64, B:u64)          -> R0 via P2
      input-abi   demand__q_bf/1(<dcol>:u64)      -> R0 via P3   [fabricated, driver-suppressed]
      input-abi   demand__q_fb/1(<dcol>:u64)      -> R0 via P4   [fabricated, driver-suppressed]
      query-abi   q(A:bound u64, B:free u64)      -> R0 via P0
      query-abi   q(A:free u64, B:bound u64)      -> R0 via P1
      output-abi  <none>
    }
    region R0  owner=program-root  parents=()  children=() {
      request-port P0  query=q  adorn=bf  fields=(A)
      request-port P1  query=q  adorn=fb  fields=(B)
      ...
    }
    census: regions=1 child-calls=0 program-roots=1 request-ports=2 input-ports=3 result-ports=0 row-contracts=2

**G1 determinism / id-order:** block nesting fixed (program-root, then
regions in RegionId order; within a region: request-ports, input-ports,
result-ports, row-contracts — each sub-list in declaration order). `RegionId`
dense preorder-of-ownership (only `R0`). `PortId` dense across the concat
(requests in query-decl order, inputs in message-decl order, results in
published-decl order). `EdgeId` dense in relation-decl order. **Census reads:**
one trailing `census:` line (reuses the `.rel` idiom), the six region-skeleton
counts.

**G1 Stage-C extension WITHOUT re-bless of Stage-B pins:** request/lifecycle
constructs are APPENDED sub-blocks inside the region (e.g. a `child-call R1
key=(...) via P0` line and a `request-edge{owner=… call-site=… child=…}`
block after the row-contracts). Because they append, existing skeleton lines
stay byte-identical — **only the `census:` line re-blesses** (child-calls>0,
new op counts). Stage-D nesting appends sibling `region R1 owner=R0 {…}`
blocks; still no edit to R0's existing lines. Verdict: **minimal re-bless
(census line only) at both C and D.**

**G1 permcheck diffability:** Stage B is fully positional — a byte-golden
with an IDENTITY referee (no order-free tokens; §4). Stage C's request-edge
SETS (per-epoch, unordered) are the first place a permcheck-style order-free
multiset earns its keep; in G1 those go in a dedicated `request-edges{…}`
block whose lines a permcheck referee compares order-free while every skeleton
line stays byte-exact.

**G1 precedent:** closest to the existing `.ir` region indentation idiom
(`create %table` blocks, nested `%index`, `init proc`/`seq`) — reviewers
pattern-match instantly. Most human-readable; most verbose.

---

## 3. Grammar G2 — flat BB-with-args / tail-call (`.df`-like)

### 3.1 `join_1` — FULL

    region-program
    root(in P2: t1/2, in P3: t2/1, req P0: q, req P1: never):
      call R0(P2, P3, P0, P1)
    R0[owner=root](in P2(A:i32, B:i32), in P3(A:i32), req P0()=q, req P1()=never):
      contract E0 rel=p key(A, B) support(monotone)
      contract E1 rel=r key(A) support(monotone)
      ; body -> see .df / .rel
    census: regions=1 child-calls=0 program-roots=1 request-ports=2 input-ports=2 result-ports=0 row-contracts=2

### 3.2 `demand_tc_witness` — EXCERPT

    region-program
    root(in P1: edge_2/2, in P2: demand__reachable_from_bf/1 [fab,suppressed], req P0: reachable_from):
      call R0(P1, P2, P0)
    R0[owner=root](in P1(From:u64, To:u64), in P2(<dcol>:u64), req P0(From)=reachable_from):
      contract E0 rel=path key(From, To) support(monotone)
      contract E1 rel=reachable_from key(From, To) support(monotone)
      ; body -> see .df / .rel
    census: regions=1 child-calls=0 program-roots=1 request-ports=1 input-ports=2 result-ports=0 row-contracts=2

### 3.3 `demand_multi_adorn_witness` — EXCERPT

    root(in P2: edge_2/2, in P3: demand__q_bf/1 [fab,suppressed],
         in P4: demand__q_fb/1 [fab,suppressed], req P0: q@bf, req P1: q@fb):
      call R0(P2, P3, P4, P0, P1)
    R0[owner=root](in P2(A:u64, B:u64), in P3(<dcol>:u64), in P4(<dcol>:u64),
                   req P0(A)=q@bf, req P1(B)=q@fb):
      contract E0 rel=rel key(A, B) support(monotone)
      contract E1 rel=q key(A, B) support(monotone)   ; the shared pub
    census: regions=1 child-calls=0 program-roots=1 request-ports=2 input-ports=3 result-ports=0 row-contracts=2

**G2 determinism / id-order:** the block-arg SIGNATURE line encodes the ports
positionally (inputs, then requests, then results — each in decl order), so
port order is pinned by the signature. `call R0(...)` lists the same ports in
the same order (the tail-call to the single child). Ids assigned exactly as
G1. **Census reads:** identical trailing `census:` line.

**G2 Stage-C extension:** request/result edges become EXTRA ARGS on the
signature and the `call` line (`call R1(P5, P6) -> (P7)`), and new regions
become new `Rn[...]:` blocks — this is G2's structural advantage (edges are
literally call args, the most natural encoding of Stage C's request→child
plumbing). **Re-bless cost is higher than G1:** adding a port MUTATES the
existing `root(...)`/`R0[...]` signature line and the `call R0(...)` line →
those lines re-bless at Stage C even for the witness that only GAINS a child.
Verdict: **signature-line re-bless at Stage C** (plus census), but the new
constructs read most naturally.

**G2 permcheck diffability:** same as G1 for the skeleton (positional,
identity referee at Stage B). Stage C edge sets ride extra tail-call lines
`edge R1 owner(...) child(...)` that a permcheck referee compares order-free.

**G2 precedent:** matches the `.df` BB-with-args node form and the MEMORY
"BB-with-args/tail-call form" directive for the DataFlow/DeltaRel dumps — so
`.df`/`.rel`/`.region` share ONE grammar family (uniformity argument). Less
obvious to a first-time reader than G1's labels.

---

## 4. Grammar G3 — typed-record / S-expression, keyed by RegionId

### 4.1 `join_1` — FULL

    (region-program
      (program-root
        (input-abi t1/2 (i32 A) (i32 B) (-> R0 P2))
        (input-abi t2/1 (i32 A) (-> R0 P3))
        (query-abi q (free i32 B) (-> R0 P0))
        (query-abi never (free i32 B) (-> R0 P1))
        (output-abi))
      (region R0 (owner program-root) (parents) (children)
        (request-port P0 (query q) (fields))
        (request-port P1 (query never) (fields))
        (input-port P2 (message t1/2) (fields A B))
        (input-port P3 (message t2/1) (fields A))
        (row-contract E0 (rel p) (member-key A B) (support monotone))
        (row-contract E1 (rel r) (member-key A) (support monotone)))
      (census (regions 1) (child-calls 0) (program-roots 1)
              (request-ports 2) (input-ports 2) (result-ports 0) (row-contracts 2)))

### 4.2 `demand_tc_witness` — EXCERPT

    (region-program
      (program-root
        (input-abi edge_2/2 (u64 From) (u64 To) (-> R0 P1))
        (input-abi demand__reachable_from_bf/1 (u64 <dcol>) (-> R0 P2) (fabricated suppressed))
        (query-abi reachable_from (bound u64 From) (free u64 To) (-> R0 P0))
        (output-abi))
      (region R0 (owner program-root) (parents) (children)
        (request-port P0 (query reachable_from) (fields From))
        (input-port P1 (message edge_2/2) (fields From To))
        (input-port P2 (message demand__reachable_from_bf/1) (fields <dcol>))
        (row-contract E0 (rel path) (member-key From To) (support monotone))
        (row-contract E1 (rel reachable_from) (member-key From To) (support monotone)))
      (census (regions 1) (child-calls 0) (program-roots 1)
              (request-ports 1) (input-ports 2) (result-ports 0) (row-contracts 2)))

### 4.3 `demand_multi_adorn_witness` — EXCERPT

    (program-root
      (input-abi edge_2/2 (u64 A) (u64 B) (-> R0 P2))
      (input-abi demand__q_bf/1 (u64 <dcol>) (-> R0 P3) (fabricated suppressed))
      (input-abi demand__q_fb/1 (u64 <dcol>) (-> R0 P4) (fabricated suppressed))
      (query-abi q (bound u64 A) (free u64 B) (-> R0 P0) (adorn bf))
      (query-abi q (free u64 A) (bound u64 B) (-> R0 P1) (adorn fb)))
    (region R0 (owner program-root) (parents) (children)
      (request-port P0 (query q) (adorn bf) (fields A))
      (request-port P1 (query q) (adorn fb) (fields B))
      (row-contract E0 (rel rel) (member-key A B) (support monotone))
      (row-contract E1 (rel q) (member-key A B) (support monotone)))

**G3 determinism / id-order:** sexp child order is fixed exactly as G1's
sub-list order; ids assigned identically. **Census reads:** a `(census …)`
sexp (not a trailing text line) — the one grammar where the census is itself
a parseable node.

**G3 Stage-C/Stage-D extension:** new `(request-edge …)`, `(child-call …)`,
`(region R1 …)` sexps are ADDED as children — appended, so existing sexps'
TEXT is untouched. Crucially, because a structured referee parses G3, benign
REFORMATTING (indentation, line-wrapping of a grown sexp) does NOT force a
bless: the referee compares the parsed tree, not bytes. So Stage-B pins can
survive Stage-C/D **without ANY re-bless** if the parse-and-compare referee is
adopted — the census node's numbers change but the referee reads them
structurally. Verdict: **zero-re-bless possible** under a structured referee;
under a plain byte-golden it behaves like G1 (append-only, census re-bless).

**G3 permcheck diffability:** best-in-class — a future `regioncheck.py`
(parse-and-compare, the region analog of `permcheck.py`) sidesteps
byte-golden brittleness entirely and makes Stage C's unordered edge SETS a
natural order-free child-multiset comparison. This is the strongest structured-
oracle story and the reason G3 exists as a candidate.

**G3 precedent:** NONE — no DrL dump uses parentheses. Introduces a paren
grammar reviewers must learn; least human-friendly; most machine-friendly.

---

## 5. Determinism contract (normative, all grammars)

1. **Line/child order is a pure function of ParsedModule declaration order**
   plus a fixed kind-priority — NEVER a `QueryView *` / pointer-derived
   `UniqueId` order (HP-9, regional-arch-pseudocode.md §2). Order:
   program-root ABIs (inputs in `#message` decl order, then queries in
   `#query` redeclaration order, then outputs in published-`#message` decl
   order); within each region, request-ports (query-decl order) → input-ports
   (message-decl order) → result-ports (published-decl order) → row-contracts
   (relation-decl order).
2. **Dense structural ids minted by the dump:** `RegionId` in ownership
   preorder (Stage B: only `R0`); `PortId` dense across the
   (request,input,result) concat; `EdgeId` dense in relation-decl order.
   These are line-exact and stable — they do NOT drift between runs or modes.
3. **No `%table:N`/`%index:N`/`%col:N`** — those are `Program::Build`-minted
   and unavailable at this dump's slot (ADJ-1). Contracts key on declared
   relation identity + declared field names only.
4. **Metavariables (shape-exact, may drift):** `<dcol>` — the fabricated
   demand message's compiler-minted column name (`c3`/`c4` today). Everything
   else is **line-exact**: the skeleton is fully positionally pinned, so the
   `-region-out` golden is byte-compared with an **IDENTITY permcheck referee
   at Stage B — no order-free tokens exist, no permutation is permitted.**
   (Contrast the `.rel` policy, where published-delta tokens compare order-free
   per epoch; the Stage-B region skeleton has no such multiset — the first one
   arrives with Stage C's request-edge sets.)
5. **Census line content (identical across grammars, rendering differs):**
   `regions`, `child-calls`, `program-roots`, `request-ports`, `input-ports`,
   `result-ports`, `row-contracts`. For all three witnesses at Stage B:
   `regions=1 child-calls=0 program-roots=1`. Per-witness: `join_1`
   req=2/in=2/out=0/contracts=2; `demand_tc_witness`
   req=1/in=2/out=0/contracts=2; `demand_multi_adorn_witness`
   req=2/in=3/out=0/contracts=2. This census is the Stage-B stub of the
   Stage-C lifecycle census (V-REGION-CENSUS-IDENTITY, stage-b-diff.md H9) —
   at Stage B it must tie `regions==1` ∧ every Rel op ∈ `RegionId(0)`.
6. **Cross-mode invariance:** the skeleton is derived pre-Optimize-independent
   declaration structure, so all 4 optimization modes emit the byte-identical
   `-region-out` (the dump is orthogonal to the opt toggles, like the `.rel`
   `.irgold` pins). The demand flag DOES change it (the `demand__` ABI lines
   appear only under `-demand`) — so demand-witness region goldens live beside
   their `.drflags`, exactly as their `.rel` goldens do.

---

## 6. Comparison table (adjudication summary — NO pick)

| Axis | G1 indented block | G2 BB-with-args/tail-call | G3 S-expression |
| --- | --- | --- | --- |
| House precedent | `.ir` region indentation (closest) | `.df` BB-with-args + MEMORY directive | NONE (new paren grammar) |
| Human readability | highest | medium | lowest |
| Machine parseability | low (byte-golden) | low (byte-golden) | highest (parse-and-compare) |
| Grammar-family uniformity | separate look | unifies `.df`/`.rel`/`.region` | separate look |
| Determinism story | decl-order children; identity referee | decl-order via signature args; identity referee | decl-order sexp children; structured referee |
| Census rendering | trailing `census:` line | trailing `census:` line | `(census …)` node |
| Stage-C edges fit | appended sub-block | extra call args (most natural) | appended sexp |
| Stage-C re-bless of Stage-B pins | census line only | **signature + call lines + census** | **none** (structured referee) or census (byte-golden) |
| Stage-D nesting fit | sibling `region` blocks | new `Rn[...]:` blocks | nested `(region …)` sexps |
| permcheck diffability | byte-golden; order-free block at C | byte-golden; order-free tail-calls at C | native structured/order-free at C |
| Best when | human reviewer blesses goldens | grammar uniformity is priority | structured region oracle anticipated |

---

## 7. Adjudication inputs flagged

- **ADJ-1 (cross-grammar constraint, forces the grammar's identity model):**
  the dump slot (after `Query::Build`, before `Program::Build`) has NO
  `TableId()`. Every candidate MUST key contracts on declared relation
  identity, not `%table:N`. The owner should confirm the row-contract key is
  the declared relation name/arity (my choice) vs a raw `LogicalNodeId`
  (which would need a stable rendering and re-open HP-9).
- **ADJ-2 (the `demand__` ABI-line concession):** at Stage B the flat-demand
  witnesses' skeletons still carry `demand__…/1 [fabricated,
  driver-suppressed]` input-abi lines, because the fabricated message is a
  real received `ParsedMessageImpl` and `-region-out` enumerates received
  messages. This is the visible Stage-B demand scaffolding; **Stage C deletes
  these lines** and turns each request-port's bound `fields=(…)` into a
  `request-edge`. The owner should confirm the fabricated message belongs in
  program-root `input-abi` (my choice, honest to the graph) vs being hidden as
  a region-internal injected input (cleaner, but hides the scaffolding the
  cutover must remove). Either way it is the load-bearing "what evolves at
  Stage C" signal.
- **ADJ-3 (all-free request port = `fields=()`):** `join_1`'s `q`/`never` are
  all-free, so their request ports carry zero fields (permanent-observation-
  like, no demand). The owner should confirm all-free queries render a
  request-port at all (my choice: yes, `fields=()`, to keep query↔port total)
  vs rendering them as `output`/permanent roots with no request port.
- **ADJ-4 (re-bless surface drives the pick):** G2's ports-as-signature-args
  make Stage-C edges most natural but re-bless the signature line at Stage C;
  G1 re-blesses only the census line; G3 can re-bless NOTHING under a
  structured referee. The grammar choice therefore co-decides the Stage-C
  referee strategy (byte-golden vs parse-and-compare) — they cannot be picked
  independently.
- **ADJ-5 (census as line vs node):** G1/G2 render census as a trailing
  `census:` text line (reusing the `.rel` idiom, one existing precedent); G3
  makes it a parseable `(census …)` node. If the Stage-C V-REGION-CENSUS
  validator will parse the dump to recount, G3's node form is directly
  consumable; the text line needs a small parser.
- **Witness-set note:** stage-b-diff.md H-EXIT proposes `demand_tc_witness` +
  `merge_2` + `join_1` as the pin subset. My rendering shows
  `demand_multi_adorn_witness` (two request ports, one shared pub) exercises a
  structural shape neither `demand_tc_witness` (one request port) nor `join_1`
  (zero-field request ports) reaches — the owner may want it IN the pin subset
  as the multi-request carrier, since it is the closest Stage-B foreshadow of
  Stage C's multi-`RequestEdgeId` shape.

---

## 8. Structured summary

- **Surface:** NEW `-region-out` dump, drained after `Query::Build` / before
  `Program::Build` (third timing slot; no `TableId()` available — the
  load-bearing constraint). Renders the Stage-B skeleton: one `ProgramRoot` +
  one observation-root `RegionId(0)`, zero children — root ABIs, region tree,
  ports, row-contracts; the body graph is NOT re-printed (`.df`/`.rel` own it).
- **Desired-state contract (per witness, grammar-independent content):**
  `join_1` = 2 all-free request-ports (`fields=()`), 2 input-ports (t1/t2), 2
  monotone row-contracts (p, r), no output. `demand_tc_witness` = 1
  request-port `fields=(From)`, 2 input-ports (edge_2 + fabricated
  `demand__reachable_from_bf` flagged suppressed), 2 contracts (path,
  reachable_from). `demand_multi_adorn_witness` = 2 request-ports
  (`fields=(A)` bf / `fields=(B)` fb), 3 input-ports (edge_2 + two fabricated
  demand messages), 2 contracts (rel + the ONE shared pub q) — the two-request
  Stage-C foreshadow.
- **Determinism contract:** line/child order is a pure function of
  ParsedModule declaration order + fixed kind-priority (never
  pointer/UniqueId — HP-9); dump-minted dense `R*/P*/E*` ids are line-exact;
  `<dcol>` is the only metavariable (shape-exact); no `%table:N`; a single
  census line/node reads `regions/child-calls/program-roots/request-ports/
  input-ports/result-ports/row-contracts`; cross-mode byte-identical
  (opt-orthogonal), demand-flag-dependent. **permcheck at Stage B = IDENTITY
  referee** (fully positional, zero order-free tokens; the first order-free
  multiset arrives with Stage C's request-edge sets).
- **Three grammars rendered concretely:** G1 (`.ir`-precedent, most readable,
  census-only re-bless at C), G2 (`.df`/MEMORY-precedent, edges-as-call-args,
  signature-line re-bless at C), G3 (no precedent, parse-and-compare, possible
  zero-re-bless at C). Full `join_1` in all three; excerpts for the two demand
  witnesses. Comparison table in §6.
- **Adjudication inputs flagged (§7):** ADJ-1 no-TableId identity model;
  ADJ-2 the `demand__` scaffolding-line concession (what Stage C removes);
  ADJ-3 all-free `fields=()` request ports; ADJ-4 re-bless surface couples the
  grammar pick to the Stage-C referee strategy; ADJ-5 census as line vs node;
  plus the note that `demand_multi_adorn_witness` covers a multi-request shape
  the proposed pin subset otherwise misses.
- **NO pick** (per charge — grammar decision is owner-gated).

---

## 9. STAGE-B REFRESH (2026-08-03, session 4) — ratifications applied, fresh-dump re-grounded

Re-grounding of §§1–8 on FRESH dumps collected at tip `8a4520d9`
(`phase-d/<case>.{df,contract,rel,dot}` for `demand_tc_witness`, `join_1`,
`merge_2` [NEW — unrendered in session 1], `demand_multi_adorn_witness`),
with the two D2.8 ratifications the session-1 renderings pre-dated APPLIED:

- **ADJ-2** (`owner-adjudication-record.md:72–74`, D2.8 RATIFIED): the fabricated
  `demand__…` input renders **region-internal**, NOT as a program-root
  `input-abi`/input-port. Session-1 §§1.2/1.3/2.2/2.3 rendered it as a program-root
  input-abi flagged `[fabricated, driver-suppressed]`; this addendum moves it inside
  the region as a portless `region-internal` line (dropping the program-root
  input-port count), following `region-model-desired-states.md:215`'s line shape.
- **ADJ-3** (`owner-adjudication-record.md:73–74`, D2.8 RATIFIED): an all-free query
  renders as an **output/permanent-root with NO request port** (request-port count =
  "has demand"). Session-1 §1.1 rendered `join_1`'s all-free `q`/`never` WITH
  `fields=()` request ports; this addendum drops those ports and renders each as a
  portless `permanent-root` line, following `region-model-desired-states.md:257`.

The grammar is D2.2-ratified **G1** (`owner-adjudication-record.md:48`). This
supersedes §§1–2 for these four witnesses by dated addendum (house rule; earlier
sections untouched).

### 9.0 The STORED-RELATION (row-contract) rule — pure graph function, verified

Session-1 §1.1 asserted `join_1`'s row-contracts are the two `#local`s `p`/`r` and
that `q`/`never` are "table-less query projections — no stored contract, grounded by
`kEagerInsert=2`." **The fresh dumps FALSIFY the identity.** `join_1.rel`'s
`kEagerInsert=2` sinks are `op.14 … sink=relation args: table=%table:6` and
`op.17 … table=%table:9`; `join_1.dot:46` labels `TABLE 6 … MATERIALIZE q` and
`:48` `TABLE 9 … MATERIALIZE never`; `join_1.df` closes with
`insert ^insert.18 (B:i32) into %table:6` / `insert ^insert.19 (B:i32) into
%table:9`, both `class=monotone`. The two stored sinks are **`q` and `never`**, not
`p`/`r` — and their schema is `(B:i32)`, which cannot be `p(A,B)`. `p`/`r` survive
only as constant-specialized JOIN-OPERAND arrangement tables (`%table:12/16/19/23`,
each a `tuple`/`compare` output feeding `=> ^join.N .inN`), which are NOT relation
materializations. This is the same shape the task flags for `merge_2`: the `@inline`
locals `inner`/`proj` (and the plain `#local outer`) all collapse — `merge_2.df` has
`class=table-less` on EVERY tuple and only two `table=` sinks,
`insert ^insert.12 (X,Y) into %table:4` / `insert ^insert.13 (X) into %table:8`,
which `merge_2.dot:17,20` label `MATERIALIZE q_outer` / `MATERIALIZE q_proj`.

**Rule R-STORE (Stage-B row-contracts, pure graph function) — AS AMENDED by the
session-4 determinism critique (F1/F2/F3, §9.6).** Work over MODELS (eqsets /
`%table:N`), NOT per-view printed attributes. A model *M* is a MATERIALIZED
RELATION iff *M*'s eqset contains an INSERT sink (`insert … into %table:N`) OR a
MERGE (union) view — **regardless of which member view of *M* prints the `table=`
attribute** (a `class=table-less` merge whose model table is printed on a sibling
member view, e.g. `demand_multi_adorn`'s `merge ^merge.16` sharing eqset 5 with
`tuple ^tuple.4 table=%table:15`, still counts) **and regardless of whether *M*
also feeds join operands** (the same eqset can be both a stored union and a join
input). A model whose eqset contains NEITHER an insert-sink NOR a merge — only
`compare`/`tuple`/join-operand/index arrangements (`=> ^join .inN`), or is
`@inline`-flattened — is NOT a materialized relation ⇒ no contract. EXCLUDE
fabricated demand objects on BOTH sides of the seam: neither the fabricated
`demand__…` MESSAGE (ADJ-2, region-internal) NOR the fabricated `demand__…`
demand-RELATION union (e.g. `demand_tc`'s `merge ^merge.18 … table=%table:12`,
`class=monotone`) is a contract — both are region-internal machinery. Emit one
`row-contract E<k>` per surviving materialized model, naming it by the
SOURCE-declared relation it backs; when several source-declared relations share
one model (e.g. `merge_2`'s `%table:4` backs both `#local outer` and `#query
q_outer`), name it by the OBSERVED/pub relation — the name on the model's
INSERT/pub sink and the `.dot` MATERIALIZE label (`q_outer`, not `outer`). `k` is
dense in the declaration order of that naming relation. `member-key` = AllFields
on a cycle (D1.2, `owner-adjudication-record.md:16–19`), else the declared
visible-field list. `support` = `differential` iff any feeding message is
`@differential`, else `monotone`.

**R-STORE verified against every witness's `.df` `table=`/`into` lines:**

| witness | materializing tables (fresh `.df`) | → stored relations (decl order) | member-keys / support | count |
| --- | --- | --- | --- | --- |
| `join_1` | `into %table:6`, `into %table:9` (both `insert`, `monotone`) | `q`, `never` | `(B)`/mono, `(B)`/mono | 2 |
| `merge_2` | `into %table:4`, `into %table:8` (both `insert`, `monotone`; all `#local`s `table-less`) | `q_outer`, `q_proj` | `(X,Y)`/mono, `(X)`/mono | 2 |
| `demand_tc_witness` | `merge ^merge.17 … table=%table:8` (path UNION), `into %table:4` (`insert`) (the fabricated demand-relation union `merge ^merge.18 table=%table:12` is EXCLUDED — region-internal, F2) | `path`, `reachable_from` | `(From,To)` AllFields (cycle, D1.2; `; cycle` markers, `.contract`: `tuple ^tuple.4 role=member key=(From,To)`), `(From,To)` — both mono | 2 |
| `demand_multi_adorn_witness` | `merge ^merge.16` → `tuple.4 table=%table:15` (rel R-DUP UNION), `into %table:4` (`insert`) | `rel`, `q` (the ONE shared pub) | `(A,B)`/mono, `(A,B)`/mono | 2 |

All four = **2 row-contracts** — but `join_1`'s IDENTITY is corrected (`q`,`never`,
not `p`,`r`) and `merge_2` is grounded from scratch. Note that `q`/`never`/
`q_outer`/`q_proj` are simultaneously ADJ-3 permanent-roots (their all-free
observation handle) AND row-contracts (their own materialized table) — the two
facets are orthogonal; session-1's "queries = no stored contract" is a
**[fresh-dump correction]** wherever the query is a derived join/filter (own table)
rather than a bare projection of one stored relation (`region-model` §2.b's
`reachable`, genuinely table-less).

### 9.1 The four G1 `-region-out` blocks (baseline — NO `key-invariant`, NO `request-edges`)

Stage-B scope discipline (load-bearing): **NO `request-edges{…}` sub-block** (Stage
C, §2 extension contract), **NO 8th census field** (`request-edges` is H7-open —
`region-model` §5 H7; the census stays the SEVEN §5.5 fields), **NO `key-invariant=`
region-header token** (H8-open; §9.3 proposes it separately), and **NO key-in-output
vs sequestered convention** anywhere (H1-open, `region-model` §5 H1). Portless
annotation lines (`region-internal`, `permanent-root`) carry NO `PortId`; `PortId` is
dense across (request-ports, input-ports, result-ports) only.

#### 9.1.a `join_1` (no demand — the all-free / ADJ-3 flagship)

    region-program
    program-root {
      input-abi   t1/2(A:i32, B:i32)   -> R0 via P0
      input-abi   t2/1(A:i32)          -> R0 via P1
      query-abi   q(B:free i32)        -> permanent-root      ; ADJ-3: all-free, NO request port
      query-abi   never(B:free i32)    -> permanent-root      ; ADJ-3
      output-abi  <none>
    }
    region R0  owner=program-root  parents=()  children=() {
      input-port     P0  message=t1/2  fields=(A, B)
      input-port     P1  message=t2/1  fields=(A)
      permanent-root q(B)                                     ; ADJ-3 (materialized ⇒ also E0)
      permanent-root never(B)                                 ; ADJ-3 (materialized ⇒ also E1)
      row-contract   E0  rel=q      member-key=(B)  support=monotone
      row-contract   E1  rel=never  member-key=(B)  support=monotone
    }
    census: regions=1 child-calls=0 program-roots=1 request-ports=0 input-ports=2 result-ports=0 row-contracts=2

Deltas from session-1 §2.1:
- request-ports **2 → 0**; the two `fields=()` request ports become portless
  `permanent-root q(B)`/`never(B)` lines; the query-abis route `-> permanent-root`.
  **[reconciliation — ADJ-3]**
- PortId reindex: with request-ports=0, input-ports start at **P0/P1** (were P2/P3).
  **[reconciliation — ADJ-3]** (pure consequence of the dense request→input concat.)
- row-contract IDENTITY **`p`/`r` → `q`/`never`**, keys `(A,B)`/`(A)` → `(B)`/`(B)`;
  `p`/`r` are join-operand arrangements (`%table:12/16/19/23`), never materialized;
  the `kEagerInsert=2` sinks are `%table:6=q` / `%table:9=never` (§9.0). **[fresh-dump
  correction]**
- `q`/`never` now ALSO appear as row-contracts (session-1: "table-less, no stored
  contract"). **[fresh-dump correction]**
- census `request-ports 2 → 0`. **[reconciliation — ADJ-3]**

#### 9.1.b `merge_2` (no demand — NEW; the `@inline`-collapse witness)

    region-program
    program-root {
      input-abi   m1/2(X:i32, Y:i32)   -> R0 via P0
      input-abi   m2/2(X:i32, Y:i32)   -> R0 via P1
      input-abi   m3/2(X:i32, Y:i32)   -> R0 via P2
      query-abi   q_outer(X:free i32, Y:free i32)  -> permanent-root   ; ADJ-3
      query-abi   q_proj(X:free i32)               -> permanent-root   ; ADJ-3
      output-abi  <none>
    }
    region R0  owner=program-root  parents=()  children=() {
      input-port     P0  message=m1/2  fields=(X, Y)
      input-port     P1  message=m2/2  fields=(X, Y)
      input-port     P2  message=m3/2  fields=(X, Y)
      permanent-root q_outer(X, Y)                            ; ADJ-3 (materialized ⇒ E0)
      permanent-root q_proj(X)                                ; ADJ-3 (materialized ⇒ E1)
      row-contract   E0  rel=q_outer  member-key=(X, Y)  support=monotone
      row-contract   E1  rel=q_proj   member-key=(X)     support=monotone
    }
    census: regions=1 child-calls=0 program-roots=1 request-ports=0 input-ports=3 result-ports=0 row-contracts=2

Deltas from session-1: NONE (not rendered in session 1). Grounding:
- 3 received `#message`s `m1/m2/m3` (decl order) → `input-ports=3`,
  P0/P1/P2. `merge_2.df` selects `; recv #message m3/2` (`^select.0`), `m1/2`
  (`^select.1`), `m2/2` (`^select.2`) — but ABI/port order follows `#message`
  DECLARATION order (`m1,m2,m3`), NOT the `.df` select order (determinism §5.1).
  **[grounded]**
- `#local inner @inline`, `#local outer`, `#local proj @inline` ALL collapse: every
  `merge_2.df` tuple is `class=table-less`, the only `table=` sinks are
  `into %table:4` / `into %table:8` (§9.0). Stored relations = the two queries
  `q_outer`/`q_proj` (`merge_2.dot:17,20` MATERIALIZE labels) → `row-contracts=2`.
  **[fresh-dump — the `@inline` collapse; stored sinks are the query tables]**
- both queries all-free (`free X, free Y` / `free X`) → `permanent-root`,
  `request-ports=0` (ADJ-3). **[reconciliation — ADJ-3]**

#### 9.1.c `demand_tc_witness` (`-demand`, one demand query)

    region-program
    program-root {
      input-abi   edge_2/2(From:u64, To:u64)                  -> R0 via P1
      query-abi   reachable_from(From:bound u64, To:free u64) -> R0 via P0
      output-abi  <none>
    }
    region R0  owner=program-root  parents=()  children=() {
      request-port    P0  query=reachable_from  fields=(From)
      input-port      P1  message=edge_2/2      fields=(From, To)
      region-internal demand__reachable_from_bf/1(<dcol>:u64)  [fabricated, driver-suppressed]   ; ADJ-2
      row-contract    E0  rel=path            member-key=(From, To)  support=monotone
      row-contract    E1  rel=reachable_from  member-key=(From, To)  support=monotone
    }
    census: regions=1 child-calls=0 program-roots=1 request-ports=1 input-ports=1 result-ports=0 row-contracts=2

Deltas from session-1 §2.2:
- input-ports **2 → 1**: the `demand__reachable_from_bf/1` message moves to a portless
  `region-internal` line (`region-model` §2.a shape); it is no longer a program-root
  input-port. `demand_tc_witness.df` still receives it (`select ^select.1 (c3:u64)
  ; recv #message demand__reachable_from_bf/1`) but ADJ-2 hides it region-internal.
  **[reconciliation — ADJ-2]**
- PortId: request-port `P0` (kept — `reachable_from` is a BOUND-column demand query),
  input-port `P1=edge_2/2`. **[grounded]**
- row-contracts unchanged: `path` (materialized by `merge ^merge.17 … table=%table:8`,
  a UNION production; on the cycle → AllFields `(From,To)`, D1.2, confirmed by
  `.contract` `tuple ^tuple.4 role=member key=(From,To)` and the `.df` `; cycle`
  markers) and `reachable_from` (`insert ^insert.19 … into %table:4`). Both mono
  (no `@differential`). `merge ^merge.18 … table=%table:12` (the fabricated demand
  relation `d_path`) is region-internal machinery, not a contract (R-STORE demand
  carve-out, F2). **[grounded]**
- `<dcol>` = the fabricated column name (`c3` in `.df`; the `.dot` renders the SAME
  column `_MissingVar`) — the sole metavariable, shape-exact, may drift across
  surfaces. **[grounded — metavariable]**

#### 9.1.d `demand_multi_adorn_witness` (`-demand`, two demand adornments, one shared pub)

    region-program
    program-root {
      input-abi   edge_2/2(A:u64, B:u64)                -> R0 via P2
      query-abi   q(A:bound u64, B:free u64)  adorn=bf  -> R0 via P0
      query-abi   q(A:free u64, B:bound u64)  adorn=fb  -> R0 via P1
      output-abi  <none>
    }
    region R0  owner=program-root  parents=()  children=() {
      request-port    P0  query=q  adorn=bf  fields=(A)
      request-port    P1  query=q  adorn=fb  fields=(B)
      input-port      P2  message=edge_2/2   fields=(A, B)
      region-internal demand__q_bf/1(<dcol>:u64)  [fabricated, driver-suppressed]   ; ADJ-2
      region-internal demand__q_fb/1(<dcol>:u64)  [fabricated, driver-suppressed]   ; ADJ-2
      row-contract    E0  rel=rel  member-key=(A, B)  support=monotone
      row-contract    E1  rel=q    member-key=(A, B)  support=monotone
    }
    census: regions=1 child-calls=0 program-roots=1 request-ports=2 input-ports=1 result-ports=0 row-contracts=2

Deltas from session-1 §2.3:
- input-ports **3 → 1**: BOTH `demand__q_bf/1` and `demand__q_fb/1` move to portless
  `region-internal` lines (fabrication order bf-then-fb = `.df` `^select.1` before
  `^select.2`); only the real `edge_2/2` is a program-root input-port (`P2`).
  **[reconciliation — ADJ-2]**
- request-ports stays **2** (both `bf` and `fb` carry a BOUND column ⇒ both are demand
  queries; ADJ-3 does not fire). **[grounded]**
- row-contracts unchanged: `rel` (materialized by the R-DUP `merge ^merge.16` →
  `tuple.4 table=%table:15`, the reference-counted union of the two guards' outputs)
  and `q` (the ONE shared pub, `insert ^insert.18 … into %table:4`; `.dot:5` shows a
  SINGLE `RELATION q` node → `TABLE 4 MATERIALIZE q`). Both mono. **[grounded]**

### 9.2 Determinism-contract delta on §5 (what ADJ-2/ADJ-3/R-STORE change)

§5 stays normative; the additions/changes below apply. All lists remain a pure
function of `ParsedModule` declaration order + fixed kind-priority — never a
`QueryView *`/`UniqueId`/hash order (HP-9). Metavariable `<dcol>` unchanged (§5.4).

- **Within-region block order (was §5.1: request-ports → input-ports → result-ports →
  row-contracts) gains two portless tiers:**
  `request-ports → input-ports → result-ports → region-internal → permanent-root →
  row-contracts`. **Sort keys:**
  - request-ports: `#query` redeclaration order, **DEMAND queries only** (all-free
    excluded by ADJ-3). For an all-free-only program (`join_1`, `merge_2`) this list
    is EMPTY and the ordering rule is **VACUOUS**. **[ADJ-3]**
  - region-internal (fabricated `demand__…` messages): fabrication order =
    **ascending `forcing_index`** (equivalently `.df` `demand__` select-node decl
    order — `demand_multi_adorn`'s `bf` `^select.1` before `fb` `^select.2`); portless.
    **[ADJ-2]**
  - permanent-root (all-free queries): **`#query` declaration order**; portless.
    **[ADJ-3]**
  - row-contracts: **declaration order of the relation each materializing table backs**
    (R-STORE, §9.0) — keyed on the MODEL (eqset), not per-view `table=`; a model
    shared by several declared relations is named by its observed/pub relation
    (`q_outer`, not `outer` — F3) — a materialized relation may be a `#local`
    (`path`, `rel`) OR a `#query` (`q`, `never`, `q_outer`, `q_proj`,
    `reachable_from`); join-operand / `@inline`-fused / input-message arrangements
    are excluded. **[fresh-dump — R-STORE supersedes session-1's "#locals only"
    heuristic]**
- **Program-root ABI order (§5.1) unchanged** — inputs (`#message` decl order, REAL
  messages only; fabricated `demand__…` OMITTED, ADJ-2) → queries (`#query` redecl
  order; all-free ones route `-> permanent-root`, demand ones `-> R0 via P<req>`,
  ADJ-3/ADJ-2) → outputs (published-`#message` decl order; `<none>` for all four).
- **PortId (§5.2) unchanged in scheme, changed in yield:** dense across
  (request-ports, input-ports, result-ports); `region-internal`/`permanent-root` are
  PORTLESS, so removing all-free request ports (ADJ-3) and demand input ports (ADJ-2)
  **renumbers** the surviving ports (e.g. `join_1` inputs slide P2/P3 → P0/P1). The
  renumber is itself a pure graph function of the two ratifications — line-exact.
- **Census (§5.5) stays SEVEN fields** — `regions, child-calls, program-roots,
  request-ports, input-ports, result-ports, row-contracts`. **NO `request-edges`
  field at Stage B** (H7-open). request-ports now counts DEMAND queries only (ADJ-3);
  input-ports counts REAL messages only (ADJ-2). All four: `regions=1 child-calls=0
  program-roots=1`, `result-ports=0`, `row-contracts=2`.
- **permcheck (§5.4) still the IDENTITY referee** — the Stage-B skeleton is fully
  positional; every list above has a stated deterministic sort key, so zero order-free
  tokens exist and no permutation is permitted. The first order-free multiset arrives
  only with Stage C's `request-edges` (deferred; H7).

### 9.3 PROPOSED (H8, FLAGGED — not in the baseline blocks): `key-invariant=` header token

`region-model` §5 H8 leaves the `key-invariant=` region-header token an OPEN item
(region-model surface not frozen). The §9.1 baseline blocks OMIT it. IF the owner
wants it at Stage B, the one-line delta is a token on the `region R0 …` header:

    region R0  owner=program-root  parents=()  children=()  key-invariant=(From) {   ; demand_tc_witness
    region R0  owner=program-root  parents=()  children=()  key-invariant=()     {   ; join_1 / merge_2 (no demand)

**Deterministic derivation (proposed):** `key-invariant(region)` = the region's sole
demand forcing's BOUND columns, ordered by the query's declared column order
restricted to bound positions; across forcings, ascending `forcing_index`. Yields:
`join_1`/`merge_2` → `()` (no demand ⇒ zero-replication, record §205); `demand_tc`
→ `(From)` (the one `reachable_from_bf` forcing's bound col). **`demand_multi_adorn`
is the flagged obstruction:** it has TWO forcings (`bf` bound=A / `fb` bound=B) sharing
ONE region ⇒ the single-header token is **ill-defined at Stage B** — it would need a
per-forcing spelling (`key-invariant={bf:(A), fb:(B)}`) or, more honestly, defer to
Stage C's per-request-edge annotation. **[INVENTED token — `key-invariant=`; FLAGGED
H8-open; multi-adornment single-header form UNDEFINED at Stage B]**

### 9.4 DOT twins (advisory — NEVER goldened; `owner-adjudication-record.md:133–145`)

`cluster_region_0` wraps the region; the G1 text dump remains the referee (DOT is
advisory, node `vNNNN` ids are pointer-derived and DRIFT per run — never a determinism
target). **Stage-B has NO request edges** (the dashed labeled inter-cluster arrow of
`region-model` §3.a is a Stage-C addition). At Stage B **the query observation node
connects to its materializing table via the ORDINARY SOLID dataflow edge already in
the fresh `.dot`** — grounded per witness below. Only `demand_tc_witness` has an inner
`subgraph cluster_stratum_5` (`.dot:4`, the 11-node recursive SCC); the other three
have NO stratum cluster (Stage-A DOT clusters MULTI-view strata only — their strata are
single-view), so `cluster_region_0` wraps ALL interior `vNNNN` nodes directly.

#### 9.4.a `demand_tc_witness` — region cluster wraps the stratum cluster

    digraph {
    bgcolor="#f0f4f7";
    node [shape=none margin=0 nojustify=false labeljust=l font=courier];
    compound=true;
    subgraph cluster_region_0 {
      label="region R0";                            ; baseline: NO key-invariant (H8, §9.3)
      style="rounded,bold"; color="#3a6ea5";
      subgraph cluster_stratum_5 {                  ; the fresh inner SCC box, verbatim (.dot:4-16)
        label="stratum 5"; style="rounded,dashed";
        v4325188224; v4325197904; v4325199184; v31386009600; v31386010112;
        v31386012672; v31386013184; v31386014208; v31386013696; v4325183200; v4325197072;
      }
      ; + the region's acyclic tissue (edge_2 RECEIVE, path TABLE 8, reachable_from TABLE 4)
      ; + the region-internal demand__reachable_from_bf RECEIVE (ADJ-2, rendered inside)
    }
    t4325192864 [ ... RELATION reachable_from ... ];   ; the demand query's observation node (OUTSIDE)
    ; Stage-B: SOLID ordinary edge, NOT a dashed request edge (that is Stage C):
    t4325192864 -> v4325194992 [style=solid];          ; fresh .dot: RELATION reachable_from -> MATERIALIZE reachable_from (TABLE 4)
    }

The Stage-C dashed `q_reachable_from -> … [label="reachable_from[From](To) kind=LAZY",
lhead=cluster_region_0, style=dashed]` of `region-model` §3.a is ABSENT here — Stage B
shows only the solid `t4325192864 -> v4325194992` already in the fresh dump.
**[shape — node ids drift; grounded — the solid observation edge is the fresh
`.dot`; NO request edge at Stage B]**

#### 9.4.b `join_1` / `merge_2` / `demand_multi_adorn_witness` — region cluster, NO inner stratum box

None of these three fresh `.dot`s contains a `subgraph cluster_stratum_*` (single-view
strata). `cluster_region_0` therefore wraps ALL interior `vNNNN` nodes directly:

    subgraph cluster_region_0 {
      label="region R0";                     ; baseline: NO key-invariant (H8)
      style="rounded,bold"; color="#3a6ea5";
      ; ALL interior v-nodes: the RECEIVEs, the COMPARE/JOIN/UNION chain, and the
      ; MATERIALIZE tables — join_1: v4394551008 (TABLE 6 q) + v52026409472 (TABLE 9 never);
      ; merge_2: v34573910016 (TABLE 4 q_outer) + v34573910528 (TABLE 8 q_proj);
      ; demand_multi_adorn: v4348719488 (TABLE 4 q, the shared pub) + the two UNION nodes.
      ; demand_multi_adorn ALSO renders the two demand__ RECEIVEs region-internal (ADJ-2).
    }
    ; program-root observation nodes OUTSIDE the cluster, connected by SOLID edges
    ; (fresh .dot, NO request/dashed edge at Stage B):
    ;   join_1:             t52026311808 -> v4394551008  (RELATION q -> MATERIALIZE q)
    ;                       t52026312000 -> v52026409472 (RELATION never -> MATERIALIZE never)
    ;   merge_2:            t4328361312  -> v34573910016 (q_outer)
    ;                       t4328363168  -> v34573910528 (q_proj)
    ;   demand_multi_adorn: t4348714320  -> v4348719488  (single RELATION q -> shared pub TABLE 4)

For `join_1`/`merge_2` the observation nodes are ADJ-3 permanent-roots reaching in with
an ordinary (solid, unlabeled) edge — the visual of a no-demand program (`region-model`
§3.b). For `demand_multi_adorn` the single shared-`q` node still connects solid at
Stage B; its two Stage-C request edges (one per adornment) are absent. **[advisory;
shape — node ids drift; grounded — every edge cited from the fresh `.dot`]**

### 9.5 Census table — all four witnesses (SEVEN fields; H7-open, NO `request-edges`)

| witness | regions | child-calls | program-roots | request-ports | input-ports | result-ports | row-contracts |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `join_1` | 1 | 0 | 1 | **0** (ADJ-3) | **2** | 0 | 2 (`q`,`never`) |
| `merge_2` | 1 | 0 | 1 | **0** (ADJ-3) | **3** | 0 | 2 (`q_outer`,`q_proj`) |
| `demand_tc_witness` | 1 | 0 | 1 | 1 | **1** (ADJ-2) | 0 | 2 (`path`,`reachable_from`) |
| `demand_multi_adorn_witness` | 1 | 0 | 1 | 2 | **1** (ADJ-2) | 0 | 2 (`rel`,`q`) |

Bold cells are the ADJ-2/ADJ-3 deltas from the session-1 counts (`join_1`
req 2→0; `merge_2` NEW; `demand_tc` in 2→1; `demand_multi_adorn` in 3→1). Every
`regions==1 ∧ child-calls==0` ties the Stage-B `V-REGION-CENSUS-IDENTITY` stub
(§5.5: `regions==1 ∧ every Rel op ∈ RegionId(0)`).

### 9.6 DETERMINISM CRITIQUE (2026-08-03, session 4) — findings + dispositions

Lens: is every predicted line a pure, order-stable function of the graph; do the
stated rules mechanically reproduce the tabulated counts; ratification fidelity
(ADJ-2/ADJ-3 applied, H1/H2/H7/H8 not prejudged); cross-surface consistency with
the fresh `.dot`. Calibration: DELTA-6 (refute-by-default, concrete failure
scenario required). THREE findings survived self-refutation, all with one root
cause — R-STORE was first specified over per-view PRINTED attributes (`table=` on
the terminal node) instead of over the MODEL (eqset); the amended rule in §9.0 is
the fix and reproduces `2/2/2/2` unchanged.

**F1 [MAJOR — APPLIED, §9.0].** Literal pre-amendment R-STORE undercounted
`demand_multi_adorn` to `row-contracts=1`: `rel`'s union `merge ^merge.16` is
`class=table-less` (rule (b) did not fire) and its model `%table:15` prints on
`tuple ^tuple.4`, which feeds `=> ^join.13/.15 .in1` — exactly the shape the
exclusion clause named. `tuple.4` was simultaneously include-able and
exclude-able: nondeterministic classification. Fix: the rule now keys on the
MODEL's eqset (contains INSERT sink OR MERGE view), regardless of which member
prints `table=` and regardless of join-operand fan-out.

**F2 [MINOR — APPLIED, §9.0 + §9.0 table + §9.1.c].** The carve-out excluded only
the fabricated demand MESSAGE, not the fabricated demand-RELATION union —
`demand_tc`'s `merge ^merge.18 … table=%table:12` (`class=monotone`, the `d_path`
union) satisfies rule (b), so a literal reading emitted `row-contracts=3`. Fix:
both sides of the fabricated seam are region-internal machinery, never a
contract.

**F3 [MINOR — APPLIED, §9.0 + §9.2].** "The relation each materializing table
backs" had no tiebreak under model sharing: `merge_2`'s `%table:4` backs both
`#local outer` (table-less merge, eqset 6) and `#query q_outer` (`insert.12`,
same eqset) — an implementer could render `rel=outer`. Fix: a shared model is
named by its OBSERVED/pub relation (the INSERT/pub sink name, matching the
`.dot` `MATERIALIZE` label).

**Verified clean (dropped after self-refutation):** all four census lines
recounted field-by-field; ADJ-2/ADJ-3 fidelity; no prejudging of H1
(key-in-output), H2 (`kRequestEdgeAdd`), H7 (census `request-edges`), H8
(`key-invariant=` kept out of the baseline, §9.3); every §9.4 DOT edge verbatim
in the fresh `.dot` (only `demand_tc` has a stratum cluster); member-keys match
the `.contract` member views; the `join_1` identity correction (`q`/`never`, not
session-1's `p`/`r`) is itself CORRECT — only the rule statement was defective.

### 9.7 IMPLEMENTATION RECONCILIATION (2026-08-03, session 4 — Stage B LANDED)

The `-region-out` surface landed (lib/Regional; owner ratified ESC-4 variant
(iii)). Predict-then-verify adjudications, each resolved TOWARD DERIVABILITY
(the Stage-A adjudication-#4 pattern — produced dumps win when the prediction
assumed facts the graph does not carry):

- **R-STORE NARROWED to insert-materialized relations (supersedes the §9.0
  merge arm).** `rel=path` / `rel=rel` are NOT pure graph functions in ANY
  mode: `QueryMergeImpl` carries no declaration link, `ConnectInsertsToSelects`
  removes relation INSERTs in every mode (verified: demand_tc nodf ==
  opt byte-identical), and the only relation-named materializations are
  relation-INSERT sinks. Contracts therefore render ONLY for distinct
  non-`demand__` relation-INSERT declarations (first-insert-wins key render,
  positional by declared params). demand_tc_witness and
  demand_multi_adorn_witness census `row-contracts` are **1** (not §9.5's 2);
  join_1/merge_2 stay 2. The unnameable interior merge model (`path`) is the
  concrete NECESSITY WITNESS for the reserved logical-origin-provenance
  direction (owner-adjudication-record §table-provenance) — when that lands,
  the interior contracts become renderable and this narrowing lifts.
- **`<dcol>` realized as `p0`** — the fabricated message's PARSED param
  spelling (deterministic), not the `.df` column name `c3`. Metavariable
  contract honored (shape-exact).
- **Alignment realized as the uniform max+2 rule** (kind token, decl text,
  and per-group value fields each left-justified to group-max + 2); §9.1's
  hand-eyeballed padding yields to the produced bytes.
- **Zero-arity condition unit relations** (desugared `is_condition`
  relations): the contract renders `member-key=()` (the insert's token
  column has no declared param position). A pre-existing parser quirk is
  RECORD-ONLY: an implicitly-declared zero-arity clause-head export is minted
  without a name spelling, so its contract renders `rel=` empty (e.g.
  `booleans`) — visible only in this new dump, no pinned witness affected.
- **Cross-mode**: all four pins produced byte-identical `.region` across the
  4 modes at tip — but the pins remain PER-MODE (16 goldens), per
  F-REGION-CROSSMODE; cross-mode identity stays unclaimed.

The 16 `.region` goldens were blessed ONCE from a reviewed subset run via
`runall.sh --bless`. The V-REGION-CENSUS recount (stored vs
`DeriveRegionalCensus(query)`) is live corpus-wide, always-on.

## 10. TIER-1 DESIRED STATES (2026-08-03, session 5) — the naming-lift golden diffs, byte-exact, determinism-critiqued

Authority chain: region-model-diffs.md "DIFF-R3 AMENDMENTS (session 5)"
HUNK T1 as corrected by ADJUDICATED RESOLUTIONS RES-2/RES-4; emitter =
lib/Regional/Format.cpp:120-151 (column widths are a MAX over ALL contracts
— rel_w = max(4+len(name))+2, key_w = max(11+len(key))+2, etok_w =
max(1+len(idx))+2; kind_w is the shared port-kind width, 17 at both
witnesses). Predictions verified against the emitter arithmetic AND fresh
4-mode dumps of the CURRENT compiler (all 8 byte-match the blessed goldens
pre-implementation; -contract-out confirms the interior merge's Stage-A
member key is AllFields in every mode for both witnesses — ORC-3).

### 10.1 demand_tc_witness.region.<mode> (x4 — one diff, all four modes identical)

rel_w stays 20 (`reachable_from` dominates), key_w stays 23, etok_w stays 4:
the EXISTING E0 line is byte-UNCHANGED. Diff:

```diff
   region-internal  demand__reachable_from_bf/1(p0:u64)  [fabricated, driver-suppressed]
   row-contract     E0  rel=reachable_from  member-key=(From, To)  support=monotone
+  row-contract     E1  rel=path            member-key=(From, To)  support=monotone
 }
-census: regions=1 child-calls=0 program-roots=1 request-ports=1 input-ports=1 result-ports=0 row-contracts=1
+census: regions=1 child-calls=0 program-roots=1 request-ports=1 input-ports=1 result-ports=0 row-contracts=2
```

(`rel=path` is 8 chars, padded to rel_w=20 → 12 trailing spaces — the
t16/RES-4 correction.)

### 10.2 demand_multi_adorn_witness.region.<mode> (x4 — one diff, all four modes identical)

`rel` (len 3) beside `q` (len 1) widens rel_w 7→9: the EXISTING E0 line
RE-PADS (+2 spaces after `rel=q`) — the t15/ORC-1 whole-block-padding
consequence. key_w stays 19. Two forcings demand the one `rel` decl →
ONE interior contract (RES-2 decl-Id dedup). Diff:

```diff
   region-internal  demand__q_fb/1(p0:u64)  [fabricated, driver-suppressed]
-  row-contract     E0  rel=q  member-key=(A, B)  support=monotone
+  row-contract     E0  rel=q    member-key=(A, B)  support=monotone
+  row-contract     E1  rel=rel  member-key=(A, B)  support=monotone
 }
-census: regions=1 child-calls=0 program-roots=1 request-ports=2 input-ports=1 result-ports=0 row-contracts=1
+census: regions=1 child-calls=0 program-roots=1 request-ports=2 input-ports=1 result-ports=0 row-contracts=2
```

(Emitter-test hazard from Part R3.4: the relation NAMED `rel` vs the
`rel=` field token — the desired bytes above are the discriminating pin.)

### 10.3 The other 8 .region pins + the whole suite

join_1 / merge_2 `.region.<mode>` (x8): byte-IDENTICAL (no demand forcing →
no RecognizedSubgraph → the interior arm derives the EMPTY set). Every
other golden in the suite (bespoke stdout, oracle, monotone, behavioral,
eqgate, df/rel/ir/h/contract irgold pins): byte-IDENTICAL — T1 mints no
node, changes no lowering, and the -region-out drain is the only consumer
of the new fields. Referee: the full suite run must show EXACTLY 8 divergent
goldens at the bless review, all `.region`, none else.

### 10.4 Determinism critique (the §5 lens applied to the interior arm)

- Interior-contract ORDER: ascending FIRST-forcing order of the distinct
  demanded decl Ids (RES-2) — a pure function of `RecognizedSubgraphs()`
  append order, which is the demand pass's own deterministic per-forcing
  stamp order (HP-9-clean: no pointer order, no UniqueId order). Dedup via
  a `seen` set keyed by decl Id, first-wins — insertion-order stable.
- E-numbering: one merged dense counter, insert-derived contracts first
  (their indices UNCHANGED — the RES-4 weakened-but-true invariant), then
  interior contracts. Fully positional; zero order-free tokens (the §9.2
  identity-referee discipline holds).
- `support=`: the OR over the decl's forcings' live annotated guard JOINs
  of `v.CanReceiveDeletions()` (role-blind — see the T1-IMPL-1 amendment in
  region-model-diffs.md; the kQueryProjection-only rule was CSE-fragile)
  (post-Optimize live view, RES-2); resolve failure ABORTS at freeze —
  no mode can silently drop a line (existence is decl-counted).
- Mode-stability: member-key is decl-sourced (mode-free by construction);
  the guard-JOIN resolve must succeed in all 4 modes — SETTLED at
  implementation: the 4-mode dump diff shows the identical §10.1/§10.2
  bytes in every mode, and the corpus-wide suite compiles every demand
  case in every mode post-T1-IMPL-1 (the role-filtered rule failed
  exactly here and was amended; a divergence is a STOP, not a bless).

### 10.5 R3a desired dump states (deferred to the R3a hunk — recorded here as the target shape)

For `region_declared_tc_witness` (the bracket-annotated demand_tc twin):
- `-region-out`: byte-EQUAL to demand_tc_witness's post-T1 bytes (10.1) —
  the bracket adds no port/contract/census field.
- `-contract-out` (its OWN real golden, oracle-4/RES-5): equal to
  demand_tc_witness's contract dump MODULO exactly two added lines on the
  demanded relation's views, the declared-key and scoped inferred-key
  annotations (exact spelling fixed at R3a implementation; pinned by
  `region_declared_tc_witness.contract.opt.golden`).
- `df/rel/ir/h opt`: SYMLINK goldens to demand_tc_witness's (RES-5) —
  byte-identity IS the referee that the overlay changes no lowering.
