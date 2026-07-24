# Dr. Lojekyll

Datalog compiler ("hyde" C++ namespace). Compiles Datalog to an incremental,
message-driven C++ database. Pipeline:

    parse (lib/Lex, lib/Parse)
      → data-flow IR        lib/DataFlow    Query::Build(module, log, optimize)
      → control-flow IR     lib/ControlFlow Program::Build(query, first_id, optimize)
      → C++ codegen         lib/CodeGen/CPlusPlus/Database.cpp
      → runs against        lib/Runtime + include/drlojekyll/Runtime (no deps)

`docs/RuntimeAndCodegen.md` describes the runtime and generated-code shape.
CLI driver: `bin/drlojekyll/Main.cpp`.

## Build

```sh
cmake --preset debug            # configure (Ninja, build/debug)
cmake --build --preset debug    # build; binary at build/debug/bin/drlojekyll
```

Presets: `debug`, `release`. C++23. No third-party deps beyond `vendor/`.
Coverage build (llvm-cov):

```sh
cmake -B build/coverage -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDRLOJEKYLL_ENABLE_TESTS=OFF \
  -DCMAKE_CXX_FLAGS='-fprofile-instr-generate -fcoverage-mapping' \
  -DCMAKE_EXE_LINKER_FLAGS='-fprofile-instr-generate'
# run with LLVM_PROFILE_FILE=..., then xcrun llvm-profdata merge / llvm-cov report
```

## Test

```sh
cd build/debug && ctest --output-on-failure   # MiniDisassembler, PointsTo, Runtime
```

End-to-end tests compile a `.dr` file at build time via `compile_datalog()`
(cmake/Compiler.cmake) and link a hand-written driver against the generated
code + runtime (see `tests/MiniDisassembler`, `tests/PointsTo`).
`tests/DrTest` is a dependency-free mini-GoogleTest (`TEST`, `ASSERT_*`).

### Golden-master optimization testing

The compiler has optimization toggles: `-disable-dataflow-opt` (skips
`QueryImpl::Optimize`: CSE, canonicalization rounds, dead-flow elimination)
and `-disable-controlflow-opt` (skips `ProgramImpl::Optimize`: region
flattening, no-op removal, procedure dedup).

The suite is golden-master-based: each case in `tests/OptDiff/cases/`
(`<name>.dr` + `<name>.main.cpp`, 173 corner-case programs as of the
keyed-instances D2.c landing — symrec_tie_1 is the standing determinism
witness) has one committed expected output in
`tests/OptDiff/goldens/<name>.stdout`, and the 4 optimization modes are
just execution variants — EVERY mode's stdout is byte-compared against the
same golden (cross-mode agreement is implied). A case with a
`<name>.batches` sidecar additionally runs the derivation-counter oracle
(`bin/Oracle`, built as `drlojekyll-oracle`) and the monotone projection,
each against its own golden (`<name>.oracle.stdout` / `<name>.monotone.stdout`).
Notable directed witnesses: `fixpoint_stress_1` (same-round double-claim +
REDERIVE partial-restore + phantom pairs + add-side stale drops),
`reconverge_1` (reconvergent table-less plumbing, the DiscoverBranches-
memoization guard), and the R3 aggregate corpus (`average_weight`,
`pairwise_average_weight`, `aggregate_1`, all with `.batches` +
oracle/monotone goldens; `algebra_invertible_1` witnesses the `@invertible`
surface; `config_agg_2` witnesses the config-column `@recompute` arm with a
descending-max retraction). `demand_tc_witness` is the demand-ON witness — a
plain base `.dr` compiled under `-demand` via its `<name>.drflags` sidecar
(per-case compiler flags appended by runall.sh/diffrun.sh; the four
optimization modes stay orthogonal to it), the transform doing all the work.
`tests/OptDiff/permcheck.py` mechanizes the permutation-only
bless referee (published-delta tokens compare order-free per epoch, all
other lines byte-identical) for any emission-shape change under the
delta-relational-IR golden policy.

- One case: `tests/OptDiff/diffrun.sh <case.dr> <driver.cpp> <workdir>`
  (env: `DR=` compiler path, `TIMEOUT=` seconds).
- Full suite: `DR=build/debug/bin/drlojekyll tests/OptDiff/runall.sh
  <workroot> [jobs] [name-filter-regex]` — must end `SUITE: PASS`. The
  expected-diagnostic cases are encoded in runall.sh (READ IT for the
  authoritative list): all-4-modes-diagnostic = `kvindex_2/3/4`,
  `agg_in_scc_1`, `kv_in_scc_1` (unstratified aggregation — an aggregate/KV
  index over its own recursive result, rejected by the same Stratify pass
  as the negation reject), `algebra_dup_1`, `algebra_conflict_1` (the
  @-algebra pragma surface: a duplicate pragma / the mutually-exclusive
  @invertible+@recompute pair, rejected in Functor.cpp), `evm_func_parse`,
  `nonascii_1`, `truncated_decl_1`, `demand_multi_adorn_1` (a `-demand` query
  name carrying >1 binding pattern — the demand pass's clean per-name reject,
  via its `.drflags` sidecar), `demand_cyclic_1`/`demand_diff_input_1` (two
  `-demand-instance` nested-lowering feature-gap fences — recursive demand and
  a @differential summarized input; both COMPILE under plain `-demand` and
  reject only under `-demand-instance`) and `demand_recursive_content_1` (a
  recursive-content demanded body — rejected UPSTREAM by the plain-`-demand`
  body-walk, so its `.drflags` is a bare `-demand`; it pins the shadowed
  Build.cpp recursive-content belt); `kvindex_1` is MODE-SPLIT (compiles
  under opt/nocf where KVINDEX→TUPLE elimination fires, V-ALGEBRA-rejects
  under nodf/none). `aggregate_1` FLIPPED from diagnostic to a 4-mode
  golden at the R3 stage-C flip.
- Blessing: goldens change ONLY via explicit
  `runall.sh --bless <workroot> [filter]` after reviewing a run's outputs —
  never automatically on failure, and never to make a red case green.

`tests/OptDiff/FINDINGS.md` is the ledger of bugs found this way, with
repros (F1–F19 and F21 fixed as of July 2026; F20 is an open record-only
latent-comparator note).

### Bench harness (perf, never gates correctness)

`bench/` is the COST instrument (PerfRoadmap.md §2; methodology + first
accepted run in `bench/BASELINE.md`; how-to in `bench/README.md`). Run:
`DR=build/debug/bin/drlojekyll bench/runbench.sh <workroot> <runspec>
[modes]`. Bench builds are `-O2 -DNDEBUG`; the optional runtime counter
seam (`-DDRLOJEKYLL_BENCH_COUNTERS`, Runtime/BenchCounters.h) is a
suite-verified no-op when off, and counts binaries are never the timed
binaries. Comparisons key on (case, mode, knobs) semantics, never
generated-text hashes. Never time bench runs concurrently with suite
runs; never rebuild the compiler mid-run.

Manual compile of generated code (driver pattern in any `cases/*.main.cpp`):

```sh
build/debug/bin/drlojekyll foo.dr -cpp-out gen/          # emits datalog.h/.cpp
clang++ -std=c++23 -I include -I gen driver.cpp gen/datalog.cpp \
  lib/Runtime/Allocator.cpp -o case
```

Generated API (the hidden-friend surface, since the generated-surface
epoch): database name defaults to `datalog`, no namespace; `datalog.h` is
the whole header-only artifact (`datalog.cpp` is an anchor TU — compile
lines unchanged). `struct Database` is a sealed state struct constructed
with just the allocator; ALL driver-facing functions are hidden friends,
reachable only by unqualified ADL call with the database argument (never
qualify the calls; types may be qualified). Epoch 0 is explicit:
`init(db, log, functors)` once, immediately after construction, before
anything else (entry points and queries assert it). Messages are
`<name>_<arity>(db, log, functors, Vec<...>[, Vec<...>])`; queries are
`<name>_<bindings>(db, bound...)` (`b`/`f` per column) returning bool
(all-bound) or a cursor (`auto c = q_f(db); c.next(out...)`); queries
with a forcing function also take `(log, functors)` after `db`. Log and
functor types flow by deduction — a driver observes published deltas by
providing ITS OWN type with the message-hook signatures (no inheritance,
no virtual, no `override`); since the P1 MAP-functor migration
(ADL/functor-surface epoch) EVERY functor body is a driver-supplied FREE
FUNCTION — MAP functors are forward-declared in the header as
`<ret> <name>_<pattern>(bound...)` and defined out-of-line (unqualified
call from the generated template context; `struct DatabaseFunctors {}`
survives EMPTY as the deduction anchor drivers still construct and
pass). CURSOR CONTRACTS
(normative since the data-structures epoch — dead-row compaction
renumbers row ids): any entry-point call invalidates open cursors
(drain fully before the next message), and keyed-cursor enumeration
order is unspecified — drivers must sort keyed drains before printing
(every corpus driver does; review gate on new ones). AGGREGATE / KV
DRIVER CONTRACT (since the R3 delta-relational-IR flip): a program with
an `over(){}` aggregate or a `mutable(...)` KV merge additionally needs
its reduction bodies as DRIVER-SUPPLIED FREE FUNCTIONS (forward-declared
in the header, defined out-of-line, named after the functor) — an
`@invertible` functor `f` needs `f_identity()`, `f_combine(w, v)`,
`f_uncombine(w, v)`; an `@recompute` functor needs
`f_reduce(const S *values, const int32_t *counts, size_t n)` (a rescan
over the live multiset). The functor's own MAP deliveries (e.g.
`div_i32_bbf`) are free functions too, declared in the header and
defined out-of-line. See `tests/OptDiff/cases/average_weight.main.cpp`
for the exact landed shape. Always read the generated `datalog.h` for
exact signatures before writing a driver.

## Key internals

- Data-flow IR: `lib/DataFlow/Query.h` (node classes: SELECT/TUPLE/JOIN/
  MERGE/CMP/MAP/NEGATE/AGG/KVINDEX/INSERT). Per-node `Canonicalize` methods +
  the driver in `lib/DataFlow/Optimize.cpp` (Simplify → Canonicalize fixpoint
  → CSE; `OptimizationContext` flags in `lib/DataFlow/Optimize.h`). Every
  optimization pass carries a doc comment: algorithm, pseudocode, ASCII
  before/after diagram.
- Control-flow IR: regions in `lib/ControlFlow/Program.h` (SERIES/PARALLEL/
  INDUCTION/LET/TUPLECMP/UPDATECOUNT/CHECKMEMBER/COMMITSWEEP/CLAIM/...);
  built by `lib/ControlFlow/Build/`, optimized by
  `lib/ControlFlow/Optimize.cpp` (per-region `OptimizeImpl` overloads, same
  doc-comment convention). Differential maintenance is per-stratum
  OVERDELETE → REDERIVE → INSERT with split per-row derivation counters
  (see `docs/proposals/StackSafeNegation.md`). Since the data-structures
  epoch: differential tables COMPACT dead rows at the emitted commit-
  sweep tail (trigger: dead ≥ live with a 4096-row floor, so suite-sized
  programs never fire it; ids renumber; the sweep rebuilds each index
  under its codegen-known key projection; monotone tables never compact
  — no deaths, and `sealed` is an id-order watermark); join/scan body
  membership gates read predicates on the scan cursor id (the emitter's
  row-binding scope stack — the value-keyed re-Find is gone).
- Delta-relational IR (`lib/DeltaRel/DeltaRel.{h,cpp}` + the T2b
  `Format.cpp` dump emitter — its own compiler-internal
  static-library target with no `include/drlojekyll/DeltaRel/`
  surface; the one public seam is the `-deltarel-out` sink
  `SetDeltaRelDumpStream`, DECLARED on ControlFlow's public
  Format.h and forwarded to the lib; renamed from lib/DR at the
  keyed-instances epoch open to mirror DataFlow/ControlFlow): a
  typed-value flow
  graph between Query and Program that is now
  the SOLE authority for the stratum machinery (the hand-coded scheduling
  fixpoint + DiscoverBranches path-DFS were deleted). Objects: typed DRVecs
  (queues/frontiers/pivots) with def/use edges; DROps carrying sign /
  position (InNew/InI read placement) / claim-context as ATTRIBUTES + the
  ten membership predicates (semantics from `Runtime/Table.h`, never a
  header comment — E-12) + per-arm effect sets + access-plan spines;
  DR strata DERIVED (`DeriveDRStrata`, a monotone integer lift) and an
  independent Kahn linearizer under a band-key tie-break
  (`LinearizeAndValidateDRFlow`), the schedule a CHECKED linearization of
  the dependence graph. Always-on graph validators (fprintf+abort, survive
  NDEBUG): V-XOVER-ONE/V-PROD-MONO/V-PROD-CLASS/V-JOIN-ONE (the promoted
  B-3 asserts), the census (V-ONE-FOLD/V-SEED-SUP/V-NEG-CTX/V-CLAIM-GATE/
  V-DEFER/V-RETIRE-AFTER), V-LINEAR/V-LOOP/V-READY/V-BAND-HAZARD, and
  V-PRED-XCHECK (ties the DR model to the surviving Emit* templates — a
  reintroduced F17/F18 divergence aborts on compile). Emission path:
  `LowerDRFlow` (acyclic seeds/crossovers/product arms/claim drains/
  frontier filters), `LowerDRRounds` (per-SCC×phase fixpoint round shells),
  `LowerCommitSweeps` (commit + Seal), `LowerGroupUpdate` (R3 aggregates),
  all in `Stratum.cpp`. Since the subgraphs/demand epoch, EVERY ingest
  fold lowers from the DR-IR: a deletion-capable receive's two explicit
  folds from the stage-1 `kIngestFold` pair (`MakeStageOneIngestFolds`,
  ADL/functor-surface P2 cutover) and a monotone table-bearing receive's
  fold from its monotone op (`MakeMonotoneIngestFold`, the §6 stage) —
  each the single payload authority, `LowerIngestFold` at the original
  walk position (id-stream identity), which RETURNS the UPDATECOUNT
  cursor whose EMPTY body the still-hand-coded eager descent
  (`BuildEagerInsertionRegions`, `Build.cpp`) fills (the hole contract;
  an always-on INGEST-CURSOR-SHAPE check guards the cursor, and
  V-INGEST-XCHECK Site 5 multiset-compares every emitted fold against
  the flow's kIngestFold enrollment — the eager web is in the
  cross-checked model). The descent itself (`BuildEagerRegion`ff) is
  the PRINCIPAL (not the only) remaining hand-coded emission surface — the
  table-less monotone receive also hand-mints a VECTORLOOP shim in
  `ExtendEagerProcedure` (`Procedure.cpp`) via no DR-IR op (E-42). Since
  the R1/R2/R3/R4 slices of the Rel epoch, the descent's TUPLE-forward,
  terminal-INSERT, CMP-filter, MAP functor-call, MERGE-union,
  SELECT-rebind and NEGATE-gate arms are MODELED: the six effect-free,
  knob-independent
  `kEagerForward`/`kEagerInsert`/`kEagerCompare`/`kEagerGenerate`/
  `kEagerUnion`/`kEagerSelect` marker ops plus the R4 EFFECT-BEARING
  `kNegateGate` (a mint RELOCATION: it carries a real kFlagRead of the
  negated view's model table, reconstructed identically at mint and
  re-invocation; every minted gate is eager-walk-reached — the
  walk-cut `CanReceiveDeletions` negates mint nothing), all minted at
  the dispatch site and lowered in place by `LowerRelStep_*` wrappers
  calling the untouched region builders (zero emission change; the
  walk is the reachability authority, enrollment tail-appends after
  the ingest folds so they keep op.0/op.1; render `table=` comes from
  the union-find MERGED model — a `.df class=table-less` merge is
  typically model-table-BACKED, E-107; the CMP operator / MAP functor
  carry NO stored payload — `cmp=`/`functor=` re-derive from the
  stored `eager_view` at render time; `IsEagerMarkerKind`
  (DeltaRel.cpp) is the ONE membership predicate for the marker family
  and EXCLUDES the gate — the gate keeps its own gate_* payload,
  table-less lead-0 key, and V-READY/render branches; the union mint
  fires ONLY on a merge that does not OWN an InductionGroupId — the
  owning-merge leg is Authority A round shells; the SELECT arm lowers
  via the extracted `BuildEagerSelectRegion`; @never negates render
  IMPLICIT via `reads: Present` vs `InI`, no token). Every program's
  `.deltarel` shows its eager markers; NINE `.deltarel` goldens pin
  the surface (`demand_tc_witness` + `symrec_tie_1` + `map_3` — the
  table-less-ingest carrier witnessing `cmp=`/`functor=` — plus the R3
  trio: `merge_2` (table-BACKED union markers), `booleans` (select with
  `table=`), `elim-cond-cycle-simple` (the induction-skip NEGATIVE
  guard — its induction-owned merge mints zero unions), plus the R4
  trio: `negate_1`, `negate_6` (the @never carrier), and
  `d5_recursive_negate` (the zero-mint NEGATIVE guard — its walk-cut
  recursive negate mints no gate); all opt-mode via their `.irgold`
  sidecars). The remaining unmodeled arms
  (JOIN + E-42) migrate one slice at a time
  (see KeyedInstances.artifacts/rel-arch-pseudocode.md §4-§5).
- Core invariants (dataflow): no view is ever its own direct user (asserted
  in `RelabelGroupIDs`); a source-less forwarding cycle is unsatisfiable,
  collected by dead-flow elimination; `QueryImpl` owns no conditions —
  zero-arity predicates desugar in `BuildClause` into unit relations (1 bool
  column, `is_condition`, sole possible row `(true)`) and every inter-view
  dependency is a column edge; canonicalization never severs the last
  input-column edge to an incoming view (keep-last-edge rule); a JOIN pivot
  whose non-user side is a unit relation is never removed, and CSE never
  folds a unit SELECT into a non-unit one; a unit relation contains at most
  the row `(true)` — only the desugarer creates its INSERTs, and they insert
  only the token; zero-pivot JOINs appear only under `@product`; a table's
  member-view list holds each view at most once, by IDENTITY — never dedup
  it structurally (distinct-but-equal views sharing a model are intentional,
  the group_ids CSE guard). group_ids/InsertSetsOverlap is a CORRECTNESS
  GUARD, never an optimization target (ratified at the ADL/functor-surface
  close: the "cubic self-join" premise is false — the self-join already
  lowers to one table + two hash indexes + a pivot loop, O(join output);
  group_ids live only inside one CSE() call, recomputed after every merge;
  see ADLFunctorSurface.artifacts/p3-tc-selfjoin-target.md).
- Core invariants (differential): every inductive back-edge fold is an
  `UPDATECOUNT` whose propagation body is dominated by its zero crossing
  (termination of generated fixpoints); differential rows carry split SIGNED
  counters (`C_nr`/`C_r`; presence = total > 0) that may dip below zero only
  mid-batch — the commit sweep asserts both ≥ 0 per class and publishes only
  `was != now`; generated code reads a differential table ONLY through the
  named membership predicates (`in-I`, `in-new`, the fixpoint-round forms,
  `recursively-supported`, `present`), placed by the seed/fixpoint delta
  schemas (seed: lower position `j < i` reads InNew, `j > i` reads InI;
  fixpoint rounds use the claim-relative matrix — StackSafeNegation.md
  §5.1); claim gates re-test at dequeue (`TryClaimDel`: C_nr ≤ 0,
  `TryClaimAdd`: total > 0 — F17); negate gates are CONTEXT-keyed, never
  sign-keyed (seed context: key absent in InI for BOTH signs; fixpoint
  refire: absent in InNew for both signs; `@never` gates on Present — F18);
  each non-@never negate has exactly ONE crossover arm-pair, folding into
  the negate's own table, emitted seed-before-drain; an ACYCLIC differential
  @product emits one signed frontier arm per side×sign (monotone sides have
  no `-` arm) — position-keyed sign-INDEPENDENT non-delta reads (`j < i`
  InNew, `j > i` InI), one fold into the product's own table, every arm
  seed-before-drain (the claim gates' phantom drop depends on that order);
  on-cycle differential products are rejected by exact self-reachability
  (`ViewSelfReachable`, NOT `InductionGroupId` — a fully interior join loses
  its group id, F22); explicit message batches net with SET semantics — each
  side deduplicated, adds∩removes annihilates, leaving presence exactly what
  the rest of the program proves (OQ3).
- Union sinking (`do_sink` in `QueryImpl::Optimize`) is commented out —
  `lib/DataFlow/Merge.cpp` sinking code is currently unreachable.

## Aggregates + KV indices (the R3 delta-relational-IR family — LANDED)

Aggregates (`over(){}`) and KV indices (`mutable(...)` params) LOWER
end-to-end as of the R3 stage-C flip (delta-relational-IR epoch;
`docs/proposals/DeltaRelationalIR.md` §12 stage-C record; binding spec
`docs/proposals/DeltaRelationalIR.artifacts/v3-spec-statecell.md`). A KV
index is the degenerate aggregate: both lower to ONE `GROUP_UPDATE` op per
view (`BuildGroupUpdateOps`, DR.cpp), whose standing per-group state is a
`StateCellStore` (`include/drlojekyll/Runtime/StateCell.h`, a peer of the
agg DiffTable, own dense-group-id space, two-word sealed/working cell +
occupancy bit). Band (a) folds the summarized INPUT table's net-frontier
rows into the cell (`SC.Fold(gid, sign, summary)`); band (b) `emit_touched`
applies the occupancy-generalized ONE-NET-PAIR guard (birth: +new only;
death: −old only; change: −old,+new; else nothing) into the agg table's own
counters + del/add queues, riding the existing acyclic claim/frontier/commit
tail. The algebra attribute (`@invertible` fold/unfold O(1) | `@recompute`
per-group rescan) is a lowering selector. The aggregate/KV view is a
BRANCH CHAIN-BREAKER (`SuffixesOf`/`CollectSectionTargetsDR`, DR.cpp; the
eager walk stops at it, Build.cpp) — no branch traverses one; its monotone
message input is provisioned a net-additions frontier as a cut successor.
REDUCTION BODIES are C-5 driver-supplied FREE FUNCTIONS (forward-declared in
the header, defined out-of-line, NAMED AFTER THE FUNCTOR): for a functor `f`,
`f_identity()/f_combine(w, v)/f_uncombine(w, v)` for `@invertible`,
`f_reduce(const S *values, const int32_t *counts, size_t n)` for
`@recompute` (e.g. `sum_i32_combine`, `new_weight_i32_reduce`). Corpus +
oracle: `tests/OptDiff/cases/{average_weight, pairwise_average_weight,
aggregate_1}` (drivers + `.batches` + oracle/monotone goldens; the oracle
`bin/Oracle/Main.cpp` does the definitional per-group recompute in both
paths). `data/examples/average_weight.dr` + `pairwise_average_weight.dr`
compile.

Config-column aggregates lower on BOTH algebra arms as of the demand-seeds
epoch: the `@invertible` config arm landed in the subgraphs/demand epoch
(`config_agg_1`, config-dependent reduction, free functions gain a leading
config parameter), and the `@recompute` config arm landed in demand-seeds
(`config_agg_2`, fork (i): a codegen-emitted per-touched-group seal loop via
the store's `SealOne(gid, cfg...)`; `Old(gid)` stays config-free) — the P2c
residual fence is gone.

CLEAN-DIAGNOSTIC gaps that remain: a `mutable()` merge functor with NO
declared `@`-algebra (V-ALGEBRA reject — must be `@invertible`/`@recompute`);
aggregates/KV over INDUCTION-OWNED
(recursively-derived) inputs; unstratified aggregation (an aggregate over
its OWN recursive result, rejected by the dataflow Stratify pass as the
sibling of the unstratified-negation reject — `agg_in_scc_1`/`kv_in_scc_1`).

## The demand transform (`-demand`, magic-sets — LANDED, single-adornment slice)

`-demand` (Main.cpp `gDemand` → `Query::Build(..., demand_mode)`) is a live
magic-sets / SLDMagic rewrite of the Query graph for bound `#query`s: a SIP
walk propagates the query's bound columns backward, mints a demand relation
per reached predicate, and push-down-joins a guard (`d_p ⋈ p`) so a demanded
subgoal materializes only the rows a demanded answer needs. The pass lives in
`lib/DataFlow/Demand.cpp` (`QueryImpl::ApplyDemandTransform`, run at the
post-`ConnectInsertsToSelects` slot in `Build.cpp`); the fabrication half is
`ParsedModule::FabricateDemandMessage`/`FabricateDemandLocal`
(`lib/Parse/Demand.cpp` — real `ParsedMessageImpl`/`ParsedLocalImpl` minted at
DataFlow-build time under the reserved lowercase `demand__` prefix, a user
collision is a clean-diagnostic reject; the fabricated message's public ABI
entry is suppressed via a registry at the `kMessageHandler` codegen sites so
no driver-callable demand seam leaks). The demand seed is injected by a forcer
proc built from a `QueryDemandForcing` registry (BindingPattern-keyed).

It is MODE-GATED OFF: `demand_mode == false` returns at the pass head before
minting anything, so the flag is ORTHOGONAL to the 4 golden optimization modes
(never a 5th mode) and the 166 pre-demand corpus cases are byte-identical
flag-off. Per-case activation is the `<name>.drflags` sidecar (runall.sh /
diffrun.sh append its contents to the compiler line): `demand_tc_witness` is
the demand-ON witness, `demand_multi_adorn_1` the >1-adornment reject. The
slice is single-adornment: >1 bound query, >1 binding pattern per name,
NEGATE/AGG in a demanded body, left-linear propagation, stray consumers, and
multi-clause queries are all clean diagnostics (never miscompiles). ~23% of
corpus cases carry bound queries (38/165 at the demand-seeds seed sweep; the
3 new cases added more) — an unconditional transform would rewrite ~a quarter
of the goldens, so mode-gating is mandatory.

## The keyed-instance nested lowering (`-demand-instance` — LANDED, birth-and-rebuild)

`-demand-instance` (Main.cpp `gDemandInstance`; implies `-demand`; OFF the
PassPolicy registry — a lowering selector, not a pass) lowers a recognized
demanded subgraph to a keyed InstanceStore instead of the flat guard web (the
D2.b nested lowering). It is ANSWER-IDENTICAL to flat `-demand`: the
`demand_neighborhood_witness` case is the two-lowerings equivalence witness —
its `.eqgate` sidecar drives run_eqgate (runall.sh --one), which re-compiles the
nested arm (`.drflags` + `-demand-instance`) with the SAME driver in all four
optimization modes and byte-compares each mode's stdout against
`goldens/demand_neighborhood_witness.stdout` LIVE (no nested golden is blessed;
flat==nested==golden falls out transitively from the flat diffrun check). The
witness graph carries out-of-neighborhood edges and the driver asserts each
probe's answer is EXACTLY neighborhood(Start) — an over-materialized nested arm
both aborts and diverges (HP-5).

The witness is BIRTH-AND-REBUILD (R-a2, the first DeltaRel->Rel deliverable):
the birth phase lands all edges before its probes, then a REBUILD phase adds
edges AFTER their key's demand is standing and re-probes. EDGE-AFTER-DEMAND —
adding a monotone input edge while a demand is already standing — now REBUILDS
the standing instance via band-(a2) (a full edge-frontier rescan keyed on the
edge net-additions frontier); the birth-only enforcement (RAT-6) is lifted and
the labeled feature gap is CLOSED. Three all-4-modes
compile fences: recursive demand (`demand_cyclic_1`) and a @differential
summarized input (`demand_diff_input_1`) reject at the Program::Build nested
pre-pass (Build.cpp:1336-1346) only under `-demand-instance` (both compile
under plain `-demand`); a recursive-content demanded body
(`demand_recursive_content_1`) is caught UPSTREAM by the plain-`-demand`
body-walk (its `.drflags` is a bare `-demand`; it pins the shadowed Build.cpp
recursive-content belt).

## Other known feature gaps (clean diagnostics)

Cross-products over differential (deletable) data INSIDE RECURSIVE CYCLES
(the acyclic case landed as Stage 5 of `StackSafeNegation.plan.md`; the
fence is `ViewSelfReachable` in `Program::Build`'s pre-pass); impure
functors (control-flow build); and unstratified negation — a negated
predicate recursively derived from the negation's own result (rejected by
the dataflow Stratify pass in all modes). Corpus file:
`data/self_testing_examples/evm_func_parse.dr` (unstratified negation).
Every other file under `data/` — including `conditions_to_bools.dr`, the
acyclic differential @product example, and the two aggregate/KV examples —
compiles in all 4 modes.

## Gotchas

- macOS ships bash 3.2: no `declare -A` in scripts. zsh does not word-split
  unquoted variables — use `${=var}` when a variable holds multiple CLI args.
- Debug builds round-trip the parser and re-assert; crashes usually surface
  as `Assertion failed` + SIGABRT (exit 134), stack overflow/null deref as
  exit 139. `lldb -b -s <script-file> -- <cmd>` gets backtraces reliably;
  `-o run -o bt` sometimes truncates.
- clangd diagnostics in this repo are noise (it lacks include paths); trust
  the real build only.
- COPYRIGHT: this is not a Trail of Bits project anymore. NEW files carry
  ONLY `// Copyright <year>, Peter Goodman. All rights reserved.` (comment
  leader per file type) — never copy the Trail of Bits header idiom from
  neighboring files. ToB-era files keep their historical ToB line SECOND,
  beneath the Peter Goodman line (the 2026 project-wide sweep). vendor/ is
  third-party: never touch its notices.
