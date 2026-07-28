# D3.a.0 — BINDING DESIGN (stages (b)/(c) adjudicated)

> **House banner.** Tip **42100428** at design; committed at the stage-(c)
> close after ORCHESTRATOR re-verification of the load-bearing anchors at code
> (Query.h:479/:249/:986/:1147/:1157; View.cpp:579-591 as-is; the folded_count
> zero-writer grep; Demand.cpp:392 + the #ifndef NDEBUG census; the
> demand@2587-before-Optimize@2599 order; Program.h:1162-1164 two-arg region
> ctor; Procedure.cpp:272/:284; Operation.cpp:512/:527 Hash/Equals;
> Database.cpp:1460-1462 store ctor emit; Program.h:1943 descriptor;
> Rel.h:880 + Rel.cpp:1055-1058 mint). Produced by the 2026-07-28 stage-(b)/(c)
> fleet: 2 xhigh design lanes + 2 fresh adversarial critics + 1 xhigh
> adjudicator (~555k tokens; 12 findings CONFIRMED+folded, 1 not-material,
> ZERO escalations — no OD-15 conflict). The mechanism choice in §1 (the
> QueryImpl* back-pointer, mechanism (B)) was DELEGATED latitude, not a ruling;
> mechanism (A) threading is the recorded fallback if the owner vetoes.
> Binding for the D3.a.0 stage-(d) prototype + pristine implementation.

---

## §1 SUB-DIFF (i) — THE OWN-3 PROMOTION (OQ-OWN3, RULED)

Promote the two dormant guard-annotation checks to always-on fprintf+abort
(DataFlow-layer idiom): the fold assert at `lib/DataFlow/View.cpp:588` becomes a
record-comparing incompatible-fold diagnostic, and the pre-Optimize annotation
census at `lib/DataFlow/Demand.cpp:1136-1147` drops its `#ifndef NDEBUG`. The
ruled mechanism decision was delegated; this draft ADOPTS mechanism (B) — a
narrow `QueryImpl *query` back-pointer stamped beside the existing index stamps
— over mechanism (A) threading (~27 call sites), on proportionality (no OD-15
ruling constrains the mechanism; (A) remains the fallback if the owner vetoes a
`QueryImpl*` on `QueryViewImpl`, §1.6).

### 1.1 State (all anchors verified at tip)

- `QueryViewImpl::guard_annotation_index{~0u}` — `lib/DataFlow/Query.h:479`
  (`~0u` = "not a guard view"). The FOUR index writers today: `Demand.cpp:996`,
  `Demand.cpp:1053` (stamps), `View.cpp:581` (MOVE), `View.cpp:590` (clear).
- `QueryImpl::guard_annotations` (`std::vector<GuardAnnotation>`) —
  `Query.h:1147`. `GuardAnnotation` is the PUBLIC struct
  `include/drlojekyll/DataFlow/Query.h:988-1019`; field order is
  `{kind, demand_side, role, is_instance_key, instance_key, guarded_read,
  demanded_view, forcing_index}` (verified — `forcing_index` is LAST and has no
  in-class default; load-bearing for §1.5).
- `QueryImpl::guard_annotation_folded_count{0u}` — `Query.h:1157`. **Declared,
  never incremented** (grep-verified: reader only at `Demand.cpp:1144`, no
  writer in `lib/`). This slice adds its SOLE increment (§1.3).
- Census: `Demand.cpp:1136-1147` under `#ifndef NDEBUG`; orphan-soundness
  comment at `Demand.cpp:1128-1135`. `ApplyDemandTransform` early-returns at
  `Demand.cpp:392` (`if (!demand_mode) return true;`) BEFORE any stamp/census.

### 1.2 The compatibility predicate + the pure check (AMENDED — folds MED-1)

The fold diagnostic must PRINT both RECORDS (not just indices). Factor a PURE
free function over two `GuardAnnotation`s — no views, no `QueryImpl` — mirroring
the RAT-3 `CheckInstanceOrder` idiom (`lib/Rel/Rel.cpp:4757`, unit-tested by
hand-built inputs). Declared in the private `lib/DataFlow/Query.h`; defined in
`lib/DataFlow/View.cpp`.

**The predicate (compatible ⟺ safe fold):**

```
a.forcing_index == b.forcing_index  &&  a.instance_key == b.instance_key
```

**Soundness argument [MED-1 — REQUIRED, verified at code].** CSE folds ONLY
`Equals` JOINs (`lib/DataFlow/Optimize.cpp` CSE loop, `if v1.Equals(v2): …
v1->ReplaceAllUsesWith(v2)`). `instance_key` is the join's pivot-position
vector — part of what `Equals` structurally compares — so it is **Equals-invariant
(fold-invariant)**: two guards cannot be `Equals` yet carry different stamped
pivot vectors. By contrast `demand_side`/`kind`/`role` are **non-structural site
STAMPS recorded PRE-CSE** that LEGITIMATELY differ across a valid fold (per the
record doc `Query.h:982-984` + `Demand.cpp:1050-1052`: on non-recursive
witnesses CSE folds the raw-seed TUPLE into the d-reader, so a `kRawSeed` and a
`kDReader` guard of the SAME forcing collapse); `guarded_read`/`demanded_view`
are opaque handles (`Query.h:1012-1013`, "equality/lookup ONLY") that differ by
construction in any two-view fold. Hence the predicate keys on exactly the two
fold-invariant identity fields and excludes the stamps. **In scope** the
`forcing_index` arm is vacuous (single adornment ⇒ one forcing) and the
`instance_key` arm can fire only on a same-forcing corrupted-key-layout bug —
the deeper bug it is meant to catch; **out of scope** (multi-adornment, OD-15
OQ-ADORN-KEY N disjoint stores) a cross-forcing collapse differs in
`forcing_index` and aborts — the intended mis-key alarm. `is_instance_key` is
always `false` this slice — excluded from the predicate (a future
recursive-subgoal slice revisits it).

```c++
// lib/DataFlow/Query.h (declarations, near QueryImpl members)
bool GuardAnnotationsCompatible(const GuardAnnotation &a,
                                const GuardAnnotation &b);
void CheckGuardAnnotationFold(const GuardAnnotation &loser,
                              const GuardAnnotation &survivor);
```

```c++
// lib/DataFlow/View.cpp — DataFlow-layer abort idiom = bare fprintf(stderr,…)
// + abort() that survives NDEBUG, matching lib/DataFlow/Format.cpp:766-795
// DF-BIJECTION [COSM-1: the idiom lives in lib/DataFlow/Format.cpp, NOT
// lib/ControlFlow/Format.cpp]. NOT the lib/Rel-internal file-static
// ValidatorFail helper.
bool GuardAnnotationsCompatible(const GuardAnnotation &a,
                                const GuardAnnotation &b) {
  return a.forcing_index == b.forcing_index &&
         a.instance_key == b.instance_key;
}

// Deref-FREE record print (handles as raw %p, never dereferenced) so the pure
// function is null-handle-safe for the death test's hand-built records.
static void PrintGuardAnnotation(const char *label, const GuardAnnotation &g) {
  fprintf(stderr,
          "  %s: forcing_index=%u kind=%u demand_side=%u role=%u "
          "is_instance_key=%d instance_key=[",
          label, g.forcing_index, unsigned(g.kind), unsigned(g.demand_side),
          unsigned(g.role), int(g.is_instance_key));
  for (unsigned i = 0u; i < g.instance_key.size(); ++i) {
    fprintf(stderr, "%s%u", i ? "," : "", g.instance_key[i]);
  }
  fprintf(stderr, "] guarded_read=%p demanded_view=%p\n",
          static_cast<const void *>(g.guarded_read.impl),
          static_cast<const void *>(g.demanded_view.impl));
}

void CheckGuardAnnotationFold(const GuardAnnotation &loser,
                              const GuardAnnotation &survivor) {
  if (GuardAnnotationsCompatible(loser, survivor)) {
    return;
  }
  fprintf(stderr,
          "OWN-3: incompatible guard-annotation fold — two distinct demanded "
          "instances collapsed into one survivor (mis-keyed instance). "
          "loser forcing_index=%u, survivor forcing_index=%u:\n",
          loser.forcing_index, survivor.forcing_index);
  PrintGuardAnnotation("loser   ", loser);
  PrintGuardAnnotation("survivor", survivor);
  abort();
}
```

### 1.3 The View.cpp call site + the NDEBUG-safe deref (AMENDED — folds MED-2, LOW-3)

The both-set arm reads `query->guard_annotations[...]` always-on. Guard the deref
with an **always-on** null check, not a debug-only assert [MED-2]: a future 5th
index writer that forgets the co-located `query` stamp would otherwise degrade
the always-on abort to a release SIGSEGV — defeating the whole "survives NDEBUG"
value proposition. Also clear `query` beside the index clear [LOW-3] so
INV-OWN3-Q is bidirectional and index/query move in strict lockstep.

```c++
// View.cpp:579-591 — AMENDED (mechanism (B); replaces the stale D3-deferral
// comment at View.cpp:583-587)
  if (guard_annotation_index != ~0u) {
    if (that->guard_annotation_index == ~0u) {
      that->guard_annotation_index = guard_annotation_index;
      that->query = query;                       // propagate (INV-OWN3-Q)
    } else {
      // Two annotated guards fold into one survivor. Always-on record-comparing
      // diagnostic (OWN-3, ruled): compatible (same instance + key) folds are
      // counted; an incompatible fold is a mis-keyed instance and aborts.
      if (!query) {                              // [MED-2] always-on, NDEBUG-safe
        fprintf(stderr, "OWN-3: annotated view with null query "
                        "(INV-OWN3-Q broken)\n");
        abort();
      }
      assert(query == that->query);              // debug: single-QueryImpl owner
      // [AS LANDED this line is SUPERSEDED — Fable review fix [2],
      //  d3a-desired-states.md SLICE 0: an ALWAYS-ON survivor-side owner
      //  guard (fprintf+abort on !that->query || that->query != query),
      //  View.cpp:668-674 at 0d33bdca.]
      CheckGuardAnnotationFold(query->guard_annotations[guard_annotation_index],
                               query->guard_annotations[that->guard_annotation_index]);
      ++query->guard_annotation_folded_count;    // §1.4 — the SOLE writer
    }
    guard_annotation_index = ~0u;
    query = nullptr;                             // [LOW-3] keep index+query paired
  }
```

`View.cpp` already `#include`s `Query.h`. `query` reaches `QueryImpl`'s members
directly (single owner: one `QueryImpl` per `Query::Build`, so
`this->query == that->query` in the both-set arm — the record lookups share it).

### 1.4 The new member + the stamps (AMENDED — folds COSM-3)

```c++
// Query.h:479 area — NEW member on QueryViewImpl. NOTE: at this point QueryImpl
// is only FORWARD-DECLARED (Query.h:249); it becomes complete later in the same
// header (Query.h:986). A pointer-to-incomplete member is legal — no include
// churn; the type is complete at deref time in View.cpp. [COSM-3]
  // Owning query, set ONLY on guard-annotated views (INV-OWN3-Q: non-null iff
  // guard_annotation_index != ~0u), for the OWN-3 fold diagnostic's record
  // lookup. nullptr for every other view. Never entered into any order.
  QueryImpl *query{nullptr};
```

```c++
// Demand.cpp:996 area — stamp beside the existing index stamp
  guard->guard_annotation_index =
      static_cast<unsigned>(guard_annotations.size());
  guard->query = this;                           // NEW (INV-OWN3-Q)
// ...identical addition at Demand.cpp:1053 (the query-projection guard).
```

**INV-OWN3-Q:** `guard_annotation_index != ~0u ⟹ query != nullptr`, pointing at
the owning `QueryImpl`. Maintained by exactly the two stamps + the MOVE-branch
propagation + the paired clear (§1.3). Bidirectional after LOW-3.

### 1.5 `guard_annotation_folded_count` — sole increment + byte-neutrality

Sole writer = the compatible-fold arm (§1.3). Byte-neutral at D3.a.0: that arm
is DORMANT on the corpus (proved — see §4 [BYTE]), so the counter stays 0; its
only reader (the census) runs PRE-Optimize (§1.6), before any CSE fold could
increment it, so the census always observes `folded == 0`. Keep the `folded`
term in the census equation (do NOT simplify to `n_stamped == size()`) so the
equation states the true invariant and survives any future pre-Optimize fold.

### 1.6 The census promotion + the dead-flow-orphan resolution (AMENDED)

**Orphan question, resolved AT CODE:** dead-flow elimination CAN delete an
annotated view with no fold (`PrepareToDelete` in
`lib/DataFlow/DeadFlowElimination.cpp` marks `is_dead`/severs uses; it does NOT
call `CopyDifferentialAndGroupIdsTo`, so `n_stamped` falls while
`guard_annotations.size()` stays) — but ONLY post-Optimize. The census sits at
the tail of `ApplyDemandTransform` (`Demand.cpp:1136-1147`), called at
`Build.cpp:2587`, BEFORE `impl->Optimize(...)` at `Build.cpp:2599` (verified:
demand-before-Optimize). At census time nothing has folded or been deleted:
`n_stamped == guard_annotations.size()` and `folded == 0` hold EXACTLY.
**Resolution: promote IN PLACE, pin pre-Optimize, no equation change.** Document
the pin loudly — never relocate/duplicate the census post-Optimize without
adding orphan accounting (no such accounting is built at .0; it would be dead
code).

```c++
// Demand.cpp:1128-1147 — AMENDED: drop #ifndef NDEBUG/#endif; the two asserts
// become always-on fprintf+abort. The block MUST remain here (pre-Optimize).
  // -------------------------------------------------------------------------
  // 11. ANNOTATION CENSUS (order-free counts; ALWAYS-ON, PRE-Optimize ONLY —
  //     dead-flow elimination (Build.cpp:2599 Optimize) deletes annotated
  //     views outright with no orphan bucket, so the equation is sound ONLY on
  //     this freshly-stamped graph; runs exactly once, here; NEVER relocate/
  //     duplicate post-Optimize without orphan accounting. OWN-3 (ruled):
  //     promoted always-on as the D3 multi-guard precondition.
  // -------------------------------------------------------------------------
  {
    auto n_stamped = 0u;
    ForEachView([&n_stamped](VIEW *v) {
      if (v->guard_annotation_index != ~0u) {
        ++n_stamped;
      }
    });
    if (n_stamped + guard_annotation_folded_count != guard_annotations.size()) {
      fprintf(stderr,
              "OWN-3: guard-annotation census mismatch: %u live-stamped + %u "
              "folded != %zu total stamped (pre-Optimize; a stamp/fold "
              "accounting bug)\n",
              n_stamped, guard_annotation_folded_count,
              guard_annotations.size());
      abort();
    }
    if (recognized_subgraphs.size() != demand_forcings.size()) {
      fprintf(stderr,
              "OWN-3: recognized-subgraph/forcing count mismatch: %zu "
              "subgraphs != %zu forcings\n",
              recognized_subgraphs.size(), demand_forcings.size());
      abort();
    }
  }
```

`Demand.cpp` already has `fprintf`/`abort` in scope (the DEMAND-TRIPWIRE at
`:1110`/`:1115`) — no new include.

### 1.7 The negative test (NEW) — the promoted fold abort (AMENDED — folds LOW-1)

**Idiom:** RAT-3/RAT-5 fork/waitpid death test, exactly as
`tests/RelValidators/InstanceOrderTest.cpp` (fork → child runs the pure check →
parent `waitpid` asserts `WIFSIGNALED` + `SIGABRT` on the death arm, exit 0 on
the positive arm). The pure factoring (§1.2) is precisely what lets the test
hand-build two `GuardAnnotation` records with no `QueryImpl`/view graph.

**[LOW-1 — MANDATORY]** Do NOT transcribe an aggregate-`{}`-only skeleton:
`GuardAnnotation loser{}` value-inits every field to 0 (incl. the trailing
`forcing_index`), so `loser{}` and `survivor{}` compare COMPATIBLE and the death
arm would FALSE-RED. The test MUST explicitly set the differing field:

```c++
// death arm: incompatible — distinct forcings collapse → MUST SIGABRT
GuardAnnotation loser{};    loser.forcing_index = 0u;    // instance_key = {}
GuardAnnotation survivor{}; survivor.forcing_index = 1u; // instance_key = {}
// child: CheckGuardAnnotationFold(loser, survivor) → SIGABRT.

// positive arm: compatible — same forcing + key, only site metadata differs
GuardAnnotation a{}; a.forcing_index = 7u; a.instance_key = {2u, 3u};
                     a.demand_side = GuardAnnotation::kDReader;
GuardAnnotation b{}; b.forcing_index = 7u; b.instance_key = {2u, 3u};
                     b.demand_side = GuardAnnotation::kRawSeed;
// child: CheckGuardAnnotationFold(a, b) → EXIT 0 (returns, no abort).
```

The two `QueryView` handle slots (`guarded_read`/`demanded_view`) are the one
build wrinkle — if `QueryView` is not default-constructible, hand-build two
throwaway concrete views on a local `DefList` purely to fill the slots (never
dereferenced; the print is `%p`). Confirm at build time; does not change design.

**Home — RECOMMENDED: a sibling `tests/DataFlowValidators/`** (OWN-3 is a
DataFlow-layer diagnostic; the existing `tests/RelValidators` is Rel-layer by
name/CMake). New `GuardAnnotationFoldTest.cpp` + own target linking `DataFlow`
(+ transitive `Display Lex Parse Util`) with `${PROJECT_SOURCE_DIR}/lib/DataFlow`
in `target_include_directories` (for the private `Query.h` declarations);
`add_test(NAME DataFlowValidators …)`. **Fallback:** fold the single file into
`RelValidators` (it already links `DataFlow` + forks) and accept the name
mismatch. Recommend the sibling for the clean boundary.

**Census abort coverage:** structural — runs on every `-demand` compile in the
suite; a directed census death test (hand-built `QueryImpl`, `size()==2` vs one
stamped view) is OPTIONAL, deferred (the equality already held under the debug
assert).

---

## §2 SUB-DIFF (ii) — THE INERT DIFF-SELECTOR PLUMBING (XC-2 + G-BELT-FLIP selector half)

One authority (`DRInstance.differential = TableIsDifferential(pub_table)`,
stamped at mint) feeds two carriers (the `SUBGRAPHINSTANCE` region ctor arg + the
`ProgramInstanceStore` descriptor) plus one new always-on coherence validator.
Nothing branches on the bit yet; the store-ctor emitter appends `, false` ONLY
when the bit is true, and the bit is provably `false` program-wide (§4), so every
pinned surface — incl. the eqgate nested arm — is byte-identical.

### 2.1 The single authority — `DRInstance.differential`

The mint already computes `const bool diff = TableIsDifferential(pub_table);` at
`lib/Rel/Rel.cpp:1055` and forks the effect set on it via `InstantiateEffects`.
`DRInstance inst_desc(...)` is built in the SAME loop iteration at `Rel.cpp:1058`
(`diff` in scope). `DRInstance` carries `pub_table` at **`lib/Rel/Rel.h:880`**
[COSM C1 — design said 876; actual 880].

```
// lib/Rel/Rel.h  (DRInstance, after pub_table :880)
  TABLE *pub_table{nullptr};       // answer INSERT target (the published rel)
+ bool   differential{false};      // == TableIsDifferential(pub_table); the
+                                  //    R-DIFF store selector. FALSE program-
+                                  //    wide today (pub monotone for every
+                                  //    demanded subgraph). Read by the region
+                                  //    ctor + the descriptor + V-INST-DIFF-COHERENCE.

// lib/Rel/Rel.cpp  ~:1058  (right after `DRInstance inst_desc(...)`)
  DRInstance inst_desc(*ri.demanded_view, *ri.pub_view);
+ inst_desc.differential = diff;   // ONE spelling, shared with InstantiateEffects
```

**Predicate choice + the D3.a.1 rider [L3 — carried FORWARD as first-class].**
`TableIsDifferential(pub)` is the store's monotone-belt gate (belt OFF exactly
when a touched iid can lose rows). This is INERT and correct at .0 (bit false
either way), but note: the death MINT gate keys on
`TableIsDifferential(demand_table)` (`Rel.cpp:1139`), a DIFFERENT predicate. The
two-axis reconciliation — store belt-off = `demand-diff || input-diff`, vs the
pub-diff proxy used here — is **load-bearing for D3.a.1 correctness and MUST be
first-class in that slice**, not a footnote (§3). The eqgate flat==nested is the
oracle.

### 2.2 The `SUBGRAPHINSTANCE` region diff bit (XC-2) — CTOR ARG

Region impl ctor is SOLE-called at `lib/ControlFlow/Build/Procedure.cpp:284`
(grep-verified: one `CreateDerived<SUBGRAPHINSTANCE>`, zero raw `new`). The
lowering already holds `const DRInstance &inst = dr_flow.instances[sid];`
(`Procedure.cpp:272`), so the bit is a free read — choose ctor arg over
re-derive (re-deriving would duplicate the authority and risk drift).
`CreateDerived` is variadic perfect-forwarding (`Util/DefUse.h:895-900`) — no
template change, only the ctor signature grows.

```
// lib/ControlFlow/Program.h  :1162-1164  (ProgramSubgraphInstanceRegionImpl)
- inline ProgramSubgraphInstanceRegionImpl(REGION *parent_, unsigned store_id_)
-     : OP(parent_, ProgramOperation::kSubgraphInstance),
-       store_id(store_id_) {}
+ inline ProgramSubgraphInstanceRegionImpl(REGION *parent_, unsigned store_id_,
+                                          bool differential_)
+     : OP(parent_, ProgramOperation::kSubgraphInstance),
+       store_id(store_id_), differential(differential_) {}
   ...  const unsigned store_id;  // :1187
+  const bool differential;   // == DRInstance.differential; read by
+                             //    EmitSubgraphInstance in D3.a.1. FALSE today.

// lib/ControlFlow/Build/Procedure.cpp  :283-284
  SUBGRAPHINSTANCE *const si =
-     impl->operation_regions.CreateDerived<SUBGRAPHINSTANCE>(seq, sid);
+     impl->operation_regions.CreateDerived<SUBGRAPHINSTANCE>(
+         seq, sid, inst.differential);
```

**Hash/Equals: LEAVE UNTOUCHED (proof).** `Hash` (`Operation.cpp:512-520`) keys
on `(op, store_id, pub_table->id)`; `Equals` (`:527-543`) on
`(store_id, pub_table.get())`. `differential` is a PURE function of `pub_table`,
so two regions that `Equals` today (same store_id + same pub_table pointer)
NECESSARILY carry the same `differential` — adding it to identity can never split
an already-merged pair, omitting it can never merge a differing pair. CF
procedure-dedup is bit-for-bit unchanged. (Adding a `const bool` member does not
perturb the hand-written Hash/Equals or copyability — regions are heap nodes
referenced by pointer, already non-assignable via `const unsigned store_id`.)

**Render: ZERO change.** `operator<<(ProgramSubgraphInstanceRegion)`
(`lib/ControlFlow/Format.cpp:659-672`) enumerates explicit tokens; add none. An
`.ir` token is dump GRAMMAR — the same unruled E-71 class as the retained
`deltarel` header residual; emitting one opens an E-71 lane, out of scope.
No golden contains `subgraph-instance`/`instance_` (grep-verified empty); the
only `-demand-instance` case (`demand_neighborhood_witness`) has no byte-pinned
dump golden. No accessor `ProgramSubgraphInstanceRegion::IsDifferential()` is
added this slice (dead public API until EmitSubgraphInstance branches — D3.a.1).

**[COSM C2] The region `differential` member is write-only (dead) at .0** — this
is intentional forward-plumbing; clang does not flag unused data members
(no `-Werror` risk). The `// read by EmitSubgraphInstance in D3.a.1` comment
above suffices for a skimming reviewer.

### 2.3 The store-ctor monotone selector (G-BELT-FLIP selector half)

The store member is constructed in the `Database` ctor init-list at the SOLE
site `lib/CodeGen/CPlusPlus/Database.cpp:1460-1462`, iterating
`program.InstanceStores()` — the DESCRIPTOR list, not the regions (codegen has no
region handle here). The descriptor `struct ProgramInstanceStore` is at
`lib/ControlFlow/Program.h:1943`; its build loop is at
`lib/ControlFlow/Build/Stratum.cpp:2326-2338` (`inst = flow.instances[i]` @2326,
`ProgramInstanceStore desc(i)` @2327, `push_back` @2338 — verified).

```
// lib/ControlFlow/Program.h  :1943  (ProgramInstanceStore)
  struct ProgramInstanceStore {
    unsigned id{0u};
    std::vector<TypeLoc> key_types;
    std::vector<TypeLoc> row_types;
+   bool differential{false};   // == DRInstance.differential; !differential ==
+                               //    InstanceStore monotone ctor arg. FALSE today.
    explicit ProgramInstanceStore(unsigned id_) : id(id_) {}
  };

// lib/ControlFlow/Build/Stratum.cpp  ~:2337  (before push_back @2338)
  const DRInstance &inst = flow.instances[i];   // :2326
  ProgramInstanceStore desc(i);                 // :2327
  ...
+ desc.differential = inst.differential;        // same DRInstance field as region
  impl->instance_stores.push_back(std::move(desc));   // :2338
```

**Public accessor** (mirrors `Program.cpp:241-253`). NOTE: `IsDifferential`
already exists on a DIFFERENT class (`ProgramDataTable`-family,
`include/drlojekyll/ControlFlow/Program.h:370`) — this adds a same-named method
to the DISTINCT class `ProgramInstanceStoreInfo` (`:1394`, `RowTypes` @1400); no
collision (member functions of different classes).

```
// include/drlojekyll/ControlFlow/Program.h  (ProgramInstanceStoreInfo, ~:1400)
  const std::vector<TypeLoc> &RowTypes(void) const noexcept;
+ // R-DIFF (D3.a): true iff the store can drop rows (belt off, monotone=false).
+ bool IsDifferential(void) const noexcept;

// lib/ControlFlow/Program.cpp  (after :253)
+ bool ProgramInstanceStoreInfo::IsDifferential(void) const noexcept {
+   return static_cast<const ProgramInstanceStore *>(impl)->differential;
+ }
```

**The emitter — emit the second ctor arg ONLY when differential**
(`Database.cpp:1460-1462`; current line: `hh << ",\n" << hh.Indent() << "
instance_" << store.Id() << "(allocator_)";`):

```
  for (const ProgramInstanceStoreInfo &store : program.InstanceStores()) {
-   hh << ",\n" << hh.Indent() << "  instance_" << store.Id() << "(allocator_)";
+   hh << ",\n" << hh.Indent() << "  instance_" << store.Id() << "(allocator_";
+   if (store.IsDifferential()) {
+     hh << ", false";   // monotone=false: HP-7 belt OFF for a droppable store
+   }
+   hh << ")";
  }
```

`InstanceStore`'s ctor is `explicit InstanceStore(Allocator, bool monotone_ =
true)` (`InstanceStore.h:66`). A monotone store relies on the default; the
`if`-gate keeps monotone stores emitting `instance_<id>(allocator_)` —
BYTE-IDENTICAL to tip (verified at runtime: `demand_neighborhood_witness` gen
`.h` line stays `instance_0(allocator_) {}`).

**emit-when-differential (CHOSEN) vs always-explicit (REJECTED):** always-explicit
would rewrite the eqgate nested arm's store ctor from `(allocator_)` to
`(allocator_, true)` — a gratuitous generated-text delta on the sole store-bearing
surface (would not FAIL the eqgate — answer-identity only — but muddies the
byte-inert claim). REJECT it; keep `.0` a genuine generated-text no-op.

### 2.4 V-INST-DIFF-COHERENCE (always-on, survives NDEBUG) — AMENDED (folds L1, C3)

Add a cheap always-on check that the stamped authority still equals the live
predicate, at the region lowering in `LowerSubgraphInstances`
(`lib/ControlFlow/Build/Procedure.cpp:265`), where `inst` (@272) and the pub
(`op->table_op_table`) are in scope. **[C3]** there is NO `pub` local — the check
uses `op->table_op_table` directly:

```c++
// Procedure.cpp, inside LowerSubgraphInstances (after `inst` @272; `op` in scope)
+ // V-INST-DIFF-COHERENCE: the stamped bit must equal the live
+ // TableIsDifferential(pub) authority (the InstantiateEffects fork,
+ // Rel.cpp:1055). A future edit that desyncs the mint stamp from the live
+ // predicate aborts here.
+ if (inst.differential != TableIsDifferential(op->table_op_table)) {
+   std::fprintf(stderr,
+       "error: SUBGRAPHINSTANCE store %u: stamped differential=%d != "
+       "TableIsDifferential(pub)=%d\n", sid, inst.differential,
+       TableIsDifferential(op->table_op_table));
+   std::abort();
+ }
```

`TableIsDifferential` is declared `Build.h:319`, defined `Build.cpp:705`, already
`#include`d in `Procedure.cpp` — no new include. Vacuous-green today
(all `differential==false`, `TableIsDifferential(pub)==false`); cannot false-fire
(`TableIsDifferential` is a deterministic scan of `pub->views`, re-evaluated on
the SAME pointer with no intervening mutation).

**[L1 — wording corrected]** This check pins the **AUTHORITY to the live
predicate** (`inst.differential == TableIsDifferential(pub)`). It does NOT compare
the region ctor arg or the descriptor field against `inst.differential`. Today
both carriers literally READ `inst.differential` at their set-sites (§2.2/§2.3),
so there is nothing to desync — the single real drift vector (mint-stamp vs live
predicate) is exactly what this catches. **D3.a.1 obligation:** when the bit goes
live, IF the two carriers ever stop reading the same `inst.differential` field,
add a carrier-vs-authority assertion at each set-site.

### 2.5 The complete inert diff (file:line)

| # | File:line | Change | Renders? |
|---|-----------|--------|----------|
| 1 | `lib/Rel/Rel.h:880` | `+ bool differential{false};` on `DRInstance` | no |
| 2 | `lib/Rel/Rel.cpp:1058` | `+ inst_desc.differential = diff;` | no |
| 3 | `lib/ControlFlow/Program.h:1162-1164,1187` | region ctor gains `bool differential_`; `+ const bool differential;` | no |
| 4 | `lib/ControlFlow/Build/Procedure.cpp:284` | ctor call gains `inst.differential` | no |
| 5 | `lib/ControlFlow/Build/Procedure.cpp` (LowerSubgraphInstances) | `+ V-INST-DIFF-COHERENCE` | no |
| 6 | `lib/ControlFlow/Program.h:1943` | `+ bool differential{false};` on descriptor | no |
| 7 | `lib/ControlFlow/Build/Stratum.cpp:~2337` | `+ desc.differential = inst.differential;` | no |
| 8 | `include/…/ControlFlow/Program.h:~1400` | `+ bool IsDifferential() const noexcept;` | no |
| 9 | `lib/ControlFlow/Program.cpp:~253` | `+ ProgramInstanceStoreInfo::IsDifferential` impl | no |
| 10 | `lib/CodeGen/CPlusPlus/Database.cpp:1461` | emit `, false` iff `store.IsDifferential()` | **gen text, only-when-true** |

Untouched by design: `Operation.cpp:512/527` (Hash/Equals), `Format.cpp:659-672`
(region render), `Rel.cpp:829-857` (InstantiateEffects fork — already keys on
`diff`), the (T,F) scan / partition belt / death lowering (all D3.a.1).

---

## §3 CROSS-SLICE CONSISTENCY — WHAT D3.a.1 INHERITS

1. **The region diff bit** (`ProgramSubgraphInstanceRegionImpl::differential`,
   §2.2) — D3.a.1 adds `ProgramSubgraphInstanceRegion::IsDifferential()` and wires
   `EmitSubgraphInstance` to branch on it (the (T,F) scan + partition belt =
   G-BELT-FLIP's REPLACEMENT half). The dead write-only field goes live there.
2. **The belt selector** (`ProgramInstanceStore::differential` → the store-ctor
   `, false`, §2.3) — becomes non-vacuously true when the first differential pub
   is admitted; the monotone `frozen ⊆ current` Seal belt (`InstanceStore.h:185-193`)
   goes OFF for that store.
3. **The store-diff predicate rider [L3 — FIRST-CLASS in D3.a.1]:** the store bit
   keys on `TableIsDifferential(pub)`; the death MINT gate keys on
   `TableIsDifferential(demand_table)` (`Rel.cpp:1139`). D3.a.1 MUST reconcile the
   two axes — confirm co-activation, or switch the store predicate to the explicit
   disjunction `TableIsDifferential(demand_table) || <input differential>`
   (V-INST-SOLE, `Rel.cpp:4334-4339`, keeps one instantiate deriver per pub but
   not one WRITER). NOT deferrable to a footnote.
4. **The death-lowering seam** — the full (T,F) retract + `RecycleCurrent` +
   netting/TouchedFlag/V-INST-FRESH coupling (OD-15) lands in D3.a.1; the OWN-3
   fold diagnostic (§1) is its precondition (multi-guard folds first go live under
   multi-adornment, D3.a.3, but the always-on teeth are seated NOW).
5. **The OWN-3 `folded_count` increment** (§1.5) is dead-but-correct at .0; it
   goes live when multi-guard folds first fire.
6. **V-INST-DIFF-COHERENCE liveness** (§2.4) + the OWN-3 fold-abort negative test
   for a TRUE bit are proven by directed perturbation when the bit first goes true
   — D3.a.1 owns those.

---

## §4 GATE FAMILY + PRE-REGISTERED PREDICTIONS

**Expected reds: NONE. Zero golden churn. This slice is all-green.**

### Sub-diff (i) — OWN-3

- **[BYTE] on ALL pinned surfaces** — `.stdout` ×175 × 4 modes, the eleven `.rel`
  + `.irgold` sidecars, `.ir`/`.df` dumps, `data/` rows: byte-identical to tip.
  Grounds, PROVED not asserted:
  - *Flag-off (172 non-demand cases × 4 modes):* `ApplyDemandTransform` returns at
    `Demand.cpp:392` before any stamp/census — **zero OWN-3 code executes**
    [LOW-2 — the definitive flag-off argument].
  - *Flag-on fold arm dormant:* the EXISTING debug assert `assert(that->index ==
    this->index)` fires on any both-set fold (distinct stamps ⇒ distinct indices);
    debug + ASAN suites pass to tip ⇒ the both-set branch is NEVER reached on the
    corpus ⇒ the new diagnostic + `++folded` never fire.
  - *Census:* the two equalities held under `#ifndef NDEBUG` across every
    debug/ASAN run; promotion only newly exercises them in RELEASE on the same
    graph — passes silently. `query` member is a nullptr-default pointer read only
    in the dormant arm; struct-layout change is compile-time only (not serialized).
- **SUITE PASS(175)** debug + release + ASAN both surfaces (release newly runs the
  always-on census/aborts — must pass silently).
- **ctest 5/5** debug + ASAN, INCLUDING the new fold death test (§1.7).
- **eqgate** (`demand_neighborhood_witness`) flat==nested==golden ×4 modes, LIVE —
  answer identity only (untouched by OWN-3).
- **Config-invariance single-hash** on `demand_tc_witness` (3-run debug + release)
  — the always-on census must be config-stable-silent.

### Sub-diff (ii) — plumbing

- **[BYTE] on ALL pinned surfaces** — the whole 175-case suite carries NO instance
  stores; the region/descriptor fields + coherence check are non-rendering,
  non-dedup, non-emitting. Grounds:
  - *The bit is `false` program-wide* — REACHABILITY-CLOSED: the demand body-walk
    rejects NEGATE/AGG (`Demand.cpp:624-627`); the `-demand-instance` pre-pass
    fences reject differential INPUT / recursive content / cyclic demand; the
    fabricated demand message is not `@differential`; **and** the STEP-2
    single-clause / single-materialization reject [L2] closes the
    independent-differential-writer hole (a multi-clause demanded pub rejects
    upstream: "A demanded query with multiple clauses is not yet supported"). So
    `TableIsDifferential(pub)==false` for the sole live carrier
    (`demand_neighborhood_witness`, op.0 = the R-MONO arm) and vacuously elsewhere.
  - *Hash/Equals untouched* (§2.2) ⇒ CF procedure-dedup identical ⇒ all compiled
    `.h`/`.df` unchanged. *Renders enumerate explicit tokens* ⇒ `.rel`/`.ir` gain
    zero bytes.
- **[EQGATE ×4 — exact generated-text delta on the nested witness arm: ZERO].**
  With `differential==false` the emitter emits `instance_0(allocator_)` —
  bit-for-bit tip (verified at runtime). Four-mode stdout ==
  `goldens/demand_neighborhood_witness.stdout` (answer identity trivially
  preserved: no generated-text change at all).
- **SUITE PASS(175)** all 4 modes; **ctest 5/5** debug + ASAN (incl.
  `RelValidators`, `InstanceStore` unit); ASAN both surfaces PASS(175).
- **[COHERENCE liveness]** V-INST-DIFF-COHERENCE is vacuous-green today (like
  V-INST-ORDER); its teeth are proven by directed perturbation only when the bit
  first goes true — deferred to D3.a.1 (§3.6).

**Loud call-out (§20(AE)):** the ONLY generated-text site (the store ctor,
diff #10) is gated `only-when-differential`, false everywhere today ⇒ zero bytes
move. Any byte movement on a pinned surface would be a design defect for this
slice; NONE is expected on either sub-diff.

---

## §5 ADJUDICATION RECORD (finding → disposition, verified at code)

Zero HIGH, zero live-miscompile, **zero OD-15 conflict → 0 escalations.** All 12
findings CONFIRMED at code and FOLDED into §1/§2.

### b1 / OWN-3 (c1 critique)

| Finding | Sev | Disposition | Verified |
|---|---|---|---|
| MED-1 predicate rationale omits the load-bearing Equals-invariance property | MED | **CONFIRMED → §1.2** states it: `instance_key` is Equals-invariant (pivot structure the CSE `Equals` compares); `demand_side`/`kind`/`role` are non-structural PRE-CSE stamps that legitimately differ across a fold | Optimize.cpp CSE folds via `Equals`; `instance_key` = pivot positions (`Query.h:1008-1010`); demand_side recorded pre-CSE (`Demand.cpp:1050-1052`) |
| MED-2 always-on deref guarded only by a DEBUG assert (NDEBUG SIGSEGV hole) | MED | **CONFIRMED → §1.3** adds an always-on `if(!query){fprintf;abort;}` before the record fetch | 4 index writers (`Demand.cpp:996/1053`, `View.cpp:581/590`); a future 5th forgetting the co-located `query` stamp degrades the always-on abort to release SIGSEGV |
| LOW-1 death-test `{}` skeleton under-inits → false-red | LOW | **CONFIRMED → §1.7** sets `forcing_index` explicitly per arm | `GuardAnnotation` field order verified: `forcing_index` LAST, no default ⇒ `{}` zero-inits both to 0 ⇒ compatible |
| LOW-2 byte-neutrality under-cites the flag-off early return | LOW | **CONFIRMED → §4** makes `Demand.cpp:392` the definitive flag-off argument | `if (!demand_mode) return true;` @392, before any stamp/census |
| LOW-3 `this->query` left stale after clear-on-move | LOW | **CONFIRMED → §1.3** adds `query = nullptr;` beside the index clear (bidirectional INV-OWN3-Q) | View.cpp:590 clears index only |
| COSM-1 abort-idiom anchor dir-unqualified | COSM | **CONFIRMED → §1.2** qualifies `lib/DataFlow/Format.cpp:766-795` | idiom in DataFlow Format.cpp; ControlFlow Format.cpp has no such abort |
| COSM-2 Optimize line 2599 vs 2600 | COSM | **NOT MATERIAL** — grep shows `impl->Optimize(...)` at Build.cpp:2599; the load-bearing fact (demand@2587 BEFORE Optimize) is confirmed; §1.6 uses 2599/2587 | Build.cpp:2587 / 2599 |
| COSM-3 "QueryImpl already a complete type" imprecise | COSM | **CONFIRMED → §1.4** restated: forward-declared at member point, complete at deref | QueryImpl fwd @249, complete @986, member @479 (pointer-to-incomplete legal) |

### b2 / plumbing (c2 critique)

| Finding | Sev | Disposition | Verified |
|---|---|---|---|
| L1 "transitively pins BOTH carriers" overstates V-INST-DIFF-COHERENCE | LOW | **CONFIRMED → §2.4** softened to "pins the AUTHORITY to the live predicate"; carrier-vs-authority assert handed to D3.a.1 | the check compares `inst.differential` vs `TableIsDifferential(pub)`, not the carriers |
| L2 §6.1 inertness proof leans on the rider, not the STEP-2 closure | LOW | **CONFIRMED → §4** cites the STEP-2 single-clause reject as the closure of the independent-writer hole | Demand STEP-2 multi-clause reject (empirically closed by c2 probe) |
| L3 store keys pub-diff; death mint keys demand-diff | LOW | **CONFIRMED → §2.1 + §3.3** carried FORWARD as a first-class D3.a.1 correctness item (inert at .0) | store `= TableIsDifferential(pub)` vs death gate `TableIsDifferential(demand_table)` @Rel.cpp:1139 |
| C1 anchor drift (Rel.h:876→880; store ctor line) | COSM | **CONFIRMED → §2.1/§2.5** anchors corrected to Rel.h:880, Database.cpp:1461 | `pub_table{nullptr}` @Rel.h:880; ctor loop 1460-1462 |
| C2 region `differential` write-only/dead at .0 | COSM | **CONFIRMED → §2.2** noted intentional forward-plumbing; no `-Werror` risk (unused data members unflagged); comment present | grep: only `IsDifferential()` reads the descriptor field; region field unread this slice |
| C3 §4.2 prose references non-existent `pub` local | COSM | **CONFIRMED → §2.4** uses `op->table_op_table` directly | LowerSubgraphInstances has no `pub` local; pub Emplaced from `op->table_op_table` @Procedure.cpp:289 |

**No finding rose to a conflict with any OD-15 ruling.** The mechanism choice
(back-pointer (B) vs threading (A)) was delegated, not ruled — ADOPTED (B) with
(A) fallback (§1). The predicate shape, `folded_count` feed, census soundness,
and the store selector are all within the delegated latitude.
