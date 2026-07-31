# COST MODEL — THE QUERY-ANCHORED DESIGN

> **Lane.** IR-HOME = the **Query dataflow graph** (`lib/DataFlow/Query.h`:
> SELECT/TUPLE/JOIN/MERGE/CMP/MAP/NEGATE/AGG/KVINDEX/INSERT). Cost is computed at
> the layer where the **demand transform** (`lib/DataFlow/Demand.cpp`) and the
> **dataflow optimizer** (`lib/DataFlow/Optimize.cpp`) act, so the instrument can
> referee a transform decision — *should step-8 be minted?* — on the same objects
> the transform manipulates, **before** any lowering to Rel/CFG/C++.
>
> Written 2026-07-30, tip `eecea847`. Grounds the double-join finding
> ([[grounding-double-join]]) and the demand cost model ([[demand-cost-model]]).

---

## §1 WHY THE QUERY LAYER IS THE COST HOME

The owner's open question — *"maybe magic sets as done there, or maybe you did it
on the rel?"* — is the pivotal axis. My position: **cardinality/selectivity cost
belongs on the Query graph; the Rel layer contributes a scheduling/differential
*multiplier*, not the cost's source of truth.** Four reasons.

1. **The decision layer is the Query layer.** `ApplyDemandTransform` mints both
   guard JOINs as Query nodes (`MintGuardJoin`, `Demand.cpp:162`; step-8 at
   `Demand.cpp:1092-1127`). To answer *"is minting step-8 worth it?"* the referee
   must speak in the vocabulary the minter uses — JOIN pivots, column edges,
   demand relations — and it must be callable **before** the node exists. A cost
   model living on Rel can only judge the *already-lowered* op census
   (`kEagerJoin`/`kJoinEmit`), i.e. post-mortem, never prospectively.

2. **Cardinality is a relational-algebra property.** Rows, distinct keys, join
   selectivity, antijoin survival — these are properties of SELECT/JOIN/MERGE/
   NEGATE, which *are* the Query nodes. The Rel op vocabulary
   (`kSeedFold`/`kClaimDrain`/`kCommitSweep`/`kFixpointFire`, `mono.demand.rel`)
   is about **round/phase/effect placement**, not about how many rows flow. Rel
   answers "when and in what differential sign is this work done"; Query answers
   "how much work is there." The cost algebra needs the latter.

3. **The redundancy proof needs column provenance.** Proving step-8 redundant
   (§3 of the grounding) is a **keyset-subset** fact: the pivot column already
   carries "∈ demand" membership from step-5. That membership is a property of
   the **column dependency graph** — `JOIN::out_to_in`, `TUPLE::input_columns`
   (`Query.h:421`, `789`, `781`) — which is native to the Query layer and
   *erased* by the time we reach Rel's typed DRVecs. The grounding §5 says
   plainly the canonicalizer is "blind to it today because recognizing the
   identity join needs subset/cardinality reasoning" — that reasoning is a Query
   analysis.

4. **Recursion is legible here.** The Query graph carries `InductionInfo`
   (`Query.h:219`), `group_ids` (`Query.h:458`), per-view `depth`/SCC index
   (`Query.h:493`, `568`), and `induction_info->cyclic_views` (`Query.h:243`) —
   everything needed to name an SCC and attach a closure-size symbol. Rel's
   `rounds:` block is the *lowering* of exactly this; costing on the source keeps
   one authority.

**Answering "the graph is post-transform."** The guard JOINs are already nodes,
so the referee cannot judge step-8 by its *absence*. Instead it does two things.
(a) **Post-hoc:** cost the built graph as-is and evaluate a *redundancy predicate*
(§4) on each guard JOIN — `σ_J = 1 ⇒ ΔPrune_J = 0` flags a pure-overhead node
without needing to un-mint it. (b) **Prospective:** the same keyset-provenance
analysis (§4) is a pure function of the pre-step-8 subgraph and is **callable
from inside `ApplyDemandTransform`** right before `Demand.cpp:1104` — the minter
asks "does `q_read`'s `p_bound` already witness the demand side?" and skips the
mint when yes. The cost model is thus both a *validator* of the current graph and
a *guard* the transform can consult. **Two-layer relationship:** cost is computed
on Query; Rel folds back a per-view differential/scheduling coefficient κ (§7) as
an annotation — Query stays the authority, Rel sharpens the constant.

---

## §2 THE COST ALGEBRA — SYMBOLIC DOMAIN

### §2.1 The semiring

Costs and cardinalities live in **𝕊 = ℚ≥0-coefficient multivariate polynomials**
over a base symbol set

    B  =  { N_r  : |materialized input relation r| }          input sizes
        ∪ { D_a  : |demand relation for adornment a|,  D_a≤N } demanded-key counts
        ∪ { K_r,c: distinct values of column c of relation r } key-domain sizes
        ∪ { C_S  : fixpoint (closure) cardinality of SCC S }   recursion sizes
        ∪ { R_S  : rounds-to-quiescence of SCC S }             recursion depth
        ∪ { Δ_r  : per-batch delta size feeding relation r }   differential

with **selectivity coefficients** σ, δ, φ ∈ (0,1]∪ℚ≥0 as free rational
parameters (join selectivity, set-dedup factor, functor fan-out). Addition = `+`,
multiplication = `·`, both the ordinary polynomial operations; 0 and 1 are the
identities. This is a commutative semiring; every node cost is a polynomial in B
with σ-parameters as coefficients.

### §2.2 Two quantities per view

For each view `v` the model computes

- **`Card(v) ∈ 𝕊`** — the expected output cardinality (rows leaving `v`).
- **`Cost(v) ∈ 𝕊`** — the work to *produce* `Card(v)` from `v`'s inputs.

The **program cost** for a config is `Σ_v Cost(v)`. Configs are compared by
**symbolic dominance** under the sidecar's `assume` constraints (§6): `e₁ ≤ e₂`
iff `e₂ − e₁` normalizes to a monomial-sum with all-nonnegative coefficients,
*or* is provable from the linear assumptions `D_a ≤ N_r`, `σ ≤ 1`, `C_d ≤ C`
by coefficient-wise elimination. (Decidability caveat in §9.)

### §2.3 The distribution over common keys

The owner's *"N·M or 0.5N·0.7M"* is the **JOIN cardinality rule**. For a JOIN of
views A,B on pivot column-set P:

    Card(A ⋈_P B)  =  σ_{A,B,P} · Card(A) · Card(B)

where `σ_{A,B,P} ∈ (0,1]` is the **join-key selectivity**. Its meaning, made
precise so the sidecar can supply it:

    σ_{A,B,P}  =  ( Σ_{k∈dom(P)} a_k · b_k )  /  ( |A| · |B| )

with `a_k`,`b_k` the per-key row counts on each side. Special cases the sidecar
can name directly: a **functional/foreign-key** join (each A-row matches ≤1
B-row) gives `σ = 1/|B|`, i.e. `Card = |A|`; a **cross-product** (`num_pivots==0`,
the `@product` shape, `Query.h:791-793`) gives `σ = 1`, `Card = |A|·|B|`; the
owner's `0.5N·0.7M` is the case where only a fraction of each side carries a
joinable key. **This single coefficient is where data statistics enter** — the
model is parametric in σ, predictive only once σ is supplied (§9).

---

## §3 PER-OP COST FUNCTIONS (grounded)

Each rule is stated as `Card` then `Cost`, grounded in a Query.h node class. The
**Cost** column encodes the **house index model** (`Runtime/Table.h`): a
bound-column hash index turns a scan `O(|table|)` into a probe `O(matches)`; a
JOIN pivot *always* materializes as a hash index on the pivot, so a JOIN's cost
is `driving-side probes + emitted matches`, never `|A|·|B|` unless it is a
cross-product.

| node (`Query.h`)              | `Card(v)`                              | `Cost(v)` (index model)                    |
|-------------------------------|----------------------------------------|--------------------------------------------|
| **SELECT** `QuerySelectImpl` (`:668`) — read of relation/message | `N_r` (whole table) *or* `σ·N_r` if a bound-column probe | `0` if ingested elsewhere; `Card(v)` for a scan; `matches` for an index probe |
| **TUPLE** `QueryTupleImpl` (`:703`) — project/forward | `δ · Card(in)` (`δ≤1` set-dedup) | `Card(in)` (one forwarding pass) |
| **CMP** `QueryCompareImpl` (`:908`) — filter | `σ_cmp · Card(in)` | `Card(in)` (one test per input row) |
| **MAP** `QueryMapImpl` (`:796`) — functor, `num_free_params` (`:818`) | `φ_f · Card(in)` (`φ_f` = free-var fan-out) | `Card(in)` (one functor call/row) |
| **MERGE** `QueryMergeImpl` (`:862`), `merged_views` (`:905`) | `δ · Σ_k Card(in_k)` | `Σ_k Card(in_k)` (union pass) |
| **JOIN** `QueryJoinImpl` (`:755`), `num_pivots` (`:793`) | `σ_{A,B,P}·Card(A)·Card(B)` (§2.3); cross-product if `num_pivots==0` | `Card(drive) + Card(out)` (probe pivot index + emit) |
| **NEGATE** `QueryNegateImpl` (`:950`) — antijoin; `is_never` (`:968`) | `(1−π)·Card(in)`, `π`=present-fraction of negated view | `Card(in)` (one antijoin probe/row) |
| **AGG / KVINDEX** `QueryAggregateImpl` (`:826`) / `QueryKVIndexImpl` (`:731`) | `G` = distinct `group_by_columns` (`:850`) tuples | `Card(in)` (one fold/row) |
| **INSERT** `QueryInsertImpl` (`:974`) — sink into `%table` | — | `Card(in)` (materialize) |

Two rules earn their keep for this instrument:

- **JOIN cost is asymmetric.** `Cost = Card(drive) + Card(out)` — probe the pivot
  index with the driving side, emit matches. This is why **normal mode answers a
  bound query with ZERO joins** (`mono.normal.rel`, `kEagerJoin=0`): the bound
  query column is an **index probe** on the materialized relation, cost
  `σ·N = matches`, no intermediate table. It is also why the guard JOINs are the
  *added* cost under `-demand` (`mono.demand.rel`, `kEagerJoin=4`,`kJoinEmit=2`).

- **Cross-product guard.** `num_pivots==0` (`Query.h:793`) forces `σ=1`,
  `Card=|A|·|B|` — the explosion the instrument exists to flag. The core invariant
  "zero-pivot JOINs appear only under `@product`" (CLAUDE.md) means every such node
  is a deliberate blow-up the model surfaces immediately.

---

## §4 PROPAGATION + THE KEYSET-PROVENANCE LATTICE

### §4.1 Cardinality propagation

`Card` is computed by a **single forward pass in view-topological order** (the
Query graph is a DAG modulo induction back-edges; `depth`/SCC index at
`Query.h:493,568` give the order). Each node applies its §3 `Card` rule to its
inputs' `Card`. SELECT of an input relation seeds a base symbol `N_r`; SELECT of a
demand receive seeds `D_a` (grounded: `select.1 (c3)` over `demand__neighborhood_bf/1`,
`mono.demand.df:7`).

### §4.2 The recursion / fixpoint case

An SCC is a `MERGE` cycle: `induction_info != null` with non-empty
`cyclic_views` (`Query.h:243,273,575`), or `ViewSelfReachable` (CLAUDE.md). For
an SCC `S`:

1. Assign the recursive relation a **closure symbol** `C_S` (its fixpoint
   cardinality) and a **round symbol** `R_S`.
2. Model **semi-naïve** evaluation: round `r` joins the frontier `Δ_r` against the
   recursive relation via `S`'s back-edge JOIN. The per-round cost is the §3 JOIN
   cost with `Card(drive)=Card(Δ_r)`; the frontier recurrence is
   `Card(Δ_{r+1}) = σ_back · Card(Δ_r) · fan`.
3. The **total SCC work** telescopes to a closed form the model keeps symbolic:

       W(S)  =  Σ_{r=0}^{R_S} Cost(round r)  ≈  C_S · avg_probe   (+ R_S · guard overhead under -demand)

   i.e. semi-naïve does `O(closure-size × average back-edge probe)` total, plus,
   under `-demand`, one guard JOIN **per round** (the guard is inside the cycle).

This is the crux for the "do-less" verdict on recursion: `-demand` **shrinks the
closure** `C_S → C_S^d` (the demanded slice, `C_S^d ≤ C_S`) but **pays a guard
JOIN each round**. Demand wins iff `C_S^d + R_S·guard < C_S` — the symbolic
statement of seed §7's "shallow/message-rooted shapes never pay."

### §4.3 The keyset-provenance lattice (the subset engine)

To mechanize §3-of-the-grounding's redundancy proof, each **output column** carries

    Prov(col)  ⊆  𝒟   (the set of demand relations d such that col's value is
                        PROVABLY a member of πkey(d))

a **meet-semilattice under ⊇ intersection** (top = ∅ = "no membership known";
lower = more memberships). Propagation rules — **conservative / under-approximate**
(add `d` only when provably ∈; this direction is a soundness obligation, §10):

- **SELECT of demand receive `d`** (`select.1`, `mono.demand.df:7`): each output
  col `c` gets `Prov(c) ∋ d`.
- **Guard JOIN** `read ⋈ demand_side on P` (`MintGuardJoin`, `Demand.cpp:162`):
  each pivot output col (a real two-input column edge, `Demand.cpp:186-192`)
  gets `Prov ∪= Prov(demand_side col) ∪ Prov(read col)` — crucially, since the
  pivot value equals a `demand_side` value, it **inherits every `d` the demand
  side witnesses**. Pass-through cols keep `Prov(read col)`.
- **TUPLE / CMP / MAP** (pass-through of an input col): `Prov` preserved on the
  forwarded column (`input_columns`/`out_to_in`, `Query.h:421`,`781`).
- **MERGE** (`merged_views`, `Query.h:905`): `Prov(out col) = ⋂_k Prov(member_k
  col)` — the *intersection*, because a union row is only demand-guarded if
  **every** member guaranteed it. This is the load-bearing conservative step.
- **NEGATE / AGG / KVINDEX / cross-product JOIN**: `Prov = ∅` on outputs (do not
  propagate membership through an antijoin, an aggregation, or a `@product` — the
  key relationship is broken). Under-approximation keeps this safe.

### §4.4 The redundancy predicate

For a guard JOIN `J = read ⋈ d on P` (either step): compute its **incremental
pruning**

    σ_J = 1  and  ΔPrune_J = 0        ⟺   ∀ p∈P.  d ∈ Prov(read.col_p)

i.e. `J` removes nothing when the read's every pivot column *already witnesses* the
demand relation `d`. Such a `J` is a **pure-overhead identity join**: `Card(J) =
Card(read)`, `Cost(J) = Card(read) + materialize`, all wasted. This is the exact,
mechanical restatement of grounding §3/§5 — the "subset/cardinality reasoning the
canonicalizer does not have."

---

## §5 THE SURFACE — THE `.cost` SIDECAR

A peer of `.batches`/`.drflags`/`.eqgate` (CLAUDE.md sidecar convention). Line-
oriented; `#` comments; directives:

```
# --- base symbols & optional numeric bindings ---
param  N   = |add_edge|            # base input cardinality (symbol; or a number)
param  D   = |demand.neighborhood_bf|  default 1   # demanded keys
param  K   = distinct(edge, From)  # key-domain size for the pivot
# --- selectivities / distributions over common keys ---
dist   sigma5 : demand x edge on From = D/K        # §2.3 join-key selectivity
funfan phi_f  = 1                                  # MAP fan-out (free vars)
# --- recursion knobs (only for SCC witnesses) ---
closure C_path = ...               # names the fixpoint cardinality of `path`
rounds  R_path = ...               # rounds-to-quiescence
# --- constraints the comparator may assume ---
assume  D <= N ; sigma5 <= 1 ; C_path_demand <= C_path
# --- assertions the instrument must discharge ---
config  -demand
expect  join.7 redundant           # ΔPrune(join.7) == 0   (the §4.4 predicate)
expect  cost(-demand) > cost(normal)  when  D*avg_deg + guards > sigma5*N
```

Semantics: the instrument builds the Query graph for each named `config`, runs the
§4 passes, and **discharges each `expect`** — `redundant` checks §4.4; a cost
comparison checks §2.2 dominance under `assume`. A failed `expect` is a hard error
(like a red golden). Absent a `.cost` sidecar a case is skipped (opt-in, exactly
like `.batches`). The instrument also emits, per config, a `<name>.cost.out` dump:
per-view `Card`/`Cost`/`Prov` and the `Σ Cost` total — the analytic peer of the
`.rel` census.

---

## §6 WORKED EXAMPLE — THE MONO WITNESS

`neighborhood(bound Start, free Node) : edge(Start,Node)` over
`edge(From,To) : add_edge(From,To)`. Symbols: `N=|add_edge|=|edge|`,
`K=distinct(edge,From)`, `avg_deg=N/K`, `D=|demand|` (default 1).

### §6.1 normal (`mono.normal.df` / `.rel`, `kEagerJoin=0`)

| view | Card | Cost | Prov |
|------|------|------|------|
| `select.0` recv add_edge | N | 0 (ingest) | ∅ |
| `tuple.1` | δN≈N | N | ∅ |
| `insert.2` → `%table:4` | — | N | — |
| **query answer** (index probe on From) | σ·N = D·avg_deg | **D·avg_deg** | — |

`Σ Cost(normal) = N (build) + D·avg_deg (answer)`. **Zero joins** — reproduces the
census. The answer is a single index probe.

### §6.2 -demand (`mono.demand.df` / `.rel`, `kEagerJoin=4`,`kJoinEmit=2`)

Reading the real `mono.demand.df` node graph:

| view (`mono.demand.df`) | Card | Cost | Prov(key col) |
|--------------------------|------|------|----------------|
| `select.0` recv add_edge | N | 0 | ∅ |
| `select.1` recv demand (`:7`) | D | 0 | `{d}` on c3 |
| `tuple.2` (edge) `:11` | N | N | ∅ |
| `tuple.4` (d, c8) `:19` | D | D | **`{d}` on c8** |
| `join.6` **step-5 push-down** `:28` — pivot `From ← d.c8, edge.From` | σ5·N = **D·avg_deg** | D + D·avg_deg | **`{d}` on Start** (pivot inherits demand) |
| `tuple.3` `:15` | D·avg_deg | D·avg_deg | **`{d}` on Start** (preserved) |
| `join.7` **step-8 projection** `:35` — pivot `Start ← d.c8, tuple.3.Start` | **= Card(tuple.3)** | D·avg_deg + mat | `{d}` on Start |
| `tuple.5` `:24` → `insert.8` | D·avg_deg | D·avg_deg | — |

`Σ Cost(-demand) = N (edge build) + D (demand ingest) + [D + 2·D·avg_deg]
(join.6+mat) + [2·D·avg_deg] (join.7+mat)`. **Two joins** — reproduces census.

### §6.3 The comparison verdict

`Cost(-demand) − Cost(normal) = D + 2·D·avg_deg (join.6 chain) + 2·D·avg_deg
(join.7 chain) − D·avg_deg (the probe normal already did) > 0` for all D,avg_deg
> 0. On the 1-hop message-rooted shape **`-demand` strictly costs more** — it adds
a demand channel, forces `edge` materialization it then re-scans, and mints two
joins to reproduce what one index probe already answered. This is seed §7,
symbolic.

---

## §7 THE DOUBLE-JOIN DERIVATION (term-8 ⊆ term-5)

Define the two guard-join **cost/pruning terms** on the supported slice:

**term-5 (step-5 push-down, `join.6`):**

    Card₅  = σ5 · N = D·avg_deg          (the demanded edge slice)
    Cost₅  = D + Card₅                    (probe edge.From index + emit)
    ΔPrune₅ = N − Card₅  =  N − D·avg_deg  >  0     ← REAL pruning

term-5 is the essential mechanism: it shrinks the materialized read of `edge`
from `N` to the demanded `D·avg_deg`. **Justified.**

**term-8 (step-8 projection, `join.7`):**

    Prov(tuple.3.Start) ∋ d          — from §6.2: join.6 minted Start as a pivot
                                        against d.c8, so §4.3 gives d ∈ Prov(Start)
    ⇒ by §4.4 (∀p∈P={Start}. d ∈ Prov(read.Start)):
       σ8 = 1,   Card₈ = Card(tuple.3) = D·avg_deg,   ΔPrune₈ = 0    ← NOTHING pruned
    Cost₈  = Card(tuple.3) + materialize(tuple.5)  =  2·D·avg_deg     ← PURE overhead

**term-8 ⊆ term-5, precisely.** As output *sets*:

    out₈ = out₅ ∩ { rows : Start ∈ d }.
    But every row of out₅ already satisfies Start ∈ d, because d ∈ Prov(out₅.Start).
    ∴ out₈ = out₅.

So term-8's pruning is a *subset of* (dominated by) term-5's — it removes only
rows term-5 already removed, i.e. none. The `join.7` node's entire `Cost₈` is
waste on this slice. This is the algebraic reproduction of grounding §3.

**Census reproduction.** 1-hop 1-adorn: normal 0 joins → `-demand` **2** (join.6 +
join.7). 1-hop 2-adorn: each adornment runs the whole step-5+step-8 mint
(`Demand.cpp` per-adornment loop), so **4** joins, and each name's step-8 is
independently identity → `2·(redundant)`. tc (right-linear): normal **1** (the
recursive `path ⋈ edge`) → `-demand` **4** = `[per-round guard-5] + [projection-8]
+ [the demand relation's own recursive guard] + [body join]`; the model's SCC
account (§4.2) puts the guard inside the cycle (`R_path` copies) and the
projection once outside. All three rows of the grounding census fall out.

**The boundary where step-8 becomes load-bearing** (the honest caveat,
grounding §4). term-8's redundancy hinges on `d ∈ Prov(q_read.p_bound)`. That
fails — and `σ8 < 1`, `ΔPrune₈ > 0`, step-8 **earns its cost** — exactly when the
query's own read of `p` is NOT already demand-guarded by the body:

1. **Adornment-divergent query read** — the query reads `p` at a binding pattern
   different from the body's SIP guard, so the body guard witnesses `d_body` but
   the query read's pivot columns witness a *different* `d_query` → `d_query ∉
   Prov`. (§4.3 MERGE-intersection also drops membership when only some clauses
   propagate the key.)
2. **Non-From-preserving recursion** — a closure whose derived keys diverge from
   the seed key (the body does not preserve the bound column). Then §4.2's
   `C_S^d` no longer equals the query's seed set, `Prov` does not carry `d` to
   the query read, and step-8 re-establishes it. This is precisely the case
   grounding §4 flags as "outside today's slice."
3. **Multi-clause query** where one clause fails to propagate the bound key → the
   MERGE intersection (§4.3) yields `Prov = ∅`, `σ8 < 1`.

The model **does not special-case** these — they fall out of `Prov` being ∅/partial
on `q_read.p_bound`. On the supported slice (single-adornment, From-preserving,
single-clause) `Prov` carries `d`, `σ8=1`, and the `expect join.7 redundant`
assertion discharges. Off-slice it does not, and the instrument correctly reports
step-8 as load-bearing. **The cost model characterizes the boundary; it does not
assert unconditional redundancy** — exactly the mandate.

---

## §8 DIFFERENTIAL / RETRACTION

Under `-demand-retract` / `@differential`, maintenance is per-stratum
OVERDELETE→REDERIVE→INSERT with split signed counters (CLAUDE.md). The Query
layer models this as a **per-view coefficient** `κ_v` folded onto the delta cost:

    Cost_diff(v)  =  κ_v · Cost(v)|_{Card(in) := Δ_in}

with `κ_v = 1` for a monotone view and `κ_v ≈ 3` for a differential one (the three
passes), and the driving cardinality replaced by the **batch delta** `Δ_in`, not
the full table — incremental maintenance touches the delta, not the closure. This
is the one place the **Rel layer feeds back** (§1): whether a view is differential
(`TableIsDifferential`) and whether a rederive fan is bounded is a Rel/runtime
fact; the model imports it as `κ_v` annotations, keeping Query the cost home.
Rederive superlinearity (support-set fan) is *not* captured beyond the coarse κ
(§9).

---

## §9 WHAT THE MODEL CANNOT YET REPRESENT

1. **Absolute predictions.** The model is **parametric in σ/N/C/R**; it proves
   *relative* and *subset* facts symbolically (term-8 ⊆ term-5, config
   dominance), but any "worth it" verdict with a number needs the `.cost` sidecar
   (or a profile) to bind σ and C. It is a referee/what-if engine, not an oracle.
2. **Data skew.** The "distribution over common keys" is collapsed to one `σ` per
   join (§2.3). Heavy-hitter / degree-skew join blow-up (one key with huge fan)
   is invisible beyond the average — a real explosion source the model averages
   away.
3. **Cross-key correlation.** Multi-key joins and correlations across body atoms
   are assumed independent; correlated selectivities are mis-estimated.
4. **Exact fixpoint shape.** `R_S` and the per-round `Δ_r` recurrence (§4.2) are
   approximated by the semi-naïve telescoping; oscillating/non-monotone frontier
   sizes are not tracked.
5. **Physical / mechanical-sympathy costs.** Cache lines, branch misprediction,
   dependent loads (the bench/`purplesyringa` referee's turf), index *build*
   cost, dead-row compaction, and the **keyed-instance rescan regression** (the
   p1 full-section-walk vs index-probe) are Rel/runtime facts the Query layer
   cannot see except as an imported `κ` multiplier.
6. **Differential rederive superlinearity** beyond `κ_diff` (§8).
7. **Functor internals.** MAP cost is `Card(in)·1` (§3) with fan-out `φ_f`; a
   functor's own compute cost is opaque.
8. **Set-dedup exactness.** `δ ≤ 1` is an upper bound; exact post-dedup
   cardinalities need real value distributions.

---

## §10 KEY RISKS / SOUNDNESS GAPS (in this design)

1. **Provenance must UNDER-approximate.** If `Prov` ever credits a `d` the column
   does not provably carry, §4.4 would declare a *load-bearing* guard redundant —
   and if the transform acts on that (skips a real step-8), it **miscompiles**.
   The MERGE-intersection and the NEGATE/AGG/`@product`-⇒-∅ rules (§4.3) are the
   safety-critical steps; any bug that turns intersection into union, or leaks
   membership through an antijoin, is a correctness hole. The provenance lattice
   direction (add only when provable) is the entire soundness argument.
2. **Symbolic-comparison decidability.** Dominance (§2.2) with free `σ∈(0,1]` and
   closure symbols is not fully decidable; I restrict to a monomial-dominance
   fragment plus linear `assume` elimination. Realistic recursive comparisons
   (`C_S^d + R_S·g` vs `C_S`) may land "cannot decide" and need sidecar bounds.
3. **Selectivity is garbage-in.** Every `Card(JOIN)` rides one supplied `σ`
   (§2.3); a wrong σ silently poisons the total. The model's honesty depends on
   the sidecar author (or a future profiler) supplying faithful σ — otherwise it
   can *itself* mislabel an explosion as cheap, the exact failure it exists to
   prevent.
4. **Boundary completeness.** §7's three off-slice cases are the ones I can name;
   there may be graph shapes where `Prov` is conservative-but-imprecise (returns
   ∅ when the guard *is* redundant), causing the model to *keep* a truly-redundant
   step-8 — a missed optimization, not a miscompile (the safe failure direction),
   but a limit on the "do-less" verdict's tightness.
5. **Recursion attribution.** Assigning `C_S`/`R_S` per SCC relies on
   `induction_info`/`ViewSelfReachable` being complete; a mis-identified SCC
   (e.g. a JOIN whose interior loop lost its group id, the F22 shape) would cost
   a recursive relation as acyclic and under-count it.
