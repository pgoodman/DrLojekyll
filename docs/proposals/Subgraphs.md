# Subgraphs / demand (seed ledger): parameterized reachability without materializing the middle

Status: SEED, recorded 2026-07-14 at the bench-harness close — harvested
from the owner's idea notes (the 2026-07-14 triage) while the context was
fresh. NOTHING here is designed, let alone implemented. Sequencing per
AggregatingFunctors.md §4: this is the LAST recorded epoch — after
runtime data structures → delta-relational IR → aggregating functors +
KV indices. Like every seed ledger, the implementing session must
re-derive, critique, and check everything below before building (the
F17/F18/F22/OQ6 precedent: every epoch's seed has contained a real
defect).

## The goal

Compute reachable conclusions without materializing every intermediate
relation: today every #local is a table; a demand mechanism should let
call-shaped sub-derivations run to a caller's need and leave nothing
behind. This is the magic-sets/demand territory, with the standing
architectural guardrail (recorded in the project memory and the
push-method history): SLDMagic-style ideas enter ONLY as dataflow
TRANSFORMATIONS gated by Stratify — never as a second evaluator, and
continuation-inlining stays confined within a stratum (the
continuation-recursion vs negation temporal-drift clash is what forced
the vector/fixpoint architecture in the first place).

## Harvested seeds (owner's notes, sharpened by the triage)

- S1 (the standout): **a multi-input subgraph containing a JOIN can be
  parameterized on the join keys** — keyed demand: the subgraph's
  instantiation key IS the join key tuple, so demanding a conclusion
  demands exactly the keyed slice of the middle. This is the technical
  core the epoch should be designed around.
- S2: **`@ephemeral` annotation** — a #local the programmer asserts
  need not be materialized; the compiler must prove it demand-safe
  (stratification + finiteness of the demanded slice) or reject with a
  clean diagnostic, golden-master style.
- S3: **subgraphs share the aggregate protocol** — independently
  reached by AggregatingFunctors.md §4: an aggregate object keyed on
  (group, config) IS a keyed nested instance in miniature; the keyed-
  instance substrate is shared, which is why subgraphs sequence AFTER
  aggregates. Config columns that are compile-time constants specialize
  the instantiation (the SLDMagic use_query_const move).
- S4: **a cut = subsequent clauses negating the preceding clauses'
  bound variables** — a candidate encoding of clause ordering/committed
  choice as ordinary stratified negation. Guardrail: the injected
  negation must remain stratified or it re-imports the exact clash the
  architecture exists to avoid; expect Stratify to reject some shapes,
  and that rejection is correct.
- S5 (recorded, weaker): KVINDEX-as-subgraph returning the latest value
  via a forcing message, possibly with an `@time` metavariable. KV
  indices already have a committed home as the degenerate aggregate
  (AggregatingFunctors.md §2/§4.1); this is an alternative lens to
  check the design against, not a replacement. Stateful-functor-as-
  database, coroutine-merging of top-down/functors/subgraphs, and the
  taint-based unblocker analysis from the owner's top-down notes are
  design-space material for this epoch's alternatives section.

## Constraints already known

- Demand must survive the differential contract: a demanded-and-dropped
  instance must not leave counter residue (the commit-sweep and
  membership-predicate invariants in CLAUDE.md apply to whatever a
  subgraph materializes transiently).
- The bench baselines (bench/BASELINE.md) price what materializing the
  middle costs today — nonlinear TC's tc⋈tc witness table is the
  measured cubic blowup a demand mechanism would attack; keep that case
  as the epoch's motivating benchmark.
- Perf motivation per the standing lens: McSherry-COST honesty (demand
  must beat both materialize-everything AND the competent hand-written
  query) and mechanical sympathy (keyed instances must not degenerate
  into pointer-chasing allocation soup; the data-structures epoch's
  layouts are the substrate).

## For the implementing session

Read first: AggregatingFunctors.md (the keyed-instance substrate and
sequencing), StackSafeNegation.md §5/§11 (the counter machinery demand
must respect), the push-method/negation history in the project memory,
bench/BASELINE.md (the costs demand exists to avoid). Method: the
checkpoint method as at every epoch — re-derive, adversarially critique
(S4's stratification hazard first), hand-write one worked demanded
program against goldens before touching the compiler.
