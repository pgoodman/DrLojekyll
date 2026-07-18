# D4-S3 — the demand-transform CORE recipe: node-by-node construction at the :2575 slot

D4-S3, DESIGNER, demand-seeds / keyed-instances epoch. Written 2026-07-17
against branch `demand-seeds`, tip `0f396116` (the `-demand` FOUNDATION
landed: the flag, the mode-gated `ApplyDemandTransform` at the Build.cpp
slot, `ParsedModule::FabricateDemandMessage`). This artifact answers the
D4 implementer's declared STOP: the d1 artifact specifies the MECHANISM
(Option D′ fabrication + N-pivot guard JOIN + demand relation + SIP) and
the TARGET graph (the spike), but NOT the concrete `QueryImpl`
node-construction recipe. This is that recipe, decided between two roads,
grounded node-for-node in the ACTUAL dumped graphs.

R4/P3 discipline: every code claim is anchored to a `file:line` read THIS
session on THIS branch; every graph claim is anchored to a dumped
`.dot`/`.ir`. The recipe reuses ratified precedents (the E-32
`ApplyPositiveConditionTest` splice, the landed `FabricateDemandMessage`),
minting nothing whose ctor is not already exercised in `lib/DataFlow`.

Session scratchpad (dumped graphs + working log):
`.../scratchpad/d4s3/{base,spike}.{opt,nodf}.dot`,
`.../scratchpad/d4s3-design-log.md`.

--------------------------------------------------------------------------
## 0. ANCHOR TABLE (read this session, tip 0f396116)

| # | Anchor | Fact |
|---|--------|------|
| C1 | Build.cpp:2517-2519 | `Query::Build(module, log, optimize, demand_mode)` — the signature ALREADY carries `demand_mode` (S1 landed). |
| C2 | Build.cpp:2527-2535 | the CLAUSE LOOP: `for (ParsedClause clause : sub_module.Clauses()) BuildClause(impl, clause, context, log)` — Road-C's would-be site. |
| C3 | Build.cpp:2551-2565 | post-clause: `RemoveUnusedViews → ClearGroupIDs → TrackDifferentialUpdates → Simplify → ConnectInsertsToSelects`. |
| C4 | Build.cpp:2575 | `impl->ApplyDemandTransform(module, log, demand_mode)` — THE PASS SLOT (after Connect, before Optimize). |
| C5 | Build.cpp:2582-2607 | after the pass: `Optimize` (if opt) → `ConvertConstantInputsToTuples` → `RemoveUnusedViews` → `ProxyInsertsWithTuples` → `LinkViews` → `IdentifyInductions` → `FinalizeDepths` → **`FinalizeColumnIDs`** → `TrackDifferentialUpdates(final)`. |
| C6 | Build.cpp:219-269 | `BuildPredicate`: message branch (:226-234) mints `ios.Create(decl)` + `selects.Create(input, pred)` + `input->receives.AddUse(view)`; columns via `view->columns.Create(var, view, VarId(context,var), idx)` (:264). Needs a `ParsedPredicate` + clause-context var ids. |
| C7 | Build.cpp:1450-1512 | `ApplyPositiveConditionTest` — the ⊥c 1-pivot splice, THE E-32 precedent the guard JOIN generalizes: `joins.Create()`; `join->joined_views.AddUse(ext)+AddUse(sel)`; `join->num_pivots`; `join->columns.Create(type, join, id, idx)`; `join->out_to_in.emplace(pivot, join)` then `pivot.second.AddUse(both in-cols)`; a restoring `tuples.Create()`. Pivot col "deliberately NOT a constant" (:1457-1459). |
| C8 | Build.cpp:2227-2281 | INSERT creation: `inserts.Create(rel, decl)` / `inserts.Create(stream, decl)`; `rel->inserts.AddUse(insert)`; `insert->input_columns.AddUse(col)`. |
| C9 | Connect.cpp:161-289 | `ConnectInsertsToSelects`: per relation, `CreateProxyOfInserts(this, rel->inserts)` builds a MERGE over the rule-body proxies, `ProxySelects(this, rel->selects, insert_proxy)` links every reader SELECT to that MERGE. So POST-CONNECT: `rel = MERGE(rule-body proxies)`, read by proxied SELECTs; `rel->inserts` is CLEARED (:261). |
| C10 | Connect.cpp:10-65 | `CreateProxyOfInserts`: per INSERT a TUPLE proxy (`tuples.Create`), unioned by `merges.Create()` (:54) with `merge->merged_views.AddUse(proxy)` (:65) + `merge->columns.Create(...)` (:60). This MERGE is the `path`/`d_path` UNION node in the dumps. |
| C11 | Query.h:1103-1117 | the `QueryImpl` DefLists: `ios`, `relations`, `selects`, `tuples`, `joins`, `merges`, `inserts`, … — every ctor the recipe calls. |
| C12 | Query.h:1086-1089 | `decl_to_input` (ParsedDeclaration→QueryIOImpl), `decl_to_relation` (ParsedDeclaration→QueryRelationImpl) — the maps `BuildPredicate` keys minting on. |
| C13 | Parse/Demand.cpp:35-151 | `ParsedModule::FabricateDemandMessage(name, param_types)` — LANDED: `CreateDerived<ParsedMessageImpl>` + `messages.AddUse` + OpenBuffer/lex naming (G1) + per-param TypeLoc reuse + G3 collision scan (→nullopt) + G2 assert. Returns `optional<ParsedMessage>`. |
| C14 | DataFlow/Demand.cpp:64-96 | `QueryImpl::ApplyDemandTransform(module, log, demand_mode)` — LANDED stub: mode gate (:71-73) + G2 re-entry reject (:78-84) + a clean "not yet implemented" diagnostic (:92-95). The recipe replaces :86-95. |
| C15 | Parse.h:266-338 | `ParsedClauseImpl`: head_variables/body_variables DefLists, `variables` id-map, `groups{comparisons,assignments,aggregates,positive_predicates,negated_predicates}`, `named_var_ids`, `next_var_id` — the HEAVY object Road-C would fabricate. |
| C16 | Aggregate.cpp:100-239 | the ONLY in-repo synthetic clause+predicate precedent — and it runs DURING PARSING with a token stream (`ReadNextSubToken`, `anon_clause_toks`, `CreateVariable(clause,...)`, `pred->argument_uses.AddUse(arg)`, `FinalizeDeclAndCheckConsistency`). Evidence that clause fabrication is token/parse-time work. |
| C17 | d4-impl-log.md:53,63 | the LANDED "KEY SIMPLIFICATION": `BuildPredicate` is NOT needed for the message; the pred-less `QuerySelectImpl(stream, DisplayRange)` ctor + `BuildIOProcedure`'s minimal reads collapse the demand-message plumbing to ONE `ParsedMessage`, NO ParsedPredicate/clause/variable chain. |

DUMPED-GRAPH ANCHORS (checkpoint-3, via `-dot-out` / Main.cpp:302-314):

| # | Graph | Fact |
|---|-------|------|
| G1 | base.nodf.dot | BASE `path` = MERGE (UNION, TABLE 8) over TWO members: {base-rule TUPLE-chain → edge_2 RECEIVE} and {recursive-rule TUPLE → JOIN(pivot M) of path-TUPLE and edge_2-TUPLE}. `reachable_from` = a SEPARATE relation: UNION → TUPLE → reads path TABLE 8. |
| G2 | spike.nodf.dot | TARGET adds: `I/O d_path_bf → RECEIVE[From]`; `d_path` MERGE (TABLE 8) over TWO members {TUPLE-chain from d_path_bf RECEIVE (root seed)} and {TUPLE-chain projecting [F] from path (recursive subgoal)}; and `path` MERGE (TABLE 11) whose EACH member is now `JOIN(pivot F)` of {a d_path TUPLE} and {the original rule body}, output [F,T]. `reachable_from` UNCHANGED (still projects from path). |
| G3 | spike.opt.dot | opt CSE-fuses the two `d_path` sources into one MERGE (TABLE 8) but the two guard JOINs on `path` (TABLE 11) SURVIVE (structural distinctness holds through Optimize — §3(b)). Base→spike node delta is the SAME node set as nodf, minus the fused duplication. |

--------------------------------------------------------------------------
## 1. THE TWO ROADS, PRICED

Both roads target the SAME graph (G2). They differ in the REPRESENTATION
LEVEL at which the pass constructs it.

### Road-G — graph-level splice (mutate the built `QueryImpl`)

The pass at C4:2575 mutates the post-`ConnectInsertsToSelects` graph
directly: fabricate the demand `ParsedMessage` (C13, landed), mint its
`QueryIO`+RECEIVE+`d_path` MERGE via the C6 ctor pattern, and for each of
`path`'s rule-body proxy members interpose a guard JOIN (the C7 pattern)
between the member and the `path` MERGE.

- COMPLEXITY: the recipe mints a bounded, ENUMERABLE node set (§3): 1 IO,
  1 receive SELECT, 2 demand-source TUPLE chains, 1 `d_path` MERGE, 1
  demand INSERT, and per rule-body-member {1 demand SELECT + 1 JOIN + 1
  restoring TUPLE}. Every ctor is C6/C7/C8/C10 — ALL already exercised in
  `lib/DataFlow`. No parse-object fabrication beyond the landed
  `ParsedMessage`.
- RISK: the pass reads a graph shape (`rel = MERGE(proxies)`, C9/C10) it
  did not build, and re-derives "which rule bodies produce `path`" and
  "which subgoals demand `path`" by WALKING that graph (SIP over views,
  §3.4). The column-id discipline (§3.5) is a real hazard: minted columns
  need ids that don't collide with existing ones and that `Equals()` reads
  correctly during the intervening Optimize (before C5's
  `FinalizeColumnIDs` re-numbers). This is the genuine surface.
- JUDGED-ARGUMENT DELTA: NONE. The d1 (a)-(f) argument, the E-32
  structural-distinctness discharge, the §3.x CSE-fusion argument, and the
  p3 §1.3 pipeline-placement rationale ALL assume this level (the pass runs
  at :2566/:2575 on the built graph, before Optimize/Stratify). Road-G is
  the ratified mechanism's literal realization — zero re-argument.

### Road-C — clause-level fabrication (fabricate `ParsedClause`s, re-run BuildClause)

The pass runs EARLIER — inside/after the C2:2527 clause loop, still
mode-gated — does SIP over PARSED CLAUSES (rule structure explicit), and
fabricates the demand RULES as parsed objects (`ParsedClauseImpl` +
`ParsedPredicateImpl` + `ParsedVariableImpl` chains), letting
`BuildClause` + `ConnectInsertsToSelects` produce the graph the normal
way. The spike PROVED this road's OUTPUT graph (its hand-authored `.dr` IS
the clause-level output, and it compiled to the correct guarded/pruned
graph in all 4 modes).

- COMPLEXITY: HIGHER, and in the wrong place. A `ParsedClauseImpl` (C15)
  carries head_variables/body_variables DefLists, a token-keyed
  `variables` id-map, `named_var_ids`/`next_var_id`, and `groups` each
  with comparisons/assignments/aggregates/positive+negated predicates.
  Fabricating one means fabricating `ParsedVariableImpl`s with correct
  id-interning, `ParsedPredicateImpl`s with `argument_uses` per subgoal,
  and re-running the whole `BuildClause` machinery. The ONLY in-repo
  precedent (C16, Aggregate.cpp) does this DURING PARSING with a live
  TOKEN STREAM (`anon_clause_toks`, `ReadNextSubToken`, `CreateVariable`,
  `FinalizeDeclAndCheckConsistency`) — i.e. clause fabrication is
  fundamentally token/parse-time work, not a post-parse graph pass. Road-C
  would either (i) re-implement that token machinery post-parse, or (ii)
  move the whole demand pass BEFORE the clause loop and generate `.dr`
  source text to re-lex — the SLDMagic "never a source-generator" smell
  the d1 §1.2 Option-P rejection already named.
- RISK: it DISCARDS the two landed simplifications. The D4 implementer's
  C17 finding — fabrication collapses to ONE `ParsedMessage`, no
  ParsedPredicate/clause/variable chain — is a Road-G finding; Road-C
  needs exactly the chain C17 eliminated. And the landed
  `FabricateDemandMessage` (C13) fabricates a message, not the demand
  RELATION's clauses; Road-C needs a `FabricateDemandClause` primitive
  that does not exist.
- JUDGED-ARGUMENT DELTA: the pass would move EARLIER than :2566. The p3
  §1.3 placement argument ("AFTER ConnectInsertsToSelects so producing
  rules are wired; BEFORE Optimize so demand sources CSE-fuse; BEFORE
  Stratify so the cross-check validates demand edges") is stated against
  the :2566 slot. Running before the clause loop means the demand clauses
  go through Simplify/Connect/Optimize/Stratify the SAME way user clauses
  do — so the CSE-fusion (p3 §1.3 pt 2) and Stratify-validates-demand (pt
  3) arguments STILL HOLD (they hold for ANY clause, and demand clauses
  become ordinary clauses). BUT pt 1 ("needs Connect done to FIND
  producing rules") is only relevant to a graph-level SIP; a clause-level
  SIP reads producing rules off the clause AST directly, so pt 1 is moot
  under Road-C. Net: the pipeline-placement argument survives with pt-1
  re-scoped (SIP reads clauses, not the graph). NOT a ratified-scope
  shift, but a re-argument the owner would want to see.

--------------------------------------------------------------------------
## 2. DECISION — ROAD-G, with a clause-informed SIP

**DECISION: Road-G (graph-level splice).** The pass at C4:2575 mutates the
built `QueryImpl`, reusing the landed `FabricateDemandMessage` (C13) and
the ratified `ApplyPositiveConditionTest` splice pattern (C7).

DEFENSE (one paragraph, then the point-by-point):

The landed foundation IS a Road-G foundation. `ApplyDemandTransform` sits
at :2575 — AFTER the clause loop (C2) and AFTER `ConnectInsertsToSelects`
(C3) — so by the time the pass runs, the clauses are GONE and the graph is
built; the pass's only lever is graph mutation. The D4 implementer's KEY
SIMPLIFICATION (C17: ONE `ParsedMessage`, no ParsedPredicate/clause/variable
chain) and `FabricateDemandMessage`'s message-only fabrication (C13) were
chosen PRECISELY because a graph-level pass mints `QueryIO`/SELECT/JOIN
nodes directly and needs only a message decl to satisfy the
`ParsedMessage`-keyed handler wiring (the d1 F1/Option-D′ result). Road-C
would throw both away and re-introduce the full clause-fabrication surface
that Aggregate.cpp (C16) shows is token/parse-time work. Crucially,
Road-C's "the spike proved it" is about the TARGET graph, not the
construction level: the spike is the graph Road-G must REPRODUCE, and I
dumped it (G2/G3) node-for-node so Road-G can mint exactly the nodes
`BuildClause` produces from the spike's clauses. The judged (a)-(f)
argument and the p3 §1.3 placement rationale are stated against this level
— Road-G needs ZERO re-argument; Road-C needs the pt-1 re-scope. Road-G
reuses two ratified precedents; Road-C invents a `FabricateDemandClause`.

Point by point:
1. LANDED FOUNDATION FIT: the pass slot (:2575, post-clause-loop) and the
   message-only fabrication (C13/C17) are Road-G artifacts. Road-C strands
   both. [C4, C13, C17]
2. PRECEDENT REUSE: Road-G's guard JOIN IS `ApplyPositiveConditionTest`
   (C7) generalized 1→N pivots — the exact E-32-ratified move p3 §1.2
   specifies. Road-C's clause fabrication has only the parse-time
   Aggregate.cpp precedent (C16), which does not transplant post-parse.
3. JUDGED-ARGUMENT STABILITY: Road-G = zero delta (the argument assumes
   this level). Road-C = the pt-1 placement re-scope + a new
   clause-fabrication surface to re-argue for (a)/(b)/(f). [d1 §3, p3 §1.3]
4. SIP SIMPLICITY (the ONE Road-C advantage) is RECOVERED cheaply: the
   graph is a shallow, regular shape — `rel = MERGE(rule-body proxies)`
   (C9/C10), each proxy a TUPLE-chain bottoming at SELECTs/JOINs. The SIP
   walk reads producing rules as the MERGE's `merged_views` and demanding
   subgoals as SELECTs-on-`p`'s-relation (§3.4). The clause AST's
   explicitness buys little over this regular graph, and the graph carries
   the post-Simplify column identity the splice needs.

The residual honest cost Road-G owns: the intra-pass column-id discipline
(§3.5) and the graph-shape SIP (§3.4). Both are bounded and specified
below; neither is a re-argument of the ratified mechanism.

--------------------------------------------------------------------------
## 3. THE RECIPE (Road-G, node-by-node for the witness)

Witness: the base program (base.nodf.dot / G1):
```
#message edge_2(u64 From, u64 To).
#local path(u64 From, u64 To).
path(F, T) : edge_2(F, T).
path(F, T) : path(F, M), edge_2(M, T).
#query reachable_from(bound u64 From, free u64 To) : path(From, To).
```
Target: the spike graph (spike.nodf.dot / G2). The recipe reproduces G2
from the base module. It runs INSIDE `ApplyDemandTransform`
(DataFlow/Demand.cpp), replacing the C14 "not yet implemented" body
(:86-95), AFTER the mode gate (:71-73) and G2 re-entry reject (:78-84).

### 3.1 SIP + adornment discovery (produces the demand set)

Walk from each bound `#query` (§3.4 details the graph walk). For the
witness this yields: `path` demanded at adornment `bf` (bound col 0 =
`From`), |bound|=1 ≥ 1 (not the all-free skip, §3.6). One demanded
adornment, one demand relation `d_path` (arity 1). Multi-adornment / the
inertness skip / the negation-sink diagnostic: §3.6.

### 3.2 The demand MESSAGE + its RECEIVE + `d_path` root source

Reproducing G2's `I/O d_path_bf → RECEIVE[From]` and the root member of
the `d_path` MERGE. Every step cites its precedent ctor:

1. **Fabricate the message** (LANDED, C13):
   `auto d_msg_opt = module.FabricateDemandMessage("__demand_reachable_from_bf",
   {u64_typeloc_of_From});` where `u64_typeloc_of_From` is `p`'s bound
   parameter's `TypeLoc` (reused, not re-lexed — Parse/Demand.cpp:118-125).
   `nullopt` ⇒ G3 collision ⇒ clean diagnostic + `return false`.
2. **Mint the `QueryIO`** (C6:227-230 pattern, the ratified message
   branch): `QueryIOImpl *io = query->decl_to_input[ParsedDeclaration(d_msg)];
   if (!io) io = query->ios.Create(ParsedDeclaration(d_msg));`. This is the
   `I/O d_path_bf` node (G2).
3. **Mint the RECEIVE SELECT.** Two sub-options, and C17 picks the LANDED
   one: the D4 implementer found `BuildPredicate` (hence a
   `ParsedPredicate`) is NOT needed — the pred-less
   `QuerySelectImpl(QueryStreamImpl*, DisplayRange)` ctor (Build.cpp
   :1392/:1748 precedent) suffices because `BuildIOProcedure` reads only
   `io.Declaration()` + `receives[0].Columns()`. So:
   `QuerySelectImpl *recv = query->selects.Create(io, DisplayRange());`
   `io->receives.AddUse(recv);`
   then one output column per message param:
   `recv->columns.Create(u64_typeloc, recv, /*id=*/mint_id(), /*idx=*/0u);`
   (the `columns.Create(type, view, id, idx)` overload, C6:264 shape but
   with a minted id — §3.5). This is `RECEIVE[From]` (G2).
4. **Mint the `d_path` RELATION + root INSERT.** `d_path` is a `#local`-
   equivalent relation. Mint it directly (C6:239-244 relation branch,
   without a ParsedDeclaration — use the relation ctor keyed on a
   fabricated `#local` OR, cleaner, reuse the message's own decl is wrong;
   §5-A1 flags this). Concretely: `QueryRelationImpl *d_rel =
   query->relations.Create(<d_path local decl>);` — see §5 AMENDMENT A1 for
   the `d_path` local-decl question (the ONE genuinely-new fabrication
   beyond the landed message). Then the root member:
   a TUPLE proxy over `recv` (a `tuples.Create()` copying `recv`'s columns,
   C10 pattern) feeding a demand INSERT:
   `QueryInsertImpl *ins0 = query->inserts.Create(d_rel, <d_path decl>);`
   `for (col : recv_tuple->columns) ins0->input_columns.AddUse(col);`
   `d_rel->inserts.AddUse(ins0);` (C8 pattern).
   This is the root member of G2's `d_path` MERGE.

Note: **POST-JUDGE (F4): this Note is CORRECTED.** The claim that the demand
INSERTs "will be picked up by the RemoveUnusedViews/ProxyInsertsWithTuples/
LinkViews re-runs … which re-proxy inserts" is FALSE — none of those passes
do INSERT->SELECT relation wiring (`ProxyInsertsWithTuples`, Link.cpp:243-249,
only prepends a TUPLE to each INSERT's incoming view), and
`ConnectInsertsToSelects` is NOT re-run. Per the A2 (ii) directive the pass
MINTS `d_path`'s MERGE + reader TUPLEs itself (AMENDMENTS F4). The strikethrough
text below is retained only to show what the judge corrected:
~~minting the demand relation's INSERTs (not a pre-built MERGE) is
correct — the pass runs at :2575, and although `ConnectInsertsToSelects`
already ran (C3), the demand relation's INSERTs will be picked up by the
`RemoveUnusedViews`/`ProxyInsertsWithTuples`/`LinkViews` re-runs at
C5:2592-2595 which re-proxy inserts.~~ §5 AMENDMENT A2 flags the ordering
subtlety: `ConnectInsertsToSelects` is NOT re-run after the pass, so the
demand relation's INSERT→SELECT proxying must be done BY THE PASS (mirror
C9's `CreateProxyOfInserts`+`ProxySelects` for `d_rel` and `p`), OR the
pass must be structured to leave INSERTs the C5 passes will resolve. This
is the sharpest Road-G implementation question — pre-registered §6.

### 3.3 The demand-GUARDED COPY (the N-pivot splice)

Reproducing G2's `path` MERGE where each member is `JOIN(pivot F)` of a
`d_path` TUPLE and the original rule body. This is `ApplyPositiveCondition
Test` (C7) generalized 1→N pivots. Post-Connect, `path` = MERGE (C9/C10)
whose `merged_views` are the rule-body proxies (G1). For EACH merged-view
member `body` (the base-rule proxy and the recursive-rule proxy):

1. **Mint the `d_path` reader** (C7:1460-1464 analog): a read of `d_rel`
   producing `[From]`. **POST-JUDGE (F3/F4): NOT `selects.Create(d_rel,
   DisplayRange())` — no `(Relation,DisplayRange)` SELECT ctor exists
   (Query.h:645-647). Two corrected options, per the A2 (ii) directive: (a)
   a fabricated minimal `ParsedPredicate` over the `d_path` local +
   `selects.Create(d_rel, ParsedPredicate(pred))` (F3), or (b) — since the
   pass mints `d_path`'s MERGE itself (F4 (ii)) — a TUPLE proxy over that
   MERGE (the post-`ProxySelects` reader shape, no SELECT at all). The recipe
   uses (b) for the demand side of the guard (a TUPLE over `d_path`'s MERGE),
   reserving the fabricated read-predicate (a) for any place a live relation
   SELECT is unavoidable. See AMENDMENTS F3/F4.**
   **[ROUND-3 SUPERSEDED (N2): route (a) is DELETED — the fabricated
   ParsedPredicate is the Road-C clause/variable chain in disguise. The
   SOLE d_path read construction is (b), the TUPLE over the pass-minted
   MERGE. See AMENDMENTS ROUND 3 N2.]**
2. **Mint the guard JOIN** (C7:1470-1500, N=|bound|=1 here): for a k-pivot
   guard the loop generalizes, but the witness has k=1:
   **[ROUND-3 SUPERSEDED (N3): this step's guard-on-OUTPUT shape (AddUse(body)
   at the rule-body output) is REPLACED by the SIP join-tree PUSH-DOWN — the
   guard joins at the DEMANDED SUBGOAL READ inside the body join tree (JOIN 18
   → TABLE 19 → JOIN 22 in the spike dump). See AMENDMENTS ROUND 3 N3 for the
   construction that supersedes steps 2-3 here.]**
   `JOIN *join = query->joins.Create();`
   `join->joined_views.AddUse(body); join->joined_views.AddUse(dsel);`
   `join->num_pivots = 1u;` (= |bound(α)| in general).
   Per PIVOT column i (here just From): mint the pivot out-col, register it
   in `out_to_in`, and `AddUse` BOTH the body's From-col AND `dsel`'s
   From-col (C7:1483-1489 — this is the E-32 real column edge):
   `COL *pv = join->columns.Create(u64_typeloc, join, mint_id(), 0u);`
   `auto [it, ok] = join->out_to_in.emplace(pv, join);`
   `it->second.AddUse(body->columns[from_idx]); it->second.AddUse(dsel->columns[0]);`
   Per NON-pivot column of `body` (here To): a pass-through out-col
   (C7:1491-1500):
   `COL *oc = join->columns.Create(body_col->var, body_col->type, join,
   mint_id(), col_index++); join->out_to_in.emplace(oc, join).first->
   second.AddUse(body_col);`
3. **Mint the restoring TUPLE** (C7:1503-1510) so the JOIN output re-
   establishes `path`'s canonical `[From, To]` shape (drop nothing —
   demand pivots ARE `p`'s bound cols, p3 §1.2):
   `QueryTupleImpl *proj = query->tuples.Create();` copying the JOIN's
   non-nothing columns in `p`'s column order.
4. **Re-point the `path` MERGE member**: replace the merged-view use of
   `body` with `proj` in `path`'s MERGE `merged_views`
   (`merged_views` is a `UseList<QueryViewImpl>`, C10:65 — swap the use).
   After this, `path`'s member = `proj` = restoring-TUPLE-over-guard-JOIN,
   exactly G2's `path` (TABLE 11) member shape.

### 3.4 SIP over the POST-CONNECT graph (mapping graph shapes → subgoals)

The demand-propagation source (G2's second `d_path` member: `d_path(F) :-
path(F,_)`) requires finding "demanding subgoals" in the graph. Post-
Connect (C9), a subgoal `path(F,M)` in another rule appears as a SELECT on
`path`'s relation (`rel->selects`, before proxying) OR — after C9's
`ProxySelects` — as a proxied read of `path`'s MERGE. The SIP walk:

- ROOT: each bound `#query`'s bound columns seed the demand of the relation
  the query projects from (`reachable_from` projects from `path` → seed
  `d_path` on `From`). The query's projection is a MERGE→TUPLE reading
  `path` (G1); the bound-col is col 0. **POST-JUDGE (F1): the query's own
  projection body is ALSO a demanding read and gets a query-projection guard
  (new §3.7) — the spike gates it against the raw demand-seed receive (the
  4th JOIN). The pre-judge text below and §4 wrongly asserted the projection
  stays unguarded; see AMENDMENTS F1.**
- PROPAGATION: for each rule body of a demanded relation `p`, walk the body
  view-tree; each SELECT-on-`p` (a recursive/mutual subgoal reading `p`)
  whose bound columns are SIP-available projects a `d_p` source. In the
  witness, `path`'s recursive rule body contains a read of `path` (the
  JOIN's `path`-side, G1 member2); its From-col is SIP-available (bound by
  `d_path`), so it projects `[From] → d_path` — G2's second `d_path`
  member. Mint it as a TUPLE projecting that From-col into a second
  `d_path` INSERT (C8), CSE-fused with the root member by Optimize (§3.x /
  G3 — the two `d_path` sources fuse in spike.opt.dot).
- SINK: reaching a NEGATE / AGG view STOPS propagation (no `d_q` minted
  across it — d1 §3(a)); if `p`'s bound cols are ONLY reachable past a
  sink, clean diagnostic (§3.6).

**POST-JUDGE (F6): the walk starts from the MERGE and enumerates USERS, not
`rel->selects`.** `ProxySelects` (Connect.cpp:126-155) clears `rel->selects`
and replaces each reader with a TUPLE proxy over the MERGE, so at :2575 there
are NO `rel->selects` to walk. A demanding read of `p` is a transitive USER
of `p`'s MERGE (a reader TUPLE) that is NOT one of the MERGE's own
`merged_views` producers. Self-joins read the MERGE twice (two distinct
subgoals, two adornments — do not conflate); the query projection is itself a
demanding read (F1/§3.7). See AMENDMENTS F6.

The graph-shape → subgoal map is regular: MERGE members = rule bodies;
a reader-TUPLE over `p`'s MERGE = a subgoal reading `p` (post-Connect; not a
live `rel->selects` SELECT — F6); JOIN pivots = the body's
equi-joins (already built). The SIP reads adornment availability off the
column-id equivalence the body already encodes (a col is "bound" if it
traces to a demand-seeded or message col). This is the ONE piece Road-C
would get for free from the clause AST; here it is a bounded graph walk
over a shallow regular shape.

### 3.5 Intra-pass column-id discipline (the Road-G hazard, discharged)

The pass mints new `QueryColumnImpl`s (receive cols, demand SELECT cols,
JOIN out-cols, TUPLE cols). Three questions:

(a) WHAT IDS does the pass mint? Every `columns.Create` takes an `id`.
CRUCIAL FINDING (verified this session): these ids are NOT globally-unique
fresh counters — they are CLAUSE-LOCAL semantic markers. `BuildPredicate`
uses `VarId(context, var)` (C6:264), sourced from a per-clause
`ClauseContext::vars` var-set (Build.cpp:88/127/139); and the condition-
test pivot col is minted with id `0u` LITERALLY (Build.cpp:1464:
`sel->columns.Create(TypeLoc(kBoolean), sel, 0u, 0u)`). So a column's `id`
is an intra-clause equivalence marker (two cols with the same id are the
same logical variable within that clause's scope), NOT a uniqueness token;
cross-view equivalence is carried by the USE edges (`input_columns`,
`out_to_in`), not by id-sharing across views. The pass has NO ClauseContext
(clauses are gone at :2575). RECIPE: the pass does NOT need globally-fresh
ids — it needs (i) ids that are self-consistent WITHIN each minted view
(matching cols in a JOIN's pivot share an id; distinct cols differ), and
(ii) all cross-view equivalence expressed via USE edges. Follow C7 exactly:
the pivot out-col gets a value (C7:1484 mints one) and `out_to_in.AddUse
(both in-cols)` carries the real equivalence edge (the E-32 point); the
pivot's id need NOT equal either input's id. For the demand SELECT/receive
cols, copy the source col's id where a use edge ties them (C10:60 pattern:
`columns.Create(col->var, col->type, view, col->id, idx)`), or use a
per-view local index — either is safe because `FinalizeColumnIDs` (c)
re-numbers globally. The discipline is: NO id collision WITHIN a view,
equivalence via uses, ids provisional.

(b) HOW does CSE's `Equals()` see them during the later Optimize (C5:2583)?
`Equals()` on a JOIN short-circuits on `num_pivots`/children (Join.cpp:467)
BEFORE column ids/group_ids — so the guard JOIN is structurally distinct
from any non-guarded view regardless of minted ids (E-32/§3(b), G3 confirms
survival). For the `d_path` sources, CSE fuses two sources IFF they project
the same columns from the same input — `Equals()` compares the
`input_columns` USE structure, not raw ids (d1 §3.x). So minted ids do not
create spurious merges or block wanted ones; the USE structure governs.
The two `d_path` members coexist in spike.opt.dot (G3) as members of the one
`d_path` MERGE. **POST-JUDGE (F7): this is MERGE-member dedup, NOT CSE fusing
two producers** — the root-seed source projects `From` off the `d_path_bf`
RECEIVE and the propagation source projects `From` off `path`; DIFFERENT
inputs, so CSE `Equals()` (input_columns USE structure) does NOT match them.
They share the demand frontier by feeding the SAME MERGE. Two propagation
sources from DIFFERENT relations would remain distinct MERGE members (no
fusion). Do not rely on CSE collapsing distinct-input demand sources. See
AMENDMENTS F7.

(c) DOES `FinalizeColumnIDs` (C5:2602) erase any hazard? YES — it runs
AFTER Optimize and re-numbers ALL columns canonically. So any minted id is
provisional; its only job is to be (i) fresh (no accidental structural
alias before Optimize) and (ii) internally consistent (the `out_to_in`/
`input_columns` uses point at the right cols). `FinalizeColumnIDs` gives
the final numbering. The hazard reduces to: mint FRESH ids, express all
equivalence via USES not ids. Discharged.

### 3.6 The five behaviors the recipe must specify

- MULTI-RULE predicates: `path` has 2 rules → 2 MERGE members → the §3.3
  splice runs PER MEMBER (loop over `merged_views`). Witness exercises this
  (base rule + recursive rule both guarded, G2).
- THE RECURSIVE RULE: the guard on `path`'s recursive body is the SAME
  §3.3 splice; the guard lands INSIDE the fixpoint (the round-2 judge's
  `%table:19 = d_path ⋈ path` finding — the guard prunes `path`'s own
  materialization upstream of the fixpoint, not just the answer). The
  recursive `path`-read subgoal ALSO seeds `d_path` (§3.4 propagation) —
  the recursion's demand is self-sustaining, grounded in the root seed.
- DEMANDING-SUBGOAL PROJECTION SOURCES: §3.4 — each SELECT-on-`p` with
  SIP-available bound cols projects a `d_p` source TUPLE→INSERT (C8),
  fused by Optimize (§3.x/G3).
- THE ALL-FREE INERTNESS SKIP: if |bound(α)|=0, mint NOTHING for that
  adornment (no message, no `d_p`, no guard) — a zero-pivot demand JOIN is
  illegal outside @product (d1 §2.3/§3(e)). The witness's `bf` is |bound|=1
  so it is not skipped; the tc.dr `ff` sibling would be (paper-only, §5-G4).
- THE (f) TRIPWIRE: after minting, walk every minted `d_p` and assert it
  has ≥1 producing INSERT reachable from a `#query` seam (a cheap graph
  walk); else `fprintf`+abort (always-on under the flag, d1 §3(f)). Emit
  the root receive/INSERT BEFORE any guard so `d_p` is never transiently
  source-less (dead-flow at C5:2592 would else collapse `p'`).

--------------------------------------------------------------------------
## 4. GRAPH-DIFF EVIDENCE (the reproduction argument)

The recipe reproduces G2 (spike.nodf.dot) from G1 (base.nodf.dot). The
NODE DELTA, and the §3 step that mints each:

| G2 node (spike.nodf.dot) | Base? | Minted by |
|--------------------------|-------|-----------|
| `I/O d_path_bf` | NEW | §3.2 step 2 (`ios.Create`) |
| `RECEIVE[From]` | NEW | §3.2 step 3 (`selects.Create(io, …)` + `receives.AddUse`) |
| `d_path` MERGE (TABLE) | NEW | C10 re-proxy of §3.2/§3.4 INSERTs (§5-A2) |
| `d_path` root member (TUPLE←receive) | NEW | §3.2 step 4 |
| `d_path` propagation member (TUPLE←path proj) | NEW | §3.4 |
| demand INSERT(s) into `d_path` | NEW | §3.2 step 4, §3.4 (C8) |
| `path` MERGE member = JOIN(pivot F) | CHANGED | §3.3 (was a bare body proxy in G1; now guarded) |
| `d_path` SELECT (per member) | NEW | §3.3 step 1 |
| guard JOIN (per member) | NEW | §3.3 step 2 (C7 generalized) |
| restoring TUPLE (per member) | NEW | §3.3 step 3 |
| `path` MERGE, edge_2, reachable_from | UNCHANGED | — (base graph; only `path`'s members re-pointed) |

The base rule bodies' INTERIORS are untouched (§3.3 splices ABOVE them):
G2's `path` members still bottom at the same edge_2/path reads as G1. The
`reachable_from` projection is byte-identical base↔spike (both dumps: a
UNION→TUPLE reading `path`). This is the reproduction claim: the recipe
adds exactly the NEW rows above and re-points `path`'s MERGE members;
everything else is the base graph. spike.opt.dot (G3) confirms the minted
nodes survive Optimize with the intended CSE fusion (two `d_path` sources
→ one MERGE) and the two guard JOINs intact — so the recipe's output is
Optimize-stable, matching the spike the transform is meant to synthesize.

CROSS-CHECK against the LANDED spike compile (d1 §A6): the spike's `.dr`
lowers (via `BuildClause`) to G2; the recipe mints G2 directly. Same graph,
two construction paths — the spike is Road-G's oracle for "did I build the
right nodes."

--------------------------------------------------------------------------
## 5. AMENDMENTS TO THE d1 ARTIFACT / JUDGED ARGUMENT

Two are LOUD (a mechanism nuance the owner should ratify); two are
clarifications folding into D4.

### **[LOUD — A1] The demand RELATION `d_path` needs a decl, and it is NOT the message decl.**

d1 §1.0 names three objects: the demand relation `d_p`, the guarded copy
`p'`, the root seed. The landed `FabricateDemandMessage` (C13) fabricates
the MESSAGE (object 3's source). But the demand RELATION `d_p` (object 1)
is a distinct `QueryImpl` relation (G2 shows `d_path` as a `#local`-style
MERGE relation, SEPARATE from the `I/O d_path_bf`). `BuildPredicate`'s
relation branch (C6:239-244) keys `decl_to_relation` on a `ParsedDeclara
tion` that IsExport/IsLocal/IsQuery. So the pass needs a `#local`-kind decl
for `d_path` — a SECOND fabrication the landed code does not provide
(`FabricateDemandMessage` mints only the message). In the SPIKE this is the
hand-written `#local d_path(u64 From)`. **RATIFICATION NEEDED**: either (a)
`FabricateDemandMessage` grows a sibling `FabricateDemandLocal` (same
CreateDerived<ParsedLocalImpl> idiom, C16:111), or (b) the pass mints the
`QueryRelationImpl` WITHOUT a ParsedDeclaration (relations.Create currently
requires one, C6:243 — would need a decl-less ctor, larger surface). The
d1 artifact treats `d_p` as "an ordinary relation" (§1.1) without naming
this decl obligation. This is a real fabrication the owner should ratify
as in-scope (recommended: option (a), the ParsedLocalImpl sibling — it
mirrors the message fabrication and Aggregate.cpp's own anon-local move).

> **POST-JUDGE (F5, F3): A1 grows.** Option (a) is NOT a bare
> `CreateDerived<ParsedLocalImpl>` — the demand table is NAMED at codegen
> from `Sanitize(ToString(decl->Name()))` (Database.cpp:466-471), so a
> text-less synthetic-atom name (the Aggregate.cpp:120 idiom, which relies
> on a parse-time `tok_range` the pass lacks) yields an unnamed/COLLIDING
> table. A1 becomes a `FabricateDemandLocal` sibling reusing the SAME
> display-buffer OpenBuffer/lex interning route as `FabricateDemandMessage`
> (Parse/Demand.cpp:59-89), with the same G3 collision scan. This is a HARD
> prerequisite of S3a, not polish. A1 ALSO absorbs F3's read-predicate: every
> DERIVED-`d_path`-relation read (§3.3 guard side, §3.4 propagation, §3.7
> query guard) needs a fabricated minimal `ParsedPredicate` over the local
> (no `(Relation,DisplayRange)` SELECT ctor exists — Query.h:645-647); the
> raw-seed/receive reads stay pred-less. See AMENDMENTS F5/F3.
> **[ROUND-3 SUPERSEDED (N2): the read-predicate sentence above is RETRACTED —
> route (a) is deleted; every derived-d_path read is the TUPLE over the
> pass-minted MERGE (F4 (ii)). FabricateDemandLocal (the decl fabrication)
> stands. See AMENDMENTS ROUND 3 N2.]**

### **[LOUD — A2] `ConnectInsertsToSelects` does NOT re-run after the pass — the demand relation's INSERT→SELECT proxying must be done by the pass.**

d1 §1.1 says `d_p` "lowers as an ordinary table … DERIVED by TrackDiffer
entialUpdates on the rewritten graph." But the pass runs at :2575 AFTER
`ConnectInsertsToSelects` (C3:2563) — which is the pass that turns
`rel->inserts` into a MERGE proxy read by proxied SELECTs (C9). It is NOT
re-run (C5:2582-2607 has no Connect). So a demand relation whose sources
the pass adds as raw INSERTs will NOT get the MERGE/proxy wiring unless the
pass does it. TWO resolutions: (i) the pass calls the C9 primitives
(`CreateProxyOfInserts`/`ProxySelects`) on `d_rel` and re-proxies `p`'s
guarded members itself; OR (ii) the pass mints the MERGE + proxy structure
directly (matching G2). The spike sidesteps this — its `d_path` clauses go
through Connect NORMALLY (they exist before :2563). The transform does not
have that luxury. **This is the sharpest Road-G implementation question**
and the most likely first-divergence site (§6). The d1 artifact's "ordinary
relation, derived downstream" glosses it. Recommend (i): reuse the C9/C10
Connect primitives on the demand relation — least new code, exercises the
ratified proxy path.

> **POST-JUDGE (F4): A2 RESOLVES TO (ii); (i) is DELETED.** Re-verified:
> `CreateProxyOfInserts`/`ProxySelects` are anon-namespace statics in
> Connect.cpp (:8, :10, :126) — UNREACHABLE from Demand.cpp. Option (i)
> would require hoisting them to `QueryImpl` methods (unbudgeted refactor of
> a ratified pass) or duplicating them; it is neither "least new code" nor
> "exercises the ratified path" absent the hoist. And `ConnectInsertsTo
> Selects` (the only exported one) is NOT re-run after :2575; NONE of the C5
> passes do INSERT->SELECT relation wiring (`ProxyInsertsWithTuples`,
> Link.cpp:243-249, only prepends a TUPLE to each INSERT's incoming view).
> **The DIRECTIVE is (ii): the pass mints the `d_path` MERGE + reader-TUPLE
> plumbing DIRECTLY**, mirroring `ConnectInsertsToSelects`:259-283 — a
> `merges.Create()` over the demand-source TUPLE proxies, and every reader of
> `d_path` (guard demand side, propagation, query guard) is a TUPLE proxy
> over that MERGE. The §3.2 "Note" claim that the C5 re-runs re-proxy the
> demand inserts is DELETED (it misdescribes those passes). See AMENDMENTS F4.

### [CLARIFICATION — A3] The pass slot is :2575, not :2566.

d1/p3 cite ":2566". The landed tree has the pass at Build.cpp:2575 (C4),
the ApplyDemandTransform call; :2566 is the doc-comment head. The p3 §1.3
placement argument (after Connect, before Optimize/Stratify) holds
byte-identically at :2575. Non-substantive; recorded so D4 does not hunt
for :2566.

### [CLARIFICATION — A4] `reachable_from` reads its own projection, not `path` directly.

Both dumps confirm (and d1 §A6 already noted): `reachable_from` is a
SEPARATE relation projecting from `path`; the query reads
`reachable_from`'s table, filled by a projection off `path`. The recipe's
§3.3 guard lands on `path`'s MERGE members (upstream of the projection), so
`reachable_from`'s projection is untouched — the guard prunes `path`, the
projection inherits the pruning. Matches d1 §A6's SPIKE-DIVERGENT note and
the round-2 `%table:19` finding. No action; recorded for D4 clarity.

> **POST-JUDGE (F1): A4 is SUPERSEDED — `reachable_from`'s projection is NOT
> untouched.** Re-verified against the dumps: `spike.nodf.dot` and
> `spike.opt.dot` both carry a FOURTH JOIN (`v4395412864` / `v4390433168`,
> pivot `From`, output `[From,To]`) that gates `reachable_from`'s projection
> against the RAW demand-seed receive (TABLE 23 = `d_path_bf`), joined with
> guarded `path` (TABLE 11). Base `reachable_from` has NO such JOIN. The
> claim "the projection inherits the pruning" is answer-true for the witness
> (the bf recursion preserves `From`, so the raw-seed guard and a
> `d_path`-guard coincide — see the F1 coincidence note) but STRUCTURALLY
> false: the spike emits an explicit query-projection guard the recipe must
> MINT (new §3.7) to reproduce G2. A4's "no action" is wrong; the action is a
> fifth splice family. The guard is answer-redundant here but LOAD-BEARING in
> the multi-adornment / sideways-passing future. See AMENDMENTS F1.

> **POST-JUDGE (F2): the INJECTOR generalization is a HIGH-cost, UNSPIKED
> ControlFlow obligation — add it to the honest-cost ledger beside A1/A2.**
> Road-G mints no `ParsedPredicateImpl`, so `query.ForcingMessage()` stays
> nullopt and `BuildQueryForceProcedure` (Build.cpp:373-381) builds NO
> injector — a demand-transformed program compiles but returns EMPTY. §7's
> "already spiked" label is WRONG: §A6 confirms only the entry-point
> suppression, not the injector generalization (the spike hand-wrote a real
> `@first` forcing pred). S4 must (1) add a demand REGISTRY (query ->
> fabricated message + bound-param binding), (2) reroute
> `BuildQueryForceProcedure` to consult it, (3) add a
> `BuildQueryForceProcedureFromRegistry` sibling reusing :317-367 verbatim
> and REPLACING the clause-var DisjointSet re-derivation (:250-311) with the
> registry binding. This is d1 §1.3 Option D step 5, ratified but unbuilt.
> Consequence: S3 alone cannot be answer-validated (no seeder); S3+S4 land
> TOGETHER. See AMENDMENTS F2 and the §7 re-staged plan.

Everything else in d1 (a)-(f) + §A1-§A7 stands: Road-G is the ratified
mechanism's literal realization, so the CSE-fusion (§3.x/G3), structural-
distinctness (§3(b)/G3), stratification (§3(a)), and pipeline-placement
(p3 §1.3) arguments apply UNCHANGED. A1/A2 are fabrication/wiring
obligations the graph-level recipe surfaces that the abstract d1 argument
did not enumerate — flagged LOUD for owner ratification, not a change to
the invariant argument.

--------------------------------------------------------------------------
## 6. PRE-REGISTERED PREDICTIONS (S3-S6)

Per the E-1..E-45 house precedent (every first-time instrument finds one
real divergence), the first end-to-end demand rewrite of the witness will
surface a divergence. Ranked bets:

1. **[PRIMARY] A2 — the demand relation's INSERT→SELECT proxying.** The
   pass runs after `ConnectInsertsToSelects`; a demand relation added as
   raw INSERTs will be MERGE/proxy-less unless the pass calls the C9/C10
   primitives. Predicted first failure: `d_path` source-less at
   `RemoveUnusedViews` (C5:2592) → dead-flow collapses `p'` → under-
   derivation (empty `reachable_from`). Instrument: the (f) tripwire
   (§3.6) + the oracle answer-identity gate.
2. A1 — the `d_path` local-decl fabrication. If minted wrong (e.g. reusing
   the message decl), `decl_to_relation`/`BuildPredicate`-relation-branch
   assumptions break; predicted as a compile-time assert (IsLocal/IsQuery),
   not a silent miscompile.
3. Column-id discipline (§3.5) — an id inconsistency WITHIN a minted view
   (not a global collision — ids are clause-local markers, §3.5a) causing a
   wrong equivalence. LOW: `Equals()` reads USE structure, not ids (§3.5b),
   and `FinalizeColumnIDs` re-numbers globally; predicted benign so long as
   equivalence rides use edges (C7).
4. SIP graph-walk (§3.4) — mis-identifying a demanding subgoal (e.g.
   missing the recursive `path`-read as a `d_path` source). Predicted as a
   wrong (but non-empty) answer → oracle disagreement.

> **POST-JUDGE (F1/F2): bets re-ranked.** Bet 1 (A2 proxy wiring) is now a
> CODE directive, not a bet (F4 resolution (ii)). The NEW primary
> first-divergence risk is F1: the recipe reproduces G2 ONLY IF §3.7 mints
> the query-projection guard — if S3 skips it, the S5 dump is structurally
> SIMPLER than the spike (one fewer JOIN, no TABLE 23) and the §7 "dump
> matches G2" gate FAILS against the oracle. And the runtime-empty failure §6
> bet 1 predicts is caused (before any A2 issue) by the MISSING INJECTOR (F2),
> which S3 cannot fix — hence S3+S4 land together (below).

**S5 WITNESS GOLDENS — predictable from the spike?** YES — but ONLY if S3
mints the F1-faithful graph (all 4 JOINs, including the §3.7 query-projection
guard and TABLE 23). The spike (d1 §A6) emitted the correct `.ir`/`.h` for the
bf witness in all 4 modes on the frozen binary. The S5 `demand_tc_witness.dr`
+ `-demand` should emit the SAME graph the spike's hand-written `.dr` emits
(§4 reproduction claim, AS CORRECTED by AMENDMENT F1) — so the S5 stdout
golden should match a run of the spike's driver, and the oracle/monotone
goldens are predictable from the spike's answer set. THIS PREDICTION IS NOW
CONDITIONED ON §3.7: without the query-projection guard the S5 emission
diverges STRUCTURALLY from the spike (fewer nodes) and the goldens-from-spike
rename shortcut is invalid. The ONE unpredictable delta: the fabricated
message's
NAME (`__demand_reachable_from_bf` vs the spike's hand-chosen `d_path_bf`)
changes the emitted proc/table names — so the goldens differ in
identifiers but not in structure. PREDICTION: S5 goldens = the spike's
emission with `d_path_bf`→`__demand_reachable_from_bf` renaming +
the F2 public-entry suppression (the spike leaks the entry, S3-S4 suppress
it — d1 §A2). If the S5 emission diverges STRUCTURALLY from the spike, the
first suspect is a MISSING §3.7 query-projection guard (F1 — the 4th JOIN /
TABLE 23), then the A1/A2 wiring; the goldens-from-spike rename shortcut is
valid ONLY when the F1-faithful graph is minted.

**Suite trajectory**: 166 → 167 (primary witness, oracle-blessed). Q5
neutral (no bound query). Existing 166 byte-identical (flag-off, the pass
returns at the mode gate). A FINDINGS entry iff the rewrite produces a
wrong answer or a source-less `d_p` (bets 1/4).

--------------------------------------------------------------------------
## 7. IMPLEMENTATION ORDER FOR S3-S6 (recipe → code)

> **POST-JUDGE RE-STAGE (F1/F2).** The order below is SUPERSEDED by the
> re-staged plan: S3a-S3d are still incremental graph-diff gates, but the S5
> ANSWER gate can only run after S4 (the injector) lands, so S3+S4 are ONE
> gated diff (structure correct AND runtime seeding present) BEFORE S5. The
> "already spiked" note on old step 4 is DELETED (F2: injector generalization
> is unspiked). §3.7 (the query-projection guard) is added as S3d.

1. S3a: mint the demand relation decl+read-pred via `FabricateDemandLocal`
   (A1, F5/F3 — the display-buffer interning route, NOT a bare
   `CreateDerived`) + message (landed) + the `d_path` root source (§3.2) +
   the A2 (ii) MERGE+reader plumbing minted by the pass (F4). Gate: `-demand`
   on the witness compiles to a graph with `d_path` a MERGE sourced by the
   receive-projection TUPLE (dump + diff vs G2's `d_path` root member); the
   demand table is NAMED (no collision).
2. S3b: the guard splice (§3.3) on `path`'s MERGE members. Gate: dump matches
   G2's `path` members (TABLE 11); the (f) tripwire passes.
3. S3c: the SIP propagation source (§3.4, the MERGE-user walk, F6) + multi-
   rule loop + inertness skip + negation-sink diagnostic (§3.6). Gate:
   `path`/`d_path` node set == spike (3 of the 4 JOINs).
4. S3d: the QUERY-PROJECTION GUARD (§3.7, F1 — the 4th JOIN pivoting
   `reachable_from`'s projection against the raw demand-seed receive). Gate:
   4-mode STRUCTURAL dump == spike (all 4 JOINs, TABLE 23 present).
5. S4 (lands WITH S3, before the S5 answer gate): the injector generalization
   (F2 — demand REGISTRY + `BuildQueryForceProcedureFromRegistry`, reusing
   Build.cpp:317-367, replacing the clause-var re-derivation :250-311) + the
   F2 public-entry suppression (rides the fabricated message's IsReceived/
   NOT-IsPublished gate; the `_detail`-call suppression IS spike-confirmed,
   d1 §A6, but the injector GENERALIZATION is not). Gate: the witness's
   `reachable_from_bf` query, called on a seed, returns the demanded rows
   (non-empty) — the first ANSWER gate.
6. S5 (the CORPUS/ANSWER gate, after S3+S4): `demand_tc_witness.dr` + driver
   + `.batches` + oracle/monotone goldens (predicted §6, CONDITIONED on the
   F1-faithful graph — all 4 JOINs). Gate: oracle answer-identity demand-ON
   vs a definitional full-eval.
7. S6: the COST spike (p3 §3.2) — instances materialized demand-ON vs OFF
   on a sparse graph; the measure-first bar (also the first case exercising
   the F1 LOAD-BEARING query-projection guard if a multi-adornment shape is
   used).

Every stage gates on a dumped-graph diff against G2/G3 (the spike is the
oracle) BEFORE the oracle answer gate — a structural miscompile shows in
the dump before it shows in an answer.

--------------------------------------------------------------------------
## AMENDMENTS (2026-07-17, post-judge)

The adversarial judge (`design/judge-d4s3.md`, verdict REVISE) found nine
issues (F1-F9). Every anchor below was RE-VERIFIED this session against the
dumped graphs and the tree at tip `0f396116`. The Road-G decision (§2) is
NOT overturned — the judge concurs. These amendments fix the reproduction
argument (§4), the injector scope (§7/§6), and three concrete ctor/wiring
errors, all WITHIN the ratified mechanism. Where an amendment changes §3-§6,
the change is stated here and the affected section is annotated below; read
this section as authoritative over the pre-judge body where they conflict.

### F1 [CRITICAL, DISCHARGED] — the recipe MUST also mint the query-projection guard (a 4th JOIN); the spike graph re-derived faithfully.

The judge is CORRECT and I re-verified it node-for-node. Counted JOINs:
`base.nodf.dot` = 1; `spike.nodf.dot` = 4; `spike.opt.dot` = 4 (the 4th
SURVIVES Optimize). §4's "reachable_from ... UNCHANGED" row and AMENDMENT A4
("reachable_from's projection is untouched") are FALSE against the dump.

FAITHFUL TARGET-GRAPH RE-DERIVATION (spike.nodf.dot, traced this session):
the 4th JOIN is `v4395412864` (STRATUM 8, EQ SET 7): pivot `From`, output
`[From,To]`. Its inputs:
  - p0 <- `v38432590912` = **TABLE 23**, a TUPLE projecting `From` off the
    `d_path_bf` RECEIVE (`v38432526720`, STRATUM 1) — i.e. the RAW DEMAND-SEED
    receive, NOT the derived `d_path` relation;
  - p1,p2 <- `v38432588224` = TABLE 11, the guarded `path` TUPLE `[From,To]`.
The JOIN feeds `v4395413888` (TUPLE) -> `v38432588672` (TUPLE) ->
`reachable_from` UNION `v4395417344` -> MATERIALIZE reachable_from (TABLE 4).
So spike's `reachable_from` body is `d_path_bf(From) ⋈ path(From,To)` — the
query answer is itself demand-guarded on the seed receive. In BASE,
`reachable_from` is UNION <- TUPLE `v4386537744` <- TUPLE `v4386546448/000`
reading `path` TABLE 8, NO JOIN.

CAUSE (confirmed in `d1-spike/demanded_tc_spike.dr`): the query clause is
`#query reachable_from(...) @first : @first d_path_bf(From), path(From, To)`
— `d_path_bf(From)` is BOTH the `@first` forcing pred AND a real body
conjunct, so `BuildClause` built the body as `d_path_bf ⋈ path`.

RESOLUTION — the recipe MINTS the query-projection guard. The recipe's oracle
is the SPIKE (§4 last para; the S5 goldens-from-spike prediction in §6 depends
on STRUCTURAL identity with G2/G3), so the recipe must reproduce G2's node
set EXACTLY — including this 4th JOIN. Add a fifth splice family to §3:

  §3.7 QUERY-PROJECTION GUARD. For each bound `#query` whose projection reads
  a demanded relation `p` at adornment α, interpose ONE guard JOIN on the
  query's own projection body, pivoting the query's bound columns against the
  RAW DEMAND-SEED SOURCE for α. Concretely, for the witness: mint a SELECT/
  TUPLE reading the demand-seed source projecting `From`; a JOIN pivoting
  `From` between that source and the `reachable_from`-projection member that
  reads `path`; a restoring TUPLE `[From,To]`; re-point the `reachable_from`
  UNION member (exactly the §3.3 splice shape, applied to the QUERY's
  projection member instead of a `path` rule body). This is the same E-32
  1->N-pivot generalization as §3.3, one extra application site.

  IMPORTANT — which source the guard pivots against. The spike's 4th JOIN
  pivots against TABLE 23 = the `d_path_bf` RECEIVE projection (the raw seed),
  NOT against `d_path` (the derived relation). To be byte-structurally
  identical to the spike, §3.7 must pivot the query projection against the
  ROOT-SEED source (the receive's `From` projection), the same node §3.2
  step 4 already mints as the `d_path` MERGE's root member. (Reuse that TUPLE
  as the JOIN's demand side; do not mint a fresh `d_path` SELECT here.)

SEMANTIC NOTE (the coincidence, and when it DIVERGES) — worked honestly.
For bf transitive closure the raw-seed guard (`d_path_bf(From) ⋈ path`) and a
`d_path`-guard (`d_path(From) ⋈ path`) COINCIDE in the answer set:
`d_path` = the demand frontier = the transitive closure of the seed under the
recursion, which for the bf adornment PRESERVES `From` (every recursive rule
`path(F,T) :- d_path(F), path(F,M), edge_2(M,T)` re-keys on the SAME `F` it
was demanded at). So `path`'s materialized rows already have `From ∈ seed`,
and pivoting on the raw seed vs the derived `d_path` yields the same
`[From,To]` set. The two guards are answer-equivalent HERE — which is why F1
option (ii) ("redundant here") is semantically true. But they are NOT
structurally identical, and the spike emits the raw-seed form, so the recipe
mints the raw-seed form to match the oracle.

  WHEN THEY DIVERGE (flagged for the multi-adornment / sideways-information-
  passing future, out of S3-witness scope): if `p` is demanded at MULTIPLE
  adornments, or the recursion does NOT preserve the bound key (a `p(F,T) :-
  p(M,T), edge(F,M)` shape where the bound column flows SIDEWAYS to a
  different position), then `path`'s rows can carry a `From` that is reachable
  but was never the QUERY's demanded key — and the raw-seed guard on the
  query projection PRUNES those (only answers for demand the query issued),
  whereas reading `d_path`-guarded `path` alone would over-report. There the
  query-projection guard is LOAD-BEARING, not redundant, and pivoting against
  the raw seed (the query's own issued demand) vs the union-`d_path` (all
  demand from all subgoals) gives DIFFERENT answers. The witness cannot
  exercise this (single adornment, From-preserving recursion). §3.7 mints the
  guard unconditionally so the mechanism is correct in the divergent case and
  structurally faithful in the coincident one; the S6 COST spike and any
  multi-adornment corpus case (d1 §2's fb/ff siblings) will exercise the
  load-bearing path.

§4 CORRECTION: the last table row changes from "reachable_from ... UNCHANGED"
to: `reachable_from` projection member = JOIN(pivot From) of {root-seed
source} and {the base `path`-reading projection} | CHANGED | §3.7. And the
prose "The `reachable_from` projection is byte-identical base<->spike" is
DELETED — it is false (base: no JOIN; spike: a From-pivot JOIN + TABLE 23).
A4 is SUPERSEDED (see the §5 update below).

### F2 [HIGH, DISCHARGED] — the injector wiring specified; S3+S4 must land TOGETHER; ForcingMessage stays nullopt.

The judge is correct: S3's output graph has NO runtime seeder. Road-G mints
only DataFlow nodes; it fabricates NO `ParsedPredicateImpl` into the query
clause's `forcing_predicates`, so after the pass `query.ForcingMessage()`
returns nullopt (Parse.cpp:1142 reads `forcing_predicates`, populated only at
parse time), and `BuildQueryForceProcedure` (CF Build.cpp:373-381, gated on
`ForcingMessage()` at :375) builds NO injector. The `d_path` relation is
graph-reachable (passes the §3.6 (f) tripwire, which checks GRAPH
reachability) but at RUNTIME nothing calls the `d_path_bf` handler, so
`d_path` is empty, `path'` produces nothing, `reachable_from` is empty. The
§7-step-4 / §6 label "already spiked (d1 §A6 confirms the injector calls
`_detail`)" is WRONG: §A6 confirms the ENTRY-POINT SUPPRESSION detail, not
the injector GENERALIZATION. The spike sidestepped the generalization by
hand-writing `@first d_path_bf(From)` — a real parsed forcing pred. This is
real, UNSPIKED ControlFlow work and belongs in the honest-cost ledger next
to A1/A2. It is the S4 obligation ratified by d1 §1.3 Option D step 5
(d1 md:297-301: "GENERALIZE `BuildQueryForceProcedureImpl` … so the handler
is `messsage_handler[d_p^α]`").

THE S4 GENERALIZATION, spelled concretely (the diff shape against
Build.cpp:373-412 and :246-368):

  (1) A DEMAND REGISTRY. The demand pass (DataFlow) records, per bound
  `#query` it transforms, a tuple `{ParsedQuery query, ParsedMessage
  fabricated_msg, vector<pair<bound-query-param-index, msg-param-index>>
  binding}` — the correspondence `BuildQueryForceProcedureImpl` currently
  RE-DERIVES off the clause via DisjointSet var-union (:263-311). Because
  Road-G never fabricates a clause, that var-union cannot run; the pass knows
  the correspondence directly (it built the guard JOINs from the query's
  bound cols), so it STORES it. This is the "F2-B(ii) registry" the
  foundation planned; the natural home is a member on `QueryImpl` (or
  `Context`, threaded to CF Build) keyed by `ParsedQuery`. (The parse-level
  `module->demand_fabricated` flag, Parse.h:582, only records THAT
  fabrication happened, not the per-query mapping — the registry is a
  distinct object.)

  (2) `BuildQueryForceProcedure` (:373-381) — CHANGED. Currently:
  `if (auto pred = query.ForcingMessage()) return
  BuildQueryForceProcedureImpl(impl, context, query,
  ParsedClause::Containing(*pred), *pred);` NEW: consult the registry FIRST —
  `if (auto *entry = context.demand_registry.Find(query)) return
  BuildQueryForceProcedureFromRegistry(impl, context, query, *entry);` then
  fall through to the existing `ForcingMessage()` path (the user `@first`
  surface stays live for hand-written forcing queries). `ForcingMessage()`
  stays nullopt for a demand-transformed query — NO parse-level forcing
  predicate exists or is needed.

  (3) `BuildQueryForceProcedureFromRegistry` — NEW, a thin sibling of
  `BuildQueryForceProcedureImpl` (:246-368). REUSED VERBATIM from :317-367:
  the proc creation (`kQueryMessageInjector`), the per-bound-param
  `input_vars.Create`, the `add_vec`/`del_vec` vectors, the VECTORAPPEND, the
  `CALL context.messsage_handler[message]` (:356 — the handler is
  `messsage_handler[fabricated_msg]`, populated by `BuildIOProcedure`/A5 in
  the A6 IO loop because the fabricated message minted a real `QueryIO` with a
  non-empty receive — d1 A5/A6, NO new CF plumbing for the handler edge), the
  RETURN. REPLACED (:250-311): the entire clause-var DisjointSet block and the
  `message_param_to_query_param` re-derivation — the registry entry's `binding`
  vector supplies `bound_query_params` and the message-param order directly
  (iterate the registry's `binding` instead of `forcing_pred.Arguments()` ×
  `bound_query_params`). No `ParsedClause`, no `forcing_pred` argument.

  (4) `BuildQueryEntryPointImpl` (:384-412) — UNCHANGED. It already calls
  `BuildQueryForceProcedure(impl, context, query)` (:402) and packs the
  resulting `forcer_proc` into `impl->queries` (:411); with (2) rerouted, a
  demand-transformed query now gets its registry-built forcer transparently.
  The F2 public-entry SUPPRESSION (the demand message must NOT leak a public
  entry point — d1 §A2) rides the existing IsPublished/IsReceived gate the
  fabricated message already sets (spike comment: `d_path_bf` is IsReceived,
  NOT IsPublished), so `BuildIOProcedure` skips its log hook and no public
  message entry is emitted; the injector calls the `_detail` receive proc
  (d1 §A6, A14) — that part IS confirmed by the spike.

S3-ALONE TESTING CONSEQUENCE (stated honestly). Until S4 lands, a
demand-transformed program COMPILES (the graph is well-formed, the (f)
tripwire passes) but its query returns EMPTY — there is no runtime seeder, so
`d_path` never fills. Therefore S3 and S4 CANNOT be validated independently
by an answer gate; only a graph-diff gate applies to S3 alone. RE-STAGE
(supersedes §7 order): S3+S4 land as ONE gated diff (graph correct AND a
runtime injector seeds `d_path`); the S5 corpus/oracle answer gate runs only
AFTER both; S6 is the COST spike. See the re-staged plan in the §7 update
below.

### F3 [HIGH, DISCHARGED] — §3.3 step-1 SELECT ctor: use a fabricated ParsedPredicate over the `d_path` local (no new ctor).

**[ROUND-3 SUPERSEDED (N2): the DECISION below is RETRACTED — the fabricated
ParsedPredicate requires a ParsedClauseImpl + ParsedVariableImpls (the Road-C
chain). The ctor-nonexistence analysis stands; the resolution is the F4 (ii)
TUPLE-over-the-pass-minted-MERGE route ONLY. See AMENDMENTS ROUND 3 N2.]**

The judge is correct: Query.h:645-647 has exactly three SELECT ctors —
`(QueryRelationImpl*, ParsedPredicate)`, `(QueryStreamImpl*, ParsedPredicate)`,
`(QueryStreamImpl*, DisplayRange)`. There is NO `(QueryRelationImpl*,
DisplayRange)`. The pred-less ctor (C17's KEY SIMPLIFICATION) is STREAM-only —
it works for the demand-MESSAGE receive (§3.2 step 3, a QueryIO/stream) but
NOT for the `d_path` RELATION read (§3.3 step 1). So §3.3 step 1 as written
(`query->selects.Create(d_rel, DisplayRange())`) does not compile.

DECISION — fabricate a minimal `ParsedPredicate` over the `d_path` local,
reusing the C5 relation-read pipeline precedent; do NOT add a new decl-less
relation-SELECT ctor. Rationale: every relation SELECT in the tree carries a
ParsedPredicate (`BuildPredicate` at Build.cpp:246, `ApplyPositiveCondition
Test` at :1460 both call `selects.Create(rel, pred)`); adding a fourth ctor
is unbudgeted surface that would need its own downstream audit (every SELECT
consumer that reads `select->pred`). The cheaper, precedented path: the
`d_path` local decl (A1/F5) supplies a `ParsedDeclaration`; mint a minimal
`ParsedPredicateImpl` over it (positive, argument_uses = the demand pivot
var(s)) via the same `CreateDerived` idiom the local decl uses, and call
`selects.Create(d_rel, ParsedPredicate(pred))`.

  HONEST COST — this re-introduces a THIN slice of predicate fabrication that
  C17 claimed the message path eliminated. C17's claim stands for the MESSAGE
  receive (stream, genuinely pred-less); it does NOT extend to the `d_path`
  RELATION reads (§3.3 guard side, §3.4 propagation reads, §3.7 query guard —
  every read of the derived `d_path` relation). This is a real fabrication
  beyond "the landed message only," priced here and folded into the §5
  ledger (A1 grows to cover BOTH the local decl AND its read-predicate). It
  is still far short of Road-C's full clause/variable/group chain: one
  positive predicate per demand-relation read, no clause, no variable-group
  machinery, no BuildClause re-run.

  NOTE — the RAW-SEED reads (the receive projection, §3.2 step 4 and §3.7's
  pivot source) stay pred-less: they read the STREAM/receive, so the C17
  stream ctor still applies there. Only the DERIVED-`d_path`-relation reads
  need the fabricated predicate.

### F4 [MEDIUM, DISCHARGED] — adopt resolution (ii): the pass mints the `d_path` MERGE + reader plumbing directly.

The judge is correct on the facts, re-verified: `CreateProxyOfInserts`
(Connect.cpp:10) and `ProxySelects` (:126) are `static` in Connect.cpp's
ANONYMOUS namespace (:8) — not in Query.h, unreachable from Demand.cpp. Only
`ConnectInsertsToSelects` (:161) is exported, and it is NOT re-run after the
pass (Build.cpp:2582-2607 has no Connect). `ProxyInsertsWithTuples`
(Link.cpp:243-249) only prepends a TUPLE to each INSERT's incoming view — it
does NOT connect a relation's SELECTs to its INSERTs and does NOT build the
MERGE-of-proxies that makes a relation readable. NONE of the C5 passes do
INSERT->SELECT relation wiring; that is EXCLUSIVELY `ConnectInsertsToSelects`
:259-283.

RESOLUTION (ii) is the DIRECTIVE (A2 rewritten below). Option (i) (call the
C9 primitives) is DELETED — it requires hoisting anon-namespace statics to
`QueryImpl` methods (an unbudgeted refactor of a ratified pass) or
duplicating them. The §3.2 Note claim that the demand relation's INSERTs "will
be picked up by the RemoveUnusedViews/ProxyInsertsWithTuples/LinkViews
re-runs at C5:2592-2595 which re-proxy inserts" is DELETED — it misdescribes
those passes (F4 above).

THE MINTED SHAPE (against G1/G2). Post-Connect, a normal relation is
`rel = MERGE(rule-body TUPLE proxies)`, read by TUPLE proxies that replaced
the reader SELECTs (C10/`ProxySelects`; base `path` MERGE = TABLE 8, its
`merged_views` are the two rule-body proxies; readers of `path` are TUPLEs
over that MERGE). The pass reproduces this for `d_path` DIRECTLY, mirroring
what `ConnectInsertsToSelects`:259-283 would have done:
  - mint a MERGE `merges.Create()` for `d_path` (the `d_path` MERGE, TABLE in
    G2); its `merged_views` = the demand-source TUPLE proxies (the root-seed
    proxy from §3.2 step 4, the propagation proxy from §3.4), each a
    `tuples.Create()` copying its source's columns (C10 pattern);
  - for every reader of `d_path` (the §3.3 guard-side SELECTs, §3.4/§3.7),
    read the `d_path` MERGE via a TUPLE proxy, NOT a live relation SELECT —
    i.e. the guard's demand side is a TUPLE over the `d_path` MERGE, matching
    G2's guarded-member shape (a TUPLE feeding the guard JOIN's demand port).
  - do NOT leave raw `d_rel->inserts` for a non-existent later Connect;
    `d_rel->inserts` may still be populated for `TrackDifferentialUpdates`
    (which DOES re-run at C5:2607) to derive the differential model, but the
    READABLE structure (MERGE + reader TUPLEs) is minted by the pass.

This makes §3.2's "Note" and its A2 obsolete; both are rewritten. It is also
why F3's fabricated read-predicate matters only for the DERIVED reads — the
pass controls exactly which node reads `d_path` and can proxy it.

### F5 [MEDIUM, DISCHARGED] — A1 carries a naming obligation: `FabricateDemandLocal` via the display-buffer route.

The judge is correct, re-verified. The demand relation's TABLE is NAMED at
codegen from `Sanitize(ToString(decl->Name()))` (Database.cpp:466-471, via
`TableDecl`). A `CreateDerived<ParsedLocalImpl>` whose name is a
`Token::Synthetic(kIdentifierUnnamedAtom, DisplayRange())` (the Aggregate.cpp
:120 idiom) is TEXT-LESS — Aggregate.cpp gets away with it ONLY because it has
a live parse-time `tok_range` backing the synthetic token; the demand pass has
no such range. A text-less name prints nothing -> the demand table is unnamed
and its emitted proc/member name COLLIDES (exactly the failure the landed
`FabricateDemandMessage` had to solve via OpenBuffer/lex — Parse/Demand.cpp
:59-89; d4-impl-log A7/G1).

RESOLUTION — A1 option (a) becomes a `FabricateDemandLocal` sibling to
`FabricateDemandMessage`, NOT a bare `CreateDerived`. It reuses the SAME
display-buffer INTERNING route: `OpenBuffer(name, config)` the synthetic
`__demand_..._local` name, lex it to a real `kIdentifierAtom` whose
`SpellingRange()` resolves through the display manager codegen reads, then
`CreateDerived<ParsedLocalImpl>` + `locals.AddUse` with THAT interned name
token. Same G3 collision scan applies (a user `#local` colliding with the
`__demand_` prefix -> nullopt -> clean diagnostic). The TABLE-NAME COLLISION
HAZARD is named explicitly: without the interned name, the first S3a compile
emits an unnamed/colliding demand table (a silent codegen collision, not a
clean diagnostic) — so `FabricateDemandLocal` is a HARD prerequisite of S3a,
not a polish. (See the §5 A1 update.)

### F6 [LOW, DISCHARGED] — the SIP walk (§3.4) must start from the `d_path`/`p` MERGE and walk USERS, not `rel->selects`.

The judge is correct, re-verified: `ProxySelects` (Connect.cpp:126-155) swaps
`rel->selects` out (`old_selects.Swap(selects)`, :128-129 — `rel->selects`
ends EMPTY) and replaces each reader SELECT with a TUPLE proxy over the MERGE
(`select->ReplaceAllUsesWith(proxy)`, :153). The pass runs at :2575 AFTER
Connect, so at pass time there are NO `rel->selects` to walk. §3.4's "each
SELECT-on-p's relation (rel->selects, before proxying)" DOES NOT EXIST at
:2575.

SHARPENED §3.4 walk (supersedes the SELECT-centric description):
  - START from the relation's MERGE (`p`'s MERGE = TABLE 8 for `path`;
    `d_path`'s MERGE once minted). A "demanding read of `p`" is a TRANSITIVE
    USER of `p`'s MERGE — a TUPLE proxy reading it (the post-`ProxySelects`
    shape) — that is NOT one of `p`'s own producer members (the MERGE's
    `merged_views` are the producers; users reached via `MERGE->successors`
    / the view's use edges are the readers). Enumerate `p`-MERGE's users,
    exclude the members, classify each remaining reader by the rule body it
    sits in.
  - SELF-JOIN (`p(x,y) :- p(x,z), p(z,y)`): ONE body proxy reads `p`'s MERGE
    TWICE at two different adornments. The walk must enumerate BOTH reads as
    distinct demanding subgoals (two `input_columns` uses of the MERGE with
    different bound-col projections) and not conflate their adornments. NOT
    exercised by the single-recursive-read witness — flagged as the first
    multi-read stress (a §6 bet).
  - THE QUERY PROJECTION (F1/§3.7): the query's own projection body reads `p`
    and IS a demanding read — in the spike it is demand-guarded (the 4th
    JOIN). §3.4 now explicitly counts the query projection as a demanding
    subgoal whose bound cols seed `d_p` AND whose body carries a §3.7 guard.
    (Previously §3.4's ROOT rule seeded `d_path` from the query's bound cols
    but was silent on guarding the query's own body; F1 fixes that.)

The witness (single-rule-recursive, one self-read) CANNOT validate the walk;
the recipe does NOT lean on it as discharging the walk — the walk correctness
is a §6 pre-registered bet, gated by the oracle on any multi-rule/self-join
corpus case.

### F7 [LOW, DISCHARGED] — the two `d_path` sources fuse by MERGE-member dedup, not CSE fusing two distinct producers.

The judge's sharpening is correct. §3.5(b)'s "the two `d_path` members DO
fuse in spike.opt.dot" is imprecise about the MECHANISM. The root-seed source
projects `From` from the `d_path_bf` RECEIVE; the propagation source projects
`From` from `path` — DIFFERENT inputs. They do NOT fuse by CSE (CSE `Equals()`
compares `input_columns` USE structure — two distinct producers with distinct
inputs do not match). They coexist as two members of the SAME `d_path` MERGE
with the same output shape; if the MERGE dedups structurally-equal members
they collapse, otherwise both remain as parallel members feeding one MERGE.
The DISTINCTION MATTERS: a future case with two PROPAGATION sources from
DIFFERENT relations would produce two distinct MERGE members that do NOT fuse
(different inputs) — the shared `d_path` frontier is the MERGE, not a fused
single producer. §3.5(b) is corrected to say "the two sources coexist as
`d_path` MERGE members (dedup by the MERGE, not CSE fusion of two producers)";
the recipe must NOT rely on CSE collapsing distinct-input demand sources.

### F8 [NIT, ACCEPTED] — pass slot is :2575, :2566 is the doc-comment head. Confirmed. No action (A3 already records it).

### F9 [NIT, DISCHARGED] — the §3.3 restoring TUPLE is a pure identity here; its Optimize-survival is checked against G3, not assumed.

Correct: the demand pivot IS `p`'s bound col, so the guard JOIN output is
already `[From,To]` = `p`'s shape and the restoring TUPLE drops nothing (a
pure identity). Kept for parity with C7 and because the spike's guarded
members show a TUPLE above each guard JOIN (matching the oracle). CAVEAT: a
"drop nothing" TUPLE is the kind of node Optimize MAY canonicalize away, so
its survival is CHECKED against G3 (spike.opt.dot), not assumed — re-verified
this session: spike.opt.dot retains the guard-member TUPLEs and all 4 JOINs,
so the restoring TUPLE survives; the S3 gate diffs against G3 and would flag
a divergence if Optimize collapsed it.

--------------------------------------------------------------------------
## AMENDMENTS ROUND 3 (2026-07-17, per judge-d4s3-round2)

The round-2 adversarial judge (`design/judge-d4s3-round2.md`, verdict
REVISE) DISCHARGED the F1-F9 diagnoses and the two HIGH structural facts
(the 4th JOIN pivots the raw seed; the injector is an unspiked S4 obligation
that lands WITH S3), leaving five concrete construction-level errors: three
HIGH (N1/N2/N3) that each fail the recipe's OWN structural dump-diff oracle,
plus N4/N5 and the F1/F3 PARTIALs. Every anchor below was RE-VERIFIED this
session against the dumped graphs (`.../scratchpad/d4s3/*.dot`) and the tree
at tip `0f396116`. The N3 join-tree ground truth was traced NODE-FOR-NODE
from `spike.nodf.dot` (all four JOINs, TABLE 19/11/23) this session. The
Road-G decision (§2) is NOT overturned — all fixes are node-construction
corrections. Read this section as authoritative over the pre-judge body AND
over the ROUND-1 AMENDMENTS where they conflict.

### N3 [HIGH] — the demand guard is a SIP JOIN-TREE PUSH-DOWN to the demanded subgoal READ, NOT a guard on the rule-body member OUTPUT. §3.3 REPLACED.

This is the substantive one. §3.3 (and its ROUND-1 body) guards the WHOLE
rule-body MERGE member OUTPUT: `join->joined_views.AddUse(body)` where `body`
is the member proxy `[F,T]`, pivoting the whole `[F,T]` output against
`d_path` on `From`. That builds `(path ⋈ edge_2) ⋈ d_path`. The spike does
the TEXTBOOK MAGIC-SETS placement: the demand guard joins at the DEMANDED
SUBGOAL READ inside the body's join tree, so the recursive member is
`edge_2 ⋈ (d_path ⋈ path)` — a STRUCTURALLY DISTINCT tree. §3.3's
guard-on-output diverges from the spike on ANY multi-atom rule body, failing
the S3b/S3c structural oracle.

NODE-FOR-NODE GROUND TRUTH (traced this session from `spike.nodf.dot`; the
four JOINs, with the dump ids/shapes). Base has exactly ONE JOIN
(`v4386540768`, the recursive `path` join on M); the spike has FOUR:

  JOIN 18 = `v4395407040` (EQ SET 18, pivot F, out [F,M]) — THE PUSH-DOWN.
    p0 → `v4395415776`:c14 = TABLE 8 TUPLE[F] → `v4395410432` = d_path UNION.
      ⇒ a `d_path` read (a TUPLE over the d_path MERGE — F4(ii)).
    p1,p2 → `v38432588224`:c19/c20 (From,To) = TABLE 11 = the `path` UNION.
      ⇒ a `path` read.
    ⇒ JOIN 18 = `d_path[F] ⋈ path[From,To]` on pivot F  →  TABLE 19
      (`v38432590464`, [F,M]). This is the guard FUSED INTO the path READ at
      the BOTTOM of the recursive body's join tree — the magic-sets sideways
      guard on the recursive subgoal.

  JOIN 22 = `v4395408112` (EQ SET 22, pivot M, out [M,T,F]) — the RECURSIVE
    member's OUTER join, UNCHANGED in SHAPE from base except its `path`-side
    input is now TABLE 19 (guarded) instead of a bare `path` read:
    p0,p2 → `v38432590016`:c27/c28 (M,T) = TABLE 15 → edge_2 RECEIVE. ⇒ edge_2.
    p1,p3 → `v38432590464`:c30/c29 (M,F) = TABLE 19 = JOIN 18 output.
    ⇒ JOIN 22 = `edge_2[M,T] ⋈ TABLE19` on pivot M = `edge_2 ⋈ (d_path ⋈ path)`.
    This TUPLE-chains up (`v4395409472` → `v38432587776`) to TABLE 11's
    (path UNION `v4395416672`) recursive member.

  JOIN 20 = `v4395403024` (EQ SET 20, pivot F, out [F,T]) — the BASE member.
    p0 → `v4395415776`:c14 = d_path read (SAME d_path TUPLE as JOIN 18).
    p1,p2 → `v38432589568`:c25/c26 (M,T) = TABLE 15 → edge_2 RECEIVE.
    ⇒ JOIN 20 = `d_path[F] ⋈ edge_2[F,T]` on pivot F. TUPLE-chains up
    (`v4395404112` → `v4395416224`) to TABLE 11's base member.
    NOTE the BASE rule has NO interior recursive/derived read of `path` — its
    body is just `edge_2(F,T)`. The demanded column `From` is carried by
    edge_2's own first column, so the guard joins at edge_2 (the body's
    bound-column source). There is NO separate "push-down to a path read" for
    the base rule; the guard lands directly on the single body atom.

  JOIN 7 = `v4395412864` (EQ SET 7, pivot From, out [From,To]) — the
    query-projection guard (N1/F1, §3.7), covered below. Not part of §3.3.

So the recursive member's guard is a PUSH-DOWN (JOIN 18 → TABLE 19) INSIDE
the body join tree; the base member's guard is a splice at the body's
bound-column source (JOIN 20). Both are the E-32 1→N-pivot generalization,
placed by the SIP walk, NOT a guard on the assembled `[F,T]` output.

§3.3 REPLACEMENT — the SIP JOIN-TREE PUSH-DOWN. For each rule-body MERGE
member `body` of a demanded relation `p` at adornment α:

  1. SIP-LOCATE the guard site. Walk `body`'s join tree (the member proxy's
     transitive producer views: JOINs, and the TUPLE/SELECT-proxy leaves that
     read relations). The guard's join site is the SUBGOAL READ whose bound
     columns carry the demand α — identified by the same SIP availability
     walk §3.4 uses:
       - if the body contains a recursive/mutual read of `p` (or another
         demanded relation) whose bound columns are α-available, THAT read is
         the demanded subgoal — the guard joins THERE (the JOIN 18 shape:
         mint `d_p ⋈ (that read)` on the α pivot columns, producing a new
         guarded intermediate that REPLACES the bare read as the input to the
         body's remaining joins);
       - if the body has NO interior demanded-subgoal read (a base rule),
         the demanded columns are carried by the body's bound-column SOURCE
         atom (edge_2 for the witness base rule); the guard joins THERE (the
         JOIN 20 shape: mint `d_p ⋈ (source atom)` on the α pivot).
     In BOTH cases the guard is ONE JOIN whose pivots = α's bound columns,
     joining a `d_p`-MERGE TUPLE (F4(ii)) against the located read/atom.

  2. MINT the guard JOIN at that site (the C7 1→N-pivot generalization,
     verbatim from ROUND-1 §3.3 step 2): `joins.Create()`;
     `joined_views.AddUse(located_read); joined_views.AddUse(d_p_tuple);`
     `num_pivots = |bound(α)|`; per α-pivot column mint the pivot out-col and
     `out_to_in.AddUse` BOTH the located-read's bound col AND the `d_p`
     TUPLE's col (the E-32 real column edge); per non-pivot column of the
     located read, a pass-through out-col. The `d_p` demand side is a TUPLE
     over the `d_p` MERGE (F4(ii) — NEVER a relation SELECT; N2).

  3. RE-WIRE the join tree. For the PUSH-DOWN case (interior demanded
     subgoal), the guard JOIN's output (via a restoring TUPLE preserving the
     read's column shape — the TABLE 19 `[F,M]` node) REPLACES the bare
     subgoal read as the input to the body's REMAINING joins (JOIN 22 then
     consumes TABLE 19 in place of the old `path` read on M). For the base
     case, the guard JOIN's restoring TUPLE `[F,T]` becomes the member
     directly. In neither case is the assembled `[F,T]` member output guarded
     as a whole.

  4. RE-POINT the `p` MERGE member: replace the merged-view use of the old
     `body` proxy with the top of the re-wired tree (the restoring TUPLE over
     the outermost join). After this, `p`'s member matches G2's TABLE 11
     members exactly: base member = restoring-TUPLE-over-JOIN-20; recursive
     member = restoring-TUPLE-over-JOIN-22-over-TABLE19-over-JOIN-18.

GENERAL RULE (for any body): the guard joins at the subgoal whose bound
columns carry the demand — the SIP walk (§3.4) identifies it. A body with a
demanded recursive/derived subgoal read gets the guard PUSHED DOWN to that
read (JOIN 18 → TABLE 19 shape). A base body with no demanded interior read
gets the guard at its bound-column source atom (JOIN 20 shape). This is the
textbook magic-sets sideways-information-passing placement; the recipe's
oracle is structural identity with the spike, so the guard MUST land at the
SIP-located site, never on the member output.

WITNESS REPRODUCTION REPLAYED NODE-FOR-NODE UNDER THE NEW RULE (all four
JOINs, tables matching the dump ids/shapes):
  - `path` (TABLE 11, `v4395416672`) MERGE, 2 members:
      base member → JOIN 20 (`v4395403024`) = `d_path[F] ⋈ edge_2[F,T]`
        (guard at the base body's bound-col source);
      recursive member → JOIN 22 (`v4395408112`) = `edge_2[M,T] ⋈ TABLE19`,
        where TABLE 19 (`v38432590464`) = JOIN 18 (`v4395407040`) =
        `d_path[F] ⋈ path[From,To]` (guard PUSHED DOWN into the recursive
        path read).
  - `d_path` (TABLE 8, `v4395410432`) MERGE, 2 members (§3.2/§3.4, F4(ii)):
      root member = TUPLE over the `d_path_bf` RECEIVE (`v4395406400` →
        `v4395400464` → receive `v38432526720`);
      propagation member = TUPLE projecting [F] off the recursive `path` read
        (`v4395401680` → `v4395411072` → TABLE 11).
  - `reachable_from` projection = JOIN 7 (`v4395412864`) = raw-seed[From] ⋈
    guarded-`path`[From,To] (§3.7/N1, below).
All four JOINs accounted for; every table id matches the dump.

### N1 [HIGH] + F1 [PARTIAL, now discharged] — §3.7 mints a FRESH receive projection for the query guard; do NOT reuse the d_path root TUPLE.

The ROUND-1 F1 amendment's §3.7 said "Reuse that TUPLE [the `d_path` MERGE's
root member from §3.2 step 4] as the JOIN's demand side; do not mint a fresh
`d_path` SELECT here." That is a NEW reproduction error, verified against
`spike.opt.dot`: the `d_path_bf` RECEIVE (`v33000919424`, c3) has TWO DISTINCT
consumer projections, BOTH surviving Optimize:
  - `v33000950848` (STRATUM 2, EQ SET 2, TUPLE From→c3) → the `d_path` UNION
    TABLE 8 (`v4390430736`) — the d_path MERGE ROOT MEMBER;
  - `v33000950400` (TABLE 23, STRATUM 6, EQ SET 2, TUPLE From→c3) → JOIN 7
    (`v4390433168`:p0) — the query-guard demand side.
They are SEPARATE nodes that do NOT fuse under Optimize. Reusing the d_path
root member for the query guard would produce a graph ONE receive-projection
short of the spike and FAIL the S3d "TABLE 23 present" gate against the
recipe's own oracle.

§3.7 CORRECTION (supersedes the ROUND-1 §3.7 "reuse that TUPLE" instruction):
mint a FRESH receive-projection TUPLE for the query guard — a NEW
`tuples.Create()` projecting `From` off the `d_path_bf` RECEIVE (matching
TABLE 23, `v38432590912`/`v33000950400`), DISTINCT from the `d_path` MERGE's
root member (§3.2 step 4). Both project `From` off the SAME receive; both must
survive as distinct nodes (they do — spike.opt.dot). The query guard JOIN 7
pivots `From` between THIS fresh receive projection and the
`reachable_from`-projection member reading guarded `path`; a restoring TUPLE
`[From,To]`; re-point the `reachable_from` UNION member. The "IMPORTANT —
pivot against the raw seed" fact from ROUND-1 F1 STANDS (the guard pivots the
receive, not the derived `d_path`); only the "reuse that TUPLE" construction
is corrected to "mint a fresh one."

F1 DISCHARGE (now honest): the S3d gate claim — 4-mode STRUCTURAL dump ==
spike, all 4 JOINs, TABLE 23 present — is now STRUCTURALLY ACHIEVABLE by the
recipe as written, because §3.7 mints the distinct TABLE-23 receive
projection the spike has rather than fusing it into the root member. The F1
diagnosis (the 4th JOIN exists; guard the query projection against the raw
seed) was already discharged in ROUND 1; this fold repairs the construction
so the gate's "structural identity with the spike dump" is truthful.

### N2 + F3 [PARTIAL, now discharged] — DELETE read-route (a) (the fabricated ParsedPredicate); the SOLE d_path read construction is the F4(ii) TUPLE-over-MERGE route.

The ROUND-1 §3.3 step 1 and F3 named TWO d_path-read routes: (a) a fabricated
minimal `ParsedPredicate` over the `d_path` local + `selects.Create(d_rel,
ParsedPredicate(pred))`, and (b) a TUPLE proxy over the `d_path` MERGE
(F4(ii)). They contradict, and route (a) is the Road-C chain in disguise:
`ParsedPredicateImpl`'s ctor (Parse.h:189-190, re-verified) REQUIRES a
non-null `ParsedClauseImpl *clause_` (const member, :199) and its
`argument_uses` is a `UseList<ParsedVariableImpl>` (:212) — you cannot
construct a `ParsedPredicateImpl` without a `ParsedClauseImpl` and interned
`ParsedVariableImpl` args, precisely the clause/variable fabrication C17
eliminated and Road-C was rejected for. Route (a) is NOT "one positive
predicate"; it is a Road-C-sized fabrication.

DECISION (supersedes ROUND-1 F3): route (a) is DELETED. The recipe's SOLE
construction for reading the DERIVED `d_path` relation is F4(ii): a TUPLE
proxy over the `d_path` MERGE. The dumps confirm every derived-`d_path` read
in the spike (the JOIN 18 / JOIN 20 demand sides = `v4395415776`, the §3.4
propagation source) is a TUPLE over the `d_path` MERGE — NO relation SELECT
exists anywhere over `d_path`. The RAW-SEED reads (the §3.2-step-4 root
member and §3.7's fresh query-guard projection) read the RECEIVE STREAM and
stay pred-less via the landed `QuerySelectImpl(QueryStreamImpl*, DisplayRange)`
ctor / TUPLE-over-receive — they were never route (a).

CONSISTENCY SWEEP (all cross-references corrected to the single route):
  - §3.3 step 1 ("Mint the `d_path` reader"): the demand side of EVERY guard
    JOIN (base JOIN 20, recursive JOIN 18, query JOIN 7) is a TUPLE proxy
    over the `d_path` MERGE (F4(ii)) — there is no `selects.Create` over
    `d_rel` and no fabricated `ParsedPredicate` anywhere in the recipe.
  - §5 A1's "A1 ALSO absorbs F3's read-predicate … every DERIVED-`d_path`-
    relation read needs a fabricated minimal `ParsedPredicate`" is DELETED.
    A1 now covers ONLY the `d_path` LOCAL DECL (the `FabricateDemandLocal`
    naming obligation, F5) — NOT a read-predicate. There is no read-predicate
    fabrication in the recipe.
  - §3.3's ROUND-1 F3 note ("Two corrected options … (a) a fabricated minimal
    ParsedPredicate … or (b) a TUPLE proxy") collapses to: option (b) ONLY.
  - The F3 amendment's "HONEST COST — this re-introduces a THIN slice of
    predicate fabrication" paragraph is WITHDRAWN: no predicate fabrication
    occurs. The message receive AND every `d_path` read are pred-less (stream
    ctor / TUPLE-over-MERGE respectively). C17's "no ParsedPredicate/clause/
    variable chain" claim now holds for the WHOLE transform, not just the
    message.

Consequence: the `d_path` local decl (A1/F5) is still required (the
`QueryRelationImpl` ctor needs a `ParsedDeclaration`, and codegen names the
table off `decl->Name()`), but NO `ParsedPredicate` over it is ever
fabricated. The decl exists so `d_rel` and its INSERTs/MERGE are well-formed
and named; all reads of it are graph-level TUPLE proxies.

### N4 [LOW] — clear `d_rel->inserts` after minting the MERGE, mirroring Connect.

The ROUND-1 F4 amendment hedged: "`d_rel->inserts` MAY still be populated for
`TrackDifferentialUpdates` (which DOES re-run)." That hedge is WRONG and its
ambiguous branch is a latent hole. Re-verified: `ConnectInsertsToSelects`
CLEARS `rel->inserts` at Connect.cpp:261 immediately after
`CreateProxyOfInserts`, for a normal MERGE-proxied relation (the IsQuery
re-add at :275-282 does NOT apply to `d_path`, which is a `#local`, not a
query). `TrackDifferentialUpdates` derives differential status by walking the
insert→select SEAM (Differential.cpp), which for a MERGE-proxied relation
works off the graph structure, NOT off live `rel->inserts`. Leaving
`d_rel->inserts` populated produces stray INSERT views that
`ProxyInsertsWithTuples` (C5:2593) decorates with TUPLEs and that feed nothing
(all readers are proxied off the MERGE) — dead-flow material and/or a spurious
insert→select seam mismatch.

DIRECTIVE (supersedes the F4 hedge and its third bullet): mirror
`ConnectInsertsToSelects`:261 EXACTLY — after minting the `d_path` MERGE over
the demand-source TUPLE proxies, CLEAR `d_rel->inserts` (`d_rel->inserts
.Clear()`). The F4 third bullet ("`d_rel->inserts` may still be populated
for `TrackDifferentialUpdates`") is STRUCK. (Minor, carried from ROUND-1: for
a SINGLE-source demand relation `CreateProxyOfInserts` returns a bare TUPLE
proxy with NO MERGE (Connect.cpp:49-51); the witness `d_path` has 2 members so
a MERGE is correct, but a demand relation with only a root seed and no
propagation source would normally be MERGE-less to match Connect's own
single-insert behavior — the pass should match that shape too.)

### N5 [NIT] — `demand_fabricated` is a single-shot PASS gate, not a per-CALL gate.

Re-verified: `demand_fabricated` is ONE module bool (Parse.h:582).
`FabricateDemandMessage` asserts `!module->demand_fabricated` at ENTRY
(Demand.cpp:47) and SETS it true at EXIT (:146). A sibling
`FabricateDemandLocal` that "reuses the same route" including that assert/set
would trip the ENTRY assert when called after the message (the flag is
already true), self-aborting.

FLAG SEMANTICS CHANGE (specify): the flag admits ONE fabrication PASS, not one
CALL. A single guarded fabrication pass may mint the demand MESSAGE, the
demand LOCAL, and any future demand decls TOGETHER inside one guarded entry.
Concretely:
  - the assert-and-set moves OUT of `FabricateDemandMessage` into the pass's
    single guarded entry: the demand pass checks `!DemandMessagesFabricated()`
    ONCE at the START of its fabrication work (hard-abort if already set — the
    G2 re-entry reject), does ALL fabrication (message + local + …), then sets
    `demand_fabricated = true` ONCE at the END of the pass;
  - `FabricateDemandMessage` DROPS its own entry assert (:47) and exit set
    (:146) — it becomes a fabrication PRIMITIVE with no flag side-effect;
  - `FabricateDemandLocal` (the F5 sibling) likewise carries NO own flag
    assert or set. Neither primitive touches the flag; the flag is owned by
    the ONE pass-level guarded entry.
This lets message and local be minted in the same pass without the
single-bool self-abort, while preserving the single-shot G2 re-entry
protection (the pass still aborts if `Query::Build` re-enters a module that
already carries fabricated demand decls).

### §6 / STAGING (S3a-S3d) RE-CHECK against ROUND-3 fixes.

The §6 predictions and the §7 S3a-S3d gates are re-checked and updated:

  - S3a (demand relation + root source + `d_path` MERGE): UNCHANGED except
    (N2) the demand-source and reader constructions are TUPLE-over-MERGE /
    TUPLE-over-receive ONLY (no fabricated read-predicate); (N4) `d_rel->
    inserts` is CLEARED after the MERGE is minted; (N5) the `d_path` LOCAL is
    minted by `FabricateDemandLocal` INSIDE the same single-shot fabrication
    pass as the message. Gate: `d_path` a MERGE sourced by the
    receive-projection TUPLE, table NAMED, no stray inserts.
  - S3b (the guard splice, §3.3 AS REPLACED BY N3): the gate now diffs the
    PUSH-DOWN tree — the recursive member must be `edge_2 ⋈ (d_path ⋈ path)`
    (JOIN 22 over TABLE 19 over JOIN 18), the base member `d_path ⋈ edge_2`
    (JOIN 20); NOT a guard-on-output shape. Gate: `path` (TABLE 11) members
    match G2's pushed-down join tree node-for-node (TABLE 19 present as the
    JOIN-18 output feeding JOIN 22).
  - S3c (SIP propagation source, §3.4, the MERGE-user walk, F6): UNCHANGED;
    gate `path`/`d_path` node set == spike for 3 of the 4 JOINs (18, 20, 22).
  - S3d (query-projection guard, §3.7, N1): mints a FRESH receive projection
    (TABLE 23), NOT the reused d_path root member. Gate: 4-mode STRUCTURAL
    dump == spike, ALL FOUR JOINs (18, 20, 22, and 7), TABLE 23 present AND
    DISTINCT from the d_path MERGE root member.
  - §6 bet re-rank UNCHANGED in spirit (F1 conditioning stands), but the
    PRIMARY structural first-divergence risk is now N3: a guard minted on the
    member OUTPUT rather than pushed down to the demanded subgoal read will
    produce `(path ⋈ edge_2) ⋈ d_path` — a dump that DIFFERS from the spike's
    `edge_2 ⋈ (d_path ⋈ path)` (TABLE 19 absent / wrong join order), caught by
    the S3b structural gate before any answer gate.

DUMP-DIFF GATE WORDING (N3-faithful, supersedes §7's closing "dumped-graph
diff against G2/G3" wording): every S3 stage gates on a NODE-FOR-NODE
structural diff against the spike dumps in which the `path` member join TREES
match — specifically the recursive member is the PUSHED-DOWN
`edge_2 ⋈ (d_path ⋈ path)` (JOIN 22 / TABLE 19 / JOIN 18), the base member is
`d_path ⋈ edge_2` (JOIN 20), and the query projection is `raw-seed ⋈ path`
(JOIN 7 / TABLE 23 distinct from the d_path root member) — NOT merely "a guard
JOIN exists on each member." A guard-on-output miscompile (wrong join tree,
missing TABLE 19) shows in this dump diff before it shows in an answer.
