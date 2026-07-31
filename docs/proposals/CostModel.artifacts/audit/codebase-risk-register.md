# Codebase Risk Register

This register captures cross-cutting findings exposed while tracing the
CostModel path. It is not a claim that every old assertion, pointer, or optional
return is wrong. It separates local repairs from larger audits so experimental
work can proceed without pretending these risks do not exist.

## High: Release Compilation Can Continue After an Arbitrary Exception

`Query::Build` catches `...`, executes `assert(false)`, removes dead nodes, and
continues. In a release build the assertion disappears. This is a muted failure
path through a partially mutated Query graph.

**Impact.** A release-only calibration binary can produce a result after the
compiler has failed internally. Debug/release agreement cannot validate a run
whose release compiler suppresses the failure.

**Disposition: fix now.** Delete the handler. Use RAII for cleanup and let the
exception reach the process boundary. Do not replace it with warning-and-
continue or a default result.

## High: Optimizer Termination Is Not a Success Contract

`QueryImpl::Canonicalize` stops on any of:

- no reported non-local changes;
- a heuristic eight-slot hash condition;
- a `max(N * N, 2 * N)` iteration cap.

Reaching the iteration cap while changes continue is silent. The hash is a
fingerprint of changed views, not a proof that graph state is equal, and the
comment describes cyclic-pattern detection more generally than the code
implements.

**Impact.** The result can be semantics-preserving but not canonical, which
changes CSE, downstream Rel inventory, presentation ids, and any cost snapshot.
It also makes "to fixpoint" comments and tests stronger than the actual
contract.

**Disposition: research, then fix.** Instrument exit reasons over the corpus and
generated stress cases. If the cap/cycle exits never fire, replace silent exits
with loud invariant failures. If they do fire, define a deterministic fallback
algorithm or weaken the canonicality claim explicitly. Do not add a configurable
round limit.

## High: Release Safety Relies Heavily on Debug Assertions

A precise source scan found roughly 95 executable `assert(false)` sites under
`bin/`, `include/`, and `lib/`. Some have explicit return fallbacks; others sit
in dispatch switches or continue after the assertion. Many are valid internal
invariants, but `NDEBUG` removes the check.

**Impact.** Unsupported or corrupted shapes can become silent default behavior,
null returns, incomplete emission, or undefined control flow in the same
release configuration proposed for counter calibration.

**Disposition: deeper audit.** Classify sites into:

1. proven total by a dominating user-facing validation;
2. internal corruption that must abort in all builds;
3. ordinary unreachable control flow that should use a language-level
   unreachable primitive only after proof;
4. unsupported input that needs a clean diagnostic.

Promote category 2 to the repository's always-on failure mechanism. This is not
a request to mechanically replace every assertion.

## High: The Strongest End-to-End Suite Is Manual

CI runs six CTest targets on Debug and Release for macOS and Linux. The 181-case
OptDiff matrix, oracle batch comparisons, equivalence gates, and Rel/DataFlow
golden surfaces are driven by shell scripts outside CTest. The fuzz workflow is
disabled by filename and uses stale configuration names. Its parser
round-trip property is not implemented, and fuzz CMake declares a backend
source file that is absent.

**Impact.** The repository's stated reality-grounding discipline depends on a
manual ritual. A locally green claim may not be reproduced on every pushed
change.

**Disposition: fixable.** Add a bounded OptDiff CI shard or scheduled full run.
Repair the fuzz workflow separately. Preserve local fast CTest; do not make
every unit-test invocation pay the full suite cost.

The detailed evidence and repair sequence are in
[`test-coverage-gaps.md`](test-coverage-gaps.md).

## Medium: Rel and ControlFlow Form a Circular Ownership Boundary

See `rel-migration-state.md`. The circular static-library relationship, raw
pointers, temporary graph lifetime, and cross-side event records are migration
scaffolding, not a stable API.

**Disposition: design decision.** Choose the Rel end state before publishing or
building a general CostModel against it.

## Medium: Required State Is Modeled as Optional

Two concrete cases have no current absent caller:

- `QueryImpl::Canonicalize(..., const PassPolicy *policy = nullptr)`; all active
  top-level calls pass the policy. The null check creates a second gating
  behavior that is not part of the compiler mode contract.
- `Context::dr_flow` is a nullable `shared_ptr`; current construction always
  calls `BuildStratumPhases`, which always installs the graph. The later null
  branch and its comment describe a retired path.

**Disposition: fixable.** Pass `PassPolicy` by reference. Give `Context` required
Rel ownership after the build phase, or split the context into pre-Rel and
post-Rel states so absence is unrepresentable. A `shared_ptr` is not justified
by shared ownership here.

This does not indict semantic optionals such as absent branch bodies or parser
failure results. Those represent real domain states.

## Medium: Diagnostic Output Uses Mutable Global Sinks

The CLI keeps global output pointers. Rel adds another global nullable stream
installed through `SetRelDumpStream`, and `Program::Build` consults it
implicitly.

**Impact.** Compilation is not reentrant, dump behavior is hidden from the
build signature, and a future CostModel collector is likely to copy this
nullable-callback pattern.

**Disposition: fixable.** Make output/collection an explicit required build
service with null-object implementations. Do not add another global sink or a
nullable CostModel callback.

## Medium: Experimental Pass Configuration Contains a Fake Dimension

`df.sink` is registered, consumes bisect positions, appears in aliases and
help/comments, but its body is commented out.

**Impact.** A configuration can claim to test a pass boundary while doing no
work. Bisect indices and mode descriptions include a nonexistent behavior.

**Disposition: fix now.** Delete the pass registration and dead body. Re-add a
named pass only with an implementation and witness.

## Medium: Primitive Identities Cross Architectural Boundaries

Query ids, table ids, vector ids, column positions, strata, SCC groups, signs,
and counter values commonly use `unsigned`, `int`, raw pointers, or tuples of
those values. This is endemic in older internals and especially visible in Rel
cross-check keys.

**Impact.** A new CostModel could easily compare a presentation id with a
semantic id, a live-row count with an event count, or a stratum with a round.

**Disposition: fix in new boundaries first.** Introduce domain types in the
snapshot/report API. Do not block experimentation on a project-wide mechanical
newtype conversion, but do not export the primitive ambiguity into new code.

## Low: Build and Documentation Hygiene Has Detectable Drift

- `lib/DataFlow/CMakeLists.txt` lists `Compare.cpp` twice.
- architecture prose retains unsupported-feature claims after aggregate/KV
  lowering landed.
- `Rel.h`, `Build.h`, `PassPolicy.h`, `Main.cpp`, and `CLAUDE.md` contain
  mutually inconsistent migration and pass-alias narratives.

**Disposition: fixable.** Repair in small behavior-neutral changes with build
and dump checks. Comments should describe current ownership and invariants, not
the sequence of prior review stages.
