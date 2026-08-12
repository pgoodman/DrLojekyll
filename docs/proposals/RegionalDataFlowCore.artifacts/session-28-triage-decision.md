# Session-28 triage decision — the "DO SOMETHING REAL" hunt

> STATUS: **DOCS-ONLY TRIAGE — no execution, no go/no-go issued.** Branch `keyed-instances`, tip
> `05af6595`. Produced by the session-28 grounding loop (my own IR/C++ reconnaissance → a 3-refuter
> opus panel over disjoint lenses → an opus synthesizer), plus a directional measurement pass over
> the dominant recursive workload. **VERDICT: no minimal, one-session, answer-equal, codegen-moving,
> bench-measurable win exists in C1/C2/C3.** This doc records the evidence and puts the re-sequence to
> the owner. NOTHING built or edited.

## §0 The mandate and the honest result

The owner's mandate (session 27, carried into the session-28 charter): the next cut must **DO SOMETHING
REAL** — change the generated C++ (`.ir`/`.h` goldens MOVE) in a way `bench/` measures (answer-equal to
M3, observably different/faster), or change answers. A byte-unchanged-codegen cut is a **shadow** and
fails the mandate. The charter's own gotcha: *"If the triage shows NO real minimal win in C1 or C2, SAY
SO plainly and put the re-sequence decision to the owner — do not manufacture a shadow."*

That is the situation. All three candidates were probed and refuted:

| Candidate | Verdict | One-line reason |
|---|---|---|
| **C1** fused-fixpoint keyed drain/seek | **DEAD** | the recursive fire is *already* a pivot-keyed semi-naive seek; `binding_prefix` is an output column, not a probe key; no demand restrictor exists to exploit (deleted at P1) |
| **C2** WCOJ / ordered-trie (P8) | **REAL but multi-session** | all-hash runtime, one recursive/differential triangle in the whole corpus, no bench carrier, ~5 mutually-dependent subsystems |
| **C3** bench-triaged physical win | **no removable redundancy** | the corpus's only full-scans are `@product` cartesians (unkeyable); the dominant hash-probe cost is intrinsic to semi-naive differential membership-gated join |

## §1 Why C1 is DEAD (confirmed ×3 + my own dumps, opt AND `none` mode)

The C1 premise — "the recursive fire full-scans where `binding_prefix` could key a seek" — is factually
false. M3 already emits a keyed seek in *every* recursive fire, keyed on the **join pivot** (optimal
semi-naive):

- `corecursion_1` (kFused, prefix=(A)): recursive round is `vector-loop {@X} over $induction_pivots_swap`
  (the pivot delta frontier) → two single-column hash seeks `idx_39.First({v38})/.Next` +
  `idx_40.First({v38})/.Next` on the join var X. Generated `datalog.h` has **zero `NumRows`** — no
  full-scan exists to eliminate. Holds in `none` mode too (`using %index:46/%index:47`).
- `tc_nonlinear_diff` (kFused, prefix=(From,To)): all four recursive fires (OVERDELETE + INSERT rounds)
  are `scan-index … plan=partial-key-hash-seek` on single-column `%index:80`/`%index:87`, driven by the
  claimed-frontier delta queue.
- `key_corecursion_1` (kFused, prefix=(K)): same pivot-keyed seek.

**The decisive point:** `binding_prefix` is the OUTPUT-preserved column set, not a runtime-bound probe
key. Keying the fire on the prefix instead of the join pivot is *wrong* (≠ pivot) or *vacuous*. The only
thing that could make a prefix key restrict fixpoint work is a **bound-query restrictor propagated
through the fixpoint** — i.e. **demand**, deleted at the P1 greenfield cut. And `binding_prefix` has zero
codegen consumer today (kFused/kJoint emit byte-identical semi-naive rounds), so C1 would mean building a
fused-round emission arm from scratch that *still has no restrictor to exploit*. **Dead absent demand.**

## §2 Why C2 / WCOJ is REAL but genuinely multi-session (confirmed ×3)

- **The runtime is 100% full-key-exact hash.** `Table.h class Index` exposes only `Find`/`FindWithHash`
  (full-key) and `First(key)`/`Next(id)` walking a hash chain for ONE full key. No `lower_bound`/ordered/
  range/trie/leapfrog primitive. Leapfrog triejoin needs `seek(x)≥x` over sorted data — impossible over
  these indexes, so there is **no shortcut reusing existing hash indexes**.
- **Exactly ONE cyclic join in the corpus:** `f16_join_witness` `r(A,B):-r(A,C),r(C,B),r(B,A)` — and it
  is *recursive + differential-capable*, not a clean base-edge triangle. M3 emits it as a two-stage
  binary join that **materializes a quadratic 3-column intermediate `%table:8`** (the genuine WCOJ
  target), but every *other* multi-atom clause in the corpus is α-acyclic, where binary hash-join is
  already worst-case-optimal.
- **All 5 bench workloads are 2-way joins** — zero WCOJ carrier; one must be fabricated.
- **~5 mutually-dependent subsystems:** (1) an order-maintaining runtime structure surviving the
  append-only-increasing-id + `CompactRowsInPlace` hash-compaction contract; (2) a DataFlow WCOJ-
  eligibility arm that *stops* decomposing cyclic joins into binary+intermediate; (3) a leapfrog
  `EmitJoin` arm in both monotone and differential forms; (4) a new Rel op + its cross-check machinery
  (V-JOIN-ONE, V-JOIN-EMIT-XCHECK, kEagerJoin/kJoinEmit are all keyed on the binary TABLEJOIN model);
  (5) a fabricated bench carrier. Even a hash-based generic-join variant (dropping #1) still needs 2–5
  and pays only on the single synthetic recursive shape. **First sub-slice = a full session before any
  answer-equal codegen moves.**

## §3 Why C3 (bench-triaged) found no removable redundancy

Static sweep: 22 `full-scan-filter` lines corpus-wide, ALL in `@product` cartesian arms (product_*/
insert_4) or condition-gated projections (cond_*). Neither is keyable — a cartesian product has no
shared variable (no key); a condition-gated projection emits all its rows (no restricting key). No
hidden full-scan to seek-ify.

Directional measurement (this session): the dominant recursive workload's hot loop (tc_random, opt)
per yielded join row is `scan-index (partial-key-hash-seek) → check-member → update-count`. The
`check-member` (`alive-at-claim`/`survives-so-far`/`in-new-*`) is an **ID-keyed side-array read on the
seek cursor**, NOT a re-Find (already harvested by D1/P7c); it is the differential liveness gate and is
semantically required. The seek's index-chain walk is the intrinsic hash cost. This matches
`bench/BASELINE.md`'s §3 finding that **hash probing is the universal #1 cost** (~2.6–3.5 probe steps
per find, ~7–9 Finds per fold) — a cost **intrinsic** to semi-naive differential membership-gated join
over hash tables, reducible only by a *different data structure* (ordered/trie → P8, or a membership-
carrying index) or a *different join algorithm* (WCOJ → C2). Neither is a minimal codegen selector flip.

**Caveat (honest):** a full `bench/`+profiler accepted-run grid was NOT executed — the measurement above
is directional (hot-loop inspection + the existing BASELINE operation breakdown). I assess a full run as
low-probability to surface a *minimal* fix (it would quantify the intrinsic cost, not remove it), but the
owner may want it run for certainty before conceding — see Q2.

## §4 The deeper truth: the arc is consumer-gated, not idea-starved

Session 27's bedrock finding generalizes cleanly. The four landed compile-time analyses (P6.1
`recursive_components`, P6.2 `rules`/`inherited_symbolic_fields`, P6.3 `fusion`/`binding_prefix`, and
F16-close) are *trustworthy* but *consumer-less* — **because the generated runtime is already well-
optimized for the corpus it targets** (acyclic / 2-way recursive joins, all-hash). The P7/P7b/P7c arc
harvested the seek wins; what remains is intrinsic. So `fusion` etc. have no codegen lever to pull *on
this runtime + this corpus*. Making the model "do something real" therefore requires changing one of the
two givens: **the runtime** (a new data structure / join algorithm — C2/P8) or **the workload semantics**
(reopen demand/keyed evaluation so a bound query materializes only demanded rows — the branch's namesake,
deleted at P1). Both are multi-session. There is no minimal middle.

## §5 RE-SEQUENCE menu (for the owner)

**(a) Commit to C2 / WCOJ as an explicit multi-session arc.** Real, but poor near-term ROI: payoff is
confined to one synthetic recursive shape (f16), no clean bench, first sub-slice is a full session before
codegen moves answer-equally. Choose only if WCOJ is a *strategic* goal regardless of near-term ROI.

**(b) Run C3 the way it was defined — a directed `bench/`+profile spike** on the 5 live workloads before
conceding. Cheap-ish, faithful to C3's charter, but my directional measurement already indicates the
hotspot is intrinsic index-maintenance/membership — likely earns an honest negative rather than a fix.

**(c) Change ANSWERS: reopen the demand/keyed-evaluation endpoint (P2–P9).** The mandate explicitly
permits "OR change answers." This is the branch's actual namesake — rebuild the keyed-instance lowering
deleted at P1 so a bound `#query` materializes only demanded rows (far fewer rows scanned = a genuine,
directed bench win, answer-equal in *result* but different in *derivation cost*). It is the highest-value
direction and the one the greenfield motivation was about — but it is the whole multi-session program,
not a minimal cut, and it re-opens the demand machinery C1 needs anyway.

### My recommendation
The model-drives-codegen arc has reached a **natural, honest pause**: every landed analysis is correct
and every minimal codegen lever is already pulled. I do **not** recommend manufacturing a next cut.
Between the real bets, **(c)** is best aligned with the branch's purpose (keyed/demand evaluation is the
namesake and the deleted-at-P1 payoff) and produces the most valuable real win; **(a)** is the narrower,
lower-ROI runtime bet; **(b)** is worth one cheap spike only if the owner wants measured certainty before
committing to (a) or (c). If the owner wants to stay strictly answer-equal AND minimal AND one-session,
the honest answer is *there is no such cut right now* — the arc pauses until a runtime or semantics change
is authorized.

## §6 Go/no-go questions for the owner
1. **Accept the unanimous refutation** that C1 (fused-fixpoint keyed drain) is dead absent demand?
2. **Run the directed `bench/`+profile spike (b)** for measured certainty before conceding C3, or accept
   the directional finding that the dominant cost is intrinsic?
3. **Which multi-session real bet:** (a) C2/WCOJ (narrow runtime win), or (c) reopen demand/keyed
   evaluation (broad, the namesake, answer-cost win)? Or (d) pause the arc entirely and pick a different
   project axis?
4. **Standing constraint:** must the next cut stay answer-equal, or is *changing answers* (option c) on
   the table as the "something real"?
