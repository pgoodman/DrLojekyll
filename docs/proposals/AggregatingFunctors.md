# Aggregating functors and KV indices — design ledger (pre-implementation)

Status: DESIGN ONLY, recorded 2026-07-12 at commit d17b177. Nothing here is
implemented; the CF build still rejects AGG/KVINDEX with the F14
diagnostics, and that is correct until the sequencing gate below is met.
This file exists so the design conversation survives sessions; it is a
seed hypothesis in the sense of the checkpoint ledgers — the implementing
session must re-derive, critique, and trace it before code.

Owner's core idea (Peter): an aggregating functor instantiates a (notional)
C++ object with TWO levels of group-by — the OUTER group-by, and an INNER
level given by the functor's constructor arguments — plus feeding input
arguments and outputs. KV indices (@mutable params with merge functors)
are the degenerate case of the same machinery.

## 1. What already exists (verified at d17b177)

- Dataflow IR: `QueryAggregate` already reifies the whole two-level shape:
  `GroupColumns()` (outer group-by), `ConfigurationColumns()` (the inner
  level = constructor arguments), `SummaryColumns()` (feeding inputs), and
  the aggregate output columns (include/drlojekyll/DataFlow/Query.h:~652).
  Parse + dataflow build/canonicalization handle it; `KVINDEX` nodes
  likewise exist. Corpus: data/examples/average_weight.dr,
  pairwise_average_weight.dr.
- The CF build rejects both with clean diagnostics (the F14 pre-pass).
- MD §11 OQ12 records the semantic gap: non-monotone aggregates (MIN/MAX)
  need retraction-aware per-key state that counters do not provide; linear
  aggregates (COUNT/SUM) become natural under a weight extension.

## 2. Differential semantics (the design)

An aggregate is non-monotone exactly like negation, and inherits the
(d)-era discipline wholesale:

- STRATIFICATION: Stratify places AGG like NEGATE — summarized inputs must
  be strictly lower-stratum; in-SCC aggregation is rejected with a
  diagnostic (sibling of the unstratified-negation error).
- PHASE-FINAL READS: the aggregate reads its input stratum after that
  stratum's frontiers are final; its own outputs are ordinary differential
  rows (the aggregate is their sole deriver, class kNonRecursive) riding
  the existing claim/frontier/commit machinery.
- OUTPUT CONTRACT: when a group's value changes within a batch, emit ONE
  net pair — retract the old output row, assert the new one — at the
  stratum boundary (touched-group frontier vector; k touches in a batch
  still emit one pair). Commit's was!=now publish then behaves exactly as
  for every other differential table.
- NETTING COMPOSES: a group touched only by an annihilated adds∩removes
  pair (OQ3) sees no state change and emits nothing.
- KV INDEX = DEGENERATE AGGREGATE: group = the key columns; state = one
  merged cell; the @mutable merge functor is the aggregator; output = the
  current cell value. One machinery closes both feature gaps — do not
  design KV indices separately.

As a diff against the whole-program pseudocode
(checkpoint-e-notes.md §1), per AGG at its stratum:

    stratum phases, after the input stratum's frontier filters:
    + AGG-UPDATE (one per AGG view):
    +   for {row,±} in input net frontiers (net_removals ⇒ −, additions ⇒ +):
    +     g := (group cols, config cols) of row
    +     state(g).fold(±, summary cols)          « touched-group set accrues g »
    +   for g in touched groups (sort-unique):
    +     new := state(g).emit()
    +     if new ≠ old(g):
    +       fold −NR {g, old(g)} into agg table → crossings → queues
    +       fold +NR {g, new}    into agg table → crossings → queues
    +   « then the agg table's existing claim drains / frontier filters run »

    row-state additions: a per-AGG keyed STATE CELL store beside the
    tables — new standing state with commit-time sealing like kInI; the
    oracle referees by from-scratch recomputation per group.

## 3. The functor ABI (constraints already decided)

- DECLARED ALGEBRA via @-pragmas on #functor (the .dr annotations are the
  single-source manifest): @pure/@deterministic; aggregate classes
  @invertible (abelian: fold+unfold, O(1) both directions — COUNT/SUM/AVG),
  @commutative/@associative (merge exists ⇒ tree/parallel reduction),
  @idempotent (order/multiplicity-insensitive — ALSO a correctness lever:
  immune by algebra to duplicate-emission artifact classes), @recompute
  (opaque fallback: re-run over the group's membership on touch).
  Annotations can lie: debug/oracle runs property-check the declared laws
  (unfold inverts fold on sampled inputs, etc.), DebugValidateCounts-style.
- ENGINE-OWNED STATE where the class permits: the functor supplies
  init/fold/unfold/merge/emit over a state blob whose layout it declares;
  the ENGINE lays blobs out columnarly by dense group id. The
  object-per-group model is the semantic contract and the opaque-functor
  fallback, not the default physical layout.
- BATCH-SHAPED, never tuple-shaped: the ABI hands columnar delta batches
  across the boundary (this is what makes the wasm path viable and is what
  the delta-relational IR wants regardless).
- PER-GROUP RECOMPUTE THRESHOLD: recompute-on-touch is the universal
  fallback and often wins for small groups (COST honesty) — same opt-in
  shape as the subgraph instances' recompute mode.
- WASM DIRECTION (see memory wasm-functor-direction, F3 spirit): functors
  eventually wasm-compiled. Two composable stories: (a) whole-module —
  generated C++ + functors LTO'd via clang --target=wasm32-wasi into one
  artifact (runtime is dependency-free by design; inlining recovered by
  LTO; candidate 5th OptDiff execution variant byte-compared against the
  same goldens); (b) late-bound functor modules (the true F3 shape) —
  requires this batch ABI + manifest. LLVM-IR interop bonuses: functors in
  any LLVM language; ORC-JIT runtime specialization of keyed instances
  (SLDMagic's use_query_const done by JIT). A clad-shaped Clang plugin
  (NOT clad itself — it does arithmetic AD, not change-structure
  derivatives) is the eventual path from a user's plain fold to its
  synthesized delta form/inverse, verified by the property tests; the same
  plugin slot serves manifest extraction and algebraic-law linting sooner.

## 4. Sequencing (decided direction)

Gate: AFTER the delta-relational IR exists — aggregates are its inaugural
operator family (GROUP-UPDATE is the first operator whose delta semantics
is NOT expressible as the §5.1 counter schemas; it forces the IR's
state-cell story and pays for the layer). Landing it earlier means a third
hand-built Stratum.cpp-style scalar emission web that the IR would rewrite.
Full order: Stage-3 close (E0 + (e)) → bench harness (COST instrument) →
runtime data structures → delta-relational IR (AGG/state-cell operator in
the initial design) → aggregating functors + KV indices → subgraphs
(sharing the keyed-instance substrate: an aggregate object keyed on
(group, config) IS a keyed nested instance in miniature; config columns
that are compile-time constants specialize the instantiation — the same
lexical-scope move as the carve-outs and SLDMagic's use_query_const knob).

Cheap early work permitted without violating the gate: the functor-ABI
one-pager (@-pragma grammar + init/fold/unfold/merge/emit contract +
manifest fields); a wasm whole-module build spike (one OptDiff case,
byte-compared) once the bench harness exists.

## 5. For the implementing session

Method: the checkpoint method (F17/F18 precedent). Re-derive this file
from Query.h/the corpus; write the AGG-UPDATE diff against the then-current
whole-program pseudocode; adversarially critique (minimum: the
one-net-pair-per-group batching vs claim-gate interactions; MIN/MAX
retraction state; oracle mirroring; stratification diagnostic scope —
whether monotone aggregates over monotone inputs may relax it); write the
desired emitted-IR end state for average_weight.dr concretely; hand-trace
a retraction batch through a MIN group and an @invertible SUM group before
code. Workflows: opus for traces/critique/implementation/review, sonnet
for mechanical audits and suite gates. Verification: the standing golden
policy; new fixtures from oracle truth (the oracle needs its per-group
recompute referee first).
