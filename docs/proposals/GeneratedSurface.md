# Generated-surface redesign (seed ledger): free functions over a sealed state struct

Status: SEED (owner-agreed direction, 2026-07-13, on the Stage-5 close).
Sequencing: this epoch runs BEFORE the bench harness (PerfRoadmap.md §4
resequencing note) — the harness is a set of drivers, drivers are written
against the generated surface, and writing them once against the final
shape beats writing them twice. The golden-invariance net is also at its
strongest right now (155 cases, fresh baselines, the E0/Stage-5
methodology proven twice).

The epoch is BEHAVIOR-NEUTRAL by mandate: byte-identical stdout across all
155 cases, zero golden churn, no runtime data-structure or perf changes
(those are the bench/data-structures epochs' work). The ONE deliberate
observable change is epoch-0 timing (decision 5), absorbed in driver text,
not in output.

## Motivation

Today's generated artifact is `class Database` (datalog.h decl +
datalog.cpp impl): member tables, message entry points and flow procedures
as methods, everything in the GLOBAL namespace (database `datalog`, no
namespace), publications observed through `DatabaseLog` — whose methods
Stage 5 made `virtual` as a stopgap so drivers could observe published
deltas at all (the pre-Stage-5 struct had non-virtual empty inline bodies:
no observation surface existed). The class is ~90% habit: no polymorphism,
no meaningful encapsulation against its single generated consumer, private
methods that are procedures over tables. What it genuinely provides is
state aggregation (multi-instance is load-bearing: product_conds
constructs TWO databases for the same-epoch double-flip fixture) and a
name for the driver-facing API.

Style anchor (owner-flagged): mcyoung's `best` library — deduced-return
declarations to decouple interface blocks from definition order; hidden
friends for ADL-only API surface.

## The five decisions

1. **State = one struct of tables.** `struct Database` holds the tables,
   indices, vectors, allocator — and nothing else. Construction allocates
   empty tables: trivial, no callbacks, cannot fail. Multi-instance
   preserved. Members PRIVATE (see 2).

2. **Driver surface = hidden friends.** Every driver-facing function —
   message entry points, query cursor factories, epoch 0 — is a `friend`
   defined IN-CLASS, reachable only by ADL on a `Database&` argument:
   `enable_feature_1(db, log, vec)`. Rationale: (a) the generated code
   lives in the global namespace, so ordinary free functions would spray
   the host program's scope — ADL-only visibility is hygiene, not style;
   (b) friendship seals the boundary: tables stay private, so the standing
   invariant "differential tables are read only through the named
   membership predicates" is ENFORCED against drivers, not just observed
   by the emitter. Constraint to respect: hidden requires in-class
   definition (a namespace-scope definition un-hides the name), so entry
   points are THIN in-class wrappers delegating to (3).

3. **Guts = internal free functions with explicit table parameters.** The
   flow procedures were never driver-facing; they become internal-linkage
   functions whose parameter lists name exactly the tables/vectors/indices
   they read and write. The signature IS the read/write set — a step
   toward the delta-relational IR, and the wasm-ABI rehearsal
   (batch-shaped calls over engine-owned state, no hidden `this`; see the
   wasm-functor direction). `auto` returns wherever a concrete return type
   would force definition-order gymnastics; the generator controls
   topology, so the deduced-return visibility rule (definition before
   call, same TU) is a non-issue.

4. **Log and Functors flow by DEDUCTION.** Entry points are function
   templates with `Log &` / `Functors &` parameters; the driver's own
   types flow through statically — no virtual, no CRTP (rejected: no
   static type flows through a base reference into a non-template call
   site), no `Database<Log>` class template. This retires the Stage-5
   `virtual DatabaseLog` deviation. Consequence: the flow-procedure bodies
   must be visible at instantiation — the generated artifact goes
   header-only (or interface header + always-included impl header). The
   compile-time cost of that move is a REAL question this epoch must
   measure (one number, not a benchmark suite — full-suite build wall
   time before/after), and it feeds the bench epoch's artifact-shape
   triggers (PerfRoadmap.md §1: bit-stable artifacts, build caching).

5. **Epoch 0 is the zeroth entry point, not a constructor side effect.**
   The empty-EDB least model is semantically forced (stratified negation:
   `foo_enabled(false) : !foo.` holds at t=0, and a @differential
   subscriber must see the t=0 publications) — but today it runs INSIDE
   the Database constructor, invoking user log callbacks mid-construction
   and swallowing its `bool`. It becomes an explicit
   `init(db, log, functors)` (bikeshed the name; it is the empty-program
   fixpoint, not "initialization"), uniform with every other epoch;
   message entry points debug-assert it has run. This is the epoch's one
   driver-visible change: every driver adds the call after construction
   (stdout unchanged — the same publications fire at the same observable
   point in every existing driver's structure).

## Recorded and rejected alternatives

- `virtual` log methods — LANDED at Stage 5 as the stopgap; retired by (4).
- CRTP on DatabaseLog — does not compose with a `DatabaseLog&` member of a
  non-template class; only the deduced-parameter form gives static
  dispatch.
- Weak-symbol link-time log defaults (declaration in header, weak empty
  definitions in the TU) — keeps the TU split and kills the vtable, but
  non-portable (no MSVC) and one log flavor per binary. Rejected.
- Compile-time-evaluated epoch 0 (emit the initial table image as static
  DATA plus a t=0 publication replay list, deleting the code path) — the
  logical far end of (5); buys a second evaluator inside the compiler and
  a layout coupling to runtime tables for a once-per-instance path.
  Recorded, not planned.
- Keeping a thin `class Database` facade over the friend surface for
  driver source compatibility during migration — DECIDE IN-EPOCH: 155
  driver files churn either way (epoch-0 call); a facade halves the churn
  per file but leaves two API spellings in the tree. Lean: no facade,
  one mechanical driver sweep, suite as the net.

## Open questions for the epoch

- OQ-S1: global namespace + ADL vs finally introducing an opt-in
  namespace (`#database` name as namespace?) — parser surface exists;
  hidden friends make global tolerable; decide with evidence from a real
  collision or stop worrying.
- OQ-S2: two-phase-lookup interactions — in-class friend templates
  calling detail functions defined later in the header (dependent-call
  resolution at instantiation vs definition): write the ONE worked header
  by hand first and compile it under clang AND a second compiler before
  generating anything.
- OQ-S3: does `-cpp-out` still emit two files (interface header +
  impl header) or one? Affects cmake/Compiler.cmake and every case's
  compile line.
- OQ-S4: cursor shape under `auto` — per-query generated structs survive,
  but their definitions move below the interface block; confirm the
  "read the top forty lines" property actually holds in the emitted file.

## Method (the checkpoint method, as at slices 4/d/e/Stage 5)

Before any emitter change: (1) inventory the current generated surface
(EmitLogDecl/EmitDatabaseDecl/entry-point and flow-proc emission in
lib/CodeGen/CPlusPlus/Database.cpp) and the full driver-usage matrix
(grep every cases/*.main.cpp call form); (2) write the TARGET generated
header BY HAND for one small case (booleans_diff) and one product case
(product_conds — two instances + log observation + epoch 0), compile and
run them against the untouched runtime with hand-ported drivers,
byte-compare stdout; (3) adversarially critique the hand artifact (OQ-S2
lookup rules, ADL pitfalls, error-message quality on a missing log
method, compile-time delta); (4) only then convert the emitter, one
region at a time, full suite between regions. Workflows: opus for the
hand-written target artifact + critique + trace-level review, sonnet for
the mechanical driver sweep and suite gates.

## Gates

Full suite SUITE: PASS (155) with ALL goldens byte-identical (the
epoch-0 relocation must not change any stdout); ctest 3/3 (its drivers
churn too); data/ corpus 4-mode sweep unchanged; the F22 fence probes
unchanged; full-suite build wall time measured before/after and recorded
here; landing record appended HERE with deviations for ratification.
After this epoch: the bench harness (PerfRoadmap.md §5 bootstrap),
measuring the NEW surface.
