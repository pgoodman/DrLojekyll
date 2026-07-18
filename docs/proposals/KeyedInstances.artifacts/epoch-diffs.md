# Epoch diffs — DEMAND-KEYED INSTANCES + IMPLICIT ASYNCHRONY

Every epoch diff expressed as a PRECISE delta against the D0-fleet-verified
as-landed pseudocode — NOT the stale PerfRoadmap §18.5 anchors (those are
close-time positions on branch demand-seeds, drifted by the copyright sweep;
E-51 rolls up the drift). Every anchor here cites a fleet report
(`scratchpad/fleet-d0/{lane-pipeline,lane-fabric-inject,lane-drir,
consolidated}.md`) or a code read done THIS session on branch
`keyed-instances` @fa9a8cc2 (off 60821adf). The E-46..E-53 corrections are
folded in inline.

FORMULATION-LANE STATUS: this is the DESIGN-GOALS-AS-DIFFS document
(checkpoint step 2). It is not code and not a judge. Each diff below states
its delta, its acceptance gate, and — mandatory per house method — its
PRE-REGISTERED PREDICTIONS (driver churn / golden churn / suite-count
trajectory / Q5) before any measurement. The recommended cut order and the
open owner decisions close the file.

READING ORDER for an implementer: this file → the cited fleet lane report
for the surface being touched → the code at the re-anchored line. Never the
§18.5 line numbers.

--------------------------------------------------------------------------
## 0. The verified as-landed baseline (what every diff is a delta against)

The fleet re-derived the demand pipeline / fabrication+injector / DR-IR
GROUP_UPDATE mold / R-A store from code. The load-bearing shape, with the
authoritative anchors (all @fa9a8cc2 unless noted):

- **Demand pass** `ApplyDemandTransform` (`lib/DataFlow/Demand.cpp:363`),
  run at the post-`ConnectInsertsToSelects` slot `Build.cpp:2576`, before
  `Optimize` :2585 (the `impl->Optimize(log)` call; ApplyDemandTransform at
  :2576). Mode gate :370 (total no-op flag-off). Steps 1-10 in
  lane-pipeline.md §1.
- **GuardSite::Kind has THREE kinds** (`Demand.cpp:128-132`):
  `kReadAtTuple` / `kPushDown` / `kBaseAtom` (E-46). Step-7 rewiring BRANCHES
  on kind (:962 kReadAtTuple → no restoring TUPLE; else → MintRestoringTuple
  :974). lane-pipeline.md:148-151.
- **Two MintGuardJoin call sites, two demand sides**: body guards
  `Demand.cpp:960` pass `d_reader`; the query-projection guard `:1002` passes
  a FRESH `raw_seed` receive-projection TUPLE (E-52 second half).
  lane-pipeline.md §2, consolidated §1.
- **producer="DEMAND-GUARD"** is `#ifndef NDEBUG` (`Query.h:522-526`) — a
  DEBUG-only string, ABSENT in release (E-52). It is NOT a usable attribute.
- **Fabrication** `lib/Parse/Demand.cpp`: `FabricateDemandMessage` :163,
  `FabricateDemandLocal` :206, `DemandFabricationWouldCollide` :140,
  `MarkDemandFabricated` :159. Reserved prefix `demand__` (lowercase).
  lane-fabric-inject.md §1.
- **Injector** `QueryDemandForcing` (`include/.../Query.h:950`), wired at
  `Build.cpp:1281`, matched :465-474 (entry.query==query AND
  BindingPattern() belt :467-469), `BuildQueryForceProcedureFromRegistry`
  :383. Suppression `Database.cpp:1435`/`:3087`. lane-fabric-inject.md §2-3.
- **DR-IR mold** `BuildGroupUpdateOps` (`lib/DR/DR.cpp:638-761`): mints a
  `DRStateCell` descriptor + a `kGroupUpdate` op (two bands, fixed effect
  set) + a trailing `kStateSeal`. Census inside `ValidateDROps`
  (DR.cpp:2683-2935), V-AGG-EFFECT/V-AGG-SOLE/V-AGG-PAIR. lane-drir.md §1-3.
- **R-A frozen-pair store**: PAPER-ONLY — zero instance code exists (grep
  for BuildSubgraphOps / SubgraphInstantiate / InstanceStore is EMPTY,
  consolidated §3). Smallest new-Runtime surface = MakeTable, InstanceStore,
  `Table::Reset()` (the last a SOUNDNESS requirement under Arena, not perf
  residue — consolidated §3 R-A store, the Arena::Free no-op gap).
- **Seam DR-IR home** = existing `kVecAppend`/`kVecDrain` effects + a NEW
  `VecRole` in `is_epoch_carried_role` (`DR.cpp:3504-3519`) + `loop_carried`
  under V-LOOP — NOT a reserved EffKind (E-50; precedent B-10).
- **Harness**: suite 168; `.drflags` sidecar appends per-case flags to ALL 4
  modes; only `demand_tc_witness` + `demand_multi_adorn_1` carry one. NO
  demand-ON emitted-header byte-identity gate exists today (consolidated §3
  harness). Bound-query corpus 41/168.

--------------------------------------------------------------------------
## (F) The determinism fix — DO-FIRST, gates the epoch

### (F).0 The delta

The seed's (F) framing is WRONG in scope, locus, and premise (E-47/E-48/E-49,
consolidated §4). Reproduced @60821adf: `-demand -ir-out demand_tc_witness`
= 3 distinct hashes / 8 runs (NOT ".ir stable"); flag-off `cf14_1` = 3/12,
`cond_in_induction` = 11/12 (NOT "flag-off deterministic"). It is a
PRE-EXISTING latent bug in SHARED induction machinery, merely unmasked by
-demand's extra minted views tipping allocation.

ROOT CAUSE (single, confirmed this session by direct read):
`lib/DataFlow/Induction.cpp:520` iterates
`std::unordered_map<VIEW *, MergeSet> merge_sets` (declared :108) in
pointer-hash bucket order. VIEW nodes are plain `new` (DefUse.h:890 `Create`),
so VIEW* varies run-to-run; the buckets permute the whole downstream chain.

### (F).1 ADJUDICATION of the fix locus — the consolidator's proposal is INCOMPLETE

[AMENDED: MEDIUM — the "merge_set_id IS emission-load-bearing" premise was
overstated. Traced repo-wide: merge_set_id's VALUE reaches emitted bytes ONLY
via the DataFlow DOT dump "SET N" (Format.cpp:47) and an NDEBUG comment
(Build/Induction.cpp:664), NOT the released .h/.ir codegen. The RECOMMENDED
LOCUS (sort the :520 iteration) still stands — it is the cleaner single move —
but the stated REASON is corrected below so an implementer scopes the fix and
its gate correctly.]

The consolidator (§4 FIX SHAPE) proposes: "after the population loop
(:520-537), Sort each distinct `related_merges` WeakUseList once via
Sort(Pred)". I READ the loop this session and this is the operative delta:

**THE ITERATION SORT AT :520 IS THE CLEANER SINGLE-MOVE LOCUS: it stabilizes
BOTH the merge_set_id assignment order AND the related_merges population order
in one move, where the WeakUseList sort alone stabilizes only the population
axis.** The population axis is the codegen-load-bearing one; the merge_set_id
VALUE axis is NOT released-codegen-load-bearing (see the corrected scope note
below). The evidence for the two axes:

  Induction.cpp:520-537 (the population loop), abridged:
    for (auto &[view_, set] : merge_sets) {          // :520 POINTER ORDER
      ...
      if (!merge_set->related_merges) {
        merge_set->related_merges.reset(new WeakUseList<VIEW>(view));
        merge_set->merge_set_id = group_id;          // :530  <-- id MINTED HERE
        ++group_id;                                  // :531  in iteration order
      }
      info->merge_set_id = merge_set->merge_set_id;  // :534
      info->cyclic_views = merge_set->related_merges;// :535
      merge_set->related_merges->AddUse(view);       // :536  population
    }

TWO axes permute, not one:
  (i)  `related_merges` POPULATION order (:536) — the AddUse order; this is
       what feeds `InductiveSet()` (Query.cpp:1274-1275) → the
       `for (auto other_view : view.InductiveSet())` walk at
       ControlFlow/Build/Induction.cpp:673 → `VectorFor` → `next_id++`
       (Procedure.cpp:163). A post-population `related_merges->Sort(Pred)`
       DOES fix this axis (the consolidator's proposal).
  (ii) `merge_set_id` ASSIGNMENT order (:530-531) — the id is minted the
       FIRST time a merge_set is seen in the :520 pointer walk. It flows to
       `info->merge_set_id` (:534) → `QueryView::InductionGroupId()`
       (Query.cpp:1216-1217) → consumed at ControlFlow/Build/Induction.cpp:662
       (`merge_group`) and is the ID SPACE over which `directly_reachable_from`
       / `group_to_label` are indexed (Induction.cpp:541-544, the second pass
       :553 keys `merge_id`). A post-population WeakUseList sort does NOTHING
       for this axis — the id was already minted during the pointer walk.
       CORRECTED SCOPE (traced repo-wide this session): the merge_set_id VALUE
       is NOT released-codegen-load-bearing. Its consumers are: (1)
       `NeedsInductionCycleVector` — `.has_value()` only (boolean); (2)
       Build/Induction.cpp:662-664 — the value only in an `#ifndef NDEBUG`
       comment string; (3) `InductionDepth`==`group_to_label[merge_id]` is
       id-permutation-INVARIANT (the topo-label pass Induction.cpp:604-626
       converges to the same per-group graph depth for any id numbering); (4)
       Stratum.cpp:199-206 keys an unordered_map on the value but uses it only
       as an order-insensitive SCC LABEL (distinctness, not order); (5) the
       ONLY VALUE-to-emitted-bytes path is the DataFlow DOT dump `SET N`
       (Format.cpp:47) plus the NDEBUG comment. So sorting the :520 iteration
       stabilizes this axis as a BONUS (the DOT `SET N` label + debug comment)
       — a belt, not the load-bearing reason. Gate 1/2 (.ir + .h) do NOT
       exercise the merge_set_id VALUE axis; the .h/.ir codegen nondeterminism
       the repro measured (E-47: $induction_in/$induction_pivots vec-id swaps)
       is the POPULATION axis (i) feeding VectorFor `next_id++`.

**VERDICT: the fix locus is the ITERATION at :520, not (only) the
post-population WeakUseList — because the iteration sort is the cleaner single
move that stabilizes BOTH axes at once (the population axis, which IS
codegen-load-bearing, AND the merge_set_id VALUE axis, whose only emitted-byte
footprint is the DOT `SET N` dump + NDEBUG comment — a bonus/belt).** The
WeakUseList sort alone would fix the codegen-load-bearing population axis (and
thus likely the .h gate) but leave the DOT `SET N` label pointer-ordered;
sorting the iteration is strictly simpler and covers both. Sorting the
merge_sets iteration by a stable view key stabilizes both because both are
consequences of the :520 walk order. The concrete diff:

  + BEFORE the population loop, materialize the merge_sets entries into a
    vector and SORT it by the stable key (below), then iterate the sorted
    vector at :520 AND at the second pass :553 (same order, so
    directly_reachable_from indices stay consistent).
  - The post-population `related_merges->Sort(Pred)` becomes REDUNDANT once
    the iteration is stable (AddUse fires in sorted order) — but keeping it
    as a belt is cheap and defends against a future unsorted AddUse path.

### (F).2 The sort-key question — Sort()==Hash() vs VIEW id; is the id stable?

[AMENDED: HIGH — the pipeline-order fact this sub-section rested on was
INVERTED (IdentifyInductions runs BEFORE FinalizeColumnIDs, code-verified),
so the recommended `firstColId` tie-break read PRE-finalize (variable/lexical)
ids the E-47 measurement never covered. The old (a)/(b) reasoning is preserved
below the corrected block, marked SUPERSEDED; the re-decided key uses no
`col->id` at sort time.]

The consolidator proposes key `(view->Sort(), firstColId(view))` with
`Sort()==Hash()` (View.cpp:120-122, CONFIRMED this session — it literally
`return Hash();`).

**CORRECTED PIPELINE-ORDER FACT (code-verified this session, Build.cpp):**
`IdentifyInductions(log)` is called at `Build.cpp:2597`, and
`FinalizeColumnIDs()` at `Build.cpp:2603` — i.e. **`IdentifyInductions` runs
BEFORE `FinalizeColumnIDs`, not after.** `FinalizeColumnIDs` (Columns.cpp:13-25)
is the SOLE definition of the finalized column ids: it walks `ForEachView` and
reassigns `col->id = next_col_id++` in a fresh 1..N sequence. Therefore the
`%col:N` values the E-47 measurement observed as byte-stable are the
POST-finalize ids (they appear in the dump, emitted after :2603) — a DIFFERENT
id generation than the sort inside `IdentifyInductions` would read. At :2597
`col->id` is still the PRE-finalize, variable/lexical-derived id
(Build.cpp:1787 `vc->id = vc->FindAs<VarColumn>()->id`), which (i) was never
measured for stability by E-47 and (ii) can be NON-UNIQUE across views (two
views sharing a variable's column id). So a `firstColId` tie-break read at sort
time is NOT justified by the E-47 evidence AND may fail to be total precisely on
the structurally-identical inductive MERGE pair the tie-break exists to
disambiguate (a `Sort()==Hash()` collision that also shares its pre-finalize
first-column id).

**RE-DECIDED KEY (uses no `col->id` at sort time):** primary key
`view->Sort()` (== Hash(), structural, pointer-independent, run-stable AND
flag-boundary-stable — see (b) below, that half stands). For the tie-break,
prefer a key that reads NO `col->id`:
  - first tie-break: the **incoming-view `Sort()` TUPLE** (the ordered hashes of
    the merge's incoming views — structural, pre-finalize-independent).
  - final tie-break for a genuine CSE-duplicate that is structurally
    indistinguishable even there (same Hash AND same incoming-view Sort tuple):
    an **explicit build-order ordinal minted by a `ForEachView` sweep BEFORE
    the sort** — `ForEachView` is the deterministic DefList order, the SAME
    determinism source `FinalizeColumnIDs` itself relies on. This gives a
    guaranteed total order without touching any `col->id`.

  ALTERNATIVE (larger move, must be re-judged): relocate the `merge_set_id`
  assignment (the whole `IdentifyInductions` sort) to AFTER `FinalizeColumnIDs`
  so the measured-stable finalized ids ARE available as the tie-break. This is a
  pipeline reorder, not a local sort change — it changes the id generation
  visible to every consumer between :2597 and :2603 and must be judged on its
  own. Recorded as an option; the ForEachView-ordinal key above is the
  local-change default.

  The review gate must CONFIRM the chosen key is a TOTAL order on the actual
  witness set (demand_tc_witness + cf14_1 + cond_in_induction) — a partial
  order that leaves a tie re-opens (F). With the ForEachView-ordinal final
  tie-break this is total by construction, but the gate confirmation stays
  mandatory.

  (b) IS Sort()==Hash() RUN-STABLE? Hash() is a STRUCTURAL hash (content of
      the view: kind, column types, incoming structure) — pointer-independent
      by construction, so it is run-stable AND flag-boundary-stable for a
      structurally-fixed view. It CAN collide (two structurally-identical
      inductive MERGEs — exactly the group_ids CSE-guard scenario), so it is
      NOT a total order alone. This half of the original reasoning STANDS and
      motivates the tie-break above.

--- SUPERSEDED (pre-amendment reasoning; the (a) sub-question rested on the
    inverted pipeline-order fact and the firstColId tie-break it justified) ---

  (a) [SUPERSEDED] IS THE VIEW ID STABLE AT THIS POINT? `IdentifyInductions`
      runs at Build.cpp (the C5 pipeline slot, AFTER FinalizeColumnIDs but the
      VIEW id itself is a Create-order ordinal, not re-labeled). VIEW ids are
      NOT re-canonicalized after Optimize the way column ids are (Columns.cpp:21
      `next_col_id++` is COLUMN id; there is no VIEW-id relabel pass). So a
      raw VIEW id is a **malloc/Create-order artifact** — but Create order on
      the demand path is exactly what -demand perturbs (extra minted views
      shift subsequent ids). USING RAW VIEW ID AS THE SORT KEY WOULD NOT FIX
      (F): it is stable run-to-run for a FIXED input+flag (Create order is
      deterministic given the same allocation sequence) but it is NOT stable
      across the -demand/-flag-off boundary, and worse, the whole bug is that
      the DOWNSTREAM id assignment must not depend on an upstream
      pointer/Create artifact. VIEW id is therefore the WRONG primary key.
      [The rejection of raw VIEW id as primary key STILL HOLDS; only the claim
      that IdentifyInductions runs AFTER FinalizeColumnIDs was wrong, and the
      firstColId tie-break it led to is replaced above.]

  (b-original) [SUPERSEDED phrasing] RECOMMENDATION: primary key `view->Sort()`
      (== Hash(), structural, run-stable), tie-break `firstColId(view)` =
      `(*view->columns.begin())->id` (column ids ARE byte-stable here per the
      E-47 measurement: `%col:N` identical across all -demand runs). If that
      still collides (two views with identical structure AND identical
      first-column id — possible for a genuine CSE-duplicate pair sharing a
      table), append a THIRD tie-break: the full column-id tuple, then the
      incoming-view Sort() tuple. [The `%col:N` justification is for the
      POST-finalize id generation, NOT the pre-finalize ids the sort reads —
      replaced by the col-id-free key above.]

### (F).3 Secondary suspect (record-only)

`Induction.cpp:359` `for (VIEW *view : injection_sites)` iterates a
`std::set<VIEW*>` (:128) — pointer-ordered — and mints new MERGEs (:378,
producer="INDUCTIVE-LEAVE") in that order, permuting newly-minted view/column
ids WHERE it fires. NOT firing on demand_tc_witness/cf14_1/cond_in_induction
(no injection on this corpus). Fix identically (collect to vector, sort by
Sort()+col-id) IF a future witness trips it. DR.cpp's unordered_maps run
AFTER vector ids are assigned and are NOT implicated.

### (F).4 Acceptance gates (per the ledger, E-47/E-49-corrected)

  1. demand-ON `.ir` AND `.h` byte-identity restored for `demand_tc_witness`
     (N-runs → 1 hash each). The seed's ".ir stable" scope was wrong; the
     gate MUST diff `-ir-out`, not only the header.
  2. flag-off `-ir-out` stability on `cf14_1` + `cond_in_induction` (both
     1-hash / N-runs after the fix). These are the pre-existing-latent
     witnesses; they must go green independently of -demand.
  3. The fix touches SHARED machinery → flag-off emissions on
     multi-merge-set programs MAY legitimately change SHAPE ONCE (a new
     canonical order). Policy: byte-identity-WITH-STRUCTURAL-GATE vs the
     pre-fix snapshot; stdout goldens MUST be ZERO-CHURN (a shape shift that
     changes any answer is a bug, not a re-bless).
  4. ctest 3/3; suite 168 PASS.

### (F).5 PRE-REGISTERED PREDICTIONS

  - DRIVER CHURN: **zero.** No driver observes induction-vector ids or
    emission order; all corpus drivers key on runtime stdout.
  - GOLDEN CHURN (stdout, the 168): **zero required.** The fix reorders
    induction-vector-id allocation and sibling-region emission; it changes no
    answer. If ANY stdout golden moves, STOP — it is a miscompile, not a
    re-bless (gate 3).
  - THE ONE-TIME FLAG-OFF SHAPE SHIFT: enumerated by STRUCTURE — only
    programs with **>1 merge set** (>1 distinct induction group) can have
    their merge_set_id / induction-vector order re-canonicalized. The
    EXPECTED-SHIFT + NEWLY-STABILIZED set is the MEASURED 12-case flag-off
    instability floor from KeyedInstances.md §1 (lines 217-234, 6-runs/case
    -ir-out hash sweep + the fleet's independent runs), NOT a hand-narrowed
    3-case list: `cond_in_induction`, `kcfa_tiny`, `kcfa_tiny_merged`,
    `cf14_2`, `cond_in_induction_deep`, `product_ind`,
    `transitive_closure_multiple_clause_bodies`, `transitive_closure2`,
    `transitive_closure3`, `transitive_closure5`, `two_inductions`, `cf14_1`
    — every one a multi-induction/multi-merge-set program, zero non-recursive
    cases (consistent with the E-48 root cause). Plus `demand_tc_witness`
    under -demand (shifts to its now-canonical order). Single-induction and
    induction-free programs (the majority of the 168) predict BYTE-IDENTICAL
    flag-off — a single merge_set has only one iteration order regardless of
    pointer bucket. Per the ledger's own phrasing (§1): "these 12 are the (F)
    fix's expected one-time-shift + newly-stabilized set; any case NOT in this
    list that shifts shape under the fix needs an explanation before bless."
    The review must produce the ACTUAL shifted-case list from a pre/post
    `-ir-out` sweep and confirm it is a SUBSET of this measured floor; a shift
    OUTSIDE the multi-merge-set floor is an unexplained defect. (Using the old
    3-case narrowing as the allowlist would false-alarm the other 9 measured
    cases — an alarm the ledger already resolved.)
  - SUITE COUNT: unchanged at 168 (no new case; the demand-ON `.ir`/`.h`
    goldens land as sidecars on the EXISTING demand_tc_witness, see T2).
  - Q5 / FLAGSHIP: emission-neutral in the timed sense (same regions, one
    canonical order); expect within-noise on the SAME-SESSION ABABAB. No
    perf claim. This diff is a correctness/determinism fix, not a perf diff.

### (F).6 IR-goldens interaction (sequencing with T1-T3)

(F)'s acceptance gate 1-2 WANTS a byte-identity gate surface. INTERIM: (F) can
land gated by the already-used scripted `-ir-out`/`-h` hash-sweep
(KeyedInstances.md §1, 6-runs/case) — the fix's root cause is already found by
hand, so the hunt does not wait on tooling. PERMANENT: that surface is T2 (the
dumps) + directive-5 sidecars (`.dfgold`/`.deltarelgold`, plus a demand-ON `.h`
golden), which SUPERSEDE the interim script. So T2 sequences before (F)'s
PERMANENT gate (not before (F)'s code) — the dumps are the determinism
discriminators AND the restored permanent gate (ir-dump-formats.md §2.5:
"demand_tc_witness's demand-ON IR+header goldens ARE the restored (F) gate once
the fix lands"). See T1-T3 sequencing below.

--------------------------------------------------------------------------
## T1-T3 The tooling block — sequencing relative to (F)

The owner's §0.5 directives, folded into three sequenced diffs. All
flag-off-neutral by construction; all sequence around (F) because the dumps
ARE the (F) determinism discriminators and the restored gate.

### T1 — RENAME lib/DR → lib/DeltaRel (§0.5 directive 1)

DELTA: mechanical directory + target rename, mirroring lib/DataFlow /
lib/ControlFlow. Byte-identity-gated.

CAVEAT (from the demand-seeds close honesty correction, DemandSeeds.md §2.4):
DR is NOT cleanly isolable — it reads ControlFlow INTERNAL headers (Program.h,
Build/Build.h Context) while ControlFlow links DR, so the rename carries the
same cross-target include dirs in both directions the promotion did. A true
interface decoupling is NOT this diff (priced only if seam/instance growth
justifies it, §18.1 close-sweep leftover).

CLOSE-SWEEP-LEFTOVER NOTE (§18.1, PerfRoadmap:2890): the third §18.1
close-sweep leftover — the `has_one_insert` dead-branch cleanup (live code at
Connect.cpp:17/:50, referenced Demand.cpp:481/:923) — is carried unchanged; a
quiet-slot cleanup, not epoch-blocking, recorded here so the residue is not
lost (the other two §18.1 leftovers land as the COST-witness bench promotion,
D2.3, and this interface-decoupling note).

P4 RESIDUE (§18.1 / §16.1): fusion, WCOJ, dead-group compaction and the other
P4 items are carried unchanged, next-epoch (MEASURE-FIRST) — explicitly OUT of
this epoch's design-goals-as-diffs verification surface (consolidated §5). Noted
here so the omission is explicit rather than silent.

SEQUENCING: FIRST, and BEFORE any DR-IR-touching diff (D1/D2/D4). The fleet
is done reading lib/DR (§0.5 directive 1 said "DEFERRED until the D0 fleet
completes" — it has). Do it before T2's DeltaRel dump so the dump lands at
the final path.

GATES: byte-identity of all emitted output (the rename touches no emission);
suite 168 zero churn; ctest 3/3.

PREDICTIONS: driver churn zero; golden churn zero; suite 168; Q5 neutral. Any
churn = a botched rename (a missed include path), not a real change.

### T2 — the two dumps: -deltarel-out and -df-out (§0.5 directives 2, 4)

DELTA: two id-ordered textual dumps, deterministic BY CONSTRUCTION
(ir-dump-formats.md is the design draft; reconcile the DeltaRel op/attribute
inventory against lane-drir.md §0-4 before coding — the draft was written
from the CLAUDE.md summary, lane-drir is authority).
  - `-deltarel-out`: linearized-schedule order, per-op kind/sign/position/
    claim-context + effect sets + membership predicates, DRVecs with types
    and def/use, band boundaries, census counts. Emitted from the CHECKED
    model AFTER validation (ir-dump-formats.md §2). This is the DR-IR's FIRST
    observability surface — today the only fprintf in lib/DR is validator
    aborts (lane-drir.md §5: NO printer/DOT/dump exists).
  - `-df-out`: the non-DOT "basic-block-with-arguments" DataFlow form (blocks
    tail-call users; MERGE = block-args join point; JOIN declares ports)
    (ir-dump-formats.md §1). Blocks in VIEW-ID order.

CRITICAL DEPENDENCY ON (F): the -df-out dump emits blocks in VIEW-ID order
and prints induction attributes; the -deltarel-out dump emits ops in
linearization order. If VIEW-id or linearization order is nondeterministic,
the dump EXPOSES it — which is a FEATURE for the (F) hunt (ir-dump-formats.md
§2 determinism argument). So T2's dumps land BEFORE (F)'s fix as
discriminators, and become deterministic-by-construction only AFTER (F)
stabilizes the upstream id/linearization order. Practically: T2 lands the
dump code (byte-identity flag-off for the DUMP flags is trivially true — new
flags, untouched by runall.sh); the dump's OWN determinism is a POST-(F)
property, asserted by (F)'s gate 1-2 using these very dumps.

SEQUENCING: -deltarel-out AFTER T1 (lands at lib/DeltaRel path). Both dumps
BEFORE (F)'s final gate (they are the gate surface). The (F) hunt itself can
proceed in parallel with T2 landing — the hunt already found the root cause
by hand; T2 gives the REGRESSION gate.

GATES: the dump flags are never invoked by runall.sh (no suite impact); the
dumps are deterministic-by-construction POST-(F); directive-3 (end-to-end IR
review) becomes MECHANIZABLE (read .df/.ir/.deltarel for every touched
witness).

PREDICTIONS: driver churn zero; stdout golden churn zero (new flags, no
emission path touched); suite 168; Q5 neutral (dumps are opt-in, off the hot
path). NEW golden CLASS introduced but not yet populated — see T3.

### T3 — the pass harness P1 + IR-golden sidecars (§0.5 directives 5, 6)

DELTA: two coupled sub-diffs (pass-harness-design.md is the draft, judge
before code):
  - IR-GOLDEN SIDECARS (directive 5): `<name>.dfgold` / `<name>.deltarelgold`
    (+ a demand-ON `.h` golden) opt-in per case, byte-compared by the
    harness, blessed only via `runall.sh --bless`. MODE-PINNED (IR is
    mode-sensitive; first line names the mode(s), opt-only default).
    permcheck.py stays a stdout referee; IR goldens are STRICT byte-compare
    (a permutation is exactly what they catch). First carriers:
    fixpoint_stress_1, reconverge_1, demand_tc_witness (+ aggregate corpus).
  - PASS HARNESS P1 (directive 6): the pass registry + PassPolicy +
    -opt-disable/-opt-only + legacy-alias rewiring (the 4 golden modes become
    -opt-disable=df.*/cf.* aliases) + -opt-bisect-limit at the ~10 driver
    sites. DeltaRel stages are OBSERVATION POINTS, not skippable passes.
    P2-P5 (print-after / counters / passes-spec / reducer) are LATER, not
    this epoch's do-first surface.

CRITICAL DEPENDENCY ON (F): P1 is emission-neutral by construction (default =
current pipeline). But the -opt-bisect-limit counter's soundness REQUIRES
id-ordered (deterministic) iteration — "the same input yields the same index
sequence, which is what makes binary search sound" (pass-harness-design.md
§2). So P1's bisect is only MEANINGFUL post-(F). And the IR-golden sidecars
(the demand-ON gate) are only VALID post-(F) — a nondeterministic dump cannot
be a byte-compare golden. This is why directive 5's note "demand-ON IR
goldens become valid the moment (F) lands — and ARE the restored gate"
(§0.5 directive 5).

SEQUENCING: the IR-golden sidecar HARNESS lands with/after T2 (needs the dump
flags); the demand_tc_witness demand-ON sidecar is BLESSED as (F)'s gate 1
(the restored gate). The pass-harness P1 is LOWER priority — it is
infrastructure the epoch benefits from but does not BLOCK on; land it after
(F)+T2 or defer to a quiet slot. P2-P5 are explicitly out of the do-first
scope.

GATES: P1 default config = current pipeline EXACTLY (byte-identical binaries
modulo new flag parsing; pass-harness-design.md §5 prediction: "any churn = a
P1 bug"); the 4 golden modes re-expressed as aliases produce byte-identical
suite results.

PREDICTIONS: driver churn zero; stdout golden churn zero (P1 emission-neutral;
sidecars are NEW goldens, blessed not churned); suite 168 (the sidecar
goldens are additional gate surfaces on existing cases, not new cases); Q5
neutral (P1 adds flag parsing only). The demand_tc_witness demand-ON `.h` +
`.deltarelgold` + (mode-pinned) `.dfgold` sidecars are the FIRST populated
carriers and the (F) regression gate.

### T1-T3 sequencing summary (the do-first spine)

  T1 (rename)  →  T2 (dumps, as (F) discriminators)  →  (F) fix
                                                    ↘  T3 sidecars = (F) gate
  T3 pass-harness P1: parallel/after, non-blocking.

The dumps and the (F) fix are INTERLEAVED, not strictly ordered: the dumps
land to serve as discriminators DURING the hunt, and (F)'s PERMANENT gate
consumes them as the restored byte-identity surface. (F) remains the epoch's
do-first CORRECTNESS diff; it is landable IN PARALLEL with T1/T2 gated by the
interim scripted hash-sweep (KeyedInstances.md §1); T1-T2 are prerequisites for
(F)'s PERMANENT gate only; T3's sidecar is that permanent gate. Only T1 before
T2's DeltaRel dump is a hard path dependency.

--------------------------------------------------------------------------
## D1 The instance-lowering design + judge (design-only; hand-write witness IR before code)

D1 realizes the SECOND lowering of p^α: the keyed NESTED instance (the
ratified R-A frozen-pair paper) on the flat guarded copy's (D4's) frontier.
It is DESIGN + JUDGE, no emission code until the design survives its
adversarial judge and the witness IR is hand-written (§18.2 D1).

### D1.0 The delta at a glance

  + the demand pass ANNOTATES its guard sites with a release-surviving,
    per-guard-site {instance-key, demand-side} attribute (E-52; NOT the
    debug producer string; THREE kinds per E-46; and the query-projection
    guard is NOT an instance-key site, see D1.1)
  + `BuildSubgraphOps` in the `BuildGroupUpdateOps` mold mints
    SUBGRAPH_INSTANTIATE + INSTANCE_SEAL from the annotation (lane-drir mold)
  + census from the QUERY-side annotation count (E-27), no mint-order keys
    (E-28)
  + the TWO-LOWERINGS EQUIVALENCE GATE (flat D4 vs nested D1 answer-identical,
    oracle-refereed, SAME witness)
  + the acyclic-DEMAND fence (ViewSelfReachable of the demand view THROUGH
    the instance)
  + the demand-retract = instance-death arm through the frozen-pair partition
  + E-53's second-half obligation (the flat guard JOIN excision / sole-writer)

### D1.1 The annotation route — E-46 (three kinds) + E-52 (two sides) corrected

The seed's §18.5(C) "the pass marks its guard joins as instance-key sites"
must be corrected on TWO axes the fleet found:

**(a) THREE GuardSite kinds, per-kind annotation semantics (E-46).** The enum
is `{kReadAtTuple, kPushDown, kBaseAtom}` (Demand.cpp:128-132), not two. Each
guard site (Demand.cpp step 3b, one per p_merge member) carries its kind, and
step-7 rewiring branches on it (:962 vs :974). The instance-key annotation
must be PER-KIND:
  - `kReadAtTuple` (direct full-width recursive p read, consumer TUPLE/INSERT,
    located :650): the instance key IS `site.pivot_pos` (positions within
    `site.read` = the recursive p reader). This IS an instance-key site — the
    demanded recursive subgoal. Demand side = `d_reader`. NO restoring TUPLE
    (:962), so the instance write substitutes directly at the consumer.
  - `kPushDown` (recursive p read reached THROUGH a body JOIN, consumer = the
    JOIN, located :704): the instance key is `site.pivot_pos` (the found_col
    positions on the p read among the JOIN inputs). ALSO an instance-key site.
    Demand side = `d_reader`. Restoring TUPLE (:974) re-establishes the read's
    column order for the JOIN.
  - `kBaseAtom` (base rule, source = a message-receive atom, NO interior p
    read, located :659): this is NOT a recursive subgoal — a base body demands
    nothing further. It is NOT pushed to `pushdown_reads` (lane-pipeline.md:151).
    **ADJUDICATION: kBaseAtom is a demand-GUARD site but NOT an INSTANCE-KEY
    site** — there is no nested sub-relation to instantiate at a base atom;
    the base rows flow into the instance's `current` table but the base atom
    itself is not the instance-store boundary. The annotation records
    {kind=kBaseAtom, is_instance_key=false} — it participates in the guarded
    copy but the SUBGRAPH_INSTANTIATE op keys off the kReadAtTuple/kPushDown
    sites (the recursive subgoal reads), NOT the base atoms.

**(b) TWO demand sides; the query-projection guard is NOT an instance-key
site (E-52).** There are TWO MintGuardJoin call sites with DIFFERENT demand
sides (lane-pipeline.md §2, consolidated §1):
  - BODY guards (step 7, Demand.cpp:960): demand side = `d_reader` (the shared
    derived d_p reader). These are the instance-key sites (per kind, above).
  - the QUERY-PROJECTION guard (step 8, Demand.cpp:1002): demand side = a
    FRESH `raw_seed` receive-projection TUPLE (:990, producer
    "DEMAND-RAW-SEED"), joined against q's p read on `p_bound`.
    **ADJUDICATION (answering the lane's explicit question): the
    query-projection guard is NOT an instance-key site.** It is the
    OUTER-QUERY entry guard — it forces the top-level demanded answer keys
    into the frontier (the raw seed = the driver-supplied bound argument), not
    a nested subgoal boundary. Its pivot set (`p_bound`) is the query's bound
    projection, which IS the instance key OF THE TOP-LEVEL demand — but the
    instance STORE is written by the recursive subgoal sites (d_reader-guarded
    kReadAtTuple/kPushDown), and the query-projection guard is the READER of
    the top instance, not a writer. So its annotation is
    {is_instance_key=false, role=query_projection, demand_side=raw_seed}. The
    raw_seed-vs-d_reader divergence is exactly what D3 makes live (the d4s3 F1
    coincidence argument no longer holds sideways once >1 adornment exists —
    see D3).

**(c) The mechanism (E-52).** The annotation must SURVIVE NDEBUG.
`producer="DEMAND-GUARD"` is `#ifndef NDEBUG` (Query.h:522-526) — unusable.
`demand_forcings` is per-QUERY, not per-guard-site (lane-fabric-inject.md §2a).
So D1's first concrete task: a release-surviving per-guard-site side table on
QueryImpl — `std::unordered_map<JOIN*, GuardAnnotation>` (or a structured
field on the minted JOIN), stamped inside MintGuardJoin (Demand.cpp:150-196,
the single natural hook covering both call sites — lane-pipeline.md §2) or at
the two call sites (:960, :1002) after the call. `GuardAnnotation` records
{instance_key = pivot_pos (positions in site.read), demand_side (d_reader |
raw_seed), kind (kReadAtTuple|kPushDown|kBaseAtom), is_instance_key,
role (body|query_projection)}. The DR inventory READS this side table (never
re-recognizes structurally — the D3-F6 recognizer half is closed by
construction, E-53). NOTE: the side table is a `std::unordered_map<JOIN*>` —
it must be ITERATED in a stable order at DR-inventory time (id-ordered, not
pointer-keyed) or it reintroduces an (F)-class nondeterminism; iterate by JOIN
id, not map order.

### D1.2 BuildSubgraphOps in the BuildGroupUpdateOps mold

DELTA against lane-drir.md §1 (BuildGroupUpdateOps, DR.cpp:638-761):
  + `DROpKind`: add `kSubgraphInstantiate` / `kInstanceSeal` (mirror
    kGroupUpdate DR.h:129 / kStateSeal DR.h:135) — lane-drir.md §4.
  + `DROp` per-kind fields (DR.h:523-544): an instance block mirroring the agg
    block {instance_view, provenance=kDemand, instance_table, demand_view (=
    d_reader), instance_id (dense mint-order), key_cols (= pivot_pos),
    free_cols (p's non-key columns)}.
  + an instance descriptor vector `DRFlowGraph::instances` (mirror
    `statecells` DR.h:647) + accessors (mirror GroupUpdates()/StateSeals()).
  + a seeded strata map `instance_stratum` (mirror `group_update_stratum`
    DR.h:684) — cleared+populated in DeriveDRStrata (DR.cpp:2140-2154 mold)
    with a lift block (DR.cpp:2279-2291 mold). **op_stratum MUST return the
    real instance_stratum, never default-0** (lane-drir.md §4: the V-READY
    false-negative hole — DR.cpp:3331-3333 is the precedent to copy).
  + EFFECT SET: the frozen-pair R-A store's effect vocabulary. The GROUP_UPDATE
    effect set is {2 drains, 2 ± folds, 1 emit, 1 old, 2 NonRecursive
    counters, 2 kInI crossings, 2 queue appends} (lane-drir.md §2 V-AGG-EFFECT).
    The instance analog (per R-A paper, PerfRoadmap §18.5(B)): a band that
    folds the demand frontier (d_reader net-add/net-remove drains) into the
    instance store's `current`, and a `publish_touched` band that nets
    frozen-vs-current at the pre-swap instant into the instance table's
    counters + del/add queues. The EXACT effect multiset is D1's design
    deliverable (hand-write the witness IR first) — but it MUST be a fixed
    multiset per op so V-INST-EFFECT (the V-AGG-EFFECT analog) can assert
    totality.
  + EffKind: whether the frozen-pair fold/emit/old need a NEW EffKind FAMILY
    (as R3 added kStateFold/kStateEmit/kStateOld) or reuse the statecell
    family. R-A dissolves the scalar Sealed (its Seal is a POINTER SWAP, not a
    scalar fold — consolidated §3 R-A store), so the store is a NEW peer, but
    the FLOW effects (drain frontier, fold into current, emit touched, read
    frozen) MAY map onto the existing kStateFold/kStateEmit/kStateOld
    semantics. D1 decides; if a new EffKind, add the linearizer effect→access
    case (DR.cpp:3206-3246) or hazards go unmodeled (lane-drir.md §4).
  + CHAIN-BREAKER: the instance view is a branch chain-breaker like the
    agg/KV view (SuffixesOf stops at it, lane-drir.md §1) — mint AFTER
    products, BEFORE branches, assert no branch traverses it.

### D1.3 Census from the query-side annotation count (E-27/E-28)

DELTA against lane-drir.md §2 (the P0 census):
  + add an INDEPENDENT recount from the Query-side instance accessor. Per E-27,
    recount from the annotation side table (the demand pass's guard-site
    marks with is_instance_key=true), NEVER from `flow.ops`/`flow.instances`
    (that would compare flow against itself). The recount iterates the
    is_instance_key guard sites (the kReadAtTuple/kPushDown body guards),
    deduped by (instance_view, key-col multiset) — one instance per demanded
    p^α.
  + a count `expect(kSubgraphInstantiate, want, ...)` and `expect(kInstanceSeal,
    want, ...)` == the recount.
  + an order-free key multiset keyed on STABLE identity (instance table ptr,
    provenance, demand-view id, key-col tuple, view.UniqueId()), NEVER a
    mint-order instance_id (E-28: statecell_id is DELIBERATELY excluded from
    census keys, DR.cpp:2830-2832 — copy that discipline).
  + V-INST-EFFECT (the V-AGG-EFFECT analog): every kSubgraphInstantiate carries
    EXACTLY its fixed effect multiset; every kInstanceSeal exactly its seal
    effect.
  + V-INST-PAIR (the V-AGG-PAIR analog): instance_id is a bijection
    INSTANTIATE↔SEAL onto [0, |instances|) — a STRUCTURAL check, not a key
    equality (lane-drir.md §2 E-28).
  + a Site-5-style coverage check IF any instance emission is hand-coded
    (like the ingest hole). If SUBGRAPH_INSTANTIATE is FULLY DR-lowered
    (`LowerSubgraphInstantiate`, the LowerGroupUpdate mold), it needs NO
    Site-5 analog — totality is V-INST-EFFECT + census (lane-drir.md §3). The
    R-A store emission (frozen-pair swap) SHOULD be fully DR-lowered; keep it
    off the hand-coded eager web (the (E-42) principle: hand-coded surface is
    the exception, not the rule).

### D1.4 The TWO-LOWERINGS EQUIVALENCE GATE

DELTA (§18.5(C), consolidated §5 gap — NOT exercised by any lane):
The flat lowering (D4, landed) and the nested lowering (D1) of the same p^α
MUST be answer-identical. Gate SHAPE (D1 design deliverable, the consolidator
flagged no wiring exists):
  - the SAME witness (demand_tc_witness, and the D3 four-adornment tc) is
    compiled under BOTH lowerings — flat (a `-demand` mode as today) vs nested
    (a NEW mode selector, e.g. `-demand-instance` or a per-case flag). Both
    are `.drflags`-driven; neither is a 5th GOLDEN optimization mode (the 4
    modes stay orthogonal, as -demand does).
  - the ORACLE (bin/Oracle) referees the SAME `.batches` under both lowerings:
    the definitional per-key answer must match flat == nested == oracle. The
    oracle runs the PLAIN .dr (demand changes materialization, never answers —
    consolidated §3 harness), so the oracle is the SHARED referee both
    lowerings must equal.
  - IMPLEMENTATION NOTE: the equivalence gate is NOT a golden byte-compare
    (the two lowerings emit DIFFERENT code — different tables, different
    regions). It is an ANSWER-identity gate: run both, diff runtime stdout,
    both == oracle. This is a new harness mode (a two-lowering diff), which
    the consolidator flagged as unbuilt (§5). D1 designs it; D2 wires it.

### D1.5 The acyclic-DEMAND fence — ViewSelfReachable THROUGH the instance

DELTA (§18.5(C), consolidated §5 — the primitive exists, the "through the
instance" predicate is NEW):
`ViewSelfReachable` exists (Build.cpp:201, used :1258 for the acyclic-@product
fence — CONFIRMED by the consolidator). The fence for instances is DIFFERENT
(DemandSeeds.md §2.3 note 1): the condition is ACYCLIC DEMAND
(ViewSelfReachable of the DEMAND VIEW through the instance), NOT non-recursive
CONTENT. A bf-tc instance has acyclic demand (the demand frontier does not
demand itself) but RECURSIVELY-DERIVED content (Rederive = a fixpoint per
demanded key). So:
  - the fence tests: does the DEMAND view (d_reader) reach ITSELF through the
    instance boundary? If yes → cyclic demand → clean reject (the instance's
    own demand depends on its own answer, unstratifiable). If no → acyclic
    demand, even if the CONTENT is recursively derived → ALLOWED.
  - "through the instance" is a NEW predicate: ViewSelfReachable restricted to
    paths that traverse the SUBGRAPH_INSTANTIATE boundary. D1 builds it (the
    consolidator: "a NEW predicate to build"). Hand-write the witness that
    distinguishes acyclic-demand-recursive-content (bf-tc, ALLOWED) from
    cyclic-demand (a self-demanding query, REJECTED) before coding.

### D1.6 The demand-retract = instance-death arm (frozen-pair partition)

DELTA (DemandSeeds.md §2.3 note 2; §18.5(B) partition):
@differential demand is RETRACTABLE; a demand-retract IS instance death
(demand-add = birth, demand-retract = death, the unlock payload = the instance
key). In the R-A frozen-pair store:
  - the `publish_touched` band nets frozen-vs-current membership at ONE instant
    pre-swap (the F2 partition; (T,T) emits nothing).
  - the death arm: demand-retract empties `current` (death = −frozen rows) —
    the demand frontier's net-REMOVAL drain (sign −1) into the instance store
    drives the current-table teardown; publish_touched then emits −old for the
    now-absent keys.
  - this rides the EXISTING acyclic claim/frontier/commit tail (the same tail
    GROUP_UPDATE rides — lane-drir.md §1 band (b)). No new fixpoint machinery.
  - V-INST-FRESH (the paper's assert, §18.5(B)): `current` empty at rebuild
    entry. This is a runtime assert in the emitted store (survives NDEBUG,
    the always-on validator idiom).

### D1.7 E-53's second-half obligation — the flat guard excision / sole-writer

DELTA (E-53, consolidated §2): §18.1's "D3-F6 hole closed by construction"
covers ONLY the recognizer half (genuinely dissolved — the pass mints the
guard join, out_to_in carries BOTH inputs as real column edges,
Demand.cpp:150-191/AddUse :177-178). F6's SECOND half is D1/D2 emission work:
  - the instance STORE must be REACHED (the SUBGRAPH_INSTANTIATE op is the sole
    writer of the instance's outer table).
  - the flat guard JOIN must be EXCISED / not-emitted-beside the instance write
    (d3 §5.3 bet-B, the V-INSTANCE-SOLE / sole-writer check). Under the NESTED
    lowering, the flat d_p⋈p guard copy is REPLACED by the instance store — it
    must NOT ALSO emit the flat guarded copy (that would double-derive and
    break sole-writer).
  - ADJUDICATION: D1 must decide whether the nested lowering REWRITES the guard
    JOIN (excise the flat copy, redirect to the instance store) or emits the
    instance store BESIDE and dead-codes the flat copy. bet-B (sole-writer,
    excise) is the ratified direction. The V-INSTANCE-SOLE validator asserts
    exactly one writer of the instance outer table (the SUBGRAPH_INSTANTIATE
    op) — a reintroduced flat guard writing the same table aborts.

### D1.8 PRE-REGISTERED PREDICTIONS (D1 is design-only, but the design implies)

  - DRIVER CHURN: the NESTED lowering needs NO new driver contract for
    demand_tc_witness (the demand seed is driver-transparent — the injector
    is compiler-minted, lane-fabric-inject.md §4). PREDICTION: zero driver
    churn for the existing demand witness. (An instance store with driver-
    supplied reduction bodies — like the aggregate C-5 free functions — is
    NOT expected for a plain demand instance; the "reduction" is set-union
    membership, not an algebra functor. If a future keyed-instance over an
    aggregate needs free functions, that is D-later, not D1.)
  - GOLDEN CHURN: D1 is design-only → zero. The nested lowering, when it lands
    (D2), adds a NEW harness mode (the two-lowering equivalence gate), not a
    new golden on the 168.
  - SUITE COUNT: unchanged at 168 through D1 (design). D2 may add a directed
    nested-instance witness (oracle-blessed only).
  - Q5: no measurement at D1 (design). The nested lowering's perf claim (fewer
    materialized rows per demanded key vs the flat interleaved table) is a
    MEASURE-FIRST D2 obligation, not a D1 prediction.

--------------------------------------------------------------------------
## D2 The instance emission — acyclic-frozen-first, census day one

D2 is the EMISSION of the D1 design: SUBGRAPH_INSTANTIATE + INSTANCE_SEAL
lower end-to-end, acyclic-frozen first, with the census live from the first
commit (§18.2 D2).

### D2.0 The delta

  + `LowerSubgraphInstantiate` (the `LowerGroupUpdate` mold, Stratum.cpp:
    1366-1426): mint the instance control-flow region, wire the demand-frontier
    drains, the store's del/add queues, the key/free column positions
    (lane-drir.md §4 Lowering dispatch).
  + the R-A store Runtime surface: THREE new items (consolidated §3 R-A store):
    `MakeTable` (the MakeVec twin), the `InstanceStore` class, and
    `Table::Reset()` (in-place empty). **Table::Reset() is a SOUNDNESS
    requirement, not perf residue** — Arena::Free is a no-op (Allocator.h:51),
    so R-A's teardown+reconstruct Recycle LEAKS one Table's arrays per epoch
    per rebuilt instance under an Arena. INVISIBLE to the corpus witness (all
    drivers use MallocAllocator whose Free actually frees), so the leak will
    NOT show as a test failure — the review gate must CHECK Reset() is wired,
    not rely on the suite catching it.
  + the StateCell-descriptor-publish analog (Stratum.cpp:2169-2211): one
    ProgramInstance per DR instance published for codegen.
  + codegen: EmitSubgraphInstantiate (the EmitGroupUpdate mold, Database.cpp:
    2009+), the store struct plumbing (EmitStateCellStructs mold, Database.cpp:
    1077+ — the `instance_<id>` store member constructed with the allocator,
    passed as ref params to touching procs), the INSTANCE_SEAL tail on
    COMMITSWEEP (the SealStateCellId mold, Database.cpp:2316-2324 — a
    pointer-swap Seal, not a scalar).
  + the control-flow region class chain (lane-drir.md §4 codegen): a new
    region-impl class + ProgramOperation enum member + AsSubgraphInstantiate
    virtual + visitor overload + public wrapper (mirror ProgramGroupUpdateRegion,
    Program.h:1068-1123 / :802-838).

### D2.1 Acyclic-frozen-first (the fence from D1.5)

The FIRST emission slice is acyclic-DEMAND only (D1.5 fence). A cyclic-demand
instance cleanly rejects (unbuilt fence path). "Frozen-first" = the frozen-pair
Seal (pointer swap) is the batch-boundary snapshot; the current-table rebuild
(Rederive = fixpoint per demanded key for recursive content) rides the existing
per-stratum OVERDELETE→REDERIVE→INSERT tail. No new fixpoint machinery — the
recursive CONTENT reuses the differential table machinery; only the STORE
boundary (frozen pair) is new.

### D2.2 Census day one (the A7/NF1 honest scope)

The census (D1.3) lands WITH the first emission commit, not after. V-INST-EFFECT
+ V-INST-PAIR + the count/key-multiset checks are always-on (fprintf+abort,
survive NDEBUG). A partial instance lowering that mis-mints an op aborts at
compile — never miscompiles silently.

### D2.3 PRE-REGISTERED PREDICTIONS

  - DRIVER CHURN: demand_tc_witness driver UNCHANGED (demand seed is
    driver-transparent). PREDICTION: zero driver churn. A NEW directed
    nested-instance witness (if added) needs a driver, but that driver mirrors
    the demand_tc_witness shape (query call + drain), NOT a new contract.
  - GOLDEN CHURN (the 168): zero on existing goldens. A new nested-instance
    witness case (if added) lands its stdout golden oracle-BLESSED, and its
    two-lowering equivalence gate (flat==nested==oracle) is a NEW harness
    mode, not a golden byte-compare.
  - SUITE COUNT: 168 → 169 IF a directed nested-instance witness is added
    (oracle-blessed). Otherwise 168 (the equivalence gate rides
    demand_tc_witness + the D3 four-adornment witness). RECOMMEND: add ONE
    directed witness (e.g. `demand_instance_tc`) so the nested path has a
    named regression carrier.
  - Q5 / FLAGSHIP: the nested lowering's COST claim (fewer materialized rows
    per demanded key vs the flat interleaved shared table) is MEASURE-FIRST.
    Expectation: on a demand witness where the flat table interleaves many
    demanded keys, the nested store materializes only the per-key sub-relation
    — a WIN in materialization, possibly a LOSS in per-key store overhead.
    SAME-SESSION ABABAB flat-vs-nested on the demand COST witness (the
    scratchpad spike promoted to bench/, §18.1 close-sweep leftover). NO claim
    pre-measurement; if nested is slower on the witness, that is a recorded
    honest COST result, not a gate failure (the gate is answer-identity).

--------------------------------------------------------------------------
## D3 The multi-adornment lift — per-adornment registries, the four-adornment tc

D3 lifts the single-adornment slice: >1 binding pattern per query name goes
from clean-reject to a per-adornment family (§18.2 D3, §18.5(D)).

### D3.0 The delta

  - the >1-binding-pattern rejects NARROW to: one d_p^α + one guarded family
    PER adornment. The three current rejects (lane-pipeline.md, verified by
    the consolidator §5): Demand.cpp:434 (top-level patterns.size()!=1),
    :645-649 (kReadAtTuple in_col->Index()!=pos), :700-703 (kPushDown
    found_col->Index()!=pos). D3 lifts the TOP-LEVEL reject (:434) to iterate
    adornments; the two per-site rejects stay as within-adornment position
    checks (they are correct multi-COLUMN checks, not multi-adornment).
  + per-adornment REGISTRIES: the BindingPattern key ALREADY lands the identity
    (lane-fabric-inject.md §2d: the injector match requires entry.query==query
    AND BindingPattern() equality, :467-469 — "the belt against cross-wiring
    two adornments"). So `demand_forcings` becomes NATIVELY per-adornment: one
    QueryDemandForcing per (query, adornment), and the injector's existing
    BindingPattern belt already disambiguates them. D3's registry lift is
    SMALL — the belt is already there; what changes is the demand pass mints N
    demand families instead of rejecting when N>1.
  + the four-adornment tc witness (E-41: {bf,fb,ff,bb}): a compiling tc with
    all four adornments demanded. This witness does NOT exist and was NOT
    constructed (consolidated §5) — D3 builds it. It exercises N=4 demand
    families over one recursive tc relation.
  + the raw-seed-vs-d_p guard DIVERGENCE goes LIVE (§18.5(D), E-52). The d4s3
    F1 coincidence argument (raw_seed and d_reader happen to coincide in the
    single-adornment case) NO LONGER holds sideways with >1 adornment: each
    adornment α has its OWN d_reader_α (body demand side) AND its own raw_seed_α
    (query-projection demand side, the top-level bound argument for that
    adornment). The two-demand-sides fact (E-52) is what makes them distinct
    per adornment. This is CONFIRMED as a real two-demand-side fact
    (consolidated §5: "the seed's claim that it goes live at D3 is
    well-founded").

### D3.1 Interaction with D1 instance keys

Under D1's nested lowering, multi-adornment = DISTINCT key spaces
(DemandSeeds.md §2.3 note 1: "multi-adornment = distinct key spaces — one
store per adornment or a union key"). PREDICTION/RECOMMEND: one InstanceStore
PER adornment (the cleaner boundary; the union-key alternative conflates
distinct α), matching the per-adornment registry. So D3 (multi-adornment) and
D1/D2 (nested instance) COMPOSE: N adornments → N demand families → N instance
stores. The census recount (D1.3) already keys on the (instance_view, key-col
multiset) which distinguishes adornments — no census change needed for the
multi-adornment case beyond the recount naturally counting N.

### D3.2 PRE-REGISTERED PREDICTIONS

  - DRIVER CHURN: the four-adornment tc witness is a NEW case with a NEW
    driver (four query entry points, each drained/sorted). Mirrors the
    demand_tc_witness driver shape × 4 adornments. Zero churn on EXISTING
    drivers.
  - GOLDEN CHURN (the 168): zero on existing. The four-adornment witness lands
    its stdout golden oracle-blessed; it is demand-ON via `.drflags` (like
    demand_tc_witness). demand_multi_adorn_1 (the current >1-adornment REJECT
    witness) FLIPS from diagnostic to a compiling golden IF the same program
    is used — RECOMMEND keeping demand_multi_adorn_1 as a distinct
    still-rejecting case (a DIFFERENT unsupported shape) and adding the
    four-adornment tc as a NEW case, so the reject-witness stays a reject
    (the suite's diagnostic-list invariant, runall.sh, stays stable). If
    demand_multi_adorn_1's specific program becomes supported, it MOVES from
    the diagnostic list to a golden — a runall.sh edit + a bless, enumerated.
  - SUITE COUNT: 168 → 169 (the four-adornment tc witness) at minimum; +1 more
    if a nested-instance multi-adornment witness is added. The
    demand_multi_adorn_1 disposition (keep-as-reject vs flip) is an owner call
    (open decision).
  - Q5 / FLAGSHIP: no new perf claim at D3 (the lift is a capability, not a
    perf change on existing witnesses). The four-adornment witness is a
    correctness/capability carrier.

--------------------------------------------------------------------------
## D4 Implicit-asynchrony seams — design + judge FIRST, termination as the R4-style gate

D4 is the owner's implicit-asynchrony direction: the compiler introduces
asynchrony seams ITSELF where profitable (DemandSeeds.md §2.3 note 3). DESIGN
+ JUDGE FIRST; the termination argument is the R4-style gate; MEASURE-FIRST
(§18.2 D4, §18.5(E)).

### D4.0 The delta — E-50-corrected

  + a seam = an internal fabricated message (the (A) registry mechanism,
    ABI-suppressed, lane-fabric-inject.md §3) + an append site in the eager web
    + a drain re-entering the detail proc; placed where the transform's own
    analysis says PROFITABLE (adornment/SIP/cost, measure-first — NOT a
    user surface).
  + cross-batch: the epoch loop self-pumps internal queues; termination = the
    finite demand lattice (zero-crossing one level up).
  + DR-IR home — E-50 CORRECTED: the seed's "v3-spec §2 reserved-sub-domain
    pattern" is the WRONG precedent. A seam's append/drain IS the EXISTING
    `kVecAppend`/`kVecDrain` sub-domain (DR.h:74-76) — NO new EffKind. The
    cross-batch carried role is a NEW `VecRole` (DR.h:50-63) classified in
    `is_epoch_carried_role` (DR.cpp:3504-3519) + a `loop_carried` dep-edge
    under V-LOOP. The RIGHT precedent is B-10 (the live epoch-carried
    delete/add-queue mechanism), NOT the statecell EffKind family. Following
    the cited (wrong) precedent would reserve a needless EffKind
    (consolidated §1, E-50).

### D4.1 The termination argument (the R4-style gate — design FIRST)

The self-pump's termination is the DESIGN GATE, judged before any seam code
(§18.5(E), consolidated §5 — design-only, no seam code to verify against):
  - within a batch: seam deferral is ORDINARY deferred-vector phasing (the
    batch is already the async quantum, observation-invisible —
    DemandSeeds.md §2.3 note 3).
  - across batches: a self-pump in the epoch loop (drain internal queues,
    re-enter, repeat). Termination = the FINITE DEMAND LATTICE: the demand
    lattice is finite (bounded by the reachable demand keys), and each pump
    round strictly advances it (zero-crossing one level up — the same
    argument as the differential fixpoint's zero-crossing termination). D4's
    design deliverable is this argument MADE RIGOROUS: the seam token's
    demand lattice, why each round advances, why it hits a fixed point.
  - the loop_carried seam vec is validated by V-LOOP (every loop-carried edge
    is a RAW with a matching intra-scope WAR, drain-before-refill —
    lane-drir.md §3): the seam's cross-batch queue enters the CHECKED model,
    never the hand-coded web (the E-42 discipline).

### D4.2 The CONTRACT constraint (unlock vs demand)

An implicitly-introduced seam MUST preserve DEMAND semantics (the compiler
owes the seeding that makes it answer-identical) or be observation-invisible
(DemandSeeds.md §2.3 note 3). Observation points = queries AND published
deltas. Within a batch, deferral is invisible; across batches it changes
observable publication TIMING and needs a declared contract (the unlock-vs-
demand contract, §2.3 note 2 — a D-later surface-declaration item,
consolidated §5). D4's seams are DEMAND-semantics-preserving by construction
(the compiler seeds the closure); an UNLOCK-semantics seam would need the
declared surface (out of D4 scope).

### D4.3 In-scope-or-reseed adjudication (open owner decision)

D4 is gated on the termination argument surviving judge AND on measure-first
showing a profitable seam on a real witness. The consolidator flagged the
whole seam surface as design-only with no code (§5). RECOMMENDATION: D4 is
IN-SCOPE for DESIGN + JUDGE this epoch (the termination argument, the DR-IR
home per E-50, the profitability trigger), but the EMISSION of a seam is
gated on a measured-profitable witness — if none surfaces, D4 emission
RESEEDS to the next epoch (design ratified, emission deferred). This mirrors
the demand-seeds epoch's D-later deferrals. Owner ratifies (open decision d).

### D4.4 PRE-REGISTERED PREDICTIONS

  - DRIVER CHURN: zero — seams are ABI-suppressed (the (A) registry keeps
    injector twins off the public ABI, lane-fabric-inject.md §3; the forcer
    proc is self-injection in miniature, DemandSeeds.md §2.3 note 3). A seam
    is driver-INVISIBLE by contract.
  - GOLDEN CHURN (the 168): zero — a seam that changes an ANSWER is a bug
    (the contract is answer-identity or observation-invisibility). A seam
    changes MATERIALIZATION/TIMING, not published answers within a batch. If
    a golden's stdout moves, the seam violated its contract — STOP.
  - SUITE COUNT: 168 unchanged through D4 design. A directed seam witness (if
    emission lands) is oracle-blessed, +1.
  - Q5 / FLAGSHIP: MEASURE-FIRST is the whole point — a seam lands ONLY on a
    measured-profitable witness. No pre-measurement claim. The seam's value is
    a COST result (fewer eager rows materialized before a demand is known),
    measured SAME-SESSION ABABAB seam-on vs seam-off. If no witness shows
    profit, no seam emits (D4.3 reseed).

--------------------------------------------------------------------------
## Recommended CUT ORDER (owner ratifies or re-ranks)

Rationale keyed to dependencies and the do-first discipline:

  1. **T1 (rename lib/DR → lib/DeltaRel).** FIRST — mechanical, byte-identity-
     gated, the fleet is done reading lib/DR. Everything DR-IR-touching
     (D1/D2/D4) lands at the final path. Cheap, unblocks nothing but pays down
     path churn before the epoch touches DR heavily.
  2. **T2 (the two dumps, -deltarel-out + -df-out).** SECOND — the dumps are
     the (F) determinism DISCRIMINATORS and the restored gate surface. They
     must exist before (F)'s gate can be stated. -deltarel-out rides the T1
     path. The dumps' OWN determinism is a POST-(F) property (they EXPOSE the
     nondeterminism until (F) fixes it — that is the feature).
  3. **(F) the determinism fix.** The epoch's do-first CORRECTNESS diff
     (§18.2 D0/§18.5(F)) — LANDABLE IN PARALLEL with T1/T2, not strictly third.
     Fix the Induction.cpp:520 ITERATION (not just the WeakUseList — (F).1).
     This UNBLOCKS everything: the demand-ON gate is restored, IR goldens
     become valid, the pass-harness bisect becomes sound. GATING: (F) lands
     gated INITIALLY by the already-used scripted `-ir-out`/`-h` hash-sweep
     (the interim acceptance surface, KeyedInstances.md §1); the T2 dumps +
     T3 sidecars become the PERMANENT regression gate that supersedes the
     script (the ledger's point — the one-off uncommitted check becomes a
     committed gate, consolidated §3 harness). So (F) is NOT calendar-blocked
     on T1/T2; only its PERMANENT gate is.
  4. **T3 sidecars (the demand-ON IR+header goldens) = (F)'s gate.** Lands
     WITH (F)'s acceptance (blessed as the restored gate). The pass-harness P1
     is LOWER priority (infrastructure, non-blocking) — land after (F) or
     defer to a quiet slot; P2-P5 are out of do-first scope.
  5. **D1 (instance design + judge).** After (F)+T2+T3 (the IR review
     discipline, directive 3, is now MECHANIZED — D1's witness IR is
     reviewable across .df/.ir/.deltarel). Design-only; judge before any
     emission code.
  6. **D2 (instance emission, acyclic-frozen-first, census day one).** After
     D1 survives judge and the witness IR is hand-written. The Table::Reset()
     SOUNDNESS item (D2.0) is a review gate, not a suite gate (the leak is
     corpus-invisible under MallocAllocator).
  7. **D3 (multi-adornment lift).** After D1/D2 (it COMPOSES with the nested
     lowering — N adornments → N stores, D3.1). Could interleave with D2 if
     the four-adornment witness is wanted as a D2 equivalence-gate carrier —
     owner's call. The registry lift is SMALL (the BindingPattern belt already
     lands identity, D3.0).
  8. **D4 (implicit-asynchrony seams).** LAST — design + judge FIRST (the
     termination argument, R4-style gate); emission gated on a measured-
     profitable witness (D4.3 in-scope-or-reseed). Design ratifiable this
     epoch even if emission reseeds.

[AMENDED: MEDIUM — the T1→T2-before-(F) HARD spine was self-undercut ((F).6
and the T2 sequencing both admit the (F) hunt runs in parallel, and the fleet
ALREADY gated the repro with a scripted -ir-out hash-sweep). Softened so a slip
in the rename/dumps does not calendar-block the correctness fix the epoch
depends on.]

The one REAL path dependency is T1 before T2's DeltaRel dump (the dump must
land at the final lib/DeltaRel path). (F)'s CODE is landable IN PARALLEL with
T1/T2: the root cause is already found by hand ((F).0), and (F) can land gated
INITIALLY by the already-used scripted `-ir-out`/`-h` hash-sweep
(KeyedInstances.md §1, the 6-runs/case sweep) as its interim acceptance
surface. The T2 dumps + T3 sidecars then become the PERMANENT regression gate
that SUPERSEDES the script once they land — (F)'s gate 1-2 are re-expressed
against the dumps at that point. So (F) is NOT calendar-blocked on the rename
or the two dump implementations; it is the epoch's do-first CORRECTNESS diff
(§18.5(F): "this diff gates the rest" — meaning the Dn all build on the
determinism-clean path), gated first by the interim script and permanently by
T2/T3. Everything after is dependency-ordered but owner-rerankable.

--------------------------------------------------------------------------
## OPEN OWNER DECISIONS

The (a)-(d) from the epoch charter (§18.2 / DemandSeeds.md §2.3), plus the new
ones this formulation surfaces:

  (a) THE UNLOCK-vs-DEMAND CONTRACT SURFACE (§2.3 note 2, consolidated §5):
      any explicit #subgraph/#demand surface must declare which contract it
      offers. Pure owner-charter direction, no code today. IS this epoch's
      scope (design the surface) or D-later? RECOMMEND D-later — D4's implicit
      seams are demand-semantics by construction; the explicit surface is a
      separate design.

  (b) THE TWO-LOWERINGS EQUIVALENCE-GATE SHAPE (D1.4, consolidated §5): the
      gate is answer-identity (flat==nested==oracle), NOT a golden byte-compare
      (the two lowerings emit different code). This needs a NEW harness mode (a
      two-lowering diff) that does not exist. Owner ratifies the SHAPE: a mode
      selector (-demand-instance vs -demand) driven per-case by .drflags, both
      refereed by the same oracle. Is a NEW mode acceptable, or should nested
      be the ONLY demand lowering (retiring flat)? RECOMMEND keeping BOTH (the
      equivalence gate is the correctness proof; flat may still win on some
      COST profiles — measure-first).

  (c) THE (F) SORT KEY (F.2) [AMENDED: HIGH — the firstColId tie-break rested
      on an inverted pipeline-order fact; re-decided to a col-id-free key]:
      primary `view->Sort()` (== Hash(), structural, run-stable) + tie-break
      the **incoming-view `Sort()` TUPLE** + (final, for a structurally
      indistinguishable CSE-duplicate) an **explicit build-order ordinal from a
      `ForEachView` sweep before the sort**. NOT `firstColId` — code-verified
      this session, `IdentifyInductions` (Build.cpp:2597) runs BEFORE
      `FinalizeColumnIDs` (:2603), so `col->id` at sort time is the PRE-finalize
      variable/lexical id (possibly non-unique across views), NOT the
      E-47-measured byte-stable POST-finalize `%col:N`. NOT raw VIEW id either
      (a Create-order artifact, wrong across the flag boundary — that rejection
      still holds). ALTERNATIVE the owner may pick: relocate the merge_set_id
      assignment to AFTER FinalizeColumnIDs so the finalized ids are the
      tie-break (a larger pipeline move, re-judge). Owner ratifies the key; the
      review gate MUST confirm it is a TOTAL order on the witness set (a partial
      order re-opens (F)). ALSO: the fix locus is the ITERATION at :520
      (stabilizes merge_set_id AND related_merges in one move), NOT the
      consolidator's post-population WeakUseList sort alone (which leaves
      merge_set_id pointer-keyed — F.1). Owner ratifies the LOCUS correction.

  (d) D4 IN-SCOPE-OR-RESEED (D4.3): D4 design + judge (termination argument,
      DR-IR home per E-50) is in-scope this epoch; D4 EMISSION is gated on a
      measured-profitable seam witness — if none surfaces, emission reseeds to
      the next epoch (design ratified, emission deferred). Owner ratifies
      in-scope-design / gated-emission, or pulls D4 wholly to next epoch.

NEW decisions this formulation surfaces:

  (e) THE WITNESS SURFACE CHOICE (D2.3, D3.2): does the epoch ADD directed
      nested-instance / four-adornment witnesses to the suite (168 → 169+,
      oracle-blessed), or ride the equivalence gate on the existing
      demand_tc_witness only? RECOMMEND adding at least (i) one nested-instance
      witness (demand_instance_tc) as the nested-path regression carrier and
      (ii) the four-adornment tc (E-41) — both oracle-blessed, demand-ON via
      .drflags. Owner sets the count.

  (f) demand_multi_adorn_1 DISPOSITION (D3.2): when D3 lifts multi-adornment,
      does the current >1-adornment REJECT witness (demand_multi_adorn_1) FLIP
      to a compiling golden, or stay a reject (a still-unsupported DIFFERENT
      shape)? RECOMMEND keep it a reject (preserve a still-unsupported witness;
      add the four-adornment tc as a NEW compiling case) so runall.sh's
      diagnostic list stays stable. Owner ratifies the flip-or-keep.

  (g) DUMP / PASS-HARNESS SEQUENCING (T2/T3): the pass-harness P1 is
      infrastructure the epoch benefits from but does not block on. RECOMMEND
      P1 lands AFTER (F)+T2+T3-sidecars (non-blocking); P2-P5 are explicitly
      next-epoch. Owner ratifies P1's priority (this-epoch-nonblocking vs
      next-epoch).

  (h) kBaseAtom AS NON-INSTANCE-KEY (D1.1(a)): the adjudication that a base-rule
      guard site is a demand-GUARD but NOT an instance-key site (the
      SUBGRAPH_INSTANTIATE keys off the recursive-subgoal reads only). And the
      query-projection guard is NOT an instance-key site (D1.1(b), the outer
      reader not a writer). Owner ratifies the instance-key TAXONOMY (which
      guard sites become instance boundaries) — it is load-bearing for the
      census recount (D1.3) and the sole-writer check (D1.7).
