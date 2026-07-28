# Proposal: `@ephemeral` types — values that MUST NOT be persisted

Status: RECORDED 2026-07-28 (owner-directed ideation, in-session with the
epoch re-rank pending). NOT ranked, NOT ruled — this file exists so the
design conversation is not lost. Anchors verified at tip 4d3ae315. If the
feature is ever ranked, it enters the standing per-slice design ritual and
the open questions at the tail get ruled at the ritual head.

## The idea

Mark a value as *ephemeral*: it may flow through the dataflow and be
consumed, but it MUST NOT be persisted anywhere in the generated database —
no table, no index, no checkpoint. A consumer that wants the value later
must RE-RUN the functor that produced it.

Two spellings, seeded in this order but converged on the second as primary:

1. **Per-parameter** (the seed idea): `#functor blah(bound Type X,
   free ephemeral Type Y)` — this particular output is never stored.
2. **Type-level (primary)**: `@ephemeral` on a `#foreign` type declaration —
   ALL uses of the type are ephemeral, program-wide. House pragma grammar
   (the `@invertible`/`@recompute`/`@never` family, duplicate-pragma clean
   reject per the `algebra_dup_1` idiom).

Type-level is primary because it **dissolves taint propagation**: a
`Secret`-typed column is `Secret`-typed wherever it is copied (TUPLE
forward, MERGE, SELECT, JOIN pass-through) — no propagation rules exist to
get wrong. The ONLY laundering points are functor signatures that consume
the ephemeral type and emit a non-ephemeral output — a visible, greppable
audit surface. The per-parameter form remains the orthogonal follow-on for
"this ordinary-typed output is a nonce" and DOES need a real taint pass.

## The load-bearing fork: two readings, kept separate

- **Reading 1 — PURE but never stored (START HERE).** The functor stays
  pure/deterministic; `@ephemeral` is a storage-placement constraint.
  Anywhere the value would be stored, store its *recipe* (the bound inputs,
  which are persisted) and re-run on read. Differential maintenance stays
  sound: OVERDELETE/REDERIVE re-runs the functor and reproduces the same
  value, so counters net correctly. CSE across identical invocations
  remains valid.
- **Reading 2 — impure/volatile ("the value changes").** Fresh tokens,
  timestamps, sensor reads. Only sound if ephemeral-influenced views are
  confined to a TERMINAL FRINGE (query cursors + published messages,
  nothing folding into a table) — otherwise REDERIVE derives different
  tuples and corrupts counters, which is exactly why impure functors are
  rejected today. If ever wanted, this is a SEPARATE spelling (`@volatile`?)
  layered later; conflating it with `@ephemeral` would hand impurity a path
  into the fixpoint. Under Reading 2 CSE must NOT fold invocations.

## What the architecture gives for free

- **Placement is one authority.** Table placement is a single compiler
  decision (FillDataModel + the DataModel union-find), so "no table holds
  this type" is checkable structurally. The check MUST bind at the MODEL
  layer (the eqset partition, observable since OD-13) — the E-107 lesson:
  a `.df class=table-less` view is often model-table-BACKED via
  equivalence-set sharing.
- **Equality exemption.** The engine runs on value equality (table dedup,
  index keys — the Fold A Table.h contract is the Key's memberwise
  `operator==`). If ephemeral-typed columns can never be stored, indexed,
  pivoted, negated, or aggregated, the compiler NEVER needs `==`/hash for
  them — a principled exemption, exactly right for borrowed-handle foreign
  types (pointer identity ≠ value identity). Generated code only ever
  forwards them positionally within one delivery's extent — which is
  already the house cursor/hook contract (drain fully before the next
  entry point).

## Sink list (slice 1: every one a clean diagnostic)

An ephemeral-typed column may not appear in: a table-assigned view's
schema (model-layer check), an index key, a JOIN pivot, a negation key, an
aggregate `over()` list, a `mutable()` KV param, or an INPUT message
schema (a receive is the canonical materialization point). OUTPUT messages
are ALLOWED — egress is not persistence; delivery-extent validity matches
the existing hook contract. Enforcement: a V-EPHEMERAL always-on validator
(fprintf+abort idiom) + parse/dataflow clean diagnostics; corpus witnesses
are diagnostic cases (the `kvindex_2`/`algebra_dup_1` class), so slice 1
churns ZERO goldens.

## Laundering and `@declassify` (policy knob, unruled)

"Declassification" (the IFC term, Jif/FlowCaml lineage): a functor
consuming the ephemeral type and emitting ordinary data (e.g. `#functor
hash(bound Secret S, free u64 H)`) drops ephemerality by signature. Knob:
allow implicitly (fine for the space/recompute motivation), or require an
explicit `@declassify` pragma on such functors (accidents impossible;
`grep @declassify` = the entire security-audit surface). Soundness is
identical either way; rule it if/when the feature is ranked, informed by
whether real `@ephemeral` types are mostly secrets/handles or mostly fat
recomputable intermediates.

## Slice plan (if ranked)

1. **Reject-only** (mini-diff class — the `:-` separator / PIN-3 size):
   parse `@ephemeral` in lib/Parse/Foreign.cpp; a bit on
   ParsedForeignTypeImpl (`ParsedForeignType::IsEphemeral()`,
   include/drlojekyll/Parse/Parse.h:1021); the sink checks + V-EPHEMERAL;
   diagnostic corpus witnesses; Language.md. No recompute machinery.
2. **Recompute-on-read** (widens admissibility): inline the producing
   functor call at each consumption site instead of reading a stored
   column; naturally HOSTED ON THE DEMAND MACHINERY (pull semantics) —
   sequence AFTER D3.a makes that machinery differential-capable and
   multi-adornment.
3. **Optional `@volatile` tier** (Reading 2, fringe-confined) — separate
   spelling, separate ruling, maybe never.

## Placement among the standing directions

- **Agent-substrate (strongest synergy).** The substrate's premise is
  persisted, replayable, hashable state (V13 = canonicalized state hash);
  credentials/tokens/handles are precisely what must not be in it.
  `@ephemeral` gives the substrate a headline property — the database can
  PROVE secrets are not in the checkpoint — and V13 excludes ephemeral
  values from the state hash BY CONSTRUCTION. If agent-substrate is ranked
  as the next epoch, fold slice 1 into its Phase-0/1 deliverables.
- **WASM functors.** An ephemeral foreign type = a view into wasm linear
  memory valid for one batch call, manifest-declared — the compiler-level
  MUST and the ABI-level can't-even coincide. Landing the pragma early
  gives the future ABI an existing semantic slot.
- **D3.a / demand.** Sequencing only: slice 1 any time; slice 2 after
  D3.a.
- **Fold C / P2-P5 / §13 / §14.** No interaction.

Slice 1 is deliberately NOT an epoch candidate — it is a between-slices
mini-diff wherever the re-rank lands; its best home depends on that ruling.

## Open questions to rule at the ritual head (if ranked)

1. `@declassify` required vs implicit laundering.
2. Input-message hard reject vs a table-less-receive admission analysis
   (recommend hard reject for slice 1).
3. Whether output-message hooks should carry any marking so a driver knows
   the delivery-extent contract applies (or whether the generated-header
   type itself communicates it).
4. The per-parameter `free ephemeral` follow-on: adopt or drop once
   type-level lands.
5. Dump surface: does `.df`/`.rel` render an `ephemeral` attribute token
   (an E-71 grammar lane) — slice 1 needs none (diagnostics only).
