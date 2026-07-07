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
