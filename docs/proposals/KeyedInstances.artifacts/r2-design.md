======================================================================
ADJUDICATED R2 DESIGN CONTRACT (Rel epoch, the SECOND step-kind
migration; committed with the R2 landing). Formulated as a DIFF ON THE
M1-M8 MOLD (rel-arch-pseudocode.md §4). Slice: the monotone eager web's
CMP filter arm (BuildEagerCompareRegions, Compare.cpp) + MAP
functor-call arm (BuildEagerGenerateRegion, Generate.cpp) as two
effect-free marker ops kEagerCompare(20)/kEagerGenerate(21). Census
20 -> 22. Third .deltarel carrier: map_3 (owner-ruled).
PROVENANCE: three code-grounded build-out lanes (a1-walkside /
a2-modelside / a3-carrier) -> xhigh designer -> THREE fresh adversarial
critics (correctness incl. id-stream/double-mint; model/grammar;
gates/harness — the correctness lane died at the structured-output cap
and was RECOVERED FROM DISK per the house rule), all at tip 3fc9d79f
(docs) atop 8fa156bc (R1 code). CRITIC VERDICTS: ZERO HIGH/MED across
all three lanes; 11 LOW findings, ORCHESTRATOR-ADJUDICATED AT THE CODE
and folded in below (each marked [CRIT-FOLD]); ONE finding REFUTED —
the "676-row A/B label is stale, should be 692" claim is EXACTLY the
E-90-refuted recompute (the gate is the FROZEN 169x4 baseline + the
separate post-baseline-4 set; map_3 predates demand and is INSIDE the
676). OWNER RULINGS (2026-07-22, pre-code): (i) kind spelling =
kEagerGenerate (not kEagerMap); (ii) MAP render token =
functor=<name>/<arity> (not the <name>_<pattern> codegen key);
(iii) map_3.irgold pins `deltarel opt` ONLY. ADJ-R2-0..8 below are
BINDING. SINGLE-PASS: the desired-states stage (r2-desired-states.md)
re-verifies against REAL dumps before code.
FABLE REVIEW (post-implementation, 10-agent workflow, pre-commit):
4 verified findings, ZERO correctness bugs. [1] the eager-marker
membership test unified into ONE IsEagerMarkerKind predicate shared by
the A.6(c) guard + the key_of lead-0 branch — FIXED; [2] the A.6(c)
kind→view-kind chain restructured as a SWITCH with the ADJ-R2-8a
loud-abort default (a fifth kind now aborts honestly) — FIXED; [4] the
R1 [1] note's stale TUPLE/INSERT kind set updated — FIXED; [3] the
eq/neq/lt/gt spelling now has THREE hand copies (two in
lib/DataFlow/Format.cpp) — ACCEPTED-DEFERRED to a dedicated hygiene
diff (unification touches the .df emitter, outside R2 scope; marked at
the ComparisonOperatorName site). All fixes proven DUMP-NEUTRAL (three
carriers byte-identical to the blessed goldens post-fix) + post-fix
suite/A-B re-run green.
======================================================================

# R2 design — the eager web's CMP-filter + MAP-call arms as DR-IR ops

R2 = model the monotone eager web's **CMP filter** dispatch arm
(`Build.cpp:1201-1202`, `BuildEagerCompareRegions`) and **MAP functor-call**
dispatch arm (`Build.cpp:1191-1197`, `BuildEagerGenerateRegion`) as two new
DR-IR op kinds, lowered AT the original walk position — the R1 strangler-fig
mold, instantiated for two more arms. PURE MODELING + LOWERING-IN-PLACE: **zero
emission change**, full-corpus 4-mode byte-identity vs frozen **a4b807dc**
(`scratchpad/frozen-a4b807dc/{drlojekyll-debug,drlojekyll-release}`) is the hard
gate. The ONLY output bytes that move anywhere in the corpus are the `.deltarel`
dumps: census-line-only on the two existing carriers (E-101), plus the new
`map_3.deltarel.opt.golden` (RAT-8 first-ever, the sole carrier with non-zero
CMP/MAP op blocks).

**Naming decision (see ADJ-R2-0):** the two kinds are **`kEagerCompare`** (20)
and **`kEagerGenerate`** (21). `kEagerGenerate` (not `kEagerMap`) tracks the
region-builder name `BuildEagerGenerateRegion` and the "generative functor call"
role, and is the spelling the carrier predictions (A3) are built on.

**The headline structural finding (both lanes independently, code-confirmed):**
R2 needs **NO new `DROp` payload field** and **NO `Context::EmittedEagerOp`
field**. The CMP comparator and the MAP functor are PURE FUNCTIONS of the
already-stored `eager_view` — `QueryCompare::From(*eager_view).Operator()` and
`QueryMap::From(*eager_view).Functor()` — derivable at Format time exactly as
`agg_name` derives the aggregate functor name from `agg_view`
(`Format.cpp:588-591`). This is the T2b "render from a stored field or a pure
function of one" law. Contrast `kEagerInsert`, which HAD to store
`eager_sink`/`eager_message` because `ClassifyEagerSink` needs `context` and the
message needs the insert — neither reachable at Format time. The CMP operator and
MAP functor ARE reachable from the view, so storage would double-represent.

The five R1 load-bearing facts (r1-design.md §0, F-ORDER / F-DUMPGUARD /
F-CENSUSABORT / F-SWITCHTOTAL / F-EFFECTDRIVEN) carry to R2 UNCHANGED and are the
scaffolding every decision below rests on. R2 introduces no new mechanism; it
adds two members to sets R1 already parameterized.

---

## §A — THE DIFF ON THE MOLD (M1..M8)

For each mold point (rel-arch-pseudocode.md §4, lines 218-251), what R2 does.
Mostly "same, instantiated for kEagerCompare/kEagerGenerate." Every edit site was
re-read at tip; anchors inline.

### M1 — op kinds at the ENUM TAIL, EFFECT-FREE

**Same.** Append `kEagerCompare` (20), `kEagerGenerate` (21) after `kEagerInsert`
(19) in `DROpKind` (`DeltaRel.h:169-177`, the R1 kEagerForward/kEagerInsert block
is the immediate template). Both EFFECT-FREE — `op.effects` stays empty, so they
contribute NO vec/flag access, hence NO §4 dep edge, hence are invisible to
V-LINEAR/V-BAND-HAZARD/V-READY/V-LOOP (F-EFFECTDRIVEN, the kIngestFold exclusion
generalized). The enum ordinal is NOT a sort key (M5) and nothing serializes it,
so tail-append is cosmetic — its ONLY load-bearing effect is keeping every
pre-existing ordinal fixed (the 18/19 precedent).

### M2 — ONE single-authority ctor per kind

**Same shape.** Two new pure ctors in `DeltaRel.cpp`, byte-copies of
`MakeEagerForwardOp` (`DeltaRel.cpp:1279-1285`) with the kind swapped, NO ids, NO
effects:

```
DROp MakeEagerCompareOp(QueryView cmp_view, TABLE *table):
    DROp op(kEagerCompare); op.ctx = kEager;
    op.eager_view = cmp_view; op.table_op_table = table;   // usually null
    return op                                              // no ids, no effects

DROp MakeEagerGenerateOp(QueryView map_view, TABLE *table):
    DROp op(kEagerGenerate); op.ctx = kEager;
    op.eager_view = map_view; op.table_op_table = table;
    return op                                              // no ids, no effects
```

Declarations beside `MakeEagerForwardOp`/`MakeEagerInsertOp` at `DeltaRel.h:956-957`.
Note NEITHER ctor takes the operator/functor — those re-derive from `eager_view`
at Format time (the headline finding; ADJ-R2-1/2). Both invoked from BOTH the
walk mint (M3) and the inventory enrollment (M4) — the single-authority discipline.

### M3 — walk-time: mint → RecordEagerDispatch → CALL the untouched builder

**Same, two more arms.** Two new dispatch cuts in `BuildEagerRegion`
(`Build.cpp:1155-1249`), each mirroring the R1 TUPLE/INSERT cuts
(`Build.cpp:1225-1240`). The two thin wrappers are peers of
`LowerRelStep_Forward`/`LowerRelStep_Insert` (`Build.cpp:1138-1152`):

```
static void LowerRelStep_Compare(impl, context, const DROp &op,
                                 QueryCompare cmp, OP *parent):
    RecordEagerDispatch(context, op)                       # Build.cpp:1123-1131
    BuildEagerCompareRegions(impl, cmp, context, parent)   # UNTOUCHED body

static void LowerRelStep_Generate(impl, context, const DROp &op,
                                  QueryView pred_view, QueryMap map,
                                  OP *parent, TABLE *last_table):
    RecordEagerDispatch(context, op)
    BuildEagerGenerateRegion(impl, pred_view, map, context, parent, last_table)
```

**Wrapper-signature note (code-confirmed at tip):** the two builders have
DIFFERENT signatures — `BuildEagerCompareRegions(impl, cmp, context, parent)`
(`Compare.cpp:90`, NO `pred_view`, NO `last_table`) vs
`BuildEagerGenerateRegion(impl, pred_view, map, context, parent_, last_table_)`
(`Generate.cpp:85-87`). The wrappers MUST match; `LowerRelStep_Compare` forwards
neither `pred_view` nor `last_table`. `RecordEagerDispatch` (`Build.cpp:1123-1131`)
and `Context::EmittedEagerOp` (`Build.h:245-252`) need **NO edit** — they already
copy `kind/view/table/sink/message` generically; a CMP/MAP op records with
`sink=kNone`/`message=nullopt` unchanged.

**The two dispatch-arm cuts, EXACT insertion points:**

MAP arm — replace the `IsPure()` true-branch body (`Build.cpp:1191-1197`); leave
the impure `else` (`:1198`, `assert(false)`) UNTOUCHED — no mint (ADJ-R2-3):
```
} else if (view.IsMap()) {
    auto map = QueryMap::From(view);
    if (map.Functor().IsPure()) {
        const DROp op = MakeEagerGenerateOp(view, ModelTableOrNull(impl, view));
        LowerRelStep_Generate(impl, context, op, pred_view, map, parent,
                              last_table);
    } else {
        assert(false && "TODO(pag): Impure functors");   # untouched, no mint
    }
```

CMP arm — replace `Build.cpp:1201-1202`:
```
} else if (view.IsCompare()) {
    const DROp op = MakeEagerCompareOp(view, ModelTableOrNull(impl, view));
    LowerRelStep_Compare(impl, context, op, QueryCompare::From(view), parent);
```

**id-stream-neutrality argument (mold M3, mechanical — not argued case-by-case).**
The mint + record path allocates ZERO `impl->next_id`:
- `MakeEagerCompareOp`/`MakeEagerGenerateOp` assign POD fields only (byte-copies
  of `DeltaRel.cpp:1279-1285`); the `DROp(kind)` ctor (`DeltaRel.h:649`) sets only
  `kind`; `ModelTableOrNull` (`DeltaRel.cpp:1267-1273`) is a `.find()`-guarded
  read (M8), no mutation, no id.
- `RecordEagerDispatch` copies a POD struct + `push_back` (`Build.cpp:1123-1131`).
- The wrapper then CALLS the UNTOUCHED region builder with the identical arguments
  at the identical walk moment, so every `impl->next_id++` inside it (and inside
  the descent it drives) fires in the identical order. The CMP arm allocates
  ZERO `impl->next_id` at all (`CreateCompareRegion` uses `CreateDerived` +
  existing vars — A1 §1.2); the MAP arm allocates `1 + #free-params` inside
  `CreateGeneratorCall` (`Generate.cpp:17-18,27-28`), all UNMOVED because the
  builder is entered unchanged. `Compare.cpp`, `Generate.cpp`, `InTryInsert`,
  `CreateCompareRegion`, `CreateGeneratorCall` stay **byte-identical**.

### M4 — inventory enrollment: re-invoke ctor, TAIL-APPENDED after ingest folds

**One decision-bearing edit** (ADJ-R2-8a). The EAGER_WEB block
(`DeltaRel.cpp:2389-2396`) currently branches BINARY —
`if (rec.kind == kEagerForward) MakeEagerForwardOp(...) else MakeEagerInsertOp(...)`.
With four kinds the bare `else` would mis-route CMP/MAP into `MakeEagerInsertOp`.
It becomes a **4-way dispatch on `rec.kind`**:
```
for rec in context.emitted_eager_ops:            # walk (DFS) order, (F)-clean vector
    switch (static_cast<DROpKind>(rec.kind)):
      kEagerForward:  flow.ops.push_back(MakeEagerForwardOp(*rec.view, rec.table));
      kEagerCompare:  flow.ops.push_back(MakeEagerCompareOp(*rec.view, rec.table));
      kEagerGenerate: flow.ops.push_back(MakeEagerGenerateOp(*rec.view, rec.table));
      kEagerInsert:   flow.ops.push_back(MakeEagerInsertOp(*rec.view, rec.table,
                                          static_cast<EagerSink>(rec.sink),
                                          rec.message));
```
The block STAYS strictly after the INGEST_FOLD loop (**ADJ-S2 BINDING, inherited**)
so the ingest folds keep construction indices 0/1 — R2 shifts no pre-existing op's
ctor index. Effect-free ⇒ no vec def-edge (unchanged). The re-invocation stays
walk-ordered and (F)-clean (a `std::vector` iterated by index).

### M5 — key_of lead-0 off-lattice; ORDER LAW (op_table_id, sign, ctor)

**One decision-bearing-but-forced edit** (ADJ-R2-4). Extend the existing eager
`key_of` branch (`DeltaRel.cpp:4355-4358`) to OR-in the two new kinds:
```
if (op.kind == kEagerForward || op.kind == kEagerInsert ||
    op.kind == kEagerCompare || op.kind == kEagerGenerate)
  return Key{0u, 0u, 0u, op_table_id(op), 0, oi};
```
CMP/MAP markers ARE the pre-phase monotone push path exactly like TUPLE forwards,
so they belong in the SAME lead-0 off-lattice band, sign-0 (signless), keyed
`(op_table_id, sign=0, ctor=oi)`. For a table-less CMP/MAP (`table_op_table`
null), `op_table_id` returns the sentinel 0 → they sort in the LEADING table-less
block by ctor (walk order); a MAP whose merged model has a table sorts in that
table's block. The enum ordinal (20/21) carries ZERO ordering weight (ctor is the
final tie-break). No `op_band`/`op_stratum` consulted (the branch returns before
the default arm). The seven other `switch(op.kind)` sites fall through their
default arms UNCHANGED (a2 §E1-E7, all re-confirmed: DROpStratum default→0,
internal-validators default→skip, V-AGG/V-INST default break, op_pivot_hint
default→~0u, op_band default→0/never-called, scope_pair default→kEpoch,
op_round default→nullopt).

### M6 — census DAY ONE + the structural recount; MERGED-model render authority

**Two edits, one decision-bearing** (ADJ-R2-7). (a) `kAllKinds[]`
(`Format.cpp:944-955`) grows to 22 (append the two kinds at the tail, enum order)
or the `census_total != flow.ops.size()` loud-abort fires (`Format.cpp:964-969`,
F-CENSUSABORT) — FORCED day one. (b) The A.6(c) structural recount
(`DeltaRel.cpp:3413-3440`) extends: the skip-guard (`:3413-3414`) admits the two
kinds, and the kind→view-kind arm becomes 4-way — `kEagerForward→IsTuple`,
`kEagerInsert→IsInsert`, `kEagerCompare→IsCompare`, `kEagerGenerate→IsMap`
(`QueryView::IsCompare`/`IsMap`). The merged-model table match (`:3436`,
`op.table_op_table == ModelTableOrNull(impl, v)`) holds UNCHANGED and is the
**DS-ADJ-7 render-authority enforcement** — the table is the union-find MERGED
model (`view_to_model->FindAs<DataModel>()->table`), NEVER the `.df` per-view
attribute. **NO count oracle** (ADJ-S12 inherited — walk-authoritative enrollment
declines the reachability oracle; the ADJ-S10 bless-time count read compensates);
no `expect()` line, no key-multiset. The Fable-review R1 [1] note carries over: an
interior mis-CUT CMP/MAP marker of the right view-kind is tautologically
undetectable here — caught only by byte-identity A/B + eqgate + bless count read.

### M7 — Format: DROpKindName + kAllKinds + DEDICATED render case

**Same, two more.** `DROpKindName` (`Format.cpp:97-121`, `-Wswitch`-total,
loud-abort tail) gains `case kEagerCompare: return "kEagerCompare";` +
`kEagerGenerate` (F-SWITCHTOTAL forces it). Two DEDICATED render cases beside the
R1 eager cases (`Format.cpp:847-870`) — NEVER the generic `default:`
(`:872-883`), which would render `reads:`/`effects:`/`spine:` sublines + NO
`args:` line for an effect-free marker (the DS-ADJ-5 hazard, spelled out at
`:838-846`). Grammar in §B5. ONE new loud-abort name table `ComparisonOperatorName`
(EagerSinkName style, `Format.cpp:126-136`) reusing the `.df` house spelling
`eq/neq/lt/gt` (`lib/DataFlow/Format.cpp:222-225`). The MAP `functor=` reuses the
`message=` string idiom (no table). Both keep the effect-free "no
reads/effects/spine sublines, `table=` OMITTED when null" shape (ADJ-S3 upheld).
Bump the `kAllKinds` census comment 20 → 22 (ADJ-S11 hygiene), AND
[CRIT-FOLD c2-LOW-4] fix the ALREADY-STALE abort-message comment at
`Format.cpp:964` ("a 19th DROpKind not in kAllKinds" — stale since R1, becomes
"a 23rd" post-R2).

### M8 — helpers .find()-ONLY; identity extracted ONCE

**Same, strictly simpler.** Both arms use ONLY `ModelTableOrNull(impl, view)`
(`DeltaRel.cpp:1267-1273`, `.find()`-guarded — ADJ-S13/S14) plus pure `Query*`
accessors. NEITHER arm reads any `Context` map: unlike the INSERT arm — whose
`ClassifyEagerSink` had to `.find()`-probe `context.publish_vecs` /
`context.commit_published_view` (`Build.cpp:1099-1106`, ADJ-S13) — the CMP/MAP
payloads are ALL pure view/decl reads. So R2's two arms are strictly LOWER on the
hazard axis than the INSERT arm: no `operator[]` default-insert hazard exists to
avoid. The identity (`ModelTableOrNull`) is extracted ONCE at the mint site and
feeds ctor + record; the operator/functor re-derive at Format from `eager_view`.

### The EmittedEagerOp record extension for R2 — NONE

`Context::EmittedEagerOp` (`Build.h:245-252`) is UNCHANGED. It already carries
`kind` (the 4-way discriminant), `view` (re-invokes the ctor and re-derives the
operator/functor at Format), `table`, and the unused-for-CMP/MAP `sink`/`message`
(default `kNone`/`nullopt`). This is the mold's "view re-invokes the ctor"
invariant (`Build.h:247`), and it is the whole reason R2 is a genuinely additive
diff and not a payload change.

---

## §B — THE DECISION LIST (ADJ-R2-n candidates)

Each numbered candidate is owner-adjudicated at the ritual head; rationale +
alternatives recorded.

### ADJ-R2-0 — the two kind SPELLINGS: kEagerCompare / kEagerGenerate

**DECIDE: `kEagerCompare` (20), `kEagerGenerate` (21).** Rationale: `kEagerCompare`
matches the view kind (`IsCompare`) and the builder (`BuildEagerCompareRegions`),
exactly as `kEagerInsert` matched `IsInsert`. For MAP, `kEagerGenerate` tracks the
region-builder name `BuildEagerGenerateRegion` and the "generative functor call"
role (the R1 `kEagerForward` was role-named too, not `kEagerTuple`); the carrier
predictions (A3's census tables) are all built on `kEagerGenerate`. **Alternative:**
`kEagerMap` (A1's spelling, view-kind-symmetric with `IsMap`). REJECTED for
builder-name consistency and to keep the A3 predictions authoritative — but this is
a <90% call, flagged to critics (§E). The census/dump reader sees
`kEagerGenerate=<n>`; the DROpKindName spelling is the enum identifier verbatim
(t2b-grammar §2.0 DROpKind rule: render WITH the `k`).

### ADJ-R2-1 — CMP payload: the VIEW's operator, re-derived, spelled eq/neq/lt/gt

**DECIDE: record NO new field; render `cmp=<op>` re-derived from the stored
`eager_view` via `QueryCompare::From(*eager_view).Operator()`, showing the VIEW's
`ComparisonOperator` (model authority), spelled with the `.df` house table
`eq/neq/lt/gt`.**

Three sub-decisions:

1. **Model authority vs emission-normalized form.** The dump shows the VIEW's
   operator (`kEqual`/`kNotEqual`/`kLessThan`/`kGreaterThan`, `Parse.h:138-143`),
   NOT the emission's NE-normalized form. Code fact: `CreateCompareRegion`
   normalizes `kNotEqual` to a `kEqual` TUPLECMP with a `false_body` LET
   (`Compare.cpp:55-57`, the TUPLECMP ctor asserts `!= kNotEqual`) — this is an
   EMISSION device, not a model fact. `view.Operator()` still returns `kNotEqual`.
   The T2b stored-fields law + **DS-ADJ-7** (render authority = the model, never
   the emission/`.df` attribute) RULE: show `neq` for a `!=` filter, not `eq`.
   This matches the `.df` surface (`lib/DataFlow/Format.cpp:225` renders `neq`).
   **Alternative** (show the normalized `eq`): REJECTED — it would make the dump
   LIE (a `!=` filter rendered `eq`), and it couples the model dump to a lowering
   trick, the opposite of DS-ADJ-7.

2. **Store vs re-derive.** Re-derive at Format (no `DROp`/`EmittedEagerOp` field).
   The operator is a pure function of the stored `eager_view`, available at Format
   time (the `agg_name`-from-`agg_view` precedent, `Format.cpp:588-591`). Contrast
   `kEagerInsert`'s `eager_sink`, which was STORED only because `ClassifyEagerSink`
   needs `context` (unavailable at Format). **Alternative** (store a `uint8_t`
   operator field on DROp + EmittedEagerOp for R1-symmetry with sink): REJECTED —
   double-represents a derivable fact, grows the payload for no reason, and the
   mold's "view re-invokes the ctor" invariant already licenses re-derivation.

3. **Spelling table.** New loud-abort `ComparisonOperatorName(ComparisonOperator)`
   in the `EagerSinkName` style (`Format.cpp:126-136`), `-Wswitch`-total (the
   4-member `enum class : int` needs no default), reusing the `.df` k-stripped
   lowercase spellings: `kEqual→eq`, `kNotEqual→neq`, `kLessThan→lt`,
   `kGreaterThan→gt`. Reusing the `.df` spelling keeps the two surfaces consistent
   (t2b-grammar §2.0's cross-surface-consistency principle). Placement: `cmp=` in
   the HEADER (after `stratum=`), mirroring `kEagerInsert`'s `sink=` header token
   (`Format.cpp:858-861`).

### ADJ-R2-2 — MAP payload: functor NAME/ARITY, re-derived; NO positivity, NO purity

**DECIDE: record NO new field; render `functor=<name>/<arity>` re-derived from
`eager_view` via `QueryMap::From(*eager_view).Functor()`; render NO positivity
token; record NO purity field.**

1. **Identity = functor NAME + ARITY, not the binding pattern.** `functor=<name>/<arity>`
   from `ParsedFunctor::NameAsString()` + `Arity()` (`Parse.h:99`, and `Arity()`),
   re-derived at Format (pure fn of `eager_view`). Name/arity is the stable
   cross-mode identity — the functor DECL, invariant under CSE/canonicalization.
   The full binding pattern (`bbf…`, per-param bound/free) is a per-map LOWERING
   detail. [CRIT-FOLD c2-LOW-1: the pattern IS derivable from the stored
   `eager_view` (`functor.NthParameter(i).Binding()`), so the SP-1/R-8
   storage-absence precedent does NOT carry — the decision rests on IDENTITY
   GROUNDS alone: name/arity is the decl identity and the `message=` house idiom
   (`edge/2`); the binding pattern is codegen-flavored surface detail.] The
   pattern is not rendered. OWNER-RULED 2026-07-22: `functor=<name>/<arity>`. **Alternative** (render the `<name>_<pattern>` codegen key):
   deferred — heavier, and no golden needs it (map_3's three maps share the same
   `add_i32/3`; the `oi` header already disambiguates the three ops). Spelling
   reuses the `message=` string idiom (`Format.cpp:864-867`) — no name table.
   Placement: `functor=` on the ARGS line (mirroring `message=`), not the header.

2. **Positivity — NOT rendered.** `map.IsPositive()` (`Generate.cpp:105`) selects
   the empty-body-LET vs gen-body NESTING — an emission-STRUCTURE choice, not a
   model identity; an effect-free marker carries no emission structure. Both R1
   eager kinds render zero polarity. Omitting it keeps the block minimal and avoids
   shipping a corpus-UNWITNESSED `negated` spelling (map_3's three maps are all
   positive; no corpus carrier has a negated map). This is an ADJ-S5-analog
   residual: `IsPositive()` is re-derivable if a later slice wants it, flagged in
   §C/§E. **Alternative** (render `positive`/`negated`): REJECTED as an unwitnessed
   spelling for R2.

3. **Purity — NOT recorded.** The MAP marker is minted ONLY on the `IsPure()` true
   arm (ADJ-R2-3), so purity is a constant `true` at this op. A field would encode
   a constant. REJECTED.

### ADJ-R2-3 — impure-functor disposition: mint in the pure arm ONLY

**DECIDE: mint `kEagerGenerate` INSIDE the `map.Functor().IsPure()` true branch
(`Build.cpp:1191-1197`); leave the impure `else` (`:1198`, `assert(false)`)
UNTOUCHED — no mint there.**

Rationale (A1's finding, code-confirmed): impure functors are rejected UPSTREAM,
before the eager walk, by the `Program::Build` feature-gap pre-pass —
`Build.cpp:1346` (`"Impure functors are not yet supported"`), gated by the
`num_errors → std::nullopt` return at `Build.cpp:1432-1433`, which runs BEFORE the
eager walk. So a compiling program never reaches the MAP dispatch with an impure
functor; the impure `else` (`assert(false)`) and the belt `assert(functor.IsPure())`
(`Generate.cpp:91`) are DEAD backstops under NDEBUG. **Mint-in-pure-arm-only is
exact.** **Alternative** (mint-before-the-purity-branch): REJECTED — the impure
branch does NOT call a region builder (it asserts), so a marker there would enroll
an op with no emission and no reachable lowering, breaking the one-marker-per-real-
dispatch invariant; and it is unreachable anyway. The upstream reject GUARANTEES
the pure-arm mint is both necessary and sufficient.

### ADJ-R2-4 — ctor order + order law: no existing carrier's order changes

**DECIDE: extend the lead-0 `key_of` eager branch (M5) to admit the two kinds;
they sort by `(op_table_id, sign=0, ctor)` alongside the forwards/inserts/ingest-
folds.**

Where CMP/MAP sort relative to forwards/inserts on the same table: within a shared
`op_table_id`, all four eager kinds are sign-0 and sort by `ctor` (walk DFS order,
the tail-append enrollment index). A table-less CMP/MAP (`op_table_id` sentinel 0)
leads in the table-less block by ctor. **Does ANY golden ORDER change for the two
EXISTING carriers? NO — and it MUST NOT (E-101).** Both `demand_tc_witness` and
`symrec_tie_1` contain ZERO compare and ZERO map views (verified in their `.df`
goldens: `maps=0 compares=0` — A3), so NO CMP/MAP op is minted for them; their
forward/insert/ingest-fold ops keep IDENTICAL keys and IDENTICAL `ctor` indices
(the walk records the same dispatches in the same order; the EAGER_WEB enrollment
count/order for them is unchanged). Their `.deltarel` churn is **census-line-only**
(the two new `=0` columns append at the tail — M6/M1). The enum ordinal is not the
sort key (A2's finding), so tail-appending 20/21 is cosmetic.

### ADJ-R2-5 — GRAMMAR productions (E-71 PRE-CODE, t2b-grammar style)

Two new `op.<idx>` block productions, conforming to t2b-grammar §2.4 (header) +
§2.5 (sublines) + the T2b render-from-stored-field / loud-abort law. Both are
effect-free markers: **NO `reads:`/`effects:`/`spine:` sublines**; `table=` OMITTED
when null (ADJ-S3 upheld — the landed R1 render omits, `Format.cpp:852-853,863`,
superseding r1-design.md C.2's em-dash).

**Common header:** `op.<oi> <Kind> sign=· ctx=eager stratum=0`
- `sign=·` — `SignGlyph(op.table_op_sign)` with `table_op_sign==0` → `\xC2\xB7`.
- `ctx=eager` — `CtxName(op.ctx)`, `op.ctx==kEager`.
- `stratum=0` — `DROpStratum(flow, op)` default arm (no edit).

**kEagerCompare production:**
```
op.<oi> kEagerCompare sign=· ctx=eager stratum=0 cmp=<CmpOp>
    args:[ table=%table:N]
```
where `<CmpOp> ∈ {eq, neq, lt, gt}` = `ComparisonOperatorName(
QueryCompare::From(*op.eager_view).Operator())`, in the header after `stratum=`
(the `sink=` header precedent). `args:` renders ` table=%table:N` iff
`op.table_op_table != nullptr` (usually absent — a filter compare is table-less);
the bare `args:` line otherwise.

New loud-abort name table (E-71 PRE-CODE), total-by-construction (4 cases = the
4 `ComparisonOperator` members, Parse.h:138-143) + the loud-abort tail. [CRIT-FOLD
d2-LOW-1]: `-Wswitch` is WARNING-ONLY in both presets (CMakeLists gates `-Werror`
behind an unset WARNINGS_AS_ERRORS; `-Wall` enables `-Wswitch` non-fatally), so
what ENFORCES totality is the abort tail + construction, not the compiler. No
default case:
```
static const char *ComparisonOperatorName(ComparisonOperator op) {
  switch (op) {
    case ComparisonOperator::kEqual:       return "eq";
    case ComparisonOperator::kNotEqual:    return "neq";
    case ComparisonOperator::kLessThan:    return "lt";
    case ComparisonOperator::kGreaterThan: return "gt";
  }
  fprintf(stderr, "DELTAREL-DUMP: unhandled ComparisonOperator in a spelling table\n");
  abort();
}
```

**kEagerGenerate production:**
```
op.<oi> kEagerGenerate sign=· ctx=eager stratum=0
    args:[ table=%table:N] functor=<name>/<arity>
```
where `<name>/<arity>` = `QueryMap::From(*op.eager_view).Functor()` rendered
via the STREAMING idiom `os << functor.NameAsString() << "/" << functor.Arity()`
([CRIT-FOLD c2-LOW-3]: the `message=` mold at Format.cpp:864-867 — NOT string
concatenation, which is ill-formed as pseudocoded; and `Arity()` is
`ParsedFunctor::Arity()` (the 3-param decl arity), NEVER `QueryMap::Arity()`
(output-column count)). No name table. `functor=` renders on the args line AFTER the
optional `table=`; it is ALWAYS present (every MAP has a functor). No positivity
token (ADJ-R2-2).

Both render cases follow the `Format.cpp:847-870` mold exactly (header + optional
`table=` + one kind-specific token), diverging from the generic default only by
the extra `cmp=`/`functor=` token; NO subline emit helpers are called.

### ADJ-R2-6 — the map_3 sidecar plan (RAT-8 first-ever)

**DECIDE: new `tests/OptDiff/cases/map_3.irgold` pinning `deltarel opt` (ONE
line); seeds `tests/OptDiff/goldens/map_3.deltarel.opt.golden` (does not exist
today; only `map_3.stdout` + `map_3.main.cpp` present).**

`map_3` is the ONLY corpus member that will show non-zero `kEagerGenerate`/
`kEagerCompare` op BLOCKS (E-101: the two existing carriers get census-line-only
churn). It is an all-4-modes-clean plain golden (NOT in any runall.sh diagnostic
list; no `.batches`/`.drflags`/`.eqgate`), so its `run_irgold` compile drives
`deltarel.opt.out` byte-compared to the new golden.

**deltarel-only pin vs the three-surface symrec pin.** DECIDE **deltarel-only**
(RAT-8 minimal seeding). map_3's reason for existence is to WITNESS the new
Gen/Cmp op blocks; `deltarel opt` is sufficient. The `h opt`/`ir opt`/`df opt`
surfaces are R2-invariant (zero emission change, already gated corpus-wide by the
frozen A/B), so pinning them adds no R2 coverage. **RECOMMEND-BUT-OPTIONAL:** add
`h opt` to also lock map_3's codegen surface for free (owner call at the ritual
head); the minimal RAT-8 seeding is deltarel-only. (Contrast `symrec_tie_1.irgold`
= `ir/df/deltarel`, `demand_tc_witness.irgold` = `h/ir/df/deltarel`.)

**ADJ-S7 same-workroot referee** applies: diff the ACTUAL bless-source file
`$WORKROOT/map_3/map_3.irgold/deltarel.opt.out` (first run: golden absent →
IRGOLD-MISSING, not a separate compile). **ADJ-S10 count read** applies: read the
census against A3's derivation (opt: 3 Gen / 1 Cmp / 3 Fwd / 3 Ins = 10 ops).
Driver `map_3.main.cpp` sorts its keyed drains (`main.cpp:24-25/37-38`) — cursor
contract satisfied, no action.

### ADJ-R2-7 — census / recount: the kind↔view-kind additions + merged-model match

**DECIDE (M6):** the A.6(c) structural recount (`DeltaRel.cpp:3413-3440`) admits
the two kinds in its skip-guard and its kind→view-kind arm becomes 4-way —
`kEagerCompare→IsCompare`, `kEagerGenerate→IsMap` (`QueryView::IsCompare`/`IsMap`).
The model-table match (`:3436`) is UNCHANGED and enforces the **DS-ADJ-7 merged-
model render authority** corpus-wide, always-on (`op.table_op_table ==
ModelTableOrNull(impl, v)`; for a table-less CMP/MAP both are null). NO count
oracle, NO key-multiset (ADJ-S12 inherited). `kAllKinds` grows to 22 (or
F-CENSUSABORT fires). This is the ONLY model-internal check; a mis-cut interior
marker of the right view-kind is caught only by the emission-side controls
(byte-identity A/B, eqgate, goldens, bless count read) — the same coverage shape
as R1, no new hole.

### ADJ-R2-8 — the remaining a2-flagged DECISION-BEARING sites

(a) **EAGER_WEB 4-way re-invocation** (`DeltaRel.cpp:2389-2396`) — covered in M4;
the binary if/else must become 4-way or CMP/MAP mis-route into MakeEagerInsertOp.
(b) **key_of lead-0 OR** — covered in ADJ-R2-4/M5. (c) **The seven other
`switch(op.kind)` sites** fall through default arms UNCHANGED (a2 §E1-E7,
re-confirmed at tip). No other decision-bearing site exists: `RecordEagerDispatch`,
`Context::EmittedEagerOp`, `Compare.cpp`/`Generate.cpp`/`InTryInsert`/
`CreateCompareRegion`/`CreateGeneratorCall` stay byte-identical.

---

## §C — PRE-REGISTERED PREDICTIONS

Baseline = **frozen a4b807dc** (`scratchpad/frozen-a4b807dc/{drlojekyll-debug,
drlojekyll-release}`). All predictions pre-registered before code lands.

### C.1 [BYTE] — full-corpus byte-identity, all 4 modes (the zero-emission-change gate)

**Prediction: `.df`, `.ir`, `.h`, `.cpp`, `stdout` byte-identical corpus-wide, all
4 optimization modes, vs frozen a4b807dc.** Mechanism (mold M3): the mint +
record + inventory path allocates no `impl->next_id` and Emplaces no region; the
wrappers CALL the untouched `BuildEagerCompareRegions`/`BuildEagerGenerateRegion`
with identical args at the identical walk moment, so the ControlFlow IR is
byte-identical, `.h`/`.cpp` are pure functions of that IR, `.df` is upstream of
the whole CF build. The 5 non-`.deltarel` IR goldens stay byte-unchanged:
`symrec_tie_1.{ir,df}.opt.golden`, `demand_tc_witness.{ir,h,df}.opt.golden`. The
new carrier's `map_3.stdout` stays byte-unchanged (its driver output is
emission-derived). The 676-row knob-off A/B, the post-baseline-4 (incl. nested
eqgate), and the `data/` A/B all stay 0-diverged.

### C.2 [STRUCT] — .deltarel, the two EXISTING carriers: census-line-ONLY diffs

Both carriers contain zero CMP/MAP views (`maps=0 compares=0`, verified in their
`.df` goldens) ⇒ NO new op BLOCKS ⇒ **census-line-only churn** (E-101). Every op
block byte-identical; `rounds:`/`deps:` empty. The SOLE diff is the tail append
` kEagerCompare=0 kEagerGenerate=0`.

**Exact expected new census line — `demand_tc_witness` (verified R1 line = 12F/2I):**
```
census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=2 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0 kEagerForward=12 kEagerInsert=2 kEagerCompare=0 kEagerGenerate=0
```
**Exact expected new census line — `symrec_tie_1` (verified R1 line = 7F/1I):**
```
census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=1 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0 kEagerForward=7 kEagerInsert=1 kEagerCompare=0 kEagerGenerate=0
```
These are the ONLY changed bytes in either golden.

### C.3 [STRUCT] — map_3's full predicted .deltarel (opt), per A3's derivation

`map_3` is TABLE-LESS ingest (`kIngestFold=0` all modes — its `m` receive is a
table-less monotone receive taking the E-42 VECTORLOOP-shim else-branch,
`Procedure.cpp:94-106`), so the tail-appended eager ops start at ctor 0 (no ingest
prefix). Opt-mode inventory (A3): 1 surviving compare (`compare.9`, the `Z=5`
filter) + 3 maps (`map.6/7/8`, all `add_i32`) + 3 forward/insert table pairs.

**Predicted census counts (opt/nocf; nodf/none in parens):**
`kEagerGenerate=3` (3 all modes), `kEagerCompare=1` (opt/nocf; **3** nodf/none),
`kEagerForward=3` (opt/nocf; 9 nodf/none), `kEagerInsert=3` (all), `kIngestFold=0`,
all other kinds 0. Opt total = **10 ops** (nodf/none = 18). DS-ADJ-1 holds: census
counts are mode-stable ONLY across the controlflow axis (opt≡nocf, nodf≡none); the
df axis legitimately grows compares 1→3 and forwards 3→9 (maps stay 3).

**Predicted opt render order (going-in; SUPERSEDED-AT-BLESS on table= — see the
DS-ADJ-7 CAVEAT below):**
```
op.0 kEagerGenerate sign=· ctx=eager stratum=0
    args: functor=add_i32/3                        # map.6, table-less, ctor 0
op.3 kEagerGenerate sign=· ctx=eager stratum=0
    args: functor=add_i32/3                        # map.7, ctor 3
op.6 kEagerCompare  sign=· ctx=eager stratum=0 cmp=eq
    args:                                          # compare.9 (Z=5), ctor 6
op.7 kEagerGenerate sign=· ctx=eager stratum=0
    args: functor=add_i32/3                        # map.8, ctor 7
op.1 kEagerForward  sign=· ctx=eager stratum=0
    args: table=%table:6                           # tuple.3, ctor 1
op.2 kEagerInsert   sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:6                           # insert.10, ctor 2
op.4 kEagerForward  sign=· ctx=eager stratum=0
    args: table=%table:10                          # tuple.4, ctor 4
op.5 kEagerInsert   sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:10                          # insert.11, ctor 5
op.8 kEagerForward  sign=· ctx=eager stratum=0
    args: table=%table:15                          # tuple.5, ctor 8
op.9 kEagerInsert   sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:15                          # insert.12, ctor 9
```
(`rounds:`/`deps:` empty; census line = the 22-column line with the counts above.)

**[BYTE] within C.3:** the per-op header/token TEXT is fully determined —
`sign=·` (`\xC2\xB7`), `ctx=eager`, `stratum=0`, `cmp=eq` (compare.9 is a `Z=5`
equality → VIEW operator `kEqual`), `functor=add_i32/3`, `sink=relation` (map_3
publishes zero messages → every INSERT is `insert.IsRelation()`), the
`kEagerForward` table= tokens for the 3 relation tables. map_3 WITNESSES `cmp=eq`
and `functor=add_i32/3` and the `kEagerGenerate`/`kEagerCompare` blocks — the R2
observability win.

**[STRUCT] within C.3 — CAVEAT (the DS-ADJ-7 open question, flagged LOUD):** the
`table=` attributions (hence which ops LEAD) are pinned at bless, NOT byte-certain.
A3 derived "4 table-less Gen/Cmp lead" from `ir.out` showing no `%table` for the
maps/compare — but **DS-ADJ-7 is explicit that the `.df`/`.ir` per-view attribute
is NOT the render authority; the union-find MERGED model is** (R1's tc forwards
rendered at `%table:4`/`%table:23` despite the `.df` showing them table-less). If
`map.6/7/8`'s or `compare.9`'s DataModel unifies with a downstream table
(`ModelTableOrNull` non-null), those markers render a `table=` token and sort into
that table's block, NOT the leading table-less block — shifting the order above.
The census COUNTS (3 Gen / 1 Cmp / 3 Fwd / 3 Ins) are robust (they don't read
`table=`); the order + `table=` tokens are the [STRUCT] residue the A.6(c)
merged-model match validates and the bless pins. Treat A3's ordering as the
going-in prediction, superseded by the as-blessed golden per DS-ADJ-7.

### C.4 [STRUCT] — corpus-wide census-line churn on EVERY program

Every program's `.deltarel` census line grows by two `=0` columns
(` kEagerCompare=0 kEagerGenerate=0`) — but only the three carriers have a
committed `.deltarel` golden, so only they diff. All other corpus programs render
the wider census only when `-deltarel-out` is passed (never in the suite compile
lines; F-DUMPGUARD). No non-carrier golden churns.

### C.5 The A/B + suite expectation (the EXACT pre-registered divergence set)

- **676-row knob-off A/B: 0-diverged** vs frozen a4b807dc (×2 runs). Ditto
  post-baseline-4 (incl. nested eqgate ×4 modes) and `data/` A/B.
- **Suite PRE-bless reds (the EXACT set, mirroring R1; [CRIT-FOLD c2-LOW-5/
  c3-LOW-1] literal run_irgold format — `irgold` keyword + DOTTED surface.mode,
  runall.sh:293/296 — so a referee can grep the exact lines):**
  1. `demand_tc_witness irgold deltarel.opt IRGOLD-DIVERGE` (census line grows
     by 2 columns).
  2. `symrec_tie_1 irgold deltarel.opt IRGOLD-DIVERGE` (census line grows by 2
     columns — NOTE: unlike R1 where symrec was IRGOLD-MISSING, its `.deltarel`
     golden now EXISTS, so R2 is a DIVERGE not a MISSING).
  3. `map_3 irgold deltarel.opt IRGOLD-MISSING` (new golden absent first run).
  Everything else GREEN across all three PRE-bless runs. POST-bless: `SUITE: PASS
  (173)` ×3, all three deltarel goldens green.

---

## §D — THE GATE PLAN

### D.1 Standing per-slice gates (verbatim-applicable, the R1/R-a2 set)

- **Suite ×3** — `DR=build/debug/bin/drlojekyll runall.sh $WORKROOT`, must end
  `SUITE: PASS (173)`, run three times (a new ordering-sensitive Kahn input on
  every compile — the two new `flow.ops` members). Pre-registered PRE-bless reds =
  the C.5 set (tc DIVERGE, symrec DIVERGE, map_3 MISSING).
- **676-row knob-off A/B, ×2** — 0-diverged vs frozen a4b807dc (the C.1 headline).
- **post-baseline-4 A/B** — incl. the `demand_neighborhood_witness` nested eqgate
  ×4 modes (R2 does not touch demand/instance lowering; both hold).
- **`data/` A/B** — all 4 modes clean.
- **ctest 5/5** — `DeltaRelValidators` UNAFFECTED (R2's A.6(c) extension is inline,
  corpus-covered, not a new fixture; `CheckInstanceOrder` untouched).
- **ASAN both surfaces ×2** — `build/asan` ctest + OptDiff under `DR=asan` + the
  env-CXX-wrapper second surface. R2 adds no new field/lifetime (the ctors take
  the same `QueryView`/`TABLE*` handles, same lifetime as R1's). Zero reports,
  SUITE PASS both surfaces.
- **config-invariance 3-run + debug==release single hash** — on the `.deltarel`
  dump for ALL THREE carriers (tc, symrec, map_3): render `-deltarel-out` under
  both presets and `cmp -s`. The (F)-law audit ARGUES no pointer-ordered iteration
  (the EAGER_WEB loop is a `std::vector` by index; `key_of` orders by `table->id`/
  ctor; the operator/functor re-derivation reads the stored `eager_view`); the
  standing check VERIFIES it. Expected byte-identical. Note map_3 is the first
  carrier to PIN the `cmp=`/`functor=` bytes AND (with tc/symrec) the `sign=·`
  `\xC2\xB7` byte — DS-ADJ-4 hexdump at bless.
- **E-62 re-grep clean** — no NEW `pinned_order`/`body_ops`/`output_ops` reader.
  `key_of` is EDITED (a wider branch), the EAGER_WEB re-invocation is EDITED (a
  wider switch), A.6(c) is EDITED (a wider guard) — none is a NEW reader. The sole
  sanctioned external hit stays the `Stratum.cpp` comment.
- **Q5 ABABAB progsize@128, measured** — R2 adds at most two more
  `RecordEagerDispatch` `push_back`s per CMP/MAP dispatch + two more O(1) ctor
  calls at inventory; `progsize@128` (a monotone chain) has few/no CMP/MAP views,
  so the marginal cost over R1 is ~zero. MEASURE anyway (E-77: run the referee),
  expect noise (±1%).
- **bench-counter no-op — N/A** (Runtime is untouched; no `Runtime/` file changes).

### D.2 The bless ritual order

1. **Land the code diff in one commit-shaped change:** the 2 enum values
   (`DeltaRel.h` tail), 2 ctors + 2 decls (`DeltaRel.cpp`/`DeltaRel.h`), the 2
   Build.cpp dispatch arms + 2 `LowerRelStep_*` wrappers, the EAGER_WEB 4-way
   re-invocation, the key_of lead-0 OR, the A.6(c) 4-way recount, `DROpKindName` +
   `kAllKinds` (→22, comment bump) + the 2 render cases + `ComparisonOperatorName`
   in `Format.cpp`. `Compare.cpp`/`Generate.cpp`/`InTryInsert`/region builders
   byte-unchanged.
2. **Add `tests/OptDiff/cases/map_3.irgold`** = one line `deltarel opt` (ADJ-R2-6;
   owner may add `h opt` to also lock codegen).
3. **One full-suite run, ONE workroot** (ADJ-S7 provenance + same-workroot).
   Pre-registered reds = the C.5 set; run ×3.
4. **ADJ-S7 direct-diff referee on the ACTUAL bless-source files:**
   `diff goldens/demand_tc_witness.deltarel.opt.golden
   $WORKROOT/demand_tc_witness/demand_tc_witness.irgold/deltarel.opt.out` (accept:
   ONLY the census tail ` kEagerCompare=0 kEagerGenerate=0` added, no other line
   moved); ditto `symrec_tie_1`; `cat` the new
   `$WORKROOT/map_3/map_3.irgold/deltarel.opt.out` and check against C.3.
5. **ADJ-S10 structural count read** (the SOLE correctness check on the new
   columns): tc `kEagerCompare=0 kEagerGenerate=0`; symrec `=0 =0`; map_3
   `kEagerGenerate=3 kEagerCompare=1 kEagerForward=3 kEagerInsert=3` (total 10). +
   DS-ADJ-4 hexdump the first eager op's `sign=·` (`\xC2\xB7`), map_3's `cmp=eq`
   and `functor=add_i32/3` bytes. + DS-ADJ-1 knob check: map_3 `opt==nocf` and
   `nodf==none` on the eager census (a df-axis delta, compares 1→3, is EXPECTED).
   [CRIT-FOLD c3-LOW-3]: the sidecar drives ONLY the opt compile (runall.sh:271
   mode gate), so the nocf/nodf/none census reads are THREE MANUAL referee
   compiles (`-deltarel-out` by hand), NOT suite-produced — the critic pre-ran
   them at tip: opt/nocf = 10 ops, nodf/none = 18, check passes going-in.
   [CRIT-FOLD c1-LOW-1]: read every count as "markers MINTED" (per-VISIT — no
   visited-set in the walk, no dedup in RecordEagerDispatch), not "distinct
   views"; they coincide for map_3 (every CMP/MAP is single-predecessor) but a
   future diamond-fan-in carrier would legitimately count per-visit higher.
   Inherited R1 semantics, not an R2 regression.
6. **Bless from the SAME workroot:** `runall.sh --bless $WORKROOT demand_tc_witness`;
   `... symrec_tie_1`; `... map_3`.
7. **POST-condition tripwire:** `git status` shows EXACTLY the three
   `.deltarel.opt.golden` files + the new `map_3.irgold` sidecar changed. If any
   `.h`/`.ir`/`.df` golden moves, R2's zero-emission-change claim is ALREADY
   violated — STOP. Re-run suite ×3, expect `SUITE: PASS (173)` all green.

### D.3 Tie to §19(H) — R2 does NOT delete seam pieces; it GROWS the modeled set

R2 retires NO seam artifact (S1 ingest-fold hole contract, S2 cut-successor
predicates + §7d cross-check, S3 V-INGEST-XCHECK, S4 E-42 shim ALL STAND). Its
contribution toward the R-final §19(H) acceptance is **coverage of the eager-web
dispatch table**: after R2, the TUPLE-forward, terminal-INSERT, CMP-filter, and
MAP-call arms are all MODELED marker ops; only JOIN (index-probe / product),
MERGE-union, SELECT-rebind, NEGATE-gate, and the E-42 table-less shim remain
hand-coded-unmodeled (rel-arch-pseudocode §5, R3..R-JOIN / R-E42). Each modeled
arm is one fewer arm the R-final DIRECTION FLIP must convert when it makes the
inventory the reachability authority and dissolves S1/S2/S3/S4 into interior
invariants. R2 also makes the dump a MORE proportional image of the eager web
(map_3 now shows its Gen/Cmp blocks — the observability win extends to the filter
and functor-call arms), which is the §19(H) motivating measurement.

### D.4 Carried pins honored

- **E-62** — re-grepped at this DeltaRel diff (D.1); no new `pinned_order` reader.
- **The (F) law** — every new/edited loop iterates a `std::vector` by index or
  keys by deterministic `table->id`/ctor; the operator/functor re-derivation reads
  the stored `eager_view`, never a pointer-ordered container (D.1 config-invariance
  + debug==release verify it empirically).
- **T2b dump law** — `cmp=`/`functor=`/`table=` all render from the stored
  `eager_view` or a pure function of it; `ComparisonOperatorName` is a `-Wswitch`-
  total loud-abort table (§B5); no prose sublines.
- **E-71** — the two render productions + the `ComparisonOperatorName` table are
  adjudicated PRE-CODE (§B5).
- **id-stream identity** — the mint-nothing wrappers call untouched builders (M3).
- **ADJ-S2 enrollment** — the 4-way EAGER_WEB stays strictly after the ingest
  folds (M4); ops 0/1 keep their `kIngestFold` header bytes.
- **DS-ADJ-7 render authority** — `table=` is the union-find MERGED model, enforced
  always-on by the A.6(c) match (M6); the map_3 `table=`/order is the [STRUCT]
  residue pinned at bless (C.3 caveat).
- **census day one** — `kAllKinds` grows to 22 in the same diff or F-CENSUSABORT
  fires (M6/M1).

---

## §E — OPEN QUESTIONS (as adjudicated; the residual ones carry to bless)

1. **Kind spelling `kEagerGenerate` vs `kEagerMap`** (ADJ-R2-0). **RESOLVED —
   OWNER-RULED 2026-07-22: `kEagerGenerate`** (builder-name consistency,
   A3-prediction alignment).

2. **map_3's `table=` / render order (the DS-ADJ-7 hazard, C.3 caveat).** A3
   derived "4 table-less Gen/Cmp lead" from `ir.out`, but DS-ADJ-7 warns the
   `.df`/`.ir` attribute is NOT the render authority — the union-find merged model
   is. If `map.6/7/8`/`compare.9` unify their model with a downstream table, their
   markers render a `table=` token and re-sort into that table's block, changing
   the C.3 order. I am <90% that all four are table-less as-blessed. The census
   COUNTS are robust; the ORDER + `table=` tokens are pinned at bless. A critic
   should NOT treat C.3's ordering as byte-certain.

3. **map_3's `compare.9` operator = `kEqual` → `cmp=eq`.** `Z = 5` is an equality,
   so the VIEW operator should be `kEqual`. But if the canonicalizer represents the
   constant-pin differently (e.g. a `kNotEqual` on a negated form), the token would
   be `neq`. High confidence it is `eq`, but first-run-confirmed at bless. The
   `neq`/`lt`/`gt` spellings remain corpus-UNWITNESSED at R2 (an ADJ-S5-analog
   residual — only `eq` is pinned by map_3).

4. **Positivity omission for `kEagerGenerate`** (ADJ-R2-2). I render no
   positivity token (all corpus maps are positive; a `negated` spelling would ship
   unwitnessed). A critic may argue the marker should carry positivity for
   proportional-image completeness even unwitnessed. My call: omit for R2 (minimal,
   avoids an ADJ-S5 residual); `IsPositive()` is re-derivable if a later slice or a
   negated-map corpus case needs it.

5. **`cmp=` header placement vs args.** I placed `cmp=` in the HEADER (after
   `stratum=`, the `sink=` precedent) and `functor=` on the ARGS line (the
   `message=` precedent). This is a readability call, not law-forced; a critic
   might prefer both on args for uniformity. Low-stakes, flagged.

6. **The `map_3.irgold` surface set** (ADJ-R2-6). **RESOLVED — OWNER-RULED
   2026-07-22: `deltarel opt` ONLY** (minimal RAT-8 seeding; other surfaces stay
   unpinned through the Rel substrate churn, the OD-10 defer spirit).
