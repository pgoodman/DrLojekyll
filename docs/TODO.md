# Future investigations

Ideas worth investigating, not yet scoped into proposals. Each entry should
carry enough context that a fresh reader can start the investigation cold.

## 1. Modules / sub-databases / subgraphs

Pull out a subcomponent of the dataflow graph in which many inter-dependent
relations share common variables. Factor those shared variables out so that,
within the subcomponent, they are treated as constants; then extract the
subcomponent as its own independent database. At runtime, a new database
instance is instantiated per tuple of the factored-out common variables —
i.e. a mapping `common vars → database instance`.

Questions to answer:
- How to detect profitable subcomponents (clusters of relations whose clauses
  all propagate the same variable tuple unchanged)?
- Lifecycle: when is a per-tuple database instance created/destroyed, and how
  do differential retractions of the key tuple interact with it?
- How do messages and queries route across the boundary (key tuple becomes
  part of the sub-database's identity rather than its rows)?
- Interaction with induction regions that straddle the boundary.

## 2. Bottom-up re-proving of unknown tuples (stack-safe differential recheck)

Direction: draft a proposal + plan (UnitConditions.md-style) on its own
branch once the unit-conditions refactor lands, since this restructures the
same top-down checker / unknown-recheck machinery that refactor unifies. The
plan's central decision is B/F (min bookkeeping, keeps re-proof) versus the
dual-program / signed-weight pole (item 4), judged with **parallelization as
a first-class criterion**:

The tri-state representation (absent/unknown/present) is itself what makes
the current system resistant to parallelization. `unknown` is control state,
not data: correctness depends on ordering — mark unknown, let the removal
cascade quiesce, only then recheck and replay (calling checkers mid-cascade
is unsound; the induction recheck/replay machinery exists to enforce exactly
this barrier). Those quiesce points are global serialization barriers, and
top-down checkers read shared mutable table state while cascades are in
flight. A dual program eliminates the tri-state: retraction is a derived
fact of the dual (equivalently, a weight −1 tuple), tuple state is binary,
and updates combine by addition — a commutative monoid fold, safe to apply
out of order and in parallel, with consolidation instead of quiescence.
Residual serialization is then only the stratum/fixpoint-round frontier
structure, not per-retraction barriers. B/F, by contrast, still interleaves
backward-chaining proof checks against the shared materialization, so it
inherits much of the ordering problem even though it fixes the stack.

Differential removal marks tuples present→unknown and re-proves them with
recursive top-down checker procedures. The recursion is essentially unbounded
on deep derivation chains and has blown the stack in prior real use-cases.

Investigate refactoring the recheck into a bottom-up datalog program: the
"needs re-proof" set is itself a relation, and re-proving is a fixpoint over
it rather than a call tree. There appears to be a dual program /
representation — a datalog for the negations — where retraction/re-proof of
the primal program is ordinary bottom-up derivation of the dual one. If that
dual exists and can be generated mechanically from the primal rules, the
top-down checker machinery integrates much more cleanly (one evaluation
strategy everywhere, no unbounded recursion, and the recheck set becomes
observable/optimizable like any other relation).

Prior art to read first — this is a studied problem (incremental maintenance
of datalog materialisations under deletion):

- DRed (Delete/Rederive): Gupta, Mumick & Subrahmanian, "Maintaining Views
  Incrementally", SIGMOD 1993. Over-delete everything possibly supported by
  the deleted facts, then re-derive bottom-up — the canonical fixpoint
  formulation of exactly our recheck, plus the counting algorithm for the
  non-recursive case.
- Backward/Forward: Motik, Nenov, Piro & Horrocks, "Incremental Update of
  Datalog Materialisation: the Backward/Forward Algorithm", AAAI 2015 —
  bounds DRed's over-deletion by interleaving backward-chaining proof checks
  with forward propagation. Refined as FBF in "Maintenance of Datalog
  Materialisations Revisited" (Artificial Intelligence, 2019,
  https://www.cs.ox.ac.uk/people/boris.motik/pubs/mnph19maintenance-revisited.pdf);
  the DRed^c variant keeps per-fact counters split into nonrecursive vs
  recursive derivation counts. Our present checker design is closest to the
  backward-chaining half of B/F, implemented as compiled recursion — the
  papers describe how to drive it as a set-at-a-time fixpoint instead.
- Differential dataflow: McSherry, Murray, Isaacs & Isard, "Differential
  Dataflow", CIDR 2013; the living book at
  https://timelydataflow.github.io/differential-dataflow/ and many posts in
  https://github.com/frankmcsherry/blog. The strongest form of the "dual
  program" idea: updates are multisets with signed multiplicities, and
  retraction is a negative-weight tuple flowing through the *same* bottom-up
  operators — there is no separate recheck program at all. DDlog (Ryzhyk &
  Budiu, "Differential Datalog", Datalog 2.0 2019) compiles datalog onto it.
- Provenance semirings: Green, Karvounarakis & Tannen, PODS 2007 — deletion
  propagation via provenance polynomials; useful frame for deciding how much
  support bookkeeping to store vs recompute.

## 3. Ideas from "Compositional Datalog on SQL"

https://www.philipzucker.com/compose_datalog/ (Philip Zucker, 2025-08-26).

The post treats clause-body *environments* (variable→value binding sets) as
the primary relational abstraction, rather than the base relations:
relational algebra composes better over environments than over the relations
themselves. Ideas to evaluate against our IR:

- Environment-relations as a semantic foundation for the dataflow IR: a
  clause body is a composition of combinators over environment sets joined on
  shared variable names (close to how our JOIN pivots already work — is there
  a cleaner normal form here?).
- Semi-naive evaluation derived via dual-number semantics (primary + delta
  relation per table, à la automatic differentiation) instead of hand-built
  induction vector plumbing. Possibly connects to the negation-dual idea in
  item 2.
- Delta computation with anti-join (NOT EXISTS) rather than set-difference.
- Their SQL codegen strategy as a benchmark/alternative backend shape.

## 4. DBSP as theory, oracle, and audit tool for the differential machinery

Budiu, Chajed, McSherry, Ryzhyk & Tannen, "DBSP: Automatic Incremental
View-Maintenance for Rich Query Languages", VLDB 2023
(https://docs.feldera.com/assets/files/vldb23-1bfe30b29f95168c8e1f427fccfc6da2.pdf);
implemented in Feldera and the `dbsp` Rust crate. Streams + Z-sets (signed
multiplicities) + delay/integrate/differentiate operators; the theorem
Q^Δ = D ∘ Q^lifted ∘ I mechanically incrementalizes any query built from its
operator algebra, with per-operator rewrite rules (e.g. the bilinear rule for
join) and nested streams for incremental fixpoints. It is the algebraic
foundation under differential dataflow (item 2's prior art).

Candidate applications here, roughly in order of value-per-effort:

- **Audit blueprint for the hand-built removal paths.** Every hand-written
  differential mechanism in the control-flow build (pivoted-join remover,
  negate re-add, induction post-quiescence recheck/replay, products in
  inductions) is a hand-derivation of what DBSP derives mechanically. Check
  each against the corresponding operator's incrementalization rule
  (join: Δ(a⋈b) = Δa⋈b + a⋈Δb + Δa⋈Δb; distinct: threshold on integrated
  weight; fixpoints: nested delay). Any deviation is a candidate bug — the
  F9–F14 ledger shows hand-derivation is exactly where bugs live.
- **Independent oracle for the golden suite.** A small reference DBSP
  interpreter over the dataflow IR (Z-sets, lifted operators, nested streams
  for inductions) would give an executable semantics independent of the
  compiler's own backend. Goldens could then be cross-checked against a
  reference evaluation rather than only pinning the compiler against its own
  past output.
- **The other pole of item 2.** Z-set weights dissolve the recheck problem:
  retraction is a weight −1 tuple through the same bottom-up operators — no
  unknown state, no top-down checkers, no stack. The price is DBSP's cost
  model: integrated per-operator state (weights for intermediate results,
  distinct-threshold state). Item 2's design space is exactly
  bookkeeping-vs-reproof; DBSP is the max-bookkeeping/zero-reproof end,
  B/F the min-bookkeeping end. Any B/F proposal should argue its position
  on that spectrum against the DBSP alternative.
- **Alternative backend for differential testing.** Compile the dataflow IR
  to the `dbsp` crate and diff outputs against the generated C++ database on
  the whole case corpus.

Related: item 3's dual-number semi-naive trick is the same
differentiate/integrate idea in miniature.

## 5. 'Slow' functions as request/response message joins

Interfacing with a slow external system (an external database, an SMT
solver, a network service) is painful as an inline functor call: the
generated program blocks mid-fixpoint on every invocation. Reframe the slow
call as a server setup, using the message machinery we already have as the
async boundary:

    // conceptually, for a slow f demanded as  head(..) : body(X..), Y = f(X..).
    !f_request(X..)   : body(X..).          // outgoing message: the demand set
    head(..)          : body(X..), f_response(X.., Y).   // join on the reply
    // an external server consumes f_request, computes at leisure, and
    // publishes f_response(args.., result) back as an ordinary message

The program never blocks: the fixpoint quiesces with requests outstanding,
and when response batches arrive the incremental engine derives the
downstream consequences exactly as it would for any late-arriving message.
Deduplication and memoization fall out for free (the request relation is a
set; a persistent response relation is a memo table), and @differential
responses model external answers that change over time.

Design questions:
- Demand transformation: deriving the request relation from the bindings the
  rule bodies actually demand is the classic magic-sets / demand pattern —
  which binding patterns do we support, and is the request relation per
  (functor, binding pattern)?
- Surface design: an `@async` annotation on the functor declaration, e.g.

      #functor smt_check(bound Formula F, free Result R) @async.

  Uses in clause bodies stay ordinary functor applications, and the parse
  representation is untouched (print→parse→print fixpoint safe). The
  request/response shape is reified ONLY in the dataflow IR: where the
  builder would emit a MAP node for a plain functor, an `@async` declaration
  instead emits the demand set as an outgoing message (request) and models
  the result as a JOIN against the incoming response message — i.e. the
  message-based structure exists at the IR level and below, nowhere in the
  surface language (same philosophy as unit conditions: desugar in the
  builder, never the parser).
- This subsumes the impure-functor feature gap: impurity becomes explicit
  (responses are just messages that may arrive, change, or be retracted),
  and sync-vs-async is exactly the presence of the annotation choosing
  inline call vs request/response desugaring.
- Functional dependencies: is f_response constrained one-row-per-request
  (functor semantics), or do multi-row responses generalize slow functors
  into external relations (probably yes, and it is the more useful form)?
- Retraction: when body support for a request disappears, is the request
  message retracted, and are memoized responses garbage-collected or kept?
- Failure/timeout: absence of a response is indistinguishable from slowness
  (CALM-style); do we need explicit error/timeout response columns?
- Quiescence/termination: "the round is complete" now requires
  request/response accounting across the boundary (Dijkstra–Scholten-style
  termination detection) if callers need a consistent-as-of signal.
- Recursion through the boundary: a slow call whose response feeds a rule
  that generates new requests turns the fixpoint into a distributed loop
  across the external server — semantics and rate control.

Related: item 1 (an external system behaves like a sub-database keyed by the
request args); item 4 (Feldera handles the same need with async operators
over the same signed-update algebra).

## 6. Record data types (flyweight storage, pointer-like access)

Structured record types for column values — roughly JSON-shaped in syntax,
both for type declarations and for literals arriving in messages — that
compile to C++ records. The implementation discipline matters more than the
syntax: use the flyweight pattern as in the Carbon compiler toolchain
(Chandler Carruth et al. — dense integral IDs into typed, canonicalizing
value stores; see Carbon's SemIR design docs and Carruth's data-oriented
compiler-design talks), keeping a hard distinction between STORAGE and
ACCESS:

- Storage is integral/columnar: a record-typed column stores only a dense
  ID (u32/u64) into a per-record-type store; the store itself is columnar
  (one vector per field, not one heap object per record). Stores intern /
  canonicalize on construction, so ID equality IS value equality.
- Access is pointer-like: codegen emits cheap value-type handles (store +
  ID) with field accessors, so user code (functors, drivers, query
  consumers) reads them as if they held real pointers to structs; nested
  record fields are IDs into other stores, dereferenced through the same
  handle discipline.

Why this fits: relations, join pivots, table indexes, and the differential
machinery never see anything but integral values — records join, hash, and
dedup at integer speed, and tuple storage stays flat. Interning makes
record-valued join keys as cheap as scalar ones.

Design questions:
- Message boundary: ingress deserializes JSON-shaped literals by structural
  interning into the stores; what does egress publish — expanded structures
  or IDs plus store pages? (Ties to item 1: a sub-database instance likely
  owns or shares stores; store ownership must be settled there too.)
- Lifetime: interned values are append-only/immortal per store (arena
  semantics — differential retraction of tuples never frees record
  storage). Is that acceptable long-run, or does it need epoch compaction?
- Equality/ordering: interning gives identity equality; do we need
  structural ordering (for indexes/range scans) and how is it defined over
  nested records?
- Pattern matching in clause bodies (destructuring a record column into
  variables) vs accessor functors — surface design.
- Foreign-type interop: today's opaque foreign types (e.g. ASTNode as u64)
  are the degenerate single-field case; records generalize them without
  losing the integral-column property.
