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

## A. The current generated artifact and its emitter, as pseudocode
## (single-pass derivation 2026-07-13 — a SEED for the next session to
## re-verify, NOT fleet-reviewed; anchors at lib/CodeGen/CPlusPlus/
## Database.cpp @ c680f1c)

Emitter pipeline: `GenerateDatabaseCode` → `Generator::Run` (:2063) —
`ComputeNames` (:352; `ns_name = #database name's NamespaceName(kCxx)`,
EMPTY unless the module declares one — MiniDisassembler generates
`namespace mini_disassembler`, OptDiff cases generate none) →
`CollectVectorShapes` → header, then source:

    datalog.h (:2068-2115):
      #pragma once; Runtime includes (Allocator/Hash/Table/Vec)
      [inline hooks c++:database:{prologue,enums:*,...} — EmitInlines :548]
      namespace <ns_name>?               « empty by default ⇒ GLOBAL scope »
      enums (:556); Tup_<types> shape structs {cols, Hash, <=>} (:596)
      using <msg>_input = Tup_…          « 1-vec (monotone) messages ONLY »
      row structs (:612); struct DatabaseFunctors (:646)
      struct DatabaseLog {               « EmitLogDecl :719 »
        virtual ~DatabaseLog() = default;              « Stage-5 stopgap »
        virtual void <msg>_<arity>(cols…, bool added) {}  « per published msg »
      };
      class Database {                   « EmitDatabaseDecl :747 »
       public:
        Database(Allocator, DatabaseLog&, DatabaseFunctors&);
        bool <msg>_<arity>(Vec<Tup>[, Vec<Tup>]);   « one entry per message
                                                      = one EPOCH; differential
                                                      messages take adds+removes »
        bool <query>_<all-bound>(args) / cursor <query>_<pattern>(bound args)
       private:
        bool init_N(); bool proc_N(…); bool flow_N(…);   « flow procedures »
        Allocator; DatabaseLog &log; DatabaseFunctors &functors;
        Table/DiffTable<Tup_…> table_N; Index<…> index_N;  « THE STATE »
      };

    datalog.cpp:
      Database::Database(…) : members(allocator)… { init_N(); }
        « EmitConstructor :890 — EPOCH 0 RUNS IN THE CTOR, invoking log
          callbacks mid-construction; init's bool is swallowed »
      bool Database::<proc>(…) { … }     « EmitProcedure :916; body =
        EmitRegion (:249 dispatch) lowering the CF-IR region tree: vector
        ops, UPDATECOUNT folds, CHECKMEMBER gates, claim/retire, NetBatch,
        TABLEJOIN/TABLEPRODUCT/TABLESCAN, inductions, COMMITSWEEP — a
        differential-backed @differential transmit's sweep calls
        log.<msg>_<arity>(row cols…, added) (:1028, :1316);
        per-proc local vectors declared fresh at entry (~:928-931) »
      query bodies (EmitQueries :1896): index/scan + `Present(id)` filter
        on differential tables.

Driver contract today: construct (epoch 0 fires implicitly; the log must
be live BEFORE construction) → `db.<msg>_<arity>(Vec…)` per epoch →
observe via query cursors and/or DatabaseLog overrides. Build contract:
cmake/Compiler.cmake `compile_datalog()` declares byproducts
`${DATABASE_NAME}.h/.cpp` (:76-77); OptDiff's diffrun.sh compiles
`driver.cpp datalog.cpp lib/Runtime/Allocator.cpp` as separate TUs.

## B. The redesign as diffs against §A (the five decisions, rendered)

    datalog.h:
    -  struct DatabaseLog { virtual void <msg>_<arity>(…) {} … };
    +  « DatabaseLog/DatabaseFunctors as CONCRETE default types remain
    +    available for drivers that don't customize, but nothing in the
    +    generated code names them: entry points deduce Log/Functors »
    -  class Database { public: ctor-runs-init; methods; private: … };
    +  struct Database {                      « the sealed state struct »
    +   private:
    +    Allocator allocator; Table/DiffTable table_N; Index index_N;
    +    « NO log/functors members — they arrive per call by deduction »
    +   public:
    +    explicit Database(Allocator);        « allocate empty tables; NO
    +                                           callbacks; cannot fail »
    +    template <typename Log, typename Functors>
    +    friend auto init(Database &db, Log &log, Functors &f);
    +      « EPOCH 0 = the zeroth entry point; returns the swallowed bool;
    +        message entry points debug-assert it has run »
    +    template <typename Log, typename Functors>
    +    friend auto <msg>_<arity>(Database &db, Log &log, Functors &f,
    +                              Vec<Tup>[, Vec<Tup>]);
    +      « HIDDEN FRIENDS: defined in-class (ADL-only — a namespace-scope
    +        definition would un-hide), THIN — each delegates to an
    +        internal flow function below »
    +    friend auto <query>_<pattern>(const Database &db, bound args…);
    +      « auto return: cursor struct definitions may live BELOW »
    +  };
    +  « impl region (same header or an always-included datalog.inl):
    +    internal-linkage flow functions taking EXPLICIT table/index/vector
    +    references — the signature IS the read/write set; auto returns;
    +    generator emits in topological order so deduced-return
    +    definition-before-call is guaranteed by construction »
    -datalog.cpp: all Database:: definitions
    +datalog.cpp: shrinks to an anchor TU or disappears — OQ-S3 decides;
    +  cmake/Compiler.cmake byproducts and diffrun.sh compile lines follow.

Diff-to-decision map: state struct + private members = (1)+(2); hidden
friends = (2); internal explicit-parameter flow functions = (3); Log/
Functors template parameters = (4), retiring the §A virtual; `init` as
entry 0 = (5), deleting §A's ctor body.

## C. Target artifact end states (this epoch's "planned IR" — to be
## hand-written CONCRETELY next session before touching the emitter)

Two worked artifacts, written by hand and compiled against the untouched
runtime with hand-ported drivers, stdout byte-compared to the committed
goldens BEFORE any emitter change:

- **booleans_diff** (monotone+differential messages, query cursors, no
  published messages): pins the query-cursor shape under `auto`, the
  `<msg>_input` alias story, and a Log parameter that is never called
  (deduction with an empty default log).
- **product_conds** (Stage-5 fixture): pins TWO database instances,
  epoch-0-as-explicit-call (its `init:` output line must survive
  byte-identically — the driver calls `init(db, log, functors)` where the
  ctor used to fire it), log observation by deduction (PrintLog with NO
  virtual anywhere), and the differential-transmit commit sweep calling a
  deduced log.

Their hand-written headers become §C' of this ledger (committed), then
the emitter is converted to REPRODUCE them.

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

## Session bootstrap (fresh-session checklist — works on any machine)

State at seed (2026-07-13): branch differential-product @ c680f1c
(Stage 5 CLOSED — f4e3565 impl+fixtures, f736b1d docs+landing record;
NOT yet merged to main; Stage-5 deviations pending owner ratification,
incl. the `virtual DatabaseLog` stopgap this epoch retires). NOTHING of
this redesign is implemented. §A–§C above are a SINGLE-PASS derivation by
the Stage-5 session — never fleet-reviewed; the first adversarial review
is THIS epoch's first task (the F17/F18/F22 precedent: every epoch's
pre-code re-verification has caught a real defect in its seed).

- Read (in order): this file top to bottom; the Stage-5 landing record in
  StackSafeNegation.stage5-notes.md (the virtual-log deviation context and
  the fixture/driver conventions); PerfRoadmap.md §4 (the resequencing).
  Code anchors: lib/CodeGen/CPlusPlus/Database.cpp (Generator::Run :2063,
  EmitLogDecl :719, EmitDatabaseDecl :747, EmitConstructor :890,
  EmitProcedure :916, EmitQueries :1896, log call sites :1028/:1316,
  EmitInlines :548 + the c++:database:* hook stages); cmake/Compiler.cmake
  compile_datalog (:72-83); tests/OptDiff/diffrun.sh compile line; a
  REPRESENTATIVE generated pair (compile booleans_diff and product_conds
  with -cpp-out and READ datalog.h/.cpp end to end — the ground truth §A
  summarizes); the driver-usage matrix (every cases/*.main.cpp +
  tests/MiniDisassembler/Standalone.cpp + tests/PointsTo).
- Method (mandated): re-derive §A from the code and the real generated
  files; write §B's diffs against YOUR re-derivation; adversarially
  critique them (minimum: OQ-S1..S4, the hidden-friend/two-phase-lookup
  interaction, ADL pitfalls and error-message quality for a missing log
  method, the epoch-0 relocation against every driver incl. ctest's, the
  header-only compile-time delta, the cmake/diffrun contract change);
  hand-write §C's two target artifacts, compile and byte-verify them
  against committed goldens; critique those against §B; ONLY THEN convert
  the emitter region by region, full suite between regions, and sweep the
  drivers mechanically.
- Gates: as §Gates above. Blessing policy unchanged: goldens change ONLY
  via explicit review — this epoch expects ZERO golden changes.
- Environment: export PATH="/Users/pag/Code/.brew/bin:$PATH"; suite =
  DR=build/debug/bin/drlojekyll tests/OptDiff/runall.sh <workroot> [jobs]
  [filter]; macOS bash 3.2 (no declare -A); NEVER rebuild the compiler
  while a suite run is in flight (corrupts the run: DR-FAIL(127));
  runall.sh --bless filters are REGEXES — anchor them.
- End state: emitter + drivers + docs (RuntimeAndCodegen.md's generated-
  code shape section, CLAUDE.md's "Generated API" paragraph) committed;
  a landing record appended HERE with deviations for ratification;
  FINDINGS.md updated if anything is found; build-time delta recorded.
