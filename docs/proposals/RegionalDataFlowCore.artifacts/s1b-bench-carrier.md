<!-- Copyright 2026, Peter Goodman. All rights reserved. -->
# S1b bench carrier — the REAL transform's pruning, measured (closes the s29 O1 fidelity question)

Run 2026-08-13 at the s36 CP4 lineage (injector landed, `demand_tc_witness`
green). The s29 O1 spike proved the magic-sets ALGORITHM prunes but hand-staged
the guard placement (compiler unmodified) — its verdict left "compiler-emission
FIDELITY: OPEN ... to be gated on an IN-COMPILER spike whose counters are
measured — not a hand-written twin" (`session-29-grounding.md` §1). This is
that measurement.

## Setup (carrier: `bench/workloads/demand_tc/`, run instructions in its README)

- Program: the recursive-TC `kPushDown` shape (byte-identical rules to the
  suite's `demand_tc_witness`): `path :- edge` / `path :- path, edge` + ONE
  bound `#query reachable_from(bound From, free To)`.
- TWO binaries from the SAME driver source: PLAIN (no flag; post-hoc cursor
  over the fully-materialized closure) vs DEMAND (`-demand`; the s36-landed
  injector seeds `demand__reachable_from_bf` per probe). Both
  `-O2 -DNDEBUG -DDRLOJEKYLL_BENCH_COUNTERS` (counts narrative only; counts
  binaries never timed — bench/BASELINE.md discipline).
- Dataset: 4000 disjoint chains × length 10 (44,000 nodes, 40,000 edges;
  full all-pairs closure = 220,000 `path` rows).
- EQUALITY SURFACE: every probe's answers fold into one order-independent
  (count, FNV) pair — byte-equal between the binaries in EVERY cell below
  (selective: 80 rows; non-selective exhaustive: 220,000 rows — the demand-ON
  program's answers ARE the definitional closure per probed key).

## Results (ingest+probe totals)

| regime | metric | PLAIN | DEMAND | factor |
|---|---|---|---|---|
| SELECTIVE (8 head probes) | **idx_hops** (join work) | **580,080** | **1,192** | **~487×** |
| | idx_first | 480,008 | 40,336 | 11.9× |
| | probe_steps | 1,930,830 | 266,939 | 7.2× |
| | finds | 260,000 | 81,296 | 3.2× |
| NON-SELECTIVE (all 44,000 nodes probed) | idx_hops | 800,000 | 3,748,000 | **0.21× (REGRESSION)** |
| | probe_steps | 1,930,830 | 25,942,269 | 0.07× (REGRESSION) |
| | finds | 260,000 | 3,536,000 | 0.07× (REGRESSION) |

The load-bearing split: PLAIN's ingest carries the ENTIRE closure fixpoint
(580,080 `idx_hops`; its probes then cost 80 hops total). DEMAND's ingest
performs **zero** recursive-join hops — with nothing demanded, the guarded
fixpoint never fires — and each probe pays only its key's slice (1,192 hops
across 8 probes, ~149/probe ≈ the 10-row suffix + guard machinery). This is
the in-compiler confirmation that the emitted TABLEJOIN drives from the
demanded frontier (the seed §2 `kPushDown` obligation).

## Verdict

1. **Emission fidelity: CLOSED.** The real transform's emitted code prunes —
   selective join work drops ~487× on this dataset, answers exactly equal.
   (Not directly comparable to the hand spike's 40000→11: different shape —
   recursive TC vs the non-recursive `r:-s,t` the transform deliberately does
   not code — and this dataset's closure is 5.5× the edge count.)
2. **The regression is confirmed and WORSE than the hand spike** (0.21× vs
   0.71× on join work): demanding every node makes the demand relation itself
   closure-sized, so machinery cost scales with the demand set. `-demand`
   stays opt-in / cost-gated (never a fifth golden mode) — unchanged policy,
   now with in-compiler numbers.
3. Result-equality remains CONTINGENT on query-set ⊆ demand-set by design
   (demand-before-query is the driver contract; an undemanded key
   under-answers) — the s29 precision note stands.

## Residue

- `runbench.sh` integration needs a per-engine compiler-flags knob (the
  bench/README run-11 COST note) — the carrier is manual until then.
- S1c polish (Tier-1 demanded-interior naming) and S2+ (keyed InstanceStore)
  per `session-31-s1b-seed.md` §5.
