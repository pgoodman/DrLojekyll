# THE COST MODEL — REL-ANCHORED DESIGN

> **Lane.** IR-HOME = the Rel / DR-IR (`lib/Rel/Rel.h`, `lib/Rel/Format.cpp`).
> Cost is computed over the DR flow graph — the SOLE scheduling authority. Written
> 2026-07-30 at tip `eecea847`. Companion grounding:
> `../grounding-double-join.md`; memory `[[demand-cost-model]]`.

---

## §0 THESIS — WHY THE REL IS THE COST HOME

The owner's open question ("magic sets as done there, or maybe you did it on the
rel"). My position: **cost belongs on the Rel / DR flow graph, not on the Query
graph.** Three reasons, each grounded:

1. **The Rel is where the REAL joins/scans/rounds exist.** The Query graph
   (`lib/DataFlow/Query.h`) is a *dataflow* graph — a JOIN node there is an
   abstract pivot, not a physical pass. The physical join is the once-per-join
   `kJoinEmit` op (`Rel.h:247`), which `LowerJoinEmit` wraps around the untouched
   `BuildJoin` (`Rel.h:1189`); the physical probe is `Index::First`+`Next`
   (`Table.h:804`, `:821`). A Query-level JOIN with two feeding predecessors
   becomes, on the Rel, **two `kEagerJoin` per-visit dispatch markers**
   (`Rel.h:209`) plus **one `kJoinEmit`** — the mono witness census reads
   `kEagerJoin=4 kJoinEmit=2` for its two `.df` joins. Only the Rel distinguishes
   "a join appears in the graph" (dispatch markers) from "a join PASS runs"
   (`kJoinEmit`). Cost must count passes, not appearances.

2. **The Rel already carries the two hooks a cost model needs.** (a) Every access
   op carries an **ACCESS-PLAN SPINE** of `PlanNode`s (`Rel.h:485`) whose
   `Lowering` field (`Rel.h:483`) is exactly `{kPointTest, kSectionWalk,
   kFullScan}` — the physical-access discriminant that decides O(1)-probe vs
   O(matches)-walk vs O(|table|)-scan. (b) Every op carries an **effect set**
   (`DREffect`, `Rel.h:342`) naming the tables it counter-folds / flag-reads, and
   every DRVec (`Rel.h:390`) carries a `VecRole` (`Rel.h:54`) — the queues,
   frontiers and pivot vecs whose *cardinalities* are the flow quantities. The
   cost model annotates the objects that already exist.

3. **The Rel is the per-config artifact.** The owner wants "per-example,
   per-IR-configuration" cost. The DR flow graph *is* the per-config object: it
   differs structurally between `normal` and `-demand` (mono: 3 ops → 13 ops; two
   `kJoinEmit` appear from nothing). Summing per-op cost over each config's own
   flow graph IS the config comparison — no separate cost IR needed.

**Corollary (the double-join home).** §5 of the grounding asked where a fix lands.
The cost model's *judgement* lives on the Rel (it reads `kJoinEmit` count and
spine selectivity); the *fix*, if taken, lands upstream in
`ApplyDemandTransform`. Cost-on-Rel, fix-on-Query is coherent: the Rel is the
measurement plane, the Query graph is one actuation plane.

---

## §1 THE COST ALGEBRA — SYMBOLIC DOMAIN

### §1.1 Two coupled lattices per DR value

Every **DRVec** and every **DRTable** in the flow is annotated with a *cost
annotation* `⟨card, dc⟩`:

- **`card`** — a symbolic **cardinality expression** over a fixed symbol basis
  (below), in the semiring `(ℝ≥0 ∪ {∞}, +, ·)` extended with the monotone
  operators `min`, `max`, and a bounded-fixpoint operator `lfp` (§3.3). This is
  the "N·M or 0.5N·0.7M" quantity the owner named.

- **`dc`** — a **demand-constraint tag** from a small boolean lattice
  `{⊤_key, ⊥}` *per column*: `dc[c] = ⊤_key` means "column `c` of this value is
  provably a subset of some demand-key set `d`". This is the **subset reasoning
  the canonicalizer lacks** (grounding §5) — it is what proves term-8 ⊆ term-5.
  Carried as a `dc` map `col → {⊤_key(d) | ⊥}`, join at ⊥ (a column is
  demand-constrained only if PROVABLY so).

### §1.2 The symbol basis

Bound to real IR objects so the model is code-true, not free-floating:

| symbol | meaning | IR binding |
|--------|---------|-----------|
| `N_m`  | rows of message `m` per batch | `kIngestFold.ingest_message` (`Rel.h:672`) |
| `T_t`  | live rows of table `t` (steady state) | `DRTable.model` (`Rel.h:413`) |
| `D_a`  | distinct demanded keys of adornment `a` | `kSubgraphInstantiate.demand_table` (`Rel.h:711`) / demand `kIngestFold` |
| `f_{t.k}` | fan-out: avg rows of `t` per distinct value of key `k` = `T_t / K_{t.k}` | index on `t` keyed `k` (`Table.h:749`) |
| `σ_{op}` | selectivity of a filter/join op ∈ [0,1] | the op's spine gate (`PlanNode.pred`, `Rel.h:499`) |
| `R_s`  | rounds-to-closure of SCC `s` (recursion depth) | `DRRound` count for `scc_group` (`Rel.h:787`) |
| `Z_s`  | closure size of SCC `s` (fixpoint result rows) | `lfp` over the SCC tables |
| `ρ_t`  | retraction fraction of differential table `t` per batch ∈ [0,1] | `DRTable.differential` (`Rel.h:414`) |

`K_{t.k}` (number of distinct key values) is a derived symbol `T_t / f_{t.k}`; the
model carries whichever of `{T, f, K}` the sidecar supplies and derives the third.

### §1.3 Default population

Unknown symbols default to conservative placeholders so a bare run still yields a
comparable *shape*: `f = φ` (a free fan-out symbol, default 1 = "one match per
key" unless the sidecar overrides), `σ = 1` (a filter removes nothing unless
proven), `ρ = 0` (monotone unless the table is `differential`), `R_s` = a free
symbol (never numeric without a `.cost` closure hint — §4). The output is a
symbolic expression in the surviving free symbols; the owner reads the *form*
(does `-demand` multiply by `D·(1+f)` twice?) even before plugging numbers.

---

## §2 PER-OP COST FUNCTIONS

Cost is `cost : DROp → CardExpr`, summed over `flow.ops` for the program total.
Each function is grounded in the op's real emission. **Unit** = one probe / one
`TryAdd` / one functor call / one counter RMW — all O(1) amortized, so we count
*invocations* (the honest, cache-line-agnostic first cut; a later refinement can
weight units, PerfRoadmap §9).

### §2.1 The join family (the double-join subject)

`kJoinEmit` (`Rel.h:247`) is the **only cost-bearing join op**. `kEagerJoin`
(`Rel.h:209`) and `kProductArm`/`kPivotAssemble` markers are per-visit dispatch
records — **`cost = 0`** (they emit no pass; `Rel.h:164` "EFFECT-FREE position
marker … invisible to the dep-edge derivation"). Costing `kEagerJoin` would
double-count the join by its predecessor arity.

A `kJoinEmit` for join view `j` with pivot vec `P` (cardinality `card(P)`)
probing a scanned side table `t` on key `k`:

```
cost(kJoinEmit_j) = card(P)                          # one Index::First per pivot   (Table.h:804)
                  + Σ_matches                          # Next-walk over the chain     (Table.h:821)
                  = card(P) · (1 + f_{t.k} · σ_j)
output card(j)    = card(P) · f_{t.k} · σ_j
```

`σ_j` is the join's *residual* selectivity AFTER the index equality (the probe is
exact — `Table.h:791` "FULL-KEY EXACT … emits NO per-row re-check"), so `σ_j = 1`
for a pure equijoin and `< 1` only when a spine `kGate`/`kCompare` node further
filters. The `f · σ` product is the owner's "0.5N·0.7M" shape realized on one
index probe. The pivot vec `P` is a real DRVec (`DRJoin.pivot_vec`, `Rel.h:450`);
its cardinality flows from the assembling side (§3).

`form=delta` `kJoinEmit` (recursive, `Rel.h:761`) costs the same per round with
`card(P)` = the round's claimed frontier δ_r (§3.3).

### §2.2 The ingest family

`kIngestFold` (`Rel.h:136`; lowering `LowerIngestFold`, `Rel.h:1168`: "VECTORLOOP
over loop_vec → UPDATECOUNT± per row"):

```
cost(kIngestFold_m) = N_m          # one counter RMW per message row (Table.h AddDerivation)
```

`kIngestLoop` (`Rel.h:229`, table-less shim, effect-free VECTORLOOP): same
`N_m`, output feeds the descent. `ingest_is_explicit` deletion-capable receives
emit a `{+1,-1}` pair (`Rel.h:663`) → cost `2·N_m` (add + remove folds).

### §2.3 The eager-web markers (per-row descent work)

`kEagerForward/Insert/Select/Compare/Generate/Union` are **effect-free at the DR
dependence layer** (`Rel.h:164`) but each dispatches a per-ROW descent action.
Cost is `card(eager_view) · u_kind` where `u_kind` is the unit:

| kind | action | unit | ground |
|------|--------|------|--------|
| `kEagerForward` | TUPLE column re-thread | 0 (pure plumbing) | `Rel.h:164` |
| `kEagerInsert`  | terminal `TryAdd` | 1 | `Table.h:255` |
| `kEagerCompare` | CMP filter, multiplies output by σ | 1 | `Rel.h:177` |
| `kEagerGenerate`| MAP functor call | 1 (functor-weight later) | `Rel.h:184` |
| `kEagerUnion`   | MERGE `TryAdd` into shared model | 1 | `Rel.h:190` |
| `kEagerSelect`  | SELECT rebind | 0 | `Rel.h:201` |

`kEagerCompare` is the only marker that changes *downstream* cardinality
(multiply by `σ_cmp`, read from `eager_view`'s operator at render, `Rel.h:180`);
the rest are cardinality-transparent.

### §2.4 The fixpoint family (recursion)

`kFixpointFire` (`Rel.h:127`, "join × sign, claim-relative") + `kChainFold`
(`Rel.h:128`) live inside a `DRRound` shell (`Rel.h:787`, per SCC × phase). Semi-
naive: at round `r` the fire consumes only the *previous* round's claimed
frontier δ_{r-1} (the round's `test_vecs`, `Rel.h:801`):

```
cost(kFixpointFire_j)  = Σ_{r=1..R_s} δ_{r-1} · (1 + f_{t.k})
cost(kChainFold_b)     = Σ_{r=1..R_s} δ_{r-1}
with  Σ_{r} δ_r = Z_s          # frontiers partition the closure
```

For a right-linear SCC (fan-out `f` bounded) `Σ_r δ_{r-1}·(1+f) ≈ Z_s·(1+f)`, so
the recursive join costs **once per closure row**, not per round² — the semi-naive
guarantee, made symbolic. `R_s` appears only in *per-round overhead* (§2.5 claim
drains, one clear per round: `+R_s · |SCC tables|`).

### §2.5 The differential / retraction family

Per differential table `t` (`DRTable.differential`, `Rel.h:414`), the batch does
OVERDELETE → REDERIVE → INSERT (`Table.h:317`). Model as a multiplier on the
table's fold/drain/sweep ops:

```
diff_mult(t) = 1 + 2·ρ_t        # overdelete pass + rederive pass, each ~ρ·|activity|
cost(kClaimDrain_t)   = card(delta_queue_t) · diff_mult(t)     # Rel.h:129
cost(kFrontierFilter_t)= card(frontier_t)   · diff_mult(t)     # Rel.h:132
cost(kCommitSweep_t)  = (T_t + dead_t)                          # Rel.h:133; Commit+reindex, O(rows)
                        + [compaction fires] · T_t              # Table.h:119 CompactRowsInPlace
```

Compaction is gated `dead ≥ live, 4096 floor` (CLAUDE.md) → the `[compaction
fires]` indicator is 0 for suite-sized programs; the model carries it as a
symbol that only a large-batch `.cost` hint turns on. Monotone tables: `ρ=0`,
`diff_mult=1`, `kCommitSweep`→`Seal` = `card(newly-sealed)`.

### §2.6 The keyed-instance family (the "do less" subject)

`kSubgraphInstantiate` (`Rel.h:148`, birth/rebuild band-(a) rescan) is where
`-demand-instance` must be *cheaper* than the flat guard web or the feature has
backfired:

```
cost(kSubgraphInstantiate) = card(input net-additions frontier) · (rebuild rescan)
                           = ΔN_input · f_{input.key}         # band-(a2) edge-frontier rescan
```

This is the direct handle on the owner's mandate: compare
`cost(kSubgraphInstantiate)` (nested) against the two `kJoinEmit` guard joins
(flat). If the rescan re-materializes more than the flat guard prunes, the model
flags it (HP-5 over-materialization is a cost regression, not just a correctness
abort). The `p1` full-scan regression (CLAUDE.md: a keyed rescan that regressed a
`kPointTest` to a `kSectionWalk`/`kFullScan`) is visible as a spine `Lowering`
downgrade — §2.7.

### §2.7 The spine multiplier (scan vs probe — the house constraint)

The per-op cost above assumes the *access lowering* the spine records. The spine
`Lowering` (`Rel.h:483`) sets the base:

```
kPointTest   : base = 1              # Find, one probe        (Table.h:80)
kSectionWalk : base = f_{t.k}        # Index First+Next chain (Table.h:804/821)
kFullScan    : base = T_t            # whole table            (no bound_cols, Rel.h:494)
```

A regressed probe→scan (the p1 note) turns a `kPointTest` (base 1) into a
`kFullScan` (base `T_t`) — a **`T_t`-fold cost blow-up the model reports directly
from the spine**. This is the mechanical realization of "a bound-column INDEX
turns a scan O(|table|) into a probe O(matches)".

---

## §3 THE PROPAGATION RULE

Cost annotations flow **input → output along the flow graph in `pinned_order`**
(`Rel.h:915` — already a checked topological linearization, so a single forward
sweep suffices; no separate scheduling needed).

### §3.1 Sources

- A `kIngestFold` output table gets `card = N_m` (its message).
- A `demand` `kIngestFold` / forcer gets `card = D_a` (distinct demanded keys).
- A base relation `T_t` is a free symbol (steady-state input size).

### §3.2 The per-op transfer functions

For each op in order, set the output DRVec/table card from the inputs:

```
kJoinEmit_j  : card(out) = card(P) · f_{t.k} · σ_j          (§2.1)
               dc(out.c) = dc(P.pivot_c)  ⊔  dc(t.c)         # a pivot-constrained
                                                             #   column inherits ⊤_key
kEagerCompare: card(out) = card(in) · σ_cmp
kEagerUnion  : card(out) = Σ_members card(member)           # MERGE = sum (pre-dedup)
kEagerGenerate: card(out) = card(in) · φ_map                # functor fan (default 1)
GroupUpdate  : card(out) = K_{group}                        # one row per group  (Rel.h:139)
kFixpointFire: card(out) = lfp                              (§3.3)
```

The **`dc` propagation is the load-bearing rule for the double join**: a
`kJoinEmit` whose pivot came from a demand table `d` stamps its output's pivot
column with `⊤_key(d)`. Any later op reading that column against the *same* `d`
sees `dc = ⊤_key(d)` and the model knows the semijoin is an **identity** (§5).

### §3.3 The fixpoint case (`lfp`)

An SCC `s` (a set of tables sharing a `scc_group`, `Rel.h:789`) is costed as a
bounded fixpoint. The closure card `Z_s` is the least fixpoint of the SCC's
recurrence; for a right-linear recursion over a graph of `V` keys and fan-out `f`
it is bounded by the transitive-closure size — the model does **not** solve it
numerically without a hint. It carries `Z_s` as a symbol and the *cost* as
`Σ_r δ_{r-1}·(1+f)` (§2.4). A `.cost` `closure:` hint (§4) supplies `Z_s`/`R_s`
when the analyst has a measured or bounded estimate; otherwise the total stays
symbolic in `Z_s` and configs are still comparable in `Z_s`.

**Termination note.** The model never *runs* the fixpoint; it counts the rounds
the `DRRound` shells declare. This mirrors the real compiler (the round shell is
a scheduling black box, `Rel.h:782`). Recursion depth `R_s` is the analyst's or a
bound's input, never derived.

---

## §4 THE SURFACE — `.cost` SIDECAR

A peer of `.batches` / `.drflags` / `.eqgate`. Grammar (line-oriented, `#`
comments, `key: expr` bindings; missing keys default per §1.3):

```
# <case>.cost — analytic cost inputs for the cost instrument
config: normal | -demand | -demand-instance | -demand-retract   # which flow to cost
message <name>/<arity>: <card-expr>       # N_m  (rows/batch)
table   <rel-name>:     <card-expr>       # T_t  (steady-state live rows)
demand  <adornment>:    <card-expr>       # D_a  (distinct demanded keys)
fanout  <rel>.<key>:    <expr>            # f_{t.k}   (default 1)
select  <op-id|view>:   <0..1>            # σ         (default 1)
closure <scc>:          size=<expr> rounds=<expr>   # Z_s / R_s (default free)
retract <rel>:          <0..1>            # ρ_t       (default 0)
```

Symbols are free identifiers (`N`, `D`, `f`, …); expressions are `+ · min max`
over symbols and rationals. The instrument (a new `bin/Cost` peer of `bin/Oracle`,
reading `-rel-out` + the sidecar) emits, per config, a **per-op cost table** and a
**program-total expression**, and — given two configs — a **ratio expression**.

### §4.1 Worked example — the mono witness `.cost`

```
# demand_neighborhood_mono_witness.cost
message add_edge/2:              N
demand  neighborhood_bf:         D
fanout  edge.From:               f          # avg out-edges per demanded Start
```

Costing `mono.normal.rel` (3 ops):

```
op.0 kIngestFold  add_edge   : N
op.1 kEagerForward table:4    : 0
op.2 kEagerInsert  table:4    : N        # TryAdd per edge
TOTAL(normal) = 2N     +  query answer = 1 probe + f matches  (index on bound col)
```

Costing `mono.demand.rel` (13 ops; the cost-bearing ones):

```
op.0 kIngestFold  add_edge (table:11 edge)     : N
op.1 kIngestFold  demand (table:8 d)           : D
op.11 kJoinEmit   join.6  table:15  (push-down): D · (1 + f)     ; out card = D·f
op.12 kJoinEmit   join.7  table:4   (proj-guard): D · (1 + f)    ; out card = D·f
op.9/op.10 forward/insert into pub             : D·f            # TryAdd per answer row
markers (kEagerJoin×4, kEagerForward×4)        : 0
TOTAL(demand) = N + D + 2·D·(1+f) + D·f
```

---

## §5 THE DOUBLE-JOIN DERIVATION — TERM-8 ⊆ TERM-5, SYMBOLICALLY

This is the instrument's first acceptance test (grounding §6). Reproduced from
the real `mono.demand.rel` / `mono.demand.df`.

**The two guard joins** (`.df` join.6, join.7; `.rel` op.11, op.12):

- **join.6 = PUSH-DOWN, step-5** (`kJoinEmit` op.11, `table:15`, `order=3`):
  pivots demand `d` (col c8, card `D`) against `edge.From` (card `N`, fan-out
  `f`). By §2.1:

  ```
  term-5 = cost(op.11) = D · (1 + f)
  card(tuple.3) = D · f
  dc(tuple.3.Start) = ⊤_key(d)          # pivot column From←c8 came from d  (§3.2)
  ```

  The `dc` stamp is the code-true invariant grounding §3 states in prose ("every
  row of tuple.3 has Start ∈ d"): join.6 pivoted on `From ← d.c8`, so
  `π_Start(tuple.3) ⊆ d`.

- **join.7 = PROJECTION-GUARD, step-8** (`kJoinEmit` op.12, `table:4`,
  `order=5`): pivots the SAME demand `d.c8` against `tuple.3.Start`. By §2.1 its
  cost is `D·(1+f)` and its output card is `card(tuple.3) · σ_7`.

**The subset proof.** join.7 is the semijoin `tuple.3 ⋉_Start d`. Its selectivity
is

```
σ_7 = |tuple.3 ⋉_Start d| / |tuple.3| = |{r ∈ tuple.3 : r.Start ∈ d}| / |tuple.3|
```

But the `dc` annotation carries `dc(tuple.3.Start) = ⊤_key(d)`, i.e.
`π_Start(tuple.3) ⊆ d`. Therefore **every** row of tuple.3 satisfies
`Start ∈ d`, so the semijoin removes nothing:

```
σ_7 = 1     ⟺     dc(pivot column) = ⊤_key(same d)
⟹  output(join.7) = tuple.3   (IDENTITY — set-equal to join.6's output)
⟹  term-8 ⊆ term-5   as sets: the pairs join.7 produces = join.6's output exactly
```

**The redundancy is thus a `dc`-tag equality the model computes**, and the
canonicalizer cannot (grounding §5: it "needs subset/cardinality reasoning the
canonicalizer does not have"). The extra cost is pure waste:

```
Δ_redundant = term-8 = D · (1 + f)     # one full probe+walk pass producing nothing new
```

**Census reproduction.** The `-demand` total minus the redundant term:

```
TOTAL(demand)             = N + D + 2·D·(1+f) + D·f
TOTAL(demand, step-8 cut) = N + D + 1·D·(1+f) + D·f       # drop the identity join
```

The two `kJoinEmit` of the census (`kJoinEmit=2`) are the two `D·(1+f)` terms;
cutting the redundant one halves the join cost and removes the intermediate
`table:15`→`table:4` re-pass, matching grounding §3's structural claim
byte-for-byte.

### §5.1 THE BOUNDARY — WHEN STEP-8 IS LOAD-BEARING

The model does **not** assert step-8 is always redundant (grounding §4's honest
caveat). Step-8 prunes iff `σ_7 < 1` iff `dc(tuple.3.Start) ≠ ⊤_key(d)` — i.e.
the body-guard key set diverges from the query's own seed `d`. The `dc` lattice
localizes exactly the three off-slice cases:

1. **Adornment divergence** — the query reads `p` at an adornment whose bound
   column ≠ the demanded column. The pivot column of join.7 is then NOT the
   column join.6 constrained; `dc = ⊥`; `σ_7 < 1`; term-8 prunes. (Multi-
   adornment, D3.a.3: N disjoint stores over one pub — each adornment's join.7
   may pivot a different column.)
2. **Non-From-preserving recursion** — an intermediate demanded key that isn't
   the query seed. The recursion's derived rows carry a Start NOT ⊆ `d`; `dc`
   drops to ⊥ at the back-edge fold; `σ_7 < 1`.
3. **Multi-clause query** — a union whose arms bind the read column differently;
   `kEagerUnion` joins `dc` at ⊥ across arms (§3.2), so the union output is not
   demand-constrained and step-8 is real.

**Verdict.** term-8 ⊆ term-5 **on the supported slice** (single-adornment,
From-preserving: `dc = ⊤_key(d)` propagates unbroken from join.6 to join.7). The
boundary is precisely the `dc = ⊥` frontier — the model reports `σ_7 = 1`
(cut it) vs `σ_7` symbolic (keep it) per case, so a cost-guided rewrite fires
only where provably safe.

---

## §6 CONFIG COMPARISON — THE "DO LESS" REFEREE

The general mandate ("is this sensical and worth it?"). The referee is a **ratio
of program-total expressions across two configs' flow graphs**:

```
worth(-demand) = TOTAL(normal) / TOTAL(-demand)     # >1 ⟹ demand is cheaper
```

For mono (message-rooted 1-hop): `TOTAL(normal) = 2N` (materialize once, answer by
one index probe). `TOTAL(-demand) = N + D + 2D(1+f) + Df`. Since the demand
channel *also* materializes edge (`N`) AND adds `2D(1+f)` of join, and `D ≤ N`
gives no asymptotic win, `worth < 1` for all `D,f` — **demand costs strictly more
than it saves on a shallow message-rooted shape**, reproducing the seed §7 verdict
("shallow / message-rooted shapes never pay") *symbolically and mechanically*. The
model thus fulfills the owner's core ask: it flags a "do less" feature that
actually does MORE (`worth < 1`) the moment its guard/rescan terms out-weigh the
derivation it prunes.

The keyed-instance comparison is the same ratio with `cost(kSubgraphInstantiate)`
(§2.6) replacing the two `kJoinEmit`: `-demand-instance` is worth it iff the
band-(a) rescan `ΔN_input · f` is smaller than the flat guard `2D(1+f)` it
replaces — an inequality the model states per case.

---

## §7 WHAT THIS MODEL CANNOT YET REPRESENT

Honest limits (the schema's `cannot_represent`):

1. **Constant factors / cache behavior.** Units are invocation counts; a
   `kPointTest` and a `kSectionWalk` step are "1 each". Real cost is dominated by
   cache lines and dependent loads (memory `[[perf-guiding-oracles]]`,
   PerfRoadmap §9). This model ranks *asymptotic shape*, not wall-clock — it is
   the a-priori complement to `bench/`, never its replacement.
2. **Correlated selectivities.** `σ` and `f` are per-key *averages*; skewed key
   distributions (one hot Start with N/2 edges) are invisible. The "distribution
   of common keys" the owner named is modeled only by its mean fan-out `f`, not
   its shape — no histograms, no join-order-dependent selectivity products.
3. **Fixpoint closure size without a hint.** `Z_s` / `R_s` stay free symbols
   unless the `.cost` `closure:` hint supplies a bound; the model counts rounds,
   it does not *solve* the recurrence (a non-linear recursion's closure is
   genuinely data-dependent).
4. **Cross-op CSE / index sharing.** Two `kJoinEmit` sharing a rebuilt index
   (CARVE-3, `Rel.h:250`) are costed independently; shared-index amortization is
   not modeled. Likewise the `kEagerCompare` σ is applied locally, not folded
   into a downstream join's selectivity product.
5. **The functor cost.** `kEagerGenerate` / aggregate reduction bodies
   (`f_combine`/`f_reduce`) are unit-1 or `φ_map`; an expensive or fan-out MAP
   functor (future WASM functors, `[[wasm-functor-direction]]`) needs a per-
   functor weight the sidecar does not yet carry.
6. **Compaction amortization.** The `[compaction fires]` indicator (§2.5) is a
   0/1 the sidecar sets; the true amortized cost over many batches
   (`Table.h:119`) is not integrated.

---

## §8 KEY RISKS / SOUNDNESS GAPS (self-audit)

1. **The `dc` lattice is only as sound as its propagation completeness.** If ANY
   op fails to correctly *drop* `dc` to ⊥ (e.g. a MAP that rewrites the key
   column, an aggregate that regroups), the model would wrongly declare step-8
   redundant off-slice and a cost-guided rewrite could MIS-fire. `dc` must be
   ⊥-default and only *raised* by proven pivot inheritance (§3.2) — the safe
   direction, but every card-changing op needs an audited `dc` transfer or the
   subset proof is unsound. This is the model's single load-bearing correctness
   claim and deserves a validator (a `V-DC-*` peer of the DR validators).
2. **`kEagerJoin`/marker `cost=0` assumes the emission is truly the `kJoinEmit`.**
   If a model-SHARED join (E-107, `Rel.h:221`) renders a `table=` on a marker,
   the cost must still attribute the pass to the single `kJoinEmit`, not the
   markers. A shared/CSE'd join counted twice would over-state cost. The
   `kJoinEmit` census (`=2` on mono) is the authority; the sweep must key on it.
3. **Free-symbol defaults can flatter a config.** `f=1, σ=1, ρ=0` defaults make
   `-demand` look linear when a real skewed graph explodes; the *comparison* is
   only trustworthy when both configs use the SAME sidecar symbols. The
   instrument must refuse to compare two configs costed with divergent
   `.cost` inputs.
4. **Fixpoint cost undercounts non-semi-naive shapes.** §2.4 assumes the
   semi-naive `δ_{r-1}`-driven fire; a program whose round shells re-scan the
   full table each round (a mis-lowering) would cost `R_s·Z_s`, not `Z_s`. The
   model trusts the `DRRound.test_vecs` semantics (`Rel.h:801`); a lowering bug
   there is invisible to cost (bench catches it, cost does not).
5. **Steady-state vs batch conflation.** `T_t` is steady-state; `N_m` is
   per-batch. A cold-start (empty tables, first batch) and a warm incremental
   batch have different `card(P)`; the model as written costs the warm case.
   Retraction `ρ` is the only batch-dynamics knob — insufficient for a full
   incremental-workload characterization.
