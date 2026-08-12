# InstanceFlow IF4 — Flat-Grove Hand-Derivation, 3 Witnesses

Derived from `build/debug/bin/drlojekyll <case>.dr -df-out /dev/stdout` (default `PassPolicy` = **all optimizations on**, i.e. this *is* the optimized Query DAG InstanceFlow Phase B must mirror — see `bin/drlojekyll/Main.cpp:49,74`, `Query::Build(module, error_log, gPassPolicy, gDemand)`). Design anchors: `docs/proposals/InstanceFlow.md` §6 (object model, lines 466–523), §7.3 (FlatFamily, 599–622), §7.6 (SCC atomicity, 720–735), §8 (coverage/emission, 765–913).

## Stress finding #0 — the `-df` text dump is NOT a complete edge enumeration

`lib/DataFlow/Format.cpp:986-991` (`add_edge`) and `:1009-1013` (`add_join_edge`) both early-`return` when `src.IsConstantOrConstantRef()` is true (`Constant inputs have no producer block — skipped (PIN-1 territory)`, comment at `:986`). `QueryColumnImpl::IsConstantOrConstantRef` (`lib/DataFlow/Column.cpp:77-87`) is true not only for literal-SELECT columns but for any column whose `referenced_constant` was set by constant propagation through an upstream equality guard — e.g. in `join_1`, `r`'s `A` column is forced to the literal `1` by `A = t2(A), A = 1`, so its real producer→consumer edge into `join.10`'s pivot **never renders** as a `=>` line, even though `QueryView::Successors()` (the real graph, used by everything except this dump per the code comment at `:955-958`) still contains it. **Phase A's `OriginUseId`/column-lineage catalog must walk the live `QueryView`/`QueryColumn` API directly (`ForEachUse`, `NthInputPivotSet`, `Successors()`), never reparse `-df-out` text** — the text format is deliberately lossy for readability (it also never renders INSERT→SELECT materialization edges, `:955-958`). Also: the dump shows only *outgoing* edges per producer, never a consumer's incoming list — reconstructing "who feeds compare.13" requires the join/tuple/compare block's own column-source annotations (e.g. `pivot c14 <- .in0.c8, .in1.c10`), not a symmetric edge table.

---

## (a) `join_1` — pivot join, dead `@never` arm, 2 queries

Source: `tests/OptDiff/cases/join_1.dr` — `p(A,B):t1(A,B)`, `r(A):t2(A)`, `#query q(free B): p(A,B),r(A),A=1`, `#query never(free B): p(A,B),r(A),A=1,A=2`.

Dump shape (abbreviated; full capture above):
```
select.0 t1/2(A,B) => compare.12          select.1 t2/1(A) => compare.14
select.2 literal(c4)  [no rendered edge — constant]     select.3 literal(c5)  [no rendered edge]
compare.12(c18,B) eq => tuple.6(B), compare.13(B)        compare.14(c22) eq  [no successor]
compare.13(c20,B) eq => tuple.8(B)                       compare.15(c23)   [DEAD — zero successors]
tuple.6(c8,B)[table%12] => join.10.in0(B)                tuple.7(c10)[table%16]  [edge elided: constant-ref]
tuple.8(c11,B)[table%19] => join.11.in0(B)                tuple.9(c13)[table%23]  [edge elided: constant-ref]
join.10{pivot c14<-.in0.c8,.in1.c10; out B<-.in0.B} => tuple.4(B)
join.11{pivot c16<-.in0.c11,.in1.c13; out B<-.in0.B} => compare.16(B)
compare.16(c24,B) eq => compare.17(B) => tuple.5(B)
tuple.4(B)[insert-proxy] => insert.18(B) into %table:6      (q)
tuple.5(B)[insert-proxy] => insert.19(B) into %table:9      (never)
```

**QueryOrigins (20):** `select.{0,1,2,3}`, `tuple.{4,5,6,7,8,9}`, `join.{10,11}`, `compare.{12,13,14,15,16,17}`, `insert.{18,19}`.

**OriginUses** (producer→consumer; `*` = rendered constant-ref-elided edge, reconstructed from the join's own pivot/out annotation, not the `=>` grammar):
`select.0→compare.12`, `select.1→compare.14`, `select.2→compare.12*` and `select.2→compare.14*` (both are `A=1` guards — same literal value `1`), `select.3→compare.16*` (the `A=2` guard), `compare.12→{tuple.6, compare.13}`, `compare.13→tuple.8`, `tuple.6→join.10.in0`, `tuple.7→join.10.in1*`, `tuple.8→join.11.in0`, `tuple.9→join.11.in1*`, `join.10→tuple.4`, `join.11→compare.16`, `compare.16→compare.17`, `compare.17→tuple.5`, `tuple.4→insert.18`, `tuple.5→insert.19`. **`compare.14` and `compare.15` have zero live consumers** — orphan origins (open question below).

**LogicalCollections (2, INSERT-backed):** `LC(q)` = `%table:6` (writer: `insert.18` only); `LC(never)` = `%table:9` (writer: `insert.19` only). Each has exactly 1 `DerivationSiteId` — simplest case, no shared-authority tension.

**FlatFamily:** one `FamilyNode` per origin above, `context=empty`, `residual_schema` = the view's own logical schema (e.g. `join.10` residual `(A,B)` — pivot `A` still physically present pre-family-growth per §7.3's "full logical schema" baseline even though it happens to be provably constant). DAG mirrors the graph 1:1; `select.2`/`select.3` (the two literal producers) are **origins with no `FamilyEdge`** under the plain flat-edge model — they only participate as `Bind`-to-literal transfer inputs (see stress below), not as a pushed/probed port.

**UseCoverage:** every use above gets `domain=All(empty-context)`, one coverage record, `authority` = the `EmissionAuthorityId` of whichever downstream INSERT terminates that use's chain (`q`'s chain → `ea(insert.18)`; `never`'s chain → `ea(insert.19)`).

**EmissionAuthorities (2):** `ea(insert.18)` writer=`insert.18`, domain=All, derivation_site=`ds(q-rule)`; `ea(insert.19)` writer=`insert.19`, domain=All, derivation_site=`ds(never-rule)`.

**Candidate seeds:** `JoinPivot(join.10, {A})` and `JoinPivot(join.11, {A})` — both **degenerate**: the pivot slot is provably bound to the literal `1` (join.10) resp. `1` (join.11, with a *second*, structurally-separate post-join guard for `2`) by upstream constant propagation, so the "key" has cardinality ≤1 and any keyed family buys zero sharing — expect `NonPositiveEstimatedBenefit`/flat-stays-flat. No `BoundaryBinding` seeds: both `q` and `never` are declared `free`. No `AggregateGroup`/`ConsumptionGroup` seeds.

**Stresses:**
1. **Constant-ref pivot binding isn't in §5.1's vocabulary.** `EqualityClassProof(join_origin, pivot_pairs)` (InstanceFlow.md:394-400) is phrased as column-to-column identity; `join.10`'s pivot is column-to-*literal*. `ContextTransfer::Bind(slot, logical_column)` (line 362) needs to also accept a literal operand, or a new `BindLiteral` variant is needed — otherwise the transfer-certificate vocabulary can't even describe this join's real shape.
2. **Orphan origins with zero uses** (`compare.14`, `compare.15`): do they get a `QueryOriginId` at all under Phase A item 1 ("stable... catalogs after Query optimization")? They are real live views (post-CSE, not dead-flow-eliminated per `RemoveUnusedViews`/`CollectDeadCycles`), so presumably yes — but they then trivially satisfy `V-IF-COVERAGE` (empty use-set, vacuously covered) while contributing nothing to any `FamilyNode`'s `output_collection` consumer. Worth an explicit ruling: Phase A catalogs *all* live post-optimization views, dead-end or not.
3. **Unsat-but-uneliminated `never` arm** (file header: "unsatisfiable-input-view... making the JOIN unsat"): `join.11`+`insert.19` are fully lowered and executed at runtime (they will just never produce rows, since the pivot forced to `1` can never also satisfy the downstream `==2` guard) — DataFlow does **not** fold this to a compile-time reject/prune. This is a legitimate `LogicalCollectionId` (`never`) whose runtime cardinality is always 0; nothing in §6/§8 currently distinguishes "provably-empty collection" from any other, which may be worth a cheap tag for cost estimation (§7.5 benefit scoring) even if not for correctness.

---

## (b) `merge_2` — nested UNION flattening, `@inline` elision, shared-origin coverage

Source: `tests/OptDiff/cases/merge_2.dr` — `inner`/`proj` are `@inline`; `outer(X,Y):inner(X,Y)` (m1,m2) `∪ m3`; `q_outer(X,Y):outer(X,Y)`; `q_proj(X):proj(X,_)`.

Dump shape:
```
select.0 m3/2(X,Y) => tuple.9(X,Y)
select.1 m1/2(X,Y) => tuple.4(X), tuple.7(X,Y)
select.2 m2/2(Y,X) => tuple.3(X), tuple.8(Y,X)
tuple.3(X)[merge-canon/unused-col-guard from m2] => merge.11(X)
tuple.4(X)[merge-canon/unused-col-guard from m1] => merge.11(X)
tuple.7(X,Y)[merge-input-proxy, eqset2==select.1] => merge.10(X,Y)
tuple.8(Y,X)[merge-input-proxy, eqset3==select.2] => merge.10(X=Y,Y=X)
tuple.9(X,Y)[merge-input-proxy, eqset1==select.0] => merge.10(X,Y)
merge.10(X,Y) callers={tuple.7,tuple.8,tuple.9} => tuple.5(X,Y)
merge.11(X)   callers={tuple.3,tuple.4}        => tuple.6(X)
tuple.5(X,Y)[insert-proxy] => insert.12(X,Y) into %table:4     (q_outer / "outer")
tuple.6(X)[insert-proxy]   => insert.13(X)   into %table:8     (q_proj / "proj")
```

`inner` has **zero surviving QueryOriginIds** — its two clauses (`m1`,`m2`) are spliced directly into `merge.10` (the *nested-UNION-flattening* the file header names); `outer`'s own third arm (`m3`) also lands there. `proj`'s two clauses land in `merge.11`, each **first routed through a compiler-inserted `merge-canon/unused-col-guard` tuple** dropping the unused second column (Y).

**QueryOrigins (13):** `select.{0,1,2}`, `tuple.{3,4,5,6,7,8,9}`, `merge.{10,11}`, `insert.{12,13}`.

**OriginUses:** `select.0→tuple.9`, `select.1→{tuple.4, tuple.7}` (**one origin, two distinct uses — the shared-origin case**), `select.2→{tuple.3, tuple.8}` (same), `tuple.{3,4}→merge.11`, `tuple.{7,8,9}→merge.10`, `merge.10→tuple.5`, `merge.11→tuple.6`, `tuple.5→insert.12`, `tuple.6→insert.13`.

**LogicalCollections (2):** `LC(q_outer)`=`%table:4` (single writer `insert.12`), `LC(q_proj)`=`%table:8` (single writer `insert.13`) — **note the collection is named for the *query*, not the original `#local` (`outer`/`proj` names don't survive as distinct table identities; `inner` doesn't survive as an origin at all)**. Unlike `join_1`/`transitive_closure`, both collections here have exactly **one** derivation site each even though `outer` had 2 source clauses (`inner`,`m3`) and `proj` had 2 (`m1`,`m2`): CSE fully unions non-recursive multi-clause definitions down to one physical INSERT.

**FlatFamily:** `select.1` and `select.2` are the flagship §7.3 "maximal sharing" case — each is **one `FamilyNode`** with **two `OriginUse` coverage records**, exercising exactly the invariant at InstanceFlow.md:770-776 ("one Query origin may occur in many families while every consumer obligation is still covered exactly once"): here it's one *family node* (not "many families") feeding two disjoint uses, both `domain=All`, no overlap.

**UseCoverage / EmissionAuthorities:** trivial, 1:1 with the two INSERT origins as above.

**Candidate seeds:** **none.** No joins, no aggregates, both queries (`q_outer(free,free)`, `q_proj(free)`) are unbound. This is the clean "flat is final" witness — the grove has no growth frontier at all (§7.5's `build_grove` loop terminates on iteration 0, `frontier` empty from the start).

**Stresses:**
1. **`@inline` origins vanish entirely** before InstanceFlow ever runs (Phase A "after Query optimization" — `inner` is gone by then). Any later debugging surface (`why-family`, `trace-query-use`, §16.2) that wants to explain provenance back to the *source relation name* `inner` needs a side-channel (parse-level declaration ↔ optimized-view provenance), since no `QueryOriginId` will ever exist for it. This is exactly what the existing K5 `origin_decls` machinery (CLAUDE.md "TIER-2 ORIGIN PROVENANCE") already solves for row-contracts — Phase A/B should reuse `QueryView::OriginDecls()` rather than re-deriving it.
2. **The compiler-inserted `merge-canon/unused-col-guard` tuples** (`tuple.3`,`tuple.4`) are pure column-projection artifacts of canonicalization, not source-visible operators — under §7.4's `transfer_context`, they're plain "exact TUPLE forwarding," so they should backward-transfer transparently, but they *do* consume a `QueryOriginId` slot and *do* narrow the residual schema (dropping Y) even in the flat, empty-context baseline. Worth confirming Phase A's origin catalog treats compiler-synthesized canonicalization tuples identically to source-authored ones (it should — nothing in §6 distinguishes them), but it's an easy place for an implementation to special-case incorrectly.

---

## (c) `transitive_closure` — recursive SCC, `WholeQueryScc` flat region, 2 derivation sites → 1 collection

Source: `tests/OptDiff/cases/transitive_closure.dr` — `tc(From,To):-tc(From,X),tc(X,To)` (recursive) `∪ add_edge` (base); `#query reachable_from(bound From, free To)`; `#query reaching_to(free From, bound To)`; `#query is_node(free Node): tc(Node,_) ∪ tc(_,Node)`.

Dump shape (cycle-annotated edges per the `; cycle` comment emitted by the SCC pass in `Format.cpp`):
```
select.0 add_edge/2(From,To) => tuple.9(From,To)
tuple.9(From,To)[eqset1] => merge.11(From,To)                       ; base-case feed
merge.11(From,To)[table=%table:4, monotone, set=0 depth=1] callers={tuple.1,tuple.9}
  => tuple.2(AutoVar_2=From,Node=To) [cycle] => join.10.in1(AutoVar_2,Node)
  => tuple.3(From,X=To)              [cycle] => join.10.in0(X,From)
  => tuple.4(Node=From) => merge.12(Node)
  => tuple.5(Node=To)   => merge.12(Node)
  => tuple.6(From,To)   => insert.13(From,To) into %table:4         ; DerivationSite A
  => tuple.7(From,To)   => insert.14(From,To) into %table:4         ; DerivationSite B
join.10{pivot X<-.in0.X,.in1.AutoVar_2; out From<-.in0.From, To<-.in1.Node}
  => tuple.1(From,To) [cycle] => merge.11(From,To) [cycle]           ; closes recursion
merge.12(Node) callers={tuple.4,tuple.5} => tuple.8(Node) => insert.15(Node) into %table:8
```

**QueryOrigins (16):** `select.0`, `tuple.{1..9}` (9), `join.10`, `merge.{11,12}`, `insert.{13,14,15}`.

**SCC (`WholeQueryScc`, §7.6):** `{merge.11, tuple.2, tuple.3, join.10, tuple.1}` — all five share `stratum=2` in the dump and every edge among them is `; cycle`-tagged. `tuple.4,5,6,7` (is_node/tc-insert feeds), `merge.12`, `tuple.8`, `insert.{13,14,15}` are **acyclic consumers of the SCC's settled output**, sitting outside it. This is the exact InstanceFlow.md:720-735 shape: the recursive fixpoint is one atomic unit; `merge.11` is simultaneously "inside the cycle" *and* the collection's own backing model (`table=%table:4`) — a case where a `FamilyNode`'s membership in `WholeQueryScc(...)` and its role as an acyclic downstream fan-out root coincide on the same origin.

**LogicalCollections (2):** `LC(tc)` = `%table:4`, **two distinct writers** `insert.13` (recursive-rule contribution) and `insert.14` (base-rule contribution) — both read from the *identical* upstream node `merge.11` yet are kept as two separate, non-deduped INSERT views (matches CLAUDE.md's documented invariant: "a table's member-view list holds each view at most once, by IDENTITY — never dedup it structurally... the group_ids CSE guard" — this is precisely that pattern in the wild, both share `eqset=12`). `LC(is_node)` = `%table:8`, **one writer** `insert.15`, even though `is_node` also has 2 source clauses — here CSE safely folds both clause arms (`tc(Node,_)`, `tc(_,Node)`) into one `merge.12`→one insert, because `is_node`'s table is a plain non-recursive leaf.

  **Design implication:** `DerivationSiteId` cardinality per collection is **not** predictable from parse-time rule count alone (`tc`: 2 rules→2 sites; `is_node`: 2 rules→1 site). It must be read as "1 per live INSERT view post-optimization," and the pattern above (recursive relation keeps rule-provenance separate; non-recursive relation may fully merge) is the load-bearing reason recursive tables retain 2 sites — worth a `V-IF-EMISSION`-adjacent note in the design doc rather than assumed.

**FlatFamily / UseCoverage / EmissionAuthorities:** flat mirror as usual; `ea(insert.13)`/`ea(insert.14)` are two authorities over the *same* `LogicalCollectionId` but *different* `DerivationSiteId`s (no §8.4 conflict — non-overlap is required only within one derivation site's domain partition); `ea(insert.15)` is the single is_node authority (itself covering 2 logical derivation sites at once via the pre-merged upstream — an `EmissionAuthority` whose `writer` legitimately serves >1 rule-level derivation once CSE has proven them coincident).

**Candidate seeds (the rich case):**
- `BoundaryBinding(reachable_from, {From})` — `#query reachable_from(bound From, free To)` is a *real* bound root. Since `join.10`'s `out From <- .in0.From` is a straight pass-through at every level of the recursion (`tc(From,X):-tc(From,x1),tc(x1,X)` — `From` never participates in the pivot), a `From`-keyed `WholeQueryScc` family looks admissible under §7.4's recursive rule ("admit the entire SCC only if every cycle preserves the signature") — this is structurally the demand/keyed-instance TC shape CLAUDE.md's `-demand`/`key_tc_witness` corpus already exercises, now expressed natively as an InstanceFlow candidate rather than a bolted-on transform.
- `BoundaryBinding(reaching_to, {To})` — symmetric, keyed on `To` (via `join.10`'s `out To <- .in1.Node`).
- `JoinPivot(join.10, {X})` — genuine, non-degenerate (unlike `join_1`'s literal-bound pivots): `X` is a real intermediate node, not knowable at compile time.
- **Selection tension (§7.5 exercise):** `From`-keyed and `To`-keyed families are *both* admissible on the *same* SCC but are different context signatures — the global greedy loop must pick at most one (or certify both as non-overlapping-cost coexisting families sharing the SCC's settled base), which is a direct, concrete instance of the tie-break/benefit-scoring machinery the design doc leaves abstract at line 692-717. This witness is the natural first target for Phase D ("first contextual specialization — join pivots," line 1889) once Phase A/B lands.

**Stresses:**
1. **A `FamilyNode` can be both `WholeQueryScc`-owned and a plain acyclic fan-out root simultaneously** — `merge.11` is the recursive union *and* the thing `tuple.6`/`tuple.7`/`tuple.4`/`tuple.5` (all acyclic) read from. §6's `scc_ownership: Acyclic | WholeQueryScc(QuerySccId)` is a per-`FamilyTemplate` field (line 491), but a flat baseline family built by mirroring the *whole* DAG (§7.3) will contain both SCC-interior and SCC-exterior nodes in what might naively be "one family" — Phase B must decide whether the flat baseline itself splits at SCC boundaries into (at minimum) two families (one `WholeQueryScc`, one `Acyclic` downstream) or whether `scc_ownership` is tracked per-`FamilyNode` inside one family. The design doc's `FamilyTemplate.scc_ownership` (singular, template-level) suggests the former; this witness is the concrete proof that the flat-plan builder needs an explicit SCC-boundary split rule, not just "mirror the DAG."
2. Confirms stress #0's warning is load-bearing here too: `join.10`'s pivot columns (`.in0.X`,`.in1.AutoVar_2`) are real (non-constant) recursive-cycle edges and *do* render correctly in this witness (no constant-ref elision) — but only because nothing upstream forces `X` to a literal. A production Phase-A column-lineage walk must not assume "renders in `-df-out`" is a reliable signal of edge liveness in general, per witness (a).