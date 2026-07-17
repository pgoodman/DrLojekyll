# P3 — the demand transform: invariant-preservation argument (the R4-style gate)

Lane P3, DESIGNER, subgraphs-demand epoch. Written 2026-07-16 against
branch `subgraphs-demand` HEAD (P0 census landed). This is the reviewed
argument PerfRoadmap §14.3-P3 requires BEFORE any P3 code: "a DataFlow
rewrite CAN silently miscompile; a reviewed argument required before
code … IF the argument does not pass review, P3 re-seeds (as R4 did for
THREE epochs before its P3 retirement)." A judge will try to falsify
the argument; every code claim is anchored to a file:line read this
session.

Binding inputs honored throughout: ledger errata E-32 (SubgraphsDemand.md
§1:100-110 — group_ids CANNOT keep demand copies distinct; use a
structural demand INPUT EDGE, the `ApplyPositiveConditionTest` ⊥c pivot
precedent) and E-33 (§1:111-118 — obligations extended to (e)/(f) + the
Stratify.cpp:387-419 induction cross-check). SLDMagic is a
TRANSFORMATION, never an evaluator (memory: push-method-origin-and-
negation) — a Query-level rewrite upstream of the DR-IR.

The headline finding, stated up front so review can weigh it first: the
seed's own PRE-REGISTERED PREDICTION that "existing 164 zero churn since
the transform only fires on bound queries" is FALSE. **37 of the 164
corpus cases carry a `bound` column in a `#query`** (verified this
session; list in §5). A bound `#query` today materializes the FULL
relation and builds a scan index over the bound columns
(Build.cpp:393-411, ControlFlow). The demand transform, by construction,
changes WHICH instances materialize for exactly those queries — so it
touches those 37 goldens the moment it fires. This is not a defect in
the transform; it is a defect in the seed's churn prediction, and it
drives the scope recommendation (§4) and the corrected predictions (§5).

--------------------------------------------------------------------------
## 1. THE TRANSFORM (specified to argue about, not to implement)

### 1.1 Input and output

INPUT: a `#query` declaration with at least one `bound` column — the
demand SOURCE. The corpus surface is `#query p(bound T c0, free T c1, …)`
(e.g. `#query reachable_from(bound u64 From, free u64 To)`,
tests/OptDiff/cases/transitive_closure.dr). The bound columns are the
adornment seed: they are the columns the caller supplies at query time.

OUTPUT: a rewritten `QueryImpl` graph in which, for each predicate `p`
reachable from the demanded query under sideways-information-passing
(SIP), we introduce:

  - a DEMAND RELATION `d_p` — a fresh relation whose rows are the ground
    tuples of `p`'s bound-column positions for which `p` is actually
    demanded. `d_p`'s columns are exactly the adorned (bound) columns of
    `p` under the propagated adornment.
  - a DEMAND-GUARDED COPY `p'` of `p`'s rule bodies, structurally
    `p' = p ⋈ d_p` on the bound columns, so `p'` only produces rows whose
    bound-column projection appears in `d_p`.

The demand relation is populated by the DEMANDING SUBGOALS: wherever a
rule body of a demanded predicate `q` contains a subgoal `p(x̄, ȳ)` whose
bound positions x̄ are already available (bound) at that point in the
body's left-to-right SIP order, we emit a rule `d_p(x̄) :- <the bindings
that make x̄ available>`. The `#query` seed itself emits the root demand:
`d_p(bound-cols) :- <caller supplies>` — i.e. the query's own INSERT/scan
seam becomes the source of `d_p`'s root rows.

This is textbook magic-sets / SLDMagic (adornment propagation + a magic
predicate per adorned predicate + a guarded rule copy). The ONE
Dr.-Lojekyll-specific commitment is HOW the guard `⋈ d_p` is realized in
the Query IR — spelled next, because E-32 makes it load-bearing.

### 1.2 The demand edge is a REAL COLUMN EDGE (E-32, spelled exactly)

E-32 (SubgraphsDemand.md §1:100-110) proves that relying on `group_ids`
to keep `p'` distinct from `p` is UNSOUND. The chain:

  - `InsertSetsOverlap` returns `false` (MERGEABLE) whenever EITHER
    view's `group_ids` set is empty (View.cpp:1478-1480, read this
    session: `if (a->group_ids.empty() || b->group_ids.empty()) return
    false;`).
  - `group_ids` seed ONLY on JOIN / AGG / KVINDEX views
    (Optimize.cpp:423, read this session: `if (view->AsJoin() ||
    view->AsAggregate() || view->AsKVIndex())` — everything else gets
    `group_id = 0` and an empty `group_ids`).
  - Therefore a demand copy `p'` whose top view is (say) a MERGE or TUPLE
    has empty `group_ids`, and CSE's overlap test would report it
    MERGEABLE with its unguarded twin `p` — silently collapsing the
    guard and re-materializing the full relation.

So the guard MUST be structural: `p'` must differ from `p` in
`Equals()`-visible shape (arity / children), which is tested BEFORE any
`group_ids` logic runs. The precedent is `ApplyPositiveConditionTest`
(Build.cpp:1450-1512, read this session), the zero-arity-condition
desugar. Its exact shape, which the demand guard COPIES:

```
    view[X..]   CONST true
        \        /
      TUPLE [X.., t]        SELECT ⊥c[t]        <- unit relation SELECT
             \               /
            JOIN (pivot: t) [t, X..]            <- ONE pivot, real column edge
                  |
            TUPLE [X..]                          <- restores original shape
```

The load-bearing detail (Build.cpp:1457-1459, verbatim comment): "The
unit relation's SELECT column is deliberately NOT marked as a constant:
the pivot must remain an ordinary column edge so that the join keeps
expressing the presence dependency on `⊥c`." The demand guard is the
SAME move, generalized from a 1-pivot presence test to an N-pivot key
match:

  - `p'`'s rule body is built as usual, producing the tuple `[x̄, ȳ]`.
  - We insert a `SELECT d_p` producing `[x̄]` (the demand relation's rows,
    keyed on the bound columns).
  - We build a JOIN whose pivots are the |x̄| bound columns: pivot i
    equates `p'`-body column `x_i` with `d_p`-SELECT column `x_i`
    (`join->num_pivots = |x̄|`, each pivot's `out_to_in` carries BOTH the
    body column and the demand-SELECT column, exactly like
    Build.cpp:1483-1489 does for the single ⊥c pivot).
  - A restoring TUPLE drops nothing here (unlike the condition test, which
    drops the token) — the demand columns ARE `p`'s bound columns, so
    the JOIN output already has `p`'s shape; the restoring TUPLE exists
    only to re-establish canonical column identity, matching
    Build.cpp:1502-1510.

STRUCTURAL DISTINCTNESS, concretely: `p'`'s top view is a JOIN with
`num_pivots = |x̄| ≥ 1` and an extra joined view (the `d_p` SELECT). `p`
has no such JOIN input. `Equals()` on a JOIN compares `num_pivots` and
the joined-view children (Join.cpp:467, read this session:
`num_pivots != that->num_pivots || …` short-circuits). The demand copy
and its twin therefore DIFFER ON ARITY-OF-CHILDREN / num_pivots before
`group_ids` is ever consulted. E-32's "make copies STRUCTURALLY distinct
via a demand/magic INPUT EDGE … Equals() fails on arity/children before
group_ids logic runs" is satisfied by construction.

Arity note: the JOIN adds no output columns (the demand pivots ARE
`p`'s bound columns, matched not appended), so `p'` has `p`'s arity.
Distinctness comes from the extra JOIN INPUT and the nonzero pivot count,
NOT from arity. This matters for §2(b): the copy is arity-identical but
structurally distinct, which is precisely the regime E-32 requires.

### 1.3 Where in the pipeline it runs, and why

Pipeline ordering, read this session (Build.cpp:2550-2610):

```
  RemoveUnusedViews → ClearGroupIDs → TrackDifferentialUpdates
    → Simplify → ConnectInsertsToSelects
    → [Optimize: Simplify → Canonicalize fixpoint → CSE]   (if optimize)
    → ConvertConstantInputsToTuples → RemoveUnusedViews
    → ProxyInsertsWithTuples → LinkViews → RemoveUnusedViews
    → IdentifyInductions → FinalizeDepths → FinalizeColumnIDs
    → TrackDifferentialUpdates(final) → TrackConstAfterInit
    → BuildEquivalenceSets → Stratify
```

DECISION: the demand transform runs as a dedicated pass INSIDE
`Query::Build`, AFTER `BuildClause`/`Simplify`/`ConnectInsertsToSelects`
but BEFORE `Optimize` (i.e. it is invoked immediately before the
`if (optimize)` block at Build.cpp:2566). Justification, point by point:

  1. AFTER `ConnectInsertsToSelects` (Build.cpp:2562): the transform needs
     the INSERT↔SELECT plumbing connected so it can find each predicate's
     producing rules (the demand-relation sources, obligation (f)) and
     each demanding subgoal's SELECT. Before this the relation graph is
     not yet wired end to end.

  2. BEFORE `Optimize` (Build.cpp:2566-2573): the demand relations `d_p`
     are deliberately exposed to the SAME CSE / canonicalize fixpoint the
     rest of the graph gets. This is a FEATURE: two demanding subgoals
     that demand `p` on the same bound-column shape produce two `d_p`
     source rules that SHOULD CSE-merge into one demand relation (fewer
     materialized demand rows). The transform does not hand-dedup demand
     sources; it emits them structurally and lets the existing fixpoint
     fold them — this is exactly the shared-demand-frontier fusion the
     seed (§14.0.1) predicts demand multiplies. Running AFTER Optimize
     would forfeit that and require a bespoke dedup pass (a second
     evaluator smell).

  3. BEFORE `IdentifyInductions` (Build.cpp:2580) and `Stratify`
     (Build.cpp:2610): the induction SCCs and strata must be computed on
     the FINAL (post-demand) graph, so that the Stratify cross-check
     (§2(a), Stratify.cpp:387-419) validates the demand edges too. If the
     transform ran after Stratify, a demand-introduced cycle would escape
     the reject entirely — the exact silent-miscompile the R4-style gate
     exists to prevent.

  So: the demand pass slots between `ConnectInsertsToSelects` (2562-2564)
  and `Optimize` (2566). It is guarded by a mode flag (`-demand` /
  default-off during bring-up; see §5), and it is a NO-OP unless the
  module has a `#query` with a bound column — a fully-free corpus is
  untouched (this is what saves the 127 free-only cases; see §5).

### 1.4 What the transform does NOT do

  - It is NOT an evaluator. It emits no runtime dispatch, no demand-queue
    runtime object, no per-instance interpreter. The output is a plain
    `QueryImpl` graph — SELECTs, JOINs, TUPLEs, MERGEs, INSERTs — that the
    EXISTING DR-IR lowering (Query → DR → Program → C++) consumes with no
    new lowering path. The demand relation `d_p` is an ordinary relation;
    `p'` is an ordinary guarded view. (Obligation (d), argued in §2.)
  - It does NOT introduce a new op family. Unlike P2's
    SUBGRAPH_INSTANTIATE, the demand transform adds zero DR-IR ops, zero
    effect sub-domains, zero runtime stores. It is a pure Query-graph
    rewrite. (This is why it CAN be design-only without blocking P2 —
    §4.)
  - It does NOT touch the differential machinery directly. `d_p` and `p'`
    are relations like any other; whether they are monotone or
    differential is DERIVED by the existing `TrackDifferentialUpdates`
    pass on the rewritten graph. The transform asserts nothing about
    signs or counters.

--------------------------------------------------------------------------
## 2. THE INVARIANT-PRESERVATION ARGUMENT (obligation by obligation)

Obligations (a)-(d) are PerfRoadmap §14.3-P3; (e)-(f) are E-33
(SubgraphsDemand.md §1:111-118). Each: the exact code invariant site,
then why the transform preserves it OR the restriction that makes it so.

### (a) Stratification preserved + the induction cross-check

INVARIANT SITE: `QueryImpl::Stratify` (Stratify.cpp:122) computes SCCs +
strata; the always-on debug cross-check at Stratify.cpp:387-419 (read
this session) asserts, for every info-bearing view:
  - `assert(inductive == same_scc)` for each predecessor AND successor
    edge (Stratify.cpp:390/398) — an edge is masked-inductive EXACTLY when
    it stays inside the SCC;
  - `assert(*pred->stratum <= *view->stratum)` / `<= *succ->stratum`
    (:391/:399) — strata respect the topological order;
  - the union-find `merge_set_id` and SCC membership induce the SAME
    partition (Stratify.cpp:404-413);
  - every multi-view SCC contains an inductive MERGE or an IO seam
    (Stratify.cpp:418-420).

THE HAZARD (stated honestly): magic-sets is KNOWN in the literature to be
able to BREAK stratification. The classic counterexample: a predicate `p`
defined with a NEGATED subgoal `not q`, where `q` is in a lower stratum,
becomes — after magic-set rewriting — mutually recursive with its own
magic predicate `d_p` THROUGH the negation, because the demand for `p`
flows into `q`'s demand and `q`'s result flows back through `not q` into
`p'`. The naive magic rewrite can place `d_p`, `q`, and `p'` in one SCC
with a negation on a cycle edge — an unstratified negation that the
ORIGINAL program did not have. (This is the well-documented
"magic-sets destroys stratification for negation" result; the standard
fixes are magic-sets-for-stratified-programs / the "well-founded" or
"supplementary" variants that constrain SIP across negation.)

Dr. Lojekyll ALREADY rejects unstratified negation and unstratified
aggregation in ALL modes via the Stratify pass (CLAUDE.md: "unstratified
negation — a negated predicate recursively derived from the negation's
own result (rejected by the dataflow Stratify pass in all modes)";
corpus `evm_func_parse.dr`). So there are two possible outcomes if demand
would break stratification:

  OUTCOME A (the reject fires — SAFE-BUT-REGRESSIVE): the demand-rewritten
  graph is unstratifiable, Stratify's reject fires, and a program that
  COMPILED without demand now FAILS to compile with demand on. That is a
  correctness-preserving outcome (no miscompile) but a usability
  regression: `-demand` would reject programs the default accepts.

  OUTCOME B (the reject does NOT fire but the cross-check would — the
  DANGER): if the demand rewrite produced a graph whose SCC structure
  diverged from the union-find partition, the Stratify.cpp:404-413
  cross-check would ABORT in debug. Under NDEBUG it is stripped — so a
  release build could in principle proceed on a mis-stratified graph.
  This is the silent-miscompile channel the gate exists to close.

THE RESTRICTION (the argument's commitment): **demand-through-negation
and demand-through-aggregation are BLOCKED in the SIP strategy.** The
adornment propagation MUST NOT pass demand INTO a negated subgoal or into
an aggregate's summarized input across a stratum boundary that the
original program placed the negation/aggregate below. Concretely:

  - When the SIP walk over a rule body reaches a negated subgoal `not q`
    or an aggregate `q over …`, it treats `q` as a DEMAND SINK: `q` is
    materialized in FULL (as today), and NO `d_q` demand relation is
    generated from that subgoal. Demand propagation stops at the negation
    / aggregate frontier. The guarded copy `p'` still joins its POSITIVE
    demanded subgoals against `d_p`, but the negated/aggregated subgoal
    reads the full `q` — identical to today's behavior for that subgoal.

  This restriction is SUFFICIENT to preserve the invariant: with no demand
  edge crossing a negation or aggregate, the demand relations `d_·` form
  a graph over POSITIVE (monotone-or-differential-but-non-negated,
  non-aggregated) predicate copies only. A magic-predicate cycle that
  passes through a negation cannot form, because the SIP walk never emits
  a demand edge on the far side of a negation. The demand subgraph's SCCs
  are a SUBSET of the original positive dependency graph's reachability;
  the classic counterexample is structurally excluded.

DIAGNOSTIC when it would break: if the demand pass DETECTS that the only
way to make `p` demand-guarded would require passing demand through a
negation or aggregate (i.e. `p`'s bound columns are only available AFTER
a negated/aggregated subgoal in every SIP order), it does NOT silently
fall back — it emits a CLEAN DIAGNOSTIC ("query `p` cannot be
demand-transformed: its binding pattern requires sideways information
passing through a negation/aggregate; recompile without -demand or bind a
column upstream of the negation"). This is a compile-time reject in the
`-demand` mode, sibling to the existing unstratified-negation reject, and
it fires BEFORE the graph is mutated (so the default-mode compile is
unaffected). The transform never produces a graph that would trip
Stratify.cpp:404-413 — the restriction guarantees the demand edges never
create an SCC the original graph lacked on a negation/aggregate edge.

E-33's sharpened form: the `IdentifyInductions`/`merge_set` cross-check
(Stratify.cpp:387-419) is the always-on debug backstop. The argument's
claim is that with demand-through-negation blocked, a demand copy `p'`'s
SCC induction structure CANNOT diverge from the union-find partition —
because `p'` differs from `p` only by an ADDED positive JOIN input
(`⋈ d_p`), and `d_p`'s producers are demand-source rules with NO edge
back into `p'` except the pivot match (a positive equi-join edge that
respects the same topological order the original graph had). Adding a
positive predecessor edge can only MERGE SCCs monotonically; it cannot
create a negation-on-a-cycle that was not already present. If a future
relaxation lets demand cross a negation, this cross-check is the tripwire
that catches it in debug — which is why the transform must run BEFORE
Stratify (§1.3 point 3).

### (b) Structural distinctness via the demand edge (E-32 restated)

INVARIANT SITE: `InsertSetsOverlap` (View.cpp:1478-1480) + `group_ids`
seeding (Optimize.cpp:423) + JOIN `Equals()` (Join.cpp:467).

Argued in §1.2. Restated as the obligation: a demand copy `p'` and its
unguarded twin `p` must NOT CSE-merge. The transform guarantees this
STRUCTURALLY: `p'`'s top view is a JOIN with `num_pivots = |x̄| ≥ 1` and
an extra joined `d_p`-SELECT child; `p` has neither. `Equals()`
short-circuits on `num_pivots` / children (Join.cpp:467) BEFORE
`group_ids` / `InsertSetsOverlap` is consulted. The E-32 unsoundness
(empty `group_ids` ⇒ MERGEABLE) is therefore never reached for the
demand copy: CSE's `Equals()` returns false at the structural comparison.

CHECK against the keep-last-edge rule (Join.cpp:314-322): the demand
pivot edges are "never removed, even when their value is a known
constant" — the same protection the ⊥c pivot gets (Join.cpp:317-319
names the condition-test pivot explicitly). So canonicalization will NOT
strip the demand pivot and collapse `p'` back onto `p`, even if a
demand-column value is a compile-time constant. The keep-last-edge rule
is exactly what keeps the demand guard alive through the Optimize
fixpoint.

CHECK against the unit-relation CSE rule: the CLAUDE.md invariant says
"CSE never folds a unit SELECT into a non-unit one" and "a JOIN pivot
whose non-user side is a unit relation is never removed." `d_p` is NOT a
unit relation (it has real bound-column rows, arbitrary arity ≥ 1, and is
NOT `is_condition`), so the demand JOIN is an ordinary multi-column-key
JOIN, not a degenerate unit-pivot JOIN. It obeys the general JOIN
canonicalization, not the unit-relation carve-out. (This is the (e)
distinction — §2(e).)

### (c) keep-last-edge + unit-relation rules survive

INVARIANT SITE: Join.cpp:314-322 (keep-last-edge for pivots) + the
CLAUDE.md dataflow invariants block (keep-last-edge, unit-relation, CSE
carve-outs).

  - KEEP-LAST-EDGE: canonicalization "never severs the last input-column
    edge to an incoming view." The demand JOIN ADDS an input-column edge
    (the pivot to `d_p`); it never severs one. Removing the demand edge
    would require removing a pivot, which Join.cpp:314-316 forbids. So the
    rule is preserved and, in fact, is the mechanism protecting the
    transform's own edge.
  - UNIT-RELATION: the transform creates NO new unit relations. Unit
    relations are created ONLY by the zero-arity-predicate desugarer
    (CLAUDE.md: "only the desugarer creates its INSERTs"). `d_p` is a
    normal relation. The invariant "a unit relation contains at most the
    row `(true)`" is untouched because the transform never mints a unit
    INSERT.
  - The "no view is ever its own direct user" invariant (asserted in
    `RelabelGroupIDs`): the demand JOIN's inputs are `p'`-body and
    `d_p`-SELECT, neither of which is the JOIN itself. `d_p`'s producers
    are demand-source rules distinct from `p'`. No self-user edge is
    created. (Obligation (f) argues `d_p` is not a source-less cycle.)

### (d) rewrite-not-evaluator

INVARIANT SITE: the whole DR-IR lowering path (CLAUDE.md: the DR-IR is
"the SOLE authority for the stratum machinery"; the Query→DR→Program→C++
pipeline). Memory: SLDMagic is "a transformation, never an evaluator."

The transform's output is a `QueryImpl` consumed by the UNCHANGED
lowering. No new DR op, no new Program region, no new runtime store, no
new emission template. `d_p` lowers as an ordinary relation (its own
DiffTable if differential, monotone table otherwise — DERIVED by
`TrackDifferentialUpdates` on the rewritten graph). `p'` lowers as an
ordinary guarded JOIN. The demand-guardedness is a STATIC graph property,
resolved entirely at compile time; at runtime there is no demand
interpreter — just the same message-driven incremental database evaluating
a (smaller, guarded) graph. This is the SLDMagic commitment: the "which
instances to materialize" decision is COMPILED IN as graph structure, not
DISPATCHED at runtime. Contrast with a top-down SLD evaluator (which the
memory explicitly forbids): there is no goal stack, no unification at
runtime, no demand queue object. The proof obligation is discharged by
inspection: the transform touches only `lib/DataFlow` (Query graph),
adds nothing to `lib/ControlFlow` or `lib/Runtime`.

### (e) no zero-pivot demand JOIN outside @product

INVARIANT SITE: CLAUDE.md "zero-pivot JOINs appear only under `@product`";
the on-cycle-product reject `ViewSelfReachable` (Build.cpp, ControlFlow
:200/:1152, read this session).

THE HAZARD: an ALL-FREE demanded predicate. If `p` has NO bound columns
under the propagated adornment (every column is free), then the demand
"key" x̄ is EMPTY, and the naive demand JOIN `p' = p ⋈ d_p` would have
ZERO pivots — a cross-product against a nullary `d_p`. A zero-pivot JOIN
outside `@product` violates a hard structural invariant.

THE DECISION (pick, per the lane charge): **an all-free demanded
predicate gets NO demand edge at all — it is a DEMAND-INERT predicate
materialized in full, exactly as today.** Rationale: demand with an empty
key conveys no information (every ground instance is "demanded" — the
demand relation is the single nullary token, equivalent to "yes,
something wants `p`"). Guarding `p` by "is anything demanded at all"
provides no materialization reduction (it's all-or-nothing, and if the
query reaches `p` at all the answer is "all"), while introducing exactly
the zero-pivot-JOIN structural violation. So the transform SKIPS the
guard for all-free predicates: `p' = p` (no rewrite), `p` materializes
fully. This is strictly correct (a superset-free guard is a no-op) and
avoids the zero-pivot JOIN entirely.

  (Rejected alternative — a UNIT demand relation `d_p = ⊥` gating `p` on a
  single presence token: this would be a legitimate zero-pivot @product,
  but it is pure overhead — a global "is p demanded" boolean that, once
  true, admits every row of `p`. It buys nothing and costs a product arm.
  Not worth it; the SKIP is cleaner.)

So the invariant is preserved by RESTRICTION: the demand guard is emitted
ONLY when |x̄| ≥ 1, i.e. exactly when it produces a legal ≥1-pivot JOIN.
No demand JOIN is ever zero-pivot, so none needs `@product`, so the
`@product`-only invariant is never approached.

### (f) every demand relation has a real source

INVARIANT SITE: CLAUDE.md "a source-less forwarding cycle is
unsatisfiable, collected by dead-flow elimination"; the "every
inter-view dependency is a column edge" invariant; `RemoveUnusedViews`
(Build.cpp:2550/2576/2579).

THE HAZARD: if `d_p` had no producing rule, it would be an empty relation
that (i) makes `p'` produce nothing (a silent under-derivation — a
correctness bug) and (ii) if it participated in a cycle with no external
source, forms a source-less forwarding cycle that dead-flow elimination
collapses (deleting `p'` and hence `p`'s answers).

THE ARGUMENT: every `d_p` the transform introduces has AT LEAST TWO real
source classes, both genuine column-edge producers:

  1. THE ROOT SEED: the `#query` itself. The demanded query `p` has bound
     columns supplied by the caller — the query entry seam
     (Build.cpp:393-411, ControlFlow: the bound `col_indices` build a scan
     index) is the ROOT source of `d_p`'s rows. In Query-graph terms, the
     transform materializes the query's bound-column projection as the
     seed INSERT into `d_p`. This is a real INSERT view, not a phantom.

  2. THE DEMANDING SUBGOALS: for every rule `q :- …, p(x̄, ȳ), …` where a
     demanded `q` uses `p`, the SIP-available bindings for x̄ produce a
     rule `d_p(x̄) :- <those bindings>` — a real projection of a real
     (already-sourced) subgoal. This is the recursive demand propagation:
     `d_q` sources `d_p` sources `d_{p'}` … all grounded, ultimately, in
     the root seed (1).

Because every `d_p` traces back through real column-edge producers to the
`#query` root seed, NO `d_p` is source-less. Dead-flow elimination will
NOT collapse it (it has a live INSERT source). If — as a
transform-correctness self-check — a `d_p` were ever emitted with no
producer, `RemoveUnusedViews` would delete `p'` and the query would
under-derive; the transform must (and this argument commits it to)
emit the root seed FIRST, before any `p'` guard, so `d_p` is never
transiently source-less. A design-time validator (analogous to the DR-IR
census) should assert, post-transform, that every introduced `d_p` has
≥1 producing INSERT reachable from a `#query` seed — a cheap graph
walk, the (f) tripwire.

--------------------------------------------------------------------------
## 3. THE MEASURE-FIRST WITNESS

The COST referee (memory: perf-guiding-oracles — McSherry/COST honesty
referee) demands a workload where demand materializes MEASURABLY fewer
instances than full evaluation. The canonical shape is bound-source
transitive closure over a large sparse graph.

### 3.1 The witness `.dr` (specification)

`demand_tc_witness.dr` (to be authored under `bench/` or
`tests/OptDiff/cases/`, per §4/§5):

```
#message edge_2(u64 From, u64 To)          ; the base graph, injected as batches
#local path(u64 From, u64 To)              ; the transitive closure

path(F, T) : edge_2(F, T).                 ; base
path(F, T) : path(F, M), edge_2(M, T).     ; right-linear recursion

#query reachable_from(bound u64 From, free u64 To) : path(From, To).
```

The demand-benefit story: without demand, `path` is the FULL transitive
closure — on a sparse random graph with N nodes and average out-degree d,
`|path|` is O(N²) in the worst case (a long chain or a dense reachable
component makes it quadratic). WITH demand and a bound `From`, only the
nodes REACHABLE FROM the single queried source materialize:
`d_path(From)` seeds one source, and the guarded `path'` computes only
the forward-reachable frontier — O(reachable-set-size · d), which on a
sparse graph with bounded reachable components is LINEAR-ish in the
answer size, not quadratic in N.

The demand rewrite of this program (sketch, to confirm the shape):
  - `d_path(bound From)` seeded from the `#query`'s bound column.
  - The recursive rule's SIP: `path(F, M), edge_2(M, T)` — `F` is bound
    (from `d_path`), so the recursive subgoal `path(F, M)` demands
    `d_path(F)` (already have it), and `edge_2(M, T)` is demand-inert
    (a base message, materialized as delivered). The guarded
    `path'(F,T) = path-body ⋈ d_path(F)` on the `From` pivot (|x̄|=1, a
    legal 1-pivot JOIN — §2(e) satisfied, §2(b) structural distinctness
    satisfied).
  - `d_path` is NON-recursive here (it is just the singleton source), so
    (a) is trivially satisfied: no demand cycle, no negation, no
    aggregate.

### 3.2 What the COST referee measures + acceptance bar

BENCH FAMILY: this is a NEW workload, not a Q5 variant (Q5 is a fixed
chain; demand is a new-feature path — the seed's own §14.3-P2 prediction
that "Q5 neutral" holds for the family). It slots into `bench/` as a new
runspec with a SCALE KNOB (N = node count, d = out-degree, single bound
source), following the `bench/README.md` methodology and
`bench/BASELINE.md` acceptance discipline.

METRIC: the honest COST metric is INSTANCES MATERIALIZED (rows inserted
into `path`'s table) and the derived wall-time, compared demand-ON vs
demand-OFF at matched (N, d, source). The runtime counter seam
(`-DDRLOJEKYLL_BENCH_COUNTERS`, Runtime/BenchCounters.h, a suite-verified
no-op when off, CLAUDE.md) is the mechanism — count table inserts on the
`path` table in both modes.

ACCEPTANCE BAR (pre-registered): on a sparse random graph with N = 2^k
nodes (sweep k), out-degree d = O(1), single bound source with a bounded
reachable component:
  - demand-OFF materializes Θ(N²)-trending `|path|` (superlinear in N);
  - demand-ON materializes O(reachable · d), FLAT in N once the reachable
    component size is fixed;
  - the crossover (demand-ON beats demand-OFF in BOTH instance count AND
    wall time) must appear by N = some k₀ and WIDEN with N.
  If demand-ON does NOT materially reduce instances (e.g. because the
  reachable component is the whole graph — a dense witness), the witness
  is MIS-DESIGNED, not the transform; the bar requires a genuinely sparse
  reachable set. The referee REJECTS a witness where demand and full
  evaluation materialize within (say) 2× of each other — that would be an
  honesty failure (claiming a benefit the workload doesn't exhibit).

CORRECTNESS co-gate: `demand_tc_witness.dr` also gets an OptDiff golden +
`.batches` oracle/monotone referee (§5) so that demand-ON and demand-OFF
produce the SAME query ANSWERS (the demand guard must not change WHAT
`reachable_from` returns, only how much of `path` is computed to get
there). This is the non-negotiable correctness half of measure-first:
faster is worthless if it's wrong.

--------------------------------------------------------------------------
## 4. SCOPE RECOMMENDATION FOR THE OWNER

RECOMMENDATION: **DESIGN-ONLY this epoch. P3 does NOT ship code this
epoch; this argument is recorded, the witness is authored and measured
(a bench spike only, no transform), and implementation is the FIRST P0 of
the FOLLOW-ON epoch.** Reasons, honest:

  1. The argument SURVIVES review for the POSITIVE, non-recursive,
     ≥1-bound-column case (the §3 witness). But the negation/aggregation
     restriction (§2(a)) is a genuine SCOPE CUT, not a full solution — it
     BLOCKS demand-through-negation rather than solving it. A production
     demand transform eventually wants the supplementary-magic /
     stratified-magic machinery to handle negation, which is a
     substantial design surface not yet worked. Shipping the restricted
     transform is defensible, but it is a NARROW first slice, and the
     epoch's stated stage-1 core is P2's config-column aggregates + keyed
     instances (§14.0 recommendation (c)→(b)), which is the prerequisite
     substrate. Demand is "the payoff; the keyed-instance substrate is the
     prerequisite the recorded plan says to build first" (§14.0:1896-1898).

  2. THE CHURN FINDING (37 bound-query goldens, §1/§5) means shipping the
     transform this epoch forces a golden-policy negotiation and a
     re-bless of 37 cases (or a mode-gated transform that leaves default
     goldens untouched — the recommended bring-up posture). That is a
     large blast radius to take on in the SAME epoch as the P2 op family.
     Design-only defers that negotiation to when the transform is the
     epoch's primary deliverable.

THE HONEST INTERACTION WITH STAGE (b) — CAN (b) LAND WITHOUT (a)?

The seed frames demand frontiers as "the instance family's input"
(§14.2(B): "demand frontiers as the input, published instance rows as the
output"; §14.3-P2: "demand frontiers feed the §14.2(B) instance family's
input"). This suggests (b) NEEDS (a). The honest analysis:

  - (b) keyed nested instances CAN land WITHOUT the full demand transform,
    IF the instance family's demand input is supplied by a MORE RESTRICTED
    source than the general SLDMagic rewrite. Specifically: an aggregate
    keyed on (group ++ config) is ALREADY a keyed instance whose "demand"
    is the set of live groups in its summarized input (the
    net-additions frontier the R3 family already provisions — CLAUDE.md:
    "its monotone message input is provisioned a net-additions frontier
    as a cut successor"). That frontier is a DEGENERATE demand — "these
    groups exist, instantiate a cell for each" — with NO adornment
    propagation, NO magic rule, NO guarded copy. So config-column
    aggregates (P2 stage c) and the keyed-instance store (P2 stage b) can
    land driven by that existing frontier, with the DEMAND TRANSFORM (a)
    deferred: the instance family gets its input from the summarized
    relation's frontier, not from a magic-set-derived `d_p`.

  - What (b) CANNOT do without (a) is realize the FULL payoff — a demand-
    DRIVEN subgraph where only the QUERIED instances materialize. Without
    (a), (b) instantiates a cell for EVERY group present in the input
    (eager, like today's aggregates), not only the demanded ones. That is
    still a real, useful op family (it generalizes StateCellStore to hold
    sub-relations); it just isn't demand-pruned yet.

  CONCLUSION: (b) lands MEANINGFULLY without (a) — as an eager keyed-
  instance family driven by the existing net-additions frontier — but the
  DEMAND PRUNING that makes it a "subgraph" in the SLDMagic sense is (a),
  which this recommendation defers. The two are separable; the seed's
  "demand frontiers feed the instance family" is aspirational for the
  full payoff, not a hard prerequisite for the substrate. Owner should
  scope P2 as the eager keyed-instance family (input = existing frontier),
  and treat this P3 argument as the RATIFIED DESIGN for the follow-on
  epoch's demand-pruning slice.

If the owner nonetheless wants P3 IN-SCOPE this epoch: it is admissible
ONLY as the restricted positive-non-recursive slice (§3 witness), MODE-
GATED off by default (so the 37 goldens are untouched at default), with
the negation reject (§2(a)) as a hard clean diagnostic, and the measure-
first bar (§3.2) as a landing gate. That is a defensible narrow ship —
but it competes for the epoch's budget with the P2 substrate the plan
prioritizes.

--------------------------------------------------------------------------
## 5. PRE-REGISTERED PREDICTIONS (if scoped in)

These are the predictions for the RESTRICTED, MODE-GATED demand slice, if
the owner scopes P3 in this epoch (or for the follow-on epoch's P0 if
design-only).

### 5.1 The churn claim, VERIFIED (the seed's prediction is FALSIFIED)

The seed (§14.3-P3 has no explicit churn line, but the lane charge cites
"existing 164 zero churn since the transform only fires on bound
queries — VERIFY that claim"). VERIFIED THIS SESSION:

  **37 of the 164 corpus cases carry a `bound` column in a `#query`.**
  (Method: `grep -lE "#query [A-Za-z0-9_]+\([^)]*bound"` over
  tests/OptDiff/cases/*.dr; count = 37.) The list:

    average_weight, booleans, booleans_diff, cf13_6, cf14_5, cf15_4,
    cf15_5, cf15_6, cf16_3, cf16_6, cond_in_induction_deep,
    cond_multi_setter, disassemble, fibonacci, fibonacci_iterative,
    fixpoint_stress_1, force, kcfa_tiny, kcfa_tiny_merged, kv_in_scc_1,
    kvindex_1, kvindex_2, kvindex_3, kvindex_4, pairwise_average_weight,
    select_5, tower_of_compares, transitive_closure, transitive_closure2,
    transitive_closure3, transitive_closure4, transitive_closure5,
    transitive_closure_diff, transitive_closure_diff2,
    transitive_closure_lazy,
    transitive_closure_multiple_clause_bodies, two_inductions.

  A bound `#query` today MATERIALIZES THE FULL relation and builds a scan
  index over the bound columns (Build.cpp:393-411, ControlFlow, read this
  session). The demand transform, ON, changes which instances of the
  demanded predicate materialize for exactly these cases — so it DOES
  touch existing goldens whenever it fires. THE SEED'S "existing 164 zero
  churn" PREMISE IS FALSE. The prediction changes accordingly:

  - PREDICTION (mode-gated bring-up): the transform is `-demand`-gated and
    DEFAULT-OFF. Under the DEFAULT mode (all 4 existing OptDiff modes are
    demand-off), the 164 goldens are BYTE-IDENTICAL (zero churn) — the
    transform is a no-op because the pass is not run. This is the ONLY way
    to preserve the existing net; a default-on transform would re-bless 37
    cases.
  - PREDICTION (a 5th "demand-on" mode, IF added): running demand-on over
    the 37 bound-query cases would produce DIFFERENT (smaller-
    materialization) output for those cases — NOT byte-comparable against
    the demand-off golden. Those 37 would need their OWN demand-on
    goldens, oracle-refereed to prove SAME ANSWERS (the demand guard
    changes materialization, never the query answer). The other 127
    (free-only or query-less) cases would be byte-identical demand-on vs
    off (the transform no-ops on them). So a demand-on mode is NOT a
    free cross-mode-agreement variant; it is a semantically-distinct mode
    for 37 cases and identical for 127.

### 5.2 Suite growth

  - +1 corpus case minimum: `demand_tc_witness.dr` (§3) + its `.batches`
    sidecar + oracle/monotone goldens (the R3-corpus mold — CLAUDE.md).
    Possibly +1 negation-reject case (`demand_through_negation_1.dr`,
    all-4-modes-diagnostic, joining the runall.sh expected-diagnostic list
    alongside `evm_func_parse` — the §2(a) reject witness).
  - Suite grows 164 → 165 or 166. New cases oracle-blessed ONLY (never to
    make a red case green — CLAUDE.md blessing discipline).

### 5.3 Golden policy

  - Existing 164: ZERO churn under the default (demand-off) modes — the
    transform is mode-gated and does not run. This is VERIFIED-safe only
    because the transform is default-off; it is NOT safe if default-on
    (§5.1).
  - New cases: oracle-blessed via `runall.sh --bless` after review, per
    §7 policy. The demand witness's demand-on vs demand-off answer
    agreement is the oracle referee's job.
  - IF a demand-on mode is added: the 37 bound-query cases get demand-on
    goldens (new, oracle-refereed for answer-identity), a SEPARATE bless
    from their demand-off goldens.

### 5.4 The house bet

  - Per the E-1..E-26 precedent (every first-time invariant instrument
    finds a real divergence), the FIRST end-to-end demand rewrite of a
    real recursive case (the §3 witness) will likely surface a divergence
    — most probably in the SIP-order determination (which columns are
    "bound" at which point in a body) or in the demand-source seeding
    (obligation (f) — a `d_p` transiently source-less across the rewrite
    order). PRE-REGISTERED: a FINDINGS entry iff the demand rewrite
    produces a wrong answer (oracle disagreement) or a source-less `d_p`
    (dead-flow collapse) on the witness. The measure-first bar (§3.2) and
    the oracle answer-identity gate are the tripwires.

  - Q5 NEUTRAL (the seed's §14.3-P2 prediction holds): demand is a
    new-feature path; the Q5 chain has no bound query and is untouched
    (Q5 spot stays flat, per the §0 baseline discipline).

--------------------------------------------------------------------------
## APPENDIX: code anchors read this session (falsification map)

  - View.cpp:1478-1480 — InsertSetsOverlap returns MERGEABLE on empty
    group_ids (the E-32 unsoundness root).
  - Optimize.cpp:410-434 — group_ids seed ONLY on JOIN/AGG/KVINDEX
    (ClearGroupIDs); everything else empty (the E-32 gap).
  - Build.cpp:1450-1512 (DataFlow) — ApplyPositiveConditionTest, the ⊥c
    1-pivot presence-join precedent the demand edge copies; :1457-1459 the
    "deliberately NOT a constant" pivot comment.
  - Build.cpp:1483-1489 — the pivot's out_to_in carries BOTH joined
    columns (the demand JOIN's per-key pivot pattern).
  - Join.cpp:314-322 — keep-last-edge for pivots; names the ⊥c
    condition-test pivot as the protected precedent.
  - Join.cpp:467 — JOIN Equals() short-circuits on num_pivots/children
    (structural distinctness before group_ids).
  - Stratify.cpp:122 — QueryImpl::Stratify entry.
  - Stratify.cpp:387-419 — the induction cross-check (inductive==same_scc,
    stratum order, merge_set==SCC partition, multi-view-SCC has inductive
    MERGE or IO seam) — E-33's always-on debug backstop.
  - Build.cpp:2517-2616 (DataFlow) — Query::Build pipeline order
    (Simplify → ConnectInsertsToSelects → Optimize → IdentifyInductions →
    Stratify); the transform slots at :2566, before Optimize.
  - Build.cpp:200/1152 (ControlFlow) — ViewSelfReachable (on-cycle product
    reject), the (e) neighbor invariant.
  - Build.cpp:383-411 (ControlFlow) — BuildQueryEntryPointImpl: a bound
    #query materializes the full table + a bound-column scan index
    (the churn-finding root; the (f) root-seed source).
  - Corpus grep — 37/164 cases with a bound #query column (§5.1 list).
