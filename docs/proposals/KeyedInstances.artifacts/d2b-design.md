======================================================================
COMMITTED AT THE §20 EPOCH-CLOSE CHECKPOINT (2026-07-21). This is the
adjudicated BINDING implementation contract for its diff, verbatim from
the session scratchpad (d2b/d2b-design.md); its ADJUDICATION LEDGER records every
folded amendment, and the KeyedInstances.md §19(K)-(O) landing records
carry the owner rulings (RAT-1..RAT-10) that resolved its open items.
======================================================================

======================================================================
D2.b DESIGN — RECOGNIZER EXCISION + LOWERING + CODEGEN (the knob goes live)
CODE half. Repo /Users/pag/Code/DrLojekyll, branch keyed-instances, tip
677977f8 (VERIFIED: D1.a 4d92d255 + D1.b 932ad4a6 + D2.a 677977f8 landed).
Sibling agent re-derives the desired DUMPS (§B.4 refresh); this file does
NOT author dump blocks — it pre-registers .ir/.h shape predictions only.
Binding stack (WINS top-down): d1-pinned.md > d1-design-consolidated.md >
d1-desired-states.md. Rulings binding on D2.b: OD-3 fence-(ii)=labeled
feature-gap diagnostic; OD-4 MECHANISM-NATURAL provisioning; OD-5 seal
self-lowered. LOUD carries from §19(K/L/M): ABA-safe deref-free mint
identity; D1.b DROp payload + validators + Format rows EXIST; D2.a
InstanceStore.h surface EXISTS. Grammar law: t2b-grammar.md + t2-dump-spec
v3.4 p10-p14. Each landing still gated by the per-diff design ritual.
======================================================================

This is a DIFF ON PSEUDOCODE OF THE REAL CODE AT TIP. Every code path cited
was read at 677977f8. Small incremental edits; absolute paths; file:line at
tip. The single most consequential finding is R-REBUILD (§0), surfaced LOUD
because it is a correctness question the pinned band-(a) design does not
close and the equivalence gate WILL expose.

======================================================================
§0. LOAD-BEARING RISK — R-REBUILD (surface to the ritual BEFORE coding)
======================================================================

**Claim (HIGH, correctness): the pinned band-(a) "drain the demand
net-additions frontier ONLY" design is NOT answer-identical to the flat
lowering when an edge arrives AFTER a key is demanded — under R-MONO the
demand key never re-crosses, so the instance is never re-touched, and the
new edge is silently dropped from the answer.**

Evidence at tip (d1-ground-truth-nbhd.md Appendix C, the REAL flat .ir):
- `^entry:19` (:184-202): the EDGE `update-count +nonrecursive {..} in
  %table:11 if-crossed → vector-append {@From} into $pivots:25`, AND the
  DEMAND `update-count in %table:8 if-crossed → append {@28} into BOTH
  $pivots:25 and $pivots:29`.
- `^flow:58` (:223-246): join.6 loops `$pivots:25` (the NEW edge-froms AND
  new demand-keys this batch) ⋈ %table:8 (ALL present demand keys) →
  %table:15 → pivots:29 → join.7 ⋈ %table:8 → %table:4. So flat REBUILDS
  INCREMENTALLY on every edge-add whose `From` is an already-demanded key
  (the edge crossing seeds pivots:25, which is join.6's driver).
- The demand ingest fold is `if-crossed` (:194-198): a re-assert of an
  already-present demand key does NOTHING flat OR nested. So "REBUILD via
  demand flap" cannot be a demand re-cross under R-MONO (monotone %table:8
  admits each key once). The flat rebuild is EDGE-triggered via pivots:25.

Consequence: if D2.b's band-(a) drains only the demand frontier
(InstantiateEffects at DeltaRel.cpp:774-778 pushes exactly
`kVecDrain(demand, kNetAddition)`), then an edge added after the demand
assertion NEVER touches the instance → nested answer omits it →
`flat != nested` on the BIRTH-then-edge probe → P-D2b.4 FAILS and HP-5's
over-materialization/under-materialization witness catches it.

**This re-frames OD-4.** The pinned record (d1-pinned §4 OD-4, d1-desired
§C-2) calls the mechanism-natural EDGE net-additions frontier "dead
per-edge-add frontier work at runtime." R-REBUILD shows it is LOAD-BEARING:
the edge frontier is exactly the signal band-(a) needs to re-touch affected
instances on edge-add. OD-4 (ruled mechanism-natural) therefore provisions
the frontier the answer-identity actually requires — the "dead work"
characterization is the mischaracterization, not the provisioning.

**Recommended resolution (for the ritual to adjudicate):** band-(a) drains
BOTH frontiers:
  (a1) demand kNetAddition → FindOrAddInstance(key) → V-INST-FRESH →
       full Rederive rescan of the input table for that key → TryAdd →
       Touch. (BIRTH of a newly-demanded key.)
  (a2) input(edge) kNetAddition → for each new input row, project its
       instance-key column(s) (edge.From == Start); if
       `FindInstance(key) != kNoInstance` (the key is live-demanded),
       [ADJ-C1 — CORRECTED] **FULL-RESCAN the input keyed on that key,
       exactly as a1 does — NEVER an incremental TryAdd.** (REBUILD of a
       live instance on incremental input.)

[ADJ-C1 — ADJUDICATED UPHELD (HIGH, crit-correctness-1).] The a2 body as
originally written ("TouchCurrent(iid).TryAdd(irow.<row cols>)") is
INCOHERENT with the double-buffer model and ABORTS on this very §7 REBUILD
probe. `TouchCurrent` (InstanceStore.h:130-133) does NOT rescan or reset —
it returns `*current[iid]`, and `current[iid]` was `Reset()` to EMPTY at
the prior epoch's Seal (:202). An incremental TryAdd therefore leaves
`current` holding ONLY this epoch's new rows, not the full per-key
relation; band-(b) then computes born=current∖frozen and Seal's monotone
belt (:185-192, RAT-4 monotone=true) asserts frozen⊆current → the prior
rows are absent → SIGABRT. **CORRECTION: a2 must FULL-RESCAN each touched
live key** (rebuild `current` from the input keyed on that key), identical
to a1's rescan. This collapses a1/a2 into ONE mechanism: the set of
keys-to-rebuild this epoch = (newly-demanded keys, from the demand
frontier) ∪ (live keys whose input changed, from the edge frontier); each
is FindOrAddInstance/FindInstance + full rescan + Touch. §4.2 band-(a2)
and §5f band-(a2) are amended accordingly below. This does NOT change the
DUMP (still one kVecDrain per drained frontier — see ADJ-G1 for the effects
fork); it changes the LOWERING body only.

This makes the mechanism-natural edge frontier the drain source it was
provisioned to be, and is STRICTLY the incremental analogue of flat's
pivots:25 join.6 driver. The census/dump consequences (edge fold gains
kVecAppend, second monotone sweep, kCommitSweep=2, +1 WAW) are exactly
OD-4's enumerated deltas — so the DUMP shape the ds-writer derives is
UNCHANGED by adopting a2; only the LOWERING (band-(a) also drains the
provisioned edge frontier) changes, and it changes toward what the
provisioning already implies.

[ADJ-G1 — ADJUDICATED UPHELD (MED, crit-grammar-1). NARROWING.] The
"DUMP shape UNCHANGED by adopting a2" claim here (and at §9) is TRUE for
CENSUS, DEPS, and .ir/.h, but FALSE for the instantiate op's `.deltarel`
EFFECTS MULTISET. Under a2, band-(a) also drains the edge frontier, and
V-INST-DRAIN (§3.3) requires that input kVecDrain to resolve — i.e.
InstantiateEffects (DeltaRel.cpp:774-778, the SOLE effect authority,
currently one `kVecDrain(demand)`) must push a SECOND
`kVecDrain(%table:11, kNetAddition)` onto op.0, which emit_effects renders
verbatim (Format.cpp:536-548). Census stays 6 (a kVecDrain is an effect,
not an op) and deps stay 6 (the monotone edge frontier is un-minted,
ResolveVecIdx ~0u, no vec hazard). **NARROWED CLAIM: census + deps + ir/h
are a1/a2-invariant; the instantiate EFFECTS multiset gains one kVecDrain
under a2.** The ds §2.3 carries the matching effects fork (ADJ-G1 there).

**Alternative (if the ritual keeps band-(a)=demand-only):** then D2.b ships
a DOCUMENTED feature gap — nested is answer-identical to flat ONLY when all
input rows precede the first demand of each key — and the witness
`.batches`/driver MUST order every add_edge before the demand probe, and
the D2.c REBUILD probe must be a fresh-key demand (a new BIRTH), never an
edge-after-demand. HP-5's out-of-neighborhood-edges requirement is then
satisfied by edges present at BIRTH time. This is the smaller code diff but
it narrows the witness and leaves the incremental-rebuild story to a later
epoch; flag it in CLAUDE.md's feature-gap list if adopted.

[ADJ-C3 — ADJUDICATED UPHELD (MED, crit-correctness-3).] The a1-only branch
is the GENUINELY SILENT miscompile (crit-1's a2-incoherence at least
ABORTS): under R-MONO the demand is if-crossed idempotent (GT :194-198), so
an edge added after a key is demanded NEVER re-touches the instance and is
PERMANENTLY absent from the monotone pub — no belt violation, no diagnostic.
NOTE this DIRECTLY CONTRADICTS the witness's own REBUILD batch design:
demand_neighborhood_witness.dr / witness-deltarel-target.md option-2 makes
the REBUILD batch a "demand flap (edge added, then demand re-asserted)
because stage-(b) rebuild is DEMAND-triggered" — but under R-MONO monotone
demand, the re-assert is a NO-OP, so the demand-flap rebuild does NOT work
and a1-only FAILS the witness's own REBUILD batch ({2,3} vs flat {2,3,4}).
Therefore if a1-only is adopted: (1) the D2.c witness `.batches`/driver MUST
be RESHAPED to birth-only with ALL edges hard-ordered before every demand
(ENFORCED by the harness, not merely recommended); (2) the "demand-flap
rebuild" narrative is retired; (3) CLAUDE.md's feature-gap list MUST NAME
"edge-after-demand under -demand-instance is dropped (R-MONO a1-only)". The
§7 hand probe is the empirical referee for the a1-only-vs-a2 decision.

DECISION OWED at the D2.b ritual head: a1-only (feature gap) vs a1+a2
(answer-complete). This design carries BOTH branches; the pseudocode below
marks the a2 additions `[R-REBUILD-a2]` so they can be included or deferred
as one unit. The hand probe in §7 is the referee.

======================================================================
§1. Main.cpp — the -demand-instance flag (task surface 1)
======================================================================

Files: bin/drlojekyll/Main.cpp, include/drlojekyll/ControlFlow/Program.h
(Build signature), lib/ControlFlow/Build/Build.cpp (Program::Build head).

At tip: `gDemand` (Main.cpp:48) → `Query::Build(module, log, gPassPolicy,
gDemand)` (:68). `Program::Build(*query_opt, log, gFirstId, gPassPolicy)`
(:80). `-demand` handled at :466-468. `Context::demand_instance_enabled`
default-false (Build.h:207) is the sink; NO code sets it true at tip.

DIFF:
  (1a) NEW file-scope bool beside gDemand:
       ```
       static bool gDemandInstance = false;   // Main.cpp:~49
       ```
  (1b) NEW arg parse, AFTER the `-demand` arm (Main.cpp:468). -demand-instance
       IMPLIES -demand (sets both), and is OFF the PassPolicy registry (it is
       a lowering SELECTOR / semantics, the exact P1 `df.demand` precedent —
       d1-pinned N-3 prec:F-5, PassPolicy.cpp:38-43; never a registered pass
       name, never a mode):
       ```
       } else if (!strcmp(argv[i], "-demand-instance") ||
                  !strcmp(argv[i], "--demand-instance")) {
         hyde::gDemand = true;          // implies -demand
         hyde::gDemandInstance = true;
       ```
       -help text line beside the -demand line (Main.cpp:217): "Enable the
       keyed-instance nested lowering for demanded subgraphs (implies
       -demand)."
  (1c) Query::Build call UNCHANGED (:68) — the D1.a recognition registry is
       populated under plain `-demand`; the instance knob does not gate
       recognition (census-recount independence, A.1.5). Program::Build gains
       the bool:
       ```
       Program::Build(*query_opt, error_log, gFirstId, gPassPolicy,
                      gDemandInstance);
       ```
  (1d) Program.h:1360 signature gains a trailing defaulted bool (ABI-additive;
       the default keeps every other caller — tests, tools — untouched, and
       preserves G2 byte-identity flag-off):
       ```
       static std::optional<Program> Build(const Query &query,
           const ErrorLog &log, unsigned first_id, const PassPolicy &policy,
           bool demand_instance = false);
       ```
  (1e) Build.cpp Program::Build (:1164) threads it into the Context created at
       :1275, immediately after `context.demand_forcings = ...` (:1281):
       ```
       context.demand_instance_enabled = demand_instance;
       ```

WHY not thread to Query::Build: RecognizedSubgraphs()/GuardAnnotations() are
populated by ApplyDemandTransform under `-demand` regardless of the instance
knob (Demand.cpp:1076, verified). The knob only selects the Program-side
lowering + census recount (both already gated on
`context.demand_instance_enabled`, DeltaRel.cpp:1355 / :3151). So Query::Build
needs only `demand_mode=true` (implied). This matches OD-R2 (both knobs OFF
the registry) and keeps the 4 golden modes orthogonal (E-62 / N-3).

======================================================================
§2. Build.cpp — chain-breaker excision + OD-4 provisioning + ABA identity
     + the three fences (task surface 2)
======================================================================

----------------------------------------------------------------------
§2.1 The ABA-SAFE recognition primitive (the LOUD §19(K/L/M) carry)
----------------------------------------------------------------------
The D1.b mint (BuildSubgraphInstanceOps, DeltaRel.cpp:894-910) and the
census recount (:3151-3171) BOTH resolve live views via
`view.UniqueId()` against `impl->view_to_model` — a raw pointer identity
that is NOT ABA-safe (a freed QueryViewImpl* can be re-minted as a different
live view; UniqueId collides; a deref reads garbage). The stored
RecognizedSubgraph handles (`rs.pub_view`, `rs.demanded_view`,
Query.h:1019/1021) are RAW pre-Optimize handles; after CSE/dead-flow they
may dangle or have been RAUW'd. This MUST be replaced at D2.b.

**The ABA-safe anchor is the per-view stamp `guard_annotation_index`
(lib/DataFlow/Query.h:479), which TRAVELS THROUGH CSE via the choke-point
transfer (View.cpp:579-590) — the same mechanism that carries group_ids.**
A live guard JOIN carries its stamp; a stale handle does not survive to be
walked. So re-resolution is a walk of the LIVE DefList reading the stamp,
never a lookup of a stored handle (HP-9: ordered consumption is
DefList/det_seq-driven).

[ADJ-C4 — ADJUDICATED UPHELD (LOW, crit-correctness-4). D3 PRECONDITION.]
The stamp's uniqueness across CSE rests on a RELEASE-SILENT invariant: when
two annotated guards fold, CopyDifferentialAndGroupIdsTo keeps the
survivor's index (View.cpp:581), clears the loser's (View.cpp:590), and the
two-DISTINCT-index case is only `assert(...)` (View.cpp:588) — a NO-OP
under NDEBUG. In-slice this is DORMANT: single-adornment neighborhood guards
are structurally distinct and CSE-stable, and ADJ-C2 fences recursive
content OUT (which is the only D2.b shape that raises a forcing to ≥2 guards
and enlarges the fold surface). So NO D2.b action is required. **D3
PRECONDITION (owner-tracked):** before recursive/multi-guard content is
admitted, the View.cpp:585-587 record-comparing incompatible-fold
diagnostic MUST be promoted to an always-on (non-assert) check — otherwise
a release-build two-distinct-guard fold silently drops one forcing's stamp,
ResolveLiveRecognition yields empty guards, the mint skips it (§3.1b), and
the demanded query silently returns nothing.

DIFF (public accessor — the one new DataFlow surface D2.b needs):
  (2.1a) include/drlojekyll/DataFlow/Query.h, on QueryView (public), beside
         the existing per-view predicates:
         ```
         // COMPILER-INTERNAL (keyed instances): the index into
         // Query::GuardAnnotations() this view's guard JOIN was stamped with
         // by ApplyDemandTransform (D1.a), or kNoGuardAnnotation (~0u) if this
         // is not a recognized guard. ABA-safe: the stamp is a per-view field
         // that migrates through CSE with group_ids (View.cpp:579-590), never
         // a raw handle. Ordered consumption must still walk the DefList.
         unsigned GuardAnnotationIndex(void) const noexcept;
         ```
         Query.cpp forwards `impl->guard_annotation_index`. Add
         `static constexpr unsigned kNoGuardAnnotation = ~0u;` on QueryView.

**The re-resolution WALK (shared helper, used by Build.cpp excision AND the
DeltaRel mint AND the census recount — SINGLE authority so all three agree):**
```
// Live, ABA-safe map: forcing_index -> the live guard views of that forcing,
// built by ONE DefList walk (det_seq order). No stored handle is trusted; no
// pointer is dereffed except live views the walk itself yields.
struct LiveRecognition {
  // forcing_index -> live guard JOIN views carrying that forcing's stamps.
  std::unordered_map<unsigned, std::vector<QueryView>> guards;
};
LiveRecognition ResolveLiveRecognition(const Query &query) {
  LiveRecognition out;
  const auto &annots = query.GuardAnnotations();
  query.ForEachView([&](QueryView v) {           // DefList order (HP-9)
    const unsigned ai = v.GuardAnnotationIndex();
    if (ai == QueryView::kNoGuardAnnotation) return;
    assert(ai < annots.size());
    out.guards[annots[ai].forcing_index].push_back(v);
  });
  return out;
}
```
From a forcing's LIVE guard JOINs, the three tables re-resolve WITHOUT any
stale handle:
  - **input_table**: the guard JOIN is `d_reader ⋈ read`. The non-demand
    side (`read`) is the summarized monotone input. Its model table is the
    input. Resolve by: the JOIN's joined view whose SELECT/TUPLE does NOT
    trace to the fabricated demand message (§2.2 message identity) is the
    input side. (In the witness: join.6's .in1 = tuple.2 = edge = %table:11.)
  - **demand_table**: the demand side (`d_reader`) traces to the fabricated
    `forcing.message` (ParsedMessage identity — PARSE-level, Optimize-stable).
    Its model table is %table:8.
  - **pub_table**: the answer INSERT. Resolve by matching the LIVE INSERT
    whose declaration == `forcing.query`'s relation (ParsedDeclaration
    identity, stable). NOT via the stored `rs.pub_view` handle. (In the
    witness: insert.8 → %table:4.)
All three keys are PARSE-level identities (ParsedMessage / ParsedQuery /
ParsedDeclaration) or the ABA-safe per-view stamp — never a raw QueryView
pointer. This is the deref-free scheme the §19(M) carry mandates.

----------------------------------------------------------------------
§2.2 The eager-walk chain-breaker stop (GT-5) + OD-4 provisioning
----------------------------------------------------------------------
At tip the cut-successor test is Build.cpp:958-961 (inside
BuildEagerInsertionRegionsImpl), replicated on the DR side as
AnyCutSuccessorDR (DeltaRel.cpp:130-137) and lifted to table granularity by
MonotoneIngestRoleDR (:147-160). The boundary net-additions append is
Build.cpp:989-994 (fires when `any_cut_succ || is_monotone_negated` on a
monotone table). exp_sweep (DeltaRel.cpp:3033-3041) already counts a
monotone table with a `table_delta_vecs` entry — so provisioning a table
AUTO-INCREMENTS the sweep census with NO separate edit (this is why OD-4's
kCommitSweep=2 falls out mechanically).

**The excision = extend the cut test to treat a RECOGNIZED-SUBGRAPH GUARD
successor as a cut successor, under the knob.** A guard JOIN successor means
the current view is a monotone boundary input of an instance; stop the eager
descent (GT-5: the flat guard-join web / flow:58 is NOT emitted) and
provision its net-additions frontier (OD-4: for BOTH demand and edge, since
BOTH have guard-JOIN successors).

DIFF (Build.cpp:958, symmetric with DeltaRel.cpp:132):
  (2.2a) The cut predicate gains a knob-gated disjunct. `context` is in scope
         at BuildEagerInsertionRegionsImpl (param at :842):
         ```
         const bool is_recog_guard =
             context.demand_instance_enabled &&
             succ_view.GuardAnnotationIndex() != QueryView::kNoGuardAnnotation;
         if (succ_view.CanReceiveDeletions() || succ_view.IsAggregate() ||
             succ_view.IsKVIndex() || is_recog_guard) {
           any_cut_succ = true;
           continue;
         }
         ```
         Comment: "keyed instances (GT-5): a recognized-subgraph guard JOIN
         is fed by its SUBGRAPH_INSTANTIATE op (LowerSubgraphInstance),
         never the eager walk; stop the descent AND provision this monotone
         input's net-additions frontier (OD-4 mechanism-natural — both the
         demand and edge boundary inputs). The flat guard-join web (flow:58)
         is not emitted under -demand-instance."
  (2.2b) DeltaRel.cpp AnyCutSuccessorDR (:130) MUST gain the identical
         disjunct so the §7d role/walk cross-check (the always-on abort at
         the ingest-fold xcheck) does not diverge. It needs the knob; thread
         `Context &` (it is called from MonotoneIngestRoleDR :155, itself
         called with `context` at :3071 and in MakeMonotoneIngestFold):
         ```
         static bool AnyCutSuccessorDR(Context &context, QueryView view) {
           for (QueryView succ : view.Successors()) {
             if (succ.CanReceiveDeletions() || succ.IsAggregate() ||
                 succ.IsKVIndex() ||
                 (context.demand_instance_enabled &&
                  succ.GuardAnnotationIndex() != QueryView::kNoGuardAnnotation))
               return true;
           }
           return false;
         }
         ```
         The §7d cross-check (the coherence law "role and provisioning CANNOT
         diverge", d1-desired X-DS-2) then holds by construction: the walk's
         Build.cpp:989-994 append and the DR's MonotoneIngestRoleDR both read
         the SAME extended cut test. This is the mechanism that makes the
         edge fold's `kVecAppend(%table:11, kNetAddition)` and the second
         monotone sweep appear together (OD-4 census: kCommitSweep=2).

NO change to the boundary-append site (Build.cpp:989-994): it already fires
on `any_cut_succ` for a monotone table, so setting `any_cut_succ` on the
guard boundary provisions the frontier through the existing path.

Trace of the X-DS-2 chain under the knob (verified names at tip): edge
tuple.2 (%table:11) has successor join.6 (guard, stamped) → cut →
`TableDeltaVector(impl, context, %table:11, kNetAdditions)` (Build.cpp:991)
→ `context.table_delta_vecs[%table:11][kNetAdditions]` (:745) → exp_sweep
counts %table:11 (:3037) → a monotone kCommitSweep mints for it (the
:1673-1688 monotone arm). Demand tuple.4 (%table:8) → join.6/join.7 (guards)
→ same → %table:8 sweep. Net: kCommitSweep=2, both ingest folds gain
kVecAppend(kNetAddition), +1 WAW edge (ingest→sweep, per X-DS-2). This is
EXACTLY OD-4's enumerated delta.

----------------------------------------------------------------------
§2.3 The three fences (A.6.5), per-forcing over DemandForcings()
----------------------------------------------------------------------
Placement: the Program::Build pre-pass, beside the existing feature-gap
rejects (Build.cpp:1196-1267, the `num_errors → nullopt` gate at :1268).
Iterate PER FORCING over `query.DemandForcings()` (X-9 recognition unit).
All fence diagnostics are gated on `demand_instance` (the pre-pass has it as
the new Build param); under plain `-demand` NONE fire (the flat lowering
handles all three shapes). Each fence's red witness is all-4-modes-diagnostic
under `-demand -demand-instance` via its `.drflags` (lands with D2.c, OD-8).

  FENCE (i) cyclic-demand (recursive demand relation through the instance
    boundary). Per forcing, test whether the demanded MERGE view is
    self-reachable THROUGH the instance boundary (a ViewSelfReachable variant
    restricted to the demand relation, keyed by the forcing's bound cols —
    NEVER the CSE-folded d_reader handle; use the live-recognition walk).
    [ADJ-C2 — CORRECTED] rejects recursive DEMAND. Diagnostic:
    "Recursive demand relations are not yet supported under
    -demand-instance". Witness demand_cyclic_1.

  [ADJ-C2 — ADJUDICATED UPHELD (HIGH, crit-correctness-2). NEW SUB-FENCE.]
  The original fence-(i) text "Allows recursive CONTENT (bf-tc)" is a
  silent-wrong-answer hole for the D2.b R-MONO slice and MUST be narrowed:
  **fence-(i) also REJECTS recursive CONTENT** (a demanded body whose
  summarized relation is self-reachable / induction-owned, e.g.
  demand_tc_witness's `path :- path, edge`). Two independent defects make
  recursive content unlowerable here: (a) INPUT AMBIGUITY — recursive
  content raises a forcing to ≥2 guards with DIFFERENT non-demand sides
  (base→edge monotone, recursive→path differential); §2.1's input rule
  yields different tables and `lr.guards[forcing]` has NO tie-break for THE
  input_table, so fence-(iii)'s differential-input reject is not reliably
  reached (pick the base guard → input=edge=monotone → fence-(iii) PASSES →
  admitted). (b) SINGLE-SCAN CANNOT CLOSE — band-(a) is one kSectionWalk
  (§3.2c), which yields direct successors only; a per-key transitive closure
  needs a fixpoint. The witness .dr itself states the nested lowering
  "fences it out (its demanded body is recursive, so its summarized input is
  an induction-owned differential table)" — so the DESIGN INTENT already is
  rejection; this makes it explicit rather than resting it on the ambiguous
  input resolution. Diagnostic (labeled feature gap, OD-3 idiom):
  "Demanded subgraphs with recursive (induction-owned) content are not yet
  supported under -demand-instance (a keyed-instance feature gap)." Test the
  demanded MERGE's summarized relation for self-reachability via the
  live-recognition walk (never a stored handle). Witness
  demand_recursive_content_1 (joins the D2.c fence witnesses per OD-8). This
  ALSO moots the recursive-content escalation of ADJ-C4 for the D2.b slice.

  FENCE (ii) mid-stream monotone-input-add under standing demand — **OD-3
    RULING: ships as a clean diagnostic EXPLICITLY LABELED a feature gap.**
    ii-strict compile fence. Diagnostic text MUST name it a feature gap
    (OD-3, §13 CANDIDATE-A honored):
    "Adding to a monotone input of a demanded subgraph after the subgraph is
     demanded is not yet supported (a keyed-instance feature gap); provide
     all input rows before the demand, or use the demand-flap rebuild."
    NOTE: this fence's PRECISE trigger interacts with R-REBUILD (§0). If the
    ritual adopts a1+a2 (band-(a) also drains the input frontier), fence (ii)
    RELAXES to allow mid-stream input-add (a2 handles it) and the witness
    demand_midstream_edge_1 flips from red to a golden — pre-register that
    fork. If a1-only, the fence stands as the OD-3 feature-gap diagnostic.
    Witness demand_midstream_edge_1 (contingent on OD-3 landing per OD-8).

  FENCE (iii) differential-summarized-input (A5): per forcing, if the
    summarized input (the re-resolved input_table's view) CanReceiveDeletions
    → reject. Diagnostic: "Demanded subgraphs over deletable (differential)
    inputs are not yet supported under -demand-instance" (R-MONO scope, D2;
    R-DIFF is D3.a). Witness demand_diff_input_1.

All three stay SEPARATE (only iii is deletion-keyed; collapsing = the C-4
over-broadness, d1-design A.6.5). Fences (i)/(iii) proceed regardless of
OD-3; (ii) is the OD-3-labeled one.

======================================================================
§3. DeltaRel.cpp — un-gate the mint, ABA re-resolution, α-wiring, V-ALPHA,
     V-INST-DRAIN (task surface 3)
======================================================================

The knob flips `context.demand_instance_enabled` true (§1), which un-gates
BOTH BuildSubgraphInstanceOps (called at :1355) and the census recount
(:3151). Both EXIST from D1.b; D2.b AMENDS their input resolution + adds the
α-wiring + V-ALPHA + V-INST-DRAIN.

----------------------------------------------------------------------
§3.1 Replace the ABA-unsafe resolution in BuildSubgraphInstanceOps (:884)
----------------------------------------------------------------------
At tip the mint (:894-910) builds a `live` set from `impl->view_to_model`
keys' UniqueId and pre-filters stale handles, then `model_table(rs.pub_view)`
etc. REPLACE the `live`-set + stored-handle scheme with §2.1's
`ResolveLiveRecognition` + parse-identity re-resolution:
  (3.1a) Drop the `live` set (:894-897) and the `rs.pub_view.UniqueId()`
         pre-filter (:907-909). Build `LiveRecognition lr =
         ResolveLiveRecognition(query)` once at the top.
  (3.1b) For each RecognizedSubgraph `rs`, resolve the three tables from
         `lr.guards[rs.forcing_index]` (the LIVE guards) + parse identities,
         NOT from `rs.demanded_view`/`rs.pub_view` handles:
         - demand_table = model of the guard's demand-side child that traces
           to `query.DemandForcings()[rs.forcing_index].message`.
         - input_table = model of the guard's other joined side (the input).
           REPLACES the current predecessor-walk (:916-928) which starts from
           the stale `rs.demanded_view` handle.
         - pub_table = model of the LIVE INSERT whose Declaration matches the
           forcing's query relation.
         If `lr.guards[rs.forcing_index]` is EMPTY (all guards eliminated by
         dead-flow — a fully-dead forcing), SKIP this rs (no instance minted;
         mirrors the current dangling pre-filter's intent, but ABA-safe). The
         DRInstance descriptor's `demanded_view`/`pub_view` (needed for the
         ik:/row: name render, DeltaRel.h:688-689) are set to the RE-RESOLVED
         live views, never the stale handles.
  (3.1c) `key_cols = rs.key_cols` (position vectors are Optimize-stable —
         they index the published row, not a pointer) — UNCHANGED (:947).
         row_cols derivation UNCHANGED (:948-957).
  (3.1d) The `sid`, table_op_table/sign, effect builders (InstantiateEffects
         :771, DeathEffects :840, SealEffect :865), and the stratum seed
         (:998-1009) are UNCHANGED — they already operate on the resolved
         tables. (InstantiateEffects gains the α representation via
         context_col_sources on the op, §3.2, but its EFFECT set is
         unchanged.)

Same replacement in the census recount (:3151-3171): drop the UniqueId
`live` set (:3152-3155), iterate `ResolveLiveRecognition(query)` /
parse-identity, and `exp_death` keys off the re-resolved demand_table's
`TableIsDifferential` (unchanged predicate, ABA-safe input). Under R-MONO
the witness yields exp_instance=1, exp_death=0, exp_instance seals=1 —
matching P-D2b.3. **The D1.b knob-gated census recount becomes LIVE here**
(it was structurally dead at D1.b); its independence source stays
`query.RecognizedSubgraphs()` (populated by the DataFlow pass, A.1.5 — NOT
the mint's own output, E-27 honesty preserved).

----------------------------------------------------------------------
§3.2 PlanNode bound_col_sources + op context_col_sources + V-ALPHA (A.4)
----------------------------------------------------------------------
At tip PlanNode (DeltaRel.h:356-383) has `bound_cols` (the ACCESS/GATE
column set) but NO binding-source tags; DROp has no `context_cols`/
`context_col_sources`. F4-annot: the fold leaf carries only
fold_table/sign/class, so an insert-projection α-check has nowhere to live.

DIFF:
  (3.2a) NEW enum + field on PlanNode (DeltaRel.h, beside bound_cols):
         ```
         enum class BindingSource : uint8_t { kRowSlot, kInstanceKeySlot,
                                              kConfigSlot };
         // Parallel to bound_cols; defaults all-kRowSlot (all 169 corpus
         // cases unaffected — V-ALPHA short-circuits when every entry is
         // kRowSlot). A kInstanceKeySlot entry means this bound col resolves
         // against the InstanceStore key, not a row-table scan.
         std::vector<BindingSource> bound_col_sources;
         ```
  (3.2b) NEW op-level fields on DROp (DeltaRel.h, in the
         SUBGRAPH_INSTANTIATE payload block :570-580) — the FOLD-side α
         representation the fold leaf lacks (F4-annot):
         ```
         std::vector<unsigned> context_cols;          // published-row positions
         std::vector<BindingSource> context_col_sources;  // parallel tags
         ```
         Populated in BuildSubgraphInstanceOps: for each published column
         position `p`, `context_col_sources[p] = (p ∈ rs.key_cols) ?
         kInstanceKeySlot : kRowSlot`. (No kConfigSlot in the demand slice.)
  (3.2c) The Rederive PlanTree BODY of the instantiate op (the rescan the
         effect leaf `kFlagRead(input, Present)` at InstantiateEffects:785-790
         stands in for) gets a REAL spine at D2.b: a single kAccess node over
         the input table with `bound_cols = {the input column joined to the
         instance key}` and `bound_col_sources = {kInstanceKeySlot}` (the
         rescan is keyed on the instance key = a section walk, Lowering =
         kSectionWalk per X-DS-5/C-9), child = the kFold leaf into pub_table.
         Store this tree on the op (an arm or a dedicated `body` PlanNode);
         codegen (§5) walks it.
  (3.2d) V-ALPHA (single, always-on, in the validator block beside
         V-INST-EFFECT at DeltaRel.cpp:3307). Two arms:
         - arm A (ACCESS/GATE): every PlanNode across the flow with a
           bound_col_sources[k]==kInstanceKeySlot MUST have
           lowering==kPointTest OR kSectionWalk against the instance store
           (never kFullScan / a row-table gate). Covers join keys, functor
           bound args (D3+), negate gates (D3+).
         - arm B (FOLD/publish) [ADJ-P1 — CORRECTED: column-total + negative
           check added]: for every kSubgraphInstantiate op, arm B MUST:
             (B-i) COLUMN-TOTAL — iterate EVERY published position (ik: AND
               row:), asserting each has a modeled `context_col_sources`
               entry (no unmodeled column). NOT a key_cols-only loop.
             (B-ii) POSITIVE — every position in `key_cols` has
               context_col_sources[pos]==kInstanceKeySlot (published from
               KeyAt(iid), never row-projected).
             (B-iii) NEGATIVE — every row_cols position's source is NOT
               kInstanceKeySlot AND does not transitively trace to an
               instance-key value (the row payload is genuinely row-sourced,
               never an aliased key). This is the HP-4-pinned half the
               original spec OMITTED.
         Store cross-pin: `key_cols == rs.key_cols == the annotation's
         instance_key positions`; a mismatch = V-ALPHA abort.

[ADJ-P1 — ADJUDICATED UPHELD (HIGH, crit-pins-1). HP-4 honored.] HP-4
(d1-pinned.md:131-135, corr:F-C3) pins arm B as "pub_row-COLUMN-total:
every published column (ik: AND row:) has a modeled BindingSource, AND no
row: column's source transitively traces to an instance-key value." The
original §3.2d arm B iterated ONLY key_cols and asserted only the positive
key-slot half — shipping the pinned belt half-built. Today §3.2b's
construction `context_col_sources[p]=(p∈key_cols)?keyslot:rowslot` makes
the negative tautological (arm B never fires on the witness — the exact
silent-no-op HP-4/HP-12 exist to close). The pin's stated purpose is a belt
for a D3+ body relaxation / real provenance tagging: once a row column can
trace to the instance key it gets tagged kInstanceKeySlot, and a
key_cols-only arm B never iterates it → silent over-materialization with no
abort. B-i/B-ii/B-iii above close it. The §6 HP-12 SITE-3 acceptance line
gains the negative (ADJ-P1 there).
  (3.2e) HP-4 RECOGNIZER REFUSAL (couples the guard to the guarantee): if the
         demanded body contains ANY MAP/NEGATE/AGG view, RecognizedSubgraph
         mint (Demand.cpp side) / the D2.b mint MUST abort until V-ALPHA is
         extended. At tip the demand body-walk already rejects MAP/NEGATE/AGG
         (Demand.cpp:584-607, HP-4 evidence), so the refusal is a BELT: add an
         assert in BuildSubgraphInstanceOps that the re-resolved input side is
         a plain table-bearing TUPLE/JOIN/SELECT-from-IO (not a MAP/NEGATE/AGG
         model), fprintf+abort otherwise — so a D3 body relaxation that
         reopens the α-through-functor-output path trips LOUD before V-ALPHA
         is silently bypassed.

----------------------------------------------------------------------
§3.3 V-INST-DRAIN (HP-2) — the demand-frontier provisioning cross-check
----------------------------------------------------------------------
At tip the §7d ingest-fold cross-check filters `op.kind != kIngestFold`
(DeltaRel.cpp:3043-3044 region), so an instantiate op's
`kVecDrain(demand, kNetAddition)` is UNCHECKED; ResolveVecIdx returns ~0u
gracefully for an un-minted (table, role) (:3110-3127) and add_vec_access
skips it — so a MISSING demand frontier births ZERO keys silently with an
empty pub table as the "answer" (HP-2 evidence).

DIFF: NEW always-on validator V-INST-DRAIN (beside V-INST-ORDER at
CheckInstanceOrder / the validator block :3648). For every
kSubgraphInstantiate op, assert its `kVecDrain(demand_table, kNetAddition)`
effect resolves to a PROVISIONED `context.table_delta_vecs[demand_table]`
entry — i.e. Build.cpp's cut test (§2.2) actually provisioned it. Under
R-REBUILD-a2, ALSO assert the input(edge) kVecDrain resolves.
```
for (const DROp &op : flow.ops) {
  if (op.kind != DROpKind::kSubgraphInstantiate) continue;
  const bool ok =
      context.table_delta_vecs.count(op.demand_table) &&
      context.table_delta_vecs.at(op.demand_table)
          [unsigned(VectorKind::kNetAdditions)] != nullptr;
  if (!ok) ValidatorFail("V-INST-DRAIN: an instantiate's demand net-additions "
                         "frontier was never provisioned (OD-7/§2.2 gap)");
}
```
This reframes A.2.3's "always" drain as a compile-asserted PRECONDITION
(HP-2), catching an un-provisioned OD-7 on EVERY compile, not once on a
witness. It needs `context.table_delta_vecs` in the validator's scope — it
is already threaded to the census recount (:3037), so it is available.

======================================================================
§4. Stratum.cpp — LowerSubgraphInstance + self-lowered seal + V-INST-EMITTED
     (task surface 4)
======================================================================

Mold: the GROUP_UPDATE dispatch (LowerDRFlow :1594-1603) + LowerGroupUpdate
(:1366-1426). Instance ops sit at their `instance_stratum[sid]` (seeded at
BuildSubgraphInstanceOps:1008); dispatch them in LowerDRFlow's acyclic band
at that stratum, right after the GroupUpdates block (:1603), before the
acyclic claim drains (:1605).

----------------------------------------------------------------------
§4.1 The dispatch loop (LowerDRFlow, after :1603)
----------------------------------------------------------------------
Add a `dr_flow.SubgraphInstances()` accessor (mirror `GroupUpdates()`,
filtering kind==kSubgraphInstantiate) and the dispatch:
```
for (const DROp *op : dr_flow.SubgraphInstances()) {
  auto is = dr_flow.instance_stratum.find(op->instance_store_id);
  if (is == dr_flow.instance_stratum.end() || is->second != stratum) continue;
  LowerSubgraphInstance(impl, context, dr_flow, *op, seed_vector, stratum_seq);
}
```
The seal is NOT dispatched here — it self-lowers from LowerSubgraphInstance
(HP-1/OD-5, §4.3). The death op is not minted under R-MONO (HP-17); its
LowerSubgraphInstance arm compiles inert.

----------------------------------------------------------------------
§4.2 LowerSubgraphInstance (the band-(a)/band-(b) body, GROUP_UPDATE mold)
----------------------------------------------------------------------
Emits a NEW region ProgramSubgraphInstanceRegion (mirror GROUPUPDATE, a
CreateDerived region carrying: the instance_store_id, the demand
net-additions frontier VECTOR*, [R-REBUILD-a2: the input net-additions
frontier VECTOR*], the pub table, the input table, the instance-key column
positions, the row column positions, the Rederive PlanTree). Codegen (§5)
walks it. Pseudocode of the emitted structure (the codegen writes the C++;
this is the REGION shape):

```
static void LowerSubgraphInstance(impl, context, dr_flow, op, seed_vector, seq){
  assert(op.kind == kSubgraphInstantiate);
  TABLE *pub   = op.table_op_table;   // HP-3 reuse
  TABLE *demand= op.demand_table;
  TABLE *input = op.input_table;

  // band-(a) frontier(s): the demand net-additions (BIRTH keys); provisioned
  // by §2.2. seed_vector shares/sort-uniques exactly like a product arm.
  VECTOR *demand_front = seed_vector(demand, VectorKind::kNetAdditions);
  // [R-REBUILD-a2] the input net-additions (REBUILD on incremental input):
  VECTOR *input_front  = seed_vector(input,  VectorKind::kNetAdditions);

  // the pub table's del/add queues — ONLY under DIFF (R-MONO: no queues,
  // A.2.3 !DIFF arm). Under R-MONO the region publishes via a monotone
  // UPDATECOUNT (+nonrecursive) into pub, exactly as flat's insert.8 does.
  SUBGRAPHINSTANCE *si = impl->...CreateDerived<SUBGRAPHINSTANCE>(seq,
        op.instance_store_id, TableIsDifferential(pub));
  seq->AddRegion(si);
  si->demand_frontier.Emplace(si, demand_front);
  si->input_frontier.Emplace(si, input_front);   // [R-REBUILD-a2]
  si->pub_table.Emplace(si, pub);
  si->input_table.Emplace(si, input);
  si->key_positions  = op.key_cols;              // ik: positions (α)
  si->row_positions  = op.row_cols;              // row: positions
  si->rederive_plan  = op.body PlanTree (§3.2c); // keyed section-walk
  // HP-1/OD-5: the SEAL self-lowers here — attach the instance-seal as the
  // region's trailing swap, NOT via EmitCommitSweep. Codegen §5 emits
  // instance_<id>.Seal() at the region tail (band 11 placement is the
  // region's own tail, dominated by band-(b) publish).
  si->seal_store_id = op.instance_store_id;
}
```
Emitted band semantics (codegen §5 realizes these; HP-6 partition):
  band-(a1) BIRTH: `for key in drain(demand_front): iid =
    store.FindOrAddInstance(Key{key}); V-INST-FRESH(iid); Table &cur =
    store.TouchCurrent(iid); <rederive_plan: section-walk input keyed on
    key, cur.TryAdd(row_cols)>`.
  band-(a2) [R-REBUILD-a2] REBUILD [ADJ-C1 — CORRECTED to full-rescan, NOT
    incremental]: `for irow in drain(input_front): key = irow.<key col>;
    iid = store.FindInstance(Key{key}); if iid != kNoInstance AND
    !store.TouchedFlag(iid): store.TouchCurrent(iid) (empty from prior
    Seal); <rederive_plan: full section-walk of input keyed on key,
    cur.TryAdd(row_cols)>`. The rescan (not the incremental irow)
    repopulates `current`; the `!TouchedFlag` gate dedups a key with
    multiple new input rows to ONE rescan and skips a key already rebuilt by
    band-(a1). This IS a1's body — see ADJ-C1 §0. An incremental
    `TouchCurrent(iid).TryAdd(irow)` would leave `current` partial (Reset to
    empty at prior Seal, InstanceStore.h:202) and trip the monotone belt.
  band-(b) PUBLISH (HP-6 two-scan, InstanceStore A.3.5): `for iid in
    store.Touched(): scan Current(iid), Find in Frozen(iid) → born {(F,T)} →
    publish +; [R-DIFF only] scan Frozen(iid), Find in Current(iid) →
    dropped {(T,F)} → publish −.` Under R-MONO the (T,F) set is PROVABLY
    EMPTY (HP-7; the store's Seal belt asserts frozen⊆current), so band-(b)
    is the (F,T) publish only; the R-DIFF retract scan compiles inert.
  Publish target: `+` → the pub table via UPDATECOUNT +nonrecursive (R-MONO,
    the InstantiateEffects !DIFF counter) or the add-queue append (R-DIFF).
    pub_row = concat(store.KeyAt(iid) at key_positions, r at row_positions).

----------------------------------------------------------------------
§4.3 The seal self-lowered (HP-1 / OD-5) + V-INST-EMITTED
----------------------------------------------------------------------
OD-5 / HP-1: the seal's carrier is the instance dispatch, NOT EmitCommitSweep.
d1-desired C-3 resolution (iii). Two realizations, pick at the ritual:
  (i) LowerSubgraphInstance appends an INSTANCESEAL region (its own
      CreateDerived) at the STRATUM TAIL after band-(b) — cleanest, matches
      "self-lowered from its own dispatch" (DeltaRel.h:159-161). The kStateFold
      sign-0 SealEffect (:865) is its payload; codegen emits
      `instance_<id>.Seal();`.
  (ii) attach the seal as the SUBGRAPHINSTANCE region's own trailing
      sub-region (one region, seal at its tail).
RECOMMEND (i) — a distinct region keeps V-INST-EMITTED's three-kind multiset
honest (a minted-but-unlowered seal ABORTS). The seal region is emitted for
EVERY instance store id that has an instantiate (1:1, V-INST-PAIR).

**V-INST-EMITTED (HP-1, always-on, the V-INGEST-XCHECK Site-5 mold, A.5c).**
The eager walk runs BEFORE the flow is built (like ingest folds), so record
each emitted instance region's (store_id, kind) at emission into a
`context.emitted_instance_ops` multiset (mirror EmittedIngestFold,
Build.h:217), and a closing check in BuildStratumPhases multiset-compares it
against the flow's {kSubgraphInstantiate, kInstanceDeath, kInstanceSeal}
enrollment. **Enrolls ALL THREE kinds** (HP-1: the seal is IN the multiset —
a minted-but-unlowered seal aborts, closing the F17/F18-class stale-frozen
hole). Under R-MONO: 1 instantiate + 0 death + 1 seal emitted == enrolled.

======================================================================
§5. Database.cpp — instance_<id> codegen against the REAL InstanceStore.h
     (task surface 5)
======================================================================

The InstanceStore.h surface EXISTS at tip (D2.a, 677977f8); codegen MUST
emit against its ACTUAL member names. Verified surface:
  ctor `InstanceStore(Allocator, bool monotone_ = true)` (:66) — **RAT-4
    monotone ctor bool**: D2 (R-MONO) constructs default (true); the belt
    fires. FindInstance (:99) / FindOrAddInstance (:105) / TouchCurrent (:130,
    returns Table&) / Current (:138) / Frozen (:139, const) / Touched (:144,
    sort-unique) / KeyAt (:153) / WorkingOccupied (:161) / SealedOccupied
    (:167) / Seal (:174) / RecycleCurrent (:216) / DebugValidate (:222).
    kNoInstance = ~0u (:52). Nested Table is index-free (:18 — Find/TryAdd by
    whole row).

Mold: the StateCellStore member/ctor/ref-param emission (Database.cpp
member decl :1496-1504; ctor init :1394-1397; ref-param
:872-880/:917-921; StateCellStoreType :882-887; the `statecells` set on
ProgramProcedure :256, populated :745/:751).

DIFF (parallel of the StateCell path — a peer, NOT a reuse):
  (5a) A `ProgramInstanceStoreInfo` (mirror ProgramStateCellInfo) exposing
       Id(), Key/Row types, forcing. `program.InstanceStores()` accessor.
  (5b) The Key_<id> / Row_<id> type structs (mirror EmitStateCellStructs
       :1077): Key_<id> = the α (key_cols) column types; Row_<id> = the row
       (row_cols) column types. Both plain-value structs with `.Hash()` and
       `operator==` (InstanceStore requires Key.Hash()/== :100-101, :250; the
       nested Table<Row_<id>> requires Row equality for Find). NO functor ABI
       change — closed (leading plain values, A.4).
  (5c) Member decl (mirror :1496-1504): per instance store,
       ```
       ::hyde::rt::InstanceStore<Key_<id>, Row_<id>> instance_<id>;
       ```
  (5d) Ctor init (mirror :1394-1397) — **RAT-4**: R-MONO passes the default
       true (belt on); DO NOT pass false at D2.b:
       ```
       , instance_<id>(allocator_)     // monotone=true (R-MONO belt, RAT-4)
       ```
       (D3.a's R-DIFF lowering passes `allocator_, false`; not this diff.)
  (5e) Ref-param threading (mirror :872-880 statecells): the
       ProgramProcedure gains an `instance_stores` set, populated at the
       region-collection sites (mirror :745/:751 for GROUPUPDATE) when a
       SUBGRAPHINSTANCE / INSTANCESEAL region is seen; forwarded by name
       (mirror :917-921).
  (5f) EmitSubgraphInstance (new, dispatched in EmitOperation beside
       :1798-1799 `IsGroupUpdate`): writes the band-(a)/(b) body from §4.2's
       region against `instance_<id>`:
       - band-(a1): `for (const auto &[k..] : <demand_front vec>) { const auto
         iid = instance_<id>.FindOrAddInstance(Key_<id>{k..});` then the
         **V-INST-FRESH inline guard** (5g), then `auto &cur =
         instance_<id>.TouchCurrent(iid);` then the section-walk rescan of the
         input table keyed on the key, `cur.TryAdd(Row_<id>{row..});` `}`.
       - band-(a2) [R-REBUILD-a2] [ADJ-C1 — full-rescan, NOT incremental]:
         `for (const auto &[e..] : <input_front vec>) { const auto iid =
         instance_<id>.FindInstance(Key_<id>{e.<keycol>}); if (iid !=
         ::hyde::rt::kNoInstance && !instance_<id>.TouchedFlag(iid)) {
         auto &cur = instance_<id>.TouchCurrent(iid); <SAME section-walk
         rescan of the input keyed on the key as band-(a1), cur.TryAdd(...)>;
         } }`. The `!TouchedFlag` gate makes the rescan run at most once per
         key per epoch; the incremental `TouchCurrent(iid).TryAdd(e)` form is
         WRONG (partial `current`, monotone belt SIGABRT — ADJ-C1 §0).
       - band-(b): `for (const auto iid : instance_<id>.Touched()) { auto &cur
         = instance_<id>.Current(iid); const auto &frz =
         instance_<id>.Frozen(iid); for (uint32_t r=0; r<cur.NumRows(); ++r) {
         const auto &row = cur.RowAt(r); if (frz.Find(row) ==
         ::hyde::rt::kNoRow) { <publish +: UPDATECOUNT +nonrecursive of
         pub_row=(KeyAt(iid).c<k>.., row.<field>..) into pub table> } } }`
         (R-MONO — (F,T) only; the (T,F) frozen-scan is R-DIFF, emitted inert).
         KeyAt: `const auto &key = instance_<id>.KeyAt(iid);` → the ik: slots
         are `key.c<k>` (context_col_sources==kInstanceKeySlot, arm B); the
         row: slots are `row.<field>` (kRowSlot).
  (5g) **V-INST-FRESH inline guard** (A.3.3; fprintf+abort, survives NDEBUG —
       codegen-emitted, NOT #ifndef NDEBUG'd, it is the runtime death test):
       at band-(a1) entry per FIND-OR-ADD (before either arm), assert the
       freshly-found/minted current buffer is empty:
       ```
       if (instance_<id>.WorkingOccupied(iid)) {
         std::fprintf(stderr, "V-INST-FRESH: instance %u current non-empty at "
                      "band-(a) entry (id=<id>)\n", iid); std::abort();
       }
       ```
       Matched pair with the unconditional minus-arm RecycleCurrent (D3.a).
       Under R-MONO a fresh mint is always empty (FindOrAddInstance mints
       empty, :118-120); a same-epoch re-touch of an ALREADY-touched iid must
       NOT re-guard — so emit the guard only on the FIRST touch of an iid this
       epoch (gate on `!instance_<id>.TouchedFlag(iid)` BEFORE TouchCurrent,
       since TouchCurrent sets the flag). Note: TouchCurrent(:130) calls Touch
       which sets the flag; read TouchedFlag(:149) first.
  (5h) The seal (HP-1): EmitInstanceSeal (dispatched beside the others) emits
       `instance_<id>.Seal();` + `#ifndef NDEBUG instance_<id>.DebugValidate();
       #endif`. This is the self-lowered carrier — NOT hung off
       EmitCommitSweep (contrast the StateCell path :2326-2371). The monotone
       pub table %table:4's OWN commit sweep (if any) does NOT seal the
       instance.
  (5i) [ADJ-P2 — HP-6 partition guardian, crit-pins-2] band-(b)'s publish
       MUST be (F,T)-gated: `if (frz.Find(row) == kNoRow) publish+`. Post the
       HP-11 collapse, kStateEmit/kStateOld render as GENERIC reads in the
       hazard switch (DeltaRel.cpp:3791/:3795) carrying no partition
       semantics, so NO .deltarel dump byte and NO DR-IR validator can force
       the partition — a publish-ALL-current codegen edit stays green on
       every validator and byte-identical in every dump, caught only by a
       present-and-rerun HP-5 probe. Because the partition is a property of
       emitted C++, its guardian is codegen-level: (1) emit an always-on
       (survives NDEBUG) runtime assert at band-(b) that a published row is
       ABSENT from frozen — the dual of V-INST-FRESH; and (2) the HP-12
       SITE-3 acceptance line (§6) is amended to REVIEW that band-(b) is
       frozen-gated against the real generated code. See ADJ-P2 in the
       ledger — whether the runtime assert (1) is owed at D2.b or deferred is
       an OWNER DECISION; the §6 review line (2) is mandatory regardless.

NESTED TABLES INDEX-FREE (InstanceStore.h:18) × membership predicates: the
nested Table<Row_<id>> has NO index — `Find`/`TryAdd` are whole-row
(:189, the belt's `c.Find(f.RowAt(r))`). So band-(b)'s born-set test is a
whole-row `Frozen.Find(row) == kNoRow`, NOT an index probe. The flat guarded
copy %table:15 + idx_38 (a keyed hash index) DISAPPEAR (the storage win,
A.3.1) — the answer's membership is the instance's frozen⊆current relation,
read by whole-row Find. Generated membership predicates on the PUB table
(%table:4) are UNCHANGED (its counters/indices persist; only its DERIVER
changes from the flat join web to the instance publish).

======================================================================
§6. HP-12 — the α-consumer enumeration (no-uncovered-α-consumer line)
======================================================================

HP-12: D2.b must ENUMERATE every α-consumer site on the neighborhood witness
and show each is kInstanceKeySlot-tagged and reached by V-ALPHA arm A or B.
A.4's default-all-kRowSlot short-circuit means a MISSED tag is the
validator's own silent no-op — so the enumeration is the acceptance line.

The neighborhood witness (`neighborhood(bound Start, free Node) :
edge(Start, Node)`) α = {Start}. Every site consuming the α value Start:

  SITE 1 — the band-(a1) FindOrAddInstance key. The demand-frontier drain
    yields the demanded Start; it forms `Key_<id>{Start}`. COVERAGE: this is
    the instance KEY itself (the store's own key column) — tagged
    kInstanceKeySlot by construction (§3.2b: position 0 ∈ rs.key_cols). No
    row-table scan. (No V-ALPHA ACCESS node — it is the drain, not a scan;
    the store cross-pin key_cols==rs.key_cols==instance_key guards it,
    §3.2d.)
  SITE 2 — the Rederive rescan's INPUT access (the section-walk over edge
    keyed on Start, §3.2c). COVERAGE: V-ALPHA arm A — the input ACCESS
    PlanNode has bound_cols={edge.From}, bound_col_sources={kInstanceKeySlot},
    lowering=kSectionWalk (against the input keyed by the instance key). arm A
    asserts kSectionWalk/kPointTest, PASSES.
  SITE 3 — the band-(b) PUBLISH of the α column into pub_row. COVERAGE:
    V-ALPHA arm B — position 0 (Start) of the instantiate's key_cols has
    context_col_sources[0]==kInstanceKeySlot, so the published Start is
    `KeyAt(iid).c0`, NEVER row-projected from the rescan. arm B asserts
    exactly this (B-ii positive). [ADJ-P1] SITE-3 ALSO asserts the NEGATIVE
    (B-iii): the row column `Node` (position 1) is `kRowSlot`, NOT
    instance-key-sourced — published as `row.<field>`, never `KeyAt(iid)`.
    [ADJ-P2] SITE-3 ALSO REVIEWS that the band-(b) publish is FROZEN-GATED
    (`if (frz.Find(row)==kNoRow)`) against the real generated C++ — the HP-6
    (F,T)-partition guardian (§5i), since no dump byte forces it post the
    HP-11 collapse. PASSES.

There is NO fourth α-consumer on this witness (no functor bound arg, no
negate gate — HP-4's body-walk rejects them, and the witness body is a bare
edge read). ACCEPTANCE LINE (P-D2b.3): "3 α-consumer sites on
demand_neighborhood_witness; each kInstanceKeySlot-tagged; SITE-2 covered by
V-ALPHA arm A (section-walk), SITE-3 by arm B (KeyAt publish + [ADJ-P1]
`Node` proven kRowSlot NOT key-sourced + [ADJ-P2] band-(b) frozen-gated),
SITE-1 is the store key itself (cross-pin); ZERO uncovered α-consumers —
verified against the -deltarel binding-source tags (ik:Start on the
instantiate header line, §3.2/C-12)." A regression that drops a tag flips a
site to kRowSlot →
V-ALPHA short-circuits it silently → the enumeration (not the validator)
catches it; hence the line is a REVIEW gate, checked against the real
-deltarel dump before bless (HP-13(b)).

======================================================================
§7. PREDICTIONS P-D2b.1..4 (restated + refined vs REAL tip) + acceptance
======================================================================

P-D2b.1 (knob-OFF, i.e. no -demand-instance anywhere): [G2] 676-row corpus +
  data/ byte-identical vs frozen 99f211f5; SUITE PASS (169, +3 fence
  witnesses IF they land at D2.c — they land at D2.c per OD-8, so at D2.b the
  count is 169 and the fences are probe-only, §8); [G3] zero-churn (D1.b's
  census line already blessed; the new Format rows render only under the knob
  / only on an instance-bearing flow). demand_instance defaults false through
  Program::Build (§1d) so EVERY non-witness path is untouched.

P-D2b.2 (knob-ON, neighborhood witness, PROBE — the .ir/.h shape deltas vs
  GT Appendix C flat .ir). Derived exactly from the flat .ir (d1-ground-truth
  §Appendix C) minus the excised web:
  VANISH from the nested .ir/.h:
    - `create %table:15[u64,u64]` + its `%index:18` + `%index:38` (the flat
      guarded copy tuple.3 and its keyed hash index — A.3.1 storage win; the
      instance's nested Table is index-free).
    - `^flow:58(...)` entirely (the two join-tables blocks, :223-246) — the
      SUBGRAPH_INSTANTIATE region replaces it (GT-5). The `@20 += 1`
      epoch-counter, the `vector-unique $pivots` + `join-tables` + the two
      `select ... using %index` scans + the two `update-count in
      %table:15/%table:4` folds — all gone.
    - `$pivots:25` / `$pivots:29` vector-defines and their entry:19
      appends/clears (the flat seed vectors), REPLACED by the demand (and
      [a2] input) net-additions frontier vecs + the SUBGRAPH_INSTANTIATE
      body.
  APPEAR in the nested .h/.ir:
    - `::hyde::rt::InstanceStore<Key_<id>, Row_<id>> instance_<id>;` member +
      its ctor init (allocator_, monotone default) + the Key_<id>/Row_<id>
      structs.
    - the SUBGRAPHINSTANCE region body (band-(a) drain(s) + FindOrAddInstance
      + V-INST-FRESH + section-walk rescan + TryAdd + band-(b) two-scan
      publish into %table:4) + the INSTANCESEAL region (`instance_<id>.Seal()`)
      — emitted where flow:58 lived.
    - under OD-4: %table:8 AND %table:11 each gain a net-additions frontier
      vec + a monotone commit sweep (the .ir sweep tail; census kCommitSweep=2
      in the .deltarel — the ds-writer's block).
  UNCHANGED (ratified, P-2/P-5): %table:4 (pub) + its %index:7 + `%index:57`;
    %table:11 (edge) + its %index:14 + `%index:32`; %table:8 (demand) +
    `%index:10`. The pub/edge/demand tables and their indices PERSIST — only
    %table:15 and flow:58 are excised. (Refinement vs the pinned P-D2b.2 which
    named "%table:15 + idx_38 VANISH, instance_<id> appears where flow_58
    lived, idx_57 + edge idx_32 unchanged" — CONFIRMED exact against the real
    Appendix-C ids.)

P-D2b.3 (nested .deltarel census + validators): kSubgraphInstantiate=1,
  kInstanceDeath=0 (R-MONO — demand monotone, HP-17), kInstanceSeal=1;
  kIngestFold=2, kCommitSweep=2 (OD-4), kGroupUpdate=0. V-INST-EFFECT/SOLE/
  PAIR/ORDER + V-INST-DRAIN + V-ALPHA all green; V-INST-EMITTED proves the
  three-kind consumption (1 inst + 0 death + 1 seal emitted == enrolled).
  The exact .deltarel BYTES are the ds-writer's re-derivation of §B.4 (STALE
  two ways: OD-4 banner + the HP-11 collapse to kStateEmit/kStateOld/
  kStateFold; and the collapsed seal ENROLLS a kStateFold WAW write hazard so
  the "no seal edge" claim in §B.4 is WRONG — the seal's kStateFold(sign=0)
  write on pub_table WAW-edges against the instantiate's kInstanceRebuild
  write, DeltaRel.cpp:3785-3800). NOT authored here (this is the CODE half).

P-D2b.4 (ACCEPTANCE CRITERION, not a prediction): nested driver stdout ==
  flat driver stdout on the BIRTH and REBUILD probes of
  demand_neighborhood_witness. **This is where R-REBUILD (§0) is decided.**
  The D2.b hand probe (below) is the referee; D2.c consumes the result.

**THE D2.b HAND PROBE (task item 7, MANDATORY before any nested bless):**
  1. Compile demand_neighborhood_witness.dr BOTH ways:
       DR witness.dr -demand            -cpp-out flat/
       DR witness.dr -demand-instance   -cpp-out nested/   (implies -demand)
  2. Also dump both: -df-out (must be BYTE-IDENTICAL — Alt-A, C-1: the knob is
     emission-only, FillDataModel knob-blind, X-DS-4); -deltarel-out and
     -ir-out (the P-D2b.2 shape deltas above); review the nested .deltarel
     end-to-end against this contract + the ds-writer's §B.4 (HP-13(b) duty).
  3. Hand-link a driver (demand_tc_witness mold; sorted keyed drains) against
     each generated datalog.cpp + runtime, and RUN the SAME batch script:
       (BIRTH)   add_edge(1,2) add_edge(1,3) add_edge(9,9); query
                 neighborhood_bf(1) → expect {2,3} (NOT 9 — out-of-neighborhood,
                 HP-5).
       (REBUILD) add_edge(1,4); query neighborhood_bf(1) again → flat yields
                 {2,3,4}. **If nested yields {2,3} (band-(a)=demand-only, the
                 R-REBUILD gap) → the a1-only branch is confirmed insufficient
                 → adopt a1+a2 OR narrow the witness per §0.** If nested yields
                 {2,3,4} (a1+a2) → answer-identity holds.
     ASSERT byte-equal stdout flat==nested on BOTH probes. This RUN is the
     acceptance referee; a divergence is the R-REBUILD decision made empirical.

======================================================================
§8. GATES (task surface 8)
======================================================================

Per-diff standing vocabulary (d1-design §B): [G1] SUITE PASS; [G2] 676-row +
data A/B byte-identical vs frozen 99f211f5 knob-OFF; [G3] irgold byte-compare
both carriers; [G4] Q5 ABABAB bench-neutral; [G5] 3-run determinism +
debug==release on all four surfaces of every demand-ON carrier; [G6] E-62
tripwire re-grep on any DeltaRel-touching diff; [G7] data/ clean + ctest.

D2.b checklist rows:
  G1  SUITE PASS (169; fences are probe-only at D2.b — they land in the suite
      at D2.c per OD-8, so the count does not move here).
  G2  676-row corpus + data/ BYTE-IDENTICAL vs 99f211f5 with NO
      -demand-instance anywhere (demand_instance defaults false, §1d). The
      Program::Build signature change is ABI-additive (defaulted arg) so no
      caller shifts. E-62 orthogonality: the 4 golden modes are untouched by
      the knob (it is off the PassPolicy registry, §1b).
  G3  irgold both carriers (demand_tc_witness 4 surfaces, symrec_tie_1 2)
      zero-churn — the D1.b census line is already blessed; the new Format
      instance rows render only on an instance-bearing flow (none in the
      corpus at D2.b).
  G4  Q5 ABABAB progsize@128 release, median within noise.
  G5  **config-invariance on the NESTED witness FOUR surfaces
      (df/deltarel/ir/h) — 3-run 1-hash AND debug==release — BEFORE ANY
      NESTED BLESS (HP-13(b)).** The .df arm is trivially inherited (Alt-A:
      nested .df == flat .df, C-1). The deltarel/ir/h arms are the new
      surfaces; audit determinism on them explicitly.
  G6  E-62 tripwire re-grep MANDATORY (DeltaRel-touching: the mint
      re-resolution, PlanNode/DROp fields, V-ALPHA, V-INST-DRAIN, the
      SubgraphInstances accessor, Stratum dispatch): no new range-for/sort
      over guard_annotations / body_ops / output_ops / pinned_order at any
      emission-or-dump site (HP-9). The re-resolution walk is DefList-driven
      (§2.1), the one sanctioned pinned_order reader stays the validators.
  G7  data/ examples compile clean (all 4 modes); ctest 5/5 (D2.a added the
      5th target) debug AND under ASAN.
  E-62 MANDATORY: the -demand-instance knob is OFF the PassPolicy registry
      (OD-R2); re-assert the 4-mode orthogonality (a -demand-instance run is
      NOT a 5th golden mode; the 166 pre-instance cases are byte-identical
      knob-off).
  ASAN BOTH SURFACES (the standing gate, MEMORY asan-standing-gate): build/asan
      configure+build, ctest under it, AND the OptDiff suite under DR=asan —
      zero reports on both. The ABA re-resolution (§2.1/§3.1) is precisely the
      class ASAN guards; the D2.b probe MUST run the nested witness under ASAN
      (a stale-handle deref would surface here — the §19(M) carry's teeth).
  Q5 bench-neutral (knob-off path unchanged; the instance path is not in the
      timed corpus at D2.b).
  FENCES fire on their (uncommitted, probe-only at D2.b) witness programs in
      ALL 4 MODES under `-demand -demand-instance`: hand-compile
      demand_cyclic_1 / demand_midstream_edge_1 / demand_diff_input_1 and
      confirm each emits its diagnostic (fence-ii's text EXPLICITLY says
      "feature gap", OD-3) in opt/nodf/nocf/none. They enter the suite
      alternation (runall.sh:296) + CLAUDE.md at D2.c (OD-8), not here.

BLESS DISCIPLINE: no nested golden is blessed at D2.b (the witness enters the
suite at D2.c). D2.b's job is the CODE + the hand probe + the four-surface
config-invariance audit + the end-to-end nested .deltarel review (HP-13(b)) —
so that when D2.c blesses, the review already happened on green, real dumps.

======================================================================
§9. SUMMARY OF THE FILE-BY-FILE DIFF (the D2.b landing checklist)
======================================================================
- bin/drlojekyll/Main.cpp: gDemandInstance; -demand-instance arm (implies
  -demand); pass to Program::Build. (§1)
- include/drlojekyll/ControlFlow/Program.h: Build() gains `bool
  demand_instance = false`. (§1d)
- include/drlojekyll/DataFlow/Query.h + lib/DataFlow/Query.cpp: public
  QueryView::GuardAnnotationIndex() + kNoGuardAnnotation. (§2.1a)
- lib/ControlFlow/Build/Build.cpp: context.demand_instance_enabled =
  demand_instance (§1e); cut-test disjunct (§2.2a); the three fences per
  forcing (§2.3); [the boundary-append site is REUSED, no edit].
- lib/DeltaRel/DeltaRel.cpp: AnyCutSuccessorDR knob disjunct (§2.2b);
  ResolveLiveRecognition helper (§2.1); ABA re-resolution in
  BuildSubgraphInstanceOps + census recount (§3.1); PlanNode
  bound_col_sources + DROp context_cols/context_col_sources + Rederive
  PlanTree (§3.2a-c); V-ALPHA (§3.2d); HP-4 refusal belt (§3.2e);
  V-INST-DRAIN (§3.3); SubgraphInstances() accessor.
- lib/DeltaRel/DeltaRel.h: BindingSource enum + the two field pairs; the
  SubgraphInstances accessor decl.
- lib/ControlFlow/Build/Stratum.cpp: LowerSubgraphInstance + dispatch loop
  (§4.1-4.2); the self-lowered INSTANCESEAL (§4.3); V-INST-EMITTED closing
  check in BuildStratumPhases.
- lib/ControlFlow/Program.h: SUBGRAPHINSTANCE + INSTANCESEAL region-impl
  classes + the ProgramProcedure instance_stores set (§4.2/§5e).
- lib/CodeGen/CPlusPlus/Database.cpp: ProgramInstanceStoreInfo +
  InstanceStores(); Key_<id>/Row_<id> structs; instance_<id> member/ctor/
  ref-param; EmitSubgraphInstance (bands a/b) with the V-INST-FRESH inline
  guard; EmitInstanceSeal; EmitOperation dispatch arms. (§5)
- [D2.c, not this diff] the witness + fence cases + goldens + runall.sh +
  CLAUDE.md.

THE ONE OPEN DECISION for the ritual head: R-REBUILD (§0) — a1-only (feature
gap + narrowed witness + fence-ii stands) vs a1+a2 (answer-complete + fence-ii
relaxes; a2 = FULL-RESCAN per ADJ-C1, NOT incremental). The §7 hand probe
decides it empirically; adopt before writing band-(a2)/the input-frontier
drain. [ADJ-G1] The DUMP shape is unchanged by the choice for CENSUS + DEPS +
.ir/.h; the instantiate op's EFFECTS multiset gains ONE kVecDrain(%table:11,
kNetAddition) under a2 (InstantiateEffects DeltaRel.cpp:774-778 pushes a
second drain) — narrowed from the original "DUMP shape UNCHANGED."

======================================================================
§10. ADJUDICATION LEDGER (XHIGH adjudicator, 2026-07-20, tip 677977f8)
======================================================================
All 9 critic findings verified AT CODE and UPHELD. This doc is amended in
place ([ADJ-*] markers above). Verdict: GO-WITH-AMENDMENTS.

  ADJ-C1 (crit-correctness-1, HIGH) UPHELD — a1+a2 must FULL-RESCAN, never
    incremental TryAdd (TouchCurrent no-rescan InstanceStore.h:130-133;
    current Reset-empty at Seal :202; monotone belt :185-192 SIGABRTs on
    partial current). Amended §0, §4.2, §5f. Collapses a1/a2 to one
    rescan-keyed-on-affected-keys mechanism.
  ADJ-C2 (crit-correctness-2, HIGH) UPHELD — fence-(i) NARROWED to also
    reject recursive CONTENT (input ambiguity + single-scan cannot close a
    TC; witness .dr's own text says nested "fences it out"). Amended §2.3;
    new witness demand_recursive_content_1. Moots ADJ-C4's escalation.
  ADJ-C3 (crit-correctness-3, MED) UPHELD — a1-only silently drops
    edge-after-demand PERMANENTLY and BREAKS the witness's own demand-flap
    REBUILD batch (R-MONO demand idempotent). If a1-only: witness reshaped
    birth-only + hard-ordered (ENFORCED), CLAUDE.md names the gap. Amended
    §0.
  ADJ-C4 (crit-correctness-4, LOW) UPHELD as D3 PRECONDITION — CSE-fold
    stamp-drop is release-silent (View.cpp:588 NDEBUG no-op); dormant
    in-slice (ADJ-C2 fences the only multi-guard shape). Promote the
    View.cpp:585-587 diagnostic to always-on BEFORE recursive content is
    admitted. Amended §2.1. Owner-tracked.
  ADJ-P1 (crit-pins-1, HIGH) UPHELD — HP-4 V-ALPHA arm B was
    key_cols-only; added B-i column-total + B-iii negative
    (no row_cols position traces to instance-key), per d1-pinned.md:131-135.
    Amended §3.2d + §6 SITE-3.
  ADJ-P2 (crit-pins-2, MED) UPHELD — HP-6 (F,T) partition has no dump/DR
    guardian post HP-11 collapse (kStateEmit/kStateOld = generic reads).
    Codegen-level guardian: §6 SITE-3 review line (MANDATORY) + an always-on
    band-(b) runtime assert (OWNER DECISION whether at D2.b). Amended §5i +
    §6.
  ADJ-P3 (crit-pins-3, LOW) UPHELD — subsumed by the R-REBUILD owner
    decision + ADJ-G1: the ds mechanism-natural certification presupposes a2
    (or an explicit undrained-dead-frontier CLAUDE.md note under a1). No
    separate amendment; folded into ADJ-C1/C3/G1.
  ADJ-G1 (crit-grammar-1, MED) UPHELD — a1/a2 dump-invariance is FALSE for
    the instantiate EFFECTS multiset (a2 adds kVecDrain(%table:11)). Narrowed
    §0 + §9; ds §2.3 gains the fork.
  ADJ-G2 (crit-grammar-2, LOW) UPHELD (cosmetic) — the ds spine annotation
    is outside dump bytes; ds moves it out of the certified block.

OWNER DECISIONS OWED AT THE RITUAL HEAD (none block the ARCHITECTURE; all
are within the pinned design):
  OWN-1: R-REBUILD branch — a1-only (feature gap, witness reshaped
    birth-only, fence-ii stands) vs a1+a2 (full-rescan per ADJ-C1,
    answer-complete, fence-ii relaxes). §7 hand probe is the referee.
  OWN-2: ADJ-P2 — is the band-(b) always-on runtime partition assert owed at
    D2.b, or deferred (with the §6 review line as the sole D2.b guardian)?
  OWN-3: ADJ-C4 — confirm the View.cpp:585-587 always-on promotion as a
    hard D3 precondition before recursive content is admitted.

FINAL VERDICT (design half): GO-WITH-AMENDMENTS. The architecture (ABA-safe
re-resolution, chain-breaker excision, OD-4 provisioning, the three-op
family, seal self-lowered, census/deps derivation) is CODE-FAITHFUL and
sound. The amendments correct one incoherent lowering branch (ADJ-C1), close
one unlowerable-shape hole (ADJ-C2), fully build one half-specified pin
(ADJ-P1), and add the missing partition/invariance/ordering guardrails. The
per-diff ritual still gates the landing; OWN-1..3 decided at its head.
