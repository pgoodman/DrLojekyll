# Demand-keyed-instances / implicit-asynchrony epoch — design ledger

Status: OPEN 2026-07-18 on branch `keyed-instances` off main 60821adf
(the demand-seeds epoch merged + §17/§18/§18.5 on main via 36e72e35;
60821adf is the project-wide copyright sweep on top — benign, included
by design). Charter: PerfRoadmap §18 — the flat guarded copy (landed
D4) and the keyed nested instance (the ratified D3 R-A frozen-pair
paper) are TWO LOWERINGS of one object p^α (DemandSeeds.md §2.3 note
1); this epoch builds the second lowering on the first's frontier,
plus the owner's implicit-asynchrony direction (§2.3 note 3). Path as
diffs per §18.2 (owner re-ranks at epoch start): D0 seed re-verify
(E-46+), (F) determinism DO-FIRST, D1 instance design+judge, D2
instance emission, D3 multi-adornment lift, D4 seams (gated on the
termination argument). This file is the epoch's working ledger in the
DemandSeeds.md mold; the landing record goes to PerfRoadmap §19 at
close.

## 0. Epoch-start baseline (2026-07-18, branch tip == main 60821adf)

- debug + release builds green (incremental).
- ctest 3/3 (Runtime 0.05s tail; total 59.63s).
- FULL SUITE: PASS (168 cases), zero churn — the epoch-start baseline.
- Baseline binaries snapshotted to session scratchpad baseline-bin/
  (debug + release at 60821adf) — all A/B and spikes key off these
  frozen bits, per the standing operational note.
- Q5 spot: deferred to the first emission diff's SAME-SESSION
  INTERLEAVED ABABAB (the run-11 disposition: cross-session absolutes
  are load-confounded; no absolute spot taken at open).

### The (F) repro, quantified at open (before any fix)

The §18.1 LOUD carried issue reproduces immediately and is WORSE than
the ledger recorded:

- demand_tc_witness.dr under `-demand`, ONE debug binary (60821adf),
  20 runs: TEN distinct datalog.h hashes. The characteristic diff:
  sibling eager-drain loops (vec38/vec40 drains) SWAP ORDER, and vec
  numbering (vec37/vec39) swaps with them.
- CONTRADICTING DemandSeeds.md §2.4 / PerfRoadmap §18.1 ("the -ir-out
  dump is byte-stable"): the -ir-out ControlFlow IR dump is ALSO
  nondeterministic — 3 distinct hashes across 6 runs. The IR diff
  pins the layer: INDUCTION VECTOR-ID ASSIGNMENT ORDER permutes
  ($induction_in / $induction_pivots swap ids, e.g. 33/37 vs 35/37;
  the ^flow proc's parameter order swaps with them). So the
  pointer-keyed iteration is at or above the ControlFlow build layer
  (induction-vector allocation), NOT in codegen — and the fix must
  make the .ir gate valid too, not only the .h gate.
- Flag-off compiles remain byte-stable (re-confirmed).
- Repro artifacts: session scratchpad fdet/ (m1..m20 emissions,
  hdr.diff, ir1..ir6.ir).

Errata numbering: the .ir-stability contradiction is an errata
candidate pending the D0 consolidator's adjudication (E-46+ numbering
assigned there, §1 below).

## 0.6 Owner decisions (2026-07-18, RATIFIED at the design checkpoint
## — "ratified as recommended, incl. (c) for the sort key")

1. CUT: T1 rename → T2 dumps → (F) fix (parallelizable with T1/T2;
   interim scripted hash-sweep gate until T2/T3 exist) → T3 demand-ON
   IR+header golden sidecars land AS (F)'s acceptance → D1
   design+judge → D2 emission → D3 → D4 design-only (emission gated
   on the termination judge + a measured-profitable seam witness).
   Pass-harness P1 non-blocking after (F); P2-P5 next-epoch.
2. D1 WITNESS: PICK-A — a NEW non-recursive directed witness carries
   the nested lowering (demand_tc_witness is RECURSIVE; R-A is
   acyclic-frozen-first). Flat stays the DEFAULT lowering; nested
   behind a knob this epoch; the annotation records eligibility.
3. EQUIVALENCE GATE: same-witness dual-lowering oracle runs (one .dr
   + one .batches under both knobs vs the SAME oracle golden); both
   lowerings stay alive permanently.
4. D4: design+judge in-scope; emission gated (R4-style termination
   judge + measured-profitable witness) else re-seeds. E-50-corrected
   DR-IR home (existing kVecAppend/kVecDrain + new epoch-carried
   VecRole, B-10 precedent).
5. (F): locus (i) — sort the Induction.cpp:520 merge_sets ITERATION;
   sort key = OPTION (c): a stable per-view CREATION-SEQUENCE ID
   minted at Create (total by construction; also serves the BB-dump
   ^kind.id naming and all future ordering needs); the :359
   std::set<VIEW*> site fixed in the SAME landing; pre-fix snapshot
   of all 168 .ir+.h; shifts outside the known-12 + predicted
   multi-arm list need explanations before bless; the symmetric-
   recursion counterexample becomes a committed determinism witness
   (gate 1b).
6. D3-stage flags ratified as recommended: suppression keyed off a
   FABRICATED-MESSAGE SET decoupled from the forcing registry (the
   bb leak); mixed all-free-consumer + demanded programs CLEAN-REJECT
   in the D3 slice; REJECT-18 lift scoped in D3 design;
   demand_multi_adorn_1 disposition deferred to D3 design;
   SUBGRAPH_INSTANTIATE fully DR-lowered (LowerGroupUpdate mold, no
   hand-coded web); full mint-loop census day one; the mid-stream
   monotone-edge-add divergence FENCED at compile for the D2 slice.

## 0.5 Owner directives at epoch open (2026-07-18) — IR observability

Four directives, adopted as epoch work items (tooling diffs, all
flag-off-neutral, sequenced before/with the (F) fix so the dumps
serve as determinism discriminators):

1. RENAME lib/DR/ → lib/DeltaRel/, mirroring lib/DataFlow and
   lib/ControlFlow (mechanical, byte-identity-gated; DEFERRED until
   the D0 fleet completes — the fleet is actively reading lib/DR
   paths).
2. ID-ORDERED TEXTUAL DUMP of the delta-relational IR (today it has
   ZERO observability surface — the only fprintf in lib/DR is
   validator aborts). Shape: linearized schedule order, per-op
   kind/sign/position/claim-context attributes + effect sets +
   membership predicates, DRVecs with types and def/use, band
   boundaries. The dump is deterministic BY CONSTRUCTION (id-ordered)
   and becomes a third byte-identity gate surface beside .ir/.h.
   Flag: -dr-out is taken (amalgamation); working name -deltarel-out.
3. END-TO-END IR REVIEW DISCIPLINE (standing, this epoch onward):
   every emission diff's review reads the emitted DataFlow dump,
   ControlFlow .ir, and DeltaRel dump for the touched witnesses —
   not just goldens/stdout.
4. NON-DOT TEXTUAL DATAFLOW DUMP for model consumption: a
   "basic-block-with-arguments" form where views tail-call their
   users (owner's sketch; fits the push-based execution model —
   MERGE is the block-args join point, JOIN pivots are pivot
   parameters). Representation design delegated to the session;
   draft in KeyedInstances.artifacts/ir-dump-formats.md before
   implementation.
5. IR-LEVEL GOLDEN MASTERS (owner, same session): golden-master
   tests should sometimes focus PURELY on IR, not just end-to-end
   behavior. The dumps become first-class golden classes: per-case
   SIDECAR-OPT-IN (the .batches/.drflags pattern) — a case carrying
   e.g. `<name>.df.golden` / `<name>.deltarel.golden` gets its dump
   byte-compared by the harness, blessed only via the standing
   `--bless` discipline. IR goldens are MODE-SENSITIVE (the .ir
   differs across the 4 optimization modes), so a sidecar pins the
   mode(s) it covers (opt-only default). Curated directed witnesses
   (fixpoint_stress_1, reconverge_1, demand_tc_witness, the
   aggregate corpus) are the natural first carriers; demand-ON IR
   goldens become valid the moment (F) lands — and ARE the restored
   gate.

6. THE CROSS-IR PASS HARNESS (owner, same session): every
   optimization gatable from the command line (NEVER getenv
   scaffolding — house law), one GENERIC system across IR levels,
   LLVM-inspired: a pass registry with namespaced names (df.*/cf.*;
   DeltaRel stages are REQUIRED observation points, not skippable
   passes), -opt-disable/-opt-only (the 4 golden modes become exact
   aliases), a global cross-level -opt-bisect-limit counter,
   DebugCounter-style -opt-counter for individual transformation
   instances, -print-after/-print-changed wired to the three textual
   dumps, -opt-stats, and a later -passes pipeline spec enabling
   ordering speculation/fuzz (oracle-refereed). The same machinery
   carries the pass-point IR goldens (directive 5's sidecars gain an
   after=<pass> axis). Design DRAFT:
   KeyedInstances.artifacts/pass-harness-design.md (judge before
   code; P1 registry/bisect → P2 print-after → P3 counters/stats →
   P4 passes-spec/fuzz → P5 reducer residue).

## 1. Seed re-verification record (§18/§18.5 vs HEAD; 2026-07-18)

Fleet: 5 opus derivation lanes (demand pipeline / fabrication+
injector / DR-IR GROUP_UPDATE family / R-A store paper vs runtime /
determinism hunt), seed-unread, deriving pseudocode from code; 5
per-lane adversarial verifiers (seed-read); 2 sonnet mechanical
audits (§18.5 anchor drift; harness/corpus facts); xhigh
consolidator. 13 agents, ~1.04M tokens, 355 tool uses. Reports:
session scratchpad fleet-d0/ (consolidated.md is the record). THE
PRECEDENT HELD A NINTH TIME — FOUR REAL-DEFECTS.

### Seed errata (E-46.. continuing the house numbering)

- E-46 (REAL-DEFECT, §18.5(A) sites taxonomy): the seed describes TWO
  guard-site kinds (recursive read carrying α → kPushDown; base rule
  → kBaseAtom); the enum has THREE (Demand.cpp:128-132). A DIRECT
  full-width recursive p read (consumer TUPLE/INSERT) is
  kReadAtTuple (classified :649), NOT kPushDown (the read reached
  THROUGH a body JOIN, consumer = the JOIN, :704). Load-bearing:
  step-7 rewiring branches on kind (:962 kReadAtTuple gets no
  restoring TUPLE; else MintRestoringTuple :974). Misdirects D1's
  per-site annotation.
- E-47 (REAL-DEFECT, ledger contradiction): seed asserts ".ir
  stable" (PerfRoadmap:3053/:2885; DemandSeeds §2.4). MEASURED:
  -demand -ir-out demand_tc_witness = 3 distinct hashes / 8 runs.
  The fix scope is .ir AND .h; the acceptance gate must diff
  -ir-out, not just the header.
- E-48 (REAL-DEFECT, wrong root-cause locus): seed blames
  "address-dependent iteration in the -demand/DR path". Actual root
  cause is the SHARED induction machinery on EVERY path:
  lib/DataFlow/Induction.cpp:108 std::unordered_map<VIEW*,MergeSet>
  merge_sets, iterated :520 in pointer-hash order → AddUse :536 →
  related_merges == cyclic_views (:535) → QueryView::InductiveSet()
  (Query.cpp:1271-1276) → ControlFlow Build/Induction.cpp:673
  VectorFor → next_id (Procedure.cpp:163). VIEW* varies run-to-run
  (plain new, DefUse.h:890); buckets permute → induction-vector ids
  and sibling-region emission permute. An implementer grepping the
  -demand/DR path would never find it.
- E-49 (REAL-DEFECT, "flag-off deterministic" is FALSE): MEASURED
  flag-off: cf14_1.dr = 3 distinct / 12 runs; cond_in_induction.dr =
  11 distinct / 12 runs — the identical $induction_pivots ↔
  $induction_in swap. demand_tc_witness flag-off is stable only by
  allocation luck (8/8). This is a PRE-EXISTING latent bug, not a
  demand regression; acceptance adds flag-off -ir-out stability
  checks on those two cases.
- E-50 (REAL-DEFECT, §18.5(E) seam-effects precedent): the seed
  points seam vecs at "the v3-spec §2 reserved-sub-domain pattern" —
  but that pattern is a new EffKind FAMILY (statecell:fold/emit/old,
  v3-spec.md:123-126), which a seam does NOT need: a seam's
  append/drain IS the existing kVecAppend/kVecDrain (DR.h:74-76);
  the cross-batch carried role is a new VecRole (DR.h:50-63) in
  is_epoch_carried_role (DR.cpp:3504-3519) + loop_carried under
  V-LOOP — the live epoch-carried queue mechanism (right precedent:
  B-10). Following the cited precedent would reserve a needless
  EffKind.
- E-51 (STALE-ANCHOR roll-up, copyright-sweep drift): every §18.5(A)/
  injector anchor is stale; every mechanism claim correct at the
  corrected line. MarkDemandFabricated CALL = Demand.cpp:1053 (seed
  ":159" is the DEFINITION in Parse/Demand.cpp:159);
  FabricateDemandMessage = Parse/Demand.cpp:163, FabricateDemandLocal
  = :206; producer="DEMAND-GUARD" = Demand.cpp:159;
  QueryDemandForcing = Query.h:950; demand_forcings wired
  Build.cpp:1281; injector match :465-474 (belt :467-469);
  BuildQueryForceProcedureFromRegistry :383; IsDemandMessage
  suppression Database.cpp:1435/:3087. No renamed/removed symbols.
- E-52 (NUANCE, D1-load-bearing): the §18.5(C) annotation route leans
  on producer="DEMAND-GUARD" — but producer is #ifndef NDEBUG
  (Query.h:522-526), a DEBUG-only string absent in release. The seed
  itself says "becomes a real attribute"; D1 must promote it to a
  release-surviving per-guard-site mark (demand_forcings is
  per-query, not per-guard-site) recording the TWO DIFFERENT demand
  sides (d_reader guard :960 vs raw_seed query guard :1002).
- E-53 (NUANCE): §18.1's "D3-F6 hole closed by construction" covers
  only the RECOGNIZER half (genuinely dissolved). F6's second half —
  the store must be REACHED and the flat guard JOIN excised /
  not-emitted-beside (sole-writer, d3 §5.3 bet-B) — is D1/D2
  emission work, carried correctly in §18.2/§18.5(C); read §18.1
  and §18.5(C) together.

### The (F) verdict (consolidated, adjudicated with repro)

Root cause SINGLE and confirmed: the Induction.cpp:108 pointer-keyed
merge_sets map (E-48 chain above). Fix shape: after the population
loop (:520-537), Sort each distinct related_merges WeakUseList once
via Sort(Pred) (DefUse.h:206) on a STABLE structural key with a
first-column-id tie-break for a total order — one source fix corrects
both the vector-id axis and the sibling-region-emission axis, and
NEWLY STABILIZES cf14_1 + cond_in_induction flag-off. (Implementation
note: the consolidator proposes QueryViewImpl::Sort()==Hash()
(View.cpp:120-122) as the key — review must confirm the key is
run-stable and collision-tie-broken to a TOTAL order; view id is the
fallback key if Hash proves unsuitable.) Secondary record-only
suspect: Induction.cpp:359 injection-site std::set<VIEW*> mint order
(not firing on this corpus). ACCEPTANCE GATES: demand-ON .ir AND .h
byte-identity restored for demand_tc_witness; flag-off -ir-out
stability on cf14_1 + cond_in_induction; NOTE the fix touches SHARED
machinery — flag-off emissions may legitimately change shape once
(one canonical order), so the byte-identity-with-structural-gate
policy applies vs the pre-fix snapshot, and stdout goldens must be
zero-churn.

MEASURED FLAG-OFF INSTABILITY FLOOR (2026-07-18, frozen baseline
binary, corpus-wide -ir-out hash sweep, 6 runs/case; detection is
probabilistic so this is a FLOOR): 11 cases tripped —
cond_in_induction (6 variants/6 runs), kcfa_tiny (6), kcfa_tiny_merged
(6), cf14_2 (3), cond_in_induction_deep (3), product_ind,
transitive_closure_multiple_clause_bodies, transitive_closure2,
transitive_closure3, transitive_closure5, two_inductions (2 each);
union with the fleet's independent runs adds cf14_1 (3 distinct/12
there) → 12 KNOWN-UNSTABLE, every one a multi-induction/multi-
merge-set program, zero non-recursive cases — consistent with the
E-48 root cause. 147 cases stable 6/6; 10 no-compile = the expected
diagnostics. (A first sweep that flagged all 158 was a measurement
bug — zsh non-word-splitting produced a phantom empty hash line; the
CLAUDE.md ${=var} gotcha in a new costume. Recorded as a cautionary
note: stability sweeps must count from a FILE of hashes, one per
line.) These 12 are the (F) fix's expected one-time-shift +
newly-stabilized set; any case NOT in this list that shifts shape
under the fix needs an explanation before bless.

## 2. Implementation record (one diff at a time)

### T1 — lib/DR → lib/DeltaRel LANDED (2026-07-18, 17a24e66)

Mechanical rename per §0.5.1: DeltaRel.{h,cpp}, target DeltaRel,
cross-target include dirs both directions, CLAUDE.md living paths.
GATES: byte-identity on 6 stable witnesses vs the frozen baseline
binary; FULL SUITE PASS (168); ctest 3/3; both presets green.

### (F) — THE DETERMINISM FIX (record written pre-commit, at owner
### review)

THE SINGLE-LOCUS THEORY WAS WRONG — the artifact's one-hunk fix did
not survive its first gate run. The landing is SIX sites, each found
by the reproduce → trace → fix → re-measure loop (a manual
prefiguration of the pass-harness §3 workflow; every temporary trace
deleted before commit):

1. lib/DataFlow/View.cpp HashInit — THE ROOT: std::hash<const char*>
   hashed the KindName POINTER; string literals move with the binary
   image's per-run ASLR slide, so EVERY "structural" view hash was
   run-salted. Replaced with FNV-1a over the string CONTENT. (New
   erratum E-54: f-determinism-argument.md §1.1 and its adversarial
   verifier both asserted HashInit pointer-free citing
   View.cpp:405-415 — the std::hash<const char*> at :401-:406 was
   never read. Found empirically by the gate loop, not by review.)
2. lib/DataFlow/Induction.cpp — the det_seq stamp at
   IdentifyInductions entry + OrderViewsDeterministically
   (Sort() → first-col-id → det_seq, TOTAL) driving BOTH the :520
   merge_sets labeling loop AND the :359 injection-site loop
   (ratified option (a): sorted, not asserted).
3. lib/DataFlow/Join.cpp Depth() — iterated out_to_in
   (unordered_map<COL*,...>) in pointer order; on cycles the memoized
   cycle-cut makes depth VALUES visit-order-dependent (observed:
   join depth 3 vs 19 on one binary). Now iterates the join's
   columns list.
4. lib/DataFlow/Link.cpp FinalizeDepths — the reset skipped is_dead
   views (ForEachView filters them), letting stale mid-optimization
   depths leak into cycle-cutting estimates. Reset now covers every
   DefList including dead views.
5. lib/DataFlow/Optimize.cpp CSE — std::sort(candidates) BY RAW
   POINTER decided which structurally-equal view SURVIVES a merge
   (rare GRAPH-shape variance, observed live: kcfa_tiny with
   different %col ids run-to-run); the to_replace comparator was
   non-total under an unstable sort; FillViews' depth sort untied.
   All three now det_seq-total (CSE re-stamps at entry; the stamp is
   inductively deterministic).
6. lib/ControlFlow/Program.h — INDUCTION's five QueryView-keyed
   unordered_maps (view_to_add/swap/output_vec,
   output/fixpoint_cycles) iterated at emission → OrderedViewMap
   (std::map under OrderQueryViews on the new public
   QueryView::DeterministicOrder()). CAUTION recorded in-code:
   Node<> operator< / UniqueId() / wrapper Hash() are ALL
   impl-pointer-derived — a plain std::map<QueryView,...> is
   pointer-ordered too.

DEVIATION from the ratified §0.6.5 wording: det_seq is STAMPED
(ForEachView order, at CSE entries + IdentifyInductions entry), not
minted at Create — same totality/stability guarantee, zero ctor
plumbing; now also load-bearing intra-optimization.

GATES (exact tree under review): demand-ON 20-run byte-stability
(.h + .ir) on demand_tc_witness = 1 distinct; the 13 known-unstable
cases 12-run stable; WHOLE-CORPUS 8-run -ir-out sweep = 0 unstable
(the fix overshot the demand-ON goal — the ENTIRE corpus is now
emission-deterministic); FULL SUITE PASS, zero stdout churn; ctest
3/3; snapshot diff vs pre-fix = 16 shifted (.ir/.h SHAPE only): the
12 known-unstable + demand_tc_witness (their snapshots were arbitrary
draws) + THREE explained one-time canonicalization renumberings
(select_5 — in the artifact's MAY-shift list; deadflowelimination_5 +
elim-cond-cycle-simple — same induction-vector renumbering signature,
verified by diff); Q5 progsize@128 release SAME-SESSION INTERLEAVED
ABABAB: A 149.2ms vs B 150.4ms (+0.8%, noise; round-1 cold outlier
discarded and recorded); release build green; no Runtime file touched
(counter-seam re-verify not applicable). FINDINGS.md: the F20
IR-sweep note's open reproducibility question marked RESOLVED (no new
entry — gate-caught, nothing escaped to a golden).

GATE 1b: tests/OptDiff/cases/symrec_tie_1 — the critique's
symmetric-recursion counterexample (two structurally symmetric
recursive arms tie on hash AND first-col-id; only det_seq orders
them) as a PERMANENT corpus determinism witness: 30-run byte-stable,
all-4-mode golden blessed from hand-verified closure truth. SUITE
168 → 169.

### T2b.0 — THE BAND-KEY HARDENING LANDED (2026-07-19, owner-approved
### at the Fable-review brief)

One functional hunk in lib/DeltaRel/DeltaRel.cpp: op_table_id
pointer tie-break → `t ? uintptr_t(t->id) + 1u : 0u` — the REVIEW-
STRENGTHENED form (the ratified `t->id : 0u` minimum relied on the
ids-≥-3 invariant, FALSE under -first-id unsigned wraparound; the +1
shift into the 64-bit key space makes the null sentinel disjoint BY
CONSTRUCTION). Plus the comment sweep the review demanded: the
retracted "band key IS the emission walk order" claim removed from
the V-BAND-HAZARD + Kahn comments AND the V-OLD-EQUIV(order) abort
string (which now directs debugging at the linearizer's edge
derivation, not Stratum.cpp emission); -deltarel-out cited as the
unlanded T2b deliverable, not present tense. Review: 4 finders + 5
verifiers, 9 candidates → 3 CONFIRMED (all fixed pre-commit), 1
refuted (uintptr_t "leftover" — now load-bearing for the 64-bit
disjointness).

GATES (all green, re-run after the review fixes): E-62 tripwire
re-grep (zero body_ops/output_ops readers); 676-row corpus A/B (169
cases × 4 modes, exit + .h + .ir hashes) BYTE-IDENTICAL vs the
frozen 63c8443c baseline binary — the emission-neutrality prediction
held exactly; data/ 36-file A/B clean; FULL SUITE PASS (169); ctest
3/3; Q5 progsize@128 release SAME-SESSION INTERLEAVED ABABAB A
{142.5,140.6,141.5} vs B {139.4,141.3,140.8} ms (−0.5% median,
noise; warmup discarded); no Runtime file touched. The deltarel
golden surface is now pointer-free — T2b may land.

## 3. Mid-epoch checkpoint (2026-07-18, tip 5d642d9b — T1 + (F)
## landed): the as-landed surfaces as PSEUDOCODE, the remaining path
## as DIFFS against them (SINGLE-PASS record by this session — the
## next session re-verifies per the E-1..E-54 house precedent;
## continue errata at E-55)

    (A) THE DETERMINISM SUBSTRATE (new since (F); every future diff
    must respect it):

      QueryViewImpl::det_seq (lib/DataFlow/Query.h, next to `hash`):
        unsigned, ~0u = unstamped. STAMPED in ForEachView (per-kind
        DefList insertion) order at TWO entries: CSE() head
        (Optimize.cpp) and IdentifyInductions head (Induction.cpp).
        Public accessor QueryView::DeterministicOrder()
        (include/.../DataFlow/Query.h + Query.cpp; asserts stamped).
      OrderViewsDeterministically (lib/DataFlow/Induction.cpp, above
        IdentifyInductions): Sort()==Hash() → first-col-id → det_seq;
        TOTAL. Drives the :~590 merge-set labeling loop (sorted key
        vector; group_id assignment + related_merges/cyclic_views
        population both canonical) and the injection-sites loop.
      OrderQueryViews + OrderedViewMap<V> (lib/ControlFlow/Program.h
        :~46): std::map<QueryView, V, det_seq-order>. INDUCTION's
        view_to_add_vec / view_to_swap_vec / view_to_output_vec /
        output_cycles / fixpoint_cycles use it.
      HashInit (lib/DataFlow/View.cpp): FNV-1a over KindName CONTENT.
      FinalizeDepths (lib/DataFlow/Link.cpp): resets EVERY DefList
        incl. is_dead views, then recomputes in fixed order.
      QueryJoinImpl::Depth (lib/DataFlow/Join.cpp): walks columns
        list, never out_to_in bucket order.
      CSE (lib/DataFlow/Optimize.cpp): stamps det_seq at entry;
        candidates sorted by det_seq; to_replace comparator total;
        FillViews stable_sort.
      HOUSE RULES (in-code comments carry them): (1) NEVER iterate a
        pointer-keyed container into emission-visible state; (2)
        Node<> operator< / UniqueId() / wrapper Hash() are POINTER-
        derived — std::map<QueryView,...>/std::set<QueryView> are
        pointer-ordered; use OrderedViewMap or sort by
        DeterministicOrder(); (3) symrec_tie_1 is the standing
        tripwire; (4) until T3 lands, the regression instrument is
        the scripted 8-run -ir-out corpus sweep (ledger §1/§2).

    (B) THE DUMP SURFACES T2 BUILDS AGAINST (as-landed reality):
      -dot-out: DataFlow → GraphViz (lib/DataFlow/Format.cpp; public
        OutputStream operator<< in include/.../DataFlow/Format.h).
      -ir-out: ControlFlow textual dump (include/.../ControlFlow/
        Format.h family), gIRStream wired in bin/drlojekyll/Main.cpp
        :~271-282.
      lib/DeltaRel/DeltaRel.{h,cpp}: NO dump surface (only validator
        fprintf+abort). Post-rename target name DeltaRel; mutual
        internal includes with ControlFlow stand (§2.4 lineage).

    (C) T2 AS A DIFF — the two dumps (ir-dump-formats.md is the
    binding draft; reconcile its §2 against fleet-d0/lane-drir.md):
      + -df-out <PATH> (Main.cpp flag + DataFlow emitter): the BB
        tail-call form per ir-dump-formats.md §1 — blocks in a
        deterministic id order (the det_seq stamp is the natural
        ^kind.<id>; decide det_seq vs a FinalizeColumnIDs-style
        renumber at design), MERGE = block-args join point, JOIN =
        ports, producer tags shown (D1's annotation becomes visible).
      + -deltarel-out <PATH> (flag + emitter in lib/DeltaRel/): per
        ir-dump-formats.md §2 — vecs in id order, ops in the CHECKED
        LINEARIZATION order (reuse the linearizer's list, never
        re-sort), effect sets + membership predicates + bands;
        emitted from validate-exit so a dump always describes a
        validated graph.
      Both flag-off-invisible to the suite; dumps deterministic BY
      the (A) substrate; predictions: zero golden churn, zero
      emission change.
    (D) T3 AS A DIFF — IR-golden sidecars (the harness hook):
      + per-case opt-in sidecars (the .batches/.drflags idiom) pinned
        (mode, level[, after=<pass> once P1 exists]) per ledger
        §0.5.5; runall.sh/diffrun.sh grow the compare arms; blessing
        via --bless only. First carriers: demand_tc_witness (.h +
        .ir demand-ON — THE permanent (F) gate), symrec_tie_1 (.ir),
        then the curated directed witnesses. Suite count unchanged;
        goldens/ grows.
    (E) P1 AS A DIFF — the pass harness (pass-harness-design.md §2/§4
      P1 slice): PassPolicy from CLI, registry df.*/cf.* names,
      -opt-disable/-opt-only with the legacy flags as EXACT aliases,
      global -opt-bisect-limit. Byte-identity by construction at
      default config; the 4 golden modes re-expressed as aliases must
      be byte-identical.
    (F') D1 AS A DIFF — unchanged from epoch-diffs.md §D1 (with
      E-46/E-52/E-53 folded): the release-surviving per-guard-site
      annotation (three GuardSite kinds; two demand sides), the
      PICK-A non-recursive witness (§0.6.2), BuildSubgraphOps in the
      BuildGroupUpdateOps mold, census from the query-side annotation
      count, the dual-lowering equivalence gate (§0.6.3), the
      acyclic-DEMAND fence, demand-retract = death. Design + judge
      BEFORE code; witness-deltarel-target.md is the draft IR to
      re-verify against the landed dump format.

    ORDER (ratified §0.6.1): T2 → T3 → P1 (non-blocking) → D1 →
    D2 → D3 → D4-design. The next session starts at T2.

## 4. §3 re-verification record (2026-07-18, tip b577735e; the T2
## checkpoint fleet — 3 seed-unread derivation lanes (2 opus + 1
## sonnet) + 3 adversarial verifiers + xhigh consolidator, 7 agents
## ~647k tokens; record COMMITTED as
## KeyedInstances.artifacts/ckpt-fleet-consolidated.md — the raw lane/
## verify reports stayed in the session scratchpad and are
## loss-tolerant, the consolidated record adjudicates them)

Baseline re-confirmed this session: SUITE PASS (169), binaries frozen
to scratchpad baseline-bin/ at b577735e.

THE §3 SEED SUBSTANTIALLY HELD — the first seed since E-1 whose core
pseudocode blocks ((A) substrate, (B)/(C) dump surfaces) verified
CLEAN (see the consolidated appendix's claim table). The re-check
still paid: six adjudicated errata, dominated by artifact/lane/doc
defects rather than seed defects — two of them (E-58/E-59) would have
misdirected the T3 implementer.

### Errata E-55..E-60 (final, consolidator-adjudicated)

- E-55 (LANE overcount, not seed): det_seq has exactly TWO stamp
  sites (Induction.cpp:144, Optimize.cpp:287); the derivation lane's
  "three" counted IdentifyInductions' re-entrant self-call (:455,
  re-runs :144) as a third site. §3(A)'s "TWO entries" is CORRECT.
- E-56 (stale-anchor roll-up, cosmetic): color field Query.h:529 not
  268 (lane); OrderQueryViews/OrderedViewMap at Program.h:48/:55
  (seed's "~46"); FinalizeDepths(2602) runs BEFORE
  FinalizeColumnIDs(2603).
- E-57 (design tension, RESOLVED as a ruling): -df-out block ids —
  raw UniqueId() is POINTER-derived (Node.h:31; what -dot-out names
  nodes with today, which is why the DOT dump is nondeterministic in
  node naming) and is REJECTED; det_seq (or a fresh renumber, which
  by construction reproduces det_seq's numbering) is the key. There
  is NO FinalizeViewIDs pass and no integer view-id field — det_seq
  is the ONLY pointer-free total per-view id space.
- E-58 (SEED imprecision, load-bearing for T3): §3(D) "runall.sh/
  diffrun.sh grow the compare arms" — WRONG on the diffrun.sh half.
  The .batches precedent lives ENTIRELY in runall.sh's --one worker
  (run_oracle, :180-248); diffrun.sh is a pure 4-mode primitive whose
  only sidecar awareness is the .drflags flag-APPEND (:57-59). The
  T3 arm goes in --one ONLY.
- E-59 (cross-artifact, same surface): ir-dump-formats.md §2.5's
  "diffrun.sh additionally runs the compiler with -df-out" is the
  outlier contradicting the code precedent — corrected before T3
  (t2-dump-spec.md §3.1 is the binding placement).
- E-60 (stale doc, gate-adjacent): CLAUDE.md said "168 corner-case
  programs"; live count is 169 (symrec_tie_1, §2 gate 1b). Fixed in
  the same session.

Non-errata nuances recorded in the consolidated report: HashInit
also folds the two deletion flags + column count (pointer-free claim
stands); the DOT dump's column PORTS are deterministic (col.Id()),
only node NAMES are pointer-derived.

RESIDUAL-RISK CENSUS (adjudicated): NO unprotected pointer-order
iteration reaches emission-visible state beyond what (F) covers. One
future-facing hazard: ProgramImpl::Analyze() is DEAD (sole call site
commented out, Build/Build.cpp:1399) and contains a pointer-bucket
next_id++ DATARECORD id assignment (Analyze.cpp:1195) — any revival
of record-caching must sort unique_table_sources first.

DECISION-FEEDING FACTS (consolidated §4): the -deltarel-out hook is
inside BuildStratumPhases only (context.dr_flow, Build.h:198; the
DRFlowGraph never surfaces on Program) — validate-exit =
Stratum.cpp:2097 (post-linearize) or :2166 (post-stash, past the
ingest cross-check); pinned_order (DeltaRel.h:658) survives to both.
The DeltaRel inventory (15 DROpKinds, 10 Preds, 10 EffKinds, 14
VecRoles, no id fields — identity is vector index) is recorded
verbatim in consolidated §1.B and reconciled against the
ir-dump-formats §2 draft in t2-dump-spec.md.

### The T2/T3/P1 binding spec + the desired-states critique round

docs/proposals/KeyedInstances.artifacts/t2-dump-spec.md — the
formulated diffs with pre-registered predictions (zero golden churn,
suite stays 169, Q5 neutral for all three), carrying the owner
decisions with recommendations. v1 was adversarially critiqued by the
desired-states fleet (4 opus writers hand-writing the exact dump
texts for transitive_closure / symrec_tie_1 / demand-ON
demand_tc_witness / average_weight + 4 per-artifact critics + an
xhigh spec critic; 9 agents ~1.03M tokens; artifacts + critiques
COMMITTED under KeyedInstances.artifacts/t2-desired-states/, the
PICK-A witness draft as
KeyedInstances.artifacts/demand_neighborhood_witness.dr — it enters
tests/OptDiff/cases/ only at D2, with batches + oracle goldens, so
the suite discovery never sees an unblessed case). Verdicts: 2
artifacts
SOUND-WITH-AMENDMENTS, 2 UNSOUND (fixable — wrong rename-rule
application; wrong ordering rules); spec SOUND-WITH-AMENDMENTS. v2
folds every amendment; the artifacts get one revision pass against
the ratified spec before commit.

THE CRITIQUE ROUND'S REAL CATCHES (design-time, pre-code — the house
method paying again):

- (C-1, CONFIRMED at source by the orchestrator): the DeltaRel
  band-key comparator's op_table_id tie-break is
  reinterpret_cast<uintptr_t>(TABLE*) (DeltaRel.cpp:3387-3394,
  key_less :3461) — pinned_order's within-band order (8 commit
  sweeps on average_weight; the seals) is POINTER-ordered, the exact
  (F) anti-pattern, latent-only because pinned_order's sole consumers
  are the validators (grep :3804-3981; emission never reads it — the
  corpus byte-stability is real). The dump would surface it as bytes,
  and dep-edge orientation for same-key pairs is in principle
  allocation-dependent (a latent flaky-validator class, F20's
  sibling). T2b.0 (spec §2.0) hardens the tie-break to the table id
  BEFORE the dump lands; predicted zero emission change, gated by
  full-suite byte-identity.
- (spec-critic 4.1): v1's producer= line would have made .df the
  ONLY config-VARIANT golden surface (producer is #ifndef
  NDEBUG-only; every sibling surface is config-invariant) — a
  release-preset run would spuriously fail every producer-tagged
  golden. v2 drops producer from the default dump.
- (demand-critic F0): v1's `; back-edge` rule (user det_seq <= def
  det_seq) is UNSOUND — over-fires on non-cycle lower-id targets AND
  misses det_seq-forward cycle edges; the writer's own analysis of it
  was backwards (both writer and rule wrong, caught by recompute).
  v2 replaces it with reachability-exact `; cycle` (decision a2).
- (spec-critic 1.1): det_seq density at the drain rests on every
  pass between the last stamp (Build.cpp:2597) and the drain being
  VIEW-NEUTRAL (six passes enumerated + verified); the emitter now
  asserts DENSITY, not just stamped-ness.
- Plus the pinned grammar rulings (block params = own finalized
  columns; JOIN .inK/pivot/out grammar; INSERT form; MERGE-only
  sorted callers; class= accessor semantics — recursion does not
  imply differential; no _MissingVar; exhaustive deps; first-class
  join/branch sections; IRGOLD-* verdict tokens; pinned bless
  paths).

## 5. Whole-program checkpoint (2026-07-18, end of the T2-formulation
## session): the ARCHITECTURE AS PSEUDOCODE (fleet-VERIFIED, §4 —
## unlike §3 this is not single-pass; the §4 fleet + consolidator
## adjudicated every block against code) and the PATH FORWARD AS
## DIFFS against it. A new session starts HERE; re-verify per the
## house precedent (continue errata at E-61) but note §4's appendix
## already carries the per-claim verdict table.

    (A) THE COMPILE PIPELINE, whole-program (anchors verified §4):

      main(argv):                                # bin/drlojekyll/Main.cpp
        parse flags -> gCxxOutDir(:52), gDOTStream/gDRStream/
          gIRStream(:54-56), gOptimizeDataFlow/gOptimizeControlFlow
          (:46-52, set :333-346), gDemand, gFirstId
        module = Parse(...)                      # lib/Lex, lib/Parse
        gDRStream? << module (:125-129)          # parse-level amalgam
        CompileModule:
          query = Query::Build(module, log, gOptimizeDataFlow,
                               gDemand)          # lib/DataFlow/Build.cpp
          program = Program::Build(query, log, gFirstId,
                               gOptimizeControlFlow)  # lib/ControlFlow
          gIRStream?  << program (:74-77)        # the .ir dump
          emit C++ into gCxxOutDir               # lib/CodeGen
          gDOTStream? << query (:106-109)        # LAST: TableId() is
                                                 # annotated by
                                                 # Program::Build

      Query::Build tail (Build.cpp, the load-bearing order):
        BuildClause* -> ConnectInsertsToSelects(2564)
        ApplyDemandTransform(module, log, demand_mode) (2576)
                                                 # -demand magic-sets;
                                                 # total no-op flag-off
        optimize? impl->Optimize(log) (2585)     # Simplify -> Canon
                                                 # fixpoint -> CSE
        LinkViews(2595)                          # may ADD views
        IdentifyInductions(2597)                 # det_seq stamp; sorted
                                                 # merge-set labeling
        FinalizeDepths(2602); FinalizeColumnIDs(2603)  # col ids final
        TrackDifferentialUpdates(2604); TrackConstAfterInit(2608)
        BuildEquivalenceSets(2626); Stratify(2627)
        # 2598-2627 are ALL view-neutral (verified §4) -> det_seq
        # stays dense 0..N-1 over live views through Program::Build

      THE DETERMINISM SUBSTRATE (the (F) landing, §2-§3(A), §4 CLEAN):
        det_seq: Query.h:472, stamped ForEachView-order at EXACTLY TWO
          sites (CSE head Optimize.cpp:285-287; IdentifyInductions
          head Induction.cpp:142-144; re-entrant self-call re-runs the
          same site — E-55). Accessor DeterministicOrder() asserts
          stamped. ForEachView SKIPS dead views (Query.h:1176-1214) —
          the density mechanism.
        OrderViewsDeterministically (Induction.cpp:112-126): Sort()==
          Hash() (HashInit = FNV-1a over KindName + 2 deletion flags +
          col count, View.cpp:417-427) -> first-col-id (pre-finalize,
          pass-order contract) -> det_seq. TOTAL. Drives the sorted
          merge-set labeling loop (:578-604) + injection-sites loop
          (:404-440).
        OrderedViewMap (ControlFlow Program.h:48/:55) under
          DeterministicOrder — the five INDUCTION maps (:1832-1853).
        CSE: candidates/to_replace/FillViews all det_seq-tied
          (Optimize.cpp:320-379,409-422). Join::Depth walks columns
          (Join.cpp:74-110); FinalizeDepths resets dead views too
          (Link.cpp:437-472).
        RESIDUAL (§4 census; E-63-scoped): nothing pointer-ordered
          reaches emission ON THE CURRENT CORPUS — one dormant gap:
          Build.cpp:501 iterates const_to_vc (unordered_map) into
          ALL-CONSTS tuple column layout unsorted; fires only on an
          all-constants clause body with ≥2 distinct constant
          columns, nil today. Future hardening: col->id sort first.
        View identity beyond det_seq is the IMPL POINTER (UniqueId,
          Node.h:31) — why -dot-out node names are nondeterministic.

      THE DELTAREL LAYER (lib/DeltaRel, inside Program::Build's
      BuildStratumPhases, Stratum.cpp:2049):
        BuildDRInventory(2081) -> DeriveDRStrata(2088) ->
        ValidateDRInventory(2095) -> ValidateDROps(2096, the census)
        -> LinearizeAndValidateDRFlow(2097):
             key_of(op) = {lead, stratum, band, op_table_id, sign,
                           ctor}                 # DeltaRel.cpp:3432
             op_table_id = reinterpret_cast<uintptr_t>(TABLE*)
                           (:3387-3394)          # <-- THE C-1 LATENT:
                           # pointer tie-break; consumers (E-62) =
                           # the validators (:3817-3981) + the
                           # NEVER-READ body_ops/output_ops
                           # substrate loop (:3804-3815) — none
                           # emission-reaching; NULL t maps to 0
                           # today (E-64: the hardened form MUST be
                           # `t ? t->id : 0u`)
             pinned_order = Kahn linearization,  # the checked
                           ready-set tie-broken  # linearization
                           by key_less (:3702-3739; E-62 — not a
                           flat stable_sort)
        -> V-PRED-XCHECK / INGEST Site 5 (2099ff)
        -> context.dr_flow = move (2166)         # the ONLY handle that
                                                 # survives; Program
                                                 # never exposes it
        -> LowerDRFlow / LowerDRRounds / LowerCommitSweeps /
           LowerGroupUpdate (~2298ff)            # emission reads mint
                                                 # order, NOT pinned
        Inventory (verified §4/§1.B of the fleet record): 15 DROpKinds,
        10 Preds, 10 EffKinds, 14 VecRoles, no id fields (identity =
        vector index), DRFlowGraph ordered vectors + unordered
        lookup-only maps.

      THE HARNESS (tests/OptDiff, verified §4):
        diffrun.sh = PURE 4-mode primitive (opt/nodf/nocf/none);
          .drflags = flag-append only. runall.sh --one = ALL golden
          policy: 11 always-diagnostic names (:231), kvindex_1
          mode-split, run_oracle guarded by .batches; summary grep
          FAIL|DIVERGE|EXPECT-ERROR|MISSING (:284); --bless = sole
          goldens/ write path. 169 cases; 158 stdout + 52 oracle + 52
          monotone goldens.

    (B) THE PATH FORWARD AS DIFFS against (A) (spec:
    t2-dump-spec.md v2 — the BINDING formulation; per-diff gates and
    pre-registered predictions live there):

      T2a  + gDFStream/-df-out: BB-with-arguments DataFlow dump,
           blocks ^kind.<det_seq> ascending, DENSITY-asserted;
           drained beside -dot-out. Grammar pinned (spec §1.3):
           own-finalized-column params, dst=src edge maps,
           reachability-exact `; cycle`, JOIN .inK/pivot/out in
           accessor order, INSERT into-form, MERGE-only sorted
           callers, class= from deletion-capability, NO producer= in
           default output (config-invariance). Zero-churn predicted.
      T2b.0 ~ harden op_table_id pointer tie-break -> table id
           (spec §2.0; the C-1 catch). Emission-neutral predicted
           (validator-only consumers, grep-verified); full-suite
           byte-identity gate; any churn = an unknown consumer, STOP.
      T2b  + SetDeltaRelDumpStream/-deltarel-out at the post-stash
           validate-exit (Stratum.cpp:2166, reads *context.dr_flow):
           vecs (index order) + joins/branches + ops (pinned_order)
           + rounds (substrate-bannered) + exhaustive deps + census;
           Pred/EffKind spellings verbatim. Zero-churn predicted.
      T3   + cases/<name>.irgold sidecar (surface+mode lines) +
           goldens/<name>.<surface>.<mode>.golden, strict cmp; arm in
           runall.sh --one ONLY (E-58/E-59), reusing flags_of;
           IRGOLD-FAIL/-MISSING/-DIVERGE tokens; pinned nested
           workdir/bless paths. First carriers: demand_tc_witness
           (h+ir+df+deltarel, demand-ON = the permanent (F) gate),
           symrec_tie_1 (ir+df). Suite stays 169; the scripted 8-run
           sweep retires after bless.
      P1   + PassPolicy at the two Build call sites; legacy flags as
           exact aliases; byte-identity at default config. Slot:
           after T3, before D1 design (reseed-if-tight).
      D1   design+judge (per §3(F')/epoch-diffs §D1, E-46/E-52/E-53
           folded): release-surviving per-guard-site annotation,
           BuildSubgraphOps in the BuildGroupUpdateOps mold, census
           from the query-side count, dual-lowering equivalence gate,
           acyclic-DEMAND fence, demand-retract=death. Witness:
           PICK-A demand_neighborhood_witness.dr (artifact-committed,
           compiles both ways at b577735e). Then D2 emission ->
           D3 multi-adornment -> D4-design (§0.6.1).

    (C) STATUS at session end: T1 + (F) LANDED (§2). The T2/T3/P1
    formulation + desired-state artifacts are COMMITTED but the four
    t2-desired-states artifacts are DRAFT-PENDING-REVISION (their
    critiques enumerate the amendments; 2 verdicts UNSOUND are
    fixable rename/ordering-rule errors). OWNER DECISIONS PENDING:
    spec §5 (a), (a2), (a3), (b), (b2), (c), (d), (e) — brought to
    the owner at session end, NOT yet ratified. NO emission-changing
    code has been written this session (docs/artifacts only). The
    implementation order on ratification: revise the four artifacts
    -> T2a -> T2b.0 -> T2b -> T3 -> P1, one diff at a time, standing
    gates between, Fable review before each emission commit.

## 6. §5 re-verification record (2026-07-18, tip 63c8443c; the tenth
## fleet run — 4 seed-unread derivation lanes (3 opus + 1 sonnet
## harness) + 4 seed-read adversarial verifiers + 4 diff critics
## (density / t2b0 / irgold / predictions, run concurrently) + xhigh
## consolidator; 13 agents ~1.07M tokens, 379 tool uses; record
## COMMITTED as KeyedInstances.artifacts/s5-fleet-consolidated.md —
## raw lane/verify/critic reports stayed in the session scratchpad)

Baseline re-confirmed this session before the fleet: both presets
green (incremental), SUITE PASS (169), binaries frozen to session
scratchpad baseline-bin/ at 63c8443c.

THE §5 SEED SUBSTANTIALLY HELD (second consecutive clean core): all
four lanes CLEAN; §5(A)/(B) code-accurate except the errata below.
The (F) and T2b.0 emission-neutrality arguments SURVIVE. The critics
paid: two HIGH-severity implementation traps (E-61, E-64) caught
pre-code.

### Errata E-61..E-66 (consolidator-adjudicated; one attribution
### corrected by the orchestrator's own grep)

- E-61 (SPEC defect, LOAD-BEARING, misdirects T2a): the ForEachView
  kind order in t2-dump-spec §1.2 v2 was transposed — the code
  pushes NEGATIONS before COMPARES (Query.h:1196-1204/:1248-1258).
  ORCHESTRATOR ADJUDICATION: the consolidator attributed this to
  "seed §5(A) AND spec §1.2", but §5 never spells the kind list
  (grep) and all four t2-desired-*.md artifacts already have the
  CORRECT order — the defect was spec-only. det_seq numbering is
  unaffected (stamped in true code order), so the density witness
  can NOT catch a golden hand-blessed from transposed prose: any
  .df golden diffs against a code-derived traversal, never prose.
  Fixed in spec v3.
- E-62 (SEED precision, conclusion holds): "pinned_order consumers
  VALIDATORS ONLY (:3804-3981)" over-narrowed — :3804-3815 is the
  never-read body_ops/output_ops substrate-fill loop (validators
  proper :3817-3981); and pinned_order is a Kahn linearization
  tie-broken by key_less, not a flat stable-sort. Emission-
  neutrality HOLDS (zero body_ops/output_ops readers repo-wide) —
  with the standing TRIPWIRE: re-grep those readers at T2b.0
  implementation time and at any R2+ lowering; a reader voids the
  argument. §5 fixed in place.
- E-63 (SEED scope over-statement, latent): "nothing pointer-ordered
  reaches emission" is unconditionally false — Build.cpp:501
  iterates const_to_vc (unordered_map<QueryColumnImpl*,VarColumn*>,
  decl :83) into ALL-CONSTS tuple column layout unsorted. Fires only
  on an all-constants clause body with ≥2 distinct constant columns
  — nil on the current corpus, (F) byte-stability unthreatened.
  Future hardening (not T2-path): col->id sort first. §5 fixed.
- E-64 (SEED elision, LOAD-BEARING, misdirects T2b.0 — CRASHES): the
  T2b.0 one-liner "harden op_table_id to the table id" elides the
  NULL path — t is the first non-null of six DROp table fields else
  op.fire_table, itself NULL for kNegateGate(eager)/kSeedFold/
  kChainFold/kPivotAssemble; a bare t->id null-derefs (exit 139) on
  nearly every corpus case. MANDATORY form `t ? t->id : 0u`;
  collision-free because real table ids ≥ 3. Spec v3 §2.0 carries
  it as a hard precondition of (b2). AS-LANDED STRENGTHENING (the
  T2b.0 Fable review): the ≥3 invariant is FALSE under -first-id
  unsigned wraparound (table id 0 mintable, silent sentinel
  collision) — landed as `t ? uintptr_t(t->id) + 1u : 0u`, sentinel
  disjoint by construction in the 64-bit key space.
- E-65 (SPEC defect, LOAD-BEARING, misdirects T3): v2's
  `$WORKROOT/$NAME/irgold/` layout claimed to "match run_oracle" —
  run_oracle actually writes `$NAME.oracle`/`$NAME.monotone`
  SIBLING dirs (runall.sh:190,210); v2's shape was a third layout
  matching nothing. v3 pins `$NAME.irgold/` (true symmetry) + a
  `[ -d ]` bless guard.
- E-66 (cosmetic roll-up): flags_of body is runall.sh:122-134 (not
  -140); DataTable::Id() defined at ControlFlow/Program.cpp:877
  (lane cite slip); Node.h = include/drlojekyll/Util/Node.h,
  OrderedViewMap = lib/ControlFlow/Program.h (path qualifications);
  injection-sites loop tail ends :450 not :440.
  REJECTED candidate recorded: verify-pipeline's ":246 not :231"
  correction of the diagnostic-names line was FALSE (:231 is the
  11-name pattern; :246 is diffrun.sh's default dispatch arm) — a
  verifier self-error, caught by the consolidator.

Non-erratum drift recorded: the final monotonicity validator EMITS
`V-OLD-EQUIV(order)` (DeltaRel.cpp:3982) under a comment titled
V-ORDER-CONSISTENT — any dumped/censused validator token is
harvested from the ValidatorFail string literal, never comments.

### Critique adjudication → spec v3

All four critic reports adjudicated; the nine resulting amendments
are FOLDED into t2-dump-spec.md v3 this session: (1) E-61 kind-order
fix; (2) the density assert replaced by a two-pass SEEN-BITSET
bijection witness (max==count-1 is not sufficient — duplicate-with-
gap passes it; the accessor assert compiles out under NDEBUG; N==0
underflow); (3) §2.0 E-64 null-guard + precise verdict-neutrality
rationale + id≥3 invariant + the stale :3971 comment fix; (4) E-62
consumer restatement + tripwire; (5) validator-token literal harvest
+ deltarel config-invariance audit before any deltarel bless (the
(a3) discipline extended); (6) drain via the :2167 flow ref in the
:2167-:2199 window (before the no-phase early return) + PRE-guard
null sink; (7) E-65 layout + sweep-retirement rewrite (retained as
substrate-change acceptance gate) + diagnostic-set guard + sidecar/
golden same-commit atomicity; (8) flags_of cite; (9) P1 prediction
widened to all-4-modes byte-identity.

### Decision-feeding delta

ALL owner recommendations (a), (a2), (a3), (b), (b2), (c), (d), (e)
STAND — two sharpenings, no reversals: (b2) gains the E-64
null-guard as a hard precondition; (c) gains the E-65 layout, the
diagnostic-set guard, and the retained-sweep ruling; (a3)'s
config-invariance audit extends to the deltarel surface before any
deltarel bless. The four DRAFT-PENDING-REVISION artifacts were
grep-checked for the E-61 transposition: all four already carry the
correct order (their revision pass re-confirms).

### RATIFICATION (2026-07-18, same session, owner at the §6 brief)
### [→ §7 is the post-landing whole-program checkpoint; §5's DeltaRel
### op_table_id lines describe the PRE-T2b.0 pointer form]

ALL EIGHT decisions RATIFIED as recommended/v3-amended: (a) det_seq
block ids + the seen-bitset bijection witness; (a2) reachability-
exact `; cycle`; (a3) no producer= in the default dump, the
config-invariance audit extended to deltarel; (b) post-Program
-df-out drain + the lib-owned pre-guarded -deltarel-out sink at the
:2167 validate-exit; (b2) T2b.0 before T2b with the E-64 null-guard
precondition; (c) the E-65-corrected `$NAME.irgold/` sidecar
machinery, retained sweep, diagnostic-set guard; (d) P1 after T3;
(e) PICK-A witness + artifact revision before T2a. The path is
UNBLOCKED: revise the four artifacts → T2a → T2b.0 → T2b → T3 → P1,
one diff at a time, standing gates between, Fable review brought to
the owner before each emission commit.

## 7. Whole-program checkpoint (2026-07-19, end of the T2b.0 session;
## tip fa2bc7c5 + this ledger commit): the ARCHITECTURE AS PSEUDOCODE
## with T2b.0 AS-LANDED, and the PATH FORWARD AS DIFFS. SINGLE-PASS
## record by this session (the §5 blocks it incorporates were fleet-
## verified TWICE, §4 + §6; the NEW blocks below — the linearizer
## post-T2b.0, the T2a emitter contract — are this session's writing).
## A new session starts HERE; re-verify per the house precedent
## (continue errata at E-67), leaning on §6's verdicts + the v3.1
## pins so only the DELTA needs fresh lanes.

    (A) AS-LANDED SURFACES:

      A.1 COMPILE PIPELINE + DETERMINISM SUBSTRATE: §5(A) blocks
          "main(argv)" / "Query::Build tail" / "THE DETERMINISM
          SUBSTRATE" stand VERBATIM (fleet-verified twice; E-61's
          kind-order note: ForEachView pushes selects, tuples,
          kv_indices, joins, maps, aggregates, merges, NEGATIONS,
          COMPARES, inserts — Query.h:1176-1214/:1248-1258; E-63's
          const_to_vc dormant gap recorded in place).

      A.2 THE DELTAREL LINEARIZER, post-T2b.0 (LANDED 97d02111,
          §2 record; supersedes §5's pointer-form lines):
            key_of(op) = {lead, stratum, band, op_table_id, sign,
                          ctor}                # DeltaRel.cpp:~3437
            op_table_id = t ? uintptr_t(t->id) + 1u : 0u
                          # t = first non-null of six DROp table
                          # fields else fire_table; +1 shift into the
                          # 64-bit key space -> null sentinel 0 is
                          # disjoint BY CONSTRUCTION (survives even
                          # -first-id wraparound minting table id 0
                          # — the Fable review's CONFIRMED catch);
                          # t->id == the .ir's %table:<id>
            pinned_order = Kahn topo sort over non-loop-carried dep
                          edges, ready-set tie-broken by key_less
                          (:~3702-3739)
            consumers   = validators (:~3817-3981) + the NEVER-READ
                          body_ops/output_ops substrate loop
                          (:~3804-3815); EMISSION READS NEITHER
                          (Lower* walk construction order) — the
                          E-62 tripwire: re-grep body_ops/output_ops
                          readers before trusting this at any future
                          diff
          The whole key is now pointer-free: pinned_order is fit to
          become golden bytes (T2b's precondition, satisfied).
          Gate record: 676-row corpus A/B byte-identity held.

      A.3 THE FOUR BYTE-EXACT DUMP CONTRACTS (fa2bc7c5, t2-desired-
          states/): tc + symrec_tie_1 + demand_tc_witness (-df-out,
          demand-ON for the third) and average_weight
          (-deltarel-out). Each = provenance header + THE EXACT
          INTENDED DUMP BYTES + derivation sections + LOUD residuals.
          Written/critiqued/fixed against the frozen 63c8443c binary
          + spec v3 + the v3.1 pins (p1) uniform block headers
          `<kind> ^<kind>.<det_seq> (<own finalized cols>)`, (p2)
          bare-token identity in => maps, (p3) producer-side
          => .in<K> lines present with join-owned role mapping —
          .in<K> ASSIGNMENT IS PREDICTED (joined_views UseList
          order, code-read at first bless), (p4) reads: renders
          Preds only, kInIReadFrozen under effects:.
          RESIDUALS an implementer must adjudicate (never silently):
          tc R1 INSERT det_seq ids unwitnessed (predicted from
          clause order); .in<K> assignments; deltarel op.4
          never-minted-vec note + the kInIReadFrozen/reads: tension
          note on GU/SEED_FOLD lines.

      A.4 HARNESS: §5(A) harness block stands (169 cases; 158
          stdout + 52 oracle + 52 monotone goldens; --one owns all
          golden policy; :231 diagnostic names; :284 summary grep).

    (B) THE PATH FORWARD AS DIFFS (spec v3 + v3.1 = BINDING, ALL
    EIGHT owner decisions RATIFIED 2026-07-18):

      T2a  + gDFStream/-df-out (NEXT DIFF). The emitter as
           pseudocode (make the compiler print EXACTLY A.3's three
           df artifacts):
             emit_df(query):        # lib/DataFlow/Format.cpp, new
                                    # QueryDF tag operator<<; decl in
                                    # include/.../DataFlow/Format.h
               print "dataflow"
               # PASS 1 (bijection witness, always-on fprintf+abort):
               N = count via ForEachView; seen = bitset(N)
               ForEachView v: s = v.det_seq_raw
                 if s >= N or seen[s]: ABORT (unstamped ~0u caught
                                       by range; dup by bitset)
                 seen[s] = true
               # PASS 2 (emission, SHARING the ForEachView
               # traversal -- blocks ascending det_seq by
               # construction since the stamp IS traversal order):
               ForEachView v:
                 header: <kind> ^<kind>.<det_seq> (own finalized
                         col tokens)            # pin p1
                 ATTRIBUTES: table=%table:<TableId()> (table-backed),
                         class=<differential|monotone|table-less>
                         (CanReceiveDeletions-derived), stratum=,
                         set=/depth= (induction members only)
                 body:   JOIN pivot/out lines in accessor order;
                         MERGE `; callers:` ascending det_seq;
                         INSERT into-form terminal
                 edges:  one => per user column-edge, (user det_seq,
                         port) order, dst=src only on rename (p2),
                         producer-side .in<K> lines (p3),
                         `; cycle` iff def REACHABLE from user
                         (memoized reachability, decision a2-i)
             drain: Main.cpp beside -dot-out (:106-109), AFTER
             Program::Build (TableId populated). NO producer= ever.
           GATES: byte-diff vs the three artifacts (mismatch =
           fleet-adjudicate artifact-vs-compiler, amend the wrong
           one LOUDLY); zero golden churn; suite 169; flag-off
           byte-identity (dump is off-path); Q5 ABABAB; A1 rider
           (eyeball %table:<id>); Fable review -> owner -> commit.
      T2b  + SetDeltaRelDumpStream/-deltarel-out per spec §2.1-2.3:
           pre-guarded sink, drain via the :2167 flow ref in the
           :2167-:2199 window; vecs mint order / joins+branches /
           ops in pinned_order (now deterministic) / rounds
           bannered / deps exhaustive / census; Pred+EffKind
           spellings verbatim; validator tokens from ValidatorFail
           literals; CONFIG-INVARIANCE AUDIT (no NDEBUG-gated field)
           before any bless. Contract: A.3's average_weight
           artifact. Same gate shape as T2a.
      T3   + .irgold sidecars per spec §3 (E-65 layout
           $NAME.irgold/; IRGOLD-* tokens; all-4-modes-clean cases
           only; sidecar+goldens same commit). First carriers:
           demand_tc_witness (h+ir+df+deltarel, demand-ON = the
           permanent (F) gate), symrec_tie_1 (ir+df). The 8-run
           sweep retires as ROUTINE step only; RETAINED as the
           substrate-change acceptance gate.
      P1   PassPolicy at the two Build sites; legacy flags exact
           aliases; byte-identity across ALL FOUR modes (A8).
      D1   design+judge fleet after T3 (witness-deltarel-target.md +
           tc-four-adornment-target.md targets, H1-H10 holes;
           per-guard-site release-surviving annotation, PICK-A
           witness demand_neighborhood_witness.dr, dual-lowering
           equivalence gate — the minus-before-plus band-(a)
           ordering must be PINNED by an explicit DR-IR edge before
           the equivalence gate counts). Then D2 -> D3 -> D4-design
           (§0.6.1).

    (C) STATUS at session end (2026-07-19): T1 + (F) + T2b.0 LANDED
    (§2); all eight owner decisions RATIFIED; the four dump
    contracts committed (DRAFT-PENDING-REVISION retired); spec at
    v3 + v3.1 pins. NO T2a/T2b/T3/P1 code exists. Standing gates
    unchanged; the scripted 8-run sweep is still the (F) regression
    instrument until T3 blesses. Next session: re-verify §7
    (E-67+), then T2a.
