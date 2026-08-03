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
