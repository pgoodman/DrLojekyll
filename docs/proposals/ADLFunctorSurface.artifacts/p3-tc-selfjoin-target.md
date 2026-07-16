# P3 / R4 — the tc⋈tc self-join: today's shape vs. the target, and the DEFER argument

Epoch: ADL/functor-surface (`adl-functor-surface`, off main 88058879).
Seed: PerfRoadmap §12.2(C) + §12.3 P3, as amended by ADLFunctorSurface.md
§1 errata **E-23** (the R4 pivot: group_ids never outlive one `CSE()` call).
Charter: PerfRoadmap §12.3 P3 — "represent a self-join as ONE JOIN view with a
witness/self-pivot annotation instead of two distinct SELECTs + the group_id
veto, so the cubic tc⋈tc materialization is a JOIN the runtime can key with a
**self-index** rather than a value-keyed cross of two SELECT scans."

**HEADLINE FINDING (measured from HEAD, not from the seed's prose): the payoff
R4 was chartered to build ALREADY EXISTS in HEAD's emission.** The self-join
lowers to ONE physical table read via TWO hash-keyed indexes, driven by a
pivot vector — the self-index access path itself. There is no second SELECT
table and no value-keyed cross-scan. The "two distinct SELECTs + group_id veto"
is a **dataflow-representation** artifact whose lowering is already optimal
(output-size cost, `O(Σ_B indeg(B)·outdeg(B))`). **Recommendation: DEFER** —
the reshape carries the epoch's only silent-miscompile risk and buys a
measured win of, at best, one redundant register compare in the inner loop.

---

## 0. How this was measured (reproducible)

Witnesses generated with `build/debug/bin/drlojekyll` into the session
scratchpad (`scratchpad/tc2/`, `scratchpad/tc3/`); the dead-flow dumper
scope-limit the fleet lane hit (`fleet1-group-ids-extras.txt` §"Verdict on the
mechanical DOT check") is avoided by giving `tc` a `#query` consumer so the
flow survives optimization. Note `#query tc(bound i32 A, free i32 C)` IS the
declaration (it replaces `#export`, it is not an addition), and the
determination rests on DUAL witnesses — recursive tc (tc2) and non-recursive
two-hop edge⋈edge (tc3) — both lowering to the identical
one-table/two-index/pivot-loop shape, so it is not an artifact of recursion.

Recursive transitive closure:

```datalog
#message add_edge(i32 A, i32 B).
#query tc(bound i32 A, free i32 C).
tc(A, B) : add_edge(A, B).
tc(A, C) : tc(A, B), tc(B, C).       ; the self-join
```

Non-recursive two-hop (`scratchpad/tc3/nr.dr`, same shape over a `#local edge`)
was cross-checked and lowers identically (one table, two indexes, pivot loop).

---

## 1. TODAY'S DATAFLOW for `tc` (annotated)

`-dot-out` on the recursive witness (post-optimization). The load-bearing
fragment, decoded:

```
UNION (TABLE 4, "tc", cols A,B)          v…40000   ← the ONE materialized table
   │  member-view list (lib/ControlFlow/Data.cpp:246-248, IDENTITY-deduped;
   │    DR-side mirror V-MEMBER-ID, lib/ControlFlow/Build/DR.cpp:734-742) holds:
   ├── TUPLE[A,B]  (EQ SET 8)            v…41648   ← proxy side 1 of the self-join
   ├── TUPLE[B,C]  (EQ SET 8)            v…42480   ← proxy side 2 of the self-join
   ├── TUPLE (STRATUM 3)                 v…39152   ← MATERIALIZE feed
   └── TUPLE (STRATUM 1, add_edge seed)  v…37680

JOIN (EQ SET 7, pivot = B, out = A,C)    v…36128
   p0(B),p2(A) ← v…41648  (TUPLE[A,B], reads UNION as A,B)
   p1(B),p3(C) ← v…42480  (TUPLE[B,C], reads UNION as A,B relabeled B,C)
```

**The group_id mechanism, on this exact graph** (RelabelGroupIDs,
Optimize.cpp:408-482; census in `fleet1-group-ids-{pseudocode,extras}.txt`):

- The JOIN (EQ SET 7) seeds a unique nonzero scalar `group_id` (Optimize.cpp:424)
  and pushes it into its own `group_ids` set (:425).
- The deepest-first fixpoint (:449-481) propagates that label UP into every
  view feeding the JOIN — so **both** TUPLE[A,B] (v…41648) and TUPLE[B,C]
  (v…42480) end the pass carrying the JOIN's group_id in their sets.
- `InsertSetsOverlap(v…41648, v…42480)` is therefore TRUE (View.cpp:1478-1494),
  so every relational `Equals` vetoes their merge (the 8 veto sites,
  `fleet1-group-ids-pseudocode.txt` §2). The two structurally-identical proxies
  stay DISTINCT.
- **E-23 amendment**: this label exists ONLY across one `CSE()` call. It is
  cleared at `Build.cpp:2551` before optimization, re-seeded at Optimize.cpp:305,
  recomputed from scratch after every merge (:372), and cleared at the CSE tail
  (:378, ClearGroupIDs). By the time the dumped graph or any lowering runs,
  `group_ids` is **empty on every view**. The distinctness that survives is NOT
  a stored label — it is the two views being **two distinct objects both
  enrolled in the table's member-view list** (lib/ControlFlow/Data.cpp:246-248,
  identity dedup — NOTE the path: there is no lib/DataFlow/Data.cpp),
  each rooting its own branch chain (DiscoverBranches, Stratum.cpp). group_ids
  is merely the CSE-scoped guard that PREVENTS the two objects from being
  collapsed into one during that one CSE pass.

Why merging them would MISCOMPILE (the chain, confirmed at the code level):
if CSE folded v…41648 into v…42480, the JOIN's `joined_views` (a `UseList<VIEW>`)
would collapse to `Size()==1u`, tripping `ConvertTrivialJoinToTuple`
(Join.cpp:449-453 → body :400-443) — a pass-through TUPLE, destroying the
self-join. This is exactly the hazard §12.2(C) and the CLAUDE.md member-view
identity invariant protect against.

## 1a. TODAY'S EMISSION for the self-join (the decisive evidence)

`-ir-out` on the recursive witness — the JOIN region (`scratchpad/tc2/tc.ir`):

```
join-tables
  vector-loop {@B:27} over $pivots:25<i32>                          ; pivot vector = distinct join keys
  select {%col:5 as @A:32, %col:6 as @B:30}
      from %table:4 using %index:28[_,i32] where %col:6 = @B:27      ; probe idx on col6==B
  select {%col:5 as @B:31, %col:6 as @C:33}
      from %table:4 using %index:29[i32,_] where %col:5 = @B:27      ; probe idx on col5==B
    if-compare {@B:27, @B:27} = {@B:30, @B:31}                       ; (redundant, see §3)
      if-true
        update-count +recursive {@A:32, @C:33} in %table:4
```

`-cpp-out` (`scratchpad/tc2/cpp/datalog.h`, `flow_40`):

```cpp
for (auto [v27] : vec25) {                                          // pivot loop, |distinct B|
  for (uint32_t j26_0 = idx_28.First({v27}); j26_0 != kNoRow; j26_0 = idx_28.Next(j26_0)) {
    const auto r26_0 = tc_4.RowAt(j26_0);                           // rows with col6==B  (in-edges of B)
    const auto v32 = r26_0.a; const auto v30 = r26_0.b;
    for (uint32_t j26_1 = idx_29.First({v27}); j26_1 != kNoRow; j26_1 = idx_29.Next(j26_1)) {
      const auto r26_1 = tc_4.RowAt(j26_1);                         // rows with col5==B  (out-edges of B)
      const auto v31 = r26_1.a; const auto v33 = r26_1.b;
      if (v27 == v30 && v27 == v31) {                               // redundant: index guarantees it
        if (const auto ins1 = tc_4.TryAdd({v32, v33}); ins1.added) { ... }
      }
    }
  }
}
```

**`::hyde::rt::Index::First` is a HASH probe** (Table.h:762-777: `key.Hash()`,
open-addressed slot lookup), **`Next` walks the per-key chain** (Table.h:779).
So each inner scan visits ONLY the rows matching the pivot — not the whole
table. Both operands of the self-join are **`%table:4` / `tc_4`, the SAME
physical table**, probed by two indexes:

- `idx_28` = `Key28` on col6 (the in-side key),
- `idx_29` = `Key29` on col5 (the out-side key).

**Cost:** `O(Σ_B∈pivots indeg(B)·outdeg(B))` = the size of the join output.
That is the worst-case-optimal cost for a binary equi-join; there is no
super-linear-in-output blowup. There is exactly ONE materialized copy of the
relation. The non-recursive `edge⋈edge` witness (`scratchpad/tc3/nr.ir`,
line 52-59) is byte-for-byte the same shape over `%table:8` (edge) with
`%index:21`/`%index:22`.

**Therefore the seed's premise — "the distinctness IS the cubic
materialization … two SELECT scans … value-keyed cross" (§12.2(C):1478-1481) —
does not hold at HEAD.** The distinct SELECTs do NOT materialize as two tables,
and the scans are NOT value-keyed cross-scans; they are hash-keyed index
probes over one table. The self-index access path R4 set out to introduce is
what the emitter already produces.

---

## 2. THE POST-P3 TARGET SHAPE, side by side, under each candidate

### Candidate A — ONE JOIN view with a self-pivot / witness annotation

Replace the two proxy views (v…41648, v…42480) with a single JOIN view carrying
a "both operands are relation R, distinguished by pivot role (in/out)"
annotation; drop the group_id veto for self-joins.

```
TODAY:                              CANDIDATE A:
UNION(tc)                           UNION(tc)
  ├ TUPLE[A,B] ─┐                     │
  ├ TUPLE[B,C] ─┼→ JOIN(P=B)          └→ JOIN(P=B, self=R, roles={in:col6, out:col5})
  ...           ┘                     ...
```

DATAFLOW: strictly changed (fewer views; a new annotation on JOIN; a new
self-join detection in FindJoinCandidates / Build.cpp:2079).
EMISSION: **unchanged** — LowerDRFlow / EmitJoinFire would still emit the same
pivot-loop + two-index probe, since a self-join over one table with two role
keys is exactly what the two-proxy shape lowers to today.
NET: pure dataflow churn for **zero** emission change. Every one of the
invariant obligations below (P1..P7) must be discharged. High risk (§4), no
measured win.

### Candidate B — a distinguishing TAG that survives CSE without group_ids

Give each proxy an explicit `join_role`/`self_operand_id` field that `Equals`
consults directly, retiring the group_ids propagation for the self-join case.

DATAFLOW: the veto moves from `InsertSetsOverlap` (a propagated-set test) to a
local tag comparison in all 8 `Equals` sites. Per E-23, the tag must EITHER be
CSE-scoped-and-recomputed (re-seeded at Optimize.cpp:305/372, cleared at :378 —
same lifecycle as group_ids, so no simplification) OR persistent-across-passes
(then it must be propagated at all 13 `CopyDifferentialAndGroupIdsTo` sites —
obligation P2, the one most likely to silently miscompile).
EMISSION: **unchanged**.
NET: replaces one CSE-scoped guard with another; no emission change; adds the
persistence-vs-recompute fork as a fresh miscompile surface. No win.

### Candidate C — keep the dataflow shape, fix the MATERIALIZATION downstream

Leave dataflow, group_ids, CSE, member-view identity ALL untouched; if there
were a cubic cost, kill it in DR-IR lowering / runtime via a self-index access
path. **Per E-23 this option touches NO dataflow invariant and its argument is
trivial.** But the measurement (§1a) shows the downstream is ALREADY a
self-index access path: one table, two hash-index probes, a pivot loop. **There
is nothing left to fix downstream.** The only residual is the redundant
`if (v27 == v30 && v27 == v31)` compare (§3), a trivial emission cleanup that
does not need R4's framing at all.

### CHOICE

**Candidate C, in its degenerate form: the downstream is already correct, so
the correct P3 action is to take NOTHING from R4 into this epoch and DEFER.**
The one genuine (tiny) residual — the redundant inner compare — is filed as
P4-adjacent emission cleanup (§3), independent of the group_ids reshape.

---

## 3. The only real (tiny) residual: the redundant pivot re-compare

In `flow_40` the guard `if (v27 == v30 && v27 == v31)` re-tests that the two
index-probed rows actually matched the pivot. But `idx_28.First({v27})` returns
only rows with col6==v27 (so `v30==v27` by construction) and `idx_29.First({v27})`
only rows with col5==v27 (so `v31==v27`). The compare is **provably always
true** for index-driven pivots and could be elided by EmitJoinFire when the arm
is a keyed index probe on the pivot column (as opposed to a residual filter).
THE DISCRIMINATOR IS ALREADY CARRIED DATA: the elision is safe iff the arm's
ACCESS PlanNode has `lowering == Lowering::kSectionWalk` with a valid
`pivot_col` (DR.h:324-349 — kSectionWalk is documented "idx.First/Next +
pivot re-test"); a `kFullScan`/residual-filter arm MUST keep the compare.
This is a branch-in-the-inner-loop micro-cost, NOT a complexity change, and it
lives entirely in EMISSION (EmitJoinFire / the DR arm-plan spine) — it does not
require, and should not be bundled with, the group_ids reshape. Filed here as a
candidate P4 emission cleanup; measure before touching (the compiler likely
already hoists a same-register compare, so the win may be nil).

---

## 4. Risk ledger

- Candidates A and B rewrite the CSE guard that stands between a self-join and
  `ConvertTrivialJoinToTuple` — the epoch's **only** documented silent-
  miscompile path (§12.3 P3, CLAUDE.md member-view identity invariant). E-23
  adds the persistence-vs-recompute fork (obligation P2) which the seed's
  (a)-(e) omitted entirely.
- The payoff, measured, is at most one redundant register compare per output
  row — obtainable (if at all) by an emission-only cleanup that needs none of
  the reshape.
- Risk ≫ reward. This is precisely the "gated; deferral is respectable" case
  the charter names ("it has re-seeded two epochs already").

---

## 5. Recommendation

**DEFER R4.** Carry this artifact as the next epoch's head start: R4's stated
payoff (self-index access path) is already realized in HEAD's emission — and
MORE: it is already FIRST-CLASS IN THE DR-IR the seed named as its natural
home. `Lowering::kSectionWalk` ("idx.First/Next + pivot re-test"),
`PlanNode::bound_cols` (the B-1 GetOrCreateIndex identity set), and
`PlanNode::pivot_col` (DR.h:324-349) carry the exact self-pivot access path
R4 was chartered to introduce; R4-as-access-path needs ZERO new IR. So any
future R4 must FIRST re-motivate itself against the SHARPER bar: a workload
where the EXISTING kSectionWalk/kSeek access-path vocabulary is provably
non-optimal — not merely "the shape underperforms". If such a workload appears
(e.g. worst-case-optimal multiway joins / WCOJ, memory: seekable-iterators-wcoj
— a THREE-plus-way self-join where binary pivoting is provably non-optimal,
the intermediate binary-join result exceeding the final output), that is a
*materialization/access-path* epoch (Candidate C territory, trivial dataflow
invariant argument), NOT a group_ids reshape (Candidates A/B, the miscompile
risk). The group_ids reshape should be retired from the roadmap as solving a
non-problem, and group_ids/InsertSetsOverlap re-labeled a CORRECTNESS GUARD —
never an optimization target.
