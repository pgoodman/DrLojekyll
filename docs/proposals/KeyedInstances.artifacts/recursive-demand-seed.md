# RECURSIVE DEMAND — SUBSTRATE SEED (single-pass; fleet-to-verify)

> **Status.** Written 2026-07-30 at tip **f3a55a8f** by ONE session, NO fleet —
> grounded in the live `-dot-out`/`-df-out`/`-rel-out`/`-ir-out`/`-cpp-out` dumps of
> `reachable_from` (recursive + non-recursive) plus the code reads cited inline.
> **SINGLE-PASS:** the next session's fleet must re-derive and verify this before any
> code. This is a SEED for the recursive-demand capability (the largest open
> keyed-instance gap), NOT a ranked plan — the D3.a epoch is CLOSED (§20(A)-(AT))
> and the owner re-ranks candidates. Recursive demand is DEFERRED by ruling
> (`OQ-INDUCTION-UNION → DEFER`, d3a-ruling-brief.md) with a binding precondition
> (below). Companion cost-model notes: §7.

===============================================================================
## §1 THE DEMAND PIPELINE AS IT STANDS (whole-program, two lowerings)

A bound `#query` over relation `p` under adornment `α` (which columns are bound)
lowers two ways — this is the load-bearing frame (see [[demand-subgraph-unification]]):

    parse → DATA-FLOW ────────────→ Rel (DR-IR) ──────────→ control-flow → C++
              │                        │                        │
        magic-sets TRANSFORM      instance LOWERING selector
        (normal ✗ -demand)        (-demand ✗ -demand-instance)

- The demand TRANSFORM (`ApplyDemandTransform`, lib/DataFlow/Demand.cpp) is a
  Query-graph rewrite: a SIP walk propagates `α` backward, mints a demand relation
  `d_p`, and guards each site with a JOIN `d_p ⋈ p`. TWO guards per query: the
  subgoal PUSH-DOWN guard (filters the demanded relation's rows) + the
  QUERY-PROJECTION raw-seed guard (filters the published answer). **Verified: the
  data-flow graph is byte-identical between `-demand` and `-demand-instance`.**
- The instance LOWERING (`-demand-instance`) is a control-flow selector: it
  recognizes the demanded subgraph (`ResolveLiveRecognition`, Rel.cpp:918/977/992 —
  input from a `role==kBody` guard, pub by `q_decl.Id()`), mints ONE
  `kSubgraphInstantiate` per live forcing (`BuildSubgraphInstanceOps`, Rel.cpp:1035),
  and emits a keyed `InstanceStore` instead of the flat guard-join web
  (`EmitSubgraphInstance`, lib/CodeGen/CPlusPlus/Database.cpp).

===============================================================================
## §2 THE KEYED-INSTANCE LOWERING AS PSEUDOCODE (as landed, NON-recursive)

The generated `flow` proc, distilled from the real C++ (the `subgraph-instance i#0`
op unrolled). `store` = `InstanceStore<Key=α, Row=nested-cols>`; per epoch:

    band-a1 BIRTH   for k in demand_net_additions:              # a key just demanded
                      iid = store.FindOrAddInstance(k)
                      if fresh(iid): TouchCurrent(iid); RESCAN(k) -> current(iid)
    band-a2 REBUILD for r in input_net_additions:               # an input row arrived
                      iid = store.FindInstance(r.key)           # Find, NOT add:
                      if iid live && fresh: Touch; RESCAN(r.key) -> current(iid)   #  no demand from input
    band-b  PUBLISH for iid in store.Touched():
                      publish (current(iid) \ frozen(iid)) -> pub  # (F,T) born rows
                    store.Seal()                                   # current -> frozen

    RESCAN(k) = for row in input: if row.key == k: current.TryAdd(row.value)   # ← SINGLE PASS

The RESCAN is ONE monotone linear pass (a **section-walk**). It is sound ONLY
because non-recursive content reaches its fixpoint in a single body application.
`kSubgraphInstantiate` carries this: `spine: kAccess(input, section-walk) -> kFold(pub)`,
`effects: {kVecDrain(demand,+), kVecDrain(input,+), kInstanceDemand, kInstanceRebuild(pub,+),
kStateEmit, kStateOld, kCounter(+)}`. `V-INST-FRESH` (always-on) asserts `current`
empty at first touch. (The whole effect multiset is fingerprinted by
`CheckInstantiateEffects`, the §20(AT) design-1 belt.)

===============================================================================
## §3 THE FENCES (why recursive `reachable_from` rejects) — Build.cpp:1440-1474

The nested pre-pass walks the guard annotations and raises TWO distinct rejects:

- **`cyclic_demand`** = `ViewSelfReachable(guard.demand_side)` (jl[0], :1463) →
  *"Recursive demand relations are not yet supported under -demand-instance"* (:1468).
  The SIP produced a demand relation that feeds ITSELF — `α`-propagation cycles
  through the recursive body. **Transitive `reachable_from` trips this.**
- **`recursive_content`** = the `kBody` guard's summarized input has an
  `InductionGroupId()` or `ViewSelfReachable` (:1452-1461) →
  *"Demanded subgraphs with recursive (induction-owned) content …"* (:1471).

Both bottom out at `NeedsInductionCycleVector` (Induction.cpp:10): TRUE when a
view has an `InductionGroupId()` and is a MERGE, or has non-inductive
predecessors, or is its own indirect inductive successor — i.e. it needs a
**cycle vector** to host its fixpoint. **BINDING PRECONDITION (OQ-INDUCTION-UNION /
§20(AB)):** model the owning-merge union region in the Rel-IR BEFORE relaxing
`NeedsInductionCycleVector`.

===============================================================================
## §4 WHY IT IS HARD — the fixpoint-per-instance

- Flat `-demand` on a recursive relation lowers to an INDUCTION region: cycle
  vectors (`$induction_pivots`/`$induction_in`), round shells, and the guard
  JOINs live INSIDE the fixpoint (grounded: the recursive `-demand` `^entry` proc
  builds `$induction_pivots`). Flat recursive demand WORKS today (`demand_tc_witness`).
- The keyed-instance model has NO fixpoint — RESCAN(k) is a single monotone pass
  (§2). So recursive demand under `-demand-instance` requires **each instance to
  host its own inductive fixpoint, keyed on the demand** — a strictly bigger
  lowering AND a new DR-IR representation (an instance-SCOPED round shell, which
  the flat/global round shells the R-final work modeled do not cover).

===============================================================================
## §5 THE PATH FORWARD AS DIFFS (on §2/§3)

    r0  PRECONDITION (BINDING, FIRST): model the instance-scoped induction round
        in the Rel-IR (the owning-merge union region) — the DR-IR must REPRESENT a
        per-instance fixpoint before any fence lifts. This is the §20(AB) obligation.

    r1  RESCAN becomes a FIXPOINT (the core diff):
    -   RESCAN(k): for row in input: if row.key==k: current.TryAdd(row.value)    # 1 pass
    +   RESCAN_FIX(k): worklist = demand_seed(k)
    +     until quiescent:
    +       for f in worklist: apply the guarded recursive body -> derived rows
    +       new = derived rows not already in current; current.TryAdd(new); worklist = new
        The spine `kAccess(input, section-walk) -> kFold` becomes an instance-scoped
        round shell (kFixpointFire / kChainFold WITHIN the instance).

    r2  the MINT (BuildSubgraphInstanceOps) grows an inductive effect/spine arm under
        a NEW predicate axis P-RECURSIVE (content is induction-owned), NEVER folded
        into P-STORE / P-DEATH / P-INPUT (the d2 anti-fold discipline; the O-1-style
        closure belts extend).

    r3  the FENCES narrow (Build.cpp:1467/1470): lift `recursive_content` FIRST for
        the acyclic-recursive-content case (recursive content, non-recursive demand),
        then `cyclic_demand`. The R-5 OB8(i) widening obligation BINDS: re-derive the
        derived-input branch with a directed witness FIRST (§20(AR)/(AO)).

    r4  DIFFERENTIAL recursive demand (retraction) is a FURTHER step: the per-instance
        fixpoint must do overdelete/rederive/insert (split-counter) scoped to the
        instance — recursive × the D3.a.1 differential-demand axis.

===============================================================================
## §6 THE PERF DIFF (orthogonal, smaller, non-recursive) — indexed rescan

    p1  RESCAN is a full section-walk of the input table (O(|input|) per touched key;
        the generated `for s < table_11.NumRows()` + `if ir.from == k`). An index on
        the input's KEY columns turns it into O(matches):
    -   for row in input: if row.key==k: current.TryAdd(row.value)
    +   for row in input.index_on_key.Find(k): current.TryAdd(row.value)
        Independent of recursion — a pure optimization to the LANDED non-recursive
        lowering. Normal mode already answers the same bound query by an index probe
        (`idx_19` on the bound column); the instance rescan is the one place that
        regressed to a scan. Cheap, self-contained, witness-able.

===============================================================================
## §7 THE DEMAND COST MODEL (when demand pays — the conversation's payoff)

- **Two tiers.** INPUTS are push-driven messages: uncontrollable arrival order, you
  cannot refuse or defer one, so they MUST persist in full. DERIVED/INSTANCE content
  is pull-driven by demand and prunable. **Demand never prunes the input tier** —
  it only prunes derivation between the persisted inputs and the answer.
- **Demand pays iff** (pruned eager derivation cost) > (demand machinery: the demand
  relation table + its ingest + the guard joins + intermediates). A relation rooted
  directly at messages with a trivial derivation NEVER satisfies this: `-demand` adds
  3 tables (the demand relation — itself a 2nd push-persisted channel — the guarded
  intermediate, and the FORCED materialization of the input that normal fuses away),
  plus a REDUNDANT projection-guard join, and buys nothing — normal already indexes
  the bound column and answers with one probe.
- **Consequence.** For shallow/message-rooted shapes, demand's ONLY justification is
  the keyed-instance MECHANISM (per-key lifecycle, retraction, subscription), never
  cost. Recursive demand is precisely where the OPTIMIZATION genuinely pays — a
  closure is asymptotically larger than a single key's demanded slice. So recursive
  demand is the slice that makes `-demand-instance` earn its keep as an optimizer,
  not just a mechanism.

===============================================================================
## §8 THE TEN LOAD-BEARING ANCHORS (next session: re-verify at code)

1. Build.cpp:1440-1474 — the two nested-pre-pass recursive rejects (cyclic_demand :1463/:1468,
   recursive_content :1452-1461/:1471).
2. Induction.cpp:10-22 — `NeedsInductionCycleVector` (the precondition gate).
3. Demand.cpp `ApplyDemandTransform` — the SIP guard minting (subgoal + projection guards).
4. Rel.cpp:918/977/992 — `ResolveLiveRecognition` (input via role==kBody / pub via Id()).
5. Rel.cpp:1035ff — `BuildSubgraphInstanceOps` (the mint; the effect set + spine).
6. Rel.cpp:4292ff + `CheckInstantiateEffects` (the effect-totality the recursive arm extends).
7. Database.cpp `EmitSubgraphInstance` — the band-a1/a2/b codegen (the RESCAN loops, §2).
8. The recursive flat `-demand` control-flow — `$induction_pivots`/`$induction_in`
   cycle vectors + round shells (the machinery a keyed instance must host).
9. `include/drlojekyll/Runtime/InstanceStore.h` — the store API (FindOrAdd/Find/
   TouchCurrent/Current/Frozen/Touched/Seal) the fixpoint rescan would extend.
10. d3a-ruling-brief.md OQ-INDUCTION-UNION + §20(AB) — the DEFER ruling + the precondition.
