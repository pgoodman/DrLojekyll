# D3.a STAGE (a) — THE WHOLE-PROGRAM SUBSTRATE PSEUDOCODE

> **House banner.** Tip **428dae76** (verified). Stage (a) of the D3.a epoch
> (OD-14, ledger §20(AC)-(AD)). Produced by the 2026-07-28 build-out fleet:
> 4 seed-UNREAD opus derivation lanes (A InstanceStore / B instance-op family +
> lowering + codegen / C demand pass / D differential machinery; docs forbidden)
> + 1 xhigh seed-READ consolidator cross-checking against the binding contracts
> (d1b/d2a/d2b/ra2-design.md, §19(K)-(O), §20(B)/(C)/(G)/(AB)); ~683k tokens,
> zero lane deaths. ORCHESTRATOR-REVIEWED: the five most load-bearing anchors
> re-verified personally at code before commit (RecycleCurrent :216-219 with
> ZERO lib/ callers; the kInstanceDeath TableIsDifferential(demand_table) mint
> gate Rel.cpp:1138; the instantiate+seal-only lowering enrollment
> Procedure.cpp:328-331 — a minted death is FATAL-if-minted via V-INST-EMITTED;
> the two-arg SUBGRAPHINSTANCE ctor Procedure.cpp:284 = XC-2; the DeathEffects
> kNetRemoval drain Rel.cpp:865-869). This is a MAP, not a design; §5 is the
> gap ledger (17), §6 the ritual-head open questions (12) awaiting owner
> rulings before stage (b). SINGLE-PASS: the stage-(b) fleet re-verifies this
> section before designing on it.


Tip `428dae76` (verified `git rev-parse HEAD`). Written on the §7 idiom:
precise pseudocode + prose, every load-bearing statement carrying a
`file:line` anchor at this tip. Consolidates the four seed-unread lane
derivations (A InstanceStore / B instance-op family / C demand pass /
D differential machinery) against the binding contracts
(d1b/d2a/d2b/ra2-design.md, KeyedInstances.md §19(K)-(O) + §20(B)/(C)/(G)/(AB)/(AC)).

This is a MAP OF THE LANDED SUBSTRATE + THE GAP FRONTIER, not a design.
Ruled decisions (RAT-1..10, OD-1..14, ADJ-*) are NOT re-litigated. §5 is the
deduped gap ledger; §6 is the ruling-head open-question set for stage (b).

Vocabulary (as the code spells it): **R-MONO** = the monotone keyed-instance
slice LIVE at tip (pub answer table monotone, demand key monotone). **R-DIFF**
= the D3.a target (retractions flow into the demanded/instance-keyed subgraph).
`pub_table` = published answer relation; `demand_table` = fabricated demand
relation's model; `input_table` = summarized monotone input.

---

## CROSS-CHECK ADJUDICATIONS (docs drifted; code is authority)

Two binding-contract statements diverge from the landed code. Both resolved AT
THE CODE; the code is right, the doc is aspirational/drifted. Neither is a
correctness defect — both describe the *intended R-DIFF shape* that did not land
in the R-MONO slice.

- **XC-1 — the "(T,F) scan compiles inert" wording.** d2b-design.md:722-727
  ("`[R-DIFF only] scan Frozen(iid) … dropped {(T,F)} → publish −`" … "the
  R-DIFF retract scan compiles inert") and d2b-design.md:826 ("the (T,F)
  frozen-scan is R-DIFF, emitted inert") read as if an inert (T,F) scan is
  *emitted*. It is NOT. `EmitSubgraphInstance` band-(b) (`Database.cpp:2405-2454`)
  emits ONLY the (F,T) born scan — there is no (T,F) frozen-scan text in the
  emitter at all. RESOLUTION: the (T,F) scan is *absent*, not *inert-but-present*;
  it is correct-by-vacuity under R-MONO (the HP-7 belt proves `frozen⊆current`).
  All four lanes report this correctly. "inert/compiles inert" is loose prose for
  "absent, vacuous under R-MONO." No substantive conflict; the code stands.

- **XC-2 — the SUBGRAPHINSTANCE region has NO diff flag.** d2b-design.md:689-690
  shows the region minted `CreateDerived<SUBGRAPHINSTANCE>(seq,
  op.instance_store_id, TableIsDifferential(pub))` — a third `diff` ctor arg. The
  LANDED ctor is two-arg: `CreateDerived<SUBGRAPHINSTANCE>(seq, sid)`
  (`Procedure.cpp:284`); the region (`Program.h:1158-1188`) carries no diff bit.
  RESOLUTION: the region does not know its diff-ness today; codegen is R-MONO-only
  and never branches on it. The design's diff-arg is UNLANDED. This is a concrete
  D3.a hole (§5 G-DIFF-REGION): the region must learn diff-ness to select the
  R-DIFF codegen band. Lane B is right.

---

## §1 THE InstanceStore AS LANDED (`include/drlojekyll/Runtime/InstanceStore.h`)

`InstanceStore<Key, RowT>` (`:58-59`) — the keyed-instances transpose of
`StateCellStore` (class comment `:3-11`): a dense per-key instance id (`iid`)
maps to a **double-buffered nested relation** — `current[iid]` (this epoch's
rebuilt content) + `frozen[iid]` (last epoch's sealed snapshot,
`Table::kInI`-analogue lifted to relation granularity). Sole codegen emitter:
`Database.cpp::Generator::EmitSubgraphInstance` (`:2286-2461`). Sole unit pin:
`tests/InstanceStore/InstanceStoreTest.cpp`.

### 1.1 Shape (data members `:320-330`)

```
Allocator      allocator;        // owns all heap
bool           monotone;         // HP-7 seal-belt gate (:321) — the RAT-4 bool
Vec<Key>       keys;             // dense iid -> key           (:322)
Vec<uint64_t>  hashes;           // parallel cached key hashes (:323)
Vec<Table*>    frozen;           // iid -> last-epoch sealed content = old()  (:324)
Vec<Table*>    current;          // iid -> this-epoch rebuilt content         (:325)
Vec<uint8_t>   sealed_occupied;  // iid -> batch-start occupancy bit          (:326)
Vec<uint32_t>  touched;          // iids touched this epoch                   (:327)
Vec<uint8_t>   touched_flag;     // per-iid append-once bit                   (:328)
uint32_t*      slots{nullptr};   // open-addressing key->iid hash             (:329)
size_t         slot_capacity{0};
```

The nested `Table<RowT>` is **INDEX-FREE and PREDICATE-FREE by design** (class
comment `:18-19`): instance membership is `Table::Find`/`TryAdd` on the WHOLE
row (flat-demand's per-copy hash index disappears; band-(b) does `frz.Find(row)`
at `Database.cpp:2435`, never an index probe). The two type params are
`<Key, RowT>` but the "algebra" slot is FIXED to a monotone `Table<RowT>`
(`:56-57`) — there is NO invertible/recompute selector (that lives only in
`StateCellStore`). This is the first structural fact for D3.a: **the store type
cannot carry a differential/signed nested table** — only a monotone
`Table<RowT>` whose per-row membership is boolean presence.

### 1.2 The keyed hash table (mint / find / grow)

Open-addressing linear probe over `slots[]`, keyed by `key.Hash()`, sentinel
`kNoInstance = ~0u` (`:52`), RowStore mold (`:329`).
- `FindInstance(key)` (`:99-101`) = `FindInstanceWithHash(key, key.Hash())`
  (`:238-254`) — const, NON-adding, no allocation. The band-(a2) live-demanded
  gate (`kNoInstance` ⇒ stray edge SKIPS).
- `FindOrAddInstance(key)` (`:105-124`) — the minting path: find-or-append a
  fresh dense iid with empty `frozen`+`current` (`MakeTable`, `:118-119`),
  `sealed_occupied=0`. `NumInstances()=keys.Size()` (`:94-96`) — **the namespace
  is append-only for the program's life** (class comment `:13-16`). Overflow
  belt `iid==kNoInstance` (`:112-115`) is unreachable (2^32 keys).
- `InsertSlot`/`Rehash` (`:256-294`) — 7/8-load doubling, seed 64.

### 1.3 The frozen/current double buffer + Seal + Reset

`Current(uint32_t)` (`:138`) mutable rebuild target; `Frozen(uint32_t)` (`:139`)
const `old()` snapshot. **Seal (`:174-208`)** — the pointer-swap epoch advance,
over TOUCHED iids only (`:176`):
```
for iid in touched:
  #ifndef NDEBUG   // the HP-7 R-MONO belt, gated by `monotone` (:185) — §1.5
    if monotone:
      for r in frozen[iid].rows: assert current[iid].Find(frozen row) != kNoRow
  Table *f = frozen[iid]                       // :199 read-before-overwrite
  frozen.Set(iid, current[iid]); current.Set(iid, f)   // :200-201 swap
  current[iid].Reset()                         // :202 empty the retired buffer
  sealed_occupied.Set(iid, frozen[iid].NumRows()>0 ? 1:0)  // :203-204
  touched_flag.Set(iid, 0)                     // :205
touched.Clear()                                // :207
```
`current[iid].Reset()` chains `Table::Reset` (`:304`) → `RowStore::Reset`
(`Table.h:162-168`): `Truncate(0)` (retains capacity) + in-place slot clear +
`sealed=0` — NO allocator entry point (the H6 discharge; unit pin
`H6SteadyStateAllocationBounded` :256). An untouched iid keeps its buffers as-is
(content persists — fine under the rebuild-from-scratch R-MONO model; the
"stale-current" concern is a R-DIFF gap, §5 G-STALE).

### 1.4 The touched set + RecycleCurrent (the pre-built death primitive)

`Touch(iid)` (`:298-305`) append-once via `touched_flag`; `TouchCurrent(iid)`
(`:130-133`) = `Touch` + return `*current[iid]` (band-(a) entry);
`Touched()` (`:144-147`) `SortAndUnique`s then returns const; `TouchedFlag(iid)`
(`:149-151`) the first-touch-this-epoch predicate.

**`RecycleCurrent(iid)` (`:216-219`)** = `Touch(iid); current[iid].Reset()` —
UNCONDITIONAL + IDEMPOTENT (comment `:210-215`, "the R-DIFF death arm's belt +
same-epoch demand-flap rebuild"). **It has ZERO codegen callers** (grep of the
witness dump + `lib/CodeGen`/`lib/ControlFlow`: none). This is the KEY §1 fact:
the store ALREADY has the whole-instance retraction primitive; the codegen does
not emit it. Unit-proven by `RecycleCurrentIdempotentSameEpochFlap` (:210) and
`DeathHalfRecycleThenPartialReaddDropsRows` (:317, `monotone=false`).

### 1.5 Occupancy + the working_count collapse (N-1) + the RAT-4 belt

Two predicates: `WorkingOccupied(iid)` (`:161-163`) = `current[iid]->NumRows()>0`;
`SealedOccupied(iid)` (`:167-169`) = `sealed_occupied[iid]!=0` (batch-start bit).

**N-1 (the dropped signed count):** StateCellStore tracks a SIGNED
`working_count`; the InstanceStore DROPPED it (`:21`, "working_count DROPPED
(A.3.1; N-1 carried)"). Occupancy reads straight off `NumRows()>0`. EXACT under
R-MONO ONLY (comment `:157-160`): band-(a) is a monotone `TryAdd` rescan, so
`current` only GROWS within an epoch — no mid-epoch dip. The information lost is
a mid-batch add+retract that annihilates to empty: a `Table<RowT>` append-only
log cannot shrink except by whole-buffer `Reset`, so `NumRows()>0` would report
occupied — a LIE. The class ratifies exactly this at `:24-26` ("when R-DIFF
lands (D3.a) a mid-batch retraction could make NumRows()-as-occupancy a lie —
revisit re-introducing a signed count then").

**The RAT-4 `monotone` ctor bool + HP-7 seal belt** (`:66-75`, `:177-194`):
`explicit InstanceStore(Allocator, bool monotone_ = true)` — default TRUE, EXACTLY
ONE reader (the belt guard `:185`). Codegen ALWAYS constructs default
(`Database.cpp:1461` `instance_<id>(allocator_)`, no bool arg) ⇒ `monotone==true`
program-wide, belt ARMED. The belt (`#ifndef NDEBUG`, `if(monotone)`): asserts
every frozen row present in current (`frozen⊆current`) — the (T,F) drop set is
PROVABLY EMPTY under R-MONO; a "cannot-exist" (T,F) trips the LOUD assert instead
of silently retracting against a monotone pub. Debug-only, INERT under NDEBUG.
The codegen dual is V-INST-FRESH (§2.4). Belt teeth proven by
`BeltFiresOnMonotoneDrop` (:406, forked-child SIGABRT, RAT-5 negative).

### 1.6 DebugValidate + destructor

`DebugValidate` (`:222-235`, `#ifndef NDEBUG`, called post-Seal from codegen):
`touched.Empty()`, all `touched_flag==0`, `(sealed_occupied!=0)==(frozen.NumRows()>0)`
(`:230` — tautological given Seal `:203-204`; diverges from intent under R-DIFF
mid-epoch annihilation), `frozen[iid]!=current[iid]` non-aliasing. Destructor
(`:77-88`) frees `slots` + destroys both tables per iid.

---

## §2 THE INSTANCE-OP FAMILY + LOWERING + CODEGEN AS LANDED

### 2.1 The op-kind vocabulary (`lib/Rel/Rel.h:148-161`)

Three `DROpKind`s + one `DRInstance` descriptor:
- `kSubgraphInstantiate` (15) — BIRTH/REBUILD + band-(b) publish. "Sole deriver
  of the published pub_table." Rides HP-3: `table_op_table = pub_table`,
  `table_op_sign = +1` (no new pub field).
- `kInstanceDeath` (16) — whole-instance DEATH, its OWN op, "NO fold/counter —
  the zero-counter death signature is the teeth." `table_op_table = pub_table`,
  `table_op_sign = -1`. **"MINTED ONLY when the demand table is differential
  (R-DIFF); ships INERT at D1.b/D2.b (HP-17)"** (`Rel.h:153-158`).
- `kInstanceSeal` (17) — trailing pointer swap (band 11 via key_of), self-lowered
  from its own dispatch (HP-1/OD-5), sign-0 kStateFold seal.

Shared payload (`Rel.h:703-721`): `demanded_view`, `demand_table`, `input_table`,
`instance_store_id`, `forcing_index`, `context_cols` + `context_col_sources`
(the D2.b α FOLD-side, arm-B multiset; `BindingSource` `Rel.h:472-478` =
`{kRowSlot, kInstanceKeySlot, kConfigSlot}`, kConfigSlot reserved/never set).
Two new EffKinds (`Rel.h:86-93`): `kInstanceRebuild` (WRITE of the store
`current` word, hazard-keyed on pub via `value_table`, structural sign ±1 =
TryAdd vs Recycle) + `kInstanceDemand` (frozen READ of the drained demand key,
NO hazard — HP-8). `DRInstance` (`Rel.h:870-888`) carries `pub_view`,
`forcing_name` (precomputed — Format has no `query` handle), `key_cols`,
`row_cols`. `SubgraphInstances()` (`Rel.cpp:585-592`) returns every
`kSubgraphInstantiate` op in construction order (NOT deaths/seals).

### 2.2 The mint — `BuildSubgraphInstanceOps` (`Rel.cpp:1019-1173`)

Called from `BuildDRInventory` (`Rel.cpp:2040-2042`) gated on
`context.demand_instance_enabled` (the `-demand-instance` selector; when OFF
`RecognizedSubgraphs()` is empty and the family is vacuous). Per
`RecognizedSubgraph`, re-resolve the three tables from LIVE guard JOINs via
`ResolveLiveRecognition` (`Rel.cpp:917-1006`, §3.6) — NEVER a stored dangling
`RecognizedSubgraph` handle (crit-correctness-1); a fully-dead forcing
(`ri.ok==false`) is skipped ABA-safe (`Rel.cpp:1037-1039`). HP-4 recognizer belt
(`Rel.cpp:1049-1053`): aborts LOUD if the re-resolved input_view is
MAP/NEGATE/AGG/KVIndex (belt, not live gate — the demand body-walk pre-rejects).

`diff = TableIsDifferential(pub_table)` (`Rel.cpp:1055`) forks the effect set.
```
kSubgraphInstantiate  (Rel.cpp:1086-1135):
  ctx=kSeed; table_op_table=pub_table (HP-3); table_op_sign=+1
  demand_table/input_table/demanded_view/instance_store_id=sid/forcing_index set
  effects = InstantiateEffects(diff, pub, demand, input)   // §2.3
  context_cols/sources: per pub position p<npub, kInstanceKeySlot iff p in key_cols
  spine: ONE DRArm{+1}: PlanNode{kAccess} over input_table, pred=kPresent,
         lowering=kSectionWalk, bound_cols=input_key_cols (all kInstanceKeySlot)
         -> child kFold into pub_table (+1, kNonRecursive)
```

**The kInstanceDeath gate (`Rel.cpp:1138-1150`; the `if` at :1139) — the R-DIFF frontier:**
```c++
if (demand_table && TableIsDifferential(demand_table)) {
  DROp death(kInstanceDeath);
  death.ctx=kSeed; death.table_op_table=pub_table;
  death.table_op_sign=-1;   // OD-2: shares table_id ⇒ sorts before +1
  death.demand_table=demand_table; death.demanded_view=*ri.demanded_view;
  death.instance_store_id=sid; death.forcing_index=rs.forcing_index;
  death.effects = DeathEffects(pub_table, demand_table);
  flow.ops.push_back(std::move(death));
}
```
`TableIsDifferential(demand_table)` (`Build.cpp:705-721`) is true iff some member
view `CanProduceDeletions()` OR is aggregate/KV-index. **The demand table is a
fabricated `demand__` ADDS-ONLY MONOTONE message** (`Parse/Demand.cpp:163-204`,
never `@differential`) ⇒ this gate is **FALSE corpus-wide AND on any accepted
program** (recursive-demand / @differential-input reject upstream, §3.5), so the
death op is NEVER minted (census `kInstanceDeath=0`). The gate is not just
unfired-on-corpus but unreachable at the current feature frontier.

`kInstanceSeal` (`Rel.cpp:1151-1158`) always minted 1:1 with instantiate.
Stratum seed (`Rel.cpp:1160-1171`): `instance_stratum[sid] =
1+max(ready_after(demand_table), ready_after(input_table))`.

**The death↔instantiate ordering has NO explicit DRDep edge** — it is the pinned
sort key. `key_of` (`Rel.cpp:5102-5108`) falls both to
`Key{1, stratum, band=0, table_id=pub+1, sign, oi}`; they agree on all but
`op_sign` (`:5004-5019`), and death −1 < instantiate +1 (`key_less` `:5115`,
"− before +", OD-2). V-INST-ORDER/`CheckInstanceOrder` (§2.5) is the guarantor.

### 2.3 The effect builders (`Rel.cpp:781-892`) — the §3.3 regime split

`InstantiateEffects(diff, pub, demand, input)` (`Rel.cpp:781-859`). ALWAYS:
1. `kVecDrain(demand, kNetAddition)` — demand frontier (a1).
2. `kVecDrain(input, kNetAddition)` — input/edge REBUILD frontier `[R-REBUILD-a2]`.
3. `kInstanceDemand(read=demand)` — frozen demand read, no hazard.
4. `kFlagRead(read=input, pred=kPresent, ctx=kSeed)` — rederive leaf (`reads:`).
5. `kInstanceRebuild(value=pub, sign=+1)` — WRITE store `current`.
6. `kStateEmit(read=pub)` — collapsed emit, read current (the (F,T) publish).
7. `kStateOld(read=pub)` — collapsed old, read frozen (**the (T,F) retract SOURCE
   — already minted; only the downstream consumer is absent**).

Regime split (`:829-857`):
- **R-DIFF** (`diff==true`, DEAD today — pub monotone): per sign∈{+1,−1}: a
  `kCounter(pub, sign, NonRecursive)` + `kInIReadFrozen(pub, kInI, kSeed)` +
  `kVecAppend(pub, sign<0?kDeleteQueue:kAddQueue)`. ⇒ 2 counters/2 crossings/2 appends.
- **R-MONO** (`diff==false`, the only live path): ONE `kCounter(pub, +1,
  NonRecursive)`; ZERO crossings/appends; NO pub-queue TableVec.

`DeathEffects(pub, demand)` (`Rel.cpp:861-884`) — the zero-counter death
signature (EXACTLY ZERO {kStateEmit, kCounter, kInIReadFrozen, kVecAppend}):
1. **`kVecDrain(demand, kNetRemoval)`** — note kNetRemoval (the demand keys whose
   demand was retracted). A death ASSUMES a demand net-REMOVALS frontier that
   nothing provisions/checks today (§5 G-DEMAND-NEG).
2. `kInstanceDemand(read=demand)`.
3. `kStateOld(read=pub)`.
4. `kInstanceRebuild(value=pub, sign=-1)` — structural −1 = Recycle.
`SealEffect(pub)` (`Rel.cpp:886-892`): one `kStateFold(pub, sign=0)`.

### 2.4 The lowering — `LowerSubgraphInstances` (`Procedure.cpp:265-333`)

Called from `PublishDifferentialMessageVectors` (`Procedure.cpp:418`) BEFORE
`LowerCommitSweeps` (`:419`). Per `SubgraphInstances()` op (kSubgraphInstantiate
ONLY):
- resolve `demand_front = TableDeltaVector(demand_table, kNetAdditions)`
  (`:274-276`) + `input_front = TableDeltaVector(input_table, kNetAdditions)`
  (`:279-281`, the SAME memoized edge frontier the eager cut-successor writes,
  Build.cpp:999-1004).
- **the region is minted TWO-ARG** (`CreateDerived<SUBGRAPHINSTANCE>(seq, sid)`,
  `:284` — NO diff flag; see XC-2). Populate demand/input frontier, input/pub
  table (`pub = op->table_op_table`, HP-3), key/row positions,
  `input_key_cols = op->arms[0].body->bound_cols` (`:294-297`).
- **ALWAYS-ON key-arity belt** (`:305-311`): `input_key_cols.size() !=
  key_cols.size()` ⇒ fprintf+abort (the zero-fill silent-probe latent; survives
  NDEBUG).
- **enroll `{sid, kSubgraphInstantiate}` + `{sid, kInstanceSeal}` ONLY** (`:328-331`)
  — a death is NOT enrolled here.

**V-INST-EMITTED** (`Procedure.cpp:421-450`): sort + multiset-compare
`context.emitted_instance_ops` against the flow's {Instantiate, Death, Seal}
enrollment; mismatch ⇒ fprintf+abort. **CRITICAL: if a kInstanceDeath were ever
minted, the flow enrollment gains `{sid, kInstanceDeath}` but the lowering
enrolls only instantiate+seal → the multisets DISAGREE → V-INST-EMITTED ABORTS
THE COMPILE.** So a death today is not merely inert — it is fatal-if-minted.
This is the load-bearing structural gap (§5 G-DEATH-LOWER).

### 2.5 The validators (all always-on fprintf/ValidatorFail, survive NDEBUG)

- **Census recount** (`Rel.cpp:3967-3996`, gated `demand_instance_enabled`):
  re-resolves via the SAME `ResolveLiveRecognition`, `expect(kSubgraphInstantiate,
  exp_instance)` / `expect(kInstanceDeath, exp_death)` (same
  `TableIsDifferential(demand_table)` gate as the mint) / `expect(kInstanceSeal,
  exp_instance)`. Byte-visible count contract.
- **V-INST-EFFECT** (`Rel.cpp:4266-4382`): per-op source-aware effect totality
  (drain value_tables must be demand OR input, not "demand twice"). Instantiate
  gate splits on `diff`; death gate is the zero-counter signature
  (`!forbidden && drains==1 && demands==1 && olds==1 && rebuilds==1 &&
  rebuild_sign==-1`) — VACUOUS today.
- **V-INST-SOLE** (`Rel.cpp:4334-4339, 4383-4388`): an instantiate's `input_table`
  must NOT be differential and must NOT alias pub; each pub has EXACTLY one
  instantiate deriver. **Note: forbids a differential INPUT today** (§6 OQ-INPUT).
- **V-INST-PAIR** (`Rel.cpp:4389-4412`): per store `n_inst==1 && n_seal==1 &&
  n_death<=1`; every death/seal store has a matching instantiate. (This validator
  ACCEPTS a minted death — V-INST-EMITTED, not this, aborts on one.)
- **V-ALPHA** (`Rel.cpp:4415-4493`): arm A — any kInstanceKeySlot binder must be
  `lowering∈{kPointTest,kSectionWalk}` (never a full scan); arm B —
  context_col_sources column-total/positive/negative. Short-circuits on
  all-kRowSlot corpus.
- **V-INST-DRAIN** (`Rel.cpp:4495-4523`, HP-2): per instantiate,
  `table_delta_vecs[demand_table]` AND `[input_table]` must hold non-null
  **kNetAdditions** frontiers. **Nothing checks a demand net-REMOVALS frontier
  (which DeathEffects assumes)** — §5 G-DEMAND-NEG.
- **V-INST-ORDER / CheckInstanceOrder** (`Rel.cpp:4753-4785`, called `:5651`):
  `DROpStratum` for instantiate/death FAILS LOUD on a map miss (crit-pins-1,
  stronger than kGroupUpdate's silent `return 0`); `CheckInstanceOrder` is
  factored PURE (reads pinned_order + kind + store_id) and asserts
  `death_pos < inst_pos` per store. **Vacuous-green today** (0 deaths ⇒
  `death_pos` empty ⇒ loop body never runs). Teeth proven ONLY by the
  hand-built `tests/RelValidators/InstanceOrderTest.cpp` fork/waitpid SIGABRT
  test (RAT-3 permanent).

### 2.6 Codegen — `EmitSubgraphInstance` (`Database.cpp:2286-2461`)

Dispatched `Database.cpp:1877`. The store is a `Database` member
`instance_<sid>` (declared `:1578`, constructed default-monotone `:1461`,
threaded as ref param `:895-899`); `Key_<id>`/`Row_<id>` structs emitted
`:1239-1258`; runtime header `#include`d iff ≥1 store (`:3332-3333`).

Four bands, all sharing `emit_instance_rescan` (`:2308-2346`) — a FULL SCAN of
the input table with a key filter, headed by the **V-INST-FRESH** codegen dual
(`:2310-2316`, always-on fprintf+abort, NOT an assert): `if
(WorkingOccupied(iid)) abort` — current must be empty at band-(a) entry (the
codegen half of "current is a from-scratch rebuild each epoch").
```
band-(a1) BIRTH   (:2348-2367): for [k..] in demand_vec:
                    iid=FindOrAddInstance(Key{k..}); if !TouchedFlag: rescan(kbinds)
band-(a2) REBUILD (:2369-2403): for [e..] in input_front:   // full edge row
                    iid=FindInstance(Key{e<in_key>..});      // NON-adding; stray SKIPS
                    if iid!=kNoInstance && !TouchedFlag: rescan(ekeyexprs)
band-(b) PUBLISH  (:2405-2454): for iid in Touched():
                    cur=Current(iid); frz=Frozen(iid); key=KeyAt(iid)
                    for r<cur.NumRows(): row=cur.RowAt(r)
                      if frz.Find(row)==kNoRow: pub.TryAdd(<key.c*/row.c* projected>)
                    // (F,T) BORN ONLY — NO (T,F) drop scan (XC-1)
seal              (:2456-2460): instance.Seal(); #ifndef NDEBUG DebugValidate()
```
NEVER emitted today (grep of the witness dump): `RecycleCurrent`, the
`monotone=false` ctor arg, any (T,F) drop scan, any retract/SubDerivation against
pub. Render is faithful — the `.ir` `subgraph-instance` line
(`Format.cpp:659-672`) and the `.rel` op block (`Format.cpp:864-902`, incl. a
FULL kInstanceDeath render arm `:883-893` "never rendered at D1.b/D2.b") both
support R-DIFF; only mint/lowering/codegen are the gaps.

Live witness (`demand_neighborhood_witness` + `-demand-instance`): op.0
kSubgraphInstantiate stratum=1 (2 net-add drains, 1 demand read, 1 rebuild+, 1
emit, 1 old, ONE counter+, no crossings/appends) + op.1 kInstanceSeal band=11;
census `kSubgraphInstantiate=1 kInstanceDeath=0 kInstanceSeal=1`.

---

## §3 THE DEMAND PASS / ADMISSION / FABRICATION / FORCING AS LANDED

### 3.1 Where it sits + the mode gate

`QueryImpl::ApplyDemandTransform(module, log, demand_mode)` — invoked from
`lib/DataFlow/Build.cpp:2587`, AFTER `ConnectInsertsToSelects`, BEFORE
`Optimize`/`IdentifyInductions`/`Stratify`. Magic-sets / SLDMagic as node
minting + rewiring, not an evaluator. **MODE GATE** (`Demand.cpp:392-394`):
`if (!demand_mode) return true;` — flag-off mints nothing, the pre-demand corpus
is byte-identical. Every reject below fires ONLY under `-demand` (per-case
`.drflags`). The uniform reject shape (`Demand.cpp:406-410`) always appends
"`; recompile without -demand`" — the honesty contract (outside-slice shapes are
clean diagnostics, never miscompiles).

### 3.2 The 11-step pipeline + every reject (LIVE-captured)

- **STEP 0** re-entry guard (`:398-404`): `DemandMessagesFabricated()` ⇒ "Internal
  error: the demand transform was re-entered …". Single-shot fabrication.
- **STEP 1** collect bound queries (`:416-428`, `IsQuery() && Arity()` + a
  kBound param). **REJECT >1 bound query** (`:434-438`): *"Multiple demanded
  (bound) queries are not yet supported under -demand"*.
- **STEP 1b MULTI-ADORNMENT reject** (FIRST belt, `:451-461`): a query NAME
  redeclared at ≥2 binding patterns shares ONE `DeclarationContext` (REL is per
  (name,arity)), so the >1-query fence misses it. `UniqueRedeclarations()` →
  `BindingPattern()` set; `!=1` ⇒ *"Multi-adornment demand is not yet supported
  (a demanded query name with more than one binding pattern) under -demand"*
  (LIVE on `demand_multi_adorn_1`). WHY (`:443-450`): CF emits one entry per
  unique redecl, only the transformed adornment has a valid seeder, and
  `ParsedQuery::operator==` is context-keyed (name+arity ONLY) so a sibling could
  inherit the wrong injector.
- **STEP 2** trace the projection chain to p's read (`:476-576`): REJECTS —
  not-exactly-one materialization (`:476`), multi-clause query (`:513`), "must
  all read one relation" (`:532`), "must share one projection" (`:544`,
  UNREACHABLE belt — adjacent rejects fire first), "must project from a single
  derived relation" (`:521`), non-distinct bound positions (`:568`). Yields
  `q_consumer`, `q_read`, `p_merge`, `p_bound` (α as p-column positions).
- **STEP 3** SIP walk locating each guard site (`:583-758`): the body-tree
  honesty walk (`:596-637`) rejects non-TUPLE members, SELECT non-IO,
  **NEGATE/AGG on the demand path** (`:624-627`, "the demand sink"), any other
  view kind (`:628-629` — **the SHADOWED recursive-content belt**, LIVE on
  `demand_recursive_content_1` whose `.drflags` is bare `-demand`), self-join
  (`:632-636`). Site classification into `GuardSite`
  {kReadAtTuple/kPushDown/kBaseAtom}; multi-adornment α-position rejects at
  `:667`/`:722`; per-body consistency reject (`:737-749`).
- **STEP 4** stray-consumer accounting (`:768-790`): every reader of p_merge
  traced, every consumer known; REJECTS non-reader consumer (`:776`), untraced
  reader-consumer (`:782`, "a sibling query or another rule").
- **STEP 5** FABRICATE demand message + #local (`:796-847`): adornment string
  `adorn` (b/f per param); reserved `base_name = "demand__"+name+"_"+adorn`
  (`:806-809` — LEADING underscore lexes as a VARIABLE, so `demand__` lowercase
  is the lexable prefix). BOTH G3 collision pre-checks run BEFORE any mutation
  (`DemandFabricationWouldCollide` + the two Fabricate nullopt paths) ⇒ collision
  rejects without orphaning half-fabricated decls.
- **STEPS 6-8** mint the seed (IO+receive+relation+MERGE+reader, `:856-974`),
  guard each body at its site (`MintGuardJoin` `d_reader⋈read`, `:988-1024`),
  the query-projection guard (`raw_seed` guard, `:1032-1062`).
- **STEP 8b** register the `RecognizedSubgraph` (`:1070-1079`): the recognition
  unit is the FORCING (one per DemandForcing, X-9); stored `QueryView` handles
  (p_merge, q_insert) DANGLE post-Optimize (never dereferenced —
  ResolveLiveRecognition re-walks live).
- **STEP 9 TRIPWIRE** (`:1090-1117`, always-on fprintf+abort): walks the ACTUAL
  minted structure to the fabricated receive; a broken root member fires
  `DEMAND-TRIPWIRE`.
- **STEPS 10-11** register the forcing (`:1125-1126`), `MarkDemandFabricated()`
  (`:1149`); the annotation census (`:1136-1147`, `#ifndef NDEBUG` — no reader
  under flat `-demand`; the always-on abort arrives with the `-demand-instance`
  reader).

### 3.3 Fabrication (`lib/Parse/Demand.cpp`) — the R1 retract anchor

`FabricateDemandMessage` (`:163-204`) / `FabricateDemandLocal` (`:206-241`):
lex a real interned name (`LexInternedAtom` `:51-100` — a synthetic empty-range
token prints NOTHING, so names are lexed from a real display buffer), G3 scan
(`CreateDerived` BYPASSES AddDecl's redecl id-map so a collision would print two
same-named procs), mint a real `ParsedMessageImpl`/`ParsedLocalImpl`.
**LOAD-BEARING: the fabricated message is NOT `@differential`** —
`ParsedMessage::IsDifferential()` reads `differential_attribute.IsValid()`
(`Parse/Parse.cpp:1336`), never set here. So the demand seed is MONOTONE — the
root fact under `-demand-retract` (§5 G-RETRACT).

### 3.4 Forcing registry + injector + suppression

Registry structs (`Query.h:966-1032`): `QueryDemandForcing{query, message,
bound_params}`, `GuardAnnotation` (Kind/DemandSide/Role, `is_instance_key`
ALWAYS false in this slice, `forcing_index`), `RecognizedSubgraph{forcing_index,
demanded_view, key_cols, pub_view, guard_annotation_indices}`.
`BuildQueryForceProcedure` (`Build.cpp:452-484`) FIRST consults the registry
matching `entry.query==query && BindingPattern()==BindingPattern()` — **the
binding-pattern check is the SECOND belt** of the multi-adornment fix
(`:459-466`; `operator==` cannot tell adornments apart). Injector
(`BuildQueryForceProcedureFromRegistry` `:385-447`): `add_vec=kParameter`;
`del_vec` ONLY `if (message.IsDifferential())` (`:416-421` — FALSE for the
fabricated message ⇒ no del vector). Codegen suppression (`Database.cpp:1496-1502,
3359-3365`): a demand-seed message gets NO public hidden-friend entry
(`IsDemandMessage`), only its `_detail` twin.

### 3.5 The `-demand-instance` nested pre-pass fences (`Build.cpp ~1378-1447`)

Gated `if (demand_instance)` (`:1393`); under plain `-demand` NONE fire (the flat
lowering handles all shapes). Resolved from LIVE guard JOINs (never a stored
handle). Per forcing, per body guard's `jl[1]` (summarized input):
- `in.CanReceiveDeletions()` → `diff_input`.
- `in.InductionGroupId().has_value() || ViewSelfReachable(in)` → `recursive_content`
  (also over predecessors).
- `ViewSelfReachable(jl[0])` (demand side) → `cyclic_demand`.
Priority (`:1435-1445`): cyclic first, else recursive-content, else diff_input:
- **FENCE (i)** cyclic-demand: *"Recursive demand relations are not yet supported
  under -demand-instance"* (LIVE `demand_cyclic_1`).
- **FENCE (i, ADJ-C2)** recursive-content: *"Demanded subgraphs with recursive
  (induction-owned) content are not yet supported under -demand-instance (a
  keyed-instance feature gap)"*.
- **FENCE (iii)** diff-input: *"Demanded subgraphs over deletable (differential)
  inputs are not yet supported under -demand-instance"* (LIVE `demand_diff_input_1`).
FENCE (ii) mid-stream monotone edge-add is NO LONGER a gap (R-a2 band-(a2)).
LAYERING: `demand_recursive_content_1` rejects EARLIER (the plain-`-demand` body
walk §3.2 STEP 3a); `demand_cyclic_1`/`demand_diff_input_1` COMPILE under plain
`-demand` and reject only here — the flat pass rejects what it cannot GUARD, the
instance pre-pass what it cannot NEST-LOWER.

### 3.6 ResolveLiveRecognition + the eqgate

`ResolveLiveRecognition` (`Rel.cpp:917-1006`) — the SINGLE ABA-safe authority
re-resolving a forcing's three tables from live guard JOINs + parse identities
(pub = the live INSERT whose `Declaration().Id() == forcing.query.Id()`, full
name+arity). Both the mint AND the census recount call it (cannot drift). Picks
the FIRST body-guard's `instance_key` as `input_key_cols` (`:960-967`) —
**assumes ONE key layout per forcing** (the multi-adornment hazard, §5 G-ADORN).
The eqgate (`.eqgate` sidecar → `run_eqgate`, `runall.sh:305-330`): recompiles
the nested arm (`+-demand-instance`) in all 4 modes, byte-compares each stdout to
`goldens/<name>.stdout` LIVE (no nested golden blessed; flat==nested==golden
transitively) — the answer-identity contract R-DIFF must preserve.

---

## §4 THE DIFFERENTIAL MACHINERY AS LANDED WHERE D3.a TOUCHES IT

### 4.1 The engine is table-generic (the load-bearing fact)

`BuildDRInventory` (`Rel.cpp ~1780ff`) mints, per **differential,
non-induction-owned** table (`Rel.cpp:1813-1816`, gate
`TableIsDifferential(table) && !TableIsInductionOwnedDR(...)`), the six
discovery-band vecs (kDeleteQueue/kAddQueue/kOverdeleteSet/kAdditionSet/
kNetRemoval/kNetAddition, `:1817-1828`) + (SCC only) kClaimedDel/kClaimedAdd
(`:1836-1842`). **None of this keys on "is this table demanded."** So a demanded
CONTENT table that is differential inherits the whole engine for free.

### 4.2 The per-stratum OVERDELETE→REDERIVE→INSERT ops

- Split signed counters `C_nr`/`C_r` (`total = C_nr+C_r > 0`; `kCounter` w/
  `DerivClass`, `Rel.h:79/353-357`).
- **The two claim gates** (`ClaimGate` `Rel.h:318-321`, a DEDICATED vocabulary
  NOT a `Pred`): `kDelGateCnrNonPositive` (TryClaimDel: C_nr≤0),
  `kAddGateTotalPositive` (TryClaimAdd: total>0). Derived from the drain sign at
  `mint_claim` (`Rel.cpp:2458-2508`); RE-TEST at dequeue (V-CLAIM-GATE, the F17
  fix — a row claimed-for-delete whose C_nr rose is dropped). Per-row within one
  table ⇒ a demanded content table gets them free.
- `kRederive` (`Rel.cpp:2567-2591`, SCC del-side): overdeleted-but-still-
  RecursivelySupported rows re-queued for INSERT. `kRetire` (`:2549-2566`).
- `kFrontierFilter` (`mint_filter` `:2510-2537`): net-presence publish inside the
  fixpoint. `kCommitSweep` (`:2604-2650`): per-table publish (`was!=now`) +
  compaction; differential vs monotone flavor. `LowerDRFlow`/`LowerDRRounds`/
  `LowerCommitSweeps` (`Stratum.cpp:1442/1799/2067`) — **none branches on
  demand/instance state**.

### 4.3 The ten membership predicates + where a demanded subgraph's tables sit

`Pred` (`Rel.h:104-115`): kPresent/kInI/kInNew/kSurvivesSoFar/kAliveAtClaim/
kInNewWithFrontier/kInNewSansFrontier/kRecursivelySupported/kNetDeleted/kNetAdded
(runtime methods `Database.cpp:2464-2472`). Two placements:
- **Demanded content tables** (predicates in the demanded body): plain
  `impl->tables` entries; differential ⇒ full predicate vocabulary + ordinary
  claim/rederive/sweep. The demand transform only NARROWS what materializes (the
  guard join), never the membership machinery. Under flat `-demand`,
  `demand_diff_input_1`'s `ans`/`pt` ALREADY run the full differential engine
  (the flat lowering is differential-capable; the fence is INSTANCE-only).
- **The pub_table behind an InstanceStore**: a PREDICATE-FREE ISLAND — the nested
  tables are `Find`/`TryAdd` sets, occupancy is `NumRows()>0` (§1). The store
  publishes INTO pub via `TryAdd` at band-(b) (`Database.cpp:2438`); only THEN
  does pub's own commit sweep (if differential) apply the vocabulary. The store's
  OUTPUT re-enters the differential world at the pub boundary.

### 4.4 Negate-gate context keying (the retraction-safety net)

`NegateGatePred(Ctx, NegateHint)` (`Rel.cpp:535-543`): kEager→(kNever?kPresent:kInI),
kSeed→kInI (both signs, F18), kFixpoint→kInNew. A `!` negate reads the negated
table BATCH-FROZEN (`kInI`, sealed watermark) precisely because it can retract;
`@never` keeps count-based `kPresent`. Validated V-NEG-CTX (`Rel.cpp:3466-3480`).
Engages as-is for demanded content through a normal `!` negate.

### 4.5 The band-b publish + RAT-7 re-open

Runtime doc (`InstanceStore.h:135-137`) names BOTH directions: born {(F,T)} +
dropped {(T,F)}. Codegen emits ONLY (F,T) (`Database.cpp:2405-2454`; comment
`:2433-2434` RAT-7 "no runtime assert; the SITE-3 review line is the guardian").
The Seal belt (`:185-192`) asserts the (T,F) set empty under `monotone`. RAT-7
(the band-(b) partition-assert question) RE-OPENS at D3.a: whether a runtime
partition assert is owed returns when R-DIFF makes drops reachable.

### 4.6 OWN-3 — the two-distinct-guard CSE fold diagnostic (`View.cpp:583-590`)

Inside `CopyDifferentialAndGroupIdsTo` (the choke transferring
`guard_annotation_index` on view replacement): `assert(that->guard_annotation_index
== guard_annotation_index)` (`:588`). Each recognized subgraph carries a UNIQUE
stamp; two annotated guards CSE-merged must agree (else one InstanceStore absorbs
another — a mis-keyed instance). Currently a bare `assert` (NDEBUG-stripped),
DORMANT ("guard JOINs are structurally distinct and CSE-stable" — no two
annotated guards fold today). §20(C)/OWN-3 + E-96: it must be PROMOTED always-on
("the record-comparing incompatible-fold diagnostic + fold count") as a **HARD D3
PRECONDITION** before recursive/multi-guard demanded content is admitted.

### 4.7 DS-R4-10 — @never over differential input (the labeled latent gap)

The crossover loop that mints the retraction arm-pair SKIPS `@never` negates
(`Rel.cpp:1852-1855` `if HasNeverHint(): continue`); V-XOVER-ONE aborts if a
`@never` ever carries a crossover (`Rel.cpp:2923-2924`). `@never`'s `kPresent`
gate (`Rel.cpp:537`) is count-based (flaps mid-batch) — SAFE only because
`@never` PROMISES the negated view never retracts. A differential (deletable)
view fed as a `@never`-negated input violates the promise: the `kPresent` read
sees dips AND no crossover propagates the loss. DS-R4-10 (§20(C), no directed
fence): a `@never`-over-differential is a silent-miscompile shape, currently
unreachable-by-user-contract but unguarded. D3.a must decide: reject
(clean diagnostic) or auto-promote `@never`→`!` (crossover + seal + kInI).

### 4.8 NeedsInductionCycleVector — the §20(AB) modeling precondition

`Induction.cpp:10-21`: an induction-owning MERGE returns `true` UNCONDITIONALLY
(`:12-13`, `// TODO(pag): Needed to avoid stack overflow?!`). The `else` branch
(the conditional `!NonInductivePredecessors().empty() || IsOwnIndirect...`,
`:15-16`) is NEVER reached for an owning merge. This makes the second
`BuildEagerUnionRegion` caller (`Induction.cpp:1005-1006`) DEAD — a LABELED
COVERAGE HOLE (`:996-1004`): if the short-circuit is relaxed, an owning merge
with all-inductive predecessors would emit a real union region with **NO DR op
modeling it** (no `kEagerUnion`; A.6(c) would abort a mint on an owning merge),
breaking the model↔emission invariant that SD-4's set-oracle completeness rests
on. §20(AB) forward note: recursive/differential demand is precisely what could
make an owning merge's induction structure differ, so any D3.a work touching
recursive demand MUST model the union region BEFORE relaxing the short-circuit.

---

## §5 THE D3.a GAP LEDGER (deduped across all four lanes; anchored)

Each gap: the code fact that makes it a gap + a one-line "what a design must
decide." Ordered from the store outward.

**G-DIFF-TABLE — the nested table cannot represent differential content.**
The store is `<Key, RowT>` with the algebra slot fixed to a monotone
`Table<RowT>` (`InstanceStore.h:56-57`); no signed per-row counter, membership is
boolean `Find`/`TryAdd`. *Decide:* does R-DIFF make each nested buffer a
DiffTable (per-row C_nr/C_r), revive a parallel signed `working_count` Vec, or
rely wholly on the pub-table's counters after publish (the rescan model)?

**G-ROW-RETRACT — no per-row retract; the only shrink is whole-buffer Reset.**
`current` grows (`TryAdd`) or is wholly emptied (`Reset` via `RecycleCurrent`/
Seal, `InstanceStore.h:202/218`). *Decide:* is per-instance shrink realized by
the rebuild-from-scratch a1/a2 rescan (Reset + rescan the shrunk input) or by a
true incremental per-row delete? (This decides whether G-OCCUPANCY is real or
moot — Lane A OQ6 / Lane D OQ4, the single biggest fork.)

**G-OCCUPANCY (N-1) — WorkingOccupied is a monotone-only lie.**
`WorkingOccupied = current->NumRows()>0` (`InstanceStore.h:161-163`), EXACT only
because band-(a) is a monotone TryAdd (`:157-160`). A mid-batch add+retract
annihilating to empty reports occupied. *Decide:* re-introduce the dropped signed
`working_count`, or make occupancy read a differential net-presence, or keep
NumRows exact by the full-rescan model (moot under G-ROW-RETRACT = rescan).

**G-BELT-FLIP — the HP-7 seal belt hard-assumes monotone=true.**
The belt (`:185-193`) asserts `frozen⊆current`; codegen always constructs
default-true (`Database.cpp:1461`). The ctor already accepts `monotone=false`
(`:66`) to gate it off. *Decide:* the selector plumbing from
`TableIsDifferential(pub/demand)` to the ctor arg (unbuilt), AND — turning it off
is not enough — a REPLACEMENT differential invariant for the store (§6 OQ-BELT).

**G-TF-PUBLISH — band-(b) has no (T,F) drop scan (XC-1).**
Codegen emits only the (F,T) born scan (`Database.cpp:2435`); the `kStateOld(pub)`
(T,F) SOURCE effect is already minted (`Rel.cpp:824-827`) but has no consumer.
Runtime `Current`/`Frozen` are exposed (`InstanceStore.h:138-139`). *Decide:*
emit the dual scan (walk frozen, Find in current, publish a RETRACT to pub's
del-queue/counter) — the per-row analog of the StateCell net-pair
(`Database.cpp:2237-2273`), absent here.

**G-DIFF-REGION — the SUBGRAPHINSTANCE region carries no diff bit (XC-2).**
Landed ctor is two-arg (`Procedure.cpp:284`); the design's third
`TableIsDifferential(pub)` arg (d2b-design:690) is UNLANDED. *Decide:* how the
region/codegen learns diff-ness (ctor arg vs re-derive from pub_table) to select
the R-DIFF band and the R-DIFF InstantiateEffects lowering (the 2-counter/2-append
form already coded at `Rel.cpp:829-848` but never lowered).

**G-DEATH-LOWER — a minted death aborts the compile (fatal, not inert).**
`LowerSubgraphInstances` enrolls only instantiate+seal (`Procedure.cpp:328-331`);
V-INST-EMITTED (`:421-450`) multiset-aborts on any enrolled-but-not-emitted death.
`RecycleCurrent` (`InstanceStore.h:216`) has zero callers. *Decide:* the death
emitter — a SUBGRAPHINSTANCE death band (or sibling region), the V-INST-EMITTED
enrollment of `{sid, kInstanceDeath}`, the `RecycleCurrent` wiring, the (T,F)
full retract of all frozen rows for the dead key.

**G-DEMAND-NEG — no demand net-REMOVALS frontier is provisioned or checked.**
`DeathEffects` drains `kVecDrain(demand, kNetRemoval)` (`Rel.cpp:865-869`); but
V-INST-DRAIN checks only kNetAdditions (`Rel.cpp:4502-4522`), and nothing mints a
demand net-removals frontier (the cut/frontier machinery provisions
kNetAdditions only). *Decide:* where a demand table acquires a net-removals
frontier + a matching drain-existence check, and whether it needs a dependence
edge vs the demand net-additions frontier feeding the same store.

**G-RETRACT — the whole demand chain is monotone (R1-R6).**
The fabricated message is not `@differential` (`Parse/Demand.cpp:163-204`); the
injector mints no del_vec (`Build.cpp:416-421`); the demand relation is a
monotone MERGE web (`Demand.cpp:951-974`); the guard JOIN is a positive filter
that never sees a `−` on the demand side; the demand seed has no batch-netting
rule. *Decide:* the removal channel through every link (message `@differential` →
signed injector → differential demand relation → guard-JOIN overdelete →
kInstanceDeath), AND demand-batch netting semantics (add∩remove annihilation) —
and ref-counted vs set-demand policy (§6 OQ-RETRACT-POLICY).

**G-INPUT-NEG — no input net-REMOVALS rebuild band (the diff-content side).**
Band-(a2) drains only the input kNetAdditions frontier (`Procedure.cpp:279-281`);
V-INST-SOLE even FORBIDS a differential input (`Rel.cpp:4334-4339`). A retracted
input edge for a standing key triggers no rebuild. *Decide:* lift the fence
(FENCE (iii), `Build.cpp:1442-1444`), add an input net-removals frontier band,
and choose overdelete→rederive rescan vs full add-rescan for the shrunk input.

**G-STALE — untouched-but-live instances are not revisited.**
Seal swaps only TOUCHED iids (`InstanceStore.h:176`). Under R-DIFF an instance
whose shared content changed elsewhere but whose key wasn't in this epoch's
frontier is not rebuilt. *Decide:* whether the input-retraction band (G-INPUT-NEG)
subsumes this or a broader revisit protocol is needed.

**G-REBIRTH — no un-touch / demand-death / key-removal protocol.**
iids are append-only forever (`InstanceStore.h:13-16`); no `RemoveInstance`. The
a2 gate keys on `FindInstance != kNoInstance` (`Database.cpp:2390`) — once minted,
a key is forever live-demanded. *Decide:* on demand retraction, is the instance's
content fully retracted from pub and the iid tombstoned-but-retained? the
death-then-rebirth-within-one-epoch ordering (RecycleCurrent is idempotent but
nothing sequences it vs a same-epoch re-demand rebuild).

**G-FRESH-BELT — V-INST-FRESH forbids a non-empty current at band-(a) entry.**
`if (WorkingOccupied(iid)) abort` (`Database.cpp:2310-2316`). A same-epoch
demand-flap / death-then-refill has no modeled ordering. *Decide:* how V-INST-FRESH
tolerates a legitimately re-emptied current under R-DIFF (death Recycles, then a
still-standing demand rebuilds).

**G-OWN3 — the CSE fold diagnostic is a dormant debug assert (§4.6).**
`View.cpp:588`. *Decide (RULED as a precondition, mechanism open):* promote to
always-on record-comparing diagnostic + fold count BEFORE multi-guard/recursive
demand admits.

**G-NEVER-DIFF (DS-R4-10) — @never over differential input is unguarded (§4.7).**
*Decide:* reject (clean diagnostic on a `@never` over a CanReceiveDeletions view)
or auto-promote `@never`→`!`.

**G-INDUCTION-UNION (§20(AB)) — the dead union region under a relaxed
short-circuit (§4.8).** *Decide:* if recursive demand requires relaxing
`NeedsInductionCycleVector`'s merge short-circuit, MODEL the second
`BuildEagerUnionRegion` caller as a DR op FIRST (else an emitted region the .rel
dump cannot show; SD-4 stays blind).

**G-ADORN — the pass is single-forcing throughout.**
`bound_queries[0]` (`Demand.cpp:434`), `forcing_index = demand_forcings.size()`
(`:981`), the census asserts `recognized_subgraphs.size() ==
demand_forcings.size()` (`:1146`); `ResolveLiveRecognition` picks the FIRST
body-guard's key layout per forcing (`Rel.cpp:960-967`). Fabricated message
identity is ALREADY per-adornment (`base_name` embeds `adorn`, `:806-809`) — NOT
the blocker. The guard JOINs already carry `forcing_index` — the graph is already
forcing-partitioned. *Decide:* loop STEP 1b→10 per adornment; key ALL
forcing/instance lookups on `(query, BindingPattern)` (only the one injector belt
does today, `Build.cpp:467-476`); at the instance layer, one InstanceStore per
(p, α) with distinct key projections vs one store with a union key +
partial-key lookups (the OWN-3 fold diagnostic is the CSE-collision guard here).

---

## §6 RITUAL-HEAD OPEN QUESTIONS (the rulings stage (b) designs must receive)

Each phrased decidably, with the code-fact inputs a ruling needs. These draw on
the §20(AC) charter seed (kInstanceDeath ON, RAT-7 re-open, N-1, OWN-3, DS-R4-10,
-demand-retract, >1 adornment, the §20(AB) NeedsInductionCycleVector precondition).

**OQ-MODEL — full-rescan vs incremental per-row maintenance (THE pivotal fork).**
Band-(a) is a full input rescan into a reset `current` today
(`Database.cpp:2308-2346`); band-(a2) rebuilds on any edge change. Does R-DIFF
KEEP the rescan model (making D3.a mostly a pub-BOUNDARY problem — the store stays
a predicate-free set island, only pub's sweep is differential) or MAINTAIN each
nested instance differentially (per-row counters inside the store)? *Inputs:* the
rescan model moots G-OCCUPANCY/G-ROW-RETRACT (NumRows stays exact — the rescan
adds only live rows); the incremental model needs G-DIFF-TABLE + signed
occupancy. Lane A OQ6 + Lane D OQ4 both name this the single biggest
underdetermined choice. **Every other OQ's shape depends on this ruling.**

**OQ-AXES — are "differential demand" and "differential content" one axis or
two?** FENCE (iii) checks the INPUT/content side's `CanReceiveDeletions()`
(`Build.cpp:1418-1420`); kInstanceDeath mints on the DEMAND table being
differential (`Rel.cpp:1138`) — a DISTINCT axis. Nothing fences a differential
demand table today (they are monotone by construction). *Decide:* do the two axes
light up together or independently? *Inputs:* a differential demand could mint a
death and trip V-INST-EMITTED even without FENCE (iii) firing (Lane D OQ1).

**OQ-RETRACT-POLICY — what does retracting a standing demand MEAN
(-demand-retract)?** G-RETRACT names the R1-R6 removal channel. *Decide:*
ref-counted demand (retract only when the LAST demander leaves) vs set-demand
(any retract kills); and demand-batch netting (a demand added+retracted in one
batch annihilates, matching user-message SET semantics). *Inputs:* the fabricated
message is monotone (`Parse/Demand.cpp`); user messages net with SET semantics
elsewhere; multiple consumers may demand the same α (Lane C OQ1).

**OQ-N1 — what does a signed per-instance count COUNT?** N-1 says "revisit a
signed count" (`InstanceStore.h:24`) but not the representation. *Decide:* rows in
`current` (multiplicity) vs a presence bit per (iid,row) vs each nested table its
own DiffTable vs reliance on pub's counters after publish. *Inputs:* the flat
world uses split C_nr/C_r per row; the store's nested tables are index-free set
tables (no obvious per-row-counter slot); the designs differ observably (a
retraction netting to zero inside one instance vs one publishing a transient).
CONTINGENT on OQ-MODEL.

**OQ-BELT — the R-DIFF store invariant that replaces HP-7.** Once
`monotone=false` the seal belt is off (§4.5, G-BELT-FLIP) and DebugValidate's
occupancy-coherence assert (`InstanceStore.h:230`) becomes tautological-or-wrong.
*Decide:* the replacement always-on differential belt (e.g. "every published
retraction had a matching prior born"; the split-counter ≥0 discipline the
DiffTable commit sweep enforces). RAT-7's re-opened partition-assert question is
the band-(b) half of this. *Inputs:* no always-on differential store belt exists
today.

**OQ-PUBLISH-ORDER — the differential publish order at the store boundary.**
For flat tables OVERDELETE-then-INSERT is phase-enforced. The store publishes at
band-(b) into pub, which THEN sweeps. *Decide:* must a (T,F) drop from instance A
be visible before a (F,T) born from instance B in the same epoch (cross-instance
ordering)? *Inputs:* `Touched()` is sort-uniqued (`InstanceStore.h:144-147`) but
no drop-before-born discipline is specified; two demanded keys can publish the
SAME pub row (cross-instance de-dup of a retracted-in-one-still-live-in-another
row is undetermined — Lane A OQ2, Lane D OQ2).

**OQ-DEATH-VS-REBUILD — the same-epoch death+rebuild interleaving.** A key can be
both demand-retracted (death, Recycles current) and edge-rebuilt (band-a2,
TouchCurrent+rescan). The mint orders death-before-instantiate by sign; both
touch the SAME iid's current. *Decide:* does RecycleCurrent-then-rescan compose
to "rebuild wins if still demanded"? *Inputs:* `!TouchedFlag` dedup +
RecycleCurrent idempotence (`InstanceStore.h:210-219`) hint at intent; nothing
exercises the interleave (G-FRESH-BELT). CONTINGENT on OQ-MODEL.

**OQ-INPUT — is a differential INPUT ever admissible?** V-INST-SOLE forbids it
(`Rel.cpp:4334-4339`); a differential input is the natural R-DIFF source of (T,F)
drops. *Decide:* is the forbiddance a permanent invariant (R-DIFF drives drops
only from demand death / the rescan) or a D2.b fence D3.a lifts (FENCE (iii) +
V-INST-SOLE arm-a)? *Inputs:* flat `-demand` ALREADY lowers differential content
(the fence is instance-only); the eqgate answer-identity contract requires
nested==flat once the fence lifts.

**OQ-ADORN-KEY — multi-adornment instance-key identity.** Two adornments (bf, fb)
over one p key the same relation on OVERLAPPING columns (G-ADORN). *Decide:* one
store with a union key + partial-key lookups vs N disjoint stores.
*Inputs:* `ResolveLiveRecognition`'s "first body-guard wins" (`Rel.cpp:960-967`)
actively assumes one key layout per forcing; `RecognizedSubgraph.key_cols` IS the
adornment's α; the OWN-3 fold diagnostic (`View.cpp:588`) is the required
CSE-collision precondition (RULED a hard precondition — the mechanism, not the
decision, is open).

**OQ-OWN3 — the promotion mechanism (decision RULED, shape open).** OWN-3 promotes
`View.cpp:588` to an always-on record-comparing diagnostic + fold count before
multi-guard/recursive demand admits. *Decide:* the exact diagnostic shape + where
`guard_annotation_folded_count` (`Demand.cpp:1144`) feeds it, and whether the
post-Optimize census equation `n_stamped + folded == guard_annotations.size()`
(`:1146`) stays sound under more aggressive canonicalization (dead-flow can delete
an annotated view with no orphan bucket, `:1129-1135`).

**OQ-NEVER — @never over differential input: reject or promote (DS-R4-10).** §4.7.
*Decide:* clean-diagnostic reject (matches the slice's feature-gap fences) vs
auto-promote `@never`→`!` (add crossover, seal the table, switch `kPresent`→`kInI`).
*Inputs:* the crossover skip (`Rel.cpp:1853`) + V-XOVER-ONE (`:2923`) + the
count-based gate (`:537`) are the three sites a promotion touches.

**OQ-INDUCTION-UNION — recursive demand vs the induction short-circuit (§20(AB)
precondition).** *Decide:* does D3.a's recursive-demand support (FENCE (i),
`Build.cpp:1436-1437`) require relaxing `NeedsInductionCycleVector`'s
unconditional-true (`Induction.cpp:12-13`)? If so, the second
`BuildEagerUnionRegion` caller (`:1005`) MUST be modeled as a DR op FIRST (its own
op kind or a sanctioned mint; A.6(c) forbids a `kEagerUnion` on an owning merge).
*Inputs:* the forward note flags the dependency but the code does not show the
intended op kind; SD-4's set-oracle completeness rests on the short-circuit.

---

## §7 THE POST-D3.a.0 STATE (2026-07-28, tip ad2805ca; orchestrator-read
##    anchors — the epoch's whole-program view AFTER slice 0, §20(AG).
##    SINGLE-PASS: the next session's fleet re-verifies THIS section +
##    §20(AD)-(AG) before D3.a.1 code. §1-§6 above are the PRE-slice-0
##    map stamped at 428dae76 — still the subsystem authority where
##    slice 0 did not touch it; anchors there may have drifted by the
##    slice-0 insertions.)

    THE PIPELINE AS IT STANDS (only the slice-0 deltas spelled out;
    everything else per §1-§6):

    demand pass (lib/DataFlow/Demand.cpp):
      stamp sites :996-998 / :1053-1056 now stamp the PAIR
        {guard_annotation_index, query} (INV-OWN3-Q: non-null iff
        annotated; the QueryImpl* back-pointer is mechanism (B))
      the ANNOTATION CENSUS (:1131-1160) is ALWAYS-ON, PINNED
        PRE-Optimize (dead-flow deletes annotated views only
        post-Optimize; never relocate without orphan accounting);
        equation keeps the folded term.
    the fold choke (lib/DataFlow/View.cpp):
      GuardAnnotationsCompatible :584 (forcing_index + instance_key —
        the Equals-invariant identity fields) carries THE LABELED
        RESIDUAL comment (:571-583): a BINDING D3.a.3 precondition to
        re-derive the predicate against real fold shapes (survivorship
        role policy; proxy-TUPLE invariance) — the two Fable findings
        pull opposite ways.
      CheckGuardAnnotationFold :609 (record-printing fprintf+abort,
        survives NDEBUG).
      the both-set arm :651-681: TWO always-on INV-OWN3-Q guards
        (loser null :658-662; survivor null-or-different :663-674) ->
        the fold check -> ++guard_annotation_folded_count :678 (the
        SOLE writer) -> paired clear :680-681.
    the mint (lib/Rel/Rel.cpp):
      inst_desc.differential = TableIsDifferential(pub_table) :1059 —
        THE ONE AUTHORITY (same `diff` local the InstantiateEffects
        fork reads); the kInstanceDeath gate :1139
        (TableIsDifferential(demand_table)) UNCHANGED and still FALSE
        on every accepted program.
    the lowering (lib/ControlFlow/Build/Procedure.cpp):
      V-INST-DIFF-COHERENCE :274-285 (always-on: stamped bit == live
        TableIsDifferential(pub); vacuous-green, liveness-by-
        perturbation owed at D3.a.1); the region ctor is THREE-ARG
        :296-298 (Hash/Equals untouched — the bit is a pure function
        of pub_table which Equals already keys on).
    the descriptor + emitter:
      ProgramInstanceStore.differential set at Stratum.cpp:2338;
      ProgramInstanceStoreInfo::IsDifferential(); the Database.cpp
      store-ctor emitter :1461-1464 appends ", false" ONLY when
      differential — generated text is byte-identical program-wide
      today (the bit is false everywhere).
    the store (include/drlojekyll/Runtime/InstanceStore.h) —
      UNTOUCHED by slice 0: RecycleCurrent :216 still has ZERO
      codegen callers; the HP-7 monotone belt :185 still armed
      everywhere (the selector exists, nothing selects false yet).
    tests: ctest is SIX units (tests/DataFlowValidators
      GuardAnnotationFoldTest — fork/waitpid, EINTR-safe).

    THE PATH FORWARD AS DIFFS ON THIS STATE (ruled order, OD-15):

    D3.a.1 — DIFFERENTIAL DEMAND (NEXT; opens at stage (b): the
      epoch stage-(a) substrate is §1-§6 + this section):
      d1 the RETRACT CHANNEL: the fabricated demand message gains
         @differential (lib/Parse/Demand.cpp:163-204) -> the
         injector's existing IsDifferential del_vec arm
         (Build.cpp:416-421) goes live -> the demand relation model
         becomes differential; batch SET netting per OQ-RETRACT
         (add∩remove annihilates — the OQ3 semantics).
      d2 FIRST RITUAL-HEAD QUESTION (the §20(AF) §3.3 L3 rider,
         FIRST-CLASS): reconcile the two diff axes — the store/region
         bit keys TableIsDifferential(pub) while the death mint keys
         TableIsDifferential(demand_table) (Rel.cpp:1139). Rule
         co-activation (a differential demand MAKES pub differential?)
         or switch the store predicate to the explicit disjunction.
      d3 DEATH GOES REACHABLE: the :1139 gate turns true ->
         G-DEATH-LOWER discharges: the death lowering band
         (RecycleCurrent wiring — its FIRST codegen caller; the (T,F)
         full retract of the dead key's frozen rows into pub's delete
         side), V-INST-EMITTED enrollment of {sid, kInstanceDeath},
         the demand kNetRemoval frontier PROVISIONED (G-DEMAND-NEG)
         + V-INST-DRAIN extended to check it.
      d4 THE (T,F) DROP SCAN in band-(b) (G-TF-PUBLISH) + the RAT-7
         PARTITION BELT (born+carried==cur.NumRows AND
         dropped+carried==frz.NumRows, always-on) — drop scan BEFORE
         born scan per touched iid; signed deltas into pub's own
         machinery (OQ-PUBLISH-ORDER).
      d5 THE SELECTOR GOES LIVE: monotone=false stores emit
         ", false"; HP-7 off for them; V-INST-FRESH unchanged (the
         pinned OQ-DEATH-VS-REBUILD three-way coupling:
         netting kills same-batch flap; TouchedFlag suppresses
         dead-key a2; Recycle leaves current empty).
      d6 WITNESS: the eqgate witness grows RETRACT batches
         (birth-rebuild-RETRACT-rebirth probes); flat -demand handles
         retraction via the ordinary differential machinery, so the
         eqgate flat==nested stays the live oracle; DEATH stays
         oracle-blind (the .batches oracle never sees it).
      d7 LIVENESS OWED: perturb V-INST-DIFF-COHERENCE + the OWN-3
         fold abort + the partition belt with a TRUE bit (the
         §20(AF) §3.6 obligation).
      (rider) the DS-R4-10 REJECT fence (@never over a
         CanReceiveDeletions negated view + directed witness, ruled
         OQ-NEVER) has no assigned slice — natural ride-along here.
    D3.a.2 — DIFFERENTIAL INPUT: lift FENCE (iii) (Build.cpp
      nested pre-pass) + the V-INST-SOLE differential-input
      forbiddance; the input net-REMOVALS frontier as a second a2
      trigger -> RecycleCurrent + full rescan (G-INPUT-NEG; G-STALE
      subsumed).
    D3.a.3 — MULTI-ADORNMENT: loop the pass per adornment;
      (query, BindingPattern) keying sweep; N disjoint stores
      (OQ-ADORN-KEY). PRECONDITION: the View.cpp:571-583 labeled
      fold-predicate re-derivation with directed witnesses.
    DEFERRED all-epoch: recursive demand (FENCE (i) stands; the
      §20(AB) NeedsInductionCycleVector precondition binds any
      future toucher).
