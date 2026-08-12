All five dumps are **GROUND TRUTH**, not prediction — I built the pre-cut compiler at `dc965d3c^` (commit `6d6248a2`) and ran the exact witness under `-demand -demand-instance` (nested) and `-demand` (flat) with all dump flags. These are the byte-exact golden targets the executor rebuilds toward.

---

# DESIRED IR OUTPUT STATES — keyed-monotone witness

**Provenance**: not predicted. Compiled the verbatim witness with the pre-cut `drlojekyll` (worktree at `dc965d3c^`), `-demand-instance` for the keyed dumps, `-demand` for the flat contrast. Every fenced block is copy-verbatim compiler output. Confidence: **HIGH (empirical)** on all five.

Table id legend (stable across all dumps): `%table:4` = **pub** `neighborhood`/`edge` model (From,To); `%table:8` = **demand** `demand__neighborhood_bf/1` (one col); `%table:11` = **input** `edge`/`add_edge` model (From,To).

---

## 1. `.df` — dataflow after ApplyDemandTransform — **UNCHANGED by keyed lowering**

Verified: `diff out_flat/w.df out_nested/w.df` → **IDENTICAL**. The recognition front-end (`ApplyDemandTransform` + `Optimize`) runs before any keyed decision; `-demand-instance` changes nothing in the Query graph. This is the demand-shaped graph — the guard JOIN + fabricated `demand__` message + demand relation all present, exactly as flat.

```
dataflow

select ^select.0 (From:u64, To:u64)                ; recv #message add_edge/2
  ATTRIBUTES eqset=1 class=table-less stratum=0 tag=build/predicate-select
  => ^tuple.2 (From, To)

select ^select.1 (c3:u64)                          ; recv #message demand__neighborhood_bf/1
  ATTRIBUTES eqset=2 class=table-less stratum=1 tag=demand/seed-receive
  => ^tuple.3 (c6=c3)

tuple ^tuple.2 (From:u64, To:u64)
  ATTRIBUTES table=%table:11 eqset=1 class=monotone stratum=2 tag=build/clause-head
  => ^join.5 .in1(From, To)

tuple ^tuple.3 (c6:u64)
  ATTRIBUTES table=%table:8 eqset=2 class=monotone stratum=3 tag=demand/raw-seed
  => ^join.5 .in0(c6)

tuple ^tuple.4 (From:u64, To:u64)
  ATTRIBUTES eqset=5 class=table-less stratum=5 tag=link/insert-proxy
  => ^insert.6 (From, To)

join ^join.5 (From:u64, To:u64) {
  pivot From:u64 <- .in0.c6, .in1.From
  out To:u64 <- .in1.To
}
  ATTRIBUTES eqset=5 class=table-less stratum=4 tag=demand/guard-join
  => ^tuple.4 (From, To)

insert ^insert.6 (From:u64, To:u64) into %table:4
  ATTRIBUTES eqset=5 class=monotone stratum=6 tag=connect/query-insert
```

**Proving tokens (recognition, not keyed):** `tag=demand/guard-join` on `^join.5` (pivot `From <- .in0.c6, .in1.From`), `tag=demand/seed-receive` / `tag=demand/raw-seed` on the `demand__neighborhood_bf/1` path, `tag=link/insert-proxy` → `insert … into %table:4`. The keyed-vs-flat fork happens strictly downstream of this graph.

---

## 2. `.rel` — THE keyed op appears (nested `-demand-instance`)

```
rel

instances:
  DRInstance i#0 forcing=neighborhood key=%table:8 pub=%table:4 input=%table:11 store=I#0 key_cols=[From] row_cols=[To]

op.7 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:8
op.5 kIngestFold sign=+ ctx=eager stratum=0
    effects: {kCounter(%table:8, +, NonRecursive), kVecAppend(%table:8, kNetAddition)}
    spine: —
    args: table=%table:8 message=demand__neighborhood_bf/1
op.6 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:11
op.4 kIngestFold sign=+ ctx=eager stratum=0
    effects: {kCounter(%table:11, +, NonRecursive), kVecAppend(%table:11, kNetAddition)}
    spine: —
    args: table=%table:11 message=add_edge/2
op.0 kSubgraphInstantiate sign=+ ctx=seed stratum=1 i#0
    demand=%table:8 pub=%table:4 input=%table:11 pub_row=[ik:From,row:To] nested=<To>
    reads: Present(%table:11)
    effects: {kVecDrain(%table:8, kNetAddition), kVecDrain(%table:11, kNetAddition), kInstanceDemand(%table:8), kInstanceRebuild(%table:4, +), kStateEmit(%table:4), kStateOld(%table:4), kCounter(%table:4, +, NonRecursive)}
    spine: kAccess(%table:11, section-walk) -> kFold(%table:4, +, NonRecursive)
    args: demand=%table:8 pub=%table:4 input=%table:11 store=I#0
op.2 kCommitSweep sign=· ctx=seed band=9 flavor=monotone publish_target=false
    effects: {kFlagWrite(%table:8)}
    args: table=%table:8
op.3 kCommitSweep sign=· ctx=seed band=9 flavor=monotone publish_target=false
    effects: {kFlagWrite(%table:11)}
    args: table=%table:11
op.1 kInstanceSeal sign=· ctx=seed band=11 i#0
    effects: {kStateFold(%table:4, sign=0)}
    args: pub=%table:4 store=I#0

rounds:

deps:
  op.0 -> op.1 WAR epoch
  op.0 -> op.1 WAW epoch
  op.0 -> op.3 WAR epoch
  op.4 -> op.0 RAW epoch
  op.4 -> op.3 WAW epoch
  op.5 -> op.2 WAW epoch

census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=2 kNegateGate=0 kPivotAssemble=0 kIngestFold=2 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=1 kInstanceDeath=0 kInstanceSeal=1 kEagerForward=2 kEagerInsert=0 kEagerCompare=0 kEagerGenerate=0 kEagerUnion=0 kEagerSelect=0 kEagerJoin=0 kEagerProduct=0 kIngestLoop=0 kJoinEmit=0 kProductEmit=0
```

**Census delta flat → nested** (the byte proof the guard-web collapsed into a keyed store):

| kind | flat | nested | Δ |
|---|---|---|---|
| `kSubgraphInstantiate` | 0 | **1** | **+1** |
| `kInstanceSeal` | 0 | **1** | **+1** |
| `kInstanceDeath` | 0 | 0 | 0 (monotone — slice-1 invariant holds) |
| `kCommitSweep` | 0 | **2** | +2 (demand `%table:8` + input `%table:11` seal-committed for the section-walk rescan) |
| `kEagerForward` | 3 | 2 | −1 |
| `kEagerInsert` | 1 | 0 | −1 (pub insert absorbed into `kInstanceRebuild`) |
| `kEagerJoin` | 2 | 0 | −2 (guard join gone) |
| `kJoinEmit` | 1 | 0 | −1 |
| `kIngestFold` | 2 | 2 | 0 |

**Proving tokens:** the `instances:` section with `DRInstance i#0 … key_cols=[From] row_cols=[To]`; the `op.0 kSubgraphInstantiate … pub_row=[ik:From,row:To] nested=<To>`; `op.1 kInstanceSeal … band=11`; census `kSubgraphInstantiate=1 kInstanceDeath=0 kInstanceSeal=1`. The **effect multiset on `op.0` matches the design's `InstantiateEffects_MONO` verbatim** (`kVecDrain×2 kInstanceDemand kInstanceRebuild(+) kStateEmit kStateOld kCounter(+,NonRecursive)`); the spine matches the single `DRArm` (`kAccess(input, section-walk) -> kFold(pub, +)`); `kInstanceSeal` carries the single `kStateFold(%table:4, sign=0)` = design's `SealEffect`.

> **Two notes for the executor** (both differ from / extend the design §1–§2f):
> 1. `kFlagRead(input, Present, kSeed)` renders on the **`reads:` line** (`reads: Present(%table:11)`), **not** in the `effects:` set. V-INST-EFFECT's "leaf==1 kFlagRead Present" obligation must count the **reads channel**, not the effects multiset (design table §2f lists it under effects — it is a *read*).
> 2. The two `kCommitSweep` ops (band=9, `flavor=monotone`, `publish_target=false`, `kFlagWrite` on demand+input) are **not** enumerated in the design's §1 STAGE-2 pseudocode but are required: the section-walk rescan (`kAccess … section-walk`) reads `%table:11` by `NumRows()/RowAt()`, so demand+input tables get committed+sealed. Flat needs zero (`kCommitSweep=0`) — the join reads via live index. This is real emergent structure, not optional.

---

## 3. `.ir` — ControlFlow: the `subgraph-instance` region replaces the guard-web join

Nested `flow_42` (the whole guard-web `join-tables` collapses to one `subgraph-instance` line):

```
proc ^flow:42($net_additions:21<u64,u64>, $net_additions:25<u64>)
  seq
    @16 += 1
    subgraph-instance i#0 demand $net_additions:25<u64> input $net_additions:21<u64,u64> rescan %table:11[u64,u64] -> publish %table:4[u64,u64] key@{0} row@{1} seal
    commit-sweep %table:8[u64]
    commit-sweep %table:11[u64,u64]
    return-true
```

Table/index preamble (nested) — note the **partial-key index `%index:41 on %col:5`** the bound query seeks:

```
create %table:4[u64,u64]
  u64	%col:5	; From
  u64	%col:6	; To
  %index:7[u64,u64] on %col:5, %col:6
  %index:41[u64,_] on %col:5
create %table:8[u64]
  u64	%col:9
  %index:10[u64] on %col:9
create %table:11[u64,u64]
  u64	%col:12	; From
  u64	%col:13	; To
  %index:14[u64,u64] on %col:12, %col:13
```

**Flat contrast — what the `subgraph-instance` line REPLACES** (the guard-web `join-tables` scan of edge×demand into pub):

```
proc ^flow:47($pivots:21<u64>)
  seq
    @16 += 1
    vector-unique $pivots:21<u64>
    join-tables
      vector-loop {@From:26} over $pivots:21<u64>
      select {%col:9 as @28} from %table:8[u64] using %index:10[u64] where %col:9 = @From:26
      select {%col:12 as @From:29, %col:13 as @To:30} from %table:11[u64,u64] using %index:27[u64,_] where %col:12 = @From:26
        update-count +nonrecursive {@From:29, @To:30} in %table:4[u64,u64]
    vector-clear $pivots:21<u64>
    return-true
```

**Proving tokens:** the `subgraph-instance i#0 demand $…<u64> input $…<u64,u64> rescan %table:11 -> publish %table:4 key@{0} row@{1} seal` region — **there is no `join-tables` in the nested flow**, and the flat `select … from %table:8 … select … from %table:11 … update-count … in %table:4` guard-web is entirely gone. `key@{0} row@{1}` = the `key_cols=[From] row_cols=[To]` partition. Both keep the demand plumbing (`commit-sweep %table:8` / `%table:11`); the flat one instead consumes `$pivots:21` directly in the join.

---

## 4. `.h` — generated C++: `InstanceStore` member replaces the guard tables in the flow

Confirmed: flat `datalog.h` has **0** occurrences of `InstanceStore`; nested has the member and full band-a1/a2/b/seal emission.

Header include (gated on non-empty stores) + per-store hash structs:

```cpp
#include <drlojekyll/Runtime/InstanceStore.h>
// ...
// InstanceStore #0 key (the demanded alpha).
struct Key_0 {
  uint64_t c0;
  uint64_t Hash(void) const noexcept { return ::hyde::rt::HashRow(c0); }
  bool operator==(const Key_0 &) const noexcept = default;
};
// InstanceStore #0 published row.
struct Row_0 {
  uint64_t c0;
  uint64_t Hash(void) const noexcept { return ::hyde::rt::HashRow(c0); }
  bool operator==(const Row_0 &) const noexcept = default;
};
```

Database member (ctor'd with allocator, threaded into every touching proc):

```cpp
  ::hyde::rt::InstanceStore<Key_0, Row_0> instance_0;   // Database member
  // ctor: ... table_11(allocator_), instance_0(allocator_) {}
```

`flow_42` body — band-a1 (demand-key births), band-a2 (edge-after-demand rebuild), band-b (frozen-vs-current publish diff into pub table), seal:

```cpp
inline bool flow_42(..., ::hyde::rt::InstanceStore<Key_0, Row_0> &instance_0,
                    ::hyde::rt::Vec<Tup_u64_u64> vec21, ::hyde::rt::Vec<Tup_u64> vec25) {
  g16 += 1;
  for (const auto &[k0] : vec25) {                                  // band-a1: demand net-adds
    const auto iid = instance_0.FindOrAddInstance(Key_0{k0});
    if (!instance_0.TouchedFlag(iid)) {
      if (instance_0.WorkingOccupied(iid)) { /* V-INST-FRESH abort */ }
      auto &cur = instance_0.TouchCurrent(iid);
      for (uint32_t s = 0; s < table_11.NumRows(); ++s) {
        const auto ir = table_11.RowAt(s);
        if (ir.from == k0) { cur.TryAdd(Row_0{ir.to}); }             // NO .Present() conjunct (mono)
      }
    }
  }
  for (const auto &[e0, e1] : vec21) {                              // band-a2: edge net-adds
    const auto iid = instance_0.FindInstance(Key_0{e0});
    if (iid != ::hyde::rt::kNoInstance && !instance_0.TouchedFlag(iid)) {
      if (instance_0.WorkingOccupied(iid)) { /* V-INST-FRESH abort */ }
      auto &cur = instance_0.TouchCurrent(iid);
      for (uint32_t s = 0; s < table_11.NumRows(); ++s) {
        const auto ir = table_11.RowAt(s);
        if (ir.from == e0) { cur.TryAdd(Row_0{ir.to}); }
      }
    }
  }
  for (const auto iid : instance_0.Touched()) {                     // band-b: publish diff
    auto &cur = instance_0.Current(iid);
    const auto &frz = instance_0.Frozen(iid);
    const auto &key = instance_0.KeyAt(iid); (void) key;
    for (uint32_t r = 0; r < cur.NumRows(); ++r) {
      const auto &row = cur.RowAt(r);
      if (frz.Find(row) == ::hyde::rt::kNoRow) {
        if (const auto ins2 = neighborhood_4.TryAdd({key.c0, row.c0}); ins2.added) {
          idx_41.Add({key.c0}, ins2.id);                            // EmitIndexAdds (partial idx)
        }
      }
    }
  }
  instance_0.Seal();
#ifndef NDEBUG
  instance_0.DebugValidate();
#endif
  table_8.Seal();
  table_11.Seal();
  // ...
}
```

The bound-query cursor — **reads the pub table `neighborhood_4` via the partial-key index `idx_41`, injecting demand first** (it does NOT read the store directly):

```cpp
  struct neighborhood_bf_cursor {
    Database &db; uint64_t Start; uint32_t pos;
    bool next(uint64_t &Node) {
      while (pos != ::hyde::rt::kNoRow) {
        const uint32_t id = pos;
        pos = db.idx_41.Next(id);
        const auto row = db.neighborhood_4.RowAt(id);
        Node = row.to;
        return true;
      }
      return false;
    }
  };
  template <typename Log, typename Functors>
  friend neighborhood_bf_cursor neighborhood_bf(Database &db, Log &, Functors &, uint64_t Start) {
    assert(db.initialized_);
    inject_37(..., db.instance_0, Start);                            // seeds demand__ → flow → store → pub
    return {db, Start, db.idx_41.First({Start})};                   // P7 partial-key hash seek on pub
  }
```

**Proving tokens:** `#include <drlojekyll/Runtime/InstanceStore.h>`; `::hyde::rt::InstanceStore<Key_0, Row_0> instance_0;` (flat has none); `FindOrAddInstance`/`TouchCurrent`/`TryAdd`/`Frozen(iid).Find`/`Seal()`/`DebugValidate()`; the store threaded through **every** proc signature (`init_3`, `proc_15`, `add_edge_2_detail`, `flow_42`, `inject_37`).

> **Design correction (item 4 framing):** the design says "the neighborhood query cursor reading the keyed store." **It does not.** The cursor reads `neighborhood_4` (pub `%table:4`) through `idx_41` (the `%index:41[u64,_] on col5=From` partial-key seek — this is P7). The `InstanceStore` is the *intermediate* that band-b materializes into the pub table during `flow_42`; the cursor is a plain pub-table partial-key hash seek. The store is upstream of the cursor, not the cursor's source.

---

## 5. `.region` — frozen regional dump — **UNCHANGED vs flat**

Verified: `diff out_flat/w.region out_nested/w.region` → **IDENTICAL**. `FrozenRegionalProgram::Build` runs at Main's third slot on the Query graph; it has no keyed code (design STAGE 1: "No keyed code here"), so `-demand-instance` leaves it byte-identical.

```
region-program
program-root {
  input-abi   add_edge/2(From:u64, To:u64)                  -> R0 via P1
  query-abi   neighborhood(Start:bound u64, Node:free u64)  -> R0 via P0
  output-abi  <none>
}
region R0  owner=program-root  parents=()  children=() {
  request-port     P0  query=neighborhood  fields=(Start)
  input-port       P1  message=add_edge/2  fields=(From, To)
  region-internal  demand__neighborhood_bf/1(p0:u64)  [fabricated, driver-suppressed]
  row-contract     E0  rel=neighborhood  member-key=(Start, Node)  support=monotone
  row-contract     E1  rel=edge          member-key=(From, To)     support=monotone
}
census: regions=1 child-calls=0 program-roots=1 request-ports=1 input-ports=1 result-ports=0 row-contracts=2
```

**Proving tokens:** `request-port P0 query=neighborhood fields=(Start)` (bound key), `region-internal demand__neighborhood_bf/1 … [fabricated, driver-suppressed]`, `row-contracts=2` (E0 neighborhood, E1 edge, both `support=monotone`). Nothing here distinguishes flat from nested — flag for the executor: **do not expect a keyed token in `.region`.**

---

## CONSISTENCY CHECK (census/validator contracts from the extracts)

All satisfied by the ground-truth dumps:

- **Census recount** (`kSubgraphInstantiate=1`, `kInstanceSeal=1`, `kInstanceDeath=0`): one `RecognizedSubgraph` (forcing `neighborhood`) → exactly one instantiate + one seal, zero death. ✔ Design's `exp_instance` = 1.
- **V-INST-EFFECT** (mono multiset): `op.0` effects == design's `InstantiateEffects_MONO` byte-for-byte (`kVecDrain(demand,+add), kVecDrain(input,+add), kInstanceDemand(demand), kInstanceRebuild(pub,+), kStateEmit(pub), kStateOld(pub), kCounter(pub,+,NonRecursive)`); no append/crossing/second counter. `op.1` seal == single `kStateFold(pub, sign=0)`. ✔ **Caveat**: the `kFlagRead(input,Present)` obligation is met via the **`reads:` line**, not effects.
- **V-INST-SOLE**: input `%table:11` ≠ pub `%table:4`, and monotone (slice-1 invariant `TableIsDifferential(pub)==false` — census `kInstanceDeath=0` confirms no differential arm minted); exactly one `kSubgraphInstantiate` on pub `%table:4`. ✔
- **V-INST-PAIR (mono)**: ops grouped by `i#0` = `{kSubgraphInstantiate, kInstanceSeal}` exactly, no death. ✔
- **V-INST-DRAIN**: `op.0` drains `%table:8` net-additions (`kVecDrain(%table:8, kNetAddition)`), and `op.5` produces it (`kVecAppend(%table:8, kNetAddition)`) — non-null birth vector. ✔
- **`DROpStratum` keyed arm**: `op.0 stratum=1`, `op.1 band=11` (seal → trailing). The instantiate sits at stratum 1 above the base-0 ingest folds — **confirms design Risk R1 is benign for this witness** (demand/input drains at stratum 0, `instance_stratum[0]=1`). ✔
- **Linearize/deps**: `op.4 -> op.0 RAW` (input fold feeds the rescan), `op.0 -> op.1 WAW/WAR` (instantiate before seal), `op.0 -> op.3 WAR` — a valid acyclic ordering; seal (`op.1`) last. ✔
- **`.df`/`.region` invariance**: both byte-identical flat↔nested — matches design STAGE-1 claim and item-1/item-5 predictions. ✔

**Nothing marked LOW-CONFIDENCE** — every token is verbatim compiler output. The two executor-facing deltas from the design text (the `reads:`-channel `kFlagRead`, the two required `kCommitSweep` ops, and the cursor reading the **pub table** not the store) are corrections grounded in the actual dumps, not invented tokens.

Scratchpad artifacts (absolute paths) for the executor to diff against: `/private/tmp/claude-502/-Users-pag-Code-DrLojekyll/0556d194-fc78-4a6f-aca1-361fc945ead0/scratchpad/out_nested/{w.df,w.rel,w.ir,w.region}`, `/private/tmp/claude-502/-Users-pag-Code-DrLojekyll/0556d194-fc78-4a6f-aca1-361fc945ead0/scratchpad/cpp_nested/datalog.h`, and the flat twins under `out_flat/`. Deleted rendering source: `git show dc965d3c^:lib/Rel/Format.cpp` (instance render lines 648-901), `git show dc965d3c^:include/drlojekyll/Runtime/InstanceStore.h` (344 lines).