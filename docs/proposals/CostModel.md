# The Cost Model — an analytic, per-config explosion referee

> **Status.** Binding design, written 2026-07-30 at tip `eecea847` on branch
> `keyed-instances`, adjudicating a three-lane design fleet (rel-anchored /
> query-anchored / two-layer) plus an independent double-join verdict lane, all
> under `docs/proposals/CostModel.artifacts/`. Empirical anchor:
> `CostModel.artifacts/grounding-double-join.md` (the live `mono.*.df` /
> `mono.*.rel` census). Companion: memory `[[demand-cost-model]]`,
> `KeyedInstances.artifacts/recursive-demand-seed.md` §7. This is the analytic /
> symbolic counterpart to `bench/` (PerfRoadmap §2): `bench/` measures wall-clock
> after the fact; this instrument estimates, a priori and per-IR-configuration,
> whether a "do less" feature actually does less.

---

## §0 THE DECISION — IR-HOME IS TWO-LAYER (Query decides, Rel confirms)

The owner's pivotal question was *"maybe magic sets as done there, or maybe you
did it on the rel?"* — where does cost live. The fleet made three cases; the
adjudication picks **two layers, with a single-author rule per quantity**:

- **L1 — the CARDINALITY / PROVENANCE model on the Query dataflow graph**
  (`lib/DataFlow/Query.h`: SELECT/TUPLE/JOIN/MERGE/CMP/MAP/NEGATE/AGG/KVINDEX/
  INSERT). L1 owns **cardinality** (how many rows each view holds) and **keyset
  provenance** (which demand relations a column provably ranges within). It is the
  layer where `ApplyDemandTransform` (`lib/DataFlow/Demand.cpp`) and the dataflow
  optimizer act, so it is the only layer that can judge a transform decision in
  the transform's own vocabulary — JOIN pivots, column edges (`out_to_in`,
  `Query.h:782`), demand relations — and it is where the double-join subset proof
  is even *expressible* (Rel has erased the column edges).

- **L2 — the PASS-COUNT / PHYSICAL model on the Rel / DR-IR** (`lib/Rel/Rel.h`,
  `lib/Rel/Format.cpp`). L2 owns **work in row-touches**: it counts *passes not
  appearances* (a Query JOIN becomes N per-visit `kEagerJoin` markers, `Rel.h:209`,
  cost 0, plus ONE cost-bearing `kJoinEmit`, `Rel.h:247`), reads the physical
  **access lowering** the DR-IR already committed to (`Lowering ∈ {kPointTest,
  kSectionWalk, kFullScan, kSeek}`, `Rel.h:483`), and prices the differential,
  fixpoint-round, and keyed-instance machinery that has no Query-graph existence.

**Why not a single layer.** The fleet's own critiques are decisive:

- *Rel-only fails* because its flagship proof secretly runs on Query-graph pivot
  provenance, and its acceptance-test flow carries **no cardinality objects at
  all** — `mono.demand.rel` has empty `deps:`, empty `rounds:`, zero DRVecs on the
  eager web; the `D`, `D·f` cardinalities live on nothing in the Rel. Cardinality
  is a relational-algebra property of Query nodes, not of DROps.
- *Query-only fails* because it is structurally blind to the named first-class
  configs it must adjudicate: the `-demand-instance` keyed rescan emits **zero
  joins** (a per-key `StateCellStore` section-walk, `[[demand-cost-model]]`), the
  `@recompute` per-group rescan (`Algebra::kRecompute`, `Rel.h:280`), and the
  4-vs-2 `kEagerJoin`/`kJoinEmit` pass-count asymmetry are all Rel/runtime facts.
  A "κ constant annotation" cannot represent a per-op structural multiplier that
  varies by dispatch topology and by physical lowering.

**The single-author rule (the fix for the two-layer design's tautological
cross-check).** Each *quantity* has exactly one authoritative layer: **L1 authors
cardinality, L2 authors row-touch cost.** But L2 does **not** merely import L1's
card and re-emit it (that made the fleet's `V-COST-XCHECK` a tautology — it
checked L1 against L1). Instead **L2 independently *derives* each written table's
produced-row count from the op's own inputs and its recorded `Lowering`**, and
`V-COST-XCHECK` asserts that derivation equals L1's imported card. When the two
disagree, one layer has a modelling bug — a real check, in the always-on DR
validator idiom (`V-PRED-XCHECK`, CLAUDE.md). See §3.4.

**Corollary — where the double-join fix lands.** The cost model's *judgement*
(is step-8 redundant, is it worth caring about, where is the boundary) is a
two-layer read; the *fix*, if taken, is a **structural peephole in
`ApplyDemandTransform`** gated by a provenance certificate — **not** a cost-guided
runtime rewrite (§5.5). Cost-decides-priority, structure-decides-safety.

---

## §1 THE COST ALGEBRA — SYMBOLIC DOMAIN

### §1.1 Two semirings, two regimes

Cost is symbolic first, numeric only on demand. Two quantity spaces:

- **Cardinality `C`** — multivariate rational-coefficient expressions over a base
  symbol set, in the commutative semiring `(ℝ≥0 ∪ {∞}, +, ·)` extended with the
  monotone operators `min`, `max`, and a bounded least-fixpoint `lfp` (§3.3).
  `C` is *what flows* input→output through the graph.
- **Work `K`** — the same expression shape, but interpreted as **row-touches** and
  *accumulated with `+` along the schedule*, never flowing downstream.
  `TOTAL(config, regime) = Σ_op K(op)`. This is the analytic peer of `bench/`
  wall-clock: `bench/` counts cycles post-hoc, `K` counts row-touches a priori.

Every cost is stated in **one of two regimes**, because the runtime is
message-driven and the two currencies are otherwise incommensurable (the sharpest
fleet critique: a batch-relative `kIngestFold` cannot be summed with a
full-cardinality `kJoinEmit`):

- **BUILD regime (cold / full).** Every input at full size, tables at steady
  state, the fixpoint run to closure. This is the regime for *"how big is the
  closure, and does demand shrink it?"* — the recursive-demand payoff question.
- **DELTA regime (warm / incremental).** A batch `δ_m` arrives; work touches the
  delta and the standing frontier, not the full table; guard joins re-fire on the
  delta; retraction `ρ` applies. This is the regime for *"what does one more edge
  cost?"* and for all differential / `-demand-retract` accounting.

A config total is reported per regime; the two are never added. The double-join
redundancy (§5) is **regime-invariant** (structural); the "does demand pay"
crossover (§5.4) is a BUILD-regime statement; retraction cost (§2.6) is
DELTA-regime.

### §1.2 The symbol basis (bound to real IR objects)

| symbol | meaning | IR binding |
|--------|---------|-----------|
| `N_m`  | rows of message `m` (BUILD: total; DELTA: batch `δ_m`) | `kIngestFold.ingest_message` (`Rel.h:659`) |
| `T_t`  | steady-state live rows of table `t` | `DRTable.model` (`Rel.h:413`) |
| `D_a`  | distinct demanded keys of adornment `a` | `kSubgraphInstantiate` / demand `kIngestFold` (`Rel.h:704`) |
| `K_{t.c}` | distinct values of column `c` of `t` | index on `t` keyed `c` (`Table.h`) |
| `f_{t.c}` | fan-out: avg rows of `t` per distinct value of key `c` = `T_t / K_{t.c}` | — |
| `σ_{op}` | residual selectivity of a filter/join op ∈ [0,1] | spine gate `PlanNode.pred` |
| `π_v` | present-fraction surviving a NEGATE against view `v` | `kNegateGate` (`Rel.h:134`) |
| `γ_g` | grouping factor of aggregate `g` = `K_group / T_in` | `kGroupUpdate` (`Rel.h:139`) |
| `Z_S`  | closure size of SCC `S` (fixpoint rows) | `lfp` over the SCC tables |
| `R_S`  | rounds-to-closure of SCC `S` | `DRRound` count (`Rel.h:787`) |
| `ρ_t`  | retraction fraction of differential table `t` per batch ∈ [0,1] | `DRTable` differential + `SweepFlavor` (`Rel.h:308`) |

`{T, f, K}` are inter-derivable (`T = f·K`); the sidecar supplies any two, the
model derives the third. Unknowns default conservatively (§1.4).

### §1.3 The key-distribution model — the owner's `N·M` vs `0.5N·0.7M`

For a JOIN of views `A, B` on pivot column-set `P`, the output cardinality is

```
    Card(A ⋈_P B) = σ_P · Card(A) · Card(B)
    σ_P = ( Σ_{k ∈ dom(P)} a_k · b_k ) / ( |A| · |B| )      # per-key row-count products
```

with `a_k, b_k` the per-key row counts on each side. `σ_P = 1` is the Cartesian /
`@product` case (`num_pivots == 0`, `Query.h:793`); a functional / foreign-key
right side (one row per key) gives `σ_P = 1/K_{B.P}`, i.e. `Card = Card(A)` — a
probe. The owner's `0.5N · 0.7M` is exactly `σ_P` decomposed as *(fraction of A
carrying a joinable key) × (fraction of B) × (per-pair fan-out)*: **`σ_P` is the
single scalar where the join-key distribution enters.** Its honest limit — that
it is a *mean*, blind to heavy-hitter skew — is §7.2, and it is why the model
ranks shape, not wall-clock.

### §1.4 Default population

`f = φ` (free, default 1), `σ = 1` (removes nothing unless proven), `π = 1`,
`γ = 1`, `ρ = 0` (monotone unless the table is differential), `Z_S`/`R_S` free
(never numeric without a `.cost closure:` hint). A bare run yields a symbolic
expression in the surviving free symbols; the owner reads the *form* (does
`-demand` multiply by `D·(1+f)` twice?) before any numbers. **The workload axis
(`D_a`, batch count) is mandatory for a numeric verdict** (§4, §7.1): defaulting
`D` to 1 flips the does-less verdict, so a numeric comparison refuses to run
without a stated workload.

---

## §2 PER-OP COST FUNCTIONS

Cost is `Card : QueryView → C` at L1 and `K : DROp → K` at L2. Both are grounded
op-by-op. **Unit** = one probe / one `TryAdd` / one functor call / one counter
RMW — all O(1) amortized, so we count *invocations* (the cache-line-agnostic
first cut; a later refinement weights units, PerfRoadmap §9).

### §2.1 L1 — the Query-graph cardinality rules

The **house index model** (`Runtime/Table.h`): a bound-column hash index turns a
scan `O(|table|)` into a probe `O(matches)`; a JOIN pivot always materializes a
hash index on the pivot, so a JOIN's *work* is `drive-side probes + emitted
matches`, never `|A|·|B|` unless it is a `@product`.

| node (`Query.h`) | `Card(v)` | `Cost`-shape (index model) |
|------------------|-----------|----------------------------|
| **SELECT** (`:668`) read of relation/message | `N_r`, or `σ·N_r` for a bound-column probe | 0 if ingested elsewhere; else a scan/probe |
| **TUPLE** (`:703`) project/forward | `δ · Card(in)` (`δ≤1` dedup) | one forwarding pass |
| **CMP** (`:908`) filter | `σ_cmp · Card(in)` | one test/row |
| **MAP** (`:796`) functor | `φ_f · Card(in)` (`φ_f` = free-var fan-out) | one functor call/row |
| **MERGE** (`:862`) union | `δ · Σ_k Card(in_k)` | union pass |
| **JOIN** (`:755`), `num_pivots` (`:793`) | `σ_P · Π_i Card(in_i)` (§1.3); `@product` if `num_pivots==0` | `Card(drive) + Card(out)` |
| **NEGATE** (`:950`) antijoin; `is_never` (`:968`) | `π · Card(in)` (present-fraction survives) | one antijoin probe/row |
| **AGG / KVINDEX** (`:826`/`:731`) | `γ · Card(in) = K_group` (DISTINCT over-projection tuples, CLAUDE.md) | see §2.5 (algebra-dependent) |
| **INSERT** (`:974`) sink | — | materialize `Card(in)` |

Two rules earn their keep: the JOIN's **asymmetric** cost is why normal mode
answers a bound query with **zero joins** (an index probe on the materialized
relation, `mono.normal.rel`), and the **`@product` guard** (`num_pivots==0`,
`σ=1`, `Card=Π|in_i|`) surfaces the cross-product explosion immediately — the
primitive the owner most wants flagged, which a Rel-only marker model prices at 0.

### §2.2 L2 — the join family (the double-join subject)

`kJoinEmit` (`Rel.h:247`, `LowerJoinEmit → BuildJoin`) is the **only cost-bearing
join op**. `kEagerJoin` (`Rel.h:209`) and product/pivot markers are per-visit
dispatch records — **`K = 0`** (`Rel.h:209` region; costing them double-counts by
predecessor arity). For a `kJoinEmit` whose pivot vec `P` (from the driving side,
`order=` selecting the driver, `Rel.h:756`) probes scanned table `t` on key `c`:

```
K(kJoinEmit) = Card(P) + Σ_matches = Card(P) · (1 + f_{t.c} · σ_j)
Card(out)    = Card(P) · f_{t.c} · σ_j          # imported to L1's CardEnv as the join view's card
```

`σ_j` is the *residual* selectivity after the index equality (the probe is exact,
`Table.h` full-key). A `form=delta` `kJoinEmit` (recursive round, `Rel.h`) costs
the same per round with `Card(P)` = the round frontier `δ_r` (§3.3). **The
scanned-side table `t` and its pivot key are read from the op's `eager_view`
(the `QueryJoin`), not from `kJoinEmit.table=`** (which is the join *view's* model,
`Rel.h:744`) — the pass-count home is Rel, the join *structure* is a Query fact the
op points back to. This is the honest form of the "count passes not appearances"
discipline (fleet salvage) with the join-structure gap the rel critique flagged
closed by the `eager_view` back-pointer.

### §2.3 L2 — @product (the explosion primitive — NOT cost-zero)

`kProductEmit` (`Rel.h:258`) is cost-bearing and **propagates a product
cardinality downstream**; `kEagerProduct` (`Rel.h:223`) is a cost-0 marker:

```
K(kProductEmit)  = Π_i Card(side_i)          # materialize the cross-product
Card(out)        = Π_i Card(side_i)          # NOT cardinality-transparent
```

A "do less" refactor that introduces a `@product` therefore scores as *more
expensive*, and its `|s|·|t|` output flows into every downstream cost. (Rel-only's
`cost=0` for this op was the fleet's single most dangerous gap.)

### §2.4 L2 — negation / anti-join (cost AND selectivity)

`kNegateGate` (`Rel.h:134`) carries a real `kFlagRead` membership probe per row
and *changes downstream cardinality*:

```
K(kNegateGate) = Card(in)                    # one point-test on the negated view/row
Card(out)      = π_v · Card(in)              # present-fraction survives the absence gate
```

A negate that filters 90% (`π=0.1`) shrinks every downstream join accordingly —
not cardinality-transparent. `is_never` (`Query.h:968`) gates on `Present` (F18);
its `π` is the always-present fraction.

### §2.5 L2 — the aggregate algebra (invertible vs recompute — the tradeoff the selector exists for)

`kGroupUpdate` (`Rel.h:139`) output card is `γ · Card(in) = K_group` (one row per
group), but the **work depends on `Algebra` (`Rel.h:280`, `Rel.h:690`)** — this is
the entire "does it do less?" question for aggregates, and it must not collapse to
one number:

```
Algebra::kInvertible : K = Card(in-delta)                         # O(1) fold/unfold per touched row
Algebra::kRecompute  : K = Σ_{g ∈ touched-groups} |group_g|       # per-group multiset rescan (f_reduce over counts[])
```

In the DELTA regime `kRecompute` over a hot group is `#touched · |group|` —
superlinear where `kInvertible` is `#touched`. `config_agg_2` (descending-max
`@recompute` retraction) is the corpus witness the flat "one fold/row" model gets
wrong; the algebra selector `Rel.h:280` is read straight off the op.

### §2.6 L2 — the ingest, differential, and fixpoint families

```
K(kIngestFold_m)  = N_m                       # one counter RMW per message row (Rel.h:659; +1/-1 pair ⇒ 2·N_m for deletion-capable)
K(kIngestLoop_m)  = N_m                       # table-less receive shim (Rel.h:229)
K(kEagerInsert)   = Card(in)                  # terminal TryAdd (Rel.h)
K(kEagerCompare)  = Card(in) ; out = σ·Card   # CMP marker (Rel.h)
K(kEagerUnion)    = Σ_members Card(member)    # MERGE TryAdd into the shared model (§7.5 caveat: shared pub counted once)
K(kEagerForward/Select) = 0                   # pure plumbing (Rel.h:164)
```

**Differential (DELTA regime).** Per differential table `t`
(`SweepFlavor::kDifferential`, `Rel.h:308`), OVERDELETE → REDERIVE → INSERT:

```
diff_mult(t)      = 1 + ρ_t + rederive_fan(t)   # overdelete pass + rederive rescan of supported survivors
K(kClaimDrain_t)  = Card(delta_queue_t) · diff_mult(t)   # Rel.h claim drain
K(kCommitSweep_t) = T_t + [compaction fires]·T_t          # Rel.h:133; compaction gated dead≥live/4096-floor ⇒ 0 for suite-sized
```

`rederive_fan(t)` is the support-set re-scan cost — a per-table sidecar input,
NOT derivable from the graph (§7.4: cascade-retraction fan-out is the honest
limit; `ρ` of a derived table is not computed from input `ρ`). Monotone tables:
`ρ=0`, `diff_mult=1`, `kCommitSweep → Seal = Card(newly-sealed)`.

**Fixpoint (BUILD regime).** `kFixpointFire` (`Rel.h:127`) + `kChainFold` inside a
`DRRound` shell (`Rel.h:787`), semi-naive over the round frontier `δ_{r-1}`
(§3.3). The per-round work reads the recursive JOIN's *actual* drive and other
cards, so a **linear** recursion (`path ⋈ edge`, other side bounded `N`) costs
`Σ_r δ_{r-1}·(1+f) ≈ Z_S·(1+f)` — once per closure row — while a **non-linear**
self-join (`path(x,z):-path(x,y),path(y,z)`, both sides the growing relation)
costs `Σ_r δ_{r-1}·|accum_{r-1}|` — super-linear in `Z_S` (the rel critique's
"says does less when it does more" family). The model reports whichever the join's
two feeding cards produce; it does **not** silently linearize (§7.3 states the
residual: `Z_S` itself is not derivable from graph shape).

### §2.7 L2 — the keyed-instance family (the `-demand-instance` "do less" subject) — PER-KEY multiplicity

`kSubgraphInstantiate` (`Rel.h:148`) is **one static op that fires once per
demanded key**. The fleet's fatal blindness was costing it once; it must carry a
**per-key multiplicity `D_a`** read from its role (`Rel.h:704`):

```
K(kSubgraphInstantiate) = D_a · ( rescan of the input frontier per key )
                        = D_a · f_{input.key}          # band-(a) monotone section-walk per key
K(kInstanceDeath)       = D_a · Card(instance)         # whole-instance drain (Rel.h:153)
```

The `p1` regression (`[[demand-cost-model]]`: a keyed rescan that regressed a
`kPointTest` to a section-walk) is visible directly as the spine `Lowering`
downgrade (§2.8): `f_{input.key}` becomes `T_input` when the rescan is a full
section-walk instead of an indexed probe. **This is the direct handle on the
owner's mandate**: compare `D_a · f` (nested) against the flat `2·D_a·(1+f)` guard
web (flat demand); if the per-key rescan re-materializes more than the flat guard
prunes, the model flags HP-5 over-materialization as a *cost* regression.

### §2.8 The spine multiplier (scan vs probe — the house constraint)

Every access op's base multiplier is the `Lowering` the DR-IR already recorded
(`Rel.h:483`, `PlanNode.lowering`):

```
kPointTest   : base = 1              # Find, one probe
kSectionWalk : base = f_{t.c}        # Index First+Next chain
kFullScan    : base = T_t            # whole table (no bound_cols)
kSeek        : base = log T_t        # reserved, D5/WCOJ placeholder
```

A regressed probe→scan turns `base 1` into `base T_t` — a `T_t`-fold blow-up the
model reports directly from the spine. **Reading the committed lowering off the
DR-IR rather than guessing it is the two-layer model's single best asset** (fleet
consensus): L2 does not estimate the plan, only its magnitude.

---

## §3 THE PROPAGATION RULE

### §3.1 L1 cardinality — forward over the Query condensation

`Card` is computed in a single forward pass in view-topological order over the
acyclic condensation of the Query graph (`depth`/SCC index, `Query.h`; SCCs
handled in §3.3). Each node applies its §2.1 rule. SELECT of an input seeds `N_r`;
SELECT of a demand receive seeds `D_a`.

### §3.2 The keyset-provenance lattice `Prov` (the subset engine)

To mechanize the double-join subset proof, each **output column** carries

```
Prov(col) ⊆ 𝒟     # the SET of demand relations d such that col's value is
                    # PROVABLY a member of π_key(d).  Ordered by ⊇ (more memberships = lower).
```

This resolves the rel design's self-contradiction (its `dc` was both "meet at ⊥"
and "⊔ raise") by making `Prov` **set-valued**, with a single discipline:

- **⊥-default, raise only on proof.** Every column starts `Prov = ∅`. Membership
  is *added* only where provably present. This is the entire soundness argument
  (§8.1) — the safe direction is under-approximation.
- **SELECT of demand receive `d`**: each output col gets `Prov ∋ d`.
- **Guard JOIN** `read ⋈ demand_side on P` (`MintGuardJoin`, `Demand.cpp`): each
  pivot output col — *because its value equals a `demand_side` value* — gets
  `Prov ∪= Prov(demand_side col)` (**union at the pivot column only**, the proven
  raise). Pass-through cols keep their input `Prov`.
- **TUPLE / CMP / SELECT** (pass-through): `Prov` preserved along the forwarded
  column identity (`out_to_in` / `input_columns`, `Query.h:782`,`421`) — **by
  output-column identity, never positionally** (fleet gap: MERGE members permute
  columns).
- **MAP that rewrites the column**: `Prov = ∅` on the rewritten output (a functor
  can produce a value outside `d`). Pass-through MAP columns preserve `Prov`.
- **MERGE**: `Prov(out col) = ⋂_members Prov(member col)` — **intersection**, the
  load-bearing conservative step (a union row is demand-guarded only if *every*
  member guaranteed it).
- **NEGATE / AGG / KVINDEX / @product**: `Prov = ∅` on outputs (the key
  relationship is broken through an antijoin, a regrouping, or a cross-product).

Multi-adornment (D3.a.3) is handled with no special case: `Prov = {d_bf, d_fb}` is
a set; the redundancy predicate (§5) tests the **specific** `d` a guard checks, so
distinct demand keys never conflate (the rel design's `⊤_key(d) ⊔ ⊤_key(d')`
undefinedness is gone).

### §3.3 The fixpoint case (`lfp`)

An SCC (`induction_info` non-empty `cyclic_views`, `Query.h`; or
`ViewSelfReachable`, CLAUDE.md) is costed as a bounded fixpoint. Assign the
recursive relation a closure symbol `Z_S` and round symbol `R_S`. Model semi-naive
evaluation: round `r` joins frontier `δ_r` against the recursive relation via the
back-edge JOIN; the frontier recurrence is `Card(δ_{r+1}) = σ_back · Card(δ_r) ·
fan`, with `Σ_r δ_r = Z_S` (frontiers partition the closure). The **per-round
work** uses §2.6's actual-cards rule (linear vs non-linear falls out). `R_S`
enters only in per-round overhead (`+ R_S · |SCC tables|` claim clears). The model
does **not** solve the recurrence — `Z_S`/`R_S` stay symbolic unless a `.cost
closure:` hint bounds them; configs remain comparable *in `Z_S`*.

Under `-demand`, the guard join lives **inside** the cycle (`R_S` copies) and the
query-projection guard once outside — the account that reproduces the `tc` census
(§5.1).

### §3.4 L2 work — schedule order, and the NON-tautological cross-check

L2 walks the ops in a **hybrid order**: dep-topological (`pinned_order`,
`Rel.h:915`) for the effectful fold/round/sweep families, and **walk/`seq=`
order** (the emission order the markers record, `kJoinEmit.order=`/`seq=`,
`Rel.h:756`) for the eager web — because the eager markers carry **no dep edges**
(`Rel.h:164`; `mono.demand.rel` `deps:` empty), so `pinned_order` alone does not
put join.6 before join.7 (a fleet gap). The `seq=` the emitter already stamps is
the correct order for the eager family.

**`V-COST-XCHECK` (the real check).** For every DR table `T` with model-linked
Query views `member_views` (`Rel.h:416`), L2 **independently derives** `T`'s
produced-row count from the writing ops' *own inputs and lowering* (§2), and
asserts it equals L1's `Σ_{v ∈ member_views} Card(v)` (summed over ALL
identity-distinct feeders for model-shared / E-107 tables). This is **not** L1
handed to L2 and echoed back (the fleet's tautology): L2's number comes from the
op structure, L1's from the Query algebra, and a divergence means one model is
wrong — an always-on model validator gating `.cost` blessing.

---

## §4 THE SURFACE — A `.cost` SCENARIO FAMILY + `bin/Cost`

**The sidecar is NOT one cost model per program — it is a FAMILY of
cost-revealing SCENARIOS** (owner ruling, 2026-07-30; [[cost-scenario-family]]).
Each scenario pushes on a different regime — small vs large inputs, skewed vs
uniform join keys, selective vs unselective joins, few vs many demanded keys `D`,
shallow vs deep closures, monotone vs differential churn. The instrument judges an
optimization across the WHOLE family and reports the SPREAD: *"don't optimize for
one scenario — get the best general results for all scenarios."* A feature is
"good in general" only if it does not explode in ANY scenario. This is
property-based / adversarial-workload thinking applied to COST: the explosions the
instrument exists to catch (a hot key making a self-join quadratic; `D=1`
flattening `-demand` to look linear) are exactly what a single nominal workload
averages away.

A `.cost` file is a peer of `.batches` / `.drflags` / `.eqgate`
(`tests/OptDiff/cases/<name>.cost`) holding one or more named `scenario` blocks
(alternatively `<name>.<scenario>.cost` files — same family). A new instrument
`bin/Cost` (peer of `bin/Oracle`, target `drlojekyll-cost`) reads `-rel-out` +
`-df-out` + the sidecar and emits, PER SCENARIO, per config × regime, a per-op
cost table + a program-total expression + each assertion's verdict, followed by
the cross-scenario SPREAD (min/max/argmax config per scenario — the "which
scenario is worst for this optimization" line). Grammar (line-oriented, `#`
comments); shared header then N scenarios:

```
# <case>.cost — a family of cost-revealing scenarios
config  normal | -demand | -demand-instance | -demand-retract    # which flow(s) to cost
regime  build | delta                                            # which currency (default: both)

scenario shallow_sparse         # e.g. message-rooted, few demanded keys -> demand should LOSE
  message add_edge/2:  N
  demand  neighborhood_bf:  D
  fanout  edge.From:   f=1                 # each key hits ~1 edge (sparse)
  workload demanded_keys=D  batches=B  batch_size=dN
  expect  cost[-demand] > cost[normal]     regime build   # FLAG: demand does MORE here

scenario deep_dense_closure     # e.g. large closure, dense fanout -> demand should WIN
  message edge_2/2:    N
  fanout  edge_2.From: f=8                  # dense: each node ~8 out-edges
  closure path: size=Z rounds=R             # a real closure (never numeric w/o this)
  demand  reachable_from_bf: D
  workload demanded_keys=D
  expect  cost[-demand-instance] < cost[-demand]   regime build   # nested does less

scenario skew_hot_key           # one hot join key -> the quadratic-blowup probe
  message edge_2/2:    N
  fanout  edge_2.From: f=N                   # a hot key: fanout ~ whole relation
  expect  no_explosion  join.self            # K must stay sub-quadratic

# --- assertions shared across scenarios ---
expect  redundant  join.7                    # §5 structural: ΔPrune(join.7)==0 (all scenarios)
expect  xcheck                               # run §3.4 (all scenarios)
```

Symbol lines (`message`/`table`/`demand`/`fanout`/`select`/`present`/`group`/
`closure`/`retract`) bind the base symbols of §1.2 within a scenario; anything
unbound takes its §1.4 default. A `redundant`/`xcheck` assertion outside a
`scenario` block holds across the whole family (a STRUCTURAL fact like the
identity-join drop is scenario-invariant by construction — it removes no rows in
any distribution). A `cost[...]`/`no_explosion` assertion is per-scenario (it is
distribution-dependent — the whole point).

**Config-awareness (4 opt modes × demand modes).** The DR flow graph *is* the
per-config artifact, so the instrument costs whatever flow the compiler emits for
the named `config` + the OptDiff mode. A `.cost` case is mode-crossed like every
golden: the four opt modes (opt/nodf/nocf/none) are execution variants, and the
double-join drop verdict is re-confirmed in each. Demand modes are the `config`
axis, orthogonal to opt mode. **Two configs are compared only when costed with the
SAME scenario symbols** — the instrument refuses a comparison across divergent
inputs (else free-symbol defaults flatter one config, §8.3).

**Golden pinning.** Output artifact `<name>.cost.stdout` — PER SCENARIO the
symbolic `TOTAL(config, regime)` + each assertion's verdict, then the
cross-scenario spread — is blessed like any golden, only via explicit
`runall.sh --bless` after review (never auto-green). Absent a `.cost` sidecar a
case is skipped (opt-in, exactly like `.batches`). The first pins:
`demand_neighborhood_mono_witness` (the double-join acceptance, ≥2 scenarios:
`shallow_sparse` where demand loses + one where the structural drop holds
regardless), `demand_multi_adorn_witness`, and a recursive `tc` witness (the
`deep_dense_closure` scenario where the closure makes demand pay — the r0 gate's
home).

### §4.1 Worked mono `.cost`

```
# demand_neighborhood_mono_witness.cost
config  normal -demand
regime  build
message add_edge/2:  N
demand  neighborhood_bf:  D
fanout  edge.From:  f
workload  demanded_keys=D  batches=1  batch_size=N
expect  redundant  join.7
expect  cost[-demand] > cost[normal]  when D<=N  regime build
expect  xcheck
```

`TOTAL(normal, build) = 2N` (build once) `+ D·f` (answer: D index probes,
`f` matches each). `TOTAL(-demand, build) = N + D + 2·D·(1+f) + D·f` (§5.2). Both
`expect`s discharge (§5).

---

## §5 THE DOUBLE-JOIN VERDICT (the acceptance test)

The witness: `neighborhood(bound Start, free Node) : edge(Start, Node)` over
`edge(From,To) : add_edge(From,To)`, under `-demand`. `mono.demand.df` structure
(grounding §3): demand `tuple.4 (c8)` = `d` (key Start, card `D`) feeds **both**
guard joins' `.in0`; `join.6` (step-5 push-down) pivots `From ← d.c8, edge.From →
tuple.3`; `join.7` (step-8 projection) pivots `Start ← d.c8, tuple.3.Start →
tuple.5 → pub`. Rel survival: `kEagerJoin=4`, `kJoinEmit=2` — two real
`TABLEJOIN`s in C++.

### §5.1 The census, reproduced symbolically

```
mono  (1-hop, 1 adorn):   normal 0 joins  →  -demand 2   (join.6 + join.7)
multi-adorn (2 adorn):    normal 0 joins  →  -demand 4   (each adorn mints its own J5+J8 over its own store, one shared pub)
tc  (right-linear rec):   normal 1 join   →  -demand 4   (body join + base push-down + recursive demand-propagation + projection-8)
```

Step-8 is exactly **one** join per adornment (mono/multi-adorn) and one of four in
`tc`. **Caveat (verdict-lane risk):** the `tc` 1→4 breakdown is inferred from the
census + mint pattern; it must be verified against a live `tc.demand.df` dump
before any drop lands on recursive shapes (only `mono` is dumped in the artifacts).

### §5.2 term-8 ⊆ term-5 on the supported slice

By §2.2, both `kJoinEmit` cost `D·(1+f)`:

```
term-5 = K(join.6) = D·(1+f) ;  Card(tuple.3) = D·f ;  Prov(tuple.3.Start) ∋ d   # pivot From←d.c8 (§3.2)
term-8 = K(join.7) = D·(1+f) ;  join.7 is the semijoin tuple.3 ⋉_Start d
```

The `Prov` stamp is the code-true form of grounding §3's prose "every row of
tuple.3 has Start ∈ d": join.6 pivoted `From ← d.c8`, so `π_Start(tuple.3) ⊆ d`.
Then join.7's selectivity

```
σ_8 = |{r ∈ tuple.3 : r.Start ∈ d}| / |tuple.3|
```

is **exactly 1** because `Prov(tuple.3.Start) ∋ d` means every row already
satisfies `Start ∈ d` (semijoin-with-superset elimination: `R ⋉_A S = R` iff
`π_A(R) ⊆ π_A(S)`). Therefore

```
output(join.7) = tuple.3 = output(join.6)      # identity join, removes nothing
term-8 ⊆ term-5  as sets                        # in fact equal
Δ_redundant = term-8 = D·(1+f)                   # pure waste on the slice
```

This is a `Prov`-tag equality the model computes and the canonicalizer cannot
(grounding §5). It is a **structural** fact (a set intersected with a superset of
itself via the shared `d.in0`), unaffected by key skew (§7.2).

### §5.3 The boundary — where step-8 is load-bearing

term-8 prunes iff `σ_8 < 1` iff `Prov(tuple.3.Start) ∌ d` — the body-guard key set
diverges from the query's own seed. The `Prov` lattice localizes exactly three
off-slice shapes (verdict-lane §4, with the constructed programs):

1. **Adornment divergence** (`α ≠ β`) — the query binds a column the SIP guarded
   sideways (e.g. `p(bound X,free Z) :- link(Y,X), reach(Y,Z)`; demand propagates
   on `Y`, the query binds `X`). `Prov` of the read's pivot ∌ `d`; step-8 is the
   sole `α`-restriction. *(This is NOT multi-adornment: there each store is keyed
   on its own bound column, `α=β` per store, `Prov` carries its own `d`.)*
2. **Multi-clause / merge-flood** — an unguarded producer (a base-fact clause)
   floods `q_read` with un-demanded keys; the MERGE-intersection (§3.2) drops
   `Prov` to `∅`; step-8 restores `∈ d`.
3. **Non-From-preserving recursion** — a head that permutes the bound slot
   (symmetric closure `p(Y,X):-p(X,Y)`) puts an un-demanded value in the query's
   bound position; the back-edge fold drops `Prov`; step-8 is real.

**Verdict.** term-8 ⊆ term-5 **on the supported slice** (single-adornment,
From-preserving: `Prov ∋ d` propagates unbroken). The model reports `σ_8 = 1`
(redundant, safe to cut) vs `σ_8` symbolic (load-bearing, keep) per (query,
adornment) — it *characterizes the boundary*, never asserts unconditional
redundancy.

### §5.4 The normal-vs-demand crossover

BUILD-regime totals (per adornment; per round for recursion), using §5.2's
`J8 = 2S` with `S = D·f` the demanded slice and `M` the full eager
materialization:

```
W_normal = M + D·(1+f)                        # materialize all, D index probes
W_demand = N + D + (D + S) + 2S + 2S = N + 2D + 5S      # input + demand channel + both guards
W_demand^{drop-8} = N + 2D + 2S               # forward q_consumer to tuple.3
demand pays  ⟺  M > N + 2D + 5S   (both guards)   /   M > N + 2D + 2S   (step-8 dropped)
```

**1-hop / message-rooted (`M = N`, the same physical table).** The savings
`M − S` is illusory (`p` *is* the input, must persist):

```
W_demand − W_normal      = D + 5S − D(1+f) = 4Df > 0    (always — demand STRICTLY loses)
W_demand^{drop-8} − W_normal = Df > 0                   (still always)
```

Dropping step-8 shaves 75% of the pure overhead but does not flip the sign:
**shallow / message-rooted demand is justified only by the keyed-instance
MECHANISM (lifecycle / retraction / subscription), never by cost** — the seed §7
fact, made numeric. **Recursive (`M = C ≫ N`, `S = D·r`, `R` rounds):** demand
pays iff `C > N + 2D + 2·R·D·r` (step-8 dropped) — true for a-few-seeds-over-a-
big-graph, and dropping step-8 lowers the machinery coefficient `5RDr → 2RDr`,
enlarging the region where demand earns its keep.

### §5.5 Recommendation + proof obligation

**Drop step-8 as a structural peephole in `ApplyDemandTransform`, gated by a
four-condition provenance certificate — NOT a cost-guided runtime rewrite.** The
adjudication follows the verdict lane over both the query- and two-layer-designs'
prospective-mint / cost-drop legs, which the fleet critiques showed unsound:

- The redundancy is a **structural invariant** decidable at transform time from
  the graph — no cardinality estimate is needed to drop soundly. A cost-guided
  rewrite (drop-if-`σ_8`≈1) would be strictly weaker and *unsound* (dropping on an
  estimate where a proof exists), and both the query design's prospective
  mint-skip (which runs **pre-CSE**, before `raw_seed` folds into `d_reader`, so
  the redundancy does not yet exist at the mint site) and any live differential
  cost-drop (a `σ=1` guard can still be load-bearing for mid-batch retraction
  ordering) risk a miscompile.
- **The cost model's role is prioritization + guarding**, not the drop decision:
  (a) confirm the redundancy is worth caring about — it is, `2S` per adornment per
  round, surviving to `kJoinEmit`; (b) flag the §5.3 off-slice shapes so the
  peephole KEEPS step-8 there.

**Correctness proof obligation for the drop.** Forwarding `q_consumer → tuple.3`
preserves the published answer set **iff** the compiler certifies
`π_α(tuple.3) ⊆ d` — a **value-containment** fact, which the certificate must
establish by **pivot-inheritance provenance** (the value equals a `d` value),
**not** by `out_to_in` column-position aliasing alone (the two-layer critique's
sharp point: position preservation ≠ value containment; a head-rebinding recursion
aliases the column while the values escape `d`). Sufficient certificate =
verdict-lane (i)–(iv): (i) single adornment for the query name; (ii) `α = β`
(query bound-cols = SIP-propagated bound set); (iii) every `p_merge` member
reaching `q_read` is push-down-guarded on `α` (no flood path); (iv)
From-preservation on every recursive back-edge into `p`. When all four hold, the
subset is invariant and the forward is answer-identical; when any fails, keep
step-8 (the flag, not the drop), guarded by an always-on assert re-checking the
shared-`d` `.in0` structural precondition at the mint site, in the F17/F18
validator idiom.

---

## §6 SCOPE & STAGING

### §6.1 The first landable slice (minimal — a handful of witnesses, not the compiler)

1. **`bin/Cost`** (target `drlojekyll-cost`), reading `-rel-out` + `-df-out` + a
   `.cost` sidecar, emitting symbolic `TOTAL(config, regime)` + per-op tables.
2. **L1 pass**: the §2.1 cardinality rules + the §3.2 `Prov` lattice, over the
   Query graph, forward-topological, SCCs condensed to `Z_S`.
3. **L2 pass**: the §2.2–§2.8 `K` rules over the DR flow, hybrid order (§3.4),
   reading `Lowering` + `Algebra` + `eager_view` off the ops.
4. **`V-COST-XCHECK`** (§3.4) as the always-on model validator.
5. **Three golden witnesses**: `demand_neighborhood_mono_witness` (double-join
   acceptance: reproduce the census, discharge `redundant join.7` and
   `cost[-demand] > cost[normal]`), `demand_multi_adorn_witness` (the `0→4` /
   two-store / one-pub shape, `Prov = {d_bf, d_fb}`), and a recursive `tc` witness
   (BUILD-regime closure, the crossover). No differential, no `@recompute`, no
   `@product` witness in slice 1 — those op-cost functions are *specified* here
   (§2.3–§2.6) but land with their own witnesses in a later slice.

Deliberately excluded from slice 1: numeric wall-clock (that is `bench/`); a live
cost-guided rewrite (the drop is a separate structural-peephole change, §5.5);
skew histograms (§7.2); cascade-retraction fan derivation (§7.4).

### §6.2 How it gates the recursive-demand r0 work

The recursive-demand seed (`recursive-demand-seed.md` §7,
`[[demand-cost-model]]`) is the reason this instrument is built first. Its path
forward is *r0 model the induction round in the Rel-IR → r1 RESCAN→fixpoint → r2
mint a P-RECURSIVE axis → …*. Before any of that lands, the cost model must
**price whether the keyed-instance nested lowering actually does less than flat
`-demand` on a recursive shape** — the exact "do less could explode" hazard. The
gate is concrete:

```
GATE(recursive-demand r0):  for the tc witness, TOTAL(-demand-instance, build)
                            must be < TOTAL(-demand, build) under the stated workload,
                            AND kSubgraphInstantiate's per-key term D·(per-key fixpoint rescan)
                            must not exceed the flat closure work C it replaces.
```

If the nested per-key fixpoint rescan (each instance hosting its own inductive
fixpoint keyed on the demand) costs `D · Z_key` where flat demand costs `Z_S^d`
once, the model flags it when `D · Z_key > Z_S^d` — precisely the
over-materialization the p1/HP-5 notes warn of. **No recursive-demand lowering
lands without passing this gate on its witness `.cost`.**

---

## §7 WHAT THIS MODEL CANNOT YET REPRESENT

1. **Constant factors / cache behavior.** Units are invocation counts; a
   `kPointTest` and a `kSectionWalk` step are "1 each". Real wall-clock is
   cache-line- and dependent-load-dominated (`[[perf-guiding-oracles]]`). This
   model ranks *asymptotic shape*, the a-priori complement to `bench/`, never its
   replacement.
2. **Data skew.** `σ_P`, `f`, `π` are per-key *means*; heavy-hitter / degree-skew
   join blow-up (one hot Start with N/2 edges) is averaged away. The owner's
   "distribution of common keys" is captured only by mean fan-out, not a
   histogram. *(The §5.2 identity-join proof is structural and UNAFFECTED — only
   magnitudes degrade.)*
3. **Fixpoint closure size / round count without a hint.** `Z_S`/`R_S` are
   declared sidecar inputs; the model counts rounds, it does not *solve* the
   recurrence (a non-linear recursion's closure is genuinely data-dependent). §2.6
   correctly reports the super-linear *form* of a self-join, but the closure *size*
   is not derivable from graph shape.
4. **Cascade / differential fan-out.** `ρ` and `rederive_fan` are per-table
   sidecar inputs; the model does **not** map an input table's `ρ` to a derived
   table's much larger overdelete fraction. "Simulation-style all the way through"
   stops at the first differential table unless the analyst supplies each table's
   `ρ`.
5. **Cross-op CSE / shared-index amortization.** Two `kJoinEmit` sharing a rebuilt
   index (CARVE-3) are costed independently; a `kEagerCompare` `σ` is applied
   locally, not folded into a downstream join's selectivity product.
6. **Functor internals.** `kEagerGenerate` and aggregate reduction bodies are
   unit-1 / `φ`; an expensive or fan-out MAP (future WASM functors,
   `[[wasm-functor-direction]]`) needs a per-functor weight the sidecar lacks.
7. **Compaction amortization + `kSeek`/WCOJ.** Compaction is a 0/1 indicator (0
   for suite-sized programs); the D5 seekable-iterator substrate is a `log`
   placeholder, not a real model.
8. **Multi-adornment shared-pub interference.** The R-DUP MERGE-union of restored
   guards over one reference-counted pub is costed once (§7.5 below) but the
   reference-counting dynamics across adornments are not modeled beyond that.

---

## §8 KEY RISKS / SOUNDNESS GAPS (self-audit)

1. **`Prov` soundness rests on propagation COMPLETENESS.** If any card-changing op
   (a MAP rewriting the key column, an aggregate regrouping, a back-edge fold)
   fails to DROP `Prov` to ∅, the model wrongly declares step-8 redundant off-slice
   and the peephole could MIS-fire → over-answer miscompile. `Prov` must be
   ⊥-default and only *raised* by proven pivot inheritance (§3.2) — the safe
   direction, but every op needs an audited `Prov` transfer. **This is the single
   load-bearing correctness claim; it deserves a `V-PROV-*` validator peer**, and
   §5.5's certificate must prove *value-containment*, not mere column aliasing.
2. **The drop is a peephole, not a cost-decision — but the certificate's clause
   (iii) is fragile.** Missing an unguarded `p_merge` flood member silently
   violates the subset. Bias must be conservative: keep step-8 on any doubt (a
   retained redundant join costs `2S`; a wrongly-dropped one is unsound). The
   `tc` recursive breakdown (§5.1) is inferred, not dumped — verify against a live
   recursive `.df` before the drop lands on recursion.
3. **Free-symbol defaults flatter a config; the workload axis must be bound.**
   `f=1, σ=1, ρ=0, D=1` make `-demand` look linear when a real skewed / fully-
   demanded workload explodes. A *numeric* verdict requires a stated `workload`
   (§4); the comparison refuses divergent-symbol configs. Without a workload the
   output stays honestly symbolic.
4. **`V-COST-XCHECK` needs a complete normalizer.** It compares symbolic
   expressions in `C`; a weak polynomial-normal-form normalizer raises false
   failures (annoying) or misses a real card divergence between two schedules that
   only coincidentally share a card (dangerous). The normalizer's completeness is a
   genuine obligation; the many-to-one `member_views` map (E-107 shared tables)
   must sum L1 cards over ALL identity-distinct feeders exactly.
5. **Fixpoint cost trusts the round-shell semantics.** §3.3 trusts
   `DRRound.test_vecs` semi-naive firing; a mis-lowering that re-scans the full
   table each round truly costs `R_S·Z_S` while the model still reports the
   frontier form — a lowering bug there is invisible to cost (bench catches it,
   cost does not). And `-demand-instance`'s per-key fixpoint (§6.2) is priced from
   a *model* of the not-yet-landed lowering; the gate must be re-derived against the
   real emitted flow once r0 exists.

---

## §9 OPEN QUESTIONS FOR THE OWNER

- **OQ-COST-1 (regime default).** Should a bare run cost BUILD, DELTA, or both?
  The double-join is regime-invariant, but the recursive-demand gate is BUILD and
  the retraction accounting is DELTA. Proposal: default both, print side-by-side.
- **OQ-COST-2 (the drop).** Land the step-8 structural peephole (§5.5) now, as its
  own change gated by the (i)–(iv) certificate, or hold it until the cost model can
  *prioritize* it against other redundancies? The peephole is sound independently;
  the cost model only motivates it. (My lean: land the certificate + peephole
  separately; let the cost model witness the `2S` it saves.)
- **OQ-COST-3 (workload as a first-class compiler input).** The does-less verdict
  is hostage to `D` (distinct demanded keys) and batch shape, which no static
  per-IR analysis can derive. Should `workload` be a compiler-visible declaration
  (a `#workload` pragma the demand pass could itself consult) or stay a
  test-sidecar-only analytic input?
- **OQ-COST-4 (validator status).** Should `V-COST-XCHECK` and `V-PROV-*` be
  always-on (fprintf+abort, survive NDEBUG) like the DR validators, or opt-in under
  `bin/Cost` only? Always-on couples every compile to the cost model's correctness;
  opt-in keeps the instrument a side-tool.
- **OQ-COST-5 (skew).** Is a histogram-valued `σ_P` (per-key row-count
  distribution, the real "distribution of common keys") worth the surface
  complexity in a later slice, or is the mean-`σ` shape-ranking sufficient for the
  do-less mandate? The explosion cases the owner fears (a hot key making a
  self-join quadratic) are exactly the ones mean-`σ` averages away.
- **OQ-COST-6 (numeric grounding source).** When numbers are wanted, do they come
  from a `.cost let` block (analyst-supplied), or should the instrument learn to
  read a `bench/` profile to bind `σ`/`Z`/`ρ` from a real run — closing the loop
  between the a-priori and after-the-fact instruments?
```
