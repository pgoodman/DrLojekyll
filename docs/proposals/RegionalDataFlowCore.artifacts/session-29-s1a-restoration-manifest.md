# S1a restoration manifest — flat `-demand` revival (the execution playbook)

> Produced by the session-29 grounding loop (a sonnet extractor reading `git show dc965d3c` hunk-by-hunk
> against tip `05af6595`, every restore point located at tip). **Scope: FLAT `-demand` ONLY** — the keyed
> InstanceStore / `-demand-instance` machinery is S2+ and is explicitly EXCLUDED at every step. This is the
> concrete work-list for S1a; re-verify tip line numbers before each edit (the pipeline drifts). Companion:
> `session-29-grounding.md` (why), `session-30-prompt.md` (the S1a charter).
>
> **Owner-ratified (s29):** Path R flat-only; recursive-TC first slice; proceed to build S1a. The recovered
> `lib/DataFlow/Demand.cpp` (1464) + `lib/Parse/Demand.cpp` (263) are already RESTORED into the tree from
> git `48cd0a4f` (untracked, un-wired — CMake doesn't reference them, so the build is GREEN).

## Key corrections to CLAUDE.md / prior anchors (found during extraction)
- **CLAUDE.md's `Planning.cpp:161` region-port-filter anchor is STALE** — tip :161 is unrelated K5
  `ResolveOriginSupport`. The ONE real filter site is `CollectMessages` (Planning.cpp:42-59), which feeds
  BOTH `census.input_ports` (:675) and the real port-minting loop (:730-742). Fix once, there.
- **`lib/DataFlow/RowContract.cpp` needs NO changes** — never touched by the cut; its dispatch is generic
  (a demand-minted MERGE/JOIN/TUPLE falls into already-generic branches).
- **K5 `origin_decls` needs NO changes** — seeding/union is unconditional over every view; demand mints
  inherit origins like any CSE-derived view.
- **The Rel eager-web needs NO changes** — `BuildDRInventory`'s INGEST_FOLD loop (Rel.cpp:2306-2344) and
  V-INGEST-XCHECK/V-PRED-XCHECK key on `Id()`/table-pointer identity; the fabricated message is an ordinary
  `QueryIO`/`QueryReceive`, already covered. (Leave the vestigial dead `DRInstance`/`demand_table`/
  `EffKind::kInstance*` scaffolding in `lib/Rel/Rel.h:855-890` UNTOUCHED — it's S2+ D2.b reserved.)
- **`-demand-retract` / `-demand-instance` are OUT of slice-1** — the `Query::Build` `demand_retract`/
  `suppress_demand` params ride header defaults; no `gDemandRetract`/`gDemandInstance` in Main.cpp;
  `Program::Build` stays 4-arg (its `bool demand_instance` is InstanceStore-only — do NOT resurrect it).

## 1. CMake wiring
- `lib/DataFlow/CMakeLists.txt` — insert `"Demand.cpp"` between `"DeadFlowElimination.cpp"` (:25) and
  `"Differential.cpp"` (:26). No DEPENDENCIES change (DataFlow already deps Parse/Lex/Display/Util).
- `lib/Parse/CMakeLists.txt` — insert `"Demand.cpp"` between `"Clause.cpp"` (:20) and `"Enum.cpp"` (:21).

## 2. Header types + accessors (FLAT-in-scope)
### 2a. `include/drlojekyll/DataFlow/Query.h`
- `using GuardAnnotationIndex = unsigned;` after `namespace hyde {` (:71), before `class ErrorLog;` (:80).
- `QueryView::kNoGuardAnnotation` + `GuardAnnotationIndex()` accessor: after `DeterministicOrder()` (:437),
  before the K5 `OriginDecls()` comment (:439).
- `QueryDemandForcing` / `GuardAnnotation` / `RecognizedSubgraph` structs: after `class QueryKVIndex`'s `};`
  (:961), before the `// A query` comment (:963). (All FLAT-in-scope — `GuardAnnotation::is_instance_key`/
  `instance_key` are simply always-false/empty in the single-adornment slice.)
- `Query::Build` signature: add the 3 trailing defaulted bools at :969-971 —
  `..., bool demand_mode = false, bool demand_retract = false, bool suppress_demand = false`.
- `DemandForcings()` / `GuardAnnotations()` / `RecognizedSubgraphs()` / `IsDemandMessage()` decls: after
  `~Query(void);` (:973), before `ParsedModule(void)` (:975). (Definitions live in restored Demand.cpp:296-315.)
### 2b. `lib/DataFlow/Query.h`
- `QueryViewImpl::guard_annotation_index` + `::query`: after `det_seq{~0u};` (:487), before `group_id` (:490).
- `QueryImpl::ApplyDemandTransform` decl: after `ConnectInsertsToSelects` `);` (:1075), before Canonicalize (:1077).
- `QueryImpl::demand_forcings` / `guard_annotations` / `recognized_subgraphs` / `guard_annotation_folded_count`:
  after `DefList<QueryInsertImpl> inserts;` (:1189), before `num_strata` (:1191).
- `GuardAnnotationsCompatible` / `CheckGuardAnnotationFold` / `PromoteSurvivorToBody` free-fn decls: after
  `QueryImpl` `};` (:1209), before V-PROJ-ROLE-STABLE (:1211).
### 2c. CSE-migration surface (FLAT-in-scope — `Optimize`'s CSE always runs; must not strand/dup a guard stamp)
- `lib/DataFlow/View.cpp`: restore `GuardAnnotationsCompatible`/`PromoteSurvivorToBody`/`CheckGuardAnnotationFold`/
  `PrintGuardAnnotation` (~95 lines) before `CopyDifferentialAndGroupIdsTo` (:558); restore the guard-annotation
  transfer block inside it (after the `can_produce_deletions` clause :610-612, before the closing `}` :613).
- `lib/DataFlow/Query.cpp`: restore `QueryView::GuardAnnotationIndex()` after `CanReceiveDeletions()` (:349),
  before `OriginDecls()` (:351).
- `lib/DataFlow/IdentityJoin.cpp`: restore the guard-clear block as the first stmt of `ForwardToKeep` (:149).
- `lib/DataFlow/Link.cpp`: restore the save/restore-`guard_annotation_index`/`query` wrapper around the
  `CopyDifferentialAndGroupIdsTo` call in `ProxyMergedViews` (:219) ONLY (leave :70/:98/:163 alone).
- `lib/DataFlow/Join.cpp`: restore the RIDER-2b abort tripwire before `CopyDifferentialAndGroupIdsTo` in
  `ProxyUnusedInputColumns` (:279).
- `lib/DataFlow/Format.cpp`: restore the `declared-key rel=... declared=(...) inferred=(...)` block in the
  `QueryContracts` `operator<<` (after the per-view loop `});` :1734, before `census: views=` :1736).
- `Merge.cpp`/`Optimize.cpp`/`Connect.cpp`: comment-only rewords — optional/cosmetic.

## 3. `ParsedModule` fabrication surface
- `include/drlojekyll/Parse/Parse.h`: restore `FabricateDemandMessage`/`FabricateDemandLocal`/
  `DemandFabricationWouldCollide`/`DemandMessagesFabricated`/`MarkDemandFabricated` decls after `RootModule()`
  (:897), before the `inline ParsedModule(...)` ctor (:899).
- `lib/Parse/Parse.h`: restore `bool demand_fabricated{false};` on `ParsedModuleImpl` after `string_pool` (:590).
- Definitions already in restored `lib/Parse/Demand.cpp:130-262`.

## 4. `Query::Build` integration — `lib/DataFlow/Build.cpp` (the DataFlow one)
- Signature: add the 3 trailing params at :2524-2526 (match §2a).
- Transform call: insert after `if (!impl->ConnectInsertsToSelects(log, proxy_view_to_decl)) { return nullopt; }`
  (closes :2584), before the "wholesale-skip guard" comment (:2586):
  `if (!impl->ApplyDemandTransform(module, log, demand_mode, demand_retract, suppress_demand, proxy_view_to_decl)) return nullopt;`
  + the `num_errors != log.Size()` re-check. (Tip's own comment at :2575-2580 confirms `proxy_view_to_decl` is
  populated here exactly for this consumer.)
- K5 conservation belt (`#ifndef NDEBUG`, asserts each `recognized_subgraphs[i].demanded_decl` origin-reachable):
  before `return Query(std::move(impl));` (:2637).

## 5. Main.cpp flag + the Oracle fix
- `bin/drlojekyll/Main.cpp`: add `static bool gDemand = false;` near gPassPolicy (:48-51); change the call at
  :73 to `Query::Build(module, error_log, gPassPolicy, gDemand)`; add ONLY the `-demand`/`--demand`
  flag-parse arm (`hyde::gDemand = true;`) after the `-opt-bisect-limit=` branch (:600), before the search-path
  comment (:602). NO `-demand-instance`/`-demand-retract`; leave `Program::Build` (:110) untouched.
- `bin/Oracle/Main.cpp` (LOAD-BEARING): the `Query::Build` call at :749 MUST pass `suppress_demand=true`
  (else a restored RP-6 pragma-activation flaglessly demand-transforms `@key` modules inside the definitional
  referee, corrupting it). RefInterp/RefHarness never call `Query::Build` — no change.
- `runall.sh`/`diffrun.sh` `.drflags` mechanism is already generic — no harness change to reach `-demand`.

## 6. The validator surfaces (post-date demand's deletion — mostly NO-OP, one real fix + one deferred)
- **6a Regional census — REAL FIX (Planning.cpp `CollectMessages` :42-59):** `if (m.IsReceived())` →
  `if (m.IsReceived() && !query.IsDemandMessage(m))`. Feeds both `census.input_ports` (:675) and port-minting
  (:730-742); V-REGION-CENSUS (:901-906) self-corrects (re-derives from the same source). Without this the
  fabricated `demand__` message mints a real input port → belt aborts corpus-wide on any demand program.
- **6a Part 2 — Tier-1 naming lift (DEFERRED, separable):** `CollectDemandInteriorDecls`/`ResolveInteriorSupport`
  were deleted and never re-added through P2/P3/K5; `-region-out` will UNDER-NAME demanded interiors until
  re-implemented against the new typed model (new `BuildRelationSchemaFromInterior` parallel to
  `BuildRelationSchemaFromOrigin`). NOT a compile/correctness blocker — rank last / its own session.
- **6b RowContract.cpp / 6c origin_decls / 6d Rel eager-web — NO CHANGES** (see Key Corrections above).

## 7. ABI suppression — `lib/CodeGen/CPlusPlus/Database.cpp` (2 sites, gated on `program.Query().IsDemandMessage(*m)`)
- Message entry-point loop: `if (auto m = proc.Message(); m && program.Query().IsDemandMessage(*m)) continue;`
  after the `kMessageHandler` guard (:1440), before `EffectsOf(proc)` (:1442).
- "Friendly alias" loop (`EmitShapeStructs` :3143, per-message `using` loop :3145): identical guard after its
  `kMessageHandler` guard. (Both compile only once §2a's `IsDemandMessage()` decl exists.)
- OUT OF SCOPE (already zero-vestige at tip): `EmitSubgraphInstance`, `EmitInstanceStructs`, `ProcEffects::instances`.

## 8. Restoration ORDER (dependency-first) + build checkpoints
1. Types (§2a + §2b + §3) — together (Demand.cpp needs all simultaneously).
2. CMake (§1) — flips on compilation; expect loud failures = the intended API-drift surface.
3. CSE-migration hygiene (§2c).
4. `Query::Build` wiring (§4).
5. **CHECKPOINT 1**: DataFlow static lib compiles+links standalone (nothing outside DataFlow references the new
   symbols yet). WIP-commit here.
6. Main.cpp + Oracle (§5). **CHECKPOINT 2**: the binary accepts `-demand` and compiles a `.dr` through
   `Query::Build`; `-df-out` renders guard JOINs / demand relations (§2c Format.cpp included).
7. ControlFlow injector: `lib/ControlFlow/Build/Build.{h,cpp}` — the `BuildQueryInjectorFromRegistry` +
   registry-lookup loop + `context.demand_forcings` assignment (a bound `#query`'s injector proc is built from
   the `QueryDemandForcing` registry, not the absent parse-level forcing predicate). Needed before
   `Program::Build` links a demand-transformed graph. (The sibling `BuildQueryForceProcedureImpl` survives at
   tip — the vocabulary is unchanged; surprisingly clean.)
8. CodeGen ABI suppression (§7).
9. Regional census fix (§6a Part 1) — before exercising any `.region` golden / RegionInstance ctest on a
   demand program.
10. Tier-1 naming lift (§6a Part 2) — separable, rank last / defer.
11. Cosmetic comment rewords — optional.

**S1a exit gate**: build GREEN; demand OFF by default → the 181-case OptDiff SUITE PASS **byte-identical** +
ctest 5/5 (the flag-off containment gate: `ApplyDemandTransform` returns at its head when `!demand_mode &&
no @key pragma`). This proves the resurrection is orthogonal and the four validator surfaces are satisfied,
BEFORE any demand program is compiled (that's S1b: the recursive-TC witness + bench carrier).

## Top API-drift risks (ranked)
1. The two biggest files (ControlFlow `Build.{cpp,h}`, `RowContract.cpp`) are surprisingly CLEAN — F2 registry
   vocab unchanged (surviving sibling exercises it); RowContract untouched by the cut.
2. `lib/Regional/Planning.cpp` genuinely drifted (rewritten twice post-cut: P2 typed-owner, P3 request-ports).
   The Tier-1 lift (§6a Part 2) is the ONE place a naive hunk-reapply finds no anchors — re-target the new model.
   Do NOT confuse/revert P3's `census.request_ports`/`CountBoundQueryRedecls` toward the old `DemandForcings()` count.
3. `Program::Build` signature is a scope-creep trap — a mechanical ControlFlow-diff reapply resurrects the
   `bool demand_instance` 5th param + the whole InstanceStore family. Keep tip's 4-arg. Hand-pick hunks.
4. Vestigial dead InstanceStore scaffolding in `lib/Rel/Rel.h:855-890` — looks demand-related by name; leave it.
5. Mint-tag gap (cosmetic): restored Demand.cpp's ~25 `.Create()` sites are untagged (`mint_tag==nullptr`) —
   legal forever (incremental sweep), inconsistent-but-fine. Optional follow-on to `Mint(...)`.
6. Compile-order hazard: Database.cpp §7 + Planning.cpp §6a hard-depend on `IsDemandMessage()` (§2a) — do the
   headers first or get an unrelated-looking "no such member" error.
