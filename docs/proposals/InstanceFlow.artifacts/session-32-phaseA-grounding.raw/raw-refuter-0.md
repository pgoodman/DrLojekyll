I have what I need. All claims verified against tip.

---

# REFUTER #1 (buildability-determinism) — Findings

Verified against `keyed-instances` tip: pass slot, `row_contracts` precedent, `det_seq` stamp, `ForEachUse` dispatcher, JOIN pivot API, CMake helper.

## F1 — BLOCKING (determinism): the OriginUse sort key is not a total order; grove `u#` numbering leaks `ForEachUse` order on `join_1` (a named golden witness)

The design canonicalizes `OriginUseCatalog` by sorting uses on `(consumer det_seq, producer_col index, role)` (§2.2 item 4) *specifically* to defeat `ForEachUse`'s unstable order. That key is **not injective**. `include/drlojekyll/DataFlow/Query.h:486-487` is explicit: `ForEachUse` "does not provide any guarantees on column visiting order and one should assume the worst-case order." When two edges into the same consumer share role and producer-column ordinal but come from **different producer views**, the sort key ties and the tie-break falls back to `ForEachUse` emission order → non-deterministic golden.

This is not hypothetical — it fires on the design's own flagship witness. `join_1`'s pivot join (IF4(a)): `p(A,B),r(A)` joined on `A`, `A` at **index 0 in both arms**. `QueryJoin::NthInputPivotSet(0)` (`lib/DataFlow/Query.cpp:832`) returns `{tuple.6.col0, tuple.7.col0}` — two edges, both `role=kJoinPivot`, both `producer_col=0`, both `consumer=join.10`, **different producers** (`tuple.6` vs `tuple.7`). Sort key `(join.10.det_seq, 0, kJoinPivot)` collides; the two `OriginUse` records (which carry distinct `producer` fields) order nondeterministically → `u#N` labels flip run-to-run → `-instanceflow-out` golden is unstable. Leading-shared-column joins are ubiquitous (any `r(A,B):s(A,X),t(A,Y)`), and self-joins (`f16_join_witness`) make it worse.

FIX: extend the sort key to a proven total order — append `producer QueryOriginId` and the `out_col` ordinal (`in.Index()` of the consumer slot), both of which the record already carries. `NthInputPivotSet` is itself an insertion-ordered `UseList`, so keying on producer det_seq is sufficient and cheap.

## F2 — MAJOR (determinism + V-IF-EMISSION contradiction): `UseCoverage.authority` is not single-valued for fork/diamond uses

Every `UseCoverage` carries one `authority` = "the terminal INSERT reached along its chain" (§3, §4 dump `u#0 … authority=ea#0`). A non-root use whose forward cone reaches **multiple** INSERTs has no single terminal authority. Concrete witness `transitive_closure` (IF4(c)): the use `select.0→tuple.9` flows into `merge.11`, which fans out to `tuple.6→insert.13`, `tuple.7→insert.14`, **and** `merge.12→tuple.8→insert.15`. Three reachable authorities for one use. Resolving "the" authority by a forward reachability walk makes the choice depend on `QueryView::Successors()` iteration order (another container-order leak), and simultaneously violates V-IF-EMISSION's "exactly one authority per `(site, domain)`" as re-read at the coverage layer. This blocks a stable golden AND the emission validator's flat reduction.

FIX: either scope `authority` to **root uses only** (the terminal INSERTs are themselves the authorities; interior uses carry none), or make coverage a `(use, reachable-derivation-site)` product so each pair is deterministically enumerable in catalog order. Do not resolve by graph DFS. (Overlaps Refuter #3's coverage-model remit, but it is a hard determinism blocker here.)

## F3 — MAJOR (buildability): the recommended `lib/InstanceFlow` layout inverts its own cited precedent and forces a hook that leaves the validators OFF for non-`Main.cpp` consumers

The `-contract-out` precedent the design leans on does the **opposite** of what §1 recommends. `RowContractMap` is `std::unordered_map<QueryViewImpl*, RowContract>` defined in `lib/DataFlow/RowContract.h:51`, stored **by value** on `QueryImpl` (`lib/DataFlow/Query.h:1240`), built and validated by functions **inside lib/DataFlow** (`RowContract.cpp`, in `DataFlow_SRCS`). No new target, no incomplete type, no hook. The precedent generalizes cleanly only to "fold InstanceFlow into lib/DataFlow" — the choice the design demotes to "Phase-C debt."

The recommended separate target creates a real `lib/DataFlow → lib/InstanceFlow → lib/DataFlow` cycle (Build.cpp calls `BuildFlatInstanceFlow`; InstanceFlow reads `QueryImpl`). The `dr_define_static_library` helper wires deps via `target_link_libraries(... PUBLIC ...)` (`cmake/dr_define_static_library.cmake:28-32`) — a PUBLIC cycle propagates include-dirs/usage-requirements pathologically even though CMake tolerates static-lib link cycles. The design's escape — the `gInstanceFlowTailPass` function pointer wired from `Main.cpp` — is null in every link that does not call `WireInstanceFlow()`: the `ctest RegionInstance` target and any direct `FrozenRegionalProgram::Build`/`Program::Build` test path build **no grove and run none of V-IF-ORIGIN/CONTEXT/COVERAGE/EMISSION**. Byte-identity is then trivially "preserved" because the pass is inert, and the always-on validator claim (§17) is false for those consumers.

FIX: fold Phase A + flat grove into `lib/DataFlow` exactly like `RowContract` (store by value, build+validate in a new `lib/DataFlow/InstanceFlow.cpp`, dump in `Format.cpp` or a sibling). Defer the target split to the Phase-C Rel cutover when a real Rel→InstanceFlow reader exists to justify it.

## F4 — MAJOR (grounding is stale): the "load-bearing NEGATE patch" (Risk R5) is unnecessary — the tip dispatcher already handles NEGATE

IF3 §4 and §2.2 assert `QueryView::ForEachUse` "has no `AsNegate()` arm → `assert(false)`" and require the builder to special-case `if (auto n = v.AsNegate()) n.ForEachUse(...)`. **False at tip.** `lib/DataFlow/Query.cpp:519-520` has an explicit `else if (auto neg = impl->AsNegate(); neg) { QueryNegate(neg).ForEachUse(...); }` arm before the `assert(false)`. The uniform `v.ForEachUse(...)` walk is total across all ten kinds today. The special-case is dead code — harmless, but it means the grounding read a pre-tip version. That directly undermines confidence in the sibling claims read the same way (the `QueryMerge::ForEachUse` "MERGE-arm-is-INSERT" special-case and `all_cols_match`). RE-VERIFY those against tip before building; do not carry IF3 §4's gap list as fact.

## F5 — MINOR (buildability): storage shape is self-contradictory and constrains the struct

§1 stores `std::unique_ptr<InstanceFlowProgram>` (fwd-decl); IF2 stores `InstanceFlowGrove instance_flow;` **by value** next to `row_contracts`. Pick one. `~QueryImpl` is out-of-lined at `lib/DataFlow/Query.cpp:24`, so *either* shape requires the complete `InstanceFlowProgram` type to be visible in that TU (lib/DataFlow). If the struct's destructor is non-trivial and defined out-of-line in lib/InstanceFlow, `~QueryImpl` gains a link edge into lib/InstanceFlow → the F3 cycle. Constraint: keep `InstanceFlowProgram` header-only (all `std::vector`/`std::unordered_map` members, implicit dtor) so `#include`ing the header in Query.cpp suffices with no link edge. By-value (IF2) is the precedent-faithful, cycle-free choice once F3's fold-in is adopted.

## F6 — MINOR (maintainability, not a determinism break): `for_each_view` is a TU-local lambda, not reusable

The design repeatedly says to "reuse the exact `for_each_view` lambda verbatim from `Format.cpp:1553-1573`." It is a **local lambda inside `operator<<(OutputStream&, QueryContracts)`** (`lib/DataFlow/Format.cpp:1557`), not a free function — uncallable from another TU. The builder must duplicate the per-kind DefList walk (Selects→…→Inserts). Determinism is unaffected (the walk is over `DefList`s in insertion order, and the design sorts nodes by `det_seq` anyway), but the ordering contract is now duplicated in ≥3 sites (`.df`, `-contract-out`, InstanceFlow); a future kind reorder desyncs them. Extract one shared `ForEachViewKindTagged` helper.

---

## Verified-safe (attempted refutation failed — residual risk noted)

- **QueryOriginId / `det_seq`**: deterministic. Stamped via `ForEachView` over per-kind `DefList`s in insertion order (`lib/DataFlow/Induction.cpp:142-144`, comment "run-stable; pointer values are not"), dense `[0,N)`, no re-stamp between the stamp and the slot. Typed wrapper is sound.
- **LogicalCollectionId / DerivationSiteId interners**: deterministic. `Relations()`/`IOs()`/`Inserts()` back onto `DefList<...>` (`lib/DataFlow/Query.h:1183,1185,1197`) walked in insertion order; `by_decl_id` is lookup-only. I could not construct a container-order leak here — the only `unordered_map` in the path (`decl_to_relation`) is never iterated for numbering. `ParsedDeclaration::Id()` stability across redeclarations is real (`lib/Parse/Parse.cpp:321-328`). No finding.
- **CMake configure**: the helper does not hard-reject cycles (static libs), so a fold-in (F3 fix) configures cleanly; the separate-lib cycle is a fragility/coverage problem (F3), not a configure-time hard error.

---

**VERDICT: REFUTED as framed.** Phase A is buildable and its origin/collection/site IDs canonicalize deterministically, but the flat grove does **not** produce a stable golden as designed — F1 (OriginUse sort key incomplete, fires on `join_1`) and F2 (multi-valued `authority` resolved by graph walk, fires on `transitive_closure`) are concrete container-order leaks into `-instanceflow-out`, and F3 shows the recommended file layout fights its own precedent. The slice survives only after: total-ordering the use catalog by producer det_seq, defining authority assignment without a reachability walk, and folding into lib/DataFlow. F4 flags the grounding is not tip-accurate (NEGATE already handled) — re-verify the remaining IF3 §4 gaps before coding.