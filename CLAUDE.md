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

> **P1 GREENFIELD CUT (2026-08-08).** The `-demand` magic-sets transform, the
> `-demand-instance` keyed-instance nested lowering, and the flagless `@key`
> demand-activation were DELETED wholesale (the compiler is not in use — a
> greenfield delete-then-rebuild; motivation in
> `docs/proposals/RegionalDataFlowCore.artifacts/session-16-whole-program-seed.md`
> §3-P1 + memory `greenfield-rewrite-motivation`). Post-cut, `@key` is INERT
> parsed metadata (parse-surface rejects still fire; no semantic/activation
> effect) and a bound `#query` reads the canonical fully-materialized relation
> via the plain cursor. `Query::Build` is `(module, log, policy)` again; the
> `-demand*` flags are gone. Keyed/residual evaluation is NON-FUNCTIONAL until
> the typed regional path is rebuilt (phases P2–P9). The three sections below
> tagged **[REMOVED at the P1 cut — historical]** describe deleted machinery;
> read them only for the pre-cut history.
>
> **S1a LANDED (2026-08-12): FLAT `-demand` RESURRECTED (flag-off byte-identical).**
> The keyed-instance greenfield rewrite reached its namesake goal's first
> deliverable: the FLAT magic-sets transform is LIVE again (owner-ratified s29
> Path R flat-only). `lib/DataFlow/Demand.cpp` + `lib/Parse/Demand.cpp` restored
> from `6d6248a2` (the last-before-deletion tip — tip-K1-`@key`-API + S5′ Mint
> tags, NOT `48cd0a4f`); `Query::Build` is 6-arg again `(module, log, policy,
> demand_mode=false, demand_retract=false, suppress_demand=false)` with the
> `-demand` CLI flag (Main.cpp `gDemand`) re-added and `bin/Oracle` pinned
> `suppress_demand=true`. **ACTIVATION IS FLAG-ONLY:** RP-6 flagless `@key`
> force-opt-in is DEFERRED (the DIFF-R3 semantic slice, not flat `-demand`), so
> `@key` stays INERT flag-off and `key_partial_1`/`key_corecursion_1` are
> byte-identical (the gate is `if (!demand_mode) return true;`). A `-demand`
> program (recursive TC) compiles END-TO-END: guard JOINs + fabricated
> `demand__` message + demand relation lower through `Query::Build` →
> `FrozenRegionalProgram::Build` → `Program::Build` → codegen; §7 CodeGen
> ABI-suppresses the demand message's public entry point (only the `_detail`
> twin the injector calls survives) and §6a Regional `CollectMessages`
> demand-filters it region-internal (V-REGION-CENSUS stays consistent). The
> four always-on validator surfaces (RowContract/ProjectionRole, K5
> origin_decls, Rel eager-web, Regional census) all pass on the demand graph.
> **STILL REMOVED (S2+):** the `-demand-instance` keyed
> InstanceStore nested lowering + `bool demand_instance` `Program::Build` param
> (the vestigial `DRInstance`/`demand_table` scaffolding in `lib/Rel/Rel.h`
> stays untouched).
>
> **S1b CORE LANDED (2026-08-13, s36 CP4 `047a991b`): THE DEMAND SEED FLOWS —
> `-demand` programs are ANSWER-CORRECT.** `BuildQueryInjectorFromRegistry`
> (restored from pre-cut `6d6248a2`, recipe F2) + the registry-first
> `BuildQueryInjectorProcedure` dispatcher ((query, BindingPattern)-keyed;
> retract arm gated on message differentialness) + `context.demand_forcings`
> (= `Query::DemandForcings()`, assigned at the `Program::Build` head): a bound
> `#query` under `-demand` now injects its bound key as the fabricated
> `demand__` seed at query time through the `kQueryMessageInjector` proc (an
> always-on handler fence aborts if the fabricated message has no handler
> proc). WITNESS RESTORED: `demand_tc_witness` (recursive TC via its `-demand`
> `.drflags` — the first `.drflags` case back in corpus; 13 goldens re-blessed
> against tip incl. oracle/monotone/behavioral + df/ir/h/rel/contract +
> region×4; the `.rel` golden carries the s35/s36 `resource=sr#` surface).
> ANSWER-CORRECT ×4 modes: driver answers == the demand-blind oracle per
> probed key (incl. the standing-demand rebuild — edges arriving AFTER a
> demand re-fire the guarded fixpoint — and a first-demanded-after-data key);
> behavioral binary == interpreter CBF. Flag-off corpus BYTE-IDENTICAL
> (registry empty ⇒ dispatcher no-op). Gate: OptDiff **SUITE PASS (228)**,
> ctest **5/5**; every `-demand` compile passes the s36 arrangement
> cross-check quiescently. **S1b TAIL (next): the bench carrier** (the s29 O1
> selective-pruning measurement with the REAL transform — `idx_hops`
> 40000→~11 + the non-selective regression) + S1c polish (Tier-1 demanded-
> interior naming re-add). Authority: `session-31-s1b-seed.md` §3-§5.

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
cd build/debug && ctest --output-on-failure   # IdentityTypes, MiniDisassembler,
                                              # PointsTo, RegionInstance, Runtime
                                              # (RegionInstance = the P3 request/
                                              #  derivation-model discriminating gate;
                                              #  DataFlowValidators / RelValidators /
                                              #  InstanceStore removed at the P1 cut)
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
(`<name>.dr` + `<name>.main.cpp`, 177 corner-case programs after the
P1 greenfield cut removed the demand/keyed corpus — symrec_tie_1 is the standing
determinism witness; corecursion_1 is the P6.1 co-recursion witness
(mutual `ping`/`pong` → ONE `recursive-component  members=(ping, pong)`; its
region goldens the co-recursion pin, beside tc_nonlinear_diff's self-recursion
`members=(tc)`, two_inductions' TWO-independent-components, and recursion's
mode-FAITHFUL 0-vs-5 witness — un-optimized modes surface vacuous self-loops
canonicalization strips); agg_distinct_1 pins the aggregate multiplicity
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
  candidate), and the TEN `@key` rejects (DIFF-R3 sessions 5-6 + K1
  session 7, ALL FLAGLESS — RP-6 activation makes every one fire with no
  .drflags): `key_wildcard_1`/`key_anon_1`/`key_dup_1`/
  `key_unknown_1` (pragma arg-list obligations),
  `key_mismatch_1` (V-DECLARED-KEY: declared set ≠ SIP-inferred
  `p_bound`; a STABLE hard reject, RP-3), `key_multi_adorn_1`
  (RP-10 bijection Arm B: partial declaration — `@key(A)` alone against
  inferred {A},{B}; repurposed at K1 from the retired single-forcing
  reject), `key_over_adorn_1` (Arm A: a declared set with no inferred
  match), `key_multi_adorn_allfree_1` (the all-free-sibling fence fires
  upstream of the bijection, pragma-activated),
  `key_fenced_1` (a MATCHING pragma + NEGATE body draws the FENCE
  class, never V-DECLARED-KEY — O-R3.4) and `key_undemanded_1`
  (an @key on an undemanded relation — the RP-6 realization reject); `kvindex_1` is MODE-SPLIT (compiles
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
  demand-lowered; TRUE IN CODE since F32, 2026-08-05 — `mode_flags_of`, and
  the four differential-regime demand behavioral goldens were re-blessed to
  the definitional CBF, pure additions) — must byte-agree across all 4
  modes AND match the frozen `goldens/<name>.behavioral.stdout` AND match
  the interpreter's CBF. An `@key`-pragma case's behavioral binary is
  inherently pragma-activated (in-source surface).
  Diagnostic `.batches` cases run interp-only. A REFINTERP-DISAGREE is
  adjudicated per the stage doc §3 (finding, never fudge; F29 was found+fixed
  this way). The suite verdict aggregation is a WHITELIST since F32: any
  verdict line not ending in an OK shape fails the suite, plus a per-case
  verdict-coverage census (the old failure-token blacklist silently dropped
  REFINTERP-DISAGREE and BEHAVIORAL-MODE-SPLIT — F32 leg (b)). XFAM (the
  cross-family check, since the same session): every `.batches` case also
  runs `run_crossfamily` — the oracle's `--project-published` DIFFERENTIAL
  projection (published-message final membership in CBF FINAL byte shape,
  string-sorted) must byte-equal the FINAL block of the behavioral golden
  (live interp.cbf for diagnostic cases) — a fourth code-disjoint evaluator
  refereeing the published surface; vacuous-by-data for cases publishing
  nothing, zero goldens of its own.
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
  never automatically on failure, and never to make a red case green. Since
  K1 the bless loop's `bless_copy` helper MECHANIZES the symlink rule: a
  symlink golden is never written through — byte-identical produces a skip
  line, divergence is a loud BLESS-REFUSED + exit 1 (the twin-equivalence
  witnesses key_tc_witness / key_neighborhood_witness /
  key_multi_adorn_witness stay safe under an unfiltered bless). Since F32
  (2026-08-05) a byte-identical NON-symlink re-bless is also a skip line,
  so `BLESS: N golden(s) updated` counts only real content deltas.

`tests/OptDiff/FINDINGS.md` is the ledger of bugs found this way, with
repros (F1–F19, F21, and F26–F31 fixed as of August 2026; F23 promoted to
the `product_in_scc_diff_1` pin; F20 is the sole open record-only note —
the latent comparator. F29, the commit-sweep used-state collector omitting
the swept table's live indexes, was promoted and fixed the same day the I0
sweep fired its named trigger. F31, the redeclaration-consistency
off-by-one — `prev_decl` aliased the CURRENT decl, so the parameter
type/name divergence checks had been dead forever — was found by the K6
adversarial panel and fixed the same session, together with its
F-K6-SHADOW corollary: a `@key` on a non-first redeclaration was silently
dropped; the accessors now resolve through the declaration context, and
`reject_key_redecl_1` + the shadow-shaped `key_multi_adorn_witness`
header pin both).

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
- MINT TAGS (S5′, keyed-instances epoch): every `Def<T>` node carries a
  `const char *mint_tag{nullptr}` — a stable, self-identifying `"pass/what"`
  string literal naming the CREATION SITE (e.g. `demand/guard-join`,
  `connect/insert-proxy`, `merge-canon/forward`), set ONLY via the free
  `Mint(list, "tag", args...)` in `include/drlojekyll/Util/DefUse.h` (a thin
  `DefList::Create` forward + stamp; old `Create` stays legal/untagged
  forever, so the sweep is incremental). NEVER in Hash/Equals, never a
  lowering input, never moved by any field-copy helper (CDaGI included) — a
  survivor keeps its OWN tag across CSE; golden-safe by human-stability of
  the literal (unlike source locations, which churn). Rendered on the `.df`
  ATTRIBUTES line as a trailing `tag=` token and in the `-dot-out` node cell
  as a `MINT` line (advisory). Slice 1 = all 253 DataFlow `Create` sites
  (authoritative table + convention:
  `docs/proposals/RegionalDataFlowCore.artifacts/s5-mint-tags-table.md`);
  Rel/ControlFlow/Regional families are the un-swept follow-on (Mint is
  family-agnostic). This SUPERSEDES the s9 `std::source_location` mint-site
  formulation (`s9-mint-sloc-diag-formulation.md` §1, on record but dropped).
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
  visualization, never byte-goldened; since K6 fabricated demand-minted
  columns render a sane field label, no `_MissingVar`). Since K6 the Rel
  IR has its own advisory DOT twin, `-rel-dot-out` (cluster-per-stratum,
  id-ordered ops/vecs with def/use edges, census-free, never goldened),
  and `-region-dot-out` badges `declared-key` contracts.
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

## InstanceFlow (the new context-family IR — Phase A + CP2 grove + s34 MaterializationPlan LANDED)

`lib/DataFlow/InstanceFlow.{h,cpp}` — a NEW compiler IR between the optimized
Query and Rel (`docs/proposals/InstanceFlow.md`, the 2198-line vision; the
owner-chosen direction as of session 32). InstanceFlow represents a deterministic
GROVE of context-parameterized computation families built backward from consumer
uses. **Phase A + CP2 are built** (owner-ratified destination as of s33: the BIG
UNIFICATION — demand + keyed + multi-adornment + carried-column elision all
lowering from one family vocabulary, §20-E — via InstanceFlow as a SEPARATE
derived IR, NOT nested/annotated dataflow, which re-tangles the physical/logical
split-brain P1 deleted; §1). The maximally-shared FLAT empty-context grove
(§7.3) — every family binds NO context, every node's residual IS its full logical
schema. It is an OBSERVER at the Query→Rel seam: `BuildFlatInstanceFlow` runs at
the `Query::Build` tail (`Build.cpp:2664`, post-`row_contracts`) and stores an
`InstanceFlowProgram` by value on `QueryImpl` (the RowContract precedent), but
NOTHING downstream reads it — so **codegen is byte-identical** (the invariant
every slice through Phase C must preserve; the first codegen move is Phase D by
design). Typed-id catalogs (`QueryOriginId`=`det_seq` / `OriginUseId` /
`LogicalCollectionId` / `DerivationSiteId` / `EmissionAuthorityId` / `FamilyId` /
`FamilyNodeId` / `QuerySccId`, mirroring `lib/DataFlow/Identity.h`; ids from
`DeterministicOrder()` + canonical order, NEVER `UniqueId()`/iteration order).
The grove: origins, collections (INSERT-target decls), derivation sites,
SCC-condensation families (keyed on the **multi-view-stratum histogram** — a
stratum with >1 live member IS a recursive SCC, the RowContract Phase-0/P6.1
notion; `InductionGroupId` tags only the union node and would leave half an SCC
in the acyclic family), emission authorities, the total-ordered `OriginUse` walk,
and the per-consumer **coverage bijection** (coverage ≠ emission: `V-IF-COVERAGE`
covers each consumer obligation once, `V-IF-EMISSION` binds only terminal
INSERTs). Five always-on `V-IF-*` validators (fprintf+abort, NDEBUG-surviving):
ORIGIN / CONTEXT / SCC / EMISSION / COVERAGE. `-instanceflow-out` text dump
(opt-mode-pinned, goldenable, `QueryInstanceFlow` tag). **CP2 (s33, LANDED)**
finished the dump surface: per-node `role=root|interior` (root = terminal-INSERT
writer / bound-read target; candidate join/agg CONTEXTS stay the `seeds` catalog,
NEVER node roles); family `root=none` (`root_use` stays `std::optional`, nullopt
in the flat slice — the panel-adjudicated fix for the seeded `roots=(...)` plural
that contradicted §6 `root_use:OriginUseId` singular / §16 `root=u#41`); the
`kBoundQueryRead` coverage arm (one use per query-collection × bound adornment,
appended as a deterministic CONTIGUOUS use-id SUFFIX so existing ids never
renumber, covered by writer0, `V-IF-COVERAGE` arm asserts the suffix + INSERT occ).
4 `.instanceflow` goldens pin it (join_1 / merge_2 / transitive_closure — the
bound-read witness / barrier_neck_1). STILL an OBSERVER (codegen byte-identical;
the invariant every slice through Phase C must preserve). DEFERRED: candidate
seeds (S5); `role=input` (a Phase-D port concept). Authority:
`docs/proposals/InstanceFlow.artifacts/session-34-seed.md` (post-CP2 whole-program
view + the big-unification path as diffs, with the TRUE Phase-C blocker named:
runtime-resource allocation is interleaved into `lib/ControlFlow/Build/*` ~8272
LOC and must move to AFTER a Rel authority decides) + `session-33-grounding.md`
(the 3-refuter panel, all claims refuted+folded) + `session-32-phaseA-grounding.md`.

**s34 MaterializationPlan RESOURCE authority (resources-first slice, LANDED)** —
`lib/DataFlow/Materialization.{h,cpp}`, the FIRST real step of the big
unification (InstanceFlow.md §10, Phase-B item 3). A NEW typed IR authority
stored by-value on `QueryImpl::materialization`, built at the `Query::Build`
tail AFTER the grove (`PlanResources`). It DERIVES ControlFlow's stateful-storage
decisions from the FINAL graph — `DeriveStatefulClasses` is a PURE `QueryView`-API
replay of `FillDataModel`'s R1-R9 TABLE-need rules (`NeedsInduction*` inlined via
`InductionGroupId()`/`NonInductiveSuccessors()`; the DataModel classes ARE the
`EquivalenceSetId` partition, verified in the panel) — and CROSS-CHECKS them
byte-for-byte against the real `view_to_model` allocation. **GRAIN (grounding
refuter claim-d fold):** ONE authoritative `StateResource` per stateful PHYSICAL
class (`EquivalenceSetId`), NOT per collection — a BIJECTION (V-MAT-BIJECTION) so
`ResourceForView(v)=authority(EquivalenceSetId(v))` is the well-defined map the
Phase-C `TABLE*`→`StateResourceId` retype consumes. Co-recursive shared stores
(`reachable_from`+`reaching_to` alias tc's table) resolve through one canonical
(min-`LogicalCollectionId`) authority + `ForwardingAlias` entries (§10 `aliases`),
satisfying §17 V-MAT-AUTHORITY (every stateful collection resolves to exactly one
authoritative resource). Residues = stateful classes with no collection writer
(`InternalResidualCollectionId`, ascending eqset — the Tier-2 provenance shape).
Arrangements DEFERRED (§2.5.3 (iii): no standing `collect_arrangement_requirements`
pass; no Rel struct carries an arrangement identity). `CrossCheckMaterialization`
(public `Query.h` friend, run at the `Program::Build` tail after `FillDataModel`)
compares the STORED plan's classes to the real table-backed set and ABORTS on
divergence — an always-on belt, verified LIVE (corrupt R4 → fires naming the
missed residue classes; revert → quiescent). Still an OBSERVER: codegen
byte-identical (OptDiff **SUITE PASS 227**). `-materialization-out` dump
(`QueryMaterialization` tag, opt-mode-pinned); 5 `.materialization.opt` goldens
(join_1 = residues; transitive_closure = the alias/co-recursion witness;
corecursion_1 = shared-store residue; negate_1 = mixed differential/monotone;
tc_nonlinear_diff = the §2.5.2 all-differential + `internal#2 (X,From,To)`
witness). Authority: `session-34-seed.md` §2.5 + `session-34-grounding.md`
(3-refuter panel: claims a/b/d refuted, c subsumed by the bijection fold).

**s34 Phase-C step 1 (cross-checked `StateResourceId` shadow on `DRTable`, LANDED)**
— the FIRST Rel object to carry a materialization-map identity beside its
`TABLE*` (InstanceFlow.md §11). `DRTable` (`lib/Rel/Rel.h`) gains a
`StateResourceId resource`, stamped at `BuildDRInventory` (`Rel.cpp`) from the
table's member views' shared `EquivalenceSetId` via the resources plan
(`MaterializationPlanOf(query)`, a public `Query.h` friend; `lib/Rel` +
`lib/ControlFlow` gained `lib/DataFlow` on their private include paths). The
`ResourceForView(v)=authority(EquivalenceSetId(v))` map is single-valued by
V-MAT-BIJECTION. A SHADOW: the id is consumed by nothing and the `DRTable`
inventory is NOT rendered in `-rel-out`, so codegen AND every dump stay
byte-identical (SUITE PASS 227). `V-REL-RESOURCE` belt at the mint (a table's
member views all share ONE class, and that class has exactly one authoritative
resource) — belt-verified LIVE. Proves `TABLE*`->`StateResourceId` is derivable,
total, and single-valued in the Rel object model — the property the Phase-C
endpoint (retire `DRTable.model` for the id) depends on. NEXT: retype more Rel
`TABLE*` fields onto the id, then the allocation inversion (the true blocker:
allocation interleaved in `ControlFlow/Build`).

**s35 Phase-C step 2 (the WHOLE Rel op model is resource-addressable, LANDED)** —
extends the `StateResourceId` retype from the single `DRTable` to the entire
`DROp`/`DRBranch`/`DRJoin` base-table surface via ONE index (NOT a relocation):
`DRFlowGraph::table_to_resource` (`lib/Rel/Rel.h`), a
`std::unordered_map<TABLE*, StateResourceId>` built once in `BuildDRInventory`
(`Rel.cpp`) from the just-stamped `DRTable`s' resolved `resource` (mirrors the
`view_to_model`/`eqset_to_resource` `_to_` idiom). It is the SOLE source every op
base-table field resolves through: the `-rel-out` render (` resource=sr#K` after
each **`args:`-line** base-table token, 18 sites in `Format.cpp`, the SAME sr#K id
space as `-materialization-out` so the two dumps cross-reference by id) and the
`V-REL-OP-RESOURCE` belt (`ValidateOpResources`, `BuildDRInventory` tail) read it;
NOTHING that emits does, so codegen is BYTE-IDENTICAL (OptDiff **SUITE PASS 227**;
only the 10 `.rel.opt` goldens moved, an ADDITIVE token, predicted-then-verified).
The belt walks every non-null base-table field (resolution-FIRST for crash safety,
then — where a semantic view partner exists — a class cross-check `field-table
class == partner-view EquivalenceSetId`); belt-verified LIVE (drop a table from
the map → fires naming the unresolved field; revert → quiescent). HONESTLY a
REGRESSION FENCE + goldened coverage artifact, NOT a live bug-catcher: the pairing
checks are tautological-at-construction today (every partner table is minted from
`view_to_model[partner_view]->table`), earning real teeth only when Step 3 stores
an op's table and id/view from INDEPENDENT sources. The ONE forward-load-bearing
output is `table_to_resource` itself — the `TABLE*->id` function Step 3's
allocation inversion inverts. Grounded (2 sonnet extractions + 2 opus refuters +
a Step-2b measurement) in `InstanceFlow.artifacts/session-35-grounding.md`; **Step
2b DEFERRED** on a view-set-equality soundness premise (Materialization ORs
`support` over ALL class views, `TableIsDifferential` over the narrower
`table->views` — not proven equal; the panel saw only the predicate difference).
NEXT (owner-ratified scope, `session-35-arrangement-scope.md`): the ARRANGEMENT
derivation (§2.5.3) — LANDED s36 (below); then Stage C the inversion.

**s36 the ARRANGEMENT derivation (Stages A+B BUNDLED as derive-early /
cross-check-late, LANDED)** — the resources plan's missing half (§2.5.3 closed;
the true Step-3 unblocker). `DeriveArrangements` (`lib/DataFlow/
Materialization.{h,cpp}`, run at the `Query::Build` tail after `PlanResources`)
derives the INDEX requirement set PURE-side by replaying the six emission
`GetOrCreateIndex` sites' column logic from the FINAL graph — R-FULL (every
resource's all-columns creation-default index), **R-JOIN-UNIFORM** (per joined
side of EVERY pivot-JOIN, the side's pivot input ordinals — the panel-refuted
BuildJoin/EmitJoinFire routing split is extensionally INVISIBLE in the SET:
fully-interior fixpoint joins scan all k≥2 sides, partially-interior joins
always carry an eager/delta BuildJoin minting all sides; unit sides + all-bound
patterns dedup into R-FULL, so NO differentialness predicate is needed —
sidestepping the Step-2b view-set premise), R-NEG (non-@never crossover: the
negate's non-constant key input ordinals on the pred class, Stratum.cpp
:1168-1185 replayed verbatim), R-QUERY (per unique bound binding pattern over a
surviving query-INSERT; mirrors `SelectAccessPlan`, which Regional→DataFlow
layering bars calling), + the R-INTERFACE count (`#query` decls with no
surviving INSERT ⇒ the resource-less always-empty `BuildEmptyQueryEntryPoint`
table — COUNT derived + cross-checked; per-table sets a named Stage-C
residual). Typed domains: `ArrangementId`, `ColumnOrdinal` (a position in the
store's column order — its own domain, not `FieldId`, not a count),
`ArrangementKey` (the id-free `(resource, ordinals)` content whose defaulted
ordering IS the canonical order — ascending `StateResourceId` then lex ordinal
vector — and the census seam type). THE TEETH: `CrossCheckArrangements`
(`Query.h` friend) at the `Program::Build` tail censuses the REAL
`impl->tables[*]->indices` through `context.dr_flow->table_to_resource` (the
s35 map's FIRST real consumer; the universe is FINAL there — nothing mints or
deletes a TABLEINDEX after region build, `#if 0` Build.cpp:1502 dead,
`ProgramImpl::Optimize` index-free) and aborts naming every divergent
arrangement on either side — a REAL falsifiable claim (pure derivation vs
stateful emission walk are disjoint code paths), quiescent corpus-wide 227
cases × 4 modes, belt-verified LIVE (drop R-NEG → negate_1 aborts naming `sr#1
columns=(0)`; revert → quiescent). NAMED blind spot: the view-ordinal→
table-ordinal CSE congruence is REPLAYED from emission's assumption (byte-
equality, not independent soundness; the unchecked union is INSERT↔guard-TUPLE,
DataFlow Build.cpp:2412-2428). Render: `-materialization-out` header
`arrangements=K` (+ `interface-tables=J` only when J>0) + an `arrangements`
block (`ar#K resource=sr#R columns=(ordinals)`, sr# = the ONE id space shared
with `-rel-out`). Codegen byte-identical; only the 5 `.materialization.opt`
goldens moved (additive, predicted-then-verified byte-exact on the FIRST
build). Grounded: 4-sonnet recon + 4-opus refuter panel (claim (c) REFUTED as
drafted → the uniform-rule fold; a/b/d survive) in
`InstanceFlow.artifacts/session-36-grounding.md` (§5 = the named Stage-C
residual lacks: interface-table authority, TABLEINDEX id-renumbering decision,
column-order/schema authority, Step 2b, six-site column logic stays). The
owner redirected s36's tail to S1b — the injector CORE LANDED same session
(see the S1b note at the top of this file). NEXT: the S1b tail (bench carrier
+ S1c polish; charter `InstanceFlow.artifacts/session-37-prompt.md`) or Stage
C (the allocation inversion, scoped + PARKED in
`InstanceFlow.artifacts/session-37-seed.md`) — owner re-ranks.

## The demand transform (`-demand`, magic-sets) — [LIVE (flat) since S1a; multi-adornment/@key-activation text below is HISTORICAL until re-verified]

> **S1a (2026-08-12) re-scope of the text below.** The FLAT DataFlow transform
> is LIVE again (see the S1a note at the top of this file). Accurate at tip:
> `-demand` is mode-gated OFF by default (`ApplyDemandTransform` returns at its
> head when `!demand_mode`), orthogonal to the 4 golden modes, minting guard
> JOINs + a fabricated `demand__` message per bound `#query`. The rest of this
> section (single-adornment slice, `.drflags` mechanism, reject classes) is the
> restored pre-cut behavior. WHAT DIFFERS from the pre-cut text: (1) activation
> is FLAG-ONLY — the RP-6 flagless `@key` force-opt-in is DEFERRED, `@key` is
> inert flag-off; (2) the ControlFlow injector that FLOWS the seed at query
> time is S1b (not yet wired), so a `-demand` program compiles end-to-end but
> the demanded relation is not yet auto-seeded; (3) corpus counts/witnesses
> (`demand_tc_witness` etc.) were deleted at P1 and are being re-added in S1b.
> Re-verify any claim below against code before relying on it.

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

## The keyed-instance nested lowering (`-demand-instance`) — [REMOVED at the P1 cut — historical]

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

> **P6.3 fusion-DETECTION spike LANDED (session 26): classify each recursive component FUSABLE vs
> JOINT (compile-time, dump-only, codegen BYTE-UNCHANGED).** The FIRST cut of the P6.3–P6.6 runtime-eval
> arc, taken DETECT-ONLY: `ClassifyFusableComponents` (`Planning.cpp`, run after `PromoteSharedSymbolicField`
> at `:760`, before census) tags each `RecursiveComponent` `Fusion ∈ {kUnclassified(anti-stub default),
> kFused, kJoint}` and, when fused, records the common preserved binding prefix as HEAD ordinals `{0..L-1}`.
> It reads ONLY the clause-source `R.rules` (P6.2) + `R.recursive_components` (P6.1) + the field interner's
> inverse — **DELIBERATELY NOT `inherited_symbolic_fields`** (P6.2 promotion is global/symmetric and can
> promote via an unrelated single-producer chain — corecursion_1's `shared-field F1=(ping.B, pong.B)` is
> promoted even though the recursive edge preserves only column A; reusing it would over-name the prefix
> `(A, B)` where routes preserve only `(A)`). ALGORITHM: a within-cycle producer = a rule whose head ∈ SCC
> AND that carries ≥1 route sourced from an SCC member; per such rule take the maximal contiguous identity
> prefix `{0..local-1}` (head_ord == src_ord over in-SCC routes), `prefix_len = min`; `kFused` iff
> `any_recursive && prefix_len > 0`, else `kJoint`. Base/seed producers (no in-SCC route) and renamed
> recursive edges (empty route — `BuildRuleRoutingProjections` needs a shared clause var) contribute no
> within-cycle producer → default JOINT (the sound conservative under-approximation: FUSED only when
> provable). The two P6.3 fields on `RecursiveComponent` are NEVER hashed, NEVER read by codegen
> (`Program::Build` consumes `DataFlowGraph()`, never `Region()`), so **every answer golden is
> byte-identical = the detect-only soundness proof**. `-region-out` gains a gated own-width block
> (`fused-fixpoint C<k> binding-prefix=(names…)` / `joint-fixpoint C<k>`, both tokens 14 chars → self-align;
> NO census count — the P5/P6.1/P6.2 precedent). F16 join-multiplicity guard: **CLOSED in session 27**
> (was LATENT at the P6.3 landing) — `ClassifyFusableComponents` now collects the DISTINCT in-SCC source
> fields per head ordinal and marks it identity-preserved ONLY if it has exactly ONE in-SCC source at the
> SAME ordinal (mirroring `PromoteSharedSymbolicField`'s `src.size()!=1` guard); a head ordinal join-bound
> by ≥2 distinct in-cycle sources — or a single non-identity source — is correctly NOT fused (outside-SCC
> frozen sources are FILTERS, never counted). Witness `f16_join_witness`
> (`r(A,B):-r(A,C),r(C,B),r(B,A)`, routes join-bind both head ordinals): F16-lax wrongly emitted
> `fused-fixpoint binding-prefix=(A, B)`, F16-closed correctly `joint-fixpoint C0`. See the P6.3-A1
> paragraph at the section tail. Gate GREEN: OptDiff **SUITE: PASS (226)**, ctest **5/5**, codegen byte-stable;
> **18 region goldens MOVED** (pure additive suffix, 5 cases: corecursion_1 → `binding-prefix=(A)` — the
> LOAD-BEARING partial-prefix discriminator, the `(A)`-not-`(A,B)` byte proving the detector reads `rules`;
> two_inductions → two `joint-fixpoint` (renamed recursive edges); tc_nonlinear_diff →
> `binding-prefix=(From, To)`; key_corecursion_1 → `binding-prefix=(K)` == inert @key(K); recursion nodf/none
> → 5 vacuous fused self-loops, MODE-FAITHFUL, opt/nocf byte-identical). **VALUE (blunt): ZERO runtime/
> codegen/answer value today** — it CLOSES P6.3's open question with corpus proof (fusability IS
> compile-time-decidable from `rules` + `recursive_components` alone) and PINS the sound route-derived
> definition against the promotion-reuse trap as a goldened byte; its only consumer is the dump + a future
> P6.4/P6.5 planner. Grounded DOCS-ONLY (anchor sonnet → design opus → 5-concern opus refuter panel, verdict
> PROCEED) then owner-gated execution + predict-then-verify on the carriers.
> Authority `p6.3-fusion-detection-grounding.md`. **P9 (access-path inference) was grounded the same
> session (`p9-inference-grounding.md`, verdict SOUND-BUT-SPECULATIVE) and DEFERRED by the owner: it is
> CONSUMER-LESS at landing (the four-authority firewall bars `SelectAccessPlan` from reading it; P7 seeks
> from the raw bound subset), so it needs an explicit firewall-relaxation decision to earn a reader. The
> E-71 header-token mini-diff was found ALREADY DISCHARGED at tip (`lib/Rel/Format.cpp:400-403` emits `rel`).
> **P6.3-A1 / the runtime-eval fork (session 27):** grounding established that the model-drives-codegen
> payoff is gated behind real prerequisites, so every incremental step short of them is observer/shadow
> code (the P9 → P6.4 → P6.5 pattern). P6.4 (activation-edge derivation, `p6.4-activation-edge-grounding.md`,
> refuter verdict SOUND but pure ctest-only scaffolding) was SKIPPED. P6.5 was re-scoped to Architecture A
> (model DRIVES codegen — a `RecursiveEvaluationPlan` authority peer of `AccessPlan`, NOT the
> `EvaluateEpoch` interpreter, which is a zero-codegen-consumer runtime-semantics half in a different
> substrate); its grounding (`p6.5-archA-grounding.md`, UNANIMOUS 3-refuter REFUTE-AS-FRAMED) found the
> smallest achievable slice A0 is a certification SHADOW (codegen byte-unchanged) and the real divergence
> A1 is blocked on a MISSING codegen arm + the open F16 gap + possibly P8/P9. Owner chose "attack the A1
> blockers", **F16 first** — LANDED this session (see the F16 note above): the guard + the `f16_join_witness`
> discriminator, SUITE PASS + ctest 5/5, ZERO existing goldens moved, codegen byte-stable — the first real
> (non-shadow) A1-prerequisite. NEXT (owner re-ranks): the OTHER A1 blocker (scope the fused-round codegen
> arm — genuinely new emission in `LowerRoundBody`/`Stratum.cpp`), P8 (ordered trie — may be the real
> A1 unblocker), or re-sequence the endpoint. P9 stays deferred (firewall-relaxation).**

> **P7c LANDED (session 25): retire the probe-REDUNDANT partial-scan re-check — the AccessPlan
> Fold C.** Now that P7b NAMES every interior partial index scan `kPartialKeyHashSeek` and the
> V-PLAN-HONEST belt proves the emitted arm is the full-key-exact `keyed_chain`
> (`Index::First/Next`), the redundant `TUPLECMP` re-check `BuildMaybeScanPartial` wrapped around
> each seek body is GONE. **This is the FIRST cut where the `AccessPlan` authority PAYS OFF in
> emission** — unlike P7/P7b it MOVES codegen (the seek carriers' generated
> `if (key == scanned) { … }` gate disappears; `.h`/`.ir` shrink). THE CUT (shape (i), minimal):
> in `BuildMaybeScanPartial` (`Build.h`) the per-column indexed branch is reached ONLY on the
> `kPartialKeyHashSeek` arm (the all-columns-bound case early-returns before the mint), so the two
> `cmp->lhs_vars/rhs_vars.AddUse` pairs (and the dead `in_var`/`j` machinery) were exclusively on
> the seek path — deleting them leaves the `cmp` a VACUOUS (trivially-equal) TUPLECMP that survives
> only as the body's parent region + `col_id_to_var` recordization anchor. PRESERVED: `scan->in_vars`
> (the seek probe key, `Database.cpp:2818`), `in_cols`, `out_vars`, `col_id_to_var` (populated for
> EVERY column). CORRECTNESS: the index key set == the bound-column set by construction
> (`GetOrCreateIndex(in_col_indices)`), and `Index::First/Next` is FULL-KEY EXACT by contract
> (`include/drlojekyll/Runtime/Table.h:789-824`), so every yielded row already has key columns == the
> requested key — the deleted `in_var==out_var` pairs were always-true. This is the SAME argument
> that let the TABLEJOIN body (and the differential sections) omit their per-row re-check
> (`Table.h:799-803`) — P7c is Fold C for the partial-scan path; keep the two linked. The full-scan
> arm (`index==nullptr`) ALREADY built an empty cmp (the 22 `scan-table` carriers exercise the shape),
> so P7c only makes the seek arm match; the P7b V-PLAN-HONEST belt is UNCHANGED and still never fires.
> IR shape: opt/nodf (controlflow-opt ON) elide the vacuous cmp entirely (`OptimizeImpl(TUPLECMP)`,
> `Optimize.cpp:595` — replace kEqual-empty with body); nocf/none keep an empty `if-compare`
> EmitCompare renders as no gate (`Database.cpp:2914`); both emit identical gate-free codegen. Gate
> GREEN: OptDiff **SUITE: PASS (226)**, ctest **5/5**, EVERY answer golden (`.stdout`/oracle/
> monotone/behavioral) BYTE-IDENTICAL across all 4 modes (answer-invariance = the correctness proof).
> Goldens: **1 MOVED** (`negate_1.ir.opt` — the `if-compare`/`if-true` gate removed, 5 ins/7 del, all
> region ids + other lines byte-identical) + **1 NEW** (`negate_1.h.opt` via a new `h opt` `.irgold`
> step — pins the gate-free `idx_28.First/.Next` loop; `negate_1` had no `.h` golden). Grounded
> (confirm-then-ground) + in-tree empirical spike + 3-refuter opus panel (all `refuted=false` high
> confidence, "none found") in
> `docs/proposals/RegionalDataFlowCore.artifacts/p7c-execution-grounding.md`. **NEXT (owner
> re-ranks): P6.3–P6.6 runtime evaluation (P6.3 fusion-detection spike = low-risk entry) or P8/P9
> (ordered trie / path inference) — the AccessPlan-authority arc through emission is now complete
> for the partial-scan path.**

> **P7b LANDED (session 24): interior/join plan-driven scans — the EmitScan V-PLAN-HONEST belt.**
> P7 threaded the `AccessPlan` authority onto the `#query` path ONLY; P7b extends it to the
> INTERIOR/JOIN table-scan path. `ProgramTableScanRegionImpl` gains an `AccessPlan
> plan_kind{kUnplanned}` (`Program.h`) — EXCLUDED from Hash/Equals/MergeEqual (the S5′
> `mint_tag` precedent) so region CSE/id-numbering is unperturbed — stamped at the SOLE LIVE
> TABLESCAN mint `BuildMaybeScanPartial` (`Build.h`) as `index ? kPartialKeyHashSeek :
> kFullScanFilter` (the all-columns-bound case early-returns, so a minted scan always keys a
> STRICT subset → codegen's `keyed_chain` First/Next arm → seek; `keyed_probe`/all-key `.Find`
> is UNREACHABLE on this path, so the two-way form is total). The second mint,
> `BuildNestedLoopJoin` (`Join.cpp:254`), is STATICALLY DEAD (its sole caller is the `else` of
> `else if (true || …)` at `Join.cpp:742` — unreachable in every build mode; acyclic pivot
> joins emit `TABLEJOIN`, never `TABLESCAN`), so `kUnplanned` is a pure dead-default the belt
> SKIPS. **The headline is the D4 "Option-2" honesty belt: V-PLAN-HONEST moves to the EmitScan
> emission site** (`Database.cpp`, after `keyed_probe` is computed) as a per-kind IMPLICATION
> (`kFullScanFilter ⇒ ¬keyed_chain∧¬keyed_probe`, `kPartialKeyHashSeek ⇒ keyed_chain`,
> `kFullKeyHashLookup ⇒ keyed_probe`; fprintf+abort, survives NDEBUG) — it READS the booleans
> `EmitScan` already computes and NEVER drives dispatch, so **codegen is BYTE-UNCHANGED**
> (`plan_kind` is a compile-time SHADOW of the arm codegen already selected from
> `Index()`×arity). Render: a gated ` plan=<token>` on the `.ir` `scan-*` line
> (`ControlFlow/Format.cpp`), via the NEW single-source `inline hyde::AccessPlanText`
> (`RegionInstance.h`) shared with the Regional `-region-out` dump (the file-static in
> `lib/Regional/Format.cpp` was deleted → calls the inline; the two dumps can never drift).
> SCOPE: `#query`-path belt (`Build.cpp:457/515`) UNCHANGED; NO new Rel op / runtime structure
> (P8/P9 untouched). Corpus reality (swept all 4 modes): 69 `scan-index` (all seek) + 22
> `scan-table` (all filter), ZERO all-key scan-index → the belt referees 91 live scan lines and
> never fires. Gate GREEN: OptDiff **SUITE: PASS (226)**, ctest **5/5** (RegionInstance
> unchanged — P7b touches no ctest expectation), codegen byte-stable. Goldens: **0 modified, 2
> ADDITIVE-NEW** `.ir` goldens (`negate_1.ir.opt` pins `plan=partial-key-hash-seek` on the
> negation crossover scan; `insert_4.ir.opt` pins `plan=full-scan-filter` on the @product-arm
> full scan — via new `ir opt` `.irgold` steps; the sole prior `.ir` golden `symrec_tie_1` has
> zero scan lines → byte-invisible). Grounded + adversarially critiqued + EMPIRICALLY validated
> (grounding-loop workflow: sonnet anchors → opus design → 2-refuter opus panel + a
> throwaway-worktree spike that built clean, ran the suite belt-LIVE across all 4 modes with
> zero golden diffs, then HARDENED the belt to `assert plan != kUnplanned` and re-ran green to
> prove no live scan escapes unplanned) in
> `docs/proposals/RegionalDataFlowCore.artifacts/p7b-execution-grounding.md`. **NEXT (owner
> re-ranks): P6.3–P6.6 runtime evaluation (compile-time P6.3 fusion-detection spike = low-risk
> entry), P7c (retire the probe-REDUNDANT TUPLECMP belt on the now-honest seek — the R-final
> Fold C candidate), or P8/P9 (ordered trie / inference).**

> **P7 LANDED (session 23): physical access planning — the partial-key hash SEEK (Q1a broad).**
> A bound+free `#query` now lowers to an index SEEK (`Index::First/Next`) instead of P4's
> full-scan-filter cursor — the FIRST codegen QUALITY win from the `AccessPlan` authority, and the
> close of the `keyed-instances` namesake arc. `SelectAccessPlan` (`RegionInstance.h`) went from a
> `has_free`-only stub to a real dispatch: all-bound → `.Find` (`kFullKeyHashLookup`); bound+free with
> a non-empty bound subset → `kPartialKeyHashSeek` (NEW); no bound cols → `kFullScanFilter`. The enum
> SPLIT `kRetainedIndexScan` into a `kUnplanned=0` sentinel (the dispatch NEVER returns it) +
> `kPartialKeyHashSeek` (Q2 Option-A); `plan` ∉ Hash/Equals and `RequestPortRecord` is never hashed,
> so the renumber is byte-safe. SCOPE = **Q1a (raw bound subset)**: the seek fires on ANY partial
> binding, NOT gated on `@key` — the P5↔physical firewall (`Regional.h:92`) stays UP (the selector
> never reads `declared_access_paths`; `@key` is a WITNESS of intent, and `GetOrCreateIndex`'s
> `SortAndUnique` gives `[A,B]≡[B,A]` free). UNFENCED (Q3): a bound+free query reads the SETTLED
> relation after the fixpoint quiesces, so the seek over a recursive-owned relation is answer-correct
> (`GetOrCreateIndex` SHARES the fixpoint's index by `column_spec` or mints one non-colliding secondary
> index — empirically confirmed: `transitive_closure_diff`→shared `idx_80`, `key_corecursion_1`→fresh
> `idx_74`). **NEW CODEGEN SURFACE: NONE** — the flip is selector-driven: `withhold_index =
> plan==kFullScanFilter` already goes false for a seek, so `Build.cpp`'s existing
> `GetOrCreateIndex(col_indices)` provisions the bound-subset index and `EmitQueryFriends`' existing
> `via_index` First/Next arm emits (D5 is a documented no-op). V-PLAN-HONEST generalized to a per-kind
> IMPLICATION belt (`kFullScanFilter ⇒ ¬index` AND `kPartialKeyHashSeek ⇒ index`) at BOTH
> query-entry-point sites; NO EmitScan belt / NO `plan_kind` on `ProgramTableScanRegion` (Q5/Q6 — the
> D4→D6 hazard evaporates since P7 is #query-path-ONLY; interior/join scans stay index-presence-derived
> = P7b). P8 (ordered trie) / P9 (path inference) OUT of scope (runtime all-hash). GATE GREEN: OptDiff
> **SUITE: PASS**, ctest **5/5** (the P4 `RegionInstanceTest.cpp` GateA expectation flipped
> bound+free→`kPartialKeyHashSeek`, + a free-only fallback assertion). Codegen byte-stable EXCEPT the
> ~25 bound+free carriers' query cursors — golden-INVISIBLE (those cases carry NO codegen golden; their
> 4-mode `.stdout`/oracle/monotone/behavioral goldens are the free answer-invariance net and stayed
> byte-identical, recursive carriers included). Goldens MOVED (9): `key_partial_1.region.*` +
> `key_corecursion_1.region.*` (the `plan=` token → `partial-key-hash-seek`) + the FIRST codegen golden
> since P4, `key_partial_1.h.opt` (a NEW `h opt` `.irgold` step pinning the `Index<Key19> idx_19`
> member + the `idx_19.First/.Next` cursor with NO `NumRows()` rescan / NO bound-col re-check). Grounded
> + empirically de-risked (a throwaway-worktree spike: build + full suite + recursive answer-invariance
> + index-sharing, all clean) in
> `docs/proposals/RegionalDataFlowCore.artifacts/p7-execution-grounding.md` (+ `p7-grounding-seed.md`).
> **P6.3–P6.6 (runtime evaluation: fusion / cyclic activation / joint fixpoint / DRed), P7b
> (interior/join plan-driven scans), and P8/P9 remain — owner re-ranks.**

> **P6.1 LANDED (session 21): query-independent recursive components (compile-time, codegen
> BYTE-UNCHANGED).** `RegionTemplate.recursive_components` (was RESERVED-EMPTY) is now populated by
> `ComputeRecursiveComponents` (Planning.cpp, the SOLE populator): a projection of the DataFlow
> multi-view-stratum SCC condensation (`QueryView::Stratum()`, message-seam-closing) onto the frozen
> relations via `OriginDecls` — a MULTI-VIEW stratum IS an SCC cycle (self-recursion = a size-1
> `members`; co-recursion = size-N), and `OriginDecls` is narrower than rows-flow-through so a base
> relation (edge) is excluded from tc's component. It reads NO `rules` (P6.1 ⊥ P6.2). MODE-FAITHFUL
> (a per-compile observer of the actual graph — un-optimized modes surface vacuous self-loops opt
> strips). `RecursiveComponent` gains `std::vector<RelationId> members`; `-region-out` renders a
> gated own-width `recursive-component  C<k>  members=(…)` block (NO census count — the P5
> declared-key precedent, so only recursive dumps move). Grounded + adversarially critiqued (the
> panel refuted an insert-arm formulation empirically; the origin projection is the survivor) in
> `docs/proposals/RegionalDataFlowCore.artifacts/p6-grounding.md`.

> **P6.2 LANDED (session 22): typed edge-local routing + SymbolicFieldId promotion (compile-time,
> codegen BYTE-UNCHANGED).** The two last RESERVED-EMPTY routing fields are now populated (the P6
> compile-time first cut is COMPLETE). `struct RuleRoutingProjection { RuleId id; RelationId head;
> vector<pair<SymbolicFieldId,SymbolicFieldId>> body_to_head; }` — ONE per PRODUCER CLAUSE of a
> frozen relation (a `(body_field, head_field)` pair = a directed value-flow for one SHARED clause
> variable). Built CLAUSE-SOURCE, not DataFlow-source: the post-Optimize graph CSE-merges
> co-recursive relations onto one model table (ping/pong share `eqset=8 table=%table:4`) and LOSES
> per-relation field identity, whereas the parsed clauses retain it — the SAME lesson that flipped
> P6.1's insert-arm (p6.2-grounding.md §1.1). Three anon-namespace statics in `Planning.cpp`
> (wired into `FrozenRegionalProgram::Build` after the P6.1 populate, before census):
> `AssignSymbolicFields` (seeds ONE region-global `SymbolicFieldId` per (relation, ordinal) into the
> new `RegionInstanceRelations::symbolic_field_table` interner — peer of the P5 `schema_table`,
> DISTINCT domain, HP-9 dense order); `BuildRuleRoutingProjections` (the clause walk over
> frozen-relation-filtered POSITIVE body predicates — `ParsedDeclaration::Of(pred).Id() ∈
> relation_schemas` — matching head-pos↔body-pos by clause-scoped `ParsedVariable::Id()` equality;
> RuleId = dense per-clause ordinal; a clause with no frozen route is STILL stored empty as a
> BASE-CASE producer that BLOCKS promotion; negated/aggregate/@product body atoms do NOT participate
> — a sound under-approximation, F18/Q5); `PromoteSharedSymbolicField` (a `while(changed)` union-find
> FIXPOINT — F28 — with a DIRECTIONAL per-head-field primitive: union `SymFld(H,j)` with source class
> `s` iff EVERY producer clause of H sources H.j from exactly ONE frozen body field and all agree —
> an empty source = base case = NO promotion, ≥2 sources = a JOIN = ambiguous = NO promotion, the
> F16 co-occurrence trap; union-by-min gives a deterministic class-minimum rep, stored into
> `inherited_symbolic_fields`; the reconstruction-diffs `ProducerRulesOf(f_a)∪ProducerRulesOf(f_b)`
> union was INCOHERENT across relations and this is the corrected survivor). `-region-out` renders a
> gated own-width `rule R<k> produces=<rel> routes=(src.f->head.f, …)` block (empty-route base
> producers occupy RuleIds but emit no line → rendered ids non-contiguous) + a `shared-field F<rep>
> members=(rel.f, …)` block (promoted classes size≥2 only), AFTER the recursive-component block, NO
> census count (P6.1/P5 precedent). Codegen BYTE-UNCHANGED (nobody outside `lib/Regional` reads
> `rules`/`inherited_symbolic_fields`/`SymbolicFieldId`; `Program::Build` reads `DataFlowGraph()`).
> Grounded + 4-refuter EMPIRICAL opus panel (zero blocking; C1 fixed the exit-gate carrier's vacuous
> copy-cycle to a JOIN-in-cycle so `members=(p,q)` survives opt canonicalization; C2 added the F28
> anti-single-pass referee) in `p6.2-grounding.md`. Carriers: NEW `key_corecursion_1` (@key
> co-recursion — PROMOTE arm `shared-field members=(lookup.K, p.K, q.K)` + F16 DO-NOT-PROMOTE arm =
> `a.K`/`b.K` ABSENT) and `fixpoint_force` (the F28 referee — nodf/none `shared-field members=(c.K,
> p.K, a.K, b.K)`; a single-pass bug drops `p.K`); `corecursion_1`/`tc_nonlinear_diff`/`key_partial_1`
> + ~17 more region goldens gained a pure `rule`/`shared-field` SUFFIX (every prior line
> byte-identical; 30 region goldens moved, 2 new cases). Gate GREEN: OptDiff **SUITE: PASS (226)**,
> ctest **5/5**, codegen byte-stable. **P6 compile-time first cut COMPLETE; P6.3–P6.6 (runtime
> evaluation — fusion / cyclic activation / joint fixpoint / DRed) is a LATER, separately-gated cut.**

> **P3 LANDED (session 18): the RequestEdge/FactDerivation acyclic slice.** The frozen program now
> owns a TYPED P3 model beside the `RegionTemplate` — `include/drlojekyll/Regional/RegionInstance.h`
> (the new public leaf hosting ALL regional typed-id domains + `RegionInstanceRelations` with the
> edge/derivation ops), stored by value on `FrozenRegionalProgram` (`Instances()`). It is a
> COMPILE-TIME model layered over the retained full-materialization backend — **codegen is UNCHANGED**
> (M3). `BuildRequestPorts` (Planning.cpp) now SPLITS the query loop: a **bound** `#query` → a
> `RootLease` **request port** (`RouteKind::kRequestPort`, numbered after input/result so all-free
> programs stay byte-identical; `census.request_ports` re-derived); an **all-free** `#query` → a
> `PermanentRoot`; BOTH mint a `RequestEdge` (B2). The region is rooted by the ProgramRoot,
> independent of any `#query` (F1). `-region-out` renders the new `-> request-port P<k>` +
> `request-port … query=…/… bound=(…)` lines. Discriminating gate: ctest **`RegionInstance`**
> (`tests/RegionInstance/`) + the committed `booleans.region.*` bound-query goldens. The paragraph
> below describes the PRE-P3 Stage-B shape (some anchors stale: `frozen.Query()`→`DataFlowGraph()` at
> P2; "request-ports = one per demand forcing" → now one per bound `#query`). Full record:
> `docs/proposals/RegionalDataFlowCore.artifacts/p3-grounding.md`.

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
too, and since the session-8 K5 TIER-2 LIFT so is EVERY undemanded
non-@inline #local/#export interior — see the DIFF-R3 section below).
H4: `Program::Build(const FrozenRegionalProgram&, ...)`,
first statement `query = frozen.Query()` — the byte-preserving thin seam.
Referees: V-FROZEN-NO-OPEN-PORT / V-OWNERSHIP-ACYCLIC at freeze + the
ALWAYS-ON V-REGION-CENSUS recount at the ValidateDROps tail (stored census ==
`DeriveRegionalCensus(query)` re-derived fresh — a stubbed planner aborts,
corpus-wide). Dumps: `-region-out` (G1, ratified D2.2) + `-region-dot-out`
(advisory DOT twin, never goldened); 20 `.region.<mode>` goldens via the
`region` `.irgold` surface (demand_tc_witness, join_1, merge_2,
demand_multi_adorn_witness, tc_nonlinear_diff × 4 modes — byte-identical
cross-mode at tip but pinned PER-MODE; cross-mode identity is not claimed;
the region emitter's member-key column width is a PER-DUMP MAX, so a new
longest contract re-pads existing lines — E-K5-PAD, never assume
byte-additivity when predicting region golden deltas). Authority docs:
stage-b-diff.md AMENDMENTS, regional-arch-pseudocode.md Part B,
regional-dump-stage-b-desired-states.md §9/§9.7, stage-b-landed-seed.md.

## DIFF-R3: Tier-1 interior naming + the `@key` surface — [semantics REMOVED at the P1 cut; @key is now inert parsed metadata, parse-surface only — historical]

RATIFIED POLICY (RP-1..10, region-model-diffs.md session-5 AMENDMENTS + the
session-6 @DEMAND section + the SESSION-7 RATIFICATIONS): HINT-NOT-MANDATE —
the declared key NEVER drives
or constrains the lowering (R3b is DEAD); UNPROVABLE/UNREALIZABLE REJECTS
(mismatch, unseeded, undemanded — all stable hard errors, never
warn-and-accept); Minimize/DeterminedBy is the future provability WIDENING
(O-R3.5); the old `@key`-for-FD reservation DISSOLVED by convergence (RP-9: the Minimize-backed key proof and the instance key are ONE concept);
RP-10 (K1, session 7): a multi-adornment relation declares its keys by
PRAGMA REPETITION under TOTAL BIJECTION — declared set-of-sets ==
SIP-inferred set-of-sets, order-free, never subset coverage. The RP-8
auto-sweep STOP is RESOLVED strict-for-now (no goldens moved; best-effort
re-opens only on an explicit owner call).

THE SURFACE (RP-5/RP-7/RP-9/RP-10): `#local rel(u64 A, u64 B) @key(A)
@key(B).` — post-parameter-list pragmas on `#local`/`#export` ONLY
(declaration-level; a
per-clause key has no semantic referent — one keyed store per adornment).
Parsed in the ParseLocalExport pragma tail with IMMEDIATE arg resolution
(params are bound at the pragma site); named + duplicate-free + known
columns enforced per pragma; N pragmas per decl since K1 (storage
`instance_key_param_index_sets`, accessor `InstanceKeys()`; a REPEATED
column SET across pragmas is a parse-time order-free reject —
reject_key_double_1 pins it WITH a bound query so dropping the check is a
LOST CHECK, not a masked one); Step 2b V-DECLARED-KEY is the two-arm
RP-10 bijection (Arm A declared-surplus / Arm B inferred-surplus,
named-set diagnostics; N=1 byte-compatible with the retired single-set
check); the retired `rel[K...]` bracket draws a
pointed redirect diagnostic (the bracket LEXEMES remain, parser-unconsumed;
`rel[Bound](Free)` stays design-doc/dump notation). The decl formatter
round-trips all N pragmas. `-contract-out` emits ONE `declared-key` line
per set (written-pragma order, always-on no-match belt);
`key_multi_adorn_witness` is the flagless N=2 witness (stdout SYMLINK to
demand_multi_adorn_witness = nested-vs-flat answer identity; own rel.opt
golden pins `kSubgraphInstantiate=2`; own contract.opt golden byte-locks
the two-line declared-key pairing). Working authority: k1-multikey.md
(Parts A-C + the 12-finding panel record).

ACTIVATION (RP-6/RP-8): `@key` is a flagless FORCE-OPT-IN — the demand
transform runs for a pragma-bearing module with NO `-demand` flag, strict
(every fence + V-DECLARED-KEY applies; rejects say "fix or remove the
@key pragma", never "recompile without -demand" — dropping the flag
would not deactivate the pragma). The activation gate scans the PARSED
module (a #local's flows are proxied out of `relations` by Connect — the
decl is the durable carrier); a module with no pragma and no flag
short-circuits before any walk (the containment gate: the pragma-free
corpus is byte-identical). REJECTS: an @key with NO bound query
(unseeded), or on a relation that is NOT the demanded target (inert pragma
= silent lie). LOWERING (RP-9 fallback arm): `@key` SELECTS the nested
keyed-instance lowering where every forcing admits it (the
key_neighborhood_witness pin: flagless kSubgraphInstantiate=1,
byte-identical to the -demand -demand-instance compile of the pragma-free
twin) and falls back SILENTLY to the flat guard web where not (recursive
shapes — key_tc_witness); `-demand-instance` stays the STRICT override
whose fences remain diagnostics. `-demand` remains the GLOBAL AUTO layer ("try auto-demand
after the user-specified ones"); with R-1BOUND it composes trivially today;
whether the auto sweep stays STRICT or becomes BEST-EFFORT (fence -> skip,
not reject) is an OPEN owner STOP. V-DECLARED-KEY sits POST-Loop-1 in
ApplyDemandTransform (the first site the adornment count exists; fences run
first — a pragma never masks a fence): since K1 the two-arm RP-10
set-of-sets bijection (was: strict single-forcing scope + single
declared-set == inferred `p_bound`). `-contract-out` gains the
pragma-scoped `declared-key rel=... declared=(...) inferred=(...)`
line(s), one per declared set. `key_tc_witness` is the ACTIVATION-EQUIVALENCE witness: its
stdout/oracle/monotone/df/rel/ir/h/region goldens are SYMLINKS to
demand_tc_witness's (pragma-activated compile == `-demand`-activated
compile, byte-for-byte; NEVER bless its symlinked surfaces directly),
contract.opt + behavioral are its OWN real goldens (the CBF header embeds
the case name). NO eqgate (recursive TC fences under `-demand-instance`).

TIER-1 NAMING LIFT (session 5, unchanged by the surface swap): the demanded
interior relation (merge-materialized, no INSERT — formerly unnameable)
surfaces as a `-region-out` row-contract. `ConnectInsertsToSelects` records
`insert_proxy → rel->declaration` in a `Query::Build`-SCOPED map (never a
QueryImpl member — the VIEW* keys dangle past Optimize), read once by
`ApplyDemandTransform` (T1-DECL-MISS aborts on a miss) into
`RecognizedSubgraph::demanded_decl` (parse identity, Optimize-stable). At
freeze, interior-contract EXISTENCE + census COUNT are decl-driven and
resolve-free (distinct `demanded_decl` Ids, forcing order, deduped against
insert-derived decls — multi-adornment surfaces ONE interior contract);
member-key renders the decl's AllFields positionally; `support=` is the OR
over the forcing's live annotated guard JOINs of `CanReceiveDeletions()` —
ROLE-BLIND (T1-IMPL-1: `PromoteSurvivorToBody` folds a projection guard to
kBody under CSE, so a role-filtered resolve aborts on real corpus cases);
zero live guard JOINs for a counted decl ABORTS the freeze. The 8 demand
`.region` goldens carry `E1 rel=path` / `E1 rel=rel` + census
`row-contracts=2`. Cross-REDECLARATION @key consistency is CHECKED since K6
(identical-or-absent, enforced at the F31-revived consistency site;
order-independent via context-resolving accessors).

TIER-2 ORIGIN PROVENANCE (K5, session 8 — LANDED, the first sanctioned
post-F1 maintained satellite): every live view carries `origin_decls`
(`QueryViewImpl`, sorted-unique by decl Id, `QueryView::OriginDecls()`), the
monotone set of origin declarations whose rows flow through it — SEEDED at
the Connect insert-proxy mint (`!IsInline() && !IsQuery()`-guarded,
seed-once assert), UNIONED at the ONE `CopyDifferentialAndGroupIdsTo` choke
point beside `group_ids` (loser→survivor, NO clear-on-loser, Tigerstyle
sorted+no-adjacent-equal assert pair), NEVER in Hash/Equals, NEVER a
lowering input (the anti-F1 fence: a missed union under-names, never
miscompiles; five documented CDaGI-bypass gaps accepted). Freeze consumer:
`CollectOriginInteriorDecls` (Planning.cpp) names the residue outside
insert-named ∪ Tier-1 as Tier-2 row-contracts (ascending decl Id — the
STRING-POOL INTERN-OFFSET order, NOT declaration order: `edge`
suffix-aliases `add_edge` so tc_nonlinear_diff pins E1=edge, E2=tc),
member-key AllFields positional, `support=` = OR over live carriers'
`CanReceiveDeletions()` (loud-abort belt + DEBUG support-agreement assert;
sound via the differentialness-migration invariant — CDaGI OR-propagates
can_receive_deletions in lockstep). tc_nonlinear_diff (row-contracts 1→3,
the fold-migration witness — its `support=differential` byte, not line
existence, is what a dropped union loses), join_1 (+p,+r) and merge_2
(+outer; `inner`/`proj` @inline-skipped) pin it. Belt: advisory
`-origin-out` (per-view, `(min decl.Id(), det_seq)` order, run_irgold
smoke-produces it, NEVER goldened) + the DEBUG demanded-interior
conservation assert (rescoped, redundant-with-Tier-1 defense-in-depth).
ADJ-R3-C was DROPPED as moot post-RP-6 (every accepted @key'd relation is
demanded); D2.9(β) proxy-role-at-mint DEFERRED (mechanistically disjoint).
Working authority: k5-provenance.md (Parts A-E + the 17-finding panel
record; Part E's byte-diff method is the region-golden-prediction
precedent). The key-SUBSET covering-array fuzz arm (ORDER dropped as
inert) is the ranked-next follow-on.

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
