# Draupnir — relevance assessment for Dr. Lojekyll

Read-only survey of `/Users/pag/Code/Draupnir` (nothing modified). All anchors are
`file:line` into that repo unless noted. Cross-references to Dr. Lojekyll use
`DrL:` and cite `/Users/pag/Code/DrLojekyll/...`.

## TL;DR

Draupnir is a **sibling** research prototype from the same problem space as
Dr. Lojekyll — "compile SQL/Datalog to incremental (IVM) imperative code" — but
built on a **different core abstraction**: the *Aggregate Relational Algebra*
(AGRA) over semiring/monoid-**annotated** relations (the DBToaster "ring" IVM
lineage), rather than Dr. Lojekyll's split-signed derivation-counter differential
tables. It is authored by Oliver Kennedy's group at University at Buffalo (git
remote `git.odin.cse.buffalo.edu`, `README.md:217` cites the DBToaster PODS-2010
ring paper), commits Sep-2024 → Jun-2026, and is **actively mid-refactor** and
notably less mature than Dr. Lojekyll (recursion "sentinels" landed only Jun-2026
with `todo!()` still in the lowering; no cost model exists yet).

It is a sibling / reference implementation, **not** a predecessor and not a
codebase to import from — but it holds 3-4 transferable *ideas*, one of which
(the property/capability data-structure-selection seam) lands squarely on
Dr. Lojekyll's active cost-model + seekable-data-structure directions, and
another (semiring annotations) on the provenance/aggregate-unification direction.

---

## 1. What Draupnir IS

A **multi-frontend query compiler + interpreter** written in Rust (`frontend/`,
~3-400 source files) with an auxiliary C++ backend/server (`backend/`, a separate
CMake project — a message-stream server, `backend/src/server.cpp`, largely a
skeleton).

- **Frontends** (all lower to one algebra): SQL (`ir/sql/`,
  `compiler/transform/sql_to_agra.rs`), raw Datalog "hedgelog"
  (`ir/datalog/`, `compiler/transform/hedgelog_to_agralog.rs`), a Soufflé-dialect
  reader (`ir/datalog/souffle/`, driven by the `weregeld` binary,
  `frontend/bin/weregeld.rs`), and AGRA directly.
- **Binaries** (`frontend/Cargo.toml:34-53`): `draupnir` (main), `dibtoaster`
  (a DBToaster clone — SQL → IVM trigger program, `frontend/bin/dibtoaster.rs`),
  `weregeld` (Soufflé Datalog runner), `draupnir-rocks` (RocksDB-backed).
- **Backends**: an in-process column-at-a-time **interpreter**
  (`interpreter/`, the mature part), plus code generators to C
  (`compiler/transform/imp_to_c.rs`), bytecode (`imp_to_bytecode.rs`), and a C++
  target (`lib/cpp/`). Storage can be in-memory or RocksDB.

**Core idea — AGRA / Aggregate-Annotated Relations** (`README.md:5-113`,
`ir/agra/typed.rs:22-113`): a relation is a *function from tuples to annotations*
`R(t)`. The annotation is a **monoid / group / semiring** (`README.md:25-31`):
a base type, `⊕` (plus, assoc+comm), `0`, optional `⊖` (negate), optional `⊗`
(times). Encoded as `AnnotationSpecification { zero, plus, negate, properties }`
at `ir/common/annotation.rs:97-104`, with `properties ∈ {Monotone, NonAbsorptive}`
(`annotation.rs:82-94`). Counts, booleans, and SQL aggregates (SUM/MAX/COUNT) are
all "just" different annotations over the same operators.

The algebra is **closed** over annotated relations; 9 operators
(`ir/agra/typed.rs:22-113`, grammar `README.md:48-59`):
`Relation | Unit | Empty(Zero) | Join[⊗] | Union[⊕] | Project | Filter |
Lift | Reannotate(Map)`. The novel ones are **Lift** (move a value from the
annotation domain into a tuple attribute — the expensive "coalesce" that realizes
an aggregate as data, `typed.rs:92-103`) and **Lower/Reannotate/Map** (move
tuple data into the annotation, cheap, `typed.rs:105-113`). Aggregation =
`Join → group-by Project → Lift`.

**Maturity / status**: research prototype, mid-refactor. `TODO.md` shows an
in-progress "Physical Plan Revamp", "Relation Revamp", "Expr Revamp" with
batch-at-a-time processing still unchecked. `todo!()` panics remain on live paths
(e.g. the recursion sentinel's lowering, `ir/pipeline/operator/sentinel.rs:106`).
Recursion ("sentinels" + a reactive event scheduler) is the newest and least
settled subsystem (git: "Reactive scheduler lives (mostly)", "The scheduler seems
to be working... now we just need sentinels", Jun-2026 tip).

## 2. Architecture sketch (key abstractions + algorithms)

**Compilation pipeline** (`README.md:135-144`): hedgelog → agralog (typed AGRA) →
**logical pipeline** → **physical pipeline** → backend. AGRA is the semantic
waist; everything routes through it.

**The IVM / differential machinery** — two flavors, both *pure IR→IR rewrites*:

- **`differentiate_agra.rs`** — classic finite differencing on an AGRA query.
  `differentiate(query, {R↦ΔR})` returns `Option<Query>` (`None` = independent of
  the changed tables). The join rule is the textbook DBToaster product rule
  (`differentiate_agra.rs:37-65`):
  `d(q₁⋈q₂) = q₁⋈dq₂ ∪ dq₁⋈q₂ ∪ dq₁⋈dq₂`. Union/Project/Filter push through
  homomorphically. **Lift and non-homomorphic Reannotate can't be differentiated
  algebraically** and fall back to `slow_ass_undifferentiable_delta`
  (`differentiate_agra.rs:126-135`): recompute `f(old ∪ Δ) ⊖ f(old)` using the
  annotation's negate. The homomorphic/non-homomorphic split is decided by
  annotation/morphism properties (`typed.rs:249-252`, `annotation.rs:82-94`).
- **`agra_to_trigger_program.rs`** — higher-order IVM à la DBToaster: recursively
  materializes delta-views (`ivm_triggers`, `:24-100`; `extract_ivm_views`,
  `:102-209`), dedups equal views (`:40-45`), and emits a `TriggerProgram`
  (per-base-table `Trigger`s that maintain a set of `View`s).

**Logical pipeline + recursion** (`ir/pipeline/logical.rs:351-402`): a dataflow
graph of source/sink-bound operators. Recursion is modeled as a **stack of
identical pipeline layers** with `VersionReference` edge labels
`{Current, Previous, Total, FixedPoint}`; all flow is monotone *up* the stack
except `FixedPoint`, so each layer stays a DAG and the stack stratifies — a
clean **semi-naive-by-layering** formulation. Fixpoint is detected when a layer
produces no new writes (`logical.rs:385-389`). Recursion at runtime is guarded by
**Sentinel** operators (`ir/pipeline/operator/sentinel.rs:38-62`) that compare
`BASE-`/`DIFF-` inputs per trigger and a **reactive event scheduler**
(`interpreter/eval/pipeline/operator.rs:280-290`, "operator state machine").

**Physical plan = property/capability matching** (the piece most relevant to
DrL — see §4). `ir/pipeline/mod.rs`:
- `SinkConstraints` (`:110-141`) — what a materialization *must* provide:
  `coalesced, resettable, passive, clustered:Set<key>, sorted:Set<key>,
  versioned`. `SourceConstraints` (`:225-242`) is the read-side dual.
- `DataStructureCapabilities` (`:350-432`) — what a data structure *offers*, with
  `capabilities.supports(&constraints) -> bool` (`:364-407`) as the declarative
  match. Backends register a menu of structures
  (`interpreter/data_structure/mod.rs`: `RING_BUFFER`, `IN_MEM_ARRAY`,
  `INDEXED_ARRAY`, `ROCKS`, `VERSIONED_BUFFER`).
- Selection today is *first-fit, no cost* (`pick_data_structure`,
  `logical_pipeline_to_physical_pipeline.rs:713-736`: "we're not going to do any
  better until we get a proper cost-based model").

**"Coalesced" as a tracked physical property** (`typed.rs:497-531`):
`preserves_coalesced()` / `is_coalesced()` reason about whether an operator's
output has one entry per non-zero tuple. Joins preserve it; multi-input Unions
don't; Projection only if provably injective (`is_definitely_injective`,
`typed.rs:187-208`). This drives where an explicit coalesce/materialize must be
injected — a static materialization-point analysis.

**Read/write-set analysis** (`compiler/analyze/readsetanalysis.rs`, also its own
git branch): builds per-event `RWSet { reads, writes: RelationId ↦ [[Accessed]] }`
over the imperative IR (`:19-71`), with `Accessed ∈ {Var, Const, Wildcard}` and an
explicit "check whether two events have a conflict / determine overlap" goal
(`:9-11`) — i.e. coordination/commutativity analysis over event handlers.

**No cost model exists.** It is explicitly a hole at all three decision points:
data-structure selection (`logical_pipeline_to_physical_pipeline.rs:719`),
join ordering (`backend/mod.rs:53` "Eventually we'll have a proper cost-based
optimizer here"), and the data-structure abstraction itself
(`ir/pipeline/mod.rs:329` "we also want to add some sort of a cost function").
Join reordering today is a heuristic "pull up the delta'd relation"
(`compiler/optimize/reorder_joins.rs`).

## 3. Relevance to Dr. Lojekyll

### Same problem, different approach (OVERLAP)
Both compile Datalog (Draupnir also SQL) to **incremental, maintained** query
results. Both separate a logical algebra from a physical/control layer and treat
IVM as a program transformation. Both stratify recursion and detect fixpoints by
"no new output".

### Where they DIVERGE philosophically
| Axis | Draupnir | Dr. Lojekyll |
|---|---|---|
| IVM substrate | semiring/monoid **annotations** (`R(t)` payloads); delta = algebraic `d(·)` + negate-annotation | split **signed derivation counters** `C_nr/C_r`, OVERDELETE→REDERIVE→INSERT (`DrL: docs/proposals/StackSafeNegation.md`) |
| Aggregates | first-class: *any* monoid annotation, Lift/Lower operators | special-cased R3 `GROUP_UPDATE` + `StateCellStore` (`DrL: CLAUDE.md` "Aggregates + KV") |
| Delta form | recompute-with-negate fallback for non-invertible ops (`differentiate_agra.rs:126`) | dedicated differential tables + commit sweep, no whole-view recompute |
| Output | interpreter-first (column-at-a-time), codegen secondary/WIP | codegen-first (hidden-friend C++ header + zero-dep runtime), mature |
| Recursion+diff | new, `todo!()`-gapped sentinels | landed, validated, demand + keyed instances |
| Testing/rigor | unit tests, some `todo!()` on live paths | 180-case golden suite, always-on graph validators, oracle |

### Where Draupnir is AHEAD (ideas DrL could adopt)
1. **Semiring annotations as a *unifying* abstraction.** DrL bolts aggregates, KV
   indices, and (future) provenance on as separate machinery; AGRA derives all of
   them from one annotation algebra with two properties (`Monotone`,
   `NonAbsorptive`). This is the same lattice/semiring framing DrL's
   provenance/keyset and CALM directions are reaching toward.
2. **A declarative physical-property ↔ data-structure-capability matcher**
   (`ir/pipeline/mod.rs:350-432`). DrL currently hard-wires table/index shapes in
   codegen; Draupnir has a *seam* where "this flow needs {clustered on k, sorted,
   versioned}" is matched against a menu of structures. This is exactly the socket
   a cost model and the seekable-iterator / factorized-layout work plug into.
3. **`coalesced` / injectivity as static properties** that decide where to
   materialize — a principled version of DrL's implicit "which views get tables".
4. **Read/write-set conflict analysis** (`readsetanalysis.rs`) as an explicit
   artifact for coordination-freeness (CALM) and for demand (which subgoals an
   event touches).

### Where Dr. Lojekyll is AHEAD
End-to-end codegen + zero-dep runtime; working recursion-with-differential;
demand/magic-sets + keyed-instance lowering; the Rel delta-IR as a *checked*
scheduling authority with always-on validators; golden+oracle test rigor.
Draupnir's recursion/scheduler is where DrL was several epochs ago.

### Verdict
**Sibling / reference implementation** (shared academic lineage: DBToaster/IVM,
Buffalo). Not a predecessor DrL derived from, and not a source to import code
from (different language, less mature). Its value is **conceptual**: it is the
"semiring-annotation + physical-property-matching" point in the design space,
and a live worked example of the exact cost-model seam DrL is now building.

## 4. Most transferable ideas (tiered)

**T1 — LOW effort, HIGH payoff — the property/capability matcher as the cost seam.**
Adopt the *shape* of `SinkConstraints`/`DataStructureCapabilities.supports()`
(`ir/pipeline/mod.rs:110-141, 350-432`) as the interface the DrL cost model scores
against: a flow declares required physical properties; candidate data structures
declare capabilities; the cost model *ranks* the feasible set instead of
first-fit. Draupnir explicitly left the ranking hole open
(`logical_pipeline_to_physical_pipeline.rs:713-736`) — DrL can leapfrog by
filling exactly that hole. Directly serves `CostModel.artifacts/` (the seed +
data-structure-selection direction). Effort is low because it's an interface
pattern, not code to port.

**T2 — MED effort, HIGH payoff — `coalesced`/injectivity materialization analysis.**
`preserves_coalesced()`/`is_definitely_injective()` (`typed.rs:187-208, 497-531`)
are a compact static analysis for "where must we pay to materialize/dedup". DrL's
cost model needs precisely this to price a plan (each forced coalesce/table is a
cost). Portable as a concept onto DrL's Query graph.

**T3 — MED/HIGH effort, HIGH payoff (long-horizon) — semiring annotations to unify
aggregates + provenance.** The `AnnotationSpecification` monoid/group/semiring
framing (`annotation.rs:82-104`) with `Monotone`/`NonAbsorptive` properties is the
principled home for DrL's aggregates, KV indices, *and* the provenance/keyset
work — and its monotonicity property feeds CALM reasoning directly. High effort
(touches the IVM substrate) but it is the theoretically-load-bearing idea and
aligns with DrL's stated provenance direction. Treat as a reference model, not a
rewrite.

**T4 — LOW/MED effort, MED payoff — RW-set conflict analysis for CALM/demand.**
`readsetanalysis.rs`'s per-event reads/writes access-sets + overlap test are a
ready template for a DrL analysis answering "can these two message handlers run
without coordination" and "which relations does a demanded query touch".

### Anti-transfer (do NOT adopt)
- The recompute-with-negate delta fallback (`slow_ass_undifferentiable_delta`,
  `differentiate_agra.rs:126`) — DrL's differential tables already avoid full-view
  recompute; adopting it would regress.
- The reactive event scheduler / sentinel machinery — DrL's Rel-IR Kahn linearizer
  + validators are more mature and checked.
- Interpreter-first column-at-a-time execution — orthogonal to DrL's codegen model.

## Appendix — file map (Draupnir)
- AGRA IR + operators: `frontend/lib/ir/agra/typed.rs`
- Annotations (semirings): `frontend/lib/ir/common/annotation.rs`
- Differentiation: `frontend/lib/compiler/transform/differentiate_agra.rs`
- Higher-order IVM triggers: `frontend/lib/compiler/transform/agra_to_trigger_program.rs`
- Logical pipeline + recursion-by-layering: `frontend/lib/ir/pipeline/logical.rs`
- Physical property/capability matching: `frontend/lib/ir/pipeline/mod.rs`
- Data-structure selection (cost hole): `frontend/lib/compiler/transform/logical_pipeline_to_physical_pipeline.rs:713`
- Data structures / storage: `frontend/lib/interpreter/data_structure/`
- RW-set analysis: `frontend/lib/compiler/analyze/readsetanalysis.rs`
- Recursion sentinels + scheduler: `frontend/lib/ir/pipeline/operator/sentinel.rs`, `frontend/lib/interpreter/eval/pipeline/`
- C++ backend (skeleton server): `backend/src/`
</content>
</invoke>
