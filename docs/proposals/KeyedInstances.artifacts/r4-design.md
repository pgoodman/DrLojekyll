======================================================================
COMMITTED AT THE R4 LANDING (2026-07-23, the Rel epoch; base tip
b1c95bac). THE ADJUDICATED DESIGN CONTRACT for R4 NEGATE — the
FOURTH step-kind migration, executed as the owner-ruled OPTION (A)
DIFF/RE-SOURCE (E-108/§20(M)/(O)): the eager kNegateGate mint MOVES
from the query.Negations() inventory loop to the Build.cpp IsNegate
walk dispatch (MakeEagerNegateOp + LowerRelStep_Negate on the
M1-M13 mold), dropping the 9-case over-enumeration so the "every
minted gate is eager-walk-reached" premise becomes TRUE. THE FORCED
MOLD DELTA (ruled, binding): kNegateGate is EFFECT-BEARING (a REAL
kFlagRead{negated_table, NegateGatePred(kEager,hint)} — dep-edge-
bearing RAW/WAR, unlike the effect-free R1-R3 markers); the ctor
RECONSTRUCTS the effect identically at walk mint and EAGER_WEB
re-invocation (M2' extended to an effect); the op stays OUT of
IsEagerMarkerKind (gate_* payload, not eager_view/table_op_table —
REQUIRED, the marker recount loop would false-fire on null fields).
Ritual record: stage-(a) subsystem build-out + stage-(b) designer +
mechanical carrier lane -> stage-(c) 2 adversarial critics -> xhigh
adjudicator (8 findings ruled: C2-F1 MED-HIGH gate keys TABLE-LESS
op_table_id=0 — subsystem.md's gate/forward reorder STRUCK,
design.md's gate-leads adopted; C1-F1+C2-F3 MED merged into ONE
corrected structural oracle — POST census == BENR dispatch count
per (case,mode), per-VISIT (kEagerForward=12 vs 11 TUPLE views
proves per-visit≠per-view) AND emission-side (never re-derive
CanReceiveDeletions from the same graph); C1-F2 V-NEG-CTX
grounding fixed (walk-cut⇒no standalone op, not an @never lemma);
C1-F3 "inventory-only" framing struck — the dropped gates carry
REAL seed-phase read effects, safety = lead-0 off-lattice
pure-reader linearizer property + FULL-corpus A/B over all 9 cut
cases; C2-F2 the mint sits in BuildEagerRegion whose second caller
Procedure.cpp:813 passes only all-constant TUPLEs — benign, doc
note; C2-F4 the @never-token deferral basis VOID — affirmative
owner ruling required; C2-F5 M2'-technicality noted). OWNER
RULINGS (2026-07-23, binding): @never renders IMPLICIT via reads:
(Present vs InI disambiguates; NO token, NO dedicated render case
— the gate stays on the generic fallback; a POSITIVE E-71 ruling,
not an inherited deferral); the differential-input @never shape
stays a LABELED LATENT GAP (no directed fence this slice).
CARRIERS (ruled): negate_1 + negate_6 + d5_recursive_negate —
three FIRST-EVER negate .deltarel.opt goldens (RAT-8; d5 = the
ZERO-MINT NEGATIVE guard, census 1->0). Census: corpus kNegateGate
30->18 opt; disassemble the SOLE mixed case (2->1); six existing
.deltarel carriers BYTE-STABLE (kNegateGate=0). ZERO emission
change (BuildEagerNegateRegion byte-unchanged, the gate never
lowers). The body below is the xhigh adjudication verbatim.
LANE-FILE SCOPING NOTE (Fable-review R4 [4]): the body's citations
of "subsystem.md" / "design.md" / "mech.md" / "critic1.md" /
"critic2.md" name SESSION-SCRATCH lane files (disposable, NOT repo
artifacts; "design.md" there is the stage-(b) lane, never THIS
file). Every ADOPTED or STRUCK item is RESTATED IN PLACE in the
body (Part II lists the full edit list, Part III the full order
ruling, Part IV the full recount/referee) — the in-place
restatements are the binding text; the lane-file names are
provenance only. FABLE-REVIEW RECORD (at the landing, 12-agent
workflow, high): 4 verified findings, ZERO live correctness — [1]
the M10 recount gained the op.ctx==kEager guard (the reserved
standalone seed/fixpoint gate forms sit on deletion-capable negates
by definition and must not trip it; sibling key_of/V-READY sites
carry the same guard); [2] the EAGER_WEB effect-free/six-kinds
comments + Build.h EmittedEagerOp comment refreshed for the
effect-bearing seventh kind; [3] CLAUDE.md re-pointed (NINE
.deltarel goldens incl. the R4 trio; unmodeled arms = JOIN + E-42);
[4] THIS note. One candidate REFUTED (the view_to_model[] mint
idiom matches the live Build.cpp:735 precedent; ModelTableOrNull's
null-return contract is wrong for the always-tabled negated view).
Fixes proven DUMP-NEUTRAL on all 16 pinned surfaces + post-fix
suite/A-B green.
STAGE-(d) + FABLE-REVIEW DETAIL: r4-desired-states.md.
======================================================================

# R4 NEGATE — THE ADJUDICATED DESIGN (option A, diff/re-source)

XHIGH adjudicator, tip b1c95bac (repo read-only). Inputs: subsystem.md, design.md,
mech.md, critic1.md, critic2.md. Every disputed empirical re-run at the code/dumps
by the adjudicator (citations below). Owner frame (R4 = option A, single-authority
ctor + LowerRelStep_Negate wrapper over the UNTOUCHED BuildEagerNegateRegion, carrier
trio negate_6+negate_1+d5_recursive_negate, ZERO emission change) is NOT re-litigated.

BOTTOM LINE: design.md is the sound spine and is ADOPTED verbatim except where a
critic finding folds in. subsystem.md carries ONE load-bearing factual error (the
gate's dump key) that is STRUCK. Two referee-oracle findings fold into a single
corrected oracle. One framing and one proof-grounding fold in. One genuine owner
question (the @never token). Net: the design ships as-is with five recorded repairs
and one owner ruling.

---

## PART I — RULING ON EVERY CRITIC FINDING (at the code)

### C2-F1 [MED-HIGH] gate dump key: subsystem.md says negated-table, design.md says table-less
**CONFIRMED → design.md is right; STRIKE subsystem.md §0.1/§5/AppendixB-6.**
Adjudicator-verified at code: the mint (DeltaRel.cpp:2308-2320) sets `op.gate_negate`
and `op.gate_table`, and NEVER `op.negate_table`. `op_table_id` (DeltaRel.cpp:4409-4416)
resolves ONLY through `table_op_table|product_table|agg_table|negate_table|ingest_table|
fire_table` — `gate_table` is not in that list — so `op_table_id(gate)=0`. The code
comment at DeltaRel.cpp:4405 names it outright: "A NULL table (eager negate gates,
seed/chain folds, pivot assembles) maps to 0". key_of (4456-4458) = `Key{0,0,0,
op_table_id=0, 0, ctor=oi}` ⇒ the gate keys TABLE-LESS and LEADS the dump. A
gate↔same-table-eager-forward swap (subsystem.md §0.1's "secondary finding") CANNOT
occur — the eager forward is table-BEARING (id+1>0), always sorts after the table-less
gate. CONSEQUENCE for the design: adopt design.md §2.3. The ONLY ctor-order
sensitivity is negate_6's TWO table-less gates ordered among THEMSELVES by walk-DFS
ctor (both still lead). This is load-bearing for the three-way-convergence
author-prediction leg: predict "gate leads, table-less; negate_6's two gates may swap"
— NOT subsystem.md's gate/forward swap.

### C1-F1 [MED] §6 referee oracle is per-VIEW; mechanism is per-VISIT
**CONFIRMED empirically → FOLD (restate the oracle per-VISIT).**
Adjudicator-verified: demand_tc_witness (opt) has `kEagerForward` census **=12** but only
**11** TUPLE view defs in the .df (three tables each carry two kEagerForward ops — a
diamond-reached view dispatched once per path). `BuildEagerInsertionRegions` has NO
visited set — the walk multi-visits. So "POST kNegateGate == count of negate views with
!CanReceiveDeletions" is unsound AS AN INVARIANT (it coincides on the negate corpus
only because no corpus negate sits under a multiply-reached monotone predecessor; BENR
hit-counts equal reached-view counts there). It must be stated per-VISIT.

### C2-F3 [MED] §6 step-4 cross-check is circular (re-derives CanReceiveDeletions from the same graph)
**CONFIRMED → FOLD, and it MERGES with C1-F1 into one corrected oracle.**
Re-deriving `!CanReceiveDeletions` from the same dataflow graph the inventory pass reads
is a POST-vs-POST tautology, not an independent oracle. The genuinely independent,
emission-side, per-VISIT observable is the **BENR dispatch count** = the number of eager
negate CHECKMEMBERs `BuildEagerNegateRegion` actually emits. subsystem.md §0 already used
it (lldb hit-count on BuildEagerNegateRegion, "the authoritative counter": 1/2/0 for
negate_1/negate_6/d5). This single oracle satisfies BOTH critics — per-visit (C1-F1) AND
crossing the inventory→emission boundary (C2-F3). See the ADJUDICATED referee, Part IV.

### C1-F2 [LOW-MED] V-NEG-CTX proof conflates output retraction with input CanReceiveDeletions
**CONFIRMED as a logic flaw; conclusion holds → FOLD the corrected grounding.**
`CanReceiveDeletions()` is the INPUT-side cut criterion (the flowing predecessor's
deletability, Build.cpp:970), independent of the `@never` hint (which annotates the
NEGATED relation / output retraction). The lemma "@never ⟹ never retracts ⟹
!CanReceiveDeletions" is a non-sequitur. DISCARD it. Correct grounding: **walk-cut ⟹ no
standalone eager gate is minted; the op-site V-NEG-CTX (DeltaRel.cpp:3057-3072) only ever
validates minted (eager, walk-reached) gates, so the @never-outside-eager clause
(3069-3070) cannot fire on a standalone op regardless.** KEEP the empirical corpus claim
(all 4 `@never` occurrences — map_5 non-negate/E-109, negate_6, negate_downstream_diff,
negate_cobatch_mono — are walk-reached; none of the 9 cut cases contains `@never`,
grep-confirmed by two lanes). NOTE (out of R4 scope, flag to owner): a
differential-INPUT `@never` negate — if constructible — would be walk-CUT and would also
exercise the seed/fixpoint plan-spine gate with a kInI/kInNew pred while the hint is
@never; a latent pred question orthogonal to R4. Candidate directed fence, not an R4
deliverable.

### C1-F3 [LOW/framing] the 9-case drop removes REAL seed-phase edges, not "inventory-only" ornament
**CONFIRMED → FOLD the framing correction (do NOT weaken the A/B).**
d5's phantom gate (op.54) carries live WAR edges to on-lattice seed claim drains
(stratum=3), a commit sweep, and two ingest folds — it genuinely participates in the seed
schedule. Emission-neutrality is REAL but for a PRECISE reason to state: the gate is
**lead-0 off-lattice, a pure reader with no incoming edges** (key_of lead=0). In the Kahn
linearizer every lead-0 op precedes every lead>0 op by lead alone; removing a
pure-reader lead-0 source cannot reorder the lead>0 round-body ops among themselves ⇒
`body_ops`/`output_ops` (pinned_order-derived) byte-stable ⇒ `LowerDRRounds` byte-stable.
REQUIREMENT: the A/B 0-diverged gate MUST cover ALL nine cut cases (d5, merge_5,
cond_both_polarities, cond_diff_flipflop, disassemble, negate_cobatch_diff,
negate_lower_strata, negate_multiplicity, negation_flap), not the trio only — design.md
§8 already says "A/B FULL CORPUS", KEEP it there; strike the word "inventory-only" as the
safety argument (it is the linearizer property + full-corpus A/B).

### C2-F2 [MED] the mint lives in BuildEagerRegion (2 callers); M13 discharge covers only BuildEagerNegateRegion
**CONFIRMED benign → FOLD a documentation correction (no code change).**
Adjudicator-verified at code: the new mint sits in the IsNegate arm of `BuildEagerRegion`
(the dispatch), which has TWO callers — Build.cpp:978 (successor loop, AFTER the :970
cut) and Procedure.cpp:813. Procedure.cpp:788 iterates `impl->query.Tuples()` with an
all-constants filter and passes `QueryView(tuple)` — ALWAYS a QueryTuple, never a Negate
— so the IsNegate arm (and the mint) is UNREACHABLE via that caller; the transitive walk
from the constant tuple still routes every negate through the :970 cut. The record must
state: the mint sits in BuildEagerRegion; its second caller (Procedure.cpp:813) passes
only all-constant TUPLE views, so the IsNegate arm is unreachable there; the M10 recount
(Part III) is belt-and-suspenders were a stray path ever to appear. M13 (BENR
single-caller) remains true and separately relevant (BENR is byte-unchanged). Close the
gap in the desired-states doc.

### C2-F4 [LOW] @never token: R2 "declined unwitnessed" basis is now VOID
**CONFIRMED → OWNER QUESTION (Part VI). Not adjudicator-closable.**
negate_6 becomes a blessed carrier and the SOLE @never-NEGATE witness (E-109), so the R2
"declined unwitnessed" ground is gone; the E-71 lane must AFFIRMATIVELY rule, not inherit
the deferral. Both lanes recommend (a) status-quo implicit (`reads: Present` vs `InI`
disambiguates; hint re-derivable). Escalated as the single grammar owner-question.

### C2-F5 [LOW/note] M2' technicality — gate_pred/gate_hint stored though re-derivable
**NO CHANGE NEEDED — deliberate status-quo hold, recorded.**
By the letter of M2' a re-derivable value is not stored — but V-NEG-CTX
(DeltaRel.cpp:3057-3072) is a CROSS-CHECK validator whose entire job is to compare the
STORED `gate_pred`/`gate_hint` against the re-derived `want`; the stored value is the
CHECKED ARTIFACT, so storing it is intentional. `gate_table` legitimately needs
view_to_model context. Preserving today's exact field set is the zero-emission-change
choice. Ledger note, not a defect.

---

## PART II — THE ADJUDICATED DIFF SHAPE (function-level edit list)

Adopts design.md §9 verbatim with the C2-F2 documentation note. FIVE code edits across
three files; NO enum edit, NO census-constant change (stays 24), NO render-case add
(under owner ruling (a)).

**`lib/DeltaRel/DeltaRel.h`**
- ADD decl `DROp MakeEagerNegateOp(QueryView negate_view, TABLE *negated_table);` near
  the other `Make*Op` decls (~:1006). NO enum edit — `kNegateGate` STAYS value 10
  (deleting slot 10 renumbers kPivotAssemble…kEagerSelect 11-23, breaking the
  order-sensitive `kAllKinds`/census; the M1 enum-TAIL rule governs NEW kinds only, and
  option A adds no kind — it MOVES a mint).

**`lib/DeltaRel/DeltaRel.cpp`**
- DELETE the NEGATE_GATE mint loop :2289-2321 (the `----- eager NEGATE_GATE` comment
  block + the `for (QueryNegate negate : query.Negations())` loop). This disposes of the
  stale mint-head comment (E-108: the dead "Build.cpp:1048-1051" anchor + the aspirational
  "Every negate reached by the eager insertion walk"). Leave the ADJACENT INGEST_FOLD
  block untouched.
- ADD `MakeEagerNegateOp(QueryView, TABLE*)` after :1360 — the LIFTED loop body,
  byte-identical op content: sets the six `gate_*` fields AND pushes the `kFlagRead`
  effect `{read_table=negated_table, pred=NegateGatePred(kEager,hint), ctx=kEager}`.
  Populates ONLY `gate_*` + the effect — NOT `eager_view`/`table_op_table` (keeps the op
  OUT of IsEagerMarkerKind and on its own key_of/V-READY/render branches).
- EDIT the EAGER_WEB re-invocation switch :2450-2478: +1 `case kNegateGate:
  flow.ops.push_back(MakeEagerNegateOp(*rec.view, rec.table)); break;`. The existing
  `default: abort()` keeps guarding a mis-kinded record.
- EDIT the V-NEG-CTX op-site arm :3057-3072: +1 M10 strengthened recount (Part III).

**`lib/ControlFlow/Build/Build.cpp`**
- EDIT `RecordEagerDispatch` (:1123-1131): +1 `if (op.kind == kNegateGate)` branch
  sourcing `rec.view=op.gate_negate`, `rec.table=op.gate_table` (the gate's identity is
  in `gate_*`, not `eager_view`/`table_op_table`). `EmittedEagerOp.view` is
  `std::optional<QueryView>` (Build.h:247) so the assignment + later `*rec.view` deref is
  well-typed.
- ADD `LowerRelStep_Negate` wrapper after :1216 (R1 thin-wrapper shape, NO M11 verbatim
  extraction — Negate.cpp is a real 105-line builder): `RecordEagerDispatch(context, op);
  BuildEagerNegateRegion(impl, pred_view, negate, context, parent, last_table);`.
- EDIT the IsNegate dispatch arm :1310-1313: resolve `negated_table =
  view_to_model[negate.NegatedView()]->FindAs<DataModel>()->table`, mint `const DROp op =
  MakeEagerNegateOp(view, negated_table)`, call `LowerRelStep_Negate(...)`. The walk only
  reaches `!CanReceiveDeletions()` negates, so the mint is UNCONDITIONAL at the dispatch.
  DOC NOTE (C2-F2): this arm is in BuildEagerRegion; its second caller Procedure.cpp:813
  passes only all-constant TUPLE views (unreachable here).

**`lib/DeltaRel/Format.cpp`** — NO CHANGE under owner ruling (a). (Only under (b): a
`NegateHint` loud-abort name table + a dedicated render case.)

**`tests/OptDiff/cases/`**
- EDIT `negate_1.irgold`: add `deltarel opt` (keeps the PIN-3 `negate_1.df.opt` fence).
- ADD `negate_6.irgold`, `d5_recursive_negate.irgold`: `deltarel opt`.
- EDIT `d5_recursive_negate.dr` header comment: the §20(O) R4-input nit — the cut
  misattribution "CanProduceDeletions" → "CanReceiveDeletions" (both hold in d5, but the
  criterion is CanReceiveDeletions, Build.cpp:970).

**`tests/OptDiff/goldens/`** (via `runall.sh --bless`, post-review) — ADD
`negate_1.deltarel.opt.golden`, `negate_6.deltarel.opt.golden`,
`d5_recursive_negate.deltarel.opt.golden` (FIRST-EVER deltarel negate goldens).

Net: ~70 code lines (a mint RELOCATION, not new logic) across 3 code files + 3
sidecars/1 comment + 3 blessed goldens. No enum kind, no render case, no name table.

---

## PART III — EFFECT / ENROLLMENT RULING + PREDICTED .deltarel ORDER

**The forced mold delta (ruled, binding):** `kNegateGate` is EFFECT-BEARING — it carries
a REAL `kFlagRead{read_table=negated_table, pred, ctx=kEager}` unlike the effect-free
R1-R3 markers. `MakeEagerNegateOp` RECONSTRUCTS the read from `(negate_view,
negated_table)` (M2' extended to an effect), identically at walk-time and at the EAGER_WEB
re-invocation. The effect is PRESERVED FOR EVERY OP THAT STILL MINTS (the ruled
invariant). The dep-edge SET is EFFECT-determined (kFlagRead table-match vs the negated
table's flag writers), NOT enrollment-determined ⇒ the WAR edges are byte-stable (labels
renumber with oi). Verified live: negate_1 gate → op.6 (commit-sweep) WAR + → op.9
(ingest-fold) WAR; negate_6 op.7 AND op.8 each → op.6 + → op.9 WAR.

**Enrollment: TAIL-APPEND per M4 (ruled — position need NOT be preserved).** The gate
joins the `emitted_eager_ops` stream, re-invoked at :2450-2478 STRICTLY AFTER the ingest
folds. Grounds: (a) no committed .deltarel golden pins any negate case (all six carriers
kNegateGate=0 — mech.md §2, verified); (b) the trio are FIRST-EVER goldens blessed
post-move (renumber/tie captured, not compared); (c) A/B compares generated C++ and the
gate is inventory-only-to-emission (zero emission change) ⇒ 0-diverged; (d) the linearizer
is valid either way (WAR edges force gate-before-writer regardless of oi).

**Predicted .deltarel order consequences (ADOPT design.md §2.3; STRIKE subsystem.md §0.1):**
- The surviving gate keys **TABLE-LESS lead-0** (`op_table_id=0`, verified Part I C2-F1) ⇒
  it LEADS the dump. Of key_of's six fields the move changes ONLY `ctor` (=oi).
- negate_1 (single negate, no seed/chain-fold/pivot-assemble table-less ops — census
  confirms kSeedFold=0 kChainFold=0 kPivotAssemble=0): the gate is the SOLE table-less
  lead-0 op ⇒ render SEQUENCE structurally identical (gate leads, then the table-bearing
  lead-0 ingest folds + eager forwards, then lead>0 phases). Only op LABELS renumber (gate
  op.7 → a tail slot; ingest folds 8-9 → 7-8). NO gate↔forward swap (subsystem.md's error).
- negate_6 (TWO table-less gates on %table:10): both LEAD; ordered among THEMSELVES by
  walk-DFS ctor, which MAY swap them vs today's Negations()-order (op.7 normal / op.8
  @never). Deterministic (walk-DFS is a std::vector by index). This is the ONLY genuine
  ctor reorder among the survivors — capture it at bless.
- d5_recursive_negate: the op VANISHES (census 1→0); the enrolled-after ops shift down.

---

## PART IV — A.6(c): THE M10-STRENGTHENED RECOUNT + THE STRUCTURAL REFEREE

**A.6(c) arm (ADOPT design.md §4, co-located at the V-NEG-CTX op-site
DeltaRel.cpp:3057-3072):**
```
// R4 (M10 strengthened recount): under option A a standalone eager gate is minted
// ONLY at the walk dispatch, which the cut (Build.cpp:970) skips for a
// deletion-capable negate. Every enrolled gate's negate is therefore walk-uncut.
if (op.gate_negate && op.gate_negate->CanReceiveDeletions()) {
  ValidatorFail("R4 A.6(c): eager negate gate's negate is walk-cut "
                "(CanReceiveDeletions) — must not mint under option A");
}
```
Predicate = `IsNegate && !CanReceiveDeletions()` — the EXACT Build.cpp:970 cut criterion.
NOT `InductionGroupId` (F22: a fully-interior negate loses its group id). NEVER
false-aborts: a MINTED op is walk-reached ⇒ CanReceiveDeletions false ⇒ passes; a walk-cut
negate mints no op and is not checked (NON-DEAD via d5). This is a per-op check STRONGER
than a count; the gate has no `expect()`-count line and cannot join the IsEagerMarkerKind
A.6(c) marker loop (that loop reads `eager_view`/`table_op_table`, both null on the gate —
it would false-fire; keeping the gate OUT of IsEagerMarkerKind is REQUIRED, not optional).
The DS-ADJ-7 marker table-match idiom (`table_op_table == ModelTableOrNull(v)`) does NOT
transfer — the gate's table is the NEGATED view's model table, not its own. An OPTIONAL
hardening `gate_table == ModelTableOrNull(negated_view)` is noted, NOT required for R4.

**THE STRUCTURAL REFEREE (ADJUDICATED — merges C1-F1 + C2-F3; supersedes design.md §6
step 4):** orchestrator-run, mechanical, NOT a stdout diff (the gate is inventory-only).
1. Frozen baseline: for every corpus `.dr` (honoring `.drflags`), compile with
   `-deltarel-out`, record `kNegateGate=N` per (case, mode) → PRE map (all 4 knob modes).
2. R4 tip: same sweep → POST map.
3. ASSERT the diff is EXACTLY the 9-case drop of Part V (per-mode: opt/nocf identical,
   nodf/none may differ — see the mode subtlety); every other (case,mode) byte-identical.
4. **THE INDEPENDENT ORACLE (corrected):** POST `kNegateGate` census per (case,mode) ==
   the **BENR dispatch count** (lldb hit-count on `BuildEagerNegateRegion`, or a grep of
   the generated `.cpp` for the emitted InI/Present member gates). This oracle is
   **per-VISIT** (C1-F1: the walk multi-visits — kEagerForward=12 vs 11 views proves
   per-visit≠per-view; the negate corpus coincides only because no corpus negate sits
   under a multiply-reached predecessor) AND **emission-side** (C2-F3: it crosses the
   inventory→emission boundary, unlike re-deriving CanReceiveDeletions from the same
   graph). Any future negate-gate count cross-check MUST be the per-visit walk-dispatch
   multiset, NEVER a per-view count.

**Mode subtlety (verified by adjudicator):** the census is CONTROL-flow-axis stable
(opt==nocf) but DATA-flow-axis variable. merge_5 = **3 opt / 3 nocf / 4 nodf / 4 none**
(a negate CSE'd away under df-opt). d5 = 1 opt / 1 nodf (both → 0 post). So PRE is
mode-dependent; every CUT negate → 0 regardless. The referee compares POST==BENR-count
PER MODE, never a single fixed number.

---

## PART V — PER-CASE CENSUS PREDICTIONS

**The 9 over-enumerating cases (opt; PRE→POST = walk-reached = BENR count):**

| case | PRE (opt) | cut surplus | POST (opt) |
|---|---|---|---|
| cond_both_polarities | 2 | 2 | 0 |
| cond_diff_flipflop | 1 | 1 | 0 |
| d5_recursive_negate | 1 | 1 | 0 |
| disassemble | 2 | 1 | **1 (MIXED: 1 reached + 1 cut)** |
| merge_5 | 3 (opt) / 4 (nodf) | all | 0 |
| negate_cobatch_diff | 1 | 1 | 0 |
| negate_lower_strata | 1 | 1 | 0 |
| negate_multiplicity | 1 | 1 | 0 |
| negation_flap | 1 | 1 | 0 |

disassemble is the SOLE mixed case; its POST=1 rests on BENR=1-of-2, measured
independently by TWO lanes (subsystem.md §0 + critic1.md) — verified at implementation by
the Part IV BENR-count referee, not by adjudicator lldb here.

**Carrier trio (opt):** negate_1 **1→1** (PIN-3 differential-table `@%table:4` carrier;
op renumber only, gate leads table-less); negate_6 **2→2** (@never carrier, E-109; two
table-less gates may swap by walk-DFS ctor); d5_recursive_negate **1→0** (the ZERO-MINT
NEGATIVE GUARD — the walk-cut recursive negate mints no standalone gate; its real gate
stays a fixpoint chain-fold plan-spine `kGate` sub-object, untouched).

**Unchanged >0 carriers (opt, all walk-reached):** compare_4=2, insert_4=1, negate_2=2,
negate_3=3, negate_4=1, negate_5=1, negate_cobatch_mono=1, negate_downstream_diff=1,
product_conds=2. Corpus total (opt) **30 → 18**.

**Six existing .deltarel carriers (BYTE-STABLE, verified):** demand_tc_witness,
symrec_tie_1, map_3, merge_2, booleans, elim-cond-cycle-simple — all kNegateGate=0
(mech.md §2, read from committed goldens). Removing the empty Negations() loop shifts no
flow.ops index for a negate-free program; tail-appending 0 gates leaves
`emitted_eager_ops` unchanged; the census line `kNegateGate=0` already renders. The
ADJ-S2 op.0/op.1 ingest-fold pin holds (the negate loop sat BEFORE the ingest loop).

---

## PART VI — RENDER DELTA + OWNER QUESTIONS

**Render delta:** NONE under owner ruling (a). `kNegateGate` has no dedicated render case;
it falls through the generic fallback (Format.cpp:955-966, which already lists
"negate-gate") — header `kNegateGate sign=· ctx=eager stratum=0` + `reads:` (kFlagRead →
`InI(%table:N)` normal / `Present(%table:N)` @never) + `effects: {}` (kFlagRead excluded
from the effects set) + `spine: —`. Uniform 4-line block, verified byte-exact in mech.md
§4/§5 (census line 367 bytes, single `\n`, pure ASCII). The re-sourced gate that still
carries its kFlagRead renders identically. The ONLY delta is WHICH ops exist (d5: one
fewer) and their LABELS/tie-break order — never the per-op spelling.

**OWNER QUESTIONS (minimal — everything else adjudicated above):**

1. **[E-71 grammar — the @never token spelling.]** negate_6 becomes a blessed carrier and
   the sole @never-NEGATE witness (E-109), so the R2 "declined UNWITNESSED" basis is VOID
   (C2-F4) — the lane must AFFIRMATIVELY rule, not inherit the deferral. Options:
   (a) STATUS-QUO IMPLICIT — @never distinguished only by `reads: Present` vs `reads: InI`.
       Zero grammar change, render stays on the generic fallback, strengthens the
       zero-render-change claim; hint is re-derivable from the view (HasNeverHint).
   (b) EXPLICIT `hint=never` header token — needs a `NegateHint` loud-abort name table
       (M7' precedent) + a DEDICATED render case (moves the gate off the generic fallback).
       Grammar churn confined to the 3 new carriers.
   (c) a dedicated `args:` line (gate_negate/gate_table/gate_hint) — fuller, heavier.
   BOTH LANES + adjudicator RECOMMEND **(a)** as a POSITIVE ruling ("Present disambiguates;
   no token"). This is the one thing R4 cannot land without an owner word, because it fixes
   the negate_6 golden bytes. Census/op-existence are (a)/(b)/(c)-invariant.

2. **[Optional, owner-flag only — NOT an R4 blocker.]** A differential-INPUT `@never`
   negate (walk-cut, would carry a kInI/kInNew seed/fixpoint plan-spine gate while the
   hint is @never) is not in the corpus (C1-F2 note). Does the owner want a directed fence
   minted now, or left as a labeled latent gap? Orthogonal to R4; adjudicator default =
   leave as a labeled gap unless the owner wants the fence.

---

## PART VII — THE GATE BATTERY (adopt design.md §8 + the folds)

- SUITE pre-bless reds = EXACTLY 3 IRGOLD-MISSING (`negate_1`/`negate_6`/`d5_recursive_negate`
  `deltarel.opt`, new surfaces with no golden yet); after `--bless` → SUITE PASS (173) ×3.
  No red→green via bless of a failing case (new surfaces, not regressions).
- A/B FULL CORPUS: 0-DIVERGED, ×4 modes + nested + data/ rows, vs frozen baselines
  re-snapshot at the R4 session open. MUST cover ALL NINE cut cases (C1-F3), not trio-only
  — the "inventory-only" phrasing is struck; safety = the lead-0 off-lattice pure-reader
  linearizer property + full-corpus A/B.
- STRUCTURAL census referee (Part IV): the 9-case PRE/POST diff, all 4 modes, POST==BENR
  dispatch count per (case,mode) — the corrected per-visit/emission-side oracle.
- ctest 5/5 debug + 5/5 ASAN; ASAN both surfaces SUITE PASS (173), zero reports.
- config-invariance single-hash ×N on the trio × opt (3-run debug + release);
  determinism holds (walk-DFS = std::vector by index).
- Q5 progsize (unchanged emission ⇒ headers byte-identical).
- E-62 re-grep LIVE (this diff TOUCHES lib/DeltaRel — ctor, loop deletion, EAGER_WEB
  case, recount); expect the sanctioned Stratum.cpp:1073 comment + RAT-3
  InstanceOrderTest hits only.
- THREE-WAY CONVERGENCE for the trio goldens: author hand-prediction (census + the
  TABLE-LESS-gate-leads structure of Part III — NOT subsystem.md's struck gate/forward
  swap) == blind worktree prototype == implementation bytes. Exact op LABELS are the
  convergence deliverable (structure/census/edge-set predicted; walk-DFS renumber +
  negate_6's two-gate ctor order resolve at build).

---

## APPENDIX — ADJUDICATOR-VERIFIED EMPIRICALS (this session)
- op_table_id(gate)=0: DeltaRel.cpp:2308-2320 (sets gate_*, not negate_table) +
  4409-4416 (op_table_id arm list) + 4405 comment + 4456-4458 key_of. [C2-F1 → design.md right]
- Procedure.cpp:788-813 iterates query.Tuples(), all-const filter, QueryView(tuple) always
  a QueryTuple. [C2-F2 benign]
- demand_tc_witness (opt, -demand): kEagerForward census=12, kEagerForward op-count=12,
  TUPLE .df defs=11. [C1-F1 per-visit≠per-view CONFIRMED]
- merge_5 kNegateGate: opt=3, nocf=3, nodf=4, none=4. d5: opt=1, nodf=1. [mode subtlety
  + d5 cut CONFIRMED]
