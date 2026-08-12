I have what I need — the pre-cut code confirms a load-bearing omission. Writing findings.

---

# REFUTER #2 (validator-soundness) — findings

**Method**: I attacked the four named abort-validators plus the DR-IR linearizer against actual tip code and the pre-cut reference (`dc965d3c^`). Two of the four validators are provably *not* perturbed by the keyed mint (they run upstream). The eager-web/linearizer path is where the design breaks — and I found a concrete, load-bearing omission the design does not list.

---

## Finding 1 — BLOCKING: the design omits the `IsCutSuccessorDR` guard-annotation clause; without it the monotone slice hard-aborts in `flow.TableVec`

**Concrete failure.** `InstantiateEffects_MONO` (design §2a) emits `kVecDrain(demand, kNetAddition)` **and** `kVecDrain(input, kNetAddition)`. In the monotone slice both `demand_table` and `input_table` are monotone. The `kNetAddition` frontier vec for a table is minted in `flow.table_vecs` in exactly two places:
- the differential-only loop, which **skips monotone tables** — `lib/Rel/Rel.cpp:1399`: `if (!TableIsDifferential(table) || TableIsInductionOwnedDR(context, table)) { continue; }`; and
- the monotone-boundary provision, gated on `MonotoneIngestRoleDR` (`Rel.cpp:150-164`), which returns `kNetAddition` only when `AnyCutSuccessorDR(member)` is true.

`AnyCutSuccessorDR` (`Rel.cpp:133-140`) delegates to `IsCutSuccessorDR`, which **at tip** (`Rel.cpp:1130-1132`) is:

```cpp
bool IsCutSuccessorDR(Context &context, QueryView succ) {
  (void) context;
  return succ.CanReceiveDeletions() || succ.IsAggregate() || succ.IsKVIndex();
}
```

A monotone guard JOIN (the recognized demanded subgraph) is none of those three, so `MonotoneIngestRoleDR(input_table)` returns `kEmpty` → **no `kNetAddition` vec is minted** for the monotone input/demand tables. When the linearizer/validators resolve the instance op's drain effect they call `DRFlowGraph::TableVec` (`Rel.cpp:598-605`):

```cpp
unsigned DRFlowGraph::TableVec(TABLE *table, VecRole role) const {
  auto it = table_vecs.find(table);
  assert(it != table_vecs.end());          // <-- fires: no frontier minted
  auto jt = it->second.find(role);
  assert(jt != it->second.end());
  return jt->second;
}
```

→ **hard abort** in debug; **garbage vec index → memory corruption** in NDEBUG.

**Proof this is a real omission, not paranoia.** The pre-cut `IsCutSuccessorDR` (`dc965d3c^:lib/Rel/Rel.cpp:1569-1575`) had a **fourth clause the design never re-adds**:

```cpp
bool IsCutSuccessorDR(Context &context, QueryView succ) {
  if (succ.CanReceiveDeletions() || succ.IsAggregate() || succ.IsKVIndex()) {
    return true;
  }
  return context.demand_instance_enabled &&
         succ.GuardAnnotationIndex() != QueryView::kNoGuardAnnotation;
}
```

The design's file-by-file plan (E4 summary, and §2 diff) lists `ResolveLiveRecognition` / `InstantiateEffects` / `SealEffect` / `BuildSubgraphInstanceOps` / the mint call site — **but never mentions modifying `IsCutSuccessorDR`.** This one clause is what makes the guard JOIN a cut, which (a) provisions the monotone demand/input frontiers that the drain effects require and (b) stops the eager walk at the subgraph (Finding 3). It is mandatory and missing.

**Fix.** Re-add the guard-annotation clause to `IsCutSuccessorDR` (tip `Rel.cpp:1130`) as an explicit line item in the slice. The `context.demand_instance_enabled` field it depends on is already in the design (§2e); this closes both the frontier-resolution abort here and the double-lowering coverage gap in Finding 3.

---

## Finding 2 — MAJOR: the census recount is a tautology (mint and recount both call `ResolveLiveRecognition`), so it provides no independent check on instance count

**Concrete failure.** Design §2f sources `exp_instance` from `ResolveLiveRecognition(impl, query)` — the **same function** the mint (`BuildSubgraphInstanceOps`) uses to decide which forcings resolve (`ri.ok`) and mint. If `ResolveLiveRecognition` mis-resolves (e.g. CSE over-eliminates a forcing's guards so `ri.ok` is wrongly false — the exact scenario the design flags in Risk 3), the mint skips it **and** the recount expects zero → they agree → **no abort**. This is precisely the "comparing flow against itself" trap the E-27 anti-tautology rule exists to forbid; the `kGroupUpdate` precedent deliberately recounts from `query.Aggregates()`/`query.KVIndices()` — accessors independent of the mint's resolution logic. There is no such independent source here: the raw `query.RecognizedSubgraphs().size()` over-counts legitimately-dead forcings, and any resolution-filtered count re-invokes the recognizer.

The design's own ancestor already conceded this: E6 records `d3-instance-store-target.md` amendment A7/NF1 — *"the census only cross-checks the mint loop against the recognizer's list — it can't catch a recognizer that misidentifies views in the first place."*

**Why it matters for slice 1 specifically.** The monotone slice's only *structural* check that the right number of stores mint is this census. Because it is tautological, and slice 1 lists **no oracle/eqgate witness** (the `.eqgate` family is deferred to slice 2 per §3), a mis-count is completely unguarded end-to-end.

**Fix.** Either (a) derive `exp_instance` from a structurally-disjoint walk — e.g. count live `query.Inserts()` whose `Declaration().Id()` matches a `DemandForcings()[i].query` decl (an independent traversal, not the guard-JOIN re-bucketing) — or (b) explicitly demote the census to a coherence check *and* make an oracle/eqgate witness **mandatory** for slice 1 (add one non-recursive monotone bound-`#query` case whose answer is checked against `bin/Oracle`). Do not ship slice 1 with the census as its sole numeric guard.

---

## Finding 3 — MAJOR: no validator forbids an eager deriver of a keyed-owned pub; a broken cut clause double-derives pub while every golden stays green

**Concrete failure.** Even with Finding 1 fixed, `V-INST-SOLE` as specified (§2f) checks only *"exactly one `kSubgraphInstantiate` per pub table."* It does **not** check that the pub table has no *eager* INSERT deriver. The flat guard-web's pub INSERT (`^insert.6` / `connect/query-insert` in the E1 witness) is suppressed only by the cut clause of Finding 1 excluding it from the eager walk. If that clause is absent or later regresses, the eager web lowers the flat INSERT into pub **and** the keyed store publishes into pub. Because monotone `Table::TryAdd` is idempotent, the published *set* is identical → the answer golden, oracle, monotone, and behavioral goldens **all pass green** → the keyed store is silently redundant (does full double work, materializes nothing "keyed"), and the eager-web Site-5 multiset (`Rel.cpp:3554`, `IsEagerMarkerKind`) balances fine because the flat insert is genuinely emitted+enrolled and the keyed publish is not an eager marker kind. The validators cannot distinguish "keyed store owns pub" from "keyed store duplicates a fully-materialized pub."

**Fix.** Add a cross-check (in `ValidateDROps`) that a pub table owned by a `kSubgraphInstantiate` op appears in **zero** eager INSERT markers / has no `kEagerInsert` deriver — i.e. make the Site-5 exclusion an *asserted invariant*, not an incidental consequence of the cut walk. This turns the "keyed store actually replaces the flat lowering" property into a checkable one.

---

## Finding 4 — MINOR: `DROpStratum` instance arm relies on `ValidatorFail` being `[[noreturn]]`, else silent earliest-band placement

**Concrete failure.** The design's `DROpStratum` arm (§2b):

```cpp
case kSubgraphInstantiate:
case kInstanceDeath:
    if (found) return it->second;
    ValidatorFail("...");        // no `return`/`break` after
case kInstanceSeal:
    return 0u;
```

If `ValidatorFail` is ever not `[[noreturn]]` (or is compiled out), control falls through to `case kInstanceSeal: return 0u`, placing a mis-stratified instance op in the earliest band — which E3 names *"the #1 way a wrong ordering slips past every other validator undetected"* (V-READY/V-LINEAR only check the *derived* stratum's internal consistency, not that it is the correct one).

**Fix.** Put an explicit `return 0u`-free abort path: `ValidatorFail(...); return 0u;` is wrong; use `ValidatorFail(...); __builtin_unreachable();` or confirm `ValidatorFail` is `[[noreturn]]` and add `-Werror=implicit-fallthrough` coverage.

---

## Finding 5 — MINOR: `instance_stratum` self-seed (Risk R1) is sound for mono-acyclic but has no validator backstop for the untested case

**Concrete failure.** `flow.instance_stratum[sid] = 1 + max(drain_stratum(demand), drain_stratum(input))` is computed inside `BuildDRInventory` where `drain_stratum` is still empty (`DeriveDRStrata` runs after, `Stratum.cpp:2156`) → every value is `1`. Correct for the mono-acyclic corpus (producers at band 0). But nothing checks it: V-READY only fires if the frontier *producer* lands at stratum ≥1 — exactly the multi-stratum case that never occurs in the slice-1 corpus, so the guard is silently vacuous and a future recursive/multi-stratum demand under-orders undetected.

**Fix.** Add the design's own proposed post-`DeriveDRStrata` DEBUG assert: `instance_stratum[sid] > drain_stratum[demand]` and `> drain_stratum[input]` for every store. Cheap, and it converts Risk R1 from "latently wrong" to "loud on the first offending program."

---

## Finding 6 — MINOR / residual: `GuardAnnotation::kBody` / `::instance_key` names unverified feed the Finding-2 tautology

`ResolveLiveRecognition` buckets on `annots[ai].role == GuardAnnotation::kBody` and reads `annots[ai].instance_key` (E4 §1, flagged unverified). If S1a restored a drifted `GuardAnnotation`, the re-bucket picks the wrong JOIN → `input_table`/`input_key_cols` bind wrongly → and because the census reuses the same function (Finding 2), the mis-binding is invisible to the census. **Fix.** Grep `include/drlojekyll/DataFlow/Query.h:1001-1032` and confirm the `role` enum has `kBody` and the field is named `instance_key` before wiring; this is a hard prerequisite for both the mint and any independent-recount rewrite.

---

## What I could NOT refute (the two "safe" named validators)

- **RowContract / ProjectionRole**: computed at the `Query::Build` tail (`InferConservativeRowContracts`; `ProjectionRole` folded into `Equals`), strictly upstream of the Rel-layer keyed mint. `BuildSubgraphInstanceOps` creates **no** Query views and mutates no `QueryTupleImpl::ProjectionRole` — it reads live tables off `view_to_model`. **Not perturbed.** Residual risk: none found.
- **K5 `origin_decls`**: seeded at the Connect insert-proxy mint and unioned at CDaGI, both in DataFlow, upstream of the mint. The keyed mint neither creates views nor touches `origin_decls`. **Not perturbed.**
- **Regional V-REGION-CENSUS**: re-derives `DeriveRegionalCensus(query)` at freeze (`FrozenRegionalProgram::Build`), upstream of `Program::Build`. The keyed mint adds **no region**, so the stored-vs-rederived count is unaffected. The demanded-interior **Tier-1 row-contract is intact** — it is driven by the S1a recognition front-end (`demanded_decl`), independent of whether the keyed *lowering* runs. **Not perturbed.** (Note: this holds only because the mint is Rel/ControlFlow-layer; if the slice ever moved recognition into the Regional layer this would need re-checking.)

---

## VERDICT

**The monotone slice does NOT survive as written** — it hard-aborts in `DRFlowGraph::TableVec` (Finding 1) because the design omits the mandatory `IsCutSuccessorDR` guard-annotation clause that provisions the monotone demand/input frontiers the drain effects consume. **With Finding 1 folded in, the slice is validator-sound**, but Findings 2 and 3 show its validator surface cannot by itself prove the keyed store mints the right count or actually *owns* (rather than duplicates) pub — so an oracle/eqgate witness is mandatory for slice 1, not deferrable to slice 2.