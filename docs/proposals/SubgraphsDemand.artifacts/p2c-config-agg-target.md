# P2 stage (c) — CONFIG-COLUMN AGGREGATES: hand-written target artifact

Lane P2(c), subgraphs/demand epoch (branch `subgraphs-demand`, P0 census
landed). Closes the CLAUDE.md clean-diagnostic gap "aggregates with
configuration columns". Binding inputs: ledger erratum **E-31** (the fork:
config-as-KEY is threaded end-to-end and needs only the
`lib/ControlFlow/Build/Build.cpp:1108` fence lifted + corpus; a
config-DEPENDENT reduction needs the C-5 free-
function ABI extended so the reduction body receives the config value — and
the corpus case MUST exercise a config-dependent reduction) and
`v3-spec-statecell.md §1-§2`.

This artifact is written so a future implementer builds FROM it and a judge
can falsify it. Every code claim carries a `file:line` anchor read this
session (branch tip; anchors are HEAD line numbers on `subgraphs-demand`
with the P0 census landed).

---

## 0. The E-31 fork, resolved empirically

Two facts, both re-verified this session against the real compiler:

**FACT A — config-as-partition-KEY is already threaded end-to-end.** The
`(group ++ config)` projection lives in `DRStateCell::key_cols`
(`DR.cpp:1019-1025`: `group.push_back` over `InputGroupColumns()` THEN over
`InputConfigurationColumns()`), flows to `ProgramStateCell::key_types`
(`Stratum.cpp:2058-2059`, `desc.key_types.push_back(col.Type())` over
`cell.key_cols`), and emits as `Key_<id>` with one field per key column
(`Database.cpp:1082-1090`, `EmitHashStruct` over `cell.KeyTypes()`). The
agg-table row is `(group ++ config ++ summary)` in column order
(`Database.cpp:2023-2033`, `agg_tuple_exprs` = `key.c0..key.c<n-1>` then
`val`). The witness `Key_2 {c0; c1;}` in the average_weight emission
(multi-column KV key `edge_weight(From,To)`, datalog.h:205-212) proves
multi-column keys already emit correctly; a config column is just another
leading key column. **So if the reduction did NOT depend on config, lifting
the fence alone would be sufficient.**

**FACT B — the reduction ABI never receives config.** `EmitGroupUpdate`
folds only the SUMMARY positions: `Database.cpp:2004-2015` forms
`key_parts` from `gpos` (GroupPositions, which already INCLUDE config) but
passes only `summary_expr` (from `spos`) to `Fold`
(`sc.Fold(gid, sign, summary_expr)`, `Database.cpp:2014`). The runtime
`StateCellStore::Fold` forwards only `Summary_&&... s` to
`Algebra::Fold(w, sign, Summary(...))` (`StateCell.h:305-323`); both
`Invertible::Combine`/`Uncombine` (`StateCell.h:117-123`) and
`Recompute::ReduceLive` (`StateCell.h:193-199`) see only the summary value.
**A config-DEPENDENT reduction — the natural case, a bound `Threshold`
gating the fold — has no path to the threshold today.**

**Per E-31, this artifact's corpus case MUST exercise a config-DEPENDENT
reduction, or the gap ships un-caught.** `config_agg_1` below uses
`sum_above(bound Threshold, aggregate Val, summary Sum)` — a per-group
threshold gating which values enter the sum — precisely to force FACT B's
ABI extension into the diff.

**Empirical confirmation (this session):** the case parses, passes
dataflow, and reaches EXACTLY the `lib/ControlFlow/Build/Build.cpp:1108`
reject ("Aggregates with configuration columns are not yet supported") — no
earlier and no other diagnostic. Reproduced with the real
`build/debug/bin/drlojekyll` on the source in §1. A malformed first attempt
(config var only in the surrounding clause, not the over-block) instead hit
"Could not find configuration variable 'T'" from `lib/DataFlow/Build.cpp:1080`
— see §1 for why the config var must appear in the over-block signature.

---

## 1. The `.dr` source for `config_agg_1`

### 1.1 Where config columns come from (the parse/dataflow contract)

A configuration column is a functor parameter with the `bound` binding
(`Aggregate.cpp:33-36`: a `bound` param → `impl->config_vars.AddUse`). In
the dataflow builder `ApplyAggregate`, a config var is resolved by
`FindColVarInView(context, base_view, var)` where `base_view` is the
over-block's SELECT (`lib/DataFlow/Build.cpp:1076-1089`). **The config
variable must be a column produced INSIDE the over-block** (range-restricted
by the block body) — exactly like a group variable. If it is only bound in
the surrounding clause, `FindColVarInView` returns null and
`lib/DataFlow/Build.cpp:1080` fires "Could not find configuration variable".
This is a hard constraint on
the corpus source (verified empirically this session).

The config columns are input-view positions: `config_columns` are USES of
columns in `base_view` (`lib/DataFlow/Build.cpp:1087`,
`view->config_columns.AddUse(col)` where `col = FindColVarInView(...)`), and
`InputConfigurationColumns()`
returns those uses (`Query.cpp:1028-1032`). So the config value **rides the
input frontier row** and is available to `pos_of` in `LowerGroupUpdate`
(`Stratum.cpp:1396-1404`, position in `input.Columns()` order) — the
mechanism §2 exploits.

### 1.2 Config source decision: RELATION-fed (the general form)

Per the lane charter, the corpus case uses a **relation-fed config** (the
runtime-varying demand key — the general form). The threshold is a fact in
a relation `threshold(Sensor, T)` populated by a message; it is joined
INSIDE the over-block so `T` is range-restricted there. The compile-time
constant specialization (a literal config: `sum_above(50, Val, Sum)`) is
candidate-(c)'s later half and is NOT exercised here — but see §4.6 for
why the diff must not accidentally special-case literals.

### 1.3 The source (`tests/OptDiff/cases/config_agg_1.dr`)

```datalog
; config_agg_1: a CONFIG-DEPENDENT aggregate. sum_above(bound Threshold,
; aggregate Val, summary Sum) sums only the readings whose Val >= the
; per-sensor Threshold. The threshold is RELATION-FED (threshold/2, set by a
; message) and joined INSIDE the over-block so it is range-restricted there
; and becomes a CONFIGURATION column of the aggregate (Aggregate.cpp:33-36 /
; lib/DataFlow/Build.cpp:1076-1089). This is the smallest program that forces BOTH the
; config-as-key projection (already threaded, E-31 FACT A) AND the
; config-dependent reduction ABI (the E-31 FACT B extension): the fold body
; must see the threshold to decide whether each Val enters the sum.
;
; Group key = (Sensor, Threshold); the aggregate output is
; total_above(Sensor, Total). Threshold is a config column: it partitions
; the state cell AND gates the reduction.

#functor sum_above(bound i32 Threshold, aggregate i32 Val, summary i32 Sum) @invertible.

#local reading(i32 Sensor, i32 Val).
#local threshold(i32 Sensor, i32 Threshold).
#message add_reading(i32 Sensor, i32 Val).
#message set_threshold(i32 Sensor, i32 Threshold).
#query total_above(bound i32 Sensor, free i32 Total).

reading(Sensor, Val) : add_reading(Sensor, Val).
threshold(Sensor, T) : set_threshold(Sensor, T).

total_above(Sensor, Total)
  : sum_above(T, Val, Total)
    over (i32 Sensor, i32 T, i32 Val) {
      threshold(Sensor, T),
      reading(Sensor, Val)
    }.
```

Notes:

- `T` appears in the over-block signature `over (i32 Sensor, i32 T, i32 Val)`
  and is range-restricted by `threshold(Sensor, T)` inside the block. This is
  REQUIRED (see §1.1) — the earlier attempt with `T` only in the surrounding
  clause failed `lib/DataFlow/Build.cpp:1080`.
- The over-block's base view is a JOIN of `threshold(Sensor, _)` and
  `reading(Sensor, _)` on `Sensor`. Its columns (input-view space) are
  `(Sensor, T, Val)` — group = `{Sensor}`, config = `{T}`, aggregate =
  `{Val}`. The single summarized input for the GROUP_UPDATE is that join's
  differential table (walked to via `DR.cpp:653-662` if the join is
  table-less plumbing; here the join owns a table).
- `sum_above` is `@invertible`: `sum_above_combine(threshold, w, v)` adds `v`
  iff `threshold <= v`; the inverse `sum_above_uncombine(threshold, w, v)`
  subtracts `v` iff `threshold <= v`. Invertibility holds because the
  threshold is CONSTANT per group (it is in the key) — the same predicate
  gates the fold and the unfold, so `uncombine ∘ combine = id` per group.
  This is why the ABI must pass config to BOTH `_combine` and `_uncombine`.
- A `@recompute` variant (`sum_above` marked `@recompute`) would instead pass
  the threshold to `_reduce`. §2.5 records that ABI too; the corpus ships the
  `@invertible` form as the tighter (O(1)) witness, but the ABI extension in
  §4 covers both.

### 1.4 Second corpus case (recommended companion): `@recompute` config-dep

To exercise the `@recompute` config-dependent ABI arm as well (§2.5), a
companion `config_agg_2.dr` uses `max_above(bound Threshold, aggregate Val,
summary Max) @recompute` — the max of values ≥ threshold. This is OPTIONAL
for closing the clean-diagnostic gap (`config_agg_1` alone lifts the fence
and exercises the ABI); it is recommended so both algebra arms of the
extended ABI are covered. The pre-registered predictions (§6) count only
`config_agg_1` toward the 164→165 growth; `config_agg_2` would make it
164→166 and is called out separately.

---

## 2. The expected DR-IR (v3-spec §2.1 effect-table style)

### 2.1 The GROUP_UPDATE op for `config_agg_1`

Built by `BuildGroupUpdateOps` (`DR.cpp:638`), called per aggregate at
`DR.cpp:1013-1036`. For `config_agg_1`:

- `agg_view` = the `total_above` QueryAggregate.
- `provenance` = `AggProvenance::kOver` (`DR.cpp:1035`).
- `algebra` = `Algebra::kInvertible` (`SelectAlgebra(sum_above)`,
  `DR.cpp:644`; `sum_above` is `@invertible`).
- `agg_table` = `total_above`'s own DiffTable (V-AGG-SOLE, `DR.cpp:645-646`).
- `input_view` = the over-block join (the single summarized relation walked
  to at `DR.cpp:653-662`); `input_table` = its differential table.
- `statecell_id` = mint-order index into `flow.statecells` (`DR.cpp:667`).
- **`group_cols` = InputGroupColumns ++ InputConfigurationColumns =
  `[Sensor, T]`** (`DR.cpp:1019-1025`). THIS IS THE ONLY DIFFERENCE FROM
  average_weight at the DR level — `group_cols` has 2 entries (1 group + 1
  config) instead of 1 (all group). The config split is INVISIBLE to the
  DR-IR: `group_cols` is a flat vector and everything downstream (position
  mapping, key emission) treats it uniformly. **No new DR effect kind, no new
  op field, no `config_cols` split is needed at the DR level** — this is the
  key structural finding.
- `summary_cols` = InputAggregatedColumns = `[Val]` (`DR.cpp:1026-1029`).

**Effect set — IDENTICAL in kind/count to average_weight's aggregate**
(the config-dependence is a REDUCTION-BODY concern, not an effect-graph
concern). The effect set from `DR.cpp:687-738`, per the P0 census
V-AGG-EFFECT totality (`SubgraphsDemand.md §2`, "2 drains, ± folds summing
0, emit+old, 2 NonRecursive counters, 2 kInI crossings, 2 queue appends"):

| # | EffKind | table | sign | role / pred | band | source |
|---|---------|-------|------|-------------|------|--------|
| 1 | kVecDrain | input_table | — | kNetRemoval | (a) | DR.cpp:692-696 |
| 2 | kStateFold | agg_table | −1 | — | (a) | DR.cpp:698-702 |
| 3 | kVecDrain | input_table | — | kNetAddition | (a) | DR.cpp:692-696 |
| 4 | kStateFold | agg_table | +1 | — | (a) | DR.cpp:698-702 |
| 5 | kStateEmit | agg_table (read) | — | — | (b) | DR.cpp:708-711 |
| 6 | kStateOld | agg_table (read) | — | — | (b) | DR.cpp:712-715 |
| 7 | kCounter | agg_table | −1 | NonRecursive | (b) | DR.cpp:720-726 |
| 8 | kInIReadFrozen | agg_table (read) | — | kInI / kSeed | (b) | DR.cpp:727-732 |
| 9 | kVecAppend | agg_table | −1 | kDeleteQueue | (b) | DR.cpp:733-737 |
| 10 | kCounter | agg_table | +1 | NonRecursive | (b) | DR.cpp:720-726 |
| 11 | kInIReadFrozen | agg_table (read) | — | kInI / kSeed | (b) | DR.cpp:727-732 |
| 12 | kVecAppend | agg_table | +1 | kAddQueue | (b) | DR.cpp:733-737 |

Plus the paired **STATE_SEAL** op (`DR.cpp:748-758`): one `kStateFold`
effect, `sign = 0` (global rmw `sealed := working`), `agg_table` =
`total_above`'s table, same `statecell_id`.

**NEW effects the config feed requires: NONE at the DR level.** The config
value rides the input frontier row (it is an `InputConfigurationColumn`, an
input-view position — `Query.cpp:1028-1032`), so the two `kVecDrain`
effects (#1, #3) already carry it into the fold band; no new drain, no new
read effect. The census recount (P0, `SubgraphsDemand.md §2`) is unchanged:
`exp_group_update = exp_state_seal = |Aggregates()| + |KVIndices()|` counts
this aggregate exactly once, and the per-op key multiset `(agg_table*,
provenance, algebra, view UniqueId)` is config-blind by construction (E-28
deliberately excludes `statecell_id` and never keyed on `group_cols`
arity). **So P0's census passes `config_agg_1` with ZERO census change** —
a pre-registered structural prediction (§6).

### 2.2 THE CRITICAL DESIGN QUESTION, resolved: where does config enter the fold?

Three candidates were considered; the chosen one is **(iii) `f_combine`
takes the config value as a leading argument** — the fold arm passes the
config positions alongside the summary positions.

- **(i) widen the fold-arm summary projection to `(config ++ summary)`?**
  REJECTED. This would make the runtime `Fold(gid, sign, config, summary)`
  and the `Working` blob would have to store or ignore config. The `Working`
  blob is the per-group ACCUMULATOR; stuffing a constant (config is constant
  per group) into it is redundant and breaks the `Invertible` invariant
  `Working == Summary` (`StateCell.h:104-106`) that the emitted `Reduce_<id>`
  relies on (`Database.cpp:1135`). It also pollutes the `@recompute`
  membership multiset (`StateCell.h:167-170`) with a repeated constant.

- **(ii) capture config in the Working blob at group birth (Identity)?**
  REJECTED. `Identity(Working&)` (`StateCell.h:110-112`) has no access to the
  key; wiring config into it would require passing the key to `FindOrAddGroup`
  → `Identity`, a deeper runtime-API change, and would duplicate the config
  (it is ALREADY in `keys[gid]`, `StateCell.h:518`). Worse, it conflates the
  accumulator's identity with a group-constant, defeating the two-word cell's
  frozen/working dichotomy.

- **(iii) `f_combine`/`f_uncombine`/`f_reduce` take the config value(s) as
  leading argument(s), threaded through `Fold`/`Emit` from the key.** CHOSEN.

**Justification against StateCellStore layout + emitted policy structs:**

The config value is **already available at both consumption sites** with no
new storage:

1. **At FOLD time** (band a), the fold arm destructures the frontier row into
   `f0..fn` (`Database.cpp:1988-1993`) and already reads the config positions
   to build the Key (`gpos` includes config, `Database.cpp:2004-2005`,
   `key_parts.push_back("f" + p)`). The SAME locals `f<config_pos>` are passed
   to `Fold` as leading args. No new destructure, no new read.

2. **At EMIT time** (band b), the config is in `key = KeyAt(gid)`
   (`Database.cpp:2076`) — `key.c<config_slot>` — because config is a key
   column. The `@recompute` `ReduceLive` rescan (`StateCell.h:193-199`) is
   reached via `Emit(gid)`/`Old(gid)` which have `gid` in hand and can load
   the key. For `@invertible`, `Emit`/`Old` are O(1) `Finalize`/`OldOf`
   (`StateCell.h:125-135`) that do NOT need config (the fold already applied
   the threshold gate; `Finalize` is identity on the running sum) — so
   `@invertible` config-dependence is entirely a FOLD-time concern.

This means the runtime `StateCellStore::Fold` gains config parameters
forwarded to `Algebra::Fold` → `Reduce::Combine/Uncombine`, and (for
`@recompute`) `Emit`/`Old`/`SealFrom` gain config forwarded to
`Reduce::ReduceLive`. The config is a compile-time-arity list of extra
leading scalars — no blob change, no key-space change, no new Vec.

**Layout consequence:** the two-word cell (`working`/`sealed`,
`StateCell.h:26-37`) is UNCHANGED; the occupancy machinery
(`StateCell.h:38-54`) is UNCHANGED; `keys[gid]` (`StateCell.h:518`) already
holds the config as part of the key — the config threading REUSES the
existing key storage as the emit-time config source and the frontier row as
the fold-time config source. Nothing new is stored.

### 2.3 The expected `Key_<id>` / `Reduce_<id>` / store instantiation

For `config_agg_1`, exactly one state cell (id 0). `Key_0` gains a second
field (the config column) versus average_weight's single-field `Key_0`:

```cpp
// StateCell #0 group key: (Sensor, Threshold) = (group ++ config).
struct Key_0 {
  int32_t c0;   // Sensor  (group)
  int32_t c1;   // Threshold (config)
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Key_0 &) const noexcept = default;
};
```

This emits with NO codegen change: `EmitStateCellStructs`
(`Database.cpp:1082-1090`) already loops over `cell.KeyTypes()` = `[i32,
i32]` (group ++ config) and emits `c0, c1` + a 2-arg `HashRow`. Byte-for-
byte the same machinery that produced average_weight's `Key_2 {c0; c1;}`
(datalog.h:205-212). **No Key_<id> emission change.**

The `Reduce_0` policy struct and free-function decls DO change — this is the
E-31 FACT B extension. The extended C-5 ABI (see §3.2/§4.2) adds the config
type(s) as leading params:

```cpp
// StateCell #0 reduction functions over `sum_above` (C-5 driver ABI;
// config-dependent: the leading `cfg0` is the Threshold config column).
int32_t sum_above_identity();
int32_t sum_above_combine(int32_t cfg0, int32_t working, int32_t value);
int32_t sum_above_uncombine(int32_t cfg0, int32_t working, int32_t value);
// StateCell #0 reduction policy over `sum_above` (C-5 driver ABI).
struct Reduce_0 {
  using Summary = int32_t;
  using Working = Summary;
  static void Identity(Working &w) { w = sum_above_identity(); }
  static void Combine(Working &w, int32_t cfg0, const Summary &v) {
    w = sum_above_combine(cfg0, w, v);
  }
  static void Uncombine(Working &w, int32_t cfg0, const Summary &v) {
    w = sum_above_uncombine(cfg0, w, v);
  }
  static Summary Finalize(const Working &w) { return w; }
};

// The store instantiation is UNCHANGED in TYPE — config is not a store
// type parameter, only a fold argument:
::hyde::rt::StateCellStore<Key_0, ::hyde::rt::Invertible<Reduce_0>> statecell_0;
```

The `Invertible<Reduce>` policy (`StateCell.h:102-136`) forwards config from
its `Fold(w, sign, cfg..., v)` to `Reduce::Combine/Uncombine`. `Identity`
and `Finalize` are config-free (the threshold gate is applied per-fold; the
finalize is identity on the running sum). See §3.1 for the runtime signature
change.

### 2.4 Why `@invertible` config-dependence is sound

`sum_above` sums `Val` iff `Threshold <= Val`. Per group the threshold is
constant (it is a key column), so:
- `combine(cfg, w, v) = (cfg <= v) ? w + v : w`
- `uncombine(cfg, w, v) = (cfg <= v) ? w - v : w`
- `uncombine(cfg, combine(cfg, w, v), v) = w` for all v (the gate is the same
  predicate on both sides). Invertibility holds — the runtime
  `DebugValidate` round-trip check (`StateCell.h:557-570`) passes because it
  re-folds through `Algebra::Fold` with the SAME config the group was folded
  under (see §4.3 for how DebugValidate reaches the config).

### 2.5 The `@recompute` config-dependent ABI (for `config_agg_2`)

For `max_above @recompute`, the reduction rescans the live multiset and the
threshold gates which members count:

```cpp
int32_t max_above_reduce(int32_t cfg0, const int32_t *values,
                         const int32_t *counts, ::std::size_t n);
struct Reduce_0 {
  using Summary = int32_t;
  static Summary ReduceLive(int32_t cfg0,
                            const ::hyde::rt::Vec<Summary> &values,
                            const ::hyde::rt::Vec<int32_t> &counts) {
    return max_above_reduce(cfg0, values.begin(), counts.begin(),
                            values.Size());
  }
};
```

The runtime `Recompute::Emit`/`SealFrom` (`StateCell.h:193-203`) gain config
forwarded to `ReduceLive`; the store's `Emit(gid)`/`Old(gid)`
(`StateCell.h:338-347`) load `KeyAt(gid)`'s config slots and forward them.
(For `@invertible`, `Emit`/`Old` stay config-free — §2.2 point 2.)

---

## 3. The expected `datalog.h` fragments (diff-style vs average_weight)

Diffs are against the average_weight emission shape (witness datalog.h at
the epoch baseline, session scratchpad). Only the config-touched lines
differ; everything else is the R3 mold verbatim.

### 3.1 Store member + init (UNCHANGED shape)

```cpp
// datalog.h Database struct (cf. average_weight datalog.h:334-336):
::hyde::rt::StateCellStore<Key_0, ::hyde::rt::Invertible<Reduce_0>> statecell_0;
```
The store type does NOT gain a config type parameter (§2.2 choice iii). The
member, ctor init (`statecell_0(allocator_)`), and the function-signature
threading are byte-identical to average_weight's per-cell shape.

### 3.2 Forward decls + Reduce policy (CHANGED — the ABI extension)

```diff
- int32_t sum_i32_combine(int32_t working, int32_t value);
- int32_t sum_i32_uncombine(int32_t working, int32_t value);
+ int32_t sum_above_combine(int32_t cfg0, int32_t working, int32_t value);
+ int32_t sum_above_uncombine(int32_t cfg0, int32_t working, int32_t value);
```
and inside `Reduce_0`:
```diff
- static void Combine(Working &w, const Summary &v) { w = sum_i32_combine(w, v); }
- static void Uncombine(Working &w, const Summary &v) { w = sum_i32_uncombine(w, v); }
+ static void Combine(Working &w, int32_t cfg0, const Summary &v) {
+   w = sum_above_combine(cfg0, w, v); }
+ static void Uncombine(Working &w, int32_t cfg0, const Summary &v) {
+   w = sum_above_uncombine(cfg0, w, v); }
```
`Identity`/`Finalize` unchanged. The `cfg0` param count = `NumConfiguration
Columns()` (here 1); a multi-config aggregate emits `cfg0, cfg1, ...`.

### 3.3 The fold loop (CHANGED — passes config locals to Fold)

Against average_weight datalog.h:417-423 (`for (const auto &[f0,f1,f2] :
vec50) { gid = FindOrAddGroup(Key_2{f0,f1}); Fold(gid, -1, f2); }`):

For `config_agg_1` the input frontier row is `(Sensor, T, Val)` =
`(f0, f1, f2)`, `gpos = [0, 1]` (Sensor, Threshold), `spos = [2]` (Val),
`config positions = [1]` (the tail of gpos, since group=[0], config=[1]):

```cpp
for (const auto &[f0, f1, f2] : vec_neg_frontier) {
  const auto gid = statecell_0.FindOrAddGroup(Key_0{f0, f1});
  statecell_0.Fold(gid, -1, f1, f2);   // f1 = config (Threshold), f2 = summary (Val)
}
for (const auto &[f0, f1, f2] : vec_pos_frontier) {
  const auto gid = statecell_0.FindOrAddGroup(Key_0{f0, f1});
  statecell_0.Fold(gid, 1, f1, f2);
}
```
The ONLY change vs average_weight: `Fold(gid, sign, f2)` becomes
`Fold(gid, sign, <config locals>, f2)`. The config locals are the frontier
positions in `gpos` that come from `config_cols` (the tail of `group_cols`
of length `NumConfigurationColumns()`). See §4.3 for how EmitGroupUpdate
learns which of `gpos` are config.

### 3.4 emit_touched (UNCHANGED for `@invertible`)

Against average_weight datalog.h:425-448. For `@invertible` config-dependence
the emit band is byte-identical in SHAPE — `new_v = Emit(gid)` and `old_v =
Old(gid)` are config-free (§2.2 point 2), and the agg row is
`{key.c0, key.c1, <val>}` (config `key.c1` already flows into the row via the
existing `agg_tuple_exprs`, `Database.cpp:2028-2032`). No change:

```cpp
for (const auto gid : statecell_0.Touched()) {
  const auto &key = statecell_0.KeyAt(gid);
  const bool w_occ = statecell_0.WorkingOccupied(gid);
  const bool s_occ = statecell_0.SealedOccupied(gid);
  if (w_occ) {
    const auto new_v = statecell_0.Emit(gid);
    if (s_occ) {
      const auto old_v = statecell_0.Old(gid);
      if (!(new_v == old_v)) {
        total_above_table.SubDerivation({key.c0, key.c1, old_v}, ...kNonRecursive);
        vec_del.Add({key.c0, key.c1, old_v});
        total_above_table.AddDerivation({key.c0, key.c1, new_v}, ...kNonRecursive);
        vec_add.Add({key.c0, key.c1, new_v});
      }
    } else { /* birth: +new only */ ... }
  } else if (s_occ) { /* death: -old only */ ... }
}
```
For `@recompute` config-dependence (`config_agg_2`), `Emit(gid)`/`Old(gid)`
forward the key's config slots to `ReduceLive` inside the store — the
emitted call sites `statecell_0.Emit(gid)` are UNCHANGED (the store loads
KeyAt internally); only the runtime `Recompute::Emit`/store `Emit` gain the
config forward (§2.5, §4.4).

### 3.5 The driver (`tests/OptDiff/cases/config_agg_1.main.cpp`)

Complete, following the average_weight.main.cpp mold (ADL hidden-friend
surface; C-5 free-function reduction bodies; keyed-drain sort per the cursor
contract). The reduction bodies now take the leading config arg.

```cpp
// Driver for config_agg_1 (config-dependent aggregate). sum_above sums only
// readings whose Val >= the per-sensor Threshold. C-5 reduction free
// functions gain the leading config arg (Threshold). Per-Sensor query drains
// are SORTED before printing (cursor contract, CLAUDE.md).
//
// FUNCTOR SEMANTICS (must match the oracle's by-name interpretation):
//   sum_above(cfg=Threshold): running sum of Val over members with
//     Threshold <= Val (identity 0; combine +v iff cfg<=v; uncombine -v iff
//     cfg<=v — invertible because cfg is constant per group).
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#include "datalog.h"

int32_t sum_above_identity() { return 0; }
int32_t sum_above_combine(int32_t cfg, int32_t w, int32_t v) {
  return (cfg <= v) ? (w + v) : w;
}
int32_t sum_above_uncombine(int32_t cfg, int32_t w, int32_t v) {
  return (cfg <= v) ? (w - v) : w;
}

static void Dump(Database &db) {
  for (int32_t s = 0; s <= 6; ++s) {
    std::vector<int32_t> totals;
    auto c = total_above_bf(db, s);
    for (int32_t t; c.next(t);) {
      totals.push_back(t);
    }
    std::sort(totals.begin(), totals.end());
    for (auto t : totals) {
      std::cout << s << " " << t << "\n";
    }
  }
  std::cout << "--\n";
}

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  // Set per-sensor thresholds: sensor 1 -> 15, sensor 2 -> 50.
  {
    hyde::rt::Vec<set_threshold_input> vec(allocator);
    vec.Add({1, 15});
    vec.Add({2, 50});
    set_threshold_2(db, log, functors, std::move(vec));
  }
  // Readings. Sensor 1: 10 (<15, excluded), 20 (>=15), 30 (>=15) -> 50.
  //           Sensor 2: 40 (<50, excluded), 60 (>=50), 100 (>=50) -> 160.
  {
    hyde::rt::Vec<add_reading_input> vec(allocator);
    vec.Add({1, 10});
    vec.Add({1, 20});
    vec.Add({1, 30});
    vec.Add({2, 40});
    vec.Add({2, 60});
    vec.Add({2, 100});
    add_reading_2(db, log, functors, std::move(vec));
  }
  Dump(db);

  // Batch 2 (ADD-ONLY): an above-threshold reading arrives. Sensor 1's sum
  // CHANGES 50 -> 75 (25 >= 15), exercising the emit_touched change arm
  // (-old, +new) AND re-exercising the config gate (25 passes the threshold).
  // Add-only avoids any removal-message convention; the change arm is still
  // driven because the group's summary value moves.
  {
    hyde::rt::Vec<add_reading_input> vec(allocator);
    vec.Add({1, 25});
    add_reading_2(db, log, functors, std::move(vec));
  }
  Dump(db);
  return 0;
}
```

This driver mirrors the `.batches` sidecar (§5.1) exactly, as
average_weight.main.cpp mirrors average_weight.batches. Generated API names
verified this session against a config-free structural twin:
`add_reading_2(db, log, functors, Vec<add_reading_input>)`,
`set_threshold_2(...)`, `total_above_bf(db, Sensor)` returning a cursor
(`c.next(t)`). The implementer reads the ACTUAL generated `datalog.h` for the
config case (arity-3 agg view; §5.3) before finalizing, in case the projected
`total_above/2` query surface differs. Retraction-driven variants (a KV
re-key like average_weight, or an explicit removal message) are a richer
follow-on; add-only is the minimal sound witness that still hits the change
arm and the config gate.

---

## 4. The diff plan against the compiler

### 4.1 Lift the `lib/ControlFlow/Build/Build.cpp:1108` fence (exact replacement)

```diff
   for (auto agg : query.Aggregates()) {
     if (has_induction_owned_input(QueryView(agg))) { ... continue; }  // C-4 stays
-    // Feature gap: the R3 GROUP_UPDATE lowering handles a plain group-by (group
-    // ++ config) reduction; configuration columns are not yet threaded through
-    // the state-cell key projection.
-    if (agg.NumConfigurationColumns()) {
-      log.Append(agg.Functor().SpellingRange())
-          << "Aggregates with configuration columns are not yet supported";
-      continue;
-    }
+    // Config columns are the tail of the (group ++ config) key projection
+    // (DR.cpp:1019-1025) AND, when the reduction depends on them, leading args
+    // to the C-5 reduction ABI (Database.cpp; §4.2). Both are threaded; no
+    // fence. The V-ALGEBRA / induction-owned / @invertible+@recompute checks
+    // above and in Functor.cpp still apply.
```

**Residual validation that STILL applies** (unchanged by this diff):
- `has_induction_owned_input` (C-4, `lib/ControlFlow/Build/Build.cpp:1099`) — a config aggregate
  over an induction-owned input is still rejected (config does not lift the
  induction fence; the input has no phase-owned differential frontiers).
- The `@invertible`/`@recompute` mutual-exclusion + duplicate-pragma rejects
  in `Functor.cpp` (the `algebra_conflict_1`/`algebra_dup_1` diagnostics) —
  unchanged; config does not touch algebra selection.
- V-ALGEBRA for the KV sibling (`lib/ControlFlow/Build/Build.cpp:1135`) — unchanged (KV indices
  carry no config; `group = InputKeyColumns` only, `DR.cpp:1039-1041`).
- `SelectAlgebra` acyclic fence assert (`DR.cpp:718`, `klass ==
  kNonRecursive`) — unchanged; a config aggregate is still acyclic.

### 4.2 Thread config through the reduction ABI

Two coordinated changes: the runtime policy signatures and the codegen.

**(a) Runtime `StateCell.h` — forward a variadic TRAILING pack; the fixed
`Reduce::Combine` signature absorbs config.** The store `Fold` already takes
a variadic `Summary_&&... s` (`StateCell.h:305-315`) and forwards it to
`Algebra::Fold`. TWO coordinated edits are needed — the store `Fold` AND the
`Algebra::Fold` policy — because the store TODAY re-wraps the pack in a
`Summary(...)` constructor, which throws away any leading config args and
arity-mismatches the config-dependent `Reduce::Combine`.

**Edit 1 — the store `Fold` must forward the RAW pack (drop the `Summary(...)`
wrapper).** The live line is `StateCell.h:315` (verified this session):

```cpp
// BEFORE (StateCell.h:305-316, the live body): re-constructs a single Summary
// from the pack, so a caller that passes (cfg0, v) collapses to Summary(cfg0,v)
// — WRONG type/arity for a config-dependent Combine, and drops config entirely
// for a scalar Summary.
template <typename... Summary_>
void Fold(uint32_t gid, int32_t sign, Summary_ &&...s) {
  ...
  Working w = working[gid];
  Algebra::Fold(w, sign, Summary(static_cast<Summary_ &&>(s)...));   // <-- L315
  working.Set(gid, w);
  ...
}
```

```cpp
// AFTER: rename the pack to `Args` and forward it RAW (config ++ value)
// straight to Algebra::Fold — no Summary(...) reconstruction. The config-free
// path (pack == one Summary value) forwards byte-identically to today; the
// config path forwards (cfg0.., v) intact to the fixed Combine signature.
template <typename... Args>
void Fold(uint32_t gid, int32_t sign, Args &&...args) {
  ...
  Working w = working[gid];
  Algebra::Fold(w, sign, static_cast<Args &&>(args)...);            // raw pack
  working.Set(gid, w);
  ...
}
```

Byte-identity note: for the config-FREE case the pack is a single summary
scalar and `Algebra::Fold(w, sign, static_cast<T&&>(v))` binds the same
`const Summary &v` parameter the old `Summary(v)` produced — no observable
change to `Combine`/`Uncombine`, and all 164 goldens stay byte-identical
(prediction 2). The `Summary(...)` wrapper existed only to normalize a
brace-init pack into one blob; with a scalar summary it was a no-op, and with
config it is actively wrong.

**Edit 2 — `Algebra::Fold` forwards the RAW trailing pack to
`Reduce::Combine`/`Uncombine`.** Make `Algebra::Fold` variadic and forward the
whole pack; the Algebra policy's `Fold` accepts `(Working&, sign,
[config...,] summary)` per its arity (config-free = `(Working&, sign,
summary)` as today; config-dependent = the codegen-emitted fixed
`Reduce::Combine(Working&, cfg0.., const Summary&)`):

```cpp
// Invertible (StateCell.h:117-123): forward the whole trailing pack (config
// ++ value) to the fixed-signature Reduce policy. The pack is TRAILING and
// deducible; DO NOT split config into a middle pack (a mid-signature pack is
// non-deducible in C++ — the trailing pack must carry config ++ value
// together, and the generated Reduce::Combine's FIXED signature names them).
template <typename... Args>
static void Fold(Working &w, int32_t sign, Args &&...args) {
  if (0 < sign) { Reduce::Combine(w, static_cast<Args &&>(args)...); }
  else          { Reduce::Uncombine(w, static_cast<Args &&>(args)...); }
}
```
The config arity is known only at codegen, and it is baked into the FIXED
`Reduce::Combine(Working&, int32_t cfg0, const Summary& v)` signature the
generator emits (§3.2). The store/policy stay config-agnostic; the pack just
carries `(cfg0.., v)` and lands on Combine's named params. The store `Fold`
(`StateCell.h:305-323`) needs only to forward its existing `Summary_&&... s`
pack (rename to `Args`), and the caller supplies `Fold(gid, sign, f_cfg..,
f_val)`. **This forward-a-trailing-pack shape is deduction-clean AND keeps
the config-free path (average_weight, all 164 cases) BYTE-IDENTICAL:** with
no config the pack is just `(v)`, Combine's signature is the today's
`(Working&, const Summary&)`, and the emitted call is the today's
`Fold(gid, sign, f2)`. The implementer MUST verify the empty-config pack
degrades to today's exact call text (the P1 "config-free byte-compare"
analogue — the mechanism guaranteeing prediction 2's zero churn). For
`@recompute`, `Emit`/`SealFrom` (`StateCell.h:193-203`) gain config via the
store call site (§4.4), NOT a middle pack.

**Caveat the implementer must resolve (RESOLVED — option (i), the config-aware
probe):** the `DebugValidate` round-trip (`StateCell.h:557-570`, verified this
session: `Algebra::Fold(w, +1, probe)` / `Fold(w, -1, probe)` with a
`Summary probe{}` and NO config) would, under the raw-pack shape, call
`Reduce::Combine(w, probe)` — an arity mismatch against the config-dependent
`Combine(w, cfg0, v)`. Two options were sketched: **(i)** give the emitted
policy a compile-time config count so DebugValidate can synthesize identity
config probes; **(ii)** suppress DebugValidate entirely for config-bearing
cells (a recorded residue). **CHOSEN: option (i).** The invertibility law
`Uncombine(cfg, Combine(cfg, w, v), v) == w` holds for *any fixed* `cfg`
(the fold and unfold apply the SAME `cfg`-parametric gate — §2.4), so a
value-initialized probe config is a sound representative; and synthesizing
`num_config` value-initialized scalars is a two-line loop, so keeping the
property check alive is not disproportionate. Concretely, the emitted
`Reduce_<id>` policy carries `static constexpr unsigned num_config = <N>;`
(and `static constexpr bool kHasConfig = (N != 0)`), and DebugValidate
expands to `Algebra::Fold(w, +1, <cfg-probes...>, probe)` where the config
probes are `Cfg_i{}` value-inits (the config types come from the same
`ConfigTypes()` suffix §4.3 threads onto `ProgramStateCell`). The
`Algebra::Fold` signature is unchanged (it already forwards a variadic pack);
only DebugValidate's call site widens to prepend the synthesized config
probes. See §4.3 — this makes the config split (`num_config_cols`) ALSO a
compile-time discriminator on the emitted policy, one more threading site
than the group/config emission split alone (folded into prediction 8's
count below).

**(b) Codegen `Database.cpp`.** `EmitStateCellStructs`
(`Database.cpp:1076-1159`) emits the reduction decls + policy with config
leading params when `cell` has config columns; `EmitGroupUpdate`
(`Database.cpp:1970`) passes the config frontier locals to `Fold`
(`Database.cpp:2014`). See §4.3 for exactly which of `gpos` are config.

### 4.3 EmitGroupUpdate must learn the group/config SPLIT (the one real plumbing gap)

**Downstream consumer analysis of `group_cols`/`summary_cols`** (the lane's
enumerated obligation). `DR.h:537` fuses `group_cols = group ++ config`; the
question is whether anything downstream needs them SEPARATED. Findings:

- `Stratum.cpp:1414-1419` (`LowerGroupUpdate`): maps every `group_cols` entry
  to a frontier position via `pos_of`. Treats group and config uniformly —
  `group_positions` = positions of all `group_cols`. **This is fine for the
  KEY, but EmitGroupUpdate must know which positions are CONFIG** to pass them
  to Fold. **→ SPLIT NEEDED HERE:** `ProgramGroupUpdateRegion` (Program.h:
  1103-1105) must gain a `config_count` (or a `config_positions` sub-list) so
  EmitGroupUpdate knows the tail of `group_positions` (length
  `NumConfigurationColumns()`) is config. Populate it in `LowerGroupUpdate`
  from `op` — add `op.num_config_cols` to the DROp (set in
  `BuildGroupUpdateOps` from `agg.NumConfigurationColumns()`), or derive it
  from the aggregate view. Simplest: add `unsigned num_config_cols` to
  `ProgramGroupUpdate` and to the DROp, threaded `DR.cpp:684` →
  `Stratum.cpp:1414` → `Program.h`. The config positions are the LAST
  `num_config_cols` of `group_positions` (because `group_cols = group ++
  config` in that order, `DR.cpp:1019-1025`).
- `Stratum.cpp:2058-2059` (`key_types` build): loops `cell.key_cols`
  uniformly. Fine — `Key_<id>` needs group ++ config together (§2.3). **No
  split needed for the key.**
- `Database.cpp:2004-2005` (`key_parts` from `gpos`): fine — the whole key.
  **No split needed.**
- `Database.cpp:2028-2032` (`agg_tuple_exprs` from `gpos`): fine — the agg row
  is `(group ++ config ++ summary)`, all of `gpos` then `val`. **No split
  needed.**
- `Database.cpp:2014` (the `Fold` call, inside the single `emit_fold_arm`
  lambda at `Database.cpp:1982-2018`): **SPLIT NEEDED, AND ALGEBRA-FORKED** —
  must pass the config subset of `gpos` as leading args **ONLY for
  `@invertible` cells**. Uses the `config_count` from
  `ProgramGroupUpdateRegion` above: config locals are `f<gpos[k]>` for the
  last `config_count` entries of `gpos`. **The fork is on the fold-call
  ARGUMENT LIST at exactly `Database.cpp:2014`** (the sole `sc << ".Fold(gid,
  " << sign << ", " << summary_expr` emission — `emit_fold_arm` runs once per
  frontier arm, ±): for an `@invertible` cell the emitted call becomes
  `Fold(gid, sign, <config locals>, summary_expr)`; for a `@recompute` cell
  the call stays `Fold(gid, sign, summary_expr)` config-free (see the
  @recompute-suppression rule below). The algebra is already available to
  `EmitGroupUpdate` (the cell/region carries the `Algebra` selector used to
  choose the `Invertible<>`/`Recompute<>` store instantiation, §2.3), so the
  conditional is a local `if (cell.IsInvertible())` around the config-local
  prefix — no new field beyond `config_count`.

  **@recompute-CONFIG-SUPPRESSION RULE (the missing rule the judge flagged).**
  A `@recompute` cell's `Working` is a MEMBERSHIP MULTISET (values + counts,
  `StateCell.h:159-208`), and its `Fold`/band-(a) job is only to record
  presence of each SUMMARY value — it stores ONLY summary values. Passing
  config leading args into the `@recompute` Fold would corrupt that multiset
  (the config scalar would be recorded as if it were a summary member, or
  arity-mismatch the multiset insert). So a `@recompute` cell SUPPRESSES
  config at the Fold arm entirely and routes config through the REDUCE path
  instead: `Emit`/`ReduceLive` see config as a LEADING parameter (§4.4),
  loaded from `KeyAt(gid)` at emit time (config is a key column). This is the
  natural division: `@invertible` applies the config gate incrementally at
  fold time (so config must reach `Combine`/`Uncombine`); `@recompute`
  rescans the whole live multiset at emit time (so config reaches
  `ReduceLive`, and the fold stays a pure presence record). Concretely the
  artifact's earlier "§4.3 unconditional Fold-split" is WRONG for
  `@recompute` — the split is `@invertible`-only at the Fold arm.
- `EmitStateCellStructs` (`Database.cpp:1076`): **SPLIT NEEDED** — must emit
  the config leading params on the reduction decls/policy. Uses
  `cell.KeyTypes()` (group ++ config) and a new `cell.NumConfigTypes()` (or
  `ConfigTypes()`) so it can slice the config type suffix. Add
  `config_count`/`config_types` to `ProgramStateCell` (Program.h:1852,
  populated at `Stratum.cpp:2058` from `cell.key_cols` minus the group prefix
  — needs the DR statecell to record `num_config_cols`, mirroring the DROp
  addition).
- `bin/Oracle/Main.cpp` — see §4.5.

So the plumbing gap is: **add `num_config_cols` (equivalently a
config-type/position suffix) to the DR statecell + DROp + ProgramStateCell +
ProgramGroupUpdate**, threaded from `agg.NumConfigurationColumns()` at build
time. Everything else already treats `(group ++ config)` uniformly. This is
a small, mechanical addition — the config split is NEEDED only at the two
emission sites that talk to the REDUCTION ABI (the Fold call and the policy
decls), never at the key/row projection sites.

### 4.4 The `@recompute` arm (for `config_agg_2`) — where config actually enters

Per the @recompute-config-suppression rule (§4.3): a `@recompute` cell does
NOT receive config at the Fold arm — its `Fold` stays config-free
(`Fold(gid, sign, summary_expr)`), because its `Working` is a summary-only
membership multiset. Config enters ONLY at the emit/reduce path. The store
`Emit`/`Old`/`Seal` (`StateCell.h:338-363`) forward `KeyAt(gid)`'s config
slots to `Recompute::Emit`→`ReduceLive`. This needs the store to know which
key slots are config (a `config_slot_count` template/ctor param, or pass
config explicitly). Because the store is keyed on `Key` (an opaque hash
struct), the cleanest shape is: the store takes the config as an explicit
argument to `Emit`/`Old` at the call site (EmitGroupUpdate already has
`key.c<config_slot>` in scope at `Database.cpp:2076`). I.e. for `@recompute`
config-dependent, EmitGroupUpdate passes `key.c<config_slots>` to
`statecell_0.Emit(gid, key.c1)` / `.Old(gid, key.c1)`, and the reduce sees
config as a LEADING parameter (`ReduceLive(cfg0.., values, counts)`, §2.5).
This keeps the store config-agnostic in its layout and forwards config only
where the reduction needs it. (`@invertible` needs no such change at emit
time — §2.2 point 2; it took its config at the Fold arm instead.) **So the
two algebra arms take config at DISJOINT sites: `@invertible` at the Fold
arm (`Combine`/`Uncombine`), `@recompute` at the emit arm (`ReduceLive`) —
never both, and the `emit_fold_arm` fork (§4.3, `Database.cpp:2014`) is the
switch.**

### 4.5 The oracle (`bin/Oracle/Main.cpp`) — the referee learns config-dep reduction

The oracle recomputes each aggregate DEFINITIONALLY, grouping by `(group ++
config)` (already: `BuildAggregates` pushes config into `key_slots`,
`Main.cpp:1281-1295`) and running a BY-NAME reduction (`ReduceAggregate`,
`Main.cpp:1360-1399`). **The by-name reduction is currently config-BLIND** —
`AggKind::kSum/kCount/kMerge` (`Main.cpp:1379-1391`) read only `ai.val_slot`
and hardcode `sum_i32`/`count_i32`/merge (`Main.cpp:1303-1312`). For
`config_agg_1` the oracle must:
- Recognize `sum_above` (a new `AggKind::kSumAbove`, added to the by-name
  switch at `Main.cpp:1303-1312`).
- Record which `key_slots` are CONFIG (a `config_slot_count`, from
  `agg.NumConfigurationColumns()`) so the reduction can read the threshold
  from the group key (`Main.cpp:1372-1373` already builds `key` from
  key_slots; the config is the tail of `key`).
- In `ReduceAggregate` (`Main.cpp:1375-1392`), gate the accumulation on the
  config: `if (threshold <= v) acc += v`. The threshold is `key[group_count +
  config_index]`.

**The oracle's per-group recompute for a config-dependent aggregate: group
by (group ++ config); the reduction reads the config from the group key and
applies the by-name predicate (Threshold <= Val for sum_above).** This is
the definitional truth `config_agg_1.oracle.stdout` is blessed from (§5).
The oracle's arity check (`Main.cpp:1313-1315`, `key_slots.size() + 1`)
already accommodates the extra config key column.

`config_agg_2`'s `max_above` adds `AggKind::kMaxAbove` the same way.

### 4.6 Fix the stale `lib/ControlFlow/Build/Build.cpp:1106-1107` comment (E-31 tail)

The in-code comment "configuration columns are not yet threaded through the
state-cell key projection" is stale — the KEY projection IS threaded
(FACT A, `DR.cpp:1019-1025`). Deleted as part of the fence removal (§4.1):
the replacement comment records that the key is threaded and the reduction
ABI now receives config. **Do NOT special-case literal (compile-time) config
here** — the relation-fed path (§1.2) and the literal path both produce
`config_columns` uses (a literal desugars to a constant column via
`CreateLiteralVariable`, `Aggregate.cpp:319`); the fence lift and ABI
extension cover both. The literal specialization (constant-folding the
threshold into the reduction) is a candidate-(c) later-half optimization,
not a correctness requirement — the general (relation-fed) path is correct
for literals too.

### 4.7 Validators / census — no change

P0's V-AGG-EFFECT / V-AGG-SOLE / V-AGG-PAIR and the census recount
(`SubgraphsDemand.md §2`) are config-blind by construction (§2.1). They pass
`config_agg_1` unchanged — a pre-registered structural prediction (§6). A
NEW validator worth adding (optional, defensive): **V-CONFIG-ARITY** — the
emitted reduction's config param count equals `NumConfigurationColumns()`
and equals the tail length of `group_cols` beyond `NumGroupColumns()`. This
is the config analogue of the R1e-style day-one census catch; it guards
future drift where the group/config split is miscomputed.

---

## 5. The oracle + golden plan

Per the aggregate_1-flip precedent (a case FLIPPED diagnostic→golden at the
R3 stage-C flip, CLAUDE.md), `config_agg_1` flips from the
`lib/ControlFlow/Build/Build.cpp:1108` diagnostic (today) to a 4-mode golden + oracle/monotone
referee.

### 5.1 The `.batches` sidecar (`tests/OptDiff/cases/config_agg_1.batches`)

The AUTHORITATIVE +/- message log the oracle referees; the driver mirrors it
(as average_weight.main.cpp mirrors average_weight.batches). Recommended
ADD-ONLY encoding (avoids the retraction-convention caveat of §3.5) that
still exercises the below/above gate AND the emit_touched change-arm:

```
# config_agg_1 oracle sidecar: mirrors config_agg_1.main.cpp exactly.
# Batch 1: per-sensor thresholds + readings; the config gate excludes
#          below-threshold readings from each sum.
# Batch 2: an above-threshold reading arrives -> the sum CHANGES (the
#          emit_touched change arm: -old, +new).
batch
+ set_threshold 1 15
+ set_threshold 2 50
+ add_reading 1 10
+ add_reading 1 20
+ add_reading 1 30
+ add_reading 2 40
+ add_reading 2 60
+ add_reading 2 100
end
batch
+ add_reading 1 25
end
```

Expected group states (definitional):
- After batch 1: sensor 1 (threshold 15) sums {20, 30} (10 excluded) = 50;
  sensor 2 (threshold 50) sums {60, 100} (40 excluded) = 160.
- After batch 2: sensor 1 sums {20, 30, 25} = 75 (25 >= 15); sensor 2
  unchanged = 160.

`config_agg_1.stdout` (the 4-mode golden), per the Dump format
(`s total`, sorted, `--` between batches):
```
1 50
2 160
--
1 75
2 160
--
```
(Sensors 0, 3-6 produce no rows — no readings.)

`config_agg_1.oracle.stdout` (blessed FROM ORACLE TRUTH, never the engine):
the `ORACLE: OK (2 batches, N assertions)` line + the final surviving facts
`total_above 1 75` / `total_above 2 160`, in the oracle's canonical order.

`config_agg_1.monotone.stdout`: the monotone projection's surviving facts
`total_above 1 75` / `total_above 2 160`.

### 5.2 Blessing discipline

Goldens are blessed via `runall.sh --bless <workroot> config_agg_1` ONLY
after reviewing the run's outputs AND cross-checking the oracle line against
the hand-computed group states above. The oracle golden is authoritative:
if the engine's `.stdout` disagrees with the oracle's definitional recompute,
that is a BUG in the emission (the house bet — the census/oracle catches a
real miscount), NOT a reason to bless the engine's output. Per the house
rule (CLAUDE.md), never bless to make a red case green.

### 5.3 What the oracle's per-group recompute does with config

Group the input-view rows by `(Sensor, Threshold)` (the config-inclusive
key, `Main.cpp:1290-1295`); for each group read the threshold from the key
(§4.5) and run `sum_above` = Σ{Val : Threshold ≤ Val} over the group's
members. Emit `(Sensor, Threshold-in-key... no: only group ++ summary
appears in the OUTPUT)`. **Subtlety:** the aggregate OUTPUT is
`total_above(Sensor, Total)` — arity 2 — but the KEY is `(Sensor,
Threshold)`. The config column is in the key but the output projection is
`(group ++ config ++ summary)` = `(Sensor, Threshold, Total)`? No — check:
`total_above` is declared arity 2 `(Sensor, Total)`. The aggregate view's
output columns are group ++ config ++ summary (`Query.cpp:1000-1020`), so
the AGG VIEW is arity 3 `(Sensor, Threshold, Total)`, and the query
`total_above(bound Sensor, free Total)` projects it. **The oracle's
`ai.arity = key_slots.size() + 1` (`Main.cpp:1313`) = 2 (Sensor,Threshold) +
1 = 3, and must match the AGG VIEW head arity (3), not the query relation
arity (2).** This is already handled — `ai.head` is the QueryAggregate view
(`Main.cpp:1272`), whose arity is 3. The `total_above` query relation is a
separate view projecting away Threshold. **Implementer: verify the oracle's
head-arity check (`Main.cpp:1314`) uses the AGG VIEW (arity 3), and the
final projection to `total_above/2` drops Threshold — this is the same
group++config-in-key-but-projected-in-output shape the emitted agg table
uses (row = (Sensor, Threshold, Total), datalog.h agg_tuple_exprs).**

**This projection point is the single sharpest verification risk in the
whole stage** — flagged for the implementer and the judge (§6).

**Empirical evidence (this session, config-FREE analogue).** A structurally
identical program with `sum_i32` (no config) instead of `sum_above` compiles
today; its generated agg table is `total_above_4` of `Row4` = arity 2
(comment "Rows of `total_above_4` (total_above/2)"), `Key_0` single-column,
and the `total_above_bf` query reads that table DIRECTLY — the aggregate view
IS the `total_above/2` relation. WITH config, the agg view's row is
`(Sensor, Threshold, Total)` = arity 3 (group ++ config ++ summary,
`agg_tuple_exprs` emits `key.c0, key.c1, val`), so the agg TABLE becomes
arity 3 and a projecting SELECT to `total_above/2` (dropping Threshold) must
sit above it. The implementer must confirm the dataflow builder inserts that
projection (it should — the query relation `total_above/2` is a distinct view
from the arity-3 aggregate view; `ApplyAggregate` creates the agg view with
all group++config++summary columns, `lib/DataFlow/Build.cpp:1052-1128`, and the head
clause projects). If the projection is missing or mis-columned, the
oracle/emission cross-check fires here (§6 prediction 7). Generated names
verified this session: `add_reading_2`/`set_threshold_2` messages
(`<name>_<arity>`), `add_reading_input`/`set_threshold_input` = `Tup_i32_i32`,
`total_above_bf(db, Sensor)` cursor query.

---

## 6. Pre-registered predictions

1. **Suite grows 164 → 165** (`config_agg_1` added). If the `@recompute`
   companion `config_agg_2` ships too, 164 → 166 (called out separately;
   `config_agg_1` alone closes the clean-diagnostic gap).
2. **Zero churn on the existing 164 goldens.** The config-free reduction ABI
   instantiates the config pack EMPTY (§4.2a), so average_weight and every
   other case emit byte-identical text (the P1 empty-pack byte-compare
   analogue). Verified by the full suite `SUITE: PASS` + byte-compare on the
   average_weight/cf16_4 witnesses.
3. **Driver churn 0 on existing cases.** The extended C-5 ABI is
   ADDITIVE (config pack empty for config-free functors); average_weight.main
   .cpp's `sum_i32_combine(int32_t, int32_t)` is unchanged.
4. **Q5 neutral.** Config aggregates are a new-feature path, not a Q5-chain
   change (the Q5 witness has no config aggregate). Q5 spot @128 stays flat
   (release 0.11-0.13s / debug ~0.93s per the P0 baseline).
5. **P0 census passes `config_agg_1` with zero census change** (§2.1, §4.7):
   the recount and per-op key multiset are config-blind by construction. The
   census does NOT fire on `config_agg_1` (config is not a miscount vector).
6. **`config_agg_1`'s oracle/monotone/stdout goldens are blessed FROM ORACLE
   TRUTH after review** (§5.2), not from the engine. The hand-computed group
   states (§5.1: sensor 1 → 50 then 75; sensor 2 → 160) are the reference.
7. **The house bet (E-1.. precedent): the oracle/emission cross-check MAY
   fire once** on the config-projection point (§5.3) — the risk that the
   group++config-in-key vs group-in-output projection is miscomputed at the
   agg-view head arity or the final `total_above/2` projection. If it fires,
   a FINDINGS.md entry (F23+) records it. This is the single most likely
   real-defect site; if it does NOT fire, that is a clean pass (config's
   uniform treatment held).
8. **The group/config split (`num_config_cols`) threads to exactly THREE
   emission-facing sites** (widened post-judge from "the two emission
   sites"): threaded DR statecell → DROp → ProgramStateCell/ProgramGroupUpdate,
   then consumed at (i) the `EmitGroupUpdate` Fold call
   (`Database.cpp:2014`, `@invertible`-only config-local prefix — the algebra
   fork of §4.3), (ii) the `EmitStateCellStructs` reduction decls/policy
   (`Database.cpp:1076-1159`, config leading params on `Combine`/`Uncombine`
   or `ReduceLive`), and (iii) the emitted `Reduce_<id>` policy's
   `static constexpr num_config`/`kHasConfig` discriminator that
   `DebugValidate` (`StateCell.h:557-570`) reads to synthesize identity config
   probes (§4.2 option (i)). Everything else is fence removal + ABI-param
   addition + oracle by-name extension. If an implementer finds a FOURTH
   consumer of `group_cols`/`summary_cols` (or of the config split) that needs
   the split beyond these three enumerated sites, this prediction is
   falsified. (The judge's LOW finding — DebugValidate's config-free
   `Fold(w,±1,probe)` call — is discharged by site (iii), not by dropping the
   debug check: option (i) keeps the invertibility round-trip alive for
   config cells.)

---

## Appendix: session-verified anchor table

| Claim | Anchor | Verified |
|-------|--------|----------|
| config = `bound` functor param → config_var | Aggregate.cpp:33-36 | read |
| config var resolved in over-block base view | lib/DataFlow/Build.cpp:1076-1089 | read |
| "Could not find configuration variable" | lib/DataFlow/Build.cpp:1080 | read + empirical |
| config columns are input-view uses | Query.cpp:1028-1032 | read |
| group_cols = group ++ config | DR.cpp:1019-1025 | read |
| the fence to lift | lib/ControlFlow/Build/Build.cpp:1105-1112 | read + empirical |
| key_cols → key_types | Stratum.cpp:2053-2064 | read |
| Key_<id> emission over KeyTypes | Database.cpp:1082-1090 | read |
| Reduce_<id> policy + C-5 decls | Database.cpp:1113-1158 | read |
| Fold passes only summary | Database.cpp:2004-2015 | read |
| agg row = key.c0..cN ++ val | Database.cpp:2023-2037 | read |
| emit_touched band | Database.cpp:2073-2116 | read |
| Invertible::Fold/Combine/Uncombine | StateCell.h:102-136 | read |
| Recompute::Emit/ReduceLive | StateCell.h:159-208 | read |
| store Fold forwards only Summary | StateCell.h:305-323 | read |
| keys[gid] holds the (group++config) key | StateCell.h:518 | read |
| LowerGroupUpdate pos_of + positions | Stratum.cpp:1390-1419 | read |
| ProgramGroupUpdate group/summary_positions | Program.h:1103-1105 | read |
| ProgramStateCell key_types/summary_types | Program.h:1852-1853 | read |
| oracle groups by group++config | Main.cpp:1281-1295 | read |
| oracle by-name reduction (config-blind) | Main.cpp:1303-1312, 1360-1399 | read |
| oracle arity = key_slots+1 (uses agg view) | Main.cpp:1313-1315 | read |
| average_weight driver mold | average_weight.main.cpp | read |
| average_weight Key_2 multi-col witness | witness datalog.h:205-212 | read |
| average_weight fold+emit_touched witness | witness datalog.h:417-448 | read |
| BuildGroupUpdateOps effect set | DR.cpp:638-758 | read |
| P0 census (config-blind key multiset) | SubgraphsDemand.md §2 (E-28) | read |
| store Fold re-wraps pack in Summary(...) (AMEND 1) | StateCell.h:315 | read |
| emit_fold_arm single Fold-call site (AMEND 2) | Database.cpp:1982-2018 (call :2014) | read |
| DebugValidate config-free Fold(w,±1,probe) (AMEND 3) | StateCell.h:557-570 | read |
| config-var resolution (DataFlow) (AMEND 4) | lib/DataFlow/Build.cpp:1076-1089 | read |
| the fence (ControlFlow) (AMEND 4) | lib/ControlFlow/Build/Build.cpp:1108 | read |

---

## AMENDMENTS (2026-07-16, post-judge)

The P2c adversarial judge (SubgraphsDemand.md §3, "P2c — config-column
aggregates": REVISE, premise VERIFIED, three edits then GO for
`config_agg_1`) returned four findings. All four are applied below; every
new code anchor was re-read this session against the `subgraphs-demand` tip
with P0 landed. The judge's premise (E-31 FACT A/B — config-as-KEY threaded,
reduction ABI config-blind) stands unchanged; these are the local, targeted
fixes it required before the diff is landable.

**AMEND 1 — STORE-FOLD REWRITE (§4.2a), judge finding (1) MEDIUM.** The §4.2a
runtime edit was incomplete: it rewrote only `Algebra::Fold` and left the
STORE `Fold` (`StateCell.h:315`, verified live:
`Algebra::Fold(w, sign, Summary(static_cast<Summary_ &&>(s)...))`)
re-constructing a single `Summary` around the pack. That wrapper collapses a
config-carrying `(cfg0, v)` call into `Summary(cfg0, v)` — wrong arity/type
for a config-dependent `Combine`, and it silently drops config for a scalar
summary — so the mandatory `config_agg_1` case would NOT compile. Added an
explicit **Edit 1** before/after block: DROP the `Summary(...)` wrapper,
rename the pack `Summary_`→`Args`, and forward it RAW
(`Algebra::Fold(w, sign, static_cast<Args &&>(args)...)`); the Algebra
policy's `Fold` accepts `(Working&, sign, [config...,] summary)` per its
arity. Byte-identity for the config-free path is preserved (a scalar
summary binds `const Summary &v` identically to the old `Summary(v)`), so
prediction 2 holds. WHY: without dropping the wrapper the instruction is not
compile-true on the case E-31 mandates.

**AMEND 2 — @recompute CONFIG ROUTING (§4.3/§4.4), judge finding (2)
MEDIUM.** The artifact's §4.3 Fold-split was stated unconditionally; for a
`@recompute` cell that would corrupt the Working MEMBERSHIP MULTISET (which
stores only summary values). Added the **@recompute-config-suppression
rule**: config args at the Fold arm are emitted ONLY for `@invertible`
cells; a `@recompute` cell suppresses config at the Fold arm and routes it
through the emit/reduce path (`ReduceLive` sees config as a leading
parameter, loaded from `KeyAt(gid)`). Stated exactly where the emitted code
forks: `EmitGroupUpdate`'s single `emit_fold_arm` lambda
(`Database.cpp:1982-2018`) gains an algebra conditional on the fold-call
argument list at `Database.cpp:2014` (the sole `.Fold(gid, sign, ...)`
emission) — `@invertible` prepends config locals, `@recompute` stays
config-free. §4.4 rewritten to make explicit that the two algebra arms take
config at DISJOINT sites (invertible = Fold arm, recompute = emit arm),
never both. WHY: the unconditional split would corrupt the recompute
multiset.

**AMEND 3 — PREDICTION 8 + DebugValidate (§4.2), judge finding (3) LOW.**
Widened the DebugValidate caveat and prediction 8's threading inventory. The
config split must ALSO reach the emitted `Reduce_<id>` policy as a
compile-time discriminator, because `StateCellStore::DebugValidate`
(`StateCell.h:557-570`, verified) calls `Algebra::Fold(w, ±1, probe)`
config-free and would arity-mismatch a config-dependent `Combine`.
**RECORDED CHOICE: option (i)** — a `static constexpr num_config`/`kHasConfig`
on the emitted policy letting DebugValidate synthesize identity config
probes. Justification (two sentences, in §4.2): the invertibility law
`Uncombine(cfg, Combine(cfg, w, v), v) == w` holds for any fixed `cfg`
(fold and unfold apply the same `cfg`-gate), so a value-initialized probe
config is a sound representative; and synthesizing `num_config`
value-initialized scalars is a two-line loop, so keeping the property check
alive is not disproportionate. Option (ii) (suppress DebugValidate for
config cells) was NOT taken — it would silence the invertibility round-trip
for exactly the new class of cells. Prediction 8's site count widened from
TWO emission sites to THREE (Fold call, reduction decls/policy, and the
DebugValidate discriminator).

**AMEND 4 — ANCHOR DISAMBIGUATION, judge finding (4) LOW.** Every bare
`Build.cpp:NNNN` is now qualified. Verified live this session: the
config-var resolution (`FindColVarInView`, "Could not find configuration
variable", `config_columns.AddUse`) is in **lib/DataFlow/Build.cpp**
(~:1076-1089; the `ApplyAggregate` body spans :1052-1128); the feature-gap
fence and its residual rejects (induction-owned :1099, V-ALGEBRA KV :1135,
the stale :1106-1107 comment) are in **lib/ControlFlow/Build/Build.cpp**
(fence at :1108). All in-prose, in-`.dr`-comment, §4.1/§4.6 heading, §5, and
appendix-table occurrences updated.
