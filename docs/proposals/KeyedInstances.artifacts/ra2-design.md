======================================================================
COMMITTED AT THE R-a2 LANDING (2026-07-21; the first Rel-epoch slice,
OD-12). The BINDING adjudicated implementation contract: designer +
3 adversarial critics (the model/grammar critic recovered from disk
after a structured-output death) + xhigh adjudicator; amendments
ADJ-R1..R9 applied IN PLACE (R8/R9 adjudicated personally by the
orchestrator at the code). The Fable-review fix set (see §20(G) and
ra2-desired-states.md) SUPERSEDES §A.1.3's pinned single-drain
V-INST-EFFECT count (now source-aware) and the ds §3.3 option-(a)
Format choice (the printer now renders the input frontier).
======================================================================

# R-a2 DESIGN — the band-(a2) edge-frontier drain (ritual stage b)

The FIRST deliverable of the §9 "DeltaRel → Rel" epoch (owner OD-12, §20(F)),
formulated as a DIFF on the stage-a build-out pseudocode. Every anchor below is
a real file:line at tip `8a03339d` (I re-read the cited code; the build-outs are
faithful — divergences are flagged inline). Terminology: `[R-REBUILD-a2]`,
ADJ-C1 (the full-rescan collapse), ADJ-G1 (the effects fork), ADJ-C3 (why
a1-only was the silent gap) are the d2b-design binding marks.

Scope, in one sentence: give the OD-4 provisioned-but-UNDRAINED monotone
edge-input `kNetAdditions` frontier its FIRST consumer — a second drain in the
SUBGRAPH_INSTANTIATE lowering that, for each new edge row whose instance key is
LIVE-DEMANDED, full-rescans that key exactly as band-(a1) does (never an
incremental TryAdd). This un-gaps edge-after-demand and un-retires the witness
REBUILD batch.

Binding decisions taken here (the §OPEN-QUESTIONS the build-outs raised):
- **[D-EPOCH] R-a2 covers SAME-EPOCH AND CROSS-EPOCH edge-after-demand.**
  [ADJ-R2 — ADJUDICATED UPHELD (LOW-2, crit-correctness [D-EPOCH]).] The
  original text ("does NOT clear the edge frontier … cross-epoch re-drain of
  stale edges … O(edges·epochs)") was FALSIFIED by this design's own A.4.4 and
  by the generated code (verified: proc_19 declares `vec25(allocator)` FRESH
  each call, fills it if-crossed with only THIS batch's new rows, and moves it
  into flow_46). The frontiers are ENTRY-PROC-LOCAL, minted fresh PER EPOCH —
  they do NOT accumulate and there is NO cross-epoch re-drain of stale edges.
  Each epoch a2 drains exactly this epoch's NEW edges: **O(new-edges-this-epoch
  · input-scan)**, never O(edges·epochs). "Cross-epoch edge-after-demand" means
  only "an edge whose key was demanded in an EARLIER epoch" — that edge is a
  fresh row in ITS epoch's frontier, rescanned once. Correctness holds by the
  frozen⊆current belt + (F,T) republish idempotence. This is the "R-MONO ships
  the mechanism, incrementality is a later Rel slice" posture, consistent with
  the epoch brief §4. (The prior "does NOT clear" instruction is retired; A.2.3
  removes proc_19's `vec25.Clear()` by threading vec25 as a moved param — the
  frontier is consumed by the drain, not cleared. Follow A.4.4, not the struck
  cost model.)
- **[D-COLLAPSE] ONE rescan per (key, epoch), TWO drain SOURCES** (ADJ-C1). a1
  and a2 are two ways a key enters the touched/rescan set, gated by
  `!TouchedFlag(iid)`; a co-demanded-and-edge-touched key rescans exactly once
  (§2, §D).
- **[D-DRAIN-SECOND] the a2 edge drain is pushed SECOND** in the effect vector
  and emitted SECOND in the region body. [ADJ-R9 — ADJUDICATED UPHELD (LOW,
  crit-model F-3): the original rationale ("Format `gu_input_table` keys on
  the first drain") is INAPPLICABLE — `gu_input_table` (Format.cpp:527-534)
  is a GROUP_UPDATE-only helper (called only at :678/:686), never invoked for
  the instantiate op, which renders `input=` from `op.input_table` directly
  (Format.cpp:789); V-INST-DRAIN reads op FIELDS, never effects. The honest
  rationale is PUSH-ORDER DETERMINISM + DUMP READABILITY: emit_effects renders
  in push order (:447-449), so demand-first keeps the dump stable and the
  d2b-desired-states §2.3 fork byte-predictable.] (§1.1, §A.1).
- **[D-NO-RUNTIME] ZERO runtime-header change** — `FindInstance`/`kNoInstance`
  already exist at tip (§3); a new unit-test arm is ADDED (§3.2), not a new
  method.
- **[D-HP17] HP-17 UNTOUCHED** — the death op stays minted-OFF; the R-DIFF arm
  of V-INST-EFFECT is updated to `drains==2` for consistency with the single
  `InstantiateEffects` source, but no death lowering is added (§A.1, §F).

======================================================================
(A) THE DIFF, SUBSYSTEM BY SUBSYSTEM
======================================================================

No new op kinds. No new census keys. No new dep edges. The whole diff is:
one pushed effect, three relaxed hand-counts, one resolved-and-wired vector,
one added band in codegen, one classify entry, and the witness/doc churn.

----------------------------------------------------------------------
A.1 THE DELTAREL MINT SIDE — the ADJ-G1 effects fork
----------------------------------------------------------------------

[ADJ-R8 — ADJUDICATED UPHELD (LOW, crit-model F-1, model-honesty; anchors
orchestrator-re-verified at DeltaRel.cpp:3480-3520/3685-3699,
Procedure.cpp:258-266).] THE EFFECTS FORK IS DUMP + HAND-COUNT ONLY — IT
CARRIES NO WIRING GUARANTEE. No validator ties any kVecDrain effect's
`value_table` to the op fields: V-INST-EFFECT COUNTS drains (drains==2)
without inspecting their target tables; V-INST-DRAIN checks that
`op.demand_table` (and, with A.1.3, `op.input_table`) are PROVISIONED in
`context.table_delta_vecs` — op fields, never effects; and the LOWERING
resolves its vectors from `op->demand_table`/`op->input_table` directly,
never from `op.effects`. A future edit that pushed the wrong `value_table`
would pass every validator and lower correctly while the `.deltarel` dump
silently lied. This is a PRE-EXISTING asymmetry (the demand drain's
value_table is equally unchecked); a2 adds no new weakening — but the
ritual must NOT rest any answer-identity argument on the effects fork.
Answer identity rests on the LOWERING + the witness/eqgate; the fork is
observability. (A value_table↔op-field cross-check is a candidate future
validator, NOT in this slice's scope.)

### A.1.1 `InstantiateEffects` gains the second `kVecDrain`
`lib/DeltaRel/DeltaRel.cpp:791-858` — the SOLE authority for the mint's effect
multiset. Insert the edge drain IMMEDIATELY AFTER the demand drain (push order
`[demand-drain, EDGE-DRAIN, demand_read, leaf, rebuild, emit, old, counters]`),
so the first kVecDrain stays the demand drain ([D-DRAIN-SECOND]):

```diff
 static std::vector<DREffect> InstantiateEffects(bool diff, TABLE *pub,
                                                 TABLE *demand, TABLE *input) {
   std::vector<DREffect> fx;
   DREffect drain;  // OD-7 frontier drain (a LOWER-time TableDeltaVector, OD-R7)
   drain.kind = EffKind::kVecDrain;
   drain.value_table = demand;
   drain.vec_role = VecRole::kNetAddition;
   fx.push_back(drain);
+
+  // [R-REBUILD-a2] the input(edge) net-additions REBUILD drain (ADJ-G1). The
+  // mechanism-natural OD-4 edge frontier's first consumer: band-(a2) rescans a
+  // live-demanded key whose input changed. Pushed SECOND for push-order
+  // determinism / dump stability (ADJ-R9 — no reader keys on drain order).
+  // Un-minted monotone frontier -> ResolveVecIdx ~0u -> NO dep edge (§2.4).
+  DREffect edge_drain;
+  edge_drain.kind = EffKind::kVecDrain;
+  edge_drain.value_table = input;
+  edge_drain.vec_role = VecRole::kNetAddition;
+  fx.push_back(edge_drain);
 
   DREffect demand_read;  // NEW: frozen read of the demand key, no hazard (HP-8)
   ...
```

This is UNCONDITIONAL (outside the `if (diff)`), so both the R-MONO and the
(inert) R-DIFF arm now carry two drains — the diff arm carries the same two
frontiers, and keeping one source avoids a stale latent hand-count trap for
D3.a (build-out §OQ-4, recommendation adopted). CONSEQUENCE: `DeathEffects`
(`:860-883`) is UNTOUCHED — HP-17, death stays minted-OFF ([D-HP17]).

### A.1.2 `V-INST-EFFECT` — relax `drains == 1u` to `drains == 2u` (BOTH arms)
`lib/DeltaRel/DeltaRel.cpp:3510-3516`. The independent hand-count of the same
totality; it MUST move in lock-step with A.1.1 or it aborts on the mint it is
meant to certify. The `ok` predicate for `kSubgraphInstantiate`:

```diff
   const bool ok =
-      drains == 1u && demands == 1u && leaves == 1u && rebuilds == 1u &&
+      drains == 2u && demands == 1u && leaves == 1u && rebuilds == 1u &&
       rebuild_sign == 1 && emits == 1u && olds == 1u &&
       (diff ? (counters == 2u && counter_signs == 0 &&
                crossings == 2u && appends == 2u)
             : (counters == 1u && counter_signs == 1 &&
                crossings == 0u && appends == 0u));
```

`drains` is counted before the `diff` split (`:3486`), so this single edit
covers both regimes. The failure message at `:3517-3519` ("not the §3.3
regime-split totality") is unchanged — still accurate.

NOTE — no OTHER instantiate hand-count moves: `demands/leaves/rebuilds/emits/
olds/counters` are all still 1 (R-MONO), the `default:` reject at `:3505-3507`
still rejects any effect kind outside the set (kVecDrain is in the set), and
`V-INST-SOLE` (`:3521-3526`, input not differential / not aliasing pub) is
a2-invariant.

### A.1.3 `V-INST-DRAIN` — ALSO require the input frontier provisioned (HP-2)
`lib/DeltaRel/DeltaRel.cpp:3685-3699`. Today asserts the demand table has a
non-null `kNetAdditions` entry in `context.table_delta_vecs`. Under a2 the edge
drain must likewise resolve, else a2 would range a null/absent vec and silently
rebuild nothing — negative-space insurance (the edge frontier IS provisioned by
OD-4 `Build.cpp:999-1004` for the recognizer cut, so this holds by
construction). d2b-design §3.3:618 pre-registered this ("ALSO assert the
input(edge) kVecDrain resolves"):

```diff
   for (const DROp &op : flow.ops) {
     if (op.kind != DROpKind::kSubgraphInstantiate) {
       continue;
     }
     auto it = context.table_delta_vecs.find(op.demand_table);
     const bool ok =
         it != context.table_delta_vecs.end() &&
         it->second.count(static_cast<unsigned>(VectorKind::kNetAdditions)) &&
         it->second.at(static_cast<unsigned>(VectorKind::kNetAdditions)) !=
             nullptr;
     if (!ok) {
       ValidatorFail("V-INST-DRAIN: an instantiate's demand net-additions "
                     "frontier was never provisioned (OD-7/§2.2 gap)");
     }
+    // [R-REBUILD-a2] the edge(input) frontier drain must ALSO resolve.
+    auto et = context.table_delta_vecs.find(op.input_table);
+    const bool edge_ok =
+        et != context.table_delta_vecs.end() &&
+        et->second.count(static_cast<unsigned>(VectorKind::kNetAdditions)) &&
+        et->second.at(static_cast<unsigned>(VectorKind::kNetAdditions)) !=
+            nullptr;
+    if (!edge_ok) {
+      ValidatorFail("V-INST-DRAIN: an instantiate's input(edge) net-additions "
+                    "frontier was never provisioned (OD-4/R-a2 gap)");
+    }
   }
```

### A.1.4 NO census / deps / ResolveVecIdx consequence
- **census**: a kVecDrain is an EFFECT, never an op; `kSubgraphInstantiate`
  stays 1 (op-count at the census switch, unaffected by effect contents).
- **deps**: the dep builder's kVecDrain case (`:4017-4020`) calls
  `ResolveVecIdx` (`:3796` region) which returns `~0u` for the edge table —
  `flow.table_vecs` is populated ONLY by `MintTableVec`, called ONLY for
  DIFFERENTIAL non-induction-owned tables (`:1289-1290`). The edge/input table
  is MONOTONE ⇒ never minted ⇒ `~0u` ⇒ `add_vec_access` early-returns
  (`:3994-3996`) ⇒ NO vec-access, NO use-edge, NO RAW hazard. **The second
  drain is DECORATIVE in the dependence graph** exactly as the first is; deps
  stay 6. The real vector is resolved and wired at LOWER time by
  `TableDeltaVector` (§A.2), not through the flow graph.
- **`gu_input_table`** (`Format.cpp:527-534`, "first drain's value_table") is a
  GROUP_UPDATE-only helper; it is not invoked for the instantiate header (which
  renders `input=` from `op.input_table` fields). [D-DRAIN-SECOND] makes it
  robust even if that ever changed: first drain = demand, unchanged.

----------------------------------------------------------------------
A.2 THE LOWERING — band-(a2) resolve + wire (the ADJ-C1 collapse)
----------------------------------------------------------------------

The lowering is TWO pieces: the CFG builder `LowerSubgraphInstances`
(`Procedure.cpp:249`) which mints ONE SUBGRAPHINSTANCE region per op and stashes
handles, and the codegen `EmitSubgraphInstance` (`Database.cpp:2286`, §A.4)
which renders the C++ band bodies. The region carries NO child bands — the band
structure lives entirely in codegen. So the builder's a2 job is purely to
RESOLVE the edge frontier vector and WIRE it onto the region; the collapse
mechanism (one rescan routine, two drain sources) is realized in codegen.

### A.2.1 Region class — add the `input_frontier` UseRef + accessor
`lib/ControlFlow/Program.h:1173` (impl) and
`include/drlojekyll/ControlFlow/Program.h:848` (public):

```diff
   UseRef<VECTOR> demand_frontier;  // BAND (a1) drain source (birth keys)
+  UseRef<VECTOR> input_frontier;   // BAND (a2) drain source (edge REBUILD keys)
   UseRef<TABLE> input_table;       // the summarized monotone input
   UseRef<TABLE> pub_table;         // the published answer relation
```

Public accessor (`include/.../Program.h`, beside `DemandFrontier`):
```diff
   // BAND (a1) demand net-additions frontier (birth keys).
   DataVector DemandFrontier(void) const noexcept;
+  // BAND (a2) input(edge) net-additions frontier (REBUILD keys). [R-REBUILD-a2]
+  DataVector InputFrontier(void) const noexcept;
```
Definition in `lib/ControlFlow/Program.cpp` (beside `:752 DemandFrontier`):
```cpp
DataVector ProgramSubgraphInstanceRegion::InputFrontier(void) const noexcept {
  return DataVector(impl->input_frontier.get());
}
```

### A.2.2 `LowerSubgraphInstances` — resolve the edge frontier, Emplace it
`lib/ControlFlow/Build/Procedure.cpp:258-265`. `TableDeltaVector` is memoized
(`Build.cpp:745-747`), so this returns the IDENTICAL `VECTOR*` the eager walk's
OD-4 append (`Build.cpp:1001-1003`) wrote into — a2 consumes an EXISTING vec, it
provisions NOTHING.

> [ADJ-R6 — ADJUDICATED (F4 REFUTED, strengthened).] The memoization is PROVEN,
> not merely asserted. (1) BUILD ORDER: the eager receive descent
> (`Procedure.cpp:108` → `BuildEagerInsertionRegions` → the OD-4 net-additions
> append at `Build.cpp:1003`) runs BEFORE `LowerSubgraphInstances`
> (`Procedure.cpp:382`) in the same proc build — so the eager append mints the
> input frontier FIRST; a2's `TableDeltaVector(input_table)` is always a
> memoized re-fetch, never the first mint, so the vector id never shifts.
> (2) TIP DEMAND-SIDE PROOF: `LowerSubgraphInstances` ALREADY calls
> `TableDeltaVector(demand_table, kNetAdditions)` at tip (`:259`), and the
> generated code carries a SINGLE `vec29` — had that call minted fresh there
> would be two demand vectors. The input side is the byte-identical mechanism
> on the same table class. F4's stage-e id-check (dump the nested `.ir`/`.h`,
> confirm the input-frontier id == tip `vec25`) is subsumed by the §B.2 review.

```diff
     VECTOR *const demand_front =
         TableDeltaVector(impl, context, op->demand_table,
                          VectorKind::kNetAdditions);
+    // [R-REBUILD-a2] the SAME memoized edge frontier the eager cut-successor
+    // append (Build.cpp:999-1004) writes into — a2 drains it, provisions none.
+    VECTOR *const input_front =
+        TableDeltaVector(impl, context, op->input_table,
+                         VectorKind::kNetAdditions);
 
     SUBGRAPHINSTANCE *const si =
         impl->operation_regions.CreateDerived<SUBGRAPHINSTANCE>(seq, sid);
     seq->AddRegion(si);
     si->demand_frontier.Emplace(si, demand_front);
+    si->input_frontier.Emplace(si, input_front);   // [R-REBUILD-a2]
     si->input_table.Emplace(si, op->input_table);
```

`input_key_cols` / `input_row_cols` (`:270-288`) are UNCHANGED — a2 reuses the
exact same key filter + row payload the a1 rescan uses (the collapse: same
rescan mechanism, different drain source). `V-INST-EMITTED` enrollment
(`:292-295`) is UNCHANGED — a2 emits no new op kind; the region still emits
`{kSubgraphInstantiate, kInstanceSeal}`.

### A.2.3 Vector classify for procedure extraction — mark `input_frontier` read
`lib/ControlFlow/Build/Procedure.cpp:183-189`, the `kSubgraphInstance` case of
the read/write vector collector that `ExtractPrimaryProcedure` consults to
thread vectors into the primary data-flow proc. Today it marks
`demand_frontier` read; a2 must ALSO mark `input_frontier` read, else the drain
reads an out-of-scope vector:

```diff
       case ProgramOperation::kSubgraphInstance: {
         auto *si = op->AsSubgraphInstance();
         if (vec == si->demand_frontier.get()) {
           read.insert(vec);
         }
+        if (vec == si->input_frontier.get()) {
+          read.insert(vec);   // [R-REBUILD-a2] band-(a2) drains it (read-only)
+        }
         break;
       }
```

The demand and edge frontiers are DISTINCT `VECTOR*`s (distinct tables ⇒
distinct memoized `TableDeltaVector` entries), so both `if`s can fire across the
two vectors this region references. No write is added — a2, like a1, publishes
into the pub table + store, never into a vector.

### A.2.4 The ADJ-C1 collapse, stated precisely
There is ONE rescan emitter (the `for s in 0..input.NumRows()` key-filtered
scan, `Database.cpp:2325-2352`). a1 and a2 are two OUTER loops that each, per
key they surface, run that ONE rescan guarded by `!TouchedFlag(iid)`. a2 does
NOT introduce a second rescan PATH — it introduces a second WAY INTO the shared
rescan (build-out §4.2/§Q2; d2b-design ADJ-C1 §0). An incremental
`TouchCurrent(iid).TryAdd(edgeRow)` is FORBIDDEN: `current` is Reset-to-empty at
the prior Seal (`InstanceStore.h:202`), so a partial add leaves `current ⊊
frozen` and the monotone belt (`:185-192`) SIGABRTs on the false drop
(build-out-runtime §3, pinned by `BeltFiresOnMonotoneDrop`). Full rescan is the
only belt-safe rebuild.

----------------------------------------------------------------------
A.3 RUNTIME — ZERO header change; ONE new unit-test arm
----------------------------------------------------------------------

### A.3.1 The non-adding lookup ALREADY EXISTS ([D-NO-RUNTIME])
`include/drlojekyll/Runtime/InstanceStore.h`:
- `kNoInstance = ~0u` (`:52`).
- `FindInstance(const Key&) const noexcept` (`:99-101`) → `FindInstanceWithHash`
  (`:238`), returns the dense iid or `kNoInstance`. Pure, non-allocating,
  `const`, `noexcept`.

a2's "is this key live-demanded?" gate IS `FindInstance(ek) != kNoInstance`.
**There is NO InstanceStore.h edit in R-a2.** (The task-brief's defensive "if
the lookup is missing, add it" clause resolves to: it is present.) a2 MUST use
`FindInstance`, NEVER `FindOrAddInstance` — the latter would BIRTH an
un-demanded instance for a stray edge and over-materialize, which HP-5 aborts
(build-out-runtime §HEADLINE / §OQ-1).

CAVEAT recorded (build-out-runtime §OQ-1): `FindInstance(ek) != kNoInstance`
means "ever minted", not "demand currently standing". Under R-MONO birth-only,
demand never retires ⇒ the two coincide ⇒ the gate is exact. This is safe for
R-a2. Demand-retirement (where a stale iid could trigger a spurious — but
correctness-safe — rescan into an unread pub) is a D3-era concern, NOT R-a2's
(§F non-goal).

### A.3.2 ADD one unit-test arm to `tests/InstanceStore` (the edge-triggered
rebuild the tip suite does NOT yet pin, build-out-runtime §6/§OQ-4)
The store already SUPPORTS the a2 path with existing primitives, but no unit
locks it. Add `EdgeTriggeredRebuildSealsCleanPublishesBorn` (monotone=true):

```
epoch 1: FindOrAddInstance(key g); TouchCurrent(g); TryAdd 10,20; Seal.
         -> frozen[g]={10,20}, current[g]={}, sealed_occupied[g]=1.
epoch 2 (SIMULATES a2 — no demand seed, edge arrived for standing g):
         iid = FindInstance(g);  ASSERT_NE(iid, kNoInstance);   // gate fires
         ASSERT_FALSE(TouchedFlag(iid));
         cur = TouchCurrent(iid);            // schedules the Seal visit
         cur.TryAdd(10); cur.TryAdd(20); cur.TryAdd(30);   // FULL rescan
         // band-(b) born set: 30 absent from frozen{10,20} -> published born.
         ASSERT_EQ(count rows r in current where frozen.Find(r)==kNoRow, 1);
         Seal();                              // frozen⊆current -> belt PASSES
         ASSERT( frozen[g]=={10,20,30} );
   plus: FindInstance(neverKey) == kNoInstance  (the skip-continue arm).
```

This is the below-eqgate unit witness of the ADJ-C1 collapse: an edge-triggered
FULL rescan seals clean and publishes exactly the born delta, and a stray-key
edge (`kNoInstance`) is a no-op. It uses only existing methods; no CMake target
change (5th ctest target already exists). ctest count STAYS 5 (arm added to the
existing `tests/InstanceStore` binary).

----------------------------------------------------------------------
A.4 CODEGEN — the a2 band renders the SAME molds as a1
----------------------------------------------------------------------

### A.4.1 `EmitSubgraphInstance` — insert band-(a2) between a1 and band-(b)
`lib/CodeGen/CPlusPlus/Database.cpp:2356` (right after the a1 demand-drain
closes at the `// demand drain` brace, before the `// band-(b) PUBLISH`
comment). Read the two new region handles at the top (`:2290` region):

```diff
   const DataVector demand = region.DemandFrontier();
+  const DataVector input_front = region.InputFrontier();   // [R-REBUILD-a2]
   const DataTable input = region.InputTable();
```

The a2 band (mirrors a1 `:2308-2356`, differing ONLY in the outer loop source =
edge frontier, the non-adding `FindInstance` + `kNoInstance` skip, and the key
expr projected from the edge row `e<in_key[j]>` instead of the demand bind
`k<j>`). Emitted C++:

```cpp
// band-(a2) [R-REBUILD-a2] drain the edge net-additions frontier (REBUILD).
for (const auto &[e0, .., e{A-1}] : <VecName(input_front)>) {
  const auto iid = instance_<id>.FindInstance(Key_<id>{ e<in_key[0]>, .. });
  if (iid != ::hyde::rt::kNoInstance && !instance_<id>.TouchedFlag(iid)) {
    if (instance_<id>.WorkingOccupied(iid)) {
      std::fprintf(stderr, "V-INST-FRESH: instance %u current non-empty at "
                   "band-(a) entry (store <id>)\n", iid); std::abort();
    }
    auto &cur = instance_<id>.TouchCurrent(iid);
    for (uint32_t s = 0; s < <input_member>.NumRows(); ++s) {   // SAME rescan
      const auto ir = <input_member>.RowAt(s);
      if (ir.<field[in_key[0]]> == e<in_key[0]> && ..) {        // key filter
        cur.TryAdd(Row_<id>{ ir.<field[in_row[j]]>, .. });
      }
    }
  }
}
```

The code that generates this is the a1 emitter with three substitutions
(`VecName(input_front)` for `VecName(demand)`; `FindInstance` + the
`iid != kNoInstance` conjunct for `FindOrAddInstance`; the key expr `e<in_key[j]>`
for `k<j>` in BOTH the `Key_<id>{...}` ctor and the `cond`).

> [ADJ-R1 — ADJUDICATED UPHELD (LOW-1, crit-correctness arity trap).] BINDING
> IMPLEMENTER PIN: the a2 outer-loop structured binding is the FULL EDGE ROW
> (`[e0 .. e{A-1}]`, arity A = `input.ColumnTypes().size()`), NOT the demand
> key arity. The a1 emitter it is cloned from computes its bind count as
> `key_arity = demand.ColumnTypes().size()` (Database.cpp:2302-2303, == 1 for
> the witness: `for (const auto &[k0] : vec29)`), but the edge frontier is
> `Vec<Tup_u64_u64>` (arity 2). Reusing `key_arity` for the a2 bind →
> structured-binding/arity mismatch → hard compile error. Bind A vars off the
> INPUT arity, THEN project the key via `Key_<id>{ e<in_key[0]>, .. }`. Do NOT
> clone line 2302's `key_arity` for the a2 loop — the divergence is deliberate. Because the key
comes from the edge row's own key columns (`in_key`), not a separate demand
bind, the edge-loop bind vars are `e0..e{A-1}` (edge arity `A`) and the
projected key is `Key_<id>{e<in_key[0]>, ..}`; the rescan's `cond` compares
`ir.field == e<in_key[j]>`. RECOMMENDATION: factor the shared rescan body into a
private `Generator` helper `EmitInstanceRescan(cur, input, in_key, in_row,
keyExprs)` and call it from both a1 (keyExprs = `k<j>`) and a2 (keyExprs =
`e<in_key[j]>`) — the codegen realization of the ADJ-C1 collapse. Optional
(a1/a2 stay byte-identical either way); it just prevents the two rescans
drifting.

### A.4.2 ref-param collection — NO change
`Database.cpp:755-766` (`IsSubgraphInstance` arm of the table/index/statecell/
instance collector) inserts `InputTable().Id()`, `PubTable().Id()`, pub indexes,
and `StoreId()`. a2 needs NO new table here — the input table is ALREADY
collected as the a1 rescan target, and a2 rescans the same table. The edge
frontier VECTOR rides via the A.2.3 read-classify path, not this collector.

### A.4.3 What the a2 region renders as — SAME MOLDS as a1
band-(a2) is a `for`-range over a VECTOR + `FindInstance` + `!TouchedFlag` guard
+ `WorkingOccupied` V-INST-FRESH abort + `TouchCurrent` + the full-scan/key-
filter/`TryAdd` rescan — every mold already present in a1. NO nested
ProgramRegion, NO new runtime symbol, NO new table/index. band-(b) and the Seal
tail (`:2358-2413`) are UNCHANGED: they iterate `Touched()`, which now includes
a2-touched keys automatically.

### A.4.4 THE VECTOR-THREADING FACT that makes a2 work (verified in generated
code, load-bearing — corrects build-out §3.4)
I compiled the witness nested at tip and read the generated `datalog.h`. The
REAL plumbing (NOT the build-out §3.4 "frontier accumulates every edge"):

- `proc_19` is the shared entry/ingest proc. It folds the received batch:
  edges → `table_11` + the edge frontier `vec25` (if-crossed:
  `if table_11.TryAdd({v23,v24}).added: vec25.Add(...)`); demand → `table_8` +
  the demand frontier `vec29`. It then **`vec25.Clear()`** (line 213 in the
  probe dump) and calls `flow_46(..., std::move(vec29))` — passing ONLY the
  demand frontier. The edge frontier is CLEARED-AND-DROPPED because NOTHING
  reads it (a1-only).
- The reason: `ExtractPrimaryProcedure` (`Procedure.cpp:642-644`) computes
  `primary_params = written_by_entry ∩ read_by_primary`. `vec25` is written by
  entry (the eager fold) but NOT read by primary (the classify at `:183` marks
  only `demand_frontier` read) ⇒ `vec25 ∉ primary_params` ⇒ it hits the
  `try_clear_vec` path (`:687-702`, emit `VECTORCLEAR`). `vec29` (demand,
  read_by_primary) IS a param ⇒ moved into `flow_46`, NOT cleared.

**CONSEQUENCE — A.2.3 is the KEYSTONE.** Marking `input_frontier` read
(A.2.3) puts `vec25` into `written_by_entry ∩ read_by_primary = primary_params`,
which (a) threads it as a `flow_46` parameter (moved from `proc_19`, so band-a2
sees the LIVE edge frontier) and (b) DELETES its `VECTORCLEAR`. Without A.2.3,
A.4.1's band-a2 would range over `flow_46`'s dead empty local `vec25` and
rebuild nothing. This is verified end-to-end below.

> [ADJ-R6 — ADJUDICATED (MED-1 REFUTED, redefinition sub-case closed).] Once
> A.2.3 makes `vec25 ∈ written_by_entry ∩ read_by_primary = primary_params`,
> `ExtractPrimaryProcedure` (`Procedure.cpp:650-670`) puts it in
> `input_vecs` ONLY: loop 1 (`:650-652`) sets `replacements[vec25]` via
> `input_vecs.Create`, and the later read/written-by-primary/entry loops are
> each guarded `if (!replacements.count(vec))` (`:654-670`) → they SKIP vec25,
> so it is NEVER added to `primary_proc->vectors`. Codegen declares locals from
> `proc->vectors` and params from `input_vecs`, so vec25 emits as a param with
> NO local decl — no `param + local` C++ redefinition is possible. PROVEN by
> `vec29`: at tip it is a primary_param and `flow_46` carries NO
> `Vec<Tup_u64> vec29(allocator)` local (generated line 237 = param; the sole
> local, line 238, is the a1-only dead `vec25`). vec25 post-A.2.3 follows the
> identical path. A grep of the regenerated `flow_46` for a duplicate `vec25`
> is a welcome belt but not load-bearing.

**Frontier lifecycle, corrected:** the frontiers are ENTRY-PROC-LOCAL vectors
(fresh each `proc_19` call = each epoch), filled IF-CROSSED with only THIS
epoch's new rows, then moved into `flow_46`. They do NOT accumulate across
epochs (build-out §3.4's accumulation/staleness concern and §OQ-2 are moot: no
cross-epoch re-drain, no O(edges·epochs) redundant rescan). Each epoch a2 drains
exactly this epoch's new edges — O(new-edges-this-epoch) full rescans.

----------------------------------------------------------------------
A.5 THE WITNESS — un-retiring the REBUILD batch (RAT-6 → R-a2)
----------------------------------------------------------------------

I EMPIRICALLY established (compiling the witness both ways at tip, driving
extended drivers) the discrimination facts the witness rests on. See
§A.5.4 for a LOUD RISK the implementer must clear before blessing.

### A.5.1 The a1-only gap is REAL (verified) — flat handles it, nested-a1 drops
With a CLEAN per-epoch structure (one edge per epoch for a STANDING key, probe
that key immediately):
- FLAT `-demand`: probe(1) after `add_edge(1,11)` → `{2,3,4,11}` (correct).
- NESTED `-demand-instance` (a1-only, TIP): probe(1) → `{2,3,4}` — the edge is
  **silently dropped** (band-a1 idle: demand 1 is if-crossed idempotent, so the
  standing instance is never re-touched; ADJ-C3 confirmed executable).
This is the labeled edge-after-demand gap. R-a2's band-a2 (edge-frontier
full-rescan) is exactly the fix.

### A.5.2 The un-retired REBUILD driver + batch (append after the birth probes)
Keep the committed birth phase (two edge batches, probes 1/3/9/5) BYTE-FOR-BYTE.
APPEND a REBUILD phase — **one edge per epoch, probe the standing key
immediately** (this structure is verified to reliably discriminate; the
multi-key-batch + multi-probe-interleave structure does NOT — §A.5.4):

`demand_neighborhood_witness.main.cpp` (append after `probe(5, {})`):
```cpp
  // --- R-a2 REBUILD: edge-after-demand for STANDING keys (un-retires RAT-6). ---
  // Each edge lands in its own epoch AFTER its key's demand is standing; the
  // probe re-injects the (if-crossed) demand and reads the rebuilt answer.
  { hyde::rt::Vec<add_edge_input> e(allocator); e.Add({1, 11});
    add_edge_2(db, log, functors, std::move(e)); }
  probe(1, {2, 3, 4, 11});   // rebuild a multi-neighbor standing key (HP-5: 11 is
                             //   out-of-nbhd for 3/5/9 — a mis-key would leak it).
  { hyde::rt::Vec<add_edge_input> e(allocator); e.Add({3, 7});
    add_edge_2(db, log, functors, std::move(e)); }
  probe(3, {5, 6, 7});       // rebuild another standing key.
  { hyde::rt::Vec<add_edge_input> e(allocator); e.Add({5, 12});
    add_edge_2(db, log, functors, std::move(e)); }
  probe(5, {12});            // rebuild a previously-EMPTY standing instance (5).
  { hyde::rt::Vec<add_edge_input> e(allocator); e.Add({13, 14});
    add_edge_2(db, log, functors, std::move(e)); }
  probe(9, {9});             // (13,14): edge for an UNDEMANDED key. This probe only
                             //   witnesses NO LEAK INTO key 9 (holds whether or not 13
                             //   was mis-birthed — a stray birth is answer-invisible;
                             //   ADJ-R4). The FindInstance-vs-FindOrAdd skip itself is
                             //   pinned by the A.3.2 unit arm, NOT this probe.
```
`.batches` (append a matching third+ epochs for the oracle's full-closure
referee — the oracle is demand-blind, so its neighborhood relation just gains
the four rows (1,11),(3,7),(5,12),(13,14)):
```
batch
+ add_edge 1 11
end
batch
+ add_edge 3 7
end
batch
+ add_edge 5 12
end
batch
+ add_edge 13 14
end
```

### A.5.3 HP-5 discrimination retained (has TEETH)
- **key projection (From, not To):** `add_edge(1,11)` must land in key 1's
  neighborhood; a wrong-column projection would rebuild key 11 and probe(1)
  would MISS 11 → assert `{2,3,4,11}` fails.
- **key filter in the rescan:** probe(1) must be EXACTLY `{2,3,4,11}`; a rescan
  without the key filter would pull ALL edges into key 1 → out-of-neighborhood
  leak → assert fails + golden diverges.
- **FindInstance skip (not FindOrAdd) — NOT witness-discriminated
  [ADJ-R4]:** see the correction block below; the stray-key skip is
  answer-invisible in the witness and is pinned SOLELY by the A.3.2 unit arm.
- **no leakage:** probe(9) stays `{9}` after the (13,14) epoch (this holds
  REGARDLESS of whether the stray key 13 was mis-birthed — it is a no-leak-
  INTO-9 check, not a skip check).
The driver ASSERTS each probe (`assert(nodes == expected)`), so pre-a2 the
nested arm ABORTS (EQGATE-RUN-FAIL) AND diverges on the STANDING-KEY rebuild
probes (1/3/5) — a2 is genuinely gated by THOSE.

> [ADJ-R4 — ADJUDICATED UPHELD (F2, crit-gates).] The original bullet 3
> ("`FindOrAddInstance` misuse would over-materialize; ASAN + belt cover any
> store corruption") OVERSTATES. VERIFIED: the `.dr` keys neighborhood on the
> FROM column, so `add_edge(13,14)` births key 13, which is NEVER probed. A
> mis-used `FindOrAddInstance` on the edge frontier births instance 13 with
> `current={14}`, seals CLEAN (a birth trivially satisfies frozen⊆current — no
> belt fire, no ASAN report), and the stray `(13,14)` sits UNQUERIED in the pub
> table. probe(9) enumerates via `idx_45` keyed on 9 and never sees it; the flat
> golden equally never materializes key 13; so stdout is BYTE-IDENTICAL either
> way and the eqgate cannot observe the misuse. No HP-5 probe asserts on key 13,
> so there is no abort. The FindInstance-vs-FindOrAdd skip distinction is
> therefore BEHAVIORALLY SILENT in the witness — it is covered ONLY by the
> A.3.2 unit arm (`FindInstance(neverKey) == kNoInstance`). Keep the (13,14)
> epoch as a no-leak-into-9 check, but do not claim the witness discriminates
> the skip.

### A.5.4 !!! LOUD RISK — the a1-only pub-read artifact (must clear before bless)
While validating, I found the a1-only nested arm's edge-after-demand answer is
**probe-sequence-dependent** at tip: a MULTI-KEY rebuild batch
(`{(1,11),(3,7),(5,12),(13,14)}` in one epoch) followed by probing 1/3/5/9
makes nested-a1 COINCIDENTALLY return `{2,3,4,11}` == flat (a phantom that is
deterministic per-build but changes with the trailing probe sequence — textbook
stale-slot/index read; ASAN did not flag it as it stays within allocated
capacity). The CLEAN per-epoch structure in §A.5.2 does NOT phantom (reliably
`{2,3,4}` pre-a2). IMPLICATIONS, binding on the implementer:
  (i) USE THE §A.5.2 CLEAN STRUCTURE (one edge/epoch, immediate single probe);
      do NOT use a multi-key rebuild batch — it can mask the gap and make the
      eqgate pass PRE-a2, defeating the witness.
  (ii) STAGE-D GATE (mandatory): confirm the eqgate is RED on the a1-only tip
      compiler with the new driver (nested diverges from the flat golden) and
      GREEN only after landing band-a2. If it is already green pre-a2, the
      witness does not exercise a2 — reshape until it reliably drops.
  (iii) a2's full-rescan writes the CORRECT pub deterministically, so post-a2
      the answer is probe-order-INDEPENDENT (the phantom's stale-read window
      closes — the row is legitimately present). VERIFY under ASAN + the Q5
      ABABAB determinism gate with BOTH the clean and a multi-probe sequence.
  (iv) Root-cause note for the ledger: the phantom is an a1-only latent defect
      (edge-after-demand read from a stale pub/idx slot), ORTHOGONAL to a2's
      mechanism but SUBSUMED by it. Record it; a2 closes it by construction.

> [ADJ-R3 — ADJUDICATED UPHELD (F1 crit-gates + MED-2 crit-correctness).] Two
> corrections + a hardening, binding on the implementer:
> (a) The "not a structural invariant" framing (F1) is REFUTED for the
>     SANCTIONED clean per-epoch structure: the drop is STRUCTURAL, not a
>     stale read. VERIFIED — the demand fold is if-crossed (generated proc_19:
>     `if (table_8.TryAdd({v28}).added) vec29.Add({v28})`), so a standing key's
>     re-asserted demand never re-enters `vec29`; band-a1 is idle in the
>     edge-only epoch; `Touched()` is empty; band-b publishes nothing; the edge
>     is dropped DETERMINISTICALLY (ADJ-C3, owner-ratified). The PHANTOM in
>     §A.5.4 is the stale read, and it appears ONLY in the multi-key batch
>     structure this design already forbids. So the clean structure discriminates
>     STRUCTURALLY; only a deviation to a multi-key batch would silently defeat it.
> (b) LEDGER STAGE-D as a REPRODUCIBLE on-landing command, not a scratch-only
>     step. The landing record MUST carry the exact invocation and its observed
>     RED→GREEN: compile the witness `.main.cpp` against the a1-only tip compiler
>     (`-demand-instance`, all four modes) and confirm the nested arm DIVERGES
>     from the (re-blessed) golden on probes 1/3/5; then confirm GREEN only after
>     band-a2 lands. Do not `--bless` until that transition is recorded.
> (c) HEADER-DOCUMENT in `demand_neighborhood_witness.main.cpp` that the clean
>     ONE-EDGE-PER-EPOCH / IMMEDIATE-SINGLE-PROBE REBUILD structure is
>     LOAD-BEARING for a2 discrimination and MUST NOT be coalesced into a
>     multi-key rebuild batch (which can phantom-mask the gap). Name the reason:
>     if-crossed demand idempotence makes a1-only drop the edge structurally
>     only in this shape.
> (d) STANDING PROTECTION (for the ledger): post-landing the eqgate IS a
>     regression gate — a regressed band-a2 drops the rebuild rows, so the nested
>     arm diverges from the re-blessed golden and the suite goes RED. The residual
>     that STAGE-D closes is purely the LANDING-moment proof that the committed
>     witness actually exercised a2.

### A.5.5 eqgate implications (all four nested modes)
`run_eqgate` recompiles the nested arm in opt/nodf/nocf/none and byte-compares
each to `goldens/demand_neighborhood_witness.stdout`. Post-a2, the REBUILD
probes yield byte-equal flat==nested in all four modes (a2 lowers through
control-flow that `-disable-controlflow-opt` reshapes, so the 4-mode loop —
RAT-9 — genuinely matters here). The eqgate is the STANDING referee that a2
stays answer-identical to flat.

----------------------------------------------------------------------
A.6 DOC / LABEL RETIREMENTS (retire WITH the landing, per OD-12)
----------------------------------------------------------------------

### A.6.1 CLAUDE.md — the edge-after-demand paragraph (§ "-demand-instance")
The paragraph at CLAUDE.md:371-… ("The witness is ENFORCED BIRTH-ONLY (RAT-6)…
EDGE-AFTER-DEMAND … is a LABELED FEATURE GAP …") RETIRES. Replace with the
landed status: the witness now carries a REBUILD phase; edge-after-demand under
`-demand-instance` rebuilds the standing instance via band-(a2) (full-rescan
keyed on the edge frontier); the three all-4-modes compile fences
(`demand_cyclic_1`, `demand_diff_input_1`, `demand_recursive_content_1`) STAY
(§F). Note the birth-only enforcement is lifted; the witness is now
birth-AND-rebuild.

### A.6.2 Build.cpp — the FENCE (ii) comment (Build.cpp:1279-1294)
The FENCE (ii) block ("mid-stream monotone-input-add is NOT a compile reject …
ships as the DOCUMENTED 'edge-after-demand' feature gap") RETIRES. FENCE (i)
cyclic-demand, FENCE (i,ADJ-C2) recursive-content, and FENCE (iii)
differential-input STAY verbatim. Replace the FENCE (ii) paragraph with a one-
line note that mid-stream monotone edge-add is now HANDLED by band-(a2)
(R-a2), no longer a gap.

### A.6.3 RAT-6 witness-enforcement note + the witness .dr/.main.cpp headers
The `demand_neighborhood_witness.dr` header comment's "RAT-6 BIRTH-ONLY … there
is NO edge-after-demand batch … Edge-after-demand is the LOUD labeled feature
gap" RETIRES → replaced by "R-a2 birth-AND-rebuild: the REBUILD phase adds edges
after their key's demand is standing; band-(a2) rebuilds the instance." Same for
the `.main.cpp` header's RAT-6 birth-only note. KeyedInstances.md §20(C)'s
EDGE-AFTER-DEMAND residual is marked DISCHARGED by R-a2 (the ledger entry for
this landing records it). The gap labels retire WITH the landing (OD-12), not
before — so all three edits land in the SAME diff as the compiler change.

======================================================================
(B) PRE-REGISTERED PREDICTIONS (the stage-e match targets)
======================================================================

Baseline binary: `…/scratchpad/baseline/drlojekyll` (+ `-oracle`),
`TIP.txt = 5813ab8a`.

### B.1 KNOB-OFF = ZERO CHANGE
Every a2 code path is under the `-demand-instance` lowering: `InstantiateEffects`
is called only when instance ops are minted; the SUBGRAPHINSTANCE region + band-
a2 codegen fire only when an op exists; V-INST-EFFECT/DRAIN are VACUOUS with 0
instance ops; the A.2.3 classify arm and A.2.1 field touch no non-instance path.
PREDICT:
- **Binary A/B byte-identity** of the compiler is NOT expected (source changed);
  instead **generated-output byte-identity**: the 676-row corpus + `data/`
  outputs in all 4 modes are BYTE-IDENTICAL vs the frozen baseline (`P-D2a.1`/
  `P-D2b.1` lineage). i.e. NO golden churn on ANY of the 172 non-witness cases.
- **SUITE PASS = 173**, all green, UNBLESSED — EXCEPT the witness golden trio
  (`.stdout`, `.oracle.stdout`, `.monotone.stdout`) which change (§B.3, driver/
  batches churn) and re-bless under RAT-8.
- ctest 5/5 (the `tests/InstanceStore` binary gains one arm, §A.3.2; count
  stays 5).

### B.2 KNOB-ON = exactly one new `vector:drain` effect line; census/deps invariant
On `demand_neighborhood_witness -demand -demand-instance -deltarel-out`:
- the `op.0 kSubgraphInstantiate` `effects:` line gains EXACTLY ONE token, the
  edge drain, rendered SECOND (the ADJ-G1 fork; d2b-desired-states §2 exact):
  ```
  effects: {kVecDrain(%table:8, kNetAddition), kVecDrain(%table:11, kNetAddition),
            kInstanceDemand(%table:8), kInstanceRebuild(%table:4, +),
            kStateEmit(%table:4), kStateOld(%table:4),
            kCounter(%table:4, +, NonRecursive)}
  ```
  (rendered by `emit_effect` `Format.cpp:430-440`: `kVecDrain(<tid input>,
  kNetAddition)`; `%table:8`/`%table:11` are the illustrative demand/edge ids.)
- **census UNCHANGED**: `kSubgraphInstantiate=1 kInstanceSeal=1 kInstanceDeath=0
  kIngestFold=2 kCommitSweep=2 kGroupUpdate=0` (a drain is an effect, not an op).
- **deps UNCHANGED = 6**: the monotone edge frontier is un-minted ⇒
  `ResolveVecIdx` `~0u` ⇒ `add_vec_access` early-returns ⇒ no vec use-edge, no
  RAW hazard (the exact 6 edges of d2b-desired-states §2 stand).
- **`.ir`/`.h` INVARIANT modulo the new a2 region**: the `.h` gains the band-a2
  `for`-loop (FindInstance/TouchCurrent/rescan) inside `flow_46`, the
  `input_frontier` becomes a threaded param `vec25` (was `VectorClear`ed +
  dropped in `proc_19`), and `proc_19`'s `vec25.Clear()` DISAPPEARS (it moves
  `std::move(vec25)` into `flow_46` instead). Everything else byte-identical.
  `.df` INVARIANT (FillDataModel knob-blind; a2 is emission-only).
- **witness REBUILD probes: byte-equal flat==nested** in all four modes (the
  acceptance criterion, §A.5.1/§A.5.5): `{2,3,4,11}`, `{5,6,7}`, `{12}`, `{9}`.

> [ADJ-R5 — ADJUDICATED UPHELD (F3, crit-gates).] The knob-ON dump surface that
> R-a2 changes (the ADJ-G1 second `kVecDrain(%input, kNetAddition)` effect line
> in `.deltarel`, and the band-a2 region + threaded `vec25` param in `.h`) has
> NO automated byte-pin: the witness carries no `.irgold` sidecar (confirmed;
> the `.irgold` carriers are `demand_tc_witness` and `symrec_tie_1`, both flat
> compiles), and an `.irgold` for the NESTED dump is DECLINED BY POLICY
> (OD-10/OWN-5 bless no nested golden). Therefore the effects-fork review is
> MANDATORY-ON-LANDING AND LEDGERED, not optional: before any `--bless`, dump
> `demand_neighborhood_witness -demand -demand-instance -deltarel-out` and the
> nested `.ir`/`.h`, review them against d2b-desired-states §2.3's a2 effects
> fork (exactly ONE added edge drain, rendered SECOND; census/deps invariant;
> input-frontier vector id == tip `vec25` per ADJ-R6), and RECORD that review in
> the landing entry. The config-invariance 3-run + debug==release sweep on these
> dump surfaces is likewise hand-run and must be ledgered. The eqgate remains the
> STANDING BEHAVIORAL guard; the ledgered review is the one-time byte guard the
> policy trades the `.irgold` away for.

### B.3 The witness golden trio CHANGES — and WHY (driver churn only)
The golden IS the flat-arm output (the eqgate compares nested against it). The
`.main.cpp` and `.batches` gain the REBUILD phase, so the FLAT run emits four
more lines → all three goldens MUST re-bless (RAT-8 bless-bootstrap: re-captured
from a review of the FLAT arm's RAW new stdout — see the ADJ-R7 ritual in B.4;
the eqgate is necessarily RED at bless time, so "GREEN flat+nested" is NOT the
pre-bless state):
- `.stdout`: +4 lines, PURELY ADDITIVE (the birth-phase 4 lines are byte-
  unchanged — REBUILD probes run after):
  ```
  nbhd 1: 2 3 4 11
  nbhd 3: 5 6 7
  nbhd 5: 12
  nbhd 9: 9
  ```
- `.oracle.stdout` / `.monotone.stdout`: the demand-blind full closure gains
  rows `neighborhood 1 11`, `3 7`, `5 12`, `13 14` (assertion count + surviving-
  fact count grow accordingly). Driver/batches churn, RAT-8 re-bless.
DECISION STATED: **the flat-arm (== the shared) golden DOES change**, but ONLY
because the driver/.batches changed — NOT because any compiler emission moved on
existing bytes. This is the sanctioned driver-churn re-bless, distinct from a
"make a red case green" bless (RAT-8 lineage). Every OTHER golden is untouched.

### B.4 LANDING ORDER (co-dependency)
The compiler a2 change and the witness churn are CO-DEPENDENT and land in ONE
diff.

> [ADJ-R7 — ADJUDICATED UPHELD (F5, crit-gates).] State the ritual PRECISELY —
> you CANNOT bless from a "green flat+nested run" (the phrase in the original
> B.3): at bless time the committed golden is still the birth-only 4-line
> version, so BOTH the flat diffrun AND the eqgate necessarily DIVERGE from it
> (`runall.sh --bless` copies the flat opt-mode stdout unconditionally, with no
> eqgate gating — mechanism fine). The exact sequence:
>   1. Land the compiler a2 change and the witness `.main.cpp`/`.batches` churn
>      together (ONE diff).
>   2. Run the suite: the witness's flat diffrun and its eqgate BOTH diverge
>      from the stale 4-line golden — EXPECTED, not a failure to chase.
>   3. STAGE-D (ADJ-R3(b)): confirm the eqgate was RED against the a1-only tip
>      compiler on probes 1/3/5 (the recorded RED→GREEN proof).
>   4. Review the FLAT arm's RAW new stdout (+4 lines) and the oracle/monotone
>      raw stdouts against the predicted §B.3 shape.
>   5. `runall.sh --bless` the trio (`.stdout`/`.oracle.stdout`/
>      `.monotone.stdout`).
>   6. Re-run → SUITE PASS, and the eqgate is now GREEN (nested == the freshly
>      blessed golden in all four modes).
> The re-bless is sanctioned DRIVER-CHURN (RAT-8 lineage), never a "make a red
> case green" bless.

======================================================================
(C) ORDERING-SAFETY ARGUMENT
======================================================================

**Claim:** band-(a2) never drains the edge frontier before all of THIS epoch's
edge appends have landed.

**Proof (structural, from the generated code — stronger than the linearizer
lead-partition argument of build-out §3.3):**
The entry proc `proc_19` is straight-line: (1) fold the received batch —
`for (v23,v24) in vec21: if table_11.TryAdd(...).added: vec25.Add(...)` folds
EVERY edge of the batch into `table_11` and the edge frontier `vec25`; (2)
fold demand into `vec29`; (3) `flow_46(..., std::move(vec29), std::move(vec25))`.
The call to `flow_46` (which contains band-a2) is the LAST statement, after the
entire fold loop. So by construction ALL of this epoch's edges are in `vec25`
(and `table_11`) BEFORE band-a2 runs. There is NO interleaving: fill-then-call
is a single straight-line sequence in one C++ function.

The DR-model restatement (build-out §3.3, confirmed): the eager web (every edge
`VECTORAPPEND`) sits at the HEAD of `proc->body` (lead 0/eager), and
`LowerSubgraphInstances` appends the SUBGRAPHINSTANCE region AFTER `proc->body`
(`Procedure.cpp:382` after the body nests at `:303`); `kSubgraphInstantiate` is
lead 1, the edge-table monotone `kCommitSweep`/`kInstanceSeal` are lead 2 (bands
9/11) — strictly after. So a2 (lead 1) reads a COMPLETE, UNSEALED frontier.
The second kVecDrain contributes NO linearizer edge (monotone ⇒ `~0u`), so the
ordering is guaranteed STRUCTURALLY (proc construction order), never by a
`flow.table_vecs` RAW edge — consistent with a1.

**Band key / same-epoch:** a2 reads the edge frontier at the SUBGRAPHINSTANCE
region, after the eager fold and before the edge-table Seal (`table_11.Seal()`
at lead 2, `flow_46` tail line 274). So the frontier is complete and unsealed.
No hazard; no fix needed.

======================================================================
(D) SAME-EPOCH DOUBLE-REBUILD (key demanded AND edge-touched in one epoch)
======================================================================

**Scenario:** one batch carries a NEW demand for key K (→ `vec29`={K}) AND a new
edge (K,x) (→ `vec25`={(K,x)}), and (K,x) is folded into `table_11` before
`flow_46`.

**Behavior (idempotent single rescan):** band-a1 runs FIRST (`flow_46` emits a1
then a2). a1 drains `vec29`: K → `FindOrAddInstance(K)` → `!TouchedFlag(K)` true
→ `TouchCurrent(K)` (sets `TouchedFlag`) → FULL rescan of `table_11` (which
ALREADY holds (K,x)) → `current[K]` includes x. Then band-a2 drains `vec25`:
(K,x) → key K → `FindInstance(K) != kNoInstance` → BUT `!TouchedFlag(K)` is now
FALSE → **SKIP**. So the rescan runs EXACTLY ONCE, and it already saw the edge.
The `!TouchedFlag(iid)` gate is the dedup ([D-COLLAPSE]).

**Order-independence:** even if a2 ran first (it does not), a2 would touch K,
then a1's `FindOrAddInstance(K)` + `!TouchedFlag(K)` would skip — again exactly
one full rescan. And because BOTH bands emit the IDENTICAL full-rescan (ADJ-C1
collapse), whichever fires produces `current[K]` = the complete keyed relation.

**Correctness (runtime-proven, build-out-runtime §4):** `Touch` is append-once
via `touched_flag` (`InstanceStore.h:298-305`), so `touched` holds K once ⇒
Seal visits K once ⇒ one pointer-swap, one occupancy snapshot. `Table::TryAdd`
is whole-row `FindOrAdd`, so even a hypothetical double full-rescan would be
set-union idempotent (no dup rows). No dedup SET is needed — `TouchedFlag`
already collapses to one rescan. Confluent and idempotent within an epoch.

======================================================================
(E) GATES + BINDINGS (each prediction → its gate)
======================================================================

- **B.1 knob-OFF A/B** → generated-output byte-identity, 4 modes, 676-row corpus
  + `data/`, vs the frozen baseline `…/scratchpad/baseline/drlojekyll`
  (`TIP.txt 5813ab8a`). Gate: `runall.sh` full suite must end `SUITE: PASS`
  with ZERO golden churn on the 172 non-witness cases.
- **B.2 knob-ON `.deltarel`** → the eqgate + a hand `-deltarel-out` review of
  the witness: exactly one added `kVecDrain(%table:11, kNetAddition)` line,
  census/deps invariant (HP-13(b) end-to-end review against d2b-desired-states
  §2's a2 fork before any bless).
- **eqgate (`run_eqgate`, 173)** → the STANDING referee: flat==nested==golden
  in all four modes on BOTH birth and REBUILD probes (RAT-9 four-mode loop).
  Pre-landing check (§A.5.4(ii)): eqgate RED on a1-only, GREEN after a2.
- **ctest incl. new InstanceStore arm** → `EdgeTriggeredRebuildSealsCleanPublishes`
  Born (§A.3.2) green; ctest 5/5 debug + 5/5 ASAN.
- **ASAN both surfaces** → build/asan ctest + OptDiff suite under DR=asan + the
  env-CXX-wrapper second surface, per the standing gate; ZERO reports incl. the
  nested witness under ASAN AND the §A.5.4(iii) multi-probe determinism check.
- **E-62 tripwire re-grep** → this IS a DeltaRel diff (InstantiateEffects,
  V-INST-EFFECT, V-INST-DRAIN touched). Re-grep `pinned_order` consumers; sole
  sanctioned hit = validators (Stratum.cpp:1073 comment is the baseline). a2
  adds NO new pinned_order consumer.
- **T2b dump law** → the second kVecDrain is a STORED field (`edge_drain.value_
  table = input`), rendered by the loud-spelling `emit_effect`; no new grammar,
  no pointer-ordered emission.
- **(F) law — no pointer-ordered emission** → a2 emits from the region's stored
  `input_frontier` + the op's stored effect vector (push order), det_seq/mint-
  order only; no `std::sort` on pointers, no address-keyed iteration.
- **config-invariance 3-run + debug==release** on the `.deltarel`/`.ir`/`.df`
  dump surfaces (the witness) — the new effect line + a2 region are config- and
  build-invariant.
- **Q5 ABABAB** → same-session interleaved determinism, incl. the witness
  REBUILD stdout (a2's full-rescan is deterministic — this is the §A.5.4(iii)
  probe-order-independence gate).
- **witness golden re-bless** → RAT-8 bless-bootstrap lineage: the trio
  (`.stdout`/`.oracle.stdout`/`.monotone.stdout`) re-captured from a reviewed
  GREEN flat+nested run (driver/batches churn only; permcheck N/A — these are
  answer stdouts, not permutation-published deltas). NO nested golden is blessed
  (OD-10/OWN-5) — the eqgate refs the flat `.stdout`.
- **HP-17 explicitly UNTOUCHED** → `DeathEffects` unchanged, `kInstanceDeath`
  stays minted-OFF, the R-DIFF arm remains INERT (its `drains==2` bump is a
  latent-trap fix only, never executed under R-MONO). Death op executing-
  coverage stays a D3.a residual, extended through Rel per OD-11.

======================================================================
(F) NON-GOALS (what R-a2 explicitly does NOT do)
======================================================================

- **NO step-kind migration.** R-a2 does NOT mint any Rel op kind for the
  monotone eager web (TUPLE/INSERT/JOIN/…). That is R1..Rk (rel-arch-pseudocode
  §3), the strangler-fig ritual AFTER R-a2. R-a2 adds NO op kind at all.
- **NO seam-artifact retirement.** The four §19(H) seam artifacts (the ingest-
  fold hole contract, the replicated cut predicates + §7d cross-check,
  V-INGEST-XCHECK, the E-42 VECTORLOOP shim) STAY. R-a2 consumes an existing
  frontier; it does not delete the two-authority seam.
- **NO differential-input support.** `demand_diff_input_1` fence STAYS (V-INST-
  SOLE rejects a differential summarized input; R-DIFF is D3.a). `demand_cyclic_1`
  and `demand_recursive_content_1` fences STAY. R-a2 is R-MONO only.
- **NO incremental TryAdd.** a2 is FULL-RESCAN only (ADJ-C1); no incremental
  edge-apply path is added (the belt forbids it).
- **NO frontier-clearing / incrementality optimization.** a2 does a full rescan
  per edge-touched key per epoch (the frontiers are already fresh-per-epoch, so
  this is O(new-edges-this-epoch · input-scan), not O(edges·epochs)). A keyed
  index probe instead of a full scan is a later perf slice (the spine already
  tags `section-walk`; §19(N) target-vs-lowering), NOT R-a2.
- **NO runtime-header change.** `FindInstance`/`kNoInstance` exist; a2 adds a
  unit-test arm only.
- **NO death / HP-17 work.** Minted-OFF stays minted-OFF.
- **NO DeltaRel→Rel rename.** That is R-final's own mechanical commit.

======================================================================
APPENDIX — empirical verification performed (tip 8a03339d)
======================================================================
All claims above marked "verified" were established by compiling
`demand_neighborhood_witness.dr` at tip and driving extended drivers
(scratch only; no repo edits):
- flat `-demand` handles edge-after-demand (accumulates): probe(1) after
  add_edge(1,11) → {2,3,4,11}. [confirmed]
- nested `-demand-instance` a1-only DROPS it in the clean per-epoch structure:
  probe(1) → {2,3,4}. [confirmed — the gap is real]
- the §A.5.2 clean per-epoch REBUILD driver DISCRIMINATES: flat={2,3,4,11}/
  {5,6,7}/{12}/{9}, nested-a1={2,3,4}/{5,6}/{}/{9}. [confirmed by diff]
- the vector-threading: nested generated `datalog.h` shows `proc_19` fills
  edge frontier `vec25`, `vec25.Clear()`s it, passes only `vec29` to `flow_46`
  (a1-only); `ExtractPrimaryProcedure` param rule (`Procedure.cpp:642-644`,
  `687-702`) confirms marking `input_frontier` read threads+un-clears it.
  [confirmed by reading generated code + source]
- the a1-only phantom (multi-key batch + multi-probe → coincidental {2,3,4,11})
  is deterministic per-build, probe-sequence-fragile, ASAN-clean. [confirmed —
  §A.5.4 LOUD RISK]
