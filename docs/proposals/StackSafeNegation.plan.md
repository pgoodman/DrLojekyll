# Workflow plan: derivation counters, then delete the tri-state

Companion to `docs/proposals/StackSafeNegation.md` (the design — read it
first; this file is the execution plan and the file-level ground truth for
where every construct lands). Target: a multi-agent workflow session on
branch `derivation-counters`. Repo state at plan time: `main` at `3c1ed1c`
(unit-conditions refactor landed, F1–F15 resolved), ctest 3/3, ~128-case
OptDiff suite 4-mode green.

## Mission

1. Replace `TupleState`/`kUnknown` and every top-down checker with split
   signed derivation counters (`C_nr`/`C_r`) on differential tables and
   per-stratum OVERDELETE → REDERIVE → INSERT fixpoints (proposal §3–§5).
2. Delete the entire re-proof universe in the same landing: the tri-state
   transitions, `BuildTopDown*Checker`, the eager removal cascade, the
   negation coupling, the induction recheck/replay machinery, the publish
   flusher's checker branch, and query-cursor checker filtering (§6).
3. Ground `DerivClass` in the existing `InductionInfo` machinery — never
   duplicate SCC discovery (resolved below, Q3).

## Ground rules for every agent

- Build: `cmake --build --preset debug`; test: `cd build/debug && ctest
  --output-on-failure`; one case: `DR=build/debug/bin/drlojekyll TIMEOUT=120
  tests/OptDiff/diffrun.sh cases/<n>.dr cases/<n>.main.cpp <wk>`; full suite:
  `DR=build/debug/bin/drlojekyll tests/OptDiff/runall.sh <workroot> [jobs]
  [filter]` must end `SUITE: PASS`. macOS bash 3.2: no `declare -A`; clangd
  diagnostics are noise — only the real build counts.
- **No dual code paths across stage boundaries.** Stage 3 is one landing;
  its internal checkpoints may be transiently mixed (see the honesty note
  there), but the branch merges as a single change with the full §6
  deletion inventory applied and grep-verified.
- Existing goldens are never modified except via the single reviewed
  `--bless` that the Stage 0 publish audit authorizes (if it authorizes
  any). Never bless to make a red case green.
- Debug crash triage: assert+SIGABRT = exit 134; stack overflow/null deref =
  exit 139; `lldb -b -s <script> -- <cmd>` for backtraces.

## Workflow shape

    Stage 0 (sentinels+audit; CONCURRENT with Stage 1)
        │
    Stage 1 (Stratify) ──► Stage 2 (oracle) ──► Stage 3 (the swap, one landing,
        │                                        checkpoints a–e)
        └── audit results force §11 Q1/Q11 owner decisions before Stage 3
                                                      │
    Stage 5 (@product gap) ◄── Stage 4 (docs+invariants) ◄┘

After each stage: an independent verifier agent rebuilds from scratch, runs
ctest + the full suite, and greps the stage's deletion manifest — surviving
mentions outside docs are failures.

---

## Resolved architecture questions

These three questions gate the implementation; they are resolved here with
file:line evidence (verified against the tree at plan time) so no Stage 3
agent has to re-derive them.

### Q1 — Where every piece lives

**`Table`/`DiffTable` split** — `include/drlojekyll/Runtime/Table.h`.
Today: `TupleState` at `:16–20`, the five `TryChange*` methods at `:97–119`,
`TryPromote`/`TryTransition` at `:122–152`, the `states` array at `:210`.
(The proposal's `Table.h:14–152` citation is accurate.) The whole file is
rewritten per §3.1: monotone `Table` (no state array) and `DiffTable`
(packed `int32×2` counts + flags byte + `touched` vector). `Index`
(`:226–334`) is untouched. The flavor is chosen per data model at codegen
time by `QueryView::CanReceiveDeletions()`; the generated member
declaration site is `lib/CodeGen/CPlusPlus/Database.cpp:856`
(`::hyde::rt::Table<Row>`).

**New region kinds `UPDATECOUNT` / `CHECKMEMBER` / `COMMITSWEEP`** — thread
through exactly the sites CHANGETUPLE threads through today (enumerated by
tracing `ProgramChangeTupleRegion`/`AsChangeTuple`):

1. `include/drlojekyll/ControlFlow/Program.h` — forward decl (`:44` block),
   `ProgramRegion` converting constructor (`:77` block), friend decl
   (`:133` block), `ProgramRegion::Is*` predicate, public wrapper class
   (CHANGETUPLE's is `:616–643`), `ProgramVisitor::Visit` overload
   (`:1216` block).
2. `lib/ControlFlow/Program.h` (internal) — `As*` virtual on the OP base
   (`:533` block), impl class (CHANGETUPLE's is `:824+`).
3. `lib/ControlFlow/Operation.cpp` — destructor (`:26`), base-class `As*`
   default (`:121–123`), derived `As*` override (`:660`), `Hash` (`:665`),
   `IsNoOp` (`:690`), `Equals` (`:695`), `EndsWithReturn` (`:739`),
   `MergeEqual` (`:747`).
4. `lib/ControlFlow/Program.cpp` — `OPTIONAL_BODY` (`:299–300`), `FROM_OP`
   (`:330`), `USED_RANGE` (`:382`), accessors (`:504–516`), and the generic
   body walk at `:58`.
5. `lib/ControlFlow/Visitor.cpp` — default `Visit` stub (`:12` macro list).
6. `lib/ControlFlow/Format.cpp` — `operator<<` printer (CHANGETUPLE's at
   `:470`) and the `MAKE_VISITOR` dispatch table (`:860` block). The
   CHECKMEMBER printer MUST name its membership predicate (the §3.3
   auditability requirement for `-ir-out`).
7. `lib/ControlFlow/Optimize.cpp` — an `OptimizeImpl` overload (CHANGETUPLE's
   at `:1062`) plus the op-kind dispatch chain (`:1489` block).
   UPDATECOUNT gets CHANGETUPLE's dead-body treatment (crossing body
   removable when no-op, region NOT removable — the fold has effects);
   CHECKMEMBER gets TUPLECMP/CHECKTUPLE's dead-branch treatment.
8. `lib/ControlFlow/Analyze.cpp` — the CHANGETUPLE consumers at `:219`,
   `AnalyzeTable(TABLE*, CHANGETUPLE*)` `:711`, `:736`, and
   `ConvertTablesToRecords` `:946` must learn UPDATECOUNT/CHECKMEMBER (or
   the record-conversion analysis is restricted to monotone tables —
   owner decision at checkpoint (a); default: teach it).
9. `lib/ControlFlow/Build/Procedure.cpp` — `FixupContainingProcedure`'s
   failure/alternate-body arms (`:381–397`).
10. `lib/CodeGen/CPlusPlus/Database.cpp` — `WalkRegion` dispatch (`:479`
    block), `EmitRegion` dispatch (`:996` block), emitter declaration
    (`:264` block), emitter definition (`EmitChangeTuple` is `:1116–1181`;
    its state-pair→method mapping at `:1137–1181` becomes
    `EmitUpdateCount`/`EmitCheckMember`/`EmitCommitSweep`).

**Deleted IR forms and their consumer sites**: `MODESWITCH`
(`Program.h:467–488`, `Optimize.cpp:10–45`, `Database.cpp:474,949`, built at
`Build/Induction.cpp:233,914` and `Build/Build.cpp` removal walks);
CHECKTUPLE's 3-way form and `kAbsentOrUnknown` (`Program.h:695–711`,
`Procedure.cpp:387–389`, `Database.cpp:1233` state switch);
`ProcedureKind::kTupleFinder` (`Program.h:968–1000` enum,
`Database.cpp:197–198` `find_` naming); `VectorKind::kInductionRechecks`
(`Program.h:242`); `ProgramQuery::tuple_checker` (`Program.h:1120–1134`,
consumed at `Database.cpp:1734–1769`).

**Stratify pass** — new file `lib/DataFlow/Stratify.cpp` (+ entry in
`lib/DataFlow/CMakeLists.txt` source list, `:13ff`), method declared on
`QueryImpl` in `lib/DataFlow/Query.h` (next to `TrackDifferentialUpdates`,
`:1028`), public accessors in `include/drlojekyll/DataFlow/Query.h`. Call
site: `Query::Build` in `lib/DataFlow/Build.cpp`, immediately after the
final `TrackDifferentialUpdates(log, true)` at `:2587` — i.e. after
`IdentifyInductions` (`:2580`) so the UNION-injection rewrite has settled
and `merge_set_id` is available for cross-checking, and after differential
tracking so `CanReceiveDeletions` is final. (The earlier
`TrackDifferentialUpdates` call at `:2552` and the re-runs at
`Connect.cpp:286` / `Optimize.cpp:540` are pre-optimization and do not get
a Stratify call.) The INSERT→SELECT seams Stratify must traverse are the
ones `TrackDifferentialUpdates` builds as `insert_to_selects`
(`lib/DataFlow/Differential.cpp:16–41`) — factor that enumeration into a
shared helper.

**Codegen emitters** — all in `lib/CodeGen/CPlusPlus/Database.cpp`: the new
`EmitUpdateCount`/`EmitCheckMember`/`EmitCommitSweep`; the induction
re-entry loop deleted at `:1678–1699` (the `for (bool reenterN...)` wrapper;
the inner `while (!vectors empty)` fixpoint loop survives as the
OVERDELETE/INSERT round loop).

**§6 deletion-inventory citation audit** — all verified, two spans drifted
slightly: `BuildUnknownRecheck` is `Build/Induction.cpp:599–680` (the
4-phase snapshot/output/clear/reseed SERIES is assembled at `:727–741`),
not `:599–800`; the PRODUCT checker double-check spans
`Build/Product.cpp:214–232` (the `CallTopDownChecker` calls start at
`:223`), not `:212–223`. Everything else is line-exact:
`InTryMarkUnknown` `Build.cpp:1566`, `BuildEagerRemovalRegionsImpl`
`Build.cpp:1605`, `BuildTopDownChecker` `Build.cpp:408` (+ the
`BuildTopDownCheckers` driver `:780`), `GetOrCreateTopDownChecker`
`Build.cpp:1258`, `CallTopDownChecker` `Build.cpp:1361`, per-node checker
decls `Build.h:431–519`, `view_to_top_down_checker` `Build.h:150`,
`top_down_checker_work_list` `Build.h:168`, remover dispatchers
`Build.cpp:202–246` and `:1857–1902`, `PivotAroundNegation` `Build.cpp:269`,
`BuildMaybeScanPartial` `Build.cpp:312`,
`MaybeRemoveFromNegatedView`/`MaybeReAddToNegatedView`
`Build.cpp:339,:365,:390–405`, flusher vectors `Procedure.cpp:229–231`,
checker branch `Procedure.cpp:295–330`, message-output appends
`Insert.cpp:52–63`, `Runtime/Table.h:14–152`, `Program.h:242`, `:1120`.

### Q2 — How the dataflow→control-flow conversion changes

**Today's shape.** Per-message `kMessageHandler` procedures
(`Procedure.cpp:429–493`) each call the single `kEntryDataFlowFunc`
(`:667`) passing their add vector (+ remove vector iff differential,
`:451–456`) and empty vectors for every other message. The entry proc runs
eager insertion regions for adds; removals loop their vector under a
`MODESWITCH(kBottomUpRemoval)` into `InTryMarkUnknown` + the
`BuildEagerRemovalRegionsImpl` cascade. Inductions are INDUCTION regions
with one shared add/remove vector per view (`kInductionInputs`,
`GetOrInitInduction`, `Induction.cpp:1092`), double-buffered through
`kInductionSwaps` (`:1102`, swapped by `BuildInductiveSwaps` `:98–147`),
polarity dispatched at dequeue by a CHECKTUPLE on the row's state
(`BuildFixpointLoop` `:201–245`). `ExtractPrimaryProcedure` (`:505`) then
splits entry (message-vector reads, pre-induction joins) from
`kPrimaryDataFlowFunc` (everything else), and
`PublishDifferentialMessageVectors` (`:238–350`) appends the flusher.

**After the swap, one received batch's procedure pair, region by region:**

    kEntryDataFlowFunc(msg vectors):
      [ingest] per differential message: net adds against removes (§5.0),
        then AddExplicit/SubExplicit UPDATECOUNTs whose crossing bodies
        append row ids to delQ[stratum]/addQ[stratum] vectors
        (replaces InTryInsert/InTryMarkUnknown at receive sites);
        monotone receives keep today's TryAdd + eager vectors verbatim.
    kPrimaryDataFlowFunc(delQ/addQ per stratum):
      SERIES over strata in topological order:
        monotone stratum: today's eager insertion regions, unchanged.
        differential stratum: SERIES [
          OVERDELETE:  seed delta joins (per §5.1 seed schema, delta over
                       lower strata's D\A / negated A\D frontiers) emitting
                       UPDATECOUNT(−, class);
                       then the fixpoint loop: TryClaimDel drain of delQ,
                       fixpoint-schema joins over Δ_D emitting
                       UPDATECOUNT(−, kRecursive), RetireDelFrontier —
                       structurally today's induction vector skeleton
                       (clear/unique/swap/loop) with the CHECKTUPLE polarity
                       dispatch replaced by CHECKMEMBER gates per the
                       OVERDELETE read-schema column.
          REDERIVE:    one VECTORLOOP over D_s with
                       CHECKMEMBER(RecursivelySupported) → VECTORAPPEND addQ.
          INSERT:      seed delta joins (+) then the fixpoint — today's
                       induction fixpoint with CHANGETUPLE→UPDATECOUNT(+)
                       and state gates→CHECKMEMBER (§5.3, verbatim claim
                       from the proposal, verified structurally sound).
          BUILDFRONTIERS: consolidated D\A / A\D vectors per table.
        ]
      COMMITSWEEP per differential table (publish net changes).
      monotone-backed @differential messages: kMessageOutputs sort-unique +
        publish, byte-for-byte today's path (Procedure.cpp:238–277 +
        Insert.cpp:52–63), checker call deleted.

**Survives with substitution:** `InTryInsert` (insertion-site persistence,
`Build.cpp:~1520–1556`) becomes the UPDATECOUNT(+) emission point;
`BuildEagerInsertionRegionsImpl` (`:1654`) and the per-node
`BuildEagerRegion` walk; the induction vector machinery in toto —
`GetOrInitInduction` (`Induction.cpp:927–1124`) allocating
`kInductionInputs`/`kInductionSwaps` (`:1092/:1102`; these become
delQ+addQ pairs per stratum — two claim polarities need two queue pairs),
`BuildInductiveSwaps` (`:98–147`), `BuildFixpointLoop(for_add=true)`
(`:149–434`), `AppendToInductionInputVectors` (`:1143–1254`),
`kInductionOutputs` (`:1117`) repurposed as the per-stratum claim
accumulators `D_s`/`A_s` that REDERIVE and BUILDFRONTIERS read;
`BuildMaybeScanPartial` (`Build.cpp:312`) as the §3.4/§5.4 index-request
path; `kMessageHandler`/`ExtractPrimaryProcedure`/forcing-function
procedures (`Build.cpp:871`, now running a complete mini-epoch, §5.5).

**Deleted:** `BuildEagerRemovalRegionsImpl` + both remover dispatchers +
every `CreateBottomUp*Remover` (per-node files); `InTryMarkUnknown`; the
whole checker web (`BuildTopDownChecker`/`GetOrCreate`/`Call` + 8 per-node
variants + `Context` fields); `BuildFixpointLoop(for_add=false)` and the
dequeue CHECKTUPLE/MODESWITCH polarity dispatch (`Induction.cpp:201–245`);
`BuildUnknownRecheck` (`:599–680`) + the snapshot/reseed phases of the
4-phase output SERIES (`:727–741`); the output-loop checker call
(`:494–521`); codegen's re-entry loop (`Database.cpp:1678–1699`);
`MaybeRemoveFromNegatedView`/`MaybeReAddToNegatedView`/`PivotAroundNegation`
(`Build.cpp:269–405`); the PRODUCT insert-path double-check
(`Product.cpp:214–232`); the flusher checker branch
(`Procedure.cpp:295–330`); query-cursor checker filtering
(`Database.cpp:1734–1769`).

**New builders:** ingest netting; OVERDELETE seed + fixpoint emission (the
`−` mirror of the insert skeleton, one generated pattern per rule per §5.1
— the single point of failure, guarded by Stage 0/2); the REDERIVE loop;
BUILDFRONTIERS; the NEGATE crossover joins (§5.4, reusing the
`BuildMaybeScanPartial` index request); COMMITSWEEP emission; the
stratum-ordered SERIES skeleton replacing the induction work-item
`merge_depth` ordering (`ContinueInductionWorkItem` order keys,
`Induction.cpp:36–42`, generalize to stratum order).

### Q3 — DerivClass grounded in InductionInfo (counting within vs straddling an SCC)

**(a) `merge_set_id` vs Tarjan SCC condensation.** `IdentifyInductions`
(`lib/DataFlow/Induction.cpp:99–728`) seeds only MERGE/JOIN/NEGATE nodes
(`:135–143`), keeps those that transitively reach themselves (`:150–158`),
and unions two inductive views iff they are *mutually* transitively
reachable (`reached_inductions` at `:171–180`, union at `:194–202`) — that
is exactly the SCC equivalence relation, so `merge_set_id` (`:517–536`)
coincides with SCC condensation **restricted to inductive
MERGE/JOIN/NEGATE nodes**. Membership is deliberately partial: (i)
TUPLE/CMP/MAP/SELECT views interior to a cycle never get an
`InductionInfo` at all; (ii) JOIN/NEGATE nodes wholly inside back-edges
(no noninductive predecessors AND no noninductive successors) get their
info reset (`:438–444`). So view-in-SCC-but-not-in-any-merge-set is the
NORMAL case for interior plumbing views, and Stratify MUST run its own
Tarjan over all views (column edges + `negated_view` edges — use the
`ForEachPredecessorOf` shape at `Induction.cpp:23–32` — + the
INSERT→SELECT seams). The cross-check invariants Stratify asserts: every
nontrivial SCC contains ≥1 inductive MERGE (all dataflow cycles pass
through UNIONs — `Query.h:214–216`); `merge_set_id` is constant within an
SCC and distinct across SCCs; every `InductionInfo`-bearing view's
inductive/noninductive predecessor split (masks at `Query.h:233–234`,
upgraded to use-lists at `:412–434`) agrees with same-SCC membership of
the predecessor. Stratify runs after `IdentifyInductions`' UNION-injection
rewrite (`:280–406`) has settled, so it sees the normalized graph.

**(b) Reading DerivClass off the existing machinery.** One correction to
the informal framing: **nothing ever appends to `kInductionSwaps`** — both
outside rows and back-edge rows are appended to the SAME `kInductionInputs`
vector (`view_to_add_vec`), and `kInductionSwaps` is the double buffer that
`BuildInductiveSwaps` (`Induction.cpp:98–147`) clears and swaps in so each
round loops the previous frontier while refilling the live vector. The
origin split the proposal needs is nonetheless already structural: appends
emitted while `induction->state == kAccumulatingInputRegions` are recorded
in `init_appends_add/remove` (`:1239–1246`) and hoisted into the
INDUCTION's init region (`FindCommonAncestorOfInitRegions`, `:65–94`,
`:699–715`) — these arrive via noninductive-predecessor paths; appends
emitted during `kAccumulatingCycleRegions` are `cycle_appends`
(`:1248–1250`), built exclusively by walking
`merge.InductiveSuccessors()` from inside the fixpoint (`:833`, `:862`) —
these are the back-edge arrivals. `InductiveSuccessors()` /
`NonInductivePredecessors()` are direct reads of `InductionInfo`'s
use-lists (`lib/DataFlow/Query.cpp:1256–1292`). Therefore: **an
UPDATECOUNT emitted at a seed/init position is `kNonRecursive` (C_nr) and
one emitted inside a stratum's fixpoint body is `kRecursive` (C_r)** — the
class is read off the emission position that the existing
`induction->state` machinery already distinguishes, which is itself
equivalent to the dataflow-level rule "deriving view in the same SCC as
the target table's views". Stratify adds only (i) the topological stratum
order over SCCs (replacing `merge_depth`'s partial order, `:538–636`),
(ii) the in-SCC-negation diagnostic, (iii) stratum ids for tables — plus
the DerivClass labels as a cross-check against emission position (debug
assert in the CF build: seed-position emission ⇔ edge labeled
kNonRecursive). One special case to preserve: non-inductive PRODUCT
predecessor vectors alias their swap vector (`swap_vec == vec`,
`:112–115`, `:1064–1074`) — under the new skeleton these are exactly the
frontier vectors that must NOT be cleared between rounds.

**(c) Rows whose incoming support is entirely within the SCC.** Such rows
have `C_nr = 0` permanently — the firewall never protects them. Any in-SCC
decrement down-crosses them (the crossing predicate is post-state
`kInI && C_nr ≤ 0`, regardless of which counter moved), so every deletion
wave that reaches them overdeletes them; REDERIVE's post-quiescence
`C_r > 0` read is their sole survival test, and pure self-supporting
cycles drain to exactly zero. Trace (`p(X) : base(X). p(X) : p(Y),
edge(Y,X).`, edges 1→2 and 2→1, fact `base(1)`; so `p(1)` has C_nr=1,
C_r=1 and `p(2)` has C_nr=0, C_r=1): remove `base(1)` →
`SubDerivation(p(1), NR)` takes C_nr 1→0, crossing fires (post-state
C_nr ≤ 0 despite total counts = 1) → `p(1)` claimed into Δ_D → fixpoint
fires the stopped instance `(p(1), edge(1,2))` → `SubDerivation(p(2), R)`
C_r 1→0, C_nr already 0 → crossed, claimed → fires `(p(2), edge(2,1))` →
`SubDerivation(p(1), R)` C_r 1→0; `p(1)` already claimed (TryClaimDel
dedup). Quiescence: D = {p(1), p(2)}, both with C_r = 0 → REDERIVE
restores neither. Correct: the cycle was self-supporting. Had `base(2)`
also existed, `p(2)`'s C_nr = 1 would have refused the crossing and the
cascade would have stopped dead at it. **K/V framing:** the persistent
per-row state is exactly one 8-byte value maintained by a commutative
signed add — a RocksDB-style merge-operator fold — while the flags byte is
batch-transient scratch exempt from any durability story; `kInI` is the
batch-frozen read snapshot.

---

## Stage 0 — pin behavior and audit publish semantics (CONCURRENT)

Running concurrently with Stage 1 in another session; this plan only
restates its contract (proposal Stage 0). Deliverables this plan depends
on: four sentinel cases green on today's compiler (`deep_chain_retract` at
reduced depth + full-depth driver recorded as the Stage 3 acceptance gate;
`tc_mixed_batch`; `two_hop_phantom`; `negation_flap`), and the publish
audit verdict — the list of goldens (candidates: `cond_diff_flipflop`,
`cond_both_polarities`, `booleans_diff`) that encode flap emission or
flusher publication order, which is the exact `--bless` budget for
Stage 3. **Stage 3 cannot start until the audit verdict is written down.**

## Stage 1 — Stratify pass (DONE)

Pure analysis; zero behavior change; suite byte-identical. ~1 new file.
Landed as `lib/DataFlow/Stratify.cpp` with the deviations and audit results
recorded below (work items S1.1–S1.7 as planned unless noted).

**Deviations from the plan as written:**

- **Call site** is after `BuildEquivalenceSets` at the end of `Query::Build`
  (the final statement before `return Query(...)`), not immediately after
  the `TrackDifferentialUpdates(log, true)` at `Build.cpp:2587`: table
  strata are per data model, and models are built by `BuildEquivalenceSets`,
  which itself runs after both `IdentifyInductions` and the final
  differential tracking. All the plan's ordering requirements still hold.
- **S1.4 diagnostic is a warning, not an error**: the corpus audit found a
  real in-SCC negation (below), so per the stage contract the diagnostic
  demotes to a `std::cerr` warning ("unstratified negation ... removal
  through it is order-dependent") and compilation proceeds through today's
  inductive-negation path. `Stratify` consequently takes no `ErrorLog`.
  Whether it hardens to an error (rejecting `evm_func_parse.dr`) or gets
  Motik-style per-iteration traces is the §11 Q1 owner decision gating
  Stage 3.
- **S1.5**: in addition to `QueryView::Stratum()` / `Query::NumStrata()`,
  a `DerivClass` enum and `QueryView::DerivationClassInto(target)` landed
  in the public dataflow API: `kRecursive` iff the deriving view shares an
  SCC with *any* view of the target's data model (the ∃-member form matters
  because straddling models are common — see audit).

**S1.6 corpus audit results** (168 files: all of `data/` + all 132 OptDiff
cases, debug build, cross-check asserts active; these gate Stage 3):

1. **In-SCC negation: exactly one program.**
   `tests/OptDiff/cases/evm_func_parse.dr:44` (`!type_name_use(NotRParen,
   _)`) and its `data/self_testing_examples/evm_func_parse.dr` original —
   the cycle is `type_name → type_name_use → (negated in)
   fixed_function_head → modified_function_type → type_name`. No other file
   hits (the `negate_*` and `cond_in_induction*` candidates are all clean:
   their negations/conditions read strictly lower strata).
   **§11 Q1 decision (owner, 2026-07-07): reject.** Nothing has ever
   enforced stratification (`docs/Language.md` claims it; no check existed
   before Stratify) — an in-SCC negation flows through
   `GetOrInitInduction` into the induction's cycle vectors and computes
   whatever fixpoint dequeue order produces. That is an operational
   semantics: the programmer, not the engine, carries the convergence
   argument, exactly as in any Turing-complete language — it works until
   it doesn't. The diagnostic hardens to an error at Stage 3; a
   stratified rewrite of `evm_func_parse.dr` is attempted there, and the
   file moves to the feature-gap list if the rewrite fails.
2. **`disassemble.dr` stratifies cleanly** (verified explicitly): its
   NEGATE sits at stratum 14 reading a lower stratum, while the recursive
   `function_instructions` union is stratum 21 — the lower-stratum-negation
   legality witness holds.
3. **Stratum-straddling models are the NORM, not the exception**: 518
   straddle occurrences across the corpus. Universal benign shape: a
   model's members are pipeline stages in consecutive singleton SCCs
   (RECEIVE/TUPLE/INSERT chains). The differential+recursive subset (14
   occurrences, e.g. every `transitive_closure_diff*`, `disassemble`,
   `tc_mixed_batch`, `deep_chain_retract`): a recursive UNION's model also
   holds higher-stratum singleton members (guard TUPLE, MATERIALIZE,
   TRANSMIT) and sometimes lower ones. **In every one of the 62
   recursive-member straddles, the max-stratum member is a singleton, never
   the recursive view** — so Open Question 11's candidate rule "highest
   stratum owns the table's phases" would misassign every such table to a
   downstream singleton stratum instead of the fixpoint. **No model
   anywhere straddles two distinct multi-view strata.** Recommended
   ownership rule for Stage 3 (owner sign-off): *the model's unique
   multi-view stratum owns the D/R/I phases when one exists, else the
   highest member stratum*. `EquivalenceSet::stratum` currently records the
   plan's candidate (max member stratum) plus the straddle list
   (`QueryImpl::stratum_straddling_models`), which together determine
   either rule.
4. **INSERT→SELECT message seams: zero in the whole corpus** (no program
   both publishes and receives the same message), so no SCC is closed by an
   IO seam and the InductionInfo cross-check divergence rule never fires in
   practice.
5. **Cross-checks all pass corpus-wide** (debug asserts, no aborts): mask
   agreement (inductive predecessor/successor ⇔ same SCC), merge_set_id ⇔
   SCC bijection on info-bearing views, every multi-view SCC contains an
   inductive MERGE, and topological edge order.
6. Pre-existing, unrelated: `data/self_testing_examples/evm_array_parse.dr`
   and `evm_func_parse.dr` abort (exit 134) under `-cpp-out` on `#foreign`
   types with no C++ representation — reproduced on `main`; the control-flow
   build is clean.

Gate results: ctest 3/3; full OptDiff suite `SUITE: PASS` (all 132 cases ×
4 modes, incl. the four Stage-0 sentinels); goldens byte-identical (zero
golden modifications).

Original work-item list (for reference; all done, with the deviations
noted above):

1. **S1.1 SCC + strata** (`lib/DataFlow/Stratify.cpp`, new): Tarjan over
   all views; edges = predecessor column edges + `negated_view` edges
   (reuse the `ForEachPredecessorOf` shape, `Induction.cpp:23–32`) +
   INSERT→SELECT seams (factor `insert_to_selects` out of
   `TrackDifferentialUpdates`, `Differential.cpp:16–41`, into a shared
   helper on `QueryImpl`). Outputs stored on the IR: `stratum` id per view
   (new field on `QueryViewImpl`, `lib/DataFlow/Query.h`), topological
   order over SCC condensation. Method decl next to
   `TrackDifferentialUpdates` (`Query.h:1028`); registered in
   `lib/DataFlow/CMakeLists.txt`.
2. **S1.2 call site** (`lib/DataFlow/Build.cpp`): invoke after `:2587`
   (post-`IdentifyInductions`, post-final-`TrackDifferentialUpdates`);
   error-count check after it like its neighbors.
3. **S1.3 cross-check asserts** (debug builds): the three Q3(a) invariants
   (merge_set_id ⊆ SCC bijection on nontrivial SCCs; mask agreement;
   every-cycle-has-a-MERGE).
4. **S1.4 in-SCC-negation diagnostic**: for each NEGATE, if
   `stratum(negated_view) == stratum(negate)` → clean error in the style
   of the CF feature-gap block (`Build/Build.cpp:2004–2033`), but emitted
   from the dataflow side so all four modes agree. Wording per proposal
   §4. `disassemble.dr` must stratify cleanly (its `!function` negates a
   lower stratum — the legality witness).
5. **S1.5 public accessors** (`include/drlojekyll/DataFlow/Query.h`):
   `QueryView::Stratum()`; `Query::NumStrata()`. DerivClass itself needs
   no accessor — the CF build computes it as
   `stratum(deriving view) == stratum(table's views)` (Q3(b)).
6. **S1.6 corpus audit** (gate deliverable, written into the proposal's
   Open Questions 1 and 11): run the full corpus + `data/` and report
   (i) every in-SCC-negation hit (candidates: `negate_*`,
   `cond_in_induction*`); (ii) every TABLE whose data-model views land in
   different strata (audit runs as a temporary check where the CF build
   merges models — `DataTableImpl::GetOrCreate`, `lib/ControlFlow/
   Data.cpp:146` — reported, not asserted). Any hit forces the §11 Q1/Q11
   owner decisions before Stage 3 starts.
7. **S1.7 format** (`lib/DataFlow/Format.cpp`): annotate views with their
   stratum id in `-dr-out`/dot output (debug aid; OptDiff compares program
   stdout, so this is invisible to goldens).

Gate: ctest 3/3; full suite `SUITE: PASS` all 4 modes, goldens
byte-identical; audit report exists; `disassemble` stratifies clean.

## Stage 2 — reference counting oracle (DONE)

Landed: `bin/Oracle/Main.cpp` (target `drlojekyll-oracle`, registered in
`bin/CMakeLists.txt`; links DataFlow/Display/Lex/Parse/Util only — it
interprets the dataflow IR, never the control-flow IR); 24
`tests/OptDiff/cases/<name>.batches` sidecars transcribed from the
differential drivers; the oracle execution step in `runall.sh` (any case
with a `.batches` sidecar runs
`drlojekyll-oracle <case.dr> <case.batches>`, byte-compared against
`goldens/<name>.oracle.stdout`; `--bless` promotes oracle stdouts; the
oracle binary is auto-built if missing; `ORACLE=` env override); 24 oracle
goldens blessed after semantic review.

**Deviations from the plan as written:**

- **S2.1**: the `.batches` DSL is ops-only (`batch … end` blocks of
  `+/-msg values`; one block = one §5.0 epoch). No `commit`/`query`
  directives and no driver-side replay helper: sidecars are transcribed
  from the drivers (adopted owner decision 4 below), the oracle prints its
  own canonical final relation dump, and interleaved driver dumps remain
  covered by the compiled golden while the oracle's per-batch
  incremental-vs-from-scratch assertions referee every intermediate state.
- **S2.2**: the oracle builds the UNOPTIMIZED dataflow
  (`Query::Build(…, optimize=false)`) for maximal independence from the
  optimizer, and materializes per VIEW (finer than per-table; data-model
  merging is deliberately not modeled, so it does not referee decision 2
  directly — a Stage 3 ownership bug still surfaces as a view-level
  presence/counter mismatch).
- **S2.3**: the oracle is compared against its own golden
  (`<name>.oracle.stdout`), not the driver-format golden — driver output
  formats are per-case and interleaved; the semantic comparison against
  the compiled goldens was performed by hand (table below).

**Adopted owner decisions (provisional, recommended defaults):**

1. In-SCC negation is REJECTED (the Stratify warning hardens to an error
   at Stage 3; `evm_func_parse.dr` gets a rewrite attempt there). The
   oracle refuses such programs with a clean diagnostic.
2. Table phase ownership: the model's unique multi-view stratum owns
   D/R/I when one exists, else the highest member stratum.
3. Same-batch explicit add+remove of one fact NETS TO ZERO at ingest
   (§5.0) — deterministic, diverging from today's order-dependent
   outcome; `negation_flap` flap B is the known, expected divergence.
4. Oracle inputs come from `.batches` sidecars transcribed from drivers;
   compiled-side validation at Stage 3 uses an in-DB debug recount
   (`DebugValidateCounts`), not batch extraction.

**S2.4 cross-check vs the compiled goldens** (oracle final state vs the
final dump implied by `goldens/<name>.stdout`): 22 of 24 transcribed cases
agree exactly. The two divergences:

- `negation_flap` — expected (decision 3): both flaps net to zero in the
  oracle (`visible = {1,3}`); the committed golden's flap B lets the
  removal win (`visible = {1,2,3}`). Re-bless at Stage 3.
- `transitive_closure_diff` — **latent compiler bug of the
  all-modes-identically-wrong class (the F11/F15 escape route), found by
  the oracle**: after the batch `+{(4,7),(8,1)} −{(2,3)}` the edge set is
  the acyclic chain 3→4→7→8→1→2, yet the golden's round 2 retains a
  self-supporting closure residue (e.g. `from 1: 1 2 4 7 8`; truth:
  `from 1: 2`). The non-linear rule `tc(F,T) : tc(F,X), tc(X,T)` lets
  old-cycle closure rows mutually re-prove each other through the
  kUnknown→kAbsent recheck; the linear formulation
  (`transitive_closure_diff2`, same batches) matches the oracle
  row-for-row. Exactly the "no spurious tuples" property the C_nr/C_r
  split enforces structurally. Verdict: compiler wrong, oracle right
  (both oracle paths agree and match hand computation).
  `transitive_closure_diff.stdout` joins `negation_flap.stdout` on the
  Stage 3 re-bless list; neither golden was touched now.

**Proposal errata found by implementing §5 literally** (code follows the
proposal unless noted; wording fixes for a future proposal edit):

1. §5.0 netting is unspecified for multi-op nets on one row (e.g.
   `{+,+,−}`): the oracle uses the arithmetic net sign; `kExplicit` stays
   one set-semantics bit, so a net of +2 is one `AddExplicit`.
2. §5.2's seed `SubDerivation(head(F), class(edge))` is ambiguous per
   position; the only reading consistent with net-exact per-class counts
   (and with the fixpoint's hardcoded `kRecursive`) is the RULE's fixed
   class: recursive iff any body atom's `DerivationClassInto(head)` is
   recursive. Implemented so.
3. §3.1's `crossed` on `SubDerivation` fires on EVERY decrement of an
   in-I row with `C_nr ≤ 0`, not only a sign crossing (intended per §5.2;
   the field name is a footgun; duplicate enqueues absorbed by
   `TryClaimDel`).
4. §5.4's crossover silently relies on the negation key spanning the
   negated view's entire row (true in this IR); partial-key negation
   would over-fire it.
5. §5.5 Commit clears only `kDel|kAdd`; clearing all four claim/frontier
   flags is the safe spelling (the oracle does).
6. §5 is phrased per-table; per-view counting is finer and sound.
7. `DerivationClassInto` on straddling models can classify an edge from a
   strictly-lower-stratum view as kRecursive (deriving view shares an SCC
   with *some* member of the target's model); sound (seed subs complete
   before REDERIVE's read), but `C_r` is then not strictly "back-edge
   arrivals of the row's own SCC" — Stage 3 note.

**Adversarial audit + stress**: the oracle's `SeedReads`/`FixReads` were
verified line-by-line against §5.1's two read-state tables and the §3.1
crossing predicates; `tc_mixed_batch` and `two_hop_phantom` were traced by
hand through the code, reproducing §5.1.1's worked counter values at every
phase boundary (including the transient `C_nr(h(2,9)) = −1` phantom dip
that commits at exactly 0). Stress: seeds 1–30 × 25 rounds on
`tc_mixed_batch`, `two_hop_phantom`, `negation_flap`,
`tc_nonlinear_diff`, `cond_both_polarities` — 150/150 clean,
~6.1M assertions.

Gate results: ctest 3/3; full suite `SUITE: PASS` (132 cases × 4 modes +
24 oracle steps); zero modifications to existing goldens (the 24
`.oracle.stdout` files are new).

**Monotone projection (`--project-monotone`), a standing F16-class gate.**
The oracle already runs two agreeing paths per batch — the incremental
`(C_nr, C_r)` counters and an independent from-scratch semi-naive
stratified SET evaluation over the accumulated net explicit facts. The
`--project-monotone` mode exposes that from-scratch evaluator directly on
the surviving-input set: it reduces the `.batches` sequence to the facts
whose net add/remove state across the whole sequence is present (per-batch
ingest netting, set semantics — an add later cancelled by a remove, or an
add+remove in one batch, does not survive), feeds them as one add-only
epoch, and prints the same canonical relation dump as the normal final
state, prefixed by `MONOTONE-PROJECTION: N surviving facts`. This is the
ground-truth final materialization: the program evaluated as if nothing had
ever been removed. `runall.sh` runs it for every `.batches` sidecar
(verdict `<name> monotone OK|MONO-FAIL|MONO-MISSING|MONO-DIVERGE`,
byte-compared against `goldens/<name>.monotone.stdout`, promoted by
`--bless`); 24 monotone goldens blessed after hand-verifying each
surviving-fact set. The normal run additionally prints, to stderr, the line
`INVARIANT: differential-final == monotone-projection (M relations)` — the
positive counterpart to the `FailMismatch` dump — so the property the
counters exist to guarantee is stated in the oracle's own output.

This is the F16 gate made structural: `transitive_closure_diff`'s surviving
edges are the acyclic chain 3→4→7→8→1→2, so the monotone projection shows
`reachable_from 1 2` and NO self-loops. It therefore DISAGREES with the
buggy (F16) compiled golden `transitive_closure_diff.stdout`
(`from 1: 1 2 4 7 8`) and AGREES with the oracle's own differential final —
a spurious cyclic residue can never satisfy the monotone golden, because
the monotone evaluator has no removal machinery to leave a residue in.

*Limitation — final-state only.* The monotone projection is the materialized
state after the LAST batch. It does not capture the drivers' intermediate
per-batch observations (dumps between sends), which remain covered by the
compiled golden and by the oracle's per-batch incremental-vs-from-scratch
cross-assertions; it also does not exercise the differential/removal code
paths (that is the point — it is the monotone referee, not a second
differential run).

## Stage 3 — the swap (one landing; checkpoints a–e)

Honestly a monolith: the tri-state's producers and consumers cannot be
replaced separately without a dual code path, so this merges as ONE change
containing the full §6 deletion inventory. The checkpoints below order the
work inside the branch. **Honesty note on green-ness:** (a) is
compile-green with the new vocabulary dead (no producers yet) but cannot
delete the tri-state; (b) can be suite-green for the monotone subset
(filter regex) only; (c) and (d) are NOT runnable-green in isolation —
differential cases are broken from the moment (b) stops emitting removal
cascades until (e) lands the commit sweep. Verification between (b) and
(e) is: clean compile, monotone-subset suite green, `-ir-out` inspection
against the Q2 region-by-region skeleton, and unit-level DiffTable tests.
The full-suite gate applies only at the end.

- **(a) Runtime + IR vocabulary + emitters.**
  Files: `include/drlojekyll/Runtime/Table.h` (add `DiffTable` alongside
  `Table`; `TupleState` still present, deleted at (e));
  `include/drlojekyll/ControlFlow/Program.h`, `lib/ControlFlow/Program.h`,
  `Operation.cpp`, `Program.cpp`, `Visitor.cpp`, `Format.cpp`,
  `Optimize.cpp`, `Analyze.cpp` (the ten-site threading list from Q1);
  `lib/CodeGen/CPlusPlus/Database.cpp` (`EmitUpdateCount`/
  `EmitCheckMember`/`EmitCommitSweep`; table declaration `:856` learns the
  flavor split). Plus `DiffTable::DebugValidateCounts()` and direct unit
  tests (a `tests/` DrTest target for DiffTable crossing/claim/commit
  semantics, incl. phantom-pair negative-dip cases from §5.1.1).
- **(b) Monotone split + non-recursive differential strata.**
  Files: `lib/ControlFlow/Build/Build.cpp` (ingest netting at receive
  sites; `InTryInsert` → UPDATECOUNT; seed-schema emission; DELETE
  `InTryMarkUnknown`, `BuildEagerRemovalRegionsImpl`, both remover
  dispatchers), `Build/Insert.cpp`, `Build/Select.cpp`, `Build/Tuple.cpp`,
  `Build/Compare.cpp`, `Build/Map.cpp` (per-node remover/checker halves
  deleted), `Build/Procedure.cpp` (stratum-SERIES skeleton in the primary
  proc), `Build.h` (Context checker fields deleted).
- **(c) Inductions → D/R/I.**
  Files: `lib/ControlFlow/Build/Induction.cpp` (BuildFixpointLoop remove
  polarity, BuildUnknownRecheck, snapshot/reseed phases, MODESWITCH uses
  deleted; OVERDELETE mirror + REDERIVE loop + BUILDFRONTIERS added;
  claim/retire calls at dequeue), `Build/Join.cpp`, `Build/Merge.cpp`,
  `Build/Induction.h`, `Database.cpp` (re-entry loop `:1678–1699`
  deleted), `Program.h` (`kInductionRechecks` deleted).
- **(d) Negation crossover.**
  Files: `Build/Negate.cpp` (forward CHECKMEMBER gate; inductive-negation
  path deleted — Stratify's diagnostic now dominates it), `Build.cpp`
  `:269–405` deleted (crossover joins added at the NEGATE's stratum,
  reusing the `BuildMaybeScanPartial` index request), `Build/Product.cpp`
  `:214–232` deleted.
- **(e) Commit sweep, publish, query filter, final deletion sweep.**
  Files: `Build/Procedure.cpp` (flusher checker branch `:295–330` deleted;
  COMMITSWEEP appended; `kMessageOutputs` retained for monotone-backed
  messages), `Database.cpp` (query cursors → `Present(id)`,
  `:1734–1769`; `find_` naming, 3-way switch deleted), `Program.h`
  (`tuple_checker`, `kTupleFinder`, `MODESWITCH`, CHECKTUPLE 3-way,
  CHANGETUPLE deleted), `Runtime/Table.h` (`TupleState` + `TryChange*`
  deleted), all remaining §6 rows.

Merge criteria (all mandatory): ctest 3/3; 128 cases × 4 modes + oracle
variant `SUITE: PASS`; Stage 0's three mixed-batch sentinels green;
full-depth `deep_chain_retract` in constant stack (the acceptance gate);
30-seed randomized mixed add/remove stress vs the oracle;
`DebugValidateCounts` wired into debug OptDiff runs; goldens byte-identical
except the Stage-0-audited publish cases, which get exactly one reviewed
`--bless` with the oracle as referee; deletion greps empty:

    grep -rniE 'TupleState|kUnknown|TryChange|TryTransition|TopDownChecker|
    tuple_checker|kTupleFinder|InTryMarkUnknown|BottomUp.*Remover|
    MaybeRemoveFromNegatedView|MaybeReAddToNegatedView|PivotAroundNegation|
    BuildUnknownRecheck|kInductionRechecks|ModeSwitch|MODESWITCH|
    ChangeTuple|kAbsentOrUnknown' lib include bin   # → zero hits outside docs

## Stage 4 — docs and invariants

1. Rewrite the differential sections of `docs/RuntimeAndCodegen.md` and
   `docs/ControlFlowIR.md` (present tense only; no "previously").
2. `CLAUDE.md` core-invariants list: replace "every inductive back-edge
   append must be dominated by a state transition on the union's table"
   with "every inductive back-edge fold is an UPDATECOUNT whose
   propagation body is dominated by its zero crossing"; add the
   seed/fixpoint schemas, the named membership predicates, and the
   commit-time non-negativity asserts as invariants.
3. `tests/OptDiff/FINDINGS.md`: new findings from Stages 0–3, if any.

## Stage 5 — close the differential `@product` gap

1. The product delta rule is the seed/fixpoint schema with an empty pivot;
   lift the diagnostic at `lib/ControlFlow/Build/Build.cpp:2024–2030`
   ("Cross-products over differential (deletable) data") and make the
   0-pivot differential join flow through the (b)/(c) emission (the
   `swap_vec == vec` non-inductive-product-predecessor frontier semantics
   from Q3(b) is the piece to get right).
2. Promote `data/examples/conditions_to_bools.dr` into the compiling
   corpus (acceptance test) and strike it from CLAUDE.md's gap list.

## Known risks the executing workflow must respect

1. **Counter drift is the design's new silent bug class** (proposal §8):
   all defenses — commit asserts, `DebugValidateCounts`, the oracle
   variant, the Stage 0 sentinels — are merge criteria, not nice-to-haves.
2. **The seed/fixpoint schemas are one generated pattern** and the single
   point of failure; an earlier draft's version was broken by adversarial
   mixed-batch tracing. Any schema change re-runs the §5.1.1 traces by
   hand before code.
3. **Stage 3's interior is not continuously green** — do not "fix" a mid-
   branch red differential case by resurrecting a deleted mechanism; the
   only exit is forward through (e).
4. **Stratify audit hits block Stage 3** (Open Questions 1 and 11): an
   in-SCC negation in the corpus, or a stratum-straddling data model,
   needs an owner decision (reject vs. Motik-style traces; forbid vs.
   highest-stratum ownership) before the swap starts.
5. **Publish-order churn**: the commit sweep may reorder differential
   publications; the determinism knob is sorting the touched set — decide
   at Stage 0 audit time, not ad hoc when a golden goes red.
