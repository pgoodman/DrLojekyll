# Ideas triage (2026-07-14, at the bench-harness close)

The owner's free-form idea notes (/tmp/drlog.md, outside the repo) were
triaged against the tree and the first accepted bench run
(bench/BASELINE.md). This file records the VERDICTS so they survive the
notes and the session; the full reasoning lived in the triage session.
Guiding lenses, owner-stated: McSherry/COST as the honesty oracle,
purplesyringa.moe/blog as the mechanical-sympathy oracle — every perf
idea is motivated by how the hardware executes it (cache lines,
branches, dependent loads), never by big-O or abstraction aesthetics
alone.

## Ranked: the actionable ideas (best first)

1. **Row-log compaction / dead-row GC at a size threshold** (notes #21,
   with #22-23 column-statistics-at-compaction as its layout step).
   Directly kills the measured 19.6x steady-state churn drift — the
   data-structures epoch's first build. The layout analysis runs AT
   compaction (never per-tuple bookkeeping on the hot insert path) and
   doubles as the sorted-storage precondition of the seekable/WCOJ
   substrate (PerfRoadmap §4).
2. **Id-keyed membership** (from the bench, not the notes): kill the
   redundant value-keyed re-Find per index hit (7-9 finds/fold
   measured). Notes #24 (dict-encoding low-cardinality u64) and #27
   (index rework) are the right follow-ons; #27's deque+Bloom+version
   mechanism is a CANDIDATE to benchmark against the seekable form,
   not the chosen design.
3. **Per-round overhead in deep cascades** (reshape of notes #45-47;
   StackSafeNegation §11 OQ4): ~900k mostly-empty SortAndUnique calls
   per 1e5-deep retract. Coarsen/remove the per-round barrier under a
   linearizable insertion order; do NOT add per-Add bitmap bookkeeping
   (taxes the hot path the bench shows is fixed-overhead-bound).
4. **Unknown→present publish/check elision** (notes #62): attacks the
   63x pure-cycle overdeletion amplification (OQ7a). Counter-contract-
   sensitive — must be a PROVABLE no-op-publish detection, oracle-
   refereed; sequence with the data-structures epoch's OQ7 work.
5. **Subgraphs seed ledger** — DONE at this triage: the notes' best
   ideas (@ephemeral, join-key-parameterized subgraphs, the shared
   aggregate protocol, cut-as-negation) are harvested into
   docs/proposals/Subgraphs.md with their guardrails.
6. **JOIN group_ids reshape for native self-joins** (notes #44): real
   payoff (the measured cubic tc⋈tc witness blowup) but group_ids is
   the load-bearing CSE guard — only inside the delta-relational IR
   rebuild, with an invariant-preservation argument.
7. **Surface sugar** (#object/#method/dot-chaining; functor-delivery
   pragmas; functor-negation-defaults-@never ruling): zero engine
   risk, pure ergonomics, no roadmap gate — schedule after the
   semantics epochs; revisit the functor pragmas against the
   hidden-friend/deduction surface first.

## Rejected / deferred, with reasons

- **Probabilistic / admission-controlled tables** (Bloom-gated "insert
  only N% of the time"): breaks set semantics, the exact-counter
  contract (presence = total > 0), and the golden-master discipline.
  Only the Bloom-as-negative-cache half survives, monotone tables only,
  and only if the data-structures epoch's numbers ask for it.
- **UUID/iteration-clock orchestration**: blocked on the unsolved
  distributed epoch-isolation design (StackSafeNegation §11 OQ10);
  record as input to that eventual design.
- **Worker-id in the functor ABI**: Stage-6 parallelism material; the
  batch-shaped ABI in AggregatingFunctors §3 is where it enters.
- **"Never drop constant columns"**: fights canonicalization and the
  column-edge invariants; if a concrete case regressed, that is a
  targeted FINDINGS-style repro, not a policy.
- **Cardinality algebra over the dataflow** (enum counts, union=max,
  join=min...): right substrate, no consumer yet — a design note
  feeding the data-structures epoch's presizing/join decisions, not a
  freestanding pass. Static enum-partitioning into sub-tables is the
  concrete payoff and connects to Subgraphs.md S3.
- **Cap'n-Proto relation bundles**: orthogonal serialization; collides
  with the zero-dep runtime; if wanted, it is a batch-shaped columnar
  format in the wasm/orchestration story.

## Discrepancies flagged for the owner

- Notes checkbox says the functor class-vs-global pragma and the
  C++-name-override pragma exist ([x]); the tree has neither (functors
  always emit as DatabaseFunctors members, now delivered by deduction).
- @inline functor bodies are parsed but land inside template bodies
  post-generated-surface (two-phase lookup; zero corpus coverage) —
  add a golden case before relying on them (also recorded in the
  GeneratedSurface landing record).
