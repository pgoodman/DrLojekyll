# demand_tc — the S1b demand-pruning carrier (runbench family since run 12)

The measured proof that the REAL `-demand` transform prunes (the s29 O1 spike
re-run in-compiler — the spike's open "compiler-emission FIDELITY" question,
closed 2026-08-13; accepted-run record in BASELINE.md run 12 and
`docs/proposals/RegionalDataFlowCore.artifacts/s1b-bench-carrier.md`).

The program is the recursive-TC `kPushDown` shape (identical to the suite's
`demand_tc_witness`): one bound `#query` over a right-linear closure. The
driver builds nchain disjoint chains of length len, ingests them in one
batch (epoch 0), then probes (epoch 1) — `probe=selective` (chain heads) or
`probe=all` (every node; the demand-set == full-closure machinery-overhead
regime) — folding all answers into one order-independent count+FNV
sentinel. ONE driver serves both compiles: the probe call adapts at compile
time (a `requires` check) to the forced (demand) vs plain query signature,
so the runspec selects the transform purely via the harness `drflags=`
knob. The plain and `-demand` engine lines share one (workload, knobs) key,
so the runner's sentinel cross-check enforces plain==demand answer-hash
agreement per knob-point.

## Run

```sh
DR=build/debug/bin/drlojekyll REPS=5 COUNTS=1 \
  bench/runbench.sh /tmp/demand_tc_bench \
  bench/workloads/demand_tc/runspec.txt opt
```

Wall rows (`t_ingest_ns`/`t_probe_ns`) are the timing narrative (labels
`opt` vs `opt+demand`); `ctr_*` rows from the `+counts` labels are the
pruning narrative — never multiply the two.

## Accepted numbers (2026-08-13, tip = s36 CP4 lineage; 4000×10 chains,
full closure 220,000 rows; ingest+probe totals; answers count+hash EQUAL in
every cell)

| regime | metric | PLAIN | DEMAND | factor |
|---|---|---|---|---|
| SELECTIVE (8 heads) | **idx_hops** | **580,080** | **1,192** | **~487× less** |
| | idx_first | 480,008 | 40,336 | 11.9× less |
| | probe_steps | 1,930,830 | 266,939 | 7.2× less |
| | finds | 260,000 | 81,296 | 3.2× less |
| NON-SELECTIVE (all 44,000 nodes) | idx_hops | 800,000 | 3,748,000 | **4.7× MORE** |
| | probe_steps | 1,930,830 | 25,942,269 | 13.4× MORE |
| | finds | 260,000 | 3,536,000 | 13.6× MORE |

Reading: PLAIN pays the whole closure fixpoint at INGEST (580k `idx_hops` of
join work); DEMAND's ingest does ZERO recursive-join hops (nothing demanded
⇒ the guarded join never fires) and each probe pays only its key's slice.
The non-selective regime regresses exactly as the cost model predicts
(machinery is pure overhead when nothing is pruned — worse here than the
hand spike's 0.71× because demanding every NODE makes the demand relation
itself closure-sized). **`-demand` stays opt-in / cost-gated; never a fifth
golden mode.** Record: `docs/proposals/RegionalDataFlowCore.artifacts/
s1b-bench-carrier.md`.
