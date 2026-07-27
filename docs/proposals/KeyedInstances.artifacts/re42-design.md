======================================================================
COMMITTED AT THE R-E42 LANDING (2026-07-27, the DeltaRel->Rel epoch;
KeyedInstances.md §20(U) is the landing record, §20(T) the open record).
This is the BINDING R-E42 DESIGN CONTRACT: the stage-(b) designer draft
D1..D10 as adjudicated by the stage-(c) xhigh adjudicator (three fresh
adversarial critics, every disputed claim ruled AT THE CODE; adjudication
summary A1-A6 inside) with the TEN ritual-head OWNER RULINGS RH-1..RH-10
ratified 2026-07-27 (the OWNER RULINGS section at the tail). Anchors are
pinned at tip 8f8dd07d / c8888e44 (the design tips) and are NOT re-based
after later insertions — the house errata mechanism tracks drift. The
stage-(a) record's §5 id-stream contract (1+arity ids at the original
walk position; the LowerIngestFold HOLE-CONTRACT shape, never the
zero-next_id marker trick) is BINDING throughout. Stage-(d)/(e) records
live in re42-desired-states.md.
======================================================================

# R-E42 — THE ADJUDICATED STAGE-(b)/(c) DESIGN (table-less monotone receive shim → modeled op)

xhigh ADJUDICATOR output. Tip 8f8dd07d (docs-only atop 18026049; binaries ==
18026049). Every disputed claim ruled AT THE CODE this session. The designer
draft (`re42-design-draft.md`, D1..D10) is the base; the three critic detail
files are folded in below. The stage-a record §5 id-stream contract is BINDING.

VERDICT: the design is SOUND. All load-bearing mechanism (D1 kIngestLoop
sibling kind; D2 sibling LowerIngestLoop; D3 effect-free payload; D4 tail-append
enrollment; D5 per-receive count law; D6 sibling Site-5; D8 lead-0 key_of)
SURVIVES adversarial code verification across all three critics. FOUR findings
are CONFIRMED as design changes (one MED, three LOW); one NIT is a rider ruling.
None touches the byte-identity crux or the id-stream contract.

================================================================================
## ADJUDICATION SUMMARY (each critic finding ruled at code)

| # | source | sev | ruling | disposition |
|---|---|---|---|---|
| A1 | Crit-1 F1 / Crit-2 F1 | MED | **CONFIRMED** | STRIKE D3's ":3177 REQUIRED edit" — no guard exists |
| A2 | Crit-2 F2 | NIT | **CONFIRMED** | D7 bumps Format.cpp:1045 comment 26→27 |
| A3 | Crit-3 F2 | LOW | **CONFIRMED** | flip D7 render to MARKER-SHAPE (no empty sublines) |
| A4 | Crit-3 F1 | LOW | **CONFIRMED (framing)** | present "no E-71" as a ruling; note referent shift |
| A5 | Crit-3 F4 | NIT | **CONFIRMED (clarify)** | D8 V-READY check CLOSED — effect-free ⇒ transparent |
| A6 | Crit-3 F3 | NIT | **DEFERRED to owner** | D6 hygiene riders LABELED (naming §20(T)-sanctioned) |

CODE ANCHORS RE-VERIFIED BY THE ADJUDICATOR THIS SESSION:
- `DeltaRel.cpp:3177` = `case DROpKind::kIngestFold:` inside `switch (op.kind)`
  (head :3036), terminated `break;` :3255, switch `default: break;` :3257 →
  kIngestLoop AUTO-SKIPS. **No guard to add.** (A1)
- `Format.cpp:1045` = `// ---- census (26 DROpKind counts, enum order, ...)`;
  `kAllKinds[]` :1054-1068 holds 26 entries; totality guard `census_total !=
  flow.ops.size()` :1077. (A2)
- `Format.cpp:870` in-code: kEagerForward "NO reads/effects/spine sublines
  (effect-free markers, §A.2)"; :934 "DEDICATED cases per M7 — the generic
  default would silently render empty effects:/spine: sublines." All 8 eager
  markers render header+`args:`-only; **kIngestFold DOES call emit_reads/
  emit_effects/emit_spine** (fold-shape). (A3)
- `§5 R-E42 block` + `§20(T)` seed digest: BOTH starting-state caveats
  (1+arity id-mint; ingest-layer not walk-dispatch) and the M9 quad hold; the
  slice is pre-registered exactly as the draft frames it.

================================================================================
## THE SLICE (unchanged from the draft)

Model Arm C of `ExtendEagerProcedure` — the table-less monotone receive's
hand-minted VECTORLOOP shim (Procedure.cpp:94-106) — as a DR-IR op. S4 (the
LAST emission surface with ZERO model representation) retires. Gate = per-slice
BYTE-IDENTITY A/B: Arm C's *emission* is unchanged; only its *provenance* (a
real op replaces "no op"). The id-stream contract (stage-a §5, BINDING) is
reproduced **1 + arity** at the original walk position via the LowerIngestFold
HOLE-CONTRACT shape (mint the loop, return it as the descent cursor), NOT the
marker zero-next_id trick (M16 pin-the-referent).

BINDING PRIOR RULINGS NOT RE-LITIGATED: OD-1..13, RAT-1..10, R4 option (A), the
four R-JOIN owner rulings, M1-M16. The M9 answer (every kIngestFold=0 quad
member HAS ≥1 table-less receive — triple-confirmed) is SETTLED; no M12 re-probe.

--------------------------------------------------------------------------------
## CROSS-CUTTING FACTS (verified; drive D4/D5/D6)

**FACT 1 — the ingest arm is NOT reachability-gated.** ExtendEagerProcedure is
called once per IO from BuildEntryProcedure (Procedure.cpp:820) inside
`for (auto io : query.IOs())`; the inner `for receive in receives` visits every
receive. Arm C fires UNCONDITIONALLY for every `!CanReceiveDeletions() &&
model->table==null`. So walk-minted set == pure re-derivation over
query.IOs()×Receives(), 1:1. ⇒ count law is PER-RECEIVE (D5); enrollment
RE-DERIVES from query.IOs() with no walk stream (D4); M9 needs no further probe.

**FACT 2 — dump renders in pinned_order, labels in construction order.**
Format.cpp:691-695. Block SEQUENCE = key_of sort (pinned_order); `op.N` LABEL =
flow.ops construction index (oi). Tail-appended ops carry HIGH oi labels yet
render early (lead-0 band). D4 chooses where a new op lands in BOTH orderings.

================================================================================
## D1 — OP FAMILY + KIND NAME  (ratified as designed)

A new INGEST-FAMILY sibling kind **`kIngestLoop`** at the enum TAIL (value 26
after kEagerProduct=25 — the 27th kind), reusing the `ingest_*` payload.

GROUNDS: (LAYER) E-42 is an ingest-layer surface, not a BuildEagerRegion
walk-dispatch arm — the enrollment comment DeltaRel.cpp:2366 already frames it
as ingest-adjacent. (ID) every IsEagerMarkerKind op mints ZERO next_id; Arm C
MUST allocate 1+arity — so marker-family (option b) is REJECTED-BY-CONSTRUCTION.
(COUNT-CLEANLINESS) widening kIngestFold (option c) overloads one kind with two
emission shapes and SHIFTS the four committed kIngestFold=0 pins — rejected. A
distinct kind keeps kIngestFold's count law intact (quad stays kIngestFold=0;
new blocks are kIngestLoop=N). ENUM PLACEMENT: TAIL (M1) renumbers no kind.

================================================================================
## D2 — LOWERING SHAPE  (ratified as designed)

A SIBLING `LowerIngestLoop`, NOT relaxing LowerIngestFold's :1930 assert.
Signature mirrors LowerIngestFold. Body (byte-move of Procedure.cpp:95-105):

```
OP *LowerIngestLoop(ProgramImpl *impl, Context &context, const DROp &op,
                    PARALLEL *parent, VECTOR *loop_vec) {
  assert(op.kind == DROpKind::kIngestLoop);
  assert(op.ingest_table == nullptr);   // table-less by construction
  assert(op.effects.empty());           // vec-only, no kCounter (D3)
  const auto loop = impl->operation_regions.CreateDerived<VECTORLOOP>(
      impl->next_id++, parent, ProgramOperation::kLoopOverInputVector);  // id #1
  parent->AddRegion(loop);
  loop->vector.Emplace(loop, loop_vec);
  for (auto col : op.ingest_receive->Columns()) {
    VAR *const var = loop->defined_vars.Create(
        impl->next_id++, VariableRole::kVectorVariable);   // id #2..1+arity
    var->query_column = col;
    loop->col_id_to_var.emplace(col.Id(), var);
  }
  context.emitted_ingest_loops.push_back(                  // Site-5 sibling (D6)
      {op.ingest_sign, op.ingest_is_explicit,
       static_cast<uint8_t>(op.ingest_role), op.ingest_message->Id()});
  return loop;   // the descent cursor (next_parent)
}
```

Arm C at Procedure.cpp:94-106 collapses to:
```
} else {
  const DROp op = MakeTableLessIngestLoop(message, receive);   // D3 ctor
  next_parent = LowerIngestLoop(impl, context, op, parent, vec);
  // ARM-C CURSOR-SHAPE GUARD (ship — symmetry with Arm B :87-93):
  VECTORLOOP *const chk = next_parent->AsVectorLoop();
  if (!chk || chk->vector.get() != vec) {
    fprintf(stderr, "error: §6 INGEST-LOOP-SHAPE: table-less arm did not "
                    "return a VECTORLOOP over the message add-vector\n");
    abort();   // always-on, survives NDEBUG
  }
}
```
`BuildEagerInsertionRegions(impl, receive, context, next_parent,
receive.Successors(), table)` (:108, table==null) is called IDENTICALLY — the
descent Emplaces into `loop->body`, exactly as today.

WHY SIBLING (not relaxing LowerIngestFold): every LowerIngestFold assert is a
fold-invariant (kind :1911, R1e :1915, sign :1921, table!=null :1930,
kCounter-first :1937); relaxing :1930 forces null-guards on the UPDATECOUNT
(:1945-1948), col_values.AddUse, and the explicit-queue block (:1962-1975),
weakening the fold contract. The sibling keeps LowerIngestFold PRISTINE (the D1
count-reading payoff). Id-stream is provably 1+arity (byte-move of Arm C).

================================================================================
## D3 — PAYLOAD (M2') + EFFECTS + CTOR  (ratified, with the A1 correction folded in)

Reuse the existing `ingest_*` fields (no struct edit): kind=kIngestLoop,
ctx=kEager, ingest_message=message (render + key), ingest_receive=receive
(.Columns() for the VARs — the load-bearing store, not re-derivable at Format),
ingest_table=nullptr (THE discriminant → op_table_id 0 → lead-0), ingest_sign=+1,
ingest_is_explicit=false, ingest_stage1=false, ingest_role=VecRole::kEmpty.

EFFECTS: **effect-FREE** (empty op.effects). GROUNDS (ADJ-RJ-6): the shim only
READS the message param vec; no pre-existing DR effect to re-source (unlike R4's
kNegateGate, which relocated a real kFlagRead — M14). kVecAppend would be WRONG
(models a producer; the loop is a consumer). kFlagRead would be WRONG (the vec
read is emission-internal, like kEagerForward's TABLESCAN). ⇒ ZERO dep edges.

CTOR (single authority, ID-NEUTRAL — MakeStageOneIngestFolds precedent):
```
DROp MakeTableLessIngestLoop(ParsedMessage message, QueryView receive) {
  assert(!receive.CanReceiveDeletions());
  DROp op;
  op.kind = DROpKind::kIngestLoop;  op.ctx = Ctx::kEager;
  op.ingest_message = message;      op.ingest_receive = receive;
  op.ingest_table = nullptr;        op.ingest_sign = 1;
  op.ingest_is_explicit = false;    op.ingest_stage1 = false;
  op.ingest_role = VecRole::kEmpty;
  return op;   // no effects, no next_id
}
```
Invoked from BOTH the walk (Arm C) AND enrollment (D4) — payloads cannot
diverge (§12.6). Decl near the ingest ctors (DeltaRel.h ~:996-1008).

### >>> ADJUDICATED CORRECTION A1 (Crit-1 F1 / Crit-2 F1, MED — CONFIRMED at code)
The draft's D3 listed **DeltaRel.cpp:3177** among "THE kCounter-FIRST ASSERTS
MUST BE GATED (all three)" with the prescription "add `op.kind == kIngestFold`
to its guard … REQUIRED edit under either D6 branch." **THIS IS STRUCK.** At
code (verified this session): :3177 is a switch **`case DROpKind::kIngestFold:`**
inside `switch (op.kind)` (head :3036), the V-INGEST effect-shape TOTALITY block,
terminated `break;` at :3255; the switch has `default: break;` at :3257. It is
NOT a kCounter-first assert and there is **no "guard"** to add a kind predicate
to — the `case` label already IS the kind discriminant. A kIngestLoop op hits
`default: break;` and is skipped automatically, EXACTLY like Stratum.cpp:1937 and
:2135. The edit is a no-op that cannot be applied as written.

CORRECTED D3 kCounter-first paragraph (replaces the three-bullet list):
- **Stratum.cpp:1937** (inside LowerIngestFold) — kIngestLoop NEVER enters
  LowerIngestFold (D2 sibling) → auto-safe. NO edit.
- **Stratum.cpp:2135** (Site-5 fold loop) — excluded by the loop's
  `if (op.kind != kIngestFold) continue` (D6 keeps that guard) → auto-safe.
  NO edit.
- **DeltaRel.cpp:3177** — a `case kIngestFold` block; kIngestLoop hits the
  switch `default: break;` → auto-excluded. NO edit. *(was falsely "REQUIRED")*.
NET: **ZERO kCounter-first edits are required.** All three are kind-cased and
exclude kIngestLoop by construction. (The stage-a record §1.3/§6.7 shares the
same mischaracterization — it lumps :3177 with the kCounter-first asserts; that
doc text is likewise corrected: :3177 is a `case`, not an assert.)

================================================================================
## D4 — ENROLLMENT POSITION  (ratified as designed)

**TAIL-APPEND** — a dedicated query.IOs()×Receives() re-derivation loop AFTER
the EAGER_WEB block (DeltaRel.cpp ~:2510, the M4 enrollment region end per
§20(T) E-124). GROUNDS (Fact 2): kIngestLoop gets the HIGHEST oi → every
existing `op.N` label on all 11 pins stays BYTE-STABLE; the diff is additive
(new tail blocks on the quad + census line); pinned_order re-sorts kIngestLoop
into the lead-0 band regardless (D8). Verified: nothing pushes to flow.ops after
EAGER_WEB (the blocks at :2524+ are validators / DeriveDRStrata) — the appended
loop genuinely gets the highest oi.

```
// ---- table-less ingest loops (R-E42), TAIL-APPENDED after EAGER_WEB
for (QueryIO io : query.IOs()) {
  const auto receives = io.Receives();
  if (receives.empty()) continue;
  const ParsedMessage message = ParsedMessage::From(io.Declaration());
  for (QueryView receive : receives) {
    if (receive.CanReceiveDeletions()) continue;
    if (impl->view_to_model[receive]->FindAs<DataModel>()->table) continue;
    flow.ops.push_back(MakeTableLessIngestLoop(message, receive));
    // NO def-edge (effect-free, vec-only)
  }
}
```
Rejected: enroll in the per-IO ingest loop at :2400 — WHOLESALE renumbers every
op.N on the four quad carriers (kIngestFold=0 there, so kIngestLoop would take
op.0.. and shift all downstream ops).

================================================================================
## D5 — COUNT LAW (M15, PER-RECEIVE)  (ratified as designed)

```
count(kIngestLoop) == Σ over io in query.IOs(), receive in io.Receives()
    of [ !receive.CanReceiveDeletions() && model(receive)->table == nullptr ]
```
== #Arm-C fires == #VECTORLOOP shims minted at Procedure.cpp:95-96 today.

CENSUS RECOUNT — a THIRD arm on the IOs×Receives loop (DeltaRel.cpp:3423ff),
under a SEPARATE counter (NEVER absorbed into exp_ingest):
```
if (receive.CanReceiveDeletions())     { exp_ingest += 2u; ... }   // unchanged
else if (table != nullptr)             { exp_ingest += 1u; ... }   // unchanged
else /* table-less monotone */ {
  exp_ingest_loop += 1u;
  exp_ingest_loop_keys.emplace_back(1, false,
      static_cast<uint8_t>(VecRole::kEmpty), mid);
}
```
+ `expect(DROpKind::kIngestLoop, exp_ingest_loop, "ingest loops");` after the
kIngestFold expect. Verified safe: the recount chain is a clean if/else-if/else;
the `else` fires iff `!CanReceiveDeletions && table==null` — byte-identical to
Arm C; `table` is REUSED from the existing lookup, no new null-model risk.

PER-OP KEY MULTISET — a sibling block; the key OMITS table (null):
```
using IngestLoopKey = std::tuple<int, bool, uint8_t, uint64_t>;  // sign, is_explicit, role, message
// exp_ingest_loop_keys vs got_ingest_loop_keys (flow.ops where kind==kIngestLoop);
// both sorted; mismatch → abort "ingest loop keys" (E-22 completeness half).
```
Verified: count_kind/key-multiset for kIngestFold filter by EXACT kind
(`op.kind != kIngestFold continue`) → kIngestLoop never pollutes the fold count
or `got_ingest_keys`; the quad keep kIngestFold=0.

EMISSION ORACLE (M15): lldb Arm-C hit-count (bp Procedure.cpp:95, per-location
MAX within ExtendEagerProcedure) == kIngestLoop census per (case, mode). The
stage-a §2 table IS the pre-registered expectation (opt column
0/0/1/3/0/1/0/0/0/2/0 for the 11 pins in order).

================================================================================
## D6 — SITE-5 EXTENSION  (ratified; hygiene riders LABELED per A6)

A PARALLEL sibling check for kIngestLoop (keep the kIngestFold Site-5 multiset
PRISTINE). LowerIngestLoop records into a NEW
`Context::emitted_ingest_loops` (`vector<tuple<int sign, bool is_explicit,
uint8 role, uint64 message>>`); a sibling multiset compares walk-emitted vs the
flow's kIngestLoop ops (enrolled), sorted, abort on mismatch (the E-22
completeness discipline; Fact 1's 1:1 identity is the guarded invariant).
Rejected: extend the kIngestFold multiset with null-table + klass-sentinel keys
— pollutes the fold key.

### >>> HYGIENE RIDERS (A6 — DEFERRED to owner; LABELED, not slice-required)
Two safe fixes ride this slice (the slice touches Site 5). NEITHER is required
for correctness; the owner rules whether to include or defer:
1. **DELETE the dead table-less filter** (Stratum.cpp:2127-2132) + its now-stale
   comment. It defends against a table-less kIngestFold that this design makes
   forever-impossible (the table-less case is its own kind, already excluded by
   `if (op.kind != kIngestFold) continue`). Its comment ("a table-less monotone
   receive enrolls no ingest op … so none appears here") becomes actively
   MISLEADING (a table-less receive now enrolls a kIngestLoop). Recommend
   INCLUDE — the slice makes the comment stale.
2. **FIX the header comment** Stratum.cpp:2099 "V-PRED-XCHECK Site 5" →
   "V-INGEST-XCHECK Site 5" (match the authoritative :2150 abort string + docs).
   §20(T) EXPLICITLY sanctions "the R-E42 diff itself may fix it." Recommend
   INCLUDE.
Both are cosmetic-with-no-live-effect and predate R-E42; label them as RIDERS in
the commit message, not as slice-required work.

================================================================================
## D7 — RENDER + GRAMMAR  (ratified WITH A2 + A3 folded in)

A DEDICATED render case following the kEagerForward table-GUARD (never the
kIngestFold verbatim case — it prints `tid(op.ingest_table)` UNCONDITIONALLY and
`tid` :382 has NO null guard → CRASH on a table-less op). REUSE the existing
`message=<name>/<arity>` token → NO new spelling → NO E-71 lane (see the A4
ruling).

### >>> ADJUDICATED CHANGE A3 (Crit-3 F2, LOW — CONFIRMED at code): MARKER-SHAPE render
The draft rendered kIngestLoop with the kIngestFold-FAMILY sublines (`effects:
{}` / `spine: —`) "for family visibility." **FLIP to MARKER-SHAPE (header +
`args:` line ONLY; NO effects/spine/reads sublines).** GROUNDS (code-anchored,
not taste): kIngestLoop is EFFECT-FREE (D3). Format.cpp's own M7 precedent makes
effect-free ops render header+args-only precisely to AVOID empty sublines — the
in-code comment at :934 states the dedicated marker cases exist because "the
generic default would silently render empty effects:/spine: sublines," treated
as undesirable, and :870 documents kEagerForward as "NO reads/effects/spine
sublines (effect-free markers)." Rendering `effects: {}` + `spine: —` on a
truly-empty op is pure noise, and kIngestLoop sorts INTO the lead-0 marker band
(D8) where every neighbor is subline-free — so marker-shape is BOTH principled
(effect-free ⇒ no empty sublines, per M7) AND locationally consistent. The op's
distinctness from the markers is already carried by its KindName (`kIngestLoop`)
and its `message=` token; the sublines add nothing. (Note the kNegateGate
`effects: {}` in negate goldens is NOT a counterexample — the gate is
effect-BEARING with a kFlagRead that emit_effects skips; it renders a real
`reads:` line. kIngestLoop has NO read.) Owner may override to fold-shape if
family-visibility is preferred; the STRUCT prediction is stated for both below.

PREDICTED PRODUCTION (new case at Format.cpp ~:758, after kIngestFold —
MARKER-SHAPE, recommended):
```
case DROpKind::kIngestLoop: {
  os << " sign=" << SignGlyph(op.ingest_sign)   // +1 → "+"
     << " ctx=" << CtxName(op.ctx)              // "eager"
     << " stratum=" << DROpStratum(flow, op) << "\n";  // 0 (default arm)
  os << "    args:";
  // table= OMITTED (table-less; tid() has no null guard — the kEagerForward guard)
  if (op.ingest_message.has_value()) {
    os << " message=" << std::string(op.ingest_message->NameAsString())
       << "/" << op.ingest_message->Arity();
  }
  os << "\n";
  break;
}
```

PREDICTED kIngestLoop BLOCK (e.g. map_3's `m/2` receive) — MARKER-SHAPE:
```
op.N kIngestLoop sign=+ ctx=eager stratum=0
    args: message=m/2
```
(fold-shape ALTERNATIVE, if the owner overrides A3: two extra sublines
`    effects: {}` and `    spine: —` between the header and `args:`.)

DROpKindName (Format.cpp ~:110, after kIngestFold):
`case kIngestLoop: return "kIngestLoop";` — the SOLE default-less (exhaustive)
DROpKind switch (verified: all other DROpKind switches carry a `default`), so
this case is the -Wswitch/-Werror completeness requirement.

### >>> ADJUDICATED CHANGE A2 (Crit-2 F2, NIT — CONFIRMED at code): census comment bump
kAllKinds (Format.cpp:1054-1068): add `kIngestLoop` at the TAIL (after
kEagerProduct) → census 26→27; the census-line diff is APPEND-ONLY
(`kIngestLoop=N` appends after `kEagerProduct=N`, prefix byte-identical). The
`census_total != flow.ops.size()` guard (:1077) is a day-one HARD gate.
**ADDITIONALLY** bump the in-code census comment **Format.cpp:1045** from
`// ---- census (26 DROpKind counts, ...)` to `(27 DROpKind counts, ...)` — the
draft's D7 specified the kAllKinds append but omitted this comment; it must move
in lockstep for a docs-accurate slice.

================================================================================
## D8 — key_of / pinned_order + VALIDATOR VISIBILITY  (ratified WITH A5 closed)

A DEDICATED key_of arm (mirroring the kIngestFold arm), alongside it at
DeltaRel.cpp:4536 (post-R-JOIN anchors per §20(T) E-128):
```
if (op.kind == DROpKind::kIngestLoop) {
  return Key{0u, 0u, 0u, op_table_id(op) /* = 0, table-less */,
             op.ingest_sign /* +1 */, oi};
}
```
op_table_id checks ingest_table among the pointers → all null → 0. So
kIngestLoop LEADS the dump lattice (lead-0, band-0, table_id-0). Within lead-0/
table_id-0: sign-0 ops (eager markers, kNegateGate — literal sign 0) sort BEFORE
kIngestLoop (sign +1), which sorts BEFORE table-backed ingest folds (table_id
≥1). Multiple loops tie by oi (construction order = the walk's IOs×Receives
order — no pointer ordering, the (F) law). VALIDATOR-ORDERING ONLY. The
dedicated arm is genuinely REQUIRED: key_of's `default` (:4572) returns a lead-1
PHASE key — without this arm kIngestLoop would render in the PHASE section, not
the lead band. No lead-band sign-±1 table-less op exists today (kIngestFold/
kSubgraphInstantiate are table-backed; markers/gate are sign-0), so kIngestLoop
is the first — deterministic, no collision.

### >>> ADJUDICATED CLARIFICATION A5 (Crit-3 F4, NIT — the "implementation check" CLOSED)
The draft flagged an OPEN "implementation check: add a kIngestLoop skip if the
V-READY kind switch is exhaustive." **CLOSED — no skip is needed.** kIngestLoop
is effect-FREE (D3) ⇒ contributes ZERO vec/flag accesses
(LinearizeAndValidateDRFlow §2b iterates op.effects — empty) ⇒ participates in
NO dep edge ⇒ is NEVER a d.from/d.to in the V-READY loop (:5053), so the
off-lattice OR-list (which lists kIngestFold but not kIngestLoop) is IRRELEVANT
to it; V-LINEAR/V-LOOP/V-BAND-HAZARD/V-OLD-EQUIV are all edge-driven and
transparent to it. DROpStratum (:4151 `default: return 0u`) and op_band (:4441
`default: return 0u`) give kIngestLoop stratum-0/band-0 with no abort. The Kahn
linearizer places indegree-0 isolated ops (the effect-free eager-marker
precedent, present in committed goldens) → pinned_order completeness holds. Keep
this as a stage-(c) build/confirm-at-code item with the EXPECTED OUTCOME stated
(clean; no V-READY skip edit; only the DROpKindName case for -Wswitch).

================================================================================
## D9 — CARRIER / BLESS PLAN  (ratified as designed)

NO new carrier. The four kIngestFold=0 quad goldens (map_3, merge_2,
elim-cond-cycle-simple, join_1) are already committed `.deltarel.opt` pins and
gain REAL kIngestLoop blocks; all 11 pins churn census lines.

PRE-BLESS RED PREDICTION: **11 IRGOLD-DIVERGE, 0 IRGOLD-MISSING** (re-bless only
— every pin already carries a `.deltarel.opt` golden + `.irgold` sidecar).

Per-pin [STRUCT] (opt, from stage-a §4; blocks are MARKER-SHAPE per A3):
| pin | kIngestLoop | new blocks | census churn |
|---|---|---|---|
| demand_tc_witness | 0 | none | +token |
| symrec_tie_1 | 0 | none | +token |
| map_3 | 1 | +1 | +token |
| merge_2 | 3 | +3 | +token |
| booleans | 0 | none | +token |
| elim-cond-cycle-simple | 1 | +1 | +token |
| negate_1 | 0 | none | +token |
| negate_6 | 0 | none | +token |
| d5_recursive_negate | 0 | none | +token |
| join_1 | 2 | +2 | +token |
| optimize_2 | 0 | none | +token |

MODE-SPLIT FAMILY (residual): booleans/negate_1/negate_6/optimize_2 flip B→C
under nodf/none (gain kIngestLoop>0; booleans/negate_6 mixed B=1/C=1) — but the
pins are OPT-mode, so UNPINNED. LEAVE nodf unpinned (opt-only pinning
discipline; matches R4). DS-ADJ-1: opt==nocf, nodf==none (nocf axis inert — arm
selection is dataflow-model-determined). d5_recursive_negate is the ZERO-MINT
guard (Arm A/B receives only → kIngestLoop=0).

================================================================================
## D10 — THE GATE BATTERY  (ratified as designed)

- **SUITE pre-bless reds = EXACTLY 11 IRGOLD-DIVERGE (0 IRGOLD-MISSING)** →
  review → sources byte-verified same-as-reviewed → `runall.sh --bless` →
  SUITE PASS(173) ×3 + post-fix. No red→green via bless.
- **BYTE-IDENTITY A/B vs frozen e6eb2e3e (debug) / 035720ac (release)** — full
  corpus, 4 modes, 840 rows × 2 pairs, **0-DIVERGED**. THE crux gate (Arm C's
  emission unchanged; id-stream 1+arity at the original position;
  BuildEagerInsertionRegions untouched).
- **ctest 5/5 debug + 5/5 ASAN; ASAN both surfaces SUITE PASS(173), zero reports.**
- **config-invariance SINGLE-HASH** on the quad × {opt, none} (debug + release).
- **E-62 re-grep LIVE** (this slice touches lib/DeltaRel + Stratum + Procedure):
  expect ONLY the sanctioned Stratum.cpp:1073 comment + the RAT-3
  InstanceOrderTest fixture; zero out-of-lib body_ops/output_ops readers.
- **Q5 progsize@128 ABABAB** — headers byte-identical → 0.0% median (bench never gates).
- **eqgate green** (demand_tc_witness / demand_neighborhood_witness — dump-only).
- **M15 count-oracle blind lane**: lldb Arm-C hit-count == kIngestLoop census on
  all 11 carriers × 4 modes (stage-a §2 IS the pre-registered oracle).
- **-Wswitch/-Werror build**: the DROpKindName case (Format.cpp) is the sole
  exhaustive-switch completeness requirement (A5 verified all other switches
  carry defaults).
- **THREE-WAY CONVERGENCE** on the quad: author hand-prediction (census +
  MARKER-SHAPE tail-block placement — lead-0, sign +1, sorted after the sign-0
  markers, tie-broken by ctor; high oi labels) == blind worktree prototype ==
  pristine implementation ×3, all modes. Exact op.N labels + render sequence are
  the deliverable.

[BYTE] = all generated `datalog.h`/`.cpp` on 173 cases × 4 modes byte-identical
to frozen (LowerIngestLoop is the byte-move of Arm C; next_id untouched).
[STRUCT] = the 11 `.deltarel.opt` goldens (census token on all 11; MARKER-SHAPE
kIngestLoop blocks on map_3(+1)/merge_2(+3)/elim(+1)/join_1(+2)) — gated by the
irgold re-bless, not byte-identity.

================================================================================
## WHAT DOES NOT CHANGE

- **BuildEagerInsertionRegions** — UNTOUCHED; called identically in Arm C
  (table==null; descent Emplaces into loop->body).
- **Arm A / Arm B** — UNTOUCHED; only Arm C becomes one LowerIngestLoop call.
- **MakeStageOneIngestFolds / MakeMonotoneIngestFold / LowerIngestFold** —
  UNTOUCHED (all table!=null asserts intact — the D2 sibling payoff).
- **The kIngestFold count law + the four quad pins' kIngestFold=0** — UNCHANGED.
- **next_id stream** — reproduced 1+arity at the original walk position.
- **The kCounter-first sites (:1937 / :2135 / :3177)** — UNCHANGED (all
  kind-cased; exclude kIngestLoop by construction — the A1 correction).
- **The S1/S2/S3 seam artifacts** — REMAIN until R-final. Only **S4** (the
  table-less shim, the LAST zero-model-representation emission surface) retires
  this slice. R-final still owes the DIRECTION FLIP + S1/S2/S3 retirement + the
  per-join emission op + the pivot-belt fold + the DeltaRel→Rel rename.

================================================================================
## RITUAL-HEAD RULINGS (QUESTION / RECOMMENDATION / GROUNDS / ALTERNATIVES)

**RH-1 OP FAMILY + KIND NAME.**
Q: Ingest-family sibling kind, marker kind, or widen kIngestFold?
REC: a new INGEST-family kind `kIngestLoop` at the enum TAIL (value 26).
GROUNDS: layer (ingest, per the :2366 comment); id-discipline (must allocate
1+arity — impossible for a zero-id marker); count-cleanliness (keeps the quad's
kIngestFold=0). ALT: (b) marker kind — REJECTED-BY-CONSTRUCTION (violates the
zero-next_id mold + wrong layer); (c) widen kIngestFold — breaks the count law,
shifts 4 pins.

**RH-2 LOWERING SHAPE + ARM-C GUARD.**
Q: Sibling LowerIngestLoop or relax LowerIngestFold's :1930 assert? Ship the
Arm-C cursor-shape guard?
REC: a SIBLING LowerIngestLoop (byte-move of Arm C, returns the VECTORLOOP as
descent cursor); SHIP a light always-on cursor-shape guard (VECTORLOOP-over-vec,
mirrors Arm B :87-93). GROUNDS: relaxing :1930 pollutes every fold-path line with
null-guards; the sibling keeps LowerIngestFold pristine. ALT: relax :1930; omit
the guard as trivial.

**RH-3 PAYLOAD + EFFECTS.**
Q: Store what? Effect-free or model a read/vec effect?
REC: store ingest_message + ingest_receive (reuse existing fields, no struct
edit); EFFECT-FREE (`effects: {}`). GROUNDS: the shim only READS the param vec;
no pre-existing DR effect to re-source (ADJ-RJ-6); kVecAppend models a producer.
ALT: model an effect (WRONG — no DR dep edge); re-derive the message (the
kIngestFold precedent stores it and the key-multiset reads it).
NOTE: **ZERO kCounter-first edits are required** (A1) — :1937/:2135/:3177 are all
kind-cased and exclude kIngestLoop by construction.

**RH-4 ENROLLMENT POSITION.**
Q: Enroll in the per-IO ingest loop (:2400) or tail-append after EAGER_WEB?
REC: TAIL-APPEND — a dedicated query.IOs()×Receives() re-derivation loop after
EAGER_WEB. GROUNDS (Fact 1/2): the arm is not reachability-gated so enrollment
re-derives with no walk stream; tail-append gives the highest oi → every op.N
label stays byte-stable, additive diff. ALT: enroll at :2400 — WHOLESALE
renumbers op.N on the four quad carriers.

**RH-5 COUNT LAW.**
Q: Per-receive or per-visit? Absorb into exp_ingest or a separate counter?
REC: PER-RECEIVE; a THIRD recount arm under a SEPARATE `exp_ingest_loop` counter
+ `expect(kIngestLoop, …)` + a sibling per-op key multiset (table omitted).
GROUNDS: Fact 1 (per-receive == per-visit == per-emission); a separate counter
keeps the kIngestFold count law intact. ALT: absorb into exp_ingest — REJECTED
(breaks the count law, shifts the quad).

**RH-6 SITE-5 EXTENSION + HYGIENE RIDERS.**
Q: Sibling Site-5 check or extend the kIngestFold multiset? Include the two
hygiene riders?
REC: a PARALLEL sibling multiset (Context::emitted_ingest_loops, table-less
key); INCLUDE both riders AS LABELED RIDERS — (i) delete the dead Site-5
table-less filter + its now-misleading comment (:2127-2132); (ii) fix the
:2099 header comment "V-PRED-XCHECK" → "V-INGEST-XCHECK Site 5" (§20(T)-
sanctioned). GROUNDS: the sibling keeps the fold key clean; the riders are safe
and the slice touches Site 5 (the filter's comment goes stale). ALT: extend the
kIngestFold multiset with null-table/klass-sentinel keys (pollutes the fold
key); DEFER the riders (they predate R-E42 — owner's call).

**RH-7 RENDER SUBLINE SHAPE.**  *(the genuinely-open new ruling — Crit-3 F2)*
Q: Does the kIngestLoop block render the kIngestFold-family sublines (`effects:
{}` / `spine: —`) or the marker header+args-only shape?
REC: **MARKER-SHAPE (header + `args:` line only; NO empty sublines).** GROUNDS
(code-anchored): kIngestLoop is effect-free, and Format.cpp's own M7 precedent
(dedicated marker cases at :934, comment at :870) makes effect-free ops render
header+args-only SPECIFICALLY to avoid empty effects:/spine: sublines;
kIngestLoop also sorts INTO the subline-free lead-0 marker band, so marker-shape
is both principled and locationally consistent. Its distinctness is already
carried by the KindName + `message=` token. ALT: fold-shape (`effects: {}` /
`spine: —`) for ingest-family visibility — the draft's original choice; adds two
noise sublines and makes kIngestLoop the lone subline-bearing lead-band op. This
ruling sets the exact quad STRUCT bytes; the implementer follows the ratified
shape.

**RH-8 TOKEN / E-71.**  *(Crit-3 F1)*
Q: Reuse `message=<name>/<arity>` (no E-71) or mint a distinguishing
`receive=`/`loop=` token (E-71 lane)?
REC: REUSE `message=` → NO new spelling → NO E-71 lane (the R-JOIN "bare shape,
no new token" precedent — a new op-block reusing existing tokens does not
trigger E-71, which governs NEW spellings). GROUNDS: the token exists in the T2b
grammar; nothing distinguishes a dedicated token. NOTE (owner-visible, per
Crit-3 F1): `message=` shifts REFERENT here — on kIngestFold it names the
message folded-INTO; on kIngestLoop it names the add-vec being LOOPED — and the
block is a token-combination unwitnessed in any golden. Present as a ratifiable
conclusion, not a settled fact. ALT: a `receive=`/`loop=` token — triggers an
E-71 grammar lane; declined unwitnessed.

**RH-9 key_of ORDERING.**
Q: A dedicated key_of arm or reuse the kIngestFold arm?
REC: a DEDICATED lead-0 arm Key{0,0,0, op_table_id=0, sign=+1, oi} (genuinely
required — the default returns a lead-1 PHASE key). GROUNDS: table-less ⇒
op_table_id 0 ⇒ lead-0 band; deterministic tie-break by construction order.
Effect-free ⇒ INVISIBLE to V-READY/V-LINEAR/V-BAND-HAZARD (A5, closed — no skip
needed). ALT: extend the kIngestFold arm's guard to `kIngestFold || kIngestLoop`
(equivalent bytes; a dedicated arm is clearer).

**RH-10 CARRIER / BLESS PLAN.**
Q: New carrier, or re-bless the existing quad? Pin any nodf mode-split?
REC: NO new carrier (the quad is committed); 11 IRGOLD-DIVERGE + 0
IRGOLD-MISSING (re-bless only); leave the nodf/none mode-split family
(booleans/negate_1/negate_6/optimize_2 B→C) UNPINNED (opt-only pinning). GROUNDS:
the quad already gains real blocks; DS-ADJ-1 opt==nocf/nodf==none. ALT: pin a
nodf mixed-ingest witness (booleans B=1/C=1) — declined as scope creep unless the
owner wants coverage.

================================================================================
## OWNER RULINGS (ratified 2026-07-27, at the ritual head, via the session Q&A)

- RH-1..RH-5, RH-9, RH-10: RATIFIED AS RECOMMENDED (the mechanism core:
  kIngestLoop ingest-family kind at the enum tail; sibling LowerIngestLoop
  byte-move + always-on cursor-shape guard; effect-free
  ingest_message+ingest_receive payload, ZERO kCounter-first edits (A1);
  TAIL-APPEND enrollment via a dedicated query.IOs()xReceives()
  re-derivation loop; PER-RECEIVE count law under a SEPARATE counter +
  sibling Site-5 multiset; dedicated lead-0 key_of arm, no V-READY edits;
  NO new carrier — re-bless the 11 pins, predicted 11 IRGOLD-DIVERGE / 0
  MISSING).
- RH-7: MARKER-SHAPE render (header + args: only; no empty sublines).
- RH-8: REUSE message=<name>/<arity> — NO new spelling, NO E-71 lane; the
  referent shift (names the drained receive, not a published message) is
  DOCUMENTED HERE as part of the ruling.
- RH-6: BOTH hygiene riders INCLUDED (delete the dead Site-5 table-less
  filter + stale comment; fix the :2099 V-PRED-XCHECK header naming to
  V-INGEST-XCHECK Site 5).
