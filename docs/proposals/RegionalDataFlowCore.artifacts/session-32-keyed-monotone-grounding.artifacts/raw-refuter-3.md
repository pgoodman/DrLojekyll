## REFUTER #4 (scope-and-shadow) — Findings

I read the actual pre-cut suppression machinery and the tip state. The design is refuted **as framed**: its 10-item file-by-file plan omits the one mechanism that makes the keyed store *replace* rather than *duplicate* the flat lowering. Concrete findings:

---

### F1 — BLOCKING — The flat guard-web pub-insert is NOT suppressed → double-emission, not a codegen move (pure shadow + bloat)

**The failure.** In the flat `-demand` path the demanded rows flow `demand__ → guard JOIN → pub INSERT`, and that terminal insert lowers to code via the eager web (`BuildEagerInsertionRegions`). The keyed store's `kSubgraphInstantiate` band-(b) *also* writes the pub table. The pre-cut design made the keyed store *replace* the flat path via a cut-successor excision in the eager walk — the design's plan never restores it.

**The anchor (pre-cut, the mechanism the plan is missing).** `dc965d3c^:lib/Rel/Rel.cpp` `IsCutSuccessorDR`:
```cpp
bool IsCutSuccessorDR(Context &context, QueryView succ) {
  if (succ.CanReceiveDeletions() || succ.IsAggregate() || succ.IsKVIndex()) return true;
  return context.demand_instance_enabled &&
         succ.GuardAnnotationIndex() != QueryView::kNoGuardAnnotation;   // <-- the instance arm
}
```
and the walk consuming it at `dc965d3c^:lib/ControlFlow/Build/Build.cpp:1028` with the load-bearing comment (`:1020-1024`): *"under -demand-instance, a recognized-subgraph guard JOIN successor is fed by its SUBGRAPH_INSTANTIATE op … Stop the descent (**the flat guard-join web / flow:58 is NOT emitted**) AND provision this monotone input's net-additions frontier (OD-4)."*

**Verified GONE at tip.** `lib/Rel/Rel.cpp:1130`:
```cpp
bool IsCutSuccessorDR(Context &context, QueryView succ) {
  (void) context;                                                        // <-- instance arm deleted
  return succ.CanReceiveDeletions() || succ.IsAggregate() || succ.IsKVIndex();
}
```
and `grep demand_instance_enabled lib/ include/ bin/` → **zero hits** at tip.

**Consequence for a monotone slice specifically.** Because band-(b) uses idempotent `pub.TryAdd`, the double write is answer-*harmless* (`.stdout` stays byte-identical) — which is exactly why this is a *shadow* trap: the keyed store gets emitted *beside* the still-emitted flat guard-web, `.df/.rel/.ir/.h` grow with dead duplicate code, no flat table is removed, and the design's headline claim ("keyed store replaces the flat guard-web") is **false in generated code**. The slice would look like it "works" (green `.stdout`) while moving zero codegen in the intended direction.

**FIX (must fold into the plan as new items 11–12).**
- Restore the `demand_instance_enabled && GuardAnnotationIndex()!=kNoGuardAnnotation` disjunct in `IsCutSuccessorDR` (`lib/Rel/Rel.cpp:1130`). This automatically re-arms the symmetric `AnyCutSuccessorDR` (`Rel.cpp:135`) and the Build.cpp eager-walk cut + OD-4 boundary-frontier append (`Build.cpp:1028`,`:1055`) — all call through it.
- Restore `effective_demand_instance` selection + `context.demand_instance_enabled` assignment (`dc965d3c^:Build.cpp:1532-1568`), which the plan's item-3 mentions as a Context field but never wires to the flag/@key selector.
- The design's §2.b/§4 must add this to the file touch-list; it currently touches neither `Rel.cpp:1130` nor the `Build.cpp` eager walk.

---

### F2 — MAJOR — Only `pub` differential is fenced; a differential *demand* silently drops `kInstanceDeath` → stale over-materialization

**The failure.** The slice-1 correctness rests on `TableIsDifferential(demand_table)==false` (that gate is what conditions the dropped `kInstanceDeath` mint — E4 §2c `if (demand_table && TableIsDifferential(demand_table))`). But `Query::Build` takes `bool demand_retract` (`lib/DataFlow/Build.cpp:2527`) and `-demand-retract` is a live flag (`Main.cpp:265`). Under `-demand-instance -demand-retract` the fabricated `demand__` message is deletion-capable → the demand table is differential → the dropped death op means a retracted demand key's instance is **never torn down** → the store keeps publishing stale rows. Idempotent `TryAdd` does *not* save this — retraction is the whole point.

**Anchor.** Design Risk #5 adds a `ValidatorFail` only for `TableIsDifferential(pub_table)`. The demand-differential case — the one that actually triggers `kInstanceDeath` (E4 §2a `if(input_diff)` and §2c death gate are keyed on demand/input differentialness, not pub) — is unguarded.

**FIX.** In `BuildSubgraphInstanceOps`, `ValidatorFail` if `TableIsDifferential(demand_table) || TableIsDifferential(input_table) || TableIsDifferential(pub_table)` in slice 1 (fail loud until slice 2), and reject `-demand-instance` composed with `-demand-retract` at flag-parse time in `Main.cpp`.

---

### F3 — MAJOR — No validator catches the F1 double-emission; the design's own belt is admitted insufficient

**The failure.** Even if F1's excision is restored, nothing *proves* the flat path was suppressed. `V-INST-SOLE` ("exactly one `kSubgraphInstantiate` per pub table") counts only instance ops — E3 states verbatim it does not guard "against a non-instance flat deriver on the same table." So a regression that silently re-enables the flat eager insert (e.g. a future `GuardAnnotationIndex` migration bug across CSE dropping the stamp — see `lib/DataFlow/Link.cpp:223`, `IdentityJoin.cpp:154` which already note this coupling) would double-emit undetected, and again be `.stdout`-invisible because monotone.

**FIX.** Add an always-on validator asserting that a pub table owned by a `kSubgraphInstantiate` has **zero** eager/branch derivers (`DRBranch`/`kEagerInsert` marker) targeting it — i.e. make "keyed store is the sole deriver" a checked structural invariant, not an emergent property of the excision. This is the belt that turns "moves codegen" from a hope into a gate.

---

### F4 — MINOR — "moves a golden" is unprovable from `.stdout` alone; the witness must pin the *structural* replacement

**The failure.** For the monotone witness (`add_edge` → `edge` → `neighborhood(bound Start, free Node)`), the demanded answer is identical whether produced by the flat guard-web or the keyed store (both are add-only, both idempotent). So the `.stdout`/oracle/monotone goldens **cannot distinguish** a real replacement from a shadow. The design cites the eqgate discipline but commits no witness or `.irgold`.

**Confirmed monotone otherwise:** the retract injector is correctly gated off for plain `-demand` (`Build.cpp:382` `if (!is_retract)` → `nullptr` retract proc), the seed *does* flow (`BuildQueryForceProcedureImpl` is live at `Build.cpp:384`, contra the S1b-not-wired worry), and `edge`/`neighborhood` are genuinely add-only — so the witness *is* legitimately monotone.

**FIX.** The witness must carry a `.df`/`.rel`/`.h` `.irgold` that pins **(a)** the presence of the `SUBGRAPHINSTANCE`/`instance_<id>` store, and **(b)** the *absence* of the flat guard-JOIN → pub eager-insert region. Only a structural golden that asserts the flat region is *gone* proves this is a codegen move and not F1's shadow. A green `.stdout` alone is consistent with the shadow.

---

## Verdict

**Refuted as framed.** The monotone slice does *not* survive the 10-item plan as written: it omits the cut-successor excision (`IsCutSuccessorDR` instance arm + `demand_instance_enabled` wiring, both deleted at tip), so the keyed store would be emitted *beside* a still-live flat guard-web — a `.stdout`-invisible double-emission that moves no codegen in the intended direction (pure shadow + dead bloat). It *is* genuinely monotone for the proposed witness, and the gap is fixable: add F1 (restore excision + wire the flag), F2 (fence differential demand/input, reject `-demand-retract`), F3 (a sole-deriver structural validator), and F4 (a structural golden proving the flat region vanished). With those four, the slice becomes real and answer-preserving; without F1 it is a shadow.