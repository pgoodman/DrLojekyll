# Stage 5 working notes: differential @product (committed working ledger)

Branch: differential-product (off main at e20b3c7, Stage 3 CLOSED, suite
151 zero-red). Deliverable: plan.md Stage 5 — lift the F14 "Cross-products
over differential (deletable) data" diagnostic for the ACYCLIC case and
emit the 0-pivot differential join through the stratum phases; promote
conditions_to_bools into the compiling corpus. Method: the checkpoint
method — this file IS the pre-code prototype (owner-mandated: pseudocode +
planned DF/CF IR first, reviewed, then implemented). §1–§4 were reviewed
by a three-agent verification pass (opus design critique, opus IR
trace + empirical oracle probes, sonnet 10-check mechanical audit) BEFORE
implementation; §5 records their verdicts as the review ledger.

## 1. DF IR of conditions_to_bools (hand-derived — Stage 5 changes NO dataflow)

Stage 5 is control-flow-build-only: the dataflow IR already represents
differential products (the oracle interprets it today, §5.4). Hand-derived
node graph (VERIFY against -dot-out at implementation time; the dot dumper
runs after Program::Build, so it is only reachable once the diagnostic is
narrowed — first implementation step includes `diff` of this section
against the real dot):

    RECEIVE enable_feature(u32 X)
      ├─ SELECT →(X=1 path) CMP/const → INSERT foo   « unit relation (bool) »
      └─ SELECT →(X=2 path) CMP/const → INSERT bar   « unit relation (bool) »
    foo  —SELECT→ ┬─(true-arm)  TUPLE{X:=true}  ────┐
                  └─(false-arm) NEGATE !foo ← unit  ├─ MERGE foo_enabled(bool)
                                TUPLE{X:=false} ────┘
    bar  —(same shape)—————————————————————————————— MERGE bar_enabled(bool)
    JOIN[0 pivots, @product] (foo_enabled × bar_enabled)
      → INSERT (transmit) enabled_features(bool,bool) @differential

Deletion-capability: foo/bar only ever GAIN (enable_feature is add-only),
but the negation arms make foo_enabled/bar_enabled retractable (the false
row dies when the condition turns on) ⇒ both sides CanReceiveDeletions ⇒
the JOIN CanReceiveDeletions ⇒ today's blanket pre-pass rejects it.
No recursion anywhere: the JOIN carries NO InductionGroupId (audit-
confirmed shape; §5 fence).

## 2. The product arms as a diff against the whole-program pseudocode

Against checkpoint-e-notes.md §1 (the Stage-3-close skeleton), inside
"(1) SEEDS (hoisted)", after the crossovers:

    (1) SEEDS:
        branch chains ...                             « unchanged »
        dual-section joins ...                        « unchanged »
        CROSSOVER (one per non-@never NEGATE) ...     « unchanged »
    +   PRODUCT ARMS (one set per DIFFERENTIAL 0-pivot JOIN, acyclic only):
    +     sides S_1..S_k in canonical JoinedViews() order.
    +     for each side i:
    +       + arm (always): loop S_i.net_additions {S_i cols}
    +       − arm (iff S_i differential): loop S_i.net_removals {S_i cols}
    +       each arm, either sign:
    +         nested full scans of every other side j (zero bound cols —
    +         the §3.4 BuildMaybeScanPartial degenerate):
    +           CHECKMEMBER  j<i → InNew(S_j)   « batch-final-so-far »
    +                        j>i → InI(S_j)     « batch-frozen »
    +           « POSITION-KEYED, SIGN-INDEPENDENT — §5 trap T1 »
    +         → ONE fold: update-count ±class {product row} into the
    +           PRODUCT'S OWN table (class = RuleClass(sccs, product_table,
    +           side_tables); kNonRecursive in the acyclic fence)
    +           → if-crossed → the product table's delete/add queue
    +     emitted in the seed block BEFORE all claim drains (phantom +
    +     dropped by TryClaimAdd's Total>0 re-test requires every arm fold
    +     hoisted ahead of the product table's OVERDELETE drain — §5 T2).
        (2) SINGLE-PASS tables: ClaimDrain, FrontierFilter  « the product
            table drains here like any differential table — this supplies
            the phantom-dropping claim gates (P1) »
        (3) recursive-SCC tables: ...                 « unchanged; 0-pivot
            joins are FENCED out of recursive SCCs this stage »

Exactly-once (§5.1 with zero shared columns, verified case-by-case in the
review): +a/+b fires once via the LAST changed position (A-arm sees b∉InI,
B-arm sees a∈InNew); −a/−b once via the FIRST (A-arm sees b∈InI, B-arm
sees a∉InNew); +a/−b makes a phantom pair on (a_new,b_old) — the − lands
in OVERDELETE, the + in INSERT, TryClaimAdd's Total>0 re-test drops the
restored row, nothing leaks (identical to the F18 crossover phantom);
one-side-only and OQ3-annihilated batches degenerate correctly; a k=3
middle-position arm reads j=1 at InNew AND j=3 at InI simultaneously.

## 3. Planned CF IR for conditions_to_bools (concrete; dump vocabulary)

Tables: %FOO (unit foo), %BAR (unit bar), %FE (foo_enabled, differential,
2 possible rows), %BE (bar_enabled), %PROD (the product view's table,
differential, backs the @differential transmit via commit_published_view).
The four arms in ^flow, seed block, after any negate crossovers:

    ; side 1 = foo_enabled (position 1), side 2 = bar_enabled (position 2)
    vector-unique $net_additions<FE>
    vector-loop {@F} over $net_additions<FE>          ; + arm, side 1
      scan %BE full                                    ; zero bound cols
        check-member in-I {@B} in %BE                  ; j=2 > i=1 → InI
          update-count +nonrecursive {@F,@B} in %PROD
            if-crossed → vector-append into $add_queue<PROD>
    vector-unique $net_removals<FE>
    vector-loop {@F} over $net_removals<FE>           ; − arm, side 1
      scan %BE full
        check-member in-I {@B} in %BE                  ; SAME predicate: sign-independent
          update-count -nonrecursive {@F,@B} in %PROD
            if-crossed → vector-append into $delete_queue<PROD>
    vector-loop {@B} over $net_additions<BE>          ; + arm, side 2
      scan %FE full
        check-member in-new {@F} in %FE                ; j=1 < i=2 → InNew
          update-count +nonrecursive {@F,@B} in %PROD ...
    vector-loop {@B} over $net_removals<BE>           ; − arm, side 2
      scan %FE full
        check-member in-new {@F} in %FE                ; sign-independent
          update-count -nonrecursive {@F,@B} in %PROD ...
    ; then the standard machinery, already emitted for any differential
    ; table: claim-del/claim-add drains of %PROD, net-deleted/net-added
    ; frontier filters, and commit-sweep %PROD publishing enabled_features/2.

Hand-traced against the driver batches (review §5, verdict PASS):
b1 +enable_feature(1) → publishes exactly {remove (false,false),
add (true,false)}; b2 +enable_feature(2) → {remove (true,false), add
(true,true)}; b3 idempotent re-add → all four arms loop empty frontiers,
NO publish. Same-batch DOUBLE flip (both conditions in one epoch):
(true,false) receives +1 (arm A: BAR@InI∋false) and −1 (arm D:
FOO@InNew∋true) — cancels, was==now, commit publishes only
{remove (false,false), add (true,true)}: no transient. Arm D's InNew read
of the moved other side is what finds the −1's match; a uniform-InNew or
sign-keyed emitter breaks exactly here (T1).

## 4. Implementation pseudocode (diffs against Stratum.cpp / Build.cpp)

(a) Build.cpp pre-pass (~:996) — NARROW, don't delete:

    for (auto join : query.Joins()) {
      QueryView v(join);
      if (join.NumPivotColumns() || !v.CanReceiveDeletions()) continue;
    -   log.Append() << "Cross-products over differential ... not supported";
    +   if (v.InductionGroupId().has_value())   « conservative fence: the
    +     recursive-SCC closure (ComputeRecursiveSCCs) does not exist at
    +     pre-pass time; a 0-pivot join on ANY recursive cycle carries a
    +     group id (IdentifyInductions runs before Stratify, both inside
    +     Query::Build) »
    +     log.Append() << "Cross-products over differential (deletable)
    +       data inside recursive cycles are not yet supported";
    }

(b) Build.cpp FillDataModel — pre-code check first: a 0-pivot product gets
NO table from the `num_used_pivots < num_pivots` branch (0<0). Expected:
the @differential transmit's INSERT is deletion-receiving, so its
predecessor (the product view) is persisted by the every-predecessor rule
(Build.cpp:42-46). VERIFY empirically post-(a); if the product view's
model has no table, add `TABLE::GetOrCreate` for 0-pivot differential
joins beside the join-input persistence loop.

(c) Stratum.cpp — mirror CrossoverEmission end to end:

    struct ProductEmission {
      QueryView product_view;  TABLE *product_table;
      std::vector<QueryView> sides;          « JoinedViews() order = positions »
      std::vector<TABLE *> side_tables;      « assert all non-null at discovery »
      std::vector<bool> side_differential;   « TableIsDifferential per side »
      unsigned stratum;                      « lifted by the fixpoint »
    };
    discovery: for join in query.Joins():
      if join.NumPivotColumns() or not CanReceiveDeletions(join): continue
      assert !InductionGroupId(join)         « the (a) fence admitted it »
      collect; stratum = join.Stratum().value_or(0)
    scheduling fixpoint (the changed-loop, beside crossovers):
      for p in products:
        for st in p.side_tables: lift p.stratum ≥ ready_after(st)
        lift drain_stratum[p.product_table] ≥ p.stratum      « T2: every arm
          fold hoisted ahead of the product's own drains »
      + the mirror readiness asserts at emission, and
      strata.insert(p.stratum)
    emission (seed block, after crossovers, before claim drains):
      for p in products where p.stratum == stratum:
        assert drain_stratum[p.product_table] >= stratum
        EmitProductArms(p)

    EmitProductArms(p):                      « NEW emitter — modeled on
      for i in 0..k-1:                         EmitJoinFire's per-position
        for sign in {+, −}:                    scan_list, NOT on
          if sign == − and !p.side_differential[i]: continue   EmitCrossover's
          frontier = table_delta_vec(p.side_tables[i],         single fixed
                       sign==+ ? kNetAdditions : kNetRemovals) InNew read »
          VECTORUNIQUE(frontier); VECTORLOOP {side_i cols} over frontier
          region = loop body
          for j in 0..k-1, j != i:           « nested, canonical order »
            scan = BuildMaybeScanPartial(p.side_tables[j], avail=∅)  « full »
            region = CHECKMEMBER(j < i ? kInNew : kInI, S_j row) in scan
          bind product output cols via join.ForEachUse (as EmitJoinFire)
          UPDATECOUNT(sign, RuleClass(sccs, p.product_table, p.side_tables))
            {product row} in p.product_table
            if-crossed → VECTORAPPEND into product's delete/add queue
    « NO change to Product.cpp (eager path stays monotone-only: the
      Build.cpp:774 cut already makes BuildEagerProductRegion unreachable
      for differential products — audit-verified), NO change to the cut,
      NO kProductInput anywhere on this path (Q3(b) seam dissolved;
      verify post-landing: grep the emitted IR for kProductInput). »

(d) Monotone side (product_mixed shape): its eager arrival hits the :774
cut (any_cut_succ), the :804-809 boundary append fills its kNetAdditions
vec — which also enrolls it in Seal (Procedure.cpp:322-333) so the other
arm's InI read of it has a real watermark. Only a + arm exists for it.

## 5. Review ledger (pre-code verification pass, 2026-07-12)

- T1 (THE trap, F18-class): non-delta reads MUST be position-keyed
  (j<i InNew / j>i InI) and sign-independent. A uniform-InNew emitter
  (EmitCrossover's shape) double-counts same-batch both-sides-adds —
  C_nr=2 on a genuinely-new product row, silent until a later single
  retraction wrongly survives. EmitCrossover is the WRONG template for
  k≥2; EmitJoinFire's per-position scan_list is the right one. The
  same-batch-double-flip fixture batch is the discriminator.
- T2 (scheduling): the phantom-+ drop relies on TryClaimAdd's Total>0
  re-test, sound only if every arm fold precedes the product table's
  drains — the fixpoint must lift drain_stratum[product_table] above the
  (lifted) emission stratum, exactly as crossovers lift the negate table.
- Fence correction: RecursiveSccMap does NOT exist at pre-pass time; the
  conservative InductionGroupId().has_value() proxy is the fence (rejects
  every on-cycle 0-pivot differential product incl. non-induction-owned
  SCCs — correct, since EmitJoinFire is not generalized to 0 pivots here).
- P1: the product table must drain through the STANDARD EmitClaimDrain /
  EmitFrontierFilter (it does: differential + not induction-owned ⇒
  phase_table_order membership) — the claim gates are load-bearing.
- Table-backing: assert every side table and the product table non-null at
  discovery (crossover-style), don't null-deref.
- Cut: verified NO eager change needed (Build.cpp:774 cuts every
  CanReceiveDeletions successor before the :899 JOIN dispatch; :804-809
  handles the monotone-boundary frontier, consumer-kind-agnostic).
- Oracle (EMPIRICAL): drlojekyll-oracle referees differential products
  TODAY — conditions_to_bools 3-batch probe OK (727 assertions, invariant
  holds, monotone projection = 2), both-sides-differential probe OK (7
  batches, 898 assertions), 200-round stress seed=42 clean, mixed
  monotone×differential probe OK. LIMITATION: the oracle does not model
  publishing, so stdout goldens (the published-delta transcript) are
  blessed from the compiled driver only AFTER landing; oracle triads can
  be authored first.
- Docs debt found: Language.md:340's "abort in the control-flow build
  (assert + TODO)" is stale (F14 diagnostics since Round 2) — fix in this
  stage's docs touch alongside the gap-list updates.
- SELF-PRODUCT (differential `pairs(A,B) : node(A), node(B).` over a
  deletable node — both sides distinct group_ids-protected views backed by
  ONE table): oracle-verified (4-batch probe + 150-round stress, clean)
  and hand-traced through the §3 arms — exactly-once holds with NO
  special-casing: +x over pre-state {a} fires +(x,a) from arm-1
  (B@InI: a only), +(a,x) and +(x,x) from arm-2 (A@InNew: a and x);
  −a fires −(a,a)/−(a,x) from arm-1 (x∈InI pre-batch), −(x,a) from arm-2;
  same-batch +x/−a nets (a,a):−1, (x,x):+1, (x,a):+1−1 phantom-cancel,
  (a,x): no arm fires (never in old or new materialization) — all correct.
  EMITTER REQUIREMENT: tolerate side_tables[i] == side_tables[j] (no
  distinctness asserts; both arms may loop the same frontier vectors).

## 6. Fixtures and gates

New standing cases (each .dr/.main.cpp/.batches + stdout/oracle/monotone
goldens; drivers use a custom DatabaseLog printing the per-batch published
deltas — the was!=now observation surface):
- product_conds — conditions_to_bools promoted (near-clone). Pins: the two
  single-flip batches (position-keyed reads both directions), idempotent
  re-add (empty frontiers ⇒ no publish).
- product_diff — both sides @differential. Pins: retract-one-side,
  same-batch +a/−b phantom, same-batch double flip (T1 discriminator),
  OQ3 annihilation, full drain to empty.
- product_mixed — monotone × differential. Pins: monotone side has a +
  arm only; its Seal enrollment via the boundary frontier; diff-side −
  arm scanning the monotone side.
conditions_to_bools itself: promoted to compiling corpus (4-mode), struck
from the CLAUDE.md gap list; runall.sh expected-diagnostic list UNCHANGED
(it never listed conditions_to_bools; the new cases are normal cases).
Suite: 151 → 155 (the 4 new cases), SUITE: PASS, all pre-existing goldens
byte-identical (comm both directions vs the zero-red baseline). ctest 3/3.
Corpus sweep: conditions_to_bools moves from the failing-4 to compiling;
the other 3 feature-gap files unchanged. Post-landing checks: dump
product_conds IR — grep kProductInput (expect ZERO hits on the
differential path), diff the real -dot-out against §1, and run the oracle
triad + a --stress spot check. Fixture cardinalities stay small (arms are
O(|frontier| × |other side|) full scans — fine for a cross product, but
keep goldens readable).

## 7. Session bootstrap (fresh-session checklist — works on any machine)

State at handoff (2026-07-13): branch differential-product; NOTHING of
Stage 5 is implemented — this ledger is the reviewed design prototype
only. §1–§4 were verified by a three-agent fleet (§5 records its
verdicts: T1 position-keyed reads, T2 drain lift, the InductionGroupId
fence, oracle empirics, self-product). A SECOND, artifact-level
adversarial review of §3/§4 was launched and DIED on an API error without
returning — it was never completed and must be RE-RUN before code.

- Read (in order): this file top to bottom; checkpoint-e-notes.md §1 (the
  whole-program pseudocode §2 diffs against) + its landing record;
  StackSafeNegation.md §5.1/§5.4; the probes README
  (stage5-probes/); plan.md Stage 5 (:802-811). Code anchors:
  lib/ControlFlow/Build/Stratum.cpp (CrossoverEmission + EmitCrossover +
  EmitJoinFire + the scheduling fixpoint + BuildStratumPhases),
  lib/ControlFlow/Build/Build.cpp (:774 cut, :804 boundary append, :996
  pre-pass, FillDataModel), Product.cpp (eager path — must stay
  untouched), include/drlojekyll/Runtime/Table.h.
- Method (mandated; the F17/F18/T1 precedent): re-derive the
  whole-program pseudocode FROM THE CODE and check §2 against it; treat
  §1–§4 as a seed to re-verify; re-run the adversarial review of the
  concrete artifacts; write the desired post-implementation IR for
  conditions_to_bools AND prod_self concretely; hand-trace the same-batch
  double-flip and the self-product mixed batch with claim gates in force;
  only then implement §4, fixtures per §6, docs per §5's debt list.
- Gates: full suite vs the zero-red baseline (151 now; 155 after — comm
  both directions, pre-existing goldens byte-identical); ctest 3/3;
  data/ corpus 4-mode sweep (conditions_to_bools flips to compiling);
  stdout goldens for new cases blessed ONLY from 4-mode byte-agreement +
  oracle agreement post-landing; oracle/monotone goldens may be authored
  from oracle truth first.
- Environment: export PATH="/Users/pag/Code/.brew/bin:$PATH" (adjust per
  machine) before test runs; suite = DR=build/debug/bin/drlojekyll
  tests/OptDiff/runall.sh <workroot> [jobs] [filter]; macOS bash 3.2 (no
  declare -A); fixture files ASCII-only; the oracle binary builds as
  drlojekyll-oracle next to drlojekyll (runall.sh auto-builds it).
- End state: implementation + fixtures + docs committed on
  differential-product; a landing record appended HERE with deviations
  for ratification; FINDINGS.md updated if the session finds anything.
