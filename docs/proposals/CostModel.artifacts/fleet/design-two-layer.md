# COST MODEL — THE TWO-LAYER DESIGN (IR-HOME = TWO LAYERS)

> Lane: **twolayer**. Written 2026-07-30 at tip `eecea847`. Grounds every
> op-cost claim in a real `file:line`. Companion to
> `grounding-double-join.md` (the empirical anchor) and the demand cost
> model seed (`KeyedInstances.artifacts/recursive-demand-seed.md` §7).

---

## §0 THE POSITION IN ONE PARAGRAPH

Cost lives in **two coupled layers**, not one.

- **L1 — the LOGICAL / CARDINALITY model on the Query graph**
  (`lib/DataFlow/Query.h`). It answers the a-priori transform question —
  *"does this rewrite do less?"* — **before** any lowering exists. It is
  where `ApplyDemandTransform` runs (grounding §5), so it is the only layer
  that can adjudicate a demand-transform decision at the moment the decision
  is made. It computes **cardinalities** (how many rows each view holds) and
  a coarse logical work estimate. It is cheap, structural, and pre-schedule.

- **L2 — the PHYSICAL / SCHEDULED model on the Rel/DR-IR**
  (`lib/Rel/Rel.h`). It answers *"what actually runs?"* — the scheduled
  execution cost in row-touches, keyed on the **already-checked** DR
  linearization (`LinearizeAndValidateDRFlow`) and the per-op access-plan
  spines. It sees scans-vs-probes, fixpoint round scheduling, and the
  differential OVERDELETE→REDERIVE→INSERT phases — none of which exist on
  the Query graph.

The **interface** is a shared cardinality environment (`CardEnv`, §4). The
**consistency obligation** (`V-COST-XCHECK`, §4.2) is that the two layers
agree on the cardinality of every table they share. That obligation is what
makes the second layer safe rather than a second source of truth to drift.

The double-join verdict (§7) is rendered at **both** layers: L1 *decides* to
drop step-8 by a cardinality-subset argument on the Query graph (term-8 ⊆
term-5); L2 *confirms* the emitted `kJoinEmit` count and the total row-touch
cost actually drop. Neither layer alone can both decide (pre-lowering) and
confirm (post-lowering) — that is the entire argument for two layers (§6).

---

## §1 THE COST ALGEBRA (the symbolic domain)

Two distinct quantities flow, in two distinct commutative semirings.

### 1.1 Cardinality domain `C` — what flows

`C` is the semiring of multivariate rational-coefficient expressions over
**base cardinality symbols** and **selectivity coefficients**, with `+`
(union), `·` (product/join), and scalar multiplication:

- **Base cardinality symbols** — one per input relation/message and per
  user-supplied unknown. `N_add_edge = |add_edge/2|`, `D_bf =
  |demand__neighborhood_bf|` (the distinct demanded keys), etc. Written `N`,
  `D`, … when unambiguous.
- **Selectivity coefficients** `ρ_k ∈ (0, 1]` — one per join **key
  equivalence** `k`. Definition: for an equijoin `A ⋈_k B`,
  `card(A ⋈_k B) = ρ_k · card(A) · card(B)`. This is exactly the owner's
  "0.5·N · 0.7·M" reasoning generalized: `ρ_k` folds together the fraction of
  each side that participates *and* the per-matching-pair fan-out. Under a
  key-functional right side (`B` is keyed on `k`, one row per key) `ρ_k =
  1/dom_k` and the join is a probe; `ρ_k = 1` is a Cartesian/identity join.
- **Filter selectivities** `σ ∈ (0,1]` — one per CMP predicate / NEGATE gate.
- **Recursion symbols** — per recursive view `v`: a **closure cardinality**
  `Z_v` (rows at fixpoint) and per SCC a **round count** `R_scc` (rounds to
  quiescence, ≈ graph diameter). These are *declared inputs*, not derived
  (§8): the model cannot predict transitive-closure size from graph shape.

Every Query view `v` and every DR table `T` is assigned a `card(·) ∈ C`.

### 1.2 Cost domain `K` — what accumulates

`K` is the semiring of the same expression shape but interpreted as
**row-touches** (abstract work units), accumulating with `+` along the
schedule and never flowing downstream. `K_total(config) = Σ_op K(op)`. This
is the analytic counterpart to `bench/` wall-clock: `bench/` measures cycles
after the fact; `K` estimates row-touches a priori (`PerfRoadmap` honesty
referee — never cache/branch effects, those stay `bench/`'s job, §8).

The **house cost primitives** (from `Runtime/Table.h`, the physical model)
are the leaves of `K`:

| primitive | `K` cost | output card | grounding |
|-----------|----------|-------------|-----------|
| point test (hash `Find`) | `c_find ≈ 1` | `ρ_k` (≤1 match) | `Table.h:80` `FindWithHash`, open-addressing O(1) expected |
| section walk (keyed index) | `|section|` | section size | `Table.h:749` `class Index` (key→row chain) |
| full scan | `card(T)` | `card(T)` | `Table.h:108` `CompactRowsInPlace` streaming scan over the row store |
| insert (`FindOrAdd`) | `c_find ≈ 1` | +1 if new | `Table.h:172` `FindOrAdd` |
| seek | `log card(T)` (reserved) | — | `Rel.h:483` `Lowering::kSeek` (D5 substrate) |

These four are exactly the DR-IR `Lowering` enum (`Rel.h:483`
`kPointTest/kSectionWalk/kFullScan/kSeek`) — **the cost model reads the
per-access lowering choice the DR-IR already recorded** rather than guessing
it. A bound-column INDEX turns a `kFullScan` (`card(T)`) into a
`kSectionWalk`/`kPointTest` (`|matches|`) — this is the house constraint made
symbolic, and it is the p1 regression rendered as a cost delta: a
`BindingSource::kInstanceKeySlot` (`Rel.h:478`) access that lowered
`kSectionWalk` instead of `kPointTest` costs `|section|` instead of `1`.

---

## §2 LAYER L1 — THE QUERY-GRAPH CARDINALITY MODEL

**Home:** `lib/DataFlow/Query.h` node classes (SELECT / TUPLE / JOIN / MERGE /
CMP / MAP / NEGATE / AGG / KVINDEX / INSERT). **Purpose:** decide, before
lowering, whether a Query-graph rewrite (demand transform, CSE,
canonicalization) *does less*. **Unit:** cardinality `∈ C` per view, plus a
coarse logical cost `Klog(v)` (a cardinality-shaped proxy: the number of
intermediate rows a naive evaluation of `v` would touch — enough to compare
two rewrites, not to predict wall-clock).

### 2.1 Per-node cardinality propagation (`card : QueryView → C`)

Propagation is a single topological pass over the acyclic condensation of the
Query graph (SCCs handled in §2.2). For a view `v` with input views `u_i`:

- **SELECT** (`QuerySelectImpl`, `Query.h:668`) — a read of a relation/stream.
  `card(v) = card(source relation)`. A message source's card is its base
  symbol (`N_add_edge`); a materialized relation's card is the card of its
  INSERT feeders' union.
- **TUPLE** (`QueryTupleImpl`, `Query.h:703`) — pure plumbing/projection.
  `card(v) = card(u)` (forwards; set-dedup on projection is a `≤`, modeled as
  a declared `dedup` coefficient, default 1).
- **CMP** (`QueryCompareImpl`, `Query.h:258`) — filter.
  `card(v) = σ_cmp · card(u)`.
- **MAP** (`QueryMapImpl`, `Query.h:796`) — functor.
  `card(v) = fanout · card(u)` (`fanout = 1` for a partial-function MAP;
  a generating functor's `fanout` is a declared symbol).
- **MERGE** (`QueryMergeImpl`, `Query.h:256`) — union.
  `card(v) = Σ_i card(u_i)` (set-union upper bound; `dedup` coefficient
  optional).
- **NEGATE** (`QueryNegateImpl`, `Query.h:257`) — anti-join.
  `card(v) = σ_neg · card(u)` (`σ_neg` = fraction surviving the absence gate).
- **JOIN** (`QueryJoinImpl`, `Query.h:755`) — the load-bearing case. A JOIN
  carries `num_pivots` (`Query.h:793`), `joined_views` (`Query.h:789`), and
  `out_to_in` (`Query.h:782`). For pivot columns forming key equivalence `k`
  across `joined_views = {u_0…u_{m-1}}`:
  `card(v) = ρ_k · Π_i card(u_i)`.
  A zero-pivot JOIN (`num_pivots == 0`, only under `@product`) is a Cartesian
  product: `ρ_k = 1`, `card(v) = Π_i card(u_i)`.
- **AGG / KVINDEX** (`QueryAggregateImpl` / `QueryKVIndexImpl`,
  `Query.h:731/255`) — group-by. `card(v) = |distinct group keys| =
  γ · card(u)`, `γ ∈ (0,1]` the grouping factor (per aggregate-multiplicity
  semantics, folds over DISTINCT over()-projection tuples — CLAUDE.md).
- **INSERT** (`QueryInsertImpl`, `Query.h:259`) — sink; `card` = its input.

### 2.2 The fixpoint case at L1

A Query-graph SCC (a recursive relation) is condensed to a single node whose
card is the declared closure symbol `Z_v` (§1.1). L1 does **not** unroll
rounds — that is L2's job. L1 only needs the *final* cardinality to reason
about downstream cost and about whether a rewrite changes the closure. For a
right-linear recursion `path(A,C) :- edge(A,B), path(B,C)` the closure card is
`Z_path` (declared); the per-step join feeding it has
`card = ρ · card(edge) · Z_path`, but L1 records only the sink card `Z_path`.

### 2.3 L1 logical cost `Klog`

`Klog(v) = Σ over the intermediate views materialized to produce v` of their
card. This is deliberately coarse — it counts rows produced, not the physical
access to produce them. Its only job is **rewrite comparison**: `Klog` under
rewrite A vs rewrite B. It is the number the demand-transform decision reads
(§7.1). The precise row-touch total is L2's `K_total`.

---

## §3 LAYER L2 — THE REL/DR-IR SCHEDULED-EXECUTION MODEL

**Home:** `lib/Rel/Rel.h`. **Purpose:** the true physical cost of what runs,
keyed on the scheduled op list. **Unit:** row-touches `∈ K`.

L2 walks `DRFlowGraph::ops` in the **already-validated linear order** (the
schedule is a checked linearization, CLAUDE.md; no re-derivation needed) and
assigns each `DROp` a `K(op)`. Each op's cost is a function of (a) the cards
of the tables it reads/writes — **imported from L1's `CardEnv`** — and (b) its
own `Lowering` / effect set / access-plan spine.

### 3.1 Per-op cost `K(op)` by `DROpKind`

Grounded in the `DROpKind` vocabulary (`Rel.h:123-268`) and the access-plan
spine (`PlanNode`, `Rel.h:485`; `DREffect`, `Rel.h:342`):

| `DROpKind` | `K(op)` | grounding |
|-----------|---------|-----------|
| `kIngestFold` | `B` (message batch size), one `FindOrAdd` each | `Rel.h:136`; effect `kCounter` `Rel.h:353`; `Table.h:172` |
| `kIngestLoop` | `B` (table-less receive, one VAR/col) | `Rel.h:229` |
| `kEagerForward` | `card(input)` (plumbing, 1/row) | `Rel.h:162` |
| `kEagerSelect` | `card(input)` | `Rel.h:201` |
| `kEagerInsert` | `card(input)` `FindOrAdd`s | `Rel.h:169`; `Table.h:172` |
| `kEagerCompare` | `card(input)`; out `= σ·card` | `Rel.h:177` |
| `kEagerGenerate` | `card(input)·c_functor`; out `= fanout·card` | `Rel.h:184` |
| `kEagerUnion` | `Σ card(inputs)` | `Rel.h:190` |
| `kEagerJoin` | dispatch edge — cost carried by `kJoinEmit` (per-visit record, no work) | `Rel.h:209` |
| `kJoinEmit` | **the join scan**: `card(driver) · avg_section = card(output)`; `order=` (`Rel.h:750`) picks the driver | `Rel.h:247` |
| `kEagerProduct` / `kProductEmit` | `Π card(sides)` (Cartesian) | `Rel.h:223/258` |
| `kNegateGate` | `card(input) · c_find` (one point-test on negated table); out `= σ_neg·card` | `Rel.h:134`; effect `kFlagRead` `Rel.h:359` |
| `kGroupUpdate` | `card(summarized input)` folds (`@invertible` O(1)/fold; `@recompute` rescans the group multiset) | `Rel.h:139`; algebra `Rel.h:280` |
| `kSubgraphInstantiate` | keyed-instance birth/rebuild rescan: `card(edge frontier for the key)` | `Rel.h:148` |
| `kInstanceDeath` / `kInstanceSeal` | `card(instance)` drain / O(1) pointer swap | `Rel.h:153/159` |
| fixpoint family (`kSeedFold` `kFixpointFire` `kChainFold` `kClaimDrain` `kRetire` `kRederive` `kCommitSweep` `kFrontierFilter`) | summed over `DRRound`s (§3.3) | `Rel.h:126-145`; `DRRound` `Rel.h:787` |

The **`kJoinEmit` cost is the crux**. A pivot join with driver-side card `L`
and a keyed section walk on the other side has
`K(kJoinEmit) = Σ_{pivot∈driver} |section(pivot)| = ρ_k · L · card(other) =
card(output)`. The `order=` attribute (`Rel.h:750`, the ContinueJoinOrder
drain key) names which side drives — the cost model reads it directly rather
than choosing. The join's access-plan spine (`PlanNode` chain, `Rel.h:485`, a
left-deep spine with a `Lowering` per node, `Rel.h:501`) gives the per-level
lowering so the cost is `Σ_levels K(access-level)`, exactly the physical
`scan_next` nesting.

### 3.2 Reading the access-plan spine

For a `kFixpointFire`/`kSeedFold`/`kChainFold`, the `DRArm.body` is a
`PlanNode` spine (`Rel.h:529`). Each `PlanNode` (`Rel.h:485`) carries
`lowering` (`Rel.h:501`), `bound_cols` (`Rel.h:494`, the index identity), and
`bound_col_sources` (`Rel.h:498`, `kRowSlot`/`kInstanceKeySlot`). Cost of the
spine = product of per-node output cards along the chain; `K` of the spine =
sum of per-node access costs (§1.2 table) evaluated at the running card. The
`kFold` leaf (`Rel.h:509`) is a `kCounter` write, `K = 1` per surviving row.
**This is where the model is most precise: the DR-IR already recorded the
exact lowering, so L2 is not estimating the plan, only its magnitude.**

### 3.3 The fixpoint / differential case at L2

An SCC lowers to per-`DRRound` shells (`DRRound`, `Rel.h:787`; `RoundPhase ∈
{kOverdelete, kInsert}`, `Rel.h:785`). Semi-naive evaluation touches each
derived fact once across all rounds, so:

- **Monotone fixpoint** total: `W_scc = Σ_{fire ops o} ρ_o · Z_{driver(o)} ·
  card(other(o))` — the whole closure `Z` flows through each fire once. For
  right-linear tc: `W = ρ · Z_path · card(edge)`. The round count `R_scc`
  governs the loop-overhead / **commit-sweep multiplier**: `+ R_scc ·
  K(kCommitSweep)` (one Seal per round, `Rel.h:133`).
- **Differential** (deletion-capable) total: the phase split multiplies the
  delta work. `W_diff = W_scc · (1 + 2·φ)` where `φ ∈ [0,1]` is the declared
  **retraction fraction** — OVERDELETE re-touches the retracted slice,
  REDERIVE rescans supported survivors (`Rel.h:131` `kRederive`, C_r>0 gate).
  Monotone tables (`SweepFlavor::kMonotone`, `Rel.h:308`) have `φ = 0` (no
  deaths). This is the `-demand-retract` config's extra cost, made symbolic.

`R_scc` and `Z_v` are the only recursion inputs the sidecar must supply; the
op structure supplies everything else.

---

## §4 THE INTERFACE — `CardEnv` AND THE CONSISTENCY OBLIGATION

### 4.1 The interface object

`CardEnv : (QueryViewId ∪ TableModel) → C`. L1 **produces** it: after §2's
pass, every Query view has a `card`. L2 **consumes** it: a `DROp` names its
tables (`op.eager_view`, `op.args table=%table:N`, `DRTable.member_views`
`Rel.h:416`), and every DR table `T` is the model of one or more Query views
`v` (the `member_views` identity link, `Rel.h:412-417`). L2 imports
`card(T) := card(v)` from `CardEnv` and never recomputes cardinality — it only
computes physical `K` from those cards. **Cardinality has exactly one author
(L1); cost-in-row-touches has exactly one author (L2).** That single-author
rule is the whole point of the split.

### 4.2 The consistency obligation (`V-COST-XCHECK`)

> For every DR table `T` with model-linked Query view `v`
> (`DRTable.member_views`, `Rel.h:416`), L2's **produced-row count** for `T`
> — the sum of output cards of the ops that write `T` — must equal L1's
> `card(v)`.

Symbolically: normalize both expressions in `C` (canonical polynomial form
over the base symbols) and assert equality. Numerically: plug the sidecar's
`let` bindings (§5) and assert within tolerance. A divergence means one layer
has a bug (a mis-modeled op cost, or a mis-propagated cardinality) — it is a
*model* validator, in the same spirit as the always-on DR graph validators
(`V-PRED-XCHECK` etc., CLAUDE.md), not a program validator. It runs on any
`.cost` case and gates blessing a `.cost` golden.

This obligation is what earns the second layer its keep (§6): without it, L2
would be a free-floating second cost estimate that could silently disagree
with L1. With it, the two are provably one model viewed twice.

---

## §5 THE SURFACE — THE `.cost` SIDECAR

A `.cost` file is a peer of `.batches` / `.drflags` / `.eqgate`
(`tests/OptDiff/cases/<name>.cost`). Grammar (line-oriented, `#` comments):

```
# base cardinalities: one per input message/relation, symbol on the RHS
card add_edge/2            = N
card demand__neighborhood_bf = D

# selectivities: per join key equivalence (by pivot column) and per filter
sel  join@From             = rho5      # push-down guard pivot
sel  join@Start            = rho8      # projection guard pivot
sel  cmp@...               = sigma

# recursion: closure size + round count per recursive view / SCC
closure path               = Z
rounds  path_scc           = R
retract path               = phi       # differential retraction fraction

# functor / grouping knobs (optional; default fanout=1, gamma=1)
fanout gen_i32_bbf         = 1
group  agg_over            = gamma

# OPTIONAL numeric evaluation: bind symbols for a concrete K_total number
let N = 1e6, D = 10, rho5 = 1e-3, rho8 = 1, Z = 1e7, R = 20, phi = 0

# ASSERTIONS the instrument checks (the "does it do less?" gate):
#   compares K_total across configs / rewrites under the given regime.
assert cost[-demand]  <  cost[normal]  when  D << N
assert term8 subset term5              on   supported-slice
assert xcheck                          # run V-COST-XCHECK on this case
```

Semantics:
- `card`/`sel`/`closure`/`rounds`/`retract` bind symbols the two-layer pass
  reads.
- `let` optionally grounds a numeric evaluation; without it the instrument
  emits **symbolic** `K_total(config)` expressions (the default — symbolic is
  the deliverable, numbers are a convenience).
- `assert cost[A] < cost[B] when <predicate>` is the FLAG-WHEN-IT-DOES-MORE
  gate: it fails the instrument (loud, like a validator) if a "do less"
  feature's `K_total` is not actually less under the stated regime. This is
  the general mandate of grounding §6.4.
- `assert term8 subset term5` invokes the §7 redundancy check.
- `assert xcheck` runs §4.2.

Output artifact: `<name>.cost.stdout` — the symbolic `K_total(config)` per
config plus each assertion's verdict. Blessed like any golden, only via an
explicit review (never auto-green), per the golden discipline (CLAUDE.md).

---

## §6 WHY TWO LAYERS EARN THEIR KEEP

The extra complexity (a second cost author + a consistency validator) is
justified by a task **neither layer can do alone**:

1. **The transform decision must be pre-lowering.** `ApplyDemandTransform`
   mints the guard joins on the Query graph (grounding §5). To decide *not*
   to mint step-8, the model must run on the Query graph — before any Rel op
   exists. A Rel-only model would have to lower every candidate rewrite to
   cost it, which is both expensive and circular (you lower to decide whether
   to lower). ⇒ **L1 is mandatory for the decision.**

2. **The confirmation must be post-schedule.** "How much did we actually
   save?" is `−1 kJoinEmit`, `−1 intermediate table`, `−ΔK` row-touches — all
   only visible after DR lowering and scheduling (`kJoinEmit` is emitted by
   `LowerJoinEmit`, CLAUDE.md). A Query-only model cannot see that
   `kEagerJoin` is a *per-visit dispatch record* while `kJoinEmit` is the
   *once-per-join* real `TABLEJOIN` — the 4-vs-2 distinction (§7.3) is a
   scheduling fact. ⇒ **L2 is mandatory for the confirmation.**

3. **The consistency obligation (§4.2) closes the loop.** L1 claims "output
   identical" (card(pub) unchanged). L2 verifies card(pub) is unchanged across
   the two lowerings while `K_total` drops. Without the shared `CardEnv` and
   `V-COST-XCHECK`, L1's "does less" claim and L2's "runs less" measurement
   could disagree and no one would notice. The obligation makes them one
   model. ⇒ **the interface is mandatory for trust.**

A single-layer model collapses one of these three and loses either the
decision, the confirmation, or the trust. The two-layer split is the minimal
structure that keeps all three.

---

## §7 THE DOUBLE-JOIN VERDICT AT BOTH LAYERS

The first acceptance test (grounding §3-§6). The witness:
`neighborhood(bound Start, free Node) : edge(Start,Node)` over
`edge(From,To) : add_edge(From,To)`, under `-demand`.

### 7.1 L1 renders the DECISION (term-8 ⊆ term-5)

From `mono.demand.df`, the two guard joins (grounding §3):

```
tuple.4 (c8)  = d_neighborhood_bf          card = D           (demand keys, on Start)
join.6:  From <- d.c8 , edge.From          -> tuple.3 (Start,Node)   [PUSH-DOWN, step-5]
join.7:  Start <- d.c8 , tuple.3.Start      -> tuple.5 -> pub          [PROJECTION, step-8]
```

L1's cardinality pass (§2.1 JOIN rule), with `N = |edge|`:

- **term-5** `card(tuple.3) = card(join.6) = ρ_From · card(d) · card(edge)
  = ρ_From · D · N`. Concretely = the demanded neighborhood, `Σ_{s∈d} deg(s)`.
  The key insight L1 records: **`π_Start(tuple.3) ⊆ d`** — join.6 pivoted
  `edge.From` against `d` on `From`, so every output row's `Start` came from
  `d` (this is a *provenance* fact L1 reads off the JOIN's `out_to_in`,
  `Query.h:782`: the pivot column `Start` is an alias of `d.c8`).

- **term-8** `card(tuple.5) = card(join.7) = ρ_Start · card(d) · card(tuple.3)`.
  join.7 pivots the SAME `d` against `tuple.3` on `Start`. Its right input
  `tuple.3` already satisfies `π_Start(tuple.3) ⊆ d = keys(left)`. L1's
  **identity-join predicate**:

  > An equijoin `A ⋈_k B` is an IDENTITY join (removes nothing) iff
  > `π_k(B) ⊆ keys_k(A)`. Then `ρ_k = 1` and `card(A ⋈_k B) = card(B)`.

  Here `A = d` (keyed on Start), `B = tuple.3`, and `π_Start(tuple.3) ⊆ d`
  was just established. Hence **`ρ_Start = 1`, `card(tuple.5) = card(tuple.3)`
  — i.e. term-8 = term-5, and the SET produced is identical: term-8 ⊆
  term-5.** join.7 cannot drop a row; it only re-scans `tuple.3` and
  re-materializes it into `tuple.5`.

  **Symbolically:** `term8 = ρ_Start · D · (ρ_From·D·N)` collapses under
  `ρ_Start = 1` (the subset condition) to `ρ_From·D·N = term5`. The wasted
  work L1 attributes to step-8 is `Klog(join.7) + card(tuple.5)` =
  `card(tuple.3) + card(tuple.3) = 2·ρ_From·D·N` of intermediate row
  production for **zero** cardinality change. ⇒ **L1 DECIDES: drop step-8.**

  The identity-join predicate is exactly the cardinality/subset reasoning the
  canonicalizer lacks (grounding §5) — the cost model supplies it.

### 7.2 The BOUNDARY (the honest caveat, §4 of grounding)

L1's predicate `π_k(B) ⊆ keys_k(A)` is **conditional**, and the model states
where it fails — this is the "characterize the boundary" mandate:

- **On-slice (redundant):** single-adornment, From-preserving. `d` is keyed
  exactly on the query's bound column and every derived body row preserves it,
  so `π_Start(tuple.3) ⊆ d` holds. term-8 ⊆ term-5. Drop is safe.
- **Off-slice (load-bearing):** step-8 becomes real (`ρ_Start < 1`, term-8 ⊊
  term-5) exactly when `π_Start(B) ⊄ keys(d)` — i.e.
  1. the query reads `p` at a **different adornment** than the body demanded
     (the projection column is not the demand key),
  2. a **non-From-preserving recursion** (a derived body row carries a
     `Start` not in `d` — the closure escapes the seed), or
  3. a **multi-clause** query whose clauses demand divergent key sets.
  In each, the projection guard removes rows the push-down guard did not, so
  `ρ_Start < 1` and dropping it would over-answer. **The cost model's
  drop-step-8 rewrite fires only when it can prove the subset** — the
  boundary is the subset predicate itself, evaluated per (query, adornment).

### 7.3 L2 renders the CONFIRMATION (emitted join count + `K_total` drop)

L2 reads the census (grounding §2). **Before** the L1 rewrite
(`mono.demand.rel`): `kEagerJoin=4`, `kJoinEmit=2`, tables `{%table:8 (d),
%table:11 (edge), %table:15 (tuple.3), %table:4 (pub)}`. The two
`kJoinEmit` ops (`Rel.h:247`) are two real `TABLEJOIN`s:

```
op.11 kJoinEmit ... table=%table:15 order=3 seq=0   # join.6 push-down
op.12 kJoinEmit ... table=%table:4  order=5 seq=1   # join.7 projection
```

`K_total(-demand, before)` (§3.1):
```
K = K(kIngestFold d) + K(kIngestFold edge)          = D + N
  + K(kJoinEmit@table15)  [join.6]                  = ρ_From·D·N          (= card tuple.3)
  + K(kJoinEmit@table4)   [join.7]                  = ρ_Start·D·card(t3)  = ρ_From·D·N   (ρ_Start=1)
  + forwards/inserts                                = O(card touched)
  = D + N + 2·(ρ_From·D·N) + O(...)
```

**After** the L1 rewrite (drop step-8), recompile → census shows
`kEagerJoin=2`, `kJoinEmit=1`, and `%table:15` **or** `%table:4`'s
intermediate gone (join.7 + tuple.5 removed; pub fed directly by tuple.3).
`kEagerJoin` drops by **2** (each join view emits one `kEagerJoin` *per
dispatch edge* — `Rel.h:209`, per-visit; the removed join.7 had 2) and
`kJoinEmit` drops by **1** (the once-per-join emission). This is the
4→2 / 2→1 census delta, and L2 explains *why* those two numbers move by
different amounts — a distinction invisible at L1.

`K_total(-demand, after) = D + N + (ρ_From·D·N) + O(...)`. The saving is
exactly `ΔK = ρ_From·D·N = card(tuple.3)` row-touches — the second join scan —
**plus** the elimination of one intermediate table's storage. And crucially:

**`V-COST-XCHECK` confirms L1's "output identical" claim** — `card(pub)` =
`card(tuple.3)` = `ρ_From·D·N` in *both* lowerings (L2 sums the ops writing
`%table:4` and gets the same expression before and after). So the two layers
agree: L1 said "term-8 ⊆ term-5, drop it, no cardinality change"; L2 confirms
"one `kJoinEmit` fewer, one table fewer, `−ρ_From·D·N` row-touches, `card(pub)`
unchanged." The verdict is rendered, consistently, at both layers.

### 7.4 The config comparison (normal vs -demand — mandate item 3)

`normal` (`mono.normal.rel`): `kEagerJoin=0`, `kJoinEmit=0`, one table
(`%table:4`). The bound query answers by a single **index-probe** on the
materialized relation's bound column: `K_total(normal) ≈ σ·N` per probed key
(a `kPointTest`/`kSectionWalk`, §1.2) — **zero joins, zero forced second
materialization**. `-demand` pays `D + N + ρ_From·D·N` (after dropping
step-8) — a `D`-channel ingest, a **forced** `edge` materialization
(`kIngestFold @%table:11`), and one guard join. So:

> `-demand` beats `normal` iff **pruned derivations · K_derive > machinery
> cost**. For a 1-hop message-rooted shape the whole relation is demanded
> (`D ≈ |From-domain|`, nothing pruned) and the machinery is pure overhead —
> the seed §7 claim ("shallow / message-rooted shapes never pay") falls out
> as `K_total(-demand) > K_total(normal)` whenever `D` is not `≪` the
> reachable set. The `.cost` `assert cost[-demand] < cost[normal] when D << N`
> encodes exactly this gate and FAILS on the mono witness — correctly flagging
> that `-demand` on a message-rooted 1-hop query does MORE, not less.

---

## §8 WHAT THIS MODEL CANNOT (YET) REPRESENT

Stated plainly, per the mandate:

1. **Skew beyond a scalar `ρ`.** One selectivity per key approximates the
   owner's "distribution of common keys" as a single coefficient, not a
   histogram. Correlated / heavy-tailed key distributions (a few hot keys
   dominating a join) are averaged away. A histogram-valued `ρ` is future
   work.
2. **Correlated selectivities across keys.** Multi-key joins assume
   independence (`ρ` multiply). Real correlation between pivot columns is
   unmodeled.
3. **Recursion magnitude is an INPUT, not derived.** `Z_v` (closure size) and
   `R_scc` (round count) must be supplied in the `.cost` sidecar; the model
   cannot predict transitive-closure size or fixpoint depth from graph shape.
   A genuinely novel recursive shape needs an estimate to get a number
   (symbolic `K_total` still emits without them).
4. **Hash-collision / rehash amortization** (`Table.h:206` `Rehash`, 7/8 load)
   is treated as O(1) expected — the `c_find ≈ 1` leaf. Pathological
   collision chains are invisible.
5. **Cache / branch / memory-hierarchy effects.** By construction: `K` counts
   row-touches, not cycles. That is `bench/`'s lane (the mechanical-sympathy
   referee). This model is the analytic counterpart, deliberately physical
   only down to row-touches.
6. **Functor cost is a unit (`c_functor`) per call.** An expensive MAP/WASM
   functor (`kEagerGenerate`) is one work unit; its internal cost is opaque.
7. **Dead-row compaction / commit-sweep amortization** (the data-structures
   epoch compaction, `Table.h:108`) is amortized to zero — the model does not
   charge the periodic compaction pass.
8. **Compile-time cost of the optimizer modes themselves.** The 4 opt modes
   are modeled only via the op census they produce (their *runtime* cost);
   the compiler's own time to run CSE/canonicalization is out of scope.
9. **The `kSeek` substrate (D5 WCOJ).** Reserved (`Rel.h:483`); costed as a
   `log` placeholder, not yet a real seekable-iterator model.

---

## §9 KEY RISKS / SOUNDNESS GAPS (self-critique)

1. **The identity-join predicate (§7.1) rests on a provenance fact
   (`π_Start(tuple.3) ⊆ d`) that L1 must extract from `out_to_in`
   aliasing.** If the demand transform's column aliasing is not faithfully
   readable from the Query graph after CSE (grounding §4 notes CSE folds
   `raw_seed` into `d_reader`, GT-3), L1 could mis-detect the subset and drop
   a load-bearing step-8. The predicate must be *conservative*: only drop when
   the subset is provable, default to keeping. A false "subset holds" is an
   over-answer miscompile — the highest-severity failure mode.
2. **`V-COST-XCHECK` compares symbolic expressions.** Polynomial-normal-form
   equality in `C` is decidable but the `ρ`/`σ` symbols make two *structurally
   different* schedules that happen to produce the same card look equal only
   after correct algebraic simplification. A weak normalizer could raise false
   XCHECK failures (annoying) or miss real divergences (dangerous). The
   normalizer's completeness is a real obligation.
3. **`Z_v`/`R_scc` as free inputs mean the recursion verdicts are only as good
   as the supplied estimates.** A wrong `Z` makes the demand-vs-normal
   comparison for a recursive query arbitrary. The model can emit symbolic
   answers, but any *numeric* "does it do less" gate on a recursive case is
   hostage to the sidecar's honesty. Mitigation: keep recursive gates
   symbolic (assert `ρ<1` structural facts) rather than numeric where
   possible.
4. **Two cost authors can still drift if the `member_views` link is stale.**
   The consistency obligation assumes L2 can map every table to its L1 view
   via `DRTable.member_views` (`Rel.h:416`). Model-shared tables (one table,
   multiple identity-distinct feeders — the E-107 shape) make this a
   many-to-one map; summing L1 cards over feeders must exactly reconstruct
   L2's produced count. A mis-summed shared table silently breaks XCHECK's
   guarantee.
5. **The four opt modes may reshape the census such that "the same" logical
   rewrite has different L2 confirmations per mode.** The double-join drop
   must be re-confirmed in all four modes (a `.cost` case is mode-crossed like
   every golden); a rewrite that helps under `opt` but is a no-op under `none`
   (because the join was never CSE-chained) needs per-mode `K_total`, which
   the surface supports but the worked example (§7) only exercises in the
   optimized mode.
