# Session-31 seed — S1b: make the demand seed FLOW, then witness + bench the pruning

> **SUPERSEDED (session 31 close).** The owner steered away from finishing the flat back-end
> ("stop pussyfooting with intermediate flat approaches — boldly move into the target feature space").
> The flat injector/witness/bench polish below is DEPRIORITIZED; the target is now KEYED INSTANCES
> directly. See `session-32-keyed-instances-seed.md` + `session-32-prompt.md`. Kept for the flat-arm
> anchors (injector site, witness recipe) only.

> Written at the S1a close (2026-08-12, branch `keyed-instances`, tips `577c47dc`/`ca31026e`).
> S1a resurrected the FLAT `-demand` transform and proved it is orthogonal (flag-off byte-identical,
> SUITE PASS 227, ctest 5/5) AND that a `-demand` program compiles END-TO-END through the whole
> pipeline. S1b turns that compiling-but-not-yet-seeded pipeline into an ANSWER-CORRECT, bench-proven
> pruning win. Read `session-30-prompt.md` + `session-29-s1a-restoration-manifest.md` (executed) +
> `session-29-grounding.md` (the evidence/scope) + memory `regional-dataflow-core-epoch` (s30 head) first.

## §0 What S1a settled (do NOT re-litigate)
- FLAT `-demand` is LIVE, flag-ONLY (RP-6 `@key` activation deferred; `@key` inert flag-off). Restored
  from `6d6248a2` (tip-K1-@key-API + Mint tags). `Query::Build` is 6-arg. `bin/Oracle` pins
  `suppress_demand=true`.
- A `-demand` recursive-TC program (`#message edge_2; #local path; path:-edge_2; path:-path,edge_2;
  #query reachable_from(bound From, free To):-path`) COMPILES end-to-end. Verified dumps:
  `-df` shows guard JOINs (`^join.14/15/16`), `demand__reachable_from_bf/1` receive, demand relation,
  `tag=demand/*` views; `-region-out` shows `input-ports=1` (demand msg internal, §6a), `request-port
  P2 query=reachable_from bound=(From)`; codegen suppresses the public demand entry point (only
  `demand__reachable_from_bf_1_detail` survives, §7).
- The four always-on abort-validators (RowContract/ProjectionRole, K5 origin_decls, Rel eager-web,
  Regional census) all PASS on the demand graph.

## §1 THE LOAD-BEARING GAP S1b must close FIRST: the demand seed does not yet FLOW
S1a compiles the demand graph but **does not auto-seed demand at query time** — the ControlFlow
injector was deliberately deferred (manifest step 7). Consequence today: issue the bound `#query`
`reachable_from(K)` and the demand relation `d_path` is never populated → `path` materializes nothing
demanded → **wrong (empty) answers**. This is the S1b headline.

- **The scaffold SURVIVED P1** (grounding §4, re-verify at tip): `BuildQueryForceProcedureImpl` /
  the forcer-proc path in `lib/ControlFlow/Build/Build.{h,cpp}`, and `ForcingMessage()`
  (`Parse.h`). What was DELETED is the F2 **registry** wiring: for a demand-transformed query there is
  no parse-level forcing predicate, so the injector must be built from `Query::DemandForcings()`
  (the registry the S1a transform populates) instead of the clause-var DisjointSet re-derivation.
- **Work (manifest step 7, now the S1b entry):** `BuildQueryInjectorFromRegistry` + the registry-lookup
  loop + a `context.demand_forcings` assignment so a bound `#query`'s injector proc seeds
  `demand__reachable_from_bf` from the query's bound argument. Re-verify the F2 vocabulary against tip
  (`QueryDemandForcing{query, message, bound_params}` is populated in `Demand.cpp` STEP 10;
  `Query::DemandForcings()` reads it). The sibling `BuildQueryForceProcedureImpl` is the pattern to
  clone.
- **DO NOT** resurrect `Program::Build`'s `bool demand_instance` 5th param (InstanceStore = S2+). Keep
  the 4-arg signature.

## §2 The recursive-TC witness (answer-correctness gate; needs §1 first)
Recipe (precedented — `demand_tc_witness.*` @`6d6248a2`, restore/adapt, DO NOT blindly copy goldens —
re-bless against the S1a-tip binary after §1 lands):
- `.dr` = the TC shape in §1 (one bound query, NO all-free sibling, From-preserving recursion — the
  single-adornment slice). Header states the equality obligation: *the driver's demand-ON answers for
  each probed key must be exactly the oracle's rows for that key* (result-equality, per-key/per-epoch).
- `.drflags` = `-demand`. `.probes` = the demanded keys. `.main.cpp` driver = demand-then-probe (send
  `demand__reachable_from_bf` seed for each probed key, drain, assert answer == neighborhood(key)).
- `.batches` + `oracle`/`monotone`/`behavioral` goldens: the demand-BLIND referees (RefInterp / oracle /
  behavioral, all compiled from the PLAIN program) pin the DEFINITIONAL per-key closure; the demand
  binary is refereed by its own blessed `.stdout` + 4-mode byte-agreement + the human bless bridge
  (grounding §6 — NOT a disjoint byte-check of demand-vs-full). `runall.sh`/`diffrun.sh` `.drflags`
  mechanism is already generic; no harness change.
- The `kPushDown` SIP arm is the CODED path for this shape (the recursive `path:-path,edge_2` body —
  its bound `From` comes out of a JOIN one input of which is a full-width reader of `path` itself,
  `IsFullWidthReaderOf`). Confirm the emitted TABLEJOIN/fixpoint actually drives from the demanded
  frontier (the whole point). This is the fixpoint-interior guard lowering "exercised end-to-end".

## §3 The bench carrier (the namesake pruning, MEASURED in-compiler)
The grounding's O1 spike measured the win with a HAND-staged graph (compiler unmodified). S1b must
reproduce it with the REAL transform (grounding §7 "gate the build on an in-compiler spike"):
- A large SELECTIVE recursive TC (the s29 spike dataset: ~4000-distinct-`From` edges, selective query).
  Expect `idx_hops` 40000→~11, `finds` 3.5–7×, answer byte-identical; and confirm the +40% non-selective
  REGRESSION (→ demand stays cost/mode-gated, never a 5th golden mode).
- Use `bench/runbench.sh` (never concurrent with a suite run; never rebuild the compiler mid-run). The
  counter seam is `-DDRLOJEKYLL_BENCH_COUNTERS`. Compare on (case, mode, knobs) semantics.
- This is the empirical proof that the in-COMPILER emission (not the hand twin) drives from the
  restricted side (grounding §3: the spike's `idx_hops=11` was the hand-`sd` graph, not the
  rewired-JOIN graph — VERIFY the real one prunes).

## §4 S1b exit gate (all)
1. Build green; the recursive-TC demand program yields **result-equal** answers ×4 optimization modes
   (per-key == oracle) — the injector (§1) FLOWS the seed.
2. Codegen goldens **MOVE** for the witness (the demand shape emits guard JOINs / demand table / the
   fabricated ingest — census `kIngestFold`/`kEagerJoin`/`kJoinEmit` shift, `datalog.h` grows; grounding
   §2 measured 245→322 lines on the hand twin). Bless the witness goldens (`.stdout`/oracle/monotone/
   behavioral/`.rel`/`.ir`/`.h`/`.region`) against the S1a-tip binary.
3. Bench shows the selective pruning (§3) + the non-selective regression (the cost-gate justification).
4. OptDiff **SUITE PASS** still byte-identical for the flag-off corpus (the new witness is `-demand`
   via its `.drflags`, orthogonal to the 4 modes).

## §5 Then S1c (bless + ABI + census polish) / S2 (InstanceStore) — owner re-ranks
- §6a/§7 ABI+census already landed in S1a. S1c is mostly the witness/bench bless + the Tier-1 naming
  lift (manifest §6a Part 2, DEFERRED — `CollectDemandInteriorDecls`/`ResolveInteriorSupport` were never
  re-added post-P2/P3/K5; `-region-out` UNDER-NAMES demanded interiors until re-implemented against the
  new typed model, `BuildRelationSchemaFromInterior` parallel to `BuildRelationSchemaFromOrigin`). NOT a
  correctness blocker.
- S2+ = the keyed InstanceStore / `-demand-instance` nested lowering (still fully removed; the vestigial
  `DRInstance`/`demand_table` scaffolding in `lib/Rel/Rel.h:855-890` is reserved for it).

## §6 Anchors (re-verify at next tip — the pipeline drifts)
- Transform: `lib/DataFlow/Demand.cpp` (`ApplyDemandTransform`, gate at head `if (!demand_mode) return
  true;`; SIP arms `kBaseAtom`/`kPushDown`; STEP 10 forcing registration). `lib/Parse/Demand.cpp`
  (Fabricate*).
- Injector target: `lib/ControlFlow/Build/Build.{h,cpp}` (`BuildQueryForceProcedureImpl` sibling +
  the deleted registry loop to re-add). `Query::DemandForcings()`/`GuardAnnotations()`/
  `RecognizedSubgraphs()`/`IsDemandMessage()` in `include/drlojekyll/DataFlow/Query.h`.
- ABI/census (landed): `Database.cpp` 2 `IsDemandMessage` guards; `Planning.cpp` `CollectMessages`
  demand-filter.
- Witness precedent: `git show 6d6248a2:tests/OptDiff/cases/demand_tc_witness.*`.
- O1 spike numbers + dataset: `session-29-grounding.md` §1.
