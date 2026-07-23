======================================================================
COMMITTED AT THE R3 LANDING (2026-07-22; Rel epoch, the THIRD step-kind
migration — landing record KeyedInstances.md §20(L)). The ADJUDICATED
R3 design contract: the monotone eager web's MERGE-union + SELECT-rebind
dispatch arms as effect-free marker ops kEagerUnion(22)/kEagerSelect(23)
on the M1-M9 mold. PROVENANCE: 3 code-grounded lanes (a1/a2/a3) -> xhigh
designer -> stage-(c) critique (3 fresh critics + xhigh adjudicator:
SOUND-AS-AMENDED, zero correctness defects, A1-A7 applied in place) ->
stage-(d) desired-states adjudication (blind-lane ModelTableOrNull
probe; AM-1..AM-9 applied — the kEagerUnion table-BACKED inversion,
E-107). OWNER RULINGS (2026-07-22, pre-code — §F, binding, cited as
ADJ-R3-1..10): 1 kEagerUnion + kEagerSelect spellings; 2 the union
A.6(c) arm STRENGTHENED (IsMerge && !InductionGroupId — mirrors the
mint predicate); 3 the SELECT A.6(c) arm strict IsSelect-only; 4 SELECT
renders NOTHING extra; 5 EXTRACT BuildEagerSelectRegion +
LowerRelStep_Select (family uniformity, mechanical id-stream); 6
grammar docs stay un-synced this slice (E-71 productions adjudicated
here + in r3-desired-states.md); 7 carriers = merge_2 + booleans +
elim-cond-cycle-simple, sidecars pin deltarel opt ONLY; 8 NO payload
field on either kind (M2'); 9 per-visit edge-multiplicity census pin
RATIFIED; 10 the six-red pre-bless set + git-status postcondition as
pre-registered. FABLE REVIEW (10-agent workflow, high): 3 verified
findings, ZERO live correctness — [1] the Induction.cpp:996 second
BuildEagerUnionRegion caller (dead at tip behind the
NeedsInductionCycleVector TODO short-circuit) is a LABELED coverage
hole outside the marker model, loud comment landed at the call site +
recorded residual (re-visit at R-final); [2]/[3] comment drift (four->
six family) FIXED; [R1] shared render-case labels REFUTED/declined
(dedicated blocks are contract-specified, §C.2). Fixes comment-only,
proven DUMP-NEUTRAL on all six carriers + post-fix suite/A-B green.
======================================================================

# R3 design — the eager web's plain-MERGE-union + SELECT-rebind arms as DR-IR ops

R3 = model the monotone eager web's **plain-MERGE-union** dispatch arm
(`Build.cpp:1189-1197`, the `else`/non-inductive leg → `BuildEagerUnionRegion`,
`Union.cpp`) and the **SELECT-rebind** dispatch arm (`Build.cpp:1233-1248`, the
unit-condition INSERT→RELATION→SELECT rebind, no builder file) as two new DR-IR op
kinds, lowered AT the original walk position — the R1/R2 strangler-fig mold,
instantiated for two more arms. PURE MODELING + LOWERING-IN-PLACE: **zero emission
change**, full-corpus 4-mode byte-identity vs the frozen R3-open baselines
(**debug c0a8a819 / release 958ddf8b**) is the hard gate. The ONLY output bytes
that move anywhere in the corpus are the `.deltarel` dumps: census-line-only on
the three existing carriers (E-101 logic), plus the new union/select carriers'
op blocks.

**The headline structural finding (both a1 and a2 independently, code-confirmed):**
R3 needs **NO new `DROp` payload field** and **NO `Context::EmittedEagerOp`
field** (the M2' rule, R2's headline, carries verbatim). A plain UNION carries no
operator/functor — its render identity is exactly `kEagerForward`'s (header +
optional `table=`, nothing more). A SELECT's rebind carries no scalar to render;
its ONE potentially-renderable fact — unit-condition-ness — is a **pure function of
the stored `eager_view`** (`QuerySelect::From(v).IsRelation() &&
QuerySelect::From(v).Relation().IsCondition()`, re-derivable at Format), so if the
owner ever wants a `cond=`/`rel=` token it re-derives with zero storage. Both
ctors are byte-for-byte the `MakeEagerCompareOp` shape
(`DeltaRel.cpp:1317-1323`), and `EmittedEagerOp` stays closed
(`{kind,view,table,sink,message}`, M7' precedent).

The five R1 load-bearing facts (F-ORDER / F-DUMPGUARD / F-CENSUSABORT /
F-SWITCHTOTAL / F-EFFECTDRIVEN) and the R2 mold deltas (M2' no-payload-field,
M6' ONE `IsEagerMarkerKind` predicate, M7' loud-abort name tables, M9
carrier-coverage-as-design-input) carry to R3 UNCHANGED. R3 introduces no new
mechanism; it adds two members to sets R1/R2 already parameterized — with ONE
genuinely new discrimination the precedent arms never had (the union arm's
`IsMerge` view-kind is AMBIGUOUS between inductive and plain; see §B/§F).

--------------------------------------------------------------------------------

## §A — THE DISPATCH CUTS (Build.cpp — the two mints + wrapper signatures)

Two arms of `BuildEagerRegion` change, each following the R1/R2
`LowerRelStep_*` cut precedent (`Build.cpp:1138-1172`). Both wrappers are peers of
`LowerRelStep_Forward`/`LowerRelStep_Compare`/`LowerRelStep_Generate`.

### A.1 The MERGE-union arm — mint ONLY on the non-inductive leg

Current (`Build.cpp:1189-1197`, confirmed at tip):

```cpp
} else if (view.IsMerge()) {
  const auto merge = QueryMerge::From(view);
  if (view.InductionGroupId().has_value()) {
    BuildEagerInductiveRegion(impl, pred_view, merge, context, parent,
                              last_table);            // NOT R3 — fixpoint machinery
  } else {
    BuildEagerUnionRegion(impl, pred_view, merge, context, parent, last_table);
  }
}
```

**R3:** mint the marker ONLY in the `else` (non-inductive) leg — the exact ADJ-R2-3
precedent (the MAP arm minted only on the `IsPure()` TRUE leg, left the impure
`else` mint-free). The inductive leg (`InductionGroupId().has_value()`) stays
**byte-untouched** — its round shells are Authority A (`LowerDRRounds`,
Stratum.cpp; rel-arch §4 M-note E-92), NOT a monotone eager push:

```cpp
  } else {
    const DROp op = MakeEagerUnionOp(view, ModelTableOrNull(impl, view));
    LowerRelStep_Union(impl, context, op, pred_view, merge, parent, last_table);
  }
```

Wrapper (sibling to `LowerRelStep_Forward` at `:1138`; the builder's full 6-arg
signature — `BuildEagerUnionRegion(impl, pred_view, merge, context, parent,
last_table)`, `Union.cpp:15`):

```cpp
static void LowerRelStep_Union(ProgramImpl *impl, Context &context,
                               const DROp &op, QueryView pred_view,
                               QueryMerge merge, OP *parent, TABLE *last_table) {
  RecordEagerDispatch(context, op);
  BuildEagerUnionRegion(impl, pred_view, merge, context, parent, last_table);
}
```

**Load-bearing (a1 §6.1):** `BuildEagerUnionRegion` NEVER reads its `pred_view`
parameter (the fold keys on `view`, the NDEBUG const-ref assert on
`view.Predecessors()`, the recursion on `view`) — it is forwarded purely for
call-site uniformity. The wrapper MUST accept it (signature match) but the marker
payload stores only `eager_view = the MERGE` (no invented `pred_view` field —
exactly the R1 kEagerForward, which stored only the TUPLE view). `last_table` IS
load-bearing (drives `InTryInsert`'s passthrough-vs-fold and threads into the
successor recursion) and is forwarded unchanged.

### A.2 The SELECT-rebind arm — extract-and-wrap (recommended) vs inline mint

Current (`Build.cpp:1233-1248`, confirmed at tip — the rebind loop + recursion are
INLINE in `BuildEagerRegion`; there is NO `BuildEagerSelectRegion` builder today):

```cpp
} else if (view.IsSelect()) {
  assert(pred_view.IsInsert());                       // (i) hard structural invariant
  const auto insert = QueryInsert::From(pred_view);
  auto i = 0u;
  for (auto col : view.Columns()) {                   // (ii) column rebind loop
    const auto in_col = insert.NthInputColumn(i++);
    parent->col_id_to_var[col.Id()] = parent->VariableFor(impl, in_col);
  }
  BuildEagerInsertionRegions(impl, view, context, parent, view.Successors(),
                             last_table);             // (iii) recurse into successors
}
```

**R3 RECOMMENDATION — option (A), extract-and-wrap** (keeps the `LowerRelStep_*`
family uniform and makes the id-stream argument mechanical; the a1 §4 lean):

1. Extract lines (i)-(iii) VERBATIM into a new builder
   `BuildEagerSelectRegion(impl, QueryView pred_view, QueryView select_view,
   context, parent, last_table)` (body byte-identical to the inline block, with
   `view` → `select_view`). This is a byte-MOVE of existing code into a function
   called at the identical walk moment with identical args, so id-stream identity
   is mechanical (the rebind loop mints ZERO `impl->next_id` — only `col_id_to_var`
   map writes; the FIRST `impl->next_id` is inside the untouched
   `BuildEagerInsertionRegions` descent — a1 §5).
2. The arm becomes:

```cpp
} else if (view.IsSelect()) {
  const DROp op = MakeEagerSelectOp(view, ModelTableOrNull(impl, view));
  LowerRelStep_Select(impl, context, op, pred_view, view, parent, last_table);
}
```

3. Wrapper:

```cpp
static void LowerRelStep_Select(ProgramImpl *impl, Context &context,
                                const DROp &op, QueryView pred_view,
                                QueryView select_view, OP *parent,
                                TABLE *last_table) {
  RecordEagerDispatch(context, op);
  BuildEagerSelectRegion(impl, pred_view, select_view, context, parent,
                         last_table);
}
```

`ModelTableOrNull(impl, view)` is computed ONCE at the mint site (a1 §5: pure
`.find()`-guarded lookup, no id, no allocation) and feeds the ctor. The
`assert(pred_view.IsInsert())` stays INSIDE the extracted builder (a build-time
invariant, NOT marker payload — a1 §5.5).

**Alternative — option (B), inline mint (owner may prefer, the a2 §12(b) lean):**
keep the rebind inline in `BuildEagerRegion`, insert `const DROp op =
MakeEagerSelectOp(view, ModelTableOrNull(impl, view)); RecordEagerDispatch(context,
op);` immediately before the `BuildEagerInsertionRegions` call — no
`LowerRelStep_Select` wrapper, no extraction. Lower-diff, but breaks the
`LowerRelStep_*` family uniformity. **FLAG OPEN (owner) — §F.5.** Either shape is
id-stream-identical (`RecordEagerDispatch` mints nothing, `Build.cpp:1123-1131`);
option (A) is this draft's recommendation for family consistency and for a
mechanical (not argued) id-stream proof.

### A.3 id-stream neutrality (mold M3 — mechanical, not argued per-case)

Both arms allocate ZERO `impl->next_id` on the mint path (a1 §5, verified):
`ModelTableOrNull` is a `.find()` + `FindAs->table` read; `MakeEagerUnionOp`/
`MakeEagerSelectOp` are POD-field stores on a `DROp(kind)` ctor
(`DeltaRel.h:665`, no `impl` arg); `RecordEagerDispatch` copies 5 POD/optional
fields + `push_back` on `context`, never `impl`. The wrapper then CALLS the
untouched builder (`BuildEagerUnionRegion` byte-unchanged; `BuildEagerSelectRegion`
= the moved inline body) with identical args at the identical walk moment ⇒ every
`impl->next_id++` fires in the identical order ⇒ ControlFlow IR byte-identical.
`Union.cpp`, `InTryInsert`, `BuildEagerInsertionRegions`, `BuildUpdateCount` stay
**byte-identical**. Enrollment (M4) is a later tail-append that mints no
`impl->next_id` (constructs `DROp`s into `flow.ops`, a Program-independent graph).

--------------------------------------------------------------------------------

## §B — THE MODEL ADDITIONS (DeltaRel.{h,cpp} — the coordinated sites)

Site-set inventory (a2's M6' table; the ONE-predicate mold keeps sites 7/8 free):

| # | file:site | current | R3 edit |
|---|---|---|---|
| 1 | DeltaRel.h `DROpKind` enum tail (:162-189) | 22 kinds, tail 21 | +2 ordinals (22/23) |
| 2 | DeltaRel.h `MakeEager*Op` decls | 4 decls | +2 decls |
| 3 | DeltaRel.cpp ctors (:1317/:1328 siblings) | 4 bodies | +2 bodies |
| 4 | DeltaRel.cpp `IsEagerMarkerKind` (:1306) | 4 disjuncts | +2 disjuncts |
| 5 | DeltaRel.cpp EAGER_WEB switch (:2424-2446) | 4-way + abort | +2 cases |
| 6 | DeltaRel.cpp A.6(c) switch (:3473-3510) | 4-way + abort | +2 arms |
| 7 | DeltaRel.cpp `key_of` lead-0 (:4424) | rides #4 | **free** |
| 8 | DeltaRel.cpp `DROpStratum` default→0u | — | **free** |
| 9 | Format.cpp `DROpKindName` (:116-119) | 4 cases | +2 cases |
| 10 | Format.cpp render switch (:869-937) | 4 dedicated cases | +2 dedicated cases (§C) |
| 11 | Format.cpp `kAllKinds[]` (:999-1011) | 22 entries | +2 entries; ADJ-S11 comment bumps at :990 ("22 DROpKind counts"→24) AND :1020 ("23rd"→"25th") — A3 |
| 11b | DeltaRel.h:643-644 EAGER_* family doc-comment | "R1: forward/insert; R2: compare/generate" + 4-kind enumeration | append "; R3: union/select" + `\| kEagerUnion \| kEagerSelect` — A4, same hygiene sweep |
| 12 | Build.cpp mints + wrappers | §A | +2 wrappers, edit 2 arms |
| 13 | grammar docs (E-71) | §C.4 | +2 productions (owner) |

### B.1 M1 — enum tail (site 1), EFFECT-FREE

Append after `kEagerGenerate` (21) in `DROpKind` (`DeltaRel.h:189`), ordinals
**22/23**, doc-commented in the R1/R2 block style:

```cpp
  kEagerUnion,     // (22) R3: MERGE-union dispatch (Build.cpp
                   //   BuildEagerUnionRegion — a merge that does NOT OWN an
                   //   InductionGroupId; it may still SIT inside a cycle,
                   //   dominated by the owning merge (Union.cpp:12-14 NOTE).
                   //   Dispatched via IsMerge() && !InductionGroupId()
                   //   (Build.cpp:1191); the owning-merge leg is Authority A,
                   //   no marker — A6.
                   //   EFFECT-FREE marker on the R1 mold; eager_view = the
                   //   QueryMerge; table_op_table = its model table via
                   //   ModelTableOrNull (non-null in the corpus-witnessed
                   //   case, e.g. merge_2 — `.df class=table-less` does NOT
                   //   imply model-table-null; E-107/AM, D.1). NO stored
                   //   payload (a union carries no operator/functor).
  kEagerSelect,    // (23) R3: SELECT-rebind dispatch (Build.cpp IsSelect arm —
                   //   the unit-condition INSERT->RELATION->SELECT rebind).
                   //   EFFECT-FREE marker; eager_view = the QuerySelect; table
                   //   = the merged model (shared with the condition relation's
                   //   INSERT via the SELECT<->pred-INSERT union rule — so
                   //   usually NON-null). Unit-condition-ness re-derives from
                   //   the view at Format if ever rendered (M2').
```

Tail-append keeps every pre-existing ordinal fixed (the 18/19/20/21 precedent).
The ordinal is not a sort key (M5) and nothing serializes it. Both EFFECT-FREE
(`op.effects` empty) ⇒ no vec/flag access ⇒ no §4 dep edge ⇒ invisible to
V-LINEAR/V-BAND-HAZARD/V-READY/V-LOOP (F-EFFECTDRIVEN).

### B.2 M2 — ctors (sites 2/3), byte-copies of MakeEagerCompareOp

```cpp
DROp MakeEagerUnionOp(QueryView merge_view, TABLE *table) {
  DROp op(DROpKind::kEagerUnion);
  op.ctx = Ctx::kEager;
  op.eager_view = merge_view;
  op.table_op_table = table;  // may be null; table_op_sign stays 0 (signless)
  return op;
}

DROp MakeEagerSelectOp(QueryView select_view, TABLE *table) {
  DROp op(DROpKind::kEagerSelect);
  op.ctx = Ctx::kEager;
  op.eager_view = select_view;
  op.table_op_table = table;
  return op;
}
```

Decls beside `MakeEagerCompareOp`/`MakeEagerGenerateOp` (`DeltaRel.h:972-982`).
NEITHER ctor takes an operator/functor — M2' (nothing derivable is stored). Both
invoked from BOTH the walk mint (M3) and the inventory enrollment (M4) — the
single-authority discipline.

### B.3 M6' — IsEagerMarkerKind (site 4), the ONE predicate

The load-bearing edit (missing it fails SILENTLY — the E-101/Fable-review lesson,
mint-comment `DeltaRel.cpp:1301-1305`):

```cpp
static bool IsEagerMarkerKind(DROpKind kind) {
  return kind == DROpKind::kEagerForward ||
         kind == DROpKind::kEagerInsert ||
         kind == DROpKind::kEagerCompare ||
         kind == DROpKind::kEagerGenerate ||
         kind == DROpKind::kEagerUnion ||
         kind == DROpKind::kEagerSelect;
}
```

This carries **site 7** (`key_of` lead-0 branch `:4424`,
`if (IsEagerMarkerKind(op.kind)) return Key{0u,0u,0u,op_table_id(op),0,oi};`) for
FREE — union/select markers get the same lead-0 off-lattice band, sign 0,
table-id-keyed, exactly like every other marker. **Site 8** (`DROpStratum`
`default: return 0u`) is also free (both markers are lead-0 → stratum 0; the render
`stratum=0` token comes from there). Its EXACTLY-two callers today (`:3462` +
`:4424`, verified §20(K)) both follow.

### B.4 M4 — EAGER_WEB enrollment (site 5), +2 cases before the loud-abort default

`BuildDRInventory` re-invokes the single-authority ctor per walk-recorded dispatch
(`DeltaRel.cpp:2424-2446`, a `std::vector` iterated by index, (F)-clean,
TAIL-APPENDED — **ADJ-S2 BINDING**: strictly after the ingest folds so ops 0/1
keep their `kIngestFold` construction indices). Add before the `default: abort()`:

```cpp
      case DROpKind::kEagerUnion:
        flow.ops.push_back(MakeEagerUnionOp(*rec.view, rec.table));
        break;
      case DROpKind::kEagerSelect:
        flow.ops.push_back(MakeEagerSelectOp(*rec.view, rec.table));
        break;
```

Both use ONLY `rec.view` + `rec.table` (the `rec.sink`/`rec.message` fields are
`kEagerInsert`-only, default `kNone`/`nullopt` in `RecordEagerDispatch`) — **NO new
`EmittedEagerOp` field** (M7' precedent, confirmed §20(K): `EmittedEagerOp =
{kind,view,table,sink,message}` already carries everything a payload-less marker
needs).

### B.5 M6 — A.6(c) structural recount (site 6), +2 arms before the loud-abort default

The A.6(c) loop (`DeltaRel.cpp:3461-3510`) skips non-markers via `IsEagerMarkerKind`
(site 4), asserts `eager_view.has_value()`, then switches on `op.kind` for the
view-kind check, then the shared merged-model table match. Add:

```cpp
      case DROpKind::kEagerUnion:
        if (!v.IsMerge()) {
          ValidatorFail("R3 A.6(c): a kEagerUnion op's view is not a MERGE");
        }
        break;
      case DROpKind::kEagerSelect:
        if (!v.IsSelect()) {
          ValidatorFail("R3 A.6(c): a kEagerSelect op's view is not a SELECT");
        }
        break;
```

The shared model-table check (`:3505-3508`,
`op.table_op_table != ModelTableOrNull(impl, v)`) holds UNCHANGED and is the
**DS-ADJ-7 render-authority enforcement** (the table is the union-find MERGED
model, never the `.df` per-view attribute) — for both markers,
`MakeEager*Op(view, ModelTableOrNull(impl, view))` passes exactly what the check
re-computes, so it holds by construction.

**The ONE genuine R3 discrimination question — the union arm's ambiguous view-kind.**
`QueryView::IsMerge()` is TRUE for BOTH inductive and non-inductive merges, but the
mint guard is `IsMerge() && !InductionGroupId().has_value()`. So the A.6(c)
`!v.IsMerge()` check UNDER-constrains relative to the mint predicate (unlike
`IsCompare`/`IsMap`/`IsTuple`/`IsInsert`, which are unambiguous). Two readings:

- **(a) strict precedent** — check only `!v.IsMerge()`. Uniform with the Compare/Map
  arms; leans on the standing controls (byte-identity A/B, eqgate, ADJ-S10 count
  read) for the inductive-vs-plain discrimination. An inductive merge mis-enrolled
  as a union marker would NOT be caught HERE but WOULD diverge the byte-identity A/B
  loudly (an inductive merge lowers through the fixpoint machinery, not the eager
  push — the dispatch is a Build.cpp bug, not an enrollment bug).
- **(b) strengthen** — also assert `!v.InductionGroupId().has_value()`. Costs one
  boolean, mirrors the mint predicate EXACTLY, and adds real discriminating power
  the Compare/Map precedent could not have (the kInstanceDeath `DROpStratum`
  strengthening precedent, `DeltaRel.cpp` :66-80, deliberately went beyond the
  default-0 shape).

**RECOMMEND (b) strengthen for the union arm** — because the mint's actual
predicate IS `IsMerge() && !InductionGroupId()`, so A.6(c) should re-check what the
mint actually asserted (that is precisely what A.6(c) is FOR — payload/kind
integrity independent of the walk), and it closes a gap the strict arm structurally
cannot. It is a <90% call (it breaks the "recount checks only the unambiguous view
kind" uniformity); **FLAG OPEN (owner) — §F.2.** The SELECT arm is UNAMBIGUOUS
(`IsSelect()` is exactly one kind), so keep it **strict `!v.IsSelect()`** — do NOT
strengthen with `IsRelation() && Relation().IsCondition()` (a2 §6: a non-condition
relation SELECT reachable from an INSERT would false-abort a strengthened arm; the
mint arm itself does not filter to unit-conditions beyond the reachability
structure, so a strengthen must be verified against the reachable select set first
— declined for R3). **FLAG OPEN (owner) — §F.3.**

### B.6 key_of / DROpStratum / the other switch(op.kind) sites — FREE

`key_of` (site 7) rides `IsEagerMarkerKind` — no edit. `DROpStratum` (site 8) rides
`default: return 0u` — no edit (correct: lead-0 markers → stratum 0). The remaining
`switch(op.kind)` sites (DROpStratum, internal validators, V-AGG/V-INST, op_pivot_hint,
op_band, scope_pair, op_round) fall through default arms UNCHANGED, exactly as R1/R2
confirmed (a2 site 8 note). `key_of` for a table-less union/select
(`op_table_id` sentinel 0) sorts in the LEADING table-less block by ctor; a
table-backed marker (merged model) sorts in that table's block. **Census 22 → 24**
(the M1 kind-set growth; §B.7).

### B.7 M6/M1 — census DAY ONE (site 11), kAllKinds → 24

`kAllKinds[]` (`Format.cpp:1010-1011`) appends `DROpKind::kEagerUnion,
DROpKind::kEagerSelect` at the tail (enum order) or the `census_total !=
flow.ops.size()` loud-abort fires (`:1020-1025`, F-CENSUSABORT — FORCED day one).
The census line grows by two tokens `kEagerUnion=<n> kEagerSelect=<n>` at the right
end. TWO count-in-comment bumps under the one ADJ-S11 hygiene item (A3): the
census banner `Format.cpp:990` `// ---- census (22 DROpKind counts, …)` → 24,
AND the abort-message comment `// a 23rd DROpKind not in kAllKinds`
(`Format.cpp:1020`) → `// a 25th DROpKind not in kAllKinds` (the
count-in-comment must not drift; 24 kinds ⇒ a missing one is the 25th).

--------------------------------------------------------------------------------

## §C — RENDER (Format.cpp — dedicated cases per M7; the §20(K) hazard)

### C.1 M2' — what is stored vs re-derived

- **UNION: NOTHING stored, NOTHING re-derived beyond `table=`.** A plain union is a
  pure fan-in; it carries no operator, no functor, no scalar. Its render is
  byte-identical to `kEagerForward`'s (header + optional `table=`). Confirmed a1
  §4, a2 §10.
- **SELECT: NOTHING must be stored** (M2' satisfied). The one potentially-renderable
  fact — unit-condition-ness — is a PURE FUNCTION of the stored `eager_view`
  (`QuerySelect::From(v).IsRelation() && QuerySelect::From(v).Relation().IsCondition()`;
  `QueryRelation::IsCondition()` `Query.h:118`; `QuerySelect::IsRelation()`
  `Query.h:512`). **RECOMMEND rendering NOTHING extra** (match `kEagerForward`
  exactly — the rebind is structurally invisible in the dump). A `cond=<0/1>` or
  `rel=<name>/<arity>` token is re-derivable if the owner wants the condition
  relation named — **FLAG OPEN (owner) — §F.4** (E-71-OPEN spelling if adopted).

### C.2 The two dedicated render cases (before the generic `default:` at `:927`)

Both byte-identical to the `kEagerForward` case (`Format.cpp:869-877`) — a plain
header + optional `table=`, no extra token:

```cpp
      case DROpKind::kEagerUnion: {
        os << " sign=" << SignGlyph(op.table_op_sign)
           << " ctx=" << CtxName(op.ctx)
           << " stratum=" << DROpStratum(flow, op) << "\n";
        os << "    args:";
        if (op.table_op_table) os << " table=" << tid(op.table_op_table);
        os << "\n";
        break;
      }

      case DROpKind::kEagerSelect: {   // RECOMMENDED: no extra token (§C.1)
        os << " sign=" << SignGlyph(op.table_op_sign)
           << " ctx=" << CtxName(op.ctx)
           << " stratum=" << DROpStratum(flow, op) << "\n";
        os << "    args:";
        if (op.table_op_table) os << " table=" << tid(op.table_op_table);
        os << "\n";
        break;
      }
```

Header tokens: `sign=·` (`SignGlyph(0)` → `\xC2\xB7`, `table_op_sign==0`),
`ctx=eager` (`op.ctx==kEager`), `stratum=0` (`DROpStratum` default). `table=`
OMITTED when null (the `if (op.table_op_table)` guard — `tid()` has NO null guard,
ADJ-S3). NO `reads:`/`effects:`/`spine:` sublines (effect-free). `DROpKindName`
(site 9, `Format.cpp:116-119`, F-SWITCHTOTAL forces it) gains
`case DROpKind::kEagerUnion: return "kEagerUnion";` +
`case DROpKind::kEagerSelect: return "kEagerSelect";`.

### C.3 The §20(K) SILENT-MISRENDER hazard — the dedicated case is LOAD-BEARING

The generic `default:` arm (`Format.cpp:927-936`) renders the header then
`emit_reads(op); emit_effects(op); emit_spine(op);` and NO `args:` line — a SILENT
mis-render for an effect-free marker (it would emit empty effects/spine sublines
and no `args:`). The census total check does NOT catch it (the op is still counted);
it surfaces only as a golden diff. **§20(K) explicitly names this as the next
slice's mandatory edit** ("Format.cpp:927 per-op generic render default silently
mis-renders an unhandled kind ... the next marker slice MUST add its dedicated
render case per M7"). Miss it and the union/select carriers diverge without a
compile-time abort — so C.2 is not hygiene, it is correctness.

### C.4 Grammar (E-71, site 13) — two op-kind productions

The grammar docs (`t2b-grammar.md` / `t2-dump-spec.md`) are a STANDING DEBT — they
still say "15 DROpKind counts" and were NOT re-synced at R1/R2 (a2 §13). R3's
grammar obligation, IF the owner keeps the docs current, is two `op.<idx>`
productions per E-71 (the p8 principle — every rendered token is an emitter
constant or a stored-field/pure-derived read; no-source tokens flagged LOUD):

- **kEagerUnion:** header `sign=· ctx=eager stratum=0`; args `table=%table:N`
  OMITTED-when-null (the `input=` OMIT precedent); nothing more. Census token
  `kEagerUnion=<n>` in enum order. NO reads/effects/spine sublines (state ABSENT).
- **kEagerSelect:** same header/args; the OPEN `cond=`/`rel=` token (§F.4), whose
  STORED-FIELD source if adopted is `eager_view` (the `cmp=`/`functor=` re-derived
  precedent, an acceptable stored-field read). Census token `kEagerSelect=<n>`.

**FLAG OPEN (owner) — §F.6:** whether R3 syncs the grammar docs this slice or
defers (the R1/R2 precedent left them un-synced; confirm at the ritual head).

--------------------------------------------------------------------------------

## §D — CARRIER PLAN (M9 — carrier-coverage-as-design-input)

### D.1 The M9 reachability finding (§20(K) as corrected by E-106; the table-
assignment ITSELF corrected by E-107) (E-107/AM: corrected at stage-(d) —
ModelTableOrNull probe.)

The three existing `.deltarel` carriers (`demand_tc_witness`, `symrec_tie_1`,
`map_3`) contain NO plain union / unit-condition select reached eagerly (symrec
census = 7F/1I no merge; map_3 acyclic no merge; tc's merges are inductive TC
recursion). So they get **census-line-only churn 22→24** (E-101 logic — census
renders zero-count kinds). Witnessing R3's op BLOCKS needs new carrier(s) (the
map_3/RAT-8 precedent).

The corrected M9 sweep (§20(K), E-106): **`.df class=table-less` acyclic merges
are PLENTIFUL** (47 corpus hits incl. merge_1..6, compare_6, select_2/4) and
walk-reached (never deletion-capable → never cut → InTryInsert no-fold
pass-through). **`.df class=table-backed` acyclic (non-inductive) merges = ZERO**
(all 74 class=monotone merges carry `; cycle` — they are inductive). BUT the
RENDER `table=` token does NOT read `.df class=` — it reads the ControlFlow
equivalence-set DataModel via `ModelTableOrNull`, and a direct probe (stage-(d))
shows merge_2's `.df class=table-less` merges (merge.10/merge.11) ARE
model-table-BACKED (`%table:4`/`%table:8`). ⇒ EVERY witnessed `kEagerUnion`
marker at opt renders **table-BACKED** (`table=%table:N`); the table-LESS union
arm (bare `args:`) stays **opt-UNWITNESSED** (appears only in nodf/none — a
labeled residual). The SELECT arm IS witnessed by the unit-condition shape
(booleans, prove_constant, elim-cond-cycle-simple; raw select-node counts overstate
— receive selects are walk roots).

### D.2 Recommended carriers (from a3's comparison table)

| carrier | opt U/S | nodf U/S | role | mode-pin |
|---|---|---|---|---|
| **merge_2** | 5 / 0 | 12 / 0 | **PRIMARY kEagerUnion** (rich, table-BACKED %table:4/%table:8 — E-107/AM, per-mode counts differ, fully clean topology, census-verified 10/5) | `deltarel opt` |
| **booleans** | 0 / 1 | 2 / 1 | **PRIMARY kEagerSelect** (0 merges isolate the 1 select; driver exists; table= arm PREDICTED — bless-pinned per DS-ADJ-7, the merged-model union, not observable at tip; §D.3/§G.3 — A5) | `deltarel opt` |
| elim-cond-cycle-simple | 0 / 1 | ~2 / 1 | **NEGATIVE GUARD** (inductive merge mints ZERO kEagerUnion — pins the InductionGroupId() branch) + unit select | `deltarel opt` |
| select_2 | 2 / 0 | 4 / 0 | minimal kEagerUnion companion | `deltarel opt` |

**RECOMMENDATION:**
- **REQUIRED (minimal sufficient): merge_2 + booleans** — two new `.irgold`
  sidecars pinning `deltarel opt`. merge_2 witnesses the table-BACKED kEagerUnion
  block (E-107/AM: corrected at stage-(d) — ModelTableOrNull probe) with
  per-mode-differing counts (5 vs 12 — a strong `.irgold` witness);
  booleans witnesses the kEagerSelect block in isolation (opt has zero merges) with
  the `table=` arm (see D.3).
- **STRONGLY RECOMMENDED: + elim-cond-cycle-simple** — a real CORRECTNESS witness
  (the only carrier that pins `InductionGroupId().has_value()` → an on-cycle merge
  minting ZERO kEagerUnion; a3's induction-skip guard). Cheap: an existing
  all-4-modes-clean green case, `.irgold deltarel opt`.
- **OPTIONAL: + select_2** — a minimal kEagerUnion companion (opt=2). Adds little
  over merge_2; owner may include for a smallest-mint witness. **DECLINE compare_6
  for R3** — its gt/lt spellings are R2's `kEagerCompare` surface (not R3), its opt
  union topology is murky (constant-fold hides edges), and merge_2 covers the
  union block more cleanly.

**FLAG OPEN (owner) — §F.7:** the carrier set (minimum merge_2 + booleans; add
elim / select_2 per owner). All are non-diagnostic all-4-modes-clean goldens with
NO existing `.irgold` (a new `.deltarel.opt.golden` + `.irgold` sidecar per adopted
carrier — RAT-8 seeding).

### D.3 The kEagerSelect `table=` prediction (booleans / elim)

The unit-condition SELECT is TABLE-BACKED by its own mechanism, independent of
the union's `ModelTableOrNull` lookup: `SELECT↔pred-INSERT` is an explicit union
rule (`lib/DataFlow/Build.cpp:2429-2436` — A2: a DATAFLOW anchor, the one anchor
outside the provenance header's byte-exact-verified ControlFlow/DeltaRel/Format
set; per DS-R2-4), so select.3 unifies its model with the condition relation's
INSERT (into `%table:4` for booleans / `%table:5` for elim, a3). Hence
`ModelTableOrNull(select)` is NON-null and the kEagerSelect marker renders
**`table=%table:N`**. This makes booleans/elim WITNESS the kEagerSelect `table=`
arm — alongside merge_2's table-BACKED kEagerUnion arm (D.1, E-107/AM), so R3
witnesses `table=%table:N` on BOTH new marker kinds at opt. Going-in prediction
`table=%table:4` (booleans opt); **[STRUCT] bless-pinned per DS-ADJ-7** (the
exact table id resolves at bless via the merged-model authority; the A.6(c)
match enforces whichever holds).

### D.4 Sidecar mode pin + referees

- **Mode pin: `deltarel opt` ONLY** per carrier (map_3/RAT-8 minimal-seeding
  precedent; ADJ-R2-6). The nodf/none census reads are MANUAL referee compiles
  (`-deltarel-out` by hand — the sidecar drives only the opt compile per the
  runall.sh mode gate; CRIT-FOLD c3-LOW-3 / DS-ADJ-1). **FLAG OPEN (owner) — §F.7b:**
  whether to also pin a second surface (`h opt` to lock codegen for free) — declined
  by default (OD-10 defer spirit; codegen is already gated corpus-wide by the frozen
  A/B).
- **ADJ-S7 same-workroot referee:** diff the ACTUAL bless-source
  `$WORKROOT/<case>/<case>.irgold/deltarel.opt.out` (first run: golden absent →
  IRGOLD-MISSING, not a separate compile).
- **ADJ-S10 structural count read:** read each new census against a3's derivation
  (merge_2 opt 5U/0S, booleans opt 0U/1S, elim opt 0U/1S) — the SOLE correctness
  check on the new columns. Read every count as **markers MINTED (per-VISIT)**, NOT
  distinct views: a plain MERGE with N walk-reached predecessors mints N kEagerUnion
  (a1 §3 — no walk dedup; census machinery tolerates arbitrary multiplicity). This
  is why merge_2 opt=5 (not 2): merge.11 visited 2× + merge.10 visited 3×. The R3
  owner ruling to pin: **kEagerUnion/kEagerSelect census counts are
  edge-multiplicities, not view-counts** (inherited R1 per-visit semantics).
- **Driver contracts:** booleans has `booleans.main.cpp`; merge_2/select_2/elim are
  all-4-modes goldens (drivers exist / no new driver). Keyed drains sorted per the
  cursor contract (existing drivers already comply).

### D.5 Pre-registered pre-bless divergence predictions

Exactly the map_3/R2 shape:
```
merge_2  irgold deltarel.opt IRGOLD-MISSING     (new golden absent first run)
booleans irgold deltarel.opt IRGOLD-MISSING     (idem)
elim-cond-cycle-simple irgold deltarel.opt IRGOLD-MISSING   (if adopted)
demand_tc_witness irgold deltarel.opt IRGOLD-DIVERGE   (census +2 cols)
symrec_tie_1      irgold deltarel.opt IRGOLD-DIVERGE   (census +2 cols)
map_3             irgold deltarel.opt IRGOLD-DIVERGE   (census +2 cols)
```
Everything else GREEN across all three PRE-bless runs. POST-bless: `SUITE: PASS
(173 cases)` ×3 (A1: the carriers are EXISTING .dr cases; .irgold/.deltarel
golden sidecars add ZERO caselist entries — runall.sh:418 counts cases/*.dr =
173; the R2 precedent stayed PASS(173) after map_3). NOTE: unlike R2 where map_3 was MISSING and
tc/symrec DIVERGE, R3 has THREE existing DIVERGE carriers (tc, symrec, AND map_3 —
all now hold a `.deltarel` golden) + the new carriers MISSING.

--------------------------------------------------------------------------------

## §E — GATES + MIGRATION RITUAL

### E.1 Standing per-slice gates (the R1/R2/R-a2 set, verbatim-applicable)

- **676-row knob-off A/B, ×2** — 0-diverged vs frozen **debug c0a8a819 / release
  958ddf8b** (the C.1/DS-R2-5 headline: zero emission change; id-stream identity
  per a1 §5 / §A.3). This is the SOLE miscompile referee — it nets that the
  recorded dispatch stream (hence emission) did not move.
- **post-baseline-4 A/B** — incl. the `demand_neighborhood_witness` nested eqgate ×4
  modes (R3 does not touch demand/instance lowering).
- **`data/` A/B** — all 4 modes clean (NOTE the pre-existing
  `data/self_testing_examples/evm_array_parse.dr` identical SIGABRT surfaced at R2 —
  record-only, out of R3 scope).
- **Suite ×3** — `runall.sh $WORKROOT`, must end `SUITE: PASS (173 cases)` (A1:
  sidecars add no caselist entries), run three times
  (a new ordering-sensitive Kahn input on every compile — the two new `flow.ops`
  members). Pre-registered PRE-bless reds = the §D.5 set.
- **ctest 5/5 (debug + ASAN)** — `DeltaRelValidators` UNAFFECTED (the A.6(c)
  extension is inline, corpus-covered, not a new fixture; `CheckInstanceOrder`
  untouched).
- **ASAN both surfaces ×2** — `build/asan` ctest + OptDiff under `DR=asan` + the
  env-CXX-wrapper second surface. R3 adds no new field/lifetime (the ctors take the
  same `QueryView`/`TABLE*` handles as R1/R2). Zero reports, SUITE PASS both.
- **config-invariance 3-run + debug==release single hash** — on the `.deltarel`
  dump for ALL carriers (tc, symrec, map_3, merge_2, booleans, [elim/select_2]):
  render `-deltarel-out` under both presets and `cmp -s`. Expected byte-identical.
- **E-62 re-grep clean** — no NEW `pinned_order`/`body_ops`/`output_ops` reader.
  `key_of`/EAGER_WEB/A.6(c) are EDITED (wider branches), none a NEW reader; the sole
  sanctioned external hit stays the `Stratum.cpp:1073` comment + the RAT-3
  `InstanceOrderTest` fixture.
- **Q5 ABABAB progsize@128, MEASURED (ADJ-S8)** — R3 adds at most a
  `RecordEagerDispatch` `push_back` per union/select dispatch + an O(1) ctor at
  inventory; `progsize@128` (a monotone chain) has few/no plain unions, so marginal
  cost ~zero. MEASURE anyway (E-77); expect noise (±1%).
- **bench-counter no-op — N/A** (Runtime untouched).

### E.2 The bless ritual order (the D.2 mold)

1. Land the code diff in one commit-shaped change: the 2 enum values, 2 ctors + 2
   decls, the 2 Build.cpp dispatch arms + 2 `LowerRelStep_*` wrappers (+ the
   `BuildEagerSelectRegion` extraction per §A.2(A)), the EAGER_WEB +2 cases, the
   `IsEagerMarkerKind` +2 disjuncts, the A.6(c) +2 arms, `DROpKindName` + `kAllKinds`
   (→24, comment bump) + the 2 render cases. `Union.cpp`/`InTryInsert`/
   `BuildEagerInsertionRegions`/`BuildUpdateCount` byte-unchanged.
2. Add the new `.irgold` sidecars (`deltarel opt`) per §D.2.
3. One full-suite run, ONE workroot (ADJ-S7 provenance). Pre-registered reds =
   §D.5; run ×3.
4. ADJ-S7 direct-diff referee on the ACTUAL bless-source files (accept: tc/symrec/
   map_3 census-tail-only ` kEagerUnion=0 kEagerSelect=0`, no other line moved; `cat`
   the new carriers' `.out` and check against §G).
5. ADJ-S10 structural count read (per-VISIT semantics; §D.4) + DS-ADJ-4 hexdump the
   first new eager op's `sign=·` (`\xC2\xB7`) and (booleans) the `table=%table:N`
   bytes + DS-ADJ-1 knob check (opt==nocf, nodf==none on the eager census; a df-axis
   delta is EXPECTED — merge_2 5→12).
6. Bless from the SAME workroot: `runall.sh --bless $WORKROOT <each carrier>`.
7. POST-condition tripwire: `git status` shows EXACTLY the three existing
   `.deltarel.opt.golden` (census +2 cols) + the new carriers'
   `.deltarel.opt.golden` + `.irgold` sidecars. If ANY `.h`/`.ir`/`.df` golden
   moves, R3's zero-emission-change claim is ALREADY violated — STOP. Re-run suite
   ×3.

### E.3 Tie to §19(H) — R3 GROWS the modeled set, retires NO seam piece

R3 retires NO seam artifact (S1 ingest-fold hole contract, S2 cut-successor
predicates + §7d cross-check, S3 V-INGEST-XCHECK, S4 E-42 shim ALL STAND). Its
§19(H) contribution is **coverage of the eager-web dispatch table**: after R3, the
TUPLE-forward, terminal-INSERT, CMP-filter, MAP-call, plain-MERGE-union, and
SELECT-rebind arms are all MODELED marker ops; only JOIN (index-probe / product),
NEGATE-gate, MERGE-inductive-feed (decided at its own ritual), and the E-42
table-less shim remain hand-coded-unmodeled (rel-arch §5, R4/R-JOIN/R-E42). Each
modeled arm is one fewer the R-final DIRECTION FLIP must convert. R3 also makes the
dump a MORE proportional image of the eager web (union/select blocks now appear) —
the §19(H) motivating measurement.

--------------------------------------------------------------------------------

## §F — OWED OWNER DECISIONS — ALL RULED (owner, 2026-07-22, pre-code):
## F.1 kEagerUnion + kEagerSelect; F.2 STRENGTHEN the union A.6(c) arm
## (IsMerge && !InductionGroupId — mirrors the mint predicate); F.3 SELECT
## arm strict IsSelect-only; F.4 SELECT renders NOTHING extra; F.5
## EXTRACT BuildEagerSelectRegion + LowerRelStep_Select wrapper; F.6
## grammar docs stay un-synced this slice (E-71 productions adjudicated
## in the contracts); F.7 carriers = merge_2 + booleans +
## elim-cond-cycle-simple; F.7b sidecars pin deltarel opt ONLY; F.8 no
## payload field (settled); F.9 per-visit edge-multiplicity census pin
## RATIFIED. (Original decision text retained below for lineage.)

1. **Kind/ctor/wrapper SPELLINGS** — `kEagerUnion` vs `kEagerMerge`; `kEagerSelect`
   vs `kEagerRebind`/`kEagerCondition`. Precedent is MIXED (kEagerForward = role,
   not kEagerTuple; kEagerGenerate = role/builder, not kEagerMap; kEagerCompare/
   kEagerInsert = view-kind). **RECOMMEND `kEagerUnion`** (tracks the builder
   `BuildEagerUnionRegion` + the fan-in role, the kEagerGenerate analogue) and
   **`kEagerSelect`** (view-kind, no distinct builder; the kEagerCompare analogue).
   All predictions/carrier tables below are built on these spellings.

2. **UNION A.6(c) arm** — strict `!v.IsMerge()` (precedent uniformity) vs strengthen
   with `!v.InductionGroupId().has_value()` (mirrors the mint predicate EXACTLY;
   closes the inductive-vs-plain gap Compare/Map never had; the kInstanceDeath
   strengthening precedent). **RECOMMEND strengthen (b)** — a <90% call, breaks the
   "recount checks only the unambiguous view kind" uniformity. §B.5.

3. **SELECT A.6(c) arm** — strict `!v.IsSelect()` (RECOMMENDED) vs strengthen with
   `IsRelation() && Relation().IsCondition()` (checkable but risks false-abort on a
   non-condition relation SELECT reachable from an INSERT — must verify the reachable
   select set first; declined for R3). §B.5.

4. **SELECT render token** — NOTHING (RECOMMENDED — match kEagerForward; the rebind
   is structurally invisible) vs an OPEN `cond=<0/1>`/`rel=<name>/<arity>` token
   re-derived from `eager_view` (E-71-OPEN spelling if adopted). §C.1.

5. **SELECT mint shape** — extract `BuildEagerSelectRegion` + `LowerRelStep_Select`
   wrapper (RECOMMENDED — family uniformity, mechanical id-stream) vs inline mint +
   `RecordEagerDispatch` before the descent (lower-diff, no wrapper). §A.2.

6. **Grammar-doc sync** — whether R3 brings `t2b-grammar.md`/`t2-dump-spec.md`
   current (they still say "15 DROpKind counts", un-synced since R1/R2). §C.4.

7. **Carrier set** — minimum **merge_2 + booleans** (`deltarel opt` each); add
   **elim-cond-cycle-simple** (strongly recommended — the induction-skip correctness
   guard) and/or **select_2** (optional minimal union companion); DECLINE compare_6.
   §D.2.
   - **7b. Sidecar surface** — `deltarel opt` ONLY (RECOMMENDED, RAT-8 minimal) vs
     also `h opt` (lock codegen for free). §D.4.

8. **PAYLOAD** — CONFIRMED NONE (M2'): no new `DROp` field, no new `EmittedEagerOp`
   field, for EITHER kind (a union carries no operator/functor; a select's
   unit-condition-ness is view-derivable). Recorded as settled, not a live question.

9. **Per-visit census pin** — ratify that kEagerUnion/kEagerSelect counts are
   EDGE-multiplicities (a plain MERGE with N reached preds mints N), not view-counts
   (inherited R1 per-visit semantics; merge_2 opt=5 witnesses it). §D.4.

--------------------------------------------------------------------------------

## §G — PRE-REGISTERED [BYTE]/[STRUCT] PREDICTIONS

Baseline = frozen **debug c0a8a819 / release 958ddf8b**. All pre-registered before
code lands. Tied to the carried pins: E-62 (no new pinned_order reader), the (F)
law (no pointer-ordered emission — the EAGER_WEB loop is a `std::vector` by index;
`key_of` orders by `table->id`/ctor), the T2b dump law (render from stored field /
pure function of one; loud-abort spelling tables), E-71 (productions §C.4), id-stream
identity (§A.3), ADJ-S2 (enrollment tail-append — ops 0/1 keep `kIngestFold`),
DS-ADJ-7 (render authority = the union-find MERGED model), M2' (no payload field),
census day one (M6/M1), M9 (carrier coverage).

### G.1 [BYTE] — full-corpus byte-identity, all 4 modes (the zero-emission gate)

`.df`, `.ir`, `.h`, `.cpp`, `stdout` **byte-identical corpus-wide, all 4
optimization modes, vs frozen c0a8a819/958ddf8b.** Mechanism (§A.3): mint + record +
inventory allocate no `impl->next_id`, Emplace no region; the wrappers call the
untouched builders at the identical walk moment. The committed non-`.deltarel` IR
goldens (symrec `{ir,df}`, tc `{ir,h,df}`) + `map_3.stdout` + the new carriers'
`.stdout`/existing goldens stay byte-unchanged. The 676-row knob-off A/B,
post-baseline-4 (incl. nested eqgate), and `data/` A/B all stay 0-diverged. **If any
`.h`/`.ir`/`.df` golden moves at bless → STOP.**

### G.2 [STRUCT] — the THREE existing carriers: census-line-ONLY churn 22→24

tc, symrec, AND map_3 (all three now hold a `.deltarel` golden) contain zero
plain-union / unit-condition-select views reached eagerly ⇒ NO new op BLOCKS ⇒
**census-line-only churn** (E-101 logic). Every op block, `rounds:`, `deps:`
byte-identical. The SOLE diff is the tail append ` kEagerUnion=0 kEagerSelect=0`.

Exact expected new census lines (extending the R2-blessed lines):
```
# demand_tc_witness (…kEagerForward=12 kEagerInsert=2 kEagerCompare=0 kEagerGenerate=0) + " kEagerUnion=0 kEagerSelect=0"
# symrec_tie_1      (…kEagerForward=7  kEagerInsert=1 kEagerCompare=0 kEagerGenerate=0) + " kEagerUnion=0 kEagerSelect=0"
# map_3             (…kEagerForward=3  kEagerInsert=3 kEagerCompare=1 kEagerGenerate=3) + " kEagerUnion=0 kEagerSelect=0"
```
These are the ONLY changed bytes in the three goldens.

### G.3 [STRUCT] — the NEW carriers' block shape (per a3 counts)

Counts are [BYTE]-derivable (per-visit == the a3-traced walk-visit count,
cross-checked against the landed kEagerForward/kEagerInsert); render ORDER + which
markers carry `table=` are [STRUCT] bless-pinned per DS-ADJ-7.

**merge_2 (PRIMARY kEagerUnion) — opt census [BYTE]:**
`kEagerForward=10 kEagerInsert=5 kEagerCompare=0 kEagerGenerate=0 kEagerUnion=5
kEagerSelect=0` (total 20 ops opt). merge.11 (2 visits) + merge.10 (3 visits) = 5.
nodf: `kEagerUnion=12` (5 merges un-fused), Forward=36, Insert=5. **[STRUCT]
(E-107/AM: corrected at stage-(d) — ModelTableOrNull probe):** all 5 kEagerUnion
markers are **table-BACKED** (`ModelTableOrNull(merge.10)=%table:4` ×3 visits,
`ModelTableOrNull(merge.11)=%table:8` ×2 visits — the `.df class=table-less`
label does NOT determine the render table; §D.1) → the 3 %table:4 markers sort
into the `%table:4` key_of block (preceding its fwd/ins pair by ctor) and the 2
%table:8 markers sort into the `%table:8` block; the LEADING lead-0 key_of band
holds ONLY the case's 5 table-less FORWARD markers, no union. Exact interleave
within each block [STRUCT] bless-pinned (A7). DS-ADJ-1: opt==nocf (20),
nodf==none (53); df-axis growth (5→12) EXPECTED (the extra 7 nodf-only unions are
genuinely table-less, CSE-off intermediate views).

**booleans (PRIMARY kEagerSelect) — opt census [BYTE]:**
`kEagerForward=6 kEagerInsert=2 kIngestFold=2 kEagerUnion=0 kEagerSelect=1`
(total 11 ops opt). nodf: `kEagerUnion=2 kEagerSelect=1`. **[STRUCT]:** the 1
kEagerSelect marker carries **`table=%table:4`** (SELECT↔pred-INSERT union →
merged-model table-backed; §D.3) — bless-pinned per DS-ADJ-7; witnesses the
kEagerSelect `table=` arm. opt has ZERO merges (fused into JOINs) so the select
count is isolated.

**elim-cond-cycle-simple (NEGATIVE GUARD, if adopted) — opt census [BYTE]:**
`kEagerForward=6 kEagerInsert=2 kEagerUnion=0 kEagerSelect=1`. The one merge
(merge.11 opt) is INDUCTION-OWNED (`InductionGroupId().has_value()` → routes to
`BuildEagerInductiveRegion`, mints ZERO kEagerUnion) — the correctness witness for
the mint guard. The kEagerSelect marker carries `table=%table:5` (cond_res proof).

**select_2 (OPTIONAL minimal, if adopted) — opt census [BYTE]:**
`kEagerForward=4 kEagerInsert=2 kEagerUnion=2 kEagerSelect=0` (merge.5, 2 visits).
**[STRUCT] (E-107/AM):** both kEagerUnion markers are **table-BACKED %table:4**
(model-table, same ModelTableOrNull inversion as merge_2) — sort into the
`%table:4` block, not the lead-0 band. nodf: `kEagerUnion=4` = 2×table-backed +
2×table-less (CSE-off intermediate union).

### G.4 [STRUCT] — corpus-wide census-line churn on EVERY program

Every program's `.deltarel` census line grows by two `=0` columns (` kEagerUnion=0
kEagerSelect=0`) — but only the carriers (3 existing + new) have a committed
`.deltarel` golden, so only they diff. All other programs render the wider census
only under `-deltarel-out` (never in suite compile lines; F-DUMPGUARD).

### G.5 The A/B + suite expectation (the EXACT pre-registered divergence set)

- 676-row knob-off A/B: **0-diverged** vs frozen c0a8a819/958ddf8b (×2). Ditto
  post-baseline-4 (incl. nested eqgate ×4 modes) and `data/` A/B.
- Suite PRE-bless reds = the §D.5 set (new carriers MISSING, tc/symrec/map_3
  DIVERGE). Everything else GREEN ×3. POST-bless: `SUITE: PASS (173 cases)` ×3 (A1).
- git-status: EXACTLY 3 existing `.deltarel.opt.golden` (census +2 cols) + the new
  carriers' `.deltarel.opt.golden` + `.irgold` sidecars. Nothing else.

### G.6 Labeled residuals carried (ADJ-S5-analog)

- **The table-LESS kEagerUnion marker (the bare `args:` union arm) is
  opt-UNWITNESSED** (E-107/AM: corrected at stage-(d) — ModelTableOrNull probe;
  merge_2/select_2's acyclic merges are `.df class=table-less` but
  model-table-BACKED, so the arm they witness at opt is `table=%table:N`, not
  bare `args:`). The table-less arm appears only in nodf/none (merge_2 7×, elim
  1×, select_2 2× — CSE-off intermediate unions), which are NOT golden-pinned.
  Re-derivable/enforceable — the A.6(c) merged-model match validates whichever
  holds always-on — but no golden pins the bare-`args:` token. A carrier with a
  genuinely model-table-null merge would witness it (future note; the R2
  full-width-passthrough-MAP residual analogue).
- **The MERGE-inductive-feed marker** is NOT R3 (round shells are Authority A);
  decided at its own ritual (rel-arch §5).
- **The ClassifyEagerSink replica / eager count oracle / publish-* spellings** carry
  unchanged to R-final (R3 adds no new instance of them).
