# Keyed-instance rewrite — deepened current-architecture pseudocode (companion to the seed)

Session 12 (2026-08-06, branch `keyed-instances`, tip `46a404d4` + the uncommitted
Phase-0 working tree). This is the implementer-grain deepening of
`keyed-rewrite-pseudocode-seed.md` §1 ("what we cut FROM"). Every `file:line`
below was re-derived against the current tip by a per-subsystem reader pass and
cross-verified by the orchestrator on the four load-bearing sites (demand
bijection, flat/nested selector, `kSectionWalk`, InstanceStore leaf). The seed's
§2 (target model) and §3 (evaluation contract) live in the seed itself, now
rendered as operational pseudocode.

## §1.0 Re-verified anchor table (supersedes seed §5)

All anchors verified TRUE at tip unless the note says otherwise. Line drift from
the seed is noted; the seed's ±0/±4/±9/±11 offsets are the ordinary churn pattern.

| Concern | Anchor (verified @ tip 46a404d4 + Phase-0 wt) | Seed said | Note |
| --- | --- | --- | --- |
| `@key` routed via ParseLocalExport | `lib/Parse/Parser.cpp:357` | ~357 | state-machine entry; key locals @398-404 |
| `@key` pragma parse (states 8→21→22→8) | `lib/Parse/Parser.cpp:792-1014` | ~793 | dup/unknown/wildcard @909-952; close @965-1013 |
| same-decl order-free dup-set reject (ADJ-K1-A) | `lib/Parse/Parser.cpp:974-995` | — | sorts each set before compare |
| full-context redecl consistency (Phase-0) | `lib/Parse/Parser.cpp:1665-1698` | ~1656 | canon_decl = first key-bearing prior; comment @1651 |
| `SameKeySetOfSets` — order-collapsing equality | `lib/Parse/Parser.cpp:1477-1488` | (alluded §6) | sorts intra-key AND std::set inter-key |
| reject `@key` on `#query` (Phase-0) | `lib/Parse/Query.cpp:222-234` (state 6 @181) | state 6 | direct witness; RemoveDecl+return |
| `InstanceKey`/`InstanceKeySet` aliases | `include/drlojekyll/Parse/Parse.h:31-32` | — | `unsigned` / `vector<InstanceKey>` |
| `HasInstanceKey()`/`InstanceKeys()` decl | `include/drlojekyll/Parse/Parse.h:456-457` | — | + `InstanceKeyRanges()` |
| storage `instance_key_param_index_sets` | `lib/Parse/Parse.h:387,392` | — | parallel `instance_key_ranges` |
| accessor cross-redecl resolution (F-K6-SHADOW) | `lib/Parse/Parse.cpp:858-888` | — | first non-empty sibling wins |
| Query::Build stage order | `lib/DataFlow/Build.cpp:2524` | 2524 | ✓ |
| ConnectInsertsToSelects / proxy map | `lib/DataFlow/Connect.cpp:164` | (implied) | Build-scoped `proxy_view_to_decl` |
| ApplyDemandTransform head + activation gate | `lib/DataFlow/Demand.cpp:388` (gate @411-430) | 388/401 | pragma_activated @429 |
| no-bound-query / pragma-but-no-seed reject | `lib/DataFlow/Demand.cpp:473-487` | 473 | ✓ |
| Tier-1 decl resolve + RP-6 realization + V-DECLARED-KEY | `lib/DataFlow/Demand.cpp:858` (realize @873-882; bijection @892-959; sort @905) | 868/892/905 | comment-vs-code offsets only |
| Phase-1 per-adornment loop | `lib/DataFlow/Demand.cpp:531` | (implied) | over `UniqueRedeclarations` |
| Step-4 stray-consumer union (once) | `lib/DataFlow/Demand.cpp:974` | "once between loops" | over union of adornments' consumers |
| Phase-2 mint loop | `lib/DataFlow/Demand.cpp:1014` | (implied) | fabricate/mint/guard/register |
| R-DUP grouped rewire | `lib/DataFlow/Demand.cpp:1374` | (implied) | per (consumer,read) group, not global |
| FabricateDemandMessage / Local | `lib/Parse/Demand.cpp:170,226` | (named) | `demand__` mint |
| Stratify call site | `lib/DataFlow/Build.cpp:2647` | 2524-2665 | +9 churn from R.1.4 |
| InferConservativeRowContracts call | `lib/DataFlow/Build.cpp:2655` | 2665 | +9 churn |
| ValidateRowContracts guard | `lib/DataFlow/Build.cpp:2656-2658` | — | H-A7 belts |
| RowContract struct / map | `lib/DataFlow/RowContract.h:38,51` | — | `{visible_fields, member_key}` |
| InferConservativeRowContracts entry | `lib/DataFlow/RowContract.cpp:365` | — | two-phase, PURE |
| ValidateRowContracts driver + 4 belts | `lib/DataFlow/RowContract.cpp:413` | — | census/memberkey/agg-input/no-collapse |
| Stratify (Tarjan + rejects + V-SCC-SEAM) | `lib/DataFlow/Stratify.cpp:124` | — | V-SCC-SEAM @372-381 |
| SemanticMemberKey | `lib/DataFlow/Identity.h:52` | (named) | |
| ProjectionRole enum {kMember,kDistinct} | `lib/DataFlow/Query.h:729` | 718-731 | +11 churn; Equals-only fold @Tuple.cpp:308 |
| ResolveLiveRecognition | `lib/Rel/Rel.cpp:936-1025` | 936-1059 | 1026-1059 is the NEXT fn's comment |
| BuildSubgraphInstanceOps (gated) | `lib/Rel/Rel.cpp:1038-1200` | 1038-1179 | 1179 lands mid-Seal-mint |
| mint enumerator name | `kSubgraphInstantiate` (`Rel.h:148`) | `kInstanceInstantiate` | **seed name wrong** |
| HP-4 refuse MAP/NEGATE/AGG/KVIndex input | `lib/Rel/Rel.cpp:1064` | (named) | |
| `kSectionWalk` label on Rederive ACCESS | `lib/Rel/Rel.cpp:1146` | 1137-1158 | no inline "lie" comment here |
| codegen full-scan mold (the dishonesty) | `lib/CodeGen/CPlusPlus/Database.cpp:2434-2494` | 2434-2493 | ✓ byte-exact |
| in-code honesty acknowledgment | `lib/CodeGen/CPlusPlus/Database.cpp:2339-2341` | — | "a full scan with a key filter — deferred perf refinement" |
| band drivers a0/a1/a2·a2'/b | `Database.cpp:2511 / 2532 / 2577 / 2661` | (named) | a2/a2' share `emit_edge_drain` |
| Program::Build unwrap `query=frozen.Query()` | `lib/ControlFlow/Build/Build.cpp:1337` | 1333-1338 | signature @1333 |
| feature-gap pre-pass | `lib/ControlFlow/Build/Build.cpp:1345-1437` | (one line) | agg/kv/map/product rejects, `continue` not return |
| keyed-instance fences per forcing | `lib/ControlFlow/Build/Build.cpp:1439-1519` | 1439-1549 | grouping @1466; predicate @1476 |
| all_forcings_admissible + strict reject | `lib/ControlFlow/Build/Build.cpp:1505-1517` | — | FENCE (ii)/(iii) retired inline |
| RP-9 silent pragma→nested selection | `lib/ControlFlow/Build/Build.cpp:1532-1549` | 1439-1549 | `effective_demand_instance` |
| `context.demand_instance_enabled` set | `lib/ControlFlow/Build/Build.cpp:1568` | — | the one selector bit |
| GuardAnnotation struct | `include/drlojekyll/DataFlow/Query.h:1007` | 1007-1058 | kind/demand_side/role/forcing_index/instance_key |
| RecognizedSubgraph struct | `include/drlojekyll/DataFlow/Query.h:1045` | 1007-1058 | demanded_decl |
| BuildSubgraphInstanceOps call (gated) | `lib/Rel/Rel.cpp:2067` | — | `if demand_instance_enabled` |
| V-REGION-CENSUS recount | `lib/Rel/Rel.cpp:4634-4662` | (named) | DeriveRegionalCensus fresh vs stored |
| InstanceStore leaf | `include/drlojekyll/Runtime/InstanceStore.h:54-219` | 54-218 | no TryClaim; pure Touch/Seal/Recycle |
| FrozenRegionalProgram::Build (degenerate) | `lib/Regional/Planning.cpp:439` | (named) | H2 planner |
| CollectDemandInteriorDecls (Tier-1, demand) | `lib/Regional/Planning.cpp:201` | 201-285 | reads RecognizedSubgraph |
| ResolveInteriorSupport (Tier-1 support) | `lib/Regional/Planning.cpp:249` | 201-285 | reads GuardAnnotation+RecognizedSubgraph+live |
| CollectOriginInteriorDecls (Tier-2, non-demand) | `lib/Regional/Planning.cpp:298` | (named) | OriginDecls only |
| DeriveRegionalCensus (single authority) | `lib/Regional/Planning.cpp:384` | (named) | |
| friend leak `query.impl->row_contracts` | `lib/Regional/Planning.cpp:574` | — | the one DataFlow-private reach |
| V-FROZEN-NO-OPEN-PORT / V-OWNERSHIP-ACYCLIC | `lib/Regional/Planning.cpp:692,711` | (named) | |
| pipeline call site | `bin/drlojekyll/Main.cpp:92` | (implied) | FrozenRegionalProgram::Build slot |

## §1.1 Drift-correction ledger (what the seed §1 got wrong or under-said)

1. **Mint op name.** Seed `kInstanceInstantiate` → real `kSubgraphInstantiate`
   (`Rel.h:148`). `kInstanceDeath`/`kInstanceSeal` correct.
2. **The `kSectionWalk` "lie" is acknowledged in-code.** There is no dishonesty
   comment at the Rel mint (`Rel.cpp:1146` just sets the enum). The honest
   admission is at codegen `Database.cpp:2339-2341`: *"a full scan with a key
   filter — the keyed index is a deferred perf refinement; the DR spine already
   tags section-walk."* So the label is a KNOWN deferred-perf placeholder, not a
   hidden bug. The critique must frame Phase 4 as *promoting a known-placeholder
   label to an honest `FullScanFilter`*, not *exposing a concealed scan*.
3. **`SameKeySetOfSets` is duplicated, not shared.** Order-collapse lives at
   BOTH `Parser.cpp:1477-1488` and `Demand.cpp:902-919` (independent copies).
   Phase-0 item 4 (order-significant paths) must change BOTH, and the Demand copy
   only dies with the Phase-1 cut — so item 4 genuinely blocks on Phase 1 for the
   Demand half but NOT for the Parser half (a parser-local ordered-path identity
   can land immediately if the Demand bijection is left order-blind until P1).
4. **FrozenRegionalProgram holds FIVE render vectors**, not one:
   RegionalAbi/Port/Internal/PermanentRoot/Contract. Four of the five are the
   exact machinery that reaches into demand side-tables.
5. **Planning.cpp demand coupling is two functions + one direct read**, not one
   range: `CollectDemandInteriorDecls` (201, reads RecognizedSubgraph),
   `ResolveInteriorSupport` (249, reads GuardAnnotation+RecognizedSubgraph+live),
   and the request-port/ABI-routing loops read `DemandForcings()` directly. Tier-2
   (`CollectOriginInteriorDecls`) is the ONE non-demand naming path but still
   dedups against Tier-1's demand-derived set.
6. **The flat/nested grouping is view-identity-driven, not annotation-vector
   iteration.** `Build.cpp:1466` walks `query.ForEachView`, buckets views with a
   live `GuardAnnotationIndex()` by `forcing_index` — ABA-safe against CSE.
7. **Two fences already retired.** FENCE (ii) mid-stream monotone edge-add and
   FENCE (iii) differential summarized-input NO LONGER fire (band-(a2)/(a2')
   handle them). Only cyclic-demand and recursive-content remain.
8. **R-DUP is per-group, not a whole-transform branch.** Demand groups pending
   rewires by `(consumer, read)`; a singleton group rewires directly
   (byte-identical to pre-D3.a.3), a multi-guard group mints a MERGE union.
9. **InstanceStore has no claim gates.** Pure Touch/Seal/RecycleCurrent; occupancy
   = `current[iid].NumRows()>0` (no signed counter, unlike StateCellStore).
10. **`WorkingOccupied`/occupancy is exact only because rescan rebuilds from
    empty.** Band-(a) never incrementally shrinks `current` — every source (a1
    birth, a2 edge-add, a2' edge-remove) calls the ONE full-rescan mold.

---

# §1 (deepened) — per-subsystem current-algorithm pseudocode

(Reader-extracted, orchestrator-verified. Anchors above.)


### Runtime InstanceStore (include/drlojekyll/Runtime/InstanceStore.h)

```
// ---- Layout ----
template<Key, RowT>
class InstanceStore:
  # identity/config
  allocator            : Allocator
  monotone : bool       # true=R-MONO (HP-7 belt active), false=R-DIFF/death-unit (belt off)

  # dense iid namespace — APPEND-ONLY, MONOTONE-FOREVER (an iid, once minted, is
  # retained for the program's life; iids are a namespace separate from any
  # pub-table row id — a pub-table compaction never touches this store)
  keys[iid]            : Key            # complete α-bound key tuple, one full row of bound cols
  hashes[iid]           : uint64_t       # cached key.Hash()

  # the double-buffered nested relation, ONE PER iid — this is the whole point:
  # each instance owns a full monotone Table<RowT> pair, not a scalar/word cell
  frozen[iid]           : Table<RowT>*   # last epoch's sealed content (old())
  current[iid]          : Table<RowT>*   # this epoch's rebuilt content (rescanned from scratch)
  sealed_occupied[iid]   : uint8_t        # batch-start occupancy bit (frozen->NumRows()>0 as of last Seal)

  # per-epoch touched-set bookkeeping (append-once dedup, mirrors StateCell Touch)
  touched               : Vec<InstanceId>
  touched_flag[iid]      : uint8_t

  # open-addressing key->iid index (linear probe, power-of-2 capacity)
  slots[]                : InstanceId[]  # kNoInstance sentinel = empty slot
  slot_capacity           : size_t

// ---- Lookup / mint ----
FindInstance(key) -> iid:
    return FindInstanceWithHash(key, key.Hash())

FindInstanceWithHash(key, hash) -> iid:
    if slot_capacity == 0: return kNoInstance
    for i = hash & (slot_capacity-1), i = (i+1) & (slot_capacity-1), ...:
        probed = slots[i]
        if probed == kNoInstance: return kNoInstance      # open-addressing miss = true miss
        if hashes[probed] == hash and keys[probed] == key: return probed

FindOrAddInstance(key) -> iid:
    hash = key.Hash()
    if (iid = FindInstanceWithHash(key, hash)) != kNoInstance: return iid
    iid = NumInstances()                                   # next dense id
    abort if iid == kNoInstance                             # id-space exhaustion (~0u sentinel)
    keys.Add(key); hashes.Add(hash)
    frozen.Add(MakeTable())    # fresh empty table, never occupied (V-INST-FRESH precondition)
    current.Add(MakeTable())   # fresh empty working buffer
    sealed_occupied.Add(0); touched_flag.Add(0)
    InsertSlot(iid, hash)                                   # may trigger Rehash
    return iid

InsertSlot(iid, hash):
    if (NumInstances() + NumInstances()>>3) >= slot_capacity:  # ~87.5% load factor
        Rehash(); return                                       # note: iid NOT re-inserted here —
                                                                 # Rehash() rebuilds slots for ALL
                                                                 # existing iids including this one
                                                                 # (it scans keys[]/hashes[] 0..n)
    linear-probe from hash & (cap-1); write iid into first kNoInstance slot

Rehash():
    new_capacity = slot_capacity ? slot_capacity*2 : 64
    allocate new_slots[new_capacity], fill with kNoInstance
    free old slots array
    for iid in [0, NumInstances()): reinsert via linear probe on hashes[iid]

// ---- Per-epoch touch / rescan entry point ----
Touch(iid):                                     # private; append-once per epoch
    if !touched_flag[iid]:
        touched_flag[iid] = 1; touched.Add(iid)

TouchCurrent(iid) -> Table&:
    # THE band-(a) entry point: codegen calls this once per touched iid, then
    # (per the emit_instance_rescan mold in Database.cpp) asserts
    # !WorkingOccupied(iid) [V-INST-FRESH] and TryAdd-loops a FULL SCAN of the
    # summarized input table, filtering rows whose key columns == this key.
    Touch(iid); return *current[iid]

Current(iid) -> Table&           # mutable, band-(b) publish scan
Frozen(iid) -> const Table&      # read-only until next Seal

WorkingOccupied(iid) -> bool:
    return current[iid].NumRows() > 0     # occupancy = non-empty current buffer;
                                            # NO separate signed counter (unlike
                                            # StateCellStore.working_count) — dropped
                                            # under OQ-MODEL because band-(a)/(a2') never
                                            # incrementally shrinks `current` mid-epoch,
                                            # only rebuilds it from empty via full rescan

SealedOccupied(iid) -> bool: return sealed_occupied[iid] != 0

RecycleCurrent(iid):
    # R-DIFF death arm's belt + same-epoch demand-flap rebuild. Unconditional +
    # idempotent (Reset on empty table / Touch append-once => calling twice in
    # one epoch == once).
    Touch(iid); current[iid].Reset()

// ---- End of epoch ----
Touched() -> Vec<InstanceId>:                 # band-(b) driver
    touched.SortAndUnique(); return touched

Seal():
    for iid in touched:
        #ifndef NDEBUG
        if monotone:                            # HP-7 belt, R-MONO only
            for each row r in frozen[iid]:
                assert current[iid].Find(r) != kNoRow   # frozen ⊆ current: under
                    # monotone input a (T,F) drop is PROVABLY IMPOSSIBLE; trips
                    # loud abort instead of silently mis-publishing a retract
        #endif
        f = frozen[iid]
        frozen[iid] = current[iid]              # swap via Vec::Set (Vec has no
        current[iid] = f                        #   mutable operator[]; std::swap illegal)
        current[iid].Reset()                    # retire old frozen -> empty working buffer
        sealed_occupied[iid] = (frozen[iid].NumRows() > 0) ? 1 : 0
        touched_flag[iid] = 0
    touched.Clear()
    # NOTE: publish (band-(b), the (T,F)-drop-then-(F,T)-born two-scan diff of
    # frozen vs current) happens BEFORE this call, driven externally by codegen
    # walking Touched(); Seal() only swaps/resets/recomputes occupancy.

DebugValidate():
    #ifndef NDEBUG
    assert touched.Empty()
    for iid in [0, NumInstances()):
        assert touched_flag[iid] == 0
        assert sealed_occupied[iid]!=0  ==  (frozen[iid].NumRows() > 0)   # coherence
        assert frozen[iid] != current[iid]                                 # non-aliasing
    #endif

~InstanceStore():
    free slots[]
    for iid in [0, NumInstances()): DestroyTable(frozen[iid]); DestroyTable(current[iid])

MakeTable()/DestroyTable(): heap placement-new/explicit-destroy a Table<RowT>
    via allocator (Table owns Vecs => not trivially copyable => can't live
    inline in a Vec<Table>, must be Vec<Table*>)
```

```
// ---- Contrast: StateCellStore (StateCell.h) — the agg/KV peer, NOT a leaf
// relation cache but a leaf SCALAR/algebra-value cache ----
StateCellStore<Key, Algebra>:
  keys[gid], hashes[gid]                     # same open-addressing key->dense-id mold
  working[gid] : Working                     # ONE algebra value (e.g. running sum/count),
                                              #   not a relation — Fold(w, sign, args...)
                                              #   incrementally combines/uncombines
  working_count[gid] : int32_t (SIGNED)      # explicit net member counter (can dip <0
                                              #   transiently mid-epoch for @invertible
                                              #   uncombine); occupancy = working_count>0
  sealed[gid] : Summary                      # last-sealed algebra value (Old(gid))
  sealed_occupied[gid]
  touched / touched_flag                      # same per-epoch dedup mold
  Fold(gid, sign, args...)                    # INCREMENTAL: Algebra::Fold combines/
                                              #   uncombines one summarized row into the
                                              #   running Working value; O(1) per row
  SealOne(gid, cfg...) / Seal-all              # snapshot Working -> sealed, recompute
                                              #   occupancy from working_count
```


### CodeGen instance rescan (keyed InstanceStore full-scan mold + band drivers)

## Generator::EmitSubgraphInstance(region: ProgramSubgraphInstanceRegion)   [Database.cpp:2352]

One emitted block per SUBGRAPH_INSTANTIATE region (one per keyed store `instance_<id>`).
Pulls from the region: `id=StoreId()`, `diff=IsDifferential()` (== TableIsDifferential(pub)),
`demand=DemandFrontier()`, `input_front=InputFrontier()`, `input=InputTable()`,
`input_removal=InputRemovalFrontier()` (optional; presence == input table is differential),
`pub=PubTable()`, `key_pos/row_pos` (pub column layout), `in_key/in_row` (input column
projection into key vs residual row), `RemovalFrontier()` (demand net-removals, only if
demand table is differential).

### 1. `emit_instance_rescan(keyexprs[])`  — THE ONE FULL-SCAN MOLD  [2434-2494]
Emitted C++ shape (parameterized only by the key-comparison RHS exprs):
```
if (instance_<id>.WorkingOccupied(iid)) {           // V-INST-FRESH belt
  fprintf(stderr, "V-INST-FRESH: ..."); abort();
}
auto &cur = instance_<id>.TouchCurrent(iid);
for (uint32_t s = 0; s < <input_table>.NumRows(); ++s) {   // FULL SCAN, every row
  const auto ir = <input_table>.RowAt(s);
  if (ir.<in_key[0]> == keyexprs[0] && ... &&              // key filter (linear)
      [input_diff] <input_table>.Present(s)) {              // live-row conjunct, gated at codegen time
    cur.TryAdd(Row_<id>{ ir.<in_row[0]>, ... });             // residual rebuild
  }
}
```
This is called "kSectionWalk" upstream (Rel.cpp) but is really an unindexed
O(NumRows(input)) linear scan with a per-row key-equality test — no index on
the input table is used; the keyed index is a deferred perf refinement
(comment at 2418-2419 in EmitSubgraphInstance's header block). The
`Present(s)` conjunct is compiled in ONLY when `input_diff` is true (monotone
input emits byte-identically without it) and is what makes the rescan see
the epoch-net live content rather than resurrecting this-epoch-retracted
physical rows still sitting in the un-compacted table.

### 2. band-(a0) DEATH  [2511-2530]  (only if demand table is differential, i.e. RemovalFrontier() present)
```
for (d0,...,dN : Vec(RemovalFrontier())) {
  iid = instance_<id>.FindInstance(Key_<id>{d0,...,dN});   // non-adding
  if (iid != kNoInstance) instance_<id>.RecycleCurrent(iid); // Touch + current.Reset
}
```
Does NOT call the rescan mold — it empties `current` and marks TouchedFlag,
so band-(b)'s drop scan later retracts the *entire* frozen set for that key
(dropped = frozen \ current = frozen).

### 3. band-(a1) DEMAND BIRTH  [2532-2551]
```
for (k0,...,kN : Vec(DemandFrontier())) {
  iid = instance_<id>.FindOrAddInstance(Key_<id>{k0,...,kN});   // adding
  if (!instance_<id>.TouchedFlag(iid)) {
    emit_instance_rescan({k0,...,kN});      // key filter RHS = the demand key itself
  }
}
```

### 4. `emit_edge_drain(frontier)`  — shared band-(a2)/(a2') emitter  [2577-2623]
```
for (e0,...,eM : Vec(frontier)) {                       // full edge row, NOT just key cols
  ekey = { e<in_key[0]>, e<in_key[1]>, ... };            // project edge row onto instance key
  iid = instance_<id>.FindInstance(Key_<id>{ekey});      // non-adding
  if diff (demand table differential):
    if (iid != kNoInstance) {
      dq = <demand_table>.Find({ekey});
      if (dq != kNoRow && <demand_table>.Present(dq) && !TouchedFlag(iid)) {
        emit_instance_rescan(ekey);
      }
    }
  else (monotone demand, sound by irrevocability):
    if (iid != kNoInstance && !TouchedFlag(iid)) {
      emit_instance_rescan(ekey);
    }
}
```
Called twice with the SAME lambda (structural gate-set identity):
- band-(a2)  [2626]: `emit_edge_drain(input_front)`               — edge net-additions
- band-(a2') [2651-2659]: `if (input_removal) emit_edge_drain(*input_removal)` — edge net-removals
Neither arm calls RecycleCurrent (current is provably empty at first touch;
an ungated Recycle here would silently wipe a same-epoch co-added key — forbidden).

### 5. band-(b) PUBLISH  [2661-2787]
```
for (iid : instance_<id>.Touched()) {
  auto &cur = instance_<id>.Current(iid);
  auto &frz = instance_<id>.Frozen(iid);
  auto &key = instance_<id>.KeyAt(iid);
  if diff:
    born=carried=dropped=0
    // (T,F) DROP SCAN FIRST (overdelete-first ordering)
    for r in 0..frz.NumRows():
      drow = frz.RowAt(r)
      if cur.Find(drow) == kNoRow:
        ++dropped; pub.SubDerivation(key+drow, kNonRecursive); DelQueue.Add(key+drow)
      else: ++carried
    // (F,T) BORN SCAN
    for r in 0..cur.NumRows():
      row = cur.RowAt(r)
      if frz.Find(row) == kNoRow:
        ++born; pub.AddDerivation(key+row, kNonRecursive) [+ index adds]; AddQueue.Add(key+row)
    if born+carried != cur.NumRows() || dropped+carried != frz.NumRows():
      fprintf(stderr,"V-INST-PARTITION..."); abort()      // always-on belt, survives NDEBUG
  else (monotone):
    for r in 0..cur.NumRows():
      row = cur.RowAt(r)
      if frz.Find(row) == kNoRow:
        pub.TryAdd(key+row) [+ index adds on success]
}
instance_<id>.Seal();               // swap current -> frozen per touched iid
#ifndef NDEBUG
instance_<id>.DebugValidate();
#endif
```

### Runtime leaf (InstanceStore.h, unmodified by this call — grounding only)
`FindInstance`/`FindOrAddInstance` hash the complete key to an `InstanceId`
(append-only, no tombstone). `TouchCurrent`/`WorkingOccupied` gate the
V-INST-FRESH invariant. `RecycleCurrent` = Touch + current.Reset.
`Touched()`/`TouchedFlag` = the same-epoch dedup set. `Seal()` swaps
current->frozen per touched iid and resets touch state.

### Why it's a full scan, not an index lookup
The mold iterates `input.NumRows()` unconditionally — every physical row of
the summarized input table, live or (pre-compaction) dead — and applies a
per-row linear key-equality test. There is no secondary index keyed on
`in_key` consulted here; "kSectionWalk" (the Rel-IR lowering label minted at
Rel.cpp ~1038-1179, `lowering = kSectionWalk`) is a naming choice on the
upstream IR that this codegen mold makes literally true — it really does
walk the whole table section rather than seeking. All three bands (a1
birth, a2 edge-add, a2' edge-remove) drive into this ONE mold; only the
`keyexprs` actually differ (demand-bound vars vs. the edge row's own
projected key columns).

### ControlFlow Program::Build flat/nested selector

```
Program::Build(frozen: FrozenRegionalProgram, log, first_id, policy: PassPolicy,
                demand_instance: bool) -> optional<Program>:

    query = frozen.Query()                          # H4 unwrap, line 1337
    num_errors0 = log.Size()

    # ---- C-2 feature-gap pre-pass (dominates internal-invariant asserts
    #      downstream in the region-dispatch switch) ----
    has_induction_owned_input(view) =
        exists pred in view.Predecessors() with pred.InductionGroupId().has_value()

    for agg in query.Aggregates():
        if has_induction_owned_input(QueryView(agg)):
            log.Append(agg.Functor().SpellingRange()) << "Aggregates over
                recursively-derived (induction-owned) inputs are not yet supported"
            continue
        # over() with no declared algebra silently defaults to @recompute — no diag.
        # (Full StateCell/GROUP_UPDATE path is otherwise LANDED, no further fence.)

    for kv in query.KVIndices():
        merge = kv.NthValueMergeFunctor(0)
        if has_induction_owned_input(QueryView(kv)):
            log.Append(merge.SpellingRange()) << "Mutable-attributed relations
                over recursively-derived (induction-owned) inputs are not yet
                supported"
            continue
        if not merge.IsInvertible() and not merge.IsRecompute():          # V-ALGEBRA
            log.Append(merge.SpellingRange()) << "Mutable-attributed merge
                functor must declare an algebra (@invertible or @recompute)"
            continue

    for map in query.Maps():
        if not map.Functor().IsPure():
            log.Append(...) << "Impure functors are not yet supported"

    for join in query.Joins():
        if join.NumPivotColumns() == 0
           and QueryView(join).CanReceiveDeletions()
           and ViewSelfReachable(QueryView(join)):
            log.Append() << "Cross-products over differential (deletable) data
                inside recursive cycles are not yet supported"

    # ---- D2.b keyed-instance feature-gap fences, per forcing ----
    # Gated conceptually on -demand-instance (under plain -demand these never
    # fire, since the flat lowering handles every shape); resolved from LIVE
    # GuardAnnotation-stamped views (CSE-migrating index), never a stored
    # RecognizedSubgraph handle.
    any_forcing = false
    all_forcings_admissible = true
    {
        annots = query.GuardAnnotations()
        fguards: map<forcing_index -> vector<(QueryView v, guard_annot_idx ai)>>
        query.ForEachView(v ->
            ai = v.GuardAnnotationIndex()
            if ai != QueryView::kNoGuardAnnotation:
                fguards[annots[ai].forcing_index].push_back((v, ai)))

        for (forcing_index, views) in fguards:
            any_forcing = true
            recursive_content = false
            cyclic_demand = false
            for (v, ai) in views:
                if not v.IsJoin(): continue
                jl = QueryJoin::From(v).JoinedViews()      # ordered list
                if len(jl) < 2: continue
                if annots[ai].role == GuardAnnotation::kBody:
                    in = jl[1]                             # the guarded body/input side
                    if in.InductionGroupId().has_value() or ViewSelfReachable(in):
                        recursive_content = true
                    for p in in.Predecessors():
                        if p.InductionGroupId().has_value():
                            recursive_content = true
                if ViewSelfReachable(jl[0]):                # the demand/lead side
                    cyclic_demand = true

            if cyclic_demand or recursive_content:
                all_forcings_admissible = false

            if demand_instance:                             # STRICT override arm
                if cyclic_demand:
                    log.Append() << "Recursive demand relations are not yet
                        supported under -demand-instance"
                elif recursive_content:
                    log.Append() << "Demanded subgraphs with recursive
                        (induction-owned) content are not yet supported under
                        -demand-instance (a keyed-instance feature gap)"
    }
    # (FENCE (ii) — mid-stream monotone edge-add — no longer fires: R-a2's
    #  band-(a2) rebuilds the standing instance via a full edge-frontier
    #  rescan, so it is HANDLED not fenced.)
    # (FENCE (iii) — differential summarized input — handled by band-(a2'),
    #  also no longer fenced here.)

    if num_errors0 != log.Size():
        return nullopt                                     # any pre-pass OR fence diag aborts

    # ---- RP-9: silent pragma-driven fallback selection ----
    effective_demand_instance = demand_instance
    if not demand_instance and any_forcing and all_forcings_admissible:
        subgraphs = query.RecognizedSubgraphs()
        if not subgraphs.empty() and subgraphs[0].demanded_decl.HasInstanceKey():
            # R-1BOUND invariant: one demanded relation p => every
            # RecognizedSubgraph shares subgraphs[0].demanded_decl.
            assert all(rs.demanded_decl.Id() == subgraphs[0].demanded_decl.Id()
                       for rs in subgraphs)
            effective_demand_instance = true                # @key pragma -> nested, no flag needed

    # ---- build ----
    impl = ProgramImpl(query, first_id)
    context = Context()
    context.init_proc = impl.procedure_regions.Create(impl.next_id++, kInitializer)
    context.demand_forcings = &query.DemandForcings()
    context.demand_instance_enabled = effective_demand_instance   # THE selector bit:
        # gates BuildSubgraphInstanceOps (Rel.cpp DR-IR mint), the DR census
        # recount, the eager-walk chain-breaker excision + OD-4 provisioning,
        # and (re-derived) the same fences above. NOT on the PassPolicy
        # registry — a lowering selector, not an optimization pass.
    context.frozen_census = &frozen.Census()
    ... (region building continues: nested arm lowers the recognized subgraph
         via Rel's kInstanceInstantiate/kInstanceDeath/kInstanceSeal ops into
         an InstanceStore leaf; flat arm builds the ordinary guard-JOIN web —
         both consume the same underlying query graph, diverging only at the
         demand_instance_enabled-gated call sites downstream)
```

Decision-point summary (as asked):
- Per-forcing predicate: `cyclic_demand` = ViewSelfReachable on the guard JOIN's lead/demand-side child (`jl[0]`); `recursive_content` = the kBody-role guarded input side (`jl[1]`) is induction-owned OR self-reachable OR has an induction-owned predecessor.
- Grouping key: `GuardAnnotation.forcing_index`, bucketed by scanning every live view for `GuardAnnotationIndex() != kNoGuardAnnotation` (ForEachView, not a stored subgraph list — ABA-safe against CSE).
- `all_forcings_admissible` = AND over every forcing bucket of `!(cyclic_demand || recursive_content)`.
- Two consumers of the SAME per-forcing flags, different outcomes: `demand_instance==true` → STRICT reject on any inadmissible forcing (diagnostic, aborts via the `num_errors` gate); `demand_instance==false` (pragma path) → SILENT flat fallback (`effective_demand_instance` stays `demand_instance`, i.e. false) unless `all_forcings_admissible` AND `any_forcing` AND `RecognizedSubgraphs()[0].demanded_decl.HasInstanceKey()`, in which case `effective_demand_instance` flips to `true` with no diagnostic (RP-9).

### Parser/AST @key surface (lib/Parse)

## Parsing a single `@key(...)` pragma (Parser.cpp ParseLocalExport)

```
# Per-decl locals (reset per #local/#export declaration):
key_pragma_tok            # anchors this set's diagnostics
key_expect_var = false    # sub-state inside "(...)"
key_cur_set: InstanceKeySet = []   # vector<unsigned>, in-progress ordered set

state 8 (pragma tail, after ')' of param list):
    on '@key' token:
        key_pragma_tok = tok
        state = 21              # RP-10: re-entering here for a 2nd/3rd @key
                                 # opens a FRESH key_cur_set (already empty,
                                 # cleared at prior set's close)
    on '@inline' / debug-highlight / '.' / ':'  -> existing declaration-tail handling
    else -> generic "unexpected tokens" error

state 21 (expect '(' after @key):
    on '(' : key_expect_var = true; state = 22
    else   : error "Expected '(' after '@key'"; return (abort whole decl)

state 22 (inside "@key( ... )"):
    on NAMED VARIABLE tok (only if key_expect_var, else error "expected ',' or ')'"):
        resolved_index = linear-scan local->parameters for matching IdentifierId
        if not found: REJECT "Unknown key column '<tok>'"          # key_unknown_1
        if resolved_index already in key_cur_set: REJECT "Duplicate column"  # key_dup_1
        key_cur_set.push_back(resolved_index)
        key_expect_var = false
    on UNNAMED/wildcard variable:
        REJECT "columns must be named; wildcards not permitted"    # key_wildcard_1 / key_anon_1
    on ',':
        if key_expect_var: REJECT "expected named variable, got ','"
        key_expect_var = true
    on ')':
        if key_cur_set.empty() or key_expect_var:
            REJECT "must list >=1 column and may not end with trailing comma"
        # ADJ-K1-A: order-free duplicate-SET check against sets already on
        # THIS declaration (not yet cross-redecl):
        canon = sort(copy(key_cur_set))
        for prev_set in local->instance_key_param_index_sets:
            if sort(copy(prev_set)) == canon:
                REJECT "this @key declares the same column set as an earlier
                        @key pragma on this declaration"           # key_dup (same-decl)
        # Commit: push ORDERED (unsorted) set + its spelling range, parallel arrays
        local->instance_key_ranges.push_back([key_pragma_tok.Position(), tok.NextPosition()))
        local->instance_key_param_index_sets.push_back(move(key_cur_set))
        key_cur_set.clear(); key_expect_var = false
        state = 8   # back to pragma tail, ready for another @key / '.' / etc.
    else: REJECT "Expected ')' to close the instance key"
```

## Cross-redeclaration consistency (Parser.cpp FinalizeDeclAndCheckConsistency, uncommitted Phase-0)

```
FinalizeDeclAndCheckConsistency(decl):
    redecls = decl->context->redeclarations          # includes `decl` itself, appended
    num_redecls = redecls.Size()
    ...                                               # (earlier: arity/type/range checks
                                                        #  vs prev_decl = redecls[num_redecls-2],
                                                        #  the F31-fixed true-previous alias)
    # IDENTICAL-OR-ABSENT, FULL-CONTEXT (extended 2026-08-06):
    ck = decl->instance_key_param_index_sets
    if not ck.empty():
        canon_decl = None
        for i in [0, num_redecls - 2]:                # every PRIOR redecl, not just immediate
            if not redecls[i].instance_key_param_index_sets.empty():
                canon_decl = redecls[i]; break         # FIRST key-bearing prior wins
        if canon_decl and not SameKeySetOfSets(canon_decl.sets, ck):
            REJECT "Instance key declared here differs from a previous redeclaration"
            # anchors: decl's own first key range vs canon_decl's own first key range
            RemoveDecl(decl); return false
    ...
```

## SameKeySetOfSets — the order-collapsing equality (Parser.cpp:1477)

```
SameKeySetOfSets(a: vector<InstanceKeySet>, b: vector<InstanceKeySet>) -> bool:
    canon(v):
        out = std::set<vector<unsigned>>()
        for s in v:
            s_sorted = sort(copy(s))       # <-- destroys INTRA-key column ORDER
            out.insert(s_sorted)           # std::set of vectors <-- destroys
                                            #     INTER-key SET order too
        return out
    return canon(a) == canon(b)
```
This is a pure ORDER-FREE set-of-sets equality at both levels, even though a
single key is documented (Parser.cpp:939, the duplicate-column reject message)
as a "duplicate-free ORDERED column set" — i.e. conceptually a PATH, not a
set. The exact same two-level sort/std::set canonicalization is independently
re-implemented (not shared) in `lib/DataFlow/Demand.cpp` (~902-919) for the
V-DECLARED-KEY RP-10 bijection check that compares the declared set-of-sets
against the SIP-inferred set-of-sets. Both call sites are therefore blind to
column ORDER within a key.

## Storage + accessors (Parse.h / Parse.cpp)

```
using InstanceKey = unsigned                       # index into decl->parameters
using InstanceKeySet = vector<InstanceKey>          # one @key(...) pragma's columns

# per ParsedDeclarationImpl (one redeclaration's own record):
instance_key_param_index_sets: vector<InstanceKeySet>   # one entry per @key pragma
instance_key_ranges:           vector<DisplayRange>     # parallel, same index = same pragma

# ParsedDeclaration (public handle) accessors resolve ACROSS the shared
# redeclaration context (F-K6-SHADOW fix), not just the local impl:
HasInstanceKey():
    return any(redecl in context->redeclarations : !redecl.instance_key_param_index_sets.empty())

InstanceKeys():
    for redecl in context->redeclarations:
        if !redecl.instance_key_param_index_sets.empty():
            return redecl.instance_key_param_index_sets   # FIRST non-empty sibling wins
    return impl->instance_key_param_index_sets             # empty fallback

InstanceKeyRanges(): same resolution pattern, parallel to InstanceKeys()
```
"First non-empty sibling wins" is well-defined only because IDENTICAL-OR-ABSENT
is enforced at parse time (all non-empty siblings are forced identical, up to
the order-blind SameKeySetOfSets equality above) — so which one is picked
doesn't matter for VALUE, only order-blind-equal alternatives are possible.

## The two Phase-0 (uncommitted) rejects

```
# 1. lib/Parse/Query.cpp ParseQuery, state 6 (post ')' pragma tail):
    on '@key' token:
        REJECT "Unexpected '@key' pragma on query <name>; @key declares a
                relation-local access path and may only appear on a #local
                or #export, never on a #query"
        RemoveDecl(query); return
    # A direct, targeted diagnostic BEFORE falling into the generic
    # "unexpected tokens following declaration" cascade. Witness:
    # tests/OptDiff/rejects/reject_key_on_query_1.dr (#query path(...) @key(A).)

# 2. lib/Parse/Parser.cpp FinalizeDeclAndCheckConsistency — full-context scan
   (shown above) replacing the old immediate-previous-only (prev_decl) check,
   which missed the keyed/unkeyed/keyed sequence:
       @key(A), <unkeyed redecl>, @key(B)
   because the unkeyed middle redecl's empty set short-circuited the OLD
   `!pk.empty() && !ck.empty()` guard on prev_decl alone, so @key(B) was never
   compared against @key(A) — a silent LOST CHECK (InstanceKeys() would later
   just return {A}, the first non-empty). Witness:
   tests/OptDiff/rejects/reject_key_redecl_1.dr
```


### FrozenRegionalProgram + Regional planning (lib/Regional/*)


# ============================================================================
# lib/Regional (FrozenRegionalProgram) — the Stage-B degenerate planner.
# Files: include/drlojekyll/Regional/Regional.h (types), lib/Regional/Planning.cpp
# (Build + census), lib/Regional/Format.cpp (pure renders, -region-out / -dot-out).
# ============================================================================

# ---- Pipeline position (bin/drlojekyll/Main.cpp) ----
compile(module):
    query_opt  = Query::Build(module, log, policy, demand_mode, demand_retract)
    # -origin-out advisory dump reads query_opt.OriginDecls() here (unrelated to Regional)
    frozen_opt = FrozenRegionalProgram::Build(*query_opt, error_log)   # <<< THIS SUBSYSTEM
    if !frozen_opt: return FAILURE
    if gRegionStream:    emit FrozenRegionalDump{*frozen_opt}          # -region-out
    if gRegionDOTStream: emit FrozenRegionalDOT{*frozen_opt}           # -region-dot-out, advisory only
    SetRelDumpStream(...)
    program_opt = Program::Build(*frozen_opt, log, first_id, policy, demand_instance)
    GenerateDatabaseCode(program_opt, ...)


# ---- Types (Regional.h) ----
RegionId{v}; PortId{v}; EdgeId{v}          # typed id domains, intra-domain compare only,
                                            # ONLY RegionId(0) exists at Stage B

RegionalCensus {                           # the -region-out trailing `census:` line, 7 fields
    regions=1, child_calls=0, program_roots=1,   # STAGE-B CONSTANTS (struct defaults)
    request_ports=0, input_ports=0, result_ports=0, row_contracts=0
}

RegionalAbi     { kind ∈ {kInput,kQuery,kOutput}; decl_text; route_text }
RegionalPort    { kind ∈ {kRequest,kInput,kResult}; port_index; head_text; fields_text }
RegionalInternal{ text }                    # fabricated demand__ message lines
RegionalPermanentRoot { text }              # unforced-query lines
RegionalContract{ edge_index; rel_name; member_key_text; support_text;
                   declared_key = decl.HasInstanceKey() }   # DOT-only badge (K6-7a)

FrozenRegionalProgram {                     # PRIVATE fields (Regional.h:150-158)
    query: ::hyde::Query          # <<< the Query PASS-THROUGH — the real semantic owner
    census: RegionalCensus
    abis, ports, internals, permanent_roots, contracts : vector<...>
}
    # Const accessors only: Query(), Census(), Abis(), Ports(), Internals(),
    # PermanentRoots(), Contracts(). NO mutation after Build. NO typed region/
    # relation/rule graph of its own — see §"WHAT IT OWNS" below.


# ---- DeriveRegionalCensus(query): the SINGLE census authority ----
# A PURE function of the Query graph's public surface. Called TWICE:
#  (1) inside Build() to populate the stored census
#  (2) inside V-REGION-CENSUS (lib/Rel/Rel.cpp:4634-4662, ValidateDROps tail)
#      to re-derive fresh and fprintf+abort on any field mismatch against the
#      stored `context.frozen_census` (set at Build.cpp:1572,
#      `context.frozen_census = &frozen.Census()`)
DeriveRegionalCensus(query):
    request_ports = |query.DemandForcings()|
    (received, published) = CollectMessages(query)      # real (non-demand__) received
                                                          # msgs + all published msgs,
                                                          # dedup by decl id, sub-module
                                                          # walk order
    input_ports  = |received|
    result_ports = |published|
    row_contracts = |CollectContractInserts(query)|          # R-STORE (insert-named)
                  + |CollectDemandInteriorDecls(query)|       # Tier-1 (demand-named)
                  + |CollectOriginInteriorDecls(query)|       # Tier-2 (origin-named)
    regions=1, child_calls=0, program_roots=1   # constants
    return RegionalCensus{...}


# ---- FrozenRegionalProgram::Build(query, log): the H2 degenerate planner ----
# log is UNUSED today (no Stage-B reject exists; parameter reserves the seam).
Build(query, log):
    out = FrozenRegionalProgram(query)
    module = query.ParsedModule()
    forcings = query.DemandForcings()          # vector<QueryDemandForcing>

    # --- REQUEST PORTS + region-internal lines: 1 per forcing, forcing order
    #     == ascending forcing index == port index (request ports come FIRST)
    for fi, entry in enumerate(forcings):
        fdecl = ParsedDeclaration(entry.query)
        port = RegionalPort{kRequest, port_index=fi,
                 head_text = "query=" + fdecl.Name()
                            + (2<=NumForcingsOfName(forcings, entry.query)
                               ? "  adorn=" + fdecl.BindingPattern() : ""),
                 fields_text = BoundParamNames(fdecl)}
        out.ports.push(port)
        out.internals.push(RegionalInternal{
            MessageDeclText(module, entry.message) + "  [fabricated, driver-suppressed]"})

    # --- INPUT/RESULT ports + input/output ABIs, declaration order
    (received, published) = CollectMessages(query)
    next_port = |forcings|
    for m in received:  mint kInput  port (next_port++) + input_abi  "-> R0 via P<k>"
    for m in published: mint kResult port (next_port++) + output_abi "-> R0 via P<k>"
    if output_abis.empty(): output_abis = [{"<none>", ""}]

    # --- QUERY ABIs + permanent roots, decl order; per-name UniqueRedeclarations
    #     walk with a BindingPattern dedup (mirrors BuildQueryEntryPoint's idiom)
    for parsed_query in (dedup by Id over sub-module walk):
        for redecl in decl.UniqueRedeclarations():        # dedup by BindingPattern
            matched_forcing = index fi such that
                forcings[fi].query == redecl_query AND
                ParsedDeclaration(forcings[fi].query).BindingPattern()
                    == redecl.BindingPattern()             # else -1
            if matched_forcing >= 0:
                abi.route_text = "-> R0 via P<matched_forcing>"
            else:
                abi.route_text = "-> permanent-root"
                out.permanent_roots.push(name + AllParamNames(redecl))
            out.query_abis.push(abi)
    out.abis = input_abis ++ query_abis ++ output_abis

    # --- ROW CONTRACTS, 3 tiers, ONE dense `edge` counter, R-STORE FIRST:

    # Tier R-STORE (insert-materialized): CollectContractInserts(query) walks
    # query.Inserts() range, keeps relation inserts (not stream), skips
    # "demand__"-prefixed decls, keeps FIRST insert view per distinct decl id.
    row_contracts_map = query.impl->row_contracts     # <<< THE ONE FRIEND LEAK:
        # RowContractMap = unordered_map<QueryViewImpl*, RowContract>, a private
        # DataFlow field reached via quoted #include "Query.h" (lib/DataFlow
        # private header; lib/Regional/CMakeLists.txt grants PRIVATE include dir)
    for (decl, ins) in CollectContractInserts(query):
        rc = row_contracts_map[QueryView(ins).impl]     # abort (FROZEN-REGIONAL) if missing
        key_text = "(" + [decl.NthParameter(i).Name() for i in visible_fields
                           if i < decl.Arity() and visible_fields[i] in rc.member_key] + ")"
        out.contracts.push(RegionalContract{
            edge_index = edge++, rel_name = decl.Name(), member_key_text = key_text,
            support_text = view.CanReceiveDeletions() ? "differential" : "monotone",
            declared_key = decl.HasInstanceKey()})

    # Tier-1 (demand-INTERIOR, Tier-1 naming lift): CollectDemandInteriorDecls
    # walks query.RecognizedSubgraphs() [OLD demand side-table], reads
    # rs.demanded_decl (the mint-time Connect-proxy snapshot), skips decls
    # already named by R-STORE, dedups (first forcing wins).
    for decl in CollectDemandInteriorDecls(query):
        out.contracts.push(RegionalContract{
            edge_index = edge++, rel_name = decl.Name(),
            member_key_text = AllParamNames(decl),        # AllFields passthrough
            support_text = ResolveInteriorSupport(query, decl) ? "differential" : "monotone",
            declared_key = decl.HasInstanceKey()})

    # Tier-2 (origin-INTERIOR, K5): CollectOriginInteriorDecls walks
    # query.ForEachView() -> v.OriginDecls() [independent of demand side-tables],
    # dedups against (R-STORE ∪ Tier-1), Id-ordered.
    for decl in CollectOriginInteriorDecls(query):
        out.contracts.push(RegionalContract{
            edge_index = edge++, rel_name = decl.Name(),
            member_key_text = AllParamNames(decl),
            support_text = ResolveOriginSupport(query, decl) ? "differential" : "monotone",
            declared_key = decl.HasInstanceKey()})

    # --- CENSUS: derive, then self-recount the just-built vectors against it
    out.census = DeriveRegionalCensus(query)
    assert-or-abort(count(out.ports, kRequest) == out.census.request_ports)
    assert-or-abort(count(out.ports, kInput)   == out.census.input_ports)
    assert-or-abort(count(out.ports, kResult)  == out.census.result_ports)
    assert-or-abort(|out.contracts| == out.census.row_contracts)
    # (fprintf "FROZEN-REGIONAL: census mismatch (%s)..." + abort on violation)

    # --- FREEZE VALIDATORS (always-on, fprintf+abort, NDEBUG-surviving)
    V-FROZEN-NO-OPEN-PORT: every out.ports[i] has kind ∈ {request,input,result}
                           AND nonempty head_text
    V-OWNERSHIP-ACYCLIC:   out.census.regions==1 AND out.census.child_calls==0
                           (Stage-B degenerate-ownership-forest invariant;
                            Stage C is expected to generalize this to a real walk)

    return out


# ---- ResolveInteriorSupport(query, decl): Tier-1 support-bit resolution ----
# Reads BOTH old demand side-tables (GuardAnnotation, RecognizedSubgraph) AND
# the LIVE post-Optimize graph — never the dangling stored QueryView handles.
ResolveInteriorSupport(query, decl):
    decl_forcings = { rs.forcing_index for rs in query.RecognizedSubgraphs()
                       if rs.demanded_decl.Id() == decl.Id() }
    resolved = false; support = false
    for v in query.ForEachView():
        ai = v.GuardAnnotationIndex()
        if ai == QueryView::kNoGuardAnnotation or ai >= |query.GuardAnnotations()|: continue
        annot = query.GuardAnnotations()[ai]
        if annot.forcing_index not in decl_forcings or !v.IsJoin(): continue  # role-blind:
            # a projection-role annotation carrier is CSE-migrated to kBody
            # (PromoteSurvivorToBody), so filtering by annot.role would miss
            # live corpus cases — checked ONLY that it's a JOIN
        resolved = true
        support = support or v.CanReceiveDeletions()
    if !resolved: fprintf(stderr, "TIER1-SUPPORT-RESOLVE: ..."); abort()
    return support

# ---- ResolveOriginSupport(query, decl): Tier-2 support-bit resolution ----
# Independent of demand side-tables — walks OriginDecls() only.
ResolveOriginSupport(query, decl):
    resolved=false; support=false
    for v in query.ForEachView():
        for d in v.OriginDecls():          # sorted-unique per view
            if d.Id() != decl.Id(): continue
            (DEBUG) assert !resolved or (crd matches prior crd)   # differentialness-agreement
            resolved = true; support = support or v.CanReceiveDeletions(); break
    if !resolved: fprintf(stderr, "ORIGIN-SUPPORT-RESOLVE: ..."); abort()
    return support


# ---- lib/Regional/Format.cpp: pure renders, NO re-derivation ----
operator<<(os, FrozenRegionalDump{p}):     # -region-out, byte-golden-able
    print "region-program"
    print "program-root { ... }"   over p.Abis(), column-padded
    print "region R0  owner=program-root  parents=()  children=() { ... }"
        over p.Ports(), p.Internals(), p.PermanentRoots(), p.Contracts()
        (column widths computed PER-DUMP as running max -> E-K5-PAD: a new
         longest member-key re-pads ALL existing lines in that dump)
    print "census: regions=.. child-calls=.. program-roots=.. request-ports=..
           input-ports=.. result-ports=.. row-contracts=.." from p.Census()

operator<<(os, FrozenRegionalDOT{p}):      # -region-dot-out, advisory, NEVER goldened
    emit one `subgraph cluster_region_0` with port/internal/proot/contract nodes
    (contract nodes append " declared-key" iff contract.declared_key — K6-7a)
    emit ABI nodes OUTSIDE the cluster with routing edges to port/proot nodes


# ---- Downstream consumer: V-REGION-CENSUS (lib/Rel/Rel.cpp, inside
#      ValidateDROps, run per compile) ----
# Program::Build (lib/ControlFlow/Build/Build.cpp:1333) does:
#   query = frozen.Query()                       # H4: unwrap, first statement
#   context.frozen_census = &frozen.Census()      # Build.cpp:1572
# ...
# ValidateDROps(..., context, ...):
#   if context.frozen_census:
#       expect = DeriveRegionalCensus(query)      # FRESH re-derivation
#       for each of the 7 fields: stored != expect -> fprintf("V-REGION-CENSUS:
#           %s stored %u != derived %u") + abort()
#   # => a FrozenRegionalProgram stubbed to an empty shell (census all-zero-ish
#   #    defaults, no real ports/contracts) FAILS here downstream — the
#   #    "positive-presence referee" the module comment calls out.


# ============================================================================
# WHAT FrozenRegionalProgram ACTUALLY OWNS vs. reaches back into:
#
# OWNS (real state, computed once at Build, immutable after):
#   - RegionalCensus (7 ints)
#   - 5 vectors of RENDER-READY STRINGS (RegionalAbi/Port/Internal/
#     PermanentRoot/Contract) — text already formatted (decl spellings,
#     binding patterns, "(A, B)" tuples), NOT a typed graph. Format.cpp's
#     comment states this explicitly: "pure renders of the frozen row
#     structs; nothing is re-derived from the Query graph here."
#   - a `::hyde::Query query` COPY (the wrapper is a thin impl-pointer handle;
#     "owns" only in the sense of holding the handle alive)
#
# OWNS NOTHING SEMANTIC:
#   - no typed RegionId/PortId/EdgeId graph beyond the reserved 0-valued
#     domains (RegionId(0) is the only region that will ever exist at Stage B)
#   - no relation schema / rule / RuleRoutingProjection / AccessRequirement
#     records (these are the §2 TARGET types the rewrite introduces; today
#     they don't exist anywhere)
#   - member-key/support text is COPIED OUT of `query.impl->row_contracts`
#     (Stage-A RowContractMap) and Guard/Recognized side-tables at Build time,
#     never re-derived by FrozenRegionalProgram itself post-Build
#
# REACHES BACK INTO DEMAND SIDE-TABLES (the seed's central complaint,
# concretely, all inside Planning.cpp):
#   1. CollectDemandInteriorDecls (Tier-1 EXISTENCE/COUNT): reads
#      query.RecognizedSubgraphs() -> rs.demanded_decl
#   2. ResolveInteriorSupport (Tier-1 support bit): reads
#      query.GuardAnnotations() + query.RecognizedSubgraphs() (bucket by
#      forcing_index) THEN cross-references LIVE views via
#      v.GuardAnnotationIndex() / QueryView::kNoGuardAnnotation
#   3. request-port / region-internal minting: reads query.DemandForcings()
#      directly (QueryDemandForcing{query, message, bound_params})
#   4. QUERY ABI routing: matches redecl against forcings[fi].query +
#      BindingPattern to decide "-> R0 via P<k>" vs "-> permanent-root"
#   5. R-STORE contract skip rule: string-matches "demand__" decl-name prefix
#      to exclude fabricated relations
#
#   Tier-2 (CollectOriginInteriorDecls / ResolveOriginSupport) is the ONE
#   naming path that does NOT touch demand side-tables — it rides
#   QueryView::OriginDecls() (a general CDaGI-propagated provenance field,
#   independent of the demand pass), though it still DEDUPS against Tier-1's
#   demand-derived set to avoid double-naming.
#
# CONSEQUENCE for the rewrite (per seed §4 Phase 1/2): deleting
# GuardAnnotation/RecognizedSubgraph/QueryDemandForcing (Phase 1) breaks
# points 1-4 above outright — CollectDemandInteriorDecls,
# ResolveInteriorSupport, and the request-port/ABI-routing loops in Build()
# all need a REPLACEMENT naming/classification source before or alongside
# that deletion (this is exactly the gap Phase 2's RegionTemplate/
# RelationSchema/declared_access_paths records are meant to close: a typed
# planning-time record instead of demand-pass side-table lookups).
# ============================================================================


### Stratify + row contracts (Stage A identity layer)

## A. Where this subsystem sits in Query::Build (lib/DataFlow/Build.cpp:2524-2683)

```
Query::Build(module, log, policy, demand_mode, demand_retract):
    ... build graph, ConnectInsertsToSelects, ApplyDemandTransform ...
    if policy.AnyBodyOptionalEnabled(kDataFlow): impl->Optimize(log, policy)
    impl->ConvertConstantInputsToTuples(); RemoveUnusedViews()
    impl->ProxyInsertsWithTuples(); LinkViews(); RemoveUnusedViews()
    impl->IdentifyInductions(log)                      # :2633, debug cross-check source
    impl->FinalizeDepths(); FinalizeColumnIDs()         # :2638-2639, freezes view->depth,
                                                         #   QueryColumnImpl::id (value ids)
    impl->TrackDifferentialUpdates(log, true)
    impl->TrackConstAfterInit()
    BuildEquivalenceSets(impl.get())                    # :2646, models (EquivalenceSet) ready
    impl->Stratify(log)                                 # :2647 — SEE §B
    if errors: return nullopt
    impl->row_contracts = InferConservativeRowContracts(impl.get())   # :2655 — SEE §C
    if !ValidateRowContracts(impl.get(), log): return nullopt          # :2656, H-A7 belts
    #ifndef NDEBUG: K5 conservation belt (demanded-interior origin_decls reachability)
    return Query(impl)                                  # :2683
```

RowContract is a PURE, RECOMPUTABLE function of the FINAL graph (post-Stratify,
post-FinalizeColumnIDs, post-TrackConstAfterInit) — never materialized during
Optimize, so CSE/canonicalization owe it nothing (the F1 lesson: no satellite
annotation for a pass to migrate).

## B. QueryImpl::Stratify (lib/DataFlow/Stratify.cpp:124-441)

```
Stratify(log):
    views := ForEachView(this)                          # live-view index, :127-132
    sources[v] := v.predecessors                         # column-edge adjacency
               ∪ {negate->negated_view}   for v a NEGATE  # :150-155
               ∪ {insert}   for every (insert,select) INSERT->SELECT decl seam
                                                          # ForEachInsertToSelectSeam, :162-172
    io_seams := {(select,insert) : seam.insert->stream}   # message publish<->receive pairs,
                                                           #   invisible to IdentifyInductions

    # Iterative Tarjan (explicit frame stack, no recursion): pop order is a
    # topological sort with SOURCES FIRST, so stratum ids increase downstream.
    run Tarjan(sources) -> state[v].stratum               # :178-232
    num_strata := next_stratum                            # :235
    for v in views: v.stratum := state[v].stratum          # :237-240

    # model (EquivalenceSet) stratum = MAX over member views' strata;
    # record any model whose views straddle >1 stratum
    for v in views: model_strata[v.model].(min,max) update  # :245-256
    for (model,(mn,mx)) in model_strata:
        model.stratum := mx
        if mn != mx: stratum_straddling_models.push(model)  # :258-267

    # Reject unstratified negation: negate and its negated_view share an SCC
    for negate in negations (skip dead):
        if negate.stratum == negate.negated_view.stratum:
            REJECT "recursively derived from the negation's own result"   # :272-297

    # Reject unstratified aggregation/KV (same shape): the summarized input
    # predecessor(s) share the aggregate's own stratum
    reject_in_scc_agg(agg, kind):
        for input_view in agg.predecessors:
            if agg.stratum == input_view.stratum:
                REJECT "aggregate recursively derived from its own result"; break
    for agg in aggregates (skip dead):    reject_in_scc_agg(agg, "summarized")   # :329-334
    for kv  in kv_indices (skip dead):    reject_in_scc_agg(kv,  "keyed")        # :336-344

    # V-SCC-SEAM (ALWAYS-ON, fprintf+abort, survives NDEBUG — F26 promotion):
    # every multi-view stratum (>1 view) not closed by an io_seam must contain
    # an inductive MERGE (AsMerge() && induction_info); else a source-less
    # dead forwarding cycle survived to Stratify -> abort.
    for s in 0..num_strata:
        if stratum_num_views[s] != 1
           and !stratum_has_inductive_merge[s]
           and !stratum_has_io_seam[s]:
            fprintf+abort("V-SCC-SEAM: dead forwarding cycle survived")   # :372-381

    #ifndef NDEBUG:
        # cross-check SCC condensation vs IdentifyInductions' info-bearing views
        # (skipped for strata closed only through an io_seam, invisible to that
        # pass): same-SCC <=> inductive-predecessor/successor mask bit; and
        # info->merge_set_id induces the SAME partition as stratum id.  :383-440
```

Outputs stored on the IR: `view->stratum` (per view), `EquivalenceSet::stratum`
(per model, max over member views), `impl->num_strata`,
`impl->stratum_straddling_models`. `QueryView::DerivationClassInto` later
derives recursive-vs-seed purely from whether an inserting view shares its
target model's owning stratum — no separate bookkeeping.

## C. InferConservativeRowContracts (lib/DataFlow/RowContract.cpp:365-411)

A TWO-PHASE PURE GRAPH FUNCTION over the final graph (NOT a fixpoint):

```
InferConservativeRowContracts(impl) -> RowContractMap:
    views_by_depth := stable_sort(ForEachView(impl), by view.depth)
        # READ-ONLY read of the FROZEN depth (FinalizeDepths already ran).
        # MUST NOT call ForEachViewInDepthOrder — that RESETS depth as a side
        # effect and would silently re-order .rel/.ir/codegen downstream.
        # Stable sort by frozen depth is topological over the acyclic
        # condensation (cross-SCC edges go low->high depth).

    # Phase 0: per-stratum view histogram
    stratum_size[s] := |{v : v.stratum == s}|

    # Phase 1: CYCLIC rule — every view on a multi-view stratum (a recursive
    # SCC, stratum_size>1) gets member_key = AllFields directly from SCC
    # structure. This DISSOLVES the would-be contract fixpoint: a back-edge
    # input's contract is never read empty by Phase 2.
    for v in views_by_depth:
        if v.stratum.has_value() and stratum_size[v.stratum] > 1:
            out[v] = { visible_fields = AllFieldIds(v),
                       member_key    = AllFieldIds(v) }

    # Phase 2: ACYCLIC per-operator transfer, ONE pass, topological (depth)
    # order, over the remaining single-view strata (inputs already resolved:
    # either Phase-1-assigned or earlier-in-depth-order acyclic).
    for v in views_by_depth:
        if v not in out:
            out[v] = TransferContract(v, out)
    return out
```

`AllFieldIds(v)`: view's visible columns (an INSERT's `input_columns` — it is
terminal, `columns` is empty; every other kind's `columns`) mapped to
`FieldId{col.id}` (the value id, stable under column renumbering).

`TransferContract(v, out)` — flat-key, value-id-intersection transfer
(RowContract.cpp:159-248):

```
TransferContract(v, out):
    rc.visible_fields := AllFieldIds(v)
    passthrough(p) := { out_col.id : out_col in VisibleCols(v),
                         out_col.id in ProducerKeyIds(p, out) }
        # ProducerKeyIds(p,out): out[p].member_key ids if present,
        # else p's own AllFields ids (out-of-order read stays sound)

    match v.kind:
      TUPLE, projection_role == kDistinct:
          key_ids := AllFieldIds(v)                    # visible tuple IS the key
      TUPLE, projection_role == kMember:
          key_ids := passthrough(incoming_view)          # mapped producer key
      COMPARE:      key_ids := passthrough(incoming_view)
      NEGATE:       key_ids := passthrough(incoming_view)   # positive-side key preserved
      INSERT:       key_ids := passthrough(incoming_view)   # passthrough of input head
      JOIN:
          for out_col in v.columns:
              for in_col in join.out_to_in[out_col]:
                  if !in_col.IsConstant()
                     and in_col.id in ProducerKeyIds(in_col.view, out):
                      key_ids += out_col.id; break
      AGGREGATE:
          # output layout is [group_by..., config..., summary...]; key =
          # group+config PREFIX, POSITIONAL not id-matched (summary columns
          # are functionally determined by the group, never key members)
          n_key := |group_by_columns| + |config_columns|
          key_ids := { v.columns[0..n_key) }
      else (SELECT, MERGE, MAP, KVINDEX, future kinds):
          key_ids := AllFieldIds(v)                     # sound conservative fallback

    if key_ids.empty(): key_ids := AllFieldIds(v)        # never an empty key
    rc.member_key := MemberKeyFromIds(v, key_ids)        # dedup, output-column order
    return rc
```

`AggInputView(agg)`: the single non-constant producer among
`aggregated_columns` / `group_by_columns` / `config_columns`, in that
priority order (RowContract.cpp:139-156).

## D. ValidateRowContracts — the H-A7 belts (RowContract.cpp:413-419), all UNCONDITIONAL

```
ValidateRowContracts(impl, log):
    CheckContractCensus(impl)       # :254-272 ALWAYS-ON
        assert |row_contracts| == |live views|, exactly one per live view,
        none for a dead view                              -> fprintf+abort
    CheckMemberKeyRealized(impl)    # :276-296 ALWAYS-ON
        for every live view v with any visible column:
          assert rc[v].member_key nonempty
          assert every FieldId in rc[v].member_key resolves to a live column of v
                                                             -> fprintf+abort
    CheckNoCollapse(impl)           # :318-336 BELT-ONLY (E-A2)
        # the REAL "unproven collapse" reject is UNSOUND at Stage A's
        # conservative-AllFields-producer-key model (every dropped column
        # looks like a dropped key column even for a benign projection) —
        # defers to Stage B Minimize. Only the SOUND residual is checked:
        for every kMember TUPLE v:
          assert !(VisibleCols(v) nonempty and rc[v].member_key.empty())
                                                             -> fprintf+abort
        # already implied by CheckMemberKeyRealized; inert on a correct
        # pipeline (collapse_error=0 census witness).
    CheckAggInputKey(impl)          # :340-361 ALWAYS-ON
        for every aggregate/KV view v:
          in := AggInputView(v); assert in != null
          assert row_contracts[in] exists and .member_key nonempty
                                                             -> fprintf+abort
    return true    # never returns false — every failure aborts the process
```

## E. ProjectionRole / SemanticMemberKey tie-in (Identity.h, Query.h, Tuple.cpp)

```
SemanticMemberKey := std::vector<FieldId>       # Identity.h:52
FieldId { unsigned v }                          # wraps QueryColumnImpl::id — a
                                                 #   value id, stable under
                                                 #   column renumbering

QueryTupleImpl::ProjectionRole { kMember (default), kDistinct }   # Query.h:729
    # set ONCE at mint (ConvertToClauseHead is the sole kDistinct minter),
    # never mutated afterward.

QueryTupleImpl::Equals(that):                    # Tuple.cpp:290-...
    if projection_role != that.projection_role: return false   # :308, UNCONDITIONAL
        # CSE buckets by Equals (cse_color::Refine), so a role mismatch is an
        # outright merge refusal — a member-preserving TUPLE can never CSE-
        # fold into a set-collapsing one or vice versa.

QueryTupleImpl::Hash():                          # Tuple.cpp:25-58
    # projection_role is DELIBERATELY NOT folded in (NOTE(H-A2), :35-49):
    #  (1) Hash is not on the CSE decision path (Equals is);
    #  (2) folding it would perturb order-sensitive Hash-derived tie-breaks
    #      elsewhere (Merge.cpp canonicalization sort, ControlFlow Program
    #      .Hash() ordering) for zero CSE benefit.
```

`RowContractMap` (RowContract.h:51) is keyed by RAW LIVE-VIEW POINTER
(`QueryViewImpl *`), rebuilt WHOLESALE by `InferConservativeRowContracts` at
the Query::Build tail — deliberately NOT `LogicalNodeId`-keyed (OWNER-GATE
O-A2, deferred to Stage B). This is the dangle hazard: the map is valid only
against the exact live-view set present at the moment it was built; any
further graph mutation (a later Optimize pass, a CSE merge) invalidates
stale pointer keys. In the landed pipeline this is safe because the map is
built ONCE, after ALL graph-mutating passes (Optimize, LinkViews,
IdentifyInductions, FinalizeDepths/ColumnIDs) have already run and the graph
is otherwise frozen for the rest of `Query::Build`'s tail — the same
"materialize once, at the very end" discipline the doc calls the F1 lesson
(no satellite annotation for CSE/canonicalization to migrate). Downstream
consumers (Stage B `FrozenRegionalProgram::Build`, `-contract-out`,
`-dot-out`) all run against this SAME frozen `QueryImpl`, never across a
further mutation, so the pointer keys stay valid for their lifetime.

### Query::Build + ApplyDemandTransform (lib/DataFlow/Build.cpp, lib/DataFlow/Demand.cpp, lib/Parse/Demand.cpp)

## Query::Build (lib/DataFlow/Build.cpp:2524)

```
Query::Build(module, log, policy, demand_mode, demand_retract, suppress_demand):
  impl = new QueryImpl(module)
  for sub_module in ParsedModuleIterator(module):
    for clause in sub_module.Clauses() where !IsDisabled(): BuildClause(impl, clause, context, log)
    for message in sub_module.Messages(): warn if never-published-or-received (first decl only)
  impl.RemoveUnusedViews(); impl.ClearGroupIDs(); impl.TrackDifferentialUpdates(log)
  if errors: return nullopt

  if policy.Gate("df.simplify"): impl.Simplify(log)          # always-on in practice (outside optimize guard)

  proxy_view_to_decl: unordered_map<VIEW*, ParsedDeclaration>  # Query::Build-SCOPED, never a QueryImpl member
  if !impl.ConnectInsertsToSelects(log, proxy_view_to_decl): return nullopt

  # THE DEMAND CUT. Slotted after Connect (producer/consumer wiring complete),
  # before Optimize (so demand relations join the same CSE/canon fixpoint) and
  # before IdentifyInductions/Stratify (so induction cross-checks see demand
  # edges). demand_mode==false && no @key pragma ⇒ byte-identical no-op.
  if !impl.ApplyDemandTransform(module, log, demand_mode, demand_retract,
                                 suppress_demand, proxy_view_to_decl):
    return nullopt
  if errors: return nullopt

  if policy.AnyBodyOptionalEnabled(kDataFlow): impl.Optimize(log, policy)   # CSE, canon, DFE
  if errors: return nullopt

  impl.ConvertConstantInputsToTuples(); impl.RemoveUnusedViews()
  impl.ProxyInsertsWithTuples(); impl.LinkViews(); impl.RemoveUnusedViews()
  impl.IdentifyInductions(log)
  if errors: return nullopt

  impl.FinalizeDepths(); impl.FinalizeColumnIDs()
  impl.TrackDifferentialUpdates(log, force=true); impl.TrackConstAfterInit()
  if errors: return nullopt

  BuildEquivalenceSets(impl)
  impl.Stratify(log)                                          # SCC + neg/agg stratification
  if errors: return nullopt

  impl.row_contracts = InferConservativeRowContracts(impl)     # pure, post-Stratify, Stage A
  if !ValidateRowContracts(impl, log): return nullopt
  #ifndef NDEBUG: K5 conservation belt — every demanded interior's decl is origin-reachable at a live view
  return Query(impl)
```

## QueryImpl::ConnectInsertsToSelects (lib/DataFlow/Connect.cpp:164)

```
ConnectInsertsToSelects(log, out proxy_view_to_decl):
  for io in ios:
    io.transmits.Unique(); io.receives.Unique()
    if !transmits.Empty() && !receives.Empty(): error "cannot have both sends and receives"
    if num_transmits > 1: proxy = CreateProxyOfInserts(transmits); mint INSERT wrapping proxy; transmits = {insert}
    elif num_receives > 1: select = mint SELECT matching prev_sel's schema; ProxySelects(receives -> select); receives = {select}
    # (==1 case: leave as-is)

  for rel in relations:                       # queries, locals, exports
    rel.inserts.Unique(); rel.selects.Unique()
    decl = rel.declaration
    if decl.Arity() == 0:                      # unit/condition relation
      wire each SELECT's inserts list to rel.inserts directly; continue
    if rel.inserts.Empty(): error "missing a definition"; continue

    insert_proxy = CreateProxyForMutableParams(CreateProxyOfInserts(rel.inserts), decl)
    proxy_view_to_decl[insert_proxy] = decl        # Tier-1 naming snapshot: sole writer here,
                                                    # sole reader = ApplyDemandTransform (pre-Optimize)
    if !decl.IsInline() && !decl.IsQuery():
      insert_proxy.origin_decls = [decl]           # K5 Tier-2 origin-provenance seed (seed-once)

    rel.inserts.Clear()
    if rel.selects.Empty() && !decl.IsQuery(): continue   # dead INSERTs, left for canon cleanup

    ProxySelects(rel.selects -> insert_proxy)
    if decl.IsQuery():
      insert = mint INSERT wrapping insert_proxy; rel.inserts = {insert}

  RemoveUnusedViews(); TrackDifferentialUpdates(log, true)
  return true
```

## QueryImpl::ApplyDemandTransform (lib/DataFlow/Demand.cpp:388)

```
ApplyDemandTransform(module, log, demand_mode, demand_retract, suppress_demand, proxy_view_to_decl):
  if suppress_demand: return true                      # bin/Oracle: demand-blind referee override

  # --- ACTIVATION GATE (RP-6) ---
  demand_key_decls = dedup-by-Id({ d in (Locals ∪ Exports) across ParsedModuleIterator(module)
                                    : d.HasInstanceKey() })
  pragma_activated = !demand_key_decls.empty()
  if !demand_mode && !pragma_activated: return true     # hard containment gate: byte-identical no-op

  if module.DemandMessagesFabricated(): internal-error "re-entered on already-fabricated module"; return false

  reject(what) = log.Append(...) << what << (pragma_activated ? "; fix or remove the @key pragma"
                                                                : "; recompile without -demand"); return false

  # --- 1. Locate the (sole) demanded query ---
  bound_queries = [rel : rel in relations, rel.decl.IsQuery(), rel.decl.Arity()>0,
                        any param.Binding()==kBound]
  if bound_queries.empty():
    if pragma_activated: reject-at-decl "declares an instance key but no bound #query exists to seed demand"
    return true                                          # benign no-op under pure flag activation
  if bound_queries.size() > 1: return reject("Multiple demanded (bound) queries are not yet supported")
  q_rel, q_decl = bound_queries[0], bound_queries[0].declaration
  if q_rel.inserts.size() != 1: return reject("must have exactly one materialization")
  q_insert = q_rel.inserts[0]

  plan: vector<PerAdornment> = []            # {redecl, bound_indices, p_bound, q_read, q_consumer,
                                              #  p_merge, sites, pushdown_reads}
  known_consumers: set<VIEW*> = {}           # ADV-3 pass-level union
  seen_variants: set<string> = {}            # BindingPattern dedup, mirrors Build.cpp UniqueRedeclarations quirk

  # === Loop 1 / Phase 1: per-adornment locate + fence, NO minting ===
  for redecl in q_decl.UniqueRedeclarations():
    if !seen_variants.insert(redecl.BindingPattern()): continue        # dup pattern
    bound_indices = [param.Index() : param in redecl.Parameters() if param.Binding()==kBound]
    if bound_indices.empty(): return reject("all-free sibling adornment not yet supported")

    # Step 2: trace each bound query column back through forwarding TUPLEs / the
    # query's own post-Connect MERGE to a full-width reader TUPLE q_read over p's
    # MERGE p_merge; q_consumer = the view holding that read. All bound columns
    # must land on the SAME (p_merge, q_read, q_consumer); distinct-columns check.
    (p_merge, q_read, q_consumer, p_bound) = trace-projection-chain(q_insert, bound_indices)
       # rejects: unsupported shape, multi-clause query, cross-consumer bound cols,
       #          duplicate p positions

    # Step 3: for each MERGE member (rule body) of p_merge: walk its tree, count
    # reads of p (reject if >1, self-join); reject NEGATE/AGG on the demand path
    # (demand sink) and any un-witnessed node kind. Then, for each bound position,
    # trace it backward through forwarding TUPLEs / JOIN out_to_in to a GuardSite:
    #   kReadAtTuple  — bound value comes straight off a read of p_merge (recursive)
    #   kBaseAtom     — bound value's source is a message-receive atom
    #   kPushDown     — bound value flows out of a JOIN fed by a read of p_merge
    # All bound positions in one body must share ONE site (same kind/consumer/read);
    # "sideways" (non-From-preserving) propagation rejects.
    sites, pushdown_reads = locate-guard-sites(p_merge, p_bound)

    known_consumers += {q_consumer} ∪ {site.consumer : site in sites}
    assert p_merge == plan.front().p_merge if plan non-empty     # one relation p
    plan.push(PerAdornment{redecl, bound_indices, p_bound, q_read, q_consumer,
                            p_merge, sites, pushdown_reads})

  # --- Tier-1 decl resolution (once, from Connect-time map) ---
  p_demanded_decl = proxy_view_to_decl[plan.front().p_merge]     # T1-DECL-MISS: abort if missing (compiler bug)

  # --- RP-6 realization: every @key decl must BE the demanded relation ---
  for d in demand_key_decls:
    if d.Id() != p_demanded_decl.Id(): reject-at-key-range "declares an instance key but is not the demanded relation"; return false

  # --- V-DECLARED-KEY (RP-10 total bijection, order-free set-of-sets) ---
  if p_demanded_decl.HasInstanceKey():
    canon(v) = sort(v)
    declared_sets = { canon(s) : s in p_demanded_decl.InstanceKeys() }
    inferred_sets = { canon(a.p_bound) : a in plan }
    for j, d in enumerate(p_demanded_decl.InstanceKeys()):        # Arm A: declared surplus
      if canon(d) not in inferred_sets: reject-at-pragma-range "has no matching demanded query adornment"; return false
    for i in inferred_sets:                                       # Arm B: inferred surplus
      if i not in declared_sets: reject-at-decl "has no matching @key instance key"; return false

  # --- Step 4: stray-consumer union, ONCE, on the pre-mint graph ---
  for user in CollectColUsers(plan.front().p_merge):
    t = user.AsTuple()
    if !t || !IsFullWidthReaderOf(t, p_merge): return reject("consumer shape not yet supported")
    for ruser in CollectColUsers(t):
      if ruser not in known_consumers: return reject("read by a consumer demand cannot guard")

  pending: vector<PendingRewire> = []   # {consumer, read, out_for_pos, guard, restore, kind}

  # === Loop 2 / Phase 2: per adornment, fabricate + mint + guard + register ===
  for a in plan:
    adorn = "".join('b' if p.Binding()==kBound else 'f' for p in a.redecl.Parameters())
    base_name = "demand__" + q_decl.Name() + "_" + adorn
    bound_types = [a.redecl.NthParameter(bi).Type() for bi in a.bound_indices]

    if module.DemandFabricationWouldCollide(base_name, base_name+"_local", |bound_types|):
      return reject("collides with the reserved demand__ prefix")
    d_msg   = module.FabricateDemandMessage(base_name, bound_types, demand_retract)   or reject
    d_local = module.FabricateDemandLocal(base_name+"_local", bound_types)            or reject

    # Step 6: mint the demand seed graph.
    d_io   = mint IO(d_msg.decl); recv = mint SELECT over d_io with `arity` columns
    d_rel  = register relation object for d_local.decl (inserts/selects stay empty)
    root_head   = mint TUPLE forwarding recv's columns
    root_member = mint TUPLE forwarding root_head's columns   # the "insert" proxy shape
    d_members = [root_member]
    for read in dedup(a.pushdown_reads):                       # one propagation member per demanding subgoal
      proj = mint TUPLE projecting read.columns[p_bound[i]] for i in 0..arity
      prop_member = mint TUPLE forwarding proj
      d_members.push(prop_member)
    d_merge  = mint MERGE(d_members)                            # ALWAYS a MERGE, even single-member
    d_reader = mint TUPLE(full-width reader over d_merge)        # recipe N2: the only derived-d_p read ctor

    forcing_index    = demand_forcings.size()
    first_annotation = guard_annotations.size()

    # Step 7: guard every rule-body site.
    for site in a.sites:
      guard, out_for_pos = MintGuardJoin(site.read, d_reader, site.pivot_pos)
      guard.guard_annotation_index = guard_annotations.size(); guard.query = this
      guard_annotations.push(GuardAnnotation{site.kind, kDReader, kBody,
                                              is_instance_key=false, site.pivot_pos,
                                              site.read, d_reader, forcing_index})
      restore = (site.kind != kReadAtTuple) ? MintRestoringTuple(site.read, out_for_pos) : null
      pending.push(PendingRewire{site.consumer, site.read, out_for_pos, guard, restore, site.kind})

    # Step 8: the query-projection guard (a FRESH raw_seed, distinct from d_p's root member).
    raw_seed = mint TUPLE forwarding recv's columns
    guard, out_for_pos = MintGuardJoin(a.q_read, raw_seed, a.p_bound)
    guard.guard_annotation_index = guard_annotations.size(); guard.query = this
    guard_annotations.push(GuardAnnotation{kReadAtTuple, kRawSeed, kQueryProjection,
                                            false, a.p_bound, a.q_read, raw_seed, forcing_index})
    pending.push(PendingRewire{a.q_consumer, a.q_read, out_for_pos, guard, null, kReadAtTuple})

    # Step 8b: register the recognition unit (one per forcing).
    recognized_subgraphs.push(RecognizedSubgraph{forcing_index, p_merge, a.p_bound, q_insert,
                                                  guard_indices=[first_annotation..guard_annotations.size()),
                                                  p_demanded_decl})

    # Step 9: TRIPWIRE — d_merge must have a member reaching recv, else abort (compiler-bug guard).
    assert some d_merge.merged_views[*] traces via TUPLE.input_columns[0] chain to recv

    # Step 10: register the forcing; closes the fabrication window.
    demand_forcings.push(QueryDemandForcing{a.redecl, d_msg, a.bound_indices})

  # === R-DUP: grouped rewire (deferred so N adornments sharing a (consumer,read)
  #     don't double-rewire and orphan a guard) ===
  group pending by (consumer, read):
    if group.size() == 1:
      g = group[0]
      if g.kind == kReadAtTuple: RewireConsumer(g.consumer, g.read, g.out_for_pos, g.guard)
      else:                      RewireConsumer(g.consumer, g.read, g.restore.columns, g.restore)
    else:                                         # MULTI: union the restored (read-schema) members
      members = [pr.restore or MintRestoringTuple(pr.read, pr.out_for_pos) for pr in group]
      union_merge = mint MERGE(members) over g0.read's schema     # "demand/guard-union"
      RewireConsumer(g0.consumer, g0.read, union_merge.columns, union_merge)

  # === Step 11: always-on census belts (pre-Optimize only) ===
  assert (# live views with guard_annotation_index set) + guard_annotation_folded_count
         == guard_annotations.size()                              # OWN-3
  assert recognized_subgraphs.size() == demand_forcings.size()    # OWN-3
  assert every forcing with >=1 stamped guard has >=1 role==kBody guard   # OWN-3/g8

  return true
```

## FabricateDemandMessage / FabricateDemandLocal (lib/Parse/Demand.cpp)

```
ParsedModule::FabricateDemandMessage(name, param_types, differential):
  LexInternedAtom(name) -> name_tok      # else nullopt
  if any existing message has (name, param_types.size()): return nullopt   # G3 collision
  msg = declarations.CreateDerived<ParsedMessageImpl>(kMessage); modules.messages += msg
  stamp name/name_view/directive_pos/rparen
  if differential: msg.differential_attribute = Token::Synthetic(kPragmaDifferential)   # -demand-retract channel
  FabricateParams(msg, param_types)
  return ParsedMessage(msg)

ParsedModule::FabricateDemandLocal(name, param_types):
  LexInternedAtom(name) -> name_tok      # else nullopt
  if any existing local has (name, param_types.size()): return nullopt     # G3 collision
  local = declarations.CreateDerived<ParsedLocalImpl>(kLocal); module.locals += local
  stamp name/name_view/directive_pos/rparen
  FabricateParams(local, param_types)
  return ParsedLocal(local)
```


### Rel nested keyed lowering (BuildSubgraphInstanceOps + ResolveLiveRecognition, lib/Rel/Rel.cpp)

```
# ============================================================
# ResolveLiveRecognition(impl, query) -> LiveRecognition   [Rel.cpp:936-1025]
# ============================================================
# WHY: a RecognizedSubgraph's stored QueryView handles (minted by the
# DataFlow demand pass, pre-Optimize) DANGLE after Optimize/CSE. Every
# consumer must re-derive the {demand,input,pub} tables from LIVE state:
# the CSE-migrating GuardAnnotationIndex stamp on surviving views, plus
# parse-level (ParsedDeclaration::Id()) identity for the pub table.

struct ResolvedInstance:
    ok: bool = false
    demand_table, input_table, pub_table: TABLE* = null
    demanded_view, pub_view, input_view: optional<QueryView>
    pub_ncols: unsigned
    input_key_cols: vector<unsigned>   # input columns bound to the instance key

function ResolveLiveRecognition(impl, query) -> map<forcing_index, ResolvedInstance>:
    annots   = query.GuardAnnotations()
    forcings = query.DemandForcings()
    model_table(v) = impl->view_to_model[v]->table   # null if v has no model

    # 1. Bucket every LIVE view carrying a GuardAnnotationIndex by its
    #    forcing_index (a view with kNoGuardAnnotation is not a guard).
    guards: map<forcing_index, list<(QueryView, annot_idx)>> = {}
    for v in query.ForEachView():
        ai = v.GuardAnnotationIndex()
        if ai == kNoGuardAnnotation: continue
        guards[annots[ai].forcing_index].append((v, ai))

    for (fidx, bucket) in guards:
        ri = ResolvedInstance{}
        for (v, ai) in bucket:
            if not v.IsJoin(): continue
            jl = QueryJoin::From(v).JoinedViews()   # ordered list
            if len(jl) < 2: continue                # MintGuardJoin always pairs {demand_side, read}
            # joined[0] is always the demand side (traces to the fabricated
            # demand__ message); record its model unconditionally.
            if model_table(jl[0]) != null: ri.demand_table = model_table(jl[0])
            # Only a kBody-role guard's joined[1] is the real summarized
            # monotone INPUT (a kQueryProjection-role guard's joined[1] is an
            # intermediate copy, not the input) -- take the FIRST kBody hit.
            if annots[ai].role == kBody and ri.input_table == null:
                if model_table(jl[1]) != null:
                    ri.input_table    = model_table(jl[1])
                    ri.input_view     = jl[1]
                    ri.input_key_cols = annots[ai].instance_key   # pivot positions
                    ri.demanded_view  = v                          # live handle for render

        # 2. pub_table: the live INSERT whose FULL parse identity (decl.Id(),
        #    embeds name+arity) equals forcings[fidx].query -- never a
        #    name-only match (would silently bind a same-name/different-
        #    arity sibling's table and partition a mismatched-width row).
        if fidx < len(forcings):
            q_decl = ParsedDeclaration(forcings[fidx].query)
            for ins in query.Inserts():
                if ins.Declaration().Id() != q_decl.Id(): continue
                iv = QueryView::From(ins)
                if model_table(iv) != null:
                    ri.pub_table = model_table(iv)
                    # name the published columns off the INSERT's first
                    # predecessor (an INSERT view has no own Columns()).
                    ri.pub_view  = first(iv.Predecessors(), default=iv)
                    ri.pub_ncols = len(ri.pub_view.Columns())
                    break

        if ri.demanded_view == null and bucket not empty:
            ri.demanded_view = bucket[0].view   # fallback handle for render only

        ri.ok = ri.demand_table and ri.input_table and ri.pub_table
                and ri.demanded_view and ri.pub_view and ri.input_view
        out[fidx] = ri
    return out


# ============================================================
# BuildSubgraphInstanceOps(flow, impl, context, query, scc_map)  [Rel.cpp:1038-1200]
# ============================================================
# Gated: only called under context.demand_instance_enabled (-demand-instance
# or RP-9 pragma-selected nested arm). Flag-off, this body never runs, so the
# stored (possibly dangling) RecognizedSubgraph QueryView handles are never
# dereferenced off-gate.

function BuildSubgraphInstanceOps(flow, impl, context, query, scc_map):
    lr = ResolveLiveRecognition(impl, query)

    for rs in query.RecognizedSubgraphs():
        ri = lr.by_forcing.get(rs.forcing_index)
        if ri == null or not ri.ok:
            continue   # fully-dead forcing (every guard eliminated by CSE) -- ABA-safe skip

        pub_table, demand_table, input_table = ri.pub_table, ri.demand_table, ri.input_table

        # ---- HP-4 refusal belt (defense in depth; Demand.cpp's body walk
        #      already rejects these upstream -- this trips LOUD only if a
        #      future relaxation reopens an alpha-through-functor path) ----
        if ri.input_view.IsMap() or ri.input_view.IsNegate()
           or ri.input_view.IsAggregate() or ri.input_view.IsKVIndex():
            ValidatorFail("recognized-subgraph input is a MAP/NEGATE/AGG model (HP-4)")

        diff       = TableIsDifferential(pub_table)        # P-STORE
        input_diff = input_table and TableIsDifferential(input_table)  # 3rd axis, never folded into diff
        sid        = len(flow.instances)                    # this store's instance_store_id

        inst_desc = DRInstance(ri.demanded_view, ri.pub_view)
        inst_desc.differential   = diff
        inst_desc.forcing_index  = rs.forcing_index
        inst_desc.forcing_name   = query.DemandForcings()[rs.forcing_index].query.NameAsString()
            # (fprintf+abort if forcing_index is out of range -- never guess)
        inst_desc.pub_table, inst_desc.demand_table, inst_desc.input_table = pub_table, demand_table, input_table
        inst_desc.key_cols = rs.key_cols                     # <-- KEY_COLS SOURCE: RecognizedSubgraph.key_cols,
                                                               #     minted by the DataFlow demand pass (D1.a),
                                                               #     NOT re-derived here -- only the tables/views are.
        npub = ri.pub_ncols                                   # live width, never the (possibly stale) rs width
        keyset = set(rs.key_cols)
        inst_desc.row_cols = [p for p in 0..npub if p not in keyset]
        flow.instances.append(inst_desc)

        # ---- mint kSubgraphInstantiate: birth/rebuild + band-(b) publish ----
        inst = DROp(kSubgraphInstantiate)
        inst.ctx = kSeed
        inst.table_op_table = pub_table;  inst.table_op_sign = +1
        inst.demand_table = demand_table; inst.input_table = input_table
        inst.demanded_view = ri.demanded_view
        inst.instance_store_id = sid;     inst.forcing_index = rs.forcing_index
        inst.effects = InstantiateEffects(diff, input_diff, pub_table, demand_table, input_table)
            # drains: demand net-additions, input net-additions,
            #         [if input_diff] input net-removals (R-A2-TRIGGER);
            # + frozen demand-key read (HP-8, no hazard); + rederive-leaf
            #   Present(input) flag-read (renders on `reads:`); + the
            #   collapsed kInstanceRebuild/+1, kStateEmit(pub read),
            #   kStateOld(pub read) triad; + per-sign counter/InI-crossing/
            #   vec-append pairs if diff, else one +1-only counter (R-MONO)

        # per-published-position alpha decision (V-ALPHA arm B multiset):
        for p in 0..npub:
            inst.context_cols.append(p)
            inst.context_col_sources.append(
                kInstanceKeySlot if p in keyset else kRowSlot)   # <-- THE DECISION:
                # keyset = rs.key_cols; every published position that is one
                # of the declared/inferred instance-key columns is tagged
                # kInstanceKeySlot (must pair with lowering in {kPointTest,
                # kSectionWalk} per V-ALPHA arm A), every other published
                # position (the residual row payload) is kRowSlot.

        # ---- the Rederive body spine ("kSectionWalk", the LABEL) ----
        arm = DRArm(sign = +1)
        access = PlanNode(kind = kAccess)
        access.table = input_table
        access.pred  = kPresent
        access.lowering = kSectionWalk       # <-- CLAIMS an indexed idx.First/Next
                                              #     keyed walk + pivot re-test (the
                                              #     Lowering enum's OWN stated meaning,
                                              #     Rel.h:480-483).
        access.ctx = kSeed
        access.bound_cols = ri.input_key_cols            # input cols joined to the instance key
        access.bound_col_sources = [kInstanceKeySlot] * len(ri.input_key_cols)
        fold = PlanNode(kind = kFold, fold_table = pub_table, fold_sign = +1,
                        fold_class = kNonRecursive)
        access.child = fold
        arm.body = access
        inst.arms.append(arm)
        flow.ops.append(inst)

        # ---- mint kInstanceDeath (only if the demand relation is differential) ----
        if demand_table and TableIsDifferential(demand_table):
            death = DROp(kInstanceDeath)
            death.ctx = kSeed
            death.table_op_table = pub_table; death.table_op_sign = -1   # OD-2: same table_id, sorts before the +1 instantiate
            death.demand_table = demand_table; death.demanded_view = ri.demanded_view
            death.instance_store_id = sid; death.forcing_index = rs.forcing_index
            death.effects = DeathEffects(pub_table, demand_table)  # zero counter/emit/InI/append effects (§18(B))
            flow.ops.append(death)

        # ---- mint kInstanceSeal (always, one per forcing) ----
        seal = DROp(kInstanceSeal)
        seal.ctx = kSeed; seal.table_op_table = pub_table
        seal.instance_store_id = sid; seal.forcing_index = rs.forcing_index
        seal.effects = [SealEffect(pub_table)]   # sealed := current, sign 0
        flow.ops.append(seal)

        # ---- stratum seed: lift above both drained tables' ready strata ----
        flow.instance_stratum[sid] = 1 + max(ready_after(demand_table), ready_after(input_table))


# ============================================================
# WHY kSectionWalk IS DISHONEST  (Rel.h Lowering enum vs. actual codegen)
# ============================================================
# Rel.h:480-483 defines Lowering as an IDENTITY claim about the emitted
# access shape:
#   kPointTest   = Find() + predicate test (O(1) indexed)
#   kSectionWalk = idx.First()/Next() + pivot re-test  (indexed RANGE walk
#                  over an index keyed on bound_cols)
#   kFullScan    = zero bound columns, linear scan
# The Rederive arm above sets access.lowering = kSectionWalk and populates
# bound_cols = ri.input_key_cols with source kInstanceKeySlot -- i.e. it
# CLAIMS the input table is walked via an index keyed on the instance key.
#
# But the actual codegen mold (Database.cpp emit_instance_rescan,
# 2434-2494), shared by band-(a1) birth, band-(a2) edge-add rebuild and
# band-(a2') edge-removal rebuild, is:
#
#   for s in 0 .. input_member.NumRows():          # <- LINEAR scan of EVERY
#       ir = input_member.RowAt(s)                 #    physical row, no index
#       if ir.<key_field> == <bound key expr>       #    touched at all
#          [and (input_diff ? input.Present(s) : true)]:
#           cur.TryAdd(Row{...})
#
# There is no idx.First/Next call anywhere in this mold -- it is a full
# scan with an inline equality filter (the comment at Database.cpp:2339-
# 2341 admits it plainly: "a full scan with a key filter -- the keyed
# index is a deferred perf refinement; the DR spine already tags
# section-walk"). So the DR-IR PlanNode says kSectionWalk (an indexed
# access) while the physical emission is kFullScan in every real sense --
# the V-ALPHA belt (Rel.cpp ~4413-4490) only checks that a kInstanceKeySlot
# column pairs with lowering in {kPointTest, kSectionWalk}, i.e. it
# validates the LABEL is self-consistent, never that the label matches
# what codegen actually emits. This is the "THE LIE" the seed flags.
```