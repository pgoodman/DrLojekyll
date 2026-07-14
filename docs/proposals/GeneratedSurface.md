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

## D. Re-verification record (2026-07-13, this epoch's pre-code pass —
## the mandated fleet review of the §A–§C single-pass seed)

Method executed as bootstrapped: §A re-derived from Database.cpp @93c1810
by an independent opus pass + the real booleans_diff/product_conds
artifacts read end to end; full driver-usage matrix (155 drivers + 2
ctest); build-contract audit; 5-lens adversarial critique (4 opus + 1
sonnet) with compiled probes on clang 21 AND gcc 16; §C′ hand artifacts
written, compiled against the untouched runtime on BOTH compilers, and
byte-compared to committed goldens (all three: BYTE-IDENTICAL). The
precedent held again: the seed contained real defects (below).

### §A errata (all verified; §A text above left as recorded)

- E1. The header also emits 4 std includes (cstdint/optional/tuple/
  vector) @2074-2077; the .cpp emits tuple/utility. §A omitted both.
- E2. Tup shape structs have NO Hash method (operator<=> only); Hash
  lives on Row/Key structs. §A conflated them.
- E3. There are TWO log call sites: the publish region @1032 AND the
  commit sweep @1316-1325. §A's ":1028" anchor is stale (off by 4).
- E4. The `<msg>_input` alias rule is VectorParameters().size()==1, not
  "monotone" per se (equivalent in effect today).
- E5. The cursor is a NESTED struct `Database::<q>_cursor` holding a
  MUTABLE `Database &db`, bound values, `uint32_t pos`; `next()` is
  defined out-of-line in the .cpp and reaches state via `db.<member>`
  (nested-class private access).
- E6. Queries with a forcing function CALL A PROC (mutating) — §B's
  `const Database &` for query friends is WRONG; forced queries also
  need Log/Functors under deduction. Corpus-wide exactly ONE forced
  query exists: force.dr's get_next_id_bf (merge_5/
  recursive_to_downstream/view_3 match a text grep but do NOT force).
- E7. §C erratum: booleans_diff emits NO cursor (its only query is
  all-bound). A third hand artifact (cf13_1: full-scan cursor +
  Present filter) pins the cursor shape.
- E8. `Database`/`DatabaseLog`/`DatabaseFunctors` are FIXED identifiers
  regardless of #database (only namespace + filenames change).
- E9. 16 inline-hook stages exist (6 top-level, 4 functors, 4 log, 2
  namespace-adjacent); all positions preserved. No corpus file uses
  them.

### Empirical resolutions of the open questions (probes committed to the
### session record; clang 21 + gcc 16 agree on every one)

- OQ-S2 RESOLVED: in-class friend TEMPLATES calling detail functions
  defined after the class work (instantiation-point lookup). BUT
  (i) non-template friends do NOT see post-class names, and (ii) a call
  with NO dependent argument binds at phase 1 even inside a template —
  and procs that need neither Log nor Functors are PLAIN functions, so
  their call sites have no dependent args. Therefore the FORWARD
  DECLARATIONS of all detail procs are emitted BEFORE the class and are
  MANDATORY (probe: a driver Log in a foreign namespace breaks the
  ADL-only fallback on both compilers). Missing-log-method error
  quality: 3-frame instantiation chain ending "no member named X in
  'BadLog'" — acceptable, no requires machinery.
- OQ-S4 RESOLVED: cursors stay NESTED; next() moves IN-CLASS (probed:
  nested member bodies see later-declared enclosing members). No
  befriending, no relocation; artifact reading order preserved.
- OQ-S3 RESOLVED: TWO files stay. datalog.h carries everything;
  datalog.cpp becomes an anchor TU (banner + include). compile_datalog
  byproducts, LIBRARY_NAME static-lib, diffrun.sh/runall.sh compile
  lines, and the CLAUDE.md recipe survive verbatim. macOS ar/ranlib
  emit no warning on the symbol-less archive (probed).
- OQ-S1 RESOLVED: global namespace stays the default; #database
  namespace support unchanged; no new namespace. Hidden friends are
  ADL-only — qualified calls (points_to::init(...)) are ILL-FORMED
  (probed): the sweep rule is "types may be qualified; calls never".
- Deduced returns: NOT needed in the detail region — procs keep their
  concrete `bool` returns (they all return bool today); `auto` stays
  only on the friend entry points. Decision 3's auto-return device is
  retired for the guts.
- ODR: detail procs must not be `static` (IFNDR when a driver includes
  datalog.h from two TUs). Plain procs are `inline`; Log-needing procs
  are templates (external linkage). Mixed-NDEBUG two-TU link probed
  clean (`initialized_` store is unconditional; only the assert
  compiles out).

### Adversarial-critique corrections adopted (fleet findings)

- MESSAGE-HANDLER TWIN (the round's F17-class catch): force.dr's
  kQueryMessageInjector detail proc CALLS the kMessageHandler proc
  (inject_20 → trigger_generate_next_id_1). Entry points therefore
  cannot absorb handler bodies into friends: EVERY message handler
  emits as a detail twin (id-suffixed name) carrying today's body, and
  the hidden-friend entry is a thin wrapper assembling `db.` refs and
  delegating. EmitCall routes kMessageHandler callees to the twin.
- READ/WRITE-SET WALKER: a full region-tree walk (WalkRegion skeleton)
  collecting DataVariable uses (IsGlobal → globals), region
  tables/indices, PLUS per-written-table index maintenance
  (EmitIndexAdds implies the table's indices), PLUS allocator seeded
  unconditionally (it is emitted as a literal string, invisible to the
  IR — booleans_diff's init_3 touches no table yet needs it). The
  proc call graph is a strict DAG corpus-wide (recursion lives inside
  INDUCTION regions), so the transitive closure is one topological
  pass. Parameter order: allocator, [Log, Functors,] tables/indices/
  globals by Id(), then vectors/scalars in today's order —
  deterministic against the known pointer-keyed-container id drift.
- QUERY ASSERTS: queries assert `initialized_` too (uniform contract;
  contract-lens F6). Entry points and init assert as designed.
- DRIVER SWEEP is NOT regex-able: the rewrite is keyed on symbol KIND
  (message vs plain query vs forced query vs cursor), which lives in
  the generated artifact, not the driver text; and 6 drivers spell the
  receiver `d.` inside generic lambdas. Sweep = python script that
  parses each case's generated header for the symbol table, then
  rewrites token-aware; hand-port list: compare_3/4/6 (pointer-to-
  member dispatch → generic lambdas over ADL calls), cf15_1/2/3 +
  compare_1/5/6 (requires-detection: `requires { d.q(); }` →
  `requires { q_(d); }`-form, probed working, PLUS the const hazard:
  detection on a const db is silently false — bind non-const
  everywhere), force (the one forced query gains log/functors),
  product_* (PrintLog drops base/override), ctest ×2 (unqualified ADL
  calls; namespaced types stay qualified). SKIP set: the expected-
  diagnostic cases (aggregate_1, kvindex_1-4, evm_func_parse,
  nonascii_1, truncated_decl_1) never compile a driver.
- EPOCH-0 INSERTION RULE (unconditional): `init(db, log, functors);` on
  the line immediately after EVERY Database construction, before ANY
  use of db or read of its log, threading the log/functors objects in
  scope for THAT instance (product_conds: per-block log, shared
  functors — its `init:` golden line is NON-empty: `+(false,false)`).
  Construction-site counts: tc_nonlinear_diff 2 sites, product_ind 1
  site (helper fn), product_conds 2 sites.
- DOCS SWEEP SCOPE (wider than the seed's list): RuntimeAndCodegen.md
  (":3-4/:97 'template-free' headline now false; generated-code shape
  section), Architecture.md (:38, :226, worked example :264-303),
  Language.md (worked example + query/functor code-shape prose),
  CLAUDE.md ("Generated API" paragraph + manual compile recipe note),
  plus the @inline-functor caveat (verbatim user C++ now lands inside a
  template body — two-phase-lookup semantics change for a feature with
  ZERO corpus coverage; documented, not blocking).
- MULTI-HEADER-SAME-TU non-regression: two default-namespace generated
  headers in one TU are ALREADY impossible today (Tup_* redefinition),
  and two different `class Database` definitions in one program are
  already an ODR violation across TUs — the free-function move does not
  regress anything; -first-id + #database namespaces remain the
  multi-database mechanism.

### C′. The hand-written target artifacts (committed)

docs/proposals/GeneratedSurface.artifacts/{booleans_diff,product_conds,
cf13_1}/{datalog.h,datalog.cpp,main.cpp} — written by hand against the
ground-truth emitter output, compiled with clang 21 AND gcc 16 against
the UNTOUCHED runtime, stdout BYTE-IDENTICAL to the committed goldens
(3/3 artifacts × 2 compilers). booleans_diff pins: plain-inline procs
(no Log/Functors anywhere), the all-bound query, unnamed unused friend
params. product_conds pins: Log-templated procs, the commit-sweep
publish through a deduced log, TWO instances with per-block logs,
explicit init with a NON-empty `init:` publication. cf13_1 pins: the
nested cursor with in-class next() and the Present filter, and a
query-factory assert. Sealed boundary probed: driver access to
db.<table> fails to compile (private). Detail bodies are TOKEN-
IDENTICAL to today's emitted bodies (member names become parameter
names of identical spelling) — the emitter conversion for procedure
BODIES is a signature/wrapper change only.
NOTE: the artifacts inline the message-handler bodies into the friends
(valid for these three cases: nothing calls their handlers); the
EMITTER emits the handler twin split per the injector finding above.
A round-2 adversarial review of the artifacts (independent opus pass,
mechanical diffs/greps + fresh compiles on both compilers) verified:
body token-fidelity vs ground truth, exact read/write sets (flow_251
correctly excludes table_41/48), design conformance, driver-port
fidelity, and the break attempts (two-TU different-log ODR probe,
double-init and entry-before-init asserts firing, move-from-twice
preservation, foreign-namespace log per F-B2). Its two findings were
APPLIED: the init friend now delegates to the init_3 detail (uniform
emitter rule; init_3 not dead), and ALL query entry points assert
initialized_ (the D-e decision: uniform asserts). Artifacts re-verified
byte-identical after both fixes (3 cases x 2 compilers).

### Staging plan (supersedes the seed's Method bullet 4 granularity)

- Stage A (emitter-only, driver-INVISIBLE, suite green): procedure
  bodies move to detail functions with explicit read/write-set
  parameters INSIDE datalog.cpp; class/ctor/methods/virtual log all
  unchanged; methods become thin wrappers passing member log/functors
  (virtual dispatch preserved). Gates run in full.
- Stage B (atomic emitter+driver commit): the surface flip — sealed
  struct, hidden friends, de-virtualized DatabaseLog, explicit init,
  header-only move with anchor TU — plus the full driver sweep (155
  cases + 2 ctest). Nothing compiles half-way, so emitter and drivers
  land together; the suite gates the commit.
  (The critique's dual-surface 3-commit alternative was evaluated and
  REJECTED: a 1-arg ctor cannot coexist with reference members bound
  by the 3-arg ctor without converting members to pointers — churn
  that defeats the purpose; and a no-op transitional init() risks
  masking real init omissions in the sweep.)
- Stage C (docs): the widened docs sweep above + CLAUDE.md.

Gates unchanged (§Gates): zero golden churn, ctest 3/3, corpus 4-mode
sweep, F22 fence probes, build wall time before/after (baseline
recorded this session: SUITE PASS 155, wall 1:53.60 at 8 jobs, fresh
zero-red run at branch start; ctest 3/3 at 59.15s).
