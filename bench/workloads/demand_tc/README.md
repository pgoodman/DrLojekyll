# demand_tc — the S1b demand-pruning carrier (MANUAL; not runbench-integrated)

The measured proof that the REAL `-demand` transform prunes (the s29 O1 spike
re-run in-compiler — the spike's open "compiler-emission FIDELITY" question,
closed 2026-08-13). `runbench.sh` carries no compiler-transform flag (see
`bench/README.md`), so this family is run BY HAND; integrating it means
teaching the manifest a per-engine compiler-flags knob first.

The program is the recursive-TC `kPushDown` shape (identical to the suite's
`demand_tc_witness`): one bound `#query` over a right-linear closure. The
driver builds NCHAIN disjoint chains of length LEN, ingests them in one
batch, then probes — 8 chain heads (`selective`) or every node (`all`, the
demand-set == full-closure machinery-overhead regime) — folding all answers
into one order-independent count+FNV pair that must byte-agree between the
two binaries. Counters narrative ONLY (`-DDRLOJEKYLL_BENCH_COUNTERS`; the
counts binary is never timed — bench/BASELINE.md discipline).

## Run

```sh
DR=build/debug/bin/drlojekyll
W=/tmp/demand_tc_bench && mkdir -p $W/gen_plain $W/gen_demand
$DR bench/workloads/demand_tc/demand_tc.dr -cpp-out $W/gen_plain
$DR bench/workloads/demand_tc/demand_tc.dr -demand -cpp-out $W/gen_demand
CXX="clang++ -std=c++23 -O2 -DNDEBUG -DDRLOJEKYLL_BENCH_COUNTERS -I include"
$CXX -I $W/gen_plain  bench/workloads/demand_tc/driver.cpp \
    $W/gen_plain/datalog.cpp  lib/Runtime/Allocator.cpp -o $W/plain.bin
$CXX -DDEMAND_BUILD -I $W/gen_demand bench/workloads/demand_tc/driver.cpp \
    $W/gen_demand/datalog.cpp lib/Runtime/Allocator.cpp -o $W/demand.bin
$W/plain.bin  4000 10 selective 8   # vs
$W/demand.bin 4000 10 selective 8
$W/plain.bin  4000 10 all 0         # vs
$W/demand.bin 4000 10 all 0
```

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
