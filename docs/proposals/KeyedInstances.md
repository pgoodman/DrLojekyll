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
