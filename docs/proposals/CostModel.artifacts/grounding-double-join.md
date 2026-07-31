# GROUNDING — THE MAGIC-SETS DOUBLE JOIN, QUANTIFIED ON THE REAL IR

> **Status.** Written 2026-07-30 at tip **eecea847** by ONE session (orchestrator,
> inline — the cheap code-true anchor front-loaded BEFORE the design fleet). Grounded
> in live `-df-out` / `-rel-out` / `-ir-out` / `-cpp-out` dumps of three corpus
> witnesses under `normal` / `-demand` / `-demand-instance`. This is the empirical
> anchor for the COST-MODEL instrument (docs/proposals/CostModel.md, forthcoming):
> the instrument's FIRST job is to adjudicate the finding recorded here.
> Companion: the demand cost model [[demand-cost-model]] / seed §7.

## §0 THE QUESTION (owner, 2026-07-30)

> "for the whole magic sets thing with the double join, just because magic sets does
> the double join, it wasn't obviously justified in any way to me? … we have the
> dataflow graph — that isn't datalog exactly, so maybe magic sets as done there, or
> maybe you did it on the rel, but either way, you should maybe question things if
> they start introducing this explosion … and we need a way to measure / estimate
> this explosion."

The textbook justification ("magic sets does it") does not automatically carry to
*this* IR (a dataflow graph, not Datalog clauses). This artifact answers, on the
real compiler output: (1) is the double join real, (2) where does it originate and
does it survive lowering, (3) is it justified, (4) how big is the explosion.

## §1 THE TWO GUARDS (code)

`lib/DataFlow/Demand.cpp` `ApplyDemandTransform` mints, per bound `#query` on
relation `p` at adornment `α`:

- **Step 5 — the subgoal PUSH-DOWN guard** (`MintGuardJoin`, `d_p ⋈ p` at each SIP
  site). The *essential* magic-sets mechanism: only demanded rows of `p` materialize.
  **This is what makes demand prune. Justified.**
- **Step 8 — the QUERY-PROJECTION guard** (`kQueryProjection` / `kRawSeed`,
  Demand.cpp:1086-1127): a fresh receive-projection TUPLE (`DEMAND-RAW-SEED`) joined
  against the query's OWN read of `p` on the raw seed (`raw_seed ⋈ q_read`). It
  guards the query's own read separately from body reads. **This is the one to
  question.**

## §2 THE EXPLOSION, QUANTIFIED (census over three witnesses)

Node/op census from the dumps (`grep '^join '` on `.df`; `census:` line on `.rel`):

| witness (shape)                          | config   | .df joins | .rel kEagerJoin / kJoinEmit | .df tables |
|------------------------------------------|----------|-----------|-----------------------------|------------|
| mono (1-hop, message-rooted, 1 adorn)    | normal   | **0**     | 0 / 0                       | 1          |
| mono                                     | -demand  | **2**     | 4 / 2                       | 3          |
| multi-adorn (1-hop, 2 adornments)        | normal   | **0**     | — / —                       | 0          |
| multi-adorn                              | -demand  | **4**     | — / —                       | 4          |
| tc (right-linear recursive, 1 adorn)     | normal   | **1**     | — / —                       | 2          |
| tc                                       | -demand  | **4**     | — / —                       | 5          |

Pattern: **`-demand` adds 2 joins per adornment**; `normal` answers the bound query
with **zero joins** (a single index-probe on the materialized relation's bound
column). For the mono witness the double join **persists all the way to codegen**:
`kJoinEmit=2` = two real `TABLEJOIN`s emitted to C++. Neither the dataflow optimizer
(CSE, canonicalization, dead-flow) nor the Rel lowering removes it.

## §3 THE REDUNDANCY, PROVEN STRUCTURALLY (mono.demand.df)

The 1-hop witness `neighborhood(bound Start, free Node) : edge(Start, Node)` over
`edge(From,To) : add_edge(From,To)`, under `-demand` (optimized):

    tuple.4 (c8)           = the demand relation d_neighborhood_bf  (key = bound Start)
    join.6:  pivot From <- d.c8, edge.From ; out To <- edge.To   -> tuple.3 (Start,Node)
    join.7:  pivot Start <- d.c8, tuple.3.Start ; out Node <- tuple.3.Node -> tuple.5 -> pub

`join.6` (the push-down guard) already pivots the demand relation against
`edge.From`, so **every row of its output `tuple.3` has `Start ∈ d`**. `join.7` (the
projection guard) then joins the SAME demand relation `d.c8` against `tuple.3` on
that SAME column `Start` — a set already ⊆ `d` on `Start`. **`join.7` cannot remove
a single row: it is an identity join.** It adds one intermediate table + one join
pass over the already-demanded slice and produces nothing.

The same argument holds for tc (From-preserving recursion keeps `d_path` = exactly
the query's From seeds; every derived `path` row has `From ∈ d_path` from the body
guards) and for each adornment of multi-adorn. **For the entire currently-supported
demand slice (single-adornment, From-preserving), the query-projection guard is
provably redundant.**

## §4 WHY IT EXISTS (the honest caveat — NOT "just delete it")

The projection guard is a general magic-sets construct: it guards the query's own
read of `p`, which can differ from body reads when the query reads `p` at a
different adornment, over a multi-clause query, or where transitively-demanded
intermediate keys diverge from the query's own seed (e.g. a recursion that does NOT
preserve the bound column — outside today's slice). Demand.cpp:1107-1112 records
that on non-recursive witnesses CSE folds `raw_seed` into `d_reader` (GT-3), which is
exactly why the two joins end up chained on one demand relation and the second
collapses to identity. So the correct claim is **conditional**:

- **Redundant** on the supported slice (single-adornment, From-preserving) — proven above.
- **Potentially load-bearing** off-slice (adornment-divergent / non-preserving
  recursion / multi-clause). Characterizing the exact boundary is the cost model's job.

## §5 WHERE IT LIVES (answering "on the rel?")

Origin: the **dataflow transform** (`ApplyDemandTransform` mints both JOINs). It
propagates **unchanged** through the Rel/DR-IR (`kEagerJoin`/`kJoinEmit`, the census
above) into generated C++. The Rel neither introduces nor removes it. So a fix — if
we drop step-8 where provably redundant — lands in the dataflow transform, or as a
cost-guided rewrite that could act on either layer. The optimizer is blind to it
today because recognizing the identity join needs **subset/cardinality** reasoning
the canonicalizer does not have — precisely the reasoning a cost model supplies.

## §6 THE MANDATE FOR THE COST MODEL

This finding is the instrument's first acceptance test. The cost model must be able to:

1. Assign symbolic cardinalities to inputs (N = |add_edge|, D = |distinct demanded
   keys|, selectivity σ per join key) and propagate them simulation-style through the
   IR to a per-config total-cost expression.
2. Represent the two guard joins as two cost terms and prove **term-8 ⊆ term-5**
   (redundant) on the supported slice — i.e. reproduce §3 algebraically.
3. Compare configs: show `normal` (index-probe, ~σ·N per key, no forced 2nd
   materialization) vs `-demand` (D-channel + forced edge materialization + 2 joins)
   and quantify when `-demand` costs MORE than it prunes (seed §7: shallow /
   message-rooted shapes never pay).
4. Flag when a "do less" feature actually does MORE — the general mandate.

## §7 REPRO

    DR=build/debug/bin/drlojekyll
    $DR tests/OptDiff/cases/demand_neighborhood_mono_witness.dr        -df-out mono.normal.df -rel-out mono.normal.rel
    $DR tests/OptDiff/cases/demand_neighborhood_mono_witness.dr -demand -df-out mono.demand.df -rel-out mono.demand.rel
    # step-5 vs step-8: grep 'join\.' mono.demand.df ; census on mono.*.rel

Staged dumps: `mono.normal.df`, `mono.demand.df`, `mono.normal.rel`,
`mono.demand.rel` in this directory.
