I have everything I need from the real optimized graphs. Producing the three predicted `-instanceflow-out` goldens.

---

All ids below are derived from the **verified** optimized-Query DAGs (captured above via `-df-out`/`-region-out`/`-contract-out` at tip, default all-opt policy). `q#N` = the view's `det_seq` (the `^kind.N` suffix in `-df-out`); node id `if#0.N` reuses that `det_seq`. Blocks are in the design's §4 grammar. Lines I cannot pin without the real `ForEachUse` walk carry a trailing `; low-confidence` (never an invented token). Legend + findings + the V-IF-* consistency check follow the three dumps.

### (a) `join_1`

```
instanceflow  origins=20 uses=32 collections=2 sites=2 family=1

collections
  lc#0 decl=q/1
  lc#1 decl=never/1
sites
  ds#0 writer=q#18 -> lc#0
  ds#1 writer=q#19 -> lc#1

family if#0 context=empty scc-regions=0
  node if#0.0  origin=q#0  select  residual=(A,B)
  node if#0.1  origin=q#1  select  residual=(A)
  node if#0.2  origin=q#2  select  residual=(c4)
  node if#0.3  origin=q#3  select  residual=(c5)
  node if#0.4  origin=q#4  tuple   residual=(B)
  node if#0.5  origin=q#5  tuple   residual=(B)
  node if#0.6  origin=q#6  tuple   residual=(c8,B)
  node if#0.7  origin=q#7  tuple   residual=(c10)
  node if#0.8  origin=q#8  tuple   residual=(c11,B)
  node if#0.9  origin=q#9  tuple   residual=(c13)
  node if#0.10 origin=q#10 join    residual=(c14,B)
  node if#0.11 origin=q#11 join    residual=(c16,B)
  node if#0.12 origin=q#12 compare residual=(c18,B)
  node if#0.13 origin=q#13 compare residual=(c20,B)
  node if#0.14 origin=q#14 compare residual=(c22)
  node if#0.15 origin=q#15 compare residual=(c23)
  node if#0.16 origin=q#16 compare residual=(c24,B)
  node if#0.17 origin=q#17 compare residual=(c26,B)
  node if#0.18 origin=q#18 insert  residual=(B) -> lc#0
  node if#0.19 origin=q#19 insert  residual=(B) -> lc#1

coverage
  u#0  q#10 -> q#4  col=0 role=copied   domain=all occ=if#0.4  authority=ea#0
  u#1  q#17 -> q#5  col=1 role=copied   domain=all occ=if#0.5  authority=ea#1
  u#2  q#12 -> q#6  col=0 role=copied   domain=all occ=if#0.6  authority=ea#0
  u#3  q#12 -> q#6  col=1 role=copied   domain=all occ=if#0.6  authority=ea#0
  u#4  q#14 -> q#7  col=0 role=copied   domain=all occ=if#0.7  authority=ea#0   ; low-confidence (constant-elided edge)
  u#5  q#13 -> q#8  col=0 role=copied   domain=all occ=if#0.8  authority=ea#1
  u#6  q#13 -> q#8  col=1 role=copied   domain=all occ=if#0.8  authority=ea#1
  u#7  q#15 -> q#9  col=0 role=copied   domain=all occ=if#0.9  authority=ea#1   ; low-confidence (constant-elided edge)
  u#8  q#6  -> q#10 col=0 role=pivot    domain=all occ=if#0.10 authority=ea#0
  u#9  q#7  -> q#10 col=0 role=pivot    domain=all occ=if#0.10 authority=ea#0   ; low-confidence (constant-elided .in1)
  u#10 q#6  -> q#10 col=1 role=join-col domain=all occ=if#0.10 authority=ea#0
  u#11 q#8  -> q#11 col=0 role=pivot    domain=all occ=if#0.11 authority=ea#1
  u#12 q#9  -> q#11 col=0 role=pivot    domain=all occ=if#0.11 authority=ea#1   ; low-confidence (constant-elided .in1)
  u#13 q#8  -> q#11 col=1 role=join-col domain=all occ=if#0.11 authority=ea#1
  u#14 q#0  -> q#12 col=0 role=cmp-lhs  domain=all occ=if#0.12 authority=ea#0   ; shared-origin: q#12 fans to q#6(ea#0)+q#13(ea#1); ea#0=lowest-reachable
  u#15 q#0  -> q#12 col=1 role=copied   domain=all occ=if#0.12 authority=ea#0
  u#16 q#2  -> q#12 col=* role=cmp-rhs  domain=all occ=if#0.12 authority=ea#0   ; low-confidence (literal operand; col=* is UINT32_MAX sentinel)
  u#17 q#12 -> q#13 col=0 role=cmp-lhs  domain=all occ=if#0.13 authority=ea#1
  u#18 q#12 -> q#13 col=1 role=copied   domain=all occ=if#0.13 authority=ea#1
  u#19 q#2  -> q#13 col=* role=cmp-rhs  domain=all occ=if#0.13 authority=ea#1   ; low-confidence (literal operand)
  u#20 q#1  -> q#14 col=0 role=cmp-lhs  domain=all occ=if#0.14 authority=ea#0
  u#21 q#2  -> q#14 col=* role=cmp-rhs  domain=all occ=if#0.14 authority=ea#0   ; low-confidence (literal operand)
  u#22 q#1  -> q#15 col=0 role=cmp-lhs  domain=all occ=if#0.15 authority=ea#1   ; low-confidence (constant-elided edge)
  u#23 q#2  -> q#15 col=* role=cmp-rhs  domain=all occ=if#0.15 authority=ea#1   ; low-confidence (literal operand)
  u#24 q#11 -> q#16 col=0 role=cmp-lhs  domain=all occ=if#0.16 authority=ea#1
  u#25 q#11 -> q#16 col=1 role=copied   domain=all occ=if#0.16 authority=ea#1
  u#26 q#2  -> q#16 col=* role=cmp-rhs  domain=all occ=if#0.16 authority=ea#1   ; low-confidence (literal operand)
  u#27 q#16 -> q#17 col=0 role=cmp-lhs  domain=all occ=if#0.17 authority=ea#1
  u#28 q#16 -> q#17 col=1 role=copied   domain=all occ=if#0.17 authority=ea#1
  u#29 q#3  -> q#17 col=* role=cmp-rhs  domain=all occ=if#0.17 authority=ea#1   ; low-confidence (literal operand)
  u#30 root insert q#18 -> lc#0 domain=all authority=ea#0
  u#31 root insert q#19 -> lc#1 domain=all authority=ea#1

authorities
  ea#0 site=ds#0 domain=all writer=if#0.18
  ea#1 site=ds#1 domain=all writer=if#0.19

seeds
  seed join-pivot origin=q#10 columns=(c14)   ; degenerate: pivot constant-bound (A=1)
  seed join-pivot origin=q#11 columns=(c16)   ; degenerate: pivot constant-bound (A=1, A=2)
```

### (b) `merge_2`

```
instanceflow  origins=13 uses=21 collections=2 sites=2 family=1

collections
  lc#0 decl=q_outer/2
  lc#1 decl=q_proj/1
sites
  ds#0 writer=q#12 -> lc#0
  ds#1 writer=q#13 -> lc#1

family if#0 context=empty scc-regions=0
  node if#0.0  origin=q#0  select residual=(X,Y)
  node if#0.1  origin=q#1  select residual=(X,Y)
  node if#0.2  origin=q#2  select residual=(Y,X)
  node if#0.3  origin=q#3  tuple  residual=(X)
  node if#0.4  origin=q#4  tuple  residual=(X)
  node if#0.5  origin=q#5  tuple  residual=(X,Y)
  node if#0.6  origin=q#6  tuple  residual=(X)
  node if#0.7  origin=q#7  tuple  residual=(X,Y)
  node if#0.8  origin=q#8  tuple  residual=(Y,X)
  node if#0.9  origin=q#9  tuple  residual=(X,Y)
  node if#0.10 origin=q#10 merge  residual=(X,Y)
  node if#0.11 origin=q#11 merge  residual=(X)
  node if#0.12 origin=q#12 insert residual=(X,Y) -> lc#0
  node if#0.13 origin=q#13 insert residual=(X) -> lc#1

coverage
  u#0  q#2  -> q#3  col=1 role=copied domain=all occ=if#0.3  authority=ea#1
  u#1  q#1  -> q#4  col=0 role=copied domain=all occ=if#0.4  authority=ea#1
  u#2  q#10 -> q#5  col=0 role=copied domain=all occ=if#0.5  authority=ea#0
  u#3  q#10 -> q#5  col=1 role=copied domain=all occ=if#0.5  authority=ea#0
  u#4  q#11 -> q#6  col=0 role=copied domain=all occ=if#0.6  authority=ea#1
  u#5  q#1  -> q#7  col=0 role=copied domain=all occ=if#0.7  authority=ea#0
  u#6  q#1  -> q#7  col=1 role=copied domain=all occ=if#0.7  authority=ea#0
  u#7  q#2  -> q#8  col=0 role=copied domain=all occ=if#0.8  authority=ea#0
  u#8  q#2  -> q#8  col=1 role=copied domain=all occ=if#0.8  authority=ea#0
  u#9  q#0  -> q#9  col=0 role=copied domain=all occ=if#0.9  authority=ea#0
  u#10 q#0  -> q#9  col=1 role=copied domain=all occ=if#0.9  authority=ea#0
  u#11 q#7  -> q#10 col=0 role=merged domain=all occ=if#0.10 authority=ea#0
  u#12 q#8  -> q#10 col=0 role=merged domain=all occ=if#0.10 authority=ea#0
  u#13 q#9  -> q#10 col=0 role=merged domain=all occ=if#0.10 authority=ea#0
  u#14 q#7  -> q#10 col=1 role=merged domain=all occ=if#0.10 authority=ea#0
  u#15 q#8  -> q#10 col=1 role=merged domain=all occ=if#0.10 authority=ea#0
  u#16 q#9  -> q#10 col=1 role=merged domain=all occ=if#0.10 authority=ea#0
  u#17 q#3  -> q#11 col=0 role=merged domain=all occ=if#0.11 authority=ea#1
  u#18 q#4  -> q#11 col=0 role=merged domain=all occ=if#0.11 authority=ea#1
  u#19 root insert q#12 -> lc#0 domain=all authority=ea#0
  u#20 root insert q#13 -> lc#1 domain=all authority=ea#1

authorities
  ea#0 site=ds#0 domain=all writer=if#0.12
  ea#1 site=ds#1 domain=all writer=if#0.13

seeds
```

### (c) `transitive_closure`

```
instanceflow  origins=16 uses=30 collections=2 sites=3 family=1

collections
  lc#0 decl=tc/2
  lc#1 decl=is_node/1
sites
  ds#0 writer=q#13 -> lc#0
  ds#1 writer=q#14 -> lc#0
  ds#2 writer=q#15 -> lc#1

family if#0 context=empty scc-regions=1
  node if#0.0  origin=q#0  select residual=(From,To)
  node if#0.1  origin=q#1  tuple  residual=(From,To)
  node if#0.2  origin=q#2  tuple  residual=(AutoVar_2,Node)
  node if#0.3  origin=q#3  tuple  residual=(From,X)
  node if#0.4  origin=q#4  tuple  residual=(Node)
  node if#0.5  origin=q#5  tuple  residual=(Node)
  node if#0.6  origin=q#6  tuple  residual=(From,To)
  node if#0.7  origin=q#7  tuple  residual=(From,To)
  node if#0.8  origin=q#8  tuple  residual=(Node)
  node if#0.9  origin=q#9  tuple  residual=(From,To)
  node if#0.10 origin=q#10 join   residual=(X,From,To)
  node if#0.11 origin=q#11 merge  residual=(From,To)
  node if#0.12 origin=q#12 merge  residual=(Node)
  node if#0.13 origin=q#13 insert residual=(From,To) -> lc#0
  node if#0.14 origin=q#14 insert residual=(From,To) -> lc#0
  node if#0.15 origin=q#15 insert residual=(Node) -> lc#1
  scc scc#0 members=(q#1,q#2,q#3,q#10,q#11)

coverage
  u#0  q#10 -> q#1  col=1 role=copied   domain=all occ=if#0.1  authority=ea#0   ; scc-interior; ea#0=lowest-reachable
  u#1  q#10 -> q#1  col=2 role=copied   domain=all occ=if#0.1  authority=ea#0
  u#2  q#11 -> q#2  col=0 role=copied   domain=all occ=if#0.2  authority=ea#0
  u#3  q#11 -> q#2  col=1 role=copied   domain=all occ=if#0.2  authority=ea#0
  u#4  q#11 -> q#3  col=0 role=copied   domain=all occ=if#0.3  authority=ea#0
  u#5  q#11 -> q#3  col=1 role=copied   domain=all occ=if#0.3  authority=ea#0
  u#6  q#11 -> q#4  col=0 role=copied   domain=all occ=if#0.4  authority=ea#2
  u#7  q#11 -> q#5  col=1 role=copied   domain=all occ=if#0.5  authority=ea#2
  u#8  q#11 -> q#6  col=0 role=copied   domain=all occ=if#0.6  authority=ea#0
  u#9  q#11 -> q#6  col=1 role=copied   domain=all occ=if#0.6  authority=ea#0
  u#10 q#11 -> q#7  col=0 role=copied   domain=all occ=if#0.7  authority=ea#1
  u#11 q#11 -> q#7  col=1 role=copied   domain=all occ=if#0.7  authority=ea#1
  u#12 q#12 -> q#8  col=0 role=copied   domain=all occ=if#0.8  authority=ea#2
  u#13 q#0  -> q#9  col=0 role=copied   domain=all occ=if#0.9  authority=ea#0
  u#14 q#0  -> q#9  col=1 role=copied   domain=all occ=if#0.9  authority=ea#0
  u#15 q#2  -> q#10 col=0 role=pivot    domain=all occ=if#0.10 authority=ea#0
  u#16 q#3  -> q#10 col=0 role=join-col domain=all occ=if#0.10 authority=ea#0
  u#17 q#3  -> q#10 col=1 role=pivot    domain=all occ=if#0.10 authority=ea#0
  u#18 q#2  -> q#10 col=1 role=join-col domain=all occ=if#0.10 authority=ea#0
  u#19 q#1  -> q#11 col=0 role=merged   domain=all occ=if#0.11 authority=ea#0
  u#20 q#9  -> q#11 col=0 role=merged   domain=all occ=if#0.11 authority=ea#0
  u#21 q#1  -> q#11 col=1 role=merged   domain=all occ=if#0.11 authority=ea#0
  u#22 q#9  -> q#11 col=1 role=merged   domain=all occ=if#0.11 authority=ea#0
  u#23 q#4  -> q#12 col=0 role=merged   domain=all occ=if#0.12 authority=ea#2
  u#24 q#5  -> q#12 col=0 role=merged   domain=all occ=if#0.12 authority=ea#2
  u#25 root insert q#13 -> lc#0 domain=all authority=ea#0
  u#26 root insert q#14 -> lc#0 domain=all authority=ea#1
  u#27 root insert q#15 -> lc#1 domain=all authority=ea#2
  u#28 root query reachable_from bound=(From) -> reads lc#0 domain=all authority=ea#0
  u#29 root query reaching_to   bound=(To)   -> reads lc#0 domain=all authority=ea#0

authorities
  ea#0 site=ds#0 domain=all writer=if#0.13
  ea#1 site=ds#1 domain=all writer=if#0.14
  ea#2 site=ds#2 domain=all writer=if#0.15

seeds
  seed join-pivot        origin=q#10 columns=(X)
  seed boundary-binding  query=reachable_from columns=(From)
  seed boundary-binding  query=reaching_to    columns=(To)
```

---

### Legend (dump token semantics — pins fields, not pointers)

- `q#N` origin = post-optimize view whose `det_seq==N` (the `^kind.N` in `-df-out`); a total, dense bijection onto live views. `if#0.N` = that origin's node in the single flat family. `lc#N` collection, `ds#N` derivation site, `ea#N` emission authority, `u#N` origin-use — each dense in first-emitted (canonical) order.
- Node line kinds: `select tuple join merge compare insert` (the `QueryView` tags). `residual=(…)` = the FULL logical schema (empty-context baseline — V-IF-CONTEXT reduces to `residual == full schema`). `-> lc#N` present only on INSERT nodes (the collection they write).
- Coverage line = one `ForEachUse` column-role edge, ordered by `(consumer det_seq, producer_col, role-ordinal)`; `col=*` is the `UINT32_MAX` constant-sourced sentinel; `role` tokens map 1:1 to `InputColumnRole` (`copied/pivot/join-col/cmp-lhs/cmp-rhs/merged`). `occ=` = the consumer node. Root uses are per-view: `root insert q#W -> lc#L` (terminal materialization) and `root query <name> bound=(…) -> reads lc#L` (bound `#query` read).
- `authority=ea#N` on a **root insert** is its own site's writer (load-bearing, exact). On any **internal/read** use it is the *lowest-numbered forward-reachable* authority (advisory — see Finding F1).

### Consistency check (V-IF-*)

| witness | origins N | uses M | coverage records | collections/sites | authorities | scc-regions |
|---|---|---|---|---|---|---|
| join_1 | 20 | 32 | 32 (u#0..u#31) | 2 / 2 | 2 | 0 |
| merge_2 | 13 | 21 | 21 (u#0..u#20) | 2 / 2 | 2 | 0 |
| transitive_closure | 16 | 30 | 30 (u#0..u#29) | 2 / 3 | 3 | 1 |

- **V-IF-ORIGIN** — every `node.origin` is a live `q#N` in `[0,N)` and appears exactly once; the node set is a det_seq bijection onto the live views in each `-df-out` capture (20/13/16). `residual` arity equals the origin's column count on every line. PASS by construction.
- **V-IF-CONTEXT** — `context=empty` on the sole family; every node's `residual` is its full logical schema; no `transfers`. Vacuously PASS (the slice never grows context).
- **V-IF-COVERAGE** — `u#` are contiguous `[0,M)` with exactly one coverage record each, and every `occ`/producer/consumer resolves to a live node ⇒ coverage is a bijection onto `uses`. PASS in all three. (merge_2 is the clean maximal-sharing flagship: `q#1`→{u#1,u#5,u#6} and `q#2`→{u#0,u#7,u#8} — one shared origin node, disjoint uses, each covered once, into *disjoint* authorities ea#1/ea#0.)
- **V-IF-EMISSION** — `authorities` is a bijection onto `sites` (join_1 2↔2, merge_2 2↔2, tc 3↔3); every `writer` is an INSERT origin (if#0.18/19; if#0.12/13; if#0.13/14/15); every root-insert use's `authority` equals its own site's; every root-query-read's `authority` resolves to a live authority of the read collection (both tc reads → an ea over lc#0). PASS. Note tc's two authorities ea#0/ea#1 share collection lc#0 but are *distinct* sites (ds#0/ds#1) — the CLAUDE.md "member-view list holds each view once by identity" invariant in the wild — so V-IF-EMISSION's per-`(site,domain)` uniqueness holds, not per-collection.
- **Canonical-order stability** — every emitted vector orders on `det_seq` + stored relation/insert order + the fixed `ForEachUse` role order; no map iteration is ever emitted. Re-running yields byte-identical dumps.

### Findings for the design stage (surfaced, not fudged)

- **F1 — `authority=` on non-root uses is ill-defined under CSE fan-out.** `join_1 q#12` (the shared `A=1` guard) feeds both the `q` (ea#0) and `never` (ea#1) sinks; the whole `tc` SCC (`q#10/q#11` and the cyclic tuples) forward-reaches ea#0/ea#1/ea#2. I render the lowest-reachable representative (matching the design §4 sample, which prints `authority=ea#0` on the internal `q#0->q#9` and on the `reachable_from` read). **Recommend:** either drop `authority=` on internal/read uses, or spec it explicitly as "lowest reachable authority (advisory)"; keep V-IF-EMISSION asserting **only** on root-insert uses. The 16 tc SCC uses + 3 join_1 `q#12` uses are the concrete carriers.
- **F2 — collections = INSERT-target relations, not `relations ⧺ ios`.** The §4 example and all three dumps list only derivation-site-bearing relations (`q/never`; `q_outer/q_proj`; `tc/is_node`) — never the message inputs (`t1/t2`, `m1/m2/m3`, `add_edge`) nor the read-only bound-query relations (`reachable_from/reaching_to`, surfaced instead as `request-port`s + `root query` reads). Design §2.2's "iterate `query->relations` then `query->ios`, first-seen" over-counts. **Recommend §2.2 ⟶ §4:** `LogicalCollectionCatalog` = distinct `insert.Declaration().Id()` over live INSERT views.
- **F3 — constant-sourced edges are real uses the `-df` dump hides.** `Format.cpp:986/1009` early-return on `IsConstantOrConstantRef`, so eq-compare RHS operands and the constant-forced join `.in1` pivots / r-side tuples never render as `=>` lines — but `QueryCompare::ForEachUse` (`Query.cpp:1373-1377`) always yields `input_columns[1]`, and `Containing` returns the literal SELECT. These are the `col=*` / `; low-confidence` lines in join_1 (u#4,7,9,12,16,19,21,22,23,26,29). **V-IF-ORIGIN must accept literal-SELECT (`q#2`,`q#3`) producers**, and the builder must NOT reparse `-df` text. This also flips the hand-derivation's "orphan `compare.14/15`" claim: they are almost certainly not orphans — their outgoing edges to `tuple.7/tuple.9` are constant-elided (F6).
- **F4 — SCC lives per-node inside one FlatFamily (design R3).** tc's `scc#0 = {q#1,q#2,q#3,q#10,q#11}` is a region *inside* `if#0`; `q#11` (merge.11, `table=%table:4`) is simultaneously SCC-interior and the acyclic fan-out root feeding is_node/tc INSERTs. No family split. Member order = ascending `det_seq`; the design §4 sample's `(q#11,q#2,q#3,q#10,q#1)` differs — **treat the §4 order as illustrative; pin det_seq (HP-9).** (LOW-CONFIDENCE on the exact ordering rule until the builder is written.)
- **F5 — root uses are per-view, internal uses per-column.** Matches §4 (`root insert q#13` is one line; internal uses carry `col=`). `QueryInsert::ForEachUse` (`Query.cpp:1556-1590`) yields per-column `kMaterialized`; the catalog collapses to one `is_root` OriginUse per writer. **Open (LOW-CONFIDENCE):** tc's `%table:4` is read by the recursive select-proxies `tuple.2/tuple.3`; if `insert.13/14`'s `impl->successors` are non-empty the ForEachUse `IsRelation` branch takes the successor arm — this could change the root-use shape. Treated here as terminal materialization; a verify-time reconciliation point.
- **F6 — degenerate seeds should stay visible but annotated.** `join_1`'s `join-pivot` seeds key on a provably constant-bound pivot (`c14`,`c16`) ⇒ zero sharing benefit; I surface them with a `; degenerate` note rather than suppress (reviewable). `tc`'s `join-pivot origin=q#10 columns=(X)` is the genuine non-degenerate contrast, and its two `boundary-binding` seeds are the natural Phase-D first targets.