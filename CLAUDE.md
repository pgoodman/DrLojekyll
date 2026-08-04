# Dr. Lojekyll

Datalog compiler ("hyde" C++ namespace). Compiles Datalog to an incremental,
message-driven C++ database. Pipeline:

    parse (lib/Lex, lib/Parse)
      → data-flow IR        lib/DataFlow    Query::Build(module, log, optimize)
      → frozen regional     lib/Regional    FrozenRegionalProgram::Build(query, log)
      → control-flow IR     lib/ControlFlow Program::Build(frozen, first_id, optimize)
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
cd build/debug && ctest --output-on-failure   # DataFlowValidators, RelValidators,
                                              # IdentityTypes, InstanceStore,
                                              # MiniDisassembler, PointsTo, Runtime
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
(`<name>.dr` + `<name>.main.cpp`, 199 corner-case programs as of the
DIFF-R3 R3a landing — symrec_tie_1 is the standing
determinism witness; agg_distinct_1 pins the aggregate multiplicity
semantics + carries a `.contract` golden; barrier_neck_1
witnesses the `:-` separator, sugar for `@barrier` between every two
body conjuncts, its df.opt golden pinning the staged-binary-vs-3-way
join contrast and the CSE merge of the `:-`/explicit-@barrier twins) has one committed expected output in
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
  `negate_never_diff_1` (@never over a differential negated view — the
  DS-R4-10 post-fixpoint fence, F25),
  `nonascii_1`, `truncated_decl_1`, `demand_multi_adorn_1` (a `-demand` query
  name carrying >1 binding pattern — the demand pass's clean per-name reject,
  via its `.drflags` sidecar), `demand_cyclic_1` (a `-demand-instance`
  nested-lowering feature-gap fence — recursive demand; it COMPILES under plain
  `-demand` and rejects only under `-demand-instance`) and
  `demand_recursive_content_1` (a
  recursive-content demanded body — rejected UPSTREAM by the plain-`-demand`
  body-walk, so its `.drflags` is a bare `-demand`; it pins the shadowed
  Build.cpp recursive-content belt), `product_in_scc_diff_1` (on-cycle
  differential @product, the F23 shape — the ViewSelfReachable fence),
  `demand_agg_body_1`/`demand_kv_body_1`/`demand_config_agg_body_1` (an
  over(){} aggregate / KV merge / config-@recompute aggregate inside a
  demanded body — demand-sink/R-MAT rejects under their `-demand` .drflags;
  each carries `.batches` + oracle/monotone goldens pinning the definitional
  answer BEFORE any Stage-C reject lift), `demand_mutual_content_1` (mutual
  recursion inside a demanded body, R-BODYWALK), `demand_two_queries_1`
  (two independent bound query names, R-1BOUND — the Stage-C lift
  candidate), and the SEVEN R3a region-key rejects (DIFF-R3, session 5):
  `region_key_wildcard_1`/`region_key_anon_1`/`region_key_dup_1`/
  `region_key_unknown_1` (parse/resolve obligations, `-demand`-INDEPENDENT
  — no .drflags), `region_key_mismatch_1` (V-DECLARED-KEY: declared set ≠
  SIP-inferred `p_bound`; a STABLE hard reject under the RP-3
  unprovable-brackets-reject ratification), `region_key_multi_adorn_1`
  (ADJ-R3-A strict single-forcing scope) and `region_declared_fenced_1`
  (a MATCHING bracket + NEGATE body draws the FENCE class, never
  V-DECLARED-KEY — O-R3.4; the last three under `-demand` .drflags,
  flag-off the bracket is inert); `kvindex_1` is MODE-SPLIT (compiles
  under opt/nocf where KVINDEX→TUPLE elimination fires, V-ALGEBRA-rejects
  under nodf/none). `aggregate_1` FLIPPED from diagnostic to a 4-mode
  golden at the R3 stage-C flip. The @differential-summarized-input fence was
  LIFTED at D3.a.2: `demand_diff_input_1` flipped diagnostic→golden and is now
  the diff-input × diff-demand composition eqgate witness, and the NEW
  `demand_diff_neighborhood_witness` is the e5 (diff-input × MONO-demand)
  carrier — the first P-STORE∧¬P-DEATH program (`kInstanceDeath=0` beside
  `kSubgraphInstantiate=1`).
- I0 referee (the reference relational interpreter, RegionalDataFlowCore
  stage-i0 — LANDED): any `.batches` case ALSO runs `run_refinterp`.
  `bin/RefInterp` (`drlojekyll-refinterp <dr> <batches> [probes]`) is a
  DEFINITIONAL evaluator over raw parsed clauses (OG1-parsed: links no
  DataFlow, never reads `.drflags` — demand-blind by construction) emitting
  the Canonical Behavioral Format (per-epoch sorted published-membership
  deltas + FINAL + probe-restricted QUERY blocks via the `.probes` sidecar);
  `bin/RefHarness` emits a `behavioral_main.cpp` against the frozen public
  ABI (OG2-tool), and the behavioral binary — compiled from the PLAIN
  program, never `.drflags` (demand-gated publish taps legitimately diverge
  demand-lowered) — must byte-agree across all 4 modes AND match the frozen
  `goldens/<name>.behavioral.stdout` AND match the interpreter's CBF.
  Diagnostic `.batches` cases run interp-only. A REFINTERP-DISAGREE is
  adjudicated per the stage doc §3 (finding, never fudge; F29 was found+fixed
  this way).
- REJECT corpus (`tests/OptDiff/rejects/*.dr`, adopted 2026-08-04 from the
  ToB `parse_errors` branch's `data/invalid_syntax_examples` + expanded for
  the modern surface): driverless, goldenless should-FAIL cases run by
  runall.sh after the main sweep — each must exit 1 CLEANLY in both mode
  extremes (rc=0 = a LOST CHECK, >=124 = a CRASH finding; either fails the
  suite). Optional per-case `.drflags` (the demand__ reserved-prefix pair
  runs under `-demand`). Diagnostic TEXT is not pinned — the expected class
  lives in each case's header comment. Expanding this corpus found+fixed
  F30 (the kind-scoped demand__ collision scan) on day one.
- Blessing: goldens change ONLY via explicit
  `runall.sh --bless <workroot> [filter]` after reviewing a run's outputs —
  never automatically on failure, and never to make a red case green.

`tests/OptDiff/FINDINGS.md` is the ledger of bugs found this way, with
repros (F1–F19, F21, and F26–F30 fixed as of August 2026; F23 promoted to
the `product_in_scc_diff_1` pin; F20 is the sole open record-only note —
the latent comparator. F29, the commit-sweep used-state collector omitting
the swept table's live indexes, was promoted and fixed the same day the I0
sweep fired its named trigger).

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
  before/after diagram. DEAD-FLOW SPLIT (F26): the `df.dfe` PassPolicy gate
  picks WHICH dead-flow pass runs, never whether one runs —
  `EliminateDeadFlows` (full taint-based optimization) when on,
  `CollectDeadCycles` (source-less-cycle collection, REQUIRED hygiene like
  `RemoveUnusedViews`) when off; canonicalization demolishes the merge/
  io-seam structure of user-authored dead cycles and the collection is the
  janitor. Stratify's multi-view-SCC invariant is the always-on V-SCC-SEAM
  validator (fprintf+abort, survives NDEBUG).
- Stage-A identity/contract layer (RegionalDataFlowCore epoch):
  `lib/DataFlow/Identity.h` (typed id domains + static_assert battery;
  ctest IdentityTypes); `QueryTupleImpl::ProjectionRole` (kMember default,
  kDistinct at exactly the two clause-head mint sites), build-stamped and
  immutable, folded into `Equals` ONLY — never `Hash` (the Hash fold
  perturbs hash-derived Rel/CF tie-breaks for zero CSE benefit), with the
  V-PROJ-ROLE-STABLE belt at the single CSE merge choke point;
  `InferConservativeRowContracts` (`lib/DataFlow/RowContract.{h,cpp}`), a
  PURE two-phase graph function at the Query::Build tail (post-Stratify):
  Phase-1 AllFields member keys on multi-view strata, Phase-2 acyclic
  flat-key `{visible_fields, member_key}` transfer; validators
  V-CONTRACT-CENSUS / V-MEMBERKEY-REALIZED / V-AGG-INPUT-KEY always-on,
  V-NO-COLLAPSE belt-only. `-contract-out` dumps the contracts (3 blessed
  `.contract.opt.golden`s via `.irgold` sidecars: agg_distinct_1,
  demand_tc_witness, join_1; `.df` stays byte-untouched). The dataflow
  `-dot-out` DOT twin renders `role=`/`KEY(...)` annotations and groups
  multi-view strata as `subgraph cluster_stratum_<id>` (advisory
  visualization, never byte-goldened).
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
- Delta-relational IR (`lib/Rel/Rel.{h,cpp}` + the T2b
  `Format.cpp` dump emitter — its own compiler-internal
  static-library target with no `include/drlojekyll/DeltaRel/`
  surface; the one public seam is the `-rel-out` sink
  `SetRelDumpStream`, DECLARED on ControlFlow's public
  Format.h and forwarded to the lib; renamed lib/DR->lib/DeltaRel at the keyed-instances epoch open, then
  lib/DeltaRel->lib/Rel + `-rel-out`->`-rel-out` + `.rel`->`.rel`
  at the R-final rename ritual — NARROW scope, DR* identifiers deliberately
  retained; the in-dump header token is `rel` since the OD-14 mini-diff
  — ledger §20(AC)): a
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
  now the ONLY remaining hand-coded emission surface — since the R-E42
  slice the table-less monotone receive's VECTORLOOP shim in
  `ExtendEagerProcedure` (`Procedure.cpp`) is ALSO modeled: the
  ingest-family `kIngestLoop` op (id-neutral effect-free ctor,
  tail-append enrollment via its own IOs-x-Receives re-derivation, a
  per-RECEIVE count law + its own Site-5 sibling multiset over
  `Context::emitted_ingest_loops`) lowers via the sibling
  `LowerIngestLoop` — the byte-move of the old shim (VECTORLOOP + one
  VAR per receive column at the original walk position), returning the
  loop as the descent cursor under its own always-on INGEST-LOOP
  cursor-shape guard; render is marker-shape (`message=<name>/<arity>`,
  no `table=`) but the kind is NEVER an eager marker (stays out of
  `IsEagerMarkerKind`/EAGER_WEB). Since
  the R1/R2/R3/R4/R-JOIN slices of the Rel epoch, the descent's
  TUPLE-forward, terminal-INSERT, CMP-filter, MAP functor-call,
  MERGE-union, SELECT-rebind, NEGATE-gate, pivot-JOIN and @product
  dispatch arms are MODELED: the eight effect-free, knob-independent
  `kEagerForward`/`kEagerInsert`/`kEagerCompare`/`kEagerGenerate`/
  `kEagerUnion`/`kEagerSelect`/`kEagerJoin`/`kEagerProduct` marker ops
  (the JOIN/PRODUCT pair are PER-VISIT dispatch-edge records; the
  once-per-join TABLEJOIN emission is ALSO modeled since R-final slice
  3 — the `kJoinEmit`/`kProductEmit` EMISSION ops, one per (proc,
  join_view, form∈{eager,delta}) event, payload carrying the
  ContinueJoinOrder drain key + the WorkItem `work_seq` tie-break,
  recorded at work-item creation, delta-enrolled after DeriveDRStrata,
  cross-checked by the V-JOIN-EMIT-XCHECK Site-5 multiset (run on BOTH
  BuildStratumPhases exits), lowered by `LowerJoinEmit` wrapping the
  untouched `BuildJoin` at both callers — emission byte-identical,
  TABLEINDEX excluded from the id contract (CARVE-3), render surfaces
  `form=`/`order=`/`seq=` (order/seq eager-only); a join marker's
  `table=` is usually absent but a model-SHARED join renders it, the
  E-107 shape) plus the R4 EFFECT-BEARING
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
  `.rel` dump shows its eager markers; ELEVEN `.rel` goldens pin
  the surface (`demand_tc_witness` + `symrec_tie_1` + `map_3` — the
  table-less-ingest carrier witnessing `cmp=`/`functor=` — plus the R3
  trio: `merge_2` (table-BACKED union markers), `booleans` (select with
  `table=`), `elim-cond-cycle-simple` (the induction-skip NEGATIVE
  guard — its induction-owned merge mints zero unions), plus the R4
  trio: `negate_1`, `negate_6` (the @never carrier), and
  `d5_recursive_negate` (the zero-mint NEGATIVE guard — its walk-cut
  recursive negate mints no gate AND no join marker: its recursive
  differential join lives in Authority A, `kPivotAssemble=1`), plus
  the R-JOIN pair: `join_1` (the acyclic pivot-join carrier,
  `kEagerJoin=4` over 2 table-less join views) and `optimize_2` (the
  first @product carrier, `kEagerProduct=2`); all opt-mode via their
  `.irgold` sidecars; census 29 kinds since R-final slice 3
  (kJoinEmit/kProductEmit) — every pin's census line carries
  `kIngestLoop=` and `kJoinEmit=`, the kIngestFold=0 quad map_3/merge_2/
  elim-cond-cycle-simple/join_1 carries real kIngestLoop blocks, and
  the 7 join/product carriers carry real kJoinEmit/kProductEmit blocks
  incl. d5's DELTA join-emit). NO unmodeled arm remains; the R-final
  direction flip + rename remain (see
  KeyedInstances.artifacts/rel-arch-pseudocode.md §4-§5 +
  rfinal-design.md).
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
  only the token; zero-pivot JOINs appear only under `@product`; every
  MONOTONE table-less stream INSERT gets its OWN dedup table in
  `FillDataModel` so publishes are gated on the presence crossing (F27 —
  1-arm-MERGE elimination can detach a published tap from its producing
  table, and an ungated eager publish re-emits present rows; deliberately
  NOT model-shared with a cycle insert, which would consume the crossing);
  a table's
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
MULTIPLICITY SEMANTICS (normative; verified empirically 2026-07-27, both
algebras × all 4 modes): an aggregate folds over the DISTINCT tuples of
its `over(...)` projection — projected COLUMNS, not projected rows. The
over column list IS the subquery's projection; set semantics collapse
colliding tuples before band (a) sees them (folds fire on presence
transitions, so the @recompute counts[] multiset holds only 0/1 today —
each input-table row maps injectively to a (group, value) slot). Counting
ROWS requires carrying a key of the summarized relation in the over list;
aggregating a constant (the count(*) idiom) yields 1 per group; a
wildcard/dropped body column silently dedups. Aggregate canonicalization
drops only constant and duplicate group-by columns (identity-preserving)
and never drops aggregated/config columns, so a fully named over list is
counted as written in every mode. The former parse-layer advisory lint
(LintAggregateProjection) was DELETED at Stage A — the trap shape is now
covered by the contract layer (V-AGG-INPUT-KEY); agg_distinct_1 is the
corpus witness (zero warnings, stdout untouched, plus its `.contract`
golden). User-facing statement: docs/Language.md
(Aggregation bullet); data/examples/average_weight.dr + its corpus twin
compute sum/count over DISTINCT (X, Weight) pairs (edge ids projected
away), documented in-file.
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
multi-clause queries are all clean diagnostics (never miscompiles). ~27% of
corpus cases carry bound queries (49/181 measured at the RegionalDataFlowCore
adjudication; re-measure, never propagate the constant) — an unconditional
transform would rewrite ~a quarter of the goldens, so mode-gating is
mandatory.

## The keyed-instance nested lowering (`-demand-instance` — LANDED, birth-and-rebuild)

`-demand-instance` (Main.cpp `gDemandInstance`; implies `-demand`; OFF the
PassPolicy registry — a lowering selector, not a pass) lowers a recognized
demanded subgraph to a keyed InstanceStore instead of the flat guard web (the
D2.b nested lowering). It is ANSWER-IDENTICAL to flat `-demand`: the
`demand_neighborhood_witness` case is the two-lowerings equivalence witness
(since D3.a.1 it runs `-demand -demand-retract` and exercises the DIFFERENTIAL
regime — retract/death/rebirth phases + the `@differential` `nbhd_out` tap
upgrading the eqgate to answer + sorted published-delta identity; its
pre-retract twin `demand_neighborhood_mono_witness`, bare `-demand`, keeps the
R-MONO nested lowering end-to-end covered — the Fable-review [A] coverage
catch) —
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
the labeled feature gap is CLOSED. Two all-4-modes
compile fences: recursive demand (`demand_cyclic_1`) rejects at the
Program::Build nested pre-pass only under `-demand-instance` (it compiles under
plain `-demand`); a recursive-content demanded body
(`demand_recursive_content_1`) is caught UPSTREAM by the plain-`-demand`
body-walk (its `.drflags` is a bare `-demand`; it pins the shadowed Build.cpp
recursive-content belt).

D3.a.2 (differential input — LANDED) lifts the fence (iii) `diff_input` arm so a
`@differential` (deletion-capable) SUMMARIZED INPUT is admitted under
`-demand-instance`. A live-demanded key whose input net-REMOVALS arrive rebuilds
its standing instance via the band-(a2') removal-drain arm (TWO DRAINS, NO
RECYCLE — R-A2-TRIGGER), a gate-set clone of band-(a2) draining the input
net-removals frontier; the ONE Present-filtered rescan mold gains an
`input.Present(s)` conjunct on all three sources (a1 birth, a2 edge-adds, a2'
edge-removals) so a physically-present-but-dead input row is skipped. This is the
first regime where P-STORE (`TableIsDifferential(pub)`) and P-DEATH
(`TableIsDifferential(demand)`) DIVERGE by design: the e5 carrier
`demand_diff_neighborhood_witness` (diff-input × MONO-demand, bare `-demand`) has
`kInstanceDeath=0` beside `kSubgraphInstantiate=1` — its demand-liveness gate
probes a MONOTONE demand member (soundness by IRREVOCABILITY, not quiescence);
`demand_diff_input_1` (diff-input × diff-demand, `-demand -demand-retract`) is
the full composition carrier. Both are eqgate carriers (flat==nested==golden +
sorted published-delta identity through the `@differential` tap), lifting the
eqgate family to four.

D3.a.3 (multi-adornment — LANDED) lifts the per-name single-binding-pattern reject
so ONE query name may carry N declared adornments, lowering to N DISJOINT keyed
stores over ONE shared pub (OD-15). The demand pass (`ApplyDemandTransform`) is a
TWO-PHASE per-adornment loop (Phase 1 locate/check over `UniqueRedeclarations()`
with a `seen_variants` dedup, Step 4 stray-consumer union ONCE between the loops,
Phase 2 mint per adornment) and the guard rewires are DEFERRED and grouped by
`(consumer, read)`: a SINGLETON group rewires directly (byte-identical for the
single-adornment corpus), a MULTI-guard group mints a MERGE UNION of the guards'
restored read-schema outputs (R-DUP) — the flat-arm realization of the
reference-counted-union pub. The g1 kBody-survivor policy (`PromoteSurvivorToBody`,
View.cpp) protects the fold arm; V-INST-SOLE is re-keyed on `(pub_table,
forcing_index)` (`CheckInstanceSolePub`, O1) so N forcings share one pub; and
`ProxyMergedViews` (Link.cpp) PRESERVES a guard annotation on its JOIN when the
R-DUP union's identity restore collapses the guard into a direct MERGE member (the
recognition + cut-successor detection key on the annotated JOIN). The new witness
`demand_multi_adorn_witness` (`q(bound A,free B)` + `q(free A,bound B)` over a
non-recursive `rel(A,B):edge_2(A,B)`; `.drflags` bare `-demand`, `.eqgate`
`-demand -demand-instance`) is the mono flagship — the first program minting
`kSubgraphInstantiate=2` (two stores, one pub), lifting the eqgate family to five.
`demand_multi_adorn_1` STAYS diagnostic (its reject MOVES from the per-name
`:457` to the per-adornment left-linear `:717-722`); `demand_multi_adorn_allfree_1`
is the new all-4-modes-diagnostic pinning the all-free-sibling fence (a query name
carrying a bound AND an all-free adornment rejects — the all-free cursor would read
the demand-guarded pub and under-answer). The eqgate family is SIX as of
`demand_diff_pub_1` — re-derive as `ls tests/OptDiff/cases/*.eqgate | wc -l`,
never propagate a narrative constant.

## The frozen regional layer (Stage B, RegionalDataFlowCore — LANDED)

`lib/Regional` (acyclic peer: bin → {ControlFlow,Rel} → Regional → DataFlow):
`FrozenRegionalProgram::Build(query, log)` runs at the Main.cpp third slot
(after Query::Build, before Program::Build) — a degenerate no-extraction
planner (ONE ProgramRoot + ONE observation-root region R0, zero child calls,
NO re-optimize) deriving request-ports (one per demand forcing), input/result
ports (real messages only; the fabricated `demand__` messages render
region-internal, ADJ-2), permanent roots (unforced queries, ADJ-3), and
row-contracts (R-STORE NARROWED: one per distinct non-demand relation-INSERT
declaration, member-key positional from the Stage-A contracts; since the
session-5 TIER-1 NAMING LIFT the demand-INTERIOR relations are nameable
too — see the DIFF-R3 section below).
H4: `Program::Build(const FrozenRegionalProgram&, ...)`,
first statement `query = frozen.Query()` — the byte-preserving thin seam.
Referees: V-FROZEN-NO-OPEN-PORT / V-OWNERSHIP-ACYCLIC at freeze + the
ALWAYS-ON V-REGION-CENSUS recount at the ValidateDROps tail (stored census ==
`DeriveRegionalCensus(query)` re-derived fresh — a stubbed planner aborts,
corpus-wide). Dumps: `-region-out` (G1, ratified D2.2) + `-region-dot-out`
(advisory DOT twin, never goldened); 16 `.region.<mode>` goldens via the
`region` `.irgold` surface (demand_tc_witness, join_1, merge_2,
demand_multi_adorn_witness × 4 modes — byte-identical cross-mode at tip but
pinned PER-MODE; cross-mode identity is not claimed). Authority docs:
stage-b-diff.md AMENDMENTS, regional-arch-pseudocode.md Part B,
regional-dump-stage-b-desired-states.md §9/§9.7, stage-b-landed-seed.md.

## DIFF-R3: Tier-1 interior naming + the declared-region-key bracket (R3a — LANDED, session 5)

RATIFIED POLICY (2026-08-03): HINT-NOT-MANDATE — the bracket NEVER drives or
constrains the lowering (R3b declared-driven lowering is DEAD, not deferred);
UNPROVABLE BRACKETS REJECT (a mismatch is a stable hard compile error, never
warn-and-accept); the Stage-B `Minimize`/`DeterminedBy` functional-key proof
is the future provability WIDENING (O-R3.5). Authority: region-model-diffs.md
"DIFF-R3 AMENDMENTS (session 5)" + its panel record, adjudicated resolutions
(RES-1..6) and implementation findings; pseudocode = regional-arch-
pseudocode.md Part R3; desired bytes = regional-dump-stage-b-desired-states
§10.

TIER-1 NAMING LIFT (hunk 1): the demanded interior relation (merge-
materialized, no INSERT — formerly unnameable) surfaces as a `-region-out`
row-contract. `ConnectInsertsToSelects` records `insert_proxy →
rel->declaration` in a `Query::Build`-SCOPED map (never a QueryImpl member —
the VIEW* keys dangle past Optimize), read once by `ApplyDemandTransform`
(T1-DECL-MISS aborts on a miss) into `RecognizedSubgraph::demanded_decl`
(parse identity, Optimize-stable). At freeze, interior-contract EXISTENCE +
census COUNT are decl-driven and resolve-free (distinct `demanded_decl` Ids,
forcing order, deduped against insert-derived decls — multi-adornment
surfaces ONE interior contract); member-key renders the decl's AllFields
positionally; `support=` is the OR over the forcing's live annotated guard
JOINs of `CanReceiveDeletions()` — ROLE-BLIND (T1-IMPL-1:
`PromoteSurvivorToBody` folds a projection guard to kBody under CSE, so a
role-filtered resolve aborts on real corpus cases); zero live guard JOINs
for a counted decl ABORTS the freeze (a resolve failure can never silently
drop a contract line). 8 `.region` goldens re-blessed (demand_tc gains
`E1 rel=path`, multi_adorn gains `E1 rel=rel` + an E0 re-pad from the
emitter's max-over-contracts column widths; census `row-contracts` 1→2).
Tier 1 is NOT a step toward Tier 2 (origin decl-sets on models — the
general mechanism, its own future slice).

R3a BRACKET SURFACE: `rel[K...](...)` on `#local`/`#export` only. Lexemes
`kPuncOpenBracket`/`kPuncCloseBracket` + two Lexer.cpp arms (NO spelling
table exists — rendering rides `Token::SpellingRange`); `ParseLocalExport`
state 20 (named + duplicate-free vars; empty bracket, trailing comma,
wildcard/anonymous, unexpected token all reject; EOF rides the `state != 9`
truncation gate); unknown-column resolve reject at the accept path; storage
`ParsedDeclarationImpl::region_key_param_indices` (WRITTEN order — parse-
layer capture only; the SET is the logical key, the order an inert DIFF-R5
arrangement hint) behind `ParsedDeclaration::HasRegionKey()/RegionKey()`;
the decl formatter prints the bracket (parser round-trip). CHECKING (Step
2b, V-DECLARED-KEY) sits POST-Loop-1 in `ApplyDemandTransform` (the first
site the adornment count exists; fences run first, so a bracket never masks
a fence): strict single-forcing scope, then declared-set == inferred
`p_bound`, via a dedicated decl-anchored reject (NO "recompile without
-demand" suffix). Flag-off the bracket is INERT (whole corpus byte-
identical). `-contract-out` gains the bracket-scoped `declared-region-key
rel=... declared=(...) inferred=(...)` line. `region_declared_tc_witness`
is the no-op-overlay witness: its stdout/oracle/monotone/df/rel/ir/h/
region×4 goldens are SYMLINKS to demand_tc_witness's (byte-identity IS the
referee; NEVER bless its symlinked surfaces directly — bless writes THROUGH
to demand_tc's files), contract.opt + behavioral are its OWN real goldens
(the CBF header embeds the case name — a symlinked behavioral golden can
never match). NO eqgate: recursive demand rejects under `-demand-instance`
(the demand_cyclic_1 fence), so the original eqgate spec was impossible;
a declared×nested composition witness needs a non-recursive base
(follow-on). The key-SUBSET covering-array fuzz arm (ORDER dropped as
inert) is the ranked-next follow-on. `region_key_dead_relation_1` pins the
bracket-on-dead-local no-crash path.

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
