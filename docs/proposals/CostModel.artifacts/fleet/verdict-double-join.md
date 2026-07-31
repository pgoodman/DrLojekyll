# VERDICT — THE MAGIC-SETS DOUBLE JOIN (IR-home-agnostic)

> **Lane.** The rigorous double-join verdict any cost algebra must reproduce.
> Written 2026-07-30 at tip `eecea847`. Grounds every op-cost claim in a live
> file:line. Companion: `../grounding-double-join.md` (the empirical census),
> `[[demand-cost-model]]` / `recursive-demand-seed.md §7` (the crossover seed).
> This verdict is deliberately NOT a full instrument — it is the acceptance
> target the instrument's first equation set must hit.

---

## §0 SYMBOLIC DOMAIN (the cardinalities the algebra assigns)

The double join lives entirely in the demand rewrite of one bound `#query` on a
relation `p` at adornment `α`. The load-bearing symbols:

| symbol | meaning | source of the count |
|--------|---------|---------------------|
| `N`    | `\|add_edge\|` = `\|edge\|`, the pushed input (push-tier, unprunable) | `select.0` recv `add_edge/2`, mono.demand.df |
| `D`    | `\|distinct demanded keys\|` = `\|d_p\|`, the demand relation's row count | `tuple.4 (c8)` table:8, mono.demand.df |
| `φ`    | per-key **fanout**: mean number of `p`-source rows matching one demanded key on the bound column(s) `α` (the out-degree of a demanded key in `p`'s source) | pivot `From <- d.c8, edge.From`, join.6 |
| `σ`    | per-join **selectivity** of a pivot key generally (`0 ≤ σ ≤ 1`) | any `join.k` pivot |
| `S`    | **demanded slice** = `\|{ p-source rows reachable from the D seeds }\|`. Non-recursive: `S = D·φ`. Recursive: `S = Σ_seed (reachable-set size)` | tuple.3 table:15 (non-rec); closure-slice (rec) |
| `M`    | **full eager materialization** of `p` (all keys, demand-blind). 1-hop: `M = N`. Recursive: `M = C` (the full closure) | insert.2 table:4 (normal), mono.normal.df |
| `R`    | fixpoint **round count** to closure (recursive only) | INDUCTION round shells (tc) |
| `r`    | per-seed reachable size (recursive): `S ≈ D·r` | — |

Two invariants of the domain, both from `[[demand-cost-model]]` / seed §7:

- **Push tier is unprunable.** `N` is paid in EVERY config; demand never prunes
  it. (`edge` persists in full — `tuple.2` table:11 exists under `-demand`.)
- **Demand only prunes the DERIVED tier** (`M → S`). Its whole value is the gap
  `M − S`. When `p` *is* an input (`M = N`, 1-hop), that gap is zero.

Unit costs (the per-op quanta the two-layer model reads off the Rel access-plan
spines, `kAccess`/`kFold`; a bound-column index turns a scan into a probe,
Runtime/Table.h — house constraint):

- `scan(T) = |T|` — a section-walk (`for s < table.NumRows()`), Table.h.
- `probe(T, k) = 1 + matches(k)` — a hash-index lookup on a bound column.
- `join_probe(A ⋈ B on k) = |A| + Σ_{a∈A} probe(B_index, a.k) = |A| + |A⋉B|`
  — probe each `A` row into `B`'s key index, emit the matches. This is the
  emitted `TABLEJOIN` shape (`kJoinEmit`, once per join, mono.demand.rel op.11/op.12).
- `materialize(T) = |T|` — one hash-set fill.

---

## §1 THE GRAPH (mono.demand.df — the proven witness)

`neighborhood(bound Start, free Node) : edge(Start, Node)` over
`edge(From,To) : add_edge(From,To)`, `-demand` (optimized). Verbatim structure:

```
tuple.2  (edge, From,To)   table:11   <- select.0  recv add_edge/2         ... |·| = N
tuple.4  (c8 = Start)       table:8    <- select.1  recv demand__neighborhood_bf/1 ... |·| = D
join.6:  pivot From <- .in0.c8(=d), .in1.From(=edge) ; out To <- edge.To  -> tuple.3
tuple.3  (Start,Node)       table:15                                       ... |·| = S = D·φ
join.7:  pivot Start <- .in0.c8(=d), .in1.Start(=tuple.3) ; out Node       -> tuple.5 -> insert.8 -> pub table:4
```

Two facts read directly off the edges (`tuple.4 => join.6 .in0` **and**
`tuple.4 => join.7 .in0`): **the SAME demand node `d` = tuple.4 feeds BOTH
joins' `.in0`.** join.6 is step 5 (push-down, `MintGuardJoin(site.read,
d_reader, ...)`, Demand.cpp:1058); join.7 is step 8 (projection,
`MintGuardJoin(q_read, raw_seed, p_bound, ...)`, Demand.cpp:1105 — and CSE folds
`raw_seed` into `d_reader`, Demand.cpp:1107-1112, which is precisely why both
`.in0`s are the one node `d`).

Rel survival (mono.demand.rel): `kJoinEmit=2` (op.11 order=3 table:15 = join.6;
op.12 order=5 table:4 = join.7) — **two real `TABLEJOIN`s in C++.**
`kEagerJoin=4` = two dispatch-edge records per emitted join (per CLAUDE.md:
`kEagerJoin` is per-visit, `kJoinEmit` once-per-join). The double join is
physical and survives to codegen.

---

## §2 THE CENSUS, DERIVED SYMBOLICALLY

Let `J5 = join_probe(d ⋈ p_src) = D + S` (probe D keys, emit S matches) and
`J8 = join_probe(T5 ⋈ d) = S + |T5 ⋉ d|`.

### mono (1-hop, message-rooted, 1 adornment)

| config | joins | total work (dominant terms) |
|--------|-------|-----------------------------|
| normal  | **0** | `M + D·(1+φ)` = `N + D·(1+φ)` — materialize neighborhood once (= edge, a pure `select→tuple→insert` forward, mono.normal.df has NO join), answer D probes on the bound-column index. |
| -demand | **2** | `N` (edge, unprunable) `+ D` (demand channel) `+ J5 + J8 + 2·materialize(S)` |

- `J5 = D + S = D + Dφ` → materialize tuple.3 = `Dφ`.
- `J8 = S + |T5 ⋉ d|`. **§3 proves `|T5 ⋉ d| = S`** → `J8 = 2·Dφ`, materialize pub = `Dφ`.
- Census `0 → 2` reproduced (`join.6`, `join.7`). `kJoinEmit=2`. ✓

### multi-adorn (1-hop, 2 adornments) — `q(bound A,free B)` + `q(free A,bound B)`

Each adornment mints its OWN `(J5, J8)` pair over its OWN disjoint demand store
(N=2 stores, one shared pub — OD-15, Demand.cpp two-phase per-adornment loop).

| config | joins | work |
|--------|-------|------|
| normal  | **0** | `M + Σ_α D_α(1+φ_α)` |
| -demand | **4** | `N + Σ_{α∈{bf,fb}} ( D_α + J5_α + J8_α + 2·mat(S_α) )` |

Census `0 → 4 = 2 adornments × 2` reproduced. ✓ Note each adornment is
**independently ⊆-coherent** (bf guards on A, fb guards on B; §4 shows
multi-adornment does NOT by itself make step-8 load-bearing).

### tc (right-linear recursive, 1 adornment)

Recursion makes the demand relation itself recursive (the magic seed
propagates). The four joins:

| # | join | role |
|---|------|------|
| 1 | recursive body join, now demand-guarded (`edge ⋈ path`, the join normal already had) | the pruned closure step |
| 2 | base-rule push-down guard `d ⋈ edge` | seeds the demanded base slice |
| 3 | recursive push-down / demand-propagation guard `d ⋈ (edge⋈path)` | propagates demand sideways |
| 4 | **step-8 projection guard** `q_read ⋈ raw_seed` | the query's own read |

| config | joins | work |
|--------|-------|------|
| normal  | **1** | `W_full ≈ R·C` — semi-naive fixpoint over the FULL closure `C` |
| -demand | **4** | `N + D + R·(J5^{rec} over S) + J8` |

Census `1 → 4` reproduced. Step-8 is exactly **one** of the four (join #4);
dropping it gives `3`. ⚠ **Caveat:** the tc breakdown is inferred from the
census count + the mint pattern, NOT from a live per-node `tc.demand.df` dump
(only mono is dumped in this dir). Verify against a live tc dump before the drop
lands (§7 risk).

---

## §3 PROOF: `term-8 ⊆ term-5` ON THE SUPPORTED SLICE

**Claim.** On the supported slice (single adornment, From-preserving), join.7
(step 8) is an **identity semijoin**: it removes zero rows, so its cost term
`J8` reduces to a pure pass, and its OUTPUT set equals join.6's output set.

**Setup (from mono.demand.df, purely structural — no cardinality estimate).**
Let `K = π_Start(d) = { k : k ∈ tuple.4 }`, `|K| = D`. Define:

- `T5 = tuple.3 = { (Start, Node) : join.6 emits it }`. By join.6's pivot
  (`pivot From <- d.c8, edge.From`), a row `(Start, Node) ∈ T5` **iff**
  `Start ∈ K` **and** `(Start, Node) ∈ edge`. Hence:

  > **(★)  π_Start(T5) ⊆ K.**   [every T5 row's Start IS a pivot value drawn from `d`]

- `T8 = tuple.5 = T5 ⋉_Start d = { r ∈ T5 : r.Start ∈ K }` (join.7's pivot is
  `Start <- d.c8, tuple.3.Start`, i.e. a semijoin of `T5` against `d` on Start).

**Lemma (semijoin-with-superset elimination).** For any `R, S` and attribute
`A`: `R ⋉_A S = R` iff `π_A(R) ⊆ π_A(S)`.

**Application.** `R := T5`, `S := d`, `A := Start`. By (★),
`π_Start(T5) ⊆ K = π_Start(d)`. Therefore `T8 = T5 ⋉_Start d = T5`. **join.7
filters nothing; `|T8| = |T5| = S`.** ∎

The graph-level reason (★) holds is the shared `.in0`: `d` = tuple.4 feeds
join.6.in0 (which STAMPS Start with a `d`-value) and join.7.in0 (which then
TESTS Start ∈ `d`) — a set is being intersected with a superset of itself. The
canonicalizer cannot see this because it needs the *subset* fact (★), which is a
provenance/cardinality property, not a structural equality (grounding §5).

**Cost consequence.** `J8 = S + |T5 ⋉ d| = S + S = 2S`, and every one of the `2S`
units is redundant: the answer set is already `T5`. Dropping join.7 and
forwarding `q_consumer` directly to `T5` removes `J8 + materialize(pub-copy) = 2S + S`
per adornment (per round, recursively) with **zero answer change**.

---

## §4 WHEN STEP-8 BECOMES LOAD-BEARING (the exact boundary)

Step-8 is load-bearing **iff (★) fails** — iff some row reaching `q_read` (the
query's own read of `p`) carries a bound-column value NOT in `K`. Equivalently:
some path from a producer of `p` to `q_read` does NOT pass through a push-down
guard keyed on the query's own adornment `α`. Three constructed shapes, each
breaking a distinct clause of (★):

### (a) Adornment divergence — the query binds columns the SIP guarded differently

`K` restricts the column set `β` that the body's SIP chose to pivot on; step-8
filters on the query's `α`. If `α ≠ β`, `π_α(T5) ⊄ K` and step-8 is the only
`α`-restriction.

```
#query p(bound X, free Z)
p(X, Z) : link(Y, X), reach(Y, Z).     ; SIP binds Y sideways into reach;
                                        ; the demanded key propagated is on Y (β),
                                        ; but the QUERY binds X (α ≠ β). T5's X
                                        ; column ranges over all links of a
                                        ; demanded Y — NOT ⊆ the X the query asked.
```

Note this is NOT the multi-adornment case (§2): there each adornment's demand
store IS keyed on that adornment's own bound column, so `α = β` per store and
(★) holds independently. Divergence needs `α ≠ β` WITHIN one query's SIP walk.

### (b) Multi-clause / merge-flood — an unguarded producer reaches `q_read`

`q_read` reads `p_merge` (Demand.cpp:573, the post-Connect MERGE). If ANY merge
member produces `p`-rows WITHOUT traversing a push-down guard (a base-fact
clause, or a clause with no demanded subgoal), those rows have unrestricted
bound-column values ⊄ K, and step-8 is the sole filter.

```
#query p(bound X, free Y)
p(X, Y) : base_pairs(X, Y).            ; EDB flood: rows for ALL X, un-demanded
p(X, Y) : p(X, W), edge(W, Y).         ; recursive arm, demand-guarded
                                        ; base_pairs floods q_read's merge with
                                        ; every X; step-8 restores X ∈ K.
```

### (c) Non-preserving recursion — the head permutes the bound column

From-preservation = the head copies the bound-column value from an `α`-guarded
body atom. A rule that PERMUTES or TRANSFORMS the bound slot breaks (★): derived
rows carry an arbitrary value in the query's bound position.

```
#query p(bound X, free Y)
p(X, Y) : edge(X, Y).
p(Y, X) : p(X, Y).                     ; SYMMETRIC closure — head SWAPS. A row
                                        ; demanded at X derives a row with the
                                        ; old Y sitting in position 0. q_read's
                                        ; bound column now holds un-demanded
                                        ; values; step-8 is load-bearing.
```

**Certificate for redundancy (the negation of load-bearing).** Step-8 is
provably redundant iff ALL hold: (i) single adornment for this query name
(`N=1`); (ii) `α = β` (query's bound columns equal the SIP-propagated bound
set); (iii) every `p_merge` member producing `q_read` passes through a push-down
guard keyed on `α` (no flood path — rules out (b)); (iv) on every recursive
back-edge, the head's `α`-columns are copied verbatim from an `α`-guarded body
atom (From-preservation — rules out (c)). (i)–(iv) are a graph-reachability +
column-provenance check — exactly the subset reasoning the canonicalizer lacks.

---

## §5 THE CROSSOVER — WHEN `-demand` COSTS MORE THAN IT PRUNES (seed §7, quantified)

Dominant-term totals (forward derivation; retraction deferred, §6):

```
W_normal  =  M  +  D·(1 + φ)                         ; materialize all, D index-probes
W_demand  =  N  +  D  +  J5 + J8 + 2·mat(S)          ; input + demand channel + guards
          =  N  +  D  +  (D + S) + 2S + 2S           ; using §3: J8 = 2S
          =  N  +  D  +  D + 5S                       ; (per adornment, per round for recursion)
W_demand^{drop-8}  =  N + D + (D + S) + S             ; forward q_consumer to T5
          =  N  +  2D + 2S
```

**Demand pays iff `W_demand < W_normal`, i.e.**

```
   M           >    N  +  2D + 5S              (both guards)
   M           >    N  +  2D + 2S              (step-8 dropped — pays SOONER)
```

Read `M` as the pruned eager derivation and `N + …S` as machinery + demanded
slice. Two regimes:

**1-hop / message-rooted (`M = N`, `S = Dφ`).** The LHS savings `M − S = N − Dφ`
is **illusory**: `M` and `N` are the SAME physical table (`p` *is* the input,
must persist). Substituting `M = N`:

```
   W_demand − W_normal  =  D + 5S − D·(1+φ)  =  D + 5Dφ − D − Dφ  =  4Dφ  >  0   (always)
   drop-8:                D + 2S − D·(1+φ)   =  Dφ                >  0   (still always)
```

**`-demand` STRICTLY loses on shallow shapes**, by `4Dφ` (both guards) → `Dφ`
(dropped). This is the seed §7 fact made numeric: shallow/message-rooted demand
is justified ONLY by the keyed-instance MECHANISM (lifecycle/retraction/
subscription), never by cost. Dropping step-8 shaves 75% of the pure overhead
but does not flip the sign — the mechanism, not the optimizer, must carry mono.

**Recursive (`M = C` full closure ≫ `N`, `S = D·r`, R rounds).**

```
   demand pays  ⟺  C  >  N + 2D + 5·R·D·r          (both guards)
                ⟺  C  >  N + 2D + 2·R·D·r          (step-8 dropped)
```

For `D` few query seeds over a large sparse graph (`r ≪ C/D`), the RHS is
dominated by `N` and demand wins by the full `C − N`. Depth `L` enters via `r`
and `C`: chain of depth `L` ⇒ `C ≈ N·L`, `r ≈ L`, so demand pays iff
`N·L > N + 2RDL` ⇒ roughly `N > 2RD` (more edges than `2R·(seeds)`) — true for
any real query-a-few-seeds-over-a-big-graph workload. **Dropping step-8 lowers
the machinery coefficient `5RDr → 2RDr`, enlarging the region of
`(N, D, L, σ)` where demand pays** (it earns its keep on shallower closures).

---

## §6 WHAT THIS VERDICT (and the algebra reproducing it) CANNOT REPRESENT YET

1. **Retraction / differential work.** `OVERDELETE→REDERIVE→INSERT` and the
   split signed counters are not in the algebra — `W_*` is forward-only. Under
   `-demand-retract` the guard joins re-fire per death; the crossover shifts.
2. **General closure size `C` and round count `R`.** Program-dependent, not
   derivable from `(N, D, φ)` alone — needs a fixpoint-depth/closure symbol +
   semi-naive dedup accounting.
3. **Key-distribution skew.** `φ`, `σ` are point estimates assuming
   independence; a few hub keys make `Dφ` a poor magnitude summary. (The §3
   **identity-join proof is UNAFFECTED** — it is structural, not statistical —
   only the §5 magnitudes are.)
4. **Index-vs-scan constants.** The p1 rescan regression (keyed-instance RESCAN
   → full section-walk vs normal's index probe) is a constant-factor / access-
   plan detail below the cardinality algebra's resolution.
5. **Multi-adornment shared-pub interference** and the R-DUP MERGE-union cost of
   the reference-counted pub.
6. **Codegen constant factors** (`TABLEJOIN` machine cost vs an index probe) —
   the two-layer model's Rel-side unit costs are ordinal, not calibrated.

---

## §7 RECOMMENDATION + THE DROP'S PROOF OBLIGATION

**Recommend: DROP step-8 where the §4 certificate holds — as a demand-transform
peephole, NOT a runtime cost-guided rewrite.** Rationale:

- The redundancy is a **structural invariant** (★), decidable at transform time
  from the graph — no cardinality estimate is needed to DROP soundly. A
  cost-guided rewrite (drop-if-selectivity-≈1) would be strictly weaker and
  unsafe (it would drop on an *estimate* where we have a *proof*).
- The cost model's role is **prioritization + guarding**, not the drop decision:
  (a) confirm the redundancy is worth caring about — it is, `2S`/adornment/round,
  surviving to `kJoinEmit`; (b) FLAG the §4 off-slice shapes so the peephole
  KEEPS step-8 there.

**Where it lands.** The Query graph (`ApplyDemandTransform`, Demand.cpp) — the
double join is BORN there and survives Rel unchanged (grounding §5). Concretely:
at Demand.cpp:1092-1127, when the certificate holds, skip minting join.7 and
instead enqueue the deferred rewire (Demand.cpp:1125) to forward `q_consumer`
directly onto `T5` (the push-down guard's restored output) — the R-DUP
machinery already groups rewires by `(consumer, read)`, so this is a rewire
target swap, not new structure.

**IR-home of the COST computation: TWO-LAYER.** The ⊆ certificate + cardinality
provenance live on the **Query graph** (mono.demand.df is where (★) is read);
the per-op unit cost + the "survives to `kJoinEmit=2` in C++" magnitude are read
off the **Rel/DR-IR access-plan spines** (`kAccess`/`kFold`, mono.demand.rel).
Neither layer alone suffices — Query has the shape, Rel has the physical cost.

**Correctness proof obligation for the drop.** Replacing join.7 with a direct
forward of `q_consumer → T5` preserves the published answer set **iff** the
compiler certifies `π_α(T5) ⊆ K` (the §3 Lemma's hypothesis). Sufficient
syntactic certificate = §4 conditions (i)–(iv):
(i) `N=1` adornment for the query name;
(ii) `α = β` (query bound-cols = SIP-propagated bound set);
(iii) every `p_merge` member reaching `q_read` is push-down-guarded on `α`;
(iv) From-preservation on every recursive back-edge into `p`.
When all four hold, (★) is invariant and `T8 = T5` exactly (semijoin-with-
superset elimination), so the forward is answer-identical. When ANY fails
(§4 (a)/(b)/(c)), (★) can be violated and step-8 is retained — the flag, not the
drop. The obligation is discharged as a graph-reachability + column-provenance
pass, guarded by an always-on assert that re-checks (★)'s structural precondition
(shared-`d` `.in0`) at the mint site, in the F17/F18 validator idiom.

**One-line verdict.** The double join is real, physical (`kJoinEmit=2`, survives
to C++), and on the entire currently-supported demand slice its second half
(step-8) is a **provably identity semijoin** costing `2S` per adornment per round
for zero answer change; drop it under the §4 certificate (a sound peephole in
Demand.cpp), keep it off-slice, and let the two-layer cost model price the
`2S`/`4Dφ` it saves and flag the boundary where it stops being free.
