# Split-Brain CostModel Findings

Findings are ordered by severity. Each includes a disposition so a future
session does not turn an unresolved research question into a compatibility
branch or guessed implementation.

## Critical 1: The Product Is Three Different Tools

**Evidence.** `cost-simulator-seed.md:39-60` combines a static L1 pass, an L2
simulation, compilation and execution of generated code, counter snapshots,
assertions, configuration comparison, and scenario-family judgment in one
`cost/calibrate/judge` API.

**Problem.** Static estimation, exact trace interpretation, and runtime
measurement have different inputs and trust boundaries. The current design has
no concrete execution-trace representation, no generic generated-code driver,
and no process protocol for returning measured counters to `bin/Cost`.
`gBenchCounters` lives in the generated executable, not the compiler process.

**Example.** A `.cost` block saying `size=1000, fanout=4` can support an
estimate. It cannot tell the runtime which 1,000 keys to insert, in what batch
order, which duplicates occur, which demand queries run, or what retractions
happen. Those details alter counters.

**Disposition: Design decision.** Choose and name the products separately.
Recommended first choice:

```diff
- drlojekyll-cost predicts and measures all counters from a .cost summary
+ cost analysis predicts a declared subset from a concrete scenario trace
+ calibration runner separately builds/runs generated code and compares reports
```

## Critical 2: Numeric Does Not Mean Grounded or Exact

**Evidence.** The seed calls L1 values "concrete row counts" while rules use
scenario-provided selectivity, fanout, closure size, and sampled fixpoint size
(`cost-simulator-seed.md:62-86`). The existing proposal itself admits these are
unknown inputs (`CostModel.md:712-745`).

**Problem.** Replacing a symbol with an analyst-entered number does not answer
whether the model is make-believe. It can hide uncertainty more effectively
than a symbolic formula. `predicted ~= measured` also lacks an error metric,
tolerance, calibration/test split, or generalization protocol.

**Disposition: Remove claim/scope.** Call these values estimates and retain
their assumptions in output. Exact calibration is allowed only for rules whose
inputs fully determine the counter.

## Critical 3: The Rel Access Seam Assumes the Wrong Lifetime

**Evidence.** `BuildStratumPhases` constructs a local `DRFlowGraph`, validates
it, moves it into `Context::dr_flow`, dumps it, lowers it, and lets the build
context die (`Stratum.cpp:2149-2297`). `Program` does not retain it.

**Problem.** The seed recommends an accessor "if the Rel graph outlives
Program::Build" (`cost-simulator-seed.md:112-117`), but it does not. Exposing a
borrowed graph during the build would also expose raw Query and ControlFlow
pointers and a circular library dependency.

**Disposition: Design decision.** Prefer a pointer-free immutable snapshot
created at the owner boundary:

```diff
- expose const DRFlowGraph & after Program::Build
- or parse the diagnostic text dump as core input
+ during Program::Build:
+   DRFlowGraph -> CostProgramSnapshot(domain ids + op facts + mappings)
+ return BuildArtifacts { Program, CostProgramSnapshot }
```

If a callback is selected, make it a required protocol with a no-op
implementation. Do not add another nullable global sink like
`SetRelDumpStream(OutputStream *)`.

## Critical 4: L1 and L2 Have No Complete Shared Dynamic State

**Evidence.** Query owns logical columns and relations. Rel owns differential
schedule objects but stores back-pointers to Query and ControlFlow objects. The
text dumps do not carry a complete stable crosswalk or runtime state.

**Problem.** A single `card[view]` does not describe:

- standing versus newly added rows,
- delete and add frontiers,
- per-round claimed sets,
- duplicate derivations,
- touched-but-net-unchanged rows,
- per-key instance contents,
- aggregate group state.

L2 cannot independently derive these from a static L1 cardinality map. The
old `V-COST-XCHECK` is not rescued merely by making inputs numeric.

**Disposition: Research question.** Define an event/state model before a
general DELTA/fixpoint cost model. Limit slice 1 to monotone, non-recursive,
single-epoch behavior if that state model is not yet designed.

## Critical 5: The Proposed Counter Mapping Is False in General

**Evidence.** The seed states `kEagerInsert: idx_adds += card(t)` and similar
one-line mappings (`cost-simulator-seed.md:73-86`). Runtime counters are
incremented inside conditional runtime operations in `Table.h`.

**Problem.** `Index::Add` occurs for a newly stored row and once per relevant
index, not once per arbitrary incoming row. Duplicate inputs still incur folds
and finds but may not add an index entry. `probe_steps` depends on hash-table
occupancy/collisions. `commit_publishes` depends on before/after presence.

**Example negative witness.** Feed the same row four times. Incoming cardinality
as event count is four, live-row cardinality is one, `finds` and folds reflect
the events, while `idx_adds` reflects only the first new row per index.

**Disposition: Fixable after design.** Ground each rule at the exact runtime
counter increment site and add a negative witness before claiming the rule.

## Critical 6: `Lowering` Does Not Cover Every Cost-Bearing Access

**Evidence.** `PlanNode::lowering` exists on access-plan spines. The formatted
mono eager `kJoinEmit` carries marker arguments and no spine. `MakeJoinEmitOp`
retains Query join structure rather than a normalized access plan.

**Problem.** The seed's statement that every access multiplies by its recorded
`Lowering` is false. Some cost-bearing eager join behavior must be reconstructed
from Query/Program/index facts.

**Disposition: Remove claim/scope.** Inventory op families by actual payload.
Either extend the cost snapshot with normalized access facts for every supported
op or declare the family unsupported.

## High 7: The Calibration Anchor Is Not Reproducible From the Repository

**Evidence.** `measured-calibration-1.md` cites
`scratchpad/benchmeasure/measure.cpp`; no such tracked path exists. No tracked
`.cost` case or counter-calibration executable reproduces the table.

**Problem.** A prose table cannot be rerun, invalidated by runtime changes, or
used as a gate. It is evidence of an experiment, not a durable calibration.

**Disposition: Fix now.** Check in the exact scenario generator/driver, build
flags, output parser, expected exact fields, and command. Re-run from a clean
build before using the law as acceptance criteria.

## High 8: The Checked-In Mono Artifact Is Mislabeled

**Evidence.** At current tip, default `-demand` produces one Query join and one
`kJoinEmit`. Adding `-opt-disable=df.ident_join` produces two joins and two
`kJoinEmit`. The checked-in `mono.demand.df` is byte-identical to the latter.

**Problem.** The artifact name and surrounding prose imply current demand mode,
while it is actually the recognizer-off counterfactual. A future tool can
calibrate against the wrong configuration and still appear internally
consistent.

**Disposition: Fix now.** Rename artifacts with explicit config identity, for
example `mono.demand.ident-join-off.*`, and add current on artifacts or a
reproduction script. Never rely on node number `join.7` as stable identity.

## High 9: `Prov` Is Overclaimed as Cardinality Substrate

**Evidence.** `ProvMap` records value-containment projections. It has no count,
fanout, key frequency, or scenario state.

**Problem.** The seed says L1 reuses Prov for "demand-key slices." Prov can prove
that a join is an identity on a key domain. It cannot say how many rows are in
the slice.

**Disposition: Remove claim/scope.** Reuse Prov only for structural exact facts
such as subset/identity certificates. Design cardinality statistics separately.

## High 10: The Scenario Surface Cannot Drive Generated Programs

**Evidence.** Existing workloads use custom C++ drivers and driver-supplied
functors. `.batches` drives the Query-level oracle, not arbitrary generated C++
entry points and functors. The proposed `.cost` grammar contains only summaries.

**Problem.** Calibration requires identical logical events across configurations,
including demand query calls and stateful ordering. The current scenario grammar
cannot express or execute them.

**Disposition: Design decision.** Prefer reusing/expanding a concrete trace
format before inventing a statistics-only sidecar. Decide how external functors
are represented. Keep workload summaries as a separate estimator input if they
remain useful.

## High 11: The Scenario-Family Verdict Overclaims Generality

**Evidence.** The seed says a feature is good only if it wins or ties in every
scenario and recovers asymptotic shape by sampling (`measured-calibration-1.md:
55-57`).

**Problem.** Passing a finite family proves only those samples. It does not prove
all workloads or asymptotic behavior. Naming scenarios in advance as "demand
LOSES" and "demand WINS" also invites confirmation rather than discovery.

**Disposition: Remove claim/scope.** Report the observed envelope and worst
sample. Use generated boundary sweeps and held-out cases. Reserve universal
claims for structural proofs or quantified properties.

## High 12: The Prompt Commits to a Full Tool Before Adjudication

**Evidence.** The old continuation prompt asks one session to revalidate the
entire repository, reshape an 814-line binding design, design three surfaces,
prototype them, implement a tool, calibrate it, regenerate goldens, and land.

**Problem.** This creates pressure to make unresolved boundaries look decided.
It also contains machine-specific paths, model/provider instructions, a push
remote, and a stale description of the tip.

**Disposition: Fix now.** The rewritten prompt stages adjudication, one vertical
slice, and explicit stop conditions. It removes portability and provider
assumptions.

## High 13: Release Builds Can Silently Continue After Arbitrary Query Failure

**Evidence.** `lib/DataFlow/Build.cpp:2594-2638` wraps optimization/finalization
in `catch (...)`. The handler calls `assert(false)` and then cleans dead lists.
With `NDEBUG`, the assert disappears and execution continues to stratification
with partially mutated state.

**Problem.** A release-only silent fallback invalidates any calibration that
uses `-O2 -DNDEBUG`: the compiler may appear successful after an exception.

**Disposition: Fix now.** Delete the catch-all and let exceptions reach the
process boundary. If cleanup is truly required, use RAII and rethrow. Add a
test for loud failure only if an actual throwing precondition can be induced.

## High 14: Optimizer "Fixpoint" Can Be a Silent Bounded Partial Result

**Evidence.** `QueryImpl::Canonicalize` stops at an iteration cap even if the
last round reported non-local changes. Its secondary stop is a short hash
history, not structural equality. Neither stop reports its reason.

**Problem.** Query shape is an input to Rel inventory and the proposed cost
snapshot. A silent partial canonicalization can change node identity, op count,
or plan shape across witnesses while every pass still claims it reached a
fixpoint. Calibrating a cost rule against that shape can fit an optimizer
termination accident.

**Disposition: Research question, then fix.** Instrument termination reasons on
the corpus and adversarial generated graphs. Promote unexpected cap/cycle exits
to loud failures or define an explicit deterministic completion contract before
using canonical shape as a semantic CostModel identity.

## High 15: Release Totality Is Weaker Than Debug Test Evidence

**Evidence.** The query-build catch-all disappears into continuation under
`NDEBUG`, and roughly 95 executable `assert(false)` sites guard supposedly
impossible internal shapes across parser, DataFlow, and ControlFlow code.

**Problem.** Debug success proves that covered invariants held. It does not show
that a release compiler fails safely on every uncovered shape. The proposed
calibration build explicitly uses `-DNDEBUG`.

**Disposition: Deeper audit.** Remove the known catch-all now. Classify other
assert-only impossible paths by their dominating validation and promote
corruption checks that must survive release. Do not claim release calibration
is trustworthy merely because debug goldens pass.

## Medium 16: Rel Is Called a Sole Authority While It Still Cross-Checks Earlier Emission

**Evidence.** Rel and ControlFlow are mutually linked. Build comments describe
eager emission records produced before Rel inventory and later checked against
Rel ops.

**Problem.** Rel is a work-in-progress cutover from the historical direct
DataFlow-to-ControlFlow pipeline. It is already the authority for important
differential inventories and schedules, but it is not yet a standalone
`DataFlow -> Rel -> ControlFlow` stage. This is a useful migration/test
technique, but it contradicts an unqualified "sole authority" narrative. A
CostModel built directly on Rel may miss behavior whose payload is still
authored elsewhere and may freeze temporary cross-check seams into an API.

**Disposition: Research question.** Audit each op family for author versus
checker before treating Rel as complete. Do not preserve two production
authorities merely for old byte goldens; either finish the cutover or describe
the actual hybrid contract. Adjudicate the intended Rel end state described in
`rel-migration-state.md` before choosing the CostModel boundary.

## Medium 17: Prediction and Observation Types Are Collapsed

**Evidence.** The seed returns a predicted `BenchCounters`, the same conceptual
shape as observations.

**Problem.** Unsupported fields become tempting zeroes, and estimated values
look exact.

**Disposition: Fixable.** Use a separate report type with per-field prediction
kind, assumptions, and error.

## Medium 18: Node Names Such as `join.7` Are Presentation Identities

**Evidence.** Enabling identity-join removal renumbers downstream Query nodes.
The proposal and `.cost` examples assert on `join.7`.

**Problem.** A harmless optimizer change can redirect an assertion to another
node or make it disappear.

**Disposition: Fixable.** Address facts by semantic identity: query/adornment,
guard role, source relation, pivot columns, and config. Numeric dump ids belong
only in presentation.

## Medium 19: Full Counter Equality Has No Error Contract

**Evidence.** `assert close(pred, measured)` is the complete stated calibration
policy.

**Problem.** Exact integral counters should usually be equal; estimates require
per-field absolute/relative error and a calibration/test split. One global
"close" hides both.

**Disposition: Design decision.** Define exact fields and error metrics before
goldens. Never bless unexplained error into a wider tolerance.
