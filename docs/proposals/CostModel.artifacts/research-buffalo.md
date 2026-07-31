# Buffalo / ODIn Lab research — relevance assessment for Dr. Lojekyll

Assessed 2026-07-31. Whose research: the **ODIn (Online Data INteractions)
Lab, University at Buffalo CSE, led by Oliver Kennedy** (PI). Kennedy is the
co-author of DBToaster (higher-order delta / IVM), which is the lineage this
whole line of work descends from and the reason it is unusually close to
Dr. Lojekyll's differential core.

## Sources fetched (verbatim URLs)

- Landing: https://odin.cse.buffalo.edu/research/ (and .../research/index.html)
- **Flow-centric Query Evaluation Pipelines** [Talk], NEDB 2026 —
  https://odin.cse.buffalo.edu/papers/2026/NEDB-FlowCentric.pdf (read in full)
- **TreeToaster: Towards an IVM-Optimized Compiler**, SIGMOD 2021 (file is
  mislabeled `SIGMOD-IVMForCompilers.pdf`) —
  https://odin.cse.buffalo.edu/papers/2021/SIGMOD-IVMForCompilers.pdf (read pp.1-12 = whole body)
- Draupnir repo (the flow-centric on-disk Datalog engine):
  https://git.odin.cse.buffalo.edu/Norn/Draupnir
- FastPDB, SIGMOD 2025 — https://odin.cse.buffalo.edu/papers/2025/SIGMOD-FastPDB.pdf (skimmed; low relevance)
- Prior/adjacent: OnDB 2025 Flow-centric Data Pipelines; UADB VLDB 2023;
  DBToaster https://dbtoaster.github.io/ ; Kennedy CV
  https://odin.cse.buffalo.edu/artifacts/okennedy.pdf
- PI confirmation + Hedgelog description via web search (ODIn research page).

## Research agenda (three threads)

1. **Declarative compilers & program analysis** — reconceive compilation as
   database ops. Systems: **Draupnir** (Datalog engine, group/ring-theoretic
   provenance, "IVM for compilers"), **Hedgelog** (Datalog + group-theoretic
   aggregates), **DiDB** (distributed incremental DB). Active: Victoria Dib.
2. **Reproducible data science** — Vizier, Theseus (workflow/notebook, schema
   evolution). Not relevant to Dr. Lojekyll.
3. **Uncertain data management** — Mimir/Caveats, **FastPDB** (bag-PDB +
   approximate query processing), **UADB** (range-annotated incomplete data).
   Orthogonal to Dr. Lojekyll's directions.

The DBToaster → declarative-compiler evolution is the spine: thread (1) is a
near-sibling of Dr. Lojekyll.

## Cataloged papers/systems that matter here

### A. Flow-centric Query Evaluation Pipelines (NEDB 2026) — Dib, Mikalsen, Sivakumar, Zola, Kennedy

This is, essentially, an **independent restatement of Dr. Lojekyll's DR-IR /
Rel model.** Core thesis, quoted: *"operator-materialized state is (almost)
exactly the data passing over one (or more) of these flows"* — so they split
the pipeline into **stateful Data operators** and **stateless Compute
operators** (Compute pulls from Data, pushes to Data). That is exactly
Dr. Lojekyll's split: typed DRVecs (queues/frontiers/pivots) hold the state,
the DROps are the compute, and the DR-IR flow graph is the scheduling
authority.

Two specifics that map directly onto open Dr. Lojekyll directions:

- **Cursor property labels.** Each Compute-operator input is annotated with
  properties the backing Data operator must satisfy: `Coalesced`
  (pre-aggregated), `Clustered`/`Sorted` by key (→ `seek_to_key`), `Closed`
  (→ `seek_to_head`). The compiler's job is (i) merge flows that share
  sources into one Data operator, (ii) instantiate a data structure that
  satisfies the properties. **This is precisely the seekable-iterators / WCOJ
  memory note** (rntz seekable iterators; the `CompactRowsInPlace`
  sorted-layout hook; PerfRoadmap §9 D5): a property lattice on cursors that
  lets a leapfrog-triejoin-style seek be *requested* by an operator and
  *provided* by the data structure. Dr. Lojekyll has the hook but not the
  property-labelled-cursor abstraction; this paper is the design template.
- **Aggregation becomes a Data operator**, deferrable via DBToaster algebraic
  tricks (Koch 2014). Mirrors Dr. Lojekyll's R3 aggregate lowering to a
  `GROUP_UPDATE` op over a `StateCellStore` (the aggregate/KV *is* a stateful
  data node, a branch chain-breaker).

Deployment differs: Draupnir/flow-centric is **out-of-core / on-disk**, and
its motivating win is giving *the runtime scheduler visibility into memory
usage* (morsels for streaming, on-disk hash for Clustered). Dr. Lojekyll is
in-memory, message-driven, codegen'd. Not a conflict — different targets, same
IR philosophy. Their "merge flows into combined Data operators" is the analog
of Dr. Lojekyll's model-table sharing / CSE. This is **validation of the
DR-IR bet by an independent group**, plus a concrete cursor-property design to
borrow.

### B. TreeToaster: Towards an IVM-Optimized Compiler (SIGMOD 2021) — Balakrishnan, Nuessle, Kennedy, Ziarek

IVM specialized to a compiler's optimizer (AST pattern-match + rewrite as
incrementally-maintained views). Highly relevant on **two** axes:

- **Signed-multiplicity differential semantics.** §5 builds on *"generalized
  multisets ... that allow elements with negative multiplicities"* (Blizard),
  with `⊕` summing multiplicities and `⊖` subtracting — i.e. a delta is a
  signed multiset, presence = nonzero multiplicity. This is the same algebra
  as Dr. Lojekyll's **split signed derivation counters** (`C_nr`/`C_r`,
  presence = total > 0, may dip below zero mid-batch). External corroboration
  of the core differential invariant, with clean lemmas (View Correctness,
  Def. 4; IVM correctness, Lemma 5.2).
- **The cost-model direction, concretely.** Fig. 1 decomposes optimizer time
  into **Search / Ineffective Rewrites / Effective Rewrites / Fixpoint Loop**
  (33-45% search, 27-43% fixpoint on TPC-H). That is exactly the "which
  operation counts explode, and is the machinery worth it" instrument the
  CostModel work wants — an *analytic decomposition of maintenance cost into
  named phases*, then a technique that zeroes one phase (search) out. It also
  quantifies the **memory-vs-latency frontier** (Fig. 2/11): bolt-on DBToaster
  IVM costs ~2.5× AST memory; TreeToaster keeps IVM's latency at ~half the
  bolt-on approach with negligible memory. This is the honest cost referee
  (COST-paper spirit) applied to IVM machinery — directly the CostModel
  "when does -demand / keyed-instance pay for itself" question.
- **Bounded re-check via pattern depth `D(q)` / "maximal search set"** (§5.1,
  Def. 6): only ancestors up to height `D(q)` of a changed node can gain/lose
  a match, so the delta is *linear in the rewrite size*, not the tree. This is
  the same shape as Dr. Lojekyll's demand/keyed-instance reachability
  argument — materialize only what a changed/demanded key can reach. A
  candidate formal handle for the demand cost model.

Caveat worth recording (not a conflict, a *scope* note): TreeToaster finds
that **for ASTs specifically**, caching intermediate join state is *"either
redundant or minimally beneficial"* because AST nodes have a single parent and
foreign-key-constrained children (a node joins into ≤1 result per position).
Dr. Lojekyll's general relations do **not** have that property, so
DBToaster-style intermediate materialization (and the group_ids/self-join
concern) stays load-bearing for general joins — TreeToaster's "skip the cache"
result does not transfer. Read it as a boundary condition, not a supersession.

### C. Hedgelog + Draupnir ring/group provenance for aggregates — theory foundation

Hedgelog: *"group-theoretical techniques ... a language fully
backwards-compatible with Datalog"* for aggregates, on the semiring/ring-
provenance (Green et al. / ring-databases) line. This is the **theoretical
home of Dr. Lojekyll's aggregate algebra selector**: `@invertible` (a *group*
— O(1) fold *and* unfold, `f_identity/f_combine/f_uncombine`) vs `@recompute`
(no inverse — per-group rescan `f_reduce`). The invertible/non-invertible
split *is* the group-vs-monoid distinction Hedgelog formalizes. If Dr. Lojekyll
ever wants a principled account of *which* aggregates admit O(1) differential
maintenance (and how to compose them under recursion), this is the framework.
Currently unread in depth (no direct paper link surfaced beyond the SIGMOD-21
provenance framing); theory-foundation tier.

### D. FastPDB / UADB — low relevance

Bag-probabilistic queries at interactive speed; range-annotated incomplete
data with certain/possible-answer bounds. Touches Dr. Lojekyll only weakly:
"approximate cost/answer bounds" is a distant cousin of a cost model, and the
provenance-semiring machinery overlaps thread (C). Not actionable now.

## Relevance ranking (most → least load-bearing)

1. **Flow-centric Query Evaluation Pipelines + Draupnir** — the single most
   relevant item. Independent sibling of the DR-IR; hands Dr. Lojekyll a
   ready-made **cursor-property abstraction** for the WCOJ/seekable-iterator
   direction, and validates the state-lives-on-the-flow bet.
2. **TreeToaster** — the differential core's signed-multiset algebra confirmed
   externally + a concrete **phase-decomposed IVM cost model** and
   memory/latency frontier, i.e. the CostModel instrument in prior art. Plus a
   bounded-re-check bound (`D(q)`) that rhymes with demand reachability.
3. **Hedgelog / ring-provenance aggregates** — theory foundation for the
   `@invertible`/`@recompute` group-vs-monoid split and its composition under
   recursion.
4. FastPDB / UADB / Vizier / Mimir — orthogonal; skip.

## Conflict / supersession flags

- **No supersession.** Nothing here obsoletes a Dr. Lojekyll design choice.
  The two closest works (flow-centric IR, signed-multiset IVM) *converge on*
  choices Dr. Lojekyll already made independently — that is corroboration, and
  raises confidence in the DR-IR and the split-counter differential model.
- **Scope caveat (record-only), TreeToaster §3-4:** "intermediate join state
  is redundant for ASTs" is true only under the AST's single-parent /
  FK-child shape; do **not** generalize it to Dr. Lojekyll's arbitrary
  relations. It reinforces, not contradicts, keeping intermediate
  materialization for general joins.
- **Deployment mismatch (not a conflict):** flow-centric targets out-of-core
  disk + a memory-aware runtime scheduler; Dr. Lojekyll is in-memory codegen.
  The cursor-property *abstraction* transfers; the on-disk data-operator
  *implementations* (morsels, on-disk hash) are a different regime.

## Strongest to read next (tiered)

- **Immediately actionable — Flow-centric Query Evaluation Pipelines (NEDB
  2026) + the Draupnir repo.** Why: it is the DR-IR under another name and
  gives a concrete cursor-property lattice (`Clustered/Sorted/seek_to_key`)
  to lift into the seekable-iterator / WCOJ hook (`CompactRowsInPlace`,
  PerfRoadmap §9 D5). Short (2-page talk) — read the repo for the engine.
- **Immediately actionable — TreeToaster (SIGMOD 2021).** Why: a worked
  phase-decomposed IVM cost model + memory/latency frontier — the exact shape
  the CostModel work needs — and an external proof of the signed-multiset
  differential algebra. Also mine §5.1 `D(q)`/maximal-search-set for the
  demand cost model.
- **Theory-foundation / read-later — Hedgelog + the ring/semiring-provenance
  aggregate line (Kennedy et al.).** Why: principled account of which
  aggregates are O(1)-invertible (group) vs rescan-only (monoid) — the formal
  backing for `@invertible`/`@recompute`, especially if aggregates ever need
  to compose inside recursion.
- **Skip for now — FastPDB / UADB.** Uncertain-data, orthogonal to current
  directions.
