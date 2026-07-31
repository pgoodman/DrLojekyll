# Prov + the identity-join recognizer — GROUNDED IMPLEMENTATION DESIGN

> Tip eecea847. Grounds the owner's "transform pass to recognize degenerate
> double joins that are identities" (2026-07-30) + the fleet's §5.5 verdict, as
> the SHARED-Prov program (owner ruling: "Prov shared → both"). Every API below
> was read at code (Query.h / Optimize.cpp line numbers inline). This is the
> stage-(a)/(b) for the mini-slice; implement against the mono witness oracle.

## §1 THE SUBSTRATE — `Prov` (keyset-provenance)

**Definition.** `Prov(col)` = the set of `(relation, col_index)` projections `π`
such that `col`'s value-set is PROVABLY ⊆ `π`'s value-set. A conservative
under-approximation: `⊥` (empty) unless proven. This is the VALUE-containment
lattice (fleet's sharp point: value, not `out_to_in` column position).

**Lattice.** Set of projections; `⊥` = {} (know nothing); join = ∩ (a value
reachable two ways is only in projections BOTH guarantee); order: more
projections = more constrained = stronger. Bounded (finite # of relation cols).

**Propagation rules** (single forward pass in `depth`/`Depth()` order over
`ForEachView`; Query.h node kinds):

| node | rule |
|---|---|
| `SELECT` from relation `R` (Query.h:668) | each output col `i` seeds `Prov = { (R, i) }` (its own source projection — incl. a fabricated demand receive `demand__q_α`, the subset engine's root) |
| `TUPLE` (703) / pass-through | `Prov(out) = Prov(in)` per `out_to_in` (position IS value here — a copy) |
| `JOIN` PIVOT out (755) | value equals EVERY unified input col (they're join-equated), so `Prov(pivot_out) = ⋃ over joined inputs Prov(in_col)` — the **pivot-inheritance** rule; this is where a col picks up `∋ d` |
| `JOIN` non-pivot out | `Prov(out) = Prov(the single in_col)` (carried, not equated — a copy) |
| `MERGE` (862) | `Prov(out_i) = ⋂ over arms Prov(arm.col_i)` (union of arms → only what ALL arms guarantee) |
| `CMP` (908) filter | pass-through `Prov(in)`; an `=`-compare against a col MAY add `Prov(other)` (optional, later) |
| `MAP` (796) | free (functor-produced) out cols → `⊥` (functor rewrites the value); bound/passed-through cols → carried. **A MAP that rewrites the key kills Prov — the miscompile guard.** |
| `NEGATE` (950) / `AGG` (826) / `KVINDEX` (731) / `@product` JOIN (num_pivots==0) | ALL out cols → `⊥` (negation/regroup/independent-product break containment) |

**`⊥`-default is the safety invariant:** any node not proven to preserve
containment drops to `⊥`. Miss-drop = over-answer miscompile (KEY RISK #1). A MAP
rewriting a column MUST reach the `⊥` arm.

## §2 THE VALIDATORS (`V-PROV-*`, always-on — owner OQ-COST-4)

fprintf+abort, survive NDEBUG (DR-validator idiom). Structural / total-by-
construction over the WHOLE corpus (they check the lattice, not workload numbers,
so they never false-abort on unmodeled ops):

- **V-PROV-MONO**: `Prov` is monotone-consistent — a `(R,i)` in `Prov(out)` must
  be traceable to a proven rule above (no fabricated projection).
- **V-PROV-BOT**: every `NEGATE`/`AGG`/`KVINDEX`/`@product`/free-MAP output col has
  `Prov = ⊥` (the anti-fold: catches a propagation rule that forgot to drop).
- **V-PROV-PIVOT**: a JOIN pivot's `Prov` ⊆ `⋃` of its joined input cols' `Prov`
  — the ANTI-FABRICATION direction (no key without a witnessing input). NOT the
  completeness direction (`⊇`): the single forward pass legitimately UNDER-
  approximates a pivot on an inductive back-edge (the back-edge input is bot when
  the pivot is computed), which is conservative-sound, so `⊇` would false-abort
  on every cyclic join (found + fixed at step-1 validation: the DR-FAIL(134) on
  cond_in_induction / elim-cond-cycle-simple). The miscompile risk is
  over-approximation (fabricated containment the recognizer would trust), and
  `⊆ ⋃in` is the cycle-safe teeth for exactly that.

These are the always-on half of the owner's OQ-COST-4 (the numeric verdicts stay
in `bin/Cost`). Reconciliation of always-on vs minimal-slice: the validators
assert the LATTICE's structural soundness (total by construction), not cost
magnitudes — so no corpus program false-aborts.

## §3 THE RECOGNIZER — identity-join elimination

**Recognition** (a JOIN `J` with `joined_views = {R, S}`, `num_pivots ≥ 1`):
`S` is a pure GUARD side — it contributes ONLY pivot columns to `out_to_in` (no
non-pivot output of `J` comes from `S`), and for EVERY pivot `p`,
`Prov(R's pivot input col) ∋ (S_rel, S's pivot col_index)`. Then
`J` is the semijoin `R ⋉ S` with `π(R) ⊆ π(S)` on every pivot →
**output(J) = R's contribution** (identity on R). (Generalizes to N joined views:
one KEEP side R, the rest all pure-guard-and-subsumed.)

**Elimination.** Interpose a TUPLE forwarding `R`'s columns into `J`'s output
schema (column-renamed via `out_to_in`), then `J->ReplaceAllUsesWith(tuple)`
(Query.h:290) + `J->PrepareToDelete()` (279). Then `RemoveUnusedViews()` +
`TrackDifferentialUpdates(log,true)` (Optimize.cpp:797) — removing `J` can change
deletion-carrying topology, so re-derive.

**THE DIFFERENTIAL FENCE (monotone-first).** Skip (KEEP `J`) if
`J->can_receive_deletions || J->can_produce_deletions` (Query.h:520-521) OR any
joined view is inductive (`IsInductive()`, Query.h:272) / on a cycle. Rationale
(fleet): an identity join in ROWSET terms can still be load-bearing for split-
counter / seed-before-drain ORDERING under retraction. Monotone (both flags
false, acyclic) = unconditionally sound. Differential removal = a later slice with
a counter-topology proof obligation. Bias CONSERVATIVE: keep on any doubt (a
retained redundant join costs 2S; a wrongly-dropped one is unsound).

**Wiring.** New pass `EliminateIdentityJoins()` gated `policy.Gate("df.ident_join")`
(the -opt-disable convention, Optimize.cpp:788), placed in `QueryImpl::Optimize`
AFTER canonicalization settles (so demand's join.7 is in its post-CSE chained-on-
join.6 form) — right before the final `do_cse(); RemoveUnusedViews()`
(Optimize.cpp:857). Loop-to-fixpoint like `do_cse`.

## §4 THE ORACLE (end-to-end acceptance, before anything lands)

`demand_neighborhood_mono_witness` under `-demand`:
- **BEFORE**: `join.6` (push-down) + `join.7` (projection, `guard_annotation` =
  kQueryProjection). df census joins=2, Rel `kJoinEmit=2`.
- **AFTER**: `join.7` gone; joins=1, `kJoinEmit=1`. `Prov(tuple.3.Start) ∋ d`
  drives the drop.
- **INVARIANT**: the 180-case golden SUITE stays green (byte-identical for the
  166 non-demand cases — the pass is a no-op where no identity join exists), and
  the demand witness stdout is UNCHANGED (answer-identical — the whole point).
  The df.opt golden for the demand witness CHANGES (one fewer join) → re-bless via
  the ritual. Monotone-regime only, so the differential demand witnesses
  (demand_diff_*) are FENCED and byte-identical.

## §4b THE PREDICTION (predict-then-verify, [[predict-then-verify-ir]]) — committed BEFORE building

Mono witness (`demand_neighborhood_mono_witness`, `-demand`), after the
`df.ident_join` recognizer. Verify EACH against the real dump:

- **P1 [.df]** `grep -c '^join '`: **2 → 1** (join.7 eliminated; join.6 push-down survives).
- **P2 [.rel census]** `kEagerJoin`: **4 → 2**; `kJoinEmit`: **2 → 1** (one TABLEJOIN gone).
- **P3 [stdout]** driver output **byte-identical** to the golden (answer-preserving — the point).
- **P4 [suite]** `SUITE PASS(180)` with **NO bless** (mono has no `.irgold`; stdout/oracle/eqgate unchanged).
- **P5 [eqgate] (RISK)** flat==nested==golden **still holds** — join.7 was identity in BOTH
  arms; nested recognition keys on the kBody push-down guard (join.6), not the
  kQueryProjection guard, so `-demand-instance` recognition is unaffected. VERIFY the eqgate.
- **P6 [OWN-3 census] (RISK)** no abort — the annotation rides J→survivor via
  `CopyDifferentialAndGroupIdsTo` (View.cpp:684), so `n_stamped + folded == total` holds. VERIFY.
- **P7 [fence]** normal mode + all 166 non-demand + the differential demand witnesses
  (`demand_diff_*`, `demand_neighborhood_witness` under `-demand-retract`): **UNCHANGED**
  (no identity join / Prov-⊥ / `can_receive_deletions` fence).
- **P8 [tc, multi-adorn]** recognizer does **NOT** fire (step-1 confirmed): joins unchanged.

A mismatch on any Pn means my mental model OR the implementation is wrong — resolve which.

## §5 STAGING

1. Prov analysis + V-PROV-* (build, validate on mono — `Prov(tuple.3.Start) ∋ d`
   observable via a debug dump). NO graph mutation yet — SAFE checkpoint for owner review.
2. The recognizer consuming Prov, monotone-fenced (mutates the graph). Gate on the
   suite + answer-identity.
3. `bin/Cost` L1 consumes the SAME Prov for cardinality (separate sub-task).
