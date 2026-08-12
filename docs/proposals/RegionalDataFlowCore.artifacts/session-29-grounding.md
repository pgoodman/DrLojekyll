# Session-29 grounding — REOPEN demand: the evidence, the fork, the honest scope

> **STATUS: DOCS-ONLY GROUNDING — no execution, no go/no-go issued.** Branch `keyed-instances`,
> tip `05af6595`. Produced by the session-29 grounding loop: first-hand code/git reconnaissance +
> an **in-tree empirical O1 spike** (compiler UNMODIFIED) + three parallel extraction agents (Path-R
> distillation, AS-IS re-verification, bench characterization) + a 3-refuter opus panel (spike-validity,
> fork, scope+result-equality), every load-bearing claim re-verified by me against code. This records the
> evidence and puts the fork + slice-scope decisions to the owner. NOTHING built or edited in the compiler.

## §0 The one-line result

**The namesake win is REAL and measured — but the first real cut is a 2–3 session arc, not one commit, and
the confirmed non-recursive slice is the one shape the recovered transform does NOT code.** So this is a
"say so and put the slice-scope decision to the owner" moment (the charter's own escape hatch), with a
concrete, evidence-backed recommendation below.

## §1 O1 — does demand pay on the non-recursive slice? (the load-bearing question)

Settled by an **in-tree spike that modifies NO compiler code**: I hand-wrote the plain program and a
faithful hand-magic-sets *push-down* (`d_r:-dem; sd:-d_r,s; r:-sd,t`), compiled BOTH with the stock
compiler, and measured global bench counters over a 4000-distinct-A selective dataset.

| regime | metric | PLAIN | DEMAND | factor |
|---|---|---|---|---|
| SELECTIVE, KC=8  | finds | 39 200 | 11 211 | 3.5× |
| | probe_steps | 307 079 | 51 658 | 6.0× |
| | **idx_hops** (join work) | **40 000** | **11** | **~3600×** |
| SELECTIVE, KC=32 | finds | 144 800 | 20 835 | 7.0× |
| | idx_hops | 136 000 | 35 | ~3900× |
| NON-SELECTIVE (demand all A) | finds | 39 200 | 55 200 | **0.71× (REGRESSION)** |

- **Answer byte-identical** in every run (8 rows KC=8, 32 rows KC=32).
- The win **scales with the derived/input ratio** (demand's residual ≈ input ingestion + one key's slice).
- The **honest, un-conflated join-work signal is `idx_hops` (40000 → 11/35)** — the `t`-join barely fires
  under demand. `finds`/`probe_steps` are inflated by shared s+t ingestion the two programs both pay, so
  the finds ratio *under*states the r-materialization win (conservative).
- **Non-selective REGRESSES** exactly as the cost model (§7 recursive-demand-seed.md) predicts — machinery
  is pure overhead when nothing is pruned. Demand MUST be cost/mode-gated.

**O1 VERDICT (re-scoped per the spike-validity refuter):**
- **The magic-sets ALGORITHM prunes — YES, large, answer-equal, scaling.** The namesake win exists.
- **Compiler-emission FIDELITY — OPEN.** The spike hand-stages a guard placement the *real* transform does
  NOT emit for this shape (see §3). It is an optimistic-but-directional model: it omits the query-projection
  raw-seed guard (negligible selective / material non-selective), and its pruning factor is dataset-tuned
  (the `s(A,A%400)` 10:1 B-sharing). So O1 is **"algorithm proven; in-compiler emission is the real risk,
  to be gated on an IN-COMPILER spike whose `.ir`/`.h` + counters are measured — not a hand-written twin."**
- **Result-equality is CONTINGENT on query-set == demand-set** (an un-demanded key under-answers). Demand is
  a **driver-contract change** (demand-before-query), not a transparent optimization. Not a miscompile; by
  design; but state it precisely.

## §2 AS-IS (re-verified at tip)

- Bound `#query` served post-hoc: `EmitQueryFriends` (Database.cpp:1527-1737, `via_index` :1646) seeks the
  **already fully-materialized** table. The seek is P7; the materialization is unpruned — that is the cost.
- The eager descent is **query-blind**: `ExtendEagerProcedure` (Procedure.cpp:14-124) → `BuildEagerInsertion
  RegionsImpl` (Build.cpp:838-993) → `BuildEagerRegion` (Build.cpp:1138-1253). No `context.frozen`/`PlanFor`/
  `AccessPlan` anywhere in the descent — every message materializes every reachable relation.
- **Demand-guard injection site** = `ContinueJoinWorkItem::Run` (Join.cpp:555-710), the CHECKMEMBER gate
  block :623-666 (today fires only under `CanReceiveDeletions() || has_unit_pred`); fold-into-`r` + descend
  at :681-708. A "is A demanded" gate slots here for a native (Path-N) emission.
- Codegen-move magnitude (spike `.rel`/`.ir`/`.h`, hand-demand vs plain): census `kIngestFold` 2→3,
  `kEagerJoin` 2→4, `kJoinEmit` 1→2; tables 3→5; `datalog.h` 245→322. **Gate (2) (goldens MOVE) is trivial.**

## §3 The recovered transform does NOT code the confirmed slice (verified in code)

`ApplyDemandTransform` (git `48cd0a4f` `lib/DataFlow/Demand.cpp`, 1464 lines) SIP walk (verified by me at
Demand.cpp:772-806) has exactly two guard-site arms:
- **`kBaseAtom`** — the bound column reaches a message-receive TUPLE with **no intervening JOIN** (a
  *single-atom* body, e.g. `path:-edge`). This is the coded non-recursive case; the cost model says it
  **barely pays** (trivial copy derivation).
- **`kPushDown`** — the bound column comes out of a JOIN one of whose inputs `IsFullWidthReaderOf(p)` — i.e.
  a **recursive read of p itself** (the TC shape `path:-edge,path`). This is where the cost model says demand
  pays **asymptotically**.

For the charter's confirmed slice `r(A,C):-s(A,B),t(B,C)`, column A comes out of the `s⋈t` JOIN whose inputs
read `s` and `t` — **neither is `r`** — so `IsFullWidthReaderOf(p_merge)` is false and the walk **cleanly
rejects at :791-795.** The corpus corroborates: the only surviving demanded-shape witness (`key_partial_1`)
is single-atom; no 2-way-join demanded body was ever a witness. **=> The non-recursive join slice needs a
NEW SIP arm** (propagate bound A across the non-recursive join, guard the base atom `s` that carries it,
rewire the JOIN consumer) — plus verification that the emitted TABLEJOIN then **drives from the restricted
side** (the spike got `idx_hops=11`, but that was the hand-`sd` graph, not the rewired-JOIN graph).

**First-slice reconsideration (surface to owner):** the *coded* `kPushDown` case is the **recursive TC
slice**, which (a) pays the *most* (§7), (b) needs *less* new SIP code (it is the coded path), and (c) has
a **ready witness** (`demand_tc_witness`, with oracle/monotone/behavioral goldens). Flat `-demand` over
recursion **worked pre-P1** — the recursion fence (`OQ-INDUCTION-UNION`) was for `-demand-**instance**` (the
keyed store has no fixpoint), NOT flat `-demand` (which hosts the fixpoint as an INDUCTION region). So the
recursive slice may be a *better* first cut than the non-recursive one the charter confirmed.

## §4 The fork — Path R vs Path N (refuter verdict: R directionally cheaper, honestly priced)

**Path R flat-only** (resurrect `Demand.cpp` + `Parse/Demand.cpp`; **skip the InstanceStore entirely** —
that is S2+): directionally the cheaper first mechanical win. The spike proves flat guard-joins lower +
prune through the existing pipeline, and the forcer scaffold **survived P1** (`BuildQueryInjectorProcedure`
Build.cpp:399/431, `ForcingMessage()` Parse.h:375). Of Rel.cpp's 1034 deleted demand lines, the bulk was
**InstanceStore** machinery (`InstantiateEffects`/`EmitSubgraphInstance`/`V-INST-*`), NOT flat demand.

BUT the "~zero consumer changes" spike conclusion is an **overstatement** — the spike bypassed three real
(modest) consumer needs: the **forcer** (auto-seed demand at query time; the spike faked it with a manual
`dem` send), **ABI suppression** of the fabricated message (Database.cpp:1522/3692 gate on `IsDemandMessage`),
and **region-port filtering** (Planning.cpp:161).

**Path N (native)** buys **architectural coherence**: Path R re-adds the **mode-gated DataFlow graph-mutation
authority the P1 cut deliberately deleted**, standing *beside* the new typed request-port authority (P3–P7c)
— the dual-authority condition the greenfield epoch forbids. "Path N buys nothing extra" is **false**; it
buys the thing P1 was *for*. Path N's cost: a native demand-seed channel + demand table + guard emission at
the Join.cpp site, from scratch.

## §5 Scope — NOT one commit; a 2–3 session arc (refuter-3, high confidence)

Demand was cut at P1 **before** the entire typed-regional stack (P2–P9) was built, so a resurrected
transform has **never coexisted** with ~6 sessions of **always-on abort validators** its minted artifacts
must now satisfy — each an abort that only fires when the corpus runs:
1. **Stage-A `RowContract`/`ProjectionRole`** (Query::Build tail): demand-minted views flow through
   `InferConservativeRowContracts` + V-CONTRACT-CENSUS / V-PROJ-ROLE-STABLE.
2. **K5 `origin_decls`**: demand mints must seed/union the provenance sets or under-name Tier-2 row-contracts
   and trip the conservation assert.
3. **The Rel eager-web** (`MakeStageOneIngestFolds`, V-PRED-XCHECK, V-INGEST-XCHECK Site-5): **aborts on
   compile** if the demand-minted guard JOINs / fabricated ingest fold aren't modeled.
4. **Regional census — the concrete landmine**: `Planning.cpp:675` `census.input_ports = received.size()`
   counts the fabricated `demand__` message as an input port, but ADJ-2 needs it region-internal →
   V-REGION-CENSUS aborts **corpus-wide**. `IsDemandMessage` is **fully deleted** — it must be re-added to
   `Query` *and* freshly threaded into the **rebuilt** Regional layer (net-new work the old transform never had).

Plus the fabricated `ParsedLocalImpl` demand relation now flows through the P6.1/P6.2 **clause walks**
(origin projection + `BuildRuleRoutingProjections`) that post-date it. Realistic decomposition:
- **S1a**: resurrect + re-integrate with the four validator surfaces to green on the *coded* shape.
- **S1b**: the new SIP-join arm (if non-recursive slice) OR the fixpoint-interior guard (if recursive slice)
  + the witness + bench carrier + goldens.
- **S1c**: codegen ABI-suppression + Regional-census demand-awareness + bless.

## §6 The result-equality gate IS enforceable (refuter-3, verified) — the precedented pattern

The demand binary is deliberately kept **out of** the disjoint-referee comparison:
- `RefInterp` (demand-blind) emits nothing for a bound query under all-free enumeration and instead emits a
  `.probes`-restricted `PROBE <key>` block per demanded key (Main.cpp:1549/1573-1612) = the definitional
  closure restricted to that key.
- REFINTERP-DISAGREE compares the **PLAIN-compiled** behavioral golden vs `interp.cbf` — **both demand-blind**
  — so demand can never induce a false disagree.
- The **demand binary** is refereed by its own blessed `.stdout` (via `diffrun`, demand-ON) + 4-mode
  byte-agreement; the demand-vs-definitional equality is a **human bless-time reconciliation** (the witness
  `.dr` header states the obligation: "the driver's demand-ON answers for each probed key must be exactly the
  oracle's rows for that key").

So the witness recipe is precedented (`demand_tc_witness`): `.probes` naming demanded keys, a driver that
demands-then-probes those keys, `.batches` for the definitional net, blessed `.stdout` + 4-mode. **State the
gate precisely:** *per-key, per-epoch answer-set equality for demanded keys* — the definitional side is
disjoint-refereed; the demand binary is refereed by a separate blessed golden + cross-mode + a human bridge.
NOT a direct disjoint byte-check of demand-vs-full.

## §7 Recommendation + the owner decisions

**The namesake win is real and worth building.** But it is a **multi-session arc**, and the two shapes trade
off against each other. My recommendation, for the owner to ratify or override:

- **Path: Path R, FLAT-only** (skip InstanceStore = S2+) — cheaper first mechanical win, accepting the
  graph-mutation-authority debt *knowingly*; Path N (coherence) is the eventual refactor, best done once a
  working flat-demand oracle exists to refactor against. (If the owner weights greenfield coherence over
  first-win speed, choose Path N now and pay more.)
- **First slice: reconsider recursion.** The **recursive TC slice** is coded (`kPushDown`), witnessed
  (`demand_tc_witness`), and pays the most — a *stronger* "something real" than the non-recursive join, which
  needs a brand-new SIP arm. If the owner holds the non-recursive slice, the first cut's headline new code is
  the base-atom-below-join SIP arm + the JOIN drive-side verification.
- **Gate the build on an IN-COMPILER spike** (per refuter-1): before committing the full arc, a throwaway
  in-compiler spike that emits the real guard + rewires the JOIN, measured by `.ir`/`.h` + bench counters —
  to confirm emission fidelity (drive-side) that the hand-written twin cannot.

### Go/no-go questions
1. **Proceed with the demand arc at all**, given it is a 2–3 session arc (not one commit)? Or re-rank?
2. **Path R (flat-only, cheaper, re-adds graph-mutation authority) vs Path N (native, coherent, larger)?**
3. **First slice: recursive TC (coded, witnessed, biggest win) vs the confirmed non-recursive `r:-s,t`
   (needs a new SIP arm, modest win)?**
4. Sequencing: begin S1a (resurrect + validator re-integration) as its own gated cut, or scope the whole arc first?

## §8 Anchors (re-verify at next tip)
- O1 spike: scratchpad `spike/{plain,demand}.dr` + `driver.cpp` (counters seam; UNMODIFIED compiler).
- SIP arms: `48cd0a4f:lib/DataFlow/Demand.cpp:735-810` (kBaseAtom/kPushDown; the :791 reject).
- Injection site: `Join.cpp:623-666` (CHECKMEMBER); read path `Database.cpp:1527-1737`.
- Census landmine: `Planning.cpp:675/901`; `IsDemandMessage` deleted (grep-empty).
- Referee wiring: `RefInterp/Main.cpp:1549/1573-1612`, `runall.sh:381-475`; precedent `demand_tc_witness.*`@`48cd0a4f`.
- Deletion: `dc965d3c` (274 files, +9579/-11724); pre-cut tip `48cd0a4f`.
